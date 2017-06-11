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

CustomGameModeConfig::CustomGameModeConfig( GAME_MODE_CONFIG_TYPE configType )
{
	this->configType = configType;

	std::string folderName = ConfigTypeToDirectoryName( configType );

	folderPath = GetGamePath() + "\\" + folderName;

	// Create directory if it's not there
	CreateDirectory( folderPath.c_str(), NULL );

	Reset();
}

std::string CustomGameModeConfig::ConfigTypeToDirectoryName( GAME_MODE_CONFIG_TYPE configType ) {
	switch ( configType ) {
		case GAME_MODE_CONFIG_MAP:
			return "map_cfg";

		case GAME_MODE_CONFIG_CGM:
			return "cgm_cfg";

		case GAME_MODE_CONFIG_BMM:
			return "bmm_cfg";

		case GAME_MODE_CONFIG_SAGM:
			return "sagm_cfg";

		default:
			return "";
	}
}

std::string CustomGameModeConfig::ConfigTypeToGameModeCommand( GAME_MODE_CONFIG_TYPE configType ) {
	switch ( configType ) {
		case GAME_MODE_CONFIG_CGM:
			return "cgm";

		case GAME_MODE_CONFIG_BMM:
			return "bmm";

		case GAME_MODE_CONFIG_SAGM:
			return "sagm";

		default:
			return "";
	}
}

std::string CustomGameModeConfig::ConfigTypeToGameModeName( GAME_MODE_CONFIG_TYPE configType ) {
	switch ( configType ) {
		case GAME_MODE_CONFIG_CGM:
			return "Custom";

		case GAME_MODE_CONFIG_BMM:
			return "Black Mesa Minute";

		case GAME_MODE_CONFIG_SAGM:
			return "Score Attack";

		default:
			return "";
	}
}

std::vector<std::string> CustomGameModeConfig::GetAllConfigFileNames() {
	return GetAllConfigFileNames( folderPath.c_str() );
}

