#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "soundent.h"
#include "player.h"
#include "gamerules.h"
#include "../fmt/printf.h"

// crowbar copycat with it's own roast

#define	CHICKEN_BODYHIT_VOLUME 128
#define	CHICKEN_WALLHIT_VOLUME 512

LINK_ENTITY_TO_CLASS( weapon_chicken, CChicken );


enum chicken_e {
	CHICKEN_IDLE_LIGHT1 = 0,
	CHICKEN_IDLE_LIGHT2,
	CHICKEN_IDLE_STRONG1,
	CHICKEN_IDLE_STRONG2,
	CHICKEN_DRAW_LIGHT,
	CHICKEN_DRAW_STRONG,
	CHICKEN_ATTACK_LIGHT1,
	CHICKEN_ATTACK_LIGHT2,
	CHICKEN_ATTACK_STRONG,
	CHICKEN_SWITCH_LIGHT_TO_STRONG,
	CHICKEN_SWITCH_STRONG_TO_LIGHT
};


#ifndef CLIENT_DLL

TYPEDESCRIPTION	CChicken::m_SaveData[] = {
	DEFINE_FIELD( CChicken, isAttacking, FIELD_BOOLEAN ),
	DEFINE_FIELD( CChicken, isLight, FIELD_BOOLEAN ),
};

int CChicken::Save( CSave &save ) {
	if ( !CBasePlayerWeapon::Save( save ) ) {
		return 0;
	}
	return save.WriteFields( "CChicken", this, m_SaveData, ARRAYSIZE( m_SaveData ) );
}

int CChicken::Restore( CRestore &restore ) {
	if ( !CBasePlayerWeapon::Restore( restore ) ) {
		return 0;
	}

	auto result = restore.ReadFields( "CChicken", this, m_SaveData, ARRAYSIZE( m_SaveData ) );

	pev->fuser1 = 0.2f;

	return result;
}

#endif


void CChicken::Spawn() {
	Precache();
	m_iId = WEAPON_CHICKEN;
	SET_MODEL( ENT( pev ), "models/w_chicken.mdl" );
	m_iClip = -1;
	isLight = TRUE;
	isAttacking = FALSE;

	FallInit();
}


void CChicken::Precache() {
	PRECACHE_MODEL( "models/v_chicken.mdl" );
	PRECACHE_MODEL( "models/w_chicken.mdl" );
	PRECACHE_MODEL( "models/p_crowbar.mdl" );
	PRECACHE_SOUND( "chicken/chick_draw.wav" );
	PRECACHE_SOUND( "chicken/chick_attack_light1.wav" );
	PRECACHE_SOUND( "chicken/chick_attack_light2.wav" );
	PRECACHE_SOUND( "chicken/chick_attack_light3.wav" );
	PRECACHE_SOUND( "chicken/chick_attack_light4.wav" );
	PRECACHE_SOUND( "chicken/chick_attack_light5.wav" );
	PRECACHE_SOUND( "chicken/chick_attack_strong1.wav" );
	PRECACHE_SOUND( "chicken/chick_attack_strong2.wav" );
	PRECACHE_SOUND( "chicken/chick_attack_strong3.wav" );
	PRECACHE_SOUND( "chicken/chick_attack_strong4.wav" );
	PRECACHE_SOUND( "weapons/cbar_hitbod1.wav" );
	PRECACHE_SOUND( "weapons/cbar_hitbod2.wav" );
	PRECACHE_SOUND( "weapons/cbar_hitbod3.wav" );
	PRECACHE_SOUND( "weapons/cbar_miss1.wav" );

	m_usChicken = PRECACHE_EVENT( 1, "events/chicken.sc" );
}

int CChicken::GetItemInfo( ItemInfo *p ) {
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iMaxClip2 = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 1;
	p->iId = WEAPON_CHICKEN;
	p->iWeight = CHICKEN_WEIGHT;
	return 1;
}



BOOL CChicken::Deploy() {
	isAttacking = FALSE;

	EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_ITEM, "chicken/chick_draw.wav", 1, ATTN_NORM );

	auto deployResult = DefaultDeploy( "models/v_chicken.mdl", "models/p_crowbar.mdl", isLight ? CHICKEN_DRAW_LIGHT : CHICKEN_DRAW_STRONG, "chicken" );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 10.0 + UTIL_SharedRandomFloat( m_pPlayer->random_seed, -2.0f, 6.0f );

	return deployResult;
}

void CChicken::ItemPostFrame() {

	if ( isLight ) {
		if ( !( m_pPlayer->pev->button & IN_ATTACK ) ) {
			m_iSwing = 0;
		}
	} else {
		if ( isAttacking && m_flNextPrimaryAttack <= 0.9 ) {
			isAttacking = FALSE;
			Swing( 0 );

			CSoundEnt::InsertSound( bits_SOUND_PLAYER, pev->origin, 1000.0f, 2.0 );
			CSoundEnt::InsertSound( bits_SOUND_DANGER, pev->origin, 1000.0f, 0.1 );
		}
	}

	CBasePlayerWeapon::ItemPostFrame();
}

void CChicken::PrimaryAttack() {

	if ( isLight ) {
		Swing( 1 );
	} else {
		SendWeaponAnim( CHICKEN_ATTACK_STRONG );
		m_flNextPrimaryAttack = GetNextAttackDelay( 2.111111111111111 );
		isAttacking = TRUE;

		EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_ITEM, fmt::sprintf( "chicken/chick_attack_strong%d.wav", aux::rand::uniformInt( 1, 4 ) ).c_str(), 1, ATTN_NORM );
	}
}

