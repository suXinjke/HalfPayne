#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"player.h"
#include	"client.h"
#include	"weapons.h"
#include	"bmm_gamerules.h"
#include	<algorithm>
#include <fstream>
#include	"monsters.h"

int gmsgTimerDeact = 0;
int gmsgTimerValue = 0;
int gmsgTimerCheat = 0;

extern int gmsgEndTime;

CBlackMesaMinute::CBlackMesaMinute() : CCustomGameModeRules( CONFIG_TYPE_BMM )
{
	if ( config.record.time == DEFAULT_TIME ) {
		config.record.time = 0.0f;
	}

	if ( !gmsgTimerValue ) {
		gmsgTimerDeact = REG_USER_MSG( "TimerDeact", 0 );
		gmsgTimerValue = REG_USER_MSG( "TimerValue", 4 );
		gmsgTimerCheat = REG_USER_MSG( "TimerCheat", 0 );
	}
}

void CBlackMesaMinute::PlayerSpawn( CBasePlayer *pPlayer )
{
	CCustomGameModeRules::PlayerSpawn( pPlayer );

	pPlayer->activeGameMode = GAME_MODE_BMM;
	pPlayer->time = 60.0f;
	pPlayer->timerBackwards = true;
}

void CBlackMesaMinute::PlayerThink( CBasePlayer *pPlayer )
{
	CCustomGameModeRules::PlayerThink( pPlayer );

	if ( pPlayer->time <= 0.0f && pPlayer->pev->deadflag == DEAD_NO ) {
		ClientKill( ENT( pPlayer->pev ) );
	}

	MESSAGE_BEGIN( MSG_ONE, gmsgTimerValue, NULL, pPlayer->pev );
		WRITE_FLOAT( pPlayer->time );
	MESSAGE_END();
}

void CBlackMesaMinute::OnCheated( CBasePlayer *pPlayer ) {
	CCustomGameModeRules::OnCheated( pPlayer );

	MESSAGE_BEGIN( MSG_ONE, gmsgTimerCheat, NULL, pPlayer->pev );
	MESSAGE_END();
}

void CBlackMesaMinute::OnKilledEntityByPlayer( CBasePlayer *pPlayer, CBaseEntity *victim, KILLED_ENTITY_TYPE killedEntity, BOOL isHeadshot, BOOL killedByExplosion, BOOL killedByCrowbar ) {

	int timeToAdd = 0;
	std::string message;

	if ( killedByExplosion ) {
		timeToAdd = 10;
		message = "EXPLOSION BONUS";
	} else if ( killedByCrowbar ) {
		timeToAdd = 10;
		message = "MELEE BONUS";
	} else if ( isHeadshot ) {
		timeToAdd = 6;
		message = "HEADSHOT BONUS";
	} else if ( killedEntity == KILLED_ENTITY_GRENADE ) {
		timeToAdd = 1;
		message = "PROJECTILE BONUS";
	} else {
		timeToAdd = 5;
		message = "TIME BONUS";
	}

	Vector deathPos = victim->pev->origin;
	deathPos.z += victim->pev->size.z + 5.0f;

	switch ( killedEntity ) {
		case KILLED_ENTITY_SENTRY:
			deathPos = victim->EyePosition() + Vector( 0, 0, 20 );
			break;

		case KILLED_ENTITY_SNIPER:
			deathPos = victim->pev->origin + ( victim->pev->mins + victim->pev->maxs ) * 0.5;
			break;

		case KILLED_ENTITY_NIHILANTH_CRYSTAL:
			deathPos = victim->pev->origin + ( victim->pev->mins + victim->pev->maxs ) * 0.5;
			timeToAdd = 10;
			break;

		case KILLED_ENTITY_GONARCH_SACK:
			timeToAdd = 10;
			break;

		case KILLED_ENTITY_SCIENTIST:
		case KILLED_ENTITY_BARNEY:
			timeToAdd = 0;

		default:
			break;
	}

	IncreaseTime( pPlayer, deathPos, timeToAdd, message.c_str() );

	CCustomGameModeRules::OnKilledEntityByPlayer( pPlayer, victim, killedEntity, isHeadshot, killedByExplosion, killedByCrowbar );
}

void CBlackMesaMinute::IncreaseTime( CBasePlayer *pPlayer, const Vector &eventPos, int timeToAdd, const char *message )
{
	if ( pPlayer->timerPaused || timeToAdd <= 0 ) {
		return;
	}

	pPlayer->time += timeToAdd;

	SendGameLogMessage( pPlayer, message );

	char timeAddedCString[6];
	sprintf( timeAddedCString, "00:%02d", timeToAdd ); // mm:ss
	const std::string timeAddedString = std::string( timeAddedCString );

	SendGameLogWorldMessage( pPlayer, eventPos, timeAddedString );
}

void CBlackMesaMinute::OnEnd( CBasePlayer *pPlayer ) {
	MESSAGE_BEGIN( MSG_ONE, gmsgTimerDeact, NULL, pPlayer->pev );
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_ONE, gmsgEndTime, NULL, pPlayer->pev );
		WRITE_STRING( "TIME SCORE|PERSONAL BESTS" );
		WRITE_FLOAT( pPlayer->time );
		WRITE_FLOAT( config.record.time );
		WRITE_BYTE( pPlayer->time > config.record.time );
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_ONE, gmsgEndTime, NULL, pPlayer->pev );
		WRITE_STRING( "REAL TIME" );
		WRITE_FLOAT( pPlayer->realTime );
		WRITE_FLOAT( config.record.realTime );
		WRITE_BYTE( pPlayer->realTime < config.record.realTime );
	MESSAGE_END();

	float realTimeMinusTime = max( 0.0f, pPlayer->realTime - pPlayer->time );
	MESSAGE_BEGIN( MSG_ONE, gmsgEndTime, NULL, pPlayer->pev );
		WRITE_STRING( "REAL TIME MINUS SCORE" );
		WRITE_FLOAT( realTimeMinusTime );
		WRITE_FLOAT( config.record.realTimeMinusTime );
		WRITE_BYTE( realTimeMinusTime < config.record.realTimeMinusTime );
	MESSAGE_END();

	CCustomGameModeRules::OnEnd( pPlayer );
}

void CBlackMesaMinute::OnHookedModelIndex( CBasePlayer *pPlayer, edict_t *activator, int modelIndex, const std::string &targetName )
{
	CCustomGameModeRules::OnHookedModelIndex( pPlayer, activator, modelIndex, targetName );

	bool wasConstant = false;
	std::string key = STRING( gpGlobals->mapname ) + std::to_string( modelIndex ) + targetName;
	bool modelIndexHasBeenHooked = pPlayer->ModelIndexHasBeenHooked( key.c_str() );

	// Does timer_pauses section contain such index?
	if ( config.MarkModelIndex( CONFIG_FILE_SECTION_TIMER_PAUSE, std::string( STRING( gpGlobals->mapname ) ), modelIndex, targetName, &wasConstant ) ) {
		if ( wasConstant || !modelIndexHasBeenHooked ) {
			PauseTimer( pPlayer );
		}
	}

	// Does timer_resume section contain such index?
	if ( config.MarkModelIndex( CONFIG_FILE_SECTION_TIMER_RESUME, std::string( STRING( gpGlobals->mapname ) ), modelIndex, targetName, &wasConstant ) ) {
		if ( wasConstant || !modelIndexHasBeenHooked ) {
			ResumeTimer( pPlayer );
		}
	}

	if ( !modelIndexHasBeenHooked ) {
		pPlayer->RememberHookedModelIndex( ALLOC_STRING( key.c_str() ) ); // memory leak
	}
}