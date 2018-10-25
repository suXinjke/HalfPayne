#ifndef CGM_GAMERULES_H
#define CGM_GAMERULES_H

#include "custom_gamemode_config.h"
#include "gamerules.h"

class EntityRandomSpawnerController {
public:
	EntityRandomSpawnerController( const EntityRandomSpawner &entityRandomSpawner );
	void Think( CBasePlayer *pPlayer );
	void ResetSpawnPeriod();

private:
	void Spawn( CBasePlayer *pPlayer );

	EntitySpawnData spawnData;
	int maxAmount;
	float spawnPeriod;
	float nextSpawn;
	
};

// CCustomGameMode - Half-Life with additional mini-mods
class CCustomGameModeRules : public CHalfLifeRules {

public:
	CCustomGameModeRules( CONFIG_TYPE configType = CONFIG_TYPE_CGM );

	virtual void OnChangeLevel();
	virtual void OnSave( CBasePlayer *pPlayer ) override;
	virtual void PlayerSpawn( CBasePlayer *pPlayer );
	virtual void RefreshSkillData();
	virtual void PlayerThink( CBasePlayer *pPlayer );
	virtual void OnKilledEntityByPlayer( CBasePlayer *pPlayer, CBaseEntity *victim, KILLED_ENTITY_TYPE killedEntity, BOOL isHeadshot, BOOL killedByExplosion, BOOL killedByCrowbar ) override;

	virtual void OnHookedModelIndex( CBasePlayer *pPlayer, CBaseEntity *activator, int modelIndex, const std::string &className, const std::string &targetName, bool firstTime );

	virtual BOOL CanHavePlayerItem( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon );

	virtual void ApplyStartPositionToEntity( CBaseEntity *entity, const StartPosition &startPosition );

	virtual void CheckForCheats( CBasePlayer *pPlayer );
	virtual void OnCheated( CBasePlayer *pPlayer );

	virtual void RestartGame();

	virtual void PauseTimer( CBasePlayer *pPlayer );
	virtual void ResumeTimer( CBasePlayer *pPlayer );
	virtual void IncreaseTime( CBasePlayer *pPlayer, const Vector &eventPos, int timeToAdd, const char *message );
	virtual void CalculateScoreForBlackMesaMinute( CBasePlayer *pPlayer, CBaseEntity *victim, KILLED_ENTITY_TYPE killedEntity, BOOL isHeadshot, BOOL killedByExplosion, BOOL killedByCrowbar );
	virtual void CalculateScoreForScoreAttack( CBasePlayer *pPlayer, CBaseEntity *victim, KILLED_ENTITY_TYPE killedEntity, BOOL isHeadshot, BOOL killedByExplosion, BOOL killedByCrowbar );

	void VoteForRandomGameplayMod( CBasePlayer *pPlayer, const std::string &voter, int modIndex );
	void VoteForRandomGameplayMod( CBasePlayer *pPlayer, const std::string &voter, const std::string &modIndex );

	virtual void ActivateEndMarkers( CBasePlayer *pPlayer = NULL );
	bool endMarkersActive;

	bool cheatedMessageSent;
	bool startMapDoesntMatch;

	float musicSwitchDelay;

	int yOffset;
	int maxYOffset;

	CustomGameModeConfig config;

	std::vector<EntityRandomSpawnerController> entityRandomSpawnerControllers;

protected:
	void SendGameLogMessage( CBasePlayer *pPlayer, const std::string &message, bool logToConsole = false );
	void SendGameLogWorldMessage( CBasePlayer *pPlayer, const Vector &location, const std::string &message, const std::string &message2 = "" );

	virtual void OnEnd( CBasePlayer *pPlayer );
};


#endif // CGM_GAMERULES_H