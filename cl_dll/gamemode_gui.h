#ifndef GAMEMODE_GUI_H
#define GAMEMODE_GUI_H

#include "SDL2\SDL.h"
#include "SDL2\SDL_opengl.h"
#include "..\imgui\imgui.h"
#include "..\imgui\imgui_impl_sdl.h"
#include "custom_gamemode_config.h"

void GameModeGUI_Init();
void GameModeGUI_DrawMainWindow();
void GameModeGUI_RefreshConfigFileList( CONFIG_TYPE configType );
void GameModeGUI_DrawGamemodeConfigTable( CONFIG_TYPE configType );
std::vector<CustomGameModeConfig> *GameModeGUI_GameModeConfigVectorFromType( CONFIG_TYPE configType );
void GameModeGUI_RunCustomGameMode( const CustomGameModeConfig &config );
void GameModeGUI_SelectableButton( bool isSelected );
void GameModeGUI_RefreshConfigFiles();
const std::string GameModeGUI_GetFormattedTime( float time );

#endif // GAMEMODE_GUI_H