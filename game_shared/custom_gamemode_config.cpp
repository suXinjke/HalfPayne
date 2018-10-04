#include "custom_gamemode_config.h"
#include "string_aux.h"
#include <fstream>
#include <regex>
#include <sstream>
#include "sha1.h"
#include "fs_aux.h"
#include "gameplay_mod.h"
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
			Argument( "target_to_use" ).CanBeNumber()
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
			Argument( "model_index | target_name" ).CanBeNumber()
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
			Argument( "target_to_remove" ).CanBeNumber()
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
			Argument( "model_index | target_name" ).CanBeNumber()
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
			HANDLE hFind = NULL;

			if ( ( hFind = FindFirstFile( line.c_str(), &fdFile ) ) == INVALID_HANDLE_VALUE ) {
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

			bool activatedMod = false;
			for ( auto &pair : mods ) {
				auto &mod = pair.second;
				if ( mod.id == modName ) {
					mod.active = true;
					activatedMod = true;

					for ( size_t i = 0 ; i < min( mod.arguments.size(), args.size() - 1 ) ; i++ ) {
						auto parsingError = mod.arguments.at( i ).Init( args.at( i + 1 ).string );

						if ( !parsingError.empty() ) {
							return fmt::sprintf( "%s, argument %d: %s", modName.c_str(), i + 1, parsingError.c_str() );
						}
					}
					break;
				}
			}

			if ( !activatedMod ) {
				return fmt::sprintf( "incorrect mod specified: %s\n", modName.c_str() );
			}

			return std::string( "" );
		}
	);

	configSections[CONFIG_FILE_SECTION_TIMER_PAUSE] = ConfigSection(
		"timer_pause",
		{
			Argument( "map_name" ),
			Argument( "model_index | target_name" ).CanBeNumber()
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
			Argument( "model_index | target_name" ).CanBeNumber()
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
			Argument( "model_index | target_name" ).CanBeNumber(),
			Argument( "max_amount" ).IsOptional().IsNumber().MinMax( 1 ).Default( "50" ),
			Argument( "spawn_period" ).IsOptional().IsNumber().MinMax( 0.1 ).Default( "2" )
		},
		[this]( const std::vector<Argument> &args, const std::string &line ) {
			std::string entityName = args.at( 1 ).string;
			if ( GetAllowedEntityIndex( entityName.c_str() ) == -1 ) {
				return fmt::sprintf( "incorrect entity name: %s", entityName.c_str() );
			}

			EntityRandomSpawner spawner;
			spawner.mapName = args.at( 0 ).string;
			spawner.entity.name = entityName;
			spawner.maxAmount = args.at( 2 ).number;
			spawner.spawnPeriod = args.at( 3 ).number;
			spawner.entity.UpdateSpawnFlags();
				
			entityRandomSpawners.push_back( spawner );

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

	for ( const auto &commentary : maxCommentary ) {
		if ( commentary.map == map ) {
			soundsToPrecache.insert( commentary.path );
		}
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

		default:
			return "";
	}
}

std::string CustomGameModeConfig::ConfigTypeToGameModeName( bool uppercase ) {
	if ( IsGameplayModActive( GAMEPLAY_MOD_BLACK_MESA_MINUTE ) && IsGameplayModActive( GAMEPLAY_MOD_SCORE_ATTACK ) ) {
		return uppercase ? "BLACK MESA MINUTE SCORE ATTACK" : "Black Mesa Minute Score Attack";
	} else if ( IsGameplayModActive( GAMEPLAY_MOD_BLACK_MESA_MINUTE ) ) {
		return uppercase ? "BLACK MESA MINUTE" : "Black Mesa Minute";
	} else if ( IsGameplayModActive( GAMEPLAY_MOD_SCORE_ATTACK ) ) {
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

	// DUMB heuristic hack because too lazy to ensure proper replacing of \\ to / EVERYWHERE and/or making Split function support regex or several chars at once
	auto potentialConfigFileSeparated = Split( fileName, '\\' );
	auto potentialConfigFileSeparated2 = Split( fileName, '/' );
	configNameSeparated = potentialConfigFileSeparated2.size() >= potentialConfigFileSeparated.size() ? potentialConfigFileSeparated2 : potentialConfigFileSeparated;

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
		line = Trim( line );

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

	InitGameplayMods();
}

bool CustomGameModeConfig::IsGameplayModActive( GAMEPLAY_MOD mod ) {
	if ( !mods.count( mod ) ) {
		return false;
	}

	return mods[mod].active;
}

void CustomGameModeConfig::InitGameplayMods() {
	mods.clear();

	mods[GAMEPLAY_MOD_ALWAYS_GIB] = GameplayMod(
		"always_gib",
		"Always gib",
		"Kills will always try to result in gibbing.",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.gibsAlways = TRUE;
		}
	);

	mods[GAMEPLAY_MOD_BLACK_MESA_MINUTE] = GameplayMod(
		"black_mesa_minute",
		"Black Mesa Minute",
		"Time-based game mode - rush against a minute, kill enemies to get more time.",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.blackMesaMinute = true;
			gameplayMods.timerShown = true;
			gameplayMods.time = 60.0f;
			gameplayMods.timerBackwards = true;
		}
	);

	mods[GAMEPLAY_MOD_BLEEDING] = GameplayMod(
		"bleeding",
		"Bleeding",
		"After your last painkiller take, you start to lose health.",
		{
			Argument( "bleeding_handicap" ).IsOptional().MinMax( 0, 99 ).Default( "20" ).Description( []( const std::string string, float value ) {
				return "Bleeding until " + std::to_string( value ) + "%% health left\n";
			} ),
			Argument( "bleeding_update_period" ).IsOptional().MinMax( 0.01 ).Default( "1" ).Description( []( const std::string string, float value ) {
				return "Bleed update period: " + std::to_string( value ) + " sec \n";
			} ),
			Argument( "bleeding_immunity_period" ).IsOptional().MinMax( 0.05 ).Default( "10" ).Description( []( const std::string string, float value ) {
				return "Bleed again after healing in: " + std::to_string( value ) + " sec \n";
			} )
		},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.bleeding = TRUE;
			gameplayMods.bleedingHandicap = args.at( 0 ).number;
			gameplayMods.bleedingUpdatePeriod = args.at( 1 ).number;
			gameplayMods.bleedingImmunityPeriod = args.at( 2 ).number;
		}
	);

	mods[GAMEPLAY_MOD_BULLET_DELAY_ON_SLOWMOTION] = GameplayMod(
		"bullet_delay_on_slowmotion",
		"Bullet delay on slowmotion",
		"When slowmotion is activated, physical bullets shot by you will move slowly until you turn off the slowmotion.",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.bulletDelayOnSlowmotion = TRUE;
		}
	);

	mods[GAMEPLAY_MOD_BULLET_PHYSICS_DISABLED] = GameplayMod(
		"bullet_physics_disabled",
		"Bullet physics disabled",
		"Self explanatory.",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.bulletPhysicsMode = BULLET_PHYSICS_DISABLED;
		}
	);

	mods[GAMEPLAY_MOD_BULLET_PHYSICS_CONSTANT] = GameplayMod(
		"bullet_physics_constant",
		"Bullet physics constant",
		"Bullet physics is always present, even when slowmotion is NOT present.",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.bulletPhysicsMode = BULLET_PHYSICS_CONSTANT;
		}
	);

	mods[GAMEPLAY_MOD_BULLET_PHYSICS_ENEMIES_AND_PLAYER_ON_SLOWMOTION] = GameplayMod(
		"bullet_physics_enemies_and_player_on_slowmotion",
		"Bullet physics for enemies and player on slowmotion",
		"Bullet physics will be active for both enemies and player only when slowmotion is present.",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.bulletPhysicsMode = BULLET_PHYSICS_ENEMIES_AND_PLAYER_ON_SLOWMOTION;
		}
	);

	mods[GAMEPLAY_MOD_BULLET_RICOCHET] = GameplayMod( 
		"bullet_ricochet",
		"Bullet ricochet",
		"Physical bullets ricochet off the walls.",
		{
			Argument( "bullet_ricochet_count" ).MinMax( 0 ).Default( "2" ).Description( []( const std::string string, float value ) {
				return "Max ricochets: " + ( value <= 0 ? "Infinite" : std::to_string( value ) ) + "\n";
			} ),
			Argument( "bullet_ricochet_max_degree" ).MinMax( 1, 90 ).Default( "45" ).Description( []( const std::string string, float value ) {
				return "Max angle for ricochet: " + std::to_string( value ) + " deg \n";
			} ),
		},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.bulletRicochetCount = args.at( 0 ).number;
			gameplayMods.bulletRicochetMaxDotProduct = args.at( 1 ).number / 90.0f;
			// TODO
			//gameplayMods.bulletRicochetError = 5;
		}
	);

	mods[GAMEPLAY_MOD_BULLET_SELF_HARM] = GameplayMod(
		"bullet_self_harm",
		"Bullet self harm",
		"Physical bullets shot by player can harm back (ricochet mod is required).",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.bulletSelfHarm = TRUE;
		}
	);

	mods[GAMEPLAY_MOD_BULLET_TRAIL_CONSTANT] = GameplayMod(
		"bullet_trail_constant",
		"Bullet trail constant",
		"Physical bullets always get a trail, regardless if slowmotion is present.",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.bulletTrailConstant = TRUE;
		}
	);

	mods[GAMEPLAY_MOD_CONSTANT_SLOWMOTION] = GameplayMod(
		"constant_slowmotion",
		"Constant slowmotion",
		"You start in slowmotion, it's infinite and you can't turn it off.",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
#ifndef CLIENT_DLL
			player->TakeSlowmotionCharge( 100 );
			player->SetSlowMotion( true );
#endif
			gameplayMods.slowmotionInfinite = TRUE;
			gameplayMods.slowmotionConstant = TRUE;
		}
	);

	mods[GAMEPLAY_MOD_CROSSBOW_EXPLOSIVE_BOLTS] = GameplayMod(
		"crossbow_explosive_bolts",
		"Crossbow explosive bolts",
		"Crossbow bolts explode when they hit the wall.",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.crossbowExplosiveBolts = TRUE;
		}
	);

	mods[GAMEPLAY_MOD_DETACHABLE_TRIPMINES] = GameplayMod( 
		"detachable_tripmines",
		"Detachable tripmines",
		"Pressing USE button on attached tripmines will detach them.",
		{
			Argument( "detach_tripmines_instantly" ).IsOptional().Description( []( const std::string string, float value ) {
				return string == "instantly" ? "Tripmines will be INSTANTLY added to your inventory if possible" : "";
			} )
		},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.detachableTripmines = TRUE;
			gameplayMods.detachableTripminesInstantly = args.at( 0 ).string == "instantly";
		}
	);

	mods[GAMEPLAY_MOD_DIVING_ALLOWED_WITHOUT_SLOWMOTION] = GameplayMod(
		"diving_allowed_without_slowmotion",
		"Diving allowed without slowmotion",
		"You're still allowed to dive even if you have no slowmotion charge.\n"
		"In that case you will dive without going into slowmotion.",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.divingAllowedWithoutSlowmotion = TRUE;
		}
	);

	mods[GAMEPLAY_MOD_DIVING_ONLY] = GameplayMod(
		"diving_only",
		"Diving only",
		"The only way to move around is by diving.\n"
		"This enables Infinite slowmotion by default.\n"
		"You can dive even when in crouch-like position, like when being in vents etc.",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.divingOnly = TRUE;
			gameplayMods.slowmotionInfinite = TRUE;
		}
	);

	mods[GAMEPLAY_MOD_DRUNK_AIM] = GameplayMod( 
		"drunk_aim",
		"Drunk aim",
		"Your aim becomes wobbly.",
		{
			Argument( "max_horizontal_wobble" ).IsOptional().MinMax( 0, 25.5 ).Default( "20" ).Description( []( const std::string string, float value ) {
				return "Max horizontal wobble: " + std::to_string( value ) + " deg\n";
			} ),
			Argument( "max_vertical_wobble" ).IsOptional().MinMax( 0, 25.5 ).Default( "5" ).Description( []( const std::string string, float value ) {
				return "Max vertical wobble: " + std::to_string( value ) + " deg\n";
			} ),
			Argument( "wobble_frequency" ).IsOptional().MinMax( 0.01 ).Default( "1" ).Description( []( const std::string string, float value ) {
				return "Wobble frequency: " + std::to_string( value ) + "\n";
			} ),
		},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) { 
			gameplayMods.aimOffsetMaxX = args.at( 0 ).number;
			gameplayMods.aimOffsetMaxY = args.at( 1 ).number;
			gameplayMods.aimOffsetChangeFreqency = args.at( 2 ).number;
		}
	);

	mods[GAMEPLAY_MOD_DRUNK_LOOK] = GameplayMod( 
		"drunk_look",
		"Drunk look",
		"Camera view becomes wobbly and makes aim harder.",
		{
			Argument( "drunkiness" ).IsOptional().MinMax( 0.1, 100 ).Default( "25" ).Description( []( const std::string string, float value ) {
				return "Drunkiness: " + std::to_string( value ) + "%%\n";
			} ),
		},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.drunkiness = ( args.at( 0 ).number / 100.0f ) * 255;
		}
	);

	mods[GAMEPLAY_MOD_EASY] = GameplayMod(
		"easy",
		"Easy difficulty",
		"Sets up easy level of difficulty.",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {}
	);

	mods[GAMEPLAY_MOD_EDIBLE_GIBS] = GameplayMod(
		"edible_gibs",
		"Edible gibs",
		"Allows you to eat gibs by pressing USE when aiming at the gib, which restore your health by 5.",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.gibsEdible = TRUE;
		}
	);

	mods[GAMEPLAY_MOD_EMPTY_SLOWMOTION] = GameplayMod(
		"empty_slowmotion",
		"Empty slowmotion",
		"Start with no slowmotion charge.",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {}
	);

	mods[GAMEPLAY_MOD_FADING_OUT] = GameplayMod( 
		"fading_out",
		"Fading out",
		"View is fading out, or in other words it's blacking out until you can't see almost anything.\n"
		"Take painkillers to restore the vision.\n"
		"Allows to take painkillers even when you have 100 health and enough time have passed since the last take.",
		{
			Argument( "fade_out_percent" ).IsOptional().MinMax( 0, 100 ).Default( "90" ).Description( []( const std::string string, float value ) {
				return "Fade out intensity: " + std::to_string( value ) + "%%\n";
			} ),
			Argument( "fade_out_update_period" ).IsOptional().MinMax( 0.01 ).Default( "0.5" ).Description( []( const std::string string, float value ) {
				return "Fade out update period: " + std::to_string( value ) + " sec \n";
			} )
		},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.fadeOut = TRUE;
			gameplayMods.fadeOutThreshold = 255 - ( args.at( 0 ).number / 100.0f ) * 255;
			gameplayMods.fadeOutUpdatePeriod = args.at( 1 ).number;
		}
	);

	mods[GAMEPLAY_MOD_FRICTION] = GameplayMod( 
		"friction",
		"Friction",
		"Changes player's friction.",
		{
			Argument( "friction" ).IsOptional().MinMax( 0 ).Default( "4" ).Description( []( const std::string string, float value ) {
				return "Friction: " + std::to_string( value ) + "\n";
			} )
		},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			// TODO: MOVE TO GAMEPLAY MODS
			gameplayMods.frictionOverride = args.at( 0 ).number;
		}
	);

	mods[GAMEPLAY_MOD_GARBAGE_GIBS] = GameplayMod(
		"garbage_gibs",
		"Garbage gibs",
		"Replaces all gibs with garbage.",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.gibsGarbage = TRUE;
		}
	);

	mods[GAMEPLAY_MOD_GOD] = GameplayMod(
		"god",
		"God mode",
		"You are invincible and it doesn't count as a cheat.",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.godConstant = TRUE;
		}
	);

	mods[GAMEPLAY_MOD_HARD] = GameplayMod(
		"hard",
		"Hard difficulty",
		"Sets up hard level of difficulty.",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {}
	);

	mods[GAMEPLAY_MOD_HEADSHOTS] = GameplayMod(
		"headshots",
		"Headshots",
		"Headshots dealt to enemies become much more deadly.",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {}
	);

	mods[GAMEPLAY_MOD_INFINITE_AMMO] = GameplayMod(
		"infinite_ammo",
		"Infinite ammo",
		"All weapons get infinite ammo.",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.infiniteAmmo = TRUE;
		}
	);

	mods[GAMEPLAY_MOD_INFINITE_AMMO_CLIP] = GameplayMod(
		"infinite_ammo_clip",
		"Infinite ammo clip",
		"Most weapons get an infinite ammo clip and need no reloading.",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.infiniteAmmoClip = TRUE;
		}
	);

	mods[GAMEPLAY_MOD_INFINITE_PAINKILLERS] = GameplayMod(
		"infinite_painkillers",
		"Infinite painkillers",
		"Self explanatory.",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.painkillersInfinite = TRUE;
		}
	);

	mods[GAMEPLAY_MOD_INFINITE_SLOWMOTION] = GameplayMod(
		"infinite_slowmotion",
		"Infinite slowmotion",
		"You have infinite slowmotion charge and it's not considered as cheat.",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
#ifndef CLIENT_DLL
			player->TakeSlowmotionCharge( 100 );
#endif
			gameplayMods.slowmotionInfinite = TRUE;
		}
	);

	mods[GAMEPLAY_MOD_INITIAL_CLIP_AMMO] = GameplayMod( 
		"initial_clip_ammo",
		"Initial ammo clip",
		"All weapons will have specified ammount of ammo in the clip when first picked up.",
		{
			Argument( "initial_clip_ammo" ).IsOptional().MinMax( 1 ).Default( "4" ).Description( []( const std::string string, float value ) {
				return "Initial ammo in the clip: " + std::to_string( value ) + "\n";
			} )
		},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.initialClipAmmo = args.at( 0 ).number;
		}
	);

	mods[GAMEPLAY_MOD_INSTAGIB] = GameplayMod(
		"instagib",
		"Instagib",
		"Gauss Gun becomes much more deadly with 9999 damage, also gets red beam and slower rate of fire.\n"
		"More gibs come out.",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.instaGib = TRUE;
		}
	);

	mods[GAMEPLAY_MOD_HEALTH_REGENERATION] = GameplayMod( 
		"health_regeneration",
		"Health regeneration",
		"Allows for health regeneration options.",
		{
			Argument( "regeneration_max" ).IsOptional().MinMax( 0 ).Default( "20" ).Description( []( const std::string string, float value ) {
				return "Regenerate up to: " + std::to_string( value ) + " HP\n";
			} ),
			Argument( "regeneration_delay" ).IsOptional().MinMax( 0 ).Default( "3" ).Description( []( const std::string string, float value ) {
				return std::to_string( value ) + " sec after last damage\n";
			} ),
			Argument( "regeneration_frequency" ).IsOptional().MinMax( 0.01 ).Default( "0.2" ).Description( []( const std::string string, float value ) {
				return "Regenerating: " + std::to_string( 1 / value ) + " HP/sec\n";
			} )
		},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			player->pev->max_health = args.at( 0 ).number;
			gameplayMods.regenerationMax = args.at( 0 ).number;
			gameplayMods.regenerationDelay = args.at( 1 ).number;
			gameplayMods.regenerationFrequency = args.at( 2 ).number;
		}
	);

	mods[GAMEPLAY_MOD_HEAL_ON_KILL] = GameplayMod(
		"heal_on_kill",
		"Heal on kill",
		"Your health will be replenished after killing an enemy.",
		{
			Argument( "max_health_taken_percent" ).IsOptional().MinMax( 1, 5000 ).Default( "25" ).Description( []( const std::string string, float value ) {
				return "Victim's max health taken after kill: " + std::to_string( value ) + "%%\n";
			} ),
		},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.healOnKill = TRUE;
			gameplayMods.healOnKillMultiplier = args.at( 0 ).number / 100.0f;
		}
	);

	mods[GAMEPLAY_MOD_NO_FALL_DAMAGE] = GameplayMod(
		"no_fall_damage",
		"No fall damage",
		"Self explanatory.",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.noFallDamage = TRUE;
		}
	);

	mods[GAMEPLAY_MOD_NO_MAP_MUSIC] = GameplayMod(
		"no_map_music",
		"No map music",
		"Music which is defined by map will not be played.\nOnly the music defined in gameplay config files will play.",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.noMapMusic = TRUE;
		}
	);

	mods[GAMEPLAY_MOD_NO_HEALING] = GameplayMod(
		"no_healing",
		"No healing",
		"Don't allow to heal in any way, including Xen healing pools.",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.noHealing = TRUE;
		}
	);

	mods[GAMEPLAY_MOD_NO_JUMPING] = GameplayMod(
		"no_jumping",
		"No jumping",
		"Don't allow to jump.",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.noJumping = TRUE;
		}
	);

	mods[GAMEPLAY_MOD_NO_PILLS] = GameplayMod(
		"no_pills",
		"No painkillers",
		"Don't allow to take painkillers.",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.painkillersForbidden = TRUE;
		}
	);

	mods[GAMEPLAY_MOD_NO_SAVING] = GameplayMod(
		"no_saving",
		"No saving",
		"Don't allow to load saved files.",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.noSaving = TRUE;
		}
	);

	mods[GAMEPLAY_MOD_NO_SECONDARY_ATTACK] = GameplayMod(
		"no_secondary_attack",
		"No secondary attack",
		"Disables the secondary attack on all weapons.",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.noSecondaryAttack = TRUE;
		}
	);

	mods[GAMEPLAY_MOD_NO_SLOWMOTION] = GameplayMod(
		"no_slowmotion",
		"No slowmotion",
		"You're not allowed to use slowmotion.",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.slowmotionForbidden = TRUE;
		}
	);

	mods[GAMEPLAY_MOD_NO_SMG_GRENADE_PICKUP] = GameplayMod(
		"no_smg_grenade_pickup",
		"No SMG grenade pickup",
		"You're not allowed to pickup and use SMG (MP5) grenades.",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.noSmgGrenadePickup = TRUE;
		}
	);

	mods[GAMEPLAY_MOD_NO_TARGET] = GameplayMod(
		"no_target",
		"No target",
		"You are invisible to everyone and it doesn't count as a cheat.",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.noTargetConstant = TRUE;
		}
	);

	mods[GAMEPLAY_MOD_NO_WALKING] = GameplayMod(
		"no_walking",
		"No walking",
		"Don't allow to walk, swim, dive, climb ladders.",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.noWalking = TRUE;
		}
	);

	mods[GAMEPLAY_MOD_ONE_HIT_KO] = GameplayMod(
		"one_hit_ko",
		"One hit KO",
		"Any hit from an enemy will kill you instantly.\n"
		"You still get proper damage from falling and environment.",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.oneHitKO = TRUE;
		}
	);

	mods[GAMEPLAY_MOD_ONE_HIT_KO_FROM_PLAYER] = GameplayMod(
		"one_hit_ko_from_player",
		"One hit KO from player",
		"All enemies die in one hit.",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.oneHitKOFromPlayer = TRUE;
		}
	);

	mods[GAMEPLAY_MOD_PREVENT_MONSTER_MOVEMENT] = GameplayMod(
		"prevent_monster_movement",
		"Prevent monster movement",
		"Monsters will always stay at spawn spot.",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.preventMonsterMovement = TRUE;
		}
	);

	mods[GAMEPLAY_MOD_PREVENT_MONSTER_SPAWN] = GameplayMod(
		"prevent_monster_spawn",
		"Prevent monster spawn",
		"Don't spawn predefined monsters (NPCs) when visiting a new map.\n"
		"This doesn't affect dynamic monster_spawners.",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.preventMonsterSpawn = TRUE;
		}
	);

	mods[GAMEPLAY_MOD_PREVENT_MONSTER_STUCK_EFFECT] = GameplayMod(
		"prevent_monster_stuck_effect",
		"Prevent monster stuck effect",
		"If monsters get stuck after spawning, they won't have the usual yellow particles effect.",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.preventMonsterStuckEffect = TRUE;
		}
	);

	mods[GAMEPLAY_MOD_PREVENT_MONSTER_DROPS] = GameplayMod(
		"prevent_monster_drops",
		"Prevent monster drops",
		"Monsters won't drop anything when dying.",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.preventMonsterDrops = TRUE;
		}
	);

	mods[GAMEPLAY_MOD_SCORE_ATTACK] = GameplayMod(
		"score_attack",
		"Score attack",
		"Kill enemies to get as much score as possible. Build combos to get even more score.",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.scoreAttack = true;
		}
	);

	mods[GAMEPLAY_MOD_SHOTGUN_AUTOMATIC] = GameplayMod(
		"shotgun_automatic",
		"Automatic shotgun",
		"Shotgun only fires single shots and doesn't have to be reloaded after each shot.",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.automaticShotgun = true;
		}
	);

	mods[GAMEPLAY_MOD_SHOW_TIMER] = GameplayMod(
		"show_timer",
		"Show timer",
		"Timer will be shown. Time is affected by slowmotion.",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.timerShown = TRUE;
		}
	);

	mods[GAMEPLAY_MOD_SHOW_TIMER_REAL_TIME] = GameplayMod(
		"show_timer_real_time",
		"Show timer with real time",
		"Time will be shown and it's not affected by slowmotion, which is useful for speedruns.",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.timerShown = TRUE;
			gameplayMods.timerShowReal = TRUE;
		}
	);

	mods[GAMEPLAY_MOD_SLOWMOTION_FAST_WALK] = GameplayMod(
		"slowmotion_fast_walk",
		"Fast walk in slowmotion",
		"You still walk and run almost as fast as when slowmotion is not active.",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.slowmotionFastWalk = TRUE;
		}
	);

	mods[GAMEPLAY_MOD_SLOWMOTION_ON_DAMAGE] = GameplayMod(
		"slowmotion_on_damage",
		"Slowmotion on damage",
		"You get slowmotion charge when receiving damage.",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.slowmotionOnDamage = TRUE;
		}
	);

	mods[GAMEPLAY_MOD_SLOWMOTION_ONLY_DIVING] = GameplayMod(
		"slowmotion_only_diving",
		"Slowmotion only when diving",
		"You're allowed to go into slowmotion only by diving.",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.slowmotionOnlyDiving = TRUE;
		}
	);

	mods[GAMEPLAY_MOD_SLOW_PAINKILLERS] = GameplayMod( 
		"slow_painkillers",
		"Slow painkillers",
		"Painkillers take time to have an effect, like in original Max Payne.",
		{
			Argument( "healing_period" ).IsOptional().MinMax( 0.01 ).Default( "0.2" ).Description( []( const std::string string, float value ) {
				return "Healing period " + std::to_string( value ) + " sec\n";
			} )
		},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.painkillersSlow = TRUE;
			player->nextPainkillerEffectTimePeriod = args.at( 0 ).number;
		}
	);

	mods[GAMEPLAY_MOD_SNARK_FRIENDLY_TO_ALLIES] = GameplayMod(
		"snark_friendly_to_allies",
		"Snarks friendly to allies",
		"Snarks won't attack player's allies.",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.snarkFriendlyToAllies = TRUE;
		}
	);

	mods[GAMEPLAY_MOD_SNARK_FRIENDLY_TO_PLAYER] = GameplayMod(
		"snark_friendly_to_player",
		"Snarks friendly to player",
		"Snarks won't attack player.",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.snarkFriendlyToPlayer = true;
		}
	);

	mods[GAMEPLAY_MOD_SNARK_FROM_EXPLOSION] = GameplayMod(
		"snark_from_explosion",
		"Snark from explosion",
		"Snarks will spawn in the place of explosion.",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.snarkFromExplosion = TRUE;
		}
	);

	mods[GAMEPLAY_MOD_SNARK_INCEPTION] =  GameplayMod( 
		"snark_inception",
		"Snark inception",
		"Killing snark splits it into two snarks.",
		{
			Argument( "inception_depth" ).IsOptional().MinMax( 1, 100 ).Default( "10" ).Description( []( const std::string string, float value ) {
				return "Inception depth: " + std::to_string( value ) + " snarks\n";
			} )
		},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.snarkInception = TRUE;
			gameplayMods.snarkInceptionDepth = args.at( 0 ).number;
		}
	);

	mods[GAMEPLAY_MOD_SNARK_INFESTATION] = GameplayMod(
		"snark_infestation",
		"Snark infestation",
		"Snark will spawn in the body of killed monster (NPC).\n"
		"Even more snarks spawn if monster's corpse has been gibbed.",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.snarkInfestation = TRUE;
		}
	);

	mods[GAMEPLAY_MOD_SNARK_NUCLEAR] = GameplayMod(
		"snark_nuclear",
		"Snark nuclear",
		"Killing snark produces a grenade-like explosion.",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.snarkNuclear = TRUE;
		}
	);

	mods[GAMEPLAY_MOD_SNARK_PENGUINS] = GameplayMod(
		"snark_penguins",
		"Snark penguins",
		"Replaces snarks with penguins from Opposing Force.\n",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.snarkPenguins = TRUE;
		}
	);

	mods[GAMEPLAY_MOD_SNARK_STAY_ALIVE] = GameplayMod(
		"snark_stay_alive",
		"Snark stay alive",
		"Snarks will never die on their own, they must be shot.",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.snarkStayAlive = TRUE;
		}
	);

	mods[GAMEPLAY_MOD_STARTING_HEALTH] = GameplayMod( 
		"starting_health",
		"Starting Health",
		"Start with specified health amount.",
		{
			Argument( "health_amount" ).IsOptional().MinMax( 1 ).Default( "100" ).Description( []( const std::string string, float value ) {
				return "Health amount: " + std::to_string( value ) + "\n";
			} )
		},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			player->pev->health = args.at( 0 ).number;
		}
	);

	mods[GAMEPLAY_MOD_SUPERHOT] = GameplayMod(
		"superhot",
		"SUPERHOT",
		"Time moves forward only when you move around.\n"
		"Inspired by the game SUPERHOT.",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.superHot = TRUE;
		}
	);

	mods[GAMEPLAY_MOD_SWEAR_ON_KILL] = GameplayMod(
		"swear_on_kill",
		"Swear on kill",
		"Max will swear when killing an enemy. He will still swear even if Max's commentary is turned off.",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.swearOnKill = TRUE;
		}
	);

	mods[GAMEPLAY_MOD_UPSIDE_DOWN] = GameplayMod(
		"upside_down",
		"Upside down",
		"View becomes turned on upside down.",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.upsideDown = TRUE;
		}
	);

	mods[GAMEPLAY_MOD_TOTALLY_SPIES] = GameplayMod(
		"totally_spies",
		"Totally spies",
		"Replaces all HGrunts with Black Ops.",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.totallySpies = TRUE;
		}
	);

	mods[GAMEPLAY_MOD_TELEPORT_MAINTAIN_VELOCITY] = GameplayMod(
		"teleport_maintain_velocity",
		"Teleport maintain velocity",
		"Your velocity will be preserved after going through teleporters.",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.teleportMaintainVelocity = TRUE;
		}
	);

	mods[GAMEPLAY_MOD_TELEPORT_ON_KILL] = GameplayMod( 
		"teleport_on_kill",
		"Teleport on kill",
		"You will be teleported to the enemy you kill.",
		{
			Argument( "teleport_weapon" ).IsOptional().Description( []( const std::string string, float value ) {
				return "Weapon that causes teleport: " + ( string.empty() ? "any" : string ) + "\n";
			} )
		},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.teleportOnKill = TRUE;

			std::string weapon = args.at( 0 ).string;
			if ( !weapon.empty() ) {
				snprintf( gameplayMods.teleportOnKillWeapon, 64, "%s", weapon.c_str() );
			}
		}
	);

	mods[GAMEPLAY_MOD_TIME_RESTRICTION] = GameplayMod( 
		"time_restriction",
		"Time restriction",
		"You are killed if time runs out.",
		{
			Argument( "time" ).IsOptional().MinMax( 1 ).Default( "60" ).Description( []( const std::string string, float value ) {
				return std::to_string( value ) + " seconds\n";
			} ),
		},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.timerShown = TRUE;
			gameplayMods.timerBackwards = TRUE;
			gameplayMods.time = args.at( 0 ).number;
		}
	);

	mods[GAMEPLAY_MOD_VVVVVV] = GameplayMod(
		"vvvvvv",
		"VVVVVV",
		"Pressing jump reverses gravity for player.\n"
		"Inspired by the game VVVVVV.",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.vvvvvv = TRUE;
		}
	);

	mods[GAMEPLAY_MOD_WEAPON_IMPACT] = GameplayMod( 
		"weapon_impact",
		"Weapon impact",
		"Taking damage means to be pushed back",
		{
			Argument( "impact" ).IsOptional().MinMax( 1 ).Default( "1" ).Description( []( const std::string string, float value ) {
				return "Impact multiplier: " + std::to_string( value ) + "\n";
			} ),
		},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.weaponImpact = args.at( 0 ).number;
		}
	);

	mods[GAMEPLAY_MOD_WEAPON_PUSH_BACK] = GameplayMod( 
		"weapon_push_back",
		"Weapon push back",
		"Shooting weapons pushes you back.",
		{
			Argument( "weapon_push_back_multiplier" ).IsOptional().MinMax( 0.1 ).Default( "1" ).Description( []( const std::string string, float value ) {
				return "Push back multiplier: " + std::to_string( value ) + "\n";
			} ),
		},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.weaponPushBack = true;
			gameplayMods.weaponPushBackMultiplier = args.at( 0 ).number;
		}
	);

	mods[GAMEPLAY_MOD_WEAPON_RESTRICTED] = GameplayMod(
		"weapon_restricted",
		"Weapon restricted",
		"If you have no weapons - you are allowed to pick only one.\n"
		"You can have several weapons at once if they are specified in [loadout] section.\n"
		"Weapon stripping doesn't affect you.",
		{},
		[]( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.weaponRestricted = TRUE;
		}
	);
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

EntitySpawn::EntitySpawn( const std::vector<Argument> &args ) : Hookable( args ) {
	
	entity.name = args.at( 2 ).string;
	entity.x = args.at( 3 ).number;
	entity.y = args.at( 4 ).number;
	entity.z = args.at( 5 ).number;
	entity.angle = args.size() >= 7 ? args.at( 6 ).number : 0.0f;
	entity.targetName = args.size() >= 8 ? args.at( 7 ).string : "";

	entity.UpdateSpawnFlags();
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

const std::string CustomGameModeConfig::ConfigSection::OnSectionData( const std::string &configName, const std::string &line, int lineCount, CONFIG_TYPE configType ) {

	auto args = NaiveCommandLineParse( line );
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