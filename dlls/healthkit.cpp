/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "animation.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "items.h"
#include "gamerules.h"
#include <algorithm>

extern int gmsgItemPickup;

class CHealthKit : public CItem
{

public:
	void Spawn( void );
	void Precache( void );
	BOOL MyTouch( CBasePlayer *pPlayer );

	BOOL pickable;


	virtual int		Save( CSave &save ); 
	virtual int		Restore( CRestore &restore );
	
	static	TYPEDESCRIPTION m_SaveData[];


};


LINK_ENTITY_TO_CLASS( item_healthkit, CHealthKit );


TYPEDESCRIPTION	CHealthKit::m_SaveData[] = 
{
	DEFINE_FIELD( CHealthKit, pickable, FIELD_BOOLEAN ),
};


IMPLEMENT_SAVERESTORE( CHealthKit, CItem );


void CHealthKit :: Spawn( void )
{
	Precache( );
	SET_MODEL(ENT(pev), "models/w_medkit.mdl");

	pickable = TRUE;

	CItem::Spawn();
}

void CHealthKit::Precache( void )
{
	PRECACHE_MODEL("models/w_medkit.mdl");
	PRECACHE_SOUND("items/smallmedkit1.wav");
}

BOOL CHealthKit::MyTouch( CBasePlayer *pPlayer )
{
	if ( pPlayer->pev->deadflag != DEAD_NO || !(pPlayer->pev->weapons & ( 1 << WEAPON_SUIT ) ) || !pickable )
	{
		return FALSE;
	}

	//if ( pPlayer->TakeHealth( gSkillData.healthkitCapacity, DMG_GENERIC ) )
	if ( pPlayer->TakePainkiller() )
	{
		if ( g_pGameRules->ItemShouldRespawn( this ) )
		{
			Respawn();
		}
		else
		{
			UTIL_Remove(this);	
		}

		return TRUE;
	}

	return FALSE;
}



