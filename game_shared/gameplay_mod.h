#ifndef GAMEPLAY_MOD_H
#define GAMEPLAY_MOD_H

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "argument.h"
#include <map>
#include <set>
#include <optional>

#ifdef CLIENT_DLL
#include "wrect.h"
#include "../cl_dll/cl_dll.h"
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif // CLIENT_DLL

#include <vector>

enum class BulletPhysicsMode {
	Disabled,
	ForEnemiesDuringSlowmotion,
	ForEnemiesAndPlayerDuringSlowmotion,
	Enabled
};

struct BleedingInfo {
	int handicap = 20;
	float updatePeriod = 0.2f;
	float immunityPeriod = 10.0f;

	BleedingInfo( const std::vector<Argument> &args ) {
		handicap = args.at( 0 ).number;
		updatePeriod = args.at( 1 ).number;
		immunityPeriod = args.at( 2 ).number;
	}
};

struct BulletRicochetInfo {
	int count = 0;
	float maxDotProduct = 0.0f;
	int error = 0; // TODO

	BulletRicochetInfo( const std::vector<Argument> &args ) {
		count = args.at( 0 ).number;
		maxDotProduct = args.at( 1 ).number / 90.0f;
	}
};

struct DrunkAimInfo {
	float maxHorizontalWobble = 0.0f;
	float maxVerticalWobble = 0.0f;
	float wobbleFrequency = 0.0f;

	DrunkAimInfo( const std::vector<Argument> &args ) {
		maxHorizontalWobble = args.at( 0 ).number;
		maxVerticalWobble = args.at( 1 ).number;
		wobbleFrequency = args.at( 2 ).number;
	}
};

struct DrunkFOVInfo {
	float offsetAmplitude = 0.0f;
	float offsetFrequency = 0.0f;

	DrunkFOVInfo( const std::vector<Argument> &args ) {
		offsetAmplitude = args.at( 0 ).number;
		offsetFrequency = args.at( 1 ).number;
	}
};

struct FadeOutInfo {
	float threshold = 0.0f;
	float updatePeriod = 0.0f;

	FadeOutInfo( const std::vector<Argument> &args ) {
		threshold = 255 - ( args.at( 0 ).number / 100.0f ) * 255;
		updatePeriod = args.at( 1 ).number;
	}
};

struct HealthRegenerationInfo {
	int max = 20;
	float delay = 3.0f;
	float frequency = 0.2f;

	HealthRegenerationInfo( const std::vector<Argument> &args ) {
		max = args.at( 0 ).number;
		delay = args.at( 1 ).number;
		frequency = args.at( 2 ).number;
	}
};

struct RandomGameplayModsInfo {
	float timeForRandomGameplayMod = 10.0f;
	float timeUntilNextRandomGameplayMod = 10.0f;
	float timeForRandomGameplayModVoting = 10.0f;

	RandomGameplayModsInfo( const std::vector<Argument> &args ) {
		timeForRandomGameplayMod = args.at( 0 ).number;
		timeUntilNextRandomGameplayMod = args.at( 1 ).number;
		timeForRandomGameplayModVoting = min( timeUntilNextRandomGameplayMod, args.at( 2 ).number );
	}
};

struct GunGameInfo {
	int killsRequired = 1;
	float changeTime = 0.0f;
	bool isSequential = false;

	GunGameInfo( const std::vector<Argument> &args ) {
		killsRequired = args.at( 0 ).number;
		changeTime = max( 0, args.at( 1 ).number );
		isSequential = args.at( 2 ).string == "sequential";
	}
};

struct QuakeRocketsInfo {
	float delay = 0.8f;
	float speed = 1500.0f;

	QuakeRocketsInfo( const std::vector<Argument> &args ) {
		delay = args.at( 0 ).number;
		speed = args.at( 1 ).number;
	}
};

class GameplayMod;

struct ProposedGameplayMod {
	GameplayMod *mod = NULL;
	std::vector<Argument> args;
	std::set<std::string> votes;
	float voteDistributionPercent = 0.0f;
};

struct ProposedRandomGameplayModVoter {
	int alpha;
	std::string name;
};

struct ProposedGameplayModClient {
	GameplayMod *mod = NULL;
	int votes = 0;
	float voteDistributionPercent = 0.0f;
	std::list<ProposedRandomGameplayModVoter> voters;
};

struct TimedGameplayMod {
	GameplayMod *mod = NULL;
	std::vector<Argument> args;
	float time;
	float initialTime;
};

enum GAME_MODE {
	GAME_MODE_VANILLA,
	GAME_MODE_CUSTOM
};

