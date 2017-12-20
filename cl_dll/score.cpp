#include "hud.h"
#include "cl_util.h"
#include "../common/event_api.h"
#include "parsemsg.h"

DECLARE_MESSAGE( m_Score, ScoreDeact )
DECLARE_MESSAGE( m_Score, ScoreValue )
DECLARE_MESSAGE( m_Score, ScoreCheat )

extern globalvars_t *gpGlobals;


int CHudScore::Init(void)
{
	HOOK_MESSAGE( ScoreDeact );
	HOOK_MESSAGE( ScoreValue );
	HOOK_MESSAGE( ScoreCheat );
	
	gHUD.AddHudElem( this );

	return 1;
}

void CHudScore::Reset( void )
{
	m_iFlags = 0;

	cheated = false;
	currentScore = 0;
	comboMultiplier = 1;
	comboMultiplierReset = 0.0f;

	yOffset = 0;
}

int CHudScore::Draw( float flTime )
{
	if ( ( gHUD.m_iHideHUDDisplay & HIDEHUD_ALL ) || gEngfuncs.IsSpectateOnly() ) {
		return 1;
	}

	int r = MESSAGE_BRIGHTENESS;
	int g = !cheated ? MESSAGE_BRIGHTENESS : 0;
	int b = !cheated ? MESSAGE_BRIGHTENESS : 0;

	int x = ScreenWidth - CORNER_OFFSET;
	int y = CORNER_OFFSET + yOffset;

	std::string numberString = std::to_string( currentScore );
	float formattedScoreSpriteWidth = gHUD.GetNumberSpriteWidth() * ( numberString.size() + ( ( numberString.size() - 1 ) / 3 ) / 2.0f );
	int numberSpriteHeight = gHUD.GetNumberSpriteHeight();

	gHUD.DrawHudStringKeepRight( x, y, 200, "SCORE", r, g, b );

	if ( comboMultiplier > 1 ) {
		int r2 = r;
		int g2 = g;
		int b2 = b;
		int alpha = ( comboMultiplierReset / 8.0f ) * 1024;
		if ( alpha > 255 ) {
			alpha = 255;
		}

		ScaleColors( r2, g2, b2, alpha );
		gHUD.DrawHudStringKeepRight( x - gHUD.GetStringWidth( "SCORE" ) - 8, y, 200, ( "x" + std::to_string( comboMultiplier ) ).c_str(), r2, g2, b2 );
	}
	y += gHUD.m_scrinfo.iCharHeight + 2;

	gHUD.DrawFormattedNumber( currentScore, x - formattedScoreSpriteWidth, y, r, g, b );

	return 1;
}

int CHudScore::MsgFunc_ScoreValue( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	currentScore = READ_LONG();
	comboMultiplier = READ_LONG();
	comboMultiplierReset = READ_FLOAT();
	yOffset = READ_LONG();

	m_iFlags |= HUD_ACTIVE;

	return 1;
}

int CHudScore::MsgFunc_ScoreCheat( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	cheated = true;

	return 1;
}

int CHudScore::MsgFunc_ScoreDeact( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	this->Reset();

	return 1;
}