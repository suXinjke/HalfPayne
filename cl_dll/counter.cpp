#include "hud.h"
#include "cl_util.h"
#include "../common/event_api.h"
#include "parsemsg.h"
#include "triangleapi.h"

DECLARE_MESSAGE( m_Counter, CountValue )
DECLARE_MESSAGE( m_Counter, CountCheat )
DECLARE_MESSAGE( m_Counter, CountDeact )

extern globalvars_t *gpGlobals;

int CHudCounter::Init( void )
{
	HOOK_MESSAGE( CountValue );
	HOOK_MESSAGE( CountCheat );
	HOOK_MESSAGE( CountDeact );
	
	gHUD.AddHudElem( this );

	return 1;
}

void CHudCounter::Reset( void )
{
	cheated = false;
	values.clear();

	yOffset = 0;

	m_iFlags = 0;
}

int CHudCounter::Draw( float flTime )
{
	if ( ( gHUD.m_iHideHUDDisplay & HIDEHUD_ALL ) || gEngfuncs.IsSpectateOnly() ) {
	
		return 1;
	}

	int r = MESSAGE_BRIGHTENESS;
	int g = !cheated ? MESSAGE_BRIGHTENESS : 0;
	int b = !cheated ? MESSAGE_BRIGHTENESS : 0;

	int y = CORNER_OFFSET + yOffset;

	int numberSpriteWidth = gHUD.GetNumberSpriteWidth();
	int numberSpriteHeight = gHUD.GetNumberSpriteHeight();

	for ( const auto &value : values ) {
		int x = ScreenWidth - CORNER_OFFSET;

		gHUD.DrawHudStringKeepRight( x, y, 200, value.title.c_str(), r, g, b );
		y += gHUD.m_scrinfo.iCharHeight + 2;

		if ( value.maxCount ) {
			int maxCountLength = std::to_string( value.maxCount ).length() * numberSpriteWidth;
			gHUD.DrawFormattedNumber( value.maxCount, x - maxCountLength, y, r, g, b );

			x -= ( maxCountLength + 4 );

			FillRGBA( x, y, 1, gHUD.m_iFontHeight, r, g, b, 255 );

			x -= 4;
		}

		int countLength = std::to_string( value.count ).length() * numberSpriteWidth;
		gHUD.DrawFormattedNumber( value.count, x - countLength, y, r, g, b );

		y += ( numberSpriteHeight + 4 );
	}

	return 1;
}

int CHudCounter::MsgFunc_CountValue( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	int counterCount = READ_SHORT();
	for ( int i = 0 ; i < counterCount ; i++ ) {
		CounterValue value = { READ_LONG(), READ_LONG(), READ_STRING() };
		if ( i + 1 > values.size() ) {
			values.push_back( value );
		} else {
			values[i] = value;
		}
	}

	yOffset = READ_LONG();
	m_iFlags |= HUD_ACTIVE;

	return 1;
}

int CHudCounter::MsgFunc_CountCheat( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	cheated = true;

	return 1;
}

int CHudCounter::MsgFunc_CountDeact( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	this->Reset();

	return 1;
}