#include "hud.h"
#include "cl_util.h"
#include "../common/event_api.h"
#include "parsemsg.h"
#include "triangleapi.h"

DECLARE_MESSAGE( m_Counter, CountLen )
DECLARE_MESSAGE( m_Counter, CountValue )
DECLARE_MESSAGE( m_Counter, CountDeact )
DECLARE_MESSAGE( m_Counter, CountOffse )

extern globalvars_t *gpGlobals;

int CHudCounter::Init( void )
{
	HOOK_MESSAGE( CountLen );
	HOOK_MESSAGE( CountValue );
	HOOK_MESSAGE( CountDeact );
	HOOK_MESSAGE( CountOffse );
	
	gHUD.AddHudElem( this );

	return 1;
}

void CHudCounter::Reset( void )
{
	values.clear();

	yOffset = 0;

	m_iFlags = 0;
}

int CHudCounter::Draw( float flTime )
{
	if ( ( gHUD.m_iHideHUDDisplay & HIDEHUD_ALL ) || gEngfuncs.IsSpectateOnly() ) {
	
		return 1;
	}



	int y = CORNER_OFFSET + yOffset;

	int numberSpriteWidth = gHUD.GetNumberSpriteWidth();
	int numberSpriteHeight = gHUD.GetNumberSpriteHeight();

	for ( const auto &value : values ) {
		int r, g, b;

		int x = ScreenWidth - CORNER_OFFSET;

		bool isCounter = value.maxCount >= 1;
		int firstValue = isCounter ?
			( value.maxCount > 1 ? value.maxCount : -1 ) :
			value.count;
		int secondValue = isCounter ?
			( value.maxCount > 1 ? value.count : -1 ) :
			-1;

		if ( isCounter && value.count >= value.maxCount ) {
			r = 255;
			g = 255;
			b = 0;
		} else {
			r = MESSAGE_BRIGHTENESS;
			g = MESSAGE_BRIGHTENESS;
			b = MESSAGE_BRIGHTENESS;
		}

		gHUD.DrawHudStringKeepRight( x, y, 200, value.title.c_str(), r, g, b );
		y += gHUD.m_scrinfo.iCharHeight + 2;

		if ( firstValue >= 0 ) {
			int firstValueLength = std::to_string( firstValue ).length() * numberSpriteWidth;
			gHUD.DrawFormattedNumber( firstValue, x - firstValueLength, y, r, g, b );

			x -= ( firstValueLength + 24 );
		}
		if ( secondValue >= 0 ) {
			FillRGBA( x, y, 1, gHUD.m_iFontHeight, r, g, b, 255 );

			x -= 8;

			int secondValueLength = std::to_string( secondValue ).length() * numberSpriteWidth;
			gHUD.DrawFormattedNumber( secondValue, x - secondValueLength, y, r, g, b );

			y += ( numberSpriteHeight + 4 );
		}
	}

	return 1;
}

int CHudCounter::MsgFunc_CountLen( const char *pszName, int iSize, void *pbuf ) {
	BEGIN_READ( pbuf, iSize );
	
	size_t counterCount = READ_SHORT();
	if ( values.size() != counterCount ) {
		values.resize( counterCount );
	}

	return 1;
}

int CHudCounter::MsgFunc_CountValue( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	size_t index = READ_SHORT();
	CounterValue value = { READ_LONG(), READ_LONG(), READ_STRING() };
	values[index] = value;

	m_iFlags |= HUD_ACTIVE;

	return 1;
}

int CHudCounter::MsgFunc_CountOffse( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	yOffset = READ_SHORT();

	m_iFlags |= HUD_ACTIVE;

	return 1;
}

int CHudCounter::MsgFunc_CountDeact( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	this->Reset();

	return 1;
}