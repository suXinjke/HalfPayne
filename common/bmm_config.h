#ifndef BMM_CONFIG_H
#define BMM_CONFIG_H

#include "Windows.h"
EXTERN_C IMAGE_DOS_HEADER __ImageBase;

#include <string>
#include <vector>
#include <fstream>

static std::string Trim( const std::string& str,
	const std::string& whitespace = " \t" ) {
	const auto strBegin = str.find_first_not_of( whitespace );
	if ( strBegin == std::string::npos ) {
		return "";    // no content
	}

	const auto strEnd = str.find_last_not_of( whitespace );
	const auto strRange = strEnd - strBegin + 1;

	return str.substr( strBegin, strRange );
}

class BlackMesaMinuteConfig {

public:
	enum BMM_FILE_SECTION {
		BMM_FILE_SECTION_NO_SECTION,
		BMM_FILE_SECTION_START_MAP,
		BMM_FILE_SECTION_END_MAP,
		BMM_FILE_SECTION_LOADOUT
	};

	bool Init( const char *configName ) {

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
			char errorCString[1024];
			sprintf( errorCString, "Black Mesa Minute config file %s.txt doesn't exist in bmm_cfg directory.\n", configName );
			error = std::string( errorCString );

			return 0;
		}

		std::string line;
		while ( std::getline( inp, line ) ) {
			lineCount++;
			line = Trim( line );

			if ( line.empty( ) || line.find( "//" ) == 0 ) {
				continue;
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
			} else {
				if ( line == "[startmap]" ) {
					currentFileSection = BMM_FILE_SECTION_START_MAP;
					continue;
				} else if ( line == "[endmap]" ) {
					currentFileSection = BMM_FILE_SECTION_END_MAP;
					continue;
				} else if ( line == "[loadout]" ) {
					currentFileSection = BMM_FILE_SECTION_LOADOUT;
					continue;
				} else {
					char errorCString[1024];
					sprintf( errorCString, "Error parsing bmm_cfg\\%s.txt, line %d: preceding section not specified.\n", configName, lineCount );
					error = std::string( errorCString );
					break;
				}
			}
		}

		if ( error.empty( ) ) {
			return true;
		} else {
			return false;
		}
	}

	std::string error;
	std::string startMap;
	std::string endMap;
	std::vector<std::string> loadout;
};


#endif

