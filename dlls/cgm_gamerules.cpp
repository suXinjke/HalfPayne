#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"player.h"
#include	"client.h"
#include	"weapons.h"
#include	"cgm_gamerules.h"
#include	"custom_gamemode_config.h"
#include	<algorithm>
#include	<random>
#include	"monsters.h"

// Custom Game Mode Rules

int	gmsgEndActiv = 0;
int	gmsgEndTitle = 0;
int	gmsgEndTime  = 0;
int	gmsgEndScore = 0;
int	gmsgEndStat  = 0;

int	gmsgGLogDeact	= 0;
int	gmsgGLogMsg		= 0;
int	gmsgGLogWDeact	= 0;
int	gmsgGLogWMsg	= 0;
int gmsgTimerPause  = 0;

// CGameRules were recreated each level change and there were no built-in saving method,
// that means we'd lose config file state on each level change.
//
// This has been changed and now CGameRules state is preseved during CHANGE_LEVEL calls,
// using g_changeLevelOccured like below and see also CWorld::Precache 

extern int g_changeLevelOccured;
extern Intermission g_latestIntermission;

CCustomGameModeRules::CCustomGameModeRules( CONFIG_TYPE configType ) : config( configType )
{
	configs.push_back( &config );

	if ( !gmsgEndActiv ) {
		gmsgEndActiv = REG_USER_MSG( "EndActiv", 1 );
		gmsgEndTitle = REG_USER_MSG( "EndTitle", -1 );
		gmsgEndTime  = REG_USER_MSG( "EndTime", -1 );
		gmsgEndScore = REG_USER_MSG( "EndScore", -1 );
		gmsgEndStat  = REG_USER_MSG( "EndStat", -1 );


		gmsgGLogDeact = REG_USER_MSG( "GLogDeact", 0 );
		gmsgGLogMsg = REG_USER_MSG( "GLogMsg", -1 );
		gmsgGLogWDeact = REG_USER_MSG( "GLogWDeact", 0 );
		gmsgGLogWMsg = REG_USER_MSG( "GLogWMsg", -1 );
		gmsgTimerPause = REG_USER_MSG( "TimerPause", 1 );
	}

	// Difficulty must be initialized separately and here, becuase entities are not yet spawned,
	// and they take some of the difficulty info at spawn (like CWallHealth)

	// Monster entities also have to be fetched at this moment for ClientPrecache.
	const char *configName = CVAR_GET_STRING( "gamemode_config" );
	config.ReadFile( configName );
	RefreshSkillData();

	cheatedMessageSent = false;

	timeDelta = 0.0f;
	musicSwitchDelay = 0.0f;

	monsterSpawnPrevented = false;

	auto entityRandomSpawners = config.GetEntityRandomSpawners();
	for ( const auto &spawner : entityRandomSpawners ) {
		entityRandomSpawnerControllers.push_back( EntityRandomSpawnerController( spawner ) );
	}
}

void CCustomGameModeRules::RestartGame() {
	const std::string startMap = config.GetStartMap();
	char mapName[256];
	sprintf( mapName, "%s", startMap.c_str() );

	// Change level to [startmap] and queue the 'restart' command
	// for CCustomGameModeRules::PlayerSpawn.
	// Would be better just to execute 'cgm' command directly somehow.
	config.markedForRestart = true;
	CHANGE_LEVEL( mapName, NULL );
}

void CCustomGameModeRules::SendGameLogMessage( CBasePlayer *pPlayer, const std::string &message ) {
	MESSAGE_BEGIN( MSG_ONE, gmsgGLogMsg, NULL, pPlayer->pev );
		WRITE_STRING( message.c_str() );
	MESSAGE_END();
}

void CCustomGameModeRules::SendGameLogWorldMessage( CBasePlayer *pPlayer, const Vector &location, const std::string &message, const std::string &message2 ) {
	MESSAGE_BEGIN( MSG_ONE, gmsgGLogWMsg, NULL, pPlayer->pev );
		WRITE_COORD( location.x );
		WRITE_COORD( location.y );
		WRITE_COORD( location.z );

		// You cant send TWO strings - they overwrite each other, so split them with |
		WRITE_STRING( ( message + "|" + message2 ).c_str() );
	MESSAGE_END();
}

