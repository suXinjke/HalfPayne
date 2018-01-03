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
int gmsgTimerDeact = 0;
int gmsgTimerValue = 0;
int gmsgTimerCheat = 0;

int gmsgCountDeact = 0;
int gmsgCountValue = 0;
int gmsgCountCheat = 0;

int gmsgScoreDeact  = 0;
int gmsgScoreValue	= 0;
int gmsgScoreCheat	= 0;

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

		gmsgTimerDeact = REG_USER_MSG( "TimerDeact", 0 );
		gmsgTimerValue = REG_USER_MSG( "TimerValue", -1 );
		gmsgTimerCheat = REG_USER_MSG( "TimerCheat", 0 );

		gmsgCountDeact = REG_USER_MSG( "CountDeact", 0 );
		gmsgCountValue = REG_USER_MSG( "CountValue", -1 );
		gmsgCountCheat = REG_USER_MSG( "CountCheat", 0 );

		gmsgScoreDeact = REG_USER_MSG( "ScoreDeact", 0 );
		gmsgScoreValue = REG_USER_MSG( "ScoreValue", 16 );
		gmsgScoreCheat = REG_USER_MSG( "ScoreCheat", 0 );
	}

	// Difficulty must be initialized separately and here, becuase entities are not yet spawned,
	// and they take some of the difficulty info at spawn (like CWallHealth)

	// Monster entities also have to be fetched at this moment for ClientPrecache.
	const char *configName = CVAR_GET_STRING( "gamemode_config" );
	config.ReadFile( configName );
	RefreshSkillData();

	endMarkersActive = false;

	cheatedMessageSent = false;
	startMapDoesntMatch = false;

	timeDelta = 0.0f;
	musicSwitchDelay = 0.0f;

	yOffset = 0;
	maxYOffset = 0;

	for ( const auto &spawner : config.entityRandomSpawners ) {
		entityRandomSpawnerControllers.push_back( EntityRandomSpawnerController( spawner ) );
	}
}

void CCustomGameModeRules::RestartGame() {
	char mapName[256];
	sprintf( mapName, "%s", config.startMap.c_str() );

	// Change level to [startmap] and queue the 'restart' command
	// for CCustomGameModeRules::PlayerSpawn.
	// Would be better just to execute 'cgm' command directly somehow.
	config.markedForRestart = true;
	CHANGE_LEVEL( mapName, NULL );
}

