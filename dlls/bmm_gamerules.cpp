#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"player.h"
#include	"client.h"
#include	"skill.h"
#include	"weapons.h"
#include	"bmm_gamerules.h"
#include	"bmm_config.h"
#include	<algorithm>

// Black Mesa Minute

// CGameRules are recreated each level change, but there's no built-in saving method,
// that means we'd lose statistics below on each level change.
//
// For now whatever is related to BMM state just globally resorts in BMM namespace,
// but ideally they should be inside CBlackMesaMinute, not like this
// or inside the CBasePlayer like before (it has Save/Restore methods though).

bool BMM::timerPaused = false;
bool BMM::ended = false;

bool BMM::cheated = false;
bool BMM::cheatedMessageSent = false;

float BMM::currentTime = 60.0f;
float BMM::currentRealTime = 0.0f;
float BMM::lastGlobalTime = 0.0f;
float BMM::lastRealTime = 0.0f;

int BMM::kills = 0;
int BMM::headshotKills = 0;
int BMM::explosiveKills = 0;
int BMM::crowbarKills = 0;
int BMM::projectileKills = 0;
float BMM::secondsInSlowmotion = 0;

int	gmsgTimerMsg	= 0;
int	gmsgTimerEnd	= 0;
int gmsgTimerValue	= 0;
int gmsgTimerPause  = 0;
int gmsgTimerCheat  = 0;

CBlackMesaMinute::CBlackMesaMinute()
{
	if ( !gmsgTimerMsg ) {
		gmsgTimerMsg = REG_USER_MSG( "TimerMsg", -1 );
		gmsgTimerEnd = REG_USER_MSG( "TimerEnd", -1 );
		gmsgTimerValue = REG_USER_MSG( "TimerValue", 4 );
		gmsgTimerPause = REG_USER_MSG( "TimerPause", 1 );
		gmsgTimerCheat = REG_USER_MSG( "TimerCheat", 0 );
	}

	// Difficulty must be initialized separately and here, becuase entities are not yet spawned,
	// and they take some of the difficulty info at spawn (like CWallHealth)

	// Monster entities also have to be fetched at this moment for ClientPrecache.

	// I don't like the fact that it has to get called each level change. To avoid reinitalizing
	// the whole thing, these were cut out in it's own function.
	const char *configName = CVAR_GET_STRING( "bmm_config" );
	gBMMConfig.InitPreSpawn( configName );
	RefreshSkillData();

	// For subsequent level changes
	SpawnEnemiesByConfig( STRING( gpGlobals->mapname ) );
}

BOOL CBlackMesaMinute::ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[128] )
{
	// Prevent loading of Black Mesa Minute saves
	if ( strlen( STRING( VARS( pEntity )->classname ) ) != 0 ) {
		CBasePlayer *player = ( CBasePlayer* ) CBasePlayer::Instance( pEntity );
		if ( player->bmmEnabled ) {
			g_engfuncs.pfnServerPrint( "You're not allowed to load Black Mesa Minute savefiles.\n" );
			return FALSE;
		}
	}

	gBMMConfig.Reset();
	const char *configName = CVAR_GET_STRING( "bmm_config" );
	if ( !gBMMConfig.Init( configName ) ) {
		g_engfuncs.pfnServerPrint( gBMMConfig.error.c_str() );
		return FALSE;
	}

	return TRUE;
}