// Based on this answer
// http://stackoverflow.com/questions/2314542/listing-directory-contents-using-c-and-windows
std::vector<std::string> CustomGameModeConfig::GetAllConfigFileNames( const char *path ) {

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
				std::vector<std::string> sub = GetAllConfigFileNames( sPath );
				result.insert( result.end(), sub.begin(), sub.end() );
			} else {
				std::string path = sPath;
				std::string pathSubstring = folderPath + "\\";
				std::string extensionSubstring = ".txt";

				std::string::size_type pos = path.find( pathSubstring );
				if ( pos != std::string::npos ) {
					path.erase( pos, pathSubstring.length() );
				}

				pos = path.rfind( extensionSubstring );
				if ( pos != std::string::npos ) {

					path.erase( pos, extensionSubstring.length() );
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
				sprintf_s( errorCString, "Error parsing %s\\%s.txt, line %d: unknown section [%s]\n", ConfigTypeToDirectoryName( configType ).c_str(), fileName, lineCount, sectionName.str( 1 ).c_str() );
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
	this->entitiesToPrevent.clear();

	this->startPositionSpecified = false;
	this->startYawSpecified = false;
}

bool CustomGameModeConfig::IsGameplayModActive( GAMEPLAY_MOD mod ) {
	auto result = std::find_if( mods.begin(), mods.end(), [mod]( const GameplayMod &gameplayMod ) {
		return gameplayMod.id == mod;
	} );

	return result != std::end( mods );
}

bool CustomGameModeConfig::AddGameplayMod( const std::string &modName ) {
	if ( modName == "bleeding" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_BLEEDING,
			"Bleeding",
			"After 10 seconds of your last painkiller take, you start to lose health until it reaches 20.\n"
			"Health regeneration is turned off.",
			[]( CBasePlayer *player ) { player->isBleeding = true; }
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

	if ( modName == "drunk" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_DRUNK,
			"Drunk",
			"Self explanatory. The camera view becomes wobbly and makes aim harder.\n"
			"Wobble doesn't get slower when slowmotion is present.",
			[]( CBasePlayer *player ) { player->drunk = true; }
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
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_FADING_OUT,
			"Fading out",
			"View is fading out, or in other words it's blacking out until you can't see almost anything.\n"
			"Take painkillers to restore the vision.\n"
			"Allows to take painkillers even when you have 100 health and enough time have passed since the last take.",
			[]( CBasePlayer *player ) { player->isFadingOut = true; }
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

	if ( modName == "garbage_gibs" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_GARBAGE_GIBS,
			"Garbage gibs",
			"Replaces all gibs with garbage.",
			[]( CBasePlayer *player ) { player->garbageGibs = true; }
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

	if ( modName == "no_fall_damage" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_NO_FALL_DAMAGE,
			"No fall damage",
			"Self explanatory.",
			[]( CBasePlayer *player ) { player->noFallDamage = true; }
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
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_SNARK_INCEPTION,
			"Snark inception",
			"Killing snark splits it into two snarks.\n"
			"Snarks are immune to explosions.",
			[]( CBasePlayer *player ) { player->snarkInception = true; }
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

	if ( modName == "snark_paranoia" ) {
		mods.push_back( GameplayMod( 
			GAMEPLAY_MOD_SNARK_PARANOIA,
			"Snark paranoia",
			"Snarks spawn randomly around the map, mostly out of your sight.\n"
			"Spawn positions are determined by world graph.",
			[]( CBasePlayer *player ) { player->snarkParanoia = true; }
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
	} else if ( sectionName == "change_level_prevent" ) {
		currentFileSection = FILE_SECTION_CHANGE_LEVEL_PREVENT;
	} else if ( sectionName == "entity_spawn" ) {
		currentFileSection = FILE_SECTION_ENTITY_SPAWN;
	} else if ( sectionName == "entity_use" ) {
		currentFileSection = FILE_SECTION_ENTITY_USE;
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
	} else if ( sectionName == "sound_prevent" ) {
		currentFileSection = FILE_SECTION_SOUND_PREVENT;
	} else {
		return false;
	}

	return true;
}

void CustomGameModeConfig::OnSectionData( std::string line, int lineCount ) {

	switch ( currentFileSection ) {
		case FILE_SECTION_START_MAP: {
			if ( configType == GAME_MODE_CONFIG_MAP ) {
				char errorCString[1024];
				sprintf_s( errorCString, "Error parsing %s\\%s.txt, line %d: [start_map] section is not allowed for map configs\n", ConfigTypeToDirectoryName( configType ).c_str(), configName.c_str(), lineCount );
				OnError( errorCString );
				return;
			}

			startMap = line;
			currentFileSection = FILE_SECTION_NO_SECTION;
			break;

		}

		case FILE_SECTION_END_MAP: {
			if ( configType == GAME_MODE_CONFIG_MAP ) {
				char errorCString[1024];
				sprintf_s( errorCString, "Error parsing %s\\%s.txt, line %d: [end_map] section is not allowed for map configs\n", ConfigTypeToDirectoryName( configType ).c_str(), configName.c_str(), lineCount );
				OnError( errorCString );
				return;
			}

			endMap = line;
			currentFileSection = FILE_SECTION_NO_SECTION;
			break;

		}

		case FILE_SECTION_LOADOUT: {
			if ( configType == GAME_MODE_CONFIG_MAP ) {
				char errorCString[1024];
				sprintf_s( errorCString, "Error parsing %s\\%s.txt, line %d: [loadout] section is not allowed for map configs\n", ConfigTypeToDirectoryName( configType ).c_str(), configName.c_str(), lineCount );
				OnError( errorCString );
				return;
			}

			std::vector<std::string> loadoutStrings = Split( line, ' ' );

			std::string itemName = loadoutStrings.at( 0 );
			if ( GetAllowedItemIndex( itemName.c_str() ) == -1 ) {
				char errorCString[1024];
				sprintf_s( errorCString, "Error parsing %s\\%s.txt, line %d: incorrect loadout item name in [loadout] section: %s\n", ConfigTypeToDirectoryName( configType ).c_str(), configName.c_str(), lineCount, itemName.c_str() );
				OnError( errorCString );
				return;
			}

			int itemCount = 1;

			if ( loadoutStrings.size() > 1 ) {
				try {
					itemCount = std::stoi( loadoutStrings.at( 1 ) );
				} catch ( std::invalid_argument e ) {
					char errorCString[1024];
					sprintf_s( errorCString, "Error parsing %s\\%s.txt, line %d: loadout item count incorrectly specified\n", ConfigTypeToDirectoryName( configType ).c_str(), configName.c_str(), lineCount );
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
			if ( configType == GAME_MODE_CONFIG_MAP ) {
				char errorCString[1024];
				sprintf_s( errorCString, "Error parsing %s\\%s.txt, line %d: [start_position] section is not allowed for map configs\n", ConfigTypeToDirectoryName( configType ).c_str(), configName.c_str(), lineCount );
				OnError( errorCString );
				return;
			}

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
					sprintf_s( errorCString, "Error parsing %s\\%s.txt, line %d: incorrect value in [start_position] section: %s\n", ConfigTypeToDirectoryName( configType ).c_str(), configName.c_str(), lineCount, startPositionValueString.c_str() );
					OnError( errorCString );
					return;
				}

				startPositionValues.push_back( startPositionValue );
			}

			if ( startPositionValues.size() < 3 ) {
				char errorCString[1024];
				sprintf_s( errorCString, "Error parsing %s\\%s.txt, line %d: not enough coordinates provided in [start_position] section\n", ConfigTypeToDirectoryName( configType ).c_str(), configName.c_str(), lineCount );
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
			if ( configType == GAME_MODE_CONFIG_MAP ) {
				char errorCString[1024];
				sprintf_s( errorCString, "Error parsing %s\\%s.txt, line %d: [name] section is not allowed for map configs\n", ConfigTypeToDirectoryName( configType ).c_str(), configName.c_str(), lineCount );
				OnError( errorCString );
				return;
			}

			name = Uppercase( line );
			if ( name.size() > 54 ) {
				name = name.substr( 0, 53 );
			}
			currentFileSection = FILE_SECTION_NO_SECTION;
			break;

		}

		case FILE_SECTION_END_TRIGGER: {
			if ( configType == GAME_MODE_CONFIG_MAP ) {
				char errorCString[1024];
				sprintf_s( errorCString, "Error parsing %s\\%s.txt, line %d: [end_trigger] section is not allowed for map configs\n", ConfigTypeToDirectoryName( configType ).c_str(), configName.c_str(), lineCount );
				OnError( errorCString );
				return;
			}

			ModelIndex modelIndex = ParseModelIndexString( line, lineCount );
			endTriggers.insert( modelIndex );

			break;

		}

		case FILE_SECTION_TIMER_PAUSE: {
			if ( configType != GAME_MODE_CONFIG_BMM ) {
				char errorCString[1024];
				sprintf_s( errorCString, "Error parsing %s\\%s.txt, line %d: [timer_pause] section is only allowed for Black Mesa Minute configs\n", ConfigTypeToDirectoryName( configType ).c_str(), configName.c_str(), lineCount );
				OnError( errorCString );
				return;
			}

			ModelIndex modelIndex = ParseModelIndexString( line, lineCount );
			timerPauses.insert( modelIndex );

			break;

		}

		case FILE_SECTION_TIMER_RESUME: {
			if ( configType != GAME_MODE_CONFIG_BMM ) {
				char errorCString[1024];
				sprintf_s( errorCString, "Error parsing %s\\%s.txt, line %d: [timer_resume] section is only allowed for Black Mesa Minute configs\n", ConfigTypeToDirectoryName( configType ).c_str(), configName.c_str(), lineCount );
				OnError( errorCString );
				return;
			}

			ModelIndex modelIndex = ParseModelIndexString( line, lineCount );
			timerResumes.insert( modelIndex );

			break;

		}

		case FILE_SECTION_SOUND_PREVENT: {
			if ( configType != GAME_MODE_CONFIG_MAP ) {
				char errorCString[1024];
				sprintf_s( errorCString, "Error parsing %s\\%s.txt, line %d: [sound_prevent] section is only allowed for map configs\n", ConfigTypeToDirectoryName( configType ).c_str(), configName.c_str(), lineCount );
				OnError( errorCString );
				return;
			}

			ModelIndex modelIndex = ParseModelIndexString( line, lineCount );
			entitiesToPrevent.insert( modelIndex );

			break;
		}

		case FILE_SECTION_CHANGE_LEVEL_PREVENT: {
			if ( configType == GAME_MODE_CONFIG_MAP ) {
				char errorCString[1024];
				sprintf_s( errorCString, "Error parsing %s\\%s.txt, line %d: [change_level_prevent] section is not allowed for map configs\n", ConfigTypeToDirectoryName( configType ).c_str(), configName.c_str(), lineCount );
				OnError( errorCString );
				return;
			}

			changeLevelsToPrevent.insert( line );

			break;
		}

		case FILE_SECTION_SOUND:
		case FILE_SECTION_MAX_COMMENTARY: {
			if ( configType != GAME_MODE_CONFIG_MAP ) {
				char errorCString[1024];
				sprintf_s( errorCString, "Error parsing %s\\%s.txt, line %d: [sound] or [max_commentary] sections are only allowed for map configs\n", ConfigTypeToDirectoryName( configType ).c_str(), configName.c_str(), lineCount );
				OnError( errorCString );
				return;
			}

			bool isMaxCommentary = currentFileSection == FILE_SECTION_MAX_COMMENTARY;

			ModelIndexWithSound modelIndex = ParseModelIndexWithSoundString( line, lineCount, isMaxCommentary );
			sounds.insert( modelIndex );

			soundsToPrecache.insert( modelIndex.soundPath );

			break;

		}

		case FILE_SECTION_ENTITY_USE: {
			if ( configType != GAME_MODE_CONFIG_MAP ) {
				char errorCString[1024];
				sprintf_s( errorCString, "Error parsing %s\\%s.txt, line %d: [entity_use] section is only allowed for map configs\n", ConfigTypeToDirectoryName( configType ).c_str(), configName.c_str(), lineCount );
				OnError( errorCString );
				return;
			}

			ModelIndex modelIndex = ParseModelIndexString( line, lineCount );
			entityUses.insert( modelIndex );

			break;

		}

		case FILE_SECTION_ENTITY_SPAWN: {
			if ( configType == GAME_MODE_CONFIG_MAP ) {
				char errorCString[1024];
				sprintf_s( errorCString, "Error parsing %s\\%s.txt, line %d: [entity_spawn] section is not allowed for map configs\n", ConfigTypeToDirectoryName( configType ).c_str(), configName.c_str(), lineCount );
				OnError( errorCString );
				return;
			}

			std::vector<std::string> entitySpawnStrings = Split( line, ' ' );

			if ( entitySpawnStrings.size() < 5 ) {
				char errorCString[1024];
				sprintf_s( errorCString, "Error parsing %s\\%s.txt, line %d: <map_name> <entity_name> <x> <y> <z> [angle] not specified in [entity_spawn] section\n", ConfigTypeToDirectoryName( configType ).c_str(), configName.c_str(), lineCount );
				OnError( errorCString );
				return;
			}

			std::string mapName = entitySpawnStrings.at( 0 );

			std::string entityName = entitySpawnStrings.at( 1 );
			if ( GetAllowedEntityIndex( entityName.c_str() ) == -1 ) {
				char errorCString[1024];
				sprintf_s( errorCString, "Error parsing %s\\%s.txt, line %d: incorrect entity name in [entity_spawn] section: %s\n", ConfigTypeToDirectoryName( configType ).c_str(), configName.c_str(), lineCount, entityName.c_str() );
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
					sprintf_s( errorCString, "Error parsing %s\\%s.txt, line %d: incorrect value in [entity_spawn] section: %s\n", ConfigTypeToDirectoryName( configType ).c_str(), configName.c_str(), lineCount, entitySpawnString.c_str() );
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
			if ( configType == GAME_MODE_CONFIG_MAP ) {
				char errorCString[1024];
				sprintf_s( errorCString, "Error parsing %s\\%s.txt, line %d: [mods] section is not allowed for map configs\n", ConfigTypeToDirectoryName( configType ).c_str(), configName.c_str(), lineCount );
				OnError( errorCString );
				return;
			}

			if ( !AddGameplayMod( line ) ) {
				char errorCString[1024];
				sprintf_s( errorCString, "Error parsing %s\\%s.txt, line %d: incorrect mod specified in [mods] section: %s\n", ConfigTypeToDirectoryName( configType ).c_str(), configName.c_str(), lineCount, line.c_str() );
				OnError( errorCString );
				return;
			}

			break;

		}

		case FILE_SECTION_NO_SECTION:
		default: {

			char errorCString[1024];
			sprintf_s( errorCString, "Error parsing %s\\%s.txt, line %d: preceding section not specified\n", ConfigTypeToDirectoryName( configType ).c_str(), configName.c_str(), lineCount );
			OnError( errorCString );
			break;

		}
	}
}

const ModelIndex CustomGameModeConfig::ParseModelIndexString( std::string line, int lineCount ) {
	std::vector<std::string> modelIndexStrings = Split( line, ' ' );

	if ( modelIndexStrings.size() < 2 ) {
		char errorCString[1024];
		sprintf_s( errorCString, "Error parsing %s\\%s.txt, line %d: <mapname> <modelindex> not specified\n", ConfigTypeToDirectoryName( configType ).c_str(), configName.c_str(), lineCount );
		OnError( errorCString );
		return ModelIndex();
	}

	std::string mapName = modelIndexStrings.at( 0 );
	int modelIndex = -2;
	std::string targetName = "";
	try {
		modelIndex = std::stoi( modelIndexStrings.at( 1 ) );
	} catch ( std::invalid_argument ) {
		targetName = modelIndexStrings.at( 1 );
	}

	bool constant = false;
	if ( modelIndexStrings.size() >= 3 ) {
		std::string constantString = modelIndexStrings.at( 2 );
		if ( constantString == "const" ) {
			constant = true;
		}
	}

	return ModelIndex( mapName, modelIndex, targetName, constant );
}

// TODO: This is dumb copy of parser above, would simplify
const ModelIndexWithSound CustomGameModeConfig::ParseModelIndexWithSoundString( std::string line, int lineCount, bool isMaxCommentary ) {
	std::deque<std::string> modelIndexStrings = SplitDeque( line, ' ' );

	if ( modelIndexStrings.size() < 3 ) {
		char errorCString[1024];
		sprintf_s( errorCString, "Error parsing %s\\%s.txt, line %d: <map_name> <model_index> <sound_path> not specified\n", ConfigTypeToDirectoryName( configType ).c_str(), configName.c_str(), lineCount );
		OnError( errorCString );
		return ModelIndexWithSound();
	}

	std::string mapName = modelIndexStrings.at( 0 );
	modelIndexStrings.pop_front();

	int modelIndex = -2;
	std::string targetName = "";
	try {
		modelIndex = std::stoi( modelIndexStrings.at( 0 ) );
	} catch ( std::invalid_argument ) {
		targetName = modelIndexStrings.at( 0 );
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
				sprintf_s( errorCString, "Error parsing %s\\%s.txt, line %d: delay incorrectly specified\n", ConfigTypeToDirectoryName( configType ).c_str(), configName.c_str(), lineCount );
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

	return ModelIndexWithSound( mapName, modelIndex, targetName, soundPath, isMaxCommentary, delay, constant );
}

bool operator < ( const ModelIndex &modelIndex1, const ModelIndex &modelIndex2 ) {
	return modelIndex1.key < modelIndex2.key;
}