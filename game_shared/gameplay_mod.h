#ifndef GAMEPLAY_MOD_H
#define GAMEPLAY_MOD_H

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "argument.h"
#include <map>
#include <set>

#ifdef CLIENT_DLL
#include "wrect.h"
#include "../cl_dll/cl_dll.h"
#define min(a,b)            (((a) < (b)) ? (a) : (b))
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

enum GAMEPLAY_MOD {
	GAMEPLAY_MOD_ALWAYS_GIB,
	GAMEPLAY_MOD_AUTOSAVES_ONLY,
	GAMEPLAY_MOD_BLACK_MESA_MINUTE,
	GAMEPLAY_MOD_BLEEDING,
	GAMEPLAY_MOD_BULLET_DELAY_ON_SLOWMOTION,
	GAMEPLAY_MOD_BULLET_PHYSICS_CONSTANT,
	GAMEPLAY_MOD_BULLET_PHYSICS_DISABLED,
	GAMEPLAY_MOD_BULLET_PHYSICS_ENEMIES_AND_PLAYER_ON_SLOWMOTION,
	GAMEPLAY_MOD_BULLET_RICOCHET,
	GAMEPLAY_MOD_BULLET_SELF_HARM,
	GAMEPLAY_MOD_BULLET_TRAIL_CONSTANT,
	GAMEPLAY_MOD_CONSTANT_SLOWMOTION,
	GAMEPLAY_MOD_CROSSBOW_EXPLOSIVE_BOLTS,
	GAMEPLAY_MOD_DETACHABLE_TRIPMINES,
	GAMEPLAY_MOD_DIVING_ALLOWED_WITHOUT_SLOWMOTION,
	GAMEPLAY_MOD_DIVING_ONLY,
	GAMEPLAY_MOD_DRUNK_AIM,
	GAMEPLAY_MOD_DRUNK_LOOK,
	GAMEPLAY_MOD_EASY,
	GAMEPLAY_MOD_EDIBLE_GIBS,
	GAMEPLAY_MOD_EMPTY_SLOWMOTION,
	GAMEPLAY_MOD_FADING_OUT,
	GAMEPLAY_MOD_FRICTION,
	GAMEPLAY_MOD_GARBAGE_GIBS,
	GAMEPLAY_MOD_GOD,
	GAMEPLAY_MOD_HARD,
	GAMEPLAY_MOD_HEADSHOTS,
	GAMEPLAY_MOD_HEAL_ON_KILL,
	GAMEPLAY_MOD_HEALTH_REGENERATION,
	GAMEPLAY_MOD_INFINITE_AMMO,
	GAMEPLAY_MOD_INFINITE_AMMO_CLIP,
	GAMEPLAY_MOD_INFINITE_PAINKILLERS,
	GAMEPLAY_MOD_INFINITE_SLOWMOTION,
	GAMEPLAY_MOD_INITIAL_CLIP_AMMO,
	GAMEPLAY_MOD_INSTAGIB,
	GAMEPLAY_MOD_NO_FALL_DAMAGE,
	GAMEPLAY_MOD_NO_HEALING,
	GAMEPLAY_MOD_NO_JUMPING,
	GAMEPLAY_MOD_NO_MAP_MUSIC,
	GAMEPLAY_MOD_NO_PILLS,
	GAMEPLAY_MOD_NO_SAVING,
	GAMEPLAY_MOD_NO_SECONDARY_ATTACK,
	GAMEPLAY_MOD_NO_SELF_GAUSS,
	GAMEPLAY_MOD_NO_SLOWMOTION,
	GAMEPLAY_MOD_NO_SMG_GRENADE_PICKUP,
	GAMEPLAY_MOD_NO_TARGET,
	GAMEPLAY_MOD_NO_WALKING,
	GAMEPLAY_MOD_ONE_HIT_KO,
	GAMEPLAY_MOD_ONE_HIT_KO_FROM_PLAYER,
	GAMEPLAY_MOD_PREVENT_MONSTER_DROPS,
	GAMEPLAY_MOD_PREVENT_MONSTER_MOVEMENT,
	GAMEPLAY_MOD_PREVENT_MONSTER_SPAWN,
	GAMEPLAY_MOD_PREVENT_MONSTER_STUCK_EFFECT,
	GAMEPLAY_MOD_RANDOM_GAMEPLAY_MODS,
	GAMEPLAY_MOD_SCORE_ATTACK,
	GAMEPLAY_MOD_SHOTGUN_AUTOMATIC,
	GAMEPLAY_MOD_SHOW_TIMER,
	GAMEPLAY_MOD_SHOW_TIMER_REAL_TIME,
	GAMEPLAY_MOD_SLOWMOTION_FAST_WALK,
	GAMEPLAY_MOD_SLOWMOTION_ON_DAMAGE,
	GAMEPLAY_MOD_SLOWMOTION_ONLY_DIVING,
	GAMEPLAY_MOD_SLOW_PAINKILLERS,
	GAMEPLAY_MOD_SNARK_FRIENDLY_TO_ALLIES,
	GAMEPLAY_MOD_SNARK_FRIENDLY_TO_PLAYER,
	GAMEPLAY_MOD_SNARK_FROM_EXPLOSION,
	GAMEPLAY_MOD_SNARK_INCEPTION,
	GAMEPLAY_MOD_SNARK_INFESTATION,
	GAMEPLAY_MOD_SNARK_NUCLEAR,
	GAMEPLAY_MOD_SNARK_PARANOIA,
	GAMEPLAY_MOD_SNARK_PENGUINS,
	GAMEPLAY_MOD_SNARK_STAY_ALIVE,
	GAMEPLAY_MOD_STARTING_HEALTH,
	GAMEPLAY_MOD_SUPERHOT,
	GAMEPLAY_MOD_SWEAR_ON_KILL,
	GAMEPLAY_MOD_UPSIDE_DOWN,
	GAMEPLAY_MOD_TELEPORT_MAINTAIN_VELOCITY,
	GAMEPLAY_MOD_TELEPORT_ON_KILL,
	GAMEPLAY_MOD_TIME_RESTRICTION,
	GAMEPLAY_MOD_TOTALLY_SPIES,
	GAMEPLAY_MOD_VVVVVV,
	GAMEPLAY_MOD_WEAPON_IMPACT,
	GAMEPLAY_MOD_WEAPON_PUSH_BACK,
	GAMEPLAY_MOD_WEAPON_RESTRICTED,

