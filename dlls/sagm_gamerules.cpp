#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"player.h"
#include	"client.h"
#include	"weapons.h"
#include	"sagm_gamerules.h"
#include	<algorithm>
#include <fstream>
#include	"monsters.h"

#define COMBO_MULTIPLIER_DECAY_TIME 8.0f;

int	gmsgScoreMsg	= 0;
int	gmsgScoreCombo	= 0;
int	gmsgScoreEnd	= 0;
int gmsgScoreValue	= 0;
int gmsgScoreCheat	= 0;

CScoreAttack::CScoreAttack() : CCustomGameModeRules( CONFIG_TYPE_SAGM )
{
	if ( !gmsgScoreMsg ) {

		gmsgScoreMsg = REG_USER_MSG( "ScoreMsg", -1 );
		gmsgScoreEnd = REG_USER_MSG( "ScoreEnd", -1 );
		gmsgScoreValue = REG_USER_MSG( "ScoreValue", 12 );
		gmsgScoreCheat = REG_USER_MSG( "ScoreCheat", 0 );
	}
}

void CScoreAttack::PlayerSpawn( CBasePlayer *pPlayer )
{
	CCustomGameModeRules::PlayerSpawn( pPlayer );

	pPlayer->activeGameMode = GAME_MODE_SCORE_ATTACK;
}

void CScoreAttack::PlayerThink( CBasePlayer *pPlayer )
{
	CCustomGameModeRules::PlayerThink( pPlayer );

	pPlayer->comboMultiplierReset -= timeDelta;
	if ( pPlayer->comboMultiplierReset < 0.0f ) {
		pPlayer->comboMultiplierReset = 0.0f;
		pPlayer->comboMultiplier = 1;
	}

	MESSAGE_BEGIN( MSG_ONE, gmsgScoreValue, NULL, pPlayer->pev );
		WRITE_LONG( pPlayer->score );
		WRITE_LONG( pPlayer->comboMultiplier );
		WRITE_FLOAT( pPlayer->comboMultiplierReset );
	MESSAGE_END();
}

void CScoreAttack::OnCheated( CBasePlayer *pPlayer ) {
	CCustomGameModeRules::OnCheated( pPlayer );

	MESSAGE_BEGIN( MSG_ONE, gmsgScoreCheat, NULL, pPlayer->pev );
	MESSAGE_END();
}