void CCustomGameModeRules::PlayerSpawn( CBasePlayer *pPlayer )
{
	if ( config.markedForRestart ) {
		config.markedForRestart = false;
		SERVER_COMMAND( "restart\n" );
		return;
	}

	CHalfLifeRules::PlayerSpawn( pPlayer );

	pPlayer->activeGameMode = GAME_MODE_CUSTOM;
	pPlayer->activeGameModeConfig = ALLOC_STRING( config.configName.c_str() );

	for ( const GameplayMod &mod : config.mods ) {
		mod.init( pPlayer );
	}

	// For first map
	SpawnEnemiesByConfig( STRING( gpGlobals->mapname ) );

	auto loadout = config.GetLoadout();
	pPlayer->SetEvilImpulse101( true );
	for ( std::string loadoutItem : loadout ) {

		if ( loadoutItem == "all" ) {
			pPlayer->GiveAll( true );
			pPlayer->SetEvilImpulse101( true ); // it was set false by GiveAll
		}
		else {
			if ( loadoutItem == "item_healthkit" ) {
				pPlayer->TakePainkiller();
			} else if ( loadoutItem == "item_suit" ) {
				pPlayer->pev->weapons |= ( 1 << WEAPON_SUIT );
			} else if ( loadoutItem == "item_longjump" ) {
				pPlayer->m_fLongJump = TRUE;
				g_engfuncs.pfnSetPhysicsKeyValue( pPlayer->edict( ), "slj", "1" );
			} else {
				const char *item = allowedItems[CustomGameModeConfig::GetAllowedItemIndex( loadoutItem.c_str( ) )];
				pPlayer->GiveNamedItem( item, true );
			}
		}
	}
	pPlayer->SetEvilImpulse101( false );

	if ( !config.IsGameplayModActive( GAMEPLAY_MOD_EMPTY_SLOWMOTION ) ) {
		pPlayer->TakeSlowmotionCharge( 100 );
	}

	StartPosition startPosition = config.GetStartPosition();
	if ( startPosition.defined ) {
		pPlayer->pev->origin.x = startPosition.x;
		pPlayer->pev->origin.y = startPosition.y;
		pPlayer->pev->origin.z = startPosition.z;
		if ( !std::isnan( startPosition.angle ) ) {
			pPlayer->pev->angles[1] = startPosition.angle;
		}
	}

	if ( g_latestIntermission.defined ) {
		if ( !std::isnan( g_latestIntermission.x ) ) {
			pPlayer->pev->origin.x = g_latestIntermission.x;
			pPlayer->pev->origin.y = std::isnan( g_latestIntermission.y ) ? 0 : g_latestIntermission.y;
			pPlayer->pev->origin.z = std::isnan( g_latestIntermission.z ) ? 0 : g_latestIntermission.z;
		}

		if ( !std::isnan( g_latestIntermission.angle ) ) {
			pPlayer->pev->angles[1] = g_latestIntermission.angle;
		}

		if ( g_latestIntermission.strip ) {
			pPlayer->RemoveAllItems( false );
		}

		g_latestIntermission.defined = false;
	}

	if ( config.musicPlaylist.size() > 0 ) {
		pPlayer->noMapMusic = TRUE;
	}

	// Do not let player cheat by not starting at the [startmap]
	const std::string startMap = config.GetStartMap();
	const char *actualMap = STRING( gpGlobals->mapname );
	if ( startMap != actualMap ) {
		pPlayer->cheated = true;
	}

}

void CCustomGameModeRules::OnNewlyVisitedMap() {
	CHalfLifeRules::OnNewlyVisitedMap();

	SpawnEnemiesByConfig( STRING( gpGlobals->mapname ) );
	monsterSpawnPrevented = false;
}

bool CCustomGameModeRules::ChangeLevelShouldBePrevented( const char *nextMap ) {
	return config.IsChangeLevelPrevented( nextMap );
}

BOOL CCustomGameModeRules::CanHavePlayerItem( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon )
{
	if ( !pPlayer->weaponRestricted ) {
		return CHalfLifeRules::CanHavePlayerItem( pPlayer, pWeapon );
	}

	if ( !pPlayer->HasWeapons() ) {
		return CHalfLifeRules::CanHavePlayerItem( pPlayer, pWeapon );
	}

	return FALSE;
}

