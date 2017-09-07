#include "wrect.h"
#include "cl_dll.h"
#include <Windows.h>
#include <Psapi.h>
#include "FontAwesome.h"

#include "gamemode_gui.h"

extern cl_enginefunc_t gEngfuncs;
extern int isPaused;
extern bool inMainMenu;
SDL_Window *window = NULL;

std::vector< CustomGameModeConfig > cgmConfigs;
std::vector< CustomGameModeConfig > bmmConfigs;
std::vector< CustomGameModeConfig > sagmConfigs;

int selectedGamemodeTab = CONFIG_TYPE_CGM;

// To draw imgui on top of Half-Life, we take a detour from certain engine's function into GameModeGUI_Draw function
void GameModeGUI_Init() {

	// TODO: figure out if it's possible to avoid crash on startup when using Debug build
#ifdef DEBUG
	return;
#endif

	// One of the final steps before drawing a frame is calling SDL_GL_SwapWindow function
	// It must be prevented, so imgui could be drawn before calling SDL_GL_SwapWindow

	// This will hold the constant address of x86 CALL command, which looks like this
	// E8 FF FF FF FF
	// Last 4 bytes specify an offset from this address + 5 bytes of command itself
	unsigned int origin = NULL;

	// Offsets were found out by using Cheat Engine, let's hope Valve doesn't change the engine much
	// But investing into figuring the exact command location would be great
	MODULEINFO module_info;
	if ( GetModuleInformation( GetCurrentProcess(), GetModuleHandle( "hw.dll" ), &module_info, sizeof( module_info ) ) ) {
		origin = ( unsigned int ) module_info.lpBaseOfDll + 0xA9C40 + 0x2C;

		char opCode[1];
		ReadProcessMemory( GetCurrentProcess(), ( const void * ) origin, opCode, 1, NULL );
		if ( opCode[0] != 0xFFFFFFE8 ) {
			gEngfuncs.Con_DPrintf( "Failed to embed ImGUI: expected CALL OP CODE, but it wasn't there\n" );
			return;
		}
	} else {
		gEngfuncs.Con_DPrintf( "Failed to embed ImGUI: failed to get hw.dll memory base address\n" );
		return;
	}

	window = SDL_GetWindowFromID( 1 );
	ImGui_ImplSdl_Init( window );

	// To make a detour, an offset to dedicated function must be calculated and then correctly replaced
	unsigned int detourFunctionAddress = ( unsigned int ) &GameModeGUI_Draw;
	unsigned int offset = ( detourFunctionAddress ) - origin - 5;

	// The resulting offset must be little endian, so 
	// 0x0A852BA1 => A1 2B 85 0A
	char offsetBytes[4];
	for ( int i = 0; i < 4; i++ ) {
		offsetBytes[i] = ( offset >> ( i * 8 ) );
	}

	// This is WinAPI call, blatantly overwriting the memory with raw pointer would crash the program
	// Notice the 1 byte offset from the origin
	WriteProcessMemory( GetCurrentProcess(), ( void * ) ( origin + 1 ), offsetBytes, 4, NULL );

	SDL_AddEventWatch( GameModeGUI_ProcessEvent, NULL );

	ImGuiStyle *style = &ImGui::GetStyle();
	style->AntiAliasedShapes = false;
	style->WindowRounding = 0.0f;
	style->ScrollbarRounding = 0.0f;

	style->Colors[ImGuiCol_PopupBg]               = ImVec4(0.00f, 0.00f, 0.00f, 0.86f);
	style->Colors[ImGuiCol_TitleBg]               = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);
	style->Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);
	style->Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);
	style->Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.20f, 0.20f, 0.20f, 0.60f);
	style->Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(1.00f, 1.00f, 1.00f, 0.20f);
	style->Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(1.00f, 1.00f, 1.00f, 0.39f);
	style->Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(1.00f, 1.00f, 1.00f, 0.16f);
	style->Colors[ImGuiCol_Button]                = ImVec4(0.63f, 0.63f, 0.63f, 0.60f);
	style->Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.63f, 0.63f, 0.63f, 0.71f);
	style->Colors[ImGuiCol_ButtonActive]          = ImVec4(0.51f, 0.51f, 0.51f, 0.60f);
	style->Colors[ImGuiCol_Header]                = ImVec4(0.32f, 0.32f, 0.32f, 1.00f);
	style->Colors[ImGuiCol_HeaderActive]          = ImVec4(0.53f, 0.53f, 0.53f, 0.51f);
	style->Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.53f, 0.53f, 0.53f, 0.63f);
	style->Colors[ImGuiCol_CloseButton]           = ImVec4(0.50f, 0.50f, 0.90f, 0.00f);
	style->Colors[ImGuiCol_CloseButtonHovered]    = ImVec4(0.70f, 0.70f, 0.90f, 0.00f);
	style->Colors[ImGuiCol_CloseButtonActive]     = ImVec4(0.70f, 0.70f, 0.70f, 0.00f);

	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->AddFontFromFileTTF( "./half_payne/resource/DroidSans.ttf", 16 );
	ImFontConfig config;
	config.MergeMode = true;
	static const ImWchar icon_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
	io.Fonts->AddFontFromFileTTF( "./half_payne/resource/fontawesome-webfont.ttf", 13.0f, &config, icon_ranges );
	
	GameModeGUI_RefreshConfigFiles();
}

