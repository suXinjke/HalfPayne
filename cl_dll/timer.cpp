#include "hud.h"
#include "cl_util.h"
#include "../common/event_api.h"
#include "parsemsg.h"
#include "com_weapons.h"
#include "triangleapi.h"

DECLARE_MESSAGE( m_Timer, TimerValue )
DECLARE_MESSAGE( m_Timer, TimerMsg )
DECLARE_MESSAGE( m_Timer, TimerEnd )
DECLARE_MESSAGE( m_Timer, TimerPause )
DECLARE_MESSAGE( m_Timer, TimerCheat )

extern globalvars_t *gpGlobals;

#define RUNTIME_SOUND_DURATION 0.072f
#define RUNTIME_UPDATE_TIME 0.02f
#define ENDSCREEN_MESSAGE_UPDATE_TIME 3.0f
#define TIMER_PAUSED_BLINK_TIME 0.7f

#define MESSAGE_BRIGHTENESS 200
#define RGB_TIME_PB_COLOR 0x00FFFF00 // 255, 255, 255

int CHudTimer::Init(void)
{
	HOOK_MESSAGE( TimerValue );
	HOOK_MESSAGE( TimerMsg );
	HOOK_MESSAGE( TimerEnd );
	HOOK_MESSAGE( TimerPause );
	HOOK_MESSAGE( TimerCheat );
	
	gHUD.AddHudElem(this);

	return 1;
}

void CHudTimer::Reset( void )
{
	m_iFlags = 0;

	paused = false;
	ended = false;
	cheated = false;
	blinked = false;
	time = 0.0f;
	messages.clear();
	endScreenMessages.clear();

	kills = 0;
	headshotKills = 0;
	explosiveKills = 0;
	crowbarKills = 0;
	projectileKills = 0;

	secondsInSlowmotion = 0.0f;
}

int CHudTimer::Draw( float flTime )
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

	if ( !ended ) {	
		if ( paused ) {
			if ( !blinked ) {
				ScaleColors( r, g, b, 50 );
			}
			
			if ( gEngfuncs.GetAbsoluteTime() > nextTimerBlinkTime ) {
				blinked = !blinked;
				nextTimerBlinkTime = gEngfuncs.GetAbsoluteTime() + TIMER_PAUSED_BLINK_TIME;
			}
		}
		if ( cheated ) {
			gHUD.DrawFormattedTime( time, x - formattedTimeSpriteWidth, y, r, 0, 0 );
		} else {
			gHUD.DrawFormattedTime( time, x - formattedTimeSpriteWidth, y, r, g, b );
		}
		y += numberSpriteHeight;
		DrawMessages( x, y );

	} else { 
		DrawEndScreen();
	}

	return 1;
}

void CHudTimer::DrawMessages( int x, int y )
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

		int timeAddedStringWidth = gHUD.GetStringWidth( timerMessage.timeAddedString.c_str() );
		gEngfuncs.pfnFillRGBABlend( XPROJECT( screen.x ) - timeAddedStringWidth / 2 - 5, YPROJECT( screen.y ) - 1, timeAddedStringWidth + 8, gHUD.m_scrinfo.iCharHeight + 4, 110, 110, 110, alpha );
		gHUD.DrawHudStringKeepCenter( XPROJECT( screen.x ), YPROJECT( screen.y ), 200, timerMessage.timeAddedString.c_str(), r2, g2, b2 );
	}
}

void CHudTimer::DrawEndScreen()
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
	gHUD.DrawHudStringKeepCenter( x, y, 200, "BLACK MESA MINUTE", r, g, b );
	y += gHUD.m_scrinfo.iCharHeight - 2;
	gHUD.DrawHudStringKeepCenter( x, y, 200, endScreenLevelCompletedMessage.c_str(), r, g, b );
	y += gHUD.m_scrinfo.iCharHeight * 2;

	// Times
	x = xOffset + CORNER_OFFSET + 60;
	y += numberSpriteHeight;
	DrawEndScreenTimes( x, y );

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

void CHudTimer::DrawEndScreenTimes( int x, int y )
{
	int r = MESSAGE_BRIGHTENESS;
	int g = MESSAGE_BRIGHTENESS;
	int b = MESSAGE_BRIGHTENESS;

	int r2, g2, b2;
	UnpackRGB( r2, g2, b2, RGB_TIME_PB_COLOR );

	int formattedTimeSpriteWidth = gHUD.GetNumberSpriteWidth() * 8;

	gHUD.DrawHudString( x, y, 200, "TIME SCORE", r, g, b );
	y += gHUD.m_scrinfo.iCharHeight;
	if ( !timeRunningAnimation.isRunning && time > oldTimeRecord && !cheated ) {
		timeRunningAnimation.Draw( x, y, r2, g2, b2 );
		gHUD.DrawHudString( x + formattedTimeSpriteWidth + 2, y - 4, 200, "PB!", r2, g2, b2 );
	} else {
		timeRunningAnimation.Draw( x, y, r, g, b );
	}
	y += gHUD.m_scrinfo.iCharHeight * 2;

	gHUD.DrawHudString( x, y, 200, "REAL TIME", r, g, b );
	y += gHUD.m_scrinfo.iCharHeight;
	if ( !realTimeRunningAnimation.isRunning && realTime < oldRealTimeRecord && !cheated ) {
		realTimeRunningAnimation.Draw( x, y, r2, g2, b2 );
		gHUD.DrawHudString( x + formattedTimeSpriteWidth + 2, y - 4, 200, "PB!", r2, g2, b2 );
	} else {
		realTimeRunningAnimation.Draw( x, y, r, g, b );
	}
	y += gHUD.m_scrinfo.iCharHeight * 2;

	gHUD.DrawHudString( x, y, 200, "REAL TIME MINUS SCORE", r, g, b );
	y += gHUD.m_scrinfo.iCharHeight;
	if ( !realTimeMinusTimeRunningAnimation.isRunning && realTimeMinusTime < oldRealTimeMinusTimeRecord && !cheated ) {
		realTimeMinusTimeRunningAnimation.Draw( x, y, r2, g2, b2 );
		gHUD.DrawHudString( x + formattedTimeSpriteWidth + 2, y - 4, 200, "PB!", r2, g2, b2 );
	} else {
		realTimeMinusTimeRunningAnimation.Draw( x, y, r, g, b );
	}
	y += gHUD.m_scrinfo.iCharHeight * 2;

	if ( timeRunningAnimation.isRunning ) {
		if ( gpGlobals->time > nextRuntimeSoundTime ) {
			gEngfuncs.pEventAPI->EV_PlaySound( -1, gEngfuncs.GetLocalPlayer()->origin, 0, "var/runtime.wav", 1.0, ATTN_NORM, 0, PITCH_NORM, true );
			nextRuntimeSoundTime = gpGlobals->time + RUNTIME_SOUND_DURATION;
		}
	}
}

