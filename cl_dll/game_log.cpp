#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

DECLARE_MESSAGE( m_GameLog, GLogDeact )
DECLARE_MESSAGE( m_GameLog, GLogMsg )

#define TIMER_MESSAGE_REMOVAL_TIME 3.0f

extern globalvars_t *gpGlobals;

int CHudGameLog::Init( void )
{
	HOOK_MESSAGE( GLogDeact );
	HOOK_MESSAGE( GLogMsg );

	gHUD.AddHudElem( this );

	return 1;
}

void CHudGameLog::Reset( void )
{
	m_iFlags = 0;

	yOffset = 0;

	messages.clear();
}

int CHudGameLog::Draw( float flTime )
{
	if ( ( gHUD.m_iHideHUDDisplay & HIDEHUD_ALL ) || gEngfuncs.IsSpectateOnly() ) {
		return 1;
	}

	int r = MESSAGE_BRIGHTENESS;
	int g = MESSAGE_BRIGHTENESS;
	int b = MESSAGE_BRIGHTENESS;

	int numberSpriteHeight = gHUD.GetNumberSpriteHeight();

	int x = ScreenWidth - CORNER_OFFSET;
	int y = CORNER_OFFSET + yOffset;

	for ( int i = messages.size() - 1; i >= 0; i-- ) {
		GameLogMessage timerMessage = messages.at( i );

		if ( gEngfuncs.GetAbsoluteTime() >= timerMessage.second ) {
			messages.erase( messages.begin() + i );
		}
	}

	for ( int i = messages.size() - 1; i >= 0; i-- ) {
		std::string message = messages.at( i ).first;
		gHUD.DrawHudStringKeepRight( x, y, 200, message.c_str(), r, g, b );
		y += gHUD.m_scrinfo.iCharHeight - 2;
	}

	return 1;
}

int CHudGameLog::MsgFunc_GLogDeact( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	this->Reset();

	return 1;
}

int CHudGameLog::MsgFunc_GLogMsg( const char *pszName, int iSize, void *pbuf )
{
	m_iFlags |= HUD_ACTIVE;

	BEGIN_READ( pbuf, iSize );
	const char *message = READ_STRING();
	yOffset = READ_LONG();
	
	messages.push_back( GameLogMessage( message, ( float ) gEngfuncs.GetAbsoluteTime() + TIMER_MESSAGE_REMOVAL_TIME ) );

	return 1;
}