#ifndef CGM_GAMERULES_H
#define CGM_GAMERULES_H

#include "custom_gamemode_config.h"
#include "gamerules.h"
#include	<fstream>

// CCustomGameMode - Half-Life with additional mini-mods
class CCustomGameModeRules : public CHalfLifeRules {

public:
	CCustomGameModeRules( CONFIG_TYPE configType = CONFIG_TYPE_CGM );

	virtual void OnChangeLevel();
	virtual void PlayerSpawn( CBasePlayer *pPlayer );
	virtual void OnNewlyVisitedMap();
	virtual void RefreshSkillData();
	virtual void PlayerThink( CBasePlayer *pPlayer );
	virtual void OnKilledEntityByPlayer( CBasePlayer *pPlayer, CBaseEntity *victim, KILLED_ENTITY_TYPE killedEntity, BOOL isHeadshot, BOOL killedByExplosion, BOOL killedByCrowbar );

	virtual void OnHookedModelIndex( CBasePlayer *pPlayer, edict_t *activator, int edictIndex, const std::string &targetName );

	virtual void SpawnEnemiesByConfig( const char *mapName );

	virtual BOOL CanHavePlayerItem( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon );
	bool ChangeLevelShouldBePrevented( const char *nextMap );

	virtual void CheckForCheats( CBasePlayer *pPlayer );
	virtual void OnCheated( CBasePlayer *pPlayer );

	virtual void Precache();

	virtual void RestartGame();

	virtual void PauseTimer( CBasePlayer *pPlayer );
	virtual void ResumeTimer( CBasePlayer *pPlayer );

	bool cheated;
	bool cheatedMessageSent;

	float timeDelta;
	float lastGlobalTime;
	float musicSwitchDelay;

	bool monsterSpawnPrevented;

	CustomGameModeConfig config;

	bool timerBackwards;
	bool timerPaused;
	float time;
	float realTime;
	float lastRealTime;

	float recordTime;
	float recordRealTime;

protected:
	void RecordRead();
	void RecordSave();
	virtual void RecordAdditionalDefaultInit() {};
	virtual void RecordAdditionalRead( std::ifstream &inp ) {};
	virtual void RecordAdditionalWrite( std::ofstream &out ) {};

	const float DEFAULT_TIME = 59999.0f; // 999:59.00

private:
	virtual void OnEnd( CBasePlayer *pPlayer );
};


#endif // CGM_GAMERULES_H