void CHudTimer::DrawEndScreenRecords( int x, int y )
{
	int r = MESSAGE_BRIGHTENESS;
	int g = MESSAGE_BRIGHTENESS;
	int b = MESSAGE_BRIGHTENESS;

	gHUD.DrawHudString( x, y, 200, "PERSONAL BESTS", r, g, b );
	y += gHUD.m_scrinfo.iCharHeight;
	gHUD.DrawFormattedTime( oldTimeRecord, x, y, r, g, b );
	y += gHUD.m_scrinfo.iCharHeight * 2;

	y += gHUD.m_scrinfo.iCharHeight;
	gHUD.DrawFormattedTime( oldRealTimeRecord, x, y, r, g, b );
	y += gHUD.m_scrinfo.iCharHeight * 2;

	y += gHUD.m_scrinfo.iCharHeight;
	gHUD.DrawFormattedTime( oldRealTimeMinusTimeRecord, x, y, r, g, b );
	y += gHUD.m_scrinfo.iCharHeight * 2;
}

void CHudTimer::DrawEndScreenStatistics( int x, int y )
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

int CHudTimer::MsgFunc_TimerValue( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ(pbuf, iSize);
	time = READ_FLOAT();

	m_iFlags |= HUD_ACTIVE;

	return 1;
}

int CHudTimer::MsgFunc_TimerMsg( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	const char* message = READ_STRING();
	int timeAdded = READ_LONG();
	float coordsX = READ_COORD();
	float coordsY = READ_COORD();
	float coordsZ = READ_COORD();

	Vector coords( coordsX, coordsY, coordsZ );
	messages.push_back( CHudTimerMessage( std::string( message ), timeAdded, coords, gEngfuncs.GetAbsoluteTime() ) );

	m_iFlags |= HUD_ACTIVE;

	return 1;
}

int CHudTimer::MsgFunc_TimerEnd( const char *pszName, int iSize, void *pbuf )
{
	if ( ended ) {
		return 1;
	}

	BEGIN_READ( pbuf, iSize );

	endScreenLevelCompletedMessage = READ_STRING();

	time = READ_FLOAT();
	realTime = READ_FLOAT();
	float realTimeMinusTime = max( 0.0f, realTime - time ); 

	oldTimeRecord = READ_FLOAT();
	oldRealTimeRecord = READ_FLOAT();
	oldRealTimeMinusTimeRecord = READ_FLOAT();
	
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

	timeRunningAnimation.StartRunning( time );
	realTimeRunningAnimation.StartRunning( realTime );
	realTimeMinusTimeRunningAnimation.StartRunning( realTimeMinusTime );

	nextRuntimeSoundTime = gpGlobals->time + RUNTIME_SOUND_DURATION;
		
	ended = true;
	
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
	cheated = true;

	return 1;
}






CHudRunningTimerAnimation::CHudRunningTimerAnimation()
{
	isRunning = false;

	time = 0.0f;
	endTime = 0.0f;
	timeStep = 0.0f;
	nextUpdateTime = 0.0f;
}

void CHudRunningTimerAnimation::StartRunning( float endTime, float stepFraction )
{
	time = 0.0f;

	if ( endTime <= 0.0f ) {
		endTime = 0.000000001f;
	}
	this->endTime = endTime;

	if ( stepFraction <= 0.0f ) {
		stepFraction = 60.0f;
	}
	this->timeStep = endTime / stepFraction;

	isRunning = true;
	nextUpdateTime = gpGlobals->time + RUNTIME_UPDATE_TIME;
}

int CHudRunningTimerAnimation::Draw( int x, int y, int r, int g, int b )
{
	if ( isRunning && gpGlobals->time > nextUpdateTime ) {
		if ( time < endTime ) {
			time += timeStep;
		}

		if ( time >= endTime ) {
			time = endTime;
			isRunning = false;
		} else {
			nextUpdateTime = gpGlobals->time + RUNTIME_UPDATE_TIME;
		}
	}

	return gHUD.DrawFormattedTime( time, x, y, r, g, b );
}