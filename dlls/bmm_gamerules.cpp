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

int	gmsgTimerMsg	= 0;
int	gmsgTimerEnd	= 0;
int gmsgTimerValue	= 0;
int gmsgTimerPause  = 0;
int gmsgTimerCheat  = 0;

CBlackMesaMinute::CBlackMesaMinute() : CCustomGameModeRules( CONFIG_TYPE_BMM )
{
	if ( !gmsgTimerMsg ) {
		gmsgTimerMsg = REG_USER_MSG( "TimerMsg", -1 );
		gmsgTimerEnd = REG_USER_MSG( "TimerEnd", -1 );
		gmsgTimerValue = REG_USER_MSG( "TimerValue", 4 );
		gmsgTimerPause = REG_USER_MSG( "TimerPause", 1 );
		gmsgTimerCheat = REG_USER_MSG( "TimerCheat", 0 );
	}

	timerPaused = false;

	currentTime = 60.0f;
	currentRealTime = 0.0f;
	lastRealTime = 0.0f;
}

void CBlackMesaMinute::PlayerSpawn( CBasePlayer *pPlayer )
{
	CCustomGameModeRules::PlayerSpawn( pPlayer );

	pPlayer->activeGameMode = GAME_MODE_BMM;
	pPlayer->noSaving = true;

}

void CBlackMesaMinute::PlayerThink( CBasePlayer *pPlayer )
{
	CCustomGameModeRules::PlayerThink( pPlayer );

	if ( !timerPaused && !UTIL_IsPaused() && pPlayer->pev->deadflag == DEAD_NO ) {
		
		if ( fabs( timeDelta ) <= 0.1 ) {
			currentTime -= timeDelta;
		}
		
		// Counting real time
		float realTimeDetla = ( g_engfuncs.pfnTime() - lastRealTime );
		
		lastRealTime = g_engfuncs.pfnTime();

		if ( fabs( realTimeDetla ) <= 0.1 ) {
			currentRealTime += realTimeDetla;
		}
	}

	if ( currentTime <= 0.0f && pPlayer->pev->deadflag == DEAD_NO ) {
		ClientKill( ENT( pPlayer->pev ) );
	}

	MESSAGE_BEGIN( MSG_ONE, gmsgTimerValue, NULL, pPlayer->pev );
		WRITE_FLOAT( currentTime );
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

	currentTime += timeToAdd;

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

	BlackMesaMinuteRecord record( config.configName.c_str() );
	const std::string configName = config.GetName();
	
	MESSAGE_BEGIN( MSG_ONE, gmsgTimerEnd, NULL, pPlayer->pev );
	
		WRITE_STRING( configName.c_str() );

		WRITE_FLOAT( currentTime );
		WRITE_FLOAT( currentRealTime );

		WRITE_FLOAT( record.time );
		WRITE_FLOAT( record.realTime );
		WRITE_FLOAT( record.realTimeMinusTime );

		WRITE_FLOAT( pPlayer->secondsInSlowmotion );
		WRITE_SHORT( pPlayer->kills );
		WRITE_SHORT( pPlayer->headshotKills );
		WRITE_SHORT( pPlayer->explosiveKills );
		WRITE_SHORT( pPlayer->crowbarKills );
		WRITE_SHORT( pPlayer->projectileKills );
		
	MESSAGE_END();

	if ( !cheated ) {

		// Write new records if there are
		if ( currentTime > record.time ) {
			record.time = currentTime;
		}
		if ( currentRealTime < record.realTime ) {
			record.realTime = currentRealTime;
		}

		float bmmRealTimeMinusTime = max( 0.0f, currentRealTime - currentTime );
		if ( bmmRealTimeMinusTime < record.realTimeMinusTime ) {
			record.realTimeMinusTime = bmmRealTimeMinusTime;
		}

		record.Save();
	}
}

void CBlackMesaMinute::PauseTimer( CBasePlayer *pPlayer )
{
	if ( timerPaused ) {
		return;
	}
	
	timerPaused = true;
	
	MESSAGE_BEGIN( MSG_ONE, gmsgTimerPause, NULL, pPlayer->pev );
		WRITE_BYTE( true );
	MESSAGE_END();
}

void CBlackMesaMinute::ResumeTimer( CBasePlayer *pPlayer )
{
	if ( !timerPaused ) {
		return;
	}

	timerPaused = false;
	
	MESSAGE_BEGIN( MSG_ONE, gmsgTimerPause, NULL, pPlayer->pev );
		WRITE_BYTE( false );
	MESSAGE_END();
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


BlackMesaMinuteRecord::BlackMesaMinuteRecord( const char *recordName ) {

	std::string folderPath = CustomGameModeConfig::GetGamePath() + "\\bmm_records\\";

	// Create bmm_records directory if it's not there. Proceed only when directory exists
	if ( CreateDirectory( folderPath.c_str(), NULL ) || GetLastError() == ERROR_ALREADY_EXISTS ) {
		filePath = folderPath + std::string( recordName ) + ".bmmr";

		std::ifstream inp( filePath, std::ios::in | std::ios::binary );
		if ( !inp.is_open() ) {
			time = 0.0f;
			realTime = DEFAULT_TIME;
			realTimeMinusTime = DEFAULT_TIME;

		} else {
			inp.read( ( char * ) &time, sizeof( float ) );
			inp.read( ( char * ) &realTime, sizeof( float ) );
			inp.read( ( char * ) &realTimeMinusTime, sizeof( float ) );
		}
	}
}

void BlackMesaMinuteRecord::Save() {
	std::ofstream out( filePath, std::ios::out | std::ios::binary );

	out.write( (char *) &time, sizeof( float ) );
	out.write( (char *) &realTime, sizeof( float ) );
	out.write( (char *) &realTimeMinusTime, sizeof( float ) );

	out.close();
}