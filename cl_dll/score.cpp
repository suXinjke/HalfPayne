#include "hud.h"
#include "cl_util.h"
#include "../common/event_api.h"
#include "parsemsg.h"
#include "triangleapi.h"
#include <string>

DECLARE_MESSAGE( m_Score, ScoreValue )
DECLARE_MESSAGE( m_Score, ScoreMsg )
DECLARE_MESSAGE( m_Score, ScoreEnd )
DECLARE_MESSAGE( m_Score, ScoreCheat )

extern globalvars_t *gpGlobals;

#define RUNTIME_SOUND_DURATION 0.072f
#define RUNTIME_UPDATE_TIME 0.02f
#define ENDSCREEN_MESSAGE_UPDATE_TIME 3.0f

#define MESSAGE_BRIGHTENESS 200
#define RGB_TIME_PB_COLOR 0x00FFFF00 // 255, 255, 255

int CHudScore::Init(void)
{
	HOOK_MESSAGE( ScoreValue );
	HOOK_MESSAGE( ScoreMsg );
	HOOK_MESSAGE( ScoreEnd );
	HOOK_MESSAGE( ScoreCheat );
	
	gHUD.AddHudElem(this);

	return 1;
}

void CHudScore::Reset( void )
{
	m_iFlags = 0;

	ended = false;
	cheated = false;
	currentScore = 0;
	comboMultiplier = 1;
	comboMultiplierReset = 0.0f;
	messages.clear();

	kills = 0;
	headshotKills = 0;
	explosiveKills = 0;
	crowbarKills = 0;
	projectileKills = 0;
	secondsInSlowmotion = 0.0f;
}

int CHudScore::Draw( float flTime )
{
	if ( ( gHUD.m_iHideHUDDisplay & HIDEHUD_ALL )
		|| gEngfuncs.IsSpectateOnly() ) {
	
		return 1;
	}

	int r = MESSAGE_BRIGHTENESS;
	int g = MESSAGE_BRIGHTENESS;
	int b = MESSAGE_BRIGHTENESS;

	int x = ScreenWidth - CORNER_OFFSET;
	int y = 0;

	std::string numberString = std::to_string( currentScore );
	float formattedScoreSpriteWidth = gHUD.GetNumberSpriteWidth() * ( numberString.size() + ( ( numberString.size() - 1 ) / 3 ) / 2.0f );
	int numberSpriteHeight = gHUD.GetNumberSpriteHeight();

	if ( !ended ) {	
		if ( comboMultiplier > 1 ) {
			int r2 = r;
			int g2 = g;
			int b2 = b;
			int alpha = ( comboMultiplierReset / 8.0f ) * 1024;
			if ( alpha > 255 ) {
				alpha = 255;
			}

			ScaleColors( r2, g2, b2, alpha );
			gHUD.DrawHudStringKeepRight( x, y, 200, ( "x" + std::to_string( comboMultiplier ) ).c_str(), r2, g2, b2 );
		}
		y += CORNER_OFFSET;

		if ( cheated ) {
			gHUD.DrawFormattedNumber( currentScore, x - formattedScoreSpriteWidth, y, r, 0, 0 );
		} else {
			gHUD.DrawFormattedNumber( currentScore, x - formattedScoreSpriteWidth, y, r, g, b );
		}
		y += numberSpriteHeight;
		DrawMessages( x, y );
	} else { 
		DrawEndScreen();
	}

	return 1;
}