void CChicken::SecondaryAttack() {
	if ( isAttacking ) {
		return;
	}

	SendWeaponAnim( isLight ? CHICKEN_SWITCH_LIGHT_TO_STRONG : CHICKEN_SWITCH_STRONG_TO_LIGHT );
	isLight = !isLight;
	m_flNextPrimaryAttack = GetNextAttackDelay( 1.027777777777778 );
	m_flNextSecondaryAttack = GetNextAttackDelay( 1.027777777777778 );

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 10.0 + UTIL_SharedRandomFloat( m_pPlayer->random_seed, -2.0f, 6.0f );
}

int CChicken::Swing( int fFirst ) {
	int fDidHit = FALSE;

	Vector forward = m_pPlayer->GetAimForwardWithOffset();
	Vector forwardDeg = m_pPlayer->GetAimForwardWithOffset( true );

	TraceResult tr;

	UTIL_MakeVectors( forwardDeg );
	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecEnd = vecSrc + forward * 16;

	UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( m_pPlayer->pev ), &tr );

#ifndef CLIENT_DLL
	if ( tr.flFraction >= 1.0 ) {
		UTIL_TraceHull( vecSrc, vecEnd, dont_ignore_monsters, head_hull, ENT( m_pPlayer->pev ), &tr );
		if ( tr.flFraction < 1.0 ) {
			CBaseEntity *pHit = CBaseEntity::Instance( tr.pHit );
			if ( !pHit || pHit->IsBSPModel() ) {
				FindHullIntersection( vecSrc, tr, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, m_pPlayer->edict() );
			}
			vecEnd = tr.vecEndPos;
		}
	}
#endif

	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usChicken, 0.0, ( float * ) &g_vecZero, ( float * ) &g_vecZero, 0, 0, m_iSwing, isLight, 0, 0.0 );

	if ( tr.flFraction >= 1.0 ) {
		if ( fFirst ) {
			m_flNextPrimaryAttack = GetNextAttackDelay( 0.5 );
			m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

			m_iSwing = 0;
		}
	} else {
		if ( isLight ) {
			SendWeaponAnim( m_iSwing > 0 ? CHICKEN_ATTACK_LIGHT2 : CHICKEN_ATTACK_LIGHT1 );
		}

		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

#ifndef CLIENT_DLL

		fDidHit = TRUE;
		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );

		ClearMultiDamage();

		if ( ( m_flNextPrimaryAttack + 1 < UTIL_WeaponTimeBase() ) || g_pGameRules->IsMultiplayer() ) {
			pEntity->TraceAttack( m_pPlayer->pev, gSkillData.plrDmgCrowbar * ( isLight ? 1.0f : 5.0f ), forward, &tr, DMG_CLUB );
		} else {
			pEntity->TraceAttack( m_pPlayer->pev, ( gSkillData.plrDmgCrowbar / 2 ) * ( isLight ? 1.0f : 5.0f ), forward, &tr, DMG_CLUB );
		}
		ApplyMultiDamage( m_pPlayer->pev, m_pPlayer->pev );

		float flVol = 1.0;
		int fHitWorld = TRUE;

		if ( pEntity ) {
			if ( pEntity->Classify() != CLASS_NONE && pEntity->Classify() != CLASS_MACHINE ) {
				EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_AUTO, fmt::sprintf( "weapons/cbar_hitbod%d.wav", aux::rand::uniformInt( 1, 3 ) ).c_str(), 1, ATTN_NORM );
				if ( isLight ) {
					EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_ITEM, fmt::sprintf( "chicken/chick_attack_light%d.wav", aux::rand::uniformInt( 1, 5 ) ).c_str(), 1, ATTN_NORM );
				}

				m_pPlayer->m_iWeaponVolume = CHICKEN_BODYHIT_VOLUME;
				if ( !pEntity->IsAlive() )
					return TRUE;
				else
					flVol = 0.1;

				fHitWorld = FALSE;
			}
		}

		if ( fHitWorld ) {
			TEXTURETYPE_PlaySound( &tr, vecSrc, vecSrc + ( vecEnd - vecSrc ) * 2, BULLET_PLAYER_CROWBAR );

			if ( isLight ) {
				EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_ITEM, fmt::sprintf( "chicken/chick_attack_light%d.wav", aux::rand::uniformInt( 1, 5 ) ).c_str(), 1, ATTN_NORM );
			}

			m_trHit = tr;
		}

		m_pPlayer->m_iWeaponVolume = flVol * CHICKEN_WALLHIT_VOLUME;
#endif
		if ( isLight ) {
			m_flNextPrimaryAttack = GetNextAttackDelay( m_iSwing > 0 ? 0.34 : 0.28 );
			m_iSwing = m_iSwing > 0 ? 0 : 1;
		}

	}

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 10.0 + UTIL_SharedRandomFloat( m_pPlayer->random_seed, -2.0f, 6.0f );

	if ( fDidHit ) {
		CSoundEnt::InsertSound( bits_SOUND_PLAYER, pev->origin, 1000.0f, 0.3 );
	}

	return fDidHit;
}

void CChicken::WeaponIdle() {
	if ( !isAttacking && m_flTimeWeaponIdle < UTIL_WeaponTimeBase() ) {
		SendWeaponAnim( isLight ? CHICKEN_IDLE_LIGHT2 : CHICKEN_IDLE_STRONG2 );

		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 20.0 + UTIL_SharedRandomFloat( m_pPlayer->random_seed, -4.0f, 10.0f );
	}

	if ( pev->fuser1 > 0.0f ) {
		SendWeaponAnim( isLight ? CHICKEN_IDLE_LIGHT1 : CHICKEN_IDLE_STRONG1 );
	}
}
