#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "triangleapi.h"
#include "cpp_aux.h"
#include "gamemode_gui.h"

DECLARE_MESSAGE( m_EndScreen, EndActiv )
DECLARE_MESSAGE( m_EndScreen, EndTitle )
DECLARE_MESSAGE( m_EndScreen, EndTime )
DECLARE_MESSAGE( m_EndScreen, EndScore )
DECLARE_MESSAGE( m_EndScreen, EndStat )

#define TIMER_MESSAGE_REMOVAL_TIME 3.0f

extern globalvars_t *gpGlobals;

int CHudEndScreen::Init( void )
{
	HOOK_MESSAGE( EndActiv );
	HOOK_MESSAGE( EndTitle );
	HOOK_MESSAGE( EndTime );
	HOOK_MESSAGE( EndScore );
	HOOK_MESSAGE( EndStat );

	gHUD.AddHudElem( this );

	return 1;
}

void CHudEndScreen::Reset( void )
{
	m_iFlags = 0;

	animationLines.clear();
	statLines.clear();
	titleLines.clear();
	statLines.clear();
}

int CHudEndScreen::Draw( float flTime )
{
	if ( ( gHUD.m_iHideHUDDisplay & HIDEHUD_ALL ) || gEngfuncs.IsSpectateOnly() ) {
		return 1;
	}

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
	for ( const auto &titleLine : titleLines ) {
		gHUD.DrawHudStringKeepCenter( x, y, 200, titleLine.c_str(), r, g, b );
		y += gHUD.m_scrinfo.iCharHeight - 2;
	}
	y += numberSpriteHeight;

	// Timers / Score
	for ( size_t i = 0 ; i < animationLines.size() ; i++ ) {
		auto &animationLine = animationLines.at( i );

		x = xOffset + CORNER_OFFSET + 60;
		gHUD.DrawHudString( x, y, 200, animationLine.label.c_str(), r, g, b );
		int x2 = ScreenWidth - xOffset - CORNER_OFFSET - formattedTimeSpriteWidth - 60;
		gHUD.DrawHudString( x2, y, 200, animationLine.recordLabel.c_str(), r, g, b );

		y += gHUD.m_scrinfo.iCharHeight;
		if ( animationLine.recordBeaten && !animationLine.value->isRunning ) {
			int r2 = 255, g2 = 255, b2 = 0;
			animationLine.value->Draw( x, y, r2, g2, b2 );
			gHUD.DrawHudString( x + formattedTimeSpriteWidth + 2, y - 4, 200, "PB!", r2, g2, b2 );
		} else {
			animationLine.value->Draw( x, y, r, g, b );
		}
		animationLine.recordValue->Draw( x2, y, r, g, b );
		y += gHUD.m_scrinfo.iCharHeight * 2;
	}

	// Statistics
	x = ScreenWidth / 2;
	for ( const auto &statLine : statLines ) {
		gHUD.DrawHudString( x - 140, y, 160, statLine.key.c_str(), r, g, b );
		gHUD.DrawHudStringKeepRight( x + 140, y, 160, statLine.value.c_str(), r, g, b );
		y += gHUD.m_scrinfo.iCharHeight - 2;
	}

	// Bottom messages
	y = ScreenHeight - yOffset - CORNER_OFFSET - gHUD.m_scrinfo.iCharHeight;
	gHUD.DrawHudStringKeepCenter( x, y, 300, "PRESS ESC TO QUIT OR START THE NEW GAME", r, g, b );

	if ( cheated ) {
		y -= gHUD.m_scrinfo.iCharHeight;
		gHUD.DrawHudStringKeepCenter( x, y, 300, "YOU'VE BEEN CHEATING OR CHANGED STARTING MAP - RESULTS WON'T BE SAVED", 200, 0, 0 );
	}

	return 1;
}

int CHudEndScreen::MsgFunc_EndActiv( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	cheated = READ_BYTE();
	m_iFlags |= HUD_ACTIVE;

	for ( const auto &animationLine : animationLines ) {
		animationLine.value->StartRunning();
		animationLine.recordValue->StartRunning();
	}

	if ( !cheated && recordBeaten ) {
		// TODO: refresh only current config file instead of all of them
		GameModeGUI_RefreshConfigFiles();
	}

	return 1;
}

int CHudEndScreen::MsgFunc_EndTitle( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	const char *stringMessage = READ_STRING();

	titleLines.push_back( stringMessage );

	return 1;
}

int CHudEndScreen::MsgFunc_EndTime( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	const char *stringMessage = READ_STRING();
	auto messageParts = aux::str::split( stringMessage, '|' );

	float time = READ_FLOAT();
	float recordTime = READ_FLOAT();
	recordBeaten = READ_BYTE();

	animationLines.push_back( {
		messageParts.at( 0 ),
		std::make_unique<CHudRunningTimerAnimation>( time ),

		recordBeaten,
		messageParts.size() > 1 ? messageParts.at( 1 ) : "",
		std::make_unique<CHudRunningTimerAnimation>( recordTime, 0.0f )
	} );
	return 1;
}

int CHudEndScreen::MsgFunc_EndScore( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	const char *stringMessage = READ_STRING();
	auto messageParts = aux::str::split( stringMessage, '|' );

	float score = READ_LONG();
	float recordScore = READ_LONG();
	recordBeaten = READ_BYTE();

	animationLines.push_back( {
		messageParts.at( 0 ),
		std::make_unique<CHudRunningScoreAnimation>( score ),

		recordBeaten,
		messageParts.size() > 1 ? messageParts.at( 1 ) : "",
		std::make_unique<CHudRunningScoreAnimation>( recordScore, 0.0f ),
	} );

	return 1;
}

int CHudEndScreen::MsgFunc_EndStat( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	const char *stringMessage = READ_STRING();
	auto messageParts = aux::str::split( stringMessage, '|' );

	statLines.push_back( {
		messageParts.at( 0 ),
		messageParts.size() > 1 ? messageParts.at( 1 ) : "",
	} );

	return 1;
}