// ISSUE: Player instance still relies on itself in some cases, like when managin superhot
// No idea what should manage these, it's always possible to include Think function here for that too.
class GameplayModData
{
public:
	std::vector<TYPEDESCRIPTION> fields;
	template<typename T> T _Field( int offset, char *fieldName, T default, FIELDTYPE type ) {
		this->fields.push_back(
			{ type, fieldName, offset, 1, 0 }
		);
		return default;
	}
#define Field( ctype, name, default, type, ... ) ctype name = _Field( offsetof( GameplayModData, name ), #name, default, type, ##__VA_ARGS__ )
#define FieldBool( name, default ) Field( BOOL, name, default, FIELD_BOOLEAN )
#define FieldInt( name, default ) Field( int, name, default, FIELD_INTEGER )
#define FieldFloat( name, default ) Field( float, name, default, FIELD_FLOAT )
#define FieldTime( name, default ) Field( float, name, default, FIELD_TIME )

	bool addedAdditionalFields = false;

	Field( GAME_MODE, activeGameMode, GAME_MODE_VANILLA, FIELD_INTEGER );
	char activeGameModeConfig[256];
	char activeGameModeConfigHash[128];

	FieldBool( forceDisconnect, FALSE );

	FieldInt( gungameKillsLeft, 0 );
	char gungameWeapon[128];
	char gungamePriorWeapon[128];
	FieldFloat( gungameTimeLeftUntilNextWeapon, 0.0f );
	FieldInt( gungameSeed, 0 );
	FieldInt( gungameWeaponSelection, 0 );
	
	FieldInt( randomSpawnerSeed, 0 );
	FieldInt( randomSpawnerCalls, 0 );

	FieldInt( fade, 255 );
	
	FieldFloat( timescaleAdditive, 0.0f );
	
	FieldBool( holdingTwinWeapons, FALSE );

	FieldBool( reverseGravity, FALSE );

	FieldBool( slowmotionInfiniteCheat, FALSE );

	FieldBool( lastGodConstant, FALSE );
	FieldBool( lastInvisibleEnemies, FALSE );
	FieldBool( lastNoTargetConstant, FALSE );
	FieldBool( lastPayned, FALSE );
	FieldBool( lastSuperHotConstant, FALSE );
	FieldBool( lastSnarkPenguins, FALSE );
	FieldFloat( lastTimescaleMultiplier, 1.0f );
	FieldBool( lastTimescaleOnDamage, FALSE );
	FieldFloat( lastTimescaleAdditive, 0.0f );

	// Statistics and game state
	FieldBool( cheated, FALSE );
	FieldBool( usedCheat, FALSE );

	float timeLeftUntilNextRandomGameplayMod = 10.0f;
	
	FieldFloat( time, 0.0f );
	FieldFloat( realTime, 0.0f );
	FieldFloat( lastGlobalTime, 0.0f );
	FieldFloat( lastRealTime, 0.0f );

	FieldInt( score, 0 );
	FieldInt( comboMultiplier, 1 );
	FieldFloat( comboMultiplierReset, 0.0f );

	int monsterSpawnAttempts = 0;
	
	FieldBool( timerPaused, FALSE );

	FieldInt( kills, 0 );
	FieldInt( headshotKills, 0 );
	FieldInt( explosiveKills, 0 );
	FieldInt( crowbarKills, 0 );
	FieldInt( projectileKills, 0 );
	FieldFloat( secondsInSlowmotion, 0.0f );

	char endConditionsHashes[64][128];
	int endConditionsActivationCounts[64];

	void Init();

#ifndef CLIENT_DLL
	void SendToClient();
	int Save( CSave &save );
	int Restore( CRestore &restore );

	static void ToggleForceEnabledGameplayMod( const std::string &mod );
	static void ToggleForceDisabledGameplayMod( const std::string &mod );
#endif // CLIENT_DLL

	void AddArrayFieldDefinitions();

	static void Reset();

};

extern GameplayModData gameplayModsData;

#ifndef CLIENT_DLL
extern TYPEDESCRIPTION gameplayModsDataFields[];
#endif // !CLIENT_DLL


class GameplayMod {

	using IsAlsoActiveWhenFunction = std::function<std::optional<std::string>()>;
	IsAlsoActiveWhenFunction isAlsoActiveWhen = [] { return std::nullopt; };
	
	using EventInitFunction = std::function<std::pair<std::string, std::string>()>;

public:
	std::string id;
	std::string name;
	std::string randomGameplayModName;
	std::string description;
	std::string randomGameplayModDescription;
	std::vector<Argument> arguments;
	std::string forcedDefaultArguments;
	std::function<bool()> CanBeActivatedRandomly = [] { return true; };
	bool canBeCancelledAfterChangeLevel = false;
	bool isEvent = false;
	EventInitFunction EventInit = [] { return std::make_pair( "", "" ); };

	GameplayMod( const std::string &id, const std::string &name ) : id( id ), name( name ) {}

	static GameplayMod& Define( const std::string &id, const std::string &name );

