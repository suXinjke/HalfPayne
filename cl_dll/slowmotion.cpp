#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

DECLARE_MESSAGE(m_SlowMotion, SlowMotion)

int CHudSlowMotion::Init(void)
{
	HOOK_MESSAGE(SlowMotion);

	m_iFlags |= HUD_ACTIVE;
	
	slowMotionCharge = 100;
	m_iFlags = 0;
	
	gHUD.AddHudElem(this);

	return 1;
}

int CHudSlowMotion::VidInit( void )
{
	hourglassStrokeSprite = gHUD.GetSpriteIndex( "hourglass_stroke" );
	hourglassFillSprite = gHUD.GetSpriteIndex( "hourglass_fill" );

	return 1;
}

int CHudSlowMotion::Draw(float flTime)
{
	if ( !( gHUD.m_iWeaponBits & ( 1 << ( WEAPON_SUIT ) ) )
		|| ( gHUD.m_iHideHUDDisplay & HIDEHUD_HEALTH )
		|| gEngfuncs.IsSpectateOnly()
		|| std::string( gEngfuncs.pfnGetLevelName() ) == "maps/nightmare.bsp" ) {
		
		return 1;
	}

	wrect_t hourglassRect = gHUD.GetSpriteRect( hourglassStrokeSprite );
	int hourglassRectHeight = hourglassRect.bottom - hourglassRect.top;

	int x = CORNER_OFFSET + HEALTH_SPRITE_WIDTH;
	int y = ScreenHeight - hourglassRectHeight - CORNER_OFFSET;

	SPR_Set( gHUD.GetSprite( hourglassStrokeSprite ), 20, 20, 20 );
	SPR_DrawAdditive( 0, x, y, &hourglassRect );

	float slowmotionPercent = ( slowMotionCharge / 100.0f );
	float slowmotionLackPercent = 1.0f - slowmotionPercent;
	
	if ( slowmotionPercent > 0.0f ) {
		hourglassRect = gHUD.GetSpriteRect( hourglassFillSprite );
		int hourglassHeight = hourglassRectHeight * slowmotionLackPercent;
		hourglassRect.top = hourglassHeight;
		
		SPR_Set( gHUD.GetSprite( hourglassStrokeSprite ), 180, 180, 180 );
		SPR_DrawAdditive( 0, x, y + hourglassHeight, &hourglassRect );
	}

	return 1;
}

int CHudSlowMotion::MsgFunc_SlowMotion(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ(pbuf, iSize);
	slowMotionCharge = READ_BYTE();

	m_iFlags |= HUD_ACTIVE;

	return 1;
}