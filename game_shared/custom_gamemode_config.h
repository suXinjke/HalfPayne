#ifndef CUSTOM_GAMEMODE_CONFIG_H
#define CUSTOM_GAMEMODE_CONFIG_H

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "string_aux.h"
#include <string>
#include <vector>
#include <set>
#include <map>
#include <functional>
#include "custom_gamemode_record.h"

#define CHANGE_LEVEL_MODEL_INDEX -1

static char *allowedItems[] = {
	"all",
	"item_suit",
	"item_longjump",
	"item_battery",
	"item_healthkit",
	"weapon_crowbar",
	"weapon_9mmhandgun",
	"weapon_9mmhandgun_twin",
	"ammo_9mmclip",
	"weapon_shotgun",
	"ammo_buckshot",
	"weapon_9mmAR",
	"ammo_9mmAR",
	"ammo_ARgrenades",
	"weapon_handgrenade",
	"weapon_tripmine",
	"weapon_357",
	"ammo_357",
	"weapon_crossbow",
	"ammo_crossbow",
	"weapon_egon",
	"weapon_gauss",
	"ammo_gaussclip",
	"weapon_rpg",
	"ammo_rpgclip",
	"weapon_satchel",
	"weapon_snark",
	"weapon_hornetgun",
	"weapon_ingram",
	"weapon_ingram_twin",
};

static char *allowedEntities[] {
	"item_suit",
	"item_longjump",
	"item_battery",
	"item_healthkit",
	"weapon_crowbar",
	"weapon_9mmhandgun",
	"ammo_9mmclip",
	"weapon_shotgun",
	"ammo_buckshot",
	"weapon_9mmAR",
	"ammo_9mmAR",
	"ammo_ARgrenades",
	"weapon_handgrenade",
	"weapon_tripmine",
	"weapon_357",
	"ammo_357",
	"weapon_crossbow",
	"ammo_crossbow",
	"weapon_egon",
	"weapon_gauss",
	"ammo_gaussclip",
	"weapon_rpg",
	"ammo_rpgclip",
	"weapon_satchel",
	"weapon_snark",
	"weapon_hornetgun",
	"weapon_ingram",

	"monster_alien_controller",
	"monster_alien_grunt",
	"monster_alien_slave",
	"monster_barnacle",
	"monster_bullchicken",
	"monster_gargantua",
	"monster_headcrab",
	"monster_houndeye",
	"monster_human_assassin",
	"monster_human_grunt",
	"monster_human_grunt_shotgun",
	"monster_human_grunt_grenade_launcher",
	"monster_ichthyosaur",
	"monster_miniturret",
	"monster_sentry",
	"monster_snark",
	"monster_tripmine",
	"monster_zombie",

	"end_marker"
};

struct Hookable {
	std::string map = "";
	int modelIndex = -3;
	std::string targetName = "";
	bool constant = false;

	virtual bool Fits( int modelIndex, const std::string &className, const std::string &targetName, bool firstTime ) const {
		return (
			( STRING( gpGlobals->mapname ) == this->map || this->map == "everywhere" ) &&
			( ( modelIndex == this->modelIndex ) || ( !this->targetName.empty() && ( this->targetName == targetName || this->targetName == className ) ) ) &&
			( constant || ( !constant && firstTime ) )
		);
	}

	virtual void Reset() {
		map = "";
		modelIndex = -3;
		targetName = "";
		constant = false;
	}
};

struct HookableWithDelay : Hookable {
	float delay = 0.0f;

	void Reset() override {
		Hookable::Reset();
		delay = 0.0f;
	}
};

struct HookableWithTarget : Hookable {
	bool isModelIndex;
	std::string entityName;

	CBaseEntity* EdictFitsTarget( CBasePlayer *pPlayer, edict_t *edict ) const {
		if ( !edict ) {
			return NULL;
		}

		if ( CBaseEntity *entity = CBaseEntity::Instance( edict ) ) {
			if (
				( isModelIndex && entity->pev->modelindex > 0 && ( std::to_string( entity->pev->modelindex ) == entityName ) ) ||
				( !isModelIndex && ( ( STRING( entity->pev->targetname ) == entityName ) || ( STRING( entity->pev->classname ) == entityName ) ) )
			) {
				return entity;
			}
		}

		return NULL;
	}
};

