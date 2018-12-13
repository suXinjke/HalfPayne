#ifndef GAMEMODE_GUI_H
#define GAMEMODE_GUI_H

#include "SDL2\SDL.h"
#include "SDL2\SDL_opengl.h"
#include "..\imgui\imgui.h"
#include "..\imgui\imgui_impl_sdl.h"
#include "custom_gamemode_config.h"

void GameModeGUI_Init();
void GameModeGUI_DrawMainWindow();
void GameModeGUI_DrawGamemodeConfigTable( std::vector<CustomGameModeConfig> &configs );
void GameModeGUI_DrawTwitchConfig();
void GameModeGUI_DrawConfigFileInfo( CustomGameModeConfig &config );
void GameModeGUI_RunCustomGameMode( const CustomGameModeConfig &config, const std::string &mapOverride = "" );
void GameModeGUI_RefreshConfigFiles();
const std::string GameModeGUI_GetGameModeConfigName( const CustomGameModeConfig &config );
const std::string GameModeGUI_GetFormattedTime( float time );

#endif // GAMEMODE_GUI_H