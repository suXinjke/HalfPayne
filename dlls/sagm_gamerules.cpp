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

	currentScore = 0;

	comboMultiplier = 1;
	comboMultiplierReset = 0.0f;
}

void CScoreAttack::PlayerSpawn( CBasePlayer *pPlayer )
{
	CCustomGameModeRules::PlayerSpawn( pPlayer );

	pPlayer->activeGameMode = GAME_MODE_SCORE_ATTACK;
	pPlayer->noSaving = true;
}

void CScoreAttack::PlayerThink( CBasePlayer *pPlayer )
{
	CCustomGameModeRules::PlayerThink( pPlayer );

	comboMultiplierReset -= timeDelta;
	if ( comboMultiplierReset < 0.0f ) {
		comboMultiplierReset = 0.0f;
		comboMultiplier = 1;
	}

	MESSAGE_BEGIN( MSG_ONE, gmsgScoreValue, NULL, pPlayer->pev );
		WRITE_LONG( currentScore );
		WRITE_LONG( comboMultiplier );
		WRITE_FLOAT( comboMultiplierReset );
	MESSAGE_END();
}

void CScoreAttack::OnCheated( CBasePlayer *pPlayer ) {
	CCustomGameModeRules::OnCheated( pPlayer );

	MESSAGE_BEGIN( MSG_ONE, gmsgScoreCheat, NULL, pPlayer->pev );
	MESSAGE_END();
}

void CScoreAttack::OnKilledEntityByPlayer( CBasePlayer *pPlayer, CBaseEntity *victim, KILLED_ENTITY_TYPE killedEntity, BOOL isHeadshot, BOOL killedByExplosion, BOOL killedByCrowbar ) {
	CCustomGameModeRules::OnKilledEntityByPlayer( pPlayer, victim, killedEntity, isHeadshot, killedByExplosion, killedByCrowbar );

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

	currentScore += scoreToAdd * ( comboMultiplier + additionalMultiplier );

	if ( scoreToAdd != -1 ) {

		if ( scoreToAdd > 0 ) {
			MESSAGE_BEGIN( MSG_ONE, gmsgScoreMsg, NULL, pPlayer->pev );
				WRITE_STRING( message.c_str() );
				WRITE_LONG( scoreToAdd );
				WRITE_FLOAT( comboMultiplier + additionalMultiplier );
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
				comboMultiplier++;
				break;
		}

		comboMultiplierReset = COMBO_MULTIPLIER_DECAY_TIME;
	}
}

void CScoreAttack::OnEnd( CBasePlayer *pPlayer ) {

	RecordRead();
	const std::string configName = config.GetName();

	MESSAGE_BEGIN( MSG_ONE, gmsgScoreEnd, NULL, pPlayer->pev );

		WRITE_STRING( configName.c_str() );

		WRITE_LONG( currentScore );

		WRITE_LONG( recordScore );

		WRITE_FLOAT( pPlayer->secondsInSlowmotion );
		WRITE_SHORT( pPlayer->kills );
		WRITE_SHORT( pPlayer->headshotKills );
		WRITE_SHORT( pPlayer->explosiveKills );
		WRITE_SHORT( pPlayer->crowbarKills );
		WRITE_SHORT( pPlayer->projectileKills );

	MESSAGE_END();

	if ( !cheated ) {

		// Write new records if there are
		if ( currentScore > recordScore ) {
			recordScore = currentScore;
		}

		RecordSave();
	}
}

void CScoreAttack::RecordAdditionalDefaultInit() {
	recordScore = 0;
};

void CScoreAttack::RecordAdditionalRead( std::ifstream &inp ) {
	inp.read( ( char * ) &recordScore, sizeof( int ) );
};

void CScoreAttack::RecordAdditionalWrite( std::ofstream &out ) {
	out.write( ( char * ) &recordScore, sizeof( int ) );
};