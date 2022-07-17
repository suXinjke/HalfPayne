#include "wrect.h"
#include "cl_dll.h"
#include "FontAwesome.h"
#include <map>
#include <algorithm>
#include "cpp_aux.h"
#include <Windows.h>
#include <filesystem>
#include "fs_aux.h"
#include "../fmt/printf.h"

#include "shared_memory.h"
#include "gamemode_gui.h"

std::vector<std::pair<std::string, std::string>>chapterMaps = {
	{ "c0a0", "BLACK MESA INBOUND" },
	{ "c0a0a", "BLACK MESA INBOUND" },
	{ "c0a0b", "BLACK MESA INBOUND" },
	{ "c0a0c", "BLACK MESA INBOUND" },
	{ "c0a0d", "BLACK MESA INBOUND" },
	{ "c1a0", "ANOMALOUS MATERIALS" },
	{ "c1a0d", "ANOMALOUS MATERIALS" },
	{ "c1a0a", "ANOMALOUS MATERIALS" },
	{ "c1a0b", "ANOMALOUS MATERIALS" },
	{ "c1a0e", "ANOMALOUS MATERIALS" },
	{ "c1a0c", "UNFORESEEN CONSEQUENCES" },
	{ "c1a1", "UNFORESEEN CONSEQUENCES" },
	{ "c1a1a", "UNFORESEEN CONSEQUENCES" },
	{ "c1a1f", "UNFORESEEN CONSEQUENCES" },
	{ "c1a1b", "UNFORESEEN CONSEQUENCES" },
	{ "c1a1c", "UNFORESEEN CONSEQUENCES" },
	{ "c1a1d", "UNFORESEEN CONSEQUENCES" },
	{ "c1a2", "OFFICE COMPLEX" },
	{ "c1a2d", "OFFICE COMPLEX" },
	{ "c1a2a", "OFFICE COMPLEX" },
	{ "c1a2b", "OFFICE COMPLEX" },
	{ "c1a2c", "OFFICE COMPLEX" },
	{ "c1a3", "WE'VE GOT HOSITLES" },
	{ "c1a3d", "WE'VE GOT HOSITLES" },
	{ "c1a3a", "WE'VE GOT HOSITLES" },
	{ "c1a3b", "WE'VE GOT HOSITLES" },
	{ "c1a3c", "WE'VE GOT HOSITLES" },
	{ "c1a4", "BLAST PIT" },
	{ "c1a4k", "BLAST PIT" },
	{ "c1a4b", "BLAST PIT" },
	{ "c1a4i", "BLAST PIT" },
	{ "c1a4d", "BLAST PIT" },
	{ "c1a4e", "BLAST PIT" },
	{ "c1a4f", "BLAST PIT" },
	{ "c1a4g", "BLAST PIT" },
	{ "c1a4j", "BLAST PIT" },
	{ "c2a1", "POWER UP" },
	{ "c2a1b", "POWER UP" },
	{ "c2a1a", "POWER UP" },
	{ "c2a2", "ON A RAIL" },
	{ "c2a2a", "ON A RAIL" },
	{ "c2a2b2", "ON A RAIL" },
	{ "c2a2b1", "ON A RAIL" },
	{ "c2a2c", "ON A RAIL" },
	{ "c2a2d", "ON A RAIL" },
	{ "c2a2e", "ON A RAIL" },
	{ "c2a2f", "ON A RAIL" },
	{ "c2a2g", "ON A RAIL" },
	{ "c2a2h", "ON A RAIL" },
	{ "c2a3", "APPREHENSION" },
	{ "c2a3a", "APPREHENSION" },
	{ "c2a3b", "APPREHENSION" },
	{ "c2a3c", "APPREHENSION" },
	{ "c2a3d", "APPREHENSION" },
	{ "nightmare", "NIGHTMARE" },
	{ "c2a3e", "APPREHENSION" },
	{ "c2a4", "RESIDUE PROCESSING" },
	{ "c2a4a", "RESIDUE PROCESSING" },
	{ "c2a4b", "RESIDUE PROCESSING" },
	{ "c2a4c", "RESIDUE PROCESSING" },
	{ "c2a4d", "QUESTIONABLE ETHICS" },
	{ "c2a4e", "QUESTIONABLE ETHICS" },
	{ "c2a4f", "QUESTIONABLE ETHICS" },
	{ "c2a4g", "QUESTIONABLE ETHICS" },
	{ "c2a5", "SURFACE TENSION" },
	{ "c2a5w", "SURFACE TENSION" },
	{ "c2a5x", "SURFACE TENSION" },
	{ "c2a5a", "SURFACE TENSION" },
	{ "c2a5b", "SURFACE TENSION" },
	{ "c2a5c", "SURFACE TENSION" },
	{ "c2a5d", "SURFACE TENSION" },
	{ "c2a5e", "SURFACE TENSION" },
	{ "c2a5f", "SURFACE TENSION" },
	{ "c2a5g", "SURFACE TENSION" },
	{ "c3a1", "FORGET ABOUT FREEMAN" },
	{ "c3a1a", "FORGET ABOUT FREEMAN" },
	{ "c3a1b", "FORGET ABOUT FREEMAN" },
	{ "c3a2e", "LAMBDA CORE" },
	{ "c3a2", "LAMBDA CORE" },
	{ "c3a2a", "LAMBDA CORE" },
	{ "c3a2b", "LAMBDA CORE" },
	{ "c3a2c", "LAMBDA CORE" },
	{ "c3a2f", "LAMBDA CORE" },
	{ "c3a2d", "LAMBDA CORE" },
	{ "c4a1", "XEN" },
	{ "c4a2", "GONARCH'S LAIR" },
	{ "c4a2a", "GONARCH'S LAIR" },
	{ "c4a2b", "GONARCH'S LAIR" },
	{ "c4a1a", "INTERLOPER" },
	{ "c4a1b", "INTERLOPER" },
	{ "c4a1c", "INTERLOPER" },
	{ "c4a1d", "INTERLOPER" },
	{ "c4a1e", "INTERLOPER" },
	{ "c4a1f", "INTERLOPER" },
	{ "c4a3", "NIHILANTH" },
	{ "c5a1", "FINALE" },
};