void CCustomGameModeRules::SendGameLogMessage( CBasePlayer *pPlayer, const std::string &message ) {
	MESSAGE_BEGIN( MSG_ONE, gmsgGLogMsg, NULL, pPlayer->pev );
		WRITE_STRING( message.c_str() );
		WRITE_LONG( maxYOffset );
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

void CCustomGameModeRules::ApplyStartPositionToEntity( CBaseEntity *entity, const StartPosition &startPosition ) {

	if ( !std::isnan( startPosition.x ) ) {
		entity->pev->origin.x = startPosition.x;
		entity->pev->origin.y = std::isnan( startPosition.y ) ? 0 : startPosition.y;
		entity->pev->origin.z = std::isnan( startPosition.z ) ? 0 : startPosition.z;
	}

	if ( !std::isnan( startPosition.angle ) ) {
		entity->pev->angles[1] = startPosition.angle;
	}

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

	// '\' slashes are getting eaten by ALLOC_STRING? must prevent this by replacing them with '/'
	std::string sanitizedConfigName = config.configName;
	std::transform( sanitizedConfigName.begin(), sanitizedConfigName.end(), sanitizedConfigName.begin(), []( auto &letter ) {
		return letter == '\\' ? '/' : letter;
	} );
	pPlayer->activeGameModeConfig = ALLOC_STRING( sanitizedConfigName.c_str() );

	for ( const GameplayMod &mod : config.mods ) {
		mod.init( pPlayer );
	}

	pPlayer->SetEvilImpulse101( true );
	for ( const auto &item : config.loadout ) {

		for ( int i = 0 ; i < item.amount ; i++ ) {
			if ( item.name == "all" ) {
				pPlayer->GiveAll( true );
				pPlayer->SetEvilImpulse101( true ); // it was set false by GiveAll
			} else {
				if ( item.name == "item_healthkit" ) {
					pPlayer->TakePainkiller();
				} else if ( item.name == "item_suit" ) {
					pPlayer->pev->weapons |= ( 1 << WEAPON_SUIT );
				} else if ( item.name == "item_longjump" ) {
					pPlayer->m_fLongJump = TRUE;
					g_engfuncs.pfnSetPhysicsKeyValue( pPlayer->edict( ), "slj", "1" );
				} else {
					pPlayer->GiveNamedItem( allowedItems[CustomGameModeConfig::GetAllowedItemIndex( item.name.c_str( ) )], true );
				}
			}
		}
	}
	pPlayer->SetEvilImpulse101( false );
	pPlayer->loadoutReceived = true;

	if (
		config.IsGameplayModActive( GAMEPLAY_MOD_EMPTY_SLOWMOTION ) ||
		config.IsGameplayModActive( GAMEPLAY_MOD_NO_SLOWMOTION )
	) {
		pPlayer->slowMotionCharge = 0;
	}

	if ( config.startPosition.defined ) {
		ApplyStartPositionToEntity( pPlayer, config.startPosition );
	}

	bool spawningAfterIntermission = false;
	if ( g_latestIntermission.startPos.defined ) {
		ApplyStartPositionToEntity( pPlayer, g_latestIntermission.startPos );

		if ( g_latestIntermission.startPos.stripped ) {
			pPlayer->RemoveAllItems( false );
		}

		spawningAfterIntermission = true;
		g_latestIntermission.startPos.defined = false;
	}

	if ( config.musicPlaylist.size() > 0 ) {
		pPlayer->noMapMusic = TRUE;
	}

	if ( config.hasEndMarkers || !config.endConditions.empty() ) {
		pPlayer->noSaving = TRUE;
	}

	pPlayer->activeGameModeConfigHash = ALLOC_STRING( config.sha1.c_str() );
	
	// Do not let player cheat by not starting at the [startmap]
	if ( !spawningAfterIntermission ) {
		const char *actualMap = STRING( gpGlobals->mapname );
		startMapDoesntMatch = config.startMap != actualMap;
	}
}

BOOL CCustomGameModeRules::CanHavePlayerItem( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon )
{
	if ( !pPlayer->weaponRestricted ) {
		return CHalfLifeRules::CanHavePlayerItem( pPlayer, pWeapon );
	}

	if ( !pPlayer->HasWeapons() || !pPlayer->loadoutReceived ) {
		return CHalfLifeRules::CanHavePlayerItem( pPlayer, pWeapon );
	}

	return FALSE;
}

void CCustomGameModeRules::PlayerThink( CBasePlayer *pPlayer )
{
	CHalfLifeRules::PlayerThink( pPlayer );
	
	timeDelta = ( gpGlobals->time - pPlayer->lastGlobalTime );

	pPlayer->lastGlobalTime = gpGlobals->time;

	if ( !pPlayer->timerPaused && pPlayer->pev->deadflag == DEAD_NO ) {

		// This is terribly wrong, it would be better to reset lastGlobalTime on actual change level event
		// It was made to prevent timer messup during level changes, because each level has it's own local time
		if ( fabs( timeDelta ) <= 0.1 ) {
			if ( pPlayer->timerBackwards ) {
				pPlayer->time -= timeDelta;
			} else {
				pPlayer->time += timeDelta;
			}

			pPlayer->realTime += timeDelta / pPlayer->desiredTimeScale;

			if ( pPlayer->slowMotionEnabled ) {
				pPlayer->secondsInSlowmotion += timeDelta;
			}

			CheckForCheats( pPlayer );
		}
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
		size_t musicIndexToPlay = pPlayer->currentMusicPlaylistIndex + 1;
		if ( config.musicPlaylistShuffle ) {
			static std::random_device rd;
			static std::mt19937 gen( rd() );
			std::uniform_int_distribution<> dis( 0, musicPlaylistSize - 1 );
			musicIndexToPlay = dis( gen );
		}

		if ( musicIndexToPlay >= musicPlaylistSize ) {
			musicIndexToPlay = 0;
		}
		
		pPlayer->SendPlayMusicMessage( config.musicPlaylist.at( musicIndexToPlay ).path );
		pPlayer->currentMusicPlaylistIndex = musicIndexToPlay;

		musicSwitchDelay = gpGlobals->time + 0.2f;
	}

	const int SPACING = 56;
	yOffset = 0;

	if ( pPlayer->timerShown ) {
		MESSAGE_BEGIN( MSG_ONE, gmsgTimerValue, NULL, pPlayer->pev );
			WRITE_STRING(
				pPlayer->timerShowReal ? "REAL TIME" :
				pPlayer->timerBackwards ? "TIME LEFT" :
				"TIME"
			);
			WRITE_FLOAT( pPlayer->timerShowReal ? pPlayer->realTime : pPlayer->time );
			WRITE_LONG( yOffset );
		MESSAGE_END();

		yOffset += SPACING;

		if ( pPlayer->timerBackwards && pPlayer->time <= 0.0f && pPlayer->pev->deadflag == DEAD_NO ) {
			ClientKill( ENT( pPlayer->pev ) );
		}
	}

	if ( pPlayer->activeGameMode == GAME_MODE_SCORE_ATTACK ) {
		MESSAGE_BEGIN( MSG_ONE, gmsgScoreValue, NULL, pPlayer->pev );
			WRITE_LONG( pPlayer->score );
			WRITE_LONG( pPlayer->comboMultiplier );
			WRITE_FLOAT( pPlayer->comboMultiplierReset );
			WRITE_LONG( yOffset );
		MESSAGE_END();

		yOffset += SPACING;
	}

	int conditionsHeight = 0;

	MESSAGE_BEGIN( MSG_ONE, gmsgCountValue, NULL, pPlayer->pev );
	WRITE_SHORT( config.endConditions.size() );

	for ( const auto &condition : config.endConditions ) {
		WRITE_LONG( condition.activations );
		WRITE_LONG( condition.activationsRequired );

		WRITE_STRING( condition.objective.c_str() );

		conditionsHeight += condition.activationsRequired > 1 ? SPACING : SPACING - 34;
	}
	WRITE_LONG( yOffset );
	MESSAGE_END();

	yOffset += conditionsHeight;
	maxYOffset = max( yOffset, maxYOffset );
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
	}

	auto teleportOnKillWeapon = std::string( STRING( pPlayer->teleportOnKillWeapon ) );
	auto activeItem = pPlayer->m_pActiveItem;
	if (
		pPlayer->teleportOnKill &&
		( ( teleportOnKillWeapon.empty() ) || ( activeItem && STRING( activeItem->pev->classname ) == teleportOnKillWeapon ) )
	) {

		TraceResult tr;
		bool canTeleport = false;
		Vector destination = victim->pev->origin;
		for ( int i = 0; i < 73 && !canTeleport; i++ ) {
			UTIL_TraceHull( destination, destination, dont_ignore_monsters, human_hull, victim->edict(), &tr );
			if ( tr.fAllSolid ) {
				destination.z += 1.0f;
			} else {
				canTeleport = true;
			}
		}

		if ( canTeleport ) {
			UTIL_SetOrigin( pPlayer->pev, destination );
			if ( CBaseMonster *monster = dynamic_cast<CBaseMonster *>( victim ) ) {
				monster->GibMonster();
			}
		}

	}

	CHalfLifeRules::OnKilledEntityByPlayer( pPlayer, victim, killedEntity, isHeadshot, killedByExplosion, killedByCrowbar );
}

