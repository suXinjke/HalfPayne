#include "hud.h"
#include "cl_util.h"
#include "../common/event_api.h"
#include "parsemsg.h"
#include "triangleapi.h"
#include "string_aux.h"

DECLARE_MESSAGE( m_GameLogWorld, GLogWDeact )
DECLARE_MESSAGE( m_GameLogWorld, GLogWMsg )

#define TIME_ADDED_REMOVAL_TIME 1.0f
#define TIME_ADDED_FLASHING_TIME 0.1f

extern globalvars_t *gpGlobals;

int CHudGameLogWorld::Init( void )
{
	HOOK_MESSAGE( GLogWDeact );
	HOOK_MESSAGE( GLogWMsg );

	gHUD.AddHudElem( this );

	return 1;
}

void CHudGameLogWorld::Reset( void )
{
	m_iFlags = 0;

	messages.clear();
}

int CHudGameLogWorld::Draw( float flTime )
{
	if ( ( gHUD.m_iHideHUDDisplay & HIDEHUD_ALL ) || gEngfuncs.IsSpectateOnly() ) {
		return 1;
	}

	int r = MESSAGE_BRIGHTENESS;
	int g = MESSAGE_BRIGHTENESS;
	int b = MESSAGE_BRIGHTENESS;

	int numberSpriteHeight = gHUD.GetNumberSpriteHeight();

	int x = ScreenWidth - CORNER_OFFSET;
	int y = CORNER_OFFSET + numberSpriteHeight;

	for ( int i = messages.size() - 1; i >= 0; i-- ) {
		GameLogWorldMessage timerMessage = messages.at( i );

		if ( fabs( timerMessage.timeAddedRemovalTime - gEngfuncs.GetAbsoluteTime() ) >= TIME_ADDED_REMOVAL_TIME ) {
			messages.erase( messages.begin() + i );
		}
	}

	for ( int i = messages.size( ) - 1; i >= 0; i-- ) {
		GameLogWorldMessage timerMessage = messages.at( i );

		float timePassed = TIME_ADDED_REMOVAL_TIME - max( 0.0f, timerMessage.timeAddedRemovalTime - gEngfuncs.GetAbsoluteTime() );

		if ( ( timePassed >= timerMessage.timeAddedFlash1Time && timePassed <= timerMessage.timeAddedFlash2Time ) 
			|| ( timePassed >= TIME_ADDED_REMOVAL_TIME ) ) {
			continue;
		}

		Vector screen;
		if ( gEngfuncs.pTriAPI->WorldToScreen( timerMessage.coords, screen ) ) {
			// Don't draw if offscreen
			continue;
		}

		int alpha = MESSAGE_BRIGHTENESS - ( ( timePassed - TIME_ADDED_FLASHING_TIME * 2 ) / ( TIME_ADDED_REMOVAL_TIME - TIME_ADDED_FLASHING_TIME * 2 ) ) * MESSAGE_BRIGHTENESS;
		int r2 = r;
		int g2 = g;
		int b2 = b;

		// fade out
		if ( timePassed >= timerMessage.timeAddedFlash2Time ) {
			ScaleColors( r2, g2, b2, alpha );
		}
		int timeAddedStringWidth = max( gHUD.GetStringWidth( timerMessage.message.c_str() ), gHUD.GetStringWidth( timerMessage.message2.c_str() ) );

		if ( timerMessage.message2.size() > 0 ) {
			gEngfuncs.pfnFillRGBABlend( XPROJECT( screen.x ) - timeAddedStringWidth / 2 - 5, YPROJECT( screen.y ) - 1 - gHUD.m_scrinfo.iCharHeight - 4 , timeAddedStringWidth + 8, gHUD.m_scrinfo.iCharHeight + 4, 0, 0, 0, alpha );
			gHUD.DrawHudStringKeepCenter( XPROJECT( screen.x ), YPROJECT( screen.y ) - gHUD.m_scrinfo.iCharHeight - 4, 200, timerMessage.message2.c_str(), r2, g2, b2 );
		}
		
		if ( timerMessage.message.size() > 0 ) {
			gEngfuncs.pfnFillRGBABlend( XPROJECT( screen.x ) - timeAddedStringWidth / 2 - 5, YPROJECT( screen.y ) - 1, timeAddedStringWidth + 8, gHUD.m_scrinfo.iCharHeight + 4, 110, 110, 110, alpha );
			gHUD.DrawHudStringKeepCenter( XPROJECT( screen.x ), YPROJECT( screen.y ), 200, timerMessage.message.c_str(), r2, g2, b2 );
		}
	}

	return 1;
}

int CHudGameLogWorld::MsgFunc_GLogWDeact( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	this->Reset();

	return 1;
}

int CHudGameLogWorld::MsgFunc_GLogWMsg( const char *pszName, int iSize, void *pbuf )
{
	m_iFlags |= HUD_ACTIVE;

	BEGIN_READ( pbuf, iSize );
	float x = READ_COORD();
	float y = READ_COORD();
	float z = READ_COORD();
	auto messageParts = Split( READ_STRING(), '|', false );
	
	messages.push_back( {
		{ x, y, z },
		messageParts.at( 0 ),
		messageParts.size() > 1 ? messageParts.at( 1 ) : "",
		( float ) gEngfuncs.GetAbsoluteTime() + TIME_ADDED_REMOVAL_TIME,
		TIME_ADDED_FLASHING_TIME,
		TIME_ADDED_FLASHING_TIME * 2
	} );

	return 1;
}