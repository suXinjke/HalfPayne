#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gameplay_mod.h"


LINK_ENTITY_TO_CLASS( weapon_ingram, CIngram );


void CIngram::Spawn( )
{
	pev->classname = MAKE_STRING( "weapon_ingram" );
	Precache();
	m_iId = WEAPON_INGRAM;
	SET_MODEL(ENT(pev), "models/w_ingram.mdl");

	m_iDefaultAmmo = INGRAM_DEFAULT_GIVE;

	stress = 1.8f;
	nextStressDecrease = gpGlobals->time + 0.1f;

	FallInit();// get ready to fall down.
}


void CIngram::Precache( void )
{
	PRECACHE_MODEL("models/v_ingram.mdl");
	PRECACHE_MODEL("models/w_ingram.mdl");
	PRECACHE_MODEL("models/p_9mmhandgun.mdl");

	m_iShell = PRECACHE_MODEL ( "models/shell.mdl" );

	PRECACHE_SOUND( "items/9mmclip1.wav" );
	PRECACHE_SOUND( "weapons/ingram_shot.wav" );
	PRECACHE_SOUND( "weapons/ingram_clip_out.wav" );
	PRECACHE_SOUND( "weapons/ingram_bolt_pull.wav" );

	m_usFireIngram = PRECACHE_EVENT( 1, "events/ingram.sc" );
}

int CIngram::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "9mm";
	p->iMaxAmmo1 = _9MM_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = INGRAM_MAX_CLIP;
	p->iMaxClip2 = WEAPON_NOCLIP;
	p->iSlot = 2;
	p->iPosition = 3;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_INGRAM;
	p->iWeight = INGRAM_WEIGHT;

	return 1;
}

BOOL CIngram::Deploy( )
{
	int anim = INGRAM_DRAW;

	bool drawingFromTwin = false;

	if ( m_pPlayer->m_pLastItem ) {
		if ( m_pPlayer->m_pLastItem->m_iId == WEAPON_INGRAM_TWIN ) {
			drawingFromTwin = true;
		}
	}

	if ( drawingFromTwin ) {
		int leftClip = ( ( CGlockTwin * ) m_pPlayer->m_pLastItem )->m_iClip2;
		if ( leftClip > 0 ) {
			anim = m_iClip > 0 ? INGRAM_DRAW_FROM_TWIN : INGRAM_DRAW_FROM_TWIN_NOSHOT_RIGHT;
		} else {
			anim = m_iClip > 0 ? INGRAM_DRAW_FROM_TWIN_NOSHOT_LEFT : INGRAM_DRAW_FROM_TWIN_NOSHOT_BOTH;
		}
	} else if ( m_iClip == 0 ) {
		anim = INGRAM_DRAW_NOSHOT;
	}

	if ( m_pPlayer->slowMotionEnabled ) {
		anim++;
	}

	return DefaultDeploy( "models/v_ingram.mdl", "models/p_9mmhandgun.mdl", anim, "onehanded", /*UseDecrement() ? 1 : 0*/ 0 );
}

int CIngram::AddDuplicate( CBasePlayerItem *pOriginal ) {

#ifndef CLIENT_DLL
	CBasePlayer *player = ( CBasePlayer * ) CBasePlayer::Instance( g_engfuncs.pfnPEntityOfEntIndex( 1 ) );

	if ( !player->HasNamedPlayerItem( "weapon_ingram_twin" ) ) {
		CIngramTwin *twinIngrams = ( CIngramTwin * ) CBaseEntity::Create( "weapon_ingram_twin", pev->origin, Vector( 0, 0, 0 ), NULL );
		DispatchTouch( ENT( twinIngrams->pev ), ENT( player->pev ) );

		return true;
	} else {
		return CBasePlayerWeapon::AddDuplicate( pOriginal );
	}
#else
	return CBasePlayerWeapon::AddDuplicate( pOriginal );
#endif
}

int CIngram::Restore( CRestore &restore ) {
	int result = CBasePlayerWeapon::Restore( restore );
	if ( m_pPlayer ) {
		SendWeaponAnim( m_iClip == 0 ? INGRAM_IDLE_NOSHOT : INGRAM_IDLE );
	}
	return result;
}

void CIngram::ItemPostFrame(void) {

	if ( !(m_pPlayer->pev->button & IN_ATTACK) ) {

		if ( gpGlobals->time > nextStressDecrease ||
			fabs( nextStressDecrease - gpGlobals->time ) > 0.2f ) { // dumb in case of desync
			if ( m_pPlayer->slowMotionEnabled ) {
				stress *= 0.6f;
			} else {
				stress *= 0.8f;
			}

			nextStressDecrease = gpGlobals->time + 0.1f;

			if ( stress <= 1.9f ) {
				stress = 1.8f;
			}
		}
	}

	CBasePlayerWeapon::ItemPostFrame();
}

void CIngram::PrimaryAttack( void )
{
	if ( m_pPlayer->pev->waterlevel == 3 && !gameplayMods.shootUnderwater ) {
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	float delay = 0.048f;

	if (m_iClip <= 0)
	{
		if (m_fFireOnEmpty)
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = GetNextAttackDelay(0.2);
		}

		return;
	}

	if ( !gameplayMods.infiniteAmmoClip ) {
		m_iClip--;
	}

	UTIL_SetWeaponClip( WEAPON_INGRAM_TWIN, m_iClip );

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	int flags;

#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );
	Vector forward = m_pPlayer->GetAimForwardWithOffset();

#ifndef CLIENT_DLL
	if ( gameplayMods.bulletPhysical ) {

		float rightOffset = 8;

		vecSrc = vecSrc + forward * 5;
		if ( gameplayMods.upsideDown ) {
			rightOffset *= -1;
			vecSrc = vecSrc + Vector( 0, 0, 6 );
		}
		vecAiming = UTIL_VecSkew( vecSrc, vecAiming, rightOffset, ENT( pev ) );

		vecSrc = vecSrc + gpGlobals->v_right * rightOffset;
	}
#endif

	Vector vecDir;
	vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, VECTOR_CONE_2DEGREES * stress, 8192, BULLET_PLAYER_MP5, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );

	// There's not enough room for float type, so it's interpreted as int
	int stressPacked = *( int * ) &stress;
	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usFireIngram, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, stressPacked, 0, ( m_iClip == 0 ) ? 1 : 0, gameplayMods.bulletPhysical );

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay( delay );

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );

	if ( stress < 5.0f ) {
		stress += 0.5f;
	}
}

void CIngram::Reload( void )
{
	if ( m_pPlayer->ammo_9mm <= 0 )
		return;

	int iResult;

	float reloadTime = 1.5f;
	if ( m_iClip == 0 ) {
		reloadTime += 10.0f / 30.0f;
	}

	int anim = m_iClip == 0 ? INGRAM_RELOAD : INGRAM_RELOAD_NOT_EMPTY;
	if ( m_pPlayer->slowMotionEnabled ) {
		anim++;
	}

	iResult = DefaultReload( INGRAM_MAX_CLIP, anim, reloadTime, 0, reloadTime / 2.0f );

	if (iResult)
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	}
}

void CIngram::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	int anim = INGRAM_IDLE;

	if ( m_iClip <= 0 ) {
		anim = INGRAM_IDLE_NOSHOT;
	}

	SendWeaponAnim( anim, 2 );
}