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

protected:
	virtual void OnEnd( CBasePlayer *pPlayer );
};

#endif // SAGM_GAMERULES_H