#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

DECLARE_MESSAGE( m_Painkiller, PillCount ) // PillCount = PainkillerCount

int CHudPainkiller::Init( void )
{
	HOOK_MESSAGE( PillCount );

	m_iFlags |= HUD_ACTIVE;
	
	painkillerCount = 0;
	
	gHUD.AddHudElem(this);

	return 1;
}

int CHudPainkiller::VidInit( void )
{
	painKillerSprite = gHUD.GetSpriteIndex( "painkiller" );

	return 1;
}

int CHudPainkiller::Draw( float flTime )
{
	if ( !( gHUD.m_iWeaponBits & ( 1 << ( WEAPON_SUIT ) ) )
		|| ( gHUD.m_iHideHUDDisplay & HIDEHUD_HEALTH )
		|| gEngfuncs.IsSpectateOnly()
		|| painkillerCount <= 0 ) {

		return 1;
	}
	
	int r = 200, g = 200, b = 200;

	wrect_t painkillerRect = gHUD.GetSpriteRect( painKillerSprite );
	int painkillerRectHeight = painkillerRect.bottom - painkillerRect.top;

	int x = CORNER_OFFSET + HEALTH_SPRITE_WIDTH + BOTTOM_LEFT_SPACING + HOURGLASS_SPRITE_WIDTH + BOTTOM_LEFT_SPACING;
	int y = ScreenHeight - painkillerRectHeight - CORNER_OFFSET;

	SPR_Set( gHUD.GetSprite( painKillerSprite ), 255, 255, 255 );
	SPR_Draw( 0, x, y, &painkillerRect );

	x -= PAINKILLER_SPRITE_WIDTH + 4;
	y = ScreenHeight - CORNER_OFFSET - gHUD.m_iFontHeight - 2;
	gHUD.DrawHudNumber(x, y, DHN_3DIGITS , painkillerCount, r, g, b);

	return 1;
}

int CHudPainkiller::MsgFunc_PillCount( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ(pbuf, iSize);
	painkillerCount = READ_BYTE();

	m_iFlags |= HUD_ACTIVE;

	return 1;
}