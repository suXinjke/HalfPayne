#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

DECLARE_MESSAGE( m_endCredits, EndCredits )

extern globalvars_t *gpGlobals;
extern bool creditsException;

int CHudEndCredits::Init(void)
{
	HOOK_MESSAGE( EndCredits );

	gHUD.AddHudElem(this);

	return 1;
}

int CHudEndCredits::VidInit(void)
{
	creditSprites[0] = gHUD.GetSpriteIndex( "credits1" );
	creditSprites[1] = gHUD.GetSpriteIndex( "credits2" );
	creditSprites[2] = gHUD.GetSpriteIndex( "credits3" );
	creditSprites[3] = gHUD.GetSpriteIndex( "credits4" );
	creditSprites[4] = gHUD.GetSpriteIndex( "credits5" );
	creditSprites[5] = gHUD.GetSpriteIndex( "credits5a" );
	creditSprites[6] = gHUD.GetSpriteIndex( "credits6" );
	creditSprites[7] = gHUD.GetSpriteIndex( "credits10" );
	creditSprites[8] = gHUD.GetSpriteIndex( "credits11" );
	creditSprites[9] = gHUD.GetSpriteIndex( "credits12" );

	return 1;
};

void CHudEndCredits::Reset( void )
{
	m_iFlags = 0;

	ended = false;
}

int	CHudEndCredits::XPosition( float x, int width, int totalWidth )
{
	int xPos;

	if ( x == -1 )
	{
		xPos = (ScreenWidth - width) / 2;
	}
	else
	{
		if ( x < 0 )
			xPos = (1.0 + x) * ScreenWidth - totalWidth;	// Alight right
		else
			xPos = x * ScreenWidth;
	}

	if ( xPos + width > ScreenWidth )
		xPos = ScreenWidth - width;
	else if ( xPos < 0 )
		xPos = 0;

	return xPos;
}


int CHudEndCredits::YPosition( float y, int height )
{
	int yPos;

	if ( y == -1 )	// Centered?
		yPos = (ScreenHeight - height) * 0.5;
	else
	{
		// Alight bottom?
		if ( y < 0 )
			yPos = (1.0 + y) * ScreenHeight - height;	// Alight bottom
		else // align top
			yPos = y * ScreenHeight;
	}

	if ( yPos + height > ScreenHeight )
		yPos = ScreenHeight - height;
	else if ( yPos < 0 )
		yPos = 0;

	return yPos;
}


int CHudEndCredits::Draw( float flTime )
{
	float absTime = gEngfuncs.GetAbsoluteTime();
	float time = absTime - timeStart;
	//gEngfuncs.Con_Printf( "fltime: %f abs: %f time: %f\n", flTime, absTime, time );

	int spriteIndex = -1;
	int alpha = 255;

	// Black overlay
	gEngfuncs.pfnFillRGBABlend( 0, 0, ScreenWidth, ScreenHeight, 0, 0, 0, 255 );

	if ( time >= 0 && time <= 5.8 ) {
		spriteIndex = 0; // Half Payne logo
	} else if ( time >= 6.0 && time <= 11.8 ) {
		spriteIndex = 1; // Directed by suXin
	} else if ( time >= 12.0 && time <= 17.8 ) {
		spriteIndex = 2; // Max Payne hands and assistance by Rara
	} else if ( time >= 18.0 && time <= 23.8 ) {
		spriteIndex = 3; // Med cabinet rip by Saitek
	} else if ( time >= 24.0 && time <= 29.8 ) {
		spriteIndex = 4; // Menu background by NinjaNub
	} else if ( time >= 30.0 && time <= 35.8 ) {
		spriteIndex = 5; // Nightmare map
	} else if ( time >= 36.0 && time <= 41.8 ) {
		spriteIndex = 6; // Max Payne model and voice
	} else if ( time >= 42.0 && time <= 47.5 ) {
		spriteIndex = 7; // Valve 1
	} else if ( time >= 47.7 && time <= 52.2 ) {
		spriteIndex = 8; // Valve 2
	} else if ( time >= 52.4 && time <= 56.4 ) {
		spriteIndex = 9; // Valve 3
	} else if ( time >= 56.681 && time < 96.4f ) {

		spriteIndex = 0; // Half Payne logo

		float difference = 64.0f - min( time, 64.0f );
		float max = 7.3f;

		alpha = ( ( max - difference ) / max ) * 255;
	} else if ( time >= 96.4f ) {
		gEngfuncs.pfnClientCmd( "disconnect" );
	}


	if ( spriteIndex >= 0 ) {
		int width = gHUD.GetSpriteRect( creditSprites[spriteIndex] ).right - gHUD.GetSpriteRect( creditSprites[spriteIndex] ).left;
		int height = gHUD.GetSpriteRect( creditSprites[spriteIndex] ).bottom - gHUD.GetSpriteRect( creditSprites[spriteIndex] ).top;

		int x = XPosition( -1, width, width );
		int y = YPosition( -1, height );

		SPR_Set( gHUD.GetSprite( creditSprites[spriteIndex] ), alpha, alpha, alpha );
		SPR_DrawAdditive( 0, x, y, &gHUD.GetSpriteRect( creditSprites[spriteIndex] ) );
	}

	return 1;
}

int CHudEndCredits::MsgFunc_EndCredits( const char *pszName, int iSize, void *pbuf )
{
	if ( ended ) {
		return 1;
	}

	BEGIN_READ( pbuf, iSize );

	ended = true;
	timeStart = gEngfuncs.GetAbsoluteTime();
	creditsException = true;
	
	m_iFlags |= HUD_ACTIVE;

	return 1;
}