int GameModeGUI_ProcessEvent( void *data, SDL_Event* event ) {
	return ImGui_ImplSdl_ProcessEvent( event );
}

void GameModeGUI_RefreshConfigFiles() {
	GameModeGUI_RefreshConfigFileList( CONFIG_TYPE_CGM );
	GameModeGUI_RefreshConfigFileList( CONFIG_TYPE_BMM );
	GameModeGUI_RefreshConfigFileList( CONFIG_TYPE_SAGM );
}

void GameModeGUI_RefreshConfigFileList( CONFIG_TYPE configType ) {
	std::vector< CustomGameModeConfig > *configs = GameModeGUI_GameModeConfigVectorFromType( configType );
	configs->clear();

	CustomGameModeConfig dumbConfig = CustomGameModeConfig( configType );
	std::vector<std::string> files = dumbConfig.GetAllConfigFileNames();

	gEngfuncs.Con_Printf( "Command | Start map | Config name\n" );
	for ( auto file : files ) {
		CustomGameModeConfig config( configType );
		config.ReadFile( file.c_str() );

		configs->push_back( config );
	}
}

void GameModeGUI_RunCustomGameMode( CustomGameModeConfig &config ) {
	if ( config.error.length() > 0 ) {
		return;
	}

	// Prepare game mode and try to launch [start_map]
	// Launching the map then will execute CustomGameModeConfig constructor on server,
	// where the file will be parsed again.
	gEngfuncs.Cvar_Set( "gamemode_config", ( char * ) config.configName.c_str() );
	gEngfuncs.Cvar_Set( "gamemode", ( char * ) CustomGameModeConfig::ConfigTypeToGameModeCommand( config.configType ).c_str() );

	const std::string startMap = config.GetStartMap();
	char mapCmd[64];
	sprintf( mapCmd, "map %s", startMap.c_str() );
	gEngfuncs.pfnClientCmd( mapCmd );
}

void GameModeGUI_Draw() {

	if ( isPaused || inMainMenu ) {
		ImGui_ImplSdl_NewFrame( window );

		GameModeGUI_DrawMainWindow();

		glViewport( 0, 0, ( int ) ImGui::GetIO().DisplaySize.x, ( int ) ImGui::GetIO().DisplaySize.y );
		ImGui::Render();
	} else {
		SDL_ShowCursor( 0 );
	}

	SDL_GL_SwapWindow( window );
}

void GameModeGUI_SelectableButton( bool isSelected ) {
	ImGui::PushStyleColor( ImGuiCol_Button, isSelected ? ImVec4( 0.4f, 0.4f, 1.0f, 1.0f ) : ImVec4( 1.0f, 0.4f, 0.4f, 1.0f ) );
	ImGui::PushStyleColor( ImGuiCol_ButtonHovered, isSelected ? ImVec4( 0.6f, 0.6f, 1.0f, 1.0f ) : ImVec4( 1.0f, 0.6f, 0.6f, 1.0f ) );
	ImGui::PushStyleColor( ImGuiCol_ButtonActive, isSelected ? ImVec4( 0.3f, 0.3f, 1.0f, 1.0f ) : ImVec4( 1.0f, 0.3f, 0.3f, 1.0f ) );
}

