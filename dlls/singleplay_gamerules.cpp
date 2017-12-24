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
//
// teamplay_gamerules.cpp
//
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"player.h"
#include	"weapons.h"
#include	"gamerules.h"
#include	"items.h"
#include	"cgm_gamerules.h"
#include	"monsters.h"
#include	"triggers.h"

extern DLL_GLOBAL CGameRules	*g_pGameRules;
extern DLL_GLOBAL BOOL	g_fGameOver;
extern int gmsgDeathMsg;	// client dll messages
extern int gmsgScoreInfo;
extern int gmsgMOTD;

extern int gmsgOnSound;
int	gmsgEndCredits	= 0;
int gmsgOnModelIdx  = 0;

//=========================================================
//=========================================================
CHalfLifeRules::CHalfLifeRules( void ) : mapConfig( CONFIG_TYPE_MAP )
{
	configs.push_back( &mapConfig );

	if ( !gmsgEndCredits ) {
		gmsgEndCredits = REG_USER_MSG( "EndCredits", 0 );
		gmsgOnModelIdx = REG_USER_MSG( "OnModelIdx", -1 );
	}

	ended = false;
	isSpawning = false;

	if ( !mapConfig.ReadFile( STRING( gpGlobals->mapname ) ) ) {
		g_engfuncs.pfnServerPrint( mapConfig.error.c_str() );
	}

	RefreshSkillData();

	lastSkill = CVAR_GET_FLOAT( "skill" );
}

bool CHalfLifeRules::EntityShouldBePrevented( edict_t *entity )
{
	int modelIndex = entity->v.modelindex;
	std::string targetName = STRING( entity->v.targetname );
	std::string className = STRING( entity->v.classname );

	for ( const auto &config : configs ) {
		for ( const auto &entityPrevent : config->entitiesPrevented ) {
			if ( entityPrevent.Fits( modelIndex, className, targetName, true ) ) {
				return true;
			}
		}
	}

	return false;
}

void CHalfLifeRules::End( CBasePlayer *pPlayer )
{
	if ( ended ) {
		return;
	}

	if ( pPlayer->slowMotionEnabled ) {
		pPlayer->ToggleSlowMotion();
	}

	ended = true;

	pPlayer->pev->movetype = MOVETYPE_NONE;
	pPlayer->pev->flags |= FL_NOTARGET | FL_GODMODE;
	pPlayer->RemoveAllItems( true );

	OnEnd( pPlayer );
}

void CHalfLifeRules::OnEnd( CBasePlayer *pPlayer )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgEndCredits, NULL, pPlayer->pev );
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_ALL, gmsgOnSound );
		WRITE_STRING( "HP_CREDITS" );
		WRITE_BYTE( true );
		WRITE_COORD( 0 );
		WRITE_COORD( 0 );
		WRITE_COORD( 0 );
	MESSAGE_END();

	pPlayer->SendPlayMusicMessage( "./half_payne/sound/music/credits.mp3", 0.0f, 0.0f, TRUE );
}

void CHalfLifeRules::OnChangeLevel()
{
	if ( !mapConfig.ReadFile( STRING( gpGlobals->mapname ) ) ) {
		g_engfuncs.pfnServerPrint( mapConfig.error.c_str() );
	}
	tasks.push_back( { 0.0f, [this]( CBasePlayer *pPlayer ) {
		HookModelIndex( NULL );
	} } );
}

void CHalfLifeRules::OnKilledEntityByPlayer( CBasePlayer * pPlayer, CBaseEntity * victim, KILLED_ENTITY_TYPE killedEntity, BOOL isHeadshot, BOOL killedByExplosion, BOOL killedByCrowbar ) {
	HookModelIndex( victim->edict() );
}

void CHalfLifeRules::HookModelIndex( edict_t *activator ) {
	int modelIndex = activator ? activator->v.modelindex : -1;
	std::string className = activator ? STRING( activator->v.classname ) : "";
	std::string targetName = activator ? STRING( activator->v.targetname ) : "on_map_start";

	CBaseEntity *entity = activator ? CBaseEntity::Instance( activator ) : NULL;

	HookModelIndex( entity, modelIndex, className, targetName );
}

