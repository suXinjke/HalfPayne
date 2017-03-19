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
	bool		constant;

	ModelIndex() {
		this->mapName = "";
		this->modelIndex = -1;
		this->constant = false;

		this->key = mapName + std::to_string( modelIndex );
	}

	ModelIndex( const std::string &mapName, int modelIndex, bool constant = false ) {
		this->mapName = mapName;
		this->modelIndex = modelIndex;
		this->constant = constant;

		this->key = GetKey();
	}

protected:
	virtual std::string GetKey() {
		return mapName + std::to_string( modelIndex ); 
	}
};

class ModelIndexWithSound : public ModelIndex {

public:
	std::string soundPath;
	float delay;

	ModelIndexWithSound() : ModelIndex() {
		this->soundPath = "";
		this->delay = 0.0f;
	}

	ModelIndexWithSound( const std::string &mapName, int modelIndex, const std::string &soundPath, float delay = 0.0f, bool constant = false ) 
		: ModelIndex( mapName, modelIndex, constant ) {
		
		this->soundPath = soundPath;
		this->delay = delay;
		this->key = GetKey();
	}

private:
	virtual std::string GetKey() {
		return mapName + std::to_string( modelIndex ) + soundPath; 
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
		FILE_SECTION_SOUND,
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

	CustomGameModeConfig( const char *folderName );

	bool ReadFile( const char *fileName );

	void Reset();
	void OnNewSection( std::string sectionName );
	void OnSectionData( std::string line, int lineCount );
	void OnError( std::string );

	static std::string GetGamePath();
	static int GetAllowedItemIndex( const char *allowedItem );
	static int GetAllowedEntityIndex( const char *allowedEntity );

	const ModelIndex ParseModelIndexString( std::string line, int lineCount );
	const ModelIndexWithSound ParseModelIndexWithSoundString( std::string line, int lineCount );

	std::string configName;
	std::string error;

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
	std::set<std::string>		 entitiesToPrecache;
	std::set<std::string>		 soundsToPrecache;

	bool startPositionSpecified;
	float startPosition[3];

	bool startYawSpecified;
	float startYaw;

	GAME_DIFFICULTY difficulty;

	bool constantSlowmotion;
	bool infiniteSlowmotion;
	bool emptySlowmotion;
	bool noSlowmotion;
	
	bool noSaving;
	
	bool holdTimer;

	bool powerfulHeadshots;
	bool infiniteAmmo;
	bool weaponRestricted;
	bool instaGib;

protected:
	std::string folderPath;
	std::string configFolderPath;
	FILE_SECTION currentFileSection;

};

#endif // CUSTOM_GAMEMODE_CONFIG_H