void CCustomGameModeRules::PlayerThink( CBasePlayer *pPlayer )
{
	CHalfLifeRules::PlayerThink( pPlayer );
	
	timeDelta = ( gpGlobals->time - pPlayer->lastGlobalTime );

	pPlayer->lastGlobalTime = gpGlobals->time;

	if ( !pPlayer->timerPaused && !UTIL_IsPaused() && pPlayer->pev->deadflag == DEAD_NO ) {

		if ( fabs( timeDelta ) <= 0.1 ) {
			if ( pPlayer->timerBackwards ) {
				pPlayer->time -= timeDelta;
			} else {
				pPlayer->time += timeDelta;
			}
		}

		// Counting real time
		float realTimeDetla = ( g_engfuncs.pfnTime() - pPlayer->lastRealTime );

		pPlayer->lastRealTime = g_engfuncs.pfnTime();

		if ( fabs( realTimeDetla ) <= 0.1 ) {
			pPlayer->realTime += realTimeDetla;
		}
	}

	if ( pPlayer->pev->deadflag == DEAD_NO ) {
		
		// This is terribly wrong, it would be better to reset lastGlobalTime on actual change level event
		// It was made to prevent timer messup during level changes, because each level has it's own local time
		if ( fabs( timeDelta ) <= 0.1 ) {
			if ( pPlayer->slowMotionEnabled ) {
				pPlayer->secondsInSlowmotion += timeDelta;
			}
		}

		CheckForCheats( pPlayer );
	}

	if ( config.IsGameplayModActive( GAMEPLAY_MOD_PREVENT_MONSTER_SPAWN ) && !monsterSpawnPrevented ) {
		for ( int i = 0 ; i < 1024 ; i++ ) {
			edict_t *edict = g_engfuncs.pfnPEntityOfEntIndex( i );
			if ( !edict ) {
				continue;
			}

			if ( CBaseEntity *entity = CBaseEntity::Instance( edict ) ) {
				if ( entity->pev->spawnflags & SF_MONSTER_PRESERVE ) {
					continue;
				}

				if (
					FStrEq( STRING( entity->pev->classname ), "monster_alien_controller" ) ||
					FStrEq( STRING( entity->pev->classname ), "monster_alien_grunt" ) ||
					FStrEq( STRING( entity->pev->classname ), "monster_alien_slave" ) ||
					FStrEq( STRING( entity->pev->classname ), "monster_apache" ) ||
					FStrEq( STRING( entity->pev->classname ), "monster_babycrab" ) ||
					FStrEq( STRING( entity->pev->classname ), "monster_bullchicken" ) ||
					FStrEq( STRING( entity->pev->classname ), "monster_headcrab" ) ||
					FStrEq( STRING( entity->pev->classname ), "monster_houndeye" ) ||
					FStrEq( STRING( entity->pev->classname ), "monster_human_assassin" ) ||
					FStrEq( STRING( entity->pev->classname ), "monster_human_grunt" ) ||
					FStrEq( STRING( entity->pev->classname ), "monster_ichthyosaur" ) ||
					FStrEq( STRING( entity->pev->classname ), "monster_miniturret" ) ||
					FStrEq( STRING( entity->pev->classname ), "monster_sentry" ) ||
					FStrEq( STRING( entity->pev->classname ), "monster_snark" ) ||
					FStrEq( STRING( entity->pev->classname ), "monster_zombie" ) ||
					FStrEq( STRING( entity->pev->classname ), "monster_barney" ) ||
					FStrEq( STRING( entity->pev->classname ), "monster_scientist" )
				) {
					entity->pev->flags |= FL_KILLME;
				}
			}
		}

		monsterSpawnPrevented = true;
	}

	for ( auto &spawner : entityRandomSpawnerControllers ) {
		spawner.Think( pPlayer );
	}

	size_t musicPlaylistSize = config.musicPlaylist.size();
	if (
		musicPlaylistSize > 0 &&
		CVAR_GET_FLOAT( "sm_current_pos" ) == 0.0f &&
		gpGlobals->time > musicSwitchDelay
	) {
		int musicIndexToPlay = pPlayer->currentMusicPlaylistIndex + 1;
		if ( config.musicPlaylistShuffle ) {
			static std::random_device rd;
			static std::mt19937 gen( rd() );
			std::uniform_int_distribution<> dis( 0, musicPlaylistSize - 1 );
			musicIndexToPlay = dis( gen );
		}

		if ( musicIndexToPlay >= musicPlaylistSize ) {
			musicIndexToPlay = 0;
		}
		
		pPlayer->SendPlayMusicMessage( config.musicPlaylist.at( musicIndexToPlay ) );
		pPlayer->currentMusicPlaylistIndex = musicIndexToPlay;

		musicSwitchDelay = gpGlobals->time + 0.2f;
	}
}

void CCustomGameModeRules::OnKilledEntityByPlayer( CBasePlayer *pPlayer, CBaseEntity *victim, KILLED_ENTITY_TYPE killedEntity, BOOL isHeadshot, BOOL killedByExplosion, BOOL killedByCrowbar ) {

	pPlayer->kills++;

	if ( killedByExplosion ) {
		pPlayer->explosiveKills++;
	} else if ( killedByCrowbar ) {
		pPlayer->crowbarKills++;
	} else if ( isHeadshot ) {
		pPlayer->headshotKills++;
	} else if ( killedEntity == KILLED_ENTITY_GRENADE ) {
		pPlayer->projectileKills++;
	} else if ( killedEntity == KILLED_ENTITY_NIHILANTH ) {
		End( pPlayer );
	}

	HookModelIndex( victim->edict() );
}

