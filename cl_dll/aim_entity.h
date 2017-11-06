#ifndef AIM_ENTITY_H
#define AIM_ENTITY_H

#include "SDL2\SDL.h"
#include "SDL2\SDL_opengl.h"
#include "..\imgui\imgui.h"
#include "..\imgui\imgui_impl_sdl.h"
#include <string>


void AimEntity_Init();
void AimEntity_Draw();

int AimEntity_OnAimNew( const char *pszName, int iSize, void *pbuf );
int AimEntity_OnAimUpd( const char *pszName, int iSize, void *pbuf );
int AimEntity_OnAimClear( const char *pszName, int iSize, void *pbuf );

#endif