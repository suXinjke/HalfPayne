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
#if !defined( OEM_BUILD ) && !defined( HLDEMO_BUILD )

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "monsters.h"
#include "player.h"
#include "gamerules.h"
#include "gameplay_mod.h"

enum python_e {
	PYTHON_IDLE1 = 0,
	PYTHON_IDLE_NOSHOT,
	PYTHON_FIDGET,
	PYTHON_FIRE1,
	PYTHON_FIRE1_NOSHOT,

	PYTHON_RELOAD,
	PYTHON_RELOAD_FAST,

	PYTHON_RELOAD_NOSHOT,
	PYTHON_RELOAD_NOSHOT_FAST,

	PYTHON_HOLSTER,

	PYTHON_DRAW,
	PYTHON_DRAW_FAST,

	PYTHON_DRAW_NOSHOT,
	PYTHON_DRAW_NOSHOT_FAST
};

LINK_ENTITY_TO_CLASS( weapon_python, CPython );
LINK_ENTITY_TO_CLASS( weapon_357, CPython );

int CPython::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "357";
	p->iMaxAmmo1 = _357_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = PYTHON_MAX_CLIP;
	p->iMaxClip2 = WEAPON_NOCLIP;
	p->iFlags = 0;
	p->iSlot = 1;
	p->iPosition = 2;
	p->iId = m_iId = WEAPON_PYTHON;
	p->iWeight = PYTHON_WEIGHT;

	return 1;
}

int CPython::Restore( CRestore & restore ) {
	int result = CBasePlayerWeapon::Restore( restore );

	pev->fuser1 = 0.2f;

	return result;
}

void CPython::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_357"); // hack to allow for old names
	Precache( );
	m_iId = WEAPON_PYTHON;
	SET_MODEL(ENT(pev), "models/w_357.mdl");

	m_iDefaultAmmo = PYTHON_DEFAULT_GIVE;
	shotOnce = false;

	FallInit();// get ready to fall down.
}


void CPython::Precache( void )
{
	PRECACHE_MODEL("models/v_357.mdl");
	PRECACHE_MODEL("models/w_357.mdl");
	PRECACHE_MODEL("models/p_357.mdl");

	PRECACHE_MODEL("models/w_357ammobox.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");              

	PRECACHE_SOUND ("weapons/357_reload1.wav");
	PRECACHE_SOUND ("weapons/357_cock1.wav");
	PRECACHE_SOUND ("weapons/357_shot1.wav");
	PRECACHE_SOUND ("weapons/357_shot2.wav");

	m_usFirePython = PRECACHE_EVENT( 1, "events/python.sc" );
}

BOOL CPython::Deploy( )
{
#ifdef CLIENT_DLL
	if ( bIsMultiplayer() )
#else
	if ( g_pGameRules->IsMultiplayer() )
#endif
	{
		// enable laser sight geometry.
		pev->body = 1;
	}
	else
	{
		pev->body = 0;
	}

	int drawAnimation = m_iClip > 0 ? PYTHON_DRAW : PYTHON_DRAW_NOSHOT;
	if ( gameplayMods::IsSlowmotionEnabled() ) {
		drawAnimation++;
	}

	return DefaultDeploy( "models/v_357.mdl", "models/p_357.mdl", drawAnimation, "python", UseDecrement(), pev->body );
}


void CPython::Holster( int skiplocal /* = 0 */ )
{
	m_fInReload = FALSE;// cancel any reload in progress.

	if ( m_fInZoom )
	{
		SecondaryAttack();
	}

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	SendWeaponAnim( PYTHON_HOLSTER );
}

void CPython::ItemPostFrame( void ) {
	if (shotOnce && !(m_pPlayer->pev->button & IN_ATTACK)) {
		shotOnce = false;
	}

	if ( pev->fuser1 > 0.0f ) {
		SendWeaponAnim( m_iClip > 0 ? PYTHON_IDLE1 : PYTHON_IDLE_NOSHOT );
	}
	
	CBasePlayerWeapon::ItemPostFrame();
}

