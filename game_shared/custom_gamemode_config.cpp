#include "custom_gamemode_config.h"
#include "string_aux.h"
#include <fstream>
#include <regex>
#include <sstream>
#include "sha1.h"
#include "fs_aux.h"

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

void CustomGameModeConfig::OnError( std::string message ) {
	error = message;
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
		"name", true,
		[this]( ConfigSectionData &data ) {
			std::string sanitizedName = data.line;
			if ( sanitizedName.size() > 54 ) {
				sanitizedName = sanitizedName.substr( 0, 53 );
			}

			name = sanitizedName;

			return "";
		}
	);

	configSections[CONFIG_FILE_SECTION_DESCRIPTION] = ConfigSection(
		"description", false,
		[this]( ConfigSectionData &data ) {
			if ( data.line.size() == 0 ) {
				return "description not provided";
			}

			description = data.line;

			return "";
		}
	);

	configSections[CONFIG_FILE_SECTION_START_MAP] = ConfigSection(
		"start_map", true,
		[this]( ConfigSectionData &data ) {
			if ( data.line.size() == 0 ) {
				return "start map not provided";
			}

			startMap = data.line;
			
			return "";
		}
	);

	configSections[CONFIG_FILE_SECTION_START_POSITION] = ConfigSection(
		"start_position", true,
		[this]( ConfigSectionData &data ) {

			if ( data.argsFloat.size() < 3 ) {
				return std::string( "not enough coordinates provided" );
			}

			for ( size_t i = 0 ; i < min( data.argsFloat.size(), 4 ) ; i++ ) {
				float arg = data.argsFloat.at( i );
				if ( std::isnan( arg ) ) {
					char error[1024];
					sprintf_s( error, "invalid coordinate by index %d", i + 1 );
					return std::string( error );
				}
			}

			float x = data.argsFloat.at( 0 );
			float y = data.argsFloat.at( 1 );
			float z = data.argsFloat.at( 2 );
			float angle = data.argsFloat.size() > 3 ? data.argsFloat.at( 3 ) : NAN;

			startPosition = { true, x, y, z, angle, false };

			return std::string( "" );
		}
	);

	configSections[CONFIG_FILE_SECTION_END_MAP] = ConfigSection(
		"end_map", true,
		[this]( ConfigSectionData &data ) {
			if ( data.line.size() == 0 ) {
				return "end map not provided";
			}

			endMap = data.line;
			return "";
		}
	);

	configSections[CONFIG_FILE_SECTION_END_TRIGGER] = ConfigSection(
		"end_trigger", false,
		[this]( ConfigSectionData &data ) {
			if ( data.argsString.size() < 2 ) {
				return "<mapname> <modelindex | targetname> not specified";
			}

			FillHookable( endTrigger, data );

			return "";
		}
	);

	configSections[CONFIG_FILE_SECTION_CHANGE_LEVEL_PREVENT] = ConfigSection(
		"change_level_prevent", false,
		[this]( ConfigSectionData &data ) {
			forbiddenLevels.insert( data.line );
			return "";
		}
	);

	configSections[CONFIG_FILE_SECTION_LOADOUT] = ConfigSection(
		"loadout", false,
		[this]( ConfigSectionData &data ) {
			
			std::string itemName = data.argsString.at( 0 );
			int amount = 1;
			if ( GetAllowedItemIndex( itemName.c_str() ) == -1 ) {
				char error[1024];
				sprintf_s( error, "incorrect loadout item name: %s", itemName.c_str() );
				return std::string( error );
			}

			if ( data.argsFloat.size() >= 2 ) {
				if ( std::isnan( data.argsFloat.at( 1 ) ) ) {
					return std::string( "loadout item count incorrectly specified" );
				} else {
					amount = max( 1.0f, data.argsFloat.at( 1 ) );
				}
			}

			// for some reason initializer list doesn't work here
			LoadoutItem item;
			item.name = itemName;
			item.amount = amount;

			loadout.push_back( item );

			return std::string( "" );
		}
	);

	configSections[CONFIG_FILE_SECTION_ENTITY_SPAWN] = ConfigSection(
		"entity_spawn", false,
		[this]( ConfigSectionData &data ) {

			if ( data.argsString.size() < 6 ) {
				return std::string( "<mapname> <modelindex | targetname> <entity_name> <x> <y> <z> [angle] [target_name] [const] not specified" );
			}

			std::string entityName = data.argsString.at( 2 );

			if ( GetAllowedEntityIndex( entityName.c_str() ) == -1 ) {
				char errorCString[1024];
				sprintf_s( errorCString, "incorrect entity name" );
				return std::string( errorCString );
			}

			if ( entityName == "end_marker" ) {
				hasEndMarkers = true;
			}

			for ( size_t i = 3 ; i < min( max( 5, data.argsFloat.size() ), 6 ); i++ ) {
				float arg = data.argsFloat.at( i );
				if ( std::isnan( arg ) ) {
					char error[1024];
					sprintf_s( error, "invalid coordinate by index %d", i - 2 );
					return std::string( error );
				}
			}

			entitiesToPrecache.insert( entityName );

			EntitySpawn spawn;
			FillEntitySpawn( spawn, data );
			entitySpawns.push_back( spawn );

			return std::string( "" );
		}
	);

	configSections[CONFIG_FILE_SECTION_ENTITY_USE] = ConfigSection(
		"entity_use", false,
		[this]( ConfigSectionData &data ) {
			if ( data.argsString.size() < 3 ) {
				return "<mapname> <modelindex | targetname> <modelindex | targetname> not specified";
			}

			HookableWithTarget entityUse;
			FillHookableWithTarget( entityUse, data );
			entityUses.push_back( entityUse );

			return "";
		}
	);

	configSections[CONFIG_FILE_SECTION_ENTITY_PREVENT] = ConfigSection(
		"entity_prevent", false,
		[this]( ConfigSectionData &data ) {
			if ( data.argsString.size() < 2 ) {
				return "<mapname> <modelindex | targetname> not specified";
			}

			Hookable entityPrevent;
			FillHookable( entityPrevent, data );
			entitiesPrevented.push_back( entityPrevent );

			return "";
		}
	);

	configSections[CONFIG_FILE_SECTION_ENTITY_REMOVE] = ConfigSection(
		"entity_remove", false,
		[this]( ConfigSectionData &data ) {
			if ( data.argsString.size() < 3 ) {
				return "<mapname> <modelindex | targetname> <modelindex | targetname> not specified";
			}

			HookableWithTarget entityToRemove;
			FillHookableWithTarget( entityToRemove, data );
			entitiesToRemove.push_back( entityToRemove );

			return "";
		}
	);

	configSections[CONFIG_FILE_SECTION_SOUND] = ConfigSection(
		"sound", false,
		[this]( ConfigSectionData &data ) {
			if ( data.argsString.size() < 3 ) {
				return "<mapname> <modelindex | targetname> <sound_path> [delay] [const] not specified";
			}

			if ( data.argsFloat.size() >= 4 ) {
				float arg = data.argsFloat.at( 3 );
				if ( std::isnan( arg ) ) {
					return "delay incorrectly specified";
				}
			}

			Sound sound;
			FillHookableSound( sound, data );
			sounds.push_back( sound );

			soundsToPrecache.insert( sound.path );

			return "";
		}
	);

	configSections[CONFIG_FILE_SECTION_MUSIC] = ConfigSection(
		"music", false,
		[this]( ConfigSectionData &data ) {
			if ( data.argsString.size() < 3 ) {
				return "<mapname> <modelindex | targetname> <sound_path> [delay] [initial_pos] [const] [looping] [no_slowmotion_effects] not specified";
			}

			Sound music;
			FillHookableSound( music, data );
			this->music.push_back( music );

			return "";
		}
	);

	configSections[CONFIG_FILE_SECTION_PLAYLIST] = ConfigSection(
		"playlist", false,
		[this]( ConfigSectionData &data ) {
			const std::string &line = data.line;

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
					Sound music;
					music.Reset();
					music.path = file;
					musicPlaylist.push_back( music );
				}
			}

			return "";
		}
	);

	configSections[CONFIG_FILE_SECTION_MAX_COMMENTARY] = ConfigSection(
		"max_commentary", false,
		[this]( ConfigSectionData &data ) {
			if ( data.argsString.size() < 3 ) {
				return "<mapname> <modelindex | targetname> <sound_path> [delay] [const] not specified";
			}

			if ( data.argsFloat.size() >= 4 ) {
				float arg = data.argsFloat.at( 3 );
				if ( std::isnan( arg ) ) {
					return "delay incorrectly specified";
				}
			}

			Sound commentary;
			FillHookableSound( commentary, data );
			commentary.noSlowmotionEffects = true;
			maxCommentary.push_back( commentary );

			soundsToPrecache.insert( commentary.path );

			return "";
		}
	);

	configSections[CONFIG_FILE_SECTION_MODS] = ConfigSection(
		"mods", false,
		[this]( ConfigSectionData &data ) { 
			if ( !AddGameplayMod( data ) ) {
				char errorCString[1024];
				sprintf_s( errorCString, "incorrect mod specified: %s\n", data.line.c_str() );
				return std::string( errorCString );
			}

			return std::string( "" );
		}
	);

	configSections[CONFIG_FILE_SECTION_TIMER_PAUSE] = ConfigSection(
		"timer_pause", false,
		[this]( ConfigSectionData &data ) {
			if ( data.argsString.size() < 2 ) {
				return "<mapname> <modelindex | targetname> not specified";
			}

			Hookable timerPause;
			FillHookable( timerPause, data );
			timerPauses.push_back( timerPause );
		}
	);

	configSections[CONFIG_FILE_SECTION_TIMER_RESUME] = ConfigSection(
		"timer_resume", false,
		[this]( ConfigSectionData &data ) {
			if ( data.argsString.size() < 2 ) {
				return "<mapname> <modelindex | targetname> not specified";
			}

			Hookable timerResume;
			FillHookable( timerResume, data );
			timerResumes.push_back( timerResume );
		}
	);

	configSections[CONFIG_FILE_SECTION_INTERMISSION] = ConfigSection(
		"intermission", false,
		[this]( ConfigSectionData &data ) {
			if ( data.argsString.size() < 3 ) {
				return std::string( "<map_name> <model_index | target_name | next_map_name> <real_next_map_name> [x] [y] [z] [angle] [stripped] not specified" );
			}

			bool defined = true;
			float x = NAN;
			float y = NAN;
			float z = NAN;
			float angle = NAN;
			bool stripped = data.argsString.back() == "stripped";

			Intermission intermission;
			FillHookableWithTarget( intermission, data );

			for ( size_t i = 3 ; i < min( data.argsFloat.size(), 6 ) ; i++ ) {
				float arg = data.argsFloat.at( i );
				if ( std::isnan( arg ) ) {
					char error[1024];
					sprintf_s( error, "invalid coordinate by index %d", i + 1 );
					return std::string( error );
				}

				x		= i == 3 ? arg : x;
				y		= i == 4 ? arg : y;
				z		= i == 5 ? arg : z;
				angle	= i == 6 ? arg : angle;
			}

			intermission.startPos = { defined, x, y, z, angle, stripped };

			intermissions.push_back( intermission );

			return std::string( "" );
		}
	);

	configSections[CONFIG_FILE_SECTION_ENTITY_RANDOM_SPAWNER] = ConfigSection(
		"entity_random_spawner", false,
		[this]( ConfigSectionData &data ) {
			for ( const auto &line : data.argsString ) {
				if ( data.argsString.size() < 2 ) {
					return std::string( "<map_name | everywhere> <entity_name> [max_amount] [spawn_period]" );
				}

				std::string entityName = data.argsString.at( 1 );
				int maxAmount = 50;
				float spawnPeriod = 2.0f;
				if ( GetAllowedEntityIndex( entityName.c_str() ) == -1 ) {
					return std::string( "incorrect entity name" );
				}

				for ( size_t i = 2 ; i < min( data.argsFloat.size(), 4 ) ; i++ ) {
					float arg = data.argsFloat.at( i );
					if ( std::isnan( arg ) ) {
						char error[1024];
						if ( i == 2 ) {
							sprintf_s( error, "invalid max_amount value" );
						}
						if ( i == 3 ) {
							sprintf_s( error, "invalid spawn_period value" );
						}
						return std::string( error );
					}

					maxAmount	= i == 2 ? arg : maxAmount;
					spawnPeriod = i == 3 ? arg : spawnPeriod;
				}

				entitiesToPrecache.insert( entityName );

				EntityRandomSpawner spawner;
				spawner.mapName = data.argsString.at( 0 );
				spawner.entityName = entityName;
				spawner.maxAmount = maxAmount;
				spawner.spawnPeriod = spawnPeriod;

				entityRandomSpawners.push_back( spawner );
			}
			return std::string( "" );
		}
	);

	configSections[CONFIG_FILE_SECTION_END_CONDITIONS] = ConfigSection(
		"end_conditions", false,
		[this]( ConfigSectionData &data ) {
			for ( const auto &line : data.argsString ) {
				if ( data.argsString.size() < 3 ) {
					return std::string( "<map_name | everywhere> <model_index | target_name | class_name> <activations> not specified" );
				}

				if ( std::isnan( data.argsFloat.at( 2 ) ) ) {
					return std::string( "invalid count value" );
				}
			}

			EndCondition condition;
			FillHookable( condition, data );
			condition.activations = 0;
			condition.activationsRequired = data.argsFloat.at( 2 );
			condition.constant = true;
			condition.objective = data.argsString.size() > 3 ? data.argsString.at( 3 ) : "COMPLETION";

			endConditions.push_back( condition );

			return std::string( "" );
		}
	);

	configSections[CONFIG_FILE_SECTION_TELEPORT] = ConfigSection(
		"teleport", false,
		[this]( ConfigSectionData &data ) {
			if ( data.argsString.size() < 2 ) {
				return std::string( "<map_name> <model_index | target_name> [x] [y] [z] [angle] not specified" );
			}

			bool defined = true;
			float x = NAN;
			float y = NAN;
			float z = NAN;
			float angle = NAN;

			Teleport teleport;
			FillHookable( teleport, data );

			for ( size_t i = 2 ; i < min( data.argsFloat.size(), 6 ) ; i++ ) {
				float arg = data.argsFloat.at( i );
				if ( std::isnan( arg ) ) {
					char error[1024];
					sprintf_s( error, "invalid coordinate by index %d", i + 1 );
					return std::string( error );
				}

				x		= i == 2 ? arg : x;
				y		= i == 3 ? arg : y;
				z		= i == 4 ? arg : z;
				angle	= i == 5 ? arg : angle;
			}

			teleport.pos = { defined, x, y, z, angle, false };

			teleports.push_back( teleport );

			return std::string( "" );
		}
	);
}