	GAMEPLAY_MOD_EVENT_GIVE_RANDOM_WEAPON,
	GAMEPLAY_MOD_EVENT_SPAWN_RANDOM_MONSTERS,
};

struct GameplayMod {
	std::string id;
	std::string name;
	std::string description;

	std::vector<Argument> arguments;

	bool cannotBeActivatedRandomly;
	bool canBeDeactivated;
	bool isEvent;
	bool canBeCancelledAfterChangeLevel;
	std::function<void( CBasePlayer * )> Deactivate;
	std::function<bool( CBasePlayer * )> CanBeActivatedRandomly;

	std::set<std::string> votes;
	float voteDistributionPercent;

	GameplayMod() {};
	GameplayMod( const std::string &id, const std::string &name ) :
		id( id ), name( name ),
		description( "" ), arguments( std::vector<Argument>() ),

		cannotBeActivatedRandomly( false ),
		init( []( CBasePlayer *, const std::vector<Argument> & ) -> std::pair<std::string, std::string> {
			return { "", "" };
		} ),

		canBeDeactivated( false ),
		isEvent( false ),
		canBeCancelledAfterChangeLevel( false ),
		Deactivate( []( CBasePlayer * ) {} ),
		CanBeActivatedRandomly( []( CBasePlayer * ) { return true; } ),
		voteDistributionPercent( 0.0f )
	{
	}

	std::pair<std::string, std::string> Init( CBasePlayer *player ) {
		return this->init( player, this->arguments );
	}

	void Init( CBasePlayer *player, const std::vector<std::string> &stringArgs ) {
		for ( size_t i = 0; i < min( this->arguments.size(), stringArgs.size() ); i++ ) {
			auto parsingError = this->arguments.at( i ).Init( stringArgs.at( i ) );

			if ( !parsingError.empty() ) {
				ALERT( at_notice, "%s, argument %d: %s", this->name.c_str(), i, parsingError.c_str() );
				return;
			}
		}

		this->init( player, this->arguments );
	}

	GameplayMod &Description( const std::string &description ) {
		this->description = description;
		return *this;
	}

	GameplayMod &Arguments( const std::vector<Argument> &args ) {
		this->arguments = args;
		return *this;
	}

	GameplayMod &Toggles( BOOL *flag ) {
		this->OnInit( [flag]( CBasePlayer *, const std::vector<Argument> & ) {
			*flag = TRUE;
		} );
		this->OnDeactivation( [flag]( CBasePlayer * ) {
			*flag = FALSE;
		} );
		return *this;
	}
	
	GameplayMod &Toggles( const std::vector<BOOL *> flags ) {
		this->OnInit( [flags]( CBasePlayer *, const std::vector<Argument> & ) {
			for ( auto flag : flags ) {
				*flag = TRUE;
			}
		} );
		this->OnDeactivation( [flags]( CBasePlayer * ) {
			for ( auto flag : flags ) {
				*flag = FALSE;
			}
		} );
		return *this;
	}

