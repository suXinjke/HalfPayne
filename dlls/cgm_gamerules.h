#ifndef CGM_GAMERULES_H
#define CGM_GAMERULES_H

#include "custom_gamemode_config.h"
#include "gamerules.h"

// CCustomGameMode - Half-Life with additional mini-mods
class CCustomGameModeRules : public CHalfLifeRules {

public:
	CCustomGameModeRules( const char *configFolder = "cgm_cfg" );

	virtual BOOL ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[128] );
	virtual void PlayerSpawn( CBasePlayer *pPlayer );
	virtual void OnChangeLevel();
	virtual void RefreshSkillData();
	virtual void PlayerThink( CBasePlayer *pPlayer );
	virtual void OnKilledEntityByPlayer( CBasePlayer *pPlayer, CBaseEntity *victim );

	virtual void End( CBasePlayer *pPlayer );

	virtual void HookModelIndex( edict_t *activator, const char *mapName, int modelIndex );

	virtual void SpawnEnemiesByConfig( const char *mapName );

	virtual BOOL CanHavePlayerItem( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon );

	virtual void CheckForCheats( CBasePlayer *pPlayer );
	virtual void OnCheated( CBasePlayer *pPlayer );

	virtual void Precache();

	virtual void RestartGame();

	bool ended;

	bool cheated;
	bool cheatedMessageSent;

	float timeDelta;
	float lastGlobalTime;

	int kills;
	int headshotKills;
	int explosiveKills;
	int crowbarKills;
	int projectileKills;
	float secondsInSlowmotion;

	CustomGameModeConfig config;

protected:
	virtual void OnEnd( CBasePlayer *pPlayer );
};

#endif // CGM_GAMERULES_H