void CBlackMesaMinute::PlayerSpawn( CBasePlayer *pPlayer )
{
	if ( gBMMConfig.markedForRestart ) {
		gBMMConfig.markedForRestart = false;
		SERVER_COMMAND( "restart\n" );
		return;
	}

	pPlayer->bmmEnabled = 1;
	BMM::timerPaused = 0;
	BMM::ended = 0;
	BMM::cheated = 0;
	BMM::cheatedMessageSent = 0;
	BMM::currentTime = 60.0f;
	BMM::currentRealTime = 0.0f;

	BMM::kills = 0;
	BMM::headshotKills = 0;
	BMM::explosiveKills = 0;
	BMM::crowbarKills = 0;
	BMM::projectileKills = 0;
	BMM::secondsInSlowmotion = 0;

	if ( gBMMConfig.infiniteAmmo ) {
		pPlayer->infiniteAmmo = true;
	}

	if ( gBMMConfig.instaGib ) {
		gBMMConfig.weaponRestricted = true;
		gBMMConfig.infiniteAmmo = true;

		pPlayer->weaponRestricted = true;
		pPlayer->infiniteAmmo = true;
		pPlayer->instaGib = true;

		pPlayer->SetEvilImpulse101( true );
		pPlayer->GiveNamedItem( "weapon_gauss", true );
		pPlayer->SetEvilImpulse101( false );
	}

	pPlayer->SetEvilImpulse101( true );
	for ( int i = 0; i < gBMMConfig.loadout.size( ); i++ ) {
		std::string loadoutItem = gBMMConfig.loadout.at( i );

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
				const char *item = BMM::allowedItems[BMM::GetAllowedItemIndex( loadoutItem.c_str( ) )];
				pPlayer->GiveNamedItem( item, true );
			}
		}
	}
	pPlayer->SetEvilImpulse101( false );

	// For first map
	SpawnEnemiesByConfig( STRING( gpGlobals->mapname ) );

	if ( gBMMConfig.weaponRestricted ) {
		pPlayer->weaponRestricted = true;
	}
	
	if ( !gBMMConfig.emptySlowmotion ) {
		pPlayer->TakeSlowmotionCharge( 100 );
	}

	if ( gBMMConfig.startPositionSpecified ) {
		pPlayer->pev->origin = gBMMConfig.startPosition;
	}
	if ( gBMMConfig.startYawSpecified ) {
		pPlayer->pev->angles[1] = gBMMConfig.startYaw;
	}

	if ( gBMMConfig.holdTimer ) {
		PauseTimer( pPlayer );
	}

	if ( gBMMConfig.constantSlowmotion ) {
		pPlayer->TakeSlowmotionCharge( 100 );
		pPlayer->SetSlowMotion( true );
		pPlayer->infiniteSlowMotion = true;
	}

	if ( gBMMConfig.infiniteSlowmotion ) {
		pPlayer->TakeSlowmotionCharge( 100 );
		pPlayer->infiniteSlowMotion = true;
	}

	// Do not let player cheat by not starting at the [startmap]
	const char *startMap = gBMMConfig.startMap.c_str();
	const char *actualMap = STRING( gpGlobals->mapname );
	if ( strcmp( startMap, actualMap ) != 0 ) {
		BMM::cheated = true;
	}

}

BOOL CBlackMesaMinute::CanHavePlayerItem( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon )
{
	if ( !pPlayer->weaponRestricted ) {
		return CHalfLifeRules::CanHavePlayerItem( pPlayer, pWeapon );
	}

	if ( !pPlayer->HasWeapons() ) {
		return CHalfLifeRules::CanHavePlayerItem( pPlayer, pWeapon );
	}

	return FALSE;
}