void CCustomGameModeRules::CheckForCheats( CBasePlayer *pPlayer )
{
	if ( pPlayer->cheated && cheatedMessageSent || ended ) {
		return;
	}

	if ( pPlayer->cheated ) {
		OnCheated( pPlayer );
		return;
	}

	if ( ( pPlayer->pev->flags & FL_GODMODE && !pPlayer->godConstant ) ||
		 ( pPlayer->pev->flags & FL_NOTARGET && !pPlayer->noTargetConstant ) ||
		 ( pPlayer->pev->movetype & MOVETYPE_NOCLIP ) ||
		 pPlayer->usedCheat ) {
		pPlayer->cheated = true;
	}

}

void CCustomGameModeRules::OnCheated( CBasePlayer *pPlayer ) {
	cheatedMessageSent = true;
}

void CCustomGameModeRules::OnEnd( CBasePlayer *pPlayer ) {
	PauseTimer( pPlayer );

	const std::string configName = config.GetName();

	MESSAGE_BEGIN( MSG_ONE, gmsgGLogDeact, NULL, pPlayer->pev );
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_ONE, gmsgGLogWDeact, NULL, pPlayer->pev );
	MESSAGE_END();
	
	MESSAGE_BEGIN( MSG_ONE, gmsgEndTitle, NULL, pPlayer->pev );
		WRITE_STRING( CustomGameModeConfig::ConfigTypeToGameModeName( config.configType, true ).c_str() );
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_ONE, gmsgEndTitle, NULL, pPlayer->pev );
		WRITE_STRING(
			config.GetName().size() > 0 ?
			( config.GetName() + " - COMPLETE" ).c_str() :
			"LEVEL COMPLETE"
		);
	MESSAGE_END();

	int secondsInSlowmotion = ( int ) roundf( pPlayer->secondsInSlowmotion );
	if ( secondsInSlowmotion > 0 ) {
		MESSAGE_BEGIN( MSG_ONE, gmsgEndStat, NULL, pPlayer->pev );
			WRITE_STRING( ( "SECONDS IN SLOWMOTION|" + std::to_string( secondsInSlowmotion ) ).c_str() );
		MESSAGE_END();
	}

	if ( pPlayer->kills > 0 ) {
		MESSAGE_BEGIN( MSG_ONE, gmsgEndStat, NULL, pPlayer->pev );
			WRITE_STRING( ( "TOTAL KILLS|" + std::to_string( pPlayer->kills ) ).c_str() );
		MESSAGE_END();
	}

	if ( pPlayer->headshotKills > 0 ) {
		MESSAGE_BEGIN( MSG_ONE, gmsgEndStat, NULL, pPlayer->pev );
			WRITE_STRING( ( "HEADSHOT KILLS|" + std::to_string( pPlayer->headshotKills ) ).c_str() );
		MESSAGE_END();
	}

	if ( pPlayer->explosiveKills > 0 ) {
		MESSAGE_BEGIN( MSG_ONE, gmsgEndStat, NULL, pPlayer->pev );
			WRITE_STRING( ( "EXPLOSION KILLS|" + std::to_string( pPlayer->explosiveKills ) ).c_str() );
		MESSAGE_END();
	}

	if ( pPlayer->crowbarKills > 0 ) {
		MESSAGE_BEGIN( MSG_ONE, gmsgEndStat, NULL, pPlayer->pev );
			WRITE_STRING( ( "MELEE KILLS|" + std::to_string( pPlayer->crowbarKills ) ).c_str() );
		MESSAGE_END();
	}

	if ( pPlayer->projectileKills > 0 ) {
		MESSAGE_BEGIN( MSG_ONE, gmsgEndStat, NULL, pPlayer->pev );
			WRITE_STRING( ( "PROJECTILES DESTROYED KILLS|" + std::to_string( pPlayer->projectileKills ) ).c_str() );
		MESSAGE_END();
	}

	MESSAGE_BEGIN( MSG_ONE, gmsgEndActiv, NULL, pPlayer->pev );
		WRITE_BYTE( pPlayer->cheated );
	MESSAGE_END();

	if ( !pPlayer->cheated ) {
		config.record.Save( pPlayer );
	}
}

