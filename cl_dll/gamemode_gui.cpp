#include "wrect.h"
#include "cl_dll.h"
#include "FontAwesome.h"
#include <map>
#include <algorithm>

#include "gamemode_gui.h"

extern SDL_Window *window;

std::map<std::string, std::vector<CustomGameModeConfig>> configs;

int configAmount;
int configCompleted;
float configCompletedPercent;

void GameModeGUI_Init() {
	GameModeGUI_RefreshConfigFiles();
}

void GameModeGUI_RefreshConfigFiles() {

	configs.clear();

	configAmount = 0;
	configCompleted = 0;

	auto configTypes = std::vector<CONFIG_TYPE>( {
		CONFIG_TYPE_CGM,
		CONFIG_TYPE_BMM,
		CONFIG_TYPE_SAGM
	} );

	for ( auto configType : configTypes ) {

		std::vector<std::string> files = CustomGameModeConfig( configType ).GetAllConfigFileNames();

		for ( const auto &file : files ) {
			CustomGameModeConfig config( configType );
			config.ReadFile( file.c_str() );

			std::string sectionName =
				config.configNameSeparated.size() > 1 ? config.configNameSeparated.at( 0 ) :
				configType == CONFIG_TYPE_CGM  ? "Main Game - Variety" :
				configType == CONFIG_TYPE_BMM  ? "Main Game - Black Mesa Minute" :
				configType == CONFIG_TYPE_SAGM ? "Main Game - Score Attack" :
				"ERROR";

			configs[sectionName].push_back( config );

			configAmount++;
			if ( config.gameFinishedOnce ) {
				configCompleted++;
			}
		}
	}

	configCompletedPercent = ( ( ( float ) configCompleted ) / configAmount );
}


void GameModeGUI_RunCustomGameMode( CustomGameModeConfig &config ) {
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
	sprintf( mapCmd, "map %s", config.startMap.c_str() );
	gEngfuncs.pfnClientCmd( mapCmd );
}

void GameModeGUI_DrawMainWindow() {

	//static bool showTestWindow = false;
	//ImGui::SetNextWindowPos( ImVec2( 0, 0 ), ImGuiSetCond_FirstUseEver );
	//ImGui::ShowTestWindow( &showTestWindow );

	static bool showGameModeWindow = false;
	const int GAME_MODE_WINDOW_WIDTH  = 570;
	const int GAME_MODE_WINDOW_HEIGHT = 330;
	int RENDERED_WIDTH;
	SDL_GetWindowSize( window, &RENDERED_WIDTH, NULL );
	ImGui::SetNextWindowPos( ImVec2( RENDERED_WIDTH - GAME_MODE_WINDOW_WIDTH, 0 ), ImGuiSetCond_FirstUseEver );
	ImGui::SetNextWindowSize( ImVec2( GAME_MODE_WINDOW_WIDTH, GAME_MODE_WINDOW_HEIGHT ), ImGuiSetCond_FirstUseEver );
	ImGui::Begin( "Custom Game Modes", &showGameModeWindow );

	
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

		for ( const auto &configSection : configs ) {
			if ( ImGui::CollapsingHeader( configSection.first.c_str() ) ) {
				GameModeGUI_DrawGamemodeConfigTable( configSection.second );
				ImGui::Columns( 1 );
			}
		}

		ImGui::EndChild();
	}

	const char *launchHint = "Check out half_payne/README_GAME_MODES.txt for customization\n";
	ImGui::SetCursorPosX( ImGui::GetWindowWidth() / 2.0f - ImGui::CalcTextSize( launchHint ).x / 2.0f );
	ImGui::Text( launchHint );

	ImGui::End();
}

