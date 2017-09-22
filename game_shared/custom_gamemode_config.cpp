#include "custom_gamemode_config.h"
#include "string_aux.h"
#include <fstream>
#include <regex>
#include <sstream>
#include "sha1.h"
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
		[]( ConfigSectionData &data ) {
			std::string sanitizedName = data.argsString.at( 0 );
			if ( sanitizedName.size() > 54 ) {
				sanitizedName = sanitizedName.substr( 0, 53 );
			}

			data.argsString[0] = sanitizedName;

			return "";
		}
	);

	configSections[CONFIG_FILE_SECTION_DESCRIPTION] = ConfigSection(
		"description", false,
		[]( ConfigSectionData &data ) {
			if ( data.argsString.size() == 0 ) {
				return "description not provided";
			}

			return "";
		}
	);

	configSections[CONFIG_FILE_SECTION_START_MAP] = ConfigSection(
		"start_map", true,
		[]( ConfigSectionData &data ) { return ""; }
	);

	configSections[CONFIG_FILE_SECTION_START_POSITION] = ConfigSection(
		"start_position", true,
		[]( ConfigSectionData &data ) {

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

			return std::string( "" );
		}
	);



	configSections[CONFIG_FILE_SECTION_END_MAP] = ConfigSection(
		"end_map", true,
		[]( ConfigSectionData &data ) { return ""; }
	);

	configSections[CONFIG_FILE_SECTION_END_TRIGGER] = ConfigSection(
		"end_trigger", false,
		[this]( ConfigSectionData &data ) { return ValidateModelIndexSectionData( data ); }
	);

	configSections[CONFIG_FILE_SECTION_CHANGE_LEVEL_PREVENT] = ConfigSection(
		"change_level_prevent", false,
		[this]( ConfigSectionData &data ) { return ""; }
	);

	configSections[CONFIG_FILE_SECTION_LOADOUT] = ConfigSection(
		"loadout", false,
		[]( ConfigSectionData &data ) {
			
			std::string itemName = data.argsString.at( 0 );
			if ( GetAllowedItemIndex( itemName.c_str() ) == -1 ) {
				char error[1024];
				sprintf_s( error, "incorrect loadout item name: %s", itemName.c_str() );
				return std::string( error );
			}

			if ( data.argsFloat.size() >= 2 ) {
				float arg = data.argsFloat.at( 1 );
				if ( std::isnan( arg ) ) {
					return std::string( "loadout item count incorrectly specified" );
				}
			}

			return std::string( "" );
		}
	);

	configSections[CONFIG_FILE_SECTION_ENTITY_SPAWN] = ConfigSection(
		"entity_spawn", false,
		[this]( ConfigSectionData &data ) {
			if ( data.argsString.size() < 5 ) {
				return std::string( "<map_name> <entity_name> <x> <y> <z> [angle] [target_name] not specified" );
			}

			std::string entityName = data.argsString.at( 1 );
			if ( GetAllowedEntityIndex( entityName.c_str() ) == -1 ) {
				char errorCString[1024];
				sprintf_s( errorCString, "incorrect entity name" );
				return std::string( errorCString );
			}

			for ( size_t i = 2 ; i < min( data.argsFloat.size(), 5 ) ; i++ ) {
				float arg = data.argsFloat.at( i );
				if ( std::isnan( arg ) ) {
					char error[1024];
					sprintf_s( error, "invalid coordinate by index %d", i + 1 );
					return std::string( error );
				}
			}

			entitiesToPrecache.insert( entityName );

			return std::string( "" );
		}
	);

	configSections[CONFIG_FILE_SECTION_ENTITY_USE] = ConfigSection(
		"entity_use", false,
		[this]( ConfigSectionData &data ) { return ValidateModelIndexSectionData( data ); }
	);

	configSections[CONFIG_FILE_SECTION_ENTITY_PREVENT] = ConfigSection(
		"sound_prevent", false,
		[this]( ConfigSectionData &data ) { return ValidateModelIndexSectionData( data ); }
	);

	configSections[CONFIG_FILE_SECTION_SOUND] = ConfigSection(
		"sound", false,
		[this]( ConfigSectionData &data ) { return ValidateModelIndexWithSoundSectionData( data ); }
	);

	configSections[CONFIG_FILE_SECTION_MUSIC] = ConfigSection(
		"music", false,
		[this]( ConfigSectionData &data ) { return ValidateModelIndexWithMusicSectionData( data ); }
	);

	configSections[CONFIG_FILE_SECTION_PLAYLIST] = ConfigSection(
		"playlist", false,
		[this]( ConfigSectionData &data ) {
			for ( auto line : data.argsString ) {
				if ( line == "shuffle" ) {
					musicPlaylistShuffle = true;
					continue;
				}

				WIN32_FIND_DATA fdFile;
				HANDLE hFind = NULL;

				if ( ( hFind = FindFirstFile( line.c_str(), &fdFile ) ) == INVALID_HANDLE_VALUE ) {
					continue;
				}

				if ( strcmp( fdFile.cFileName, "." ) != 0 && strcmp( fdFile.cFileName, ".." ) != 0 ) {
					if ( fdFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {
						auto vec = GetAllFileNames( data.argsString.at( 0 ).c_str(), { ".wav", ".ogg", ".mp3" }, true );
						musicPlaylist.insert( musicPlaylist.end(), vec.begin(), vec.end() );
					} else {
						musicPlaylist.push_back( line );
					}
				}
			}

			return "";
		}
	);

	configSections[CONFIG_FILE_SECTION_MAX_COMMENTARY] = ConfigSection(
		"max_commentary", false,
		[this]( ConfigSectionData &data ) { return ValidateModelIndexWithSoundSectionData( data ); }
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
		[this]( ConfigSectionData &data ) { return ValidateModelIndexSectionData( data ); }
	);

	configSections[CONFIG_FILE_SECTION_TIMER_RESUME] = ConfigSection(
		"timer_resume", false,
		[this]( ConfigSectionData &data ) { return ValidateModelIndexSectionData( data ); }
	);

	configSections[CONFIG_FILE_SECTION_INTERMISSION] = ConfigSection(
		"intermission", false,
		[this]( ConfigSectionData &data ) {
			for ( auto line : data.argsString ) {
				if ( data.argsString.size() < 3 ) {
					return std::string( "<map_name> <model_index | target_name | next_map_name> <real_next_map_name> [x] [y] [z] [angle] [stripped] not specified" );
				}

				for ( size_t i = 3 ; i < min( data.argsFloat.size(), 6 ) ; i++ ) {
					float arg = data.argsFloat.at( i );
					if ( std::isnan( arg ) ) {
						char error[1024];
						sprintf_s( error, "invalid coordinate by index %d", i + 1 );
						return std::string( error );
					}
				}
			}
			return std::string( "" );
		}
	);

	configSections[CONFIG_FILE_SECTION_ENTITY_RANDOM_SPAWNER] = ConfigSection(
		"entity_random_spawner", false,
		[this]( ConfigSectionData &data ) {
			for ( auto line : data.argsString ) {
				if ( data.argsString.size() < 2 ) {
					return std::string( "<map_name | everywhere> <entity_name> [max_amount] [spawn_period]" );
				}

				std::string entityName = data.argsString.at( 1 );
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
				}

				entitiesToPrecache.insert( entityName );
			}
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

std::string CustomGameModeConfig::ValidateModelIndexWithSoundSectionData( ConfigSectionData &data ) {

	if ( data.argsString.size() < 3 ) {
		return "<mapname> <modelindex | targetname> <sound_path> [delay] [const] not specified";
	}

	if ( data.argsFloat.size() >= 4 ) {
		float arg = data.argsFloat.at( 3 );
		if ( std::isnan( arg ) ) {
			return "delay incorrectly specified";
		}
	}

	soundsToPrecache.insert( data.argsString.at( 2 ) );

	return "";
}

std::string CustomGameModeConfig::ValidateModelIndexWithMusicSectionData( ConfigSectionData &data ) {

	if ( data.argsString.size() < 3 ) {
		return "<mapname> <modelindex | targetname> <sound_path> [delay] [initial_pos] [const] [looping] [no_slowmotion_effects] not specified";
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
	return GetAllFileNames( folderPath.c_str(), ".txt" );
}

std::vector<std::string> CustomGameModeConfig::GetAllFileNames( const char *path, const std::vector<std::string> &extensions, bool includeExtension ) {
	std::vector<std::string> result;
	for ( auto extension : extensions ) {
		auto extensionVector = GetAllFileNames( path, extension.c_str(), includeExtension );
		result.insert( result.end(), extensionVector.begin(), extensionVector.end() );
	}

	return result;
}

// Based on this answer
// http://stackoverflow.com/questions/2314542/listing-directory-contents-using-c-and-windows
std::vector<std::string> CustomGameModeConfig::GetAllFileNames( const char *path, const char *extension, bool includeExtension ) {

	std::vector<std::string> result;

	WIN32_FIND_DATA fdFile;
	HANDLE hFind = NULL;

	char sPath[2048];
	sprintf( sPath, "%s\\*.*", path );

	if ( ( hFind = FindFirstFile( sPath, &fdFile ) ) == INVALID_HANDLE_VALUE ) {
		return std::vector<std::string>();
	}

	do {
		if ( strcmp( fdFile.cFileName, "." ) != 0 && strcmp( fdFile.cFileName, ".." ) != 0 ) {
			
			sprintf( sPath, "%s\\%s", path, fdFile.cFileName );

			if ( fdFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {
				std::vector<std::string> sub = GetAllFileNames( sPath, extension, includeExtension );
				result.insert( result.end(), sub.begin(), sub.end() );
			} else {
				std::string path = sPath;
				std::string pathSubstring = folderPath + "\\";
				std::string extensionSubstring = extension;

				std::string::size_type pos = path.find( pathSubstring );
				if ( pos != std::string::npos ) {
					path.erase( pos, pathSubstring.length() );
				}

				pos = path.rfind( extensionSubstring );
				if ( pos != std::string::npos ) {

					if ( !includeExtension ) {
						path.erase( pos, extensionSubstring.length() );
					}
					result.push_back( path );
				}
			}
		}
	} while ( FindNextFile( hFind, &fdFile ) );

	FindClose( hFind );

	return result;
}

bool CustomGameModeConfig::ReadFile( const char *fileName ) {

	error = "";
	Reset();

	configName = fileName;
	std::string filePath = folderPath + "\\" + std::string( fileName ) + ".txt";

	int lineCount = 0;

	std::ifstream inp( filePath );
	if ( !inp.is_open( ) ) {
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
			};
		} else {
			std::string error = configSections[currentFileSection].OnSectionData( configName, line, lineCount, configType );
			if ( error.size() > 0 ) {
				OnError( error );
				return false;
			}
		}

		if ( error.size() > 0 ) {
			inp.close();
			return false;
		}
	}

	sha1 = GetHash();

	const std::string recordDirectoryPath = GetGamePath() + "\\records\\";
	const std::string recordFileName = CustomGameModeConfig::ConfigTypeToGameModeCommand( configType ) + "_" + configName +  + "_" + sha1 + ".hpr";
	gameFinishedOnce = record.Read( recordDirectoryPath, recordFileName );

	inp.close(); // TODO: find out if it's called automatically

	return true;
}

const std::string CustomGameModeConfig::GetHash() {
	std::ostringstream result;

	for ( const auto &section : configSections ) {
		for ( const auto &sectionData : section.second.data ) {
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
	this->error.clear();
	this->musicPlaylist.clear();
	musicPlaylistShuffle = false;
	gameFinishedOnce = false;

	this->markedForRestart = false;

	this->entitiesToPrecache.clear();
	this->soundsToPrecache.clear();
}

bool CustomGameModeConfig::IsGameplayModActive( GAMEPLAY_MOD mod ) {
	auto result = std::find_if( mods.begin(), mods.end(), [mod]( const GameplayMod &gameplayMod ) {
		return gameplayMod.id == mod;
	} );

	return result != std::end( mods );
}

bool CustomGameModeConfig::AddGameplayMod( ConfigSectionData &data ) {
	std::string modName = data.argsString.at( 0 );

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
			"Crossbow bolts explode when then hit the wall.",
			[]( CBasePlayer *player ) {
				player->crossbowExplosiveBolts = true;
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
				"Max vertical wobble: " + std::to_string( aimMaxOffsetX ) + " deg\n",
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
				regenerationMax = min( max( 0, data.argsFloat.at( i ) ), 100 );
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
			"Don't allow to walk, crouch, swim, dive, climb ladders.",
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
			"If you have no weapons - you can only have one.\n"
			"You can have several weapons at once if they are specified in [loadout] section.\n"
			"Weapon stripping doesn't affect you.",
			[]( CBasePlayer *player ) { player->weaponRestricted = true; }
		) );
		return true;
	}

	return false;
}

bool CustomGameModeConfig::OnNewSection( std::string sectionName ) {

	for ( const auto &configSection : configSections ) {
		if ( configSection.second.name == sectionName ) {
			currentFileSection = configSection.first;
			return true;
		}
	}

	return true;
}

const std::string CustomGameModeConfig::GetName() {
	ConfigSection section = configSections[CONFIG_FILE_SECTION_NAME];
	return section.data.size() == 0 ? "" : section.data.at( 0 ).line;
}

const std::string CustomGameModeConfig::GetDescription() {
	std::string result = "";
	ConfigSection section = configSections[CONFIG_FILE_SECTION_DESCRIPTION];
	for ( auto line : section.data ) {
		result += line.line + "\n";
	}

	return result;
}

const std::string CustomGameModeConfig::GetStartMap() {
	ConfigSection section = configSections[CONFIG_FILE_SECTION_START_MAP];
	return section.data.size() == 0 ? "" : section.data.at( 0 ).line;
}

const std::string CustomGameModeConfig::GetEndMap() {
	ConfigSection section = configSections[CONFIG_FILE_SECTION_END_MAP];
	return section.data.size() == 0 ? "" : section.data.at( 0 ).line;
}

const StartPosition CustomGameModeConfig::GetStartPosition() {
	ConfigSection section = configSections[CONFIG_FILE_SECTION_START_POSITION];
	if ( section.data.size() == 0 ) {
		return { false, NAN, NAN, NAN, NAN };
	}

	std::vector<float> args = section.data.at( 0 ).argsFloat;
	float x = args.at( 0 );
	float y = args.at( 1 );
	float z = args.at( 2 );
	float angle = args.size() > 3 ? args.at( 3 ) : NAN;

	return { true, x, y, z, angle };
}

const Intermission CustomGameModeConfig::GetIntermission( const std::string &mapName, int modelIndex, const std::string &targetName ) {
	ConfigSection section = configSections[CONFIG_FILE_SECTION_INTERMISSION];
	if ( section.data.size() == 0 ) {
		return { false, "", NAN, NAN, NAN, NAN, false };
	}

	for ( auto data : section.data ) {
		std::string storedMapName = data.argsString.at( 0 );
		int storedModelIndex = std::isnan( data.argsFloat.at( 1 ) ) ? -2 : data.argsFloat.at( 1 );
		std::string storedTargetName = data.argsString.at( 1 );
		std::string storedToMap = data.argsString.at( 2 );

		if ( 
			mapName != storedMapName ||
			modelIndex != storedModelIndex &&
			( storedTargetName != targetName || storedTargetName.size() == 0 )
		) {
			continue;
		}

		float x = data.argsFloat.size() > 3 ? data.argsFloat.at( 3 ) : NAN;
		float y = data.argsFloat.size() > 4 ? data.argsFloat.at( 4 ) : NAN;
		float z = data.argsFloat.size() > 5 ? data.argsFloat.at( 5 ) : NAN;
		float angle = data.argsFloat.size() > 6 ? data.argsFloat.at( 6 ) : NAN;

		bool stripped = data.argsFloat.size() > 7 ? data.argsString.at( 7 ) == "strip" : false;

		return { true, storedToMap, x, y, z, angle, stripped };
	}

	return { false, "", NAN, NAN, NAN, NAN, false };
}

const std::vector<std::string> CustomGameModeConfig::GetLoadout() {
	std::vector<std::string> loadout;
	for ( auto data : configSections[CONFIG_FILE_SECTION_LOADOUT].data ) {
		int itemCount = 1;

		if ( data.argsFloat.size() >= 2 ) {
			itemCount = ceil( data.argsFloat.at( 1 ) );
		}

		for ( int i = 0 ; i < itemCount ; i++ ) {
			loadout.push_back( data.argsString.at( 0 ) );
		}
	}

	return loadout;
}

const std::vector<EntityRandomSpawner> CustomGameModeConfig::GetEntityRandomSpawners() {
	std::vector<EntityRandomSpawner> result;
	for ( auto line : configSections[CONFIG_FILE_SECTION_ENTITY_RANDOM_SPAWNER].data ) {
		const std::string mapName = line.argsString.at( 0 );
		const std::string entityName = line.argsString.at( 1 );
		const int maxAmount = line.argsFloat.size() >= 3 ? line.argsFloat.at( 2 ) : 50;
		const float spawnPeriod = line.argsFloat.size() >= 4 ? line.argsFloat.at( 3 ) : 2.0f;
		
		result.push_back( { mapName, entityName, maxAmount, spawnPeriod } );
	}

	return result;
}

const std::vector<EntitySpawn> CustomGameModeConfig::GetEntitySpawnsForMapOnce( const std::string &map ) {
	std::vector<EntitySpawn> result;
	auto *sectionData = &configSections[CONFIG_FILE_SECTION_ENTITY_SPAWN].data;

	auto i = sectionData->begin();
	while ( i != sectionData->end() ) {
		const std::string storedMapName = i->argsString.at( 0 );
		if ( storedMapName != map ) {
			i++;
			continue;
		}

		const std::string entityName = i->argsString.at( 1 );
		const float x = i->argsFloat.at( 2 );
		const float y = i->argsFloat.at( 3 );
		const float z = i->argsFloat.at( 4 );
		const float angle = i->argsFloat.size() >= 6 ? i->argsFloat.at( 5 ) : 0.0f;
		const std::string targetName = i->argsFloat.size() >= 7 ? i->argsString.at( 6 ) : "";

		result.push_back( { entityName, x, y, z, angle, targetName } );

		i = sectionData->erase( i );
	}

	return result;
}

bool CustomGameModeConfig::MarkModelIndex( CONFIG_FILE_SECTION fileSection, const std::string &mapName, int modelIndex, const std::string &targetName ) {
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

		bool constant = false;
		if ( i->argsString.size() >= 3 ) {
			constant = i->argsString.at( 2 ) == "const";
		}

		if ( !constant ) {
			i = sectionData->erase( i );
		}

		return true;
	}

	return false;
}

const std::vector<ConfigFileSound> CustomGameModeConfig::MarkModelIndexesWithSound( CONFIG_FILE_SECTION fileSection, const std::string &mapName, int modelIndex, const std::string &targetName ) {
	std::vector<ConfigFileSound> result;
	
	auto *sectionData = &configSections[fileSection].data;
	auto i = sectionData->begin();
	while ( i != sectionData->end() ) {
		std::string storedMapName = i->argsString.at( 0 );
		int storedModelIndex = std::isnan( i->argsFloat.at( 1 ) ) ? -2 : i->argsFloat.at( 1 );
		std::string storedTargetName = i->argsString.at( 1 );
		std::string storedSoundPath = i->argsString.at( 2 );

		if ( 
			mapName != storedMapName ||
			modelIndex != storedModelIndex &&
			( storedTargetName != targetName || storedTargetName.size() == 0 )
		) {
			i++;
			continue;
		}

		bool constant = false;
		float delay = 0.0f;
		if ( i->argsString.size() >= 4 ) {
			for ( size_t arg = 3 ; arg < i->argsString.size() ; arg++ ) {
				constant = constant || i->argsString.at( arg ) == "const";
				delay = std::isnan( i->argsFloat.at( arg ) ) ? 0.0f : i->argsFloat.at( arg );
			}
		}

		if ( modelIndex == CHANGE_LEVEL_MODEL_INDEX && delay < 0.101f ) {
			delay = 0.101f;
		}

		result.push_back( { true, storedSoundPath, constant, delay } );
		
		if ( !constant ) {
			i = sectionData->erase( i );
		} else {
			i++;
		}
	}

	return result;
}

const std::vector<ConfigFileMusic> CustomGameModeConfig::MarkModelIndexesWithMusic( CONFIG_FILE_SECTION fileSection, const std::string &mapName, int modelIndex, const std::string &targetName ) {
	std::vector<ConfigFileMusic> result;
	
	auto *sectionData = &configSections[fileSection].data;
	auto i = sectionData->begin();
	while ( i != sectionData->end() ) {
		std::string storedMapName = i->argsString.at( 0 );
		int storedModelIndex = std::isnan( i->argsFloat.at( 1 ) ) ? -2 : i->argsFloat.at( 1 );
		std::string storedTargetName = i->argsString.at( 1 );
		std::string storedSoundPath = i->argsString.at( 2 );

		if ( 
			mapName != storedMapName ||
			modelIndex != storedModelIndex &&
			( storedTargetName != targetName || storedTargetName.size() == 0 )
		) {
			i++;
			continue;
		}

		bool constant = false;
		bool looping = false;
		bool noSlowmotionEffects = false;
		float delay = NAN;
		float initialPos = NAN;
		for ( size_t arg = 3 ; arg < i->argsString.size() ; arg++ ) {
			constant = constant || i->argsString.at( arg ) == "const";
			looping = looping || i->argsString.at( arg ) == "looping";
			noSlowmotionEffects = noSlowmotionEffects || i->argsString.at( arg ) == "no_slowmotion_effects";
			if ( std::isnan( delay ) && !std::isnan( i->argsFloat.at( arg ) ) ) {
				delay = i->argsFloat.at( arg );
			} else if ( !std::isnan( delay ) && !std::isnan( i->argsFloat.at( arg ) ) ) {
				initialPos = i->argsFloat.at( arg );
			}
		}
		if ( std::isnan( delay ) ) {
			delay = 0.0f;
		}
		if ( std::isnan( initialPos ) ) {
			initialPos = 0.0f;
		}

		if ( modelIndex == CHANGE_LEVEL_MODEL_INDEX && delay < 0.101f ) {
			delay = 0.101f;
		}

		result.push_back( { true, storedSoundPath, constant, looping, noSlowmotionEffects, delay, initialPos } );

		if ( !constant ) {
			i = sectionData->erase( i );
		} else {
			i++;
		}
	}

	return result;
}

bool CustomGameModeConfig::IsChangeLevelPrevented( const std::string &nextMap ) {
	for ( auto data : configSections[CONFIG_FILE_SECTION_CHANGE_LEVEL_PREVENT].data ) {
		if ( data.line == nextMap ) {
			return true;
		}
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
	sectionData.argsString = Split( line, ' ' );
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