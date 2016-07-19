#ifndef BMM_GAMERULES_H
#define BMM_GAMERULES_H

#include "bmm_config.h"
#include "gamerules.h"

// CBlackMesaMinute - rules for time/score attack gamemode
class CBlackMesaMinute : public CHalfLifeRules {

public:
	CBlackMesaMinute();

	virtual BOOL ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[128] );
	virtual void PlayerSpawn( CBasePlayer *pPlayer );
	virtual void RefreshSkillData();
	virtual void PlayerThink( CBasePlayer *pPlayer );

	virtual void IncreaseTime( CBasePlayer *pPlayer, const Vector &eventPos, bool isHeadshot, bool killedByExplosion, bool destroyedGrenade, bool killedByCrowbar );
	virtual void End( CBasePlayer *pPlayer );

	virtual void PauseTimer( CBasePlayer *pPlayer );
	virtual void ResumeTimer( CBasePlayer *pPlayer );

	virtual void HookModelIndex( edict_t *activator, const char *mapName, int modelIndex );

	virtual void SpawnEnemiesByConfig( const char *mapName );

	virtual BOOL CanHavePlayerItem( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon );
};

namespace BMM
{
	extern bool timerPaused;
	extern bool ended;

	extern float currentTime;
	extern float currentRealTime;
	extern float lastGlobalTime;
	extern float lastRealTime;

	extern int kills;
	extern int headshotKills;
	extern int explosiveKills;
	extern int crowbarKills;
	extern int projectileKills;
	extern float secondsInSlowmotion;
};

extern BlackMesaMinuteConfig gBMMConfig;

#endif // BMM_GAMERULES_H