std::string CustomGameModeConfig::ValidateModelIndexSectionData( ConfigSectionData &data ) {

	if ( data.argsString.size() < 2 ) {
		return "<mapname> <modelindex | targetname> [const] not specified";
	}

	return "";
}

std::string CustomGameModeConfig::ConfigTypeToDirectoryName( CONFIG_TYPE configType ) {
	switch ( configType ) {
		case CONFIG_TYPE_MAP:
			return "map_cfg";

		case CONFIG_TYPE_CGM:
			return "cgm_cfg";

		case CONFIG_TYPE_BMM:
			return "bmm_cfg";

		case CONFIG_TYPE_SAGM:
			return "sagm_cfg";

		default:
			return "";
	}
}

std::string CustomGameModeConfig::ConfigTypeToGameModeCommand( CONFIG_TYPE configType ) {
	switch ( configType ) {
		case CONFIG_TYPE_CGM:
			return "cgm";

		case CONFIG_TYPE_BMM:
			return "bmm";

		case CONFIG_TYPE_SAGM:
			return "sagm";

		default:
			return "";
	}
}

std::string CustomGameModeConfig::ConfigTypeToGameModeName( CONFIG_TYPE configType, bool uppercase ) {
	switch ( configType ) {
		case CONFIG_TYPE_CGM:
			return uppercase ? "CUSTOM GAME MODE" : "Custom Game Mode";

		case CONFIG_TYPE_BMM:
			return uppercase ? "BLACK MESA MINUTE" : "Black Mesa Minute";

		case CONFIG_TYPE_SAGM:
			return uppercase ? "SCORE ATTACK" : "Score Attack";

		default:
			return "";
	}
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
	std::string lastSection;

	std::ifstream inp( filePath );
	if ( !inp.is_open( ) && configType != CONFIG_TYPE_MAP ) {
		char errorCString[1024];
		sprintf_s( errorCString, "Config file %s\\%s.txt doesn't exist\n", ConfigTypeToDirectoryName( configType ).c_str(), fileName );
		OnError( std::string( errorCString ) );

		return false;
	}

	bool readingSection = false;

	std::string line;
	std::string fileContents = "";
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
			if ( !OnNewSection( sectionName.str( 1 ) ) ) {
			
				char errorCString[1024];
				sprintf_s( errorCString, "Error parsing %s\\%s.txt, line %d: unknown section [%s]\n", ConfigTypeToDirectoryName( configType ).c_str(), fileName, lineCount, sectionName.str( 1 ).c_str() );
				OnError( std::string( errorCString ) );
				break;

			} else {
				if ( sectionDataLines == 0 ) {
					char errorCString[1024];
					sprintf_s( errorCString, "Error parsing %s\\%s.txt, line %d: tried to parse new section [%s], but section [%s] has no data defined\n", ConfigTypeToDirectoryName( configType ).c_str(), fileName, lineCount, sectionName.str( 1 ).c_str(), lastSection.c_str() );
					OnError( std::string( errorCString ) );
					break;
				}
				sectionDataLines = 0;
				lastSection = sectionName.str( 1 );
			};
		} else {
			std::string error = configSections[currentFileSection].OnSectionData( configName, line, lineCount, configType );
			sectionDataLines++;
			if ( error.size() > 0 ) {
				OnError( error );
				break;
			}
		}

	}

	if ( startMap.empty() && configType != CONFIG_TYPE_MAP ) {
		char errorCString[1024];
		sprintf_s( errorCString, "Error parsing %s\\%s.txt: [start_map] section must be defined\n", ConfigTypeToDirectoryName( configType ).c_str(), fileName );
		OnError( std::string( errorCString ) );
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

		auto sortedData = section.second.data;
		std::sort( sortedData.begin(), sortedData.end(), []( const ConfigSectionData &data1, const ConfigSectionData &data2 ) {
			return data1.line > data2.line;
		} );

		for ( const auto &sectionData : sortedData ) {
			result << sectionData.line;
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
		configSections[(CONFIG_FILE_SECTION) section].data.clear();
	}
	
	this->configName.clear();
	this->configNameSeparated.clear();
	this->error.clear();
	musicPlaylistShuffle = false;
	gameFinishedOnce = false;

	this->markedForRestart = false;
	this->hasEndMarkers = false;

	this->entitiesToPrecache.clear();
	this->soundsToPrecache.clear();

	
	name = "";
	description = "";
	startPosition = { false, NAN, NAN, NAN, NAN, false };
	startMap = "";
	endMap = "";
	endTrigger.Reset();
	forbiddenLevels.clear();
	loadout.clear();
	entityUses.clear();
	entitySpawns.clear();
	entitiesPrevented.clear();
	sounds.clear();
	maxCommentary.clear();
	music.clear();
	musicPlaylist.clear();
	intermissions.clear();
	timerPauses.clear();
	timerResumes.clear();
	entityRandomSpawners.clear();
	endConditions.clear();
	teleports.clear();
	entitiesToRemove.clear();
}

