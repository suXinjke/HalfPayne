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
#include	"gameplay_mod.h"

#define COMBO_MULTIPLIER_DECAY_TIME 8.0f;
extern int gmsgScoreCheat;
extern int gmsgScoreDeact;

extern int gmsgEndScore;

CScoreAttack::CScoreAttack() : CCustomGameModeRules( CONFIG_TYPE_SAGM )
{}

void CScoreAttack::PlayerSpawn( CBasePlayer *pPlayer )
{
	CCustomGameModeRules::PlayerSpawn( pPlayer );

	gameplayMods.activeGameMode = GAME_MODE_SCORE_ATTACK;
}

void CScoreAttack::PlayerThink( CBasePlayer *pPlayer )
{
	CCustomGameModeRules::PlayerThink( pPlayer );

	gameplayMods.comboMultiplierReset -= timeDelta;
	if ( gameplayMods.comboMultiplierReset < 0.0f ) {
		gameplayMods.comboMultiplierReset = 0.0f;
		gameplayMods.comboMultiplier = 1;
	}
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

	gameplayMods.score += scoreToAdd * ( gameplayMods.comboMultiplier + additionalMultiplier );

	if ( scoreToAdd != -1 ) {

		if ( scoreToAdd > 0 ) {
			SendGameLogMessage( pPlayer, message );

			float comboMultiplier = gameplayMods.comboMultiplier + additionalMultiplier;
			bool comboMultiplierIsInteger = abs( comboMultiplier - std::lround( comboMultiplier ) ) < 0.00000001f;

			char upperString[128];
			if ( comboMultiplierIsInteger ) {
				std::sprintf( upperString, "%d x%.0f", scoreToAdd, comboMultiplier );
			} else {
				std::sprintf( upperString, "%d x%.1f", scoreToAdd, comboMultiplier );
			}

			SendGameLogWorldMessage( pPlayer, deathPos, std::to_string( ( int ) ( scoreToAdd * comboMultiplier ) ), upperString );
		}

		switch ( killedEntity ) {
			case KILLED_ENTITY_BABYCRAB:
			case KILLED_ENTITY_BARNACLE:
			case KILLED_ENTITY_GRENADE:
				// don't add to combo multiplier
				break;

			default:
				gameplayMods.comboMultiplier++;
				break;
		}

		gameplayMods.comboMultiplierReset = COMBO_MULTIPLIER_DECAY_TIME;
	}

	CCustomGameModeRules::OnKilledEntityByPlayer( pPlayer, victim, killedEntity, isHeadshot, killedByExplosion, killedByCrowbar );
}

void CScoreAttack::OnEnd( CBasePlayer *pPlayer ) {
	MESSAGE_BEGIN( MSG_ONE, gmsgScoreDeact, NULL, pPlayer->pev );
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_ONE, gmsgEndScore, NULL, pPlayer->pev );
		WRITE_STRING( "SCORE|PERSONAL BEST" );
		WRITE_LONG( gameplayMods.score );
		WRITE_LONG( config.record.score );
		WRITE_BYTE( gameplayMods.score > config.record.score );
	MESSAGE_END();

	CCustomGameModeRules::OnEnd( pPlayer );
}