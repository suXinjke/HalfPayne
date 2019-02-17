#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gameplay_mod.h"

LINK_ENTITY_TO_CLASS( weapon_ingram_twin, CIngramTwin );

void CIngramTwin::Spawn() {
	pev->classname = MAKE_STRING( "weapon_ingram_twin" ); // hack to allow for old names
	Precache();
	m_iId = WEAPON_INGRAM_TWIN;
	SET_MODEL( ENT( pev ), "models/w_ingram.mdl" );

	m_iDefaultAmmo = INGRAM_DEFAULT_GIVE;

	stress = 2.0f;
	nextStressDecrease = gpGlobals->time + 0.1f;

	FallInit();// get ready to fall down.
}


void CIngramTwin::Precache( void ) {
	PRECACHE_MODEL("models/v_ingram.mdl");
	PRECACHE_MODEL("models/w_ingram.mdl");
	PRECACHE_MODEL("models/p_9mmhandgun.mdl");

	m_iShell = PRECACHE_MODEL( "models/shell.mdl" );

	PRECACHE_SOUND( "items/9mmclip1.wav" );
	PRECACHE_SOUND( "weapons/ingram_shot.wav" );
	PRECACHE_SOUND( "weapons/ingram_clip_out.wav" );
	PRECACHE_SOUND( "weapons/ingram_clip_out_twin.wav" );
	PRECACHE_SOUND( "weapons/ingram_bolt_pull.wav" );

	m_usFireIngram = PRECACHE_EVENT( 1, "events/ingram_twin.sc" );
	m_usFireIngramTracer = PRECACHE_EVENT( 1, "events/ingram_twin_tracer.sc" );
}

int CIngramTwin::GetItemInfo( ItemInfo *p ) {
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "9mm";
	p->iMaxAmmo1 = _9MM_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = INGRAM_MAX_CLIP;
	p->iMaxClip2 = INGRAM_MAX_CLIP;
	p->iSlot = 2;
	p->iPosition = 4;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_INGRAM_TWIN;
	p->iWeight = INGRAM_WEIGHT;

	return 1;
}

int CIngramTwin::Restore( CRestore &restore ) {
	int result = CBasePlayerWeapon::Restore( restore );

	pev->fuser1 = 0.2f;

	return result;
}

BOOL CIngramTwin::Deploy() {

	bool drawingFromSingle = false;

	if ( m_pPlayer->m_pLastItem ) {
		if ( m_pPlayer->m_pLastItem->m_iId == WEAPON_INGRAM ) {
			drawingFromSingle = true;
		}
	}

	int anim = drawingFromSingle ? INGRAM_TWIN_DRAW_ONLY_LEFT : INGRAM_TWIN_DRAW;

	if ( m_iClip == 0 && m_iClip2 == 0 ) {
		anim = drawingFromSingle ? INGRAM_TWIN_DRAW_NOSHOT_BOTH_ONLY_LEFT : INGRAM_TWIN_DRAW_NOSHOT_BOTH ;
	} else if ( m_iClip == 0 ) {
		anim = drawingFromSingle ? INGRAM_TWIN_DRAW_NOSHOT_RIGHT_ONLY_LEFT : INGRAM_TWIN_DRAW_NOSHOT_RIGHT;
	} else if ( m_iClip2 == 0 ) {
		anim = drawingFromSingle ? INGRAM_TWIN_DRAW_NOSHOT_LEFT_ONLY_LEFT : INGRAM_TWIN_DRAW_NOSHOT_LEFT;
	}

	if ( gameplayMods::IsSlowmotionEnabled() ) {
		anim++;
	}

	return DefaultDeploy( "models/v_ingram.mdl", "models/p_9mmhandgun.mdl", anim, "onehanded", 0 );
}

void CIngramTwin::ItemPostFrame( void ) {

	if ( !( m_pPlayer->pev->button & IN_ATTACK ) ) {

		if ( gpGlobals->time > nextStressDecrease ||
			fabs( nextStressDecrease - gpGlobals->time ) > 0.2f ) { // dumb in case of desync
			if ( gameplayMods::IsSlowmotionEnabled() ) {
				stress *= 0.6f;
			} else {
				stress *= 0.8f;
			}

			nextStressDecrease = gpGlobals->time + 0.1f;

			if ( stress <= 2.15f ) {
				stress = 2.0f;
			}
		}
	}

	if ( pev->fuser1 > 0.0f ) {
		SendWeaponAnim( GetIdleAnimation() );
	}

	CBasePlayerWeapon::ItemPostFrame();
}