void CCustomGameModeRules::OnHookedModelIndex( CBasePlayer *pPlayer, edict_t *activator, int modelIndex, const std::string &targetName )
{
	CHalfLifeRules::OnHookedModelIndex( pPlayer, activator, modelIndex, targetName );

	// Does end_trigger section contain such index?
	if ( config.MarkModelIndex( CONFIG_FILE_SECTION_END_TRIGGER, std::string( STRING( gpGlobals->mapname ) ), modelIndex, targetName ) ) {
		End( pPlayer );
	}

	Intermission potentialIntermission = config.GetIntermission( STRING( gpGlobals->mapname ), modelIndex, targetName );
	if ( potentialIntermission.defined ) {
		g_latestIntermission = potentialIntermission;
		CHANGE_LEVEL( ( char * ) g_latestIntermission.toMap.c_str(), NULL );
		// after that, g_latestIntermission becomes undefined in PlayerSpawn function
	}

	if (
		config.IsGameplayModActive( GAMEPLAY_MOD_SNARK_PENGUINS ) && 
		config.IsGameplayModActive( GAMEPLAY_MOD_SNARK_FRIENDLY_TO_PLAYER ) && 
		config.IsGameplayModActive( GAMEPLAY_MOD_SNARK_FRIENDLY_TO_ALLIES ) &&
		std::string( STRING( gpGlobals->mapname ) ) == "c1a0e"
	) {
		CBaseEntity *list[512];
		int amount = UTIL_MonstersInSphere( list, 512, Vector( 0, 0, 0 ), 8192.0f );

		if ( targetName == "se_motor_sound" ) {
			
			for ( int i = 0 ; i < amount ; i++ ) {
				if ( CSqueakGrenade *penguin = dynamic_cast<CSqueakGrenade *>( list[i] ) ) {
					if ( !penguin->isPanic ) {
						penguin->pev->velocity = Vector( 0, 0, 0 );
						penguin->pev->angles = UTIL_VecToAngles( activator->v.origin - penguin->pev->origin );
						penguin->pev->angles[0] = 0;
						penguin->pev->sequence = 1;
						penguin->isStill = true;
					}
				}
			}
		} else if ( targetName == "speaker_ohno" ) {

			for ( int i = 0 ; i < amount ; i++ ) {
				if ( CSqueakGrenade *penguin = dynamic_cast<CSqueakGrenade *>( list[i] ) ) {
					penguin->pev->sequence = 2;
					penguin->isStill = false;
					penguin->isPanic = true;
				}
			}
		}
	}
}

void CCustomGameModeRules::SpawnEnemiesByConfig( const char *mapName )
{
	auto entitySpawns = config.GetEntitySpawnsForMapOnce( std::string( mapName ) );
	for ( auto entitySpawn : entitySpawns ) {
		CBaseEntity *entity = CBaseEntity::Create(
			allowedEntities[CustomGameModeConfig::GetAllowedEntityIndex( entitySpawn.entityName.c_str() )],
			Vector( entitySpawn.x, entitySpawn.y, entitySpawn.z ),
			Vector( 0, entitySpawn.angle, 0 )
		);

		entity->pev->spawnflags |= SF_MONSTER_PRESERVE;
		if ( entitySpawn.targetName.size() > 0 ) {
			entity->pev->targetname = ALLOC_STRING( entitySpawn.targetName.c_str() ); // memory leak
		}
	}
}

void CCustomGameModeRules::OnChangeLevel() {
	CHalfLifeRules::OnChangeLevel();
	musicSwitchDelay = 0.0f;

	for ( auto &spawner : entityRandomSpawnerControllers ) {
		spawner.ResetSpawnPeriod();
	}
}

void CCustomGameModeRules::Precache() {
	CHalfLifeRules::Precache();

	for ( std::string spawn : config.entitiesToPrecache ) {
		UTIL_PrecacheOther( spawn.c_str() );
	}
}

void CCustomGameModeRules::PauseTimer( CBasePlayer *pPlayer )
{
	if ( pPlayer->timerPaused ) {
		return;
	}

	pPlayer->timerPaused = true;

	MESSAGE_BEGIN( MSG_ONE, gmsgTimerPause, NULL, pPlayer->pev );
		WRITE_BYTE( true );
	MESSAGE_END();
}

void CCustomGameModeRules::ResumeTimer( CBasePlayer *pPlayer )
{
	if ( !pPlayer->timerPaused ) {
		return;
	}

	pPlayer->timerPaused = false;

	MESSAGE_BEGIN( MSG_ONE, gmsgTimerPause, NULL, pPlayer->pev );
		WRITE_BYTE( false );
	MESSAGE_END();
}