std::vector<std::string> formattedChapterMaps;

extern SharedMemory sharedMemory;

extern SDL_Window *window;

std::map<std::string, std::vector<CustomGameModeConfig>> configs;
std::map<std::string, CustomGameModeConfig> mapConfigs;
std::map<CustomGameModeConfig *, int> mapOverrides;
std::vector<CustomGameModeConfig> vanillaConfigs;

int configAmount;
int configCompleted;
float configCompletedPercent;

CustomGameModeConfig *selectedConfig = nullptr;
bool drawingTwitchSettings = false;

void GameModeGUI_Init() {
	for ( auto &pair : chapterMaps ) {
		auto formattedChapterName = pair.first + " - " + pair.second;
		formattedChapterMaps.push_back( formattedChapterName );

		CustomGameModeConfig vanillaConfig( CONFIG_TYPE_VANILLA );
		vanillaConfig.startMap = pair.first;
		vanillaConfig.name = formattedChapterName;
		vanillaConfig.configName = "vanilla_" + pair.first;
		vanillaConfigs.push_back( vanillaConfig );
	}

	GameModeGUI_RefreshConfigFiles();

	auto twitch_credentials = aux::twitch::readCredentialsFromFile();
	snprintf( sharedMemory.twitchCredentials.login, 128, "%s", twitch_credentials.first.c_str() );
	snprintf( sharedMemory.twitchCredentials.oAuthPassword, 128, "%s", twitch_credentials.second.c_str() );
}