std::vector<CustomGameModeConfig> *GameModeGUI_GameModeConfigVectorFromType( CONFIG_TYPE configType ) {
	switch ( configType ) {
		case CONFIG_TYPE_CGM:
			return &cgmConfigs;

		case CONFIG_TYPE_BMM:
			return &bmmConfigs;

		case CONFIG_TYPE_SAGM:
			return &sagmConfigs;
	}
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
	ImGui::Begin( "Custom game modes", &showGameModeWindow, ImGuiWindowFlags_NoTitleBar );

	// REFRESH BUTTON
	{
		ImGui::Columns( 1, "gamemode_refresh_button_column" );

		if ( ImGui::Button( "Refresh", ImVec2( -1, 0 ) ) ) {
			GameModeGUI_RefreshConfigFiles();
		}
	}

	// TABLES
	{
		ImGui::BeginChild( "gamemode_scrollable_data_child", ImVec2( -1, -1 ) );
		if ( ImGui::CollapsingHeader( "Custom Game Modes" ) ) {
			GameModeGUI_DrawGamemodeConfigTable( CONFIG_TYPE_CGM );
			ImGui::Columns( 1 );
		}

		if ( ImGui::CollapsingHeader( "Black Mesa Minute" ) ) {
			GameModeGUI_DrawGamemodeConfigTable( CONFIG_TYPE_BMM );
			ImGui::Columns( 1 );
		}

		if ( ImGui::CollapsingHeader( "Score Attack" ) ) {
			GameModeGUI_DrawGamemodeConfigTable( CONFIG_TYPE_SAGM );
			ImGui::Columns( 1 );
		}

		ImGui::EndChild();
	}

	ImGui::End();
}

void GameModeGUI_DrawGamemodeConfigTable( CONFIG_TYPE configType ) {
	
	const std::vector<CustomGameModeConfig> *configs = GameModeGUI_GameModeConfigVectorFromType( configType );

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

		for ( int i = 0; i < configs->size(); i++ ) {

			CustomGameModeConfig config = configs->at( i );

			const char *file = config.configName.c_str();
			const std::string name = config.GetName();
			const std::string description = config.GetDescription();
			const std::string startMap = config.GetStartMap();
			
			// COMPLETED?
			ImGui::TextColored( ImVec4( 1.0, 1.0, 1.0, config.gameFinishedOnce ? 1.0 : 0.0 ), "%s", ICON_FA_CHECK );
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
			ImGui::Text( name.c_str() ); ImGui::NextColumn();

			// CONFIG FILE INFO
			{
				ImGui::Text( "%s", ICON_FA_INFO_CIRCLE );
				if ( ImGui::IsItemHovered() ) {
					ImGui::BeginTooltip();

					if ( config.error.length() > 0 ) {
						ImGui::PushTextWrapPos(300.0f);
						ImGui::Text( config.error.c_str() );
						ImGui::PopTextWrapPos();
					} else {
						ImGui::Text( "Start map\n" );
						ImGui::Text( startMap.c_str() );

						if ( description.size() > 0 ) {
							ImGui::Text( "\nDescription\n" );
							ImGui::Text( description.c_str() );
						}

						if ( config.configType == CONFIG_TYPE_BMM ) {
							ImGui::TextColored( ImVec4( 1, 0.66, 0, 1 ), "\n\nBlack Mesa Minute\n" );
							ImGui::Text( "Time-based game mode - rush against a minute, kill enemies to get more time.\n" );
						} else if ( config.configType == CONFIG_TYPE_SAGM ) {
							ImGui::TextColored( ImVec4( 1, 0.66, 0, 1 ), "\n\nScore Attack\n" );
							ImGui::Text( "Kill enemies to get as much score as possible. Build combos to get even more score.\n" );
						}

						for ( const GameplayMod &mod : config.mods ) {
							ImGui::TextColored( ImVec4( 1, 0.66, 0, 1 ), ( "\n" + mod.name + "\n" ).c_str() );
							ImGui::Text( mod.description.c_str() );
							for ( auto argDescription : mod.argDescriptions ) {
								ImGui::TextColored( ImVec4( 1, 0.66, 0, 1 ), "   OPTION" ); ImGui::SameLine();
								ImGui::Text( argDescription.c_str() );
							}
						}

					}

					ImGui::EndTooltip();
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