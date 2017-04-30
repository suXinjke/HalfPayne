#ifndef CUSTOM_GAMEMODE_CONFIG_H
#define CUSTOM_GAMEMODE_CONFIG_H

#include <string>
#include <vector>
#include <set>

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

	bool startPositionSpecified;
	float startPosition[3];

	bool startYawSpecified;
	float startYaw;

	GAME_DIFFICULTY difficulty;

	bool constantSlowmotion;
	bool infiniteSlowmotion;
	bool emptySlowmotion;
	bool noSlowmotion;
	bool slowmotionOnDamage;
	
	bool noSaving;
	bool noPills;
	bool preventMonsterSpawn;

	bool isBleeding;
	bool divingOnly;
	bool totallySpies;

	// Half-Payne - first mod that gives snarks the second chance
	bool snarkParanoia;
	bool snarkInception;
	bool snarkNuclear;
	bool snarkStayAlive;
	bool snarkInfestation;
	
	bool holdTimer;

	bool powerfulHeadshots;
	bool infiniteAmmo;
	bool weaponRestricted;
	bool instaGib;
	bool swearOnKill;
	bool oneHitKOFromPlayer;
	bool oneHitKO;

	// dumb, but I'd like to avoid including player.h where BULLET_PHYSICS_MODE enum is defined
	bool bulletPhysicsDisabled;
	bool bulletPhysicsEnemiesAndPlayerOnSlowmotion;
	bool bulletPhysicsConstant;

protected:
	std::string folderPath;
	std::string configFolderPath;
	FILE_SECTION currentFileSection;

};

#endif // CUSTOM_GAMEMODE_CONFIG_H