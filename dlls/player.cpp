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
/*

===== player.cpp ========================================================

  functions dealing with the player

*/

#include "extdll.h"
#include "util.h"

#include "cbase.h"
#include "player.h"
#include "trains.h"
#include "nodes.h"
#include "weapons.h"
#include "soundent.h"
#include "monsters.h"
#include "shake.h"
#include "decals.h"
#include "gamerules.h"
#include "game.h"
#include "pm_shared.h"
#include "hltv.h"
#include "client.h"
#include "func_break.h"
#include "cgm_gamerules.h"

#include "gameplay_mod.h"

extern cvar_t *g_gl_vsync;
extern bool using_sys_timescale;
extern int g_changeLevelOccured;

float last_fps_max = 0.0f;

// #define DUCKFIX

extern DLL_GLOBAL ULONG		g_ulModelIndexPlayer;
extern DLL_GLOBAL BOOL		g_fGameOver;
extern DLL_GLOBAL	BOOL	g_fDrawLines;
int gEvilImpulse101;
extern DLL_GLOBAL int		g_iSkillLevel, gDisplayTitle;

extern "C" int				g_playerHasSuit;
extern "C" int				g_slowMotionCharge;
extern "C" int				g_divingAllowedWithoutSlowmotion;
extern "C" float			g_frictionOverride;
extern "C" int				g_noJumping;
extern "C" int				g_upsideDown;
extern "C" int				g_inverseControls;
extern "C" int				g_noWalking;
extern "C" int				g_doubleSpeed;

BOOL gInitHUD = TRUE;

extern void CopyToBodyQue(entvars_t* pev);
extern void respawn(entvars_t *pev, BOOL fCopyCorpse);
extern Vector VecBModelOrigin(entvars_t *pevBModel );
extern edict_t *EntSelectSpawnPoint( CBaseEntity *pPlayer );

// the world node graph
extern CGraph	WorldGraph;

#define TRAIN_ACTIVE	0x80 
#define TRAIN_NEW		0xc0
#define TRAIN_OFF		0x00
#define TRAIN_NEUTRAL	0x01
#define TRAIN_SLOW		0x02
#define TRAIN_MEDIUM	0x03
#define TRAIN_FAST		0x04 
#define TRAIN_BACK		0x05

#define	FLASH_DRAIN_TIME	 1.2 //100 units/3 minutes
#define	FLASH_CHARGE_TIME	 0.2 // 100 units/20 seconds  (seconds per unit)

#define SLOWMOTION_DRAIN_TIME 0.0288 // full slowmotion charge drains in 3 seconds (by ingame time)

#define TIMELEFT_UPDATE_TIME 0.001

// Global Savedata for player
TYPEDESCRIPTION	CBasePlayer::m_playerSaveData[] = 
{
	DEFINE_FIELD( CBasePlayer, m_flFlashLightTime, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, m_iFlashBattery, FIELD_INTEGER ),

	DEFINE_FIELD( CBasePlayer, m_afButtonLast, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_afButtonPressed, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_afButtonReleased, FIELD_INTEGER ),

	DEFINE_ARRAY( CBasePlayer, m_rgItems, FIELD_INTEGER, MAX_ITEMS ),
	DEFINE_FIELD( CBasePlayer, m_afPhysicsFlags, FIELD_INTEGER ),

	DEFINE_FIELD( CBasePlayer, m_flTimeStepSound, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, m_flTimeWeaponIdle, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, m_flSwimTime, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, m_flDuckTime, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, m_flWallJumpTime, FIELD_TIME ),

	DEFINE_FIELD( CBasePlayer, m_flSuitUpdate, FIELD_TIME ),
	DEFINE_ARRAY( CBasePlayer, m_rgSuitPlayList, FIELD_INTEGER, CSUITPLAYLIST ),
	DEFINE_FIELD( CBasePlayer, m_iSuitPlayNext, FIELD_INTEGER ),
	DEFINE_ARRAY( CBasePlayer, m_rgiSuitNoRepeat, FIELD_INTEGER, CSUITNOREPEAT ),
	DEFINE_ARRAY( CBasePlayer, m_rgflSuitNoRepeatTime, FIELD_TIME, CSUITNOREPEAT ),
	DEFINE_FIELD( CBasePlayer, m_lastDamageAmount, FIELD_INTEGER ),

	DEFINE_ARRAY( CBasePlayer, m_rgpPlayerItems, FIELD_CLASSPTR, MAX_ITEM_TYPES ),
	DEFINE_FIELD( CBasePlayer, m_pActiveItem, FIELD_CLASSPTR ),
	DEFINE_FIELD( CBasePlayer, m_pLastItem, FIELD_CLASSPTR ),

	DEFINE_ARRAY( CBasePlayer, m_rgAmmo, FIELD_INTEGER, MAX_AMMO_SLOTS ),
	DEFINE_FIELD( CBasePlayer, m_idrowndmg, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_idrownrestored, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_tSneaking, FIELD_TIME ),

	DEFINE_FIELD( CBasePlayer, m_iTrain, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_bitsHUDDamage, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_flFallVelocity, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, m_iTargetVolume, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_iWeaponVolume, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_iExtraSoundTypes, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_iWeaponFlash, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_fLongJump, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBasePlayer, m_fInitHUD, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBasePlayer, m_tbdPrev, FIELD_TIME ),

	DEFINE_FIELD( CBasePlayer, m_pTank, FIELD_EHANDLE ),
	DEFINE_FIELD( CBasePlayer, m_iHideHUD, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_iFOV, FIELD_INTEGER ),

	DEFINE_ARRAY( CBasePlayer, hookedModelIndexes, FIELD_STRING, MAX_HOOKED_MODEL_INDEXES ),
	DEFINE_FIELD( CBasePlayer, hookedModelIndexesCount, FIELD_INTEGER ),

	DEFINE_ARRAY( CBasePlayer, hookedMapsWithKerotans, FIELD_STRING, 128 ),
	DEFINE_FIELD( CBasePlayer, hookedMapsWithKerotansCount, FIELD_INTEGER ),

	DEFINE_ARRAY( CBasePlayer, soundQueueSoundNames, FIELD_STRING, MAX_SOUND_QUEUE ),
	DEFINE_ARRAY( CBasePlayer, soundQueueSoundDelays, FIELD_FLOAT, MAX_SOUND_QUEUE ),
	DEFINE_ARRAY( CBasePlayer, soundQueueIsMaxPayneCommentarySound, FIELD_BOOLEAN, MAX_SOUND_QUEUE ),
	DEFINE_ARRAY( CBasePlayer, soundQueueIsMaxPayneCommentaryImportant, FIELD_BOOLEAN, MAX_SOUND_QUEUE ),
	DEFINE_FIELD( CBasePlayer, soundQueueCounter, FIELD_INTEGER ),

	DEFINE_ARRAY( CBasePlayer, visitedMaps, FIELD_STRING, MAX_VISITED_MAPS ),
	DEFINE_FIELD( CBasePlayer, visitedMapsCount, FIELD_INTEGER ),

	DEFINE_FIELD( CBasePlayer, aimOffsetX, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, aimOffsetY, FIELD_FLOAT ),

	DEFINE_FIELD( CBasePlayer, slowMotionWasEnabled, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBasePlayer, slowMotionUpdateTime, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, slowMotionCharge, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, desiredTimeScale, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, nextSmoothTimeScaleChange, FIELD_TIME ),

	DEFINE_FIELD( CBasePlayer, superHotMultiplier, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, nextSuperHotMultiplierUpdate, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, superHotJumping, FIELD_TIME ),

	DEFINE_FIELD( CBasePlayer, jumpedOnce, FIELD_BOOLEAN ),

	DEFINE_FIELD( CBasePlayer, loadoutReceived, FIELD_BOOLEAN ),
	
	DEFINE_FIELD( CBasePlayer, painkillerCount, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, painkillerEnergy, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, nextPainkillerEffectTime, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, nextPainkillerEffectTimePeriod, FIELD_FLOAT ),

	DEFINE_FIELD( CBasePlayer, lastDamageTime, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, healthChargeTime, FIELD_TIME ),

	DEFINE_FIELD( CBasePlayer, showCredits, FIELD_BOOLEAN ),

	DEFINE_FIELD( CBasePlayer, gameTitleShown, FIELD_BOOLEAN ),

	DEFINE_FIELD( CBasePlayer, nextAttackSlowmotionOffset, FIELD_FLOAT ),

	DEFINE_FIELD( CBasePlayer, desperation, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, untilNextDesperation, FIELD_TIME ),

	DEFINE_FIELD( CBasePlayer, lastHealingTime, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, bleedTime, FIELD_TIME ),

	DEFINE_FIELD( CBasePlayer, fadeOutTime, FIELD_TIME ),

	DEFINE_FIELD( CBasePlayer, crystalsDestroyed, FIELD_INTEGER ),

	DEFINE_FIELD( CBasePlayer, postRestoreDelay, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, postSpawnDelay, FIELD_TIME ),

	DEFINE_FIELD( CBasePlayer, musicFile, FIELD_STRING ),
	DEFINE_FIELD( CBasePlayer, musicPos, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, musicLooping, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBasePlayer, musicNoSlowmotionEffects, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBasePlayer, musicGoingThroughChangeLevel, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBasePlayer, currentMusicPlaylistIndex, FIELD_INTEGER ),

	DEFINE_FIELD( CBasePlayer, delayedMusicFilePath, FIELD_STRING ),
	DEFINE_FIELD( CBasePlayer, delayedMusicStartTime, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, delayedMusicStartPos, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, delayedMusicLooping, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBasePlayer, delayedMusicNoSlowmotionEffects, FIELD_BOOLEAN ),
	
	//DEFINE_FIELD( CBasePlayer, m_fDeadTime, FIELD_FLOAT ), // only used in multiplayer games
	//DEFINE_FIELD( CBasePlayer, m_fGameHUDInitialized, FIELD_INTEGER ), // only used in multiplayer games
	//DEFINE_FIELD( CBasePlayer, m_flStopExtraSoundTime, FIELD_TIME ),
	//DEFINE_FIELD( CBasePlayer, m_fKnownItem, FIELD_INTEGER ), // reset to zero on load
	//DEFINE_FIELD( CBasePlayer, m_iPlayerSound, FIELD_INTEGER ),	// Don't restore, set in Precache()
	//DEFINE_FIELD( CBasePlayer, m_pentSndLast, FIELD_EDICT ),	// Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_flSndRoomtype, FIELD_FLOAT ),	// Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_flSndRange, FIELD_FLOAT ),	// Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_fNewAmmo, FIELD_INTEGER ), // Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_flgeigerRange, FIELD_FLOAT ),	// Don't restore, reset in Precache()
	//DEFINE_FIELD( CBasePlayer, m_flgeigerDelay, FIELD_FLOAT ),	// Don't restore, reset in Precache()
	//DEFINE_FIELD( CBasePlayer, m_igeigerRangePrev, FIELD_FLOAT ),	// Don't restore, reset in Precache()
	//DEFINE_FIELD( CBasePlayer, m_iStepLeft, FIELD_INTEGER ), // Don't need to restore
	//DEFINE_ARRAY( CBasePlayer, m_szTextureName, FIELD_CHARACTER, CBTEXTURENAMEMAX ), // Don't need to restore
	//DEFINE_FIELD( CBasePlayer, m_chTextureType, FIELD_CHARACTER ), // Don't need to restore
	//DEFINE_FIELD( CBasePlayer, m_fNoPlayerSound, FIELD_BOOLEAN ), // Don't need to restore, debug
	//DEFINE_FIELD( CBasePlayer, m_iUpdateTime, FIELD_INTEGER ), // Don't need to restore
	//DEFINE_FIELD( CBasePlayer, m_iClientHealth, FIELD_INTEGER ), // Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_iClientBattery, FIELD_INTEGER ), // Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_iClientHideHUD, FIELD_INTEGER ), // Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_fWeapon, FIELD_BOOLEAN ),  // Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_nCustomSprayFrames, FIELD_INTEGER ), // Don't restore, depends on server message after spawning and only matters in multiplayer
	//DEFINE_FIELD( CBasePlayer, m_vecAutoAim, FIELD_VECTOR ), // Don't save/restore - this is recomputed
	//DEFINE_ARRAY( CBasePlayer, m_rgAmmoLast, FIELD_INTEGER, MAX_AMMO_SLOTS ), // Don't need to restore
	//DEFINE_FIELD( CBasePlayer, m_fOnTarget, FIELD_BOOLEAN ), // Don't need to restore
	//DEFINE_FIELD( CBasePlayer, m_nCustomSprayFrames, FIELD_INTEGER ), // Don't need to restore
	
};	


int giPrecacheGrunt = 0;
int gmsgShake = 0;
int gmsgFade = 0;
int gmsgSelAmmo = 0;
int gmsgFlashlight = 0;
int gmsgFlashBattery = 0;
int gmsgResetHUD = 0;
int gmsgInitHUD = 0;
int gmsgShowGameTitle = 0;
int gmsgCurWeapon = 0;
int gmsgLckWeapon = 0;
int gmsgRemWeapon = 0;
int gmsgHealth = 0;
int gmsgSlowMotion = 0;
int gmsgPainkillerCount = 0;
int gmsgDamage = 0;
int gmsgBattery = 0;
int gmsgTrain = 0;
int gmsgLogo = 0;
int gmsgWeaponList = 0;
int gmsgAmmoX = 0;
int gmsgHudText = 0;
int gmsgDeathMsg = 0;
int gmsgScoreInfo = 0;
int gmsgTeamInfo = 0;
int gmsgTeamScore = 0;
int gmsgGameMode = 0;
int gmsgMOTD = 0;
int gmsgServerName = 0;
int gmsgAmmoPickup = 0;
int gmsgWeapPickup = 0;
int gmsgItemPickup = 0;
int gmsgHideWeapon = 0;
int gmsgSetCurWeap = 0;
int gmsgSayText = 0;
int gmsgTextMsg = 0;
int gmsgSetFOV = 0;
int gmsgShowMenu = 0;
int gmsgGeigerRange = 0;
int gmsgTeamNames = 0;
int gmsgConcuss = 0;
int gmsgFadeOut = 0;
int gmsgFlash = 0;
int gmsgBassPlay = 0;
int gmsgBassStop = 0;
int gmsgBassStopC = 0;
int gmsgBassComm = 0;

int gmsgStatusText = 0;
int gmsgStatusValue = 0; 

int gmsgAimCoords = 0; 
int gmsgSetSkin = 0;
int gmsgAimOffset = 0;

int gmsgOnSound = 0;
int gmsgSubtClear = 0;
int gmsgSubtRemove = 0;

edict_t *aimLastEntity = 0;
int gmsgOnAimNew = 0;
int gmsgOnAimUpd = 0;
int gmsgOnAimClear = 0;
int gmsgOnPlyUpd = 0;
int gmsgKillConfirmed = 0;

void LinkUserMessages( void )
{
	// Already taken care of?
	if ( gmsgSelAmmo )
	{
		return;
	}

	// DONT USE MESSAGE NAMES LONGER THAN 10 CHARS

	gmsgSelAmmo = REG_USER_MSG("SelAmmo", sizeof(SelAmmo));
	gmsgCurWeapon = REG_USER_MSG("CurWeapon", 4);
	gmsgLckWeapon = REG_USER_MSG("LckWeapon", 2);
	gmsgGeigerRange = REG_USER_MSG("Geiger", 1);
	gmsgFlashlight = REG_USER_MSG("Flashlight", 2);
	gmsgFlashBattery = REG_USER_MSG("FlashBat", 1);
	gmsgHealth = REG_USER_MSG( "Health", 6 );
	gmsgSlowMotion = REG_USER_MSG( "SlowMotion", 1 );
	gmsgOnSound = REG_USER_MSG( "OnSound", -1 );
	gmsgPainkillerCount = REG_USER_MSG( "PillCount", 1 );
	gmsgDamage = REG_USER_MSG( "Damage", 12 );
	gmsgBattery = REG_USER_MSG( "Battery", 2);
	gmsgTrain = REG_USER_MSG( "Train", 1);
	//gmsgHudText = REG_USER_MSG( "HudTextPro", -1 );
	gmsgHudText = REG_USER_MSG( "HudText", -1 ); // we don't use the message but 3rd party addons may!
	gmsgSayText = REG_USER_MSG( "SayText", -1 );
	gmsgTextMsg = REG_USER_MSG( "TextMsg", -1 );
	gmsgWeaponList = REG_USER_MSG("WeaponList", -1);
	gmsgResetHUD = REG_USER_MSG("ResetHUD", 1);		// called every respawn
	gmsgInitHUD = REG_USER_MSG("InitHUD", 0 );		// called every time a new player joins the server
	gmsgShowGameTitle = REG_USER_MSG("GameTitle", 1);
	gmsgDeathMsg = REG_USER_MSG( "DeathMsg", -1 );
	gmsgScoreInfo = REG_USER_MSG( "ScoreInfo", 9 );
	gmsgTeamInfo = REG_USER_MSG( "TeamInfo", -1 );  // sets the name of a player's team
	gmsgTeamScore = REG_USER_MSG( "TeamScore", -1 );  // sets the score of a team on the scoreboard
	gmsgGameMode = REG_USER_MSG( "GameMode", 1 );
	gmsgMOTD = REG_USER_MSG( "MOTD", -1 );
	gmsgServerName = REG_USER_MSG( "ServerName", -1 );
	gmsgAmmoPickup = REG_USER_MSG( "AmmoPickup", 2 );
	gmsgWeapPickup = REG_USER_MSG( "WeapPickup", 1 );
	gmsgItemPickup = REG_USER_MSG( "ItemPickup", -1 );
	gmsgHideWeapon = REG_USER_MSG( "HideWeapon", 1 );
	gmsgSetFOV = REG_USER_MSG( "SetFOV", 1 );
	gmsgShowMenu = REG_USER_MSG( "ShowMenu", -1 );
	gmsgShake = REG_USER_MSG("ScreenShake", sizeof(ScreenShake));
	gmsgFade = REG_USER_MSG("ScreenFade", sizeof(ScreenFade));
	gmsgAmmoX = REG_USER_MSG("AmmoX", 2);
	gmsgTeamNames = REG_USER_MSG( "TeamNames", -1 );

	gmsgStatusText = REG_USER_MSG("StatusText", -1);
	gmsgStatusValue = REG_USER_MSG("StatusValue", 3); 
	gmsgAimCoords = REG_USER_MSG( "AimCoords", 8 );
	gmsgSetSkin = REG_USER_MSG( "SetSkin", 1 );
	gmsgAimOffset = REG_USER_MSG( "AimOffset", 8 );
	gmsgConcuss = REG_USER_MSG( "Concuss", 1 );
	gmsgFadeOut = REG_USER_MSG( "FadeOut", 1 );
	gmsgFlash = REG_USER_MSG( "Flash", 8 );

	gmsgBassPlay = REG_USER_MSG( "BassPlay", -1 );
	gmsgBassStop = REG_USER_MSG( "BassStop", 1 );
	gmsgBassStopC = REG_USER_MSG( "BassStopC", 0 );
	gmsgBassComm = REG_USER_MSG( "BassComm", -1 );

	gmsgSubtClear = REG_USER_MSG( "SubtClear", 0 );
	gmsgSubtRemove = REG_USER_MSG( "SubtRemove", -1 );

	gmsgOnAimNew = REG_USER_MSG( "OnAimNew", -1 );
	gmsgOnAimUpd = REG_USER_MSG( "OnAimUpd", 28 );
	gmsgOnAimClear = REG_USER_MSG( "OnAimClear", 0 );

	gmsgOnPlyUpd = REG_USER_MSG( "OnPlyUpd", 30 );

	gmsgKillConfirmed = REG_USER_MSG( "KillConf", 0 );
}

LINK_ENTITY_TO_CLASS( player, CBasePlayer );



void CBasePlayer :: Pain( void )
{
	float	flRndSound;//sound randomizer

	flRndSound = RANDOM_FLOAT ( 0 , 1 ); 
	
	if ( flRndSound <= 0.33 )
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_pain5.wav", 1, ATTN_NORM);
	else if ( flRndSound <= 0.66 )	
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_pain6.wav", 1, ATTN_NORM);
	else
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_pain7.wav", 1, ATTN_NORM);
}

/* 
 *
 */
Vector VecVelocityForDamage(float flDamage)
{
	Vector vec(RANDOM_FLOAT(-100,100), RANDOM_FLOAT(-100,100), RANDOM_FLOAT(200,300));

	if (flDamage > -50)
		vec = vec * 0.7;
	else if (flDamage > -200)
		vec = vec * 2;
	else
		vec = vec * 10;
	
	return vec;
}

#if 0 /*
static void ThrowGib(entvars_t *pev, char *szGibModel, float flDamage)
{
	edict_t *pentNew = CREATE_ENTITY();
	entvars_t *pevNew = VARS(pentNew);

	pevNew->origin = pev->origin;
	SET_MODEL(ENT(pevNew), szGibModel);
	UTIL_SetSize(pevNew, g_vecZero, g_vecZero);

	pevNew->velocity		= VecVelocityForDamage(flDamage);
	pevNew->movetype		= MOVETYPE_BOUNCE;
	pevNew->solid			= SOLID_NOT;
	pevNew->avelocity.x		= RANDOM_FLOAT(0,600);
	pevNew->avelocity.y		= RANDOM_FLOAT(0,600);
	pevNew->avelocity.z		= RANDOM_FLOAT(0,600);
	CHANGE_METHOD(ENT(pevNew), em_think, SUB_Remove);
	pevNew->ltime		= gpGlobals->time;
	pevNew->nextthink	= gpGlobals->time + RANDOM_FLOAT(10,20);
	pevNew->frame		= 0;
	pevNew->flags		= 0;
}
	
	
static void ThrowHead(entvars_t *pev, char *szGibModel, floatflDamage)
{
	SET_MODEL(ENT(pev), szGibModel);
	pev->frame			= 0;
	pev->nextthink		= -1;
	pev->movetype		= MOVETYPE_BOUNCE;
	pev->takedamage		= DAMAGE_NO;
	pev->solid			= SOLID_NOT;
	pev->view_ofs		= Vector(0,0,8);
	UTIL_SetSize(pev, Vector(-16,-16,0), Vector(16,16,56));
	pev->velocity		= VecVelocityForDamage(flDamage);
	pev->avelocity		= RANDOM_FLOAT(-1,1) * Vector(0,600,0);
	pev->origin.z -= 24;
	ClearBits(pev->flags, FL_ONGROUND);
}


*/ 
#endif

int TrainSpeed(int iSpeed, int iMax)
{
	float fSpeed, fMax;
	int iRet = 0;

	fMax = (float)iMax;
	fSpeed = iSpeed;

	fSpeed = fSpeed/fMax;

	if (iSpeed < 0)
		iRet = TRAIN_BACK;
	else if (iSpeed == 0)
		iRet = TRAIN_NEUTRAL;
	else if (fSpeed < 0.33)
		iRet = TRAIN_SLOW;
	else if (fSpeed < 0.66)
		iRet = TRAIN_MEDIUM;
	else
		iRet = TRAIN_FAST;

	return iRet;
}

void CBasePlayer :: DeathSound( void )
{
	// water death sounds
	/*
	if (pev->waterlevel == 3)
	{
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/h2odeath.wav", 1, ATTN_NONE);
		return;
	}
	*/

	// temporarily using pain sounds for death sounds
	switch (RANDOM_LONG(1,5)) 
	{
	case 1: 
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_pain5.wav", 1, ATTN_NORM);
		break;
	case 2: 
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_pain6.wav", 1, ATTN_NORM);
		break;
	case 3: 
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_pain7.wav", 1, ATTN_NORM);
		break;
	}

	EMIT_SOUND( ENT( pev ), CHAN_AUTO, "var/death.wav", 1, ATTN_NORM, true );
}

// override takehealth
// bitsDamageType indicates type of damage healed. 

int CBasePlayer :: TakeHealth( float flHealth, int bitsDamageType )
{
	if ( gameplayMods::noHealing.isActive() ) {
		return 0;
	}

	return CBaseMonster :: TakeHealth (flHealth, bitsDamageType);

}

int CBasePlayer::TakePainkiller()
{
	if ( painkillerCount >= MAX_PAINKILLERS || gameplayMods::noPainkillers.isActive() ) {
		return 0;
	}
	
	painkillerCount++;
	MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pev );
		WRITE_STRING( "item_healthkit" ); // hardcoded, but I don't have to do custom messages in that case
	MESSAGE_END( );

	EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/pills.wav", 1, ATTN_NORM, true );

	if (
		CVAR_GET_FLOAT( "max_commentary_painkiller_pickup" ) > 0.0f &&
		RANDOM_LONG( 0, 100 ) < 33 &&
		gpGlobals->time > allowedToReactOnPainkillerPickup
	) {
		char fileName[256];
		sprintf_s( fileName, "max/painkiller/FIND_PILLS_%d.wav", RANDOM_LONG( 1, 16 ) );
		TryToPlayMaxCommentary( MAKE_STRING( fileName ), false );

		allowedToReactOnPainkillerPickup = gpGlobals->time + 30.0f;
	}

	return 1;
}

void CBasePlayer::UsePainkiller()
{
	if ( painkillerCount <= 0 ) {
		return;
	}

	if ( ( pev->health + painkillerEnergy ) < pev->max_health || ( gameplayMods::fadingOut.isActive() && gameplayModsData.fade <= 180 ) ) {
		TakeHealth( gameplayMods::painkillersSlow.isActive() ? 0 : 20, DMG_GENERIC );

		painkillerCount--;
		lastHealingTime = gpGlobals->time;
		if ( auto bleeding = gameplayMods::bleeding.isActive<BleedingInfo>() ) {
			lastHealingTime += bleeding->immunityPeriod;
		}
		if ( gameplayMods::painkillersSlow.isActive() ) {
			painkillerEnergy += min( 20, pev->max_health - ( pev->health + painkillerEnergy ) );
		}

		if ( gameplayMods::fadingOut.isActive() ) {
			gameplayModsData.fade = 255;
		}

		// white screen flash
		MESSAGE_BEGIN( MSG_ONE, gmsgFlash, NULL, pev );
			WRITE_BYTE( 255 );
			WRITE_BYTE( 255 );
			WRITE_BYTE( 255 );
			WRITE_BYTE( 120 );
			WRITE_FLOAT( 0.12 );
		MESSAGE_END();
		EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/pills_use.wav", 1, ATTN_NORM, true );

		if (
			CVAR_GET_FLOAT( "max_commentary_painkiller_use" ) > 0.0f &&
			RANDOM_LONG( 0, 100 ) < 33 &&
			gpGlobals->time > allowedToReactOnPainkillerTake
		) {
			char fileName[256];
			sprintf_s( fileName, "max/painkiller/TAKE_PILLS_%d.wav", RANDOM_LONG( 1, 8 ) );
			TryToPlayMaxCommentary( MAKE_STRING( fileName ), false );

			allowedToReactOnPainkillerTake = gpGlobals->time + 30.0f;
		}
	}
}

void CBasePlayer::TakeSlowmotionCharge( int slowMotionCharge )
{
	this->slowMotionCharge += slowMotionCharge;
	this->slowMotionCharge = min( this->slowMotionCharge, MAX_SLOWMOTION_CHARGE );
}





void CBasePlayer::GiveAll( bool nonCheat ) {
	gEvilImpulse101 = TRUE;
	GiveNamedItem( "item_suit", nonCheat );
	GiveNamedItem( "item_battery", nonCheat );
	GiveNamedItem( "weapon_crowbar", nonCheat );
	GiveNamedItem( "weapon_9mmhandgun", nonCheat );
	GiveNamedItem( "weapon_9mmhandgun_twin", nonCheat );
	GiveNamedItem( "weapon_ingram", nonCheat );
	GiveNamedItem( "weapon_ingram_twin", nonCheat );
	GiveNamedItem( "ammo_9mmclip", nonCheat );
	GiveNamedItem( "weapon_shotgun", nonCheat );
	GiveNamedItem( "ammo_buckshot", nonCheat );
	GiveNamedItem( "weapon_9mmAR", nonCheat );
	GiveNamedItem( "ammo_9mmAR", nonCheat );
	GiveNamedItem( "weapon_m249", nonCheat );
	GiveNamedItem( "ammo_ARgrenades", nonCheat );
	GiveNamedItem( "weapon_handgrenade", nonCheat );
	GiveNamedItem( "weapon_tripmine", nonCheat );
#ifndef OEM_BUILD
	GiveNamedItem( "weapon_357", nonCheat );
	GiveNamedItem( "ammo_357", nonCheat );
	GiveNamedItem( "weapon_crossbow", nonCheat );
	GiveNamedItem( "ammo_crossbow", nonCheat );
	GiveNamedItem( "weapon_egon", nonCheat );
	GiveNamedItem( "weapon_gauss", nonCheat );
	GiveNamedItem( "ammo_gaussclip", nonCheat );
	GiveNamedItem( "weapon_rpg", nonCheat );
	GiveNamedItem( "ammo_rpgclip", nonCheat );
	GiveNamedItem( "weapon_satchel", nonCheat );
	GiveNamedItem( "weapon_snark", nonCheat );
	GiveNamedItem( "weapon_hornetgun", nonCheat );
#endif
	gEvilImpulse101 = FALSE;
}

void CBasePlayer::SetEvilImpulse101( bool evilImpulse101 ) {
	gEvilImpulse101 = evilImpulse101;
}