void GameModeGUI_RefreshConfigFiles() {

	selectedConfig = nullptr;
	configs.clear();
	mapConfigs.clear();
	mapOverrides.clear();

	configAmount = 0;
	configCompleted = 0;

	auto configTypes = std::vector<CONFIG_TYPE>( {
		CONFIG_TYPE_MAP,
		CONFIG_TYPE_CGM
	} );

	for ( auto configType : configTypes ) {

		std::vector<std::string> files = CustomGameModeConfig( configType ).GetAllConfigFileNames();

		for ( const auto &file : files ) {
			CustomGameModeConfig config( configType );
			config.ReadFile( file.c_str() );

			if ( configType == CONFIG_TYPE_CGM ) {

				std::string sectionName =
					config.configNameSeparated.size() > 1 ? config.configNameSeparated.at( 0 ) :
					configType == CONFIG_TYPE_CGM ? "Main Game - Variety" :
					"ERROR";

				configs[sectionName].push_back( config );

				configAmount++;
				if ( config.gameFinishedOnce ) {
					configCompleted++;
				}
			} else if ( configType == CONFIG_TYPE_MAP ) {
				mapConfigs.emplace( config.configName, config );
			}
		}

	}

	configCompletedPercent = ( ( ( float ) configCompleted ) / configAmount );
}


void GameModeGUI_RunCustomGameMode( const CustomGameModeConfig &config, const std::string &mapOverride ) {
	if ( config.error.length() > 0 ) {
		return;
	}

	std::string sanitizedConfigName = config.configName;
	std::transform( sanitizedConfigName.begin(), sanitizedConfigName.end(), sanitizedConfigName.begin(), []( auto &letter ) {
		return letter == '\\' ? '/' : letter;
	} );

	// Prepare game mode and try to launch [start_map]
	// Launching the map then will execute CustomGameModeConfig constructor on server,
	// where the file will be parsed again.
	gEngfuncs.Cvar_Set( "gamemode_config", ( char * ) sanitizedConfigName.c_str() );
	gEngfuncs.Cvar_Set( "gamemode", ( char * ) CustomGameModeConfig::ConfigTypeToGameModeCommand( config.configType ).c_str() );

	char mapCmd[64];
	sprintf( mapCmd, "map %s", mapOverride.empty() ? config.startMap.c_str() : mapOverride.c_str() );
	gEngfuncs.pfnClientCmd( mapCmd );
}

