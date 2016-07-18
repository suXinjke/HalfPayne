#include "extdll.h"
#include "util.h"

#include "Windows.h"
EXTERN_C IMAGE_DOS_HEADER __ImageBase;

#include "bmm_config.h"
#include <fstream>

BlackMesaMinuteConfig gBMMConfig;

BlackMesaMinuteRecord::BlackMesaMinuteRecord( const char *recordName ) {

	// Retrieve absolute path to the DLL that calls this function
	WCHAR dllPathWString[MAX_PATH] = { 0 };
	GetModuleFileNameW( ( HINSTANCE ) &__ImageBase, dllPathWString, _countof( dllPathWString ) );
	char dllPathCString[MAX_PATH];
	wcstombs( dllPathCString, dllPathWString, MAX_PATH );
	filePath = dllPathCString;

	// dumb way to get absolute path to Black Mesa Minute record file
	filePath = filePath.substr( 0, filePath.rfind( "\\dll\\hl.dll" ) ).append( "\\bmm_records\\" + std::string( recordName ) + ".bmmr" );

    std::ifstream inp( filePath, std::ios::in | std::ios::binary );
    if ( !inp.is_open() ) {
		time = 0.0f;
        realTime = DEFAULT_TIME;
        realTimeMinusTime = DEFAULT_TIME;

        Save();
    } else {
        inp.read( ( char * ) &time, sizeof( float ) );
        inp.read( ( char * ) &realTime, sizeof( float ) );
        inp.read( ( char * ) &realTimeMinusTime, sizeof( float ) );
    }
}

void BlackMesaMinuteRecord::Save() {
    std::ofstream out( filePath, std::ios::out | std::ios::binary );

    out.write( (char *) &time, sizeof( float ) );
    out.write( (char *) &realTime, sizeof( float ) );
    out.write( (char *) &realTimeMinusTime, sizeof( float ) );

    out.close();
}

BlackMesaMinuteConfig::BlackMesaMinuteConfig()
{
	// Retrieve absolute path to the DLL that calls this function
	WCHAR dllPathWString[MAX_PATH] = { 0 };
	GetModuleFileNameW( ( HINSTANCE ) &__ImageBase, dllPathWString, _countof( dllPathWString ) );
	char dllPathCString[MAX_PATH];
	wcstombs( dllPathCString, dllPathWString, MAX_PATH );
	std::string dllPath( dllPathCString );

	// dumb way to get absolute path to Black Mesa Minute configuration file
#ifdef CLIENT_DLL
	configFolderPath = dllPath.substr( 0, dllPath.rfind( "\\cl_dlls\\client.dll" ) ).append( "\\bmm_cfg\\" );
#else
	configFolderPath = dllPath.substr( 0, dllPath.rfind( "\\dll\\hl.dll" ) ).append( "\\bmm_cfg\\" );
#endif
}

