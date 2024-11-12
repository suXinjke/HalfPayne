#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"

LINK_ENTITY_TO_CLASS( weapon_m249, CM249 );

//=========================================================
//=========================================================

void CM249::Spawn() {
	pev->classname = MAKE_STRING( "weapon_m249" );
	Precache();
	SET_MODEL( ENT( pev ), "models/w_saw.mdl" );
	m_iId = WEAPON_M249;

	m_iDefaultAmmo = M249_DEFAULT_GIVE;
	stress = 0.0f;
	stress2 = 0.0f;
	nextStressDecrease = gpGlobals->time + 0.1f;
	nextStress2Decrease = gpGlobals->time + 0.1f;

	FallInit();// get ready to fall down.
}


void CM249::Precache( void ) {
	PRECACHE_MODEL( "models/v_saw.mdl" );
	PRECACHE_MODEL( "models/w_saw.mdl" );
	PRECACHE_MODEL( "models/p_saw.mdl" );

	m_iShell = PRECACHE_MODEL( "models/saw_shell.mdl" );
	m_iLink = PRECACHE_MODEL( "models/saw_link.mdl" );
	PRECACHE_SOUND( "items/9mmclip1.wav" );

	PRECACHE_SOUND( "weapons/saw_fire1.wav" );
	PRECACHE_SOUND( "weapons/saw_fire2.wav" );
	PRECACHE_SOUND( "weapons/saw_fire3.wav" );

	PRECACHE_SOUND( "weapons/357_cock1.wav" );

	m_usM249 = PRECACHE_EVENT( 1, "events/m249.sc" );
}

void CM249::ItemPostFrame() {
	if ( !( m_pPlayer->pev->button & IN_ATTACK ) ) {
		if ( gpGlobals->time > nextStressDecrease ||
			fabs( nextStressDecrease - gpGlobals->time ) > 0.2f ) { // dumb in case of desync
			if ( gameplayMods::IsSlowmotionEnabled() ) {
				stress *= 0.7f;
			} else {
				stress *= 0.9f;
			}

			nextStressDecrease = gpGlobals->time + 0.1f;

			if ( stress <= 0.15f ) {
				stress = 0.0f;
			}
		}

		if ( gpGlobals->time > nextStress2Decrease ||
			fabs( nextStress2Decrease - gpGlobals->time ) > 0.2f ) { // dumb in case of desync
			stress2 *= 0.9f;

			nextStress2Decrease = gpGlobals->time + 0.1f;

			if ( stress2 <= 0.1f ) {
				stress2 = 0.0f;
			}
		}
	}


	CBasePlayerWeapon::ItemPostFrame();
}

int CM249::GetItemInfo( ItemInfo *p ) {
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "9mm";
	p->iMaxAmmo1 = _9MM_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iMaxClip2 = WEAPON_NOCLIP;
	p->iSlot = 3;
	p->iPosition = 4;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_M249;
	p->iWeight = M249_WEIGHT;

	return 1;
}

void CM249::UpdateBody() {
	int &ammo = m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType];

	pev->body =
		ammo <= 0 ? 8 :
		ammo <= 7 ? 9 - ammo :
		0;
}

BOOL CM249::Deploy() {
	UpdateBody();
	return DefaultDeploy( "models/v_saw.mdl", "models/p_saw.mdl", M249_DEPLOY, "mp5", 0, pev->body );
}

void CM249::PrimaryAttack() {
	if ( m_pPlayer->pev->waterlevel == 3 && !gameplayMods::shootUnderwater.isActive() ) {
		PlayEmptySound();

		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15;
		return;
	}
	
	int &ammo = m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType];

	if ( ammo <= 0 ) {
		if ( !m_fInReload ) {
			PlayEmptySound();

			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15;
		}

		return;
	}

	if ( !gameplayMods::infiniteAmmo.isActive() ) {
		ammo--;
	}

	UpdateBody();

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_pPlayer->pev->effects |= EF_MUZZLEFLASH;

	m_flNextAnimTime = UTIL_WeaponTimeBase() + 0.2;

	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

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

	Vector vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, vecBullet, 8192.0, BULLET_PLAYER_M249, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );


	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	int stressPacked = *( int * ) &stress;

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usM249, 0, ( float * ) &g_vecZero, ( float * ) &g_vecZero, vecDir.x, vecDir.y, pev->body, stressPacked, gameplayMods::PlayerShouldProducePhysicalBullets(), 0 );

	if ( !m_iClip ) {
		if ( m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] <= 0 ) {
			m_pPlayer->SetSuitUpdate( "!HEV_AMO0", SUIT_SENTENCE, SUIT_REPEAT_OK );
		}
	}

	float diff = ( 0.0923 - 0.0706 ) * ( stress2 / 2.0f );
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.0706 + diff;
	if ( gameplayMods::IsSlowmotionEnabled() ) {
		m_flNextPrimaryAttack *= 0.8f;
	}

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.1;

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

#ifndef  CLIENT_DLL

	const Vector& vecVelocity = m_pPlayer->pev->velocity;

	const float flZVel = vecVelocity.z;

	Vector vecInvPushDir = gpGlobals->v_forward * 35.0;

	float flNewZVel = CVAR_GET_FLOAT( "sv_maxspeed" );

	if ( vecInvPushDir.z >= 10.0 )
		flNewZVel = vecInvPushDir.z;

	Vector vecNewVel = vecVelocity - vecInvPushDir;

	//Restore Z velocity to make deathmatch easier.
	vecNewVel.z = flZVel;

	m_pPlayer->pev->velocity = vecNewVel;

#endif // ! CLIENT_DLL

	if ( stress < 3.0f ) {
		stress += 0.2f;
	}
	
	if ( stress2 < 2.0f ) {
		stress2 += 0.05f;
	}
}

void CM249::WeaponIdle( void ) {
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	SendWeaponAnim( M249_SLOWIDLE, 1, pev->body );

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.05;
}