// Hardcoded values so it won't depend on console variables
void CCustomGameModeRules::RefreshSkillData() 
{
	gSkillData.barneyHealth = 35;
	gSkillData.slaveDmgClawrake = 25.0f;

	gSkillData.leechHealth = 2.0f;
	gSkillData.leechDmgBite = 2.0f;

	gSkillData.scientistHealth = 20.0f;

	gSkillData.snarkHealth = 2.0f;
	gSkillData.snarkDmgBite = 10.0f;
	gSkillData.snarkDmgPop = 5.0f;

	gSkillData.plrDmgCrowbar = 10.0f;
	gSkillData.plrDmg9MM = 8.0f;
	gSkillData.plrDmg357 = 40.0f;
	gSkillData.plrDmgMP5 = 5.0f;
	gSkillData.plrDmgM203Grenade = 100.0f;
	gSkillData.plrDmgBuckshot = 5.0f;
	gSkillData.plrDmgCrossbowClient = 10.0f;
	gSkillData.plrDmgCrossbowMonster = 50.0f;
	gSkillData.plrDmgRPG = 100.0f;
	gSkillData.plrDmgGauss = 20.0f;
	gSkillData.plrDmgEgonNarrow = 6.0f;
	gSkillData.plrDmgEgonWide = 14.0f;
	gSkillData.plrDmgHornet = 7;
	gSkillData.plrDmgHandGrenade = 100.0f;
	gSkillData.plrDmgSatchel = 150.0f;
	gSkillData.plrDmgTripmine = 150.0f;

	gSkillData.healthkitCapacity = 15.0f; // doesn't matter - it's painkiller
	gSkillData.scientistHeal = 25.0f;

	if ( config.IsGameplayModActive( GAMEPLAY_MOD_HEADSHOTS ) ) {
		gSkillData.monHead = 10.0f;
	} else {
		gSkillData.monHead = 3.0f;
	}
	gSkillData.monChest = 1.0f;
	gSkillData.monStomach = 1.0f;
	gSkillData.monLeg = 1.0f;
	gSkillData.monArm = 1.0f;

	gSkillData.plrHead = 3.0f;
	gSkillData.plrChest = 1.0f;
	gSkillData.plrStomach = 1.0f;
	gSkillData.plrLeg = 1.0f;
	gSkillData.plrArm = 1.0f;

	if ( config.IsGameplayModActive( GAMEPLAY_MOD_EASY ) ) {
		
		gSkillData.iSkillLevel = 1;

		gSkillData.agruntHealth = 60.0f;
		gSkillData.agruntDmgPunch = 10.0f;

		gSkillData.apacheHealth = 150.0f;
	
		gSkillData.bigmommaHealthFactor = 1.0f;
		gSkillData.bigmommaDmgSlash = 50.0f;
		gSkillData.bigmommaDmgBlast = 100.0f;
		gSkillData.bigmommaRadiusBlast = 250.0f;

		gSkillData.bullsquidHealth = 40.0f;
		gSkillData.bullsquidDmgBite = 15.0f;
		gSkillData.bullsquidDmgWhip = 25.0f;
		gSkillData.bullsquidDmgSpit = 10.0f;

		gSkillData.gargantuaHealth = 800.0f;
		gSkillData.gargantuaDmgSlash = 10.0f;
		gSkillData.gargantuaDmgFire = 3.0f;
		gSkillData.gargantuaDmgStomp = 50.0f;

		gSkillData.hassassinHealth = 30.0f;

		gSkillData.headcrabHealth = 10.0f;
		gSkillData.headcrabDmgBite = 5.0f;

		gSkillData.hgruntHealth = 50.0f;
		gSkillData.hgruntDmgKick = 5.0f;
		gSkillData.hgruntShotgunPellets = 3.0f;
		gSkillData.hgruntGrenadeSpeed = 400.0f;

		gSkillData.houndeyeHealth = 20.0f;
		gSkillData.houndeyeDmgBlast = 10.0f;

		gSkillData.slaveHealth = 30.0f;
		gSkillData.slaveDmgClaw = 8.0f;
		gSkillData.slaveDmgZap = 10.0f;

		gSkillData.ichthyosaurHealth = 200.0f;
		gSkillData.ichthyosaurDmgShake = 20.0f;

		gSkillData.controllerHealth = 60.0f;
		gSkillData.controllerDmgZap = 15.0f;
		gSkillData.controllerSpeedBall = 650.0f;
		gSkillData.controllerDmgBall = 3.0f;

		gSkillData.nihilanthHealth = 800.0f;
		gSkillData.nihilanthZap = 30.0f;
	
		gSkillData.zombieHealth = 50.0f;
		gSkillData.zombieDmgOneSlash = 10.0f;
		gSkillData.zombieDmgBothSlash = 25.0f;

		gSkillData.turretHealth = 50.0f;
		gSkillData.miniturretHealth = 40.0f;
		gSkillData.sentryHealth = 40.0f;

		gSkillData.monDmg12MM = 8.0f;
		gSkillData.monDmgMP5 = 3.0f;
		gSkillData.monDmg9MM = 5.0f;
		
		gSkillData.monDmgHornet = 4.0f;

		gSkillData.suitchargerCapacity = 75.0f;
		gSkillData.batteryCapacity = 15.0f;
		gSkillData.healthchargerCapacity = 50.0f;
		
	} else if ( config.IsGameplayModActive( GAMEPLAY_MOD_HARD ) ) {
		gSkillData.iSkillLevel = 3;

		gSkillData.agruntHealth = 120.0f;
		gSkillData.agruntDmgPunch = 20.0f;

		gSkillData.apacheHealth = 400.0f;
	
		gSkillData.bigmommaHealthFactor = 2.0f;
		gSkillData.bigmommaDmgSlash = 70.0f;
		gSkillData.bigmommaDmgBlast = 160.0f;
		gSkillData.bigmommaRadiusBlast = 275.0f;

		gSkillData.bullsquidHealth = 40.0f;
		gSkillData.bullsquidDmgBite = 25.0f;
		gSkillData.bullsquidDmgWhip = 35.0f;
		gSkillData.bullsquidDmgSpit = 10.0f;

		gSkillData.gargantuaHealth = 1000.0f;
		gSkillData.gargantuaDmgSlash = 30.0f;
		gSkillData.gargantuaDmgFire = 5.0f;
		gSkillData.gargantuaDmgStomp = 100.0f;

		gSkillData.hassassinHealth = 50.0f;

		gSkillData.headcrabHealth = 20.0f;
		gSkillData.headcrabDmgBite = 10.0f;

		gSkillData.hgruntHealth = 80.0f;
		gSkillData.hgruntDmgKick = 10.0f;
		gSkillData.hgruntShotgunPellets = 6.0f;
		gSkillData.hgruntGrenadeSpeed = 800.0f;

		gSkillData.houndeyeHealth = 30.0f;
		gSkillData.houndeyeDmgBlast = 15.0f;

		gSkillData.slaveHealth = 60.0f;
		gSkillData.slaveDmgClaw = 10.0f;
		gSkillData.slaveDmgZap = 15.0f;

		gSkillData.ichthyosaurHealth = 400.0f;
		gSkillData.ichthyosaurDmgShake = 50.0f;

		gSkillData.controllerHealth = 100.0f;
		gSkillData.controllerDmgZap = 35.0f;
		gSkillData.controllerSpeedBall = 1000.0f;
		gSkillData.controllerDmgBall = 5.0f;

		gSkillData.nihilanthHealth = 1000.0f;
		gSkillData.nihilanthZap = 50.0f;
	
		gSkillData.zombieHealth = 100.0f;
		gSkillData.zombieDmgOneSlash = 20.0f;
		gSkillData.zombieDmgBothSlash = 40.0f;

		gSkillData.turretHealth = 60.0f;
		gSkillData.miniturretHealth = 50.0f;
		gSkillData.sentryHealth = 50.0f;

		gSkillData.monDmg12MM = 10.0f;
		gSkillData.monDmgMP5 = 5.0f;
		gSkillData.monDmg9MM = 8.0f;
		
		gSkillData.monDmgHornet = 8.0f;

		gSkillData.suitchargerCapacity = 35.0f;
		gSkillData.batteryCapacity = 10.0f;
		gSkillData.healthchargerCapacity = 25.0f;
	} else {
		gSkillData.iSkillLevel = 2;

		gSkillData.agruntHealth = 90.0f;
		gSkillData.agruntDmgPunch = 20.0f;

		gSkillData.apacheHealth = 250.0f;

		gSkillData.bigmommaHealthFactor = 1.5f;
		gSkillData.bigmommaDmgSlash = 60.0f;
		gSkillData.bigmommaDmgBlast = 120.0f;
		gSkillData.bigmommaRadiusBlast = 250.0f;

		gSkillData.bullsquidHealth = 40.0f;
		gSkillData.bullsquidDmgBite = 25.0f;
		gSkillData.bullsquidDmgWhip = 35.0f;
		gSkillData.bullsquidDmgSpit = 10.0f;

		gSkillData.gargantuaHealth = 800.0f;
		gSkillData.gargantuaDmgSlash = 30.0f;
		gSkillData.gargantuaDmgFire = 5.0f;
		gSkillData.gargantuaDmgStomp = 100.0f;

		gSkillData.hassassinHealth = 50.0f;

		gSkillData.headcrabHealth = 10.0f;
		gSkillData.headcrabDmgBite = 10.0f;

		gSkillData.hgruntHealth = 50.0f;
		gSkillData.hgruntDmgKick = 10.0f;
		gSkillData.hgruntShotgunPellets = 5.0f;
		gSkillData.hgruntGrenadeSpeed = 600.0f;

		gSkillData.houndeyeHealth = 20.0f;
		gSkillData.houndeyeDmgBlast = 15.0f;

		gSkillData.slaveHealth = 30.0f;
		gSkillData.slaveDmgClaw = 10.0f;
		gSkillData.slaveDmgZap = 10.0f;

		gSkillData.ichthyosaurHealth = 200.0f;
		gSkillData.ichthyosaurDmgShake = 35.0f;

		gSkillData.controllerHealth = 60.0f;
		gSkillData.controllerDmgZap = 25.0f;
		gSkillData.controllerSpeedBall = 800.0f;
		gSkillData.controllerDmgBall = 4.0f;

		gSkillData.nihilanthHealth = 800.0f;
		gSkillData.nihilanthZap = 30.0f;

		gSkillData.zombieHealth = 50.0f;
		gSkillData.zombieDmgOneSlash = 20.0f;
		gSkillData.zombieDmgBothSlash = 40.0f;

		gSkillData.turretHealth = 50.0f;
		gSkillData.miniturretHealth = 40.0f;
		gSkillData.sentryHealth = 40.0f;

		gSkillData.monDmg12MM = 10.0f;
		gSkillData.monDmgMP5 = 4.0f;
		gSkillData.monDmg9MM = 5.0f;

		gSkillData.monDmgHornet = 5.0f;

		gSkillData.suitchargerCapacity = 50.0f;
		gSkillData.batteryCapacity = 15.0f;
		gSkillData.healthchargerCapacity = 40.0f;
	}
}

