#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "end_marker.h"
#include "gamerules.h"

LINK_ENTITY_TO_CLASS( end_marker, CEndMarker );

TYPEDESCRIPTION	CEndMarker::m_SaveData[] = {
	DEFINE_FIELD( CEndMarker, active, FIELD_BOOLEAN ),
};


IMPLEMENT_SAVERESTORE( CEndMarker, CBaseEntity );

void CEndMarker::Spawn( void ) {
	Precache();
	SET_MODEL( ENT( pev ), "models/end_marker.mdl" );

	active = FALSE;

	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_TRIGGER;
	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 72 ) );
	SetTouch( &CEndMarker::OnTouch );
	SetUse( &CEndMarker::OnUse );
}

void CEndMarker::Precache() {
	PRECACHE_MODEL( "models/end_marker.mdl" );
	PRECACHE_SOUND( "var/end_marker.wav" );
}

void CEndMarker::OnTouch( CBaseEntity *pOther ) {
	if ( !pOther->IsPlayer() ) {
		return;
	}

	if ( !active ) {
		return;
	}

	if ( CHalfLifeRules *rules = dynamic_cast< CHalfLifeRules * >( g_pGameRules ) ) {
		rules->End( ( CBasePlayer * ) pOther );
	}
}

void CEndMarker::OnUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value ) {
	active = TRUE;

	SetBodygroup( 1, 1 );
	pev->sequence = 0;
	ResetSequenceInfo();
}