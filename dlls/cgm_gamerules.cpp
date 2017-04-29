#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"player.h"
#include	"client.h"
#include	"weapons.h"
#include	"cgm_gamerules.h"
#include	"custom_gamemode_config.h"
#include	<algorithm>
#include	"monsters.h"

// Custom Game Mode Rules

int	gmsgCustomEnd	= 0;

// CGameRules were recreated each level change and there were no built-in saving method,
// that means we'd lose config file state on each level change.
//
// This has been changed and now CGameRules state is preseved during CHANGE_LEVEL calls,
// using g_changeLevelOccured like below and see also CWorld::Precache 

extern int g_changeLevelOccured;

CCustomGameModeRules::CCustomGameModeRules( CustomGameModeConfig::GAME_MODE_CONFIG_TYPE configType ) : config( configType )
{
	if ( !gmsgCustomEnd ) {
		gmsgCustomEnd = REG_USER_MSG( "CustomEnd", -1 );
	}

	// Difficulty must be initialized separately and here, becuase entities are not yet spawned,
	// and they take some of the difficulty info at spawn (like CWallHealth)

	// Monster entities also have to be fetched at this moment for ClientPrecache.
	const char *configName = CVAR_GET_STRING( "gamemode_config" );
	config.ReadFile( configName );
	RefreshSkillData();

	cheated = false;
	cheatedMessageSent = false;

	timeDelta = 0.0f;
	lastGlobalTime = 0.0f;

	monsterSpawnPrevented = false;
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

	// For first map
	SpawnEnemiesByConfig( STRING( gpGlobals->mapname ) );

	pPlayer->noSaving = config.noSaving;
	pPlayer->noPills = config.noPills;
	pPlayer->infiniteAmmo = config.infiniteAmmo;
	pPlayer->instaGib = config.instaGib;
	pPlayer->swearOnKill = config.swearOnKill;
	pPlayer->slowmotionOnDamage = config.slowmotionOnDamage;
	pPlayer->oneHitKO = config.oneHitKO;
	pPlayer->oneHitKOFromPlayer = config.oneHitKOFromPlayer;

	pPlayer->snarkParanoia = config.snarkParanoia;
	pPlayer->snarkInception = config.snarkInception;
	pPlayer->snarkNuclear = config.snarkNuclear;
	pPlayer->snarkStayAlive = config.snarkStayAlive;

	pPlayer->SetEvilImpulse101( true );
	for ( size_t i = 0; i < config.loadout.size( ); i++ ) {
		std::string loadoutItem = config.loadout.at( i );

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

	if ( !config.emptySlowmotion ) {
		pPlayer->TakeSlowmotionCharge( 100 );
	}

	if ( config.startPositionSpecified ) {
		pPlayer->pev->origin = config.startPosition;
	}
	if ( config.startYawSpecified ) {
		pPlayer->pev->angles[1] = config.startYaw;
	}

	if ( !config.noSlowmotion ) {

		if ( config.constantSlowmotion ) {
			pPlayer->TakeSlowmotionCharge( 100 );
			pPlayer->SetSlowMotion( true );
			pPlayer->infiniteSlowMotion = true;
			pPlayer->constantSlowmotion = true;
		}

		if ( config.infiniteSlowmotion ) {
			pPlayer->TakeSlowmotionCharge( 100 );
			pPlayer->infiniteSlowMotion = true;
		}

	} else {
		pPlayer->noSlowmotion = true;
	}

	if ( config.bulletPhysicsConstant ) {
		pPlayer->bulletPhysicsMode = BULLET_PHYSICS_CONSTANT;
	} else if ( config.bulletPhysicsEnemiesAndPlayerOnSlowmotion ) {
		pPlayer->bulletPhysicsMode = BULLET_PHYSICS_ENEMIES_AND_PLAYER_ON_SLOWMOTION;
	} else if ( config.bulletPhysicsDisabled ) {
		pPlayer->bulletPhysicsMode = BULLET_PHYSICS_DISABLED;
	}

	pPlayer->weaponRestricted = config.weaponRestricted; // must be set after player has received his weapons

	// Do not let player cheat by not starting at the [startmap]
	const char *startMap = config.startMap.c_str();
	const char *actualMap = STRING( gpGlobals->mapname );
	if ( strcmp( startMap, actualMap ) != 0 ) {
		cheated = true;
	}

}

void CCustomGameModeRules::OnNewlyVisitedMap() {
	CHalfLifeRules::OnNewlyVisitedMap();

	SpawnEnemiesByConfig( STRING( gpGlobals->mapname ) );
	monsterSpawnPrevented = false;
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

	timeDelta = ( gpGlobals->time - lastGlobalTime );

	lastGlobalTime = gpGlobals->time;

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

	if ( config.preventMonsterSpawn && !monsterSpawnPrevented ) {
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
}

void CCustomGameModeRules::CheckForCheats( CBasePlayer *pPlayer )
{
	if ( cheated && cheatedMessageSent || ended ) {
		return;
	}

	if ( cheated ) {
		OnCheated( pPlayer );
		return;
	}

	if ( ( pPlayer->pev->flags & FL_GODMODE ) ||
		 ( pPlayer->pev->flags & FL_NOTARGET ) ||
		 ( pPlayer->pev->movetype & MOVETYPE_NOCLIP ) ||
		 pPlayer->usedCheat ) {
		cheated = true;
	}

}

void CCustomGameModeRules::OnCheated( CBasePlayer *pPlayer ) {
	cheatedMessageSent = true;
}

void CCustomGameModeRules::OnEnd( CBasePlayer *pPlayer ) {
	MESSAGE_BEGIN( MSG_ONE, gmsgCustomEnd, NULL, pPlayer->pev );

	WRITE_STRING( config.name.c_str() );

		WRITE_FLOAT( pPlayer->secondsInSlowmotion );
		WRITE_SHORT( pPlayer->kills );
		WRITE_SHORT( pPlayer->headshotKills );
		WRITE_SHORT( pPlayer->explosiveKills );
		WRITE_SHORT( pPlayer->crowbarKills );
		WRITE_SHORT( pPlayer->projectileKills );

	MESSAGE_END();
}

void CCustomGameModeRules::OnHookedModelIndex( CBasePlayer *pPlayer, edict_t *activator, int modelIndex, const std::string &targetName )
{
	CHalfLifeRules::OnHookedModelIndex( pPlayer, activator, modelIndex, targetName );

	ModelIndex indexToFind( STRING( gpGlobals->mapname ), modelIndex, targetName );
	
	// Does endTriggers contain such index?
	auto foundIndex = config.endTriggers.find( indexToFind );
	if ( foundIndex != config.endTriggers.end() ) {
		config.endTriggers.erase( foundIndex );

		End( pPlayer );
	}
}

void CCustomGameModeRules::SpawnEnemiesByConfig( const char *mapName )
{
	if ( config.entitySpawns.size() == 0 ) {
		return;
	}

	std::vector<EntitySpawn>::iterator entitySpawn = config.entitySpawns.begin();
	for ( entitySpawn; entitySpawn != config.entitySpawns.end(); entitySpawn++ ) {
		if ( entitySpawn->mapName == mapName ) {
			CBaseEntity::Create( allowedEntities[CustomGameModeConfig::GetAllowedEntityIndex( entitySpawn->entityName.c_str() )], entitySpawn->origin, Vector( 0, entitySpawn->angle, 0 ) );
			config.entitySpawns.erase( entitySpawn );
			entitySpawn--;
		}
	}
}

void CCustomGameModeRules::Precache() {
	CHalfLifeRules::Precache();

	for ( std::string spawn : config.entitiesToPrecache ) {
		UTIL_PrecacheOther( spawn.c_str() );
	}
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

	if ( config.powerfulHeadshots ) {
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

	if ( config.difficulty == CustomGameModeConfig::GAME_DIFFICULTY_EASY ) {
		
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
		
	} else if ( config.difficulty == CustomGameModeConfig::GAME_DIFFICULTY_MEDIUM ) {
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
	} else if ( config.difficulty == CustomGameModeConfig::GAME_DIFFICULTY_HARD ) {
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
	}
}