	GameplayMod &OnInit( const std::function<void( CBasePlayer*, const std::vector<Argument> & )> &init ) {
		this->init = [init]( CBasePlayer *player, const std::vector<Argument> &args ) -> std::pair<std::string, std::string> {
			init( player, args );
			return { "", "" };
		};
		return *this;
	}

	GameplayMod &OnEventInit( const std::function<std::pair<std::string, std::string>( CBasePlayer*, const std::vector<Argument> & )> &init ) {
		this->init = init;
		this->isEvent = true;
		return *this;
	}

	GameplayMod &OnDeactivation( const std::function<void( CBasePlayer* )> &deactivate ) {
		this->Deactivate = deactivate;
		this->canBeDeactivated = true;
		return *this;
	}

	GameplayMod &CannotBeActivatedRandomly() {
		this->cannotBeActivatedRandomly = true;
		return *this;
	}

	GameplayMod &CanOnlyBeActivatedRandomlyWhen( const std::function<bool( CBasePlayer* )> &randomActivateCondition ) {
		this->CanBeActivatedRandomly = randomActivateCondition;
		return *this;
	}

	GameplayMod &CanBeCancelledAfterChangeLevel() {
		this->canBeCancelledAfterChangeLevel = true;
		return *this;
	}

	private:
		std::function<std::pair<std::string, std::string>( CBasePlayer*, const std::vector<Argument> & )> init;
};

// ISSUE: Player instance still relies on itself in some cases, like when managin superhot or aimOffset
// No idea what should manage these, it's always possible to include Think function here for that too.

class GameplayMods
{
public:
	std::vector<TYPEDESCRIPTION> fields;
	template<typename T> T _Field( int offset, char *fieldName, T default, FIELDTYPE type ) {
		this->fields.push_back(
			{ type, fieldName, offset, 1, 0 }
		);
		return default;
	}
#define Field( ctype, name, default, type, ... ) ctype name = _Field( offsetof( GameplayMods, name ), #name, default, type, ##__VA_ARGS__ )
#define FieldBool( name, default ) Field( BOOL, name, default, FIELD_BOOLEAN )
#define FieldInt( name, default ) Field( int, name, default, FIELD_INTEGER )
#define FieldFloat( name, default ) Field( float, name, default, FIELD_FLOAT )
#define FieldTime( name, default ) Field( float, name, default, FIELD_TIME )

	bool addedAdditionalFields = false;

	Field( GAME_MODE, activeGameMode, GAME_MODE_VANILLA, FIELD_INTEGER );
	char activeGameModeConfig[256];
	char activeGameModeConfigHash[128];

	FieldFloat( aimOffsetMaxX, 0.0f );
	FieldFloat( aimOffsetMaxY, 0.0f );
	FieldFloat( aimOffsetChangeFreqency, 0.0f );

	FieldBool( automaticShotgun, FALSE );
	FieldBool( autosavesOnly, FALSE );

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
	FieldBool( noSelfGauss, FALSE );
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

	FieldBool( randomGameplayMods, FALSE );
	FieldFloat( timeForRandomGameplayMod, 10.0f );
	FieldFloat( timeUntilNextRandomGameplayMod, 10.0f );
	FieldFloat( timeForRandomGameplayModVoting, 10.0f );
	float timeLeftUntilNextRandomGameplayMod = 10.0f;
	std::vector<GameplayMod> proposedGameplayMods;
	
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

	bool AllowedToVoteOnRandomGameplayMods();

	void AddArrayFieldDefinitions();
	void SetGameplayModActiveByString( const std::string &line, bool isActive = false );
	std::vector<GameplayMod> GetRandomGameplayMod( CBasePlayer *player, size_t modAmount, std::function<bool( const GameplayMod &mod )> filter = []( const GameplayMod & ){ return true; } );
	std::map<GAMEPLAY_MOD, GameplayMod> FilterGameplayMods( std::map<GAMEPLAY_MOD, GameplayMod> mods, CBasePlayer *player, std::function<bool( const GameplayMod &mod )> filter = []( const GameplayMod & ) { return true; } );
	GameplayMod GetRandomGameplayMod( CBasePlayer *player );

	std::vector<std::pair<GameplayMod, float>> timedGameplayMods;

	static void Reset();
};

extern GameplayMods gameplayMods;
extern std::map<GAMEPLAY_MOD, GameplayMod> gameplayModDefs;

#ifndef CLIENT_DLL
extern TYPEDESCRIPTION gameplayModsSaveData[];
#endif // !CLIENT_DLL

#endif // GAMEPLAY_MOD_H