bool BlackMesaMinuteConfig::Init( const char *configName ) {

	this->error.clear();
	this->startMap.clear();
	this->endMap.clear();
	this->name.clear();
	this->configName.clear();
	this->loadout.clear();

	this->timerPauses.clear();
	this->timerResumes.clear();
	this->endTriggers.clear();
	this->entitySpawns.clear();
	this->entitiesToPrecache.clear();

	this->startPositionSpecified = false;
	this->startYawSpecified = false;

	this->difficulty = BMM_DIFFICULTY_MEDIUM;

	this->constantSlowmotion = false;
	this->infiniteSlowmotion = false;
	this->emptySlowmotion = false;
	this->noSlowmotion = false;

	this->powerfulHeadshots = false;

	std::string configPath = configFolderPath + ( std::string( configName ) + ".txt" );
	
	BMM_FILE_SECTION currentFileSection = BMM_FILE_SECTION_NO_SECTION;
	int lineCount = 0;

	std::ifstream inp( configPath );
	if ( !inp.is_open( ) ) {
		char errorCString[1024];
		sprintf( errorCString, "Black Mesa Minute config file %s.txt doesn't exist in bmm_cfg directory.\n", configName );
		error = std::string( errorCString );

		return 0;
	}

	this->configName = configName;

	std::string line;
	while ( std::getline( inp, line ) ) {
		lineCount++;
		line = Trim( line );

		// remove trailing comments
		line = line.substr( 0, line.find( "//" ) );

		if ( line.empty() ) {
			continue;
		}

		if ( line == "[startmap]" ) {
			currentFileSection = BMM_FILE_SECTION_START_MAP;
			continue;
		} else if ( line == "[endmap]" ) {
			currentFileSection = BMM_FILE_SECTION_END_MAP;
			continue;
		} else if ( line == "[loadout]" ) {
			currentFileSection = BMM_FILE_SECTION_LOADOUT;
			continue;
		} else if ( line == "[startposition]" ) {
			currentFileSection = BMM_FILE_SECTION_START_POSITION;
			startPositionSpecified = true;
			continue;
		} else if ( line == "[name]" ) {
			currentFileSection = BMM_FILE_SECTION_NAME;
			continue;
		} else if ( line == "[timerpause]" ) {
			currentFileSection = BMM_FILE_SECTION_TIMER_PAUSE;
			continue;
		} else if ( line == "[timerresume]" ) {
			currentFileSection = BMM_FILE_SECTION_TIMER_RESUME;
			continue;
		} else if ( line == "[endtrigger]" ) {
			currentFileSection = BMM_FILE_SECTION_END_TRIGGER;
			continue;
		} else if ( line == "[entityspawn]" ) {
			currentFileSection = BMM_FILE_SECTION_ENTITY_SPAWN;
			continue;
		} else if ( line == "[mods]" ) {
			currentFileSection = BMM_FILE_SECTION_MODS;
			continue;
		} else {
			if ( currentFileSection == BMM_FILE_SECTION_NO_SECTION ) {
				char errorCString[1024];
				sprintf( errorCString, "Error parsing bmm_cfg\\%s.txt, line %d: preceding section not specified.\n", configName, lineCount );
				error = std::string( errorCString );
				return false;
			}
		}

		if ( currentFileSection == BMM_FILE_SECTION_START_MAP ) {
			startMap = line;
			currentFileSection = BMM_FILE_SECTION_NO_SECTION;
			continue;
		} else if ( currentFileSection == BMM_FILE_SECTION_END_MAP ) {
			endMap = line;
			currentFileSection = BMM_FILE_SECTION_NO_SECTION;
			continue;
		} else if ( currentFileSection == BMM_FILE_SECTION_LOADOUT ) {
			std::vector<std::string> loadoutStrings = Split( line, ' ' );
			std::string itemName = loadoutStrings.at( 0 );
			
			int itemCount = 1;

			if ( loadoutStrings.size() > 1 ) {
				try {
					itemCount = std::stof( loadoutStrings.at( 1 ) );
				} catch ( std::invalid_argument e ) {
					char errorCString[1024];
					sprintf( errorCString, "Error parsing bmm_cfg\\%s.txt, line %d: loadout item count incorrectly specified.\n", configName, lineCount );
					error = std::string( errorCString );
					return false;
				}
			}
			
			for ( int i = 0; i < itemCount; i++ ) {
				loadout.push_back( itemName );
			}
			
			continue;
		} else if ( currentFileSection == BMM_FILE_SECTION_START_POSITION ) {
			std::vector<std::string> startPositionValueStrings = Split( line, ' ' );
			std::vector<float> startPositionValues;
			for ( int i = 0; i < startPositionValueStrings.size( ); i++ ) {
				std::string startPositionValueString = startPositionValueStrings.at( i );
				float startPositionValue;
				try {
					startPositionValue = std::stof( startPositionValueString );
				}
				catch ( std::invalid_argument e ) {
					char errorCString[1024];
					sprintf( errorCString, "Error parsing bmm_cfg\\%s.txt, line %d: incorrect value in [startposition] section: %s\n", configName, lineCount, startPositionValueString.c_str() );
					error = std::string( errorCString );
					return false;
				}

				startPositionValues.push_back( startPositionValue );
			}

			if ( startPositionValues.size() < 3 ) {
				char errorCString[1024];
				sprintf( errorCString, "Error parsing bmm_cfg\\%s.txt, line %d: not enough coordinates provided in [startposition] section\n", configName, lineCount );
				error = std::string( errorCString );
				return false;
			}

			startPosition[0] = startPositionValues.at( 0 );
			startPosition[1] = startPositionValues.at( 1 );
			startPosition[2] = startPositionValues.at( 2 );

			if ( startPositionValues.size() >= 4 ) {
				startYawSpecified = true;
				startYaw = startPositionValues.at( 3 );
			}

			currentFileSection = BMM_FILE_SECTION_NO_SECTION;
			continue;
		} else if ( currentFileSection == BMM_FILE_SECTION_NAME ) {
			name = Uppercase( line );
			if ( name.size() > 54 ) {
				name = name.substr( 0, 53 );
			}
			currentFileSection = BMM_FILE_SECTION_NO_SECTION;
			continue;
		} else if ( 
			currentFileSection == BMM_FILE_SECTION_TIMER_PAUSE ||
			currentFileSection == BMM_FILE_SECTION_TIMER_RESUME ||
			currentFileSection == BMM_FILE_SECTION_END_TRIGGER ) {
			std::vector<std::string> modelIndexStrings = Split( line, ' ' );

			if ( modelIndexStrings.size() < 2 ) {
				char errorCString[1024];
				sprintf( errorCString, "Error parsing bmm_cfg\\%s.txt, line %d: <mapname> <modelindex> not specified.\n", configName, lineCount );
				error = std::string( errorCString );
				return false;
			}

			std::string mapName = modelIndexStrings.at( 0 );
			int modelIndex;
			try {
				modelIndex = std::stoi( modelIndexStrings.at( 1 ) );
			} catch ( std::invalid_argument ) {
				char errorCString[1024];
				sprintf( errorCString, "Error parsing bmm_cfg\\%s.txt, line %d: model index incorrectly specified.\n", configName, lineCount );
				error = std::string( errorCString );
				return false;
			}

			if ( currentFileSection == BMM_FILE_SECTION_TIMER_PAUSE ) {
				timerPauses.insert( ModelIndex( mapName, modelIndex ) );
			} else if ( currentFileSection == BMM_FILE_SECTION_TIMER_RESUME ) {
				timerResumes.insert( ModelIndex( mapName, modelIndex ) );
			} else if ( currentFileSection == BMM_FILE_SECTION_END_TRIGGER ) {
				endTriggers.insert( ModelIndex( mapName, modelIndex ) );
			}
		} else if ( currentFileSection == BMM_FILE_SECTION_ENTITY_SPAWN ) {
			std::vector<std::string> entitySpawnStrings = Split( line, ' ' );

			if ( entitySpawnStrings.size() < 5 ) {
				char errorCString[1024];
				sprintf( errorCString, "Error parsing bmm_cfg\\%s.txt, line %d: <mapname> <entityname> <x> <y> <z> [angle] not specified.\n", configName, lineCount );
				error = std::string( errorCString );
				return false;
			}

			std::string mapName = entitySpawnStrings.at( 0 );
			std::string entityName = entitySpawnStrings.at( 1 );
			Vector entityOrigin;
			int entityAngle = 0;

			std::vector<float> originValues;
			for ( int i = 2; i < entitySpawnStrings.size( ); i++ ) {
				std::string entitySpawnString = entitySpawnStrings.at( i );
				float originValue;
				try {
					originValue = std::stof( entitySpawnString );
				}
				catch ( std::invalid_argument e ) {
					char errorCString[1024];
					sprintf( errorCString, "Error parsing bmm_cfg\\%s.txt, line %d: incorrect value in [entityspawn] section: %s\n", configName, lineCount, entitySpawnString.c_str() );
					error = std::string( errorCString );
					return false;
				}

				originValues.push_back( originValue );
			}

			entityOrigin[0] = originValues.at( 0 );
			entityOrigin[1] = originValues.at( 1 );
			entityOrigin[2] = originValues.at( 2 );

			if ( originValues.size() >= 4 ) {
				entityAngle = originValues.at( 3 );
			}

			entitySpawns.push_back( { mapName, entityName, entityOrigin, entityAngle } );

			entitiesToPrecache.insert( entityName );
		} else if ( currentFileSection == BMM_FILE_SECTION_MODS ) {
			if ( line == "easy" ) {
				difficulty = BMM_DIFFICULTY_EASY;
			} else if ( line == "hard" ) {
				difficulty = BMM_DIFFICULTY_HARD;
			} else if ( line == "constantslowmotion" ) {
				constantSlowmotion = true;
			} else if ( line == "infiniteslowmotion" ) {
				infiniteSlowmotion = true;
			} else if ( line == "emptyslowmotion" ) {
				emptySlowmotion = true;
			} else if ( line == "noslowmotion" ) {
				noSlowmotion = true;
			} else if ( line == "headshots" ) {
				powerfulHeadshots = true;
			} else {
				char errorCString[1024];
				sprintf( errorCString, "Error parsing bmm_cfg\\%s.txt, line %d: incorrect mod specified in [mods] section: %s\n", configName, lineCount, line.c_str() );
				error = std::string( errorCString );
				return false;
			}
		}
	}

	return true;
}