void CHalfLifeRules::HookModelIndex( CBaseEntity *activator, int modelIndex, const std::string &className, const std::string &targetName ) {
	CBasePlayer *pPlayer = dynamic_cast<CBasePlayer *>( CBaseEntity::Instance( g_engfuncs.pfnPEntityOfEntIndex( 1 ) ) );
	if ( !pPlayer ) {
		return;
	}

	const float print_model_indexes = CVAR_GET_FLOAT( "print_model_indexes" );
	if ( print_model_indexes >= 2.0f ) {
		MESSAGE_BEGIN( MSG_ONE, gmsgOnModelIdx, NULL, pPlayer->pev );
			WRITE_STRING( STRING( gpGlobals->mapname ) );
			WRITE_LONG( modelIndex );
			WRITE_STRING( targetName.c_str() );
			WRITE_STRING( className.c_str() );
		MESSAGE_END();
	} else if ( print_model_indexes >= 1.0f ) {
		char message[128];
		sprintf( message, "[%s] Hooked model index: %d; target name: %s; class name: %s\n", STRING( gpGlobals->mapname ), modelIndex, targetName.c_str(), className.c_str() );
		g_engfuncs.pfnServerPrint( message );
	}

	std::string key = std::string( STRING( gpGlobals->mapname ) ) + "_" + std::to_string( modelIndex ) + "_" + className + "_" + targetName;
	bool firstTime = !pPlayer->ModelIndexHasBeenHooked( key.c_str() );
	if ( firstTime ) {
		pPlayer->RememberHookedModelIndex( ALLOC_STRING( key.c_str() ) ); // sorry for memory leak
	}

	OnHookedModelIndex( pPlayer, activator, modelIndex, className, targetName, firstTime );
}

extern int gEvilImpulse101;
void CHalfLifeRules::OnHookedModelIndex( CBasePlayer *pPlayer, CBaseEntity *activator, int modelIndex, const std::string &className, const std::string &targetName, bool firstTime )
{
	bool noPlaylists = true;
	for ( const auto &config : configs ) {
		if ( !config->musicPlaylist.empty() ) {
			noPlaylists = false;
			break;
		}
	}

	for ( const auto &config : configs ) {

		for ( const auto &sound : config->sounds ) {
			if ( sound.Fits( modelIndex, className, targetName, firstTime ) ) {
				pPlayer->AddToSoundQueue( ALLOC_STRING( sound.path.c_str() ), sound.delay, false, true );
			}
		}

		for ( const auto &commentary : config->maxCommentary ) {
			if ( commentary.Fits( modelIndex, className, targetName, firstTime ) ) {
				if ( !( gEvilImpulse101 && className.find( "weapon_" ) == 0 ) ) {
					pPlayer->AddToSoundQueue( ALLOC_STRING( commentary.path.c_str() ), commentary.delay, true, true );
				}
			}
		}

		if ( noPlaylists ) {
			for ( const auto &music : config->music ) {
				if ( music.Fits( modelIndex, className, targetName, firstTime ) ) {
					if ( isSpawning || std::string( CVAR_GET_STRING( "sm_current_file" ) ) != music.path ) {
						pPlayer->PlayMusicDelayed( music.path, music.delay, music.initialPos, music.looping, music.noSlowmotionEffects );
					}
				}
			}
		}

		for ( const auto &entityUse : config->entityUses ) {
			if ( !entityUse.Fits( modelIndex, className, targetName, firstTime ) ) {
				continue;
			}

			for ( int i = 0 ; i < 1024 ; i++ ) {
				edict_t *edict = g_engfuncs.pfnPEntityOfEntIndex( i );
				if ( CBaseEntity *entity = entityUse.EdictFitsTarget( pPlayer, edict ) ) {
					if ( CBaseTrigger *trigger = dynamic_cast<CBaseTrigger *>( entity ) ) {
						trigger->ActivateMultiTrigger( pPlayer );
					} else {
						entity->Use( pPlayer, pPlayer, USE_SET, 1 );
					}
				}
			}
		}

		for ( const auto &entityRemove : config->entitiesToRemove ) {
			if ( !entityRemove.Fits( modelIndex, className, targetName, firstTime ) ) {
				continue;
			}

			for ( int i = 0 ; i < 1024 ; i++ ) {
				edict_t *edict = g_engfuncs.pfnPEntityOfEntIndex( i );
				if ( CBaseEntity *entity = entityRemove.EdictFitsTarget( pPlayer, edict ) ) {
					entity->pev->flags |= FL_KILLME;
				}
			}
		}

		for ( const auto &entitySpawn : config->entitySpawns ) {
			if ( entitySpawn.Fits( modelIndex, className, targetName, firstTime ) ) {
				CBaseEntity *entity = CBaseEntity::Create(
					allowedEntities[CustomGameModeConfig::GetAllowedEntityIndex( entitySpawn.entity.name.c_str() )],
					Vector( entitySpawn.entity.x, entitySpawn.entity.y, entitySpawn.entity.z ),
					Vector( 0, entitySpawn.entity.angle, 0 ),
					NULL,
					entitySpawn.entity.weaponFlags,
					entitySpawn.entity.spawnFlags
				);

				entity->pev->spawnflags |= SF_MONSTER_PRESERVE;
				if ( entitySpawn.entity.targetName.size() > 0 ) {
					entity->pev->targetname = ALLOC_STRING( entitySpawn.entity.targetName.c_str() ); // memory leak
				}
			}
		}
	}
}
void CHalfLifeRules::Precache()
{
	std::string mapname = STRING( gpGlobals->mapname );
	for ( const auto &config : configs ) {

		auto soundsToPrecache = config->GetSoundsToPrecacheForMap( mapname );
		auto entitiesToPrecache = config->GetEntitiesToPrecacheForMap( mapname );

		// I'm very sorry for this memory leak for now
		for ( const auto &sound : soundsToPrecache ) {
			PRECACHE_SOUND( ( char * ) STRING( ALLOC_STRING( sound.c_str() ) ) );
		}

		for ( const auto &spawn : entitiesToPrecache ) {
			UTIL_PrecacheOther( spawn.c_str() );
		}
	}
}