void CHudScore::DrawMessages( int x, int y )
{
	int r = MESSAGE_BRIGHTENESS;
	int g = MESSAGE_BRIGHTENESS;
	int b = MESSAGE_BRIGHTENESS;

	for ( int i = messages.size() - 1; i >= 0; i-- ) {
		CHudTimerMessage timerMessage = messages.at( i );

		if ( fabs( timerMessage.timerMessageRemovalTime - gEngfuncs.GetAbsoluteTime() ) >= TIMER_MESSAGE_REMOVAL_TIME ) {
			messages.erase( messages.begin() + i );
		}
	}

	for ( int i = messages.size( ) - 1; i >= 0; i-- ) {
		CHudTimerMessage timerMessage = messages.at( i );
		gHUD.DrawHudStringKeepRight( x, y, 200, timerMessage.message.c_str(), r, g, b );
		y += gHUD.m_scrinfo.iCharHeight - 2;

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
		int timeAddedStringWidth = max( gHUD.GetStringWidth( timerMessage.timeAddedString.c_str() ), gHUD.GetStringWidth( timerMessage.timeAddedUpperString.c_str() ) );

		// new beg
		gEngfuncs.pfnFillRGBABlend( XPROJECT( screen.x ) - timeAddedStringWidth / 2 - 5, YPROJECT( screen.y ) - 1 - gHUD.m_scrinfo.iCharHeight - 4 , timeAddedStringWidth + 8, gHUD.m_scrinfo.iCharHeight + 4, 0, 0, 0, alpha );
		gHUD.DrawHudStringKeepCenter( XPROJECT( screen.x ), YPROJECT( screen.y ) - gHUD.m_scrinfo.iCharHeight - 4, 200, timerMessage.timeAddedUpperString.c_str(), r2, g2, b2 );
		// new end

		gEngfuncs.pfnFillRGBABlend( XPROJECT( screen.x ) - timeAddedStringWidth / 2 - 5, YPROJECT( screen.y ) - 1, timeAddedStringWidth + 8, gHUD.m_scrinfo.iCharHeight + 4, 110, 110, 110, alpha );
		gHUD.DrawHudStringKeepCenter( XPROJECT( screen.x ), YPROJECT( screen.y ), 200, timerMessage.timeAddedString.c_str(), r2, g2, b2 );
	}
}

void CHudScore::DrawEndScreen()
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
	gHUD.DrawHudStringKeepCenter( x, y, 200, "SCORE ATTACK", r, g, b );
	y += gHUD.m_scrinfo.iCharHeight - 2;
	gHUD.DrawHudStringKeepCenter( x, y, 200, endScreenLevelCompletedMessage.c_str(), r, g, b );
	y += gHUD.m_scrinfo.iCharHeight * 2;

	// Score
	x = xOffset + CORNER_OFFSET + 60;
	y += numberSpriteHeight;
	DrawEndScreenScore( x, y );

	// Records
	x = ScreenWidth - xOffset - CORNER_OFFSET - formattedTimeSpriteWidth - 60;
	DrawEndScreenRecords( x, y );

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

void CHudScore::DrawEndScreenScore( int x, int y )
{
	int r = MESSAGE_BRIGHTENESS;
	int g = MESSAGE_BRIGHTENESS;
	int b = MESSAGE_BRIGHTENESS;

	int r2, g2, b2;
	UnpackRGB( r2, g2, b2, RGB_TIME_PB_COLOR );

	std::string numberString = std::to_string( currentScore );
	float formattedScoreSpriteWidth = gHUD.GetNumberSpriteWidth() * ( numberString.size() + ( ( numberString.size() - 1 ) / 3 ) / 2.0f );

	gHUD.DrawHudString( x, y, 200, "SCORE", r, g, b );
	y += gHUD.m_scrinfo.iCharHeight;
	if ( !scoreRunningAnimation.isRunning && currentScore > oldScoreRecord && !cheated ) {
		scoreRunningAnimation.Draw( x, y, r2, g2, b2 );
		gHUD.DrawHudString( x + formattedScoreSpriteWidth + 2, y - 4, 200, "PB!", r2, g2, b2 );
	} else {
		scoreRunningAnimation.Draw( x, y, r, g, b );
	}
	y += gHUD.m_scrinfo.iCharHeight * 2;

	if ( scoreRunningAnimation.isRunning ) {
		if ( gpGlobals->time > nextRuntimeSoundTime ) {
			gEngfuncs.pEventAPI->EV_PlaySound( -1, gEngfuncs.GetLocalPlayer()->origin, 0, "var/runtime.wav", 1.0, ATTN_NORM, 0, PITCH_NORM, true );
			nextRuntimeSoundTime = gpGlobals->time + RUNTIME_SOUND_DURATION;
		}
	}
}