// Used only by client
std::string BlackMesaMinuteConfig::GetMapNameFromConfig( const char *configName )
{
	// Retrieve absolute path to the DLL that calls this function
	WCHAR dllPathWString[MAX_PATH] = { 0 };
	GetModuleFileNameW( ( HINSTANCE ) &__ImageBase, dllPathWString, _countof( dllPathWString ) );
	char dllPathCString[MAX_PATH];
	wcstombs( dllPathCString, dllPathWString, MAX_PATH );
	std::string dllPath( dllPathCString );

	// dumb way to get absolute path to Black Mesa Minute configuration file
#ifdef CLIENT_DLL
	dllPath = dllPath.substr( 0, dllPath.rfind( "\\cl_dlls\\client.dll" ) ).append( "\\bmm_cfg\\" + std::string( configName ) + ".txt" );
#else
	dllPath = dllPath.substr( 0, dllPath.rfind( "\\dll\\hl.dll" ) ).append( "\\bmm_cfg\\" + std::string( configName ) + ".txt" );
#endif
		
	BMM_FILE_SECTION currentFileSection = BMM_FILE_SECTION_NO_SECTION;
	int lineCount = 0;

	std::ifstream inp( dllPath );
	if ( !inp.is_open( ) ) {
		return std::string();
	}

	std::string line;
	std::string startMap;
	while ( std::getline( inp, line ) ) {
		lineCount++;

		line = Trim( line );
		
		// remove trailing comments
		line = line.substr( 0, line.find( "//" ) );

		if ( line.empty() ) {
			continue;
		}

		if ( line == "[startmap]" ) {
			currentFileSection = BMM_FILE_SECTION_START_MAP;
			continue;
		}

		if ( currentFileSection == BMM_FILE_SECTION_START_MAP ) {
			startMap = line;
			
			return startMap;
		}
	}

	return startMap;
}

bool operator < ( const BlackMesaMinuteConfig::ModelIndex &modelIndex1, const BlackMesaMinuteConfig::ModelIndex &modelIndex2 ) {
	return modelIndex1.key < modelIndex2.key;
}