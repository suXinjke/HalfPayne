#include "gl_hack.h"
#include <subhook.h>

subhook::Hook glHook;

extern int g_mirror;
extern bool shouldRenderMirrored;

void glHackThink() {
	if ( glHook.IsInstalled() == false && g_mirror ) {
		glHook.Install( ( void * ) glFrustum, ( void * ) flustrumHook );
	}
}

void __stdcall flustrumHook(
	GLdouble left,
	GLdouble right,
	GLdouble bottom,
	GLdouble top,
	GLdouble nearVal,
	GLdouble farVal
) {
	subhook::ScopedHookRemove remove( &glHook );

	if ( shouldRenderMirrored ) {
		glFrustum( right, left, bottom, top, nearVal, farVal );
		glFrontFace( GL_CW );
	} else {
		glFrustum( left, right, bottom, top, nearVal, farVal );
		glFrontFace( GL_CCW );
	}
}