void CCustomGameModeRules::ActivateEndMarkers( CBasePlayer *pPlayer ) {
	if ( !endMarkersActive ) {
		endMarkersActive = true;
		if ( pPlayer ) {
			EMIT_SOUND( ENT( pPlayer->pev ), CHAN_STATIC, "var/end_marker.wav", 1, ATTN_NORM, true );
			SendGameLogMessage( pPlayer, "HEAD TO THE GOAL" );
		}
	}

	tasks.push_back( { 0.0f, []( CBasePlayer *pPlayer ) {
		CBaseEntity *pEntity = NULL;
		while ( ( pEntity = UTIL_FindEntityInSphere( pEntity, Vector( 0, 0, 0 ), 8192 ) ) != NULL ) {
			if ( FStrEq( STRING( pEntity->pev->classname ), "end_marker" ) ) {
				pEntity->Use( NULL, NULL, USE_ON, 0.0f );
			}
		}
	} } );
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
		 pPlayer->usedCheat || startMapDoesntMatch ||
		 STRING( pPlayer->activeGameModeConfigHash ) != config.sha1
	) {
		SendGameLogMessage( pPlayer, "YOU'VE BEEN CHEATING - RESULTS WON'T BE SAVED" );
		pPlayer->cheated = true;
	}

}