	GameplayMod& Description( const std::string &description );
	GameplayMod& RandomGameplayModName( const std::string &name );
	GameplayMod& RandomGameplayModDescription( const std::string &description );
	GameplayMod& IsAlsoActiveWhen( const IsAlsoActiveWhenFunction &func );
	GameplayMod& Arguments( const std::vector<Argument> &args );
	GameplayMod& CannotBeActivatedRandomly();
	GameplayMod& CanOnlyBeActivatedRandomlyWhen( const std::function<bool()> &randomActivateCondition );
	GameplayMod& CanBeCancelledAfterChangeLevel();
	GameplayMod& ForceDefaultArguments( const std::string &arguments );
	GameplayMod& OnEventInit( const EventInitFunction &func );

	std::optional<std::vector<Argument>> getActiveArguments( bool discountForcedArguments = false );
	bool isActive( bool discountForcedArguments = false );
	std::vector<Argument> ParseStringArguments( const std::vector<std::string> &argsParsed );
	std::vector<Argument> ParseStringArguments( const std::string &args );
	std::vector<Argument> GetRandomArguments();

	std::optional<std::vector<Argument>> GetArgumentsFromCustomGameplayModeConfig();

	inline std::string GetRandomGameplayModName() {
		return randomGameplayModName.empty() ? name : randomGameplayModName;
	}

	inline std::string GetRandomGameplayModDescription() {
		return randomGameplayModDescription.empty() ? description : randomGameplayModDescription;
	}
	
	template <typename T>
	std::optional<T> isActive( bool discountForcedArguments = false ) {
		auto args = getActiveArguments( discountForcedArguments );

		if constexpr ( std::is_same<T, int>::value || std::is_same<T, float>::value ) {
			return args ? T( args->at( 0 ).number ) : std::optional<T> {};
		} else if constexpr ( std::is_same<T, std::string>::value ) {
			return args ? args->at( 0 ).string : std::optional<std::string> {};
		} else if constexpr ( std::is_same<T, BulletPhysicsMode>::value ) {
			auto mode = args->at( 0 ).string;
			if ( mode == "disabled" ) {
				return BulletPhysicsMode::Disabled;
			} else if ( mode == "for_enemies_and_player_on_slowmotion" ) {
				return BulletPhysicsMode::ForEnemiesAndPlayerDuringSlowmotion;
			} else if ( mode == "constant" ) {
				return BulletPhysicsMode::Enabled;
			}

			return BulletPhysicsMode::ForEnemiesDuringSlowmotion;
		} else if constexpr ( std::is_same<T, std::vector<Argument>>::value ) {
			return args;
		} else {
			return args ? T( *args ) : std::optional<T> {};
		}
	}
};

namespace gameplayMods {

	std::optional<std::pair<GameplayMod *, std::vector<Argument>>> GetModAndParseArguments( const std::string &line );

	extern std::map<std::string, GameplayMod *> byString;
	extern std::set<GameplayMod *> allowedForRandom;
	extern std::set<GameplayMod *> previouslyProposedRandomMods;

	extern std::map<GameplayMod *, std::vector<Argument>> forceEnabledMods;
	extern std::set<GameplayMod *> forceDisabledMods;
	extern std::vector<ProposedGameplayMod> proposedGameplayMods;
	extern std::vector<ProposedGameplayModClient> proposedGameplayModsClient;
	extern std::vector<TimedGameplayMod> timedGameplayMods;

	extern GameplayMod& accuracy;
	extern GameplayMod& accuracyLow;
	extern GameplayMod& accuracyHigh;

	extern GameplayMod& autoSavesOnly;

	extern GameplayMod& blackMesaMinute;
	
	extern GameplayMod& bleeding;

	extern GameplayMod& bulletDelayOnSlowmotion;
	extern GameplayMod& bulletPhysics;
	extern GameplayMod& bulletRicochet;
	extern GameplayMod& bulletSelfHarm;
	extern GameplayMod& bulletTrail;

	extern GameplayMod& chaosEdition;

	extern GameplayMod& cncSounds;

	extern GameplayMod& crossbowExplosiveBolts;

	extern GameplayMod& damageMultiplier;
	extern GameplayMod& damageMultiplierFromPlayer;

	extern GameplayMod& deusExSounds;
	
	extern GameplayMod& difficultyEasy;
	extern GameplayMod& difficultyHard;
	
	extern GameplayMod& divingAllowedWithoutSlowmotion;
	extern GameplayMod& divingOnly;

	extern GameplayMod& drunkAim;
	extern GameplayMod& drunkFOV;
	extern GameplayMod& drunkLook;
	
	extern GameplayMod& explosionJumping;
	
	extern GameplayMod& fadingOut;

	extern GameplayMod& frictionOverride;

	extern GameplayMod& godConstant;
	