struct EntitySpawnData {
	std::string name;
	std::string targetName;
	float		x;
	float		y;
	float		z;
	float		angle;
	int spawnFlags = 0;
	int weaponFlags = 0;
};

struct EntitySpawn : Hookable {
	EntitySpawnData entity;
};

struct EndCondition : Hookable {
	int activations;
	int activationsRequired;

	std::string objective;
};

struct StartPosition {
	bool defined;
	float x;
	float y;
	float z;
	float angle;
	bool stripped;
};

struct LoadoutItem {
	std::string name = "";
	int amount = 1;
};

struct Sound : Hookable {

	std::string path;
	float delay = 0.01;
	float initialPos = 0.0f;
	bool looping = false;
	bool noSlowmotionEffects = false;
};

struct Intermission : HookableWithTarget {
	StartPosition startPos;
};

struct Teleport : Hookable {
	StartPosition pos;
};

struct EntityRandomSpawner
{
	std::string mapName;
	EntitySpawnData entity;
	int maxAmount;
	float spawnPeriod;
};

enum GAMEPLAY_MOD {
	GAMEPLAY_MOD_UNKNOWN,
	GAMEPLAY_MOD_ALWAYS_GIB,
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
	GAMEPLAY_MOD_TIME_RESTRICTION,
	GAMEPLAY_MOD_TOTALLY_SPIES,
	GAMEPLAY_MOD_VVVVVV,
	GAMEPLAY_MOD_WEAPON_IMPACT,
	GAMEPLAY_MOD_WEAPON_PUSH_BACK,
	GAMEPLAY_MOD_WEAPON_RESTRICTED,
};

enum CONFIG_FILE_SECTION {
	CONFIG_FILE_SECTION_NO_SECTION,

	CONFIG_FILE_SECTION_NAME,
	CONFIG_FILE_SECTION_DESCRIPTION,
	CONFIG_FILE_SECTION_START_MAP,
	CONFIG_FILE_SECTION_START_POSITION,
	CONFIG_FILE_SECTION_END_MAP,
	CONFIG_FILE_SECTION_END_TRIGGER,
	CONFIG_FILE_SECTION_CHANGE_LEVEL_PREVENT,
	CONFIG_FILE_SECTION_INTERMISSION,
	CONFIG_FILE_SECTION_LOADOUT,
	CONFIG_FILE_SECTION_ENTITY_SPAWN,
	CONFIG_FILE_SECTION_ENTITY_USE,
	CONFIG_FILE_SECTION_ENTITY_PREVENT,
	CONFIG_FILE_SECTION_ENTITY_REMOVE,
	CONFIG_FILE_SECTION_ENTITY_RANDOM_SPAWNER,
	CONFIG_FILE_SECTION_SOUND,
	CONFIG_FILE_SECTION_MUSIC,
	CONFIG_FILE_SECTION_PLAYLIST,
	CONFIG_FILE_SECTION_MAX_COMMENTARY,
	CONFIG_FILE_SECTION_MODS,
	CONFIG_FILE_SECTION_END_CONDITIONS,
	CONFIG_FILE_SECTION_TIMER_PAUSE,
	CONFIG_FILE_SECTION_TIMER_RESUME,
	CONFIG_FILE_SECTION_TELEPORT,

	CONFIG_FILE_SECTION_AUX_END
};

enum CONFIG_TYPE {
	CONFIG_TYPE_MAP,
	CONFIG_TYPE_CGM,
	CONFIG_TYPE_BMM,
	CONFIG_TYPE_SAGM
};

struct GameplayMod
{
	GAMEPLAY_MOD id;
	std::string name;
	std::string description;
	std::vector<std::string> argDescriptions;
	std::function<void(CBasePlayer*)> init;

	GameplayMod() {
		this->id = GAMEPLAY_MOD_UNKNOWN;
		this->name = "UNKNOWN";
		this->description = "This mod has not been accounted and you should never see this message";
		this->init = []( CBasePlayer * ){};
	}

	GameplayMod(
		GAMEPLAY_MOD id,
		const std::string &name,
		const std::string description,
		std::function<void(CBasePlayer*)> initFunction,
		std::vector<std::string> argDescriptions = {}
	) :
		id( id ), name( name ), description( description ), init( initFunction ), argDescriptions( argDescriptions )
	{}
};

class CustomGameModeConfig {

public:

