#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"player.h"
#include	"client.h"
#include	"skill.h"
#include	"weapons.h"
#include	"bmm_gamerules.h"
#include	<algorithm>
#include <fstream>

// Black Mesa Minute

int	gmsgTimerMsg	= 0;
int	gmsgTimerEnd	= 0;
int gmsgTimerValue	= 0;
int gmsgTimerPause  = 0;
int gmsgTimerCheat  = 0;

CBlackMesaMinute::CBlackMesaMinute() : CCustomGameModeRules( "bmm_cfg" )
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

	pPlayer->bmmEnabled = 1;
	pPlayer->noSaving = true;

	if ( config.holdTimer ) {
		PauseTimer( pPlayer );
	}

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

void CBlackMesaMinute::IncreaseTime( CBasePlayer *pPlayer, const Vector &eventPos, bool isHeadshot, bool killedByExplosion, bool destroyedGrenade, bool killedByCrowbar ) {

	if ( timerPaused ) {
		return;
	}

	kills++;
		
	if ( killedByExplosion ) {
		int timeToAdd = TIMEATTACK_EXPLOSION_BONUS_TIME;
		currentTime += timeToAdd;
		explosiveKills++;

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
		currentTime += timeToAdd;
		crowbarKills++;

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
		currentTime += timeToAdd;
		headshotKills++;

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
		currentTime += timeToAdd;
		projectileKills++;

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
		currentTime += timeToAdd;

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
	if ( timerPaused ) {
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

void CBlackMesaMinute::End( CBasePlayer *pPlayer ) {
	if ( ended ) {
		return;
	}

	if ( pPlayer->slowMotionEnabled ) {
		pPlayer->ToggleSlowMotion();
	}

	ended = true;
	PauseTimer( pPlayer );

	pPlayer->pev->movetype = MOVETYPE_NONE;
	pPlayer->pev->flags |= FL_NOTARGET;
	pPlayer->RemoveAllItems( true );

	BlackMesaMinuteRecord record( config.configName.c_str() );
	
	MESSAGE_BEGIN( MSG_ONE, gmsgTimerEnd, NULL, pPlayer->pev );
	
		WRITE_STRING( config.name.c_str() );

		WRITE_FLOAT( currentTime );
		WRITE_FLOAT( currentRealTime );

		WRITE_FLOAT( record.time );
		WRITE_FLOAT( record.realTime );
		WRITE_FLOAT( record.realTimeMinusTime );

		WRITE_FLOAT( secondsInSlowmotion );
		WRITE_SHORT( kills );
		WRITE_SHORT( headshotKills );
		WRITE_SHORT( explosiveKills );
		WRITE_SHORT( crowbarKills );
		WRITE_SHORT( projectileKills );
		
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

void CBlackMesaMinute::HookModelIndex( edict_t *activator, const char *mapName, int modelIndex )
{
	CCustomGameModeRules::HookModelIndex( activator, mapName, modelIndex );

	CBasePlayer *pPlayer = ( CBasePlayer * ) CBasePlayer::Instance( g_engfuncs.pfnPEntityOfEntIndex( 1 ) );
	if ( !pPlayer ) {
		return;
	}

	ModelIndex indexToFind( mapName, modelIndex );

	// Does timerPauses contain such index?
	auto foundIndex = config.timerPauses.find( indexToFind ); // it's complex iterator type, so leave it auto
	if ( foundIndex != config.timerPauses.end() ) {
		bool constant = foundIndex->constant;

		if ( !constant ) {
			config.timerPauses.erase( foundIndex );
		}

		PauseTimer( pPlayer );
		return;
	}

	// Does timerResumes contain such index?
	foundIndex = config.timerResumes.find( indexToFind );
	if ( foundIndex != config.timerResumes.end() ) {
		bool constant = foundIndex->constant;

		if ( !constant ) {
			config.timerResumes.erase( foundIndex );
		}

		ResumeTimer( pPlayer );
		return;
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