void GameModeGUI_DrawGamemodeConfigTable( const std::vector<CustomGameModeConfig> &configs ) {
	
	//const std::vector<CustomGameModeConfig> *configs = GameModeGUI_GameModeConfigVectorFromType( configType );

	{
		ImGui::Columns( 1, "gamemode_hint_column" );

		const char *launchHint = "Double click on the row to launch game mode\n";
		ImGui::SetCursorPosX( ImGui::GetWindowWidth() / 2.0f - ImGui::CalcTextSize( launchHint ).x / 2.0f );
		ImGui::Text( launchHint );

		ImGui::NextColumn();
	}

	// TABLE HEADERS
	{
		ImGui::Columns( 3, "gamemode_header_columns" );
		ImGui::TextColored( ImVec4( 1.0, 1.0, 1.0, 0.0 ), "%s", ICON_FA_CHECK ); ImGui::SameLine(); ImGui::Text( "File" ); ImGui::NextColumn();
		ImGui::Text( "Name" ); ImGui::NextColumn();
		ImGui::Text( "" ); ImGui::NextColumn();
		ImGui::SetColumnOffset( 2, ImGui::GetWindowWidth() - 70 );
		ImGui::Separator();
	}

	// TABLE ITSELF
	{

		for ( size_t i = 0; i < configs.size(); i++ ) {

			CustomGameModeConfig config = configs.at( i );

			const char *file = config.configNameSeparated.at( config.configNameSeparated.size() - 1 ).c_str();
			
			// COMPLETED?
			ImGui::TextColored( ImVec4( 1.0, 1.0, 1.0, config.gameFinishedOnce ? 1.0 : 0.0 ), "%s", ICON_FA_CHECK );
			if ( config.gameFinishedOnce && ImGui::IsItemHovered() ) {
				ImGui::BeginTooltip();

				bool timerBackwards = config.configType == CONFIG_TYPE_BMM || config.IsGameplayModActive( GAMEPLAY_MOD_TIME_RESTRICTION );

				ImGui::Text( timerBackwards ? "Time score: %s" : "Time: %s", GameModeGUI_GetFormattedTime( config.record.time ).c_str() );
				ImGui::Text( "Real time: %s", GameModeGUI_GetFormattedTime( config.record.realTime ).c_str() );
				if ( timerBackwards ) {
					ImGui::Text( "Real time minus score: %s", GameModeGUI_GetFormattedTime( config.record.realTimeMinusTime ).c_str() );
				}
				if ( config.configType == CONFIG_TYPE_SAGM ) {
					ImGui::Text( "Score: %d", config.record.score );
				}

				ImGui::EndTooltip();
			}
			ImGui::SameLine();

			// FILE NAME + ROW
			ImGui::Selectable( file, false, ImGuiSelectableFlags_SpanAllColumns );
			ImGui::SetItemAllowOverlap();
			if ( config.error.length() > 0 ) {
				ImGui::SameLine();
				ImGui::TextColored( ImVec4( 1, 0, 0, 1 ), "error" );
			}
			if ( ImGui::IsItemHovered() ) {
				if ( ImGui::IsMouseDoubleClicked( 0 ) ) {
					GameModeGUI_RunCustomGameMode( config );
				}
			}
			ImGui::NextColumn();

			// CONFIG NAME
			ImGui::Text( "%s", config.name.c_str() ); ImGui::NextColumn();

			// CONFIG FILE INFO
			{
				ImGui::Text( "%s", ICON_FA_INFO_CIRCLE );
				if ( ImGui::IsItemHovered() ) {
					ImGui::BeginTooltip();

					GameModeGUI_DrawConfigFileInfo( config );

					ImGui::EndTooltip();
				}

				const std::string modalKey = config.name.length() > 0 ? config.name : std::string( file ) + std::to_string( i );
				if ( ImGui::IsItemClicked() ) {
					ImGui::OpenPopup( modalKey.c_str() );
				}
				if ( ImGui::BeginPopupModal( modalKey.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize ) ) {
					GameModeGUI_DrawConfigFileInfo( config );

					if ( ImGui::Button( "Close", ImVec2( -1, 0 ) ) ) {
						ImGui::CloseCurrentPopup();
					}
					ImGui::EndPopup();
				}
			}

			// LAUNCH BUTTON
			ImGui::SameLine();
			{
				ImGui::Text( "%s", ICON_FA_ARROW_RIGHT );
				if ( ImGui::IsItemHovered() ) {

					ImGui::BeginTooltip();

					ImGui::Text( "Launch" );
					
					ImGui::EndTooltip();
				}
				if ( ImGui::IsItemClicked() ) {
					GameModeGUI_RunCustomGameMode( config );
				}
			}

			ImGui::NextColumn();

		}
	}
}

void GameModeGUI_DrawConfigFileInfo( CustomGameModeConfig &config ) {

	if ( config.error.length() > 0 ) {
		ImGui::PushTextWrapPos(300.0f);
		ImGui::Text( config.error.c_str() );
		ImGui::PopTextWrapPos();
	} else {
		ImGui::Text( "Start map                   " );
		ImGui::SameLine();

		const std::string sha1 = config.sha1.substr( 0, 7 );
		ImGui::SetCursorPosX( ImGui::GetWindowWidth() - ImGui::CalcTextSize( sha1.c_str() ).x - 12 );
		ImGui::Text( sha1.c_str() );

		ImGui::Text( config.startMap.c_str() );

		if ( !config.description.empty() ) {
			ImGui::Text( "\nDescription\n" );
			ImGui::Text( config.description.c_str() );
		}

		if ( config.configType == CONFIG_TYPE_BMM ) {
			ImGui::TextColored( ImVec4( 1, 0.66, 0, 1 ), "\n\nBlack Mesa Minute\n" );
			ImGui::Text( "Time-based game mode - rush against a minute, kill enemies to get more time.\n" );
		} else if ( config.configType == CONFIG_TYPE_SAGM ) {
			ImGui::TextColored( ImVec4( 1, 0.66, 0, 1 ), "\n\nScore Attack\n" );
			ImGui::Text( "Kill enemies to get as much score as possible. Build combos to get even more score.\n" );
		}

		for ( const auto &pair : config.mods ) {
			const auto &mod = pair.second;
			if ( !mod.active ) {
				continue;
			}

			ImGui::TextColored( ImVec4( 1, 0.66, 0, 1 ), ( "\n" + mod.name + "\n" ).c_str() );
			ImGui::Text( mod.description.c_str() );

			for ( const auto &arg : mod.arguments ) {
				if ( !arg.description.empty() ) {
					ImGui::TextColored( ImVec4( 1, 0.66, 0, 1 ), "   %s", ICON_FA_WRENCH ); ImGui::SameLine();
					ImGui::Text( arg.description.c_str() );

				}
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