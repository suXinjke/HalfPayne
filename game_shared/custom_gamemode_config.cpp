#include "custom_gamemode_config.h"
#include <fstream>
#include <regex>
#include <sstream>
#include "sha1.h"
#include "fs_aux.h"
#include "gameplay_mod.h"
#include "argument.h"
#include "../fmt/printf.h"

int CustomGameModeConfig::GetAllowedItemIndex( const char *allowedItem ) {

	for ( int i = 0; i < sizeof( allowedItems ) / sizeof( allowedItems[0] ); i++ )
	{
		if ( strcmp( allowedItem, allowedItems[i] ) == 0 ) {
			return i;
		}
	}

	return -1;
}

int CustomGameModeConfig::GetAllowedEntityIndex( const char *allowedEntity ) {

	for ( int i = 0; i < sizeof( allowedEntities ) / sizeof( allowedEntities[0] ); i++ )
	{
		if ( strcmp( allowedEntity, allowedEntities[i] ) == 0 ) {
			return i;
		}
	}

	return -1;
}

EXTERN_C IMAGE_DOS_HEADER __ImageBase;
std::string CustomGameModeConfig::GetGamePath() {

	// Retrieve absolute path to the DLL that calls this function
	WCHAR dllPathWString[MAX_PATH] = { 0 };
	GetModuleFileNameW( ( HINSTANCE ) &__ImageBase, dllPathWString, _countof( dllPathWString ) );
	char dllPathCString[MAX_PATH];
	wcstombs( dllPathCString, dllPathWString, MAX_PATH );
	std::string dllPath( dllPathCString );

	// dumb way to get absolute path to mod directory
#ifdef CLIENT_DLL
	return dllPath.substr( 0, dllPath.rfind( "\\cl_dlls\\client.dll" ) );
#else
	return dllPath.substr( 0, dllPath.rfind( "\\dll\\hl.dll" ) );
#endif
}

CustomGameModeConfig::CustomGameModeConfig( CONFIG_TYPE configType )
{
	this->configType = configType;

	std::string folderName = ConfigTypeToDirectoryName( configType );

	folderPath = GetGamePath() + "\\" + folderName;

	// Create directory if it's not there
	CreateDirectory( folderPath.c_str(), NULL );

	InitConfigSections();
	Reset();
}

