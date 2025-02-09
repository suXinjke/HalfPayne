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
#include "soundent.h"
#include "gamerules.h"
#include "gameplay_mod.h"

enum mp5_e
{
	MP5_LONGIDLE = 0,
	MP5_IDLE1,
	MP5_LAUNCH,
	MP5_RELOAD,
	MP5_RELOAD_FAST,
	MP5_DEPLOY,
	MP5_DEPLOY2,
	MP5_FIRE1,
	MP5_FIRE2,
	MP5_FIRE3,
};



LINK_ENTITY_TO_CLASS( weapon_mp5, CMP5 );
LINK_ENTITY_TO_CLASS( weapon_9mmAR, CMP5 );


//=========================================================
//=========================================================
int CMP5::SecondaryAmmoIndex( void )
{
	return m_iSecondaryAmmoType;
}

void CMP5::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_9mmAR"); // hack to allow for old names
	Precache( );
	SET_MODEL(ENT(pev), "models/w_9mmAR.mdl");
	m_iId = WEAPON_MP5;

	m_iDefaultAmmo = MP5_DEFAULT_GIVE;
	stress = 0.0f;
	nextStressDecrease = gpGlobals->time + 0.1f;

	FallInit();// get ready to fall down.
}


void CMP5::Precache( void )
{
	PRECACHE_MODEL("models/v_9mmAR.mdl");
	PRECACHE_MODEL("models/w_9mmAR.mdl");
	PRECACHE_MODEL("models/p_9mmAR.mdl");

	m_iShell = PRECACHE_MODEL ("models/shell.mdl");// brass shellTE_MODEL

	PRECACHE_MODEL("models/grenade.mdl");	// grenade

	PRECACHE_MODEL("models/w_9mmARclip.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");              

	PRECACHE_SOUND("items/clipinsert1.wav");
	PRECACHE_SOUND("items/cliprelease1.wav");

	PRECACHE_SOUND ("weapons/hks1.wav");// H to the K
	PRECACHE_SOUND ("weapons/hks2.wav");// H to the K
	PRECACHE_SOUND ("weapons/hks3.wav");// H to the K

	PRECACHE_SOUND( "weapons/glauncher.wav" );
	PRECACHE_SOUND( "weapons/glauncher2.wav" );

	PRECACHE_SOUND ("weapons/357_cock1.wav");

	m_usMP5 = PRECACHE_EVENT( 1, "events/mp5.sc" );
	m_usMP52 = PRECACHE_EVENT( 1, "events/mp52.sc" );
}

int CMP5::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "9mm";
	p->iMaxAmmo1 = _9MM_MAX_CARRY;
	p->pszAmmo2 = "ARgrenades";
	p->iMaxAmmo2 = M203_GRENADE_MAX_CARRY;
	p->iMaxClip = MP5_MAX_CLIP;
	p->iMaxClip2 = WEAPON_NOCLIP;
	p->iSlot = 2;
	p->iPosition = 0;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_MP5;
	p->iWeight = MP5_WEIGHT;

	return 1;
}

BOOL CMP5::Deploy( )
{
	return DefaultDeploy( "models/v_9mmAR.mdl", "models/p_9mmAR.mdl", MP5_DEPLOY, "mp5" );
}

void CMP5::ItemPostFrame()
{
	if ( !( m_pPlayer->pev->button & IN_ATTACK ) )
	{
		if ( gpGlobals->time > nextStressDecrease ||
			 fabs( nextStressDecrease - gpGlobals->time ) > 0.2f ) { // dumb in case of desync
			if ( gameplayMods::IsSlowmotionEnabled() ) {
				stress *= 0.5f;
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

void CMP5::PrimaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3 && !gameplayMods::shootUnderwater.isActive())
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	if (m_iClip <= 0)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	if ( !gameplayMods::infiniteAmmoClip.isActive() ) {
		m_iClip--;
	}


	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );
	Vector vecDir;

	// Accuracy lowers with more bullets you shot (bulletsShot up to 3)
	Vector vecBullet = VECTOR_CONE_2DEGREES * stress;

#ifndef CLIENT_DLL
	if ( gameplayMods::PlayerShouldProducePhysicalBullets() ) {

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
	if ( !bIsMultiplayer() )
#else
	if ( !g_pGameRules->IsMultiplayer() )
#endif
	{
		// optimized multiplayer. Widened to make it easier to hit a moving player
		vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, vecBullet, 8192, BULLET_PLAYER_MP5, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );
	}
	else
	{
		// single player spread
		vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, vecBullet, 8192, BULLET_PLAYER_MP5, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );
	}

  int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	// There's not enough room for float type, so it's interpreted as int
	int stressPacked = *( int * ) &stress;

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usMP5, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, stressPacked, 0, gameplayMods::PlayerShouldProducePhysicalBullets(), 0 );

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	float delay = 0.1;
	if ( gameplayMods::IsSlowmotionEnabled() ) {
		delay *= 0.85f;
	}
	m_flNextPrimaryAttack = GetNextAttackDelay( delay );

	if ( m_flNextPrimaryAttack < UTIL_WeaponTimeBase() )
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + delay;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );

	if ( stress < 3.0f ) {
		stress += 0.2f;
	}
}