// Intercepts certain entity kills by player
// Gives slowmotion charge based on that and forwards the killed entity to custom game mode
void CBasePlayer::OnKilledEntity( CBaseEntity *victim )
{
	const char *victimName = STRING( victim->pev->classname );

	KILLED_ENTITY_TYPE killedEntity = KILLED_ENTITY_UNDEFINED;
	bool maySwearAfterKill = false;
	
	if ( strcmp( victimName, "monster_alien_controller" ) == 0 ) {
		killedEntity = KILLED_ENTITY_ALIEN_CONTROLLER;
		maySwearAfterKill = true;
	}
	else if ( strcmp( victimName, "monster_alien_grunt" ) == 0 ) {
		killedEntity = KILLED_ENTITY_ALIEN_GRUNT;
		maySwearAfterKill = true;
	}
	else if ( strcmp( victimName, "monster_alien_slave" ) == 0 ) {
		killedEntity = KILLED_ENTITY_ALIEN_SLAVE;
		maySwearAfterKill = true;
	}
	else if ( strcmp( victimName, "monster_apache" ) == 0 ) {
		killedEntity = KILLED_ENTITY_ARMORED_VEHICLE;
		maySwearAfterKill = true;
	}
	else if ( strcmp( victimName, "monster_babycrab" ) == 0 ) {
		killedEntity = KILLED_ENTITY_BABYCRAB;
	}
	else if ( strcmp( victimName, "monster_barnacle" ) == 0 ) {
		killedEntity = KILLED_ENTITY_BARNACLE;
		maySwearAfterKill = true;
	}
	else if ( strcmp( victimName, "monster_bigmomma" ) == 0 ) {
		killedEntity = KILLED_ENTITY_BIG_MOMMA;
		maySwearAfterKill = true;
	}
	else if ( strcmp( victimName, "monster_bullchicken" ) == 0 ) {
		killedEntity = KILLED_ENTITY_BULLSQUID;
		maySwearAfterKill = true;
	}
	else if ( strcmp( victimName, "monster_gargantua" ) == 0 ) {
		killedEntity = KILLED_ENTITY_GARGANTUA;
		maySwearAfterKill = true;
	}
	else if ( strcmp( victimName, "monster_headcrab" ) == 0 ) {
		killedEntity = KILLED_ENTITY_HEADCRAB;
		maySwearAfterKill = true;
	}
	else if ( strcmp( victimName, "monster_houndeye" ) == 0 ) {
		killedEntity = KILLED_ENTITY_HOUNDEYE;
		maySwearAfterKill = true;
	}
	else if ( strcmp( victimName, "monster_human_assassin" ) == 0 ) {
		killedEntity = KILLED_ENTITY_HUMAN_ASSASSIN;
		maySwearAfterKill = true;
	}
	else if ( strcmp( victimName, "monster_human_grunt" ) == 0 ) {
		killedEntity = KILLED_ENTITY_HUMAN_GRUNT;
		maySwearAfterKill = true;
	}
	else if ( strcmp( victimName, "monster_ichthyosaur" ) == 0 ) {
		killedEntity = KILLED_ENTITY_ICHTYOSAUR;
		maySwearAfterKill = true;
	}
	else if ( strcmp( victimName, "monster_miniturret" ) == 0 ) {
		killedEntity = KILLED_ENTITY_MINITURRET;
		maySwearAfterKill = true;
	}
	else if ( strcmp( victimName, "monster_sentry" ) == 0 ) {
		killedEntity = KILLED_ENTITY_SENTRY;
		maySwearAfterKill = true;
	}
	else if ( strcmp( victimName, "monster_snark" ) == 0 ) {
		bool snarkOwnedByPlayer = victim->pev->owner != 0;
		
		if ( !snarkOwnedByPlayer ) {
			killedEntity = KILLED_ENTITY_SNARK;
			maySwearAfterKill = true;
		}
	}
	else if ( strcmp( victimName, "monster_zombie" ) == 0 ) {
		killedEntity = KILLED_ENTITY_ZOMBIE;
		maySwearAfterKill = true;
	}

	else if ( strcmp( victimName, "grenade" ) == 0 ) {
		killedEntity = KILLED_ENTITY_GRENADE;
	} else if ( strcmp( victimName, "monster_barney" ) == 0 ) {
		killedEntity = KILLED_ENTITY_BARNEY;
		ComplainAboutKillingInnocent();
	} else if ( strcmp( victimName, "monster_scientist" ) == 0 ) {
		killedEntity = KILLED_ENTITY_SCIENTIST;
		ComplainAboutKillingInnocent();
	} else if ( strcmp( victimName, "monster_gman" ) == 0 ) {
		killedEntity = KILLED_ENTITY_GMAN;

		if ( desperation == DESPERATION_NO && FStrEq( STRING( gpGlobals->mapname ), "c5a1" ) ) {
			desperation = DESPERATION_REVENGE;
			STOP_SOUND( victim->edict(), CHAN_VOICE, "!GM_FINAL" );
			MESSAGE_BEGIN( MSG_ALL, gmsgSubtRemove );
				WRITE_STRING( "!GM_FINAL" );
			MESSAGE_END();
		}

		if ( desperation == DESPERATION_REVENGE ) {
			untilNextDesperation = gpGlobals->time + 0.5f;
		}

	}
	
	else if ( strcmp( victimName, "func_tankmortar" ) == 0 || strcmp( victimName, "func_tankrocket" ) == 0 ) {
		killedEntity = KILLED_ENTITY_ARMORED_VEHICLE;
		maySwearAfterKill = true;
	}
	else if ( victim->killedOrCausedByPlayer && strstr( STRING( victim->pev->target ), "sniper_die" ) ) {
		killedEntity = KILLED_ENTITY_SNIPER;
		maySwearAfterKill = true;
	}
	else if ( victim->killedOrCausedByPlayer && strstr( STRING( victim->pev->target ), "crystal" ) && strcmp( STRING( gpGlobals->mapname ), "c4a1" ) ) {
		killedEntity = KILLED_ENTITY_NIHILANTH_CRYSTAL;

		crystalsDestroyed++;

		switch ( crystalsDestroyed ) {
			case 1:
				AddToSoundQueue( MAKE_STRING( "comment/onedowntwotogo.wav" ), 0.6f, true, true );
				break;

			case 2:
				AddToSoundQueue( MAKE_STRING( "comment/twodownonetogo.wav" ), 0.6f, true, true );
				break;
		}
	}

	if ( ( maySwearAfterKill && gameplayMods::swearOnKill.isActive() ) || desperation == DESPERATION_FIGHTING ) {
		SayRandomSwear();
	}

	TakeSlowmotionCharge( KilledEntityToSlowmotionCharge( killedEntity ) );

	BOOL isHeadshot = false;
	if ( CBaseMonster *monsterVictim = dynamic_cast< CBaseMonster * >( victim ) ) {
		if ( monsterVictim->m_LastHitGroup == HITGROUP_HEAD ) {
			isHeadshot = true;
		}
	}
	
	BOOL killedByExplosion = victim->killedByExplosion;
	BOOL killedByEnvExplosion = victim->killedByEnvExplosion;
	BOOL killedByCrowbar = victim->killedByCrowbar;

	if ( killedEntity != KILLED_ENTITY_UNDEFINED ) {
		if ( CHalfLifeRules *rules = dynamic_cast< CHalfLifeRules * >( g_pGameRules ) ) {
			rules->OnKilledEntityByPlayer( this, victim, killedEntity, isHeadshot, killedByExplosion, killedByEnvExplosion, killedByCrowbar );
		}

		if ( auto healOnKillPercent = gameplayMods::healOnKill.isActive<float>() ) {
			TakeHealth( max( 1, victim->pev->max_health * ( *healOnKillPercent * 0.01f ) ), DMG_GENERIC );
		}	
		
		if ( gameplayMods::timescaleOnDamage.isActive() ) {
			gameplayModsData.timescaleAdditive += 0.33f;
			if ( gameplayModsData.timescaleAdditive > 1.0f ) {
				gameplayModsData.timescaleAdditive = 1.0f;
			}
		}

		MESSAGE_BEGIN( MSG_ONE, gmsgKillConfirmed, NULL, pev );
		MESSAGE_END();
	}
}

int CBasePlayer::KilledEntityToSlowmotionCharge( KILLED_ENTITY_TYPE killedEntity )
{
	switch ( killedEntity ) {
		case KILLED_ENTITY_BIG_MOMMA:
		case KILLED_ENTITY_GARGANTUA:
		case KILLED_ENTITY_ARMORED_VEHICLE:
		case KILLED_ENTITY_ICHTYOSAUR:
			return 100;

		case KILLED_ENTITY_ALIEN_CONTROLLER:
		case KILLED_ENTITY_ALIEN_GRUNT:
		case KILLED_ENTITY_HUMAN_ASSASSIN:
		case KILLED_ENTITY_HUMAN_GRUNT:
		case KILLED_ENTITY_SNIPER:
			return 20;

		case KILLED_ENTITY_ALIEN_SLAVE:
		case KILLED_ENTITY_MINITURRET:
		case KILLED_ENTITY_SENTRY:
			return 15;

		case KILLED_ENTITY_BULLSQUID:
		case KILLED_ENTITY_ZOMBIE:
			return 10;

		case KILLED_ENTITY_HEADCRAB:
		case KILLED_ENTITY_HOUNDEYE:
		case KILLED_ENTITY_SNARK:
			return 5;

		case KILLED_ENTITY_BABYCRAB:
		case KILLED_ENTITY_BARNACLE:
			return 1;

		default:
			return 0;
	}
}

Vector CBasePlayer :: GetGunPosition( )
{
//	UTIL_MakeVectors(pev->v_angle);
//	m_HackedGunPos = pev->view_ofs;
	Vector origin;
	
	origin = pev->origin + pev->view_ofs;

	return origin;
}

//=========================================================
// TraceAttack
//=========================================================
void CBasePlayer :: TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	if ( pev->takedamage )
	{
		m_LastHitGroup = ptr->iHitgroup;

		switch ( ptr->iHitgroup )
		{
		case HITGROUP_GENERIC:
			break;
		case HITGROUP_HEAD:
			flDamage *= gSkillData.plrHead;
			break;
		case HITGROUP_CHEST:
			flDamage *= gSkillData.plrChest;
			break;
		case HITGROUP_STOMACH:
			flDamage *= gSkillData.plrStomach;
			break;
		case HITGROUP_LEFTARM:
		case HITGROUP_RIGHTARM:
			flDamage *= gSkillData.plrArm;
			break;
		case HITGROUP_LEFTLEG:
		case HITGROUP_RIGHTLEG:
			flDamage *= gSkillData.plrLeg;
			break;
		default:
			break;
		}

		SpawnBlood(ptr->vecEndPos, BloodColor(), flDamage);// a little surface blood.
		TraceBleed( flDamage, vecDir, ptr, bitsDamageType );
		AddMultiDamage( pevAttacker, this, flDamage, bitsDamageType );
	}
}

/*
	Take some damage.  
	NOTE: each call to TakeDamage with bitsDamageType set to a time-based damage
	type will cause the damage time countdown to be reset.  Thus the ongoing effects of poison, radiation
	etc are implemented with subsequent calls to TakeDamage using DMG_GENERIC.
*/

#define ARMOR_RATIO	 0.2	// Armor Takes 80% of the damage
#define ARMOR_BONUS  0.5	// Each Point of Armor is work 1/x points of health

int CBasePlayer :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if ( gameplayMods::noHealing.isActive() && flDamage < 0 ) {
		return 0;
	}

	if (
		gameplayMods::teleportOnKill.isActive() &&
		pevInflictor == pev && pevAttacker == pev &&
		m_pActiveItem && FStrEq( STRING( m_pActiveItem->pev->classname ), "weapon_gauss" )
	) {
		return 0;
	}

	// have suit diagnose the problem - ie: report damage type
	int bitsDamage = bitsDamageType;
	int ffound = TRUE;
	int fmajor;
	int fcritical;
	int fTookDamage;
	int ftrivial;
	float flRatio;
	float flBonus;
	float flHealthPrev = pev->health;

	flBonus = ARMOR_BONUS;
	flRatio = ARMOR_RATIO;

	if ( gameplayMods::noFallDamage.isActive() && ( bitsDamageType & DMG_FALL ) ) {
		return 0;
	}

	if ( ( bitsDamageType & DMG_BLAST ) && g_pGameRules->IsMultiplayer() )
	{
		// blasts damage armor more.
		flBonus *= 2;
	}

	// Already dead
	if ( !IsAlive() )
		return 0;
	// go take the damage first

	
	CBaseEntity *pAttacker = pevAttacker ? CBaseEntity::Instance( pevAttacker ) : NULL;
	CBaseEntity *pInflctor = pevInflictor ? CBaseEntity::Instance( pevInflictor ) : NULL;

	if ( gameplayMods::oneHitKO.isActive() ) {
		CBaseMonster *isMonster = dynamic_cast< CBaseMonster * >( pAttacker );

		bool isSquidspit = FStrEq( STRING( pAttacker->pev->classname ), "squidspit" );
		bool isTank = FStrEq( STRING( pAttacker->pev->classname ), "func_tank" );
		if ( isMonster || isSquidspit || isTank ) {
			flDamage = pev->health + 1;
		}
	}

	bool gonnaDie = flDamage >= pev->health;

	// this looks more complicated than it should
	
	// always play pain sounds when falling, but never play sounds if you're going to die
	if ( ( bitsDamage & DMG_FALL || CVAR_GET_FLOAT( "max_commentary_pain" ) > 0.0f ) && !gonnaDie ) {
		if ( bitsDamageType & ( DMG_BULLET | DMG_BLAST | DMG_FALL | DMG_SHOCK | DMG_CLUB | DMG_CRUSH | DMG_ENERGYBEAM | DMG_SLASH | DMG_SONIC ) ) {

			// don't play pain sounds too often, but always play a sound after falling
			if (
				bitsDamage & DMG_FALL || (
					( ( gpGlobals->time > allowedToReactOnSeriousInjury && flDamage >= 30.0f ) || ( gpGlobals->time > allowedToReactOnMinorInjury && flDamage < 30.0f ) ) &&
					RANDOM_LONG( 0, 100 ) < 66
				)
			) {
				char fileName[256];
				if ( flDamage < 30.0f ) {
					sprintf_s( fileName, "max/pain/MINOR_PAIN_%d.wav", RANDOM_LONG( 1, 16 ) );
					EMIT_SOUND( ENT( pev ), CHAN_STATIC, fileName, 1, ATTN_NORM, true );
					allowedToReactOnMinorInjury = gpGlobals->time + 1.0f;
				} else {
					if ( bitsDamage & DMG_FALL ) {
						sprintf_s( fileName, "max/pain/SERIOUS_PAIN_%d.wav", RANDOM_LONG( 8, 13 ) ); // DUMB: a subset of serious pain sounds which is alright only for falls
					} else {
						sprintf_s( fileName, "max/pain/SERIOUS_PAIN_%d.wav", RANDOM_LONG( 1, 13 ) );
					}
					EMIT_SOUND( ENT( pev ), CHAN_STATIC, fileName, 1, ATTN_NORM, true );
					allowedToReactOnSeriousInjury = gpGlobals->time + 10.0f;

					// if you've fallen down or made such an injury with explosive yourself - leave a remark
					if (
						pAttacker &&
						CVAR_GET_FLOAT( "max_commentary_pain_self" ) > 0.0f && (
							bitsDamageType & DMG_FALL ||
							strcmp( STRING( pAttacker->pev->classname ), "player" ) == 0 ||
							( pAttacker && pAttacker->auxOwner && ( strcmp( STRING( pAttacker->auxOwner->v.classname ), "player" ) == 0 ) )
						)
					) {
						sprintf_s( fileName, "max/pain/SELF_PAIN_%d.wav", RANDOM_LONG( 1, 20 ) );

						// sorry for this memory leak
						AddToSoundQueue( ALLOC_STRING( fileName ), 1.5f, true );
					}
				}
			}
		}
	}

	if ( pAttacker && !g_pGameRules->FPlayerCanTakeDamage( this, pAttacker ) )
	{
		// Refuse the damage
		return 0;
	}

	// keep track of amount of damage last sustained
	m_lastDamageAmount = flDamage;

	if ( gameplayMods::slowmotionOnDamage.isActive() ) {
		if ( CBaseMonster *monster = dynamic_cast<CBaseMonster *>( pInflctor ) ) {
			if ( flDamage > 0 && ( bitsDamageType & ( DMG_BULLET | DMG_SLASH | DMG_CLUB | DMG_SHOCK | DMG_SONIC | DMG_ENERGYBEAM ) ) ) {
				TakeSlowmotionCharge( max( 1, flDamage / 2.0f ) );
			}
		}
	}

	if ( gameplayMods::timescaleOnDamage.isActive() ) {
		if ( CBaseMonster *monster = dynamic_cast<CBaseMonster *>( pInflctor ) ) {
			if ( flDamage > 0 && ( bitsDamageType & ( DMG_BULLET | DMG_SLASH | DMG_CLUB | DMG_SHOCK | DMG_SONIC | DMG_ENERGYBEAM ) ) ) {
				gameplayModsData.timescaleAdditive -= ( flDamage / 50.0f );
				if ( gameplayModsData.timescaleAdditive < -0.25f ) {
					gameplayModsData.timescaleAdditive = -0.25f;
				}
			}
		}
	}

	// Armor. 
	if (pev->armorvalue && !(bitsDamageType & (DMG_FALL | DMG_DROWN)) )// armor doesn't protect against fall or drown damage!
	{
		float flNew = flDamage * flRatio;

		float flArmor;

		flArmor = (flDamage - flNew) * flBonus;

		// Does this use more armor than we have?
		if (flArmor > pev->armorvalue)
		{
			flArmor = pev->armorvalue;
			flArmor *= (1/flBonus);
			flNew = flDamage - flArmor;
			pev->armorvalue = 0;
		}
		else
			pev->armorvalue -= flArmor;
		
		flDamage = flNew;
	}

	// this cast to INT is critical!!! If a player ends up with 0.5 health, the engine will get that
	// as an int (zero) and think the player is dead! (this will incite a clientside screentilt, etc)
	fTookDamage = CBaseMonster::TakeDamage(pevInflictor, pevAttacker, (int)flDamage, bitsDamageType);

	// reset damage time countdown for each type of time based damage player just sustained

	{
		for (int i = 0; i < CDMG_TIMEBASED; i++)
			if (bitsDamageType & (DMG_PARALYZE << i))
				m_rgbTimeBasedDamage[i] = 0;
	}

	// tell director about it
	MESSAGE_BEGIN( MSG_SPEC, SVC_DIRECTOR );
		WRITE_BYTE ( 9 );	// command length in bytes
		WRITE_BYTE ( DRC_CMD_EVENT );	// take damage event
		WRITE_SHORT( ENTINDEX(this->edict()) );	// index number of primary entity
		WRITE_SHORT( ENTINDEX(ENT(pevInflictor)) );	// index number of secondary entity
		WRITE_LONG( 5 );   // eventflags (priority and flags)
	MESSAGE_END();


	// how bad is it, doc?

	ftrivial = (pev->health > 75 || m_lastDamageAmount < 5);
	fmajor = (m_lastDamageAmount > 25);
	fcritical = (pev->health < 30);

	// handle all bits set in this damage message,
	// let the suit give player the diagnosis

	// UNDONE: add sounds for types of damage sustained (ie: burn, shock, slash )

	// UNDONE: still need to record damage and heal messages for the following types

		// DMG_BURN	
		// DMG_FREEZE
		// DMG_BLAST
		// DMG_SHOCK

	m_bitsDamageType |= bitsDamage; // Save this so we can report it to the client
	m_bitsHUDDamage = -1;  // make sure the damage bits get resent

	while (fTookDamage && (!ftrivial || (bitsDamage & DMG_TIMEBASED)) && ffound && bitsDamage)
	{
		ffound = FALSE;

		if (bitsDamage & DMG_CLUB)
		{
			if (fmajor)
				SetSuitUpdate("!HEV_DMG4", FALSE, SUIT_NEXT_IN_30SEC);	// minor fracture
			bitsDamage &= ~DMG_CLUB;
			ffound = TRUE;
		}
		if (bitsDamage & (DMG_FALL | DMG_CRUSH))
		{
			if (fmajor)
				SetSuitUpdate("!HEV_DMG5", FALSE, SUIT_NEXT_IN_30SEC);	// major fracture
			else
				SetSuitUpdate("!HEV_DMG4", FALSE, SUIT_NEXT_IN_30SEC);	// minor fracture
	
			bitsDamage &= ~(DMG_FALL | DMG_CRUSH);
			ffound = TRUE;
		}
		
		if (bitsDamage & DMG_BULLET)
		{
			if (m_lastDamageAmount > 5)
				SetSuitUpdate("!HEV_DMG6", FALSE, SUIT_NEXT_IN_30SEC);	// blood loss detected
			//else
			//	SetSuitUpdate("!HEV_DMG0", FALSE, SUIT_NEXT_IN_30SEC);	// minor laceration
			
			bitsDamage &= ~DMG_BULLET;
			ffound = TRUE;
		}

		if (bitsDamage & DMG_SLASH)
		{
			if (fmajor)
				SetSuitUpdate("!HEV_DMG1", FALSE, SUIT_NEXT_IN_30SEC);	// major laceration
			else
				SetSuitUpdate("!HEV_DMG0", FALSE, SUIT_NEXT_IN_30SEC);	// minor laceration

			bitsDamage &= ~DMG_SLASH;
			ffound = TRUE;
		}
		
		if (bitsDamage & DMG_SONIC)
		{
			if (fmajor)
				SetSuitUpdate("!HEV_DMG2", FALSE, SUIT_NEXT_IN_1MIN);	// internal bleeding
			bitsDamage &= ~DMG_SONIC;
			ffound = TRUE;
		}

		if (bitsDamage & (DMG_POISON | DMG_PARALYZE))
		{
			SetSuitUpdate("!HEV_DMG3", FALSE, SUIT_NEXT_IN_1MIN);	// blood toxins detected
			bitsDamage &= ~(DMG_POISON | DMG_PARALYZE);
			ffound = TRUE;
		}

		if (bitsDamage & DMG_ACID)
		{
			SetSuitUpdate("!HEV_DET1", FALSE, SUIT_NEXT_IN_1MIN);	// hazardous chemicals detected
			bitsDamage &= ~DMG_ACID;
			ffound = TRUE;
		}

		if (bitsDamage & DMG_NERVEGAS)
		{
			SetSuitUpdate("!HEV_DET0", FALSE, SUIT_NEXT_IN_1MIN);	// biohazard detected
			bitsDamage &= ~DMG_NERVEGAS;
			ffound = TRUE;
		}

		if (bitsDamage & DMG_RADIATION)
		{
			SetSuitUpdate("!HEV_DET2", FALSE, SUIT_NEXT_IN_1MIN);	// radiation detected
			bitsDamage &= ~DMG_RADIATION;
			ffound = TRUE;
		}
		if (bitsDamage & DMG_SHOCK)
		{
			bitsDamage &= ~DMG_SHOCK;
			ffound = TRUE;
		}
	}

	pev->punchangle.x = -2;

	if (fTookDamage && !ftrivial && fmajor && flHealthPrev >= 75) 
	{
		// first time we take major damage...
		// turn automedic on if not on
		SetSuitUpdate("!HEV_MED1", FALSE, SUIT_NEXT_IN_30MIN);	// automedic on

		// give morphine shot if not given recently
		SetSuitUpdate("!HEV_HEAL7", FALSE, SUIT_NEXT_IN_30MIN);	// morphine shot
	}
	
	if (fTookDamage && !ftrivial && fcritical && flHealthPrev < 75)
	{

		// already took major damage, now it's critical...
		if (pev->health < 6)
			SetSuitUpdate("!HEV_HLTH3", FALSE, SUIT_NEXT_IN_10MIN);	// near death
		else if (pev->health < 20)
			SetSuitUpdate("!HEV_HLTH2", FALSE, SUIT_NEXT_IN_10MIN);	// health critical
	
		// give critical health warnings
		if (!RANDOM_LONG(0,3) && flHealthPrev < 50)
			SetSuitUpdate("!HEV_DMG7", FALSE, SUIT_NEXT_IN_5MIN); //seek medical attention
	}

	// if we're taking time based damage, warn about its continuing effects
	if (fTookDamage && (bitsDamageType & DMG_TIMEBASED) && flHealthPrev < 75)
		{
			if (flHealthPrev < 50)
			{
				if (!RANDOM_LONG(0,3))
					SetSuitUpdate("!HEV_DMG7", FALSE, SUIT_NEXT_IN_5MIN); //seek medical attention
			}
			else
				SetSuitUpdate("!HEV_HLTH1", FALSE, SUIT_NEXT_IN_10MIN);	// health dropping
		}

	lastDamageTime = gpGlobals->time;
	if ( auto healthRegeneration = gameplayMods::healthRegeneration.isActive<HealthRegenerationInfo>() ) {
		lastDamageTime += healthRegeneration->delay;
	}
	return fTookDamage;
}

//=========================================================
// PackDeadPlayerItems - call this when a player dies to
// pack up the appropriate weapons and ammo items, and to
// destroy anything that shouldn't be packed.
//
// This is pretty brute force :(
//=========================================================
void CBasePlayer::PackDeadPlayerItems( void )
{
	int iWeaponRules;
	int iAmmoRules;
	int i;
	CBasePlayerWeapon *rgpPackWeapons[ 20 ];// 20 hardcoded for now. How to determine exactly how many weapons we have?
	int iPackAmmo[ MAX_AMMO_SLOTS + 1];
	int iPW = 0;// index into packweapons array
	int iPA = 0;// index into packammo array

	memset(rgpPackWeapons, 0, sizeof(rgpPackWeapons) );
	memset(iPackAmmo, -1, sizeof(iPackAmmo) );

	// get the game rules 
	iWeaponRules = g_pGameRules->DeadPlayerWeapons( this );
 	iAmmoRules = g_pGameRules->DeadPlayerAmmo( this );

	if ( iWeaponRules == GR_PLR_DROP_GUN_NO && iAmmoRules == GR_PLR_DROP_AMMO_NO )
	{
		// nothing to pack. Remove the weapons and return. Don't call create on the box!
		RemoveAllItems( TRUE );
		return;
	}

// go through all of the weapons and make a list of the ones to pack
	for ( i = 0 ; i < MAX_ITEM_TYPES ; i++ )
	{
		if ( m_rgpPlayerItems[ i ] )
		{
			// there's a weapon here. Should I pack it?
			CBasePlayerItem *pPlayerItem = m_rgpPlayerItems[ i ];

			while ( pPlayerItem )
			{
				switch( iWeaponRules )
				{
				case GR_PLR_DROP_GUN_ACTIVE:
					if ( m_pActiveItem && pPlayerItem == m_pActiveItem )
					{
						// this is the active item. Pack it.
						rgpPackWeapons[ iPW++ ] = (CBasePlayerWeapon *)pPlayerItem;
					}
					break;

				case GR_PLR_DROP_GUN_ALL:
					rgpPackWeapons[ iPW++ ] = (CBasePlayerWeapon *)pPlayerItem;
					break;

				default:
					break;
				}

				pPlayerItem = pPlayerItem->m_pNext;
			}
		}
	}

// now go through ammo and make a list of which types to pack.
	if ( iAmmoRules != GR_PLR_DROP_AMMO_NO )
	{
		for ( i = 0 ; i < MAX_AMMO_SLOTS ; i++ )
		{
			if ( m_rgAmmo[ i ] > 0 )
			{
				// player has some ammo of this type.
				switch ( iAmmoRules )
				{
				case GR_PLR_DROP_AMMO_ALL:
					iPackAmmo[ iPA++ ] = i;
					break;

				case GR_PLR_DROP_AMMO_ACTIVE:
					if ( m_pActiveItem && i == m_pActiveItem->PrimaryAmmoIndex() ) 
					{
						// this is the primary ammo type for the active weapon
						iPackAmmo[ iPA++ ] = i;
					}
					else if ( m_pActiveItem && i == m_pActiveItem->SecondaryAmmoIndex() ) 
					{
						// this is the secondary ammo type for the active weapon
						iPackAmmo[ iPA++ ] = i;
					}
					break;

				default:
					break;
				}
			}
		}
	}

// create a box to pack the stuff into.
	CWeaponBox *pWeaponBox = (CWeaponBox *)CBaseEntity::Create( "weaponbox", pev->origin, pev->angles, edict() );

	pWeaponBox->pev->angles.x = 0;// don't let weaponbox tilt.
	pWeaponBox->pev->angles.z = 0;

	pWeaponBox->SetThink( &CWeaponBox::Kill );
	pWeaponBox->pev->nextthink = gpGlobals->time + 120;

// back these two lists up to their first elements
	iPA = 0;
	iPW = 0;

// pack the ammo
	while ( iPackAmmo[ iPA ] != -1 )
	{
		pWeaponBox->PackAmmo( MAKE_STRING( CBasePlayerItem::AmmoInfoArray[ iPackAmmo[ iPA ] ].pszName ), m_rgAmmo[ iPackAmmo[ iPA ] ] );
		iPA++;
	}

// now pack all of the items in the lists
	while ( rgpPackWeapons[ iPW ] )
	{
		// weapon unhooked from the player. Pack it into der box.
		pWeaponBox->PackWeapon( rgpPackWeapons[ iPW ] );

		iPW++;
	}

	pWeaponBox->pev->velocity = pev->velocity * 1.2;// weaponbox has player's velocity, then some.

	RemoveAllItems( TRUE );// now strip off everything that wasn't handled by the code above.
}

void CBasePlayer::ApplyWeaponPushback( float impulse ) {
	if ( auto pushBackMultiplier = gameplayMods::weaponPushBack.isActive<float>() ) {
		pev->velocity = pev->velocity - ( GetAimForwardWithOffset() * impulse * *pushBackMultiplier );
	}
}

void CBasePlayer::SendWeaponLockInfo() {
	CBasePlayerItem *pItem;

	for ( int i = 0; i < MAX_ITEM_TYPES; i++ ) {
		pItem = m_rgpPlayerItems[i];

		while ( pItem ) {
			if ( CBasePlayerWeapon *weapon = dynamic_cast< CBasePlayerWeapon * >( pItem ) ) {
				MESSAGE_BEGIN( MSG_ONE, gmsgLckWeapon, NULL, pev );
					WRITE_BYTE( weapon->m_iId );
					WRITE_BYTE( weapon->locked );
				MESSAGE_END();
			}
			pItem = pItem->m_pNext;
		}
	}
}

