#include "custom_gamemode_config.h"
#include "string_aux.h"
#include <fstream>
#include <regex>
#include "Windows.h"

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

CustomGameModeConfig::CustomGameModeConfig( const char *folderName )
{
	folderPath = GetGamePath() + "\\" + folderName;

	// Create directory if it's not there
	CreateDirectory( folderPath.c_str(), NULL );

	Reset();
}

bool CustomGameModeConfig::ReadFile( const char *fileName ) {

	error = "";

	std::string filePath = folderPath + "\\" + std::string( fileName ) + ".txt";

	int lineCount = 0;

	std::ifstream inp( filePath );
	if ( !inp.is_open( ) ) {
		char errorCString[1024];
		sprintf_s( errorCString, "Config file '%s.txt' doesn't exist in '%s' directory.\n", fileName, folderPath.c_str() );
		OnError( std::string( errorCString ) );

		return false;
	}

	bool readingSection = false;

	std::string line;
	while ( std::getline( inp, line ) && error.size() == 0 ) {
		lineCount++;
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
				sprintf_s( errorCString, "Error parsing cfg\\%s.txt, line %d: unknown section [%s]\n", configName.c_str(), lineCount, sectionName.str( 1 ).c_str() );
				OnError( std::string( errorCString ) );
			
			};
		} else {
			OnSectionData( line, lineCount );
		}

		if ( error.size() > 0 ) {
			inp.close();
			return false;
		}
	}

	inp.close(); // TODO: find out if it's called automatically

	configName = fileName;
	return true;
}

// This function is called in CHalfLifeRules constructor
// to ensure we won't get a leftover variable value in the default gamemode.
void CustomGameModeConfig::Reset() {
	
	this->configName.clear();
	this->error.clear();

	this->markedForRestart = false;

	this->name.clear();
	this->startMap.clear();
	this->endMap.clear();
	
	this->loadout.clear();
	
	this->timerPauses.clear();
	this->timerResumes.clear();
	this->endTriggers.clear();
	this->sounds.clear();
	this->entitySpawns.clear();
	this->entitiesToPrecache.clear();
	this->soundsToPrecache.clear();

	this->startPositionSpecified = false;
	this->startYawSpecified = false;

	this->difficulty = GAME_DIFFICULTY_MEDIUM;

	this->constantSlowmotion = false;
	this->infiniteSlowmotion = false;
	this->emptySlowmotion = false;
	this->noSlowmotion = false;

	this->noSaving = false;

	this->holdTimer = false;

	this->powerfulHeadshots = false;
	this->infiniteAmmo = false;
	this->weaponRestricted = false;
	this->instaGib = false;

	this->bulletPhysicsDisabled = false;
	this->bulletPhysicsEnemiesAndPlayerOnSlowmotion = false;
	this->bulletPhysicsConstant = false;
}

bool CustomGameModeConfig::OnNewSection( std::string sectionName ) {

	if ( sectionName == "start_map" ) {
		currentFileSection = FILE_SECTION_START_MAP;
	} else if ( sectionName == "end_map" ) {
		currentFileSection = FILE_SECTION_END_MAP;
	} else if ( sectionName == "loadout" ) {
		currentFileSection = FILE_SECTION_LOADOUT;
	} else if ( sectionName == "start_position" ) {
		currentFileSection = FILE_SECTION_START_POSITION;
		startPositionSpecified = true;
	} else if ( sectionName == "name" ) {
		currentFileSection = FILE_SECTION_NAME;
	} else if ( sectionName == "end_trigger" ) {
		currentFileSection = FILE_SECTION_END_TRIGGER;
	} else if ( sectionName == "entity_spawn" ) {
		currentFileSection = FILE_SECTION_ENTITY_SPAWN;
	} else if ( sectionName == "mods" ) {
		currentFileSection = FILE_SECTION_MODS;
	} else if ( sectionName == "timer_pause" ) {
		currentFileSection = FILE_SECTION_TIMER_PAUSE;
	} else if ( sectionName == "timer_resume" ) {
		currentFileSection = FILE_SECTION_TIMER_RESUME;
	} else if ( sectionName == "sound" ) {
		currentFileSection = FILE_SECTION_SOUND;
	} else if ( sectionName == "max_commentary" ) {
		currentFileSection = FILE_SECTION_MAX_COMMENTARY;
	} else {
		return false;
	}

	return true;
}

