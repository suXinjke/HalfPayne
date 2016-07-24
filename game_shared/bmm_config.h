#ifndef BMM_CONFIG_H
#define BMM_CONFIG_H

#include "string_aux.h"
#include <set>

namespace BMM
{
	static const char *allowedItems[] = {
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

	static int GetAllowedItemIndex( const char *allowedItem ) {

		for ( int i = 0; i < sizeof( allowedItems ) / sizeof( allowedItems[0] ); i++ )
		{
			if ( strcmp( allowedItem, allowedItems[i] ) == 0 ) {
				return i;
			}
		}

		return -1;
	}

	static int GetAllowedEntityIndex( const char *allowedEntity ) {

		for ( int i = 0; i < sizeof( allowedEntities ) / sizeof( allowedEntity[0] ); i++ )
		{
			if ( strcmp( allowedEntity, allowedEntities[i] ) == 0 ) {
				return i;
			}
		}

		return -1;
	}
}

class BlackMesaMinuteRecord
{
public:
	BlackMesaMinuteRecord( const char *recordName );
	void Save();

    float time;
    float realTime;
    float realTimeMinusTime;

private:
	const float DEFAULT_TIME = 59999.0f; // 999:59.00
    std::string filePath;
};

class BlackMesaMinuteConfig {

public:
	enum BMM_FILE_SECTION {
		BMM_FILE_SECTION_NO_SECTION,
		BMM_FILE_SECTION_START_MAP,
		BMM_FILE_SECTION_END_MAP,
		BMM_FILE_SECTION_LOADOUT,
		BMM_FILE_SECTION_START_POSITION,
		BMM_FILE_SECTION_NAME,
		BMM_FILE_SECTION_TIMER_PAUSE,
		BMM_FILE_SECTION_TIMER_RESUME,
		BMM_FILE_SECTION_END_TRIGGER,
		BMM_FILE_SECTION_ENTITY_SPAWN,
		BMM_FILE_SECTION_MODS,
	};
	
	enum BMM_DIFFICULTY	{
		BMM_DIFFICULTY_EASY,
		BMM_DIFFICULTY_MEDIUM,
		BMM_DIFFICULTY_HARD,
	};

	struct ModelIndex
	{
		std::string key;

		std::string mapName;
		int			modelIndex;
		bool		constant;

		ModelIndex( const std::string &mapName, int modelIndex, bool constant = false ) {
			this->mapName = mapName;
			this->modelIndex = modelIndex;
			this->constant = constant;

			this->key = mapName + std::to_string( modelIndex );
		}
	};

	struct EntitySpawn
	{
		std::string mapName;
		std::string entityName;
		Vector		origin;
		int			angle;
	};

	BlackMesaMinuteConfig();

	void Reset();
	bool Init( const char *configName );

	std::string error;

	std::string startMap;
	std::string endMap;
	std::string name;
	std::string configName;
	std::vector<std::string> loadout;

	std::set<ModelIndex>	     timerPauses;
	std::set<ModelIndex>	     timerResumes;
	std::set<ModelIndex>	     endTriggers;
	std::vector<EntitySpawn>     entitySpawns;
	std::set<std::string>		 entitiesToPrecache;

	bool startPositionSpecified;
	Vector startPosition;
	
	bool startYawSpecified;
	float startYaw;

	BMM_DIFFICULTY				difficulty;

	bool constantSlowmotion;
	bool infiniteSlowmotion;
	bool emptySlowmotion;
	bool noSlowmotion;

	bool powerfulHeadshots;
	
	bool infiniteAmmo;
	bool weaponRestricted;
	bool instaGib;

private:
	std::string configFolderPath;

};

bool operator < ( const BlackMesaMinuteConfig::ModelIndex &modelIndex1, const BlackMesaMinuteConfig::ModelIndex &modelIndex2 );


#endif

