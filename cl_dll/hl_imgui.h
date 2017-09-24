#ifndef HL_IMGUI_H
#define HL_IMGUI_H

#include "SDL2\SDL.h"
#include "SDL2\SDL_opengl.h"
#include "..\imgui\imgui.h"
#include "..\imgui\imgui_impl_sdl.h"

void HL_ImGUI_Init();
void HL_ImGUI_Deinit();
void HL_ImGUI_Draw();
int HL_ImGUI_ProcessEvent( void *data, SDL_Event* event );

#endif // HL_IMGUI_H