	struct ConfigSectionData
	{
		std::string line;
		std::vector<std::string> argsString;
		std::vector<float> argsFloat;
	};

	struct ConfigSection
	{
		typedef std::function<std::string( ConfigSectionData &data )> ConfigSectionValidateFunction;

		bool single;
		std::string name;
		std::vector<ConfigSectionData> data;
		ConfigSectionValidateFunction Validate;

		ConfigSection();
		ConfigSection( const std::string &sectionName, bool single, ConfigSectionValidateFunction validateFunction );

		const std::string OnSectionData( const std::string &configName, const std::string &line, int lineCount, CONFIG_TYPE configType );
	};
	
	CustomGameModeConfig( CONFIG_TYPE configType );
	CustomGameModeRecord record;

	bool ReadFile( const char *fileName );

	void Reset();
	bool OnNewSection( std::string sectionName );
	void OnError( std::string );

	void InitConfigSections();

	const std::string GetHash();

	static std::string ConfigTypeToDirectoryName( CONFIG_TYPE configType );
	static std::string ConfigTypeToGameModeCommand( CONFIG_TYPE configType );
	static std::string ConfigTypeToGameModeName( CONFIG_TYPE configType, bool uppercase = false );

	std::vector<std::string> GetAllConfigFileNames();
	std::vector<std::string> GetAllFileNames( const char *path, const char *extension = "*", bool includeExtension = false );
	std::vector<std::string> GetAllFileNames( const char *path, const std::vector<std::string> &extensions, bool includeExtension = false );

	static std::string GetGamePath();
	static int GetAllowedItemIndex( const char *allowedItem );
	static int GetAllowedEntityIndex( const char *allowedEntity );

	std::vector<std::string> configNameSeparated;
	std::string configName;
	std::string error;
	std::string sha1;
	CONFIG_TYPE configType;
	bool gameFinishedOnce;

	bool		markedForRestart;
	
	std::map< CONFIG_FILE_SECTION, ConfigSection> configSections;

	bool MarkModelIndex( CONFIG_FILE_SECTION fileSection, const std::string &mapName, int modelIndex, const std::string &targetName, bool *isConstant = NULL, ConfigSectionData *outData = NULL );

	std::set<std::string> GetSoundsToPrecacheForMap( const std::string &map );
	std::set<std::string> GetEntitiesToPrecacheForMap( const std::string &map );

	std::string ValidateModelIndexSectionData( ConfigSectionData &data );

	std::vector<GameplayMod> mods;
	bool IsGameplayModActive( GAMEPLAY_MOD mod );
	bool AddGameplayMod( ConfigSectionData &modName );

	bool musicPlaylistShuffle;
	
	bool hasEndMarkers;

	void FillHookable( Hookable &hookable, const ConfigSectionData &data );
	void FillHookableWithDelay( HookableWithDelay &hookableWithDelays, const ConfigSectionData &data );
	void FillHookableWithTarget( HookableWithTarget &hookableWithTarget, const ConfigSectionData &data );
	void FillHookableSound( Sound &hookableSound, const ConfigSectionData &data );
	void FillEntitySpawn( EntitySpawn &entitySpawn, const ConfigSectionData &data );
	void FillEntitySpawnDataFlags( EntitySpawnData &entitySpawn );

	std::string name;
	std::string description;
	std::string startMap;
	std::string endMap;

	StartPosition startPosition;
	Hookable endTrigger;
	std::set<std::string> forbiddenLevels;
	std::vector<LoadoutItem> loadout;
	std::vector<EntitySpawn> entitySpawns;
	std::vector<HookableWithTarget> entityUses;
	std::vector<Hookable> entitiesPrevented;

	std::vector<Sound> sounds;
	std::vector<Sound> maxCommentary;
	std::vector<Sound> music;
	std::vector<Sound> musicPlaylist;
	std::vector<Intermission> intermissions;
	std::vector<Teleport> teleports;

	std::vector<Hookable> timerPauses;
	std::vector<Hookable> timerResumes;
	std::vector<EntityRandomSpawner> entityRandomSpawners;
	std::vector<EndCondition> endConditions;
	std::vector<HookableWithTarget> entitiesToRemove;

protected:
	std::string folderPath;
	std::string configFolderPath;
	CONFIG_FILE_SECTION currentFileSection;

};

#endif // CUSTOM_GAMEMODE_CONFIG_H