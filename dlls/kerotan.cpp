#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "kerotan.h"
#include "gamerules.h"

LINK_ENTITY_TO_CLASS( kerotan, CKerotan );

TYPEDESCRIPTION	CKerotan::m_SaveData[] = {
	DEFINE_FIELD( CKerotan, hasBeenFound, FIELD_BOOLEAN ),
	DEFINE_FIELD( CKerotan, hitsReceived, FIELD_INTEGER ),
	DEFINE_FIELD( CKerotan, soundsLeft, FIELD_INTEGER ),
	DEFINE_FIELD( CKerotan, nextSound, FIELD_TIME ),
	DEFINE_FIELD( CKerotan, rotationLeft, FIELD_FLOAT ),
	DEFINE_FIELD( CKerotan, rotationDirection, FIELD_INTEGER ),
	DEFINE_FIELD( CKerotan, rollAmplitude, FIELD_FLOAT ),
	DEFINE_FIELD( CKerotan, rollDirection, FIELD_INTEGER ),
};


IMPLEMENT_SAVERESTORE( CKerotan, CBaseToggle );

void CKerotan::Spawn( void ) {
	Precache();
	SET_MODEL( ENT( pev ), "models/kerotan.mdl" );

	hasBeenFound = FALSE;
	hitsReceived = 0;
	soundsLeft = 0;
	nextSound = 0;
	rotationLeft = 0.0f;
	rotationDirection = 0;
	rollAmplitude = 0.0f;
	rollDirection = 0;

	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_SLIDEBOX;
	pev->takedamage = DAMAGE_YES;

	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize( pev, Vector( -2, -2, -2 ), Vector( 2, 2, 2 ) );

	SetThink( &CKerotan::OnThink );
	pev->nextthink = gpGlobals->time + 0.01f;
}

void CKerotan::Precache() {
	PRECACHE_MODEL( "models/kerotan.mdl" );
	PRECACHE_SOUND( "var/kerotan.wav" );
	PRECACHE_SOUND( "var/kerotan_alert.wav" );
	PRECACHE_SOUND( "var/kerotan_broke.wav" );
	PRECACHE_SOUND( "var/kerotan_short.wav" );
}

int CKerotan::TakeDamage( entvars_t * pevInflictor, entvars_t * pevAttacker, float flDamage, int bitsDamageType ) {
	rotationLeft = RANDOM_FLOAT( 40, 70 );
	rotationDirection = RANDOM_LONG( 0, 1 ) ? 1 : -1;

	if ( !rollAmplitude ) {
		rollAmplitude = RANDOM_FLOAT( 10, 30 );
		rollDirection = RANDOM_LONG( 0, 1 ) ? 1 : -1;
	}

	if ( !hasBeenFound ) {
		if ( CHalfLifeRules *rules = dynamic_cast< CHalfLifeRules * >( g_pGameRules ) ) {
			if ( std::string( STRING( pev->targetname ) ).empty() ) {
				pev->targetname = MAKE_STRING( "kerotan_found" );
			}
			rules->HookModelIndex( edict() );
		}
		
		if ( CBasePlayer *pPlayer = dynamic_cast< CBasePlayer * >( CBasePlayer::Instance( g_engfuncs.pfnPEntityOfEntIndex( 1 ) ) ) ) {
			if ( RANDOM_LONG( 0, 100 ) > 50 ) {
				char fileName[256];
				sprintf_s( fileName, "max/weird_item/WEIRD_ITEM_%d.wav", RANDOM_LONG( 1, 16 ) );
				pPlayer->TryToPlayMaxCommentary( MAKE_STRING( fileName ), false );
			}
		}

		hasBeenFound = TRUE;
	}

	if ( hitsReceived >= 5 ) {
		return 1;
	}

	hitsReceived++;

	EMIT_SOUND_DYN( ENT( pev ), CHAN_STATIC, "var/kerotan_alert.wav", 1, ATTN_NORM, 0, 100 );

	if ( !soundsLeft ) {
		soundsLeft = 10;
		nextSound = gpGlobals->time + 1.0f;
	}
	
	return 1;
}

void CKerotan::OnThink () {

	pev->nextthink = gpGlobals->time + 0.1f;

	if ( nextSound && gpGlobals->time >= nextSound ) {
		
		soundsLeft--;

		int pitch = min( 100, 100 - ( hitsReceived * 9 ) + 16 );
		
		if ( hitsReceived >= 5 && soundsLeft <= 0 ) {
			EMIT_SOUND_DYN( ENT( pev ), CHAN_STATIC, "var/kerotan_short.wav", 1, ATTN_NORM, 0, pitch );
			EMIT_SOUND( ENT( pev ), CHAN_STATIC, "var/kerotan_broke.wav", 1, ATTN_NORM );
		} else {
			EMIT_SOUND_DYN( ENT( pev ), CHAN_STATIC, "var/kerotan.wav", 1, ATTN_NORM, 0, pitch );
		}

		
		if ( soundsLeft > 0 ) {
			nextSound = gpGlobals->time + 1.3f + ( hitsReceived / 8.0f );
		} else {
			nextSound = 0.0f;
		}
	}

	if ( rotationLeft ) {
		float rotationStep = min( 5, rotationLeft );
		pev->angles.y += ( rotationStep * rotationDirection );
		rotationLeft -= rotationStep;
		if ( rotationLeft <= 0.0f ) {
			rotationLeft = 0.0f;
		}
	}

	pev->angles.z = rollDirection * rollAmplitude * sin( gpGlobals->time * 16 );
	rollAmplitude -= 0.5f;
	if ( rollAmplitude < 0.0f ) {
		rollAmplitude = 0.0f;
	}

	pev->nextthink = gpGlobals->time + 0.01f;
}
