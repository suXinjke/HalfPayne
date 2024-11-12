#include "wrect.h"
#include "cl_dll.h"
#include <Windows.h>
#include <Psapi.h>
#include "FontAwesome.h"
#include "gamemode_gui.h"
#include "subtitles.h"
#include "model_indexes.h"
#include "aim_entity.h"
#include "player_info_window.h"
#include "fs_aux.h"

#include "hl_imgui.h"
#include "gl_hack.h"
#include <subhook.h>

extern cl_enginefunc_t gEngfuncs;
extern int isPaused;
extern bool inMainMenu;
SDL_Window *window = NULL;

subhook::Hook swapWindowHookController;

// To draw imgui on top of Half-Life, we take a detour from certain engine's function into HL_ImGUI_Draw function
void HL_ImGUI_Init() {
	auto moduleHandle = GetModuleHandle( "SDL2.dll" );
	if ( moduleHandle == NULL ) {
		gEngfuncs.Con_DPrintf( "Failed to get a handle on SDL module: %d\n", GetLastError() );
		return;
	}
	auto OriginalSwapWindow = GetProcAddress( moduleHandle, "SDL_GL_SwapWindow" );
	swapWindowHookController.Install( ( void * ) OriginalSwapWindow, ( void * ) HL_ImGUI_Draw );

	window = SDL_GetWindowFromID( 1 );
	ImGui_ImplSdl_Init( window );

	SDL_AddEventWatch( HL_ImGUI_ProcessEvent, NULL );

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
	style->Colors[ImGuiCol_PlotHistogram]         = ImVec4(1.00f, 1.00f, 1.00f, 0.35f);

	style->WindowPadding = ImVec2( 8.0f, 4.0f );

	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->AddFontFromFileTTF( FS_ResolveModPath( "resource\\DroidSans.ttf" ).c_str(), 16 );
	ImFontConfig config;
	config.MergeMode = true;
	static const ImWchar icon_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
	io.Fonts->AddFontFromFileTTF( FS_ResolveModPath( "resource\\fontawesome-webfont.ttf" ).c_str(), 14.0f, &config, icon_ranges );
	static const ImWchar icon_ranges_cyrillic[] = { 0x0410, 0x044F, 0 };
	io.Fonts->AddFontFromFileTTF( FS_ResolveModPath( "resource\\DroidSans.ttf" ).c_str(), 16, &config, icon_ranges_cyrillic );

	GameModeGUI_Init();
	Subtitles_Init();
	ModelIndexes_Init();
	AimEntity_Init();
	PlayerInfoWindow_Init();
}

void HL_ImGUI_Deinit() {
	if ( !window ) {
		return;
	}

	SDL_DelEventWatch( HL_ImGUI_ProcessEvent, NULL );
}

extern cvar_t  *printmodelindexes;
extern cvar_t  *print_aim_entity;
extern cvar_t  *print_player_info;
void HL_ImGUI_Draw() {

	subhook::ScopedHookRemove remove( &swapWindowHookController );

	// HACK: shouldn't be here because it got nothing
	// to do with ImGUI, but placement is convenient
	glHackThink();

	ImGui_ImplSdl_NewFrame( window );

	if ( isPaused || inMainMenu ) {
		SDL_ShowCursor( 1 );
		GameModeGUI_DrawMainWindow();
	} else {
		SDL_ShowCursor( 0 );
		Subtitles_Draw();
	}
	
	if ( printmodelindexes && printmodelindexes->value >= 2.0f ) {
		ModelIndexes_Draw();
	}

	if ( print_aim_entity && print_aim_entity->value >= 1.0f ) {
		AimEntity_Draw();
	}

	if ( print_player_info && print_player_info->value >= 1.0f ) {
		PlayerInfoWindow_Draw();
	}

	glViewport( 0, 0, ( int ) ImGui::GetIO().DisplaySize.x, ( int ) ImGui::GetIO().DisplaySize.y );
	ImGui::Render();

	SDL_GL_SwapWindow( window );
}

int HL_ImGUI_ProcessEvent( void *data, SDL_Event* event ) {
	return ImGui_ImplSdl_ProcessEvent( event );
}