void CBlackMesaMinute::PlayerThink( CBasePlayer *pPlayer )
{
	// Black Mesa Minute running timer
	if ( !BMM::timerPaused && pPlayer->pev->deadflag == DEAD_NO ) {
		float timeDelta = ( gpGlobals->time - BMM::lastGlobalTime );

		// This is terribly wrong, it would be better to reset lastGlobalTime on actual change level event
		// It was made to prevent timer messup during level changes, because each level has it's own local time
		if ( fabs( timeDelta ) > 0.1 ) {
			BMM::lastGlobalTime = gpGlobals->time;
		}
		else {
			BMM::currentTime -= timeDelta;
			BMM::lastGlobalTime = gpGlobals->time;

			if ( pPlayer->slowMotionEnabled ) {
				BMM::secondsInSlowmotion += timeDelta;
			}
			
			if ( BMM::currentTime <= 0.0f && pPlayer->pev->deadflag == DEAD_NO ) {
				ClientKill( ENT( pPlayer->pev ) );
			}
		}
		
		// Counting real time
		if ( !UTIL_IsPaused() ) {
			float realTimeDetla = ( g_engfuncs.pfnTime() - BMM::lastRealTime );

			if ( fabs( realTimeDetla ) > 0.1 ) {
				BMM::lastRealTime = g_engfuncs.pfnTime();
			} else {
				BMM::currentRealTime += realTimeDetla;
				BMM::lastRealTime = g_engfuncs.pfnTime();
			}
		}
	}

	MESSAGE_BEGIN( MSG_ONE, gmsgTimerValue, NULL, pPlayer->pev );
		WRITE_FLOAT( BMM::currentTime );
	MESSAGE_END();

	CheckForCheats( pPlayer );
}

void CBlackMesaMinute::CheckForCheats( CBasePlayer *pPlayer )
{
	if ( BMM::cheated && BMM::cheatedMessageSent || BMM::ended ) {
		return;
	}

	if ( BMM::cheated ) {
		MESSAGE_BEGIN( MSG_ONE, gmsgTimerCheat, NULL, pPlayer->pev );
		MESSAGE_END();
		
		BMM::cheatedMessageSent = true;
		return;
	}

	if ( ( pPlayer->pev->flags & FL_GODMODE ) ||
		 ( pPlayer->pev->flags & FL_NOTARGET ) ||
		 ( pPlayer->pev->movetype & MOVETYPE_NOCLIP ) ||
		 pPlayer->usedCheat ) {
		BMM::cheated = true;
	}

}

void CBlackMesaMinute::IncreaseTime( CBasePlayer *pPlayer, const Vector &eventPos, bool isHeadshot, bool killedByExplosion, bool destroyedGrenade, bool killedByCrowbar ) {

	if ( BMM::timerPaused ) {
		return;
	}

	BMM::kills++;
		
	if ( killedByExplosion ) {
		int timeToAdd = TIMEATTACK_EXPLOSION_BONUS_TIME;
		BMM::currentTime += timeToAdd;
		BMM::explosiveKills++;

		MESSAGE_BEGIN( MSG_ONE, gmsgTimerMsg, NULL, pPlayer->pev );
			WRITE_STRING( "EXPLOSION BONUS" );
			WRITE_LONG( timeToAdd );
			WRITE_COORD( eventPos.x );
			WRITE_COORD( eventPos.y );
			WRITE_COORD( eventPos.z );
		MESSAGE_END();
	}
	else if ( killedByCrowbar ) {
		int timeToAdd = TIMEATTACK_KILL_CROWBAR_BONUS_TIME;
		BMM::currentTime += timeToAdd;
		BMM::crowbarKills++;

		MESSAGE_BEGIN( MSG_ONE, gmsgTimerMsg, NULL, pPlayer->pev );
			WRITE_STRING( "MELEE BONUS" );
			WRITE_LONG( timeToAdd );
			WRITE_COORD( eventPos.x );
			WRITE_COORD( eventPos.y );
			WRITE_COORD( eventPos.z );
		MESSAGE_END( );
	}
	else if ( isHeadshot ) {
		int timeToAdd = TIMEATTACK_KILL_BONUS_TIME + TIMEATTACK_HEADSHOT_BONUS_TIME;
		BMM::currentTime += timeToAdd;
		BMM::headshotKills++;

		MESSAGE_BEGIN( MSG_ONE, gmsgTimerMsg, NULL, pPlayer->pev );
			WRITE_STRING( "HEADSHOT BONUS" );
			WRITE_LONG( timeToAdd );
			WRITE_COORD( eventPos.x );
			WRITE_COORD( eventPos.y );
			WRITE_COORD( eventPos.z );
		MESSAGE_END();
	}
	else if ( destroyedGrenade ) {
		int timeToAdd = TIMEATTACK_GREANDE_DESTROYED_BONUS_TIME;
		BMM::currentTime += timeToAdd;
		BMM::projectileKills++;

		MESSAGE_BEGIN( MSG_ONE, gmsgTimerMsg, NULL, pPlayer->pev );
			WRITE_STRING( "PROJECTILE BONUS" );
			WRITE_LONG( timeToAdd );
			WRITE_COORD( eventPos.x );
			WRITE_COORD( eventPos.y );
			WRITE_COORD( eventPos.z );
		MESSAGE_END( );
	}
	else {
		int timeToAdd = TIMEATTACK_KILL_BONUS_TIME;
		BMM::currentTime += timeToAdd;

		MESSAGE_BEGIN( MSG_ONE, gmsgTimerMsg, NULL, pPlayer->pev );
			WRITE_STRING( "TIME BONUS" );
			WRITE_LONG( timeToAdd );
			WRITE_COORD( eventPos.x );
			WRITE_COORD( eventPos.y );
			WRITE_COORD( eventPos.z );
		MESSAGE_END( );
	}


}