//-------------------------------------------------------------
// Wall mounted health kit
//-------------------------------------------------------------
class CWallHealth : public CBaseToggle
{
public:
	void Spawn( );
	void Precache( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual int	ObjectCaps( void ) { return (CBaseToggle :: ObjectCaps() | FCAP_CONTINUOUS_USE); }

    void EXPORT WaitUntilOpened( void );

    enum WALL_HEALTH_ANIM {
        IDLE,
        OPENING,
        IDLE_OPENED
    };
};

LINK_ENTITY_TO_CLASS(func_healthcharger, CWallHealth);


void CWallHealth::Spawn()
{
	Precache( );

	pev->solid		= SOLID_SLIDEBOX;
	pev->movetype	= MOVETYPE_PUSH;
	

	UTIL_SetOrigin(pev, pev->origin );		// set size and link into world
	UTIL_SetSize(pev, pev->mins, pev->maxs );
	SET_MODEL( ENT( pev ), STRING( pev->model ) );

    // We're working with brush based entity which has no rotation value
    // which we have to turn into actual model, and model requires rotation value.
    
    // Does the health charger brush look to north/south (horizontal) or west/east (vertical)?
    bool horizontallyPlaced = pev->size.x > pev->size.y;

    // Corner of the brush, middle height
    Vector beginPos = Vector( pev->mins.x, pev->mins.y, pev->mins.z + pev->size.z );

    // The general idea is that we LineTrace back and forward, and with trace results decide on 
    // the rotation angle that should result in facing the side with the greatest distance,
    // and shouldn't face the side with the least distance or wall\void.
    TraceResult	tr1, tr2;
    float tr1Length, tr2Length;
    if ( horizontallyPlaced ) {
        UTIL_TraceLine( beginPos, beginPos + Vector( 0, 13, 0 ), ignore_monsters, ENT( pev ), &tr1 );
        UTIL_TraceLine( beginPos, beginPos - Vector( 0, 13, 0 ), ignore_monsters, ENT( pev ), &tr2 );
    } else {
        UTIL_TraceLine( beginPos, beginPos - Vector( 13, 0, 0 ), ignore_monsters, ENT( pev ), &tr1 );
        UTIL_TraceLine( beginPos, beginPos + Vector( 13, 0, 0 ), ignore_monsters, ENT( pev ), &tr2 );
    }

    tr1Length = ( beginPos - tr1.vecEndPos ).Length();
    tr2Length = ( beginPos - tr2.vecEndPos ).Length();

    if ( ( ( tr1Length > tr2Length ) && tr1.fInOpen ) || !tr2.fInOpen ) {
        if ( horizontallyPlaced ) {
            pev->angles.y += 90;
        } else {
            pev->angles.y += 180;
        }
    } else {
        if ( horizontallyPlaced ) {
            pev->angles.y -= 90;
        }
    }
    

    Vector realPos = pev->origin + ( pev->mins + pev->maxs ) * 0.5;
    realPos.z -= pev->size.z / 3;

    SET_MODEL(ENT(pev), "models/w_med_cabinet.mdl" );

    UTIL_SetOrigin( pev, realPos );
    if ( horizontallyPlaced ) {
        UTIL_SetSize( pev, Vector( -5, -6, 0 ), Vector( 5, 6, 50 ) );
    } else {
        UTIL_SetSize( pev, Vector( -6, -5, 0 ), Vector( 6, 5, 50 ) );
    }

	int painkillersToSpawn = ceil( gSkillData.healthchargerCapacity / 10.0f ) - 1;

    // Attachment points for painkillers inside the cabinet, with random position offset
    float diversity = 1.5f;
    float diversity2 = 3.5f;
    Vector horizontalPainkillerSpots[4] = {
        realPos + Vector( -5 + RANDOM_FLOAT( -diversity2, diversity2 ), RANDOM_FLOAT( -diversity, diversity ), 13.3 ),
        realPos + Vector( 5 + RANDOM_FLOAT( -diversity2, diversity2 ), RANDOM_FLOAT( -diversity, diversity ), 13.3 ),
        realPos + Vector( -5 + RANDOM_FLOAT( -diversity2, diversity2 ), RANDOM_FLOAT( -diversity, diversity ), 0.5 ),
        realPos + Vector( 5 + RANDOM_FLOAT( -diversity2, diversity2 ), RANDOM_FLOAT( -diversity, diversity ), 0.5 )
	};

	Vector verticalPainkillerSpots[4] = {
        realPos + Vector( RANDOM_FLOAT( -diversity, diversity ), -5 + RANDOM_FLOAT( -diversity2, diversity2 ), 13.3f ),
        realPos + Vector( RANDOM_FLOAT( -diversity2, diversity2 ), 5 + RANDOM_FLOAT( -diversity, diversity ), 13.3f ),
        realPos + Vector( RANDOM_FLOAT( -diversity, diversity ), -5 + RANDOM_FLOAT( -diversity2, diversity2 ), 0.6 ),
        realPos + Vector( RANDOM_FLOAT( -diversity2, diversity2 ), 5 + RANDOM_FLOAT( -diversity, diversity ), 0.6 )
    };

	std::random_shuffle( std::begin( horizontalPainkillerSpots ), std::end( horizontalPainkillerSpots ) );
	std::random_shuffle( std::begin( verticalPainkillerSpots ), std::end( verticalPainkillerSpots ) );

    // Put painkillers inside the cabinet
    for ( int i = 0; i < painkillersToSpawn; i++ ) {
        int offset = horizontallyPlaced ? 0 : 4;

        CHealthKit *healthKit = ( CHealthKit * ) CBaseEntity::Create( "item_healthkit", realPos, Vector( 0, RANDOM_FLOAT( 0, 360 ), 0 ), NULL );
        healthKit->Spawn();
        UTIL_SetOrigin( healthKit->pev, horizontallyPlaced ? horizontalPainkillerSpots[i] : verticalPainkillerSpots[i] );

        // But don't let the player to take painkillers until the cabinet is opened
        healthKit->pev->movetype = MOVETYPE_NONE;
        healthKit->pickable = FALSE;
    }
}

void CWallHealth::Precache()
{
	PRECACHE_MODEL( "models/w_medkit.mdl" );
	PRECACHE_MODEL( "models/w_med_cabinet.mdl" );

	PRECACHE_SOUND("items/medshot4.wav");
	PRECACHE_SOUND("items/medshotno1.wav");
	PRECACHE_SOUND("items/medcharge4.wav");
	PRECACHE_SOUND("items/med_cabinet_open.wav");
}


void CWallHealth::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{ 
	// Make sure that we have a caller
	if (!pActivator)
		return;
	// if it's not a player, ignore
	if ( !pActivator->IsPlayer() )
		return;

    if ( pev->sequence == WALL_HEALTH_ANIM::OPENING || pev->sequence == WALL_HEALTH_ANIM::IDLE_OPENED ) {
        return;
    }

	EMIT_SOUND( ENT(pev), CHAN_ITEM, "items/med_cabinet_open.wav", 1, ATTN_NORM );

    pev->sequence = WALL_HEALTH_ANIM::OPENING;
    ResetSequenceInfo();
    SetThink( &CWallHealth::WaitUntilOpened );
    pev->nextthink = pev->ltime + 0.1;
}

void CWallHealth::WaitUntilOpened( void ) {
    pev->nextthink = pev->ltime + 0.1;

    float flInterval = StudioFrameAdvance( 0.1 );
    DispatchAnimEvents( flInterval );

    if ( m_fSequenceFinished ) {
        pev->sequence = WALL_HEALTH_ANIM::IDLE_OPENED;
        ResetSequenceInfo();

        Vector vecSrc = pev->origin;
        CBaseEntity *pEntity = NULL;
        while ( ( pEntity = UTIL_FindEntityInSphere( pEntity, vecSrc, 60.0f ) ) != NULL ) {
            const char *entityName = STRING( pEntity->pev->classname );

            if ( CHealthKit *healthKit = dynamic_cast<CHealthKit *>( pEntity ) ) {
				healthKit->pickable = TRUE;
            }
        }
    }
}