void CBasePlayer::RemoveAllItems( BOOL removeSuit )
{
	while ( painkillerCount > 0 ) {
		TakeHealth( 20, DMG_GENERIC );
		painkillerCount--;
	}

	if ( gameplayMods::weaponRestricted.isActive() && pev->deadflag == DEAD_NO ) {
		return;
	}

	if (m_pActiveItem)
	{
		ResetAutoaim( );
		m_pActiveItem->Holster( );
		m_pActiveItem = NULL;
	}

	m_pLastItem = NULL;

	if ( m_pTank != NULL )
	{
		m_pTank->Use( this, this, USE_OFF, 0 );
		m_pTank = NULL;
	}

	int i;
	CBasePlayerItem *pPendingItem;
	for (i = 0; i < MAX_ITEM_TYPES; i++)
	{
		m_pActiveItem = m_rgpPlayerItems[i];
		while (m_pActiveItem)
		{
			pPendingItem = m_pActiveItem->m_pNext; 
			m_pActiveItem->Drop( );
			m_pActiveItem = pPendingItem;
		}
		m_rgpPlayerItems[i] = NULL;
	}
	m_pActiveItem = NULL;

	pev->viewmodel		= 0;
	pev->weaponmodel	= 0;
	
	if ( removeSuit )
		pev->weapons = 0;
	else
		pev->weapons &= ~WEAPON_ALLWEAPONS;

	for ( i = 0; i < MAX_AMMO_SLOTS;i++)
		m_rgAmmo[i] = 0;

	UpdateClientData();
	// send Selected Weapon Message to our client
	MESSAGE_BEGIN( MSG_ONE, gmsgCurWeapon, NULL, pev );
		WRITE_BYTE(0);
		WRITE_BYTE(0);
		WRITE_BYTE(0);
		WRITE_BYTE(0);
	MESSAGE_END();
}

/*
 * GLOBALS ASSUMED SET:  g_ulModelIndexPlayer
 *
 * ENTITY_METHOD(PlayerDie)
 */
entvars_t *g_pevLastInflictor;  // Set in combat.cpp.  Used to pass the damage inflictor for death messages.
								// Better solution:  Add as parameter to all Killed() functions.

void CBasePlayer::Killed( entvars_t *pevAttacker, int iGib )
{
	CSound *pSound;

	// Holster weapon immediately, to allow it to cleanup
	if ( m_pActiveItem )
		m_pActiveItem->Holster( );

	g_pGameRules->PlayerKilled( this, pevAttacker, g_pevLastInflictor );

	if ( m_pTank != NULL )
	{
		m_pTank->Use( this, this, USE_OFF, 0 );
		m_pTank = NULL;
	}

	// this client isn't going to be thinking for a while, so reset the sound until they respawn
	pSound = CSoundEnt::SoundPointerForIndex( CSoundEnt::ClientSoundIndex( edict() ) );
	{
		if ( pSound )
		{
			pSound->Reset();
		}
	}

	SetAnimation( PLAYER_DIE );
	
	m_iRespawnFrames = 0;

	pev->modelindex = g_ulModelIndexPlayer;    // don't use eyes

	pev->deadflag		= DEAD_DYING;
	pev->movetype		= MOVETYPE_TOSS;
	ClearBits( pev->flags, FL_ONGROUND );
	if (pev->velocity.z < 10)
		pev->velocity.z += RANDOM_FLOAT(0,300);

	// clear out the suit message cache so we don't keep chattering
	SetSuitUpdate(NULL, FALSE, 0);

	// send "health" update message to zero
	m_iClientHealth = 0;
	MESSAGE_BEGIN( MSG_ONE, gmsgHealth, NULL, pev );
		WRITE_LONG( m_iClientHealth );
		WRITE_SHORT( painkillerEnergy );
	MESSAGE_END();

	// Tell Ammo Hud that the player is dead
	MESSAGE_BEGIN( MSG_ONE, gmsgCurWeapon, NULL, pev );
		WRITE_BYTE(0);
		WRITE_BYTE(0XFF);
		WRITE_BYTE(0xFF);
		WRITE_BYTE(0xFF);
	MESSAGE_END();

	// reset FOV
	pev->fov = m_iFOV = m_iClientFOV = 0;

	MESSAGE_BEGIN( MSG_ONE, gmsgSetFOV, NULL, pev );
		WRITE_BYTE(0);
	MESSAGE_END();


	// UNDONE: Put this in, but add FFADE_PERMANENT and make fade time 8.8 instead of 4.12
	// UTIL_ScreenFade( edict(), Vector(128,0,0), 6, 15, 255, FFADE_OUT | FFADE_MODULATE );

	DeathSound();

	if ( ( pev->health < -40 && iGib != GIB_NEVER && !FStrEq( STRING( gpGlobals->mapname ), "nightmare" ) ) || iGib == GIB_ALWAYS )
	{
		pev->solid			= SOLID_NOT;
		GibMonster();	// This clears pev->model
		pev->effects |= EF_NODRAW;
		pev->movetype = MOVETYPE_NONE; // so the death cam will be onspot
	}

	pev->angles.x = 0;
	pev->angles.z = 0;

	pev->skin = 1; // DEATH FACE - embedded in model

	pev->flags &= ~FL_DIVING;
	FlashlightTurnOff();

	SendStopMusicMessage();

	SetSlowMotion( true );
	SetThink(&CBasePlayer::PlayerDeathThink);
	pev->nextthink = gpGlobals->time + 0.1;
}


// Set the activity based on an event or current state
void CBasePlayer::SetAnimation( PLAYER_ANIM playerAnim )
{
	int animDesired;
	float speed;
	char szAnim[64];

	speed = pev->velocity.Length2D();

	if (pev->flags & FL_FROZEN)
	{
		speed = 0;
		playerAnim = PLAYER_IDLE;
	}

	switch (playerAnim) 
	{
	case PLAYER_JUMP:
		m_IdealActivity = ACT_HOP;
		break;
	
	case PLAYER_SUPERJUMP:
		m_IdealActivity = ACT_LEAP;
		break;
	
	case PLAYER_DIE:
		m_IdealActivity = ACT_DIESIMPLE;
		m_IdealActivity = GetDeathActivity( );
		break;

	case PLAYER_ATTACK1:	
		switch( m_Activity )
		{
		case ACT_HOVER:
		case ACT_SWIM:
		case ACT_HOP:
		case ACT_LEAP:
		case ACT_DIESIMPLE:
			m_IdealActivity = m_Activity;
			break;
		default:
			m_IdealActivity = ACT_RANGE_ATTACK1;
			break;
		}
		break;
	case PLAYER_IDLE:
	case PLAYER_WALK:
		if ( !FBitSet( pev->flags, FL_ONGROUND ) && (m_Activity == ACT_HOP || m_Activity == ACT_LEAP) )	// Still jumping
		{
			m_IdealActivity = m_Activity;
		}
		else if ( pev->waterlevel > 1 )
		{
			if ( speed == 0 )
				m_IdealActivity = ACT_HOVER;
			else
				m_IdealActivity = ACT_SWIM;
		}
		else
		{
			m_IdealActivity = ACT_WALK;
		}
		break;
	}

	switch (m_IdealActivity)
	{
	case ACT_HOVER:
	case ACT_LEAP:
	case ACT_SWIM:
	case ACT_HOP:
	case ACT_DIESIMPLE:
	default:
		if ( m_Activity == m_IdealActivity)
			return;
		m_Activity = m_IdealActivity;

		animDesired = LookupActivity( m_Activity );
		// Already using the desired animation?
		if (pev->sequence == animDesired)
			return;

		pev->gaitsequence = 0;
		pev->sequence		= animDesired;
		pev->frame			= 0;
		ResetSequenceInfo( );
		return;

	case ACT_RANGE_ATTACK1:
		if ( FBitSet( pev->flags, FL_DUCKING ) )	// crouching
			strcpy( szAnim, "crouch_shoot_" );
		else
			strcpy( szAnim, "ref_shoot_" );
		strcat( szAnim, m_szAnimExtention );
		animDesired = LookupSequence( szAnim );
		if (animDesired == -1)
			animDesired = 0;

		if ( pev->sequence != animDesired || !m_fSequenceLoops )
		{
			pev->frame = 0;
		}

		if (!m_fSequenceLoops)
		{
			pev->effects |= EF_NOINTERP;
		}

		m_Activity = m_IdealActivity;

		pev->sequence		= animDesired;
		ResetSequenceInfo( );
		break;

	case ACT_WALK:
		if (m_Activity != ACT_RANGE_ATTACK1 || m_fSequenceFinished)
		{
			if ( FBitSet( pev->flags, FL_DUCKING ) )	// crouching
				strcpy( szAnim, "crouch_aim_" );
			else
				strcpy( szAnim, "ref_aim_" );
			strcat( szAnim, m_szAnimExtention );
			animDesired = LookupSequence( szAnim );
			if (animDesired == -1)
				animDesired = 0;
			m_Activity = ACT_WALK;
		}
		else
		{
			animDesired = pev->sequence;
		}
	}

	if ( FBitSet( pev->flags, FL_DUCKING ) )
	{
		if ( speed == 0)
		{
			pev->gaitsequence	= LookupActivity( ACT_CROUCHIDLE );
			// pev->gaitsequence	= LookupActivity( ACT_CROUCH );
		}
		else
		{
			pev->gaitsequence	= LookupActivity( ACT_CROUCH );
		}
	}
	else if ( speed > 220 )
	{
		pev->gaitsequence	= LookupActivity( ACT_RUN );
	}
	else if (speed > 0)
	{
		pev->gaitsequence	= LookupActivity( ACT_WALK );
	}
	else
	{
		// pev->gaitsequence	= LookupActivity( ACT_WALK );
		pev->gaitsequence	= LookupSequence( "deep_idle" );
	}


	// Already using the desired animation?
	if (pev->sequence == animDesired)
		return;

	//ALERT( at_console, "Set animation to %d\n", animDesired );
	// Reset to first frame of desired animation
	pev->sequence		= animDesired;
	pev->frame			= 0;
	ResetSequenceInfo( );
}

/*
===========
TabulateAmmo
This function is used to find and store 
all the ammo we have into the ammo vars.
============
*/
void CBasePlayer::TabulateAmmo()
{
	ammo_9mm = AmmoInventory( GetAmmoIndex( "9mm" ) );
	ammo_357 = AmmoInventory( GetAmmoIndex( "357" ) );
	ammo_argrens = AmmoInventory( GetAmmoIndex( "ARgrenades" ) );
	ammo_bolts = AmmoInventory( GetAmmoIndex( "bolts" ) );
	ammo_buckshot = AmmoInventory( GetAmmoIndex( "buckshot" ) );
	ammo_rockets = AmmoInventory( GetAmmoIndex( "rockets" ) );
	ammo_uranium = AmmoInventory( GetAmmoIndex( "uranium" ) );
	ammo_hornets = AmmoInventory( GetAmmoIndex( "Hornets" ) );
}


/*
===========
WaterMove
============
*/
#define AIRTIME	12		// lung full of air lasts this many seconds

void CBasePlayer::WaterMove()
{
	int air;

	if (pev->movetype == MOVETYPE_NOCLIP)
		return;

	if (pev->health < 0)
		return;

	// waterlevel 0 - not in water
	// waterlevel 1 - feet in water
	// waterlevel 2 - waist in water
	// waterlevel 3 - head in water

	if (pev->waterlevel != 3) 
	{
		// not underwater
		
		// play 'up for air' sound
		if (pev->air_finished < gpGlobals->time)
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_wade1.wav", 1, ATTN_NORM);
		else if (pev->air_finished < gpGlobals->time + 9)
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_wade2.wav", 1, ATTN_NORM);

		pev->air_finished = gpGlobals->time + AIRTIME;
		pev->dmg = 2;

		// if we took drowning damage, give it back slowly
		if (m_idrowndmg > m_idrownrestored)
		{
			// set drowning damage bit.  hack - dmg_drownrecover actually
			// makes the time based damage code 'give back' health over time.
			// make sure counter is cleared so we start count correctly.
			
			// NOTE: this actually causes the count to continue restarting
			// until all drowning damage is healed.

			m_bitsDamageType |= DMG_DROWNRECOVER;
			m_bitsDamageType &= ~DMG_DROWN;
			m_rgbTimeBasedDamage[itbd_DrownRecover] = 0;
		}

	}
	else
	{	// fully under water
		// stop restoring damage while underwater
		m_bitsDamageType &= ~DMG_DROWNRECOVER;
		m_rgbTimeBasedDamage[itbd_DrownRecover] = 0;

		if (pev->air_finished < gpGlobals->time)		// drown!
		{
			if (pev->pain_finished < gpGlobals->time)
			{
				// take drowning damage
				pev->dmg += 1;
				if (pev->dmg > 5)
					pev->dmg = 5;
				TakeDamage(VARS(eoNullEntity), VARS(eoNullEntity), pev->dmg, DMG_DROWN);
				pev->pain_finished = gpGlobals->time + 1;
				
				// track drowning damage, give it back when
				// player finally takes a breath

				m_idrowndmg += pev->dmg;
			} 
		}
		else
		{
			m_bitsDamageType &= ~DMG_DROWN;
		}
	}

	if (!pev->waterlevel)
	{
		if (FBitSet(pev->flags, FL_INWATER))
		{       
			ClearBits(pev->flags, FL_INWATER);
		}
		return;
	}
	
	// make bubbles

	air = (int)(pev->air_finished - gpGlobals->time);
	if (!RANDOM_LONG(0,0x1f) && RANDOM_LONG(0,AIRTIME-1) >= air)
	{
		switch (RANDOM_LONG(0,3))
			{
			case 0:	EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_swim1.wav", 0.8, ATTN_NORM); break;
			case 1:	EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_swim2.wav", 0.8, ATTN_NORM); break;
			case 2:	EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_swim3.wav", 0.8, ATTN_NORM); break;
			case 3:	EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_swim4.wav", 0.8, ATTN_NORM); break;
		}
	}

	if (pev->watertype == CONTENT_LAVA)		// do damage
	{
		if (pev->dmgtime < gpGlobals->time)
			TakeDamage(VARS(eoNullEntity), VARS(eoNullEntity), 10 * pev->waterlevel, DMG_BURN);
	}
	else if (pev->watertype == CONTENT_SLIME)		// do damage
	{
		pev->dmgtime = gpGlobals->time + 1;
		TakeDamage(VARS(eoNullEntity), VARS(eoNullEntity), 4 * pev->waterlevel, DMG_ACID);
	}
	
	if (!FBitSet(pev->flags, FL_INWATER))
	{
		SetBits(pev->flags, FL_INWATER);
		pev->dmgtime = 0;
	}
}


// TRUE if the player is attached to a ladder
BOOL CBasePlayer::IsOnLadder( void )
{ 
	return ( pev->movetype == MOVETYPE_FLY );
}

void CBasePlayer::PlayerDeathThink(void)
{
	float flForward;

	if (FBitSet(pev->flags, FL_ONGROUND))
	{
		flForward = pev->velocity.Length() - 20;
		if (flForward <= 0)
			pev->velocity = g_vecZero;
		else    
			pev->velocity = flForward * pev->velocity.Normalize();
	}

	if ( HasWeapons() )
	{
		// we drop the guns here because weapons that have an area effect and can kill their user
		// will sometimes crash coming back from CBasePlayer::Killed() if they kill their owner because the
		// player class sometimes is freed. It's safer to manipulate the weapons once we know
		// we aren't calling into any of their code anymore through the player pointer.
		PackDeadPlayerItems();
	}
	
	// activate thirdperson
	CVAR_SET_FLOAT( "cam_command", 1.0f );
	
	// block mouse input
	pev->fixangle = 1;
	
	// spin the camera
	deathCameraYaw += 0.25;
	CVAR_SET_FLOAT( "cam_idealyaw", deathCameraYaw );
	if ( deathCameraYaw >= 360 ) {
		deathCameraYaw = 0;
	}

	if (pev->modelindex && (!m_fSequenceFinished) && (pev->deadflag == DEAD_DYING) )
	{
		StudioFrameAdvance( );
		return;
	}

	// once we're done animating our death and we're on the ground, we want to set movetype to None so our dead body won't do collisions and stuff anymore
	// this prevents a bug where the dead body would go to a player's head if he walked over it while the dead player was clicking their button to respawn
	if ( pev->movetype != MOVETYPE_NONE && FBitSet(pev->flags, FL_ONGROUND) )
		pev->movetype = MOVETYPE_NONE;

	if (pev->deadflag == DEAD_DYING)
		pev->deadflag = DEAD_DEAD;

	BOOL fAnyButtonDown = (pev->button & ~IN_SCORE );
	
	// wait for all buttons released
	if (pev->deadflag == DEAD_DEAD)
	{
		if (fAnyButtonDown)
			return;

		if ( g_pGameRules->FPlayerCanRespawn( this ) )
		{
			m_fDeadTime = gpGlobals->time;
			pev->deadflag = DEAD_RESPAWNABLE;
		}
		
		return;
	}

// if the player has been dead for one second longer than allowed by forcerespawn, 
// forcerespawn isn't on. Send the player off to an intermission camera until they 
// choose to respawn.
	
	if ( g_pGameRules->IsMultiplayer() && ( gpGlobals->time > (m_fDeadTime + 6) ) && !(m_afPhysicsFlags & PFLAG_OBSERVER) )
	{
		// go to dead camera. 
		StartDeathCam();
	}

	if ( pev->iuser1 )	// player is in spectator mode
		return;	
	
// wait for any button down,  or mp_forcerespawn is set and the respawn time is up
	if (!fAnyButtonDown 
		&& !( g_pGameRules->IsMultiplayer() && forcerespawn.value > 0 && (gpGlobals->time > (m_fDeadTime + 5))) )
		return;

	pev->button = 0;
	m_iRespawnFrames = 0;

	//ALERT(at_console, "Respawn\n");

	respawn(pev, !(m_afPhysicsFlags & PFLAG_OBSERVER) );// don't copy a corpse if we're in deathcam.
	pev->nextthink = -1;
}

//=========================================================
// StartDeathCam - find an intermission spot and send the
// player off into observer mode
//=========================================================
void CBasePlayer::StartDeathCam( void )
{
	edict_t *pSpot, *pNewSpot;
	int iRand;

	if ( pev->view_ofs == g_vecZero )
	{
		// don't accept subsequent attempts to StartDeathCam()
		return;
	}

	pSpot = FIND_ENTITY_BY_CLASSNAME( NULL, "info_intermission");	

	if ( !FNullEnt( pSpot ) )
	{
		// at least one intermission spot in the world.
		iRand = RANDOM_LONG( 0, 3 );

		while ( iRand > 0 )
		{
			pNewSpot = FIND_ENTITY_BY_CLASSNAME( pSpot, "info_intermission");
			
			if ( pNewSpot )
			{
				pSpot = pNewSpot;
			}

			iRand--;
		}

		CopyToBodyQue( pev );

		UTIL_SetOrigin( pev, pSpot->v.origin );
		pev->angles = pev->v_angle = pSpot->v.v_angle;
	}
	else
	{
		// no intermission spot. Push them up in the air, looking down at their corpse
		TraceResult tr;
		CopyToBodyQue( pev );
		UTIL_TraceLine( pev->origin, pev->origin + Vector( 0, 0, 128 ), ignore_monsters, edict(), &tr );

		UTIL_SetOrigin( pev, tr.vecEndPos );
		pev->angles = pev->v_angle = UTIL_VecToAngles( tr.vecEndPos - pev->origin  );
	}

	// start death cam

	m_afPhysicsFlags |= PFLAG_OBSERVER;
	pev->view_ofs = g_vecZero;
	pev->fixangle = TRUE;
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;
	pev->movetype = MOVETYPE_NONE;
	pev->modelindex = 0;
}

void CBasePlayer::StartObserver( Vector vecPosition, Vector vecViewAngle )
{
	// clear any clientside entities attached to this player
	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_KILLPLAYERATTACHMENTS );
		WRITE_BYTE( (BYTE)entindex() );
	MESSAGE_END();

	// Holster weapon immediately, to allow it to cleanup
	if (m_pActiveItem)
		m_pActiveItem->Holster( );

	if ( m_pTank != NULL )
	{
		m_pTank->Use( this, this, USE_OFF, 0 );
		m_pTank = NULL;
	}

	// clear out the suit message cache so we don't keep chattering
	SetSuitUpdate(NULL, FALSE, 0);

	// Tell Ammo Hud that the player is dead
	MESSAGE_BEGIN( MSG_ONE, gmsgCurWeapon, NULL, pev );
		WRITE_BYTE(0);
		WRITE_BYTE(0XFF);
		WRITE_BYTE(0xFF);
		WRITE_BYTE(0xFF);
	MESSAGE_END();

	// reset FOV
	m_iFOV = m_iClientFOV = 0;
	pev->fov = m_iFOV;
	MESSAGE_BEGIN( MSG_ONE, gmsgSetFOV, NULL, pev );
		WRITE_BYTE(0);
	MESSAGE_END();

	// Setup flags
	m_iHideHUD = (HIDEHUD_HEALTH | HIDEHUD_WEAPONS);
	m_afPhysicsFlags |= PFLAG_OBSERVER;
	pev->effects = EF_NODRAW;
	pev->view_ofs = g_vecZero;
	pev->angles = pev->v_angle = vecViewAngle;
	pev->fixangle = TRUE;
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;
	pev->movetype = MOVETYPE_NONE;
	ClearBits( m_afPhysicsFlags, PFLAG_DUCKING );
	ClearBits( pev->flags, FL_DUCKING );
	pev->deadflag = DEAD_RESPAWNABLE;
	pev->health = 1;

	// Clear out the status bar
	m_fInitHUD = TRUE;

	pev->team =  0;
	MESSAGE_BEGIN( MSG_ALL, gmsgTeamInfo );
		WRITE_BYTE( ENTINDEX(edict()) );
		WRITE_STRING( "" );
	MESSAGE_END();

	// Remove all the player's stuff
	RemoveAllItems( FALSE );

	// Move them to the new position
	UTIL_SetOrigin( pev, vecPosition );

	// Find a player to watch
	m_flNextObserverInput = 0;
	Observer_SetMode( m_iObserverLastMode );
}

// 
// PlayerUse - handles USE keypress
//
#define	PLAYER_SEARCH_RADIUS	(float)64

void CBasePlayer::PlayerUse ( void )
{
	if ( IsObserver() )
		return;

	// Was use pressed or released?
	if ( ! ((pev->button | m_afButtonPressed | m_afButtonReleased) & IN_USE) )
		return;

	// Hit Use on a train?
	if ( m_afButtonPressed & IN_USE )
	{
		if ( m_pTank != NULL )
		{
			// Stop controlling the tank
			// TODO: Send HUD Update
			m_pTank->Use( this, this, USE_OFF, 0 );
			m_pTank = NULL;
			return;
		}
		else
		{
			if ( m_afPhysicsFlags & PFLAG_ONTRAIN )
			{
				m_afPhysicsFlags &= ~PFLAG_ONTRAIN;
				m_iTrain = TRAIN_NEW|TRAIN_OFF;
				return;
			}
			else
			{	// Start controlling the train!
				CBaseEntity *pTrain = CBaseEntity::Instance( pev->groundentity );

				if ( pTrain && !(pev->button & IN_JUMP) && FBitSet(pev->flags, FL_ONGROUND) && (pTrain->ObjectCaps() & FCAP_DIRECTIONAL_USE) && pTrain->OnControls(pev) )
				{
					m_afPhysicsFlags |= PFLAG_ONTRAIN;
					m_iTrain = TrainSpeed(pTrain->pev->speed, pTrain->pev->impulse);
					m_iTrain |= TRAIN_NEW;
					EMIT_SOUND( ENT(pev), CHAN_ITEM, "plats/train_use1.wav", 0.8, ATTN_NORM, true);
					return;
				}
			}
		}
	}

	CBaseEntity *pObject = NULL;
	CBaseEntity *pClosest = NULL;
	Vector		vecLOS;
	float flMaxDot = VIEW_FIELD_NARROW;
	float flDot;

	UTIL_MakeVectors ( pev->v_angle );// so we know which way we are facing
	
	while ((pObject = UTIL_FindEntityInSphere( pObject, pev->origin, PLAYER_SEARCH_RADIUS )) != NULL)
	{

		if (pObject->ObjectCaps() & (FCAP_IMPULSE_USE | FCAP_CONTINUOUS_USE | FCAP_ONOFF_USE))
		{
			// !!!PERFORMANCE- should this check be done on a per case basis AFTER we've determined that
			// this object is actually usable? This dot is being done for every object within PLAYER_SEARCH_RADIUS
			// when player hits the use key. How many objects can be in that area, anyway? (sjb)
			vecLOS = (VecBModelOrigin( pObject->pev ) - (pev->origin + pev->view_ofs));
			
			// This essentially moves the origin of the target to the corner nearest the player to test to see 
			// if it's "hull" is in the view cone
			vecLOS = UTIL_ClampVectorToBox( vecLOS, pObject->pev->size * 0.5 );
			
			flDot = DotProduct (vecLOS , gpGlobals->v_forward);
			if (flDot > flMaxDot )
			{// only if the item is in front of the user
				pClosest = pObject;
				flMaxDot = flDot;
//				ALERT( at_console, "%s : %f\n", STRING( pObject->pev->classname ), flDot );
			}
//			ALERT( at_console, "%s : %f\n", STRING( pObject->pev->classname ), flDot );
		}
	}
	pObject = pClosest;

	// Found an object
	if (pObject )
	{
		//!!!UNDONE: traceline here to prevent USEing buttons through walls			
		int caps = pObject->ObjectCaps();

		if ( m_afButtonPressed & IN_USE )
			EMIT_SOUND( ENT(pev), CHAN_ITEM, "common/wpn_select.wav", 0.4, ATTN_NORM, true);

		if ( ( (pev->button & IN_USE) && (caps & FCAP_CONTINUOUS_USE) ) ||
			 ( (m_afButtonPressed & IN_USE) && (caps & (FCAP_IMPULSE_USE|FCAP_ONOFF_USE)) ) )
		{
			if ( caps & FCAP_CONTINUOUS_USE )
				m_afPhysicsFlags |= PFLAG_USING;

			pObject->Use( this, this, USE_SET, 1 );
		}
		// UNDONE: Send different USE codes for ON/OFF.  Cache last ONOFF_USE object to send 'off' if you turn away
		else if ( (m_afButtonReleased & IN_USE) && (pObject->ObjectCaps() & FCAP_ONOFF_USE) )	// BUGBUG This is an "off" use
		{
			pObject->Use( this, this, USE_SET, 0 );
		}
	}
	else
	{
		if ( m_afButtonPressed & IN_USE )
			EMIT_SOUND( ENT(pev), CHAN_ITEM, "common/wpn_denyselect.wav", 0.4, ATTN_NORM);
	}
}



void CBasePlayer::Jump()
{
	Vector		vecWallCheckDir;// direction we're tracing a line to find a wall when walljumping
	Vector		vecAdjustedVelocity;
	Vector		vecSpot;
	TraceResult	tr;
	
	if (FBitSet(pev->flags, FL_WATERJUMP))
		return;
	
	if (pev->waterlevel >= 2)
	{
		return;
	}

	// jump velocity is sqrt( height * gravity * 2)

	// If this isn't the first frame pressing the jump button, break out.
	if ( !FBitSet( m_afButtonPressed, IN_JUMP ) )
		return;         // don't pogo stick

	if ( !(pev->flags & FL_ONGROUND) || !pev->groundentity )
	{
		return;
	}

	if ( gameplayMods::vvvvvv.isActive() ) {
		const char *texture = g_engfuncs.pfnTraceTexture( NULL, pev->origin, gpGlobals->v_up * 8192 );
		if ( std::string( texture ) != "sky" ) {
			gameplayModsData.reverseGravity = !gameplayModsData.reverseGravity;
			pev->gravity *= -1.0f;

			return;
		}
	}

// many features in this function use v_forward, so makevectors now.
	UTIL_MakeVectors (pev->angles);

	// ClearBits(pev->flags, FL_ONGROUND);		// don't stairwalk
	
	SetAnimation( PLAYER_JUMP );

	if ( m_fLongJump &&
		(pev->button & IN_DUCK) &&
		( pev->flDuckTime > 0 ) &&
		pev->velocity.Length() > 50 )
	{
		SetAnimation( PLAYER_SUPERJUMP );
	}

	// If you're standing on a conveyor, add it's velocity to yours (for momentum)
	entvars_t *pevGround = VARS(pev->groundentity);
	if ( pevGround && (pevGround->flags & FL_CONVEYOR) )
	{
		pev->velocity = pev->velocity + pev->basevelocity;
	}

	if ( gameplayMods::superHot.isActive() ) {
		superHotJumping = gpGlobals->time + 0.15f;
	}
	jumpedOnce = TRUE;
}



// This is a glorious hack to find free space when you've crouched into some solid space
// Our crouching collisions do not work correctly for some reason and this is easier
// than fixing the problem :(
void FixPlayerCrouchStuck( edict_t *pPlayer )
{
	TraceResult trace;

	// Move up as many as 18 pixels if the player is stuck.
	for ( int i = 0; i < 18; i++ )
	{
		UTIL_TraceHull( pPlayer->v.origin, pPlayer->v.origin, dont_ignore_monsters, head_hull, pPlayer, &trace );
		if ( trace.fStartSolid )
			pPlayer->v.origin.z ++;
		else
			break;
	}
}

void CBasePlayer::Duck( )
{
	if (pev->button & IN_DUCK) 
	{
		if ( m_IdealActivity != ACT_LEAP )
		{
			SetAnimation( PLAYER_WALK );
		}
	}
}

//
// ID's player as such.
//
int  CBasePlayer::Classify ( void )
{
	return CLASS_PLAYER;
}