bool CustomGameModeConfig::IsGameplayModActive( GAMEPLAY_MOD mod ) {
	auto result = std::find_if( mods.begin(), mods.end(), [mod]( const GameplayMod &gameplayMod ) {
		return gameplayMod.id == mod;
	} );

	return result != std::end( mods );
}

bool CustomGameModeConfig::AddGameplayMod( ConfigSectionData &data ) {
	std::string modName = data.argsString.at( 0 );

	if ( modName == "always_gib" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_ALWAYS_GIB,
			"Always gib",
			"Kills will always try to result in gibbing.",
			[]( CBasePlayer *player ) { player->alwaysGib = true; }
		) );
		return true;
	}

	if ( modName == "bleeding" ) {
		int bleedHandicap = 20;
		float bleedUpdatePeriod = 1.0f;
		float bleedImmunityPeriod = 10.0f;
		for ( size_t i = 1 ; i < data.argsFloat.size() ; i++ ) {
			if ( std::isnan( data.argsFloat.at( i ) ) ) {
				continue;
			}
			if ( i == 1 ) {
				bleedHandicap = min( max( 0, data.argsFloat.at( i ) ), 99 );
			}
			if ( i == 2 ) {
				bleedUpdatePeriod = data.argsFloat.at( i );
			}
			if ( i == 3 ) {
				bleedImmunityPeriod = max( 0.05, data.argsFloat.at( i ) );
			}
		}

		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_BLEEDING,
			"Bleeding",
			"After your last painkiller take, you start to lose health.\n"
			"Health regeneration is turned off.",
			[bleedHandicap, bleedUpdatePeriod, bleedImmunityPeriod]( CBasePlayer *player ) {
				player->isBleeding = true;
				player->bleedHandicap = bleedHandicap;
				player->bleedUpdatePeriod = bleedUpdatePeriod;
				player->bleedImmunityPeriod = bleedImmunityPeriod;

				player->lastHealingTime = gpGlobals->time + bleedImmunityPeriod;
			},
			{
				"Bleeding until " + std::to_string( bleedHandicap ) + "%% health left\n",
				"Bleed update period: " + std::to_string( bleedUpdatePeriod ) + " sec \n",
				"Bleed again after healing in: " + std::to_string( bleedImmunityPeriod ) + " sec \n",
			}
		) );
		return true;
	}

	if ( modName == "bullet_delay_on_slowmotion" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_BULLET_DELAY_ON_SLOWMOTION,
			"Bullet delay on slowmotion.",
			"When slowmotion is activated, physical bullets shot by you will move slowly until you turn off the slowmotion.",
			[]( CBasePlayer *player ) { player->bulletDelayOnSlowmotion = true; }
		) );
		return true;
	}

	if ( modName == "bullet_physics_disabled" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_BULLET_PHYSICS_DISABLED,
			"Bullet physics disabled",
			"Self explanatory.",
			[]( CBasePlayer *player ) { player->bulletPhysicsMode = BULLET_PHYSICS_DISABLED; }
		) );
		return true;
	}

	if ( modName == "bullet_physics_constant" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_BULLET_PHYSICS_CONSTANT,
			"Bullet physics constant",
			"Bullet physics is always present, even when slowmotion is NOT present.",
			[]( CBasePlayer *player ) { player->bulletPhysicsMode = BULLET_PHYSICS_CONSTANT; }
		) );
		return true;
	}

	if ( modName == "bullet_physics_enemies_and_player_on_slowmotion" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_BULLET_PHYSICS_ENEMIES_AND_PLAYER_ON_SLOWMOTION,
			"Bullet physics for enemies and player on slowmotion",
			"Bullet physics will be active for both enemies and player only when slowmotion is present.",
			[]( CBasePlayer *player ) { player->bulletPhysicsMode = BULLET_PHYSICS_ENEMIES_AND_PLAYER_ON_SLOWMOTION; }
		) );
		return true;
	}

	if ( modName == "bullet_ricochet" ) {
		
		int bulletRicochetCount = 2;
		int bulletRicochetError = 5;
		float bulletRicochetMaxDegree = 45;

		for ( size_t i = 1 ; i < data.argsFloat.size() ; i++ ) {
			if ( std::isnan( data.argsFloat.at( i ) ) ) {
				continue;
			}
			if ( i == 1 ) {
				bulletRicochetCount = data.argsFloat.at( i );
				if ( bulletRicochetCount <= 0 ) {
					bulletRicochetCount = -1;
				}
			}
			if ( i == 2 ) {
				bulletRicochetMaxDegree = min( max( 1, data.argsFloat.at( i ) ), 90 );
			}
			if ( i == 3 ) {
				bulletRicochetError = data.argsFloat.at( i );
			}
		}

		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_BULLET_RICOCHET,
			"Bullet ricochet",
			"Physical bullets ricochet off the walls.",
			[bulletRicochetCount, bulletRicochetError, bulletRicochetMaxDegree]( CBasePlayer *player ) {
				player->bulletRicochetCount = bulletRicochetCount;
				player->bulletRicochetError = bulletRicochetError;
				player->bulletRicochetMaxDotProduct = bulletRicochetMaxDegree / 90.0f;
			},
			{
				"Max ricochets: " + ( bulletRicochetCount <= 0 ? "Infinite" : std::to_string( bulletRicochetCount ) ) + "\n",
				"Max angle for ricochet: " + std::to_string( bulletRicochetMaxDegree ) + " deg \n",
				//"Ricochet error: " + std::to_string( bulletRicochetError ) + "%% \n",
			}
		) );
		return true;
	}

	if ( modName == "bullet_self_harm" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_BULLET_SELF_HARM,
			"Bullet self harm",
			"Physical bullets shot by player can harm back (ricochet mod is required).",
			[]( CBasePlayer *player ) { player->bulletSelfHarm = true; }
		) );
		return true;
	}

	if ( modName == "bullet_trail_constant" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_BULLET_TRAIL_CONSTANT,
			"Bullet trail constant",
			"Physical bullets always get a trail, regardless if slowmotion is present.",
			[]( CBasePlayer *player ) { player->bulletTrailConstant = true; }
		) );
		return true;
	}

	if ( modName == "constant_slowmotion" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_CONSTANT_SLOWMOTION,
			"Constant slowmotion",
			"You start in slowmotion, it's infinite and you can't turn it off.",
			[]( CBasePlayer *player ) {
#ifndef CLIENT_DLL
				player->TakeSlowmotionCharge( 100 );
				player->SetSlowMotion( true );
#endif
				player->infiniteSlowMotion = true;
				player->constantSlowmotion = true;
			}
		) );
		return true;
	}

	if ( modName == "crossbow_explosive_bolts" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_CROSSBOW_EXPLOSIVE_BOLTS,
			"Crossbow explosive bolts",
			"Crossbow bolts explode when they hit the wall.",
			[]( CBasePlayer *player ) {
				player->crossbowExplosiveBolts = true;
			}
		) );
		return true;
	}

	if ( modName == "detachable_tripmines" ) {
		bool detachInstantly = false;
		if ( data.argsString.size() > 1 && data.argsString.at( 1 ) == "instantly" ) {
			detachInstantly = true;
		}

		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_DETACHABLE_TRIPMINES,
			"Detachable tripmines.",
			"Pressing USE button on attached tripmines will detach them.",
			[detachInstantly]( CBasePlayer *player ) {
				player->detachableTripmines = true;
				player->detachableTripminesInstantly = detachInstantly;
			}
		) );
		return true;
	}

	if ( modName == "diving_allowed_without_slowmotion" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_DIVING_ALLOWED_WITHOUT_SLOWMOTION,
			"Diving allowed without slowmotion",
			"You're still allowed to dive even if you have no slowmotion charge.\n"
			"In that case you will dive without going into slowmotion.",
			[]( CBasePlayer *player ) { player->divingAllowedWithoutSlowmotion = true; }
		) );
		return true;
	}

	if ( modName == "diving_only" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_DIVING_ONLY,
			"Diving only",
			"The only way to move around is by diving.\n"
			"This enables Infinite slowmotion by default.\n"
			"You can dive even when in crouch-like position, like when being in vents etc.",
			[]( CBasePlayer *player ) {
				player->divingOnly = true;
				player->infiniteSlowMotion = true;
			}
		) );
		return true;
	}

	if ( modName == "drunk_aim" ) {
		float aimMaxOffsetX = 20.0f;
		float aimMaxOffsetY = 5.0f;
		float aimOffsetChangeFreqency = 1.0f;

		for ( size_t i = 1 ; i < data.argsFloat.size() ; i++ ) {
			if ( std::isnan( data.argsFloat.at( i ) ) ) {
				continue;
			}
			if ( i == 1 ) {
				aimMaxOffsetX = min( max( 0, data.argsFloat.at( i ) ), 25.5 );
			}
			if ( i == 2 ) {
				aimMaxOffsetY = min( max( 0, data.argsFloat.at( i ) ), 25.5 );
			}
			if ( i == 3 ) {
				aimOffsetChangeFreqency = max( 0.01, data.argsFloat.at( i ) );
			}
		}

		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_DRUNK_AIM,
			"Drunk aim",
			"Your aim becomes wobbly.",
			[aimMaxOffsetX, aimMaxOffsetY, aimOffsetChangeFreqency]( CBasePlayer *player ) { 
				player->aimMaxOffsetX = aimMaxOffsetX;
				player->aimMaxOffsetY = aimMaxOffsetY;
				player->aimOffsetChangeFreqency = aimOffsetChangeFreqency;
			},
			{
				"Max horizontal wobble: " + std::to_string( aimMaxOffsetX ) + " deg\n",
				"Max vertical wobble: " + std::to_string( aimMaxOffsetY ) + " deg\n",
				"Wobble frequency: " + std::to_string( aimOffsetChangeFreqency ) + "\n",
			}
		) );
		return true;
	}

	if ( modName == "drunk_look" ) {
		int drunkinessPercent = 25;
		if ( data.argsFloat.size() >= 2 && !std::isnan( data.argsFloat.at( 1 ) ) ) {
			drunkinessPercent = min( max( 0, data.argsFloat.at( 1 ) ), 100 );
		}

		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_DRUNK_LOOK,
			"Drunk look",
			"Camera view becomes wobbly and makes aim harder.",
			[drunkinessPercent]( CBasePlayer *player ) { 
			player->drunkiness = ( drunkinessPercent / 100.0f ) * 255;
		},
			{ "Drunkiness: " + std::to_string( drunkinessPercent ) + "%%\n" }
		) );
		return true;
	}

	if ( modName == "easy" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_EASY,
			"Easy difficulty",
			"Sets up easy level of difficulty.",
			[]( CBasePlayer *player ) {}
		) );
		return true;
	}

	if ( modName == "edible_gibs" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_EDIBLE_GIBS,
			"Edible gibs",
			"Allows you to eat gibs by pressing USE when aiming at the gib, which restore your health by 5.",
			[]( CBasePlayer *player ) { player->edibleGibs = true; }
		) );
		return true;
	}

	if ( modName == "empty_slowmotion" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_EMPTY_SLOWMOTION,
			"Empty slowmotion",
			"Start with no slowmotion charge.",
			[]( CBasePlayer *player ) {}
		) );
		return true;
	}

	if ( modName == "fading_out" ) {
		int fadeOutPercent = 90;
		float fadeOutUpdatePeriod = 0.5f;
		for ( size_t i = 1 ; i < data.argsFloat.size() ; i++ ) {
			if ( std::isnan( data.argsFloat.at( i ) ) ) {
				continue;
			}
			if ( i == 1 ) {
				fadeOutPercent = min( max( 0, data.argsFloat.at( i ) ), 100 );
			}
			if ( i == 2 ) {
				fadeOutUpdatePeriod = data.argsFloat.at( i );
			}
		}

		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_FADING_OUT,
			"Fading out",
			"View is fading out, or in other words it's blacking out until you can't see almost anything.\n"
			"Take painkillers to restore the vision.\n"
			"Allows to take painkillers even when you have 100 health and enough time have passed since the last take.",
			[fadeOutPercent, fadeOutUpdatePeriod]( CBasePlayer *player ) {
				player->isFadingOut = true;
				player->fadeOutThreshold = 255 - ( fadeOutPercent / 100.0f ) * 255;
				player->fadeOutUpdatePeriod = fadeOutUpdatePeriod;
			},
			{
				"Fade out intensity: " + std::to_string( fadeOutPercent ) + "%%\n",
				"Fade out update period: " + std::to_string( fadeOutUpdatePeriod ) + " sec \n",
			}
		) );
		return true;
	}

	if ( modName == "friction" ) {
		float friction = 4.0f;
		for ( size_t i = 1 ; i < data.argsFloat.size() ; i++ ) {
			if ( std::isnan( data.argsFloat.at( i ) ) ) {
				continue;
			}
			if ( i == 1 ) {
				friction = max( 0, data.argsFloat.at( i ) );
			}
		}

		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_FRICTION,
			"Friction",
			"Changes player's friction.",
			[friction]( CBasePlayer *player ) {
				player->frictionOverride = friction;
			},
			{ "Friction: " + std::to_string( friction ) + "\n" }
		) );
		return true;
	}

	if ( modName == "garbage_gibs" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_GARBAGE_GIBS,
			"Garbage gibs",
			"Replaces all gibs with garbage.",
			[]( CBasePlayer *player ) { player->garbageGibs = true; }
		) );
		return true;
	}

	if ( modName == "god" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_GOD,
			"God mode",
			"You are invincible and it doesn't count as a cheat.",
			[]( CBasePlayer *player ) {
				player->godConstant = true;
			}
		) );
		return true;
	}

	if ( modName == "hard" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_HARD,
			"Hard difficulty",
			"Sets up hard level of difficulty.",
			[]( CBasePlayer *player ) {}
		) );
		return true;
	}

	if ( modName == "headshots" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_HEADSHOTS,
			"Headshots",
			"Headshots dealt to enemies become much more deadly.",
			[]( CBasePlayer *player ) {}
		) );
		return true;
	}

	if ( modName == "infinite_ammo" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_INFINITE_AMMO,
			"Infinite ammo",
			"All weapons get infinite ammo.",
			[]( CBasePlayer *player ) { player->infiniteAmmo = true; }
		) );
		return true;
	}

	if ( modName == "infinite_ammo_clip" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_INFINITE_AMMO_CLIP,
			"Infinite ammo clip",
			"Most weapons get an infinite ammo clip and need no reloading.",
			[]( CBasePlayer *player ) { player->infiniteAmmoClip = true; }
		) );
		return true;
	}

	if ( modName == "infinite_painkillers" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_INFINITE_PAINKILLERS,
			"Infinite painkillers",
			"Self explanatory.",
			[]( CBasePlayer *player ) { player->infinitePainkillers = true; }
		) );
		return true;
	}

	if ( modName == "infinite_slowmotion" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_INFINITE_SLOWMOTION,
			"Infinite slowmotion",
			"You have infinite slowmotion charge and it's not considered as cheat.",
			[]( CBasePlayer *player ) {
#ifndef CLIENT_DLL
				player->TakeSlowmotionCharge( 100 );
#endif
				player->infiniteSlowMotion = true;
			}
		) );
		return true;
	}

	if ( modName == "instagib" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_INSTAGIB,
			"Instagib",
			"Gauss Gun becomes much more deadly with 9999 damage, also gets red beam and slower rate of fire.\n"
			"More gibs come out.",
			[]( CBasePlayer *player ) { player->instaGib = true; }
		) );
		return true;
	}

	if ( modName == "health_regeneration" ) {
		int regenerationMax = 20;
		float regenerationDelay = 3.0f;
		float regenerationFrequency = 0.2f;

		for ( size_t i = 1 ; i < data.argsFloat.size() ; i++ ) {
			if ( std::isnan( data.argsFloat.at( i ) ) ) {
				continue;
			}
			if ( i == 1 ) {
				regenerationMax = max( 0, data.argsFloat.at( i ) );
			}
			if ( i == 2 ) {
				regenerationDelay = max( 0, data.argsFloat.at( i ) );
			}
			if ( i == 3 ) {
				regenerationFrequency = max( 0, data.argsFloat.at( i ) );
			}
		}

		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_HEALTH_REGENERATION,
			"Health regeneration",
			"Allows for health regeneration options.",
			[regenerationMax, regenerationDelay, regenerationFrequency]( CBasePlayer *player ) {
				player->pev->max_health = regenerationMax;
				player->regenerationMax = regenerationMax;
				player->regenerationDelay = regenerationDelay;
				player->regenerationFrequency = regenerationFrequency;
			},
			{
				"Regenerate up to: " + std::to_string( regenerationMax ) + " HP\n",
				std::to_string( regenerationDelay ) + " sec after last damage\n",
				"Regenerating: " + std::to_string( 1 / regenerationFrequency ) + " HP/sec\n",
			}
		) );
		return true;
	}

	if ( modName == "heal_on_kill" ) {
		float maxHealthTakenPercent = 25.0f;

		for ( size_t i = 1 ; i < data.argsFloat.size() ; i++ ) {
			if ( std::isnan( data.argsFloat.at( i ) ) ) {
				continue;
			}
			if ( i == 1 ) {
				maxHealthTakenPercent = min( max( 1, data.argsFloat.at( i ) ), 5000 );
			}
		}

		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_HEAL_ON_KILL,
			"Heal on kill",
			"Your health will be replenished after killing an enemy.",
			[maxHealthTakenPercent]( CBasePlayer *player ) {
				player->healOnKill = true;
				player->healOnKillMultiplier = maxHealthTakenPercent / 100.0f;
			},
			{
				"Victim's max health taken after kill: " + std::to_string( maxHealthTakenPercent ) + "%%\n",
			}
		) );
		return true;
	}

	if ( modName == "no_fall_damage" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_NO_FALL_DAMAGE,
			"No fall damage",
			"Self explanatory.",
			[]( CBasePlayer *player ) { player->noFallDamage = true; }
		) );
		return true;
	}

	if ( modName == "no_map_music" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_NO_MAP_MUSIC,
			"No map music",
			"Music which is defined by map will not be played.\nOnly the music defined in map and gameplay config files will play.",
			[]( CBasePlayer *player ) { player->noMapMusic = true; }
		) );
		return true;
	}

	if ( modName == "no_healing" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_NO_HEALING,
			"No healing",
			"Don't allow to heal in any way, including Xen healing pools.",
			[]( CBasePlayer *player ) { player->noHealing = true; }
		) );
		return true;
	}

	if ( modName == "no_jumping" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_NO_JUMPING,
			"No jumping",
			"Don't allow to jump.",
			[]( CBasePlayer *player ) { player->noJumping = true; }
		) );
		return true;
	}

	if ( modName == "no_pills" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_NO_PILLS,
			"No pills",
			"Don't allow to take painkillers.",
			[]( CBasePlayer *player ) { player->noPills = true; }
		) );
		return true;
	}

	if ( modName == "no_saving" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_NO_SAVING,
			"No saving",
			"Don't allow to load saved files.",
			[]( CBasePlayer *player ) { player->noSaving = true; }
		) );
		return true;
	}

	if ( modName == "no_secondary_attack" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_NO_SECONDARY_ATTACK,
			"No secondary attack",
			"Disables the secondary attack on all weapons.",
			[]( CBasePlayer *player ) { player->noSecondaryAttack = true; }
		) );
		return true;
	}

	if ( modName == "no_slowmotion" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_NO_SLOWMOTION,
			"No slowmotion",
			"You're not allowed to use slowmotion.",
			[]( CBasePlayer *player ) { player->noSlowmotion = true; }
		) );
		return true;
	}

	if ( modName == "no_smg_grenade_pickup" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_NO_SMG_GRENADE_PICKUP,
			"No SMG grenade pickup",
			"You're not allowed to pickup and use SMG (MP5) grenades.",
			[]( CBasePlayer *player ) { player->noSmgGrenadePickup = true; }
		) );
		return true;
	}

	if ( modName == "no_target" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_NO_TARGET,
			"No target",
			"You are invisible to everyone and it doesn't count as a cheat.",
			[]( CBasePlayer *player ) {
				player->noTargetConstant = true;
			}
		) );
		return true;
	}

	if ( modName == "no_walking" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_NO_WALKING,
			"No walking",
			"Don't allow to walk, swim, dive, climb ladders.",
			[]( CBasePlayer *player ) { player->noWalking = true; }
		) );
		return true;
	}

	if ( modName == "one_hit_ko" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_ONE_HIT_KO,
			"One hit KO",
			"Any hit from an enemy will kill you instantly.\n"
			"You still get proper damage from falling and environment.",
			[]( CBasePlayer *player ) { player->oneHitKO = true; }
		) );
		return true;
	}

	if ( modName == "one_hit_ko_from_player" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_ONE_HIT_KO_FROM_PLAYER,
			"One hit KO from player",
			"All enemies die in one hit.",
			[]( CBasePlayer *player ) { player->oneHitKOFromPlayer = true; }
		) );
		return true;
	}

	if ( modName == "prevent_monster_spawn" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_PREVENT_MONSTER_SPAWN,
			"Prevent monster spawn",
			"Don't spawn predefined monsters (NPCs) when visiting a new map.\n"
			"This doesn't affect dynamic monster_spawners.",
			[]( CBasePlayer *player ) {}
		) );
		return true;
	}

	if ( modName == "shotgun_automatic" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_SHOTGUN_AUTOMATIC,
			"Automatic shotgun",
			"Shotgun only fires single shots and doesn't have to be reloaded after each shot.",
			[]( CBasePlayer *player ) { player->automaticShotgun = true; }
		) );
		return true;
	}

	if ( modName == "show_timer" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_SHOW_TIMER,
			"Show timer",
			"Timer will be shown. Time is affected by slowmotion.",
			[]( CBasePlayer *player ) {
				player->timerShown = true;
			}
		) );
		return true;
	}

	if ( modName == "show_timer_real_time" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_SHOW_TIMER_REAL_TIME,
			"Show timer with real time",
			"Time will be shown and it's not affected by slowmotion, which is useful for speedruns.",
			[]( CBasePlayer *player ) {
				player->timerShown = true;
				player->timerShowReal = true;
			}
		) );
		return true;
	}

	if ( modName == "slowmotion_fast_walk" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_SLOWMOTION_FAST_WALK,
			"Fast walk in slowmotion",
			"You still walk and run almost as fast as when slowmotion is not active.",
			[]( CBasePlayer *player ) { player->slowmotionFastWalk = true; }
		) );
		return true;
	}

	if ( modName == "slowmotion_on_damage" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_SLOWMOTION_ON_DAMAGE,
			"Slowmotion on damage",
			"You get slowmotion charge when receiving damage.",
			[]( CBasePlayer *player ) { player->slowmotionOnDamage = true; }
		) );
		return true;
	}

	if ( modName == "slowmotion_only_diving" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_SLOWMOTION_ONLY_DIVING,
			"Slowmotion only when diving",
			"You're allowed to go into slowmotion only by diving.",
			[]( CBasePlayer *player ) { player->slowmotionOnlyDiving = true; }
		) );
		return true;
	}

	if ( modName == "slow_painkillers" ) {
		float nextPainkillerEffectTimePeriod = 0.2f;
		for ( size_t i = 1 ; i < data.argsFloat.size() ; i++ ) {
			if ( std::isnan( data.argsFloat.at( i ) ) ) {
				continue;
			}
			if ( i == 1 ) {
				nextPainkillerEffectTimePeriod = max( 0, data.argsFloat.at( i ) );
			}
		}

		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_SLOW_PAINKILLERS,
			"Slow painkillers",
			"Painkillers take time to have an effect, like in original Max Payne.",
			[nextPainkillerEffectTimePeriod]( CBasePlayer *player ) {
				player->slowPainkillers = true;
				player->nextPainkillerEffectTimePeriod = nextPainkillerEffectTimePeriod;
			},
			{ "Healing period " + std::to_string( nextPainkillerEffectTimePeriod ) + " sec\n" }
		) );
		return true;
	}

	if ( modName == "snark_friendly_to_allies" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_SNARK_FRIENDLY_TO_ALLIES,
			"Snarks friendly to allies",
			"Snarks won't attack player's allies.",
			[]( CBasePlayer *player ) { player->snarkFriendlyToAllies = true; }
		) );
		return true;
	}

	if ( modName == "snark_friendly_to_player" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_SNARK_FRIENDLY_TO_PLAYER,
			"Snarks friendly to player",
			"Snarks won't attack player.",
			[]( CBasePlayer *player ) { player->snarkFriendlyToPlayer = true; }
		) );
		return true;
	}

	if ( modName == "snark_from_explosion" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_SNARK_FROM_EXPLOSION,
			"Snark from explosion",
			"Snarks will spawn in the place of explosion.",
			[]( CBasePlayer *player ) { player->snarkFromExplosion = true; }
		) );
		return true;
	}

	if ( modName == "snark_inception" ) {
		int snarkInceptionDepth = 10;
		for ( size_t i = 1 ; i < data.argsFloat.size() ; i++ ) {
			if ( std::isnan( data.argsFloat.at( i ) ) ) {
				continue;
			}
			if ( i == 1 ) {
				snarkInceptionDepth = min( max( 1, data.argsFloat.at( i ) ), 100 );
			}
		}

		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_SNARK_INCEPTION,
			"Snark inception",
			"Killing snark splits it into two snarks.",
			[snarkInceptionDepth]( CBasePlayer *player ) {
				player->snarkInception = true;
				player->snarkInceptionDepth = snarkInceptionDepth;
			},
			{
				"Inception depth: " + std::to_string( snarkInceptionDepth ) + " snarks\n"
			}
		) );
		return true;
	}

	if ( modName == "snark_infestation" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_SNARK_INFESTATION,
			"Snark infestation",
			"Snark will spawn in the body of killed monster (NPC).\n"
			"Even more snarks spawn if monster's corpse has been gibbed.",
			[]( CBasePlayer *player ) { player->snarkInfestation = true; }
		) );
		return true;
	}

	if ( modName == "snark_nuclear" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_SNARK_NUCLEAR,
			"Snark nuclear",
			"Killing snark produces a grenade-like explosion.",
			[]( CBasePlayer *player ) { player->snarkNuclear = true; }
		) );
		return true;
	}

	if ( modName == "snark_penguins" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_SNARK_PENGUINS,
			"Snark penguins",
			"Replaces snarks with penguins from Opposing Force.\n",
			[]( CBasePlayer *player ) { player->snarkPenguins = true; }
		) );
		return true;
	}

	if ( modName == "snark_stay_alive" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_SNARK_STAY_ALIVE,
			"Snark stay alive",
			"Snarks will never die on their own, they must be shot.",
			[]( CBasePlayer *player ) { player->snarkStayAlive = true; }
		) );
		return true;
	}

	if ( modName == "starting_health" ) {
		int startingHealth = 100;
		for ( size_t i = 1 ; i < data.argsFloat.size() ; i++ ) {
			if ( std::isnan( data.argsFloat.at( i ) ) ) {
				continue;
			}
			if ( i == 1 ) {
				startingHealth = data.argsFloat.at( i );
				if ( startingHealth <= 0 ) {
					startingHealth = 1;
				}
			}
		}

		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_STARTING_HEALTH,
			"Starting Health",
			"Start with specified health amount.",
			[startingHealth]( CBasePlayer *player ) { player->pev->health = startingHealth; },
			{ "Health amount: " + std::to_string( startingHealth ) + "\n" }
		) );
		return true;
	}

	if ( modName == "superhot" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_SUPERHOT,
			"SUPERHOT",
			"Time moves forward only when you move around.\n"
			"Inspired by the game SUPERHOT.",
			[]( CBasePlayer *player ) { player->superHot = true; }
		) );
		return true;
	}

	if ( modName == "swear_on_kill" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_SWEAR_ON_KILL,
			"Swear on kill",
			"Max will swear when killing an enemy. He will still swear even if Max's commentary is turned off.",
			[]( CBasePlayer *player ) { player->swearOnKill = true; }
		) );
		return true;
	}

	if ( modName == "upside_down" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_UPSIDE_DOWN,
			"Upside down",
			"View becomes turned on upside down.",
			[]( CBasePlayer *player ) { player->upsideDown = true; }
		) );
		return true;
	}

	if ( modName == "totally_spies" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_TOTALLY_SPIES,
			"Totally spies",
			"Replaces all HGrunts with Black Ops.",
			[]( CBasePlayer *player ) {}
		) );
		return true;
	}

	if ( modName == "teleport_maintain_velocity" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_TELEPORT_MAINTAIN_VELOCITY,
			"Teleport maintain velocity",
			"Your velocity will be preserved after going through teleporters.",
			[]( CBasePlayer *player ) {
				player->teleportMaintainVelocity = true;
			}
		) );
		return true;
	}

	if ( modName == "time_restriction" ) {
		float timeOut = 60;
		for ( size_t i = 1 ; i < data.argsFloat.size() ; i++ ) {
			if ( std::isnan( data.argsFloat.at( i ) ) ) {
				continue;
			}
			if ( i == 1 ) {
				timeOut = data.argsFloat.at( i );
				if ( timeOut <= 0 ) {
					timeOut = 1;
				}
			}
		}

		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_TIME_RESTRICTION,
			"Time restriction",
			"You are killed if time runs out.",
			[timeOut]( CBasePlayer *player ) {
				player->timerShown = true;
				player->timerBackwards = true;
				player->time = timeOut;
			},
			{
				std::to_string( timeOut ) + " seconds\n"
			}
		) );
		return true;
	}

	if ( modName == "vvvvvv" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_VVVVVV,
			"VVVVVV",
			"Pressing jump reverses gravity for player.\n"
			"Inspired by the game VVVVVV.",
			[]( CBasePlayer *player ) { player->vvvvvv = true; }
		) );
		return true;
	}

	if ( modName == "weapon_impact" ) {
		float weaponImpact = 1.0f;
		for ( size_t i = 1 ; i < data.argsFloat.size() ; i++ ) {
			if ( std::isnan( data.argsFloat.at( i ) ) ) {
				continue;
			}
			if ( i == 1 ) {
				weaponImpact = max( 1.0f, data.argsFloat.at( i ) );
			}
		}

		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_WEAPON_PUSH_BACK,
			"Weapon impact",
			"Taking damage means to be pushed back",
			[weaponImpact]( CBasePlayer *player ) {
				player->weaponImpact = weaponImpact;
			},
			{
				"Impact multiplier: " + std::to_string( weaponImpact ) + "\n",
			}
		) );
		return true;
	}

	if ( modName == "weapon_push_back" ) {

		float weaponPushBackMultiplier = 1.0f;
		for ( size_t i = 1 ; i < data.argsFloat.size() ; i++ ) {
			if ( std::isnan( data.argsFloat.at( i ) ) ) {
				continue;
			}
			if ( i == 1 ) {
				weaponPushBackMultiplier = max( 0.1f, data.argsFloat.at( i ) );
			}
		}

		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_WEAPON_PUSH_BACK,
			"Weapon push back",
			"Shooting weapons pushes you back.",
			[weaponPushBackMultiplier]( CBasePlayer *player ) {
				player->weaponPushBack = true;
				player->weaponPushBackMultiplier = weaponPushBackMultiplier;
			},
			{
				"Push back multiplier: " + std::to_string( weaponPushBackMultiplier ) + "\n",
			}
		) );
		return true;
	}

	if ( modName == "weapon_restricted" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_WEAPON_RESTRICTED,
			"Weapon restricted",
			"If you have no weapons - you are allowed to pick only one.\n"
			"You can have several weapons at once if they are specified in [loadout] section.\n"
			"Weapon stripping doesn't affect you.",
			[]( CBasePlayer *player ) { player->weaponRestricted = true; }
		) );
		return true;
	}

	return false;
}

