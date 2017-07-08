#include "SDL2\SDL.h"
#include "SDL2\SDL_opengl.h"
#include "..\imgui\imgui.h"
#include "..\imgui\imgui_impl_sdl.h"
#include "custom_gamemode_config.h"

void GameModeGUI_Init();
void GameModeGUI_Draw();
void GameModeGUI_DrawMainWindow();
void GameModeGUI_RefreshConfigFileList( CustomGameModeConfig::GAME_MODE_CONFIG_TYPE configType );
void GameModeGUI_DrawGamemodeConfigTable( CustomGameModeConfig::GAME_MODE_CONFIG_TYPE configType );
std::vector<CustomGameModeConfig> *GameModeGUI_GameModeConfigVectorFromType( CustomGameModeConfig::GAME_MODE_CONFIG_TYPE configType );
void GameModeGUI_RunCustomGameMode( const CustomGameModeConfig &config );
void GameModeGUI_SelectableButton( bool isSelected );
int GameModeGUI_ProcessEvent( void *data, SDL_Event* event );
void GameModeGUI_RefreshConfigFiles();