void GameModeGUI_DrawMainWindow() {

	//static bool showTestWindow = false;
	//ImGui::SetNextWindowPos( ImVec2( 0, 0 ), ImGuiSetCond_FirstUseEver );
	//ImGui::ShowTestWindow( &showTestWindow );

	static bool showGameModeWindow = false;
	const int GAME_MODE_WINDOW_WIDTH = 760;
	const int GAME_MODE_WINDOW_HEIGHT = 330;
	int RENDERED_WIDTH;
	SDL_GetWindowSize( window, &RENDERED_WIDTH, NULL );
	ImGui::SetNextWindowPos( ImVec2( RENDERED_WIDTH - GAME_MODE_WINDOW_WIDTH, 0 ), ImGuiSetCond_FirstUseEver );
	ImGui::SetNextWindowSize( ImVec2( GAME_MODE_WINDOW_WIDTH, GAME_MODE_WINDOW_HEIGHT ), ImGuiSetCond_FirstUseEver );
	ImGui::Begin( "Half-Payne", &showGameModeWindow );

	{
		// PROGRESSION
		char progressLabel[64];
		sprintf( progressLabel, "Completed %d/%d (%.1f%%)", configCompleted, configAmount, configCompletedPercent * 100 );
		ImGui::ProgressBar( configCompletedPercent, ImVec2( -100.0f, 0.0f ), progressLabel );

		ImGui::SameLine();

		// REFRESH BUTTON
		if ( ImGui::Button( "Refresh", ImVec2( -1, 0 ) ) ) {
			GameModeGUI_RefreshConfigFiles();
		}
	}

	// TABLES
	{
		ImGui::BeginChild( "gamemode_scrollable_data_child", ImVec2( -1, -20 ) );

		ImGui::BeginChild( "LeftHalf", ImVec2( 270, -20 ), false, ImGuiWindowFlags_HorizontalScrollbar ); {
			ImGui::PushStyleColor( ImGuiCol_Header, ImVec4( 0.50f, 0.50f, 0.90f, 1.00f ) );
			ImGui::PushStyleColor( ImGuiCol_HeaderHovered, ImVec4( 0.70f, 0.70f, 0.90f, 0.63f ) );
			ImGui::PushStyleColor( ImGuiCol_HeaderActive, ImVec4( 0.70f, 0.70f, 0.70f, 0.51f ) );
			ImGui::Selectable( "        Twitch integration settings", &drawingTwitchSettings );
			ImGui::PopStyleColor( 3 );

			if ( ImGui::CollapsingHeader( "Main Game - Vanilla" ) ) {
				GameModeGUI_DrawGamemodeConfigTable( vanillaConfigs );
			}

			for ( auto &configSection : configs ) {
				if ( ImGui::CollapsingHeader( configSection.first.c_str() ) ) {
					GameModeGUI_DrawGamemodeConfigTable( configSection.second );
				}
			}
		} ImGui::EndChild();

		ImGui::SameLine();

		ImGui::BeginChild( "RightHalf", ImVec2( 0, -20 ) ); {
			if ( drawingTwitchSettings ) {
				GameModeGUI_DrawTwitchConfig();
			} else if ( selectedConfig ) {
				GameModeGUI_DrawConfigFileInfo( *selectedConfig );
			} else {
				ImGui::TextWrapped( "Select a gameplay mod config file on the left to show it's info" );
			}
		} ImGui::EndChild();

		ImGui::EndChild();
	}

	static std::string launchHint;
	if ( launchHint.empty() ) {
		auto filePath = FS_ResolveModPath( "README_GAME_MODES.txt" );

		if ( filePath.empty() ) {
			launchHint = "Check out README_GAME_MODES.txt for customization";
		} else {
			auto moduleHandle = GetModuleHandle( "hl.exe" );
			char modulePath[MAX_PATH];
			GetModuleFileName( moduleHandle, modulePath, MAX_PATH );

			launchHint = fmt::sprintf(
				"Check out %s for customization\n",
				std::filesystem::relative(
					filePath,
					std::filesystem::path( modulePath ).append( ".." )
				).string()
			);
		}
	}

	ImGui::SetCursorPosX( ImGui::GetWindowWidth() / 2.0f - ImGui::CalcTextSize( launchHint.c_str() ).x / 2.0f );
	ImGui::Text( launchHint.c_str() );

	ImGui::End();
}

void GameModeGUI_DrawGamemodeConfigTable( std::vector<CustomGameModeConfig> &configs ) {

	for ( auto &config : configs ) {
		
		ImGui::PushStyleColor( ImGuiCol_Text,
			!config.error.empty() ? ImVec4( 1.00f, 0.00f, 0.00f, 1.00f ) :
			config.gameFinishedOnce ? ImVec4( 1.00f, 0.66f, 0.00f, 1.00f ) :
			ImVec4( 1.00f, 1.00f, 1.00f, 1.00f )
		);

		auto name = "   " + ( GameModeGUI_GetGameModeConfigName( config ) );

		ImGui::PushID( config.configName.c_str() );
		if ( ImGui::Selectable( name.c_str(), &config == selectedConfig, ImGuiSelectableFlags_AllowDoubleClick ) ) {

			selectedConfig = &config;
			drawingTwitchSettings = false;

			if ( ImGui::IsMouseDoubleClicked( 0 ) ) {
				GameModeGUI_RunCustomGameMode( config );
			}
		}
		if ( !config.error.empty() && ImGui::IsItemHovered() ) {
			ImGui::BeginTooltip(); {
				ImGui::PushTextWrapPos( 300.0f );
				ImGui::Text( config.error.c_str() );
				ImGui::PopTextWrapPos();
			} ImGui::EndTooltip();
		}
		ImGui::PopID();

		ImGui::PopStyleColor();
	}
}