void CustomGameModeConfig::FillHookable( Hookable &hookable, const ConfigSectionData &data ) {
	hookable.map = data.argsString.at( 0 );
	if ( std::isnan( data.argsFloat.at( 1 ) ) ) {
		hookable.targetName = data.argsString.at( 1 );
	} else {
		hookable.modelIndex = data.argsFloat.at( 1 );
	}

	if ( data.argsString.size() > 3 ) {
		for ( size_t i = data.argsString.size() - 1 ; i >= 3 ; i-- ) {
			if ( data.argsString.at( i ) == "const" ) {
				hookable.constant = true;
				break;
			}
		}
	}
}

void CustomGameModeConfig::FillHookableWithDelay( HookableWithDelay &hookableWithDelay, const ConfigSectionData &data ) {
	FillHookable( hookableWithDelay, data );

	if ( data.argsFloat.size() > 2 ) {
		if ( !std::isnan( data.argsFloat.at( 2 ) ) ) {
			hookableWithDelay.delay = data.argsFloat.at( 2 );
		}
	}
}

void CustomGameModeConfig::FillHookableWithTarget( HookableWithTarget &hookableWithTarget, const ConfigSectionData &data ) {
	FillHookable( hookableWithTarget, data );

	if ( data.argsString.size() > 2 ) {
		hookableWithTarget.isModelIndex = !std::isnan( data.argsFloat.at( 2 ) );
		hookableWithTarget.entityName = data.argsString.at( 2 );
	}
}