void CIngramTwin::PrimaryAttack( void )
{
	if ( m_pPlayer->pev->waterlevel == 3 && !gameplayMods::shootUnderwater.isActive() ) {
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	float delay = 0.048f;

	if ( m_iClip <= 0 && m_iClip2 <= 0 )
	{
		if ( m_fFireOnEmpty )
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = GetNextAttackDelay( 0.2 );
			SendWeaponAnim( GetIdleAnimation() );
		}

		return;
	}

	int shootingRight = m_iClip > 0;
	int shootingLeft = m_iClip2 > 0;
	int shootingBoth = shootingLeft && shootingRight;

	if ( shootingRight ) {
		m_iClip--;
	}
	
	if ( shootingLeft ) {
		m_iClip2--;
	}
	
	if ( gameplayMods::infiniteAmmoClip.isActive() ) {
		m_iClip += shootingRight;
		m_iClip2 += shootingLeft;
	}

	UTIL_SetWeaponClip( WEAPON_INGRAM, m_iClip );

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	int flags;

#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );


	Vector vecDir;
	Vector vecDir2;

	auto bulletPhysical = gameplayMods::PlayerShouldProducePhysicalBullets();

	for ( int i = 0 ; i < 2 ; i++ ) {
		Vector vecSrc = m_pPlayer->GetGunPosition();
		Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );
		Vector forward = m_pPlayer->GetAimForwardWithOffset();

		if (
			( i == 0 && !shootingRight ) ||
			( i == 1 && !shootingLeft ) 
		) {
			continue;
		}

#ifndef CLIENT_DLL
		if ( bulletPhysical ) {

			float rightOffset = i == 0 ? 8 : -8;

			vecSrc = vecSrc + forward * 5;
			if ( gameplayMods::upsideDown.isActive() ) {
				rightOffset *= -1;
				vecSrc = vecSrc + Vector( 0, 0, 6 );
			}

			vecAiming = UTIL_VecSkew( vecSrc, vecAiming, rightOffset, ENT( pev ) );

			vecSrc = vecSrc + gpGlobals->v_right * rightOffset;
		}
#endif
		Vector dir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, VECTOR_CONE_2DEGREES * stress, 8192, BULLET_PLAYER_MP5, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed + ( i == 1 ? 22 : 0 ) );
		if ( i == 0 ) {
			vecDir = dir;
		} else {
			vecDir2 = dir;
		}

		if ( stress < 5.0f ) {
			stress += 0.25f;
		}
	}

	int emptyFlag = 0;
	if ( m_iClip == 0 ) {
		emptyFlag |= 1;
	}
	if ( m_iClip2 == 0 ) {
		emptyFlag |= 2;
	}

	// There's not enough room for float type, so it's interpreted as int
	int stressPacked = *( int * ) &stress;
	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usFireIngram, 0.0, ( float * ) &g_vecZero, ( float * ) &g_vecZero, 0.0f, 0.0f, emptyFlag, stressPacked, bulletPhysical, shootingBoth ? 2 : shootingLeft ? 1 : 0 );
	if ( !bulletPhysical ) {
		if ( shootingRight ) {
			PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usFireIngramTracer, 0.0, ( float * ) &g_vecZero, ( float * ) &g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, true /* shooting right */ );
		}
		if ( shootingLeft ) {
			PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usFireIngramTracer, 0.0, ( float * ) &g_vecZero, ( float * ) &g_vecZero, vecDir2.x, vecDir2.y, 0, 0, 0, false /* shooting left */ );
		}
	}	

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay( delay );

	if ( !m_iClip && !m_iClip2 && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + ( 16.0f / 58.0f );
}

void CIngramTwin::Reload( void )
{
	if ( m_pPlayer->ammo_9mm <= 0 )
		return;

	int ammoStock = m_pPlayer->ammo_9mm;
	int ammoForClip1 = min( iMaxClip() - m_iClip, ammoStock );

	ammoStock -= ammoForClip1;

	int ammoForClip2 = min( iMaxClip2() - m_iClip2, ammoStock );

	bool realodingOnlyOneGun = false;

	int anim = INGRAM_TWIN_RELOAD;

	if ( ammoForClip1 > 0 && ammoForClip2 > 0 ) {
		if ( m_iClip == 0 && m_iClip2 == 0 ) {
			anim = INGRAM_TWIN_RELOAD_NOSHOT_BOTH;
		} else if ( m_iClip == 0 ) {
			anim = INGRAM_TWIN_RELOAD_NOSHOT_RIGHT;
		} else if ( m_iClip2 == 0 ) {
			anim = INGRAM_TWIN_RELOAD_NOSHOT_LEFT;
		}
	} else if ( ammoForClip1 == 0 ) {
		if ( m_iClip == 0 && m_iClip2 == 0 ) {
			anim = INGRAM_TWIN_RELOAD_ONLY_LEFT_NOSHOT; // unlikely to happen, but whatever
		} else {
			anim = m_iClip2 == 0 ? INGRAM_TWIN_RELOAD_ONLY_LEFT_NOSHOT : INGRAM_TWIN_RELOAD_ONLY_LEFT;
		}

		realodingOnlyOneGun = true;

	} else if ( ammoForClip2 == 0 ) {
		if ( m_iClip == 0 && m_iClip2 == 0 ) {
			anim = INGRAM_TWIN_RELOAD_ONLY_RIGHT_NOSHOT_BOTH;
		} else {
			anim = m_iClip == 0 ? INGRAM_TWIN_RELOAD_ONLY_RIGHT_NOSHOT : INGRAM_TWIN_RELOAD_ONLY_RIGHT;
		}

		realodingOnlyOneGun = true;
	}

	if ( gameplayMods::IsSlowmotionEnabled() ) {
		anim++;
	}

	float reloadTime = realodingOnlyOneGun ? ( 56.0f / 30.0f ) : ( 76.0f / 30.0f );
	
	if ( m_iClip == 0 ) {
		reloadTime += 10.0f / 30.0f;
	}

	if ( m_iClip2 == 0 ) {
		reloadTime += 10.0f / 30.0f;
	}

	int iResult = DefaultReload( INGRAM_MAX_CLIP, anim, reloadTime, 0, reloadTime / 2.0f );

	if ( iResult )
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	}
}



void CIngramTwin::WeaponIdle( void )
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	SendWeaponAnim( GetIdleAnimation() );
}

int CIngramTwin::GetIdleAnimation() {
	return
		m_iClip == 0 && m_iClip2 == 0 ? INGRAM_TWIN_IDLE_NOSHOT_BOTH :
		m_iClip == 0 ? INGRAM_TWIN_IDLE_NOSHOT_RIGHT :
		m_iClip2 == 0 ? INGRAM_TWIN_IDLE_NOSHOT_LEFT :
		INGRAM_TWIN_IDLE;
}
