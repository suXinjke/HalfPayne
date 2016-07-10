#ifndef BMM_CONFIG_H
#define BMM_CONFIG_H

#include "Windows.h"
EXTERN_C IMAGE_DOS_HEADER __ImageBase;

#include <string>
#include <vector>
#include <fstream>
#include <algorithm>

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

static std::vector<std::string> Split( const std::string &text, char sep ) {
	std::vector<std::string> tokens;
	std::size_t start = 0, end = 0;
	while ( ( end = text.find( sep, start ) ) != std::string::npos ) {
		std::string temp = text.substr( start, end - start );
		if ( temp != "" ) {
			tokens.push_back( temp );
		}
		start = end + 1;
	}
	std::string temp = text.substr( start );
	if ( temp != "" ) {
		tokens.push_back( temp );
	}
	return tokens;
}

static std::string Uppercase( std::string text )
{
    std::transform(text.begin(), text.end(), text.begin(), ::toupper);

    return text;
}

class BlackMesaMinuteConfig {

public:
	enum BMM_FILE_SECTION {
		BMM_FILE_SECTION_NO_SECTION,
		BMM_FILE_SECTION_START_MAP,
		BMM_FILE_SECTION_END_MAP,
		BMM_FILE_SECTION_LOADOUT,
		BMM_FILE_SECTION_START_POSITION,
		BMM_FILE_SECTION_NAME,
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
				} else if ( line == "[startposition]" ) {
					currentFileSection = BMM_FILE_SECTION_START_POSITION;
					startPositionSpecified = true;
					continue;
				} else if ( line == "[name]" ) {
					currentFileSection = BMM_FILE_SECTION_NAME;
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
	std::string name;
	std::vector<std::string> loadout;

	bool startPositionSpecified = false;
	Vector startPosition;
	
	bool startYawSpecified = false;
	float startYaw = 0.0f;
};


#endif

