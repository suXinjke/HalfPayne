#ifndef MODEL_INDEXES_H
#define MODEL_INDEXES_H

#include "SDL2\SDL.h"
#include "SDL2\SDL_opengl.h"
#include "..\imgui\imgui.h"
#include "..\imgui\imgui_impl_sdl.h"
#include <string>

struct ModelIndexMessage {
	std::string map;
	int index;
	std::string target_name;
	std::string class_name;
};

void ModelIndexes_Init();
void ModelIndexes_Draw();
void ModelIndexes_AddModelIndex( const ModelIndexMessage &modelIndex );

int ModelIndexes_OnModelIdx( const char *pszName, int iSize, void *pbuf );

#endif