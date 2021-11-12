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

extern cl_enginefunc_t gEngfuncs;
extern int isPaused;
extern bool inMainMenu;
SDL_Window *window = NULL;

// To draw imgui on top of Half-Life, we take a detour from certain engine's function into HL_ImGUI_Draw function
void HL_ImGUI_Init() {

	// One of the final steps before drawing a frame is calling SDL_GL_SwapWindow function
	// It must be prevented, so imgui could be drawn before calling SDL_GL_SwapWindow

	// This will hold the constant address of x86 CALL command, which looks like this
	// E8 FF FF FF FF
	// Last 4 bytes specify an offset from this address + 5 bytes of command itself
	unsigned int origin = NULL;

	// We're scanning 1 MB at the beginning of hw.dll for a certain sequence of bytes
	// Based on location of that sequnce, the location of CALL command is calculated
	MODULEINFO module_info;
	if ( GetModuleInformation( GetCurrentProcess(), GetModuleHandle( "hw.dll" ), &module_info, sizeof( module_info ) ) ) {
		origin = ( unsigned int ) module_info.lpBaseOfDll;
		
		const int MEGABYTE = 1024 * 1024;
		char *slice = new char[MEGABYTE];
		ReadProcessMemory( GetCurrentProcess(), ( const void * ) origin, slice, MEGABYTE, NULL );

		unsigned char magic[] = { 0x8B, 0x4D, 0x08, 0x83, 0xC4, 0x08, 0x89, 0x01, 0x5D, 0xC3, 0x90, 0x90, 0x90, 0x90, 0x90, 0xA1 };

		for ( unsigned int i = 0 ; i < MEGABYTE - 16; i++ ) {
			bool sequenceIsMatching = memcmp( slice + i, magic, 16 ) == 0;
			if ( sequenceIsMatching ) {
				origin += i + 27;
				break;
			}
		}

		delete[] slice;

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
	unsigned int detourFunctionAddress = ( unsigned int ) &HL_ImGUI_Draw;
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