//=========================================================
//=========================================================
void CHalfLifeRules::Think ( void )
{
}

//=========================================================
//=========================================================
BOOL CHalfLifeRules::IsMultiplayer( void )
{
	return FALSE;
}

//=========================================================
//=========================================================
BOOL CHalfLifeRules::IsDeathmatch ( void )
{
	return FALSE;
}

//=========================================================
//=========================================================
BOOL CHalfLifeRules::IsCoOp( void )
{
	return FALSE;
}


//=========================================================
//=========================================================
BOOL CHalfLifeRules::FShouldSwitchWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon )
{
	if ( !pPlayer->m_pActiveItem )
	{
		// player doesn't have an active item!
		return TRUE;
	}

	if ( CVAR_GET_FLOAT( "hud_autoswitch" ) == 0.0f ) {
		return false;
	}

	if ( !pPlayer->m_pActiveItem->CanHolster() )
	{
		return FALSE;
	}

	return TRUE;
}

//=========================================================
//=========================================================
BOOL CHalfLifeRules :: GetNextBestWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pCurrentWeapon )
{
	return FALSE;
}

//=========================================================
//=========================================================
BOOL CHalfLifeRules :: ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[ 128 ] )
{

	if ( CBasePlayer *player = dynamic_cast< CBasePlayer * >( CBasePlayer::Instance( pEntity ) ) ) {
		if ( player->noSaving ) {
			g_engfuncs.pfnServerPrint( "You're not allowed to load this savefile.\n" );
			return FALSE;
		}
	}
	
	return TRUE;
}

void CHalfLifeRules :: InitHUD( CBasePlayer *pl )
{
}

//=========================================================
//=========================================================
void CHalfLifeRules :: ClientDisconnected( edict_t *pClient )
{
}

//=========================================================
//=========================================================
float CHalfLifeRules::FlPlayerFallDamage( CBasePlayer *pPlayer )
{
	// subtract off the speed at which a player is allowed to fall without being hurt,
	// so damage will be based on speed beyond that, not the entire fall
	pPlayer->m_flFallVelocity -= PLAYER_MAX_SAFE_FALL_SPEED;
	return pPlayer->m_flFallVelocity * DAMAGE_FOR_FALL_SPEED;
}

//=========================================================
//=========================================================
void CHalfLifeRules :: PlayerSpawn( CBasePlayer *pPlayer )
{
	isSpawning = true;
	HookModelIndex( NULL );
	isSpawning = false;
}

//=========================================================
//=========================================================
BOOL CHalfLifeRules :: AllowAutoTargetCrosshair( void )
{
	return ( g_iSkillLevel == SKILL_EASY );
}

//=========================================================
//=========================================================
void CHalfLifeRules :: PlayerThink( CBasePlayer *pPlayer )
{
	if ( pPlayer->activeGameMode == GAME_MODE_VANILLA ) {
		int currentSkill = CVAR_GET_FLOAT( "skill" );
		if ( currentSkill != lastSkill ) {
			RefreshSkillData();
			lastSkill = currentSkill;
		}
	}

	std::vector<Task> postponedTasks;

	while ( !tasks.empty() ) {
		const auto &task = tasks.front();
		if ( gpGlobals->time >= task.delay ) {
			task.task( pPlayer );
		} else {
			postponedTasks.push_back( task );
		}

		tasks.pop_front();
	}

	for ( const auto &task : postponedTasks ) {
		tasks.push_back( task );
	}
}


//=========================================================
//=========================================================
BOOL CHalfLifeRules :: FPlayerCanRespawn( CBasePlayer *pPlayer )
{
	return TRUE;
}

//=========================================================
//=========================================================
float CHalfLifeRules :: FlPlayerSpawnTime( CBasePlayer *pPlayer )
{
	return gpGlobals->time;//now!
}