void CustomGameModeConfig::OnSectionData( std::string line, int lineCount ) {

	switch ( currentFileSection ) {
		case FILE_SECTION_START_MAP: {
			
			startMap = line;
			currentFileSection = FILE_SECTION_NO_SECTION;
			break;

		}

		case FILE_SECTION_END_MAP: {
		
			endMap = line;
			currentFileSection = FILE_SECTION_NO_SECTION;
			break;

		}

		case FILE_SECTION_LOADOUT: {
			
			std::vector<std::string> loadoutStrings = Split( line, ' ' );

			std::string itemName = loadoutStrings.at( 0 );
			if ( GetAllowedItemIndex( itemName.c_str() ) == -1 ) {
				char errorCString[1024];
				sprintf_s( errorCString, "Error parsing cfg\\%s.txt, line %d: incorrect loadout item name in [loadout] section: %s\n", configName.c_str(), lineCount, itemName.c_str() );
				OnError( errorCString );
				return;
			}

			int itemCount = 1;

			if ( loadoutStrings.size() > 1 ) {
				try {
					itemCount = std::stoi( loadoutStrings.at( 1 ) );
				} catch ( std::invalid_argument e ) {
					char errorCString[1024];
					sprintf_s( errorCString, "Error parsing cfg\\%s.txt, line %d: loadout item count incorrectly specified.\n", configName.c_str(), lineCount );
					OnError( errorCString );
					return;
				}
			}

			for ( int i = 0; i < itemCount; i++ ) {
				loadout.push_back( itemName );
			}

			break;

		}

		case FILE_SECTION_START_POSITION: {

			std::vector<std::string> startPositionValueStrings = Split( line, ' ' );
			std::vector<float> startPositionValues;
			for ( size_t i = 0; i < startPositionValueStrings.size( ); i++ ) {
				std::string startPositionValueString = startPositionValueStrings.at( i );
				float startPositionValue;
				try {
					startPositionValue = std::stof( startPositionValueString );
				}
				catch ( std::invalid_argument e ) {
					char errorCString[1024];
					sprintf_s( errorCString, "Error parsing cfg\\%s.txt, line %d: incorrect value in [startposition] section: %s\n", configName.c_str(), lineCount, startPositionValueString.c_str() );
					OnError( errorCString );
					return;
				}

				startPositionValues.push_back( startPositionValue );
			}

			if ( startPositionValues.size() < 3 ) {
				char errorCString[1024];
				sprintf_s( errorCString, "Error parsing cfg\\%s.txt, line %d: not enough coordinates provided in [startposition] section\n", configName.c_str(), lineCount );
				OnError( errorCString );
				return;
			}

			startPosition[0] = startPositionValues.at( 0 );
			startPosition[1] = startPositionValues.at( 1 );
			startPosition[2] = startPositionValues.at( 2 );

			if ( startPositionValues.size() >= 4 ) {
				startYawSpecified = true;
				startYaw = startPositionValues.at( 3 );
			}

			currentFileSection = FILE_SECTION_NO_SECTION;
			break;

		}

		case FILE_SECTION_NAME: {

			name = Uppercase( line );
			if ( name.size() > 54 ) {
				name = name.substr( 0, 53 );
			}
			currentFileSection = FILE_SECTION_NO_SECTION;
			break;

		}

		case FILE_SECTION_END_TRIGGER: {

			ModelIndex modelIndex = ParseModelIndexString( line, lineCount );
			endTriggers.insert( modelIndex );

			break;

		}

		case FILE_SECTION_TIMER_PAUSE: {

			ModelIndex modelIndex = ParseModelIndexString( line, lineCount );
			timerPauses.insert( modelIndex );

			break;

		}

		case FILE_SECTION_TIMER_RESUME: {

			ModelIndex modelIndex = ParseModelIndexString( line, lineCount );
			timerResumes.insert( modelIndex );

			break;

		}

		case FILE_SECTION_SOUND:
		case FILE_SECTION_MAX_COMMENTARY: {

			bool isMaxCommentary = currentFileSection == FILE_SECTION_MAX_COMMENTARY;

			ModelIndexWithSound modelIndex = ParseModelIndexWithSoundString( line, lineCount, isMaxCommentary );
			sounds.insert( modelIndex );

			soundsToPrecache.insert( modelIndex.soundPath );

			break;

		}

		case FILE_SECTION_ENTITY_SPAWN: {
			std::vector<std::string> entitySpawnStrings = Split( line, ' ' );

			if ( entitySpawnStrings.size() < 5 ) {
				char errorCString[1024];
				sprintf_s( errorCString, "Error parsing cfg\\%s.txt, line %d: <mapname> <entityname> <x> <y> <z> [angle] not specified in [entityspawn] section.\n", configName.c_str(), lineCount );
				OnError( errorCString );
				return;
			}

			std::string mapName = entitySpawnStrings.at( 0 );

			std::string entityName = entitySpawnStrings.at( 1 );
			if ( GetAllowedEntityIndex( entityName.c_str() ) == -1 ) {
				char errorCString[1024];
				sprintf_s( errorCString, "Error parsing cfg\\%s.txt, line %d: incorrect entity name in [entityspawn] section: %s\n", configName.c_str(), lineCount, entityName.c_str() );
				OnError( errorCString );
				return;
			}

			float entityAngle = 0;

			std::vector<float> originValues;
			for ( size_t i = 2; i < entitySpawnStrings.size( ); i++ ) {
				std::string entitySpawnString = entitySpawnStrings.at( i );
				float originValue;
				try {
					originValue = std::stof( entitySpawnString );
				}
				catch ( std::invalid_argument e ) {
					char errorCString[1024];
					sprintf_s( errorCString, "Error parsing cfg\\%s.txt, line %d: incorrect value in [entityspawn] section: %s\n", configName.c_str(), lineCount, entitySpawnString.c_str() );
					OnError( errorCString );
					return;
				}

				originValues.push_back( originValue );
			}

			if ( originValues.size() >= 4 ) {
				entityAngle = originValues.at( 3 );
			}

			entitySpawns.push_back( { mapName, entityName, { originValues.at( 0 ), originValues.at( 1 ), originValues.at( 2 ) }, entityAngle } );
			entitiesToPrecache.insert( entityName );
			break;

		}

		case FILE_SECTION_MODS: {

			if ( line == "easy" ) {
				difficulty = GAME_DIFFICULTY_EASY;
			} else if ( line == "hard" ) {
				difficulty = GAME_DIFFICULTY_HARD;
			} else if ( line == "constant_slowmotion" ) {
				constantSlowmotion = true;
			} else if ( line == "infinite_slowmotion" ) {
				infiniteSlowmotion = true;
			} else if ( line == "empty_slowmotion" ) {
				emptySlowmotion = true;
			} else if ( line == "no_slowmotion" ) {
				noSlowmotion = true;
			} else if ( line == "headshots" ) {
				powerfulHeadshots = true;
			} else if ( line == "infinite_ammo" ) {
				infiniteAmmo = true;
			} else if ( line == "weapon_restricted" ) {
				weaponRestricted = true;
			} else if ( line == "instagib" ) {
				instaGib = true;
			} else if ( line == "hold_timer" ) {
				holdTimer = true;
			} else if ( line == "no_saving" ) {
				noSaving = true;
			} else if ( line == "bullet_physics_disabled" ) {
				bulletPhysicsDisabled = true;
			} else if ( line == "bullet_physics_enemies_and_player_on_slowmotion" ) {
				bulletPhysicsEnemiesAndPlayerOnSlowmotion = true;
			} else if ( line == "bullet_physics_constant" ) {
				bulletPhysicsConstant = true;
			} else {
				char errorCString[1024];
				sprintf_s( errorCString, "Error parsing cfg\\%s.txt, line %d: incorrect mod specified in [mods] section: %s\n", configName.c_str(), lineCount, line.c_str() );
				OnError( errorCString );
				return;
			}

			break;

		}

		case FILE_SECTION_NO_SECTION:
		default: {

			char errorCString[1024];
			sprintf_s( errorCString, "Error parsing cfg\\%s.txt, line %d: preceding section not specified.\n", configName.c_str(), lineCount );
			OnError( errorCString );
			break;

		}
	}
}

