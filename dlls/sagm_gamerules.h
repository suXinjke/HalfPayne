#ifndef SAGM_GAMERULES_H
#define SAGM_GAMERULES_H

#include "cgm_gamerules.h"

class ScoreAttackRecord
{
public:
	ScoreAttackRecord( const char *recordName );
	void Save();

	int score;

private:
	const float DEFAULT_TIME = 59999.0f; // 999:59.00
	std::string filePath;
};

// CBlackMesaMinute - rules for time/score attack gamemode
class CScoreAttack : public CCustomGameModeRules {

public:
	CScoreAttack();

	virtual void PlayerSpawn( CBasePlayer *pPlayer );
	virtual void PlayerThink( CBasePlayer *pPlayer );

	virtual void OnKilledEntityByPlayer( CBasePlayer *pPlayer, CBaseEntity *victim, KILLED_ENTITY_TYPE killedEntity, BOOL isHeadshot, BOOL killedByExplosion, BOOL killedByCrowbar );

	virtual void OnCheated( CBasePlayer *pPlayer );

	int currentScore;

	int comboMultiplier;
	float comboMultiplierReset;

protected:
	virtual void OnEnd( CBasePlayer *pPlayer );
};

#endif // SAGM_GAMERULES_H