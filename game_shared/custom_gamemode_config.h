#ifndef CUSTOM_GAMEMODE_CONFIG_H
#define CUSTOM_GAMEMODE_CONFIG_H

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include <string>
#include <vector>
#include <set>
#include <functional>

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
	"weapon_hornetgun"
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

	"monster_alien_controller",
	"monster_alien_grunt",
	"monster_alien_slave",
	"monster_bullchicken",
	"monster_gargantua",
	"monster_headcrab",
	"monster_houndeye",
	"monster_human_assassin",
	"monster_human_grunt",
	"monster_ichthyosaur",
	"monster_miniturret",
	"monster_sentry",
	"monster_snark",
	"monster_zombie",

};

class ModelIndex
{

public:
	std::string key;

	std::string mapName;
	int			modelIndex;
	std::string targetName;
	bool		constant;

	ModelIndex() {
		this->mapName = "";
		this->modelIndex = -2;
		this->targetName = "";
		this->constant = false;

		this->key = mapName + std::to_string( modelIndex );
	}

	ModelIndex( const std::string &mapName, int modelIndex, const std::string &targetName, bool constant = false ) {
		this->mapName = mapName;
		this->modelIndex = modelIndex;
		this->targetName = targetName;
		this->constant = constant;

		this->key = GetKey();
	}

protected:
	virtual std::string GetKey() {
		return mapName + std::to_string( modelIndex ) + targetName; 
	}
};

class ModelIndexWithSound : public ModelIndex {

public:
	std::string soundPath;
	float delay;
	bool isMaxCommentary;

	ModelIndexWithSound() : ModelIndex() {
		this->soundPath = "";
		this->delay = 0.0f;
	}

	ModelIndexWithSound( const std::string &mapName, int modelIndex, const std::string &targetName, const std::string &soundPath, bool isMaxCommentary = false, float delay = 0.0f, bool constant = false ) 
		: ModelIndex( mapName, modelIndex, targetName, constant ) {
		
		this->soundPath = soundPath;
		this->delay = delay;
		this->isMaxCommentary = isMaxCommentary;
		this->key = GetKey();
	}

private:
	virtual std::string GetKey() {
		return mapName + std::to_string( modelIndex ) + targetName + soundPath; 
	}
};

bool operator < ( const ModelIndex &modelIndex1, const ModelIndex &modelIndex2 );

struct EntitySpawn
{
	std::string mapName;
	std::string entityName;
	float		origin[3];
	float		angle;
};

enum GAMEPLAY_MOD {
	GAMEPLAY_MOD_UNKNOWN,
	GAMEPLAY_MOD_BLEEDING,
	GAMEPLAY_MOD_BULLET_PHYSICS_CONSTANT,
	GAMEPLAY_MOD_BULLET_PHYSICS_DISABLED,
	GAMEPLAY_MOD_BULLET_PHYSICS_ENEMIES_AND_PLAYER_ON_SLOWMOTION,
	GAMEPLAY_MOD_CONSTANT_SLOWMOTION,
	GAMEPLAY_MOD_DIVING_ALLOWED_WITHOUT_SLOWMOTION,
	GAMEPLAY_MOD_DIVING_ONLY,
	GAMEPLAY_MOD_DRUNK,
	GAMEPLAY_MOD_EASY,
	GAMEPLAY_MOD_EDIBLE_GIBS,
	GAMEPLAY_MOD_EMPTY_SLOWMOTION,
	GAMEPLAY_MOD_FADING_OUT,
	GAMEPLAY_MOD_HARD,
	GAMEPLAY_MOD_HEADSHOTS,
	GAMEPLAY_MOD_GARBAGE_GIBS,
	GAMEPLAY_MOD_INFINITE_AMMO,
	GAMEPLAY_MOD_INFINITE_SLOWMOTION,
	GAMEPLAY_MOD_INSTAGIB,
	GAMEPLAY_MOD_NO_FALL_DAMAGE,
	GAMEPLAY_MOD_NO_PILLS,
	GAMEPLAY_MOD_NO_SAVING,
	GAMEPLAY_MOD_NO_SECONDARY_ATTACK,
	GAMEPLAY_MOD_NO_SLOWMOTION,
	GAMEPLAY_MOD_NO_SMG_GRENADE_PICKUP,
	GAMEPLAY_MOD_ONE_HIT_KO,
	GAMEPLAY_MOD_ONE_HIT_KO_FROM_PLAYER,
	GAMEPLAY_MOD_PREVENT_MONSTER_SPAWN,
	GAMEPLAY_MOD_SLOWMOTION_ON_DAMAGE,
	GAMEPLAY_MOD_SNARK_FROM_EXPLOSION,
	GAMEPLAY_MOD_SNARK_INCEPTION,
	GAMEPLAY_MOD_SNARK_INFESTATION,
	GAMEPLAY_MOD_SNARK_NUCLEAR,
	GAMEPLAY_MOD_SNARK_PARANOIA,
	GAMEPLAY_MOD_SNARK_STAY_ALIVE,
	GAMEPLAY_MOD_SUPERHOT,
	GAMEPLAY_MOD_SWEAR_ON_KILL,
	GAMEPLAY_MOD_UPSIDE_DOWN,
	GAMEPLAY_MOD_TOTALLY_SPIES,
	GAMEPLAY_MOD_VVVVVV,
	GAMEPLAY_MOD_WEAPON_RESTRICTED
};

