#include "hud.h"
#include "cl_util.h"
#include "../common/event_api.h"
#include "parsemsg.h"
#include "gameplay_mod.h"
#include "cpp_aux.h"

DECLARE_MESSAGE( m_CentralLabel, CLabelVal )
DECLARE_MESSAGE( m_CentralLabel, CLabelGMod )

extern globalvars_t *gpGlobals;

int CHudCentralLabel::Init(void)
{
	HOOK_MESSAGE( CLabelVal );
	HOOK_MESSAGE( CLabelGMod );
	
	gHUD.AddHudElem( this );

	return 1;
}

void CHudCentralLabel::Reset( void )
{
	m_iFlags = 0;

	timeUntilStopDrawing = 0.0f;
	alpha = 0;
	label = "";

	m_iFlags |= HUD_ACTIVE;
}

int CHudCentralLabel::Draw( float flTime )
{
	if ( ( gHUD.m_iHideHUDDisplay & HIDEHUD_ALL ) || gEngfuncs.IsSpectateOnly() ) {
		return 1;
	}

	if ( gEngfuncs.GetAbsoluteTime() > timeUntilStopDrawing ) {
		alpha -= 5;
		if ( alpha < 0 ) {
			alpha = 0;
		}
	} else {
		alpha += 5;
		if ( alpha > 255 ) {
			alpha = 255;
		}
	}

	int r = 255;
	int g = 255;
	int b = 255;
	ScaleColors( r, g, b, alpha );

	int x = ScreenWidth / 2;
	int y = ScreenHeight / 4;

	gHUD.DrawHudStringKeepCenter( x, y, 0, label.c_str(), r, g, b );
	if ( subLabel.size() > 0 ) {
		int lineWidth = max( gHUD.GetStringWidth( label.c_str() ), gHUD.GetStringWidth( subLabel.c_str() ) ) + 8;
		FillRGBA( x - lineWidth / 2, y + gHUD.m_scrinfo.iCharHeight + 2, lineWidth, 1, r, g, b, alpha / 3 );

		gHUD.DrawHudStringKeepCenter( x, y + gHUD.m_scrinfo.iCharHeight + 2, 0, subLabel.c_str(), r, g, b );
	}

	return 1;
}

int CHudCentralLabel::MsgFunc_CLabelVal( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	label = READ_STRING();
	subLabel = READ_STRING();

	m_iFlags |= HUD_ACTIVE;
	timeUntilStopDrawing = gEngfuncs.GetAbsoluteTime() + 9.0f;

	return 1;
}

int CHudCentralLabel::MsgFunc_CLabelGMod( const char *pszName, int iSize, void *pbuf ) {
	BEGIN_READ( pbuf, iSize );
	std::string modName = READ_STRING();
	
	if ( gameplayMods::byString.find( modName ) != gameplayMods::byString.end() ) {
		auto mod = gameplayMods::byString[modName];

		label = mod->GetRandomGameplayModName();
		auto descriptionSplitted = aux::str::split( mod->GetRandomGameplayModDescription(), '\n' );
		subLabel = descriptionSplitted.size() > 0 ? descriptionSplitted.at( 0 ) : "";
		m_iFlags |= HUD_ACTIVE;
		timeUntilStopDrawing = gEngfuncs.GetAbsoluteTime() + 9.0f;
	}

	return 1;
}