void CustomGameModeConfig::FillHookableSound( Sound &hookableSound, const ConfigSectionData &data ) {
	FillHookable( hookableSound, data );

	hookableSound.path = data.argsString.at( 2 );

	for ( size_t arg = 3 ; arg < data.argsString.size() ; arg++ ) {
		 
		hookableSound.looping = hookableSound.looping || data.argsString.at( arg ) == "looping";
		hookableSound.noSlowmotionEffects = hookableSound.noSlowmotionEffects || data.argsString.at( arg ) == "no_slowmotion_effects";

		hookableSound.delay			= arg == 3 ? data.argsFloat.at( arg ) : hookableSound.delay;
		hookableSound.initialPos	= arg == 4 ? data.argsFloat.at( arg ) : hookableSound.initialPos;
	}
}

void CustomGameModeConfig::FillEntitySpawn( EntitySpawn &entitySpawn, const ConfigSectionData &data ) {
	FillHookable( entitySpawn, data );

	entitySpawn.entityName = data.argsString.at( 2 );
	entitySpawn.x = data.argsFloat.at( 3 );
	entitySpawn.y = data.argsFloat.at( 4 );
	entitySpawn.z = data.argsFloat.at( 5 );
	entitySpawn.angle = data.argsFloat.size() >= 7 ? data.argsFloat.at( 6 ) : 0.0f;
	entitySpawn.targetName = data.argsFloat.size() >= 8 ? data.argsString.at( 7 ) : "";
}