	extern GameplayMod& gungame;

	extern GameplayMod& gaussFastCharge;
	extern GameplayMod& gaussJumping;
	extern GameplayMod& gaussNoSelfGauss;

	extern GameplayMod& gibsAlways;
	extern GameplayMod& gibsEdible;
	extern GameplayMod& gibsGarbage;
	
	extern GameplayMod& gravity;

	extern GameplayMod& grenadePellets;
	
	extern GameplayMod& headshots;

	extern GameplayMod& healthRegeneration;
	extern GameplayMod& healOnKill;
	
	extern GameplayMod& infiniteAmmo;
	extern GameplayMod& infiniteAmmoClip;
	
	extern GameplayMod& initialClipAmmo;
	extern GameplayMod& initialHealth;

	extern GameplayMod& instagib;
	
	extern GameplayMod& inverseControls;

	extern GameplayMod& invisibility;

	extern GameplayMod& kerotanDetector;

	extern GameplayMod& modsDoubleTime;

	extern GameplayMod& noFallDamage;
	extern GameplayMod& noGameTitle;
	extern GameplayMod& noJumping;
	extern GameplayMod& noHealing;
	extern GameplayMod& noMapMusic;
	extern GameplayMod& noPainkillers;
	extern GameplayMod& noSaving;
	extern GameplayMod& noSecondaryAttack;
	extern GameplayMod& noSmgGrenadePickup;
	extern GameplayMod& noStartDark;
	extern GameplayMod& noTargetConstant;
	extern GameplayMod& noWalking;
	
	extern GameplayMod& oneHitKO;
	extern GameplayMod& oneHitKOFromPlayer;

	extern GameplayMod& painkillersInfinite;
	extern GameplayMod& painkillersSlow;
	
	extern GameplayMod& payned;
	extern GameplayMod& paynedSounds;
	extern GameplayMod& paynedSoundsHumans;
	extern GameplayMod& paynedSoundsMonsters;
	
	extern GameplayMod& preventMonsterDrops;
	extern GameplayMod& preventMonsterMovement;
	extern GameplayMod& preventMonsterSpawn;
	
	extern GameplayMod& preserveNightmare;

	extern GameplayMod& quakeRockets;

	extern GameplayMod& randomGameplayMods;
	extern GameplayMod& randomSpawnerSeed;

	extern GameplayMod& scoreAttack;

	extern GameplayMod& shotgunAutomatic;
	
	extern GameplayMod& shootUnderwater;

	extern GameplayMod& slowmotionConstant;
	extern GameplayMod& slowmotionFastWalk;
	extern GameplayMod& slowmotionForbidden;
	extern GameplayMod& slowmotionInfinite;
	extern GameplayMod& slowmotionInitialCharge;
	extern GameplayMod& slowmotionOnDamage;
	extern GameplayMod& slowmotionOnlyDiving;
	
	extern GameplayMod& snarkFriendlyToAllies;
	extern GameplayMod& snarkFriendlyToPlayer;
	extern GameplayMod& snarkFromExplosion;
	extern GameplayMod& snarkInception;
	extern GameplayMod& snarkInfestation;
	extern GameplayMod& snarkNuclear;
	extern GameplayMod& snarkPenguins;
	extern GameplayMod& snarkStayAlive;
	
	extern GameplayMod& swearOnKill;
	
	extern GameplayMod& superHot;
	extern GameplayMod& superHotToggleable;
	
	extern GameplayMod& teleportMaintainVelocity;
	extern GameplayMod& teleportOnKill;
	
	extern GameplayMod& timeRestriction;
	extern GameplayMod& timerShown;
	extern GameplayMod& timerShownReal;

	extern GameplayMod& timescale;
	extern GameplayMod& timescaleOnDamage;
	
	extern GameplayMod& tripminesDetachable;
	
	extern GameplayMod& upsideDown;
	
	extern GameplayMod& vvvvvv;

	extern GameplayMod& weaponImpact;
	extern GameplayMod& weaponPushBack;
	extern GameplayMod& weaponRestricted;
	
	extern GameplayMod& eventGiveRandomWeapon;
	extern GameplayMod& eventGivePainkillers;
	extern GameplayMod& eventModPack;
	extern GameplayMod& eventSpawnRandomMonsters;

	bool PlayerShouldProducePhysicalBullets();
	bool IsSlowmotionEnabled();
	bool AllowedToVoteOnRandomGameplayMods();
	bool PaynedSoundsEnabled( bool isMonster );

	std::set<GameplayMod *> GetFilteredRandomGameplayMods();

	template<class T = bool>
	void OnFlagChange( T &flag, T newValue, const std::function<void( T )> &func ) {
		if ( flag != newValue ) {
			func( newValue );
		}

		flag = newValue;
	}
};

#endif // GAMEPLAY_MOD_H