void CMP5::SecondaryAttack( void )
{
	if ( gameplayMods::noSecondaryAttack.isActive() ) {
		return;
	}
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3 && !gameplayMods::shootUnderwater.isActive() )
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	if (m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType] == 0)
	{
		PlayEmptySound( );
		return;
	}

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	m_pPlayer->m_iExtraSoundTypes = bits_SOUND_DANGER;
	m_pPlayer->m_flStopExtraSoundTime = UTIL_WeaponTimeBase() + 0.2;
			
	if ( !gameplayMods::infiniteAmmo.isActive() ) {
		m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType]--;
	}

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

 	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	Vector forward = m_pPlayer->GetAimForwardWithOffset();

	// we don't add in player velocity anymore.
	CGrenade::ShootContact( m_pPlayer->pev, 
							m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + forward * 16, 
							forward * 800 );

	m_pPlayer->ApplyWeaponPushback( 1000 );

	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	m_pPlayer->pev->punchangle[0] -= 5.0f * ( gameplayMods::upsideDown.isActive() ? -1 : 1 );
	PLAYBACK_EVENT( flags, m_pPlayer->edict(), m_usMP52 );
	
	m_flNextPrimaryAttack = GetNextAttackDelay(1);
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5;// idle pretty soon after shooting.

	if (!m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType])
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);
}

void CMP5::Reload( void )
{
	if ( m_pPlayer->ammo_9mm <= 0 )
		return;

	float reloadTime = 1.5f;

	DefaultReload( MP5_MAX_CLIP, gameplayMods::IsSlowmotionEnabled() ? MP5_RELOAD_FAST : MP5_RELOAD, reloadTime, 0, reloadTime / 2.0f );
	stress = 0.0f;
}


void CMP5::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	int iAnim;
	switch ( RANDOM_LONG( 0, 1 ) )
	{
	case 0:	
		iAnim = MP5_LONGIDLE;	
		break;
	
	default:
	case 1:
		iAnim = MP5_IDLE1;
		break;
	}

	SendWeaponAnim( iAnim );

	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 ); // how long till we do this again.
}



class CMP5AmmoClip : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_9mmARclip.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_9mmARclip.mdl");
		PRECACHE_SOUND( "items/ammopickup.wav" );
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		int bResult = (pOther->GiveAmmo( AMMO_MP5CLIP_GIVE, "9mm", _9MM_MAX_CARRY) != -1);
		if (bResult)
		{
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/ammopickup.wav", 1, ATTN_NORM, true );
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS( ammo_mp5clip, CMP5AmmoClip );
LINK_ENTITY_TO_CLASS( ammo_9mmAR, CMP5AmmoClip );



class CMP5Chainammo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_chainammo.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_chainammo.mdl");
		PRECACHE_SOUND("items/ammopickup2.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		int bResult = (pOther->GiveAmmo( AMMO_CHAINBOX_GIVE, "9mm", _9MM_MAX_CARRY) != -1);
		if (bResult)
		{
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/ammopickup2.wav", 1, ATTN_NORM, true );
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS( ammo_9mmbox, CMP5Chainammo );


class CMP5AmmoGrenade : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_ARgrenade.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_ARgrenade.mdl");
		PRECACHE_SOUND( "items/ammopickup2.wav" );
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		if ( gameplayMods::noSmgGrenadePickup.isActive() ) {
			return FALSE;
		}

		int bResult = (pOther->GiveAmmo( AMMO_M203BOX_GIVE, "ARgrenades", M203_GRENADE_MAX_CARRY ) != -1);

		if (bResult)
		{
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/ammopickup2.wav", 1, ATTN_NORM, true );
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS( ammo_mp5grenades, CMP5AmmoGrenade );
LINK_ENTITY_TO_CLASS( ammo_ARgrenades, CMP5AmmoGrenade );


















