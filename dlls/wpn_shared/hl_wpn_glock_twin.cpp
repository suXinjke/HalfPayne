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
#include "weapons.h"
#include "nodes.h"
#include "player.h"

enum glock_twin_e {
	GLOCK_TWIN_IDLE = 0,
	GLOCK_TWIN_IDLE_NOSHOT_BOTH,
	GLOCK_TWIN_IDLE_NOSHOT_LEFT,
	GLOCK_TWIN_IDLE_NOSHOT_RIGHT,

	GLOCK_TWIN_SHOOT_RIGHT,
	GLOCK_TWIN_SHOOT_RIGHT_FAST,
	GLOCK_TWIN_SHOOT_RIGHT_WHEN_LEFT_EMPTY,
	GLOCK_TWIN_SHOOT_RIGHT_WHEN_LEFT_EMPTY_FAST,
	GLOCK_TWIN_SHOOT_RIGHT_THEN_EMPTY,
	GLOCK_TWIN_SHOOT_RIGHT_THEN_EMPTY_FAST,
	GLOCK_TWIN_SHOOT_RIGHT_THEN_EMPTY_WHEN_LEFT_EMPTY,
	GLOCK_TWIN_SHOOT_RIGHT_THEN_EMPTY_WHEN_LEFT_EMPTY_FAST,

	GLOCK_TWIN_SHOOT_LEFT,
	GLOCK_TWIN_SHOOT_LEFT_FAST,
	GLOCK_TWIN_SHOOT_LEFT_WHEN_RIGHT_EMPTY,
	GLOCK_TWIN_SHOOT_LEFT_WHEN_RIGHT_EMPTY_FAST,
	GLOCK_TWIN_SHOOT_LEFT_THEN_EMPTY,
	GLOCK_TWIN_SHOOT_LEFT_THEN_EMPTY_FAST,
	GLOCK_TWIN_SHOOT_LEFT_THEN_EMPTY_WHEN_RIGHT_EMPTY,
	GLOCK_TWIN_SHOOT_LEFT_THEN_EMPTY_WHEN_RIGHT_EMPTY_FAST,

	GLOCK_TWIN_RELOAD,
	GLOCK_TWIN_RELOAD_FAST,

	GLOCK_TWIN_RELOAD_NOSHOT_BOTH,
	GLOCK_TWIN_RELOAD_NOSHOT_BOTH_FAST,
	GLOCK_TWIN_RELOAD_NOSHOT_LEFT,
	GLOCK_TWIN_RELOAD_NOSHOT_LEFT_FAST,
	GLOCK_TWIN_RELOAD_NOSHOT_RIGHT,
	GLOCK_TWIN_RELOAD_NOSHOT_RIGHT_FAST,

	GLOCK_TWIN_RELOAD_ONLY_LEFT,
	GLOCK_TWIN_RELOAD_ONLY_LEFT_FAST,
	GLOCK_TWIN_RELOAD_ONLY_LEFT_NOSHOT,
	GLOCK_TWIN_RELOAD_ONLY_LEFT_NOSHOT_FAST,
	GLOCK_TWIN_RELOAD_ONLY_LEFT_NOSHOT_BOTH,
	GLOCK_TWIN_RELOAD_ONLY_LEFT_NOSHOT_BOTH_FAST,

	GLOCK_TWIN_RELOAD_ONLY_RIGHT,
	GLOCK_TWIN_RELOAD_ONLY_RIGHT_FAST,
	GLOCK_TWIN_RELOAD_ONLY_RIGHT_NOSHOT,
	GLOCK_TWIN_RELOAD_ONLY_RIGHT_NOSHOT_FAST,
	GLOCK_TWIN_RELOAD_ONLY_RIGHT_NOSHOT_BOTH,
	GLOCK_TWIN_RELOAD_ONLY_RIGHT_NOSHOT_BOTH_FAST,