void CBasePlayer::AddPoints( int score, BOOL bAllowNegativeScore )
{
	// Positive score always adds
	if ( score < 0 )
	{
		if ( !bAllowNegativeScore )
		{
			if ( pev->frags < 0 )		// Can't go more negative
				return;
			
			if ( -score > pev->frags )	// Will this go negative?
			{
				score = -pev->frags;		// Sum will be 0
			}
		}
	}

	pev->frags += score;

	MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
		WRITE_BYTE( ENTINDEX(edict()) );
		WRITE_SHORT( pev->frags );
		WRITE_SHORT( m_iDeaths );
		WRITE_SHORT( 0 );
		WRITE_SHORT( g_pGameRules->GetTeamIndex( m_szTeamName ) + 1 );
	MESSAGE_END();
}


void CBasePlayer::AddPointsToTeam( int score, BOOL bAllowNegativeScore )
{
	int index = entindex();

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );

		if ( pPlayer && i != index )
		{
			if ( g_pGameRules->PlayerRelationship( this, pPlayer ) == GR_TEAMMATE )
			{
				pPlayer->AddPoints( score, bAllowNegativeScore );
			}
		}
	}
}

//Player ID
void CBasePlayer::InitStatusBar()
{
	m_flStatusBarDisappearDelay = 0;
	m_SbarString1[0] = m_SbarString0[0] = 0; 
}

void CBasePlayer::UpdateStatusBar()
{
	int newSBarState[ SBAR_END ];
	char sbuf0[ SBAR_STRING_SIZE ];
	char sbuf1[ SBAR_STRING_SIZE ];

	memset( newSBarState, 0, sizeof(newSBarState) );
	strcpy( sbuf0, m_SbarString0 );
	strcpy( sbuf1, m_SbarString1 );

	// Find an ID Target
	TraceResult tr;
	UTIL_MakeVectors( pev->v_angle + pev->punchangle );
	Vector vecSrc = EyePosition();
	Vector vecEnd = vecSrc + (gpGlobals->v_forward * MAX_ID_RANGE);
	UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, edict(), &tr);

	if (tr.flFraction != 1.0)
	{
		if ( !FNullEnt( tr.pHit ) )
		{
			CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );

			if (pEntity->Classify() == CLASS_PLAYER )
			{
				newSBarState[ SBAR_ID_TARGETNAME ] = ENTINDEX( pEntity->edict() );
				strcpy( sbuf1, "1 %p1\n2 Health: %i2%%\n3 Armor: %i3%%" );

				// allies and medics get to see the targets health
				if ( g_pGameRules->PlayerRelationship( this, pEntity ) == GR_TEAMMATE )
				{
					newSBarState[ SBAR_ID_TARGETHEALTH ] = 100 * (pEntity->pev->health / pEntity->pev->max_health);
					newSBarState[ SBAR_ID_TARGETARMOR ] = pEntity->pev->armorvalue; //No need to get it % based since 100 it's the max.
				}

				m_flStatusBarDisappearDelay = gpGlobals->time + 1.0;
			}
		}
		else if ( m_flStatusBarDisappearDelay > gpGlobals->time )
		{
			// hold the values for a short amount of time after viewing the object
			newSBarState[ SBAR_ID_TARGETNAME ] = m_izSBarState[ SBAR_ID_TARGETNAME ];
			newSBarState[ SBAR_ID_TARGETHEALTH ] = m_izSBarState[ SBAR_ID_TARGETHEALTH ];
			newSBarState[ SBAR_ID_TARGETARMOR ] = m_izSBarState[ SBAR_ID_TARGETARMOR ];
		}
	}

	BOOL bForceResend = FALSE;

	if ( strcmp( sbuf0, m_SbarString0 ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgStatusText, NULL, pev );
			WRITE_BYTE( 0 );
			WRITE_STRING( sbuf0 );
		MESSAGE_END();

		strcpy( m_SbarString0, sbuf0 );

		// make sure everything's resent
		bForceResend = TRUE;
	}

	if ( strcmp( sbuf1, m_SbarString1 ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgStatusText, NULL, pev );
			WRITE_BYTE( 1 );
			WRITE_STRING( sbuf1 );
		MESSAGE_END();

		strcpy( m_SbarString1, sbuf1 );

		// make sure everything's resent
		bForceResend = TRUE;
	}

	// Check values and send if they don't match
	for (int i = 1; i < SBAR_END; i++)
	{
		if ( newSBarState[i] != m_izSBarState[i] || bForceResend )
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgStatusValue, NULL, pev );
				WRITE_BYTE( i );
				WRITE_SHORT( newSBarState[i] );
			MESSAGE_END();

			m_izSBarState[i] = newSBarState[i];
		}
	}
}

void CBasePlayer::PlayMusicDelayed( const std::string &filePath, float delay, float musicPos, BOOL looping, BOOL noSlowmotionEffects ) {
	if ( delay <= 0.0f ) {
		SendPlayMusicMessage( filePath, musicPos, looping, noSlowmotionEffects );
	} else {
		delayedMusicFilePath = ALLOC_STRING( filePath.c_str() ); // memory leak
		delayedMusicStartTime = gpGlobals->time + delay;
		delayedMusicStartPos = musicPos;
		delayedMusicLooping = looping;
		delayedMusicNoSlowmotionEffects = noSlowmotionEffects;
	}
}

void CBasePlayer::SendPlayMusicMessage( const std::string &filePath, float musicPos, BOOL looping, BOOL noSlowmotionEffects )
{
	if ( !gmsgBassPlay ) {
		return;
	}

	musicNoSlowmotionEffects = noSlowmotionEffects;

	MESSAGE_BEGIN( MSG_ONE, gmsgBassPlay, NULL, this->pev );
		WRITE_STRING( filePath.c_str() );
		WRITE_FLOAT( musicPos );
		WRITE_BYTE( gameplayMods::IsSlowmotionEnabled() && !musicNoSlowmotionEffects );
		WRITE_BYTE( looping );
	MESSAGE_END();
}

void CBasePlayer::SendStopMusicMessage( BOOL smooth ) {
	if ( !gmsgBassStop ) {
		return;
	}

	MESSAGE_BEGIN( MSG_ONE, gmsgBassStop, NULL, this->pev );
		WRITE_BYTE( smooth );
	MESSAGE_END();
}







#define CLIMB_SHAKE_FREQUENCY	22	// how many frames in between screen shakes when climbing
#define	MAX_CLIMB_SPEED			200	// fastest vertical climbing speed possible
#define	CLIMB_SPEED_DEC			15	// climbing deceleration rate
#define	CLIMB_PUNCH_X			-7  // how far to 'punch' client X axis when climbing
#define CLIMB_PUNCH_Z			7	// how far to 'punch' client Z axis when climbing

void CBasePlayer::PreThink(void)
{
	int buttonsChanged = (m_afButtonLast ^ pev->button);	// These buttons have changed this frame
	
	// Debounced button codes for pressed/released
	// UNDONE: Do we need auto-repeat?
	m_afButtonPressed =  buttonsChanged & pev->button;		// The changed ones still down are "pressed"
	m_afButtonReleased = buttonsChanged & (~pev->button);	// The ones not down are "released"

	HandleSlowmotionFlags();

	if ( g_fGameOver )
		return;         // intermission or finale

	UTIL_MakeVectors(pev->v_angle);             // is this still used?
	
	ItemPreFrame( );
	WaterMove();

	if ( g_pGameRules && g_pGameRules->FAllowFlashlight() )
		m_iHideHUD &= ~HIDEHUD_FLASHLIGHT;
	else
		m_iHideHUD |= HIDEHUD_FLASHLIGHT;


	// JOHN: checks if new client data (for HUD and view control) needs to be sent to the client
	UpdateClientData();
	
	CheckTimeBasedDamage();

	CheckSuitUpdate();

	// Moved this lower than UpdateClientData, so HUD gets reset before calling PlayerThink 
	g_pGameRules->PlayerThink( this );

	// Observer Button Handling
	if ( IsObserver() )
	{
		Observer_HandleButtons();
		Observer_CheckTarget();
		Observer_CheckProperties();
		pev->impulse = 0;
		return;
	}

	if (pev->deadflag >= DEAD_DYING)
	{
		PlayerDeathThink();
		return;
	}

	gameplayMods::OnFlagChange<BOOL>( gameplayModsData.lastGodConstant, gameplayMods::godConstant.isActive(), [this]( BOOL on ) {
		if ( on ) {
			pev->flags |= FL_GODMODE;
		} else {
			pev->flags &= ~FL_GODMODE;
		}
	} );

	gameplayMods::OnFlagChange<BOOL>( gameplayModsData.lastNoTargetConstant, gameplayMods::noTargetConstant.isActive(), [this]( BOOL on ) {
		if ( on ) {
			pev->flags |= FL_NOTARGET;
		} else {
			pev->flags &= ~FL_NOTARGET;
		}
	} );

	// So the correct flags get sent to client asap.
	//
	if ( m_afPhysicsFlags & PFLAG_ONTRAIN )
		pev->flags |= FL_ONTRAIN;
	else 
		pev->flags &= ~FL_ONTRAIN;

	// Train speed control
	if ( m_afPhysicsFlags & PFLAG_ONTRAIN )
	{
		CBaseEntity *pTrain = CBaseEntity::Instance( pev->groundentity );
		float vel;
		
		if ( !pTrain )
		{
			TraceResult trainTrace;
			// Maybe this is on the other side of a level transition
			UTIL_TraceLine( pev->origin, pev->origin + Vector(0,0,-38), ignore_monsters, ENT(pev), &trainTrace );

			// HACKHACK - Just look for the func_tracktrain classname
			if ( trainTrace.flFraction != 1.0 && trainTrace.pHit )
			pTrain = CBaseEntity::Instance( trainTrace.pHit );


			if ( !pTrain || !(pTrain->ObjectCaps() & FCAP_DIRECTIONAL_USE) || !pTrain->OnControls(pev) )
			{
				//ALERT( at_error, "In train mode with no train!\n" );
				m_afPhysicsFlags &= ~PFLAG_ONTRAIN;
				m_iTrain = TRAIN_NEW|TRAIN_OFF;
				return;
			}
		}
		else if ( !FBitSet( pev->flags, FL_ONGROUND ) || FBitSet( pTrain->pev->spawnflags, SF_TRACKTRAIN_NOCONTROL ) || (pev->button & (IN_MOVELEFT|IN_MOVERIGHT) ) )
		{
			// Turn off the train if you jump, strafe, or the train controls go dead
			m_afPhysicsFlags &= ~PFLAG_ONTRAIN;
			m_iTrain = TRAIN_NEW|TRAIN_OFF;
			return;
		}

		pev->velocity = g_vecZero;
		vel = 0;
		if ( m_afButtonPressed & IN_FORWARD )
		{
			vel = 1;
			pTrain->Use( this, this, USE_SET, (float)vel );
		}
		else if ( m_afButtonPressed & IN_BACK )
		{
			vel = -1;
			pTrain->Use( this, this, USE_SET, (float)vel );
		}

		if (vel)
		{
			m_iTrain = TrainSpeed(pTrain->pev->speed, pTrain->pev->impulse);
			m_iTrain |= TRAIN_ACTIVE|TRAIN_NEW;
		}

	} else if (m_iTrain & TRAIN_ACTIVE)
		m_iTrain = TRAIN_NEW; // turn off train

	if (pev->button & IN_JUMP)
	{
		// If on a ladder, jump off the ladder
		// else Jump
		Jump();
	}


	// If trying to duck, already ducked, or in the process of ducking
	if ((pev->button & IN_DUCK) || FBitSet(pev->flags,FL_DUCKING) || (m_afPhysicsFlags & PFLAG_DUCKING) )
		Duck();

	if ( !FBitSet ( pev->flags, FL_ONGROUND ) )
	{
		if ( !gameplayModsData.reverseGravity ) {
			m_flFallVelocity = -pev->velocity.z;
		} else {
			m_flFallVelocity = pev->velocity.z;
		}
	}

	// StudioFrameAdvance( );//!!!HACKHACK!!! Can't be hit by traceline when not animating?

	// Clear out ladder pointer
	m_hEnemy = NULL;

	if ( m_afPhysicsFlags & PFLAG_ONBARNACLE )
	{
		pev->velocity = g_vecZero;
	}

	g_slowMotionCharge = slowMotionCharge;
	g_playerHasSuit = pev->weapons & ( 1 << WEAPON_SUIT );
	g_divingAllowedWithoutSlowmotion = gameplayMods::divingAllowedWithoutSlowmotion.isActive();

	CBaseEntity *pEntity = NULL;
	while ( ( pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, 48.0f ) ) != NULL ) {

		const char *entityName = STRING( pEntity->pev->classname );

		CBaseMonster *monster = dynamic_cast<CBaseMonster *>( pEntity );
		if ( monster ) {

			// Let the player immediatly pass through the dying monster (while dying animation is playing).
			// The hacky way to do this is copied from CBaseMonster::RunTask
			// Previously this was executed immediatly on any monster's death, but that would make physical bullets go through monster.
			if ( monster->pev->deadflag == DEAD_DYING && monster->pev->iuser4 == 0 ) {
				if ( !monster->BBoxFlat( ) ) {
					UTIL_SetSize( monster->pev, Vector( -4, -4, 0 ), Vector( 4, 4, 1 ) );
				} else {
					UTIL_SetSize( monster->pev, Vector( monster->pev->mins.x, monster->pev->mins.y, monster->pev->mins.z + 1 ), Vector( monster->pev->maxs.x, monster->pev->maxs.y, monster->pev->mins.z + 2 ) );
					monster->pev->iuser4 = 1;
				}
			}
		}
	}

	if ( postRestoreDelay && gpGlobals->time >= postRestoreDelay ) {
		if ( CHalfLifeRules *singlePlayerRules = dynamic_cast< CHalfLifeRules * >( g_pGameRules ) ) {
			singlePlayerRules->HookModelIndex( NULL );
		}

		if ( !musicGoingThroughChangeLevel ) {
			if ( strlen( STRING( musicFile ) ) > 0 ) {
				this->SendPlayMusicMessage( STRING( musicFile ), musicPos, musicLooping, musicNoSlowmotionEffects );
			} else {
				SendStopMusicMessage();
			}
		}
		
		MESSAGE_BEGIN( MSG_ALL, gmsgSubtClear );
		MESSAGE_END();

		MESSAGE_BEGIN( MSG_ALL, gmsgBassStopC );
		MESSAGE_END();

		musicGoingThroughChangeLevel = FALSE;
		postRestoreDelay = 0.0f;
	}

	if ( postSpawnDelay && gpGlobals->time >= postSpawnDelay ) {
		SendStopMusicMessage();

		if ( !HasWeapons() ) {
			MESSAGE_BEGIN( MSG_ONE, gmsgCurWeapon, NULL, pev );
				WRITE_BYTE(0);
				WRITE_BYTE(0);
				WRITE_BYTE(0);
				WRITE_BYTE(0);
			MESSAGE_END();
		}

		MESSAGE_BEGIN( MSG_ALL, gmsgSubtClear );
		MESSAGE_END();

		postSpawnDelay = 0.0f;
	}

	if ( CVAR_GET_FLOAT( "print_aim_entity" ) >= 1.0f ) {
		TraceResult tr;
		UTIL_TraceLine( pev->origin + pev->view_ofs, pev->origin + pev->view_ofs + gpGlobals->v_forward * 2048, dont_ignore_monsters, edict(), &tr );

		edict_t *entity = tr.pHit;
		if ( aimLastEntity != entity ) {
			MESSAGE_BEGIN( MSG_ONE, gmsgOnAimNew, NULL, pev );
				WRITE_STRING( STRING( entity->v.classname ) );
				WRITE_STRING( STRING( entity->v.target ) );
				WRITE_STRING( STRING( entity->v.targetname ) );
				WRITE_SHORT( entity->v.modelindex );

				WRITE_COORD( entity->v.mins.x );
				WRITE_COORD( entity->v.mins.y );
				WRITE_COORD( entity->v.mins.z );

				WRITE_COORD( entity->v.maxs.x );
				WRITE_COORD( entity->v.maxs.y );
				WRITE_COORD( entity->v.maxs.z );

				WRITE_BYTE( entity->v.spawnflags );
			MESSAGE_END();
						
			aimLastEntity = entity;
		}

		if ( entity ) {
			MESSAGE_BEGIN( MSG_ONE, gmsgOnAimUpd, NULL, pev );
				WRITE_COORD( entity->v.origin.x );
				WRITE_COORD( entity->v.origin.y );
				WRITE_COORD( entity->v.origin.z );

				WRITE_COORD( entity->v.angles.x );
				WRITE_COORD( entity->v.angles.y );
				WRITE_COORD( entity->v.angles.z );

				WRITE_COORD( entity->v.velocity.x );
				WRITE_COORD( entity->v.velocity.y );
				WRITE_COORD( entity->v.velocity.z );

				WRITE_FLOAT( entity->v.health );
				WRITE_FLOAT( entity->v.max_health );

				WRITE_BYTE( entity->v.flags );
				WRITE_BYTE( entity->v.deadflag );
			MESSAGE_END();
		}
	} else {
		if ( aimLastEntity ) {
			aimLastEntity = 0;
		}
	}
}
/* Time based Damage works as follows: 
	1) There are several types of timebased damage:

		#define DMG_PARALYZE		(1 << 14)	// slows affected creature down
		#define DMG_NERVEGAS		(1 << 15)	// nerve toxins, very bad
		#define DMG_POISON			(1 << 16)	// blood poisioning
		#define DMG_RADIATION		(1 << 17)	// radiation exposure
		#define DMG_DROWNRECOVER	(1 << 18)	// drown recovery
		#define DMG_ACID			(1 << 19)	// toxic chemicals or acid burns
		#define DMG_SLOWBURN		(1 << 20)	// in an oven
		#define DMG_SLOWFREEZE		(1 << 21)	// in a subzero freezer

	2) A new hit inflicting tbd restarts the tbd counter - each monster has an 8bit counter,
		per damage type. The counter is decremented every second, so the maximum time 
		an effect will last is 255/60 = 4.25 minutes.  Of course, staying within the radius
		of a damaging effect like fire, nervegas, radiation will continually reset the counter to max.

	3) Every second that a tbd counter is running, the player takes damage.  The damage
		is determined by the type of tdb.  
			Paralyze		- 1/2 movement rate, 30 second duration.
			Nervegas		- 5 points per second, 16 second duration = 80 points max dose.
			Poison			- 2 points per second, 25 second duration = 50 points max dose.
			Radiation		- 1 point per second, 50 second duration = 50 points max dose.
			Drown			- 5 points per second, 2 second duration.
			Acid/Chemical	- 5 points per second, 10 second duration = 50 points max.
			Burn			- 10 points per second, 2 second duration.
			Freeze			- 3 points per second, 10 second duration = 30 points max.

	4) Certain actions or countermeasures counteract the damaging effects of tbds:

		Armor/Heater/Cooler - Chemical(acid),burn, freeze all do damage to armor power, then to body
							- recharged by suit recharger
		Air In Lungs		- drowning damage is done to air in lungs first, then to body
							- recharged by poking head out of water
							- 10 seconds if swiming fast
		Air In SCUBA		- drowning damage is done to air in tanks first, then to body
							- 2 minutes in tanks. Need new tank once empty.
		Radiation Syringe	- Each syringe full provides protection vs one radiation dosage
		Antitoxin Syringe	- Each syringe full provides protection vs one poisoning (nervegas or poison).
		Health kit			- Immediate stop to acid/chemical, fire or freeze damage.
		Radiation Shower	- Immediate stop to radiation damage, acid/chemical or fire damage.
		
	
*/

// If player is taking time based damage, continue doing damage to player -
// this simulates the effect of being poisoned, gassed, dosed with radiation etc -
// anything that continues to do damage even after the initial contact stops.
// Update all time based damage counters, and shut off any that are done.

// The m_bitsDamageType bit MUST be set if any damage is to be taken.
// This routine will detect the initial on value of the m_bitsDamageType
// and init the appropriate counter.  Only processes damage every second.

//#define PARALYZE_DURATION	30		// number of 2 second intervals to take damage
//#define PARALYZE_DAMAGE		0.0		// damage to take each 2 second interval

//#define NERVEGAS_DURATION	16
//#define NERVEGAS_DAMAGE		5.0

//#define POISON_DURATION		25
//#define POISON_DAMAGE		2.0

//#define RADIATION_DURATION	50
//#define RADIATION_DAMAGE	1.0

//#define ACID_DURATION		10
//#define ACID_DAMAGE			5.0

//#define SLOWBURN_DURATION	2
//#define SLOWBURN_DAMAGE		1.0

//#define SLOWFREEZE_DURATION	1.0
//#define SLOWFREEZE_DAMAGE	3.0

/* */

void CBasePlayer::HandleSlowmotionFlags()
{
	if ( ( pev->flags & FL_ACTIVATE_SLOWMOTION_REQUESTED ) && 
		 ( pev->flags & FL_DIVING ) ) {

		if ( slowMotionCharge > 0 ) {
			SetSlowMotion( true );
			slowMotionCharge -= min( slowMotionCharge, DIVING_SLOWMOTION_CHARGE_COST );
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, "slowmo/shootdodge.wav", 1, ATTN_NORM, true );
		}		

		pev->flags &= ~FL_ACTIVATE_SLOWMOTION_REQUESTED;
	} else if ( pev->flags & FL_DEACTIVATE_SLOWMOTION_REQUESTED ) {
		if ( desperation != DESPERATION_IMMINENT && desperation != DESPERATION_FIGHTING || gameplayMods::slowmotionOnlyDiving.isActive() ) {
			DeactivateSlowMotion( true );
		}
		pev->flags &= ~FL_DEACTIVATE_SLOWMOTION_REQUESTED;
	}

}

void CBasePlayer::CheckTimeBasedDamage() 
{
	int i;
	BYTE bDuration = 0;

	static float gtbdPrev = 0.0;

	if (!(m_bitsDamageType & DMG_TIMEBASED))
		return;

	// only check for time based damage approx. every 2 seconds
	if (abs(gpGlobals->time - m_tbdPrev) < 2.0)
		return;
	
	m_tbdPrev = gpGlobals->time;

	for (i = 0; i < CDMG_TIMEBASED; i++)
	{
		// make sure bit is set for damage type
		if (m_bitsDamageType & (DMG_PARALYZE << i))
		{
			switch (i)
			{
			case itbd_Paralyze:
				// UNDONE - flag movement as half-speed
				bDuration = PARALYZE_DURATION;
				break;
			case itbd_NerveGas:
//				TakeDamage(pev, pev, NERVEGAS_DAMAGE, DMG_GENERIC);	
				bDuration = NERVEGAS_DURATION;
				break;
			case itbd_Poison:
				TakeDamage(pev, pev, POISON_DAMAGE, DMG_GENERIC);
				bDuration = POISON_DURATION;
				break;
			case itbd_Radiation:
//				TakeDamage(pev, pev, RADIATION_DAMAGE, DMG_GENERIC);
				bDuration = RADIATION_DURATION;
				break;
			case itbd_DrownRecover:
				// NOTE: this hack is actually used to RESTORE health
				// after the player has been drowning and finally takes a breath
				if (m_idrowndmg > m_idrownrestored)
				{
					int idif = min(m_idrowndmg - m_idrownrestored, 10);

					TakeHealth(idif, DMG_GENERIC);
					m_idrownrestored += idif;
				}
				bDuration = 4;	// get up to 5*10 = 50 points back
				break;
			case itbd_Acid:
//				TakeDamage(pev, pev, ACID_DAMAGE, DMG_GENERIC);
				bDuration = ACID_DURATION;
				break;
			case itbd_SlowBurn:
//				TakeDamage(pev, pev, SLOWBURN_DAMAGE, DMG_GENERIC);
				bDuration = SLOWBURN_DURATION;
				break;
			case itbd_SlowFreeze:
//				TakeDamage(pev, pev, SLOWFREEZE_DAMAGE, DMG_GENERIC);
				bDuration = SLOWFREEZE_DURATION;
				break;
			default:
				bDuration = 0;
			}

			if (m_rgbTimeBasedDamage[i])
			{
				// use up an antitoxin on poison or nervegas after a few seconds of damage					
				if (((i == itbd_NerveGas) && (m_rgbTimeBasedDamage[i] < NERVEGAS_DURATION)) ||
					((i == itbd_Poison)   && (m_rgbTimeBasedDamage[i] < POISON_DURATION)))
				{
					if (m_rgItems[ITEM_ANTIDOTE])
					{
						m_rgbTimeBasedDamage[i] = 0;
						m_rgItems[ITEM_ANTIDOTE]--;
						SetSuitUpdate("!HEV_HEAL4", FALSE, SUIT_REPEAT_OK);
					}
				}


				// decrement damage duration, detect when done.
				if (!m_rgbTimeBasedDamage[i] || --m_rgbTimeBasedDamage[i] == 0)
				{
					m_rgbTimeBasedDamage[i] = 0;
					// if we're done, clear damage bits
					m_bitsDamageType &= ~(DMG_PARALYZE << i);	
				}
			}
			else
				// first time taking this damage type - init damage duration
				m_rgbTimeBasedDamage[i] = bDuration;
		}
	}
}

/*
THE POWER SUIT

The Suit provides 3 main functions: Protection, Notification and Augmentation. 
Some functions are automatic, some require power. 
The player gets the suit shortly after getting off the train in C1A0 and it stays
with him for the entire game.

Protection

	Heat/Cold
		When the player enters a hot/cold area, the heating/cooling indicator on the suit 
		will come on and the battery will drain while the player stays in the area. 
		After the battery is dead, the player starts to take damage. 
		This feature is built into the suit and is automatically engaged.
	Radiation Syringe
		This will cause the player to be immune from the effects of radiation for N seconds. Single use item.
	Anti-Toxin Syringe
		This will cure the player from being poisoned. Single use item.
	Health
		Small (1st aid kits, food, etc.)
		Large (boxes on walls)
	Armor
		The armor works using energy to create a protective field that deflects a
		percentage of damage projectile and explosive attacks. After the armor has been deployed,
		it will attempt to recharge itself to full capacity with the energy reserves from the battery.
		It takes the armor N seconds to fully charge. 

Notification (via the HUD)

x	Health
x	Ammo  
x	Automatic Health Care
		Notifies the player when automatic healing has been engaged. 
x	Geiger counter
		Classic Geiger counter sound and status bar at top of HUD 
		alerts player to dangerous levels of radiation. This is not visible when radiation levels are normal.
x	Poison
	Armor
		Displays the current level of armor. 

Augmentation 

	Reanimation (w/adrenaline)
		Causes the player to come back to life after he has been dead for 3 seconds. 
		Will not work if player was gibbed. Single use.
	Long Jump
		Used by hitting the ??? key(s). Caused the player to further than normal.
	SCUBA	
		Used automatically after picked up and after player enters the water. 
		Works for N seconds. Single use.	
	
Things powered by the battery

	Armor		
		Uses N watts for every M units of damage.
	Heat/Cool	
		Uses N watts for every second in hot/cold area.
	Long Jump	
		Uses N watts for every jump.
	Alien Cloak	
		Uses N watts for each use. Each use lasts M seconds.
	Alien Shield	
		Augments armor. Reduces Armor drain by one half
 
*/

// if in range of radiation source, ping geiger counter

#define GEIGERDELAY 0.25

void CBasePlayer :: UpdateGeigerCounter( void )
{
	BYTE range;

	// delay per update ie: don't flood net with these msgs
	if (gpGlobals->time < m_flgeigerDelay)
		return;

	m_flgeigerDelay = gpGlobals->time + GEIGERDELAY;
		
	// send range to radition source to client

	range = (BYTE) (m_flgeigerRange / 4);

	if (range != m_igeigerRangePrev)
	{
		m_igeigerRangePrev = range;

		MESSAGE_BEGIN( MSG_ONE, gmsgGeigerRange, NULL, pev );
			WRITE_BYTE( range );
		MESSAGE_END();
	}

	// reset counter and semaphore
	if (!RANDOM_LONG(0,3))
		m_flgeigerRange = 1000;

}

/*
================
CheckSuitUpdate

Play suit update if it's time
================
*/

#define SUITUPDATETIME	3.5
#define SUITFIRSTUPDATETIME 0.1

void CBasePlayer::CheckSuitUpdate()
{
	int i;
	int isentence = 0;
	int isearch = m_iSuitPlayNext;
	
	// Ignore suit updates if no suit
	if ( !(pev->weapons & (1<<WEAPON_SUIT)) )
		return;

	// if in range of radiation source, ping geiger counter
	UpdateGeigerCounter();

	if ( g_pGameRules->IsMultiplayer() )
	{
		// don't bother updating HEV voice in multiplayer.
		return;
	}

	if ( gpGlobals->time >= m_flSuitUpdate && m_flSuitUpdate > 0)
	{
		// play a sentence off of the end of the queue
		for (i = 0; i < CSUITPLAYLIST; i++)
			{
			if (isentence = m_rgSuitPlayList[isearch])
				break;
			
			if (++isearch == CSUITPLAYLIST)
				isearch = 0;
			}

		if (isentence)
		{
			m_rgSuitPlayList[isearch] = 0;
			if (isentence > 0)
			{
				// play sentence number

				char sentence[CBSENTENCENAME_MAX+1];
				strcpy(sentence, "!");
				strcat(sentence, gszallsentencenames[isentence]);
				EMIT_SOUND_SUIT(ENT(pev), sentence);
			}
			else
			{
				// play sentence group
				EMIT_GROUPID_SUIT(ENT(pev), -isentence);
			}
		m_flSuitUpdate = gpGlobals->time + SUITUPDATETIME;
		}
		else
			// queue is empty, don't check 
			m_flSuitUpdate = 0;
	}
}
 
// add sentence to suit playlist queue. if fgroup is true, then
// name is a sentence group (HEV_AA), otherwise name is a specific
// sentence name ie: !HEV_AA0.  If iNoRepeat is specified in
// seconds, then we won't repeat playback of this word or sentence
// for at least that number of seconds.

