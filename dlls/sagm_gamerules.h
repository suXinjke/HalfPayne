#ifndef SAGM_GAMERULES_H
#define SAGM_GAMERULES_H

#include "cgm_gamerules.h"

// CBlackMesaMinute - rules for time/score attack gamemode
class CScoreAttack : public CCustomGameModeRules {

public:
	CScoreAttack();

	virtual void PlayerSpawn( CBasePlayer *pPlayer );
	virtual void PlayerThink( CBasePlayer *pPlayer );

	virtual void OnKilledEntityByPlayer( CBasePlayer *pPlayer, CBaseEntity *victim, KILLED_ENTITY_TYPE killedEntity, BOOL isHeadshot, BOOL killedByExplosion, BOOL killedByCrowbar );

	virtual void OnCheated( CBasePlayer *pPlayer );

	int currentScore;
	int recordScore;

	int comboMultiplier;
	float comboMultiplierReset;

protected:
	virtual void OnEnd( CBasePlayer *pPlayer );

	virtual void RecordAdditionalDefaultInit() override;
	virtual void RecordAdditionalRead( std::ifstream &inp ) override;
	virtual void RecordAdditionalWrite( std::ofstream &out ) override;
};

#endif // SAGM_GAMERULES_H