struct GameplayMod
{
	GAMEPLAY_MOD id;
	std::string name;
	std::string description;
	std::function<void(CBasePlayer*)> init;

	GameplayMod() {
		this->id = GAMEPLAY_MOD_UNKNOWN;
		this->name = "UNKNOWN";
		this->description = "This mod has not been accounted and you should never see this message";
		this->init = []( CBasePlayer * ){};
	}

	GameplayMod( GAMEPLAY_MOD id, const std::string &name, const std::string description, std::function<void(CBasePlayer*)> initFunction ) :
		id( id ), name( name ), description( description ), init( initFunction )
	{}
};

class CustomGameModeConfig {

public:

	enum GAME_MODE_CONFIG_TYPE {
		GAME_MODE_CONFIG_MAP,
		GAME_MODE_CONFIG_CGM,
		GAME_MODE_CONFIG_BMM,
		GAME_MODE_CONFIG_SAGM
	};

	enum FILE_SECTION {
		FILE_SECTION_NO_SECTION,
		FILE_SECTION_NAME,
		FILE_SECTION_START_MAP,
		FILE_SECTION_START_POSITION,
		FILE_SECTION_END_MAP,
		FILE_SECTION_END_TRIGGER,
		FILE_SECTION_CHANGE_LEVEL_PREVENT,
		FILE_SECTION_LOADOUT,
		FILE_SECTION_ENTITY_SPAWN,
		FILE_SECTION_ENTITY_REMOVE,
		FILE_SECTION_ENTITY_MOVE,
		FILE_SECTION_ENTITY_USE,
		FILE_SECTION_SOUND_PREVENT,
		FILE_SECTION_SOUND,
		FILE_SECTION_MAX_COMMENTARY,
		FILE_SECTION_MODS,

		// Black Mesa Minute
		FILE_SECTION_TIMER_PAUSE,
		FILE_SECTION_TIMER_RESUME,
	};

	enum GAME_DIFFICULTY {
		GAME_DIFFICULTY_EASY,
		GAME_DIFFICULTY_MEDIUM,
		GAME_DIFFICULTY_HARD
	};
	
	CustomGameModeConfig( GAME_MODE_CONFIG_TYPE configType );

	bool ReadFile( const char *fileName );

	void Reset();
	bool OnNewSection( std::string sectionName );
	void OnSectionData( std::string line, int lineCount );
	void OnError( std::string );

	static std::string ConfigTypeToDirectoryName( GAME_MODE_CONFIG_TYPE configType );
	static std::string ConfigTypeToGameModeCommand( GAME_MODE_CONFIG_TYPE configType );
	static std::string ConfigTypeToGameModeName( GAME_MODE_CONFIG_TYPE configType );

	std::vector<std::string> GetAllConfigFileNames();
	std::vector<std::string> GetAllConfigFileNames( const char *path );

	static std::string GetGamePath();
	static int GetAllowedItemIndex( const char *allowedItem );
	static int GetAllowedEntityIndex( const char *allowedEntity );

	const ModelIndex ParseModelIndexString( std::string line, int lineCount );
	const ModelIndexWithSound ParseModelIndexWithSoundString( std::string line, int lineCount, bool isMaxCommentary );

	std::string configName;
	std::string error;
	GAME_MODE_CONFIG_TYPE configType;

	bool		markedForRestart;

	std::string name;
	std::string startMap;
	std::string endMap;

	std::vector<std::string> loadout;

	std::set<ModelIndex>	     timerPauses;
	std::set<ModelIndex>	     timerResumes;
	std::set<ModelIndexWithSound>	     sounds;
	std::set<ModelIndex>	     endTriggers;
	std::vector<EntitySpawn>     entitySpawns;
	std::set<ModelIndex>		 entityUses;
	std::set<std::string>		 entitiesToPrecache;
	std::set<std::string>		 soundsToPrecache;
	std::set<ModelIndex>		 entitiesToPrevent;
	std::set<std::string>		 changeLevelsToPrevent;

	bool startPositionSpecified;
	float startPosition[3];

	bool startYawSpecified;
	float startYaw;

	std::vector<GameplayMod> mods;
	bool IsGameplayModActive( GAMEPLAY_MOD mod );
	bool AddGameplayMod( const std::string &modName );

protected:
	std::string folderPath;
	std::string configFolderPath;
	FILE_SECTION currentFileSection;

};

#endif // CUSTOM_GAMEMODE_CONFIG_H