void GameModeGUI_DrawConfigFileInfo( CustomGameModeConfig &config ) {
	if ( !config.error.empty() ) {
		ImGui::TextWrapped( config.error.c_str() );
		return;
	}

	bool overrideMap = mapOverrides.find( &config ) != mapOverrides.end();

	auto map = config.startMap;
	if ( overrideMap ) {
		map = chapterMaps.at( mapOverrides[&config] ).first;
	}

	if ( ImGui::Button( ( "PLAY " + GameModeGUI_GetGameModeConfigName( config ) ).c_str(), ImVec2( -1, 40 ) ) ) {
		GameModeGUI_RunCustomGameMode( config, map );
	}

	ImGui::TextWrapped( "Start map" );
	ImGui::SameLine();

	const std::string sha1 = config.sha1.substr( 0, 7 );
	ImGui::SetCursorPosX( ImGui::GetWindowWidth() - ImGui::CalcTextSize( sha1.c_str() ).x - 16 );
	ImGui::Text( sha1.c_str() );

	ImGui::Separator();
	ImGui::TextWrapped( map.c_str() );

	if ( !config.startPosition.defined && config.configType == CONFIG_TYPE_CGM ) {
		ImGui::SameLine();

		bool overrideMapCheckbox = overrideMap;
		if ( ImGui::Checkbox( "Override map", &overrideMapCheckbox ) ) {
			if ( overrideMapCheckbox ) {
				mapOverrides[&config] = 0;
			} else {
				mapOverrides.erase( mapOverrides.find( &config ) );
				overrideMap = false;
			}
		}
	}

	if ( config.configType == CONFIG_TYPE_VANILLA ) {
		ImGui::TextWrapped( "Difficulty" );
		ImGui::Separator();

		float skill_cvar = gEngfuncs.pfnGetCvarFloat( "skill" );
		int skill =
			skill_cvar >= 3.0f ? 3 :
			skill_cvar >= 2.0f ? 2 :
			1;

		if ( ImGui::RadioButton( "Easy", &skill, 1 ) ) {
			gEngfuncs.Cvar_Set( "skill", "1" );
		}
		ImGui::SameLine();
		if ( ImGui::RadioButton( "Medium", &skill, 2 ) ) {
			gEngfuncs.Cvar_Set( "skill", "2" );
		}
		ImGui::SameLine();
		if ( ImGui::RadioButton( "Hard", &skill, 3 ) ) {
			gEngfuncs.Cvar_Set( "skill", "3" );
		}
	}

	if ( overrideMap ) {
		ImGui::TextWrapped( "Starting on a different map will be considered cheating and your personal bests will not be saved." );

		ImGui::PushItemWidth( -1 );
		ImGui::Combo( "", &mapOverrides[&config], []( void *data, int n, const char **out_text ) -> bool {
			const std::vector<std::string> *maps = ( std::vector<std::string> * ) data;
			*out_text = maps->at( n ).c_str();
			return true;
		}, ( void * ) &formattedChapterMaps, formattedChapterMaps.size(), 15 );
		ImGui::PopItemWidth();
	}

	if ( !config.description.empty() ) {
		ImGui::Text( "\nDescription\n" );
		ImGui::Separator();
		ImGui::TextWrapped( config.description.c_str() );
	}

	if ( config.gameFinishedOnce ) {
		ImGui::Text( "\nPersonal bests\n" );
		ImGui::Separator();

		bool timerBackwards =
			config.mods.find( &gameplayMods::blackMesaMinute ) != config.mods.end() ||
			config.mods.find( &gameplayMods::timeRestriction ) != config.mods.end();

		ImGui::TextWrapped( timerBackwards ? "Time score: %s" : "Time: %s", GameModeGUI_GetFormattedTime( config.record.time ).c_str() );
		ImGui::TextWrapped( "Real time: %s", GameModeGUI_GetFormattedTime( config.record.realTime ).c_str() );
		if ( timerBackwards ) {
			ImGui::Text( "Real time minus score: %s", GameModeGUI_GetFormattedTime( config.record.realTimeMinusTime ).c_str() );
		}
		if ( config.mods.find( &gameplayMods::scoreAttack ) != config.mods.end() ) {
			ImGui::TextWrapped( "Score: %d", config.record.score );
		}
	}

	if ( !config.endConditions.empty() ) {
		ImGui::TextWrapped( "\nObjectives / End conditions\n" );
		ImGui::Separator();
		for ( auto &endCondition : config.endConditions ) {
			ImGui::TextWrapped( "%s%s", endCondition.objective.c_str(), endCondition.activationsRequired > 1 ? ( " (" + std::to_string( endCondition.activationsRequired ) + ")" ).c_str() : "" );
		}
	}

	auto &loadout = config.loadout;
	if ( loadout.empty() ) {
		auto mapConfig = mapConfigs.find( config.startMap );
		if ( mapConfig != mapConfigs.end() ) {
			loadout = mapConfig->second.loadout;
		}
	}

	if ( !loadout.empty() ) {
		static std::map<std::string, std::string> loadoutMap = {
			{ "item_suit", "Suit" },
			{ "item_longjump", "Jump module" },
			{ "item_healthkit", "Painkillers" },

			{ "weapon_crowbar", "Crowbar" },
			{ "weapon_9mmhandgun", "Beretta" },
			{ "weapon_9mmhandgun_twin", "Beretta dual" },
			{ "weapon_ingram", "Ingram" },
			{ "weapon_ingram_twin", "Ingram dual" },
			{ "weapon_357", "Deagle" },
			{ "weapon_9mmAR", "SMG" },
			{ "weapon_m249", "M249" },
			{ "weapon_shotgun", "Shotgun" },
			{ "weapon_crossbow", "Crossbow" },
			{ "weapon_rpg", "RPG" },
			{ "weapon_gauss", "Gauss gun" },
			{ "weapon_egon", "Gluon gun" },
			{ "weapon_hornetgun", "Hornet gun" },
			{ "weapon_handgrenade", "Hand grenades" },
			{ "weapon_satchel", "Remote explosive" },
			{ "weapon_tripmine", "Tripmines" },
			{ "weapon_snark", "Snarks" },
			{ "all", "All weapons and suit" },
		};

		ImGui::Text( "\nLoadout\n" );
		ImGui::Separator();
		for ( auto &item : loadout ) {
			if ( aux::str::startsWith( item.name, "ammo" ) ) {
				continue;
			}

			auto itemName = item.name;
			if ( loadoutMap.find( itemName ) != loadoutMap.end() ) {
				itemName = loadoutMap[itemName];
			}
			ImGui::TextWrapped( itemName.c_str() );

			if ( item.amount > 1 ) {
				ImGui::SameLine( 150 ); ImGui::TextWrapped( "x%d", item.amount );
			}

		}
	}

	if ( !config.mods.empty() ) {
		ImGui::Text( "\nGameplay mods" );
		ImGui::Separator();

		bool first = true;
		for ( const auto &pair : config.mods ) {
			auto &mod = *pair.first;

			ImGui::TextColored( ImVec4( 1, 0.66, 0, 1 ), ( ( first ? "" : "\n" ) + mod.name + "\n" ).c_str() );
			if ( !mod.description.empty() ) {
				ImGui::TextWrapped( mod.description.c_str() );
			}

			for ( const auto &arg : pair.second ) {
				if ( !arg.description.empty() ) {
					ImGui::TextColored( ImVec4( 1, 0.66, 0, 1 ), "   %s", ICON_FA_WRENCH ); ImGui::SameLine();
					ImGui::TextWrapped( arg.description.c_str() );
				}
			}

			first = false;
		}
	}

	if ( !config.randomModsWhitelist.empty() ) {
		ImGui::TextWrapped( "\nRandom gameplay mods whitelist\n" );
		ImGui::Separator();
		for ( auto &mod : config.randomModsWhitelist ) {
			ImGui::TextWrapped( gameplayMods::byString[mod]->name.c_str() );
		}
	}

	if ( !config.randomModsBlacklist.empty() ) {
		ImGui::TextWrapped( "\nRandom gameplay mods blacklist\n" );
		ImGui::Separator();
		for ( auto &mod : config.randomModsBlacklist ) {
			ImGui::TextWrapped( gameplayMods::byString[mod]->name.c_str() );
		}
	}

	if ( !config.entityRandomSpawners.empty() ) {
		ImGui::TextWrapped( "\nRandom spawners\n" );
		ImGui::Separator();
		for ( auto &spawner : config.entityRandomSpawners ) {
			if ( spawner.spawnOnce ) {
				ImGui::TextWrapped(
					"(%s) %s up to %d on first map visit",
					spawner.mapName.c_str(),
					spawner.entity.originalName.c_str(),
					spawner.maxAmount
				);
			} else {
				ImGui::TextWrapped(
					"(%s) %s up to %d every %.0f %s",
					spawner.mapName.c_str(),
					spawner.entity.originalName.c_str(),
					spawner.maxAmount,
					spawner.spawnPeriod,
					spawner.spawnPeriod > 1 ? "seconds" : "second"
				);
			}
		}
	}
}