bool CustomGameModeConfig::OnNewSection( std::string sectionName ) {

	for ( const auto &configSection : configSections ) {
		if ( configSection.second.name == sectionName ) {
			currentFileSection = configSection.first;
			return true;
		}
	}

	return false;
}

bool CustomGameModeConfig::MarkModelIndex( CONFIG_FILE_SECTION fileSection, const std::string &mapName, int modelIndex, const std::string &targetName, bool *outIsConstant, ConfigSectionData *outData ) {
	auto *sectionData = &configSections[fileSection].data;
	auto i = sectionData->begin();
	while ( i != sectionData->end() ) {
		std::string storedMapName = i->argsString.at( 0 );
		int storedModelIndex = std::isnan( i->argsFloat.at( 1 ) ) ? -2 : i->argsFloat.at( 1 );
		std::string storedTargetName = i->argsString.at( 1 );

		if ( 
			mapName != storedMapName ||
			modelIndex != storedModelIndex &&
			( storedTargetName != targetName || storedTargetName.size() == 0 )
		) {
			i++;
			continue;
		}

		if ( outData ) {
			outData->line = i->line;
			outData->argsFloat = i->argsFloat;
			outData->argsString = i->argsString;
		}

		bool constant = false;
		if ( i->argsString.size() >= 3 ) {
			constant = i->argsString.at( 2 ) == "const";
		}

		if ( !constant ) {
			i = sectionData->erase( i );
		}

		if ( outIsConstant ) {
			*outIsConstant = constant;
		}

		return true;
	}

	return false;
}

