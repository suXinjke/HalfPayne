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
#include "gamerules.h"
#include "gameplay_mod.h"

// special deathmatch shotgun spreads
#define VECTOR_CONE_DM_SHOTGUN	Vector( 0.08716, 0.04362, 0.00  )// 10 degrees by 5 degrees
#define VECTOR_CONE_DM_DOUBLESHOTGUN Vector( 0.17365, 0.04362, 0.00 ) // 20 degrees by 5 degrees

enum shotgun_e {
	SHOTGUN_IDLE = 0,
	SHOTGUN_FIRE,
	SHOTGUN_FIRE_FAST,
	SHOTGUN_FIRE2,
	SHOTGUN_FIRE2_FAST,
	SHOTGUN_RELOAD,
	SHOTGUN_PUMP,
	SHOTGUN_PUMP_FAST,
	SHOTGUN_START_RELOAD,
	SHOTGUN_START_RELOAD_FAST,
	SHOTGUN_DRAW,
	SHOTGUN_HOLSTER,
	SHOTGUN_IDLE4,
	SHOTGUN_IDLE_DEEP,
	SHOTGUN_FIRE_NO_RELOAD,
	SHOTGUN_FIRE_NO_RELOAD_FAST
};

LINK_ENTITY_TO_CLASS( weapon_shotgun, CShotgun );

void CShotgun::Spawn( )
{
	Precache( );
	m_iId = WEAPON_SHOTGUN;
	SET_MODEL(ENT(pev), "models/w_shotgun.mdl");

	m_iDefaultAmmo = SHOTGUN_DEFAULT_GIVE;

	FallInit();// get ready to fall
}


void CShotgun::Precache( void )
{
	PRECACHE_MODEL("models/v_shotgun.mdl");
	PRECACHE_MODEL("models/w_shotgun.mdl");
	PRECACHE_MODEL("models/p_shotgun.mdl");

	m_iShell = PRECACHE_MODEL ("models/shotgunshell.mdl");// shotgun shell

	PRECACHE_SOUND("items/9mmclip1.wav");              

	PRECACHE_SOUND ("weapons/dbarrel1.wav");//shotgun
	PRECACHE_SOUND ("weapons/sbarrel1.wav");//shotgun

	PRECACHE_SOUND ("weapons/reload1.wav");	// shotgun reload
	PRECACHE_SOUND ("weapons/reload3.wav");	// shotgun reload

//	PRECACHE_SOUND ("weapons/sshell1.wav");	// shotgun reload - played on client
//	PRECACHE_SOUND ("weapons/sshell3.wav");	// shotgun reload - played on client
	
	PRECACHE_SOUND ("weapons/357_cock1.wav"); // gun empty sound
	PRECACHE_SOUND ("weapons/scock1.wav");	// cock gun

	m_usSingleFire = PRECACHE_EVENT( 1, "events/shotgun1.sc" );
	m_usDoubleFire = PRECACHE_EVENT( 1, "events/shotgun2.sc" );
}

int CShotgun::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "buckshot";
	p->iMaxAmmo1 = BUCKSHOT_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = SHOTGUN_MAX_CLIP;
	p->iMaxClip2 = WEAPON_NOCLIP;
	p->iSlot = 2;
	p->iPosition = 1;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_SHOTGUN;
	p->iWeight = SHOTGUN_WEIGHT;

	return 1;
}



BOOL CShotgun::Deploy( )
{
	return DefaultDeploy( "models/v_shotgun.mdl", "models/p_shotgun.mdl", SHOTGUN_DRAW, "shotgun" );
}

void CShotgun::ItemPostFrame() {
	// Copy paste from WeaponIdle. WeaponIdle won't be called 
	// while you hold the attack button, but we still have to play the reload sound
	if ( shotOnce && m_flPumpTime && m_flPumpTime < gpGlobals->time)
	{
		// play pumping sound
		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/scock1.wav", 1, ATTN_NORM, 0, 95 + RANDOM_LONG(0, 0x1f));
		m_flPumpTime = 0;
	}

	if (shotOnce && !(m_pPlayer->pev->button & (IN_ATTACK )) ) {
		if (m_flNextPrimaryAttack <= 0.0f) {
			shotOnce = false;
		}
	}

	if (shotSecondaryOnce && !(m_pPlayer->pev->button & (IN_ATTACK2))) {
		if (m_flNextSecondaryAttack <= 0.0f) {
			shotSecondaryOnce = false;
		}
	}
	
	CBasePlayerWeapon::ItemPostFrame();
}

