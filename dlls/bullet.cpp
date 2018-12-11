#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "skill.h"
#include "gamerules.h"
#include "player.h"

LINK_ENTITY_TO_CLASS( bullet, CBullet );

TYPEDESCRIPTION	CBullet::m_SaveData[] = 
{
	DEFINE_FIELD( CBullet, bulletType, FIELD_INTEGER ),
	DEFINE_FIELD( CBullet, ricochetCount, FIELD_INTEGER ),
	DEFINE_FIELD( CBullet, ricochetError, FIELD_INTEGER ),
	DEFINE_FIELD( CBullet, ricochetMaxDotProduct, FIELD_FLOAT ),
	DEFINE_FIELD( CBullet, ricochetVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( CBullet, lastVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( CBullet, tryToRicochet, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBullet, ricochettedOnce, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBullet, selfHarm, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBullet, startTime, FIELD_TIME ),
};
IMPLEMENT_SAVERESTORE( CBullet, CBaseEntity );

CBullet *CBullet::BulletCreate( Vector vecSrc, Vector velocity, int bulletType, edict_t *owner ) {
	CBullet *bullet = ( CBullet * ) CBaseEntity::Create( "bullet", vecSrc, UTIL_VecToAngles( velocity ), owner );
	bullet->pev->velocity = velocity;
	bullet->lastVelocity = velocity;
	bullet->bulletType = bulletType;
	bullet->pev->owner = owner;
	bullet->activateTrail = gameplayMods::bulletTrail.isActive();
	
	if ( auto ricochetInfo = gameplayMods::bulletRicochet.isActive<BulletRicochetInfo>() ) {
		bullet->ricochetCount = ricochetInfo->count;
		bullet->ricochetError = ricochetInfo->error;
		bullet->ricochetMaxDotProduct = ricochetInfo->maxDotProduct;
	}

	bullet->selfHarm = gameplayMods::bulletSelfHarm.isActive();
	bullet->auxOwner = ENT( owner );

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
	ricochetCount = 0;
	ricochetError = 5;
	ricochetMaxDotProduct = 0.5;
	tryToRicochet = FALSE;
	ricochettedOnce = FALSE;
	selfHarm = FALSE;
	startTime = gpGlobals->time;

	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_NOT;

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
	Vector vecSrc = pev->origin;
	Vector vecEnd = pev->origin + pev->velocity.Normalize() * 3;
	TraceResult tr;
	UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT(pev), &tr );
	TEXTURETYPE_PlaySound( &tr, vecSrc, vecEnd, bulletType );
	DecalGunshot( &tr, bulletType );

	if ( !ricochettedOnce ) {
		if ( CBasePlayer *player = dynamic_cast<CBasePlayer *>( CBaseEntity::Instance( pev->owner ) ) ) {
			player->OnBulletHit( pOther );
		}
	}

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

			case BULLET_PLAYER_M249:
				damage = gSkillData.plrDmgMP5 + 1;
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

		ApplyMultiDamage( pev, pevOwner ? pevOwner : pev );
		pev->velocity = Vector( 0, 0, 0 );

		Killed( pev, GIB_NEVER );

	} else if ( ricochetCount != 0 ) {
		Vector normal = tr.vecPlaneNormal;
		// TODO: add an error by utilising ricochetError correctly

		if ( DotProduct( -pev->velocity.Normalize(), normal ) < ricochetMaxDotProduct ) {
			ricochetVelocity = pev->velocity - 2 * ( DotProduct( pev->velocity, normal ) ) * normal;
			ricochettedOnce = TRUE;
			tryToRicochet = true;
			if ( ricochetCount > 0 ) {
				ricochetCount--;
			}
		} else {
			Killed( pev, GIB_NEVER );
		}
	} else if ( FStrEq( "bullet", STRING( pOther->pev->classname ) ) ) {
		// Try to go around other bullet
		UTIL_SetOrigin( pev, pev->origin + Vector( RANDOM_LONG( -1, 1 ), RANDOM_LONG( -1, 1 ), RANDOM_LONG( -1, 1 ) ) );
	} else {
		Killed( pev, GIB_NEVER );
	}

	pev->nextthink = gpGlobals->time + 0.01;
}

void CBullet::ActivateTrail( int life ) {
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_BEAMFOLLOW );
		WRITE_SHORT( entindex() );	// entity
		WRITE_SHORT( m_iTrail );	// model
		WRITE_BYTE( life ); // life
		WRITE_BYTE( 1 ); // width

		WRITE_BYTE( 255 ); // r, g, b
		WRITE_BYTE( 255 ); // r, g, b
		WRITE_BYTE( 255 ); // r, g, b

		WRITE_BYTE( 64 );	// brightness

	MESSAGE_END();

	activateTrail = false;
}

void CBullet::BubbleThink( void )
{
	pev->nextthink = gpGlobals->time + 0.01;
	
	if ( ( gpGlobals->time - startTime ) > 0.01 ) {
		pev->solid = SOLID_BBOX;
	}

	if ( pev->velocity.Length() != lastVelocity.Length() ) {
		pev->velocity = lastVelocity;
	}
	
	if ( selfHarm && pev->owner != NULL ) {
		CBaseEntity *potentialOwner = NULL;
		while ( ( potentialOwner = UTIL_FindEntityInSphere( potentialOwner, pev->origin, 48.0f ) ) != NULL ) {

			if ( pev->owner == potentialOwner->edict() ) {
				break;
			}
		}
		if ( !potentialOwner ) {
			pev->owner = NULL;
		}
	}

	if ( tryToRicochet ) {
		pev->velocity = ricochetVelocity;
		pev->angles = UTIL_VecToAngles( pev->velocity );
		tryToRicochet = false;
	}

	if ( activateTrail ) {
		bool longTrail = pev->velocity.Length() < 100;
		ActivateTrail( longTrail ? 10 : 2 );
	}

	if (pev->waterlevel == 0)
		return;

	UTIL_BubbleTrail( pev->origin - pev->velocity * 0.1, pev->origin, 1 );
}