// Added for Gonarch
// Might actually call this from old IncreaseTime
void CBlackMesaMinute::IncreaseTime( CBasePlayer *pPlayer, const Vector &eventPos, int timeToAdd, const char *message )
{
	if ( BMM::timerPaused ) {
		return;
	}

	BMM::currentTime += timeToAdd;

	MESSAGE_BEGIN( MSG_ONE, gmsgTimerMsg, NULL, pPlayer->pev );
		WRITE_STRING( message );
		WRITE_LONG( timeToAdd );
		WRITE_COORD( eventPos.x );
		WRITE_COORD( eventPos.y );
		WRITE_COORD( eventPos.z );
	MESSAGE_END();
}

void CBlackMesaMinute::End( CBasePlayer *pPlayer ) {
	if ( BMM::ended ) {
		return;
	}

	if ( pPlayer->slowMotionEnabled ) {
		pPlayer->ToggleSlowMotion();
	}

	BMM::ended = true;
	PauseTimer( pPlayer );

	pPlayer->pev->movetype = MOVETYPE_NONE;
	pPlayer->pev->flags |= FL_NOTARGET;
	pPlayer->RemoveAllItems( true );

	BlackMesaMinuteRecord record( gBMMConfig.configName.c_str() );
	
	MESSAGE_BEGIN( MSG_ONE, gmsgTimerEnd, NULL, pPlayer->pev );
	
		WRITE_STRING( gBMMConfig.name.c_str() );

		WRITE_FLOAT( BMM::currentTime );
		WRITE_FLOAT( BMM::currentRealTime );

		WRITE_FLOAT( record.time );
		WRITE_FLOAT( record.realTime );
		WRITE_FLOAT( record.realTimeMinusTime );

		WRITE_FLOAT( BMM::secondsInSlowmotion );
		WRITE_SHORT( BMM::kills );
		WRITE_SHORT( BMM::headshotKills );
		WRITE_SHORT( BMM::explosiveKills );
		WRITE_SHORT( BMM::crowbarKills );
		WRITE_SHORT( BMM::projectileKills );
		
	MESSAGE_END();

	if ( !BMM::cheated ) {

		// Write new records if there are
		if ( BMM::currentTime > record.time ) {
			record.time = BMM::currentTime;
		}
		if ( BMM::currentRealTime < record.realTime ) {
			record.realTime = BMM::currentRealTime;
		}

		float bmmRealTimeMinusTime = max( 0.0f, BMM::currentRealTime - BMM::currentTime );
		if ( bmmRealTimeMinusTime < record.realTimeMinusTime ) {
			record.realTimeMinusTime = bmmRealTimeMinusTime;
		}

		record.Save();
	}
}

