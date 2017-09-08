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

extern int gmsgTimerValue;
extern int gmsgTimerCheat;
extern int gmsgTimerEnd;
extern int gmsgTimerMsg;

CBlackMesaMinute::CBlackMesaMinute() : CCustomGameModeRules( CONFIG_TYPE_BMM )
{
	time = 60.0f;
	timerBackwards = true;
}

void CBlackMesaMinute::PlayerSpawn( CBasePlayer *pPlayer )
{
	CCustomGameModeRules::PlayerSpawn( pPlayer );

	pPlayer->activeGameMode = GAME_MODE_BMM;
}

void CBlackMesaMinute::PlayerThink( CBasePlayer *pPlayer )
{
	CCustomGameModeRules::PlayerThink( pPlayer );

	if ( time <= 0.0f && pPlayer->pev->deadflag == DEAD_NO ) {
		ClientKill( ENT( pPlayer->pev ) );
	}

	MESSAGE_BEGIN( MSG_ONE, gmsgTimerValue, NULL, pPlayer->pev );
		WRITE_FLOAT( time );
	MESSAGE_END();
}

void CBlackMesaMinute::OnCheated( CBasePlayer *pPlayer ) {
	CCustomGameModeRules::OnCheated( pPlayer );

	MESSAGE_BEGIN( MSG_ONE, gmsgTimerCheat, NULL, pPlayer->pev );
	MESSAGE_END();
}

void CBlackMesaMinute::OnKilledEntityByPlayer( CBasePlayer *pPlayer, CBaseEntity *victim, KILLED_ENTITY_TYPE killedEntity, BOOL isHeadshot, BOOL killedByExplosion, BOOL killedByCrowbar ) {
	CCustomGameModeRules::OnKilledEntityByPlayer( pPlayer, victim, killedEntity, isHeadshot, killedByExplosion, killedByCrowbar );

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
}

void CBlackMesaMinute::IncreaseTime( CBasePlayer *pPlayer, const Vector &eventPos, int timeToAdd, const char *message )
{
	if ( timerPaused || timeToAdd <= 0 ) {
		return;
	}

	time += timeToAdd;

	MESSAGE_BEGIN( MSG_ONE, gmsgTimerMsg, NULL, pPlayer->pev );
		WRITE_STRING( message );
		WRITE_LONG( timeToAdd );
		WRITE_COORD( eventPos.x );
		WRITE_COORD( eventPos.y );
		WRITE_COORD( eventPos.z );
	MESSAGE_END();
}

void CBlackMesaMinute::OnEnd( CBasePlayer *pPlayer ) {

	PauseTimer( pPlayer );

	RecordRead();
	const std::string configName = config.GetName();
	
	MESSAGE_BEGIN( MSG_ONE, gmsgTimerEnd, NULL, pPlayer->pev );
	
		WRITE_STRING( configName.c_str() );

		WRITE_FLOAT( time );
		WRITE_FLOAT( realTime );

		WRITE_FLOAT( recordTime );
		WRITE_FLOAT( recordRealTime );
		WRITE_FLOAT( recordRealTimeMinusTime );

		WRITE_FLOAT( pPlayer->secondsInSlowmotion );
		WRITE_SHORT( pPlayer->kills );
		WRITE_SHORT( pPlayer->headshotKills );
		WRITE_SHORT( pPlayer->explosiveKills );
		WRITE_SHORT( pPlayer->crowbarKills );
		WRITE_SHORT( pPlayer->projectileKills );
		
	MESSAGE_END();

	if ( !cheated ) {

		// Write new records if there are
		if ( time > recordTime ) {
			recordTime = time;
		}
		if ( realTime < recordRealTime ) {
			recordRealTime = realTime;
		}

		float realTimeMinusTime = max( 0.0f, realTime - time );
		if ( realTimeMinusTime < recordRealTimeMinusTime ) {
			recordRealTimeMinusTime = realTimeMinusTime;
		}

		RecordSave();
	}
}

void CBlackMesaMinute::OnHookedModelIndex( CBasePlayer *pPlayer, edict_t *activator, int modelIndex, const std::string &targetName )
{
	CCustomGameModeRules::OnHookedModelIndex( pPlayer, activator, modelIndex, targetName );

	// Does timer_pauses section contain such index?
	if ( config.MarkModelIndex( CONFIG_FILE_SECTION_TIMER_PAUSE, std::string( STRING( gpGlobals->mapname ) ), modelIndex, targetName ) ) {
		PauseTimer( pPlayer );
	}

	// Does timer_resume section contain such index?
	if ( config.MarkModelIndex( CONFIG_FILE_SECTION_TIMER_RESUME, std::string( STRING( gpGlobals->mapname ) ), modelIndex, targetName ) ) {
		ResumeTimer( pPlayer );
	}
}

void CBlackMesaMinute::RecordAdditionalDefaultInit() {
	recordRealTimeMinusTime = DEFAULT_TIME;
};

void CBlackMesaMinute::RecordAdditionalRead( std::ifstream &inp ) {
	inp.read( ( char * ) &recordRealTimeMinusTime, sizeof( float ) );
};

void CBlackMesaMinute::RecordAdditionalWrite( std::ofstream &out ) {
	out.write( ( char * ) &recordRealTimeMinusTime, sizeof( float ) );
};