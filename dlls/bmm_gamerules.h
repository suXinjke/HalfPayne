#ifndef BMM_GAMERULES_H
#define BMM_GAMERULES_H

#include "cgm_gamerules.h"

class BlackMesaMinuteRecord
{
public:
	BlackMesaMinuteRecord( const char *recordName );
	void Save();

	float time;
	float realTime;
	float realTimeMinusTime;

private:
	const float DEFAULT_TIME = 59999.0f; // 999:59.00
	std::string filePath;
};

// CBlackMesaMinute - rules for time/score attack gamemode
class CBlackMesaMinute : public CCustomGameModeRules {

public:
	CBlackMesaMinute();

	virtual void PlayerSpawn( CBasePlayer *pPlayer );
	virtual void PlayerThink( CBasePlayer *pPlayer );

	virtual void IncreaseTime( CBasePlayer *pPlayer, const Vector &eventPos, bool isHeadshot, bool killedByExplosion, bool destroyedGrenade, bool killedByCrowbar );
	virtual void IncreaseTime( CBasePlayer *pPlayer, const Vector &eventPos, int seconds, const char *message );
	virtual void End( CBasePlayer *pPlayer );

	virtual void PauseTimer( CBasePlayer *pPlayer );
	virtual void ResumeTimer( CBasePlayer *pPlayer );

	virtual void HookModelIndex( edict_t *activator, const char *mapName, int modelIndex );

	virtual void CheckForCheats( CBasePlayer *pPlayer );

	bool timerPaused;
	bool ended;

	float currentTime;
	float currentRealTime;
	float lastRealTime;
};

#endif // BMM_GAMERULES_H