EntityRandomSpawnerController::EntityRandomSpawnerController( const EntityRandomSpawner &entityRandomSpawner ) :
	entityName( entityRandomSpawner.entityName ),
	maxAmount( entityRandomSpawner.maxAmount ),
	spawnPeriod( entityRandomSpawner.spawnPeriod ),
	nextSpawn( gpGlobals->time + entityRandomSpawner.spawnPeriod )
{ }

void EntityRandomSpawnerController::Think( CBasePlayer *pPlayer ) {
	if ( gpGlobals->time > nextSpawn ) {
		Spawn( pPlayer );
		ResetSpawnPeriod();
	}
}

void EntityRandomSpawnerController::ResetSpawnPeriod() {
	nextSpawn = gpGlobals->time + spawnPeriod;
}

void EntityRandomSpawnerController::Spawn( CBasePlayer *pPlayer ) {
	CBaseEntity *list[1024] = { NULL };
	int amountOfMonsters = UTIL_MonstersInSphere( list, 1024, Vector( 0, 0, 0 ), 8192.0f );
	int amountOfMonstersOfThisType = 0;
	for ( int i = 0 ; i < amountOfMonsters ; i++ ) {
		CBaseEntity *entity = list[i];
		if (
			( entity->pev->spawnflags & SF_MONSTER_PRESERVE ) &&
		   !( entity->pev->deadflag & DEAD_DEAD ) &&
			FStrEq( STRING( entity->pev->classname ), entityName.c_str() )
		) {
			amountOfMonstersOfThisType++;
		}
	}

	if ( amountOfMonstersOfThisType >= maxAmount ) {
		return;
	}

	bool spawnPositionDecided = false;

	do {
		TraceResult tr;
		char bottomTexture[256] = "(null)";
		char upperTexture[256] = "(null)";

		Vector randomPoint = Vector( RANDOM_FLOAT( -4096, 4096 ), RANDOM_FLOAT( -4096, 4096 ), RANDOM_FLOAT( -4096, 4096 ) );
		sprintf( bottomTexture, "%s", g_engfuncs.pfnTraceTexture( NULL, randomPoint, randomPoint - gpGlobals->v_up * 8192 ) );
		sprintf( upperTexture, "%s", g_engfuncs.pfnTraceTexture( NULL, randomPoint, randomPoint + gpGlobals->v_up * 8192 ) );

		if ( FStrEq( bottomTexture, "(null)" )  || FStrEq( bottomTexture, "sky" ) || FStrEq( upperTexture, "(null)" ) ) {
			continue;
		}

		// Drop randomPoint on the floor
		UTIL_TraceLine( randomPoint, randomPoint - gpGlobals->v_up * 8192, dont_ignore_monsters, ignore_glass, pPlayer->edict(), &tr );
		if ( tr.fAllSolid ) {
			continue;
		}

		randomPoint = tr.vecEndPos;

		// Check there are no monsters around
		CBaseEntity *list[1] = { NULL };
		UTIL_MonstersInSphere( list, 1, randomPoint, 32.0f );
		if ( list[0] != NULL ) {
			continue;
		}

		// Prefer not to spawn near player
		UTIL_TraceLine( pPlayer->pev->origin, randomPoint, dont_ignore_monsters, dont_ignore_glass, pPlayer->edict(), &tr );
		if ( tr.flFraction >= 1.0f ) {
			continue;
		}

		// DROP_TO_FLOOR call is required to prevent staying in CLIP brushes
		CBaseEntity *entity = CBaseEntity::Create( ( char * ) entityName.c_str(), randomPoint + Vector( 0, 0, 4 ), Vector( 0, RANDOM_LONG( 0, 360 ), 0 ), NULL );
		if ( DROP_TO_FLOOR( ENT( entity->pev ) ) >= 1 && WALK_MOVE( entity->edict(), 0, 0, WALKMOVE_NORMAL ) ) {
			entity->pev->spawnflags = SF_MONSTER_PRESERVE;
			entity->pev->velocity = Vector( RANDOM_FLOAT( -50, 50 ), RANDOM_FLOAT( -50, 50 ), RANDOM_FLOAT( -50, 50 ) );
			spawnPositionDecided = true;
		} else {
			g_engfuncs.pfnRemoveEntity( ENT( entity->pev ) );
		}

	} while ( !spawnPositionDecided );
}