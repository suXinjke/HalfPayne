#ifndef BMM_GAMERULES_H
#define BMM_GAMERULES_H

#include "cgm_gamerules.h"

// CBlackMesaMinute - rules for time/score attack gamemode
class CBlackMesaMinute : public CCustomGameModeRules {

public:
	CBlackMesaMinute();

	virtual void PlayerSpawn( CBasePlayer *pPlayer );
	virtual void PlayerThink( CBasePlayer *pPlayer );

	virtual void IncreaseTime( CBasePlayer *pPlayer, const Vector &eventPos, int seconds, const char *message );
	virtual void OnKilledEntityByPlayer( CBasePlayer *pPlayer, CBaseEntity *victim, KILLED_ENTITY_TYPE killedEntity, BOOL isHeadshot, BOOL killedByExplosion, BOOL killedByCrowbar );
	
	virtual void OnCheated( CBasePlayer *pPlayer );

	virtual void OnHookedModelIndex( CBasePlayer *pPlayer, edict_t *activator, int edictIndex, const std::string &targetName );
	
	float recordRealTimeMinusTime;

protected:
	virtual void OnEnd( CBasePlayer *pPlayer );

	virtual void RecordAdditionalDefaultInit() override;
	virtual void RecordAdditionalRead( std::ifstream &inp ) override;
	virtual void RecordAdditionalWrite( std::ofstream &out ) override;
};

#endif // BMM_GAMERULES_H