void CScoreAttack::OnKilledEntityByPlayer( CBasePlayer *pPlayer, CBaseEntity *victim, KILLED_ENTITY_TYPE killedEntity, BOOL isHeadshot, BOOL killedByExplosion, BOOL killedByCrowbar ) {

	if ( pPlayer->pev->deadflag != DEAD_NO ) {
		return;
	}

	int scoreToAdd = -1;
	float additionalMultiplier = 0.0f;

	switch ( killedEntity ) {
		case KILLED_ENTITY_BIG_MOMMA:
		case KILLED_ENTITY_GARGANTUA:
		case KILLED_ENTITY_ARMORED_VEHICLE:
		case KILLED_ENTITY_ICHTYOSAUR:
		case KILLED_ENTITY_NIHILANTH_CRYSTAL:
		case KILLED_ENTITY_GONARCH_SACK:
			scoreToAdd = 1000;
			break;

		case KILLED_ENTITY_ALIEN_CONTROLLER:
		case KILLED_ENTITY_ALIEN_GRUNT:
		case KILLED_ENTITY_HUMAN_ASSASSIN:
		case KILLED_ENTITY_HUMAN_GRUNT:
		case KILLED_ENTITY_SNIPER:
		case KILLED_ENTITY_ALIEN_SLAVE:
			scoreToAdd = 100;
			break;

		case KILLED_ENTITY_MINITURRET:
		case KILLED_ENTITY_SENTRY:
			scoreToAdd = 50;
			break;

		case KILLED_ENTITY_BULLSQUID:
		case KILLED_ENTITY_ZOMBIE:
			scoreToAdd = 30;
			break;

		case KILLED_ENTITY_HEADCRAB:
		case KILLED_ENTITY_HOUNDEYE:
		case KILLED_ENTITY_SNARK:
			scoreToAdd = 20;
			break;

		case KILLED_ENTITY_BARNACLE:
			scoreToAdd = 5;
			break;

		case KILLED_ENTITY_BABYCRAB:
		case KILLED_ENTITY_GRENADE:
			scoreToAdd = 0;
			break;

		default:
			return;
	}

	std::string message;

	if ( killedByExplosion ) {
		message = "EXPLOSION BONUS";
		additionalMultiplier = 1.0f;
	} else if ( killedByCrowbar ) {
		message = "MELEE BONUS";
		additionalMultiplier = 1.0f;
	} else if ( isHeadshot ) {
		message = "HEADSHOT BONUS";
		additionalMultiplier = 0.5f;
	} else if ( killedEntity == KILLED_ENTITY_GRENADE ) {
		message = "PROJECTILE BONUS";
	} else {
		message = "SCORE BONUS";
	}

	Vector deathPos = victim->pev->origin;
	deathPos.z += victim->pev->size.z + 5.0f;

	switch ( killedEntity ) {
		case KILLED_ENTITY_SENTRY:
			deathPos = victim->EyePosition() + Vector( 0, 0, 20 );
			break;

		case KILLED_ENTITY_SNIPER:
			deathPos = victim->pev->origin + ( victim->pev->mins + victim->pev->maxs ) * 0.5;
			break;

		case KILLED_ENTITY_NIHILANTH_CRYSTAL:
			deathPos = victim->pev->origin + ( victim->pev->mins + victim->pev->maxs ) * 0.5;
			break;

		default:
			break;
	}

	pPlayer->score += scoreToAdd * ( pPlayer->comboMultiplier + additionalMultiplier );

	if ( scoreToAdd != -1 ) {

		if ( scoreToAdd > 0 ) {
			MESSAGE_BEGIN( MSG_ONE, gmsgScoreMsg, NULL, pPlayer->pev );
				WRITE_STRING( message.c_str() );
				WRITE_LONG( scoreToAdd );
				WRITE_FLOAT( pPlayer->comboMultiplier + additionalMultiplier );
				WRITE_COORD( deathPos.x );
				WRITE_COORD( deathPos.y );
				WRITE_COORD( deathPos.z );
			MESSAGE_END();
		}

		switch ( killedEntity ) {
			case KILLED_ENTITY_BABYCRAB:
			case KILLED_ENTITY_BARNACLE:
			case KILLED_ENTITY_GRENADE:
				// don't add to combo multiplier
				break;

			default:
				pPlayer->comboMultiplier++;
				break;
		}

		pPlayer->comboMultiplierReset = COMBO_MULTIPLIER_DECAY_TIME;
	}

	CCustomGameModeRules::OnKilledEntityByPlayer( pPlayer, victim, killedEntity, isHeadshot, killedByExplosion, killedByCrowbar );
}

void CScoreAttack::OnEnd( CBasePlayer *pPlayer ) {

	const std::string configName = config.GetName();

	MESSAGE_BEGIN( MSG_ONE, gmsgScoreEnd, NULL, pPlayer->pev );

		WRITE_STRING( configName.c_str() );

		WRITE_LONG( pPlayer->score );

		WRITE_LONG( config.record.score );

		WRITE_FLOAT( pPlayer->secondsInSlowmotion );
		WRITE_SHORT( pPlayer->kills );
		WRITE_SHORT( pPlayer->headshotKills );
		WRITE_SHORT( pPlayer->explosiveKills );
		WRITE_SHORT( pPlayer->crowbarKills );
		WRITE_SHORT( pPlayer->projectileKills );

	MESSAGE_END();

	if ( !pPlayer->cheated ) {
		config.record.Save( pPlayer );
	}
}