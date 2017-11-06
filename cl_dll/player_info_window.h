#ifndef PLAYER_INFO_WINDOW_H
#define PLAYER_INFO_WINDOW_H

#include "SDL2\SDL.h"
#include "SDL2\SDL_opengl.h"
#include "..\imgui\imgui.h"
#include "..\imgui\imgui_impl_sdl.h"

void PlayerInfoWindow_Init();
void PlayerInfoWindow_Draw();

int PlayerInfoWindow_OnPlyUpd( const char *pszName, int iSize, void *pbuf );

#endif