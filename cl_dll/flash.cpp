#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "flash.h"

float flash = 0.0f;
int flashColorR = 0;
int flashColorG = 0;
int flashColorB = 0;
int flashColorAlpha = 0;
float flashEnd = 0.0f;
float fadeOut = 0.0f;

void Flash_Init() {
	gEngfuncs.pfnHookUserMsg( "Flash", MsgFunc_Flash );
	gEngfuncs.pfnHookUserMsg( "FadeOut", MsgFunc_FadeOut );
}

void Flash_Draw() {
	gEngfuncs.pfnFillRGBABlend( 0, 0, ScreenWidth, ScreenHeight, 0, 0, 0, fadeOut );

	if ( flash ) {
		float flashPosition = gEngfuncs.GetAbsoluteTime() - flash;

		int alpha = flashColorAlpha * sin( ( flashPosition / flashEnd ) * 3.1415926535897932384626433832795f );
		if ( alpha > flashColorAlpha ) {
			alpha = flashColorAlpha;
		}
		if ( alpha < 0 ) {
			alpha = 0;
		}

		gEngfuncs.pfnFillRGBABlend( 0, 0, ScreenWidth, ScreenHeight, flashColorR, flashColorG, flashColorB, alpha );

		if ( flashPosition >= flashEnd ) {
			flash = 0.0f;
		}
	}
}

int MsgFunc_Flash( const char *pszName,  int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	flash = gEngfuncs.GetAbsoluteTime();
	flashColorR = READ_BYTE();
	flashColorG = READ_BYTE();
	flashColorB = READ_BYTE();
	flashColorAlpha = READ_BYTE();
	flashEnd = READ_FLOAT();

	return 1;
}

int MsgFunc_FadeOut( const char *pszName,  int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	int receivedFade = READ_BYTE();

	fadeOut = 255 - receivedFade;

	return 1;
}