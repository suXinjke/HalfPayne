#ifndef CUSTOM_GAMEMODE_CONFIG_H
#define CUSTOM_GAMEMODE_CONFIG_H

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "cpp_aux.h"
#include <string>
#include <vector>
#include <set>
#include <map>
#include <functional>
#include "custom_gamemode_record.h"
#include "gameplay_mod.h"
#include "argument.h"

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
	"weapon_m249",
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
	"weapon_m249",
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
	"monster_barney",
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
	"monster_scientist",
	"monster_tripmine",
	"monster_zombie",

	"end_marker",
	"kerotan"
};

struct Hookable {
	std::string map;
	int modelIndex;
	std::string targetName;
	bool constant;

	Hookable() : map( "" ), modelIndex( -3 ), targetName( "" ), constant( false ) {};
	Hookable( const std::vector<Argument> &args );

	virtual bool Fits( int modelIndex, const std::string &className, const std::string &targetName, bool firstTime ) const {
		return (
			( STRING( gpGlobals->mapname ) == this->map || this->map == "everywhere" ) &&
			( ( modelIndex == this->modelIndex ) || ( !this->targetName.empty() && ( this->targetName == targetName || this->targetName == className ) ) ) &&
			( constant || ( !constant && firstTime ) )
		);
	}
};

struct HookableWithDelay : Hookable {
	float delay;

	HookableWithDelay() : Hookable(), delay( 0.0f ) {};
	HookableWithDelay( const std::vector<Argument> &args );
};

struct HookableWithTarget : Hookable {
	bool isModelIndex;
	std::string entityName;

	HookableWithTarget() : Hookable(), isModelIndex( false ), entityName( "" ) {};
	HookableWithTarget( const std::vector<Argument> &args );

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
	std::string originalName;
	std::string name;
	std::string targetName;
	float		x;
	float		y;
	float		z;
	float		angle;
	int spawnFlags = 0;
	int weaponFlags = 0;

	void UpdateSpawnFlags();
	void DetermineBestSpawnPosition( CBasePlayer *player );
};

struct EntitySpawn : Hookable {
	EntitySpawnData entity;

	EntitySpawn( const std::vector<Argument> &args );
};

struct EntityReplace : Hookable {
	std::string replacedEntity;
	EntitySpawnData newEntity;

	EntityReplace( const std::vector<Argument> &args );
};

struct EndCondition : Hookable {
	int activations;
	int activationsRequired;

	std::string objective;

	EndCondition( const std::vector<Argument> &args );

	std::string GetHash();
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
	std::string name;
	int amount;

	LoadoutItem() : name( "" ), amount( 1 ) {};
	LoadoutItem( const std::vector<Argument> &args );
};

struct Sound : Hookable {

	Sound( const std::vector<Argument> &args );

	std::string path;
	float delay = 0.1;
	float initialPos = 0.0f;
	bool looping = false;
	bool noSlowmotionEffects = false;
};

struct Intermission : HookableWithTarget {
	StartPosition startPos;

	Intermission() : HookableWithTarget() {};
	Intermission( const std::vector<Argument> &args );
};

struct Teleport : Hookable {
	StartPosition pos;

	Teleport( const std::vector<Argument> &args );
};

struct EntityRandomSpawner
{
	std::string mapName;
	EntitySpawnData entity;
	int maxAmount;
	bool spawnOnce;
	float spawnPeriod;
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
	CONFIG_FILE_SECTION_ENTITY_REPLACE,
	CONFIG_FILE_SECTION_SOUND,
	CONFIG_FILE_SECTION_MUSIC,
	CONFIG_FILE_SECTION_MUSIC_STOP,
	CONFIG_FILE_SECTION_PLAYLIST,
	CONFIG_FILE_SECTION_MAX_COMMENTARY,
	CONFIG_FILE_SECTION_MODS,
	CONFIG_FILE_SECTION_RANDOM_MODS_WHITELIST,
	CONFIG_FILE_SECTION_RANDOM_MODS_BLACKLIST,
	CONFIG_FILE_SECTION_END_CONDITIONS,
	CONFIG_FILE_SECTION_TIMER_PAUSE,
	CONFIG_FILE_SECTION_TIMER_RESUME,
	CONFIG_FILE_SECTION_TELEPORT,

	CONFIG_FILE_SECTION_AUX_END
};

enum CONFIG_TYPE {
	CONFIG_TYPE_MAP,
	CONFIG_TYPE_CGM,
	CONFIG_TYPE_VANILLA
};

class CustomGameModeConfig {

public:

	struct ConfigSection
	{
		typedef std::function<std::string( const std::vector<Argument> &args, const std::string &line )> ConfigSectionInitFunction;

		std::string name;
		std::vector<std::string> lines;
		std::vector<Argument> args;
		ConfigSectionInitFunction Init;
		
		int requiredAmountOfArguments;
		std::string argumentsString;

		ConfigSection() {};
		ConfigSection( const std::string &name, const std::vector<Argument> &args, const ConfigSectionInitFunction &init )
			: name( name ), args( args ), Init( init ), requiredAmountOfArguments( 0 ) {

			for ( const auto &arg : args ) {
				if ( arg.isRequired ) {
					requiredAmountOfArguments++;
				}

				argumentsString += arg.isRequired ?
					( " <" + arg.name + ">" ) :
					( " [" + arg.name + "]" );
			}

			argumentsString = aux::str::trim( argumentsString );
		}

		const std::string OnSectionData( const std::string &configName, const std::string &line, int lineCount, CONFIG_TYPE configType );
	};
	
	CustomGameModeConfig( CONFIG_TYPE configType );
	CustomGameModeRecord record;

	bool ReadFile( const char *fileName );

	void Reset();

	void InitConfigSections();

	const std::string GetHash();

	static std::string ConfigTypeToDirectoryName( CONFIG_TYPE configType );
	static std::string ConfigTypeToGameModeCommand( CONFIG_TYPE configType );
	std::string ConfigTypeToGameModeName( bool uppercase = false );

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

	std::set<std::string> GetSoundsToPrecacheForMap( const std::string &map );
	std::set<std::string> GetEntitiesToPrecacheForMap( const std::string &map );
	
	std::map<GameplayMod *, std::vector<Argument>> mods;

	bool musicPlaylistShuffle;
	
	bool hasEndMarkers;

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
	std::vector<Hookable> musicStops;
	std::vector<Intermission> intermissions;
	std::vector<Teleport> teleports;

	std::vector<Hookable> timerPauses;
	std::vector<Hookable> timerResumes;
	std::vector<EntityRandomSpawner> entityRandomSpawners;
	std::vector<EndCondition> endConditions;
	std::vector<HookableWithTarget> entitiesToRemove;
	std::vector<EntityReplace> entityReplaces;

	std::set<std::string> randomModsWhitelist;
	std::set<std::string> randomModsBlacklist;

protected:
	std::string folderPath;
	std::string configFolderPath;

};

#endif // CUSTOM_GAMEMODE_CONFIG_H