void CBlackMesaMinute::PauseTimer( CBasePlayer *pPlayer )
{
	if ( BMM::timerPaused ) {
		return;
	}
	
	BMM::timerPaused = true;
	
	MESSAGE_BEGIN( MSG_ONE, gmsgTimerPause, NULL, pPlayer->pev );
		WRITE_BYTE( true );
	MESSAGE_END();
}

void CBlackMesaMinute::ResumeTimer( CBasePlayer *pPlayer )
{
	if ( !BMM::timerPaused ) {
		return;
	}

	BMM::timerPaused = false;
	
	MESSAGE_BEGIN( MSG_ONE, gmsgTimerPause, NULL, pPlayer->pev );
		WRITE_BYTE( false );
	MESSAGE_END();
}

void CBlackMesaMinute::HookModelIndex( edict_t *activator, const char *mapName, int modelIndex )
{
	CBasePlayer *pPlayer = ( CBasePlayer * ) CBasePlayer::Instance( g_engfuncs.pfnPEntityOfEntIndex( 1 ) );
	if ( !pPlayer ) {
		return;
	}

	BlackMesaMinuteConfig::ModelIndex indexToFind( mapName, modelIndex );
	

	// Does timerPauses contain such index?
	auto foundIndex = gBMMConfig.timerPauses.find( indexToFind ); // it's complex iterator type, so leave it auto
	if ( foundIndex != gBMMConfig.timerPauses.end() ) {
		bool constant = foundIndex->constant;

		if ( !constant ) {
			gBMMConfig.timerPauses.erase( foundIndex );
		}

		PauseTimer( pPlayer );
		return;
	}

	// Does timerResumes contain such index?
	foundIndex = gBMMConfig.timerResumes.find( indexToFind );
	if ( foundIndex != gBMMConfig.timerResumes.end() ) {
		bool constant = foundIndex->constant;

		if ( !constant ) {
			gBMMConfig.timerResumes.erase( foundIndex );
		}

		ResumeTimer( pPlayer );
		return;
	}

	// Does endTriggers contain such index?
	foundIndex = gBMMConfig.endTriggers.find( indexToFind );
	if ( foundIndex != gBMMConfig.endTriggers.end() ) {
		gBMMConfig.endTriggers.erase( foundIndex );

		End( pPlayer );
		return;
	}
}

void CBlackMesaMinute::SpawnEnemiesByConfig( const char *mapName )
{
	if ( gBMMConfig.entitySpawns.size() == 0 ) {
		return;
	}

	std::vector<BlackMesaMinuteConfig::EntitySpawn>::iterator entitySpawn = gBMMConfig.entitySpawns.begin();
	for ( entitySpawn; entitySpawn != gBMMConfig.entitySpawns.end(); entitySpawn++ ) {
		if ( entitySpawn->mapName == mapName ) {
			CBaseEntity::Create( BMM::allowedEntities[BMM::GetAllowedEntityIndex( entitySpawn->entityName.c_str() )], entitySpawn->origin, Vector( 0, entitySpawn->angle, 0 ) );
			gBMMConfig.entitySpawns.erase( entitySpawn );
			entitySpawn--;
		}
	}
}

// Hardcoded values so it won't depend on console variables
void CBlackMesaMinute::RefreshSkillData() 
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

	if ( gBMMConfig.powerfulHeadshots ) {
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

	if ( gBMMConfig.difficulty == BlackMesaMinuteConfig::BMM_DIFFICULTY_EASY ) {
		
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
		
	} else if ( gBMMConfig.difficulty == BlackMesaMinuteConfig::BMM_DIFFICULTY_MEDIUM ) {
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
	} else if ( gBMMConfig.difficulty == BlackMesaMinuteConfig::BMM_DIFFICULTY_HARD ) {
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