void CCustomGameModeRules::OnCheated( CBasePlayer *pPlayer ) {
	cheatedMessageSent = true;

	MESSAGE_BEGIN( MSG_ONE, gmsgTimerCheat, NULL, pPlayer->pev );
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_ONE, gmsgCountCheat, NULL, pPlayer->pev );
	MESSAGE_END();
}

void CCustomGameModeRules::OnEnd( CBasePlayer *pPlayer ) {
	PauseTimer( pPlayer );

	if ( !config.gameFinishedOnce && pPlayer->timerBackwards ) {
		config.record.time = 0.0f;
	}

	MESSAGE_BEGIN( MSG_ONE, gmsgGLogDeact, NULL, pPlayer->pev );
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_ONE, gmsgGLogWDeact, NULL, pPlayer->pev );
	MESSAGE_END();
	
	MESSAGE_BEGIN( MSG_ONE, gmsgEndTitle, NULL, pPlayer->pev );
		WRITE_STRING( CustomGameModeConfig::ConfigTypeToGameModeName( config.configType, true ).c_str() );
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_ONE, gmsgEndTitle, NULL, pPlayer->pev );
		WRITE_STRING(
			config.name.empty() ?
			"LEVEL COMPLETE" :
			( config.name + " - COMPLETE" ).c_str()
		);
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_ONE, gmsgTimerDeact, NULL, pPlayer->pev );
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_ONE, gmsgEndTime, NULL, pPlayer->pev );
		WRITE_STRING( "TIME SCORE|PERSONAL BESTS" );
		WRITE_FLOAT( pPlayer->time );
		WRITE_FLOAT( config.record.time );
		WRITE_BYTE( pPlayer->timerBackwards ? pPlayer->time > config.record.time : pPlayer->time < config.record.time  );
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_ONE, gmsgEndTime, NULL, pPlayer->pev );
		WRITE_STRING( "REAL TIME" );
		WRITE_FLOAT( pPlayer->realTime );
		WRITE_FLOAT( config.record.realTime );
		WRITE_BYTE( pPlayer->realTime < config.record.realTime );
	MESSAGE_END();

	if ( pPlayer->timerBackwards ) {
		float realTimeMinusTime = max( 0.0f, pPlayer->realTime - pPlayer->time );
		MESSAGE_BEGIN( MSG_ONE, gmsgEndTime, NULL, pPlayer->pev );
			WRITE_STRING( "REAL TIME MINUS SCORE" );
			WRITE_FLOAT( realTimeMinusTime );
			WRITE_FLOAT( config.record.realTimeMinusTime );
			WRITE_BYTE( realTimeMinusTime < config.record.realTimeMinusTime );
		MESSAGE_END();
	}

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

