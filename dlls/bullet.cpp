#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "skill.h"
#include "gamerules.h"

LINK_ENTITY_TO_CLASS( bullet, CBullet );

CBullet *CBullet::BulletCreate( Vector vecSrc, Vector velocity, int bulletType, bool trailActive, edict_t *owner )
{
	CBullet *bullet = ( CBullet * ) CBaseEntity::Create( "bullet", vecSrc, UTIL_VecToAngles( velocity ), owner );
	bullet->pev->velocity = velocity;
	bullet->bulletType = bulletType;
	bullet->pev->owner = owner;
	bullet->activateTrail = trailActive;

	switch ( bulletType ) {
		case BULLET_PLAYER_BUCKSHOT:
			SET_MODEL(ENT(bullet->pev), "models/bullet_12g.mdl");
			break;
		default:
			SET_MODEL(ENT(bullet->pev), "models/bullet_9mm.mdl");
			break;
	}

	return bullet;
}

void CBullet::Spawn( )
{
	Precache( );

	bulletType = BULLET_MONSTER_9MM;

	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BSP;

	SET_MODEL(ENT(pev), "models/bullet_9mm.mdl");

	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );

	SetTouch( &CBullet::BulletTouch );
	SetThink( &CBullet::BubbleThink );
	pev->nextthink = gpGlobals->time + 0.01;
}


void CBullet::Precache( )
{
	PRECACHE_MODEL ("models/bullet_9mm.mdl");
	m_iTrail = PRECACHE_MODEL("sprites/streak2.spr");
}

int CBullet::Classify( void )
{
	return CLASS_NONE;
}

void CBullet::BulletTouch( CBaseEntity *pOther )
{

	// Don't touch neighbour bullets or owner
	if ( pev->owner == pOther->pev->owner || pev->owner == pOther->edict() ) {
		pev->nextthink = gpGlobals->time + 0.01;
		return;
	}

	SetTouch( NULL );
	SetThink( NULL );

	Vector vecSrc = pev->origin;
	Vector vecEnd = pev->origin + pev->velocity.Normalize() * 3;
	TraceResult tr;
	UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT(pev), &tr );
	TEXTURETYPE_PlaySound( &tr, vecSrc, vecEnd, bulletType );
	DecalGunshot( &tr, bulletType );

	if ( pOther->pev->takedamage )
	{
		tr = UTIL_GetGlobalTrace( );
		entvars_t *pevOwner = VARS( pev->owner );
		ClearMultiDamage();

		float damage = 0.0f;
		switch ( bulletType ) {
			case BULLET_MONSTER_MP5:
				damage = gSkillData.monDmgMP5;
				break;

			case BULLET_MONSTER_12MM:
				damage = gSkillData.monDmg12MM;
				break;

			case BULLET_MONSTER_9MM:
				damage = gSkillData.monDmg9MM;
				break;

			case BULLET_PLAYER_357:
				damage = gSkillData.plrDmg357;
				break;

			case BULLET_PLAYER_MP5:
				damage = gSkillData.plrDmgBuckshot;
				break;

			case BULLET_PLAYER_BUCKSHOT:
				damage = gSkillData.plrDmgBuckshot;
				break;

			case BULLET_PLAYER_9MM:
			default:
				damage = gSkillData.plrDmg9MM;
				break;
		}

		if ( pOther->IsPlayer() )
		{
			pOther->TraceAttack(pevOwner, damage, pev->velocity.Normalize(), &tr, DMG_NEVERGIB ); 
		}
		else
		{
			pOther->TraceAttack(pevOwner, damage, pev->velocity.Normalize(), &tr, DMG_BULLET | DMG_NEVERGIB ); 
		}

		ApplyMultiDamage( pev, pevOwner );
		pev->velocity = Vector( 0, 0, 0 );

		if ( !g_pGameRules->IsMultiplayer() )
		{
			Killed( pev, GIB_NEVER );
		}
	}
	else
	{
		SetThink( &CBaseEntity::SUB_Remove );
		pev->nextthink = gpGlobals->time + 0.01;
	}
}

void CBullet::BubbleThink( void )
{
	pev->nextthink = gpGlobals->time + 0.01;

	if ( activateTrail ) {
		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_BEAMFOLLOW );
			WRITE_SHORT( entindex() );	// entity
			WRITE_SHORT( m_iTrail );	// model
			WRITE_BYTE( 2 ); // life
			WRITE_BYTE( 1 ); // width

			WRITE_BYTE( 255 ); // r, g, b
			WRITE_BYTE( 255 ); // r, g, b
			WRITE_BYTE( 255 ); // r, g, b

			WRITE_BYTE( 64 );	// brightness

		MESSAGE_END();

		activateTrail = false;
	}

	if (pev->waterlevel == 0)
		return;

	UTIL_BubbleTrail( pev->origin - pev->velocity * 0.1, pev->origin, 1 );
}