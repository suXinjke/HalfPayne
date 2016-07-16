#ifndef BMM_CONFIG_H
#define BMM_CONFIG_H

#include "string_aux.h"

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

	static int GetAllowedItemIndex( const char *allowedItem ) {

		for ( int i = 0; i < sizeof( allowedItems ) / sizeof( allowedItems[0] ); i++ )
		{
			if ( strcmp( allowedItem, allowedItems[i] ) == 0 ) {
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
	};

	struct ModelIndex
	{
		std::string mapName;
		int			modelIndex;
	};

	BlackMesaMinuteConfig();

	bool Init( const char *configName );

	// Used only by client
	static std::string GetMapNameFromConfig( const char *configName );

	std::string error;

	std::string startMap;
	std::string endMap;
	std::string name;
	std::string configName;
	std::vector<std::string> loadout;

	std::vector<ModelIndex>	     timerPauseModelIndexes;
	std::vector<ModelIndex>	     timerResumeIndexes;

	bool startPositionSpecified;
	Vector startPosition;
	
	bool startYawSpecified;
	float startYaw;

private:
	std::string configFolderPath;

};


#endif

