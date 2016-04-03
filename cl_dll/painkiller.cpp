#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

DECLARE_MESSAGE( m_Painkiller, PillCount ) // PillCount = PainkillerCount

int CHudPainkiller::Init( void )
{
	HOOK_MESSAGE( PillCount );

	m_iFlags |= HUD_ACTIVE;
	
	painkillerCount = 0;
	m_iFlags = 0;
	
	gHUD.AddHudElem(this);

	return 1;
}

int CHudPainkiller::Draw( float flTime )
{
	int r = 255, g = 0, b = 0;
	int x = 42;
	int y = ScreenHeight - gHUD.m_iFontHeight - gHUD.m_iFontHeight * 2 * 2;

	if (gHUD.m_iHideHUDDisplay || gEngfuncs.IsSpectateOnly())
		return 1;

	gHUD.DrawHudNumber(x, y, DHN_3DIGITS | DHN_DRAWZERO, painkillerCount, r, g, b);

	return 1;
}

int CHudPainkiller::MsgFunc_PillCount( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ(pbuf, iSize);
	painkillerCount = READ_BYTE();

	m_iFlags |= HUD_ACTIVE;

	return 1;
}