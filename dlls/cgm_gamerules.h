#ifndef CGM_GAMERULES_H
#define CGM_GAMERULES_H

#include "custom_gamemode_config.h"
#include "gamerules.h"

// CCustomGameMode - Half-Life with additional mini-mods
class CCustomGameModeRules : public CHalfLifeRules {

public:
	CCustomGameModeRules( CustomGameModeConfig::GAME_MODE_CONFIG_TYPE configType = CustomGameModeConfig::GAME_MODE_CONFIG_CGM );

	virtual void PlayerSpawn( CBasePlayer *pPlayer );
	virtual void OnChangeLevel();
	virtual void RefreshSkillData();
	virtual void PlayerThink( CBasePlayer *pPlayer );
	virtual void OnKilledEntityByPlayer( CBasePlayer *pPlayer, CBaseEntity *victim, KILLED_ENTITY_TYPE killedEntity, BOOL isHeadshot, BOOL killedByExplosion, BOOL killedByCrowbar );

	virtual void OnHookedModelIndex( CBasePlayer *pPlayer, edict_t *activator, int edictIndex, const std::string &targetName );

	virtual void SpawnEnemiesByConfig( const char *mapName );

	virtual BOOL CanHavePlayerItem( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon );

	virtual void CheckForCheats( CBasePlayer *pPlayer );
	virtual void OnCheated( CBasePlayer *pPlayer );

	virtual void Precache();

	virtual void RestartGame();

	bool cheated;
	bool cheatedMessageSent;

	float timeDelta;
	float lastGlobalTime;

	CustomGameModeConfig config;

private:
	virtual void OnEnd( CBasePlayer *pPlayer );
};

#endif // CGM_GAMERULES_H