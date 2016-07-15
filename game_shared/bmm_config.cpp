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

	this->startPositionSpecified = false;
	this->startYawSpecified = false;

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
		} else {
			if ( currentFileSection == BMM_FILE_SECTION_NO_SECTION ) {
				char errorCString[1024];
				sprintf( errorCString, "Error parsing bmm_cfg\\%s.txt, line %d: preceding section not specified.\n", configName, lineCount );
				error = std::string( errorCString );
				break;
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
			loadout.push_back( line );
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
					sprintf( errorCString, "Error incorrect value in [startposition] section: %s\n", startPositionValueString.c_str() );
					error = std::string( errorCString );
					break;
				}

				startPositionValues.push_back( startPositionValue );
			}

			if ( startPositionValues.size() < 3 ) {
				char errorCString[1024];
				sprintf( errorCString, "Error not enough coordinates provided in [startposition] section\n" );
				error = std::string( errorCString );
				break;
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
		}
	}

	if ( error.empty( ) ) {
		return true;
	} else {
		return false;
	}
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