const ModelIndex CustomGameModeConfig::ParseModelIndexString( std::string line, int lineCount ) {
	std::vector<std::string> modelIndexStrings = Split( line, ' ' );

	if ( modelIndexStrings.size() < 2 ) {
		char errorCString[1024];
		sprintf_s( errorCString, "Error parsing cfg\\%s.txt, line %d: <mapname> <modelindex> not specified.\n", configName.c_str(), lineCount );
		OnError( errorCString );
		return ModelIndex();
	}

	std::string mapName = modelIndexStrings.at( 0 );
	int modelIndex;
	try {
		modelIndex = std::stoi( modelIndexStrings.at( 1 ) );
	} catch ( std::invalid_argument ) {
		char errorCString[1024];
		sprintf_s( errorCString, "Error parsing cfg\\%s.txt, line %d: model index incorrectly specified.\n", configName.c_str(), lineCount );
		OnError( errorCString );
		return ModelIndex();
	}

	bool constant = false;
	if ( modelIndexStrings.size() >= 3 ) {
		std::string constantString = modelIndexStrings.at( 2 );
		if ( constantString == "const" ) {
			constant = true;
		}
	}

	return ModelIndex( mapName, modelIndex, constant );
}

// TODO: This is dumb copy of parser above, would simplify
const ModelIndexWithSound CustomGameModeConfig::ParseModelIndexWithSoundString( std::string line, int lineCount, bool isMaxCommentary ) {
	std::deque<std::string> modelIndexStrings = SplitDeque( line, ' ' );

	if ( modelIndexStrings.size() < 3 ) {
		char errorCString[1024];
		sprintf_s( errorCString, "Error parsing cfg\\%s.txt, line %d: <mapname> <modelindex> <soundpath> not specified.\n", configName.c_str(), lineCount );
		OnError( errorCString );
		return ModelIndexWithSound();
	}

	std::string mapName = modelIndexStrings.at( 0 );
	modelIndexStrings.pop_front();

	int modelIndex;
	try {
		modelIndex = std::stoi( modelIndexStrings.at( 0 ) );
	} catch ( std::invalid_argument ) {
		char errorCString[1024];
		sprintf_s( errorCString, "Error parsing cfg\\%s.txt, line %d: model index incorrectly specified.\n", configName.c_str(), lineCount );
		OnError( errorCString );
		return ModelIndexWithSound();
	}
	modelIndexStrings.pop_front();


	std::string soundPath = modelIndexStrings.at( 0 );
	modelIndexStrings.pop_front();

	bool constant = false;
	float delay = 0.0f;
	
	while ( modelIndexStrings.size() > 0 ) {
		std::string constantString = modelIndexStrings.at( 0 );
		if ( constantString == "const" ) {
			constant = true;
		} else {
			delay = 0.0f;
			try {
				delay = std::stof( modelIndexStrings.at( 0 ) );
			} catch ( std::invalid_argument ) {
				char errorCString[1024];
				sprintf_s( errorCString, "Error parsing cfg\\%s.txt, line %d: delay incorrectly specified.\n", configName.c_str(), lineCount );
				OnError( errorCString );
				return ModelIndexWithSound();
			}
		}

		modelIndexStrings.pop_front();
	}

	// this forced delay is required for CHANGE_LEVEL calls, becuase they mess
	// up with global time and player's sound queue is not able to catch it.
	if ( modelIndex == CHANGE_LEVEL_MODEL_INDEX && delay < 0.101f ) {
		delay = 0.101f;
	}

	return ModelIndexWithSound( mapName, modelIndex, soundPath, isMaxCommentary, delay, constant );
}

bool operator < ( const ModelIndex &modelIndex1, const ModelIndex &modelIndex2 ) {
	return modelIndex1.key < modelIndex2.key;
}