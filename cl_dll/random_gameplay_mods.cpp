#include "hud.h"
#include "cl_util.h"
#include "../common/event_api.h"
#include "parsemsg.h"
#include "triangleapi.h"
#include "util_aux.h"
#include "string_aux.h"
#include "../fmt/printf.h"

DECLARE_MESSAGE( m_RandomGameplayMods, RandModLen )
DECLARE_MESSAGE( m_RandomGameplayMods, RandModVal )

DECLARE_MESSAGE( m_RandomGameplayMods, PropModLen )
DECLARE_MESSAGE( m_RandomGameplayMods, PropModVal )
DECLARE_MESSAGE( m_RandomGameplayMods, PropModVot )
DECLARE_MESSAGE( m_RandomGameplayMods, PropModAni )

extern globalvars_t *gpGlobals;

int CHudRandomGameplayMods::Init( void )
{
	HOOK_MESSAGE( RandModVal );
	HOOK_MESSAGE( RandModLen );
	
	HOOK_MESSAGE( PropModLen );
	HOOK_MESSAGE( PropModVal );
	HOOK_MESSAGE( PropModVot );
	HOOK_MESSAGE( PropModAni );
	
	gHUD.AddHudElem( this );

	return 1;
}

void CHudRandomGameplayMods::Reset( void )
{
	m_iFlags |= HUD_ACTIVE;

	highlightIndex = -1;
	timeUntilNextHighlight = 0.0f;
	mods.clear();
	proposedMods.clear();
}

int CHudRandomGameplayMods::Draw( float flTime )
{
	if ( ( gHUD.m_iHideHUDDisplay & HIDEHUD_ALL ) || gEngfuncs.IsSpectateOnly() ) {
	
		return 1;
	}

	const int ITEM_WIDTH = 180;
	const int ITEM_HEIGHT = 40;
	const int SPACING = 10;

	for ( size_t i = 0; i < mods.size(); i++ ) {
		auto &mod = mods.at( i );

		int x = ScreenWidth - ITEM_WIDTH * 2;

		int y = CORNER_OFFSET + i * ( gHUD.m_scrinfo.iCharHeight ) + i * 4;

		int r = 255;
		int g = 255;
		int b = 255;

		gHUD.DrawHudString( x, y, 0, mod.name.c_str(), r, g, b );

		int timeWidth = ( mod.timeLeft / mod.initialTime ) * ITEM_WIDTH;
		FillRGBA( x, y + gHUD.m_scrinfo.iCharHeight, timeWidth, 1, r, g, b, 120 );
	}

	int colAmount = min( 3, ( ScreenWidth - ITEM_WIDTH * 2 ) / ( ITEM_WIDTH + SPACING ) );
	for ( size_t i = 0; i < proposedMods.size(); i++ ) {
		auto &mod = proposedMods.at( i );

		int row = i / colAmount;
		int col = i % colAmount;

		int x = ScreenWidth - ITEM_WIDTH - col * ITEM_WIDTH - SPACING * col - ITEM_WIDTH * 2 - SPACING;

		int y = CORNER_OFFSET + ITEM_HEIGHT * row;

		int r = 255;
		int g = 255;
		int b = highlightIndex == i ? 0 : 255;
		
		std::string modName = "#" + std::to_string( proposedMods.size() - i ) + "  " + mod.name;
		auto modNameLines = Wrap( modName.c_str(), ITEM_WIDTH - 20, []( const std::string &str ) -> float {
			return gHUD.GetStringWidth( str.c_str() );
		} );

		for ( size_t j = 0 ; j < min( modNameLines.size(), 2 ) ; j++ ) {
			auto &modNameLine = modNameLines.at( j );
			gHUD.DrawHudString( x, y + j * gHUD.m_scrinfo.iCharHeight, 0, modNameLine.c_str(), r, g, b );
		}

		if ( ShouldDrawVotes() ) {
			std::string vote_noun = mod.votes == 1 ? "vote" : "votes";
			gHUD.DrawHudString( x, y + gHUD.m_scrinfo.iCharHeight * 2 + 4, 0, fmt::sprintf( "%d %s", mod.votes, vote_noun ).c_str(), r, g, b );
			gHUD.DrawHudStringKeepRight( x + ITEM_WIDTH, y + gHUD.m_scrinfo.iCharHeight * 2 + 4, 0, fmt::sprintf( "%.1f%%", mod.percent ).c_str(), r, g, b );
		}
	}

	if ( timeUntilNextHighlight && gEngfuncs.GetAbsoluteTime() > timeUntilNextHighlight ) {
		HighlightRandomProposedMod();
	}

	return 1;
}

bool CHudRandomGameplayMods::ShouldDrawVotes() {
	for ( auto &proposedMod : proposedMods ) {
		if ( proposedMod.votes > 0 ) {
			return true;
		}
	}

	return false;
}

void CHudRandomGameplayMods::HighlightRandomProposedMod() {
	timeUntilNextHighlight = gEngfuncs.GetAbsoluteTime() + 0.1f;
	
	int lastHighlightIndex = highlightIndex;
	while ( highlightIndex == lastHighlightIndex && proposedMods.size() > 1 ) {
		highlightIndex = UniformInt( 0, proposedMods.size() - 1 );
	}
}

int CHudRandomGameplayMods::MsgFunc_RandModLen( const char *pszName, int iSize, void *pbuf ) {
	BEGIN_READ( pbuf, iSize );

	size_t counterCount = READ_SHORT();
	if ( mods.size() != counterCount ) {
		mods.resize( counterCount );
	}

	return 1;
}

int CHudRandomGameplayMods::MsgFunc_RandModVal( const char *pszName, int iSize, void *pbuf ) {
	BEGIN_READ( pbuf, iSize );

	size_t index = READ_SHORT();
	mods[index] = { READ_FLOAT(), READ_FLOAT(), READ_STRING() };

	m_iFlags |= HUD_ACTIVE;

	return 1;
}



int CHudRandomGameplayMods::MsgFunc_PropModLen( const char *pszName, int iSize, void *pbuf ) {
	BEGIN_READ( pbuf, iSize );

	size_t counterCount = READ_SHORT();
	if ( proposedMods.size() != counterCount ) {
		proposedMods.resize( counterCount );
	}
	
	if ( counterCount <= 0 ) {
		highlightIndex = -1;
		timeUntilNextHighlight = 0.0f;
	}

	return 1;
}

int CHudRandomGameplayMods::MsgFunc_PropModVal( const char *pszName, int iSize, void *pbuf ) {
	BEGIN_READ( pbuf, iSize );

	size_t index = READ_SHORT();
	proposedMods[index] = { 0, 0.0f, READ_STRING() };

	m_iFlags |= HUD_ACTIVE;

	return 1;
}

int CHudRandomGameplayMods::MsgFunc_PropModVot( const char *pszName, int iSize, void *pbuf ) {
	BEGIN_READ( pbuf, iSize );

	size_t index = READ_SHORT();
	proposedMods[index].votes++;

	// TODO: Calculate vote percents

	m_iFlags |= HUD_ACTIVE;

	return 1;
}

int CHudRandomGameplayMods::MsgFunc_PropModAni( const char *pszName, int iSize, void *pbuf ) {
	BEGIN_READ( pbuf, iSize );
	
	if ( !timeUntilNextHighlight ) {
		HighlightRandomProposedMod();
	}

	m_iFlags |= HUD_ACTIVE;

	return 1;
}