const std::string GameModeGUI_GetFormattedTime( float time ) {
	float minutes = time / 60.0f;
	int actualMinutes = floor( minutes );
	float seconds = fmod( time, 60.0f );
	float secondsIntegral;
	float secondsFractional = modf( seconds, &secondsIntegral );

	int actualSeconds = secondsIntegral;
	int actualMilliseconds = secondsFractional * 100;

	std::string result;

	if ( actualMinutes < 10 ) {
		result += "0";
	}
	result += std::to_string( actualMinutes ) + ":";

	if ( actualSeconds < 10 ) {
		result += "0";
	}
	result += std::to_string( actualSeconds ) + ".";

	if ( actualMilliseconds < 10 ) {
		result += "0";
	}
	result += std::to_string( actualMilliseconds );

	return result;
}

const std::string GameModeGUI_GetGameModeConfigName( const CustomGameModeConfig & config ) {
	return config.name.empty() ? config.configName : config.name;
}

void GameModeGUI_DrawTwitchConfig() {
	static std::string helpMessage;
	if ( helpMessage.empty() ) {
		helpMessage = fmt::sprintf(
			"Filling both fields and enabling one of the options below will allow "
			"your Twitch viewers to affect your gameplay.\n"
			"This works only in custom gameplay mods.\n\n"
			"Your credentials are stored in %s/twitch_credentials.cfg\n"
			"Don't have the console open while pasting your password to prevent leaking it in console input\n\n"
			"Don't forget to save your login info to avoid entering the credentials every time.",

			FS_GetModDirectoryName()
		);
	}

	ImGui::TextWrapped( helpMessage.c_str() );

	ImGui::InputText( "Twitch username", sharedMemory.twitchCredentials.login, 128 );
	ImGui::InputText( "OAuth password", sharedMemory.twitchCredentials.oAuthPassword, 128, ImGuiInputTextFlags_Password );

	if ( ImGui::Button( "Get OAuth password..." ) ) {
		ShellExecute( 0, 0, "https://twitchapps.com/tmi/", 0, 0, SW_SHOW );
	}
	ImGui::SameLine();

	static int credentialSaveErrorCode = 0;
	if ( ImGui::Button( "Save login info" ) ) {
		try {
			aux::twitch::saveCredentialsToFile( sharedMemory.twitchCredentials.login, sharedMemory.twitchCredentials.oAuthPassword );
		} catch ( std::ifstream::failure e ) {
			credentialSaveErrorCode = GetLastError();
		}

		ImGui::OpenPopup( "Save login info" );
	}

	if ( ImGui::BeginPopupModal( "Save login info" ) ) {

		if ( credentialSaveErrorCode ) {
			ImGui::Text(
				"Failed to save credentials, error code: %d\n"
				"You can still play with the Twitch integration on, but you will\n"
				"have to enter the credentials again after restarting the game\n"
				"%s",

				credentialSaveErrorCode,
				credentialSaveErrorCode == 5 ?
				"Error code 5 may indicate denied access, "
				"check that you don't have read-only flag set on mod directory" :
				""
			);
		} else {
			ImGui::Text( "Successfully saved the credentials" );
		}

		if ( ImGui::Button( "OK" ) ) {
			credentialSaveErrorCode = 0;
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	ImGui::Text( "\n" );

	bool vote_checkbox = gEngfuncs.pfnGetCvarFloat( "twitch_integration_random_gameplay_mods_voting" ) > 0.0f;
	if ( ImGui::Checkbox( "Viewers can vote on random gameplay mods", &vote_checkbox ) ) {
		gEngfuncs.Cvar_Set( "twitch_integration_random_gameplay_mods_voting", vote_checkbox ? "1" : "0" );
	}

	if ( vote_checkbox ) {
		int voting_result = std::string( gEngfuncs.pfnGetCvarString( "twitch_integration_random_gameplay_mods_voting_result" ) ) == "most_votes_wins" ? 0 : 1;
		if ( ImGui::RadioButton( "Voting affects which mod WILL BE activated next", &voting_result, 0 ) ) {
			gEngfuncs.Cvar_Set( "twitch_integration_random_gameplay_mods_voting_result", "most_votes_wins" );
		}
		if ( ImGui::RadioButton( "Voting affects which mod IS MORE LIKELY to be activated next", &voting_result, 1 ) ) {
			gEngfuncs.Cvar_Set( "twitch_integration_random_gameplay_mods_voting_result", "most_votes_more_likely" );
		}
	}

	bool mirror_chat_checkbox = gEngfuncs.pfnGetCvarFloat( "twitch_integration_mirror_chat" ) > 0.0f;
	if ( ImGui::Checkbox( "Relay Twitch chat to Half-Life", &mirror_chat_checkbox ) ) {
		gEngfuncs.Cvar_Set( "twitch_integration_mirror_chat", mirror_chat_checkbox ? "1" : "0" );
	}

	bool say_checkbox = gEngfuncs.pfnGetCvarFloat( "twitch_integration_say" ) > 0.0f;
	if ( ImGui::Checkbox( "Relay say commands to Twitch chat", &say_checkbox ) ) {
		gEngfuncs.Cvar_Set( "twitch_integration_say", say_checkbox ? "1" : "0" );
	}

	bool random_kill_messages_checkbox = gEngfuncs.pfnGetCvarFloat( "twitch_integration_random_kill_messages" ) > 0.0f;
	if ( ImGui::Checkbox( "Show random Twitch chat messages when you kill someone", &random_kill_messages_checkbox ) ) {
		gEngfuncs.Cvar_Set( "twitch_integration_random_kill_messages", random_kill_messages_checkbox ? "1" : "0" );
	}

	bool random_kill_messages_sender_checkbox = gEngfuncs.pfnGetCvarFloat( "twitch_integration_random_kill_messages_sender" ) > 0.0f;
	if ( random_kill_messages_checkbox ) {
		if ( ImGui::Checkbox( "       and also show the SENDER name", &random_kill_messages_sender_checkbox ) ) {
			gEngfuncs.Cvar_Set( "twitch_integration_random_kill_messages_sender", random_kill_messages_sender_checkbox ? "1" : "0" );
		}
	}
}