void CPython::SecondaryAttack( void )
{
#ifdef CLIENT_DLL
	if ( !bIsMultiplayer() )
#else
	if ( !g_pGameRules->IsMultiplayer() )
#endif
	{
		return;
	}

	if ( m_pPlayer->pev->fov != 0 )
	{
		m_fInZoom = FALSE;
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 0;  // 0 means reset to default fov
	}
	else if ( m_pPlayer->pev->fov != 40 )
	{
		m_fInZoom = TRUE;
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 40;
	}

	m_flNextSecondaryAttack = 0.5;
}

void CPython::PrimaryAttack()
{
	if (shotOnce) {
		return;
	}

	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3 && !gameplayMods::shootUnderwater.isActive())
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	if (m_iClip <= 0)
	{
		if (!m_fFireOnEmpty)
			Reload( );
		else
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = 0.15;
		}

		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	if ( !gameplayMods::infiniteAmmoClip.isActive() ) {
		m_iClip--;
	}

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );


	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

#ifndef CLIENT_DLL
	if ( gameplayMods::PlayerShouldProducePhysicalBullets() ) {
		float rightOffset = 3;

		if ( gameplayMods::mirror.isActive() ) {
			rightOffset *= -1;
		}
		if ( gameplayMods::upsideDown.isActive() ) {
			rightOffset *= -1;
			vecSrc = vecSrc + Vector( 0, 0, 6 );
		}

		vecAiming = UTIL_VecSkew( vecSrc, vecAiming, rightOffset, ENT( pev ) );

		vecSrc = vecSrc + gpGlobals->v_right * rightOffset;
	}
#endif

	Vector vecDir;
	vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, VECTOR_CONE_1DEGREES, 8192, BULLET_PLAYER_357, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );

    int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	int empty = m_iClip == 0;

	m_pPlayer->pev->punchangle[0] -= 5.0f * ( gameplayMods::upsideDown.isActive() ? -1 : 1 );
	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usFirePython, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, empty, gameplayMods::PlayerShouldProducePhysicalBullets() );

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = 0.4;
	shotOnce = true;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + ( 19.0f / 30.0f );
}


void CPython::Reload( void )
{
	if ( m_pPlayer->ammo_357 <= 0 )
		return;

	if ( m_pPlayer->pev->fov != 0 )
	{
		m_fInZoom = FALSE;
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 0;  // 0 means reset to default fov
	}

	int bUseScope = FALSE;
#ifdef CLIENT_DLL
	bUseScope = bIsMultiplayer();
#else
	bUseScope = g_pGameRules->IsMultiplayer();
#endif

	float reloadTime = 2.0f;
	int anim = m_iClip > 0 ? PYTHON_RELOAD : PYTHON_RELOAD_NOSHOT;
	if ( gameplayMods::IsSlowmotionEnabled() ) {
		anim++;
	}

	DefaultReload( 7, anim, reloadTime, bUseScope, reloadTime / 2.0f );
}


void CPython::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	int iAnim = m_iClip > 0 ? PYTHON_IDLE1 : PYTHON_IDLE_NOSHOT;
	float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0, 1 );
	if (flRand <= 0.5 && m_iClip > 0)
	{
		iAnim = PYTHON_FIDGET;
	}

	m_flTimeWeaponIdle = (49.0/30.0);
	
	int bUseScope = FALSE;
#ifdef CLIENT_DLL
	bUseScope = bIsMultiplayer();
#else
	bUseScope = g_pGameRules->IsMultiplayer();
#endif
	
	SendWeaponAnim( iAnim, UseDecrement() ? 1 : 0, bUseScope );
}


class CPythonAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_357ammobox.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_357ammobox.mdl");
		PRECACHE_SOUND( "items/ammopickup2.wav" );
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		if (pOther->GiveAmmo( AMMO_357BOX_GIVE, "357", _357_MAX_CARRY ) != -1)
		{
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/ammopickup2.wav", 1, ATTN_NORM, true );
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_357, CPythonAmmo );


#endif