	GLOCK_TWIN_DRAW,
	GLOCK_TWIN_DRAW_FAST,
	GLOCK_TWIN_DRAW_NOSHOT_BOTH,
	GLOCK_TWIN_DRAW_NOSHOT_BOTH_FAST,
	GLOCK_TWIN_DRAW_NOSHOT_LEFT,
	GLOCK_TWIN_DRAW_NOSHOT_LEFT_FAST,
	GLOCK_TWIN_DRAW_NOSHOT_RIGHT,
	GLOCK_TWIN_DRAW_NOSHOT_RIGHT_FAST,

	GLOCK_TWIN_DRAW_ONLY_LEFT,
	GLOCK_TWIN_DRAW_ONLY_LEFT_FAST,
	GLOCK_TWIN_DRAW_NOSHOT_BOTH_ONLY_LEFT,
	GLOCK_TWIN_DRAW_NOSHOT_BOTH_ONLY_LEFT_FAST,
	GLOCK_TWIN_DRAW_NOSHOT_LEFT_ONLY_LEFT,
	GLOCK_TWIN_DRAW_NOSHOT_LEFT_ONLY_LEFT_FAST,
	GLOCK_TWIN_DRAW_NOSHOT_RIGHT_ONLY_LEFT,
	GLOCK_TWIN_DRAW_NOSHOT_RIGHT_ONLY_LEFT_FAST
};

LINK_ENTITY_TO_CLASS( weapon_glock_twin, CGlockTwin );
LINK_ENTITY_TO_CLASS( weapon_9mmhandgun_twin, CGlockTwin );


void CGlockTwin::Spawn() {
	pev->classname = MAKE_STRING( "weapon_9mmhandgun_twin" ); // hack to allow for old names
	Precache();
	m_iId = WEAPON_GLOCK_TWIN;
	SET_MODEL( ENT( pev ), "models/w_9mmhandgun_twin.mdl" );

	m_iDefaultAmmo = GLOCK_DEFAULT_GIVE;
	shotOnce = false;

	stress = 0.0f;
	nextStressDecrease = gpGlobals->time + 0.1f;

	FallInit();// get ready to fall down.
}


void CGlockTwin::Precache( void ) {
	PRECACHE_MODEL( "models/v_9mmhandgun_twin.mdl" );
	PRECACHE_MODEL( "models/w_9mmhandgun_twin.mdl" );
	PRECACHE_MODEL( "models/p_9mmhandgun_twin.mdl" );

	m_iShell = PRECACHE_MODEL( "models/shell.mdl" );// brass shell

	PRECACHE_SOUND( "items/9mmclip1.wav" );
	PRECACHE_SOUND( "items/9mmclip2.wav" );

	PRECACHE_SOUND( "weapons/pl_gun1.wav" );//silenced handgun
	PRECACHE_SOUND( "weapons/pl_gun2.wav" );//silenced handgun
	PRECACHE_SOUND( "weapons/pl_gun3.wav" );//handgun

	m_usFireGlock1 = PRECACHE_EVENT( 1, "events/glock_twin.sc" );
}

int CGlockTwin::GetItemInfo( ItemInfo *p ) {
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "9mm";
	p->iMaxAmmo1 = _9MM_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = GLOCK_MAX_CLIP;
	p->iMaxClip2 = GLOCK_MAX_CLIP;
	p->iSlot = 1;
	p->iPosition = 1;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_GLOCK_TWIN;
	p->iWeight = GLOCK_WEIGHT;

	return 1;
}