void CCustomGameModeRules::OnHookedModelIndex( CBasePlayer *pPlayer, CBaseEntity *activator, int modelIndex, const std::string &className, const std::string &targetName, bool firstTime )
{
	CHalfLifeRules::OnHookedModelIndex( pPlayer, activator, modelIndex, className, targetName, firstTime );
	
	if ( config.endTrigger.Fits( modelIndex, className, targetName, firstTime ) ) {
		End( pPlayer );
	}

	for ( const auto &potentialIntermission : config.intermissions ) {
		if ( potentialIntermission.Fits( modelIndex, className, targetName, firstTime ) ) {
			g_latestIntermission = potentialIntermission;
			CHANGE_LEVEL( ( char * ) g_latestIntermission.entityName.c_str(), NULL );
			// after that, g_latestIntermission becomes undefined in PlayerSpawn function
			break;
		}
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
						penguin->pev->angles = UTIL_VecToAngles( activator->pev->origin - penguin->pev->origin );
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

	if ( targetName == "on_map_start" && config.IsGameplayModActive( GAMEPLAY_MOD_PREVENT_MONSTER_SPAWN ) ) {

		for ( int i = 0 ; i < 1024 ; i++ ) {
			edict_t *edict = g_engfuncs.pfnPEntityOfEntIndex( i );
			if ( !edict ) {
				continue;
			}

			if ( CBaseEntity *entity = CBaseEntity::Instance( edict ) ) {
				if ( entity->pev->spawnflags & SF_MONSTER_PRESERVE ) {
					continue;
				}

				if ( std::string( STRING( entity->pev->classname ) ).find( "monster_" ) == 0 ) {
					entity->pev->flags |= FL_KILLME;
				}
			}
		}
	}

	for ( const auto &timerPause : config.timerPauses ) {
		if ( timerPause.Fits( modelIndex, className, targetName, firstTime ) ) {
			PauseTimer( pPlayer );
		}
	}

	for ( const auto &timerResume : config.timerResumes ) {
		if ( timerResume.Fits( modelIndex, className, targetName, firstTime ) ) {
			ResumeTimer( pPlayer );
		}
	}

	for ( auto &condition : config.endConditions ) {
		if ( condition.Fits( modelIndex, className, targetName, firstTime ) ) {
			condition.activations++;

			if ( condition.activations >= condition.activationsRequired ) {

				bool allComplete = true;

				for ( const auto &condition : config.endConditions ) {
					if ( condition.activations < condition.activationsRequired ) {
						allComplete = false;
						break;
					}
				}

				if ( allComplete ) {
					if ( config.hasEndMarkers ) {
						ActivateEndMarkers( pPlayer );
					} else {
						End( pPlayer );
					}
				}
			}
		}
	}

	for ( const auto &teleporterRedirect : config.teleports ) {
		if ( teleporterRedirect.Fits( modelIndex, className, targetName, true ) ) {
			ApplyStartPositionToEntity( pPlayer, teleporterRedirect.pos );
		}
	}

}

void CCustomGameModeRules::OnChangeLevel() {
	CHalfLifeRules::OnChangeLevel();
	musicSwitchDelay = 0.0f;

	for ( auto &spawner : entityRandomSpawnerControllers ) {
		spawner.ResetSpawnPeriod();
	}

	if ( endMarkersActive ) {
		ActivateEndMarkers();
	}
}

extern int g_autoSaved;
void CCustomGameModeRules::OnSave( CBasePlayer *pPlayer ) {
	if ( pPlayer->noSaving && !g_autoSaved ) {
		SendGameLogMessage( pPlayer, "SAVING IS ACTUALLY DISABLED" );
	}
	CHalfLifeRules::OnSave( pPlayer );
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
	spawnData( entityRandomSpawner.entity ),
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
			FStrEq( STRING( entity->pev->classname ), spawnData.name.c_str() )
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

		// Player should not be out of bounds
		sprintf( bottomTexture, "%s", g_engfuncs.pfnTraceTexture( NULL, pPlayer->pev->origin, randomPoint - gpGlobals->v_up * 8192 ) );
		sprintf( upperTexture, "%s", g_engfuncs.pfnTraceTexture( NULL, pPlayer->pev->origin, randomPoint + gpGlobals->v_up * 8192 ) );
		bool playerIsOutOfBounds = FStrEq( bottomTexture, "(null)" ) || FStrEq( upperTexture, "(null)" );

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
		if ( tr.flFraction >= 1.0f && !playerIsOutOfBounds ) {
			continue;
		}

		spawnData.x = randomPoint.x;
		spawnData.y = randomPoint.y;
		spawnData.z = randomPoint.z + 4;
		spawnData.angle = RANDOM_LONG( 0, 360 );

		CBaseEntity *entity = CCustomGameModeRules::SpawnBySpawnData( spawnData );
		if ( entity ) {
			// DROP_TO_FLOOR call is required to prevent staying in CLIP brushes
			if ( DROP_TO_FLOOR( ENT( entity->pev ) ) >= 1 && WALK_MOVE( entity->edict(), 0, 0, WALKMOVE_NORMAL ) ) {
				entity->pev->velocity = Vector( RANDOM_FLOAT( -50, 50 ), RANDOM_FLOAT( -50, 50 ), RANDOM_FLOAT( -50, 50 ) );
				spawnPositionDecided = true;
			} else {
				g_engfuncs.pfnRemoveEntity( ENT( entity->pev ) );
			}
		}

	} while ( !spawnPositionDecided );
}