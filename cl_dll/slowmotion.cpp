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

int CHudSlowMotion::Draw(float flTime)
{
	int r = 255, g = 255, b = 255;
	int x = 42;
	int y = ScreenHeight - gHUD.m_iFontHeight - gHUD.m_iFontHeight * 2;

	if (gHUD.m_iHideHUDDisplay || gEngfuncs.IsSpectateOnly())
		return 1;

	gHUD.DrawHudNumber(x, y, DHN_3DIGITS | DHN_DRAWZERO, slowMotionCharge, r, g, b);

	return 1;
}

int CHudSlowMotion::MsgFunc_SlowMotion(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ(pbuf, iSize);
	slowMotionCharge = READ_BYTE();

	m_iFlags |= HUD_ACTIVE;

	return 1;
}