void CBasePlayer::SetSuitUpdate(char *name, int fgroup, int iNoRepeatTime)
{
	return; // POWER ARMOR IS FOR PUSSIES

	int i;
	int isentence;
	int iempty = -1;
	
	
	// Ignore suit updates if no suit
	if ( !(pev->weapons & (1<<WEAPON_SUIT)) )
		return;

	if ( g_pGameRules->IsMultiplayer() )
	{
		// due to static channel design, etc. We don't play HEV sounds in multiplayer right now.
		return;
	}

	// if name == NULL, then clear out the queue

	if (!name)
	{
		for (i = 0; i < CSUITPLAYLIST; i++)
			m_rgSuitPlayList[i] = 0;
		return;
	}
	// get sentence or group number
	if (!fgroup)
	{
		isentence = SENTENCEG_Lookup(name, NULL);
		if (isentence < 0)
			return;
	}
	else
		// mark group number as negative
		isentence = -SENTENCEG_GetIndex(name);

	// check norepeat list - this list lets us cancel
	// the playback of words or sentences that have already
	// been played within a certain time.

	for (i = 0; i < CSUITNOREPEAT; i++)
	{
		if (isentence == m_rgiSuitNoRepeat[i])
			{
			// this sentence or group is already in 
			// the norepeat list

			if (m_rgflSuitNoRepeatTime[i] < gpGlobals->time)
				{
				// norepeat time has expired, clear it out
				m_rgiSuitNoRepeat[i] = 0;
				m_rgflSuitNoRepeatTime[i] = 0.0;
				iempty = i;
				break;
				}
			else
				{
				// don't play, still marked as norepeat
				return;
				}
			}
		// keep track of empty slot
		if (!m_rgiSuitNoRepeat[i])
			iempty = i;
	}

	// sentence is not in norepeat list, save if norepeat time was given

	if (iNoRepeatTime)
	{
		if (iempty < 0)
			iempty = RANDOM_LONG(0, CSUITNOREPEAT-1); // pick random slot to take over
		m_rgiSuitNoRepeat[iempty] = isentence;
		m_rgflSuitNoRepeatTime[iempty] = iNoRepeatTime + gpGlobals->time;
	}

	// find empty spot in queue, or overwrite last spot
	
	m_rgSuitPlayList[m_iSuitPlayNext++] = isentence;
	if (m_iSuitPlayNext == CSUITPLAYLIST)
		m_iSuitPlayNext = 0;

	if (m_flSuitUpdate <= gpGlobals->time)
	{
		if (m_flSuitUpdate == 0)
			// play queue is empty, don't delay too long before playback
			m_flSuitUpdate = gpGlobals->time + SUITFIRSTUPDATETIME;
		else 
			m_flSuitUpdate = gpGlobals->time + SUITUPDATETIME; 
	}

}

/*
================
CheckPowerups

Check for turning off powerups

GLOBALS ASSUMED SET:  g_ulModelIndexPlayer
================
*/
	static void
CheckPowerups(entvars_t *pev)
{
	if (pev->health <= 0)
		return;

	pev->modelindex = g_ulModelIndexPlayer;    // don't use eyes
}


//=========================================================
// UpdatePlayerSound - updates the position of the player's
// reserved sound slot in the sound list.
//=========================================================
void CBasePlayer :: UpdatePlayerSound ( void )
{
	int iBodyVolume;
	int iVolume;
	CSound *pSound;

	pSound = CSoundEnt::SoundPointerForIndex( CSoundEnt :: ClientSoundIndex( edict() ) );

	if ( !pSound )
	{
		ALERT ( at_console, "Client lost reserved sound!\n" );
		return;
	}

	pSound->m_iType = bits_SOUND_NONE;

	// now calculate the best target volume for the sound. If the player's weapon
	// is louder than his body/movement, use the weapon volume, else, use the body volume.
	
	if ( FBitSet ( pev->flags, FL_ONGROUND ) )
	{	
		iBodyVolume = pev->velocity.Length(); 

		// clamp the noise that can be made by the body, in case a push trigger,
		// weapon recoil, or anything shoves the player abnormally fast. 
		if ( iBodyVolume > 512 )
		{
			iBodyVolume = 512;
		}
	}
	else
	{
		iBodyVolume = 0;
	}

	if ( pev->button & IN_JUMP )
	{
		iBodyVolume += 100;
	}

// convert player move speed and actions into sound audible by monsters.
	if ( m_iWeaponVolume > iBodyVolume )
	{
		m_iTargetVolume = m_iWeaponVolume;

		// OR in the bits for COMBAT sound if the weapon is being louder than the player. 
		pSound->m_iType |= bits_SOUND_COMBAT;
	}
	else
	{
		m_iTargetVolume = iBodyVolume;
	}

	// decay weapon volume over time so bits_SOUND_COMBAT stays set for a while
	m_iWeaponVolume -= 250 * gpGlobals->frametime;
	if ( m_iWeaponVolume < 0 )
	{
		iVolume = 0;
	}


	// if target volume is greater than the player sound's current volume, we paste the new volume in 
	// immediately. If target is less than the current volume, current volume is not set immediately to the
	// lower volume, rather works itself towards target volume over time. This gives monsters a much better chance
	// to hear a sound, especially if they don't listen every frame.
	iVolume = pSound->m_iVolume;

	if ( m_iTargetVolume > iVolume )
	{
		iVolume = m_iTargetVolume;
	}
	else if ( iVolume > m_iTargetVolume )
	{
		iVolume -= 250 * gpGlobals->frametime;

		if ( iVolume < m_iTargetVolume )
		{
			iVolume = 0;
		}
	}

	if ( m_fNoPlayerSound )
	{
		// debugging flag, lets players move around and shoot without monsters hearing.
		iVolume = 0;
	}

	if ( gpGlobals->time > m_flStopExtraSoundTime )
	{
		// since the extra sound that a weapon emits only lasts for one client frame, we keep that sound around for a server frame or two 
		// after actual emission to make sure it gets heard.
		m_iExtraSoundTypes = 0;
	}

	if ( pSound )
	{
		pSound->m_vecOrigin = pev->origin;
		pSound->m_iType |= ( bits_SOUND_PLAYER | m_iExtraSoundTypes );
		pSound->m_iVolume = iVolume;
	}

	// keep track of virtual muzzle flash
	m_iWeaponFlash -= 256 * gpGlobals->frametime;
	if (m_iWeaponFlash < 0)
		m_iWeaponFlash = 0;

	//UTIL_MakeVectors ( pev->angles );
	//gpGlobals->v_forward.z = 0;

	// Below are a couple of useful little bits that make it easier to determine just how much noise the 
	// player is making. 
	// UTIL_ParticleEffect ( pev->origin + gpGlobals->v_forward * iVolume, g_vecZero, 255, 25 );
	//ALERT ( at_console, "%d/%d\n", iVolume, m_iTargetVolume );
}

void CBasePlayer::TryToPlayMaxCommentary( string_t string, BOOL isImportant, BOOL shortDelay )
{
	bool isCommentaryPresent = latestMaxCommentaryTime && ( gpGlobals->time - latestMaxCommentaryTime < ( shortDelay ? 1.5f : 10.0f ) );
	if ( !isImportant && isCommentaryPresent ) {
		return;
	}

	MESSAGE_BEGIN( MSG_ONE, gmsgBassComm, NULL, this->pev );
		WRITE_STRING( STRING( string ) );
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_ALL, gmsgOnSound );
		WRITE_STRING( STRING( string ) );
		WRITE_BYTE( 0 );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
	MESSAGE_END();

	latestMaxCommentary = STRING( string );
	latestMaxCommentaryIsImportant = isImportant;
	latestMaxCommentaryTime = gpGlobals->time;
}

void CBasePlayer::CheckSoundQueue()
{
	for ( int i = 0 ; i < MAX_SOUND_QUEUE ; i++ ) {
		if ( soundQueueSoundNames[i] != 0 && gpGlobals->time >= soundQueueSoundDelays[i] ) {
			
			BOOL isMaxCommentary = soundQueueIsMaxPayneCommentarySound[i];

			if ( !isMaxCommentary ) {
				EMIT_SOUND( edict(), CHAN_AUTO, STRING( soundQueueSoundNames[i] ), 1.0, ATTN_STATIC, soundQueueIsMaxPayneCommentarySound[i] );
			} else if ( isMaxCommentary ) {
				TryToPlayMaxCommentary( soundQueueSoundNames[i], soundQueueIsMaxPayneCommentaryImportant[i] );
			}
			soundQueueSoundNames[i] = 0;
			soundQueueSoundDelays[i] = 0;
			soundQueueIsMaxPayneCommentarySound[i] = false;
			soundQueueIsMaxPayneCommentaryImportant[i] = false;
		}
	}
}

void CBasePlayer::AddToSoundQueue( string_t string, float delay, bool isMaxCommentary, bool isImportant )
{
	if ( isMaxCommentary && isImportant && CVAR_GET_FLOAT( "max_commentary" ) < 1.0f ) {
		return;
	}

	if ( soundQueueCounter == MAX_SOUND_QUEUE ) {
		soundQueueCounter = 0;
	}

	soundQueueSoundNames[soundQueueCounter] = string;
	soundQueueSoundDelays[soundQueueCounter] = gpGlobals->time + delay;
	soundQueueIsMaxPayneCommentarySound[soundQueueCounter] = isMaxCommentary;
	soundQueueIsMaxPayneCommentaryImportant[soundQueueCounter] = isImportant;

	soundQueueCounter++;
}

void CBasePlayer::ClearSoundQueue()
{
	for ( int i = 0 ; i < MAX_SOUND_QUEUE; i++ ) {
		soundQueueSoundNames[i] = 0;
		soundQueueSoundDelays[i] = 0.0f;
	}
	soundQueueCounter = 0;
}

void CBasePlayer::PostThink()
{
	if ( g_fGameOver )
		goto pt_end;         // intermission or finale

	if (!IsAlive())
		goto pt_end;

	// Handle Tank controlling
	if ( m_pTank != NULL )
	{ // if they've moved too far from the gun,  or selected a weapon, unuse the gun
		if ( m_pTank->OnControls( pev ) && !pev->weaponmodel )
		{  
			m_pTank->Use( this, this, USE_SET, 2 );	// try fire the gun
		}
		else
		{  // they've moved off the platform
			m_pTank->Use( this, this, USE_OFF, 0 );
			m_pTank = NULL;
		}
	}

// do weapon stuff
	ItemPostFrame( );

	CheckSoundQueue();

	if ( delayedMusicStartTime && gpGlobals->time >= delayedMusicStartTime ) {
		SendPlayMusicMessage( STRING( delayedMusicFilePath ), delayedMusicStartPos, delayedMusicLooping, delayedMusicNoSlowmotionEffects );
		delayedMusicStartTime = 0.0f;
	}

// check to see if player landed hard enough to make a sound
// falling farther than half of the maximum safe distance, but not as far a max safe distance will
// play a bootscrape sound, and no damage will be inflicted. Fallling a distance shorter than half
// of maximum safe distance will make no sound. Falling farther than max safe distance will play a 
// fallpain sound, and damage will be inflicted based on how far the player fell

	if ( !gameplayMods::noFallDamage.isActive() && (FBitSet(pev->flags, FL_ONGROUND)) && (pev->health > 0) && m_flFallVelocity >= PLAYER_FALL_PUNCH_THRESHHOLD )
	{
		// ALERT ( at_console, "%f\n", m_flFallVelocity );

		if (pev->watertype == CONTENT_WATER)
		{
			// Did he hit the world or a non-moving entity?
			// BUG - this happens all the time in water, especially when 
			// BUG - water has current force
			// if ( !pev->groundentity || VARS(pev->groundentity)->velocity.z == 0 )
				// EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_wade1.wav", 1, ATTN_NORM);
		}
		else if ( m_flFallVelocity > PLAYER_MAX_SAFE_FALL_SPEED )
		{// after this point, we start doing damage
			
			float flFallDamage = g_pGameRules->FlPlayerFallDamage( this );

			if ( flFallDamage > pev->health )
			{//splat
				// note: play on item channel because we play footstep landing on body channel
				EMIT_SOUND(ENT(pev), CHAN_ITEM, "common/bodysplat.wav", 1, ATTN_NORM, true);
			}

			if ( flFallDamage > 0 )
			{
				TakeDamage(VARS(eoNullEntity), VARS(eoNullEntity), flFallDamage, DMG_FALL ); 
				pev->punchangle.x = 0;
			}
		}

		if ( IsAlive() )
		{
			SetAnimation( PLAYER_WALK );
		}
    }

	if (FBitSet(pev->flags, FL_ONGROUND))
	{		
		if (m_flFallVelocity > 64 && !g_pGameRules->IsMultiplayer())
		{
			CSoundEnt::InsertSound ( bits_SOUND_PLAYER, pev->origin, m_flFallVelocity, 0.2 );
			// ALERT( at_console, "fall %f\n", m_flFallVelocity );
		}
		m_flFallVelocity = 0;
	}

	// select the proper animation for the player character	
	if ( IsAlive() )
	{
		if (!pev->velocity.x && !pev->velocity.y)
			SetAnimation( PLAYER_IDLE );
		else if ((pev->velocity.x || pev->velocity.y) && (FBitSet(pev->flags, FL_ONGROUND)))
			SetAnimation( PLAYER_WALK );
		else if (pev->waterlevel > 1)
			SetAnimation( PLAYER_WALK );
	}

	StudioFrameAdvance( );
	CheckPowerups(pev);

	UpdatePlayerSound();

pt_end:
#if defined( CLIENT_WEAPONS )
		// Decay timers on weapons
	// go through all of the weapons and make a list of the ones to pack
	for ( int i = 0 ; i < MAX_ITEM_TYPES ; i++ )
	{
		if ( m_rgpPlayerItems[ i ] )
		{
			CBasePlayerItem *pPlayerItem = m_rgpPlayerItems[ i ];

			while ( pPlayerItem )
			{
				CBasePlayerWeapon *gun;

				gun = (CBasePlayerWeapon *)pPlayerItem->GetWeaponPtr();
				
				if ( gun && gun->UseDecrement() )
				{
					gun->m_flNextPrimaryAttack		= max( gun->m_flNextPrimaryAttack - gpGlobals->frametime, -1.0 );
					gun->m_flNextSecondaryAttack	= max( gun->m_flNextSecondaryAttack - gpGlobals->frametime, -0.001 );

					if ( gun->m_flTimeWeaponIdle != 1000 )
					{
						gun->m_flTimeWeaponIdle		= max( gun->m_flTimeWeaponIdle - gpGlobals->frametime, -0.001 );
					}

					if ( gun->pev->fuser1 != 1000 )
					{
						gun->pev->fuser1	= max( gun->pev->fuser1 - gpGlobals->frametime, -0.001 );
					}

					// Only decrement if not flagged as NO_DECREMENT
//					if ( gun->m_flPumpTime != 1000 )
				//	{
				//		gun->m_flPumpTime	= max( gun->m_flPumpTime - gpGlobals->frametime, -0.001 );
				//	}
					
				}

				pPlayerItem = pPlayerItem->m_pNext;
			}
		}
	}

	m_flNextAttack -= gpGlobals->frametime;
	if ( m_flNextAttack < -0.001 )
		m_flNextAttack = -0.001;
	
	if ( m_flNextAmmoBurn != 1000 )
	{
		m_flNextAmmoBurn -= gpGlobals->frametime;
		
		if ( m_flNextAmmoBurn < -0.001 )
			m_flNextAmmoBurn = -0.001;
	}

	if ( m_flAmmoStartCharge != 1000 )
	{
		m_flAmmoStartCharge -= gpGlobals->frametime;
		
		if ( m_flAmmoStartCharge < -0.001 )
			m_flAmmoStartCharge = -0.001;
	}
#endif

	// Track button info so we can detect 'pressed' and 'released' buttons next frame
	m_afButtonLast = pev->button;

	if ( auto frictionOverride = gameplayMods::frictionOverride.isActive<float>() ) {
		g_frictionOverride = *frictionOverride;
	} else {
		g_frictionOverride = -1.0f;
	}
	g_noJumping = gameplayMods::noJumping.isActive();
	g_upsideDown = gameplayMods::upsideDown.isActive();
	g_inverseControls = gameplayMods::inverseControls.isActive();
	g_noWalking = gameplayMods::noWalking.isActive();
	g_doubleSpeed = gameplayMods::IsSlowmotionEnabled() && gameplayMods::slowmotionFastWalk.isActive();

	pev->fuser4 = gameplayMods::divingOnly.isActive() ? 1.0 : 0.0;

	if ( gameplayModsData.reverseGravity ) {
		pev->vuser3[1] = 1.0f;
		pev->angles[2] = 180;
	} else {
		pev->vuser3[1] = 0.0f;
		pev->angles[2] = 0;
	}

	if ( desperation != DESPERATION_NO ) {
		ThinkAboutFinalDesperation();
	}

	if ( auto drunkAim = gameplayMods::drunkAim.isActive<DrunkAimInfo>() ) {
		float phi = gpGlobals->time * drunkAim->wobbleFrequency;
		aimOffsetX = sin( phi ) * drunkAim->maxHorizontalWobble;
		aimOffsetY = cos( phi ) * sin( phi ) * drunkAim->maxVerticalWobble;
	} else {
		aimOffsetX = 0.0f;
		aimOffsetY = 0.0f;
	}
	MESSAGE_BEGIN( MSG_ONE, gmsgAimOffset, NULL, pev );
		WRITE_FLOAT( aimOffsetX );
		WRITE_FLOAT( aimOffsetY );
	MESSAGE_END();

	SET_CROSSHAIRANGLE( edict(), aimOffsetY, -aimOffsetX );

	if ( showCredits ) {
		if ( CHalfLifeRules *rules = dynamic_cast< CHalfLifeRules * >( g_pGameRules ) ) {
			rules->End( this );
		}
	}
}


// checks if the spot is clear of players
BOOL IsSpawnPointValid( CBaseEntity *pPlayer, CBaseEntity *pSpot )
{
	CBaseEntity *ent = NULL;

	if ( !pSpot->IsTriggered( pPlayer ) )
	{
		return FALSE;
	}

	while ( (ent = UTIL_FindEntityInSphere( ent, pSpot->pev->origin, 128 )) != NULL )
	{
		// if ent is a client, don't spawn on 'em
		if ( ent->IsPlayer() && ent != pPlayer )
			return FALSE;
	}

	return TRUE;
}


DLL_GLOBAL CBaseEntity	*g_pLastSpawn;
inline int FNullEnt( CBaseEntity *ent ) { return (ent == NULL) || FNullEnt( ent->edict() ); }

/*
============
EntSelectSpawnPoint

Returns the entity to spawn at

USES AND SETS GLOBAL g_pLastSpawn
============
*/
edict_t *EntSelectSpawnPoint( CBaseEntity *pPlayer )
{
	CBaseEntity *pSpot;
	edict_t		*player;

	player = pPlayer->edict();

// choose a info_player_deathmatch point
	if (g_pGameRules->IsCoOp())
	{
		pSpot = UTIL_FindEntityByClassname( g_pLastSpawn, "info_player_coop");
		if ( !FNullEnt(pSpot) )
			goto ReturnSpot;
		pSpot = UTIL_FindEntityByClassname( g_pLastSpawn, "info_player_start");
		if ( !FNullEnt(pSpot) ) 
			goto ReturnSpot;
	}
	else if ( g_pGameRules->IsDeathmatch() )
	{
		pSpot = g_pLastSpawn;
		// Randomize the start spot
		for ( int i = RANDOM_LONG(1,5); i > 0; i-- )
			pSpot = UTIL_FindEntityByClassname( pSpot, "info_player_deathmatch" );
		if ( FNullEnt( pSpot ) )  // skip over the null point
			pSpot = UTIL_FindEntityByClassname( pSpot, "info_player_deathmatch" );

		CBaseEntity *pFirstSpot = pSpot;

		do 
		{
			if ( pSpot )
			{
				// check if pSpot is valid
				if ( IsSpawnPointValid( pPlayer, pSpot ) )
				{
					if ( pSpot->pev->origin == Vector( 0, 0, 0 ) )
					{
						pSpot = UTIL_FindEntityByClassname( pSpot, "info_player_deathmatch" );
						continue;
					}

					// if so, go to pSpot
					goto ReturnSpot;
				}
			}
			// increment pSpot
			pSpot = UTIL_FindEntityByClassname( pSpot, "info_player_deathmatch" );
		} while ( pSpot != pFirstSpot ); // loop if we're not back to the start

		// we haven't found a place to spawn yet,  so kill any guy at the first spawn point and spawn there
		if ( !FNullEnt( pSpot ) )
		{
			CBaseEntity *ent = NULL;
			while ( (ent = UTIL_FindEntityInSphere( ent, pSpot->pev->origin, 128 )) != NULL )
			{
				// if ent is a client, kill em (unless they are ourselves)
				if ( ent->IsPlayer() && !(ent->edict() == player) )
					ent->TakeDamage( VARS(INDEXENT(0)), VARS(INDEXENT(0)), 300, DMG_GENERIC );
			}
			goto ReturnSpot;
		}
	}

	// If startspot is set, (re)spawn there.
	if ( FStringNull( gpGlobals->startspot ) || !strlen(STRING(gpGlobals->startspot)))
	{
		pSpot = UTIL_FindEntityByClassname(NULL, "info_player_start");
		if ( !FNullEnt(pSpot) )
			goto ReturnSpot;
	}
	else
	{
		pSpot = UTIL_FindEntityByTargetname( NULL, STRING(gpGlobals->startspot) );
		if ( !FNullEnt(pSpot) )
			goto ReturnSpot;
	}

ReturnSpot:
	if ( FNullEnt( pSpot ) )
	{
		ALERT(at_error, "PutClientInServer: no info_player_start on level");
		return INDEXENT(0);
	}

	g_pLastSpawn = pSpot;
	return pSpot->edict();
}

void CBasePlayer::Spawn( void )
{
	m_flStartCharge = gpGlobals->time;

	pev->classname		= MAKE_STRING("player");
	pev->health			= 100;
	pev->armorvalue		= 0;
	pev->takedamage		= DAMAGE_AIM;
	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_WALK;
	pev->max_health		= pev->health;
	pev->flags		   &= FL_PROXY;	// keep proxy flag sey by engine
	pev->flags		   |= FL_CLIENT;
	pev->air_finished	= gpGlobals->time + 12;
	pev->dmg			= 2;				// initial water damage
	pev->effects		= 0;
	pev->deadflag		= DEAD_NO;
	pev->dmg_take		= 0;
	pev->dmg_save		= 0;
	pev->friction		= 1.0;
	pev->gravity		= 1.0;
	m_bitsHUDDamage		= -1;
	m_bitsDamageType	= 0;
	m_afPhysicsFlags	= 0;
	m_fLongJump			= FALSE;// no longjump module. 

	g_engfuncs.pfnSetPhysicsKeyValue( edict(), "slj", "0" );
	g_engfuncs.pfnSetPhysicsKeyValue( edict(), "hl", "1" );

	pev->fov = m_iFOV				= 0;// init field of view.
	m_iClientFOV		= -1; // make sure fov reset is sent

	m_flNextDecalTime	= 0;// let this player decal as soon as he spawns.

	m_flgeigerDelay = gpGlobals->time + 2.0;	// wait a few seconds until user-defined message registrations
												// are recieved by all clients

	for ( int i = 0 ; i < MAX_HOOKED_MODEL_INDEXES ; i++ ) {
		hookedModelIndexes[i] = 0;
	}
	hookedModelIndexesCount = 0;
	hookedMapsWithKerotansCount = 0;

	ClearSoundQueue();

	for ( int i = 0 ; i < MAX_VISITED_MAPS ; i++ ) {
		visitedMaps[i] = 0;
	}
	visitedMapsCount = 0;
	
	m_flTimeStepSound	= 0;
	m_iStepLeft = 0;
	m_flFieldOfView		= 0.5;// some monsters use this to determine whether or not the player is looking at them.

	m_bloodColor	= BLOOD_COLOR_RED;
	m_flNextAttack	= UTIL_WeaponTimeBase();
	StartSneaking();

	m_iFlashBattery = 99;
	m_flFlashLightTime = 1; // force first message

	slowMotionCharge = 100;
	slowMotionUpdateTime = 1;
	nextSmoothTimeScaleChange = 0.0f;

	currentMusicPlaylistIndex = 0;

	painkillerCount = 0;
	painkillerEnergy = 0;
	nextPainkillerEffectTime = 0.0f;
	if ( auto period = gameplayMods::painkillersSlow.isActive<float>() ) {
		nextPainkillerEffectTimePeriod = *period;
	} else {
		nextPainkillerEffectTimePeriod = 0.0f;
	}

	gameTitleShown = false;

	loadoutReceived = false;

	allowedToReactOnPainkillerPickup = 0.0f;
	allowedToReactOnPainkillerTake = 0.0f;
	allowedToReactOnPainkillerNeed = 0.0f;

	allowedToReactOnMinorInjury = 0.0f;
	allowedToReactOnSeriousInjury = 0.0f;

	dumbShots = 0;
	readyToComplainAboutDumbShots = false;
	allowedToComplainAboutDumbShots = 0.0f;

	allowedToComplainAboutKillingInnocent = 0.0f;

	allowedToSayAboutRhetoricalQuestion = 0.0f;
	rhetoricalQuestionHolder = NULL;

	allowedToSwear = 0.0f;

	superHotMultiplier = 0.0005f;
	nextSuperHotMultiplierUpdate = 0.0f;
	superHotJumping = 0.0f;
	jumpedOnce = FALSE;

	desperation = DESPERATION_NO;

	postRestoreDelay = 0.0f;
	postSpawnDelay = 0.1f;

	aimOffsetX = 0.0f;
	aimOffsetY = 0.0f;

	deathCameraYaw = 0.0f;
	CVAR_SET_FLOAT( "cam_idealyaw", 0.0f );
	CVAR_SET_FLOAT( "cam_idealpitch", 0.0f );
	CVAR_SET_FLOAT( "cam_idealdist", 70.0f );
	CVAR_SET_FLOAT( "cam_command", 2.0f );

// dont let uninitialized value here hurt the player
	m_flFallVelocity = 0;

	g_pGameRules->SetDefaultPlayerTeam( this );
	g_pGameRules->GetPlayerSpawnSpot( this );

    SET_MODEL(ENT(pev), "models/player.mdl");
    g_ulModelIndexPlayer = pev->modelindex;
	pev->sequence		= LookupActivity( ACT_IDLE );

	if ( FBitSet(pev->flags, FL_DUCKING) ) 
		UTIL_SetSize(pev, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX);
	else
		UTIL_SetSize(pev, VEC_HULL_MIN, VEC_HULL_MAX);

    pev->view_ofs = VEC_VIEW;
	Precache();
	m_HackedGunPos		= Vector( 0, 32, 0 );

	if ( m_iPlayerSound == SOUNDLIST_EMPTY )
	{
		ALERT ( at_console, "Couldn't alloc player sound slot!\n" );
	}

	m_fNoPlayerSound = FALSE;// normal sound behavior.

	m_pLastItem = NULL;
	m_fInitHUD = TRUE;
	m_iClientHideHUD = -1;  // force this to be recalculated
	m_fWeapon = FALSE;
	m_pClientActiveItem = NULL;
	m_iClientBattery = -1;

	// reset all ammo values to 0
	for ( int i = 0; i < MAX_AMMO_SLOTS; i++ )
	{
		m_rgAmmo[i] = 0;
		m_rgAmmoLast[i] = 0;  // client ammo values also have to be reset  (the death hud clear messages does on the client side)
	}

	m_lastx = m_lasty = 0;
	
	m_flNextChatTime = gpGlobals->time;

	nextTime = SDL_GetTicks() + GET_TICK_INTERVAL();

	showCredits = FALSE;

	slowMotionNextHeartbeatSound = 0;
	auto timescale_multiplier = *gameplayMods::timescale.isActive<float>();
	desiredTimeScale = 1.0f * timescale_multiplier + gameplayModsData.timescaleAdditive;
	SetSlowMotion( false );

	lastDamageTime = 0.0f;
	healthChargeTime = 1.0f;

	lastHealingTime = 0.0f;
	bleedTime = 0.0f;

	fadeOutTime = 0.0f;

	crystalsDestroyed = 0;

	latestMaxCommentaryTime = 0.0f;
	latestMaxCommentaryIsImportant = false;

	musicFile = 0;
	musicPos = 0.0f;
	musicLooping = 0;
	musicNoSlowmotionEffects = 0;
	musicGoingThroughChangeLevel = FALSE;

	delayedMusicFilePath = NULL;
	delayedMusicStartTime = 0.0f;
	delayedMusicStartPos = 0.0f;
	delayedMusicLooping = false;
	delayedMusicNoSlowmotionEffects = false;

	g_pGameRules->PlayerSpawn( this );

	MESSAGE_BEGIN( MSG_ALL, gmsgBassStopC );
	MESSAGE_END();
}

void CBasePlayer::SayRandomSwear()
{
	if ( gpGlobals->time <= allowedToSwear ) {
		return;
	}
	
	char fileName[256];
	do {
		sprintf_s( fileName, "max/finale_swear/GENERAL_SWEAR_CMBT_ENEMY_%d.wav", RANDOM_LONG( 1, 21 ) );
	} while ( lastSwearingLine == fileName );
	lastSwearingLine = fileName;

	TryToPlayMaxCommentary( MAKE_STRING( fileName ), false, true );

	allowedToSwear = gpGlobals->time + 1.5f;
}

void CBasePlayer::AddVisitedMap( string_t mapName ) 
{
	if ( visitedMapsCount == MAX_VISITED_MAPS ) {
		visitedMapsCount = 0;
	}

	visitedMaps[visitedMapsCount] = mapName;
	visitedMapsCount++;
}

bool CBasePlayer::HasVisitedMap( string_t mapName ) {
	for ( int i = 0 ; i < MAX_VISITED_MAPS ; i++ ) {
		if ( FStrEq( STRING( mapName ), STRING( visitedMaps[i] ) ) ) {
			return true;
		}
	}

	return false;
}