void CustomGameModeConfig::InitConfigSections() {

	configSections[CONFIG_FILE_SECTION_NAME] = ConfigSection(
		"name",
		{
			Argument( "name" )
		},
		[this]( const std::vector<Argument> &args, const std::string &line ) {
			std::string sanitizedName = line;
			if ( sanitizedName.size() > 54 ) {
				sanitizedName = sanitizedName.substr( 0, 53 );
			}

			name = sanitizedName;

			return "";
		}
	);

	configSections[CONFIG_FILE_SECTION_DESCRIPTION] = ConfigSection(
		"description",
		{
			Argument( "description" )
		},
		[this]( const std::vector<Argument> &args, const std::string &line ) {
			description = line;

			return "";
		}
	);

	configSections[CONFIG_FILE_SECTION_START_MAP] = ConfigSection(
		"start_map",
		{
			Argument( "start_map" )
		},
		[this]( const std::vector<Argument> &args, const std::string &line ) {
			startMap = line;
			
			return "";
		}
	);

	configSections[CONFIG_FILE_SECTION_START_POSITION] = ConfigSection(
		"start_position",
		{
			Argument( "x" ).IsNumber(),
			Argument( "y" ).IsNumber(),
			Argument( "z" ).IsNumber(),
			Argument( "angle" ).IsOptional().IsNumber().Default( "NAN" )
		},
		[this]( const std::vector<Argument> &args, const std::string &line ) {

			startPosition = {
				true,
				args.at( 0 ).number,
				args.at( 1 ).number,
				args.at( 2 ).number,
				args.at( 3 ).number,
				false 
			};

			return "";
		}
	);

	configSections[CONFIG_FILE_SECTION_END_MAP] = ConfigSection(
		"end_map",
		{
			Argument( "end_map" )
		},
		[this]( const std::vector<Argument> &args, const std::string &line ) {
			endMap = line;
			return "";
		}
	);

	configSections[CONFIG_FILE_SECTION_END_TRIGGER] = ConfigSection(
		"end_trigger",
		{
			Argument( "map_name" ),
			Argument( "model_index | target_name" ).CanBeNumber()
		},
		[this]( const std::vector<Argument> &args, const std::string &line ) {
			endTrigger = Hookable( args );

			return "";
		}
	);

	configSections[CONFIG_FILE_SECTION_CHANGE_LEVEL_PREVENT] = ConfigSection(
		"change_level_prevent",
		{
			Argument( "prevented_map" )
		},
		[this]( const std::vector<Argument> &args, const std::string &line ) {
			forbiddenLevels.insert( line );
			return "";
		}
	);

	configSections[CONFIG_FILE_SECTION_LOADOUT] = ConfigSection(
		"loadout",
		{
			Argument( "item" ),
			Argument( "amount" ).IsOptional().IsNumber().Default( "1" )
		},
		[this]( const std::vector<Argument> &args, const std::string &line ) {
			
			std::string itemName = args.at( 0 ).string;
			if ( GetAllowedItemIndex( itemName.c_str() ) == -1 ) {
				return fmt::sprintf( "incorrect loadout item name: %s", itemName.c_str() );
			}

			loadout.push_back( LoadoutItem( args ) );
			return std::string( "" );
		}
	);

	configSections[CONFIG_FILE_SECTION_ENTITY_SPAWN] = ConfigSection(
		"entity_spawn",
		{
			Argument( "map_name" ),
			Argument( "model_index | target_name" ).CanBeNumber(),
			Argument( "spawned_entity_name" ),
			Argument( "x" ).IsNumber(),
			Argument( "y" ).IsNumber(),
			Argument( "z" ).IsNumber(),
			Argument( "angle" ).IsOptional().IsNumber().Default( "0" ),
			Argument( "spawned_entity_target_name" ).IsOptional()
		},
		[this]( const std::vector<Argument> &args, const std::string &line ) {

			std::string entityName = args.at( 2 ).string;

			if ( GetAllowedEntityIndex( entityName.c_str() ) == -1 ) {
				return fmt::sprintf( "incorrect entity name: %s", entityName.c_str() );
			}

			if ( entityName == "end_marker" ) {
				hasEndMarkers = true;
			}

			entitySpawns.push_back( EntitySpawn( args ) );

			return std::string( "" );
		}
	);

	configSections[CONFIG_FILE_SECTION_ENTITY_USE] = ConfigSection(
		"entity_use",
		{
			Argument( "map_name" ),
			Argument( "model_index | target_name" ).CanBeNumber(),
			Argument( "target_to_use" ).CanBeNumber(),
			Argument( "const" ).IsOptional()
		},
		[this]( const std::vector<Argument> &args, const std::string &line ) {
			entityUses.push_back( HookableWithTarget( args ) );

			return "";
		}
	);

	configSections[CONFIG_FILE_SECTION_ENTITY_PREVENT] = ConfigSection(
		"entity_prevent",
		{
			Argument( "map_name" ),
			Argument( "model_index | target_name" ).CanBeNumber(),
			Argument( "const" ).IsOptional()
		},
		[this]( const std::vector<Argument> &args, const std::string &line ) {
			entitiesPrevented.push_back( Hookable( args ) );

			return "";
		}
	);

	configSections[CONFIG_FILE_SECTION_ENTITY_REMOVE] = ConfigSection(
		"entity_remove",
		{
			Argument( "map_name" ),
			Argument( "model_index | target_name" ).CanBeNumber(),
			Argument( "target_to_remove" ).CanBeNumber(),
			Argument( "const" ).IsOptional()
		},
		[this]( const std::vector<Argument> &args, const std::string &line ) {
			entitiesToRemove.push_back( HookableWithTarget( args ) );

			return "";
		}
	);

	configSections[CONFIG_FILE_SECTION_SOUND] = ConfigSection(
		"sound",
		{
			Argument( "map_name" ),
			Argument( "model_index | target_name" ).CanBeNumber(),
			Argument( "sound_path" ),
			Argument( "delay" ).IsOptional().IsNumber().MinMax( 0.1 ).Default( "0.1" ),
			Argument( "const" ).IsOptional()
		},
		[this]( const std::vector<Argument> &args, const std::string &line ) {
			sounds.push_back( Sound( args ) );

			return "";
		}
	);

	configSections[CONFIG_FILE_SECTION_MUSIC] = ConfigSection(
		"music",
		{
			Argument( "map_name" ),
			Argument( "model_index | target_name" ).CanBeNumber(),
			Argument( "sound_path" ),
			Argument( "delay" ).IsOptional().IsNumber().MinMax( 0.1 ).Default( "0.1" ),
			Argument( "initial_pos" ).IsOptional().IsNumber().MinMax( 0 ).Default( "0" ),
			Argument( "const" ).IsOptional(),
			Argument( "looping" ).IsOptional(),
			Argument( "no_slowmotion_effects" ).IsOptional()
		},
		[this]( const std::vector<Argument> &args, const std::string &line ) {
			music.push_back( Sound( args ) );

			return "";
		}
	);

	configSections[CONFIG_FILE_SECTION_MUSIC_STOP] = ConfigSection(
		"music_stop",
		{
			Argument( "map_name" ),
			Argument( "model_index | target_name" ).CanBeNumber(),
			Argument( "const" ).IsOptional()
		},
		[this]( const std::vector<Argument> &args, const std::string &line ) {
			musicStops.push_back( Hookable( args ) );

			return "";
		}
	);

	configSections[CONFIG_FILE_SECTION_PLAYLIST] = ConfigSection(
		"playlist",
		{
			Argument( "file_path" )
		},
		[this]( const std::vector<Argument> &args, const std::string &line ) {

			if ( line == "shuffle" ) {
				musicPlaylistShuffle = true;
				return "";
			}

			WIN32_FIND_DATA fdFile;

			if ( FindFirstFile( line.c_str(), &fdFile ) == INVALID_HANDLE_VALUE ) {
				return "";
			}

			if ( strcmp( fdFile.cFileName, "." ) != 0 && strcmp( fdFile.cFileName, ".." ) != 0 ) {
				std::vector<std::string> files;
				if ( fdFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {
					files = GetAllFileNames( line.c_str(), { "wav", "ogg", "mp3" }, true );
				} else {
					files.push_back( line );
				}

				for ( const auto &file : files ) {

					// TODO: GET RID OF THIS HACK We only need file paths
					auto argsCopy = args;
					argsCopy.at( 0 ).string = file;
					argsCopy.insert( argsCopy.begin(), Argument( "dummy" ) );
					argsCopy.insert( argsCopy.begin(), Argument( "dummy" ) );
					musicPlaylist.push_back( Sound( argsCopy ) );
				}
			}

			return "";
		}
	);

	configSections[CONFIG_FILE_SECTION_MAX_COMMENTARY] = ConfigSection(
		"max_commentary",
		{
			Argument( "map_name" ),
			Argument( "model_index | target_name" ).CanBeNumber(),
			Argument( "sound_path" ),
			Argument( "delay" ).IsOptional().IsNumber().MinMax( 0.1 ).Default( "0.1" ),
			Argument( "const" ).IsOptional()
		},
		[this]( const std::vector<Argument> &args, const std::string &line ) {
			Sound commentary = Sound( args );
			commentary.noSlowmotionEffects = true;
			maxCommentary.push_back( commentary );

			return "";
		}
	);

	configSections[CONFIG_FILE_SECTION_MODS] = ConfigSection(
		"mods",
		{
			Argument( "mod" ),
			Argument( "param1" ).IsOptional().CanBeNumber(),
			Argument( "param2" ).IsOptional().CanBeNumber(),
			Argument( "param3" ).IsOptional().CanBeNumber(),
			Argument( "param4" ).IsOptional().CanBeNumber(),
			Argument( "param5" ).IsOptional().CanBeNumber()
		},
		[this]( const std::vector<Argument> &args, const std::string &line ) { 

			std::string modName = args.at( 0 ).string;

			if ( auto modWithArguments = gameplayMods::GetModAndParseArguments( line ) ) {
				mods[modWithArguments->first] = modWithArguments->second;
			} else {
				return fmt::sprintf( "incorrect mod specified: %s\n", modName.c_str() );
			}

			return std::string( "" );
		}
	);

	configSections[CONFIG_FILE_SECTION_RANDOM_MODS_WHITELIST] = ConfigSection(
		"random_mods_whitelist",
		{
			Argument( "mod" ),
		},
		[this]( const std::vector<Argument> &args, const std::string &line ) {
			std::string modName = args.at( 0 ).string;

			if ( auto modWithArguments = gameplayMods::GetModAndParseArguments( modName ) ) {
				randomModsWhitelist.insert( modWithArguments->first->id );
			} else {
				return fmt::sprintf( "incorrect mod specified: %s\n", modName.c_str() );
			}

			return std::string( "" );
		}
	);

	configSections[CONFIG_FILE_SECTION_RANDOM_MODS_BLACKLIST] = ConfigSection(
		"random_mods_blacklist",
		{
			Argument( "mod" ),
		},
		[this]( const std::vector<Argument> &args, const std::string &line ) {
			std::string modName = args.at( 0 ).string;

			if ( auto modWithArguments = gameplayMods::GetModAndParseArguments( modName ) ) {
				randomModsBlacklist.insert( modWithArguments->first->id );
			} else {
				return fmt::sprintf( "incorrect mod specified: %s\n", modName.c_str() );
			}

			return std::string( "" );
		}
	);

	configSections[CONFIG_FILE_SECTION_TIMER_PAUSE] = ConfigSection(
		"timer_pause",
		{
			Argument( "map_name" ),
			Argument( "model_index | target_name" ).CanBeNumber(),
			Argument( "const" ).IsOptional()
		},
		[this]( const std::vector<Argument> &args, const std::string &line ) {
			timerPauses.push_back( Hookable( args ) );

			return "";
		}
	);

	configSections[CONFIG_FILE_SECTION_TIMER_RESUME] = ConfigSection(
		"timer_resume",
		{
			Argument( "map_name" ),
			Argument( "model_index | target_name" ).CanBeNumber(),
			Argument( "const" ).IsOptional()
		},
		[this]( const std::vector<Argument> &args, const std::string &line ) {
			timerResumes.push_back( Hookable( args ) );

			return "";
		}
	);

	configSections[CONFIG_FILE_SECTION_INTERMISSION] = ConfigSection(
		"intermission",
		{
			Argument( "map_name" ),
			Argument( "model_index | target_name" ).CanBeNumber(),
			Argument( "next_map" ),
			Argument( "x" ).IsOptional().IsNumber(),
			Argument( "y" ).IsOptional().IsNumber(),
			Argument( "z" ).IsOptional().IsNumber(),
			Argument( "angle" ).IsOptional().IsNumber().Default( "0" ),
			Argument( "stripped" ).IsOptional()
		},
		[this]( const std::vector<Argument> &args, const std::string &line ) {
			intermissions.push_back( Intermission( args ) );

			return std::string( "" );
		}
	);

	configSections[CONFIG_FILE_SECTION_ENTITY_RANDOM_SPAWNER] = ConfigSection(
		"entity_random_spawner",
		{
			Argument( "map_name" ),
			Argument( "entity_name" ),
			Argument( "max_amount" ).IsOptional().IsNumber().MinMax( 1 ).Default( "50" ),
			Argument( "spawn_period" ).IsOptional().CanBeNumber().MinMax( 0.1 ).Default( "2" )
		},
		[this]( const std::vector<Argument> &args, const std::string &line ) {
			std::string entityName = args.at( 1 ).string;
			if ( GetAllowedEntityIndex( entityName.c_str() ) == -1 ) {
				return fmt::sprintf( "incorrect entity name: %s", entityName.c_str() );
			}

			EntityRandomSpawner spawner;
			spawner.mapName = args.at( 0 ).string;
			spawner.entity.originalName = entityName;
			spawner.entity.name = entityName;
			spawner.maxAmount = args.at( 2 ).number;
			spawner.spawnOnce = args.at( 3 ).string == "once";
			spawner.spawnPeriod = std::isnan( args.at( 3 ).number ) ? 2.0f : args.at( 3 ).number;
			spawner.entity.UpdateSpawnFlags();
				
			entityRandomSpawners.push_back( spawner );

			return std::string( "" );
		}
	);

	configSections[CONFIG_FILE_SECTION_ENTITY_REPLACE] = ConfigSection(
		"entity_replace",
		{
			Argument( "map_name" ),
			Argument( "model_index | target_name" ).CanBeNumber(),
			Argument( "replaced_entity" ),
			Argument( "new_entity" )
		},
		[this]( const std::vector<Argument> &args, const std::string &line ) {
			std::string entityName = args.at( 2 ).string;
			if ( GetAllowedEntityIndex( entityName.c_str() ) == -1 ) {
				return fmt::sprintf( "incorrect entity name: %s", entityName.c_str() );
			}
			std::string newEntityName = args.at( 3 ).string;
			if ( GetAllowedEntityIndex( newEntityName.c_str() ) == -1 ) {
				return fmt::sprintf( "incorrect entity name: %s", newEntityName.c_str() );
			}

			entityReplaces.push_back( EntityReplace( args ) );

			return std::string( "" );
		}
	);

	configSections[CONFIG_FILE_SECTION_END_CONDITIONS] = ConfigSection(
		"end_conditions",
		{
			Argument( "map_name" ),
			Argument( "model_index | target_name" ).CanBeNumber(),
			Argument( "activations_required" ).IsOptional().IsNumber().MinMax( 1 ).Default( "1" ),
			Argument( "objective" ).IsOptional().Default( "COMPLETION" )
		},
		[this]( const std::vector<Argument> &args, const std::string &line ) {
			endConditions.push_back( EndCondition( args ) );

			return std::string( "" );
		}
	);

	configSections[CONFIG_FILE_SECTION_TELEPORT] = ConfigSection(
		"teleport",
		{
			Argument( "map_name" ),
			Argument( "model_index | target_name" ).CanBeNumber(),
			Argument( "x" ).IsNumber(),
			Argument( "y" ).IsNumber(),
			Argument( "z" ).IsNumber(),
			Argument( "angle" ).IsOptional().IsNumber().Default( "0" )
		},
		[this]( const std::vector<Argument> &args, const std::string &line ) {
			teleports.push_back( Teleport( args ) );

			return std::string( "" );
		}
	);
}

std::set<std::string> CustomGameModeConfig::GetSoundsToPrecacheForMap( const std::string &map ) {
	std::set<std::string> soundsToPrecache;

	for ( const auto &sound : sounds ) {
		if ( sound.map == map ) {
			soundsToPrecache.insert( sound.path );
		}
	}

	if (
		gameplayMods::randomGameplayMods.isActive() ||
		gameplayMods::paynedSoundsHumans.isActive() ||
		gameplayMods::paynedSoundsMonsters.isActive()
	) {
		soundsToPrecache.insert( "payned/alert1.wav" );
		soundsToPrecache.insert( "payned/alert2.wav" );
		soundsToPrecache.insert( "payned/alert3.wav" );
		soundsToPrecache.insert( "payned/alert4.wav" );
		soundsToPrecache.insert( "payned/alert5.wav" );
		soundsToPrecache.insert( "payned/alert6.wav" );
		soundsToPrecache.insert( "payned/alert7.wav" );
		soundsToPrecache.insert( "payned/attack1.wav" );
		soundsToPrecache.insert( "payned/attack2.wav" );
		soundsToPrecache.insert( "payned/attack3.wav" );
		soundsToPrecache.insert( "payned/die1.wav" );
		soundsToPrecache.insert( "payned/die2.wav" );
		soundsToPrecache.insert( "payned/die3.wav" );
		soundsToPrecache.insert( "payned/die4.wav" );
		soundsToPrecache.insert( "payned/die5.wav" );
		soundsToPrecache.insert( "payned/die6.wav" );
		soundsToPrecache.insert( "payned/die7.wav" );
		soundsToPrecache.insert( "payned/pain1.wav" );
		soundsToPrecache.insert( "payned/pain2.wav" );
		soundsToPrecache.insert( "payned/pain3.wav" );
		soundsToPrecache.insert( "payned/pain4.wav" );
		soundsToPrecache.insert( "payned/pain5.wav" );
		soundsToPrecache.insert( "payned/pain6.wav" );
		soundsToPrecache.insert( "payned/pain7.wav" );
		soundsToPrecache.insert( "payned/pain8.wav" );
		soundsToPrecache.insert( "payned/pain9.wav" );
	}

	if (
		gameplayMods::randomGameplayMods.isActive() ||
		gameplayMods::cncSounds.isActive()
	) {
		soundsToPrecache.insert( "cnc/cnc_ded01.wav" );
		soundsToPrecache.insert( "cnc/cnc_ded02.wav" );
		soundsToPrecache.insert( "cnc/cnc_ded03.wav" );
		soundsToPrecache.insert( "cnc/cnc_ded04.wav" );
		soundsToPrecache.insert( "cnc/cnc_ded05.wav" );
		soundsToPrecache.insert( "cnc/cnc_ded06.wav" );
		soundsToPrecache.insert( "cnc/cnc_ded07.wav" );
		soundsToPrecache.insert( "cnc/cnc_ded08.wav" );
		soundsToPrecache.insert( "cnc/cnc_ded09.wav" );
		soundsToPrecache.insert( "cnc/cnc_ded10.wav" );
		soundsToPrecache.insert( "cnc/cnc_ded11.wav" );
	}

	if (
		gameplayMods::randomGameplayMods.isActive() ||
		gameplayMods::deusExSounds.isActive()
	) {
		soundsToPrecache.insert( "deusex/dead01.wav" );
		soundsToPrecache.insert( "deusex/dead02.wav" );
		soundsToPrecache.insert( "deusex/dead03.wav" );
		soundsToPrecache.insert( "deusex/pain01.wav" );
		soundsToPrecache.insert( "deusex/pain02.wav" );
	}

	return soundsToPrecache;
}

std::set<std::string> CustomGameModeConfig::GetEntitiesToPrecacheForMap( const std::string &map ) {
	std::set<std::string> entitiesToPrecache;

	for ( const auto &entitySpawn : entitySpawns ) {
		if ( entitySpawn.map == map || entitySpawn.map == "everywhere" ) {
			entitiesToPrecache.insert( entitySpawn.entity.name );
		}
	}

	for ( const auto &entityRandomSpawner : entityRandomSpawners ) {
		if ( entityRandomSpawner.mapName == map || entityRandomSpawner.mapName == "everywhere" ) {
			entitiesToPrecache.insert( entityRandomSpawner.entity.name );
		}
	}

	for ( const auto &entityReplace : entityReplaces ) {
		if ( entityReplace.map == map || entityReplace.map == "everywhere" ) {
			entitiesToPrecache.insert( entityReplace.newEntity.name );
		}
	}

	return entitiesToPrecache;
}

std::string CustomGameModeConfig::ConfigTypeToDirectoryName( CONFIG_TYPE configType ) {
	switch ( configType ) {
		case CONFIG_TYPE_MAP:
			return "map_cfg";

		case CONFIG_TYPE_CGM:
			return "cgm_cfg";

		default:
			return "";
	}
}

std::string CustomGameModeConfig::ConfigTypeToGameModeCommand( CONFIG_TYPE configType ) {
	switch ( configType ) {
		case CONFIG_TYPE_CGM:
			return "cgm";	
		
		case CONFIG_TYPE_VANILLA:
			return "vanilla";

		default:
			return "";
	}
}

std::string CustomGameModeConfig::ConfigTypeToGameModeName( bool uppercase ) {
	auto blackMesaMinute = gameplayMods::blackMesaMinute.isActive();
	auto scoreAttack = gameplayMods::scoreAttack.isActive();

	if ( blackMesaMinute && scoreAttack ) {
		return uppercase ? "BLACK MESA MINUTE SCORE ATTACK" : "Black Mesa Minute Score Attack";
	} else if ( blackMesaMinute ) {
		return uppercase ? "BLACK MESA MINUTE" : "Black Mesa Minute";
	} else if ( scoreAttack ) {
		return uppercase ? "SCORE ATTACK" : "Score Attack";
	} else if ( configType == CONFIG_TYPE_CGM ) {
		return uppercase ? "CUSTOM GAME MODE" : "Custom Game Mode";
	}

	return "";
}

std::vector<std::string> CustomGameModeConfig::GetAllConfigFileNames() {
	return GetAllFileNames( folderPath.c_str(), "txt" );
}

std::vector<std::string> CustomGameModeConfig::GetAllFileNames( const char *path, const std::vector<std::string> &extensions, bool includeExtension ) {
	std::vector<std::string> result;
	for ( const auto &extension : extensions ) {
		auto extensionVector = GetAllFileNames( path, extension.c_str(), includeExtension );
		result.insert( result.end(), extensionVector.begin(), extensionVector.end() );
	}

	return result;
}

// Based on this answer
// http://stackoverflow.com/questions/2314542/listing-directory-contents-using-c-and-windows
std::vector<std::string> CustomGameModeConfig::GetAllFileNames( const char *path, const char *extension, bool includeExtension ) {

	std::vector<std::string> result = FS_GetAllFilesInDirectory( path, extension );

	std::transform( result.begin(), result.end(), result.begin(), [this, extension, includeExtension]( std::string &path ){
		
		std::string pathSubstring = folderPath + "\\";
		std::string extensionSubstring = "." + std::string( extension );

		std::string::size_type pos = path.find( pathSubstring );
		if ( pos != std::string::npos ) {
			path.erase( pos, pathSubstring.length() );
		}

		pos = path.rfind( extensionSubstring );
		if ( pos != std::string::npos ) {

			if ( !includeExtension ) {
				path.erase( pos, extensionSubstring.length() );
			}
		}

		return path;
	} );

	return result;
}

bool CustomGameModeConfig::ReadFile( const char *fileName ) {

	error = "";
	Reset();

	configName = fileName;
	configNameSeparated = aux::str::split( fileName, "[\\\\\\/]" );

	std::string filePath = folderPath + "\\" + std::string( fileName ) + ".txt";

	int lineCount = 0;
	int sectionDataLines = -1;

	std::ifstream inp( filePath );
	if ( !inp.is_open( ) && configType != CONFIG_TYPE_MAP ) {

		error = fmt::sprintf( "Config file %s\\%s.txt doesn't exist\n", ConfigTypeToDirectoryName( configType ).c_str(), fileName );
		return false;
	}

	bool readingSection = false;

	std::string line;
	std::string fileContents = "";
	CONFIG_FILE_SECTION currentFileSection = CONFIG_FILE_SECTION_NO_SECTION;
	while ( std::getline( inp, line ) && error.size() == 0 ) {
		lineCount++;
		fileContents += line;
		line = aux::str::trim( line );

		// remove trailing comments
		line = line.substr( 0, line.find( "//" ) );

		if ( line.empty() ) {
			continue;
		}

		std::regex sectionRegex( "\\[([a-z0-9_]+)\\]" );
		std::smatch sectionName;

		if ( std::regex_match( line, sectionName, sectionRegex ) ) {

			for ( const auto &configSection : configSections ) {
				if ( configSection.second.name == sectionName.str( 1 ) ) {
					currentFileSection = configSection.first;
					break;
				}
			}

			if ( currentFileSection == CONFIG_FILE_SECTION_NO_SECTION ) {
				error = fmt::sprintf( "Error parsing %s\\%s.txt, line %d: unknown section [%s]\n", ConfigTypeToDirectoryName( configType ).c_str(), fileName, lineCount, sectionName.str( 1 ).c_str() );
				break;
			}
		} else {
			if ( currentFileSection == CONFIG_FILE_SECTION_NO_SECTION ) {

				error = fmt::sprintf( "Error parsing %s\\%s.txt, line %d: declaring section data without declared section beforehand\n", ConfigTypeToDirectoryName( configType ).c_str(), fileName, lineCount );
				break;
			}

			std::string configSectionError = configSections[currentFileSection].OnSectionData( configName, line, lineCount, configType );
			sectionDataLines++;
			if ( configSectionError.size() > 0 ) {
				error = configSectionError;
				break;
			}
		}

	}

	if ( startMap.empty() && configType != CONFIG_TYPE_MAP ) {
		error = fmt::sprintf( "Error parsing %s\\%s.txt: [start_map] section must be defined\n", ConfigTypeToDirectoryName( configType ).c_str(), fileName );
	}

	if ( error.size() > 0 ) {
		inp.close();
		return false;
	}

	sha1 = GetHash();

	const std::string recordDirectoryPath = GetGamePath() + "\\records\\";
	const std::string recordFileName = CustomGameModeConfig::ConfigTypeToGameModeCommand( configType ) + "_" + configNameSeparated.back() +  + "_" + sha1 + ".hpr";
	
	gameFinishedOnce = record.Read( recordDirectoryPath, recordFileName );

	inp.close();

	return true;
}

const std::string CustomGameModeConfig::GetHash() {
	std::ostringstream result;

	auto ignoredSections = std::vector<CONFIG_FILE_SECTION>( {
		CONFIG_FILE_SECTION_NAME,
		CONFIG_FILE_SECTION_DESCRIPTION,
		CONFIG_FILE_SECTION_SOUND,
		CONFIG_FILE_SECTION_MUSIC,
		CONFIG_FILE_SECTION_PLAYLIST,
		CONFIG_FILE_SECTION_MAX_COMMENTARY
	} );

	for ( const auto &section : configSections ) {

		// if contains ignored section
		if ( std::find( ignoredSections.begin(), ignoredSections.end(), section.first ) != ignoredSections.end() ) {
			continue;
		}

		auto sortedLines = section.second.lines;
		std::sort( sortedLines.begin(), sortedLines.end(), []( const std::string &line1, const std::string &line2 ) {
			return line1 > line2;
		} );

		for ( const auto &line : sortedLines ) {
			result << line;
		}
	}

	auto sha1 = SHA1::SHA1();
	sha1.update( result.str() );
	return sha1.final();
}

// This function is called in CHalfLifeRules constructor
// to ensure we won't get a leftover variable value in the default gamemode.
void CustomGameModeConfig::Reset() {

	for ( int section = CONFIG_FILE_SECTION_NO_SECTION + 1 ; section < CONFIG_FILE_SECTION_AUX_END - 1 ; section++ ) {
		configSections[(CONFIG_FILE_SECTION) section].lines.clear();
	}
	
	this->configName.clear();
	this->configNameSeparated.clear();
	this->error.clear();
	musicPlaylistShuffle = false;
	gameFinishedOnce = false;

	this->markedForRestart = false;
	this->hasEndMarkers = false;
		
	name = "";
	description = "";
	startPosition = { false, NAN, NAN, NAN, NAN, false };
	startMap = "";
	endMap = "";
	endTrigger = Hookable();
	forbiddenLevels.clear();
	loadout.clear();
	entityUses.clear();
	entitySpawns.clear();
	entitiesPrevented.clear();
	sounds.clear();
	maxCommentary.clear();
	music.clear();
	musicPlaylist.clear();
	musicStops.clear();
	intermissions.clear();
	timerPauses.clear();
	timerResumes.clear();
	entityRandomSpawners.clear();
	endConditions.clear();
	teleports.clear();
	entitiesToRemove.clear();
	mods.clear();
	entityReplaces.clear();
	randomModsWhitelist.clear();
	randomModsBlacklist.clear();
}

LoadoutItem::LoadoutItem( const std::vector<Argument> &args ) {
	name = args.at( 0 ).string;
	amount = args.at( 1 ).number;
}

Hookable::Hookable( const std::vector<Argument> &args ) {
	
	map = args.at( 0 ).string;
	
	auto targetArg = args.at( 1 );
	targetName = std::isnan( targetArg.number ) ? targetArg.string : "";
	modelIndex = std::isnan( targetArg.number ) ? -3 : targetArg.number;
	constant = false;

	if ( args.size() > 2 ) {
		for ( size_t i = args.size() - 1 ; i >= 2 ; i-- ) {
			if ( args.at( i ).string == "const" ) {
				constant = true;
				break;
			}
		}
	}
}

HookableWithDelay::HookableWithDelay( const std::vector<Argument> &args ) : Hookable( args ) {

	if ( args.size() > 2 ) {
		if ( !std::isnan( args.at( 2 ).number ) ) {
			delay = args.at( 2 ).number;
		}
	}
}

HookableWithTarget::HookableWithTarget( const std::vector<Argument> &args ) : Hookable( args ) {
	if ( args.size() > 2 ) {
		isModelIndex = !std::isnan( args.at( 2 ).number );
		entityName = args.at( 2 ).string;
	}
}

Sound::Sound( const std::vector<Argument> &args ) : Hookable( args ) {

	path = args.at( 2 ).string;

	for ( size_t arg = 3 ; arg < args.size() ; arg++ ) {
		 
		looping = looping || args.at( arg ).string == "looping";
		noSlowmotionEffects = noSlowmotionEffects || args.at( arg ).string == "no_slowmotion_effects";

		delay		= max( 0.1, arg == 3 ? args.at( arg ).number : delay );
		initialPos	= arg == 4 ? args.at( arg ).number : initialPos;
	}
}

void EntitySpawnData::UpdateSpawnFlags() {
	if ( name == "monster_human_grunt_shotgun" ) {
		name = "monster_human_grunt";
		weaponFlags |= ( 1 << 3 ); // HGRUNT_SHOTGUN flag
	} else if ( name == "monster_human_grunt_grenade_launcher" ) {
		name = "monster_human_grunt";
		weaponFlags |= ( 1 | ( 1 << 2 ) ); // HGRUNT_9MMAR | HGRUNT_GRENADELAUNCHER flags
	} else if ( name == "monster_sentry" ) {
		spawnFlags |= 32; // SF_MONSTER_TURRET_AUTOACTIVATE flag
	}
}

bool EntitySpawnData::DetermineBestSpawnPosition( CBasePlayer *pPlayer, bool useSeed ) {
#ifndef CLIENT_DLL

	if ( aux::str::startsWith( this->name, "monster" ) ) {
		CBaseEntity *pEntity = NULL;
		while ( ( pEntity = UTIL_FindEntityInSphere( pEntity, Vector( 0, 0, 0 ), 8192.0f ) ) != NULL ) {
			if ( std::string( STRING( pEntity->pev->classname ) ) == "player_loadsaved" ) {
				return false;
			}
		}
	}

	static std::mt19937 gen;
	static std::uniform_real_distribution<float> posDis( -4096, 4096 );
	static std::uniform_real_distribution<float> angDis( 0, 360 );

	if ( useSeed ) {
		std::string randomSpawnerSeed = std::to_string( gameplayModsData.randomSpawnerSeed );
		if ( auto configFileSeed = gameplayMods::randomSpawnerSeed.isActive<std::string>() ) {
			randomSpawnerSeed = *configFileSeed;
		}

		auto seed = std::to_string( gameplayModsData.randomSpawnerCalls ) + randomSpawnerSeed + STRING( gpGlobals->mapname );
		gen.seed( std::seed_seq( seed.begin(), seed.end() ) );

		gameplayModsData.randomSpawnerCalls++;
	}

	for ( int i = 0 ; i < 10000 ; i++ ) {
		TraceResult tr;
		char bottomTexture[256] = "(null)";
		char upperTexture[256] = "(null)";

		float x = posDis( gen );
		float y = posDis( gen );
		float z = posDis( gen );

		Vector randomPoint( x, y, z );
		sprintf( bottomTexture, "%s", g_engfuncs.pfnTraceTexture( NULL, randomPoint, randomPoint - gpGlobals->v_up * 8192 ) );
		sprintf( upperTexture, "%s", g_engfuncs.pfnTraceTexture( NULL, randomPoint, randomPoint + gpGlobals->v_up * 8192 ) );

		if ( FStrEq( bottomTexture, "(null)" ) || FStrEq( bottomTexture, "sky" ) || FStrEq( bottomTexture, "black" ) || FStrEq( upperTexture, "(null)" ) ) {
			continue;
		}

		// Player should not be out of bounds
		sprintf( bottomTexture, "%s", g_engfuncs.pfnTraceTexture( NULL, pPlayer->pev->origin, randomPoint - gpGlobals->v_up * 8192 ) );
		sprintf( upperTexture, "%s", g_engfuncs.pfnTraceTexture( NULL, pPlayer->pev->origin, randomPoint + gpGlobals->v_up * 8192 ) );
		bool playerIsOutOfBounds = FStrEq( bottomTexture, "(null)" ) || FStrEq( upperTexture, "(null)" );

		// Drop randomPoint on the floor
		UTIL_TraceLine( randomPoint, randomPoint - gpGlobals->v_up * 8192, dont_ignore_monsters, ignore_glass, pPlayer->edict(), &tr );
		if ( tr.fAllSolid ) {
			continue;
		}

		randomPoint = tr.vecEndPos;

		bool hasFaultyEntityNearby = false;

		CBaseEntity *pEntity = NULL;
		while ( ( pEntity = UTIL_FindEntityInSphere( pEntity, randomPoint, 80.0f ) ) != NULL ) {
			std::string classname = STRING( pEntity->pev->classname );
			if (
				aux::str::startsWith( classname, "monster" ) ||
				classname == "func_door" ||
				classname == "func_door_rotating" ||
				classname == "func_rot_button" ||
				classname == "func_rotating" ||
				classname == "func_train" || 
				classname == "func_tracktrain"
			) {
				hasFaultyEntityNearby = true;
				break;
			}
		}

		if ( hasFaultyEntityNearby ) {
			continue;
		}

		// Prefer not to spawn near player
		UTIL_TraceLine( pPlayer->pev->origin, randomPoint, dont_ignore_monsters, dont_ignore_glass, pPlayer->edict(), &tr );
		if ( tr.flFraction >= 1.0f && !playerIsOutOfBounds ) {
			continue;
		}

		this->x = randomPoint.x;
		this->y = randomPoint.y;
		this->z = randomPoint.z + 4;

		if ( aux::str::startsWith( this->name, "monster" ) ) {
			auto angleAndDistance = std::make_pair<float, float>( 0.0f, 0.0f );
			for ( float angle = 0.0f; angle < 360.0f; angle += ( 360.0f / 8.0f ) ) {
				UTIL_TraceLine( randomPoint, Vector( cos( angle ), sin( angle ), 0 ) * 8192, ignore_monsters, NULL, &tr );

				float distance = ( tr.vecEndPos - randomPoint ).Length();

				if ( distance > angleAndDistance.second ) {
					angleAndDistance.second = distance;
					angleAndDistance.first = angle;
				}
			}

			this->angle = angleAndDistance.first;
		} else {
			this->angle = angDis( gen );
		}

		return true;

	}

#endif
	return false;
}

EntitySpawn::EntitySpawn( const std::vector<Argument> &args ) : Hookable( args ) {
	
	entity.name = args.at( 2 ).string;
	entity.x = args.at( 3 ).number;
	entity.y = args.at( 4 ).number;
	entity.z = args.at( 5 ).number;
	entity.angle = args.size() >= 7 ? args.at( 6 ).number : 0.0f;
	entity.targetName = args.size() >= 8 ? args.at( 7 ).string : "";

	entity.UpdateSpawnFlags();
}

EntityReplace::EntityReplace( const std::vector<Argument> &args ) : Hookable( args ) {
	
	replacedEntity = args.at( 2 ).string;
	newEntity.name = args.at( 3 ).string;

	newEntity.UpdateSpawnFlags();
}

Intermission::Intermission( const std::vector<Argument>& args ) : HookableWithTarget( args ) {
	bool defined = true;
	float x = args.at( 3 ).number;
	float y = args.at( 4 ).number;
	float z = args.at( 5 ).number;
	float angle = args.at( 6 ).number;
	bool stripped = args.at( 7 ).string == "stripped";

	startPos = { defined, x, y, z, angle, stripped };
}

Teleport::Teleport( const std::vector<Argument>& args ) : Hookable( args ) {

	bool defined = true;
	float x = args.at( 2 ).number;
	float y = args.at( 3 ).number;
	float z = args.at( 4 ).number;
	float angle = args.at( 5 ).number;
	bool stripped = false;

	pos = { defined, x, y, z, angle, stripped };
}

EndCondition::EndCondition( const std::vector<Argument>& args ) : Hookable( args ) {
	activations = 0;
	activationsRequired = args.at( 2 ).number;
	constant = true;
	objective = args.at( 3 ).string;
}

std::string EndCondition::GetHash() {
	std::ostringstream result;

	result << map << modelIndex << targetName << constant << objective << activationsRequired;

	auto sha1 = SHA1::SHA1();
	sha1.update( result.str() );
	return sha1.final();
}

const std::string CustomGameModeConfig::ConfigSection::OnSectionData( const std::string &configName, const std::string &line, int lineCount, CONFIG_TYPE configType ) {

	auto args = aux::str::getCommandLineArguments( line );
	std::vector<Argument> parsedArgs = this->args;

	int amountOfParsedArguments = 0;
	
	for ( size_t i = 0 ; i < min( parsedArgs.size(), args.size() ) ; i++ ) {
		auto value = args.at( i );
		auto parsingError = parsedArgs.at( i ).Init( value );
		if ( !parsingError.empty() ) {
			return fmt::sprintf( "Error parsing %s\\%s.txt, line %d, section [%s]: %s\n", ConfigTypeToDirectoryName( configType ).c_str(), configName.c_str(), lineCount, name.c_str(), parsingError.c_str() );
		}

		amountOfParsedArguments++;
	}

	if ( amountOfParsedArguments < requiredAmountOfArguments ) {
		return fmt::sprintf( "Error parsing %s\\%s.txt, line %d, section [%s], not enough arguments:\n%s", ConfigTypeToDirectoryName( configType ).c_str(), configName.c_str(), lineCount, name.c_str(), argumentsString.c_str() );
	}
	
	std::string error = Init( parsedArgs, line );
	if ( error.size() > 0 ) {
		return fmt::sprintf( "Error parsing %s\\%s.txt, line %d, section [%s]: %s\n", ConfigTypeToDirectoryName( configType ).c_str(), configName.c_str(), lineCount, name.c_str(), error.c_str() );
	}

	lines.push_back( line );

	return "";

}

std::string Argument::Init( const std::string &value ) {
	string = value;
	if ( value == "" && default != "" ) {
		string = default;
	}

	if ( isNumber ) {
		try {
			number = std::stof( string );
			if ( !std::isnan( min ) && number < min ) {
				number = min;
			}
			if ( !std::isnan( max ) && number > max ) {
				number = max;
			}
		} catch ( std::invalid_argument e ) {
			number = NAN;

			if ( mustBeNumber ) {
				return "invalid number specified";
			}
		}
	}

	description = GetDescription( string, number );

	return "";
}