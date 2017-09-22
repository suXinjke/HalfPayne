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

enum glock_e {
	GLOCK_IDLE1 = 0,
	GLOCK_IDLE_NOSHOT,
	GLOCK_IDLE3,
	GLOCK_SHOOT,
	GLOCK_SHOOT_EMPTY,
	GLOCK_RELOAD_NOT_EMPTY,
	GLOCK_RELOAD_NOT_EMPTY_FAST,
	GLOCK_RELOAD,
	GLOCK_RELOAD_FAST,

	GLOCK_DRAW,
	GLOCK_DRAW_FAST,
	GLOCK_DRAW_NOSHOT,
	GLOCK_DRAW_NOSHOT_FAST,

	GLOCK_HOLSTER,
	
	GLOCK_DRAW_FROM_TWIN,
	GLOCK_DRAW_FROM_TWIN_FAST,

	GLOCK_DRAW_FROM_TWIN_NOSHOT_BOTH,
	GLOCK_DRAW_FROM_TWIN_NOSHOT_BOTH_FAST,

	GLOCK_DRAW_FROM_TWIN_NOSHOT_LEFT,
	GLOCK_DRAW_FROM_TWIN_NOSHOT_LEFT_FAST,

	GLOCK_DRAW_FROM_TWIN_NOSHOT_RIGHT,
	GLOCK_DRAW_FROM_TWIN_NOSHOT_RIGHT_FAST
};

LINK_ENTITY_TO_CLASS( weapon_glock, CGlock );
LINK_ENTITY_TO_CLASS( weapon_9mmhandgun, CGlock );


void CGlock::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_9mmhandgun"); // hack to allow for old names
	Precache( );
	m_iId = WEAPON_GLOCK;
	SET_MODEL(ENT(pev), "models/w_9mmhandgun.mdl");

	m_iDefaultAmmo = GLOCK_DEFAULT_GIVE;
	shotOnce = false;

	stress = 0.0f;
	nextStressDecrease = gpGlobals->time + 0.1f;

	FallInit();// get ready to fall down.
}


void CGlock::Precache( void )
{
	PRECACHE_MODEL("models/v_9mmhandgun.mdl");
	PRECACHE_MODEL("models/w_9mmhandgun.mdl");
	PRECACHE_MODEL("models/p_9mmhandgun.mdl");

	m_iShell = PRECACHE_MODEL ("models/shell.mdl");// brass shell

	PRECACHE_SOUND("items/9mmclip1.wav");
	PRECACHE_SOUND("items/9mmclip2.wav");

	PRECACHE_SOUND ("weapons/pl_gun1.wav");//silenced handgun
	PRECACHE_SOUND ("weapons/pl_gun2.wav");//silenced handgun
	PRECACHE_SOUND ("weapons/pl_gun3.wav");//handgun

	m_usFireGlock1 = PRECACHE_EVENT( 1, "events/glock1.sc" );
	m_usFireGlock2 = PRECACHE_EVENT( 1, "events/glock2.sc" );
}

int CGlock::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "9mm";
	p->iMaxAmmo1 = _9MM_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = GLOCK_MAX_CLIP;
	p->iMaxClip2 = WEAPON_NOCLIP;
	p->iSlot = 1;
	p->iPosition = 0;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_GLOCK;
	p->iWeight = GLOCK_WEIGHT;

	return 1;
}

BOOL CGlock::Deploy( )
{
	int anim = GLOCK_DRAW;

	bool drawingFromTwin = false;

	if ( m_pPlayer->m_pLastItem ) {
		if ( m_pPlayer->m_pLastItem->m_iId == WEAPON_GLOCK_TWIN ) {
			drawingFromTwin = true;
		}
	}

	if ( drawingFromTwin ) {
		int leftClip = ( ( CGlockTwin * ) m_pPlayer->m_pLastItem )->m_iClip2;
		if ( leftClip > 0 ) {
			anim = m_iClip > 0 ? GLOCK_DRAW_FROM_TWIN : GLOCK_DRAW_FROM_TWIN_NOSHOT_RIGHT;
		} else {
			anim = m_iClip > 0 ? GLOCK_DRAW_FROM_TWIN_NOSHOT_LEFT : GLOCK_DRAW_FROM_TWIN_NOSHOT_BOTH;
		}
	} else if ( m_iClip == 0 ) {
		anim = GLOCK_DRAW_NOSHOT;
	}

	if ( m_pPlayer->slowMotionEnabled ) {
		anim++;
	}

	// pev->body = 1;
	return DefaultDeploy( "models/v_9mmhandgun.mdl", "models/p_9mmhandgun.mdl", anim, "onehanded", /*UseDecrement() ? 1 : 0*/ 0 );
}