void CBasePlayer::ThinkAboutFinalDesperation()
{
	if ( strcmp( STRING( gpGlobals->mapname ), "c5a1" ) != 0 ) {
		return;
	}

	if ( untilNextDesperation && gpGlobals->time > untilNextDesperation ) {

		untilNextDesperation = 0.0f;

		switch ( desperation ) {
			case DESPERATION_PRE_IMMINENT: {
				CBaseEntity *pEntity = NULL;
				while ( ( pEntity = UTIL_FindEntityInSphere( pEntity, this->pev->origin, 4610.0f ) ) != NULL ) {
					if (
						( strcmp( STRING( pEntity->pev->classname ), "monster_alien_grunt" ) == 0 || strcmp( STRING( pEntity->pev->classname ), "monster_alien_controller" ) == 0 ) &&
						pEntity->pev->deadflag == DEAD_NO
					) {
						pEntity->pev->effects |= EF_DIMLIGHT;
					}
				}
				untilNextDesperation = gpGlobals->time + 0.6f;
				desperation = DESPERATION_IMMINENT;
				break;
			}

			case DESPERATION_IMMINENT: {
				GiveNamedItem( "weapon_9mmhandgun", true );
				GiveNamedItem( "weapon_9mmhandgun_twin", true );
				GiveNamedItem( "weapon_ingram", true );
				GiveNamedItem( "weapon_ingram_twin", true );
				if ( gameplayMods::slowmotionOnlyDiving.isActive() ) {
					SetSlowMotion( false );
				}
				if ( CHalfLifeRules *singlePlayerRules = dynamic_cast< CHalfLifeRules * >( g_pGameRules ) ) {
					singlePlayerRules->HookModelIndex( this, -2, "", "final_battle_start" );
				}

				CBaseEntity *pEntity = NULL;
				while ( ( pEntity = UTIL_FindEntityInSphere( pEntity, this->pev->origin, 4610.0f ) ) != NULL ) {
					if (
						( strcmp( STRING( pEntity->pev->classname ), "monster_alien_grunt" ) == 0 || strcmp( STRING( pEntity->pev->classname ), "monster_alien_controller" ) == 0 ) &&
						pEntity->pev->deadflag == DEAD_NO
					) {
						pEntity->pev->spawnflags = 0;
					}
				}

				desperation = DESPERATION_FIGHTING;

				break;
			}

			case DESPERATION_RELIEF: {
				edict_t *edicts[] = {
					FIND_ENTITY_BY_TARGETNAME( NULL, "backport" ),
					FIND_ENTITY_BY_TARGETNAME( NULL, "zap_sound_1" ),
					FIND_ENTITY_BY_TARGETNAME( NULL, "zap_sound_2" ),
					FIND_ENTITY_BY_TARGETNAME( NULL, "start_flash" )
				};

				for ( int i = 0 ; i < 4 ; i++ ) {
					if ( CBaseEntity *backport = CBaseEntity::Instance( edicts[i] ) ) {
						backport->Use( this, this, USE_SET, 1 );
					}
				}

				CBaseEntity *teleport = CBaseEntity::Create( "trigger_teleport", edicts[0]->v.origin, Vector( 0, 0, 0 ) );
				UTIL_SetSize( VARS( teleport->edict() ), Vector( 161, -1815, 1350 ), Vector( 221, -1755, 1410  ) );
				teleport->Spawn();
				teleport->pev->target= MAKE_STRING( "backtrain" );

				break;
			}

			case DESPERATION_ALL_FOR_REVENGE: {
				GiveNamedItem( "weapon_9mmhandgun", true );
				gSkillData.plrDmg9MM = 9999.0f;
				desperation = DESPERATION_REVENGE;
				break;
			}

			case DESPERATION_REVENGE: {
				desperation = DESPERATION_ALMOST_OVER;
				AddToSoundQueue( MAKE_STRING( "comment/finalespeech.wav" ), 3, false, true );
				DeactivateSlowMotion( true );
				slowMotionCharge = 0;
				untilNextDesperation = gpGlobals->time + 8.0f;
				break;
			}

			case DESPERATION_ALMOST_OVER: {
				desperation = DESPERATION_OVER;
				RemoveAllItems( FALSE );
				untilNextDesperation = gpGlobals->time + 25.5f;
				break;
			}

			case DESPERATION_OVER: {
				showCredits = TRUE;
				break;
			}

			default:
				break;
		}

		
	}

	if ( ( desperation == DESPERATION_FIGHTING || desperation == DESPERATION_RELIEF ) && pev->origin.z < 1220 ) {
		TakeDamage( pev, pev, 9999.9f, DMG_GENERIC );
	}

	bool theyAreAlive = false;
	CBaseEntity *pEntity = NULL;
	while ( ( pEntity = UTIL_FindEntityInSphere( pEntity, this->pev->origin, 4610.0f ) ) != NULL ) {
		if (
			( strcmp( STRING( pEntity->pev->classname ), "monster_alien_grunt" ) == 0 || strcmp( STRING( pEntity->pev->classname ), "monster_alien_controller" ) == 0 ) &&
			pEntity->pev->deadflag == DEAD_NO
		) {
			theyAreAlive = true;
			
			if ( desperation == DESPERATION_FIGHTING ) {
				pEntity->pev->effects |= EF_DIMLIGHT;
			}
		}
	}

	

	if ( !theyAreAlive && desperation == DESPERATION_FIGHTING ) {
		desperation = DESPERATION_RELIEF;
		untilNextDesperation = gpGlobals->time + 14.1f;
		RemoveAllItems( FALSE );
		DeactivateSlowMotion( true );

		if ( CHalfLifeRules *singlePlayerRules = dynamic_cast< CHalfLifeRules * >( g_pGameRules ) ) {
			singlePlayerRules->HookModelIndex( this, -2, "", "final_battle_won" );
		}
		AddToSoundQueue( MAKE_STRING( "comment/finalewon.wav" ), 1.6, false, true );
	}
}

void CBasePlayer::RememberHookedModelIndex( string_t string )
{
	if ( hookedModelIndexesCount + 1 == MAX_HOOKED_MODEL_INDEXES ) {
		hookedModelIndexesCount = 0;
	}

	hookedModelIndexes[hookedModelIndexesCount] = string;
	hookedModelIndexesCount++;
}

void CBasePlayer::RememberKerotanOnCurrentMap() {
	if ( hookedMapsWithKerotansCount + 1 == 128 ) {
		hookedMapsWithKerotansCount = 0;
	}

	hookedMapsWithKerotans[hookedMapsWithKerotansCount] = gpGlobals->mapname;
	hookedMapsWithKerotansCount++;
}

static std::map<std::string, std::vector<std::string>> chapterMaps = {
	{ "BLACK MESA INBOUND", { "c0a0", "c0a0a", "c0a0b", "c0a0c", "c0a0d", "c0a0e" } },
	{ "ANOMALOUS MATERIALS", { "c1a0", "c1a0a", "c1a0b", "c1a0e" } },
	{ "UNFORESEEN CONSEQUENCES", { "c1a0c", "c1a1", "c1a1a", "c1a1b", "c1a1c", "c1a1d", "c1a1f" } },
	{ "OFFICE COMPLEX", { "c1a2", "c1a2a", "c1a2b", "c1a2c", "c1a2d" } },
	{ "WE'VE GOT HOSITLES", { "c1a3", "c1a3a", "c1a3b", "c1a3c", "c1a3d" } },
	{ "BLAST PIT", { "c1a4", "c1a4b", "c1a4d", "c1a4e", "c1a4f", "c1a4g", "c1a4i", "c1a4j", "c1a4k" } },
	{ "POWER UP", { "c2a1", "c2a1a", "c2a1b" } },
	{ "ON A RAIL", { "c2a2", "c2a2a", "c2a2b1", "c2a2b2", "c2a2c", "c2a2d", "c2a2e", "c2a2f", "c2a2g", "c2a2h" } },
	{ "APPREHENSION", { "c2a3", "c2a3a", "c2a3b", "c2a3c", "c2a3d", "c2a3e" } },
	{ "RESIDUE PROCESSING", { "c2a4", "c2a4a", "c2a4b", "c2a4c" } },
	{ "QUESTIONABLE ETHICS", { "c2a4d", "c2a4e", "c2a4f", "c2a4g" } },
	{ "SURFACE TENSION", { "c2a5", "c2a5a", "c2a5b", "c2a5c", "c2a5d", "c2a5e", "c2a5f", "c2a5g", "c2a5w", "c2a5x" } },
	{ "FORGET ABOUT FREEMAN", { "c3a1", "c3a1a", "c3a1b" } },
	{ "LAMBDA CORE", { "c3a2e", "c3a2", "c3a2a", "c3a2b", "c3a2c", "c3a2d", "c3a2f" } },
	{ "XEN", { "c4a1" } },
	{ "GONARCH'S LAIR", { "c4a2", "c4a2a", "c4a2b" } },
	{ "INTERLOPER", { "c4a1a", "c4a1b", "c4a1c", "c4a1d", "c4a1e", "c4a1f" } },
	{ "NIHILANTH", { "c4a3" } },
	{ "FINALE", { "c5a1" } },
};

int CBasePlayer::GetAmountOfKerotansInCurrentChapter() {
	
	int result = 0;

	for ( auto &pair : chapterMaps ) {
		if ( result > 0 ) {
			break;
		}

		for ( auto &map : pair.second ) {
			if ( FStrEq( map.c_str(), STRING( gpGlobals->mapname ) ) ) {
				for ( int i = 0; i < 128; i++ ) {
					if ( aux::ctr::includes( pair.second, STRING( hookedMapsWithKerotans[i] ) ) ) {
						result++;
					}
				}
				break;
			}
		}
	}

	return result;
}

std::pair<std::string, std::vector<std::string>> CBasePlayer::GetCurrentChapterMapNames() {
	for ( auto &pair : chapterMaps ) {

		for ( auto &map : pair.second ) {
			if ( FStrEq( map.c_str(), STRING( gpGlobals->mapname ) ) ) {
				return pair;
			}
		}
	}
	return { "", { } };
}

bool CBasePlayer::ModelIndexHasBeenHooked( const char *modelIndexKey )
{
	for ( int i = 0 ; i < hookedModelIndexesCount ; i++ ) {
		const char *hookedModelIndex = STRING( hookedModelIndexes[i] );
		if ( strcmp( hookedModelIndex, modelIndexKey ) == 0 ) {
			return true;
		}
	}

	return false;
}


void CBasePlayer :: Precache( void )
{
	// in the event that the player JUST spawned, and the level node graph
	// was loaded, fix all of the node graph pointers before the game starts.
	
	// !!!BUGBUG - now that we have multiplayer, this needs to be moved!
	if ( WorldGraph.m_fGraphPresent && !WorldGraph.m_fGraphPointersSet )
	{
		if ( !WorldGraph.FSetGraphPointers() )
		{
			ALERT ( at_console, "**Graph pointers were not set!\n");
		}
		else
		{
			ALERT ( at_console, "**Graph Pointers Set!\n" );
		} 
	}

	// SOUNDS / MODELS ARE PRECACHED in ClientPrecache() (game specific)
	// because they need to precache before any clients have connected

	// init geiger counter vars during spawn and each time
	// we cross a level transition

	m_flgeigerRange = 1000;
	m_igeigerRangePrev = 1000;

	m_bitsDamageType = 0;
	m_bitsHUDDamage = -1;

	m_iClientBattery = -1;

	m_iTrain = TRAIN_NEW;

	// Make sure any necessary user messages have been registered
	LinkUserMessages();

	m_iUpdateTime = 5;  // won't update for 1/2 a second

	if ( gInitHUD )
		m_fInitHUD = TRUE;
}


int CBasePlayer::Save( CSave &save )
{
	if ( !g_changeLevelOccured ) {
		musicFile = MAKE_STRING( CVAR_GET_STRING( "sm_current_file" ) );
		musicPos = CVAR_GET_FLOAT( "sm_current_pos" );
		musicLooping = CVAR_GET_FLOAT( "sm_looping" ) > 0.0f;
	} else {
		musicGoingThroughChangeLevel = TRUE;
	}

	if ( !CBaseMonster::Save(save) )
		return 0;

	return save.WriteFields( "PLAYER", this, m_playerSaveData, ARRAYSIZE(m_playerSaveData) );
}


//
// Marks everything as new so the player will resend this to the hud.
//
void CBasePlayer::RenewItems(void)
{

}


int CBasePlayer::Restore( CRestore &restore )
{
	if ( !CBaseMonster::Restore(restore) )
		return 0;

	int status = restore.ReadFields( "PLAYER", this, m_playerSaveData, ARRAYSIZE(m_playerSaveData) );

	SAVERESTOREDATA *pSaveData = (SAVERESTOREDATA *)gpGlobals->pSaveData;
	// landmark isn't present.
	if ( pSaveData && !pSaveData->fUseLandmark )
	{
		ALERT( at_console, "No Landmark:%s\n", pSaveData->szLandmarkName );

		// default to normal spawn
		edict_t* pentSpawnSpot = EntSelectSpawnPoint( this );
		pev->origin = VARS(pentSpawnSpot)->origin + Vector(0,0,1);
		pev->angles = VARS(pentSpawnSpot)->angles;
	}
	pev->v_angle.z = 0;	// Clear out roll
	pev->angles = pev->v_angle;

	pev->fixangle = TRUE;           // turn this way immediately

// Copied from spawn() for now
	m_bloodColor	= BLOOD_COLOR_RED;

    g_ulModelIndexPlayer = pev->modelindex;

	if ( FBitSet(pev->flags, FL_DUCKING) ) 
	{
		// Use the crouch HACK
		//FixPlayerCrouchStuck( edict() );
		// Don't need to do this with new player prediction code.
		UTIL_SetSize(pev, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX);
	}
	else
	{
		UTIL_SetSize(pev, VEC_HULL_MIN, VEC_HULL_MAX);
	}

	g_engfuncs.pfnSetPhysicsKeyValue( edict(), "hl", "1" );

	if ( m_fLongJump )
	{
		g_engfuncs.pfnSetPhysicsKeyValue( edict(), "slj", "1" );
	}
	else
	{
		g_engfuncs.pfnSetPhysicsKeyValue( edict(), "slj", "0" );
	}

	RenewItems();
	TabulateAmmo();

#if defined( CLIENT_WEAPONS )
	// HACK:	This variable is saved/restored in CBaseMonster as a time variable, but we're using it
	//			as just a counter.  Ideally, this needs its own variable that's saved as a plain float.
	//			Barring that, we clear it out here instead of using the incorrect restored time value.
	m_flNextAttack = UTIL_WeaponTimeBase();
#endif

	SetSlowMotion( slowMotionWasEnabled );

	nextTime = SDL_GetTicks() + GET_TICK_INTERVAL();

	deathCameraYaw = 0.0f;
	CVAR_SET_FLOAT( "cam_idealyaw", 0.0f );
	CVAR_SET_FLOAT( "cam_idealpitch", 0.0f );
	CVAR_SET_FLOAT( "cam_idealdist", 70.0f );
	CVAR_SET_FLOAT( "cam_command", 2.0f );

	allowedToReactOnPainkillerPickup = 0.0f;
	allowedToReactOnPainkillerTake = 0.0f;
	allowedToReactOnPainkillerNeed = 0.0f;

	allowedToReactOnMinorInjury = 0.0f;
	allowedToReactOnSeriousInjury = 0.0f;

	dumbShots = 0;
	readyToComplainAboutDumbShots = false;
	allowedToComplainAboutDumbShots = 0.0f;

	allowedToComplainAboutKillingInnocent = 0.0f;

	allowedToSayAboutRhetoricalQuestion = 0.0f;
	rhetoricalQuestionHolder = NULL;

	allowedToSwear = 0.0f;

	latestMaxCommentaryTime = 0.0f;
	latestMaxCommentaryIsImportant = false;

	postRestoreDelay = gpGlobals->time + 0.01f;

	return status;
}

void CBasePlayer::ComplainAboutKillingInnocent()
{
	if ( CVAR_GET_FLOAT( "max_commentary_kill_innocent" ) <= 0.0f || gpGlobals->time < allowedToComplainAboutKillingInnocent || RANDOM_LONG( 0, 100 ) > 33 ) {
		return;
	}

	char fileName[256];
	sprintf_s( fileName, "max/innocent_killed/INNOCENT_KILLED_%d.wav", RANDOM_LONG( 1, 5 ) );
	TryToPlayMaxCommentary( MAKE_STRING( fileName ), false );

	allowedToComplainAboutKillingInnocent = gpGlobals->time + 60.0f;
}

void CBasePlayer::ComplainAboutNoAmmo( bool weaponIsBulletBased )
{
	if ( CVAR_GET_FLOAT( "max_commentary_no_ammo" ) <= 0.0f || gpGlobals->time < allowedToComplainAboutNoAmmo || RANDOM_LONG( 0, 100 ) > 50 ) {
		return;
	}

	char fileName[256];
	if ( weaponIsBulletBased ) {
		sprintf_s( fileName, "max/no_ammo/NO_AMMO_%d.wav", RANDOM_LONG( 1, 20 ) );
	} else {
		sprintf_s( fileName, "max/no_ammo/NO_AMMO_%d.wav", RANDOM_LONG( 1, 13 ) ); // not including bullet subset
	}
	TryToPlayMaxCommentary( MAKE_STRING( fileName ), false );

	allowedToComplainAboutNoAmmo = gpGlobals->time + 10.0f;
}

void CBasePlayer::AnswerAboutRhetoricalQuestion()
{
	CBaseEntity *pEntity = NULL;
	while ( ( pEntity = UTIL_FindEntityInSphere( pEntity, this->pev->origin, 256.0f ) ) != NULL ) {
		if ( pEntity->edict() == rhetoricalQuestionHolder ) {
			AddToSoundQueue( MAKE_STRING( "max/rhetorical_question.wav" ), 0.1f, true, true );
		}
	}

	allowedToSayAboutRhetoricalQuestion = 0.0f;
	rhetoricalQuestionHolder = NULL;
}

void CBasePlayer::OnBulletHit( CBaseEntity *hitEntity )
{
	if ( CVAR_GET_FLOAT( "max_commentary_wasted_shots" ) <= 0.0f || gpGlobals->time < allowedToComplainAboutDumbShots ) {
		return;
	}

	const char *entityClassName = STRING( hitEntity->pev->classname );

	if ( CBreakable *breakable = dynamic_cast<CBreakable *>( hitEntity ) ) {
		if ( FStringNull( breakable->pev->target ) && !m_iszKillTarget ) {
			dumbShots++;
		} else {
			dumbShots = 0;
			readyToComplainAboutDumbShots = false;
			return;
		}
	} else if ( hitEntity->pev->takedamage <= 0.0f ) {
		dumbShots++;
	} else {
		dumbShots = 0;
		readyToComplainAboutDumbShots = false;
		return;
	}

	if ( RANDOM_LONG( 40, 200 ) < dumbShots ) {
		readyToComplainAboutDumbShots = true;
	}
}

void CBasePlayer::SelectNextItem( int iItem )
{
	CBasePlayerItem *pItem;

	pItem = m_rgpPlayerItems[ iItem ];
	
	if (!pItem)
		return;

	if (pItem == m_pActiveItem)
	{
		// select the next one in the chain
		pItem = m_pActiveItem->m_pNext; 
		if (! pItem)
		{
			return;
		}

		CBasePlayerItem *pLast;
		pLast = pItem;
		while (pLast->m_pNext)
			pLast = pLast->m_pNext;

		// relink chain
		pLast->m_pNext = m_pActiveItem;
		m_pActiveItem->m_pNext = NULL;
		m_rgpPlayerItems[ iItem ] = pItem;
	}

	ResetAutoaim( );

	// FIX, this needs to queue them up and delay
	if (m_pActiveItem)
	{
		m_pActiveItem->Holster( );
	}
	
	m_pActiveItem = pItem;

	if (m_pActiveItem)
	{
		m_pActiveItem->Deploy( );
		m_pActiveItem->UpdateItemInfo( );
	}
}

void CBasePlayer::SelectItem(const char *pstr)
{
	if (!pstr)
		return;

	CBasePlayerItem *pItem = NULL;

	for (int i = 0; i < MAX_ITEM_TYPES; i++)
	{
		if (m_rgpPlayerItems[i])
		{
			pItem = m_rgpPlayerItems[i];
	
			while (pItem)
			{
				if (FClassnameIs(pItem->pev, pstr))
					break;
				pItem = pItem->m_pNext;
			}
		}

		if (pItem)
			break;
	}

	if (!pItem)
		return;

	
	if (pItem == m_pActiveItem)
		return;

	if ( pItem->locked ) {
		return;
	}

	ResetAutoaim( );

	// FIX, this needs to queue them up and delay
	if (m_pActiveItem)
		m_pActiveItem->Holster( );
	
	m_pLastItem = m_pActiveItem;
	m_pActiveItem = pItem;

	if (m_pActiveItem)
	{
		m_pActiveItem->Deploy( );
		m_pActiveItem->UpdateItemInfo( );
	}
}

void CBasePlayer::SelectLastItem(void)
{
	if (!m_pLastItem)
	{
		return;
	}

	if ( m_pActiveItem && !m_pActiveItem->CanHolster() )
	{
		return;
	}

	if ( m_pLastItem->locked ) {
		return;
	}

	ResetAutoaim( );

	// FIX, this needs to queue them up and delay
	if (m_pActiveItem)
		m_pActiveItem->Holster( );
	
	CBasePlayerItem *pTemp = m_pActiveItem;
	m_pActiveItem = m_pLastItem;
	m_pLastItem = pTemp;
	m_pActiveItem->Deploy( );
	m_pActiveItem->UpdateItemInfo( );
}

//==============================================
// HasWeapons - do I have any weapons at all?
//==============================================
BOOL CBasePlayer::HasWeapons( void )
{
	int i;

	for ( i = 0 ; i < MAX_ITEM_TYPES ; i++ )
	{
		if ( m_rgpPlayerItems[ i ] )
		{
			return TRUE;
		}
	}

	return FALSE;
}

void CBasePlayer::SelectPrevItem( int iItem )
{
}


const char *CBasePlayer::TeamID( void )
{
	if ( pev == NULL )		// Not fully connected yet
		return "";

	// return their team name
	return m_szTeamName;
}


//==============================================
// !!!UNDONE:ultra temporary SprayCan entity to apply
// decal frame at a time. For PreAlpha CD
//==============================================
class CSprayCan : public CBaseEntity
{
public:
	void	Spawn ( entvars_t *pevOwner );
	void	Think( void );

	virtual int	ObjectCaps( void ) { return FCAP_DONT_SAVE; }
};

void CSprayCan::Spawn ( entvars_t *pevOwner )
{
	pev->origin = pevOwner->origin + Vector ( 0 , 0 , 32 );
	pev->angles = pevOwner->v_angle;
	pev->owner = ENT(pevOwner);
	pev->frame = 0;

	pev->nextthink = gpGlobals->time + 0.1;
	EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/sprayer.wav", 1, ATTN_NORM, true);
}

void CSprayCan::Think( void )
{
	TraceResult	tr;	
	int playernum;
	int nFrames;
	CBasePlayer *pPlayer;
	
	pPlayer = (CBasePlayer *)GET_PRIVATE(pev->owner);

	if (pPlayer)
		nFrames = pPlayer->GetCustomDecalFrames();
	else
		nFrames = -1;

	playernum = ENTINDEX(pev->owner);
	
	// ALERT(at_console, "Spray by player %i, %i of %i\n", playernum, (int)(pev->frame + 1), nFrames);

	UTIL_MakeVectors(pev->angles);
	UTIL_TraceLine ( pev->origin, pev->origin + gpGlobals->v_forward * 128, ignore_monsters, pev->owner, & tr);

	// No customization present.
	if (nFrames == -1)
	{
		UTIL_DecalTrace( &tr, DECAL_LAMBDA6 );
		UTIL_Remove( this );
	}
	else
	{
		UTIL_PlayerDecalTrace( &tr, playernum, pev->frame, TRUE );
		// Just painted last custom frame.
		if ( pev->frame++ >= (nFrames - 1))
			UTIL_Remove( this );
	}

	pev->nextthink = gpGlobals->time + 0.1;
}

class	CBloodSplat : public CBaseEntity
{
public:
	void	Spawn ( entvars_t *pevOwner );
	void	Spray ( void );
};

void CBloodSplat::Spawn ( entvars_t *pevOwner )
{
	pev->origin = pevOwner->origin + Vector ( 0 , 0 , 32 );
	pev->angles = pevOwner->v_angle;
	pev->owner = ENT(pevOwner);

	SetThink ( &CBloodSplat::Spray );
	pev->nextthink = gpGlobals->time + 0.1;
}

void CBloodSplat::Spray ( void )
{
	TraceResult	tr;	
	
	if ( g_Language != LANGUAGE_GERMAN )
	{
		UTIL_MakeVectors(pev->angles);
		UTIL_TraceLine ( pev->origin, pev->origin + gpGlobals->v_forward * 128, ignore_monsters, pev->owner, & tr);

		UTIL_BloodDecalTrace( &tr, BLOOD_COLOR_RED );
	}
	SetThink ( &CBloodSplat::SUB_Remove );
	pev->nextthink = gpGlobals->time + 0.1;
}

//==============================================



void CBasePlayer::GiveNamedItem( const char *pszName, bool nonCheat )
{
	edict_t	*pent;

	int istr = MAKE_STRING(pszName);

	pent = CREATE_NAMED_ENTITY(istr);
	if ( FNullEnt( pent ) )
	{
		ALERT ( at_console, "NULL Ent in GiveNamedItem!\n" );
		return;
	}
	pent->v.effects |= EF_NODRAW;
	VARS( pent )->origin = pev->origin + Vector( 0, 0, 40.0f );
	pent->v.spawnflags |= SF_NORESPAWN;

	DispatchSpawn( pent );
	DispatchTouch( pent, ENT( pev ) );

	if ( !nonCheat ) {
		gameplayModsData.usedCheat = true;
	}
}



CBaseEntity *FindEntityForward( CBaseEntity *pMe )
{
	TraceResult tr;

	UTIL_MakeVectors(pMe->pev->v_angle);
	UTIL_TraceLine(pMe->pev->origin + pMe->pev->view_ofs,pMe->pev->origin + pMe->pev->view_ofs + gpGlobals->v_forward * 8192,dont_ignore_monsters, pMe->edict(), &tr );
	if ( tr.flFraction != 1.0 && !FNullEnt( tr.pHit) )
	{
		CBaseEntity *pHit = CBaseEntity::Instance( tr.pHit );
		return pHit;
	}
	return NULL;
}

void CBasePlayer::ToggleSlowMotion() {
	if ( gameplayMods::superHot.isActive() && !gameplayMods::superHotToggleable.isActive() ) {
		return;
	}

	if ( slowMotionWasEnabled ) {
		DeactivateSlowMotion();
	} else {

		if ( !gameplayMods::slowmotionOnlyDiving.isActive() && ActivateSlowMotion() ) {
			if ( slowMotionWasEnabled ) {
				MESSAGE_BEGIN( MSG_ONE, gmsgFlash, NULL, pev );
					WRITE_BYTE( 0 );
					WRITE_BYTE( 0 );
					WRITE_BYTE( 0 );
					WRITE_BYTE( 120 );
					WRITE_FLOAT( 0.1 );
				MESSAGE_END();
				EMIT_SOUND( ENT( pev ), CHAN_AUTO, "slowmo/slowmo_start.wav", 1, ATTN_NORM, true );
			}
		}
	}
}

bool CBasePlayer::ActivateSlowMotion()
{
	if ( slowMotionWasEnabled || gameplayMods::slowmotionConstant.isActive() || !( pev->weapons & ( 1 << WEAPON_SUIT ) ) ) {
		return false;
	}

	if ( CHalfLifeRules *rules = dynamic_cast< CHalfLifeRules * >( g_pGameRules ) ) {
		if ( rules->ended ) {
			return false;
		}
	}

	if ( gameplayMods::superHotToggleable.isActive() ) {
		if ( slowMotionCharge < 12 ) {
			return false;
		}

		slowMotionCharge -= 10;
	}

	if ( slowMotionCharge <= 0 ) {
		return false;
	}

	SetSlowMotion( true );

	return true;
}

bool CBasePlayer::DeactivateSlowMotion( bool smooth )
{
	if ( !slowMotionWasEnabled || gameplayMods::slowmotionConstant.isActive() ) {
		return false;
	}

	if ( CHalfLifeRules *rules = dynamic_cast< CHalfLifeRules * >( g_pGameRules ) ) {
		if ( rules->ended ) {
			return false;
		}
	}

	m_flNextAttack += nextAttackSlowmotionOffset;
	nextAttackSlowmotionOffset = 0.0f;

	if ( smooth ) {
		nextSmoothTimeScaleChange = gpGlobals->time + 0.01f;
	}

	SetSlowMotion( false );

	EMIT_SOUND( ENT( pev ), CHAN_AUTO, "slowmo/slowmo_end.wav", 1, ATTN_NORM, true );

	CBaseEntity *entity = NULL;
	while ( ( entity = UTIL_FindEntityInSphere( entity, pev->origin, 8192.0f ) ) != NULL ) {
		if (
			FStrEq( STRING( entity->pev->classname ), "bullet" ) &&
			entity->auxOwner == edict() &&
			entity->pev->velocity.Length() < 2000
		) {
			if ( CBullet *bullet = dynamic_cast<CBullet *>( entity ) ) {
				bullet->pev->velocity = bullet->pev->velocity.Normalize() * 2000;
				bullet->lastVelocity = bullet->pev->velocity;
				bullet->ActivateTrail();
			}
		} else if ( FStrEq( STRING( entity->pev->classname ), "bolt" ) ) {
			entity->pev->velocity = entity->pev->velocity.Normalize() * ( entity->pev->waterlevel == 3 ? 1000 : 2000 );
		}
	}

	return true;
}

