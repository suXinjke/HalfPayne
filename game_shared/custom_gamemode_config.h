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

struct StartPosition {
	bool defined;
	float x;
	float y;
	float z;
	float angle;
};

struct ConfigFileSound {

	bool valid;
	std::string soundPath;
	bool constant;
	float delay;

};

struct ConfigFileMusic {

	bool valid;
	std::string musicPath;
	bool constant;
	bool looping;
	float delay;
	float initialPos;

};


struct EntitySpawn
{
	std::string entityName;
	float		x;
	float		y;
	float		z;
	float		angle;
};

struct Intermission
{
	bool defined;
	
	std::string toMap;
	float x;
	float y;
	float z;
	float angle;
	bool strip;
};

enum GAMEPLAY_MOD {
	GAMEPLAY_MOD_UNKNOWN,
	GAMEPLAY_MOD_BLEEDING,
	GAMEPLAY_MOD_BULLET_PHYSICS_CONSTANT,
	GAMEPLAY_MOD_BULLET_PHYSICS_DISABLED,
	GAMEPLAY_MOD_BULLET_PHYSICS_ENEMIES_AND_PLAYER_ON_SLOWMOTION,
	GAMEPLAY_MOD_BULLET_RICOCHET,
	GAMEPLAY_MOD_CONSTANT_SLOWMOTION,
	GAMEPLAY_MOD_CROSSBOW_EXPLOSIVE_BOLTS,
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
	GAMEPLAY_MOD_INFINITE_AMMO_CLIP,
	GAMEPLAY_MOD_INFINITE_PAINKILLERS,
	GAMEPLAY_MOD_INFINITE_SLOWMOTION,
	GAMEPLAY_MOD_INSTAGIB,
	GAMEPLAY_MOD_NO_FALL_DAMAGE,
	GAMEPLAY_MOD_NO_HEALING,
	GAMEPLAY_MOD_NO_MAP_MUSIC,
	GAMEPLAY_MOD_NO_PILLS,
	GAMEPLAY_MOD_NO_SAVING,
	GAMEPLAY_MOD_NO_SECONDARY_ATTACK,
	GAMEPLAY_MOD_NO_SLOWMOTION,
	GAMEPLAY_MOD_NO_SMG_GRENADE_PICKUP,
	GAMEPLAY_MOD_ONE_HIT_KO,
	GAMEPLAY_MOD_ONE_HIT_KO_FROM_PLAYER,
	GAMEPLAY_MOD_PREVENT_MONSTER_SPAWN,
	GAMEPLAY_MOD_SLOWMOTION_ON_DAMAGE,
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
	GAMEPLAY_MOD_TOTALLY_SPIES,
	GAMEPLAY_MOD_VVVVVV,
	GAMEPLAY_MOD_WEAPON_RESTRICTED
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
	CONFIG_FILE_SECTION_SOUND_PREVENT,
	CONFIG_FILE_SECTION_SOUND,
	CONFIG_FILE_SECTION_MUSIC,
	CONFIG_FILE_SECTION_PLAYLIST,
	CONFIG_FILE_SECTION_MAX_COMMENTARY,
	CONFIG_FILE_SECTION_MODS,

	// Black Mesa Minute
	CONFIG_FILE_SECTION_TIMER_PAUSE,
	CONFIG_FILE_SECTION_TIMER_RESUME,

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

	bool ReadFile( const char *fileName );

	void Reset();
	bool OnNewSection( std::string sectionName );
	void OnError( std::string );

	void InitConfigSections();

	static std::string ConfigTypeToDirectoryName( CONFIG_TYPE configType );
	static std::string ConfigTypeToGameModeCommand( CONFIG_TYPE configType );
	static std::string ConfigTypeToGameModeName( CONFIG_TYPE configType );

	std::vector<std::string> GetAllConfigFileNames();
	std::vector<std::string> GetAllFileNames( const char *path, const char *extension, bool includeExtension = false );
	std::vector<std::string> GetAllFileNames( const char *path, const std::vector<std::string> &extensions, bool includeExtension = false );

	static std::string GetGamePath();
	static int GetAllowedItemIndex( const char *allowedItem );
	static int GetAllowedEntityIndex( const char *allowedEntity );

	const StartPosition GetStartPosition();
	const Intermission GetIntermission( const std::string &mapName, int modelIndex, const std::string &targetName );

	std::string configName;
	std::string error;
	CONFIG_TYPE configType;

	bool		markedForRestart;

	const std::string GetName();
	const std::string GetDescription();
	const std::string GetStartMap();
	const std::string GetEndMap();

	const std::vector<std::string> GetLoadout();
	const std::vector<EntitySpawn> GetEntitySpawnsForMapOnce( const std::string &map );
	bool IsChangeLevelPrevented( const std::string &nextMap );

	std::map< CONFIG_FILE_SECTION, ConfigSection> configSections;

	bool MarkModelIndex( CONFIG_FILE_SECTION fileSection, const std::string &mapName, int modelIndex, const std::string &targetName );
	const ConfigFileSound MarkModelIndexWithSound( CONFIG_FILE_SECTION fileSection, const std::string &mapName, int modelIndex, const std::string &targetName );
	const ConfigFileMusic MarkModelIndexWithMusic( CONFIG_FILE_SECTION fileSection, const std::string &mapName, int modelIndex, const std::string &targetName );

	std::set<std::string>		 entitiesToPrecache;
	std::set<std::string>		 soundsToPrecache;

	std::string ValidateModelIndexSectionData( ConfigSectionData &data );
	std::string ValidateModelIndexWithSoundSectionData( ConfigSectionData &data );
	std::string ValidateModelIndexWithMusicSectionData( ConfigSectionData &data );

	std::vector<GameplayMod> mods;
	bool IsGameplayModActive( GAMEPLAY_MOD mod );
	bool AddGameplayMod( ConfigSectionData &modName );

	std::vector<std::string> musicPlaylist;
	bool musicPlaylistShuffle;

protected:
	std::string folderPath;
	std::string configFolderPath;
	CONFIG_FILE_SECTION currentFileSection;

};

#endif // CUSTOM_GAMEMODE_CONFIG_H