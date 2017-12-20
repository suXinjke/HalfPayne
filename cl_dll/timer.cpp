#include "hud.h"
#include "cl_util.h"
#include "../common/event_api.h"
#include "parsemsg.h"
#include "triangleapi.h"

#define TIMER_PAUSED_BLINK_TIME 0.7f

DECLARE_MESSAGE( m_Timer, TimerValue )
DECLARE_MESSAGE( m_Timer, TimerPause )
DECLARE_MESSAGE( m_Timer, TimerCheat )
DECLARE_MESSAGE( m_Timer, TimerDeact )

extern globalvars_t *gpGlobals;

int CHudTimer::Init( void )
{
	HOOK_MESSAGE( TimerValue );
	HOOK_MESSAGE( TimerPause );
	HOOK_MESSAGE( TimerCheat );
	HOOK_MESSAGE( TimerDeact );
	
	gHUD.AddHudElem( this );

	return 1;
}

void CHudTimer::Reset( void )
{
	m_iFlags = 0;

	paused = false;
	cheated = false;
	blinked = false;
	time = 0.0f;
	title = "";

	yOffset = 0;
}

int CHudTimer::Draw( float flTime )
{
	if ( ( gHUD.m_iHideHUDDisplay & HIDEHUD_ALL ) || gEngfuncs.IsSpectateOnly() ) {
	
		return 1;
	}

	int r = MESSAGE_BRIGHTENESS;
	int g = !cheated ? MESSAGE_BRIGHTENESS : 0;
	int b = !cheated ? MESSAGE_BRIGHTENESS : 0;

	int x = ScreenWidth - CORNER_OFFSET;
	int y = CORNER_OFFSET + yOffset;

	int formattedTimeSpriteWidth = gHUD.GetNumberSpriteWidth() * 8;
	int numberSpriteHeight = gHUD.GetNumberSpriteHeight();

	gHUD.DrawHudStringKeepRight( x, y, 200, title.c_str(), r, g, b );
	y += gHUD.m_scrinfo.iCharHeight + 2;

	if ( paused ) {
		if ( !blinked ) {
			ScaleColors( r, g, b, 50 );
		}

		if ( gEngfuncs.GetAbsoluteTime() > nextTimerBlinkTime ) {
			blinked = !blinked;
			nextTimerBlinkTime = gEngfuncs.GetAbsoluteTime() + TIMER_PAUSED_BLINK_TIME;
		}
	}
	gHUD.DrawFormattedTime( time, x - formattedTimeSpriteWidth, y, r, g, b );

	return 1;
}

int CHudTimer::MsgFunc_TimerValue( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ(pbuf, iSize);
	title = READ_STRING();
	time = READ_FLOAT();
	yOffset = READ_LONG();

	m_iFlags |= HUD_ACTIVE;

	return 1;
}

int CHudTimer::MsgFunc_TimerPause( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	
	paused = READ_BYTE() != 0;
	nextTimerBlinkTime = gEngfuncs.GetAbsoluteTime() + TIMER_PAUSED_BLINK_TIME;

	return 1;
}

int CHudTimer::MsgFunc_TimerCheat( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	cheated = true;

	return 1;
}

int CHudTimer::MsgFunc_TimerDeact( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	this->Reset();

	return 1;
}