void CBasePlayer::SetSlowMotion( BOOL slowMotionEnabled ) {
	auto timescale_multiplier = *gameplayMods::timescale.isActive<float>() + gameplayModsData.timescaleAdditive;

	if ( slowMotionEnabled ) {
		if ( !gameplayMods::superHotToggleable.isActive() ) {
			desiredTimeScale = using_sys_timescale ? 0.25f * timescale_multiplier : GET_FRAMERATE_BASE() / 4.0f;	
		}
		slowMotionUpdateTime = SLOWMOTION_DRAIN_TIME + gpGlobals->time;
		this->slowMotionWasEnabled = true;
		nextSmoothTimeScaleChange = 0.0f;
	}
	else {
		if ( !nextSmoothTimeScaleChange ) {
			desiredTimeScale = using_sys_timescale ? 1.0f * timescale_multiplier : GET_FRAMERATE_BASE();
		}
		this->slowMotionWasEnabled = false;
	}
}

// Done using SDL
// Required for smooth host_framerate appliance and slow motion
// Slow motion with host_framerate has not been tested with FPS below 100
void CBasePlayer::ApplyFPSCap() {
	Uint32 timeLeft = 0;

	Uint32 now = SDL_GetTicks();
	if (nextTime > now) {
		timeLeft = nextTime - now;
	}
	SDL_Delay(timeLeft);
	nextTime += GET_TICK_INTERVAL();
}

BOOL CBasePlayer :: FlashlightIsOn( void )
{
	return FBitSet(pev->effects, EF_DIMLIGHT);
}


void CBasePlayer :: FlashlightTurnOn( void )
{
	if ( !g_pGameRules->FAllowFlashlight() )
	{
		return;
	}

	if ( (pev->weapons & (1<<WEAPON_SUIT)) )
	{
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, SOUND_FLASHLIGHT_ON, 1.0, ATTN_NORM, 0, PITCH_NORM, true );
		SetBits(pev->effects, EF_DIMLIGHT);
		MESSAGE_BEGIN( MSG_ONE, gmsgFlashlight, NULL, pev );
		WRITE_BYTE(1);
		WRITE_BYTE(m_iFlashBattery);
		MESSAGE_END();

		m_flFlashLightTime = FLASH_DRAIN_TIME + gpGlobals->time;

	}
}


void CBasePlayer :: FlashlightTurnOff( void )
{
	EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, SOUND_FLASHLIGHT_OFF, 1.0, ATTN_NORM, 0, PITCH_NORM, true );
    ClearBits(pev->effects, EF_DIMLIGHT);
	MESSAGE_BEGIN( MSG_ONE, gmsgFlashlight, NULL, pev );
	WRITE_BYTE(0);
	WRITE_BYTE(m_iFlashBattery);
	MESSAGE_END();

	m_flFlashLightTime = FLASH_CHARGE_TIME + gpGlobals->time;

}

/*
===============
ForceClientDllUpdate

When recording a demo, we need to have the server tell us the entire client state
so that the client side .dll can behave correctly.
Reset stuff so that the state is transmitted.
===============
*/
void CBasePlayer :: ForceClientDllUpdate( void )
{
	m_iClientHealth  = -1;
	m_iClientBattery = -1;
	m_iTrain |= TRAIN_NEW;  // Force new train message.
	m_fWeapon = FALSE;          // Force weapon send
	m_fKnownItem = FALSE;    // Force weaponinit messages.
	m_fInitHUD = TRUE;		// Force HUD gmsgResetHUD message

	// Now force all the necessary messages
	//  to be sent.
	UpdateClientData();
}

/*
============
ImpulseCommands
============
*/
void CBasePlayer::ImpulseCommands( )
{
	TraceResult	tr;// UNDONE: kill me! This is temporary for PreAlpha CDs

	// Handle use events
	PlayerUse();
		
	int iImpulse = (int)pev->impulse;
	
	switch (iImpulse)
	{
	case 22:
		ToggleSlowMotion();
		break;
	case 23:
		UsePainkiller();
		break;
	case 24:
		gameplayModsData.slowmotionInfiniteCheat = !gameplayModsData.slowmotionInfiniteCheat;
		if ( gameplayModsData.slowmotionInfiniteCheat ) {
			g_engfuncs.pfnServerPrint( "Infinite slowmotion ON\n" );
			gameplayModsData.usedCheat = true;
		}
		else {
			g_engfuncs.pfnServerPrint( "Infinite slowmotion OFF\n" );
		}
		break;

	case 99:
		{

		int iOn;

		if (!gmsgLogo)
		{
			iOn = 1;
			gmsgLogo = REG_USER_MSG("Logo", 1);
		} 
		else 
		{
			iOn = 0;
		}
		
		ASSERT( gmsgLogo > 0 );
		// send "health" update message
		MESSAGE_BEGIN( MSG_ONE, gmsgLogo, NULL, pev );
			WRITE_BYTE(iOn);
		MESSAGE_END();

		if(!iOn)
			gmsgLogo = 0;
		break;
		}
	case 100:
        // temporary flashlight for level designers
        if ( FlashlightIsOn() )
		{
			FlashlightTurnOff();
		}
        else 
		{
			FlashlightTurnOn();
		}
		break;

	case	201:// paint decal
		
		if ( gpGlobals->time < m_flNextDecalTime )
		{
			// too early!
			break;
		}

		UTIL_MakeVectors(pev->v_angle);
		UTIL_TraceLine ( pev->origin + pev->view_ofs, pev->origin + pev->view_ofs + gpGlobals->v_forward * 128, ignore_monsters, ENT(pev), & tr);

		if ( tr.flFraction != 1.0 )
		{// line hit something, so paint a decal
			m_flNextDecalTime = gpGlobals->time + decalfrequency.value;
			CSprayCan *pCan = GetClassPtr((CSprayCan *)NULL);
			pCan->Spawn( pev );
		}

		break;

	default:
		// check all of the cheat impulse commands now
		CheatImpulseCommands( iImpulse );
		break;
	}
	
	pev->impulse = 0;
}

//=========================================================
//=========================================================
void CBasePlayer::CheatImpulseCommands( int iImpulse )
{
#if !defined( HLDEMO_BUILD )
	if ( !UTIL_CheatsAllowed() )
	{
		return;
	}

	CBaseEntity *pEntity;
	TraceResult tr;

	switch ( iImpulse )
	{
	case 76:
		{
			if (!giPrecacheGrunt)
			{
				giPrecacheGrunt = 1;
				ALERT(at_console, "You must now restart to use Grunt-o-matic.\n");
			}
			else
			{
				UTIL_MakeVectors( Vector( 0, pev->v_angle.y, 0 ) );
				Create("monster_human_grunt", pev->origin + gpGlobals->v_forward * 128, pev->angles);
			}
			break;
		}


	case 101:
		GiveAll();
		break;

	case 102:
		// Gibbage!!!
		CGib::SpawnRandomGibs( pev, 1, 1 );
		break;

	case 103:
		// What the hell are you doing?
		pEntity = FindEntityForward( this );
		if ( pEntity )
		{
			CBaseMonster *pMonster = pEntity->MyMonsterPointer();
			if ( pMonster )
				pMonster->ReportAIState();
		}
		break;

	case 104:
		// Dump all of the global state varaibles (and global entity names)
		gGlobalState.DumpGlobals();
		break;

	case	105:// player makes no sound for monsters to hear.
		{
			if ( m_fNoPlayerSound )
			{
				ALERT ( at_console, "Player is audible\n" );
				m_fNoPlayerSound = FALSE;
			}
			else
			{
				ALERT ( at_console, "Player is silent\n" );
				m_fNoPlayerSound = TRUE;
			}
			break;
		}

	case 106:
		// Give me the classname and targetname of this entity.
		pEntity = FindEntityForward( this );
		if ( pEntity )
		{
			ALERT ( at_console, "Classname: %s", STRING( pEntity->pev->classname ) );
			
			if ( !FStringNull ( pEntity->pev->targetname ) )
			{
				ALERT ( at_console, " - Targetname: %s\n", STRING( pEntity->pev->targetname ) );
			}
			else
			{
				ALERT ( at_console, " - TargetName: No Targetname\n" );
			}

			ALERT ( at_console, "Model: %s\n", STRING( pEntity->pev->model ) );
			if ( pEntity->pev->globalname )
				ALERT ( at_console, "Globalname: %s\n", STRING( pEntity->pev->globalname ) );
		}
		break;

	case 107:
		{
			TraceResult tr;

			edict_t		*pWorld = g_engfuncs.pfnPEntityOfEntIndex( 0 );

			Vector start = pev->origin + pev->view_ofs;
			Vector end = start + gpGlobals->v_forward * 1024;
			UTIL_TraceLine( start, end, ignore_monsters, edict(), &tr );
			if ( tr.pHit )
				pWorld = tr.pHit;
			const char *pTextureName = TRACE_TEXTURE( pWorld, start, end );
			if ( pTextureName )
				ALERT( at_console, "Texture: %s\n", pTextureName );
		}
		break;
	case	195:// show shortest paths for entire level to nearest node
		{
			Create("node_viewer_fly", pev->origin, pev->angles);
		}
		break;
	case	196:// show shortest paths for entire level to nearest node
		{
			Create("node_viewer_large", pev->origin, pev->angles);
		}
		break;
	case	197:// show shortest paths for entire level to nearest node
		{
			Create("node_viewer_human", pev->origin, pev->angles);
		}
		break;
	case	199:// show nearest node and all connections
		{
			ALERT ( at_console, "%d\n", WorldGraph.FindNearestNode ( pev->origin, bits_NODE_GROUP_REALM ) );
			WorldGraph.ShowNodeConnections ( WorldGraph.FindNearestNode ( pev->origin, bits_NODE_GROUP_REALM ) );
		}
		break;
	case	202:// Random blood splatter
		UTIL_MakeVectors(pev->v_angle);
		UTIL_TraceLine ( pev->origin + pev->view_ofs, pev->origin + pev->view_ofs + gpGlobals->v_forward * 128, ignore_monsters, ENT(pev), & tr);

		if ( tr.flFraction != 1.0 )
		{// line hit something, so paint a decal
			CBloodSplat *pBlood = GetClassPtr((CBloodSplat *)NULL);
			pBlood->Spawn( pev );
		}
		break;
	case	203:// remove creature.
		pEntity = FindEntityForward( this );
		if ( pEntity )
		{
			if ( pEntity->pev->takedamage )
				pEntity->SetThink(&CBaseEntity::SUB_Remove);
		}
		break;
	}
#endif	// HLDEMO_BUILD
}

//
// Add a weapon to the player (Item == Weapon == Selectable Object)
//
int CBasePlayer::AddPlayerItem( CBasePlayerItem *pItem )
{
	CBasePlayerItem *pInsert;
	
	pInsert = m_rgpPlayerItems[pItem->iItemSlot()];

	while (pInsert)
	{
		if (FClassnameIs( pInsert->pev, STRING( pItem->pev->classname) ))
		{
			if (pItem->AddDuplicate( pInsert ))
			{
				g_pGameRules->PlayerGotWeapon ( this, pItem );
				pItem->CheckRespawn();

				// ugly hack to update clip w/o an update clip message
				pInsert->UpdateItemInfo( );
				if (m_pActiveItem)
					m_pActiveItem->UpdateItemInfo( );

				pItem->Kill( );
			}
			else if (gEvilImpulse101)
			{
				// FIXME: remove anyway for deathmatch testing
				pItem->Kill( );
			}
			return FALSE;
		}
		pInsert = pInsert->m_pNext;
	}

	if (pItem->AddToPlayer( this ))
	{
		g_pGameRules->PlayerGotWeapon ( this, pItem );
		pItem->CheckRespawn();

		pItem->m_pNext = m_rgpPlayerItems[pItem->iItemSlot()];
		m_rgpPlayerItems[pItem->iItemSlot()] = pItem;

		// should we switch to this item?
		if ( g_pGameRules->FShouldSwitchWeapon( this, pItem ) )
		{
			SwitchWeapon( pItem );
		}
		return TRUE;
	}
	else if (gEvilImpulse101)
	{
		// FIXME: remove anyway for deathmatch testing
		pItem->Kill( );
	}
	return FALSE;
}



int CBasePlayer::RemovePlayerItem( CBasePlayerItem *pItem )
{
	if (m_pActiveItem == pItem)
	{
		ResetAutoaim( );
		pItem->Holster( );
		pItem->pev->nextthink = 0;// crowbar may be trying to swing again, etc.
		pItem->SetThink( NULL );
		m_pActiveItem = NULL;
		pev->viewmodel = 0;
		pev->weaponmodel = 0;
	}
	else if ( m_pLastItem == pItem )
		m_pLastItem = NULL;

	CBasePlayerItem *pPrev = m_rgpPlayerItems[pItem->iItemSlot()];

	if (pPrev == pItem)
	{
		m_rgpPlayerItems[pItem->iItemSlot()] = pItem->m_pNext;
		pev->weapons &= ~( 1 << pPrev->m_iId );
		return TRUE;
	}
	else
	{
		while (pPrev && pPrev->m_pNext != pItem)
		{
			pPrev = pPrev->m_pNext;
		}
		if (pPrev)
		{
			pPrev->m_pNext = pItem->m_pNext;
			pev->weapons &= ~( 1 << pItem->m_iId );
			return TRUE;
		}
	}
	return FALSE;
}


//
// Returns the unique ID for the ammo, or -1 if error
//
int CBasePlayer :: GiveAmmo( int iCount, char *szName, int iMax )
{
	if ( !szName )
	{
		// no ammo.
		return -1;
	}

	if ( !g_pGameRules->CanHaveAmmo( this, szName, iMax ) )
	{
		// game rules say I can't have any more of this ammo type.
		return -1;
	}

	int i = 0;

	i = GetAmmoIndex( szName );

	if ( i < 0 || i >= MAX_AMMO_SLOTS )
		return -1;

	int iAdd = min( iCount, iMax - m_rgAmmo[i] );
	if ( iAdd < 1 )
		return i;

	m_rgAmmo[ i ] += iAdd;


	if ( gmsgAmmoPickup )  // make sure the ammo messages have been linked first
	{
		// Send the message that ammo has been picked up
		MESSAGE_BEGIN( MSG_ONE, gmsgAmmoPickup, NULL, pev );
			WRITE_BYTE( GetAmmoIndex(szName) );		// ammo ID
			WRITE_BYTE( iAdd );		// amount
		MESSAGE_END();
	}

	TabulateAmmo();

	return i;
}


/*
============
ItemPreFrame

Called every frame by the player PreThink
============
*/
void CBasePlayer::ItemPreFrame()
{
#if defined( CLIENT_WEAPONS )
    if ( m_flNextAttack > 0 )
#else
    if ( gpGlobals->time < m_flNextAttack )
#endif
	{
		return;
	}

	if (!m_pActiveItem)
		return;

	m_pActiveItem->ItemPreFrame( );
}


/*
============
ItemPostFrame

Called every frame by the player PostThink
============
*/
void CBasePlayer::ItemPostFrame()
{
	static int fInSelect = FALSE;
	
	ImpulseCommands();

	// check if the player is using a tank
	if ( m_pTank != NULL )
		return;

#if defined( CLIENT_WEAPONS )
    if ( m_flNextAttack > 0 )
#else
    if ( gpGlobals->time < m_flNextAttack )
#endif
	{
		return;
	}

	

	if (!m_pActiveItem)
		return;

	m_pActiveItem->ItemPostFrame( );
}

int CBasePlayer::AmmoInventory( int iAmmoIndex )
{
	if (iAmmoIndex == -1)
	{
		return -1;
	}

	return m_rgAmmo[ iAmmoIndex ];
}

int CBasePlayer::GetAmmoIndex(const char *psz)
{
	int i;

	if (!psz)
		return -1;

	for (i = 1; i < MAX_AMMO_SLOTS; i++)
	{
		if ( !CBasePlayerItem::AmmoInfoArray[i].pszName )
			continue;

		if (stricmp( psz, CBasePlayerItem::AmmoInfoArray[i].pszName ) == 0)
			return i;
	}

	return -1;
}

// Called from UpdateClientData
// makes sure the client has all the necessary ammo info,  if values have changed
void CBasePlayer::SendAmmoUpdate(void)
{
	for (int i=0; i < MAX_AMMO_SLOTS;i++)
	{
		if (m_rgAmmo[i] != m_rgAmmoLast[i])
		{
			m_rgAmmoLast[i] = m_rgAmmo[i];

			ASSERT( m_rgAmmo[i] >= 0 );
			ASSERT( m_rgAmmo[i] < 255 );

			// send "Ammo" update message
			MESSAGE_BEGIN( MSG_ONE, gmsgAmmoX, NULL, pev );
				WRITE_BYTE( i );
				WRITE_BYTE( max( min( m_rgAmmo[i], 254 ), 0 ) );  // clamp the value to one byte
			MESSAGE_END();
		}
	}
}

/*
=========================================================
	UpdateClientData

resends any changed player HUD info to the client.
Called every frame by PlayerPreThink
Also called at start of demo recording and playback by
ForceClientDllUpdate to ensure the demo gets messages
reflecting all of the HUD state info.
=========================================================
*/
void CBasePlayer :: UpdateClientData( void )
{
	if ( !using_sys_timescale ) {
		ApplyFPSCap( );
	}

	gameplayModsData.SendToClient();

	char com[256];
	if ( using_sys_timescale ) {
		sprintf_s( com, "sys_timescale %f\n", desiredTimeScale );
		SERVER_COMMAND( com );
		sprintf_s( com, "host_framerate 0\n" );
		SERVER_COMMAND( com );
	} else {
		sprintf_s( com, "host_framerate %f\n", desiredTimeScale );
		SERVER_COMMAND( com );
	}

	if (m_fInitHUD)
	{
		m_fInitHUD = FALSE;
		gInitHUD = FALSE;
		
		MESSAGE_BEGIN( MSG_ONE, gmsgResetHUD, NULL, pev );
			WRITE_BYTE( 0 );
		MESSAGE_END();

		if ( !m_fGameHUDInitialized )
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgInitHUD, NULL, pev );
			MESSAGE_END();

			g_pGameRules->InitHUD( this );
			m_fGameHUDInitialized = TRUE;
			
			m_iObserverLastMode = OBS_ROAMING;
			
			if ( g_pGameRules->IsMultiplayer() )
			{
				FireTargets( "game_playerjoin", this, this, USE_TOGGLE, 0 );
			}
		}

		FireTargets( "game_playerspawn", this, this, USE_TOGGLE, 0 );

		InitStatusBar();
	}

	if ( m_iHideHUD != m_iClientHideHUD )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgHideWeapon, NULL, pev );
			WRITE_BYTE( m_iHideHUD );
		MESSAGE_END();

		m_iClientHideHUD = m_iHideHUD;
	}

	if ( m_iFOV != m_iClientFOV )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgSetFOV, NULL, pev );
			WRITE_BYTE( m_iFOV );
		MESSAGE_END();

		// cache FOV change at end of function, so weapon updates can see that FOV has changed
	}

	// HACKHACK -- send the message to display the game title
	if (gDisplayTitle && !gameTitleShown)
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgShowGameTitle, NULL, pev );
		WRITE_BYTE( 0 );
		MESSAGE_END();
		gDisplayTitle = 0;
		gameTitleShown = true;
	}

	if (pev->health != m_iClientHealth || painkillerEnergy)
	{
#define clamp( val, min, max ) ( ((val) > (max)) ? (max) : ( ((val) < (min)) ? (min) : (val) ) )
		int iHealth = clamp( pev->health, 0, 2147483647 );  // make sure that no negative health values are sent
		if ( pev->health > 0.0f && pev->health <= 1.0f )
			iHealth = 1;

		// send "health" update message
		MESSAGE_BEGIN( MSG_ONE, gmsgHealth, NULL, pev );
			WRITE_LONG( iHealth );
			WRITE_SHORT( painkillerEnergy );
		MESSAGE_END();

		m_iClientHealth = pev->health;
	}

	MESSAGE_BEGIN(MSG_ONE, gmsgSlowMotion, NULL, pev);
		WRITE_BYTE(slowMotionCharge);
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_ONE, gmsgPainkillerCount, NULL, pev );
		WRITE_BYTE( painkillerCount );
	MESSAGE_END();

	if (pev->armorvalue != m_iClientBattery)
	{
		m_iClientBattery = pev->armorvalue;

		ASSERT( gmsgBattery > 0 );
		// send "health" update message
		MESSAGE_BEGIN( MSG_ONE, gmsgBattery, NULL, pev );
			WRITE_SHORT( (int)pev->armorvalue);
		MESSAGE_END();
	}

	if (pev->dmg_take || pev->dmg_save || m_bitsHUDDamage != m_bitsDamageType)
	{
		// Comes from inside me if not set
		Vector damageOrigin = pev->origin;
		// send "damage" message
		// causes screen to flash, and pain compass to show direction of damage
		edict_t *other = pev->dmg_inflictor;
		if ( other )
		{
			CBaseEntity *pEntity = CBaseEntity::Instance(other);
			if ( pEntity )
				damageOrigin = pEntity->Center();
		}

		// only send down damage type that have hud art
		int visibleDamageBits = m_bitsDamageType & DMG_SHOWNHUD;

		MESSAGE_BEGIN( MSG_ONE, gmsgDamage, NULL, pev );
			WRITE_BYTE( pev->dmg_save );
			WRITE_BYTE( pev->dmg_take );
			WRITE_LONG( visibleDamageBits );
			WRITE_COORD( damageOrigin.x );
			WRITE_COORD( damageOrigin.y );
			WRITE_COORD( damageOrigin.z );
		MESSAGE_END();
	
		pev->dmg_take = 0;
		pev->dmg_save = 0;
		m_bitsHUDDamage = m_bitsDamageType;
		
		// Clear off non-time-based damage indicators
		m_bitsDamageType &= DMG_TIMEBASED;
	}

	// Update Flashlight
	if ((m_flFlashLightTime) && (m_flFlashLightTime <= gpGlobals->time))
	{
		if (FlashlightIsOn())
		{
			if (m_iFlashBattery)
			{
				m_flFlashLightTime = FLASH_DRAIN_TIME + gpGlobals->time;
				//m_iFlashBattery--;
				
				if (!m_iFlashBattery)
					FlashlightTurnOff();
			}
		}
		else
		{
			if (m_iFlashBattery < 100)
			{
				m_flFlashLightTime = FLASH_CHARGE_TIME + gpGlobals->time;
				m_iFlashBattery++;
			}
			else
				m_flFlashLightTime = 0;
		}

		MESSAGE_BEGIN( MSG_ONE, gmsgFlashBattery, NULL, pev );
		WRITE_BYTE(m_iFlashBattery);
		MESSAGE_END();
	}

	// Regenerate health if you can
	if ( lastDamageTime <= gpGlobals->time && healthChargeTime <= gpGlobals->time && pev->deadflag == DEAD_NO )
	{
		if ( auto healthRegeneration = gameplayMods::healthRegeneration.isActive<HealthRegenerationInfo>() ) {
			if ( pev->health < healthRegeneration->max ) {
				healthChargeTime = healthRegeneration->frequency + gpGlobals->time;
				TakeHealth( 1, DMG_GENERIC );

				if ( pev->health >= 20 && healthRegeneration->max <= 30 ) {
					if (
						CVAR_GET_FLOAT( "max_commentary_near_death" ) > 0.0f &&
						RANDOM_LONG( 0, 100 ) < 50 &&
						gpGlobals->time > allowedToReactOnPainkillerNeed
					) {
						char fileName[256];
						if ( painkillerCount > 0 ) {
							sprintf_s( fileName, "max/painkiller/HAS_PILLS_%d.wav", RANDOM_LONG( 1, 6 ) );
						} else {
							sprintf_s( fileName, "max/painkiller/NO_PILLS_%d.wav", RANDOM_LONG( 1, 9 ) );
						}
						TryToPlayMaxCommentary( MAKE_STRING( fileName ), false );

						allowedToReactOnPainkillerNeed = gpGlobals->time + 30.0f;
					}
				}
			}
		}
	}

	if ( auto bleeding = gameplayMods::bleeding.isActive<BleedingInfo>() ) {
		if ( lastHealingTime <= gpGlobals->time && bleedTime <= gpGlobals->time && pev->deadflag == DEAD_NO ) {
			if ( pev->health > bleeding->handicap ) {
				bleedTime = bleeding->updatePeriod + gpGlobals->time;
				pev->health--;
				if ( pev->health <= 1.0f ) {
					// Force proper dying
					TakeDamage( pev, pev, 1.0f, DMG_GENERIC );
				}
			}
		}
	}

	if ( painkillerEnergy && nextPainkillerEffectTime <= gpGlobals->time && pev->deadflag == DEAD_NO ) {
		TakeHealth( 1, DMG_GENERIC );
		painkillerEnergy--;
		if ( auto painkillersSlow = gameplayMods::painkillersSlow.isActive<float>() ) {
			nextPainkillerEffectTime = *painkillersSlow + gpGlobals->time;
		} else {
			nextPainkillerEffectTime = 0.0f;
		}
	}

	MESSAGE_BEGIN( MSG_ONE, gmsgFadeOut, NULL, pev );
		WRITE_BYTE( gameplayModsData.fade );
	MESSAGE_END();

	if ( auto fadingOut = gameplayMods::fadingOut.isActive<FadeOutInfo>() ) {
		if ( lastHealingTime <= gpGlobals->time && fadeOutTime <= gpGlobals->time && pev->deadflag == DEAD_NO ) {
			fadeOutTime = fadingOut->updatePeriod + gpGlobals->time;
			gameplayModsData.fade--;
			if ( gameplayModsData.fade < fadingOut->threshold ) {
				gameplayModsData.fade = fadingOut->threshold;
			}
		}
	} else {
		gameplayModsData.fade = 255;
	}

	if ( last_fps_max != g_fps_max->value ) {
		last_fps_max = g_fps_max->value;
		SetSlowMotion( slowMotionWasEnabled );
	}

	if ( !using_sys_timescale && g_gl_vsync->value != 0.0f ) {
		g_engfuncs.pfnServerPrint( "gl_vsync has been automatically disabled - this is required for slowmotion to work correctly\n" );
		CVAR_SET_FLOAT( "gl_vsync", 0.0f );
	}

	if ( jumpedOnce && ( pev->flags & FL_ONGROUND ) ) {
		jumpedOnce = FALSE;
	}
	
	gameplayMods::OnFlagChange<BOOL>( gameplayModsData.lastSuperHotConstant, gameplayMods::superHot.isActive(), [this]( bool on ) {
		if ( !gameplayMods::superHotToggleable.isActive() ) {
			SetSlowMotion( false );
		}
	} );
	if ( gameplayModsData.lastSuperHotConstant ) {
		auto timescale_multiplier = *gameplayMods::timescale.isActive<float>() + gameplayModsData.timescaleAdditive;

		float base = using_sys_timescale ? 1.0f * timescale_multiplier : GET_FRAMERATE_BASE();

		if ( pev->deadflag == DEAD_NO ) {
			bool afterMP5Fire = false;
			if ( CBasePlayerWeapon *weapon = dynamic_cast<CBasePlayerWeapon*>( m_pActiveItem ) ) {
				if ( weapon->m_iId == WEAPON_MP5 && gpGlobals->time - weapon->m_flLastFireTime < 0.1 ) {
					afterMP5Fire = true;
				} else if ( ( weapon->m_iId == WEAPON_INGRAM || weapon->m_iId == WEAPON_INGRAM_TWIN ) && gpGlobals->time - weapon->m_flLastFireTime < 0.02 ) {
					afterMP5Fire = true;
				}
			}

			nextSuperHotMultiplierUpdate = using_sys_timescale ? 0.01 * timescale_multiplier : superHotMultiplier + gpGlobals->time;
			if ( superHotJumping && gpGlobals->time >= superHotJumping ) {
				superHotJumping = 0.0f;
			}

			if ( pev->button & ( IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT ) || ( jumpedOnce && superHotJumping && gpGlobals->time < superHotJumping ) || afterMP5Fire ) {
				superHotMultiplier += using_sys_timescale ? 0.02 * timescale_multiplier : ( base / 40.0f );
				if ( superHotMultiplier > base ) {
					superHotMultiplier = base;
				}
			} else {
				superHotMultiplier -= using_sys_timescale ? 0.05 * timescale_multiplier : base / 2.0f;
				if ( superHotMultiplier < ( using_sys_timescale ? 0.015 * timescale_multiplier : ( base / 20.0f ) ) ) {
					superHotMultiplier = using_sys_timescale ? 0.015 * timescale_multiplier : ( base / 20.0f );
				}
			}
		} else {
			superHotMultiplier = using_sys_timescale ? 0.25 * timescale_multiplier : base / 4.0f;
		}

		desiredTimeScale = superHotMultiplier;
	} else {
		superHotMultiplier = 0.0005f;
		superHotJumping = 0.0f;
	}

	gameplayModsData.holdingTwinWeapons = m_pActiveItem && ( m_pActiveItem->m_iId == WEAPON_GLOCK_TWIN || m_pActiveItem->m_iId == WEAPON_INGRAM_TWIN );

	if ( nextSmoothTimeScaleChange && nextSmoothTimeScaleChange <= gpGlobals->time ) {
		auto timescale_multiplier = *gameplayMods::timescale.isActive<float>() + gameplayModsData.timescaleAdditive;
		float cap = using_sys_timescale ? 1.0f * timescale_multiplier : GET_FRAMERATE_BASE();
		
		desiredTimeScale *= 1.08f;
		if ( desiredTimeScale >= cap ) {
			desiredTimeScale = cap;
			nextSmoothTimeScaleChange = 0.0f;
		} else {
			nextSmoothTimeScaleChange = gpGlobals->time + 0.01f;
		}
	}

	// Update slowmotion meter
	if ((slowMotionUpdateTime) && (slowMotionUpdateTime <= gpGlobals->time) && !( pev->flags & FL_DIVING ) && pev->deadflag == DEAD_NO)
	{
		if ( slowMotionWasEnabled )
		{
			slowMotionUpdateTime = ( gameplayMods::superHotToggleable.isActive() ? SLOWMOTION_DRAIN_TIME * 1.33f : SLOWMOTION_DRAIN_TIME ) + gpGlobals->time;
			slowMotionCharge--;

			if ( slowMotionCharge <= 0 ) 
			{
				DeactivateSlowMotion( true );
				slowMotionCharge = 0;
			}
		}	
	}

	if ( CVAR_GET_FLOAT( "print_player_info" ) >= 1.0f ) {
		TraceResult tr;
		UTIL_MakeVectors( pev->v_angle );
		UTIL_TraceLine( pev->origin + pev->view_ofs, pev->origin + pev->view_ofs + gpGlobals->v_forward * 8192, dont_ignore_monsters, edict(), &tr );

		MESSAGE_BEGIN( MSG_ONE, gmsgOnPlyUpd, NULL, pev );
			WRITE_COORD( tr.vecEndPos.x );
			WRITE_COORD( tr.vecEndPos.y );
			WRITE_COORD( tr.vecEndPos.z );
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z );
			WRITE_COORD( pev->view_ofs.x );
			WRITE_COORD( pev->view_ofs.y );
			WRITE_COORD( pev->view_ofs.z );
			WRITE_COORD( pev->v_angle.x );
			WRITE_COORD( pev->v_angle.y );
			WRITE_COORD( pev->v_angle.z );
			WRITE_COORD( pev->velocity.x );
			WRITE_COORD( pev->velocity.y );
			WRITE_COORD( pev->velocity.z );
		MESSAGE_END();
	}

	// Show aim coordinates
	if ( CVAR_GET_FLOAT( "print_aim_coordinates" ) >= 1.0f ) {

		TraceResult tr;
		UTIL_MakeVectors( pev->v_angle );
		UTIL_TraceLine( pev->origin + pev->view_ofs, pev->origin + pev->view_ofs + gpGlobals->v_forward * 8192, dont_ignore_monsters, edict(), &tr );

		// -179 => 181
		// -1	=> 359
		float aimAngle = pev->v_angle[1];
		if ( aimAngle < 0.0f ) {
			aimAngle *= -1.0f;
			aimAngle = 180.0f + ( 180.0f - aimAngle );
		}

		MESSAGE_BEGIN( MSG_ONE, gmsgAimCoords, NULL, pev );
			WRITE_COORD( tr.vecEndPos.x );
			WRITE_COORD( tr.vecEndPos.y );
			WRITE_COORD( tr.vecEndPos.z );
			WRITE_COORD( aimAngle );
		MESSAGE_END();
	}

	// Play heartbeat sounds during slowmotion
	if ( gameplayMods::IsSlowmotionEnabled() && pev->deadflag == DEAD_NO ) {
		if ( slowMotionWasEnabled && slowMotionNextHeartbeatSound <= gpGlobals->time )
		{
			slowMotionNextHeartbeatSound = gpGlobals->time + 0.3;
			EMIT_SOUND( ENT( pev ), CHAN_AUTO, "slowmo/slowmo_heartbeat.wav", 1.0, ATTN_NORM, true );
		}
	}
	
	if ( gameplayMods::slowmotionConstant.isActive() ) {
		SetSlowMotion( true );
	}

	if ( gameplayMods::slowmotionForbidden.isActive() ) {
		slowMotionCharge = 0;
	}

	if ( gameplayMods::slowmotionInfinite.isActive() ) {
		slowMotionCharge = 100;
	} 
	

	if ( gameplayMods::painkillersInfinite.isActive() ) {
		painkillerCount = MAX_PAINKILLERS;
	} else if ( gameplayMods::noPainkillers.isActive() ) {
		painkillerCount = 0;
	}

	if ( !gameplayMods::vvvvvv.isActive() ) {
		gameplayModsData.reverseGravity = FALSE;
		pev->gravity = abs( pev->gravity );
	}

	if ( readyToComplainAboutDumbShots && !( this->pev->button & IN_ATTACK ) ) {
		char fileName[256];
		sprintf_s( fileName, "max/dumb_shoot/SHOOT_THING_%d.wav", RANDOM_LONG( 1, 22 ) );
		TryToPlayMaxCommentary( MAKE_STRING( fileName ), false );
		readyToComplainAboutDumbShots = false;
		dumbShots = 0;

		allowedToComplainAboutDumbShots = gpGlobals->time + 10.0f;
	}

	if ( allowedToSayAboutRhetoricalQuestion && gpGlobals->time > allowedToSayAboutRhetoricalQuestion ) {
		AnswerAboutRhetoricalQuestion();
	}

	if (m_iTrain & TRAIN_NEW)
	{
		ASSERT( gmsgTrain > 0 );
		// send "health" update message
		MESSAGE_BEGIN( MSG_ONE, gmsgTrain, NULL, pev );
			WRITE_BYTE(m_iTrain & 0xF);
		MESSAGE_END();

		m_iTrain &= ~TRAIN_NEW;
	}

	gameplayMods::OnFlagChange<BOOL>( gameplayModsData.lastSnarkPenguins, gameplayMods::snarkPenguins.isActive(), [this]( BOOL on ) {
		m_fKnownItem = FALSE;
	} );

	gameplayMods::OnFlagChange<BOOL>( gameplayModsData.lastTimescaleOnDamage, gameplayMods::timescaleOnDamage.isActive(), []( BOOL on ) {
		if ( !on ) {
			gameplayModsData.timescaleAdditive = 0.0f;
		}
	} );
	
	gameplayMods::OnFlagChange<float>( gameplayModsData.lastTimescaleMultiplier, *gameplayMods::timescale.isActive<float>(), [this]( float value ) {
		SetSlowMotion( slowMotionWasEnabled );
	} );

	gameplayMods::OnFlagChange<float>( gameplayModsData.lastTimescaleAdditive, gameplayModsData.timescaleAdditive, [this]( float value ) {
		SetSlowMotion( slowMotionWasEnabled );
	} );

	//
	// New Weapon?
	//
	if (!m_fKnownItem)
	{
		m_fKnownItem = TRUE;

	// WeaponInit Message
	// byte  = # of weapons
	//
	// for each weapon:
	// byte		name str length (not including null)
	// bytes... name
	// byte		Ammo Type
	// byte		Ammo2 Type
	// byte		bucket
	// byte		bucket pos
	// byte		flags	
	// ????		Icons
		
		// Send ALL the weapon info now
		int i;

		for (i = 0; i < MAX_WEAPONS; i++)
		{
			ItemInfo& II = CBasePlayerItem::ItemInfoArray[i];

			if ( !II.iId )
				continue;

			const char *pszName;
			if (!II.pszName)
				pszName = "Empty";
			else
				pszName = II.pszName;

			if ( II.iId == WEAPON_SNARK ) {
				if ( gameplayMods::snarkPenguins.isActive() ) {
					II.iFlags |= 32;
				} else {
					II.iFlags &= ~32;
				}
			}

			MESSAGE_BEGIN( MSG_ONE, gmsgWeaponList, NULL, pev );  
				WRITE_STRING(pszName);			// string	weapon name
				WRITE_BYTE(GetAmmoIndex(II.pszAmmo1));	// byte		Ammo Type
				WRITE_BYTE(II.iMaxAmmo1);				// byte     Max Ammo 1
				WRITE_BYTE(GetAmmoIndex(II.pszAmmo2));	// byte		Ammo2 Type
				WRITE_BYTE(II.iMaxAmmo2);				// byte     Max Ammo 2
				WRITE_BYTE(II.iSlot);					// byte		bucket
				WRITE_BYTE(II.iPosition);				// byte		bucket pos
				WRITE_BYTE(II.iId);						// byte		id (bit index into pev->weapons)
				WRITE_BYTE(II.iFlags);					// byte		Flags
			MESSAGE_END();
		}

		SendWeaponLockInfo();
	}


	SendAmmoUpdate();

	// Update all the items
	for ( int i = 0; i < MAX_ITEM_TYPES; i++ )
	{
		if ( m_rgpPlayerItems[i] )  // each item updates it's successors
			m_rgpPlayerItems[i]->UpdateClientData( this );
	}

	// Cache and client weapon change
	m_pClientActiveItem = m_pActiveItem;
	m_iClientFOV = m_iFOV;

	// Update Status Bar
	if ( m_flNextSBarUpdateTime < gpGlobals->time )
	{
		UpdateStatusBar();
		m_flNextSBarUpdateTime = gpGlobals->time + 0.2;
	}
}


