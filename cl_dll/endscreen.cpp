#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

DECLARE_MESSAGE( m_endScreen, CustomEnd )
DECLARE_MESSAGE( m_endScreen, CustomChea )

#define MESSAGE_BRIGHTENESS 200

extern globalvars_t *gpGlobals;

int CHudEndScreen::Init(void)
{
	HOOK_MESSAGE( CustomEnd );
	HOOK_MESSAGE( CustomChea );

	gHUD.AddHudElem(this);

	return 1;
}

void CHudEndScreen::Reset( void )
{
	m_iFlags = 0;

	ended = false;
	cheated = false;

	messages.clear();

	kills = 0;
	headshotKills = 0;
	explosiveKills = 0;
	crowbarKills = 0;
	projectileKills = 0;

	secondsInSlowmotion = 0.0f;
}

int CHudEndScreen::Draw( float flTime )
{
	if ( ( gHUD.m_iHideHUDDisplay & HIDEHUD_ALL )
		|| gEngfuncs.IsSpectateOnly() ) {
	
		return 1;
	}

	int r = MESSAGE_BRIGHTENESS;
	int g = MESSAGE_BRIGHTENESS;
	int b = MESSAGE_BRIGHTENESS;

	int x = ScreenWidth - CORNER_OFFSET;
	int y = CORNER_OFFSET;

	int formattedTimeSpriteWidth = gHUD.GetNumberSpriteWidth() * 8;
	int numberSpriteHeight = gHUD.GetNumberSpriteHeight();

	if ( ended ) {	
		DrawEndScreen();
	}

	return 1;
}

void CHudEndScreen::DrawEndScreen()
{
	int r = MESSAGE_BRIGHTENESS;
	int g = MESSAGE_BRIGHTENESS;
	int b = MESSAGE_BRIGHTENESS;

	int actualScreenWidth = 640;
	int actualScreenHeight = 480;

	int xOffset = ( ScreenWidth / 2 ) - ( actualScreenWidth / 2 );
	int yOffset = ( ScreenHeight / 2 ) - ( actualScreenHeight / 2 );

	int x = ScreenWidth / 2;
	int y = CORNER_OFFSET + yOffset;

	int formattedTimeSpriteWidth = gHUD.GetNumberSpriteWidth() * 8;
	int numberSpriteHeight = gHUD.GetNumberSpriteHeight();

	// Black overlay
	gEngfuncs.pfnFillRGBABlend( 0, 0, ScreenWidth, ScreenHeight, 0, 0, 0, 255 );

	// Title Messages
	gHUD.DrawHudStringKeepCenter( x, y, 200, "CUSTOM GAME MODE", r, g, b );
	y += gHUD.m_scrinfo.iCharHeight - 2;
	gHUD.DrawHudStringKeepCenter( x, y, 200, endScreenLevelCompletedMessage.c_str(), r, g, b );
	y += gHUD.m_scrinfo.iCharHeight * 2;

	// Bottom statistics
	x = ScreenWidth / 2;
	y = ScreenHeight - yOffset - CORNER_OFFSET - gHUD.m_scrinfo.iCharHeight * 8;
	DrawEndScreenStatistics( x, y );

	// Bottom messages
	y = ScreenHeight - yOffset - CORNER_OFFSET - gHUD.m_scrinfo.iCharHeight;
	gHUD.DrawHudStringKeepCenter( x, y, 300, "PRESS ESC TO QUIT OR START THE NEW GAME", r, g, b );

	if ( cheated ) {
		y -= gHUD.m_scrinfo.iCharHeight;
		gHUD.DrawHudStringKeepCenter( x, y, 300, "YOU'VE BEEN CHEATING - PB WILL NOT BE SAVED", 200, 0, 0 );
	}
}

void CHudEndScreen::DrawEndScreenStatistics( int x, int y )
{
	int r = MESSAGE_BRIGHTENESS;
	int g = MESSAGE_BRIGHTENESS;
	int b = MESSAGE_BRIGHTENESS;

	char messageBuffer[16];
	
	int roundedSecondsInSlowmotion = ( int ) roundf( secondsInSlowmotion );
	if ( roundedSecondsInSlowmotion > 0 ) {
		gHUD.DrawHudString( x - 140, y, 160, "SECONDS IN SLOWMOTION", r, g, b );
		sprintf( messageBuffer, "%d", roundedSecondsInSlowmotion );
		gHUD.DrawHudStringKeepRight( x + 140, y, 160, messageBuffer, r, g, b );
		y += gHUD.m_scrinfo.iCharHeight - 2;
	}

	if ( kills > 0 ) {
		gHUD.DrawHudString( x - 140, y, 160, "TOTAL KILLS", r, g, b );
		sprintf( messageBuffer, "%d", kills );
		gHUD.DrawHudStringKeepRight( x + 140, y, 160, messageBuffer, r, g, b );
		y += gHUD.m_scrinfo.iCharHeight - 2;
	}

	if ( headshotKills > 0 ) {
		gHUD.DrawHudString( x - 140, y, 160, "HEADSHOT KILLS", r, g, b );
		sprintf( messageBuffer, "%d", headshotKills );
		gHUD.DrawHudStringKeepRight( x + 140, y, 160, messageBuffer, r, g, b );
		y += gHUD.m_scrinfo.iCharHeight - 2;
	}

	if ( explosiveKills > 0 ) {
		gHUD.DrawHudString( x - 140, y, 160, "EXPLOSION KILLS", r, g, b );
		sprintf( messageBuffer, "%d", explosiveKills );
		gHUD.DrawHudStringKeepRight( x + 140, y, 160, messageBuffer, r, g, b );
		y += gHUD.m_scrinfo.iCharHeight - 2;
	}

	if ( crowbarKills > 0 ) {
		gHUD.DrawHudString( x - 140, y, 160, "MELEE KILLS", r, g, b );
		sprintf( messageBuffer, "%d", crowbarKills );
		gHUD.DrawHudStringKeepRight( x + 140, y, 160, messageBuffer, r, g, b );
		y += gHUD.m_scrinfo.iCharHeight - 2;
	}

	if ( projectileKills > 0 ) {
		gHUD.DrawHudString( x - 140, y, 160, "PROJECTILES DESTROYED", r, g, b );
		sprintf( messageBuffer, "%d", projectileKills );
		gHUD.DrawHudStringKeepRight( x + 140, y, 160, messageBuffer, r, g, b );
		y += gHUD.m_scrinfo.iCharHeight - 2;
	}

}

int CHudEndScreen::MsgFunc_CustomEnd( const char *pszName, int iSize, void *pbuf )
{
	if ( ended ) {
		return 1;
	}

	BEGIN_READ( pbuf, iSize );

	endScreenLevelCompletedMessage = READ_STRING();
	
	secondsInSlowmotion = READ_FLOAT();
	kills = READ_SHORT();
	headshotKills = READ_SHORT();
	explosiveKills = READ_SHORT();
	crowbarKills = READ_SHORT();
	projectileKills = READ_SHORT();

	if ( endScreenLevelCompletedMessage.size() > 0 ) {
		endScreenLevelCompletedMessage = endScreenLevelCompletedMessage.append( " - COMPLETE" );
	} else {
		endScreenLevelCompletedMessage = endScreenLevelCompletedMessage.append( "LEVEL COMPLETE" );
	}
		
	ended = true;
	
	m_iFlags |= HUD_ACTIVE;

	return 1;
}

int CHudEndScreen::MsgFunc_CustomChea( const char *pszName, int iSize, void *pbuf )
{
	cheated = true;

	return 1;
}
