#include "wrect.h"
#include "cl_dll.h"
#include <Windows.h>
#include <Psapi.h>

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
	const int GAME_MODE_WINDOW_WIDTH  = 480;
	const int GAME_MODE_WINDOW_HEIGHT = 330;
	int RENDERED_WIDTH;
	SDL_GetWindowSize( window, &RENDERED_WIDTH, NULL );
	ImGui::SetNextWindowPos( ImVec2( RENDERED_WIDTH - GAME_MODE_WINDOW_WIDTH, 0 ), ImGuiSetCond_Once );
	ImGui::SetNextWindowSize( ImVec2( GAME_MODE_WINDOW_WIDTH, GAME_MODE_WINDOW_HEIGHT ), ImGuiSetCond_Once );
	ImGui::Begin( "Custom game modes", &showGameModeWindow );

	ImGui::Columns( 3, "gamemode_button_columns" );

	// HEADER BUTTONS
	{
		GameModeGUI_SelectableButton( selectedGamemodeTab == CONFIG_TYPE_CGM );
		if ( ImGui::Button( "Custom Game Mode", ImVec2( -1, 0 ) ) ) {
			selectedGamemodeTab = CONFIG_TYPE_CGM;
		}
		ImGui::PopStyleColor( 3 );
		ImGui::NextColumn();

		GameModeGUI_SelectableButton( selectedGamemodeTab == CONFIG_TYPE_BMM );
		if ( ImGui::Button( "Black Mesa Minute", ImVec2( -1, 0 ) ) ) {
			selectedGamemodeTab = CONFIG_TYPE_BMM;
		}
		ImGui::PopStyleColor( 3 );
		ImGui::NextColumn();

		GameModeGUI_SelectableButton( selectedGamemodeTab == CONFIG_TYPE_SAGM );
		if ( ImGui::Button( "Score Attack", ImVec2( -1, 0 ) ) ) {
			selectedGamemodeTab = CONFIG_TYPE_SAGM;
		}
		ImGui::PopStyleColor( 3 );
		ImGui::NextColumn();
	}

	{
		ImGui::Columns( 1, "gamemode_hint_column" );

		const char *launchHint = "Double click on the row to launch game mode";
		ImGui::SetCursorPosX( ImGui::GetWindowWidth() / 2.0f - ImGui::CalcTextSize( launchHint ).x / 2.0f );
		ImGui::Text( launchHint );
	}

	// FIXED TABLE HEADERS
	{
		ImGui::Columns( 2, "gamemode_header_columns" );
		ImGui::Text( "File" ); ImGui::NextColumn();
		ImGui::Text( "Name" ); ImGui::NextColumn();
	}

	// SCROLLABLE TABLE
	{
		ImGui::Columns( 1, "gamemode_scrollable_data_column" );
		ImGui::BeginChild( "gamemode_scrollable_data_child", ImVec2( -1, ImGui::GetWindowHeight() - 110 ), true ); // TODO: hardcoded number of 110 - how do I calculate this dynamic?

		ImGui::Columns( 2, "gamemode_data_columns" );
		GameModeGUI_DrawGamemodeConfigTable( ( CONFIG_TYPE ) selectedGamemodeTab );

		ImGui::EndChild();
	}

	// REFRESH BUTTON FOOTER
	{
		ImGui::Columns( 1, "gamemode_refresh_button_column" );

		if ( ImGui::Button( "Refresh", ImVec2( -1, 0 ) ) ) {
			GameModeGUI_RefreshConfigFiles();
		}
	}

	ImGui::End();
}

void GameModeGUI_DrawGamemodeConfigTable( CONFIG_TYPE configType ) {
	
	const std::vector<CustomGameModeConfig> *configs = GameModeGUI_GameModeConfigVectorFromType( configType );

	static int selected = -1;
	for ( int i = 0; i < configs->size(); i++ )
	{
		CustomGameModeConfig config = configs->at( i );

		const char *file = config.configName.c_str();
		const std::string name = config.GetName();
		const std::string startMap = config.GetStartMap();

		if ( ImGui::Selectable( file, selected == i, ImGuiSelectableFlags_SpanAllColumns ) ) {
			selected = i;
		}
		if ( config.error.length() > 0 ) {
			if ( ImGui::IsItemHovered() ) {
				ImGui::BeginTooltip();
				ImGui::PushTextWrapPos(300.0f);
				ImGui::Text( config.error.c_str() );
				ImGui::PopTextWrapPos();
				ImGui::EndTooltip();
			}

			ImGui::SameLine();
			ImGui::TextColored( ImVec4( 1, 0, 0, 1 ), "error" );
		} else {
			if ( ImGui::IsItemHovered() ) {
				ImGui::BeginTooltip();

				ImGui::Text( "Start map\n" );
				ImGui::Text( startMap.c_str() );

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
				}

				ImGui::EndTooltip();
			}
		}
		ImGui::NextColumn();

		if ( ImGui::IsItemHovered()  ) {
			if ( ImGui::IsMouseDoubleClicked( 0 ) ) {
				GameModeGUI_RunCustomGameMode( config );
			}
		}
		
		ImGui::Text( name.c_str() ); ImGui::NextColumn();
	}
}