//=========================================================
// FBecomeProne - Overridden for the player to set the proper
// physics flags when a barnacle grabs player.
//=========================================================
BOOL CBasePlayer :: FBecomeProne ( void )
{
	m_afPhysicsFlags |= PFLAG_ONBARNACLE;
	return TRUE;
}

//=========================================================
// BarnacleVictimBitten - bad name for a function that is called
// by Barnacle victims when the barnacle pulls their head
// into its mouth. For the player, just die.
//=========================================================
void CBasePlayer :: BarnacleVictimBitten ( entvars_t *pevBarnacle )
{
	TakeDamage ( pevBarnacle, pevBarnacle, pev->health + pev->armorvalue, DMG_SLASH | DMG_ALWAYSGIB );
}

//=========================================================
// BarnacleVictimReleased - overridden for player who has
// physics flags concerns. 
//=========================================================
void CBasePlayer :: BarnacleVictimReleased ( void )
{
	m_afPhysicsFlags &= ~PFLAG_ONBARNACLE;
}


//=========================================================
// Illumination 
// return player light level plus virtual muzzle flash
//=========================================================
int CBasePlayer :: Illumination( void )
{
	int iIllum = CBaseEntity::Illumination( );

	iIllum += m_iWeaponFlash;
	if (iIllum > 255)
		return 255;
	return iIllum;
}


void CBasePlayer :: EnableControl(BOOL fControl)
{
	if (!fControl)
		pev->flags |= FL_FROZEN;
	else
		pev->flags &= ~FL_FROZEN;

}


#define DOT_1DEGREE   0.9998476951564
#define DOT_2DEGREE   0.9993908270191
#define DOT_3DEGREE   0.9986295347546
#define DOT_4DEGREE   0.9975640502598
#define DOT_5DEGREE   0.9961946980917
#define DOT_6DEGREE   0.9945218953683
#define DOT_7DEGREE   0.9925461516413
#define DOT_8DEGREE   0.9902680687416
#define DOT_9DEGREE   0.9876883405951
#define DOT_10DEGREE  0.9848077530122
#define DOT_15DEGREE  0.9659258262891
#define DOT_20DEGREE  0.9396926207859
#define DOT_25DEGREE  0.9063077870367

Vector CBasePlayer::GetAimForwardWithOffset( bool degrees ) 
{
	Vector vecDirShooting;
	Vector crosshairAngle = Vector( -aimOffsetY, -aimOffsetX, 0 );
	Vector angles = pev->v_angle + crosshairAngle;

	if ( degrees ) {
		return angles;
	}

	g_engfuncs.pfnAngleVectors( angles, vecDirShooting, NULL, NULL );

	return vecDirShooting;
}

//=========================================================
// Autoaim
// set crosshair position to point to enemey
//=========================================================
Vector CBasePlayer :: GetAutoaimVector( float flDelta )
{
	Vector forward = GetAimForwardWithOffset();
	if (g_iSkillLevel == SKILL_HARD)
	{
		UTIL_MakeVectors( pev->v_angle + pev->punchangle );
		return forward;
	}

	Vector vecSrc = GetGunPosition( );
	float flDist = 8192;

	// always use non-sticky autoaim
	// UNDONE: use sever variable to chose!
	if (1 || g_iSkillLevel == SKILL_MEDIUM)
	{
		m_vecAutoAim = Vector( 0, 0, 0 );
		// flDelta *= 0.5;
	}

	BOOL m_fOldTargeting = m_fOnTarget;
	Vector angles = AutoaimDeflection(vecSrc, flDist, flDelta );

	// update ontarget if changed
	if ( !g_pGameRules->AllowAutoTargetCrosshair() )
		m_fOnTarget = 0;
	else if (m_fOldTargeting != m_fOnTarget)
	{
		m_pActiveItem->UpdateItemInfo( );
	}

	if (angles.x > 180)
		angles.x -= 360;
	if (angles.x < -180)
		angles.x += 360;
	if (angles.y > 180)
		angles.y -= 360;
	if (angles.y < -180)
		angles.y += 360;

	if (angles.x > 25)
		angles.x = 25;
	if (angles.x < -25)
		angles.x = -25;
	if (angles.y > 12)
		angles.y = 12;
	if (angles.y < -12)
		angles.y = -12;


	// always use non-sticky autoaim
	// UNDONE: use sever variable to chose!
	if (0 || g_iSkillLevel == SKILL_EASY)
	{
		m_vecAutoAim = m_vecAutoAim * 0.67 + angles * 0.33;
	}
	else
	{
		m_vecAutoAim = angles * 0.9;
	}

	// m_vecAutoAim = m_vecAutoAim * 0.99;

	// Don't send across network if sv_aim is 0
	if ( g_psv_aim->value != 0 )
	{
		if ( m_vecAutoAim.x != m_lastx ||
			 m_vecAutoAim.y != m_lasty )
		{
			SET_CROSSHAIRANGLE( edict(), -m_vecAutoAim.x, m_vecAutoAim.y );
			
			m_lastx = m_vecAutoAim.x;
			m_lasty = m_vecAutoAim.y;
		}
	}

	// ALERT( at_console, "%f %f\n", angles.x, angles.y );

	UTIL_MakeVectors( pev->v_angle + pev->punchangle + m_vecAutoAim );
	return forward;
}


Vector CBasePlayer :: AutoaimDeflection( Vector &vecSrc, float flDist, float flDelta  )
{
	edict_t		*pEdict = g_engfuncs.pfnPEntityOfEntIndex( 1 );
	CBaseEntity	*pEntity;
	float		bestdot;
	Vector		bestdir;
	edict_t		*bestent;
	TraceResult tr;

	if ( g_psv_aim->value == 0 )
	{
		m_fOnTarget = FALSE;
		return g_vecZero;
	}

	UTIL_MakeVectors( pev->v_angle + pev->punchangle + m_vecAutoAim );

	// try all possible entities
	bestdir = gpGlobals->v_forward;
	bestdot = flDelta; // +- 10 degrees
	bestent = NULL;

	m_fOnTarget = FALSE;

	UTIL_TraceLine( vecSrc, vecSrc + bestdir * flDist, dont_ignore_monsters, edict(), &tr );


	if ( tr.pHit && tr.pHit->v.takedamage != DAMAGE_NO)
	{
		// don't look through water
		if (!((pev->waterlevel != 3 && tr.pHit->v.waterlevel == 3) 
			|| (pev->waterlevel == 3 && tr.pHit->v.waterlevel == 0)))
		{
			if (tr.pHit->v.takedamage == DAMAGE_AIM)
				m_fOnTarget = TRUE;

			return m_vecAutoAim;
		}
	}

	for ( int i = 1; i < gpGlobals->maxEntities; i++, pEdict++ )
	{
		Vector center;
		Vector dir;
		float dot;

		if ( pEdict->free )	// Not in use
			continue;
		
		if (pEdict->v.takedamage != DAMAGE_AIM)
			continue;
		if (pEdict == edict())
			continue;
//		if (pev->team > 0 && pEdict->v.team == pev->team)
//			continue;	// don't aim at teammate
		if ( !g_pGameRules->ShouldAutoAim( this, pEdict ) )
			continue;

		pEntity = Instance( pEdict );
		if (pEntity == NULL)
			continue;

		if (!pEntity->IsAlive())
			continue;

		// don't look through water
		if ((pev->waterlevel != 3 && pEntity->pev->waterlevel == 3) 
			|| (pev->waterlevel == 3 && pEntity->pev->waterlevel == 0))
			continue;

		center = pEntity->BodyTarget( vecSrc );

		dir = (center - vecSrc).Normalize( );

		// make sure it's in front of the player
		if (DotProduct (dir, gpGlobals->v_forward ) < 0)
			continue;

		dot = fabs( DotProduct (dir, gpGlobals->v_right ) ) 
			+ fabs( DotProduct (dir, gpGlobals->v_up ) ) * 0.5;

		// tweek for distance
		dot *= 1.0 + 0.2 * ((center - vecSrc).Length() / flDist);

		if (dot > bestdot)
			continue;	// to far to turn

		UTIL_TraceLine( vecSrc, center, dont_ignore_monsters, edict(), &tr );
		if (tr.flFraction != 1.0 && tr.pHit != pEdict)
		{
			// ALERT( at_console, "hit %s, can't see %s\n", STRING( tr.pHit->v.classname ), STRING( pEdict->v.classname ) );
			continue;
		}

		// don't shoot at friends
		if (IRelationship( pEntity ) < 0)
		{
			if ( !pEntity->IsPlayer() && !g_pGameRules->IsDeathmatch())
				// ALERT( at_console, "friend\n");
				continue;
		}

		// can shoot at this one
		bestdot = dot;
		bestent = pEdict;
		bestdir = dir;
	}

	if (bestent)
	{
		bestdir = UTIL_VecToAngles (bestdir);
		bestdir.x = -bestdir.x;
		bestdir = bestdir - pev->v_angle - pev->punchangle;

		if (bestent->v.takedamage == DAMAGE_AIM)
			m_fOnTarget = TRUE;

		return bestdir;
	}

	return Vector( 0, 0, 0 );
}


void CBasePlayer :: ResetAutoaim( )
{
	if (m_vecAutoAim.x != 0 || m_vecAutoAim.y != 0)
	{
		m_vecAutoAim = Vector( 0, 0, 0 );
		SET_CROSSHAIRANGLE( edict(), 0, 0 );
	}
	m_fOnTarget = FALSE;
}

/*
=============
SetCustomDecalFrames

  UNDONE:  Determine real frame limit, 8 is a placeholder.
  Note:  -1 means no custom frames present.
=============
*/
void CBasePlayer :: SetCustomDecalFrames( int nFrames )
{
	if (nFrames > 0 &&
		nFrames < 8)
		m_nCustomSprayFrames = nFrames;
	else
		m_nCustomSprayFrames = -1;
}

/*
=============
GetCustomDecalFrames

  Returns the # of custom frames this player's custom clan logo contains.
=============
*/
int CBasePlayer :: GetCustomDecalFrames( void )
{
	return m_nCustomSprayFrames;
}


//=========================================================
// DropPlayerItem - drop the named item, or if no name,
// the active item. 
//=========================================================
void CBasePlayer::DropPlayerItem ( char *pszItemName )
{
	if ( !g_pGameRules->IsMultiplayer() || (weaponstay.value > 0) )
	{
		// no dropping in single player.
		return;
	}

	if ( !strlen( pszItemName ) )
	{
		// if this string has no length, the client didn't type a name!
		// assume player wants to drop the active item.
		// make the string null to make future operations in this function easier
		pszItemName = NULL;
	} 

	CBasePlayerItem *pWeapon;
	int i;

	for ( i = 0 ; i < MAX_ITEM_TYPES ; i++ )
	{
		pWeapon = m_rgpPlayerItems[ i ];

		while ( pWeapon )
		{
			if ( pszItemName )
			{
				// try to match by name. 
				if ( !strcmp( pszItemName, STRING( pWeapon->pev->classname ) ) )
				{
					// match! 
					break;
				}
			}
			else
			{
				// trying to drop active item
				if ( pWeapon == m_pActiveItem )
				{
					// active item!
					break;
				}
			}

			pWeapon = pWeapon->m_pNext; 
		}

		
		// if we land here with a valid pWeapon pointer, that's because we found the 
		// item we want to drop and hit a BREAK;  pWeapon is the item.
		if ( pWeapon )
		{
			if ( !g_pGameRules->GetNextBestWeapon( this, pWeapon ) )
				return; // can't drop the item they asked for, may be our last item or something we can't holster

			UTIL_MakeVectors ( pev->angles ); 

			pev->weapons &= ~(1<<pWeapon->m_iId);// take item off hud

			CWeaponBox *pWeaponBox = (CWeaponBox *)CBaseEntity::Create( "weaponbox", pev->origin + gpGlobals->v_forward * 10, pev->angles, edict() );
			pWeaponBox->pev->angles.x = 0;
			pWeaponBox->pev->angles.z = 0;
			pWeaponBox->PackWeapon( pWeapon );
			pWeaponBox->pev->velocity = gpGlobals->v_forward * 300 + gpGlobals->v_forward * 100;
			
			// drop half of the ammo for this weapon.
			int	iAmmoIndex;

			iAmmoIndex = GetAmmoIndex ( pWeapon->pszAmmo1() ); // ???
			
			if ( iAmmoIndex != -1 )
			{
				// this weapon weapon uses ammo, so pack an appropriate amount.
				if ( pWeapon->iFlags() & ITEM_FLAG_EXHAUSTIBLE )
				{
					// pack up all the ammo, this weapon is its own ammo type
					pWeaponBox->PackAmmo( MAKE_STRING(pWeapon->pszAmmo1()), m_rgAmmo[ iAmmoIndex ] );
					m_rgAmmo[ iAmmoIndex ] = 0; 

				}
				else
				{
					// pack half of the ammo
					pWeaponBox->PackAmmo( MAKE_STRING(pWeapon->pszAmmo1()), m_rgAmmo[ iAmmoIndex ] / 2 );
					m_rgAmmo[ iAmmoIndex ] /= 2; 
				}

			}

			return;// we're done, so stop searching with the FOR loop.
		}
	}
}

//=========================================================
// HasPlayerItem Does the player already have this item?
//=========================================================
BOOL CBasePlayer::HasPlayerItem( CBasePlayerItem *pCheckItem )
{
	CBasePlayerItem *pItem = m_rgpPlayerItems[pCheckItem->iItemSlot()];

	while (pItem)
	{
		if (FClassnameIs( pItem->pev, STRING( pCheckItem->pev->classname) ))
		{
			return TRUE;
		}
		pItem = pItem->m_pNext;
	}

	return FALSE;
}

CBasePlayerItem * CBasePlayer::GetPlayerItem( const char *pszItemName ) {

	for ( int i = 0; i < MAX_ITEM_TYPES; i++ ) {
		CBasePlayerItem *pItem = m_rgpPlayerItems[i];

		while ( pItem ) {
			if ( FStrEq( pszItemName, STRING( pItem->pev->classname ) ) ) {
				return pItem;
			}
			pItem = pItem->m_pNext;
		}

	}

	return 0;
}

//=========================================================
// HasNamedPlayerItem Does the player already have this item?
//=========================================================
BOOL CBasePlayer::HasNamedPlayerItem( const char *pszItemName, bool ignoreGungameWeapons )
{
	CBasePlayerItem *pItem = GetPlayerItem( pszItemName );
	if ( !pItem ) {
		return FALSE;
	}

	if ( ignoreGungameWeapons && pItem->isGungameWeapon ) {
		return FALSE;
	}

	return TRUE;
}

BOOL CBasePlayer::HasNamedPlayerWeaponWithAmmo( const char *pszItemName, bool ignoreGungameWeapons )
{
	CBasePlayerItem *pItem = GetPlayerItem( pszItemName );
	if ( !pItem ) {
		return FALSE;
	}

	if ( ignoreGungameWeapons && pItem->isGungameWeapon ) {
		return FALSE;
	}

	CBasePlayerWeapon *pWeapon = dynamic_cast<CBasePlayerWeapon *>( pItem );
	if ( !pWeapon ) {
		return FALSE;
	}

	return pWeapon->HasAmmo();
}

//=========================================================
// 
//=========================================================
BOOL CBasePlayer :: SwitchWeapon( CBasePlayerItem *pWeapon ) 
{
	if ( !pWeapon->CanDeploy() )
	{
		return FALSE;
	}

	if ( pWeapon->locked ) {
		return FALSE;
	}
	
	ResetAutoaim( );
	
	if (m_pActiveItem)
	{
		m_pActiveItem->Holster( );
	}

	m_pActiveItem = pWeapon;
	pWeapon->Deploy( );

	return TRUE;
}

//=========================================================
// Dead HEV suit prop
//=========================================================
class CDeadHEV : public CBaseMonster
{
public:
	void Spawn( void );
	int	Classify ( void ) { return	CLASS_HUMAN_MILITARY; }

	void KeyValue( KeyValueData *pkvd );

	int	m_iPose;// which sequence to display	-- temporary, don't need to save
	static char *m_szPoses[4];
};

char *CDeadHEV::m_szPoses[] = { "deadback", "deadsitting", "deadstomach", "deadtable" };

void CDeadHEV::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else 
		CBaseMonster::KeyValue( pkvd );
}

LINK_ENTITY_TO_CLASS( monster_hevsuit_dead, CDeadHEV );

//=========================================================
// ********** DeadHEV SPAWN **********
//=========================================================
void CDeadHEV :: Spawn( void )
{
	PRECACHE_MODEL("models/deadhaz.mdl");
	SET_MODEL(ENT(pev), "models/deadhaz.mdl");

	pev->effects		= 0;
	pev->yaw_speed		= 8;
	pev->sequence		= 0;
	pev->body			= 1;
	m_bloodColor		= BLOOD_COLOR_RED;

	pev->sequence = LookupSequence( m_szPoses[m_iPose] );

	if (pev->sequence == -1)
	{
		ALERT ( at_console, "Dead hevsuit with bad pose\n" );
		pev->sequence = 0;
		pev->effects = EF_BRIGHTFIELD;
	}

	// Corpses have less health
	pev->health			= 8;

	MonsterInitDead();
}


class CStripWeapons : public CPointEntity
{
public:
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

private:
};

LINK_ENTITY_TO_CLASS( player_weaponstrip, CStripWeapons );

void CStripWeapons :: Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBasePlayer *pPlayer = NULL;

	if ( pActivator && pActivator->IsPlayer() )
	{
		pPlayer = (CBasePlayer *)pActivator;
	}
	else if ( !g_pGameRules->IsDeathmatch() )
	{
		pPlayer = (CBasePlayer *)CBaseEntity::Instance( g_engfuncs.pfnPEntityOfEntIndex( 1 ) );
	}

	if ( pPlayer )
		pPlayer->RemoveAllItems( FALSE );
}


class CRevertSaved : public CPointEntity
{
public:
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void	EXPORT MessageThink( void );
	void	EXPORT LoadThink( void );
	void	KeyValue( KeyValueData *pkvd );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	inline	float	Duration( void ) { return pev->dmg_take; }
	inline	float	HoldTime( void ) { return pev->dmg_save; }
	inline	float	MessageTime( void ) { return m_messageTime; }
	inline	float	LoadTime( void ) { return m_loadTime; }

	inline	void	SetDuration( float duration ) { pev->dmg_take = duration; }
	inline	void	SetHoldTime( float hold ) { pev->dmg_save = hold; }
	inline	void	SetMessageTime( float time ) { m_messageTime = time; }
	inline	void	SetLoadTime( float time ) { m_loadTime = time; }

private:
	float	m_messageTime;
	float	m_loadTime;
};

LINK_ENTITY_TO_CLASS( player_loadsaved, CRevertSaved );

TYPEDESCRIPTION	CRevertSaved::m_SaveData[] = 
{
	DEFINE_FIELD( CRevertSaved, m_messageTime, FIELD_FLOAT ),	// These are not actual times, but durations, so save as floats
	DEFINE_FIELD( CRevertSaved, m_loadTime, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CRevertSaved, CPointEntity );

void CRevertSaved :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "duration"))
	{
		SetDuration( atof(pkvd->szValue) );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "holdtime"))
	{
		SetHoldTime( atof(pkvd->szValue) );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "messagetime"))
	{
		SetMessageTime( atof(pkvd->szValue) );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "loadtime"))
	{
		SetLoadTime( atof(pkvd->szValue) );
		pkvd->fHandled = TRUE;
	}
	else 
		CPointEntity::KeyValue( pkvd );
}

void CRevertSaved :: Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	UTIL_ScreenFadeAll( pev->rendercolor, Duration(), HoldTime(), pev->renderamt, FFADE_OUT );
	pev->nextthink = gpGlobals->time + MessageTime();
	SetThink( &CRevertSaved::MessageThink );
}


void CRevertSaved :: MessageThink( void )
{
	UTIL_ShowMessageAll( STRING(pev->message) );
	float nextThink = LoadTime() - MessageTime();
	if ( nextThink > 0 ) 
	{
		pev->nextthink = gpGlobals->time + nextThink;
		SetThink( &CRevertSaved::LoadThink );
	}
	else
		LoadThink();
}


void CRevertSaved :: LoadThink( void )
{
	if ( !gpGlobals->deathmatch )
	{

		if ( gameplayMods::autoSavesOnly.isActive() ) {
			SERVER_COMMAND( "reload\n" );
		} else if ( gameplayMods::noSaving.isActive() ) {
			if ( CCustomGameModeRules *cgm = dynamic_cast< CCustomGameModeRules * >( g_pGameRules ) ) {
				cgm->RestartGame();
				return;
			}
		} else {
			SERVER_COMMAND( "reload\n" );
		}
	}
}


//=========================================================
// Multiplayer intermission spots.
//=========================================================
class CInfoIntermission:public CPointEntity
{
	void Spawn( void );
	void Think( void );
};

void CInfoIntermission::Spawn( void )
{
	UTIL_SetOrigin( pev, pev->origin );
	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;
	pev->v_angle = g_vecZero;

	pev->nextthink = gpGlobals->time + 2;// let targets spawn!

}

void CInfoIntermission::Think ( void )
{
	edict_t *pTarget;

	// find my target
	pTarget = FIND_ENTITY_BY_TARGETNAME( NULL, STRING(pev->target) );

	if ( !FNullEnt(pTarget) )
	{
		pev->v_angle = UTIL_VecToAngles( (pTarget->v.origin - pev->origin).Normalize() );
		pev->v_angle.x = -pev->v_angle.x;
	}
}

LINK_ENTITY_TO_CLASS( info_intermission, CInfoIntermission );

