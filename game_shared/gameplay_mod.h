#ifndef GAMEPLAY_MOD_H
#define GAMEPLAY_MOD_H

#include "extdll.h"
#include "util.h"
#include "cbase.h"

#ifdef CLIENT_DLL
#include "wrect.h"
#include "../cl_dll/cl_dll.h"
#endif // CLIENT_DLL

#include <vector>

enum BULLET_PHYSICS_MODE {
	BULLET_PHYSICS_DISABLED,
	BULLET_PHYSICS_ENEMIES_ONLY_ON_SLOWMOTION,
	BULLET_PHYSICS_ENEMIES_AND_PLAYER_ON_SLOWMOTION,
	BULLET_PHYSICS_CONSTANT
};

enum GAME_MODE {
	GAME_MODE_VANILLA,
	GAME_MODE_CUSTOM
};

// ISSUE: Player instance still relies on itself in some cases, like when managin superhot or aimOffset
// No idea what should manage these, it's always possible to include Think function here for that too.

class GameplayMods
{
public:
	std::vector<TYPEDESCRIPTION> fields;
	template<typename T> T _Field( int offset, T default, FIELDTYPE type, short count = 1 ) {
		std::string fieldName = "gameplayModsField" + std::to_string( fields.size() + 1 );

		this->fields.push_back(
			{ type, _strdup( fieldName.c_str() ), offset, count, 0 }
		);
		return default;
	}
#define Field( ctype, name, default, type, ... ) ctype name = _Field( offsetof( GameplayMods, name ), default, type, ##__VA_ARGS__ )
#define FieldBool( name, default ) Field( BOOL, name, default, FIELD_BOOLEAN )
#define FieldInt( name, default ) Field( int, name, default, FIELD_INTEGER )
#define FieldFloat( name, default ) Field( float, name, default, FIELD_FLOAT )
#define FieldTime( name, default ) Field( float, name, default, FIELD_TIME )

	Field( GAME_MODE, activeGameMode, GAME_MODE_VANILLA, FIELD_INTEGER );
	char activeGameModeConfig[256];
	char activeGameModeConfigHash[128];

	FieldFloat( aimOffsetMaxX, 0.0f );
	FieldFloat( aimOffsetMaxY, 0.0f );
	FieldFloat( aimOffsetChangeFreqency, 0.0f );

	FieldBool( automaticShotgun, FALSE );

	FieldBool( bleeding, FALSE );
	FieldFloat( bleedingUpdatePeriod, 1.0f );
	FieldFloat( bleedingHandicap, 20.0f );
	FieldFloat( bleedingImmunityPeriod, 10.0f );

	FieldBool( bulletDelayOnSlowmotion, FALSE );
	FieldInt( bulletRicochetCount, 0 );
	FieldInt( bulletRicochetError, 5 );
	FieldFloat( bulletRicochetMaxDotProduct, 0.5 );
	FieldBool( bulletPhysical, FALSE );
	Field( BULLET_PHYSICS_MODE, bulletPhysicsMode, BULLET_PHYSICS_ENEMIES_ONLY_ON_SLOWMOTION, FIELD_INTEGER );
	FieldBool( bulletSelfHarm, FALSE );
	FieldBool( bulletTrailConstant, FALSE );

	FieldBool( crossbowExplosiveBolts, FALSE );

	FieldBool( detachableTripmines, FALSE );
	FieldBool( detachableTripminesInstantly, FALSE );

	FieldBool( divingOnly, FALSE );
	FieldBool( divingAllowedWithoutSlowmotion, FALSE );

	FieldInt( drunkiness, 0 );

	FieldBool( gibsAlways, FALSE );
	FieldBool( gibsEdible, FALSE );
	FieldBool( gibsGarbage, FALSE );

	FieldInt( fade, 255 );
	FieldBool( fadeOut, FALSE );
	FieldInt( fadeOutThreshold, 25 );
	FieldFloat( fadeOutUpdatePeriod, 0.5f );

	FieldFloat( frictionOverride, -1.0f );

	FieldBool( godConstant, FALSE );

	FieldBool( healOnKill, FALSE );
	FieldFloat( healOnKillMultiplier, 0.25f );
	
	FieldBool( infiniteAmmo, FALSE );
	FieldBool( infiniteAmmoClip, FALSE );
	FieldInt( initialClipAmmo, 0 );