void CHudScore::DrawEndScreenRecords( int x, int y )
{
	int r = MESSAGE_BRIGHTENESS;
	int g = MESSAGE_BRIGHTENESS;
	int b = MESSAGE_BRIGHTENESS;

	gHUD.DrawHudString( x, y, 200, "PERSONAL BEST", r, g, b );
	y += gHUD.m_scrinfo.iCharHeight;
	gHUD.DrawFormattedNumber( oldScoreRecord, x, y, r, g, b );
	y += gHUD.m_scrinfo.iCharHeight * 2;
}

void CHudScore::DrawEndScreenStatistics( int x, int y )
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

int CHudScore::MsgFunc_ScoreValue( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ(pbuf, iSize);
	currentScore = READ_LONG();
	comboMultiplier = READ_LONG();
	comboMultiplierReset = READ_FLOAT();

	m_iFlags |= HUD_ACTIVE;

	return 1;
}

int CHudScore::MsgFunc_ScoreMsg( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	const char* message = READ_STRING();
	int scoreToAdd = READ_LONG();
	float comboMultiplier = READ_FLOAT();
	float coordsX = READ_COORD();
	float coordsY = READ_COORD();
	float coordsZ = READ_COORD();

	bool comboMultiplierIsInteger = abs( comboMultiplier - std::lround( comboMultiplier ) ) < 0.00000001f;

	char upperString[128];
	if ( comboMultiplierIsInteger ) {
		std::sprintf( upperString, "%d x%.0f", scoreToAdd, comboMultiplier );
	} else {
		std::sprintf( upperString, "%d x%.1f", scoreToAdd, comboMultiplier );
	}

	Vector coords( coordsX, coordsY, coordsZ );
	messages.push_back( CHudTimerMessage( std::string( message ), std::to_string( ( int ) ( scoreToAdd * comboMultiplier ) ), coords, gEngfuncs.GetAbsoluteTime(), upperString ) );

	m_iFlags |= HUD_ACTIVE;

	return 1;
}

int CHudScore::MsgFunc_ScoreEnd( const char *pszName, int iSize, void *pbuf )
{
	if ( ended ) {
		return 1;
	}

	BEGIN_READ( pbuf, iSize );

	endScreenLevelCompletedMessage = READ_STRING();

	currentScore = READ_LONG();

	oldScoreRecord = READ_LONG();
	
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

	// TODO: IT DOESN'T ACCEPT TIME DAMN IT
	scoreRunningAnimation.StartRunning( currentScore );

	nextRuntimeSoundTime = gpGlobals->time + RUNTIME_SOUND_DURATION;
		
	ended = true;
	
	m_iFlags |= HUD_ACTIVE;

	return 1;
}

int CHudScore::MsgFunc_ScoreCheat( const char *pszName, int iSize, void *pbuf )
{
	cheated = true;

	return 1;
}





CHudRunningScoreAnimation::CHudRunningScoreAnimation()
{
	isRunning = false;

	score = 0;
	endScore = 0;
	scoreStep = 0.0f;
	nextUpdateTime = 0.0f;
}

void CHudRunningScoreAnimation::StartRunning( int endScore, float stepFraction )
{
	score = 0;

	if ( endScore < 0 ) {
		endScore = 0;
	}
	this->endScore = endScore;

	if ( stepFraction <= 0.0f ) {
		stepFraction = 60.0f;
	}
	this->scoreStep = endScore / stepFraction;

	isRunning = true;
	nextUpdateTime = gpGlobals->time + RUNTIME_UPDATE_TIME;
}

int CHudRunningScoreAnimation::Draw( int x, int y, int r, int g, int b )
{
	if ( isRunning && gpGlobals->time > nextUpdateTime ) {
		if ( score < endScore ) {
			score += scoreStep;
		}

		if ( score >= endScore ) {
			score = endScore;
			isRunning = false;
		} else {
			nextUpdateTime = gpGlobals->time + RUNTIME_UPDATE_TIME;
		}
	}

	return gHUD.DrawFormattedNumber( score, x, y, r, g, b );
}