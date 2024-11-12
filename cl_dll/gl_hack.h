#include <Windows.h>
#include <gl/GL.h>

void glHackThink();
void __stdcall flustrumHook( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble nearVal, GLdouble farVal );