	FieldBool( instaGib, FALSE );

	FieldBool( noFallDamage, FALSE );
	FieldBool( noJumping, FALSE );
	FieldBool( noMapMusic, FALSE );
	FieldBool( noHealing, FALSE );
	FieldBool( noSaving, FALSE );
	FieldBool( noSecondaryAttack, FALSE );
	FieldBool( noSmgGrenadePickup, FALSE );
	FieldBool( noTargetConstant, FALSE );
	FieldBool( noWalking, FALSE );

	FieldBool( oneHitKO, FALSE );
	FieldBool( oneHitKOFromPlayer, FALSE );

	FieldBool( preventMonsterDrops, FALSE );
	FieldBool( preventMonsterMovement, FALSE );
	FieldBool( preventMonsterSpawn, FALSE );
	FieldBool( preventMonsterStuckEffect, FALSE );

	FieldFloat( regenerationMax, 20.0f );
	FieldFloat( regenerationDelay, 3.0f );
	FieldFloat( regenerationFrequency, 0.2f );
	FieldBool( reverseGravity, FALSE );

	FieldBool( painkillersForbidden, FALSE );
	FieldBool( painkillersInfinite, FALSE );
	FieldBool( painkillersSlow, FALSE );

	FieldBool( slowmotionConstant, FALSE );
	FieldBool( slowmotionFastWalk, FALSE );
	FieldBool( slowmotionForbidden, FALSE );
	FieldBool( slowmotionInfinite, FALSE );
	FieldBool( slowmotionOnDamage, FALSE );
	FieldBool( slowmotionOnlyDiving, FALSE );

	FieldBool( snarkFriendlyToAllies, FALSE );
	FieldBool( snarkFriendlyToPlayer, FALSE );
	FieldBool( snarkFromExplosion, FALSE );
	FieldBool( snarkInception, FALSE );
	FieldInt( snarkInceptionDepth, 10 );
	FieldBool( snarkInfestation, FALSE );
	FieldBool( snarkNuclear, FALSE );
	FieldBool( snarkPenguins, FALSE );
	FieldBool( snarkStayAlive, FALSE );

	FieldBool( superHot, FALSE );
	FieldBool( swearOnKill, FALSE );

	FieldBool( teleportMaintainVelocity, FALSE );
	FieldBool( teleportOnKill, FALSE );
	char teleportOnKillWeapon[64];
	
	FieldBool( totallySpies, FALSE );

	FieldBool( upsideDown, FALSE );
	FieldBool( vvvvvv, FALSE );

	FieldFloat( weaponImpact, 0.0f );
	FieldBool( weaponPushBack, FALSE );
	FieldFloat( weaponPushBackMultiplier, 1.0f );
	FieldBool( weaponRestricted, FALSE );

	// Statistics and game state
	FieldBool( cheated, FALSE );
	FieldBool( usedCheat, FALSE );

	FieldBool( blackMesaMinute, FALSE );
	FieldBool( scoreAttack, FALSE );
	
	FieldFloat( time, 0.0f );
	FieldFloat( realTime, 0.0f );
	FieldFloat( lastGlobalTime, 0.0f );
	FieldFloat( lastRealTime, 0.0f );

	FieldInt( score, 0 );
	FieldInt( comboMultiplier, 1 );
	FieldTime( comboMultiplierReset, 0.0f );
	
	FieldBool( timerShown, FALSE );
	FieldBool( timerBackwards, FALSE );
	FieldBool( timerPaused, FALSE );
	FieldBool( timerShowReal, FALSE );

	FieldInt( kills, 0 );
	FieldInt( headshotKills, 0 );
	FieldInt( explosiveKills, 0 );
	FieldInt( crowbarKills, 0 );
	FieldInt( projectileKills, 0 );
	FieldFloat( secondsInSlowmotion, 0.0f );

	void Init();

#ifndef CLIENT_DLL
	void SendToClient();
	int Save( CSave &save );
	int Restore( CRestore &restore );
#endif // CLIENT_DLL

	static void Reset();
};

extern GameplayMods gameplayMods;

#ifndef CLIENT_DLL
extern TYPEDESCRIPTION gameplayModsSaveData[];
#endif // !CLIENT_DLL

#endif // GAMEPLAY_MOD_H