BOOL CGlockTwin::Deploy() {
	// pev->body = 1;

	bool drawingFromSingle = false;

	if ( m_pPlayer->m_pLastItem ) {
		if ( m_pPlayer->m_pLastItem->m_iId == WEAPON_GLOCK ) {
			drawingFromSingle = true;
		}
	}

	int anim = drawingFromSingle ? GLOCK_TWIN_DRAW_ONLY_LEFT : GLOCK_TWIN_DRAW;

	if ( m_iClip == 0 && m_iClip2 == 0 ) {
		anim = drawingFromSingle ? GLOCK_TWIN_DRAW_NOSHOT_BOTH_ONLY_LEFT : GLOCK_TWIN_DRAW_NOSHOT_BOTH ;
	} else if ( m_iClip == 0 ) {
		anim = drawingFromSingle ? GLOCK_TWIN_DRAW_NOSHOT_RIGHT_ONLY_LEFT : GLOCK_TWIN_DRAW_NOSHOT_RIGHT;
	} else if ( m_iClip2 == 0 ) {
		anim = drawingFromSingle ? GLOCK_TWIN_DRAW_NOSHOT_LEFT_ONLY_LEFT : GLOCK_TWIN_DRAW_NOSHOT_LEFT;
	}

	if ( m_pPlayer->slowMotionEnabled ) {
		anim++;
	}

	return DefaultDeploy( "models/v_9mmhandgun_twin.mdl", "models/p_9mmhandgun.mdl", anim, "onehanded", /*UseDecrement() ? 1 : 0*/ 0 );
}

void CGlockTwin::ItemPostFrame( void ) {

	if ( !( m_pPlayer->pev->button & IN_ATTACK ) ) {
		if ( shotOnce ) {
			shotOnce = false;
		}

		if ( gpGlobals->time > nextStressDecrease ||
			fabs( nextStressDecrease - gpGlobals->time ) > 0.2f ) { // dumb in case of desync
			if ( m_pPlayer->slowMotionEnabled ) {
				stress *= 0.6f;
			} else {
				stress *= 0.8f;
			}

			nextStressDecrease = gpGlobals->time + 0.1f;

			if ( stress <= 0.15f ) {
				stress = 0.0f;
			}
		}
	}

	CBasePlayerWeapon::ItemPostFrame();
}

void CGlockTwin::SecondaryAttack( void ) {
	// Commented in favor of shooting by yourself, clicking the button rapidly
	// GlockFire( 0.1, 0.2, FALSE );
}

void CGlockTwin::PrimaryAttack( void ) {
	bool onlyOneClipLeft = m_iClip <= 0 || m_iClip2 <= 0;
	float delay = onlyOneClipLeft ? 0.215f : 0.175f;
	if ( m_pPlayer->slowMotionEnabled ) {
		delay *= 0.75f;
	}
	GlockFire( 0.01, delay, TRUE );
}

void CGlockTwin::GlockFire( float flSpread, float flCycleTime, BOOL fUseAutoAim ) {
	if ( shotOnce ) {
		return;
	}

	if ( m_iClip <= 0 && m_iClip2 <= 0 )
	{
		if ( m_fFireOnEmpty )
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = GetNextAttackDelay( 0.2 );
		}

		return;
	}

	int shootingRight = ( m_iClip + m_iClip2 ) % 2 == 0;
	if ( m_iClip == 0 ) {
		shootingRight = false;
	} else if ( m_iClip2 == 0 ) {
		shootingRight = true;
	}
	
	if ( shootingRight ) {
		m_iClip--;
		UTIL_SetWeaponClip( WEAPON_GLOCK, m_iClip );
	} else {
		if ( !m_pPlayer->infiniteAmmoClip ) {
			m_iClip2--;
		} else {
			m_iClip++;
		}
	}

	m_pPlayer->pev->effects = ( int ) ( m_pPlayer->pev->effects ) | EF_MUZZLEFLASH;

	int flags;

#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	// silenced
	if ( pev->body == 1 )
	{
		m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;
	} else
	{
		// non-silenced
		m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;
	}

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming;

	if ( fUseAutoAim )
	{
		vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );
	} else
	{
		vecAiming = gpGlobals->v_forward;
	}

#ifndef CLIENT_DLL
	if ( m_pPlayer->shouldProducePhysicalBullets ) {

		float rightOffset = shootingRight ? 8 : -8;

		vecSrc = vecSrc + gpGlobals->v_forward * 5;
		if ( m_pPlayer->upsideDown ) {
			rightOffset *= -1;
			vecSrc = vecSrc + Vector( 0, 0, 6 );
		}
		vecAiming = UTIL_VecSkew( vecSrc, vecAiming, rightOffset, ENT( pev ) );

		vecSrc = vecSrc + gpGlobals->v_right * rightOffset;
	}