//=========================================================
// IPointsForKill - how many points awarded to anyone
// that kills this player?
//=========================================================
int CHalfLifeRules :: IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled )
{
	return 1;
}

//=========================================================
// PlayerKilled - someone/something killed this player
//=========================================================
void CHalfLifeRules :: PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor )
{
}

//=========================================================
// Deathnotice
//=========================================================
void CHalfLifeRules::DeathNotice( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor )
{
}

//=========================================================
// PlayerGotWeapon - player has grabbed a weapon that was
// sitting in the world
//=========================================================
void CHalfLifeRules :: PlayerGotWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon )
{
	HookModelIndex( pWeapon->edict() );
}

//=========================================================
// FlWeaponRespawnTime - what is the time in the future
// at which this weapon may spawn?
//=========================================================
float CHalfLifeRules :: FlWeaponRespawnTime( CBasePlayerItem *pWeapon )
{
	return -1;
}

//=========================================================
// FlWeaponRespawnTime - Returns 0 if the weapon can respawn 
// now,  otherwise it returns the time at which it can try
// to spawn again.
//=========================================================
float CHalfLifeRules :: FlWeaponTryRespawn( CBasePlayerItem *pWeapon )
{
	return 0;
}

//=========================================================
// VecWeaponRespawnSpot - where should this weapon spawn?
// Some game variations may choose to randomize spawn locations
//=========================================================
Vector CHalfLifeRules :: VecWeaponRespawnSpot( CBasePlayerItem *pWeapon )
{
	return pWeapon->pev->origin;
}

//=========================================================
// WeaponShouldRespawn - any conditions inhibiting the
// respawning of this weapon?
//=========================================================
int CHalfLifeRules :: WeaponShouldRespawn( CBasePlayerItem *pWeapon )
{
	return GR_WEAPON_RESPAWN_NO;
}

//=========================================================
//=========================================================
BOOL CHalfLifeRules::CanHaveItem( CBasePlayer *pPlayer, CItem *pItem )
{
	return TRUE;
}

//=========================================================
//=========================================================
void CHalfLifeRules::PlayerGotItem( CBasePlayer *pPlayer, CItem *pItem )
{
	HookModelIndex( pItem->edict() );
}

//=========================================================
//=========================================================
int CHalfLifeRules::ItemShouldRespawn( CItem *pItem )
{
	return GR_ITEM_RESPAWN_NO;
}


//=========================================================
// At what time in the future may this Item respawn?
//=========================================================
float CHalfLifeRules::FlItemRespawnTime( CItem *pItem )
{
	return -1;
}

//=========================================================
// Where should this item respawn?
// Some game variations may choose to randomize spawn locations
//=========================================================
Vector CHalfLifeRules::VecItemRespawnSpot( CItem *pItem )
{
	return pItem->pev->origin;
}

//=========================================================
//=========================================================
BOOL CHalfLifeRules::IsAllowedToSpawn( CBaseEntity *pEntity )
{
	return TRUE;
}

//=========================================================
//=========================================================
void CHalfLifeRules::PlayerGotAmmo( CBasePlayer *pPlayer, CBasePlayerAmmo *pAmmo )
{
	HookModelIndex( pAmmo->edict() );
}

//=========================================================
//=========================================================
int CHalfLifeRules::AmmoShouldRespawn( CBasePlayerAmmo *pAmmo )
{
	return GR_AMMO_RESPAWN_NO;
}

//=========================================================
//=========================================================
float CHalfLifeRules::FlAmmoRespawnTime( CBasePlayerAmmo *pAmmo )
{
	return -1;
}

//=========================================================
//=========================================================
Vector CHalfLifeRules::VecAmmoRespawnSpot( CBasePlayerAmmo *pAmmo )
{
	return pAmmo->pev->origin;
}

//=========================================================
//=========================================================
float CHalfLifeRules::FlHealthChargerRechargeTime( void )
{
	return 0;// don't recharge
}

//=========================================================
//=========================================================
int CHalfLifeRules::DeadPlayerWeapons( CBasePlayer *pPlayer )
{
	return GR_PLR_DROP_GUN_NO;
}

//=========================================================
//=========================================================
int CHalfLifeRules::DeadPlayerAmmo( CBasePlayer *pPlayer )
{
	return GR_PLR_DROP_AMMO_NO;
}

//=========================================================
//=========================================================
int CHalfLifeRules::PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget )
{
	// why would a single player in half life need this? 
	return GR_NOTTEAMMATE;
}

//=========================================================
//=========================================================
BOOL CHalfLifeRules :: FAllowMonsters( void )
{
	return TRUE;
}