// m_pPlayer is not initialized yet during this call, hence accessing player via engine function.
// Spawn twin pistols entity on player, which should be received when you pickup one more pistol (duplicate).
int CGlock::AddDuplicate( CBasePlayerItem *pOriginal ) {

#ifndef CLIENT_DLL
	CBasePlayer *player = ( CBasePlayer * ) CBasePlayer::Instance( g_engfuncs.pfnPEntityOfEntIndex( 1 ) );

	if ( !player->HasNamedPlayerItem( "weapon_9mmhandgun_twin" ) ) {
		CGlockTwin *twinPistols = ( CGlockTwin * ) CBaseEntity::Create( "weapon_9mmhandgun_twin", pev->origin, Vector( 0, 0, 0 ), NULL );
		DispatchTouch( ENT( twinPistols->pev ), ENT( player->pev ) );

		return true;
	} else {
		return CBasePlayerWeapon::AddDuplicate( pOriginal );
	}
#else
	return CBasePlayerWeapon::AddDuplicate( pOriginal );
#endif
}

void CGlock::ItemPostFrame(void) {

	if ( !(m_pPlayer->pev->button & IN_ATTACK) ) {
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

void CGlock::SecondaryAttack( void )
{
	// Commented in favor of shooting by yourself, clicking the button rapidly
	// GlockFire( 0.1, 0.2, FALSE );
}

void CGlock::PrimaryAttack( void )
{
	float delay = 0.215f;
	if ( m_pPlayer->slowMotionEnabled ) {
		delay *= 0.75f;
	}
	GlockFire( 0.01, delay, TRUE );
}

void CGlock::GlockFire( float flSpread , float flCycleTime, BOOL fUseAutoAim )
{
	if (shotOnce) {
		return;
	}

	if (m_iClip <= 0)
	{
		if (m_fFireOnEmpty)
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = GetNextAttackDelay(0.2);
		}

		return;
	}

	if ( !m_pPlayer->infiniteAmmoClip ) {
		m_iClip--;
	}
	UTIL_SetWeaponClip( WEAPON_GLOCK_TWIN, m_iClip );

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	int flags;

#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	// silenced
	if (pev->body == 1)
	{
		m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;
	}
	else
	{
		// non-silenced
		m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;
	}

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming;
	Vector forward = m_pPlayer->GetAimForwardWithOffset();
	
	if ( fUseAutoAim )
	{
		vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );
	}
	else
	{
		vecAiming = forward;
	}

#ifndef CLIENT_DLL
	if ( m_pPlayer->shouldProducePhysicalBullets ) {

		float rightOffset = 8;

		vecSrc = vecSrc + forward * 5;
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

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), fUseAutoAim ? m_usFireGlock1 : m_usFireGlock2, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, ( m_iClip == 0 ) ? 1 : 0, m_pPlayer->shouldProducePhysicalBullets );

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(flCycleTime);
	shotOnce = true;

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );

	if ( stress < 1.5f ) {
		stress += 0.5f;
	}
}


void CGlock::Reload( void )
{
	if ( m_pPlayer->ammo_9mm <= 0 )
		 return;

	int iResult;

	float reloadTime = 1.5f;

	int anim = m_iClip == 0 ? GLOCK_RELOAD : GLOCK_RELOAD_NOT_EMPTY;
	if ( m_pPlayer->slowMotionEnabled ) {
		anim++;
	}

	iResult = DefaultReload( 17, anim, reloadTime, 0, reloadTime / 2.0f );

	if (iResult)
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	}
}



void CGlock::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	int anim = GLOCK_IDLE1;

	if ( m_iClip <= 0 ) {
		anim = GLOCK_IDLE_NOSHOT;
	}

	SendWeaponAnim( anim, 2 );
}








class CGlockAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_9mmclip.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_9mmclip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
		PRECACHE_SOUND( "items/ammopickup.wav" );
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		if (pOther->GiveAmmo( AMMO_GLOCKCLIP_GIVE, "9mm", _9MM_MAX_CARRY ) != -1)
		{
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/ammopickup.wav", 1, ATTN_NORM, true );
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_glockclip, CGlockAmmo );
LINK_ENTITY_TO_CLASS( ammo_9mmclip, CGlockAmmo );















