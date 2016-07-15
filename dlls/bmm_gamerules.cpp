#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"player.h"
#include	"client.h"
#include	"bmm_gamerules.h"
#include	"bmm_config.h"

// Black Mesa Minute

// CGameRules are recreated each level change, but there's no built-in saving method,
// that means we'd lose statistics below on each level change.
//
// For now whatever is related to BMM state just globally resorts in BMM namespace,
// but ideally they should be inside CBlackMesaMinute, not like this
// or inside the CBasePlayer like before (it has Save/Restore methods though).

bool BMM::timerPaused = false;
bool BMM::ended = false;

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

CBlackMesaMinute::CBlackMesaMinute()
{
	RefreshSkillData();

	if ( !gmsgTimerMsg ) {
		gmsgTimerMsg = REG_USER_MSG( "TimerMsg", -1 );
		gmsgTimerEnd = REG_USER_MSG( "TimerEnd", -1 );
		gmsgTimerValue = REG_USER_MSG( "TimerValue", 4 );
		gmsgTimerPause = REG_USER_MSG( "TimerPause", 1 );
	}
}

BOOL CBlackMesaMinute::ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[128] )
{
	const int READ_BLACK_MESA_MINUTE_CONFIG = true;

	if ( READ_BLACK_MESA_MINUTE_CONFIG ) {
		const char *configName = CVAR_GET_STRING( "bmm_config" );
		if ( !gBMMConfig.Init( configName ) ) {
			return FALSE;
		}
	}

	// Prevent loading of Black Mesa Minute saves
	if ( strlen( STRING( VARS( pEntity )->classname ) ) != 0 ) {
		CBasePlayer *player = ( CBasePlayer* ) CBasePlayer::Instance( pEntity );
		if ( player->bmmEnabled ) {
			g_engfuncs.pfnServerPrint( "You're not allowed to load Black Mesa Minute savefiles.\n" );
			return FALSE;
		}
	}

	return TRUE;
}

void CBlackMesaMinute::PlayerSpawn( CBasePlayer *pPlayer )
{
	pPlayer->bmmEnabled = 1;
	BMM::timerPaused = 0;
	BMM::ended = 0;
	BMM::currentTime = 60.0f;
	BMM::currentRealTime = 0.0f;

	BMM::kills = 0;
	BMM::headshotKills = 0;
	BMM::explosiveKills = 0;
	BMM::crowbarKills = 0;
	BMM::projectileKills = 0;
	BMM::secondsInSlowmotion = 0;

	pPlayer->SetEvilImpulse101( true );
	for ( int i = 0; i < gBMMConfig.loadout.size( ); i++ ) {
		std::string loadoutItem = gBMMConfig.loadout.at( i );

		if ( loadoutItem == "all" ) {
			pPlayer->GiveAll();
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
				pPlayer->GiveNamedItem( item );
			}
		}
	}
	pPlayer->SetEvilImpulse101( false );

	pPlayer->TakeSlowmotionCharge( 100 );

	if ( gBMMConfig.startPositionSpecified ) {
		pPlayer->pev->origin = gBMMConfig.startPosition;
	}
	if ( gBMMConfig.startYawSpecified ) {
		pPlayer->pev->angles[1] = gBMMConfig.startYaw;
	}
}

void CBlackMesaMinute::RefreshSkillData() 
{
	CVAR_SET_FLOAT( "skill", 2.0f );
	CGameRules::RefreshSkillData();

	// Need to hardcode values depending on config file modificators, which are yet to be implemented
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
}

void CBlackMesaMinute::IncreaseTime( CBasePlayer *pPlayer, const Vector &eventPos, bool isHeadshot, bool killedByExplosion, bool destroyedGrenade, bool killedByCrowbar ) {

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