CustomGameModeConfig::ConfigSection::ConfigSection() {
	this->name = "UNDEFINED_SECTION";
	this->single = false;
	this->Validate = []( ConfigSectionData &data ){ return ""; };
}

CustomGameModeConfig::ConfigSection::ConfigSection( const std::string &sectionName, bool single, ConfigSectionValidateFunction validateFunction ) {
	this->name = sectionName;
	this->single = single;
	this->Validate = validateFunction;
}

const std::string CustomGameModeConfig::ConfigSection::OnSectionData( const std::string &configName, const std::string &line, int lineCount, CONFIG_TYPE configType ) {
	if ( single && data.size() > 0 ) {
		char error[1024];
		sprintf_s( error, "Error parsing %s\\%s.txt, line %d: [%s] section can only have one line\n", ConfigTypeToDirectoryName( configType ).c_str(), configName.c_str(), lineCount, name.c_str() );
		return error;
	}

	ConfigSectionData sectionData;
	sectionData.line = line;
	sectionData.argsString = NaiveCommandLineParse( line );
	
	for ( const std::string &arg : sectionData.argsString ) {
		float value;
		try {
			value = std::stof( arg );
		} catch ( std::invalid_argument e ) {
			value = NAN;
		}

		sectionData.argsFloat.push_back( value );
	}

	std::string error = Validate( sectionData );
	if ( error.size() > 0 ) {
		char errorComposed[1024];
		sprintf_s( errorComposed, "Error parsing %s\\%s.txt, line %d, section [%s]: %s\n", ConfigTypeToDirectoryName( configType ).c_str(), configName.c_str(), lineCount, name.c_str(), error.c_str() );
		return errorComposed;
	} else {
		data.push_back( sectionData );
	}

	return "";

}