#endif

	Vector vecDir;
	vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, VECTOR_CONE_2DEGREES * stress, 8192, BULLET_PLAYER_9MM, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );

	int empty = ( m_iClip == 0 ) ? 1 : 0;
	int empty2 = ( m_iClip2 == 0 ) ? 1 : 0;
	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), fUseAutoAim ? m_usFireGlock1 : m_usFireGlock2, 0.0, ( float * ) &g_vecZero, ( float * ) &g_vecZero, vecDir.x, vecDir.y, empty, empty2, m_pPlayer->shouldProducePhysicalBullets, shootingRight );

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay( flCycleTime );
	shotOnce = true;

	if ( !m_iClip && !m_iClip2 && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + ( 16.0f / 58.0f );

	if ( stress < 1.5f ) {
		stress += 0.5f;
	}
}


void CGlockTwin::Reload( void ) {
	if ( m_pPlayer->ammo_9mm <= 0 )
		return;

	int ammoStock = m_pPlayer->ammo_9mm;
	int ammoForClip1 = min( iMaxClip() - m_iClip, ammoStock );
	
	ammoStock -= ammoForClip1;

	int ammoForClip2 = min( iMaxClip2() - m_iClip2, ammoStock );

	bool realodingOnlyOneGun = false;

	int anim = GLOCK_TWIN_RELOAD;

	if ( ammoForClip1 > 0 && ammoForClip2 > 0 ) {
		if ( m_iClip == 0 && m_iClip2 == 0 ) {
			anim = GLOCK_TWIN_RELOAD_NOSHOT_BOTH;
		} else if ( m_iClip == 0 ) {
			anim = GLOCK_TWIN_RELOAD_NOSHOT_RIGHT;
		} else if ( m_iClip2 == 0 ) {
			anim = GLOCK_TWIN_RELOAD_NOSHOT_LEFT;
		}
	} else if ( ammoForClip1 == 0 ) {
		if ( m_iClip == 0 && m_iClip2 == 0 ) {
			anim = GLOCK_TWIN_RELOAD_ONLY_LEFT_NOSHOT_BOTH; // unlikely to happen, but whatever
		} else {
			anim = m_iClip2 == 0 ? GLOCK_TWIN_RELOAD_ONLY_LEFT_NOSHOT : GLOCK_TWIN_RELOAD_ONLY_LEFT;
		}

		realodingOnlyOneGun = true;

	} else if ( ammoForClip2 == 0 ) {
		if ( m_iClip == 0 && m_iClip2 == 0 ) {
			anim = GLOCK_TWIN_RELOAD_ONLY_RIGHT_NOSHOT_BOTH;
		} else {
			anim = m_iClip == 0 ? GLOCK_TWIN_RELOAD_ONLY_RIGHT_NOSHOT : GLOCK_TWIN_RELOAD_ONLY_RIGHT;
		}

		realodingOnlyOneGun = true;
	}

	if ( m_pPlayer->slowMotionEnabled ) {
		anim++;
	}

	float reloadTime = realodingOnlyOneGun ? 2.375 : 3.05;
	int iResult = DefaultReload( 17, anim, reloadTime, 0, reloadTime / 2.0f );

	if ( iResult )
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	}
}



void CGlockTwin::WeaponIdle( void ) {
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	int anim = GLOCK_TWIN_IDLE;

	if ( m_iClip == 0 && m_iClip2 == 0 ) {
		anim = GLOCK_TWIN_IDLE_NOSHOT_BOTH;
	} else if ( m_iClip == 0 ) {
		anim = GLOCK_TWIN_IDLE_NOSHOT_RIGHT;
	} else if ( m_iClip2 == 0 ) {
		anim = GLOCK_TWIN_IDLE_NOSHOT_LEFT;
	}

	SendWeaponAnim( anim, 2 );
}