void CShotgun::PrimaryAttack()
{
	if (shotOnce) {
		return;
	}
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3 && !gameplayMods::shootUnderwater.isActive())
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = GetNextAttackDelay(0.15);
		return;
	}

	if (m_iClip <= 0)
	{
		Reload( );
		if (m_iClip == 0)
			PlayEmptySound( );
		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	if ( !gameplayMods::infiniteAmmoClip.isActive() ) {
		m_iClip--;
	}

	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif


	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	Vector vecDir;

	auto bulletPhysical = gameplayMods::PlayerShouldProducePhysicalBullets();

#ifndef CLIENT_DLL
	if ( bulletPhysical ) {

		float rightOffset = 2;

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

#ifdef CLIENT_DLL
	if ( bIsMultiplayer() )
#else
	if ( g_pGameRules->IsMultiplayer() )
#endif
	{
		vecDir = m_pPlayer->FireBulletsPlayer( 4, vecSrc, vecAiming, VECTOR_CONE_DM_SHOTGUN, 2048, BULLET_PLAYER_BUCKSHOT, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );
	}
	else
	{
		// regular old, untouched spread. 
		vecDir = m_pPlayer->FireBulletsPlayer( 6, vecSrc, vecAiming, VECTOR_CONE_10DEGREES, 2048, BULLET_PLAYER_BUCKSHOT, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );
	}
	
	auto shotgunAutomatic = gameplayMods::shotgunAutomatic.isActive();

	m_pPlayer->pev->punchangle[0] -= 2.5f * ( gameplayMods::upsideDown.isActive() ? -1 : 1 );
	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usSingleFire, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, bulletPhysical, shotgunAutomatic );


	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	if ( !shotgunAutomatic ) {
		float pumpDelay = gameplayMods::IsSlowmotionEnabled() ? 0.25 : 0.5;
		if (m_iClip != 0)
			m_flPumpTime = gpGlobals->time + pumpDelay;
	}

	float attackDelay = shotgunAutomatic ? 0.25 : 0.75f;
	if ( gameplayMods::IsSlowmotionEnabled() ) {
		attackDelay /= 1.6f;
	}

	m_flNextPrimaryAttack = GetNextAttackDelay(attackDelay);
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + attackDelay;
	if (m_iClip != 0)
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0;
	else
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + attackDelay;
	m_fInSpecialReload = 0;

	shotOnce = true;
	shotSecondaryOnce = true;
}


void CShotgun::SecondaryAttack( void )
{
	if ( shotSecondaryOnce || gameplayMods::noSecondaryAttack.isActive() || gameplayMods::shotgunAutomatic.isActive() ) {
		return;
	}
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3 && !gameplayMods::shootUnderwater.isActive() )
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = GetNextAttackDelay(0.15);
		return;
	}

	if (m_iClip <= 1)
	{
		Reload( );
		PlayEmptySound( );
		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	if ( !gameplayMods::infiniteAmmoClip.isActive() ) {
		m_iClip -= 2;
	}


	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	Vector vecDir;

	auto bulletPhysical = gameplayMods::PlayerShouldProducePhysicalBullets();

#ifndef CLIENT_DLL
	if ( bulletPhysical ) {
		
		float rightOffset = 2;

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

	
#ifdef CLIENT_DLL
	if ( bIsMultiplayer() )
#else
	if ( g_pGameRules->IsMultiplayer() )
#endif
	{
		// tuned for deathmatch
		vecDir = m_pPlayer->FireBulletsPlayer( 8, vecSrc, vecAiming, VECTOR_CONE_DM_DOUBLESHOTGUN, 2048, BULLET_PLAYER_BUCKSHOT, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );
	}
	else
	{
		// untouched default single player
		vecDir = m_pPlayer->FireBulletsPlayer( 12, vecSrc, vecAiming, VECTOR_CONE_10DEGREES, 2048, BULLET_PLAYER_BUCKSHOT, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );
	}
		
	m_pPlayer->pev->punchangle[0] -= 5.0f * ( gameplayMods::upsideDown.isActive() ? -1 : 1 );
	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usDoubleFire, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, bulletPhysical, 0 );

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	float pumpDelay = gameplayMods::IsSlowmotionEnabled() ? 0.6 : 0.95;
	if (m_iClip != 0)
		m_flPumpTime = gpGlobals->time + pumpDelay;

	float attackDelay = 1.5f;
	if ( gameplayMods::IsSlowmotionEnabled() ) {
		attackDelay /= 1.6f;
	}

	m_flNextPrimaryAttack = GetNextAttackDelay(attackDelay);
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + attackDelay;
	if (m_iClip != 0)
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 6.0;
	else
		m_flTimeWeaponIdle = attackDelay;

	m_fInSpecialReload = 0;

	shotSecondaryOnce = true;

}


void CShotgun::Reload( void )
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == SHOTGUN_MAX_CLIP)
		return;

	// don't reload until recoil is done
	if (m_flNextPrimaryAttack > UTIL_WeaponTimeBase())
		return;

	// check to see if we're ready to reload
	if (m_fInSpecialReload == 0)
	{
		float delay1 = 0.2;
		float delay2 = 0.5;
		if ( gameplayMods::IsSlowmotionEnabled() ) {
			delay1 /= 2.0f;
			delay2 /= 2.0f;
		}

		SendWeaponAnim( gameplayMods::IsSlowmotionEnabled() ? SHOTGUN_START_RELOAD_FAST : SHOTGUN_START_RELOAD );
		m_fInSpecialReload = 1;
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + delay1;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + delay1;
		m_flNextPrimaryAttack = GetNextAttackDelay(delay2);
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + delay2;
		return;
	}
	else if (m_fInSpecialReload == 1)
	{
		if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
			return;
		// was waiting for gun to move to side
		m_fInSpecialReload = 2;

		if (RANDOM_LONG(0,1))
			EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/reload1.wav", 1, ATTN_NORM, 0, 85 + RANDOM_LONG(0,0x1f));
		else
			EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/reload3.wav", 1, ATTN_NORM, 0, 85 + RANDOM_LONG(0,0x1f));

		SendWeaponAnim( SHOTGUN_RELOAD );

		m_flNextReload = UTIL_WeaponTimeBase() + 0.35;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.35;
	}
	else
	{
		// Add them to the clip
		m_iClip += 1;
		if ( !gameplayMods::infiniteAmmo.isActive() ) {
			m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= 1;
		}
		m_fInSpecialReload = 1;
	}
}


void CShotgun::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if ( m_flPumpTime && m_flPumpTime < gpGlobals->time )
	{
		// play pumping sound
		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/scock1.wav", 1, ATTN_NORM, 0, 95 + RANDOM_LONG(0,0x1f));
		m_flPumpTime = 0;
	}

	if (m_flTimeWeaponIdle <  UTIL_WeaponTimeBase() )
	{
		if (m_iClip == 0 && m_fInSpecialReload == 0 && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
		{
			Reload( );
		}
		else if (m_fInSpecialReload != 0)
		{
			if (m_iClip != 8 && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
			{
				Reload( );
			}
			else
			{
				float pumpTime = 1.5f;
				int anim = SHOTGUN_PUMP;

				if ( gameplayMods::IsSlowmotionEnabled() ) {
					pumpTime /= 2.0f;
					anim = SHOTGUN_PUMP_FAST;
				}
				// reload debounce has timed out
				SendWeaponAnim( anim );
				
				// play cocking sound
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/scock1.wav", 1, ATTN_NORM, 0, 95 + RANDOM_LONG(0,0x1f));
				m_fInSpecialReload = 0;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + pumpTime;
			}
		}
		else
		{
			int iAnim;
			float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0, 1 );
			if (flRand <= 0.8)
			{
				iAnim = SHOTGUN_IDLE_DEEP;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + (60.0/12.0);// * RANDOM_LONG(2, 5);
			}
			else if (flRand <= 0.95)
			{
				iAnim = SHOTGUN_IDLE;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + (20.0/9.0);
			}
			else
			{
				iAnim = SHOTGUN_IDLE4;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + (20.0/9.0);
			}
			SendWeaponAnim( iAnim );
		}
	}
}



class CShotgunAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_shotbox.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_shotbox.mdl");
		PRECACHE_SOUND( "items/ammopickup2.wav" );
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		if (pOther->GiveAmmo( AMMO_BUCKSHOTBOX_GIVE, "buckshot", BUCKSHOT_MAX_CARRY ) != -1)
		{
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/ammopickup2.wav", 1, ATTN_NORM, true );
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_buckshot, CShotgunAmmo );


