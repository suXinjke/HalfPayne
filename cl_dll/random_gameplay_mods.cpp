#include "hud.h"
#include "cl_util.h"
#include "../common/event_api.h"
#include "parsemsg.h"
#include "triangleapi.h"
#include "cpp_aux.h"
#include "../fmt/printf.h"
#include "gameplay_mod.h"

DECLARE_MESSAGE( m_RandomGameplayMods, PropModVin )

extern globalvars_t *gpGlobals;

using namespace gameplayMods;

int CHudRandomGameplayMods::Init( void )
{
	HOOK_MESSAGE( PropModVin );
	
	gHUD.AddHudElem( this );

	return 1;
}

void CHudRandomGameplayMods::Reset( void )
{
	m_iFlags |= HUD_ACTIVE;

	highlightIndex = -1;
	timeUntilNextHighlight = 0.0f;
}

int CHudRandomGameplayMods::Draw( float flTime )
{
	if ( ( gHUD.m_iHideHUDDisplay & HIDEHUD_ALL ) || gEngfuncs.IsSpectateOnly() ) {
	
		return 1;
	}

	auto randomGameplayMods = gameplayMods::randomGameplayMods.isActive<RandomGameplayModsInfo>();
	if ( !randomGameplayMods ) {
		return 1;
	}

	const int ITEM_WIDTH = 180;
	const int ITEM_HEIGHT = 40;
	const int SPACING = 10;

	for ( size_t i = 0; i < timedGameplayMods.size(); i++ ) {
		auto &timedMod = timedGameplayMods.at( i );
		
		if ( !timedMod.mod ) {
			continue;
		}

		if ( randomGameplayMods->timeForRandomGameplayMod < 10.0f && timedMod.mod != &gameplayMods::chaosEdition ) {
			continue;
		}

		int x = ScreenWidth - ITEM_WIDTH * 2;

		int y = CORNER_OFFSET + i * ( gHUD.m_scrinfo.iCharHeight ) + i * 4;

		int r = 255;
		int g = 255;
		int b = 255;

		gHUD.DrawHudString( x, y, 0, timedMod.mod->GetRandomGameplayModName().c_str(), r, g, b );

		int timeWidth = ( timedMod.time / timedMod.initialTime ) * ITEM_WIDTH;
		FillRGBA( x, y + gHUD.m_scrinfo.iCharHeight, timeWidth, 1, r, g, b, 120 );
	}

	if ( randomGameplayMods->timeForRandomGameplayMod < 10.0f || gameplayModsData.timeLeftUntilNextRandomGameplayMod > randomGameplayMods->timeForRandomGameplayModVoting ) {
		return 1;
	}

	int timeLeftXRight = ScreenWidth - ITEM_WIDTH * 2 - SPACING;
	int timeLeftXLeft = timeLeftXRight;

	int colAmount = min( 3, ( ScreenWidth - ITEM_WIDTH * 2 ) / ( ITEM_WIDTH + SPACING ) );
	for ( size_t i = 0; i < proposedGameplayModsClient.size(); i++ ) {
		auto &proposedMod = proposedGameplayModsClient.at( i );

		int row = i / colAmount;
		int col = i % colAmount;

		int x = ScreenWidth - ITEM_WIDTH - col * ITEM_WIDTH - SPACING * col - ITEM_WIDTH * 2 - SPACING;
		if ( x < timeLeftXLeft ) {
			timeLeftXLeft = x;
		}

		int y = CORNER_OFFSET + ITEM_HEIGHT * row;

		int r = 255;
		int g = 255;
		int b = highlightIndex == i ? 0 : 255;
		
		std::string modName = "#" + std::to_string( proposedGameplayModsClient.size() - i ) + "  " + proposedMod.mod->GetRandomGameplayModName();
		auto modNameLines = aux::str::wordWrap( modName.c_str(), ITEM_WIDTH - 20, []( const std::string &str ) -> float {
			return gHUD.GetStringWidth( str.c_str() );
		} );

		for ( size_t j = 0 ; j < min( modNameLines.size(), 2 ) ; j++ ) {
			auto &modNameLine = modNameLines.at( j );
			gHUD.DrawHudString( x, y + j * gHUD.m_scrinfo.iCharHeight, 0, modNameLine.c_str(), r, g, b );
		}

		if ( ShouldDrawVotes() ) {
			std::string vote_noun = proposedMod.votes == 1 ? "vote" : "votes";
			gHUD.DrawHudString( x, y + gHUD.m_scrinfo.iCharHeight * 2 + 4, 0, fmt::sprintf( "%d %s", proposedMod.votes, vote_noun ).c_str(), r, g, b );
			gHUD.DrawHudStringKeepRight( x + ITEM_WIDTH, y + gHUD.m_scrinfo.iCharHeight * 2 + 4, 0, fmt::sprintf( "%.1f%%", proposedMod.voteDistributionPercent ).c_str(), r, g, b );
		}

		for ( auto it = proposedMod.voters.begin(); it != proposedMod.voters.end(); ) {
			if ( it->alpha <= 0 ) {
				it = proposedMod.voters.erase( it );
			} else {
				int vote_offset = ( ( 255 - it->alpha ) / 255.0f ) * 30;
				int vote_r = 255;
				int vote_g = 255;
				int vote_b = 255;
				ScaleColors( vote_r, vote_g, vote_b, it->alpha );
				gHUD.DrawHudString( x, y + vote_offset + gHUD.m_scrinfo.iCharHeight * 3 + 4, 0, it->name.c_str(), vote_r, vote_g, vote_b );

				it->alpha -= 4;
				it++;
			}
		}
	}

	int timeLeftWidth = ( gameplayModsData.timeLeftUntilNextRandomGameplayMod / randomGameplayMods->timeForRandomGameplayModVoting ) * ( timeLeftXRight - timeLeftXLeft );
	FillRGBA( timeLeftXLeft, CORNER_OFFSET - 2, timeLeftWidth, 1, 255, 255, 255, 120 );

	if ( gameplayModsData.timeLeftUntilNextRandomGameplayMod >= 3.0f ) {
		timeUntilNextHighlight = 0.0f;
		highlightIndex = -1;
	} else {
		if ( !timeUntilNextHighlight ) {
			timeUntilNextHighlight = gEngfuncs.GetAbsoluteTime() + 0.1f;
		}
	}

	if ( timeUntilNextHighlight && gEngfuncs.GetAbsoluteTime() > timeUntilNextHighlight ) {
		HighlightRandomProposedMod();
	}

	return 1;
}

bool CHudRandomGameplayMods::ShouldDrawVotes() {
	for ( auto &proposedMod : proposedGameplayModsClient ) {
		if ( proposedMod.votes > 0 ) {
			return true;
		}
	}

	return false;
}

void CHudRandomGameplayMods::HighlightRandomProposedMod() {
	timeUntilNextHighlight = gEngfuncs.GetAbsoluteTime() + 0.1f;
	
	int lastHighlightIndex = highlightIndex;
	while ( highlightIndex == lastHighlightIndex && proposedGameplayModsClient.size() > 1 ) {
		highlightIndex = aux::rand::uniformInt<int>( 0, proposedGameplayModsClient.size() - 1 );
	}
}

int CHudRandomGameplayMods::MsgFunc_PropModVin( const char *pszName, int iSize, void *pbuf ) {
	BEGIN_READ( pbuf, iSize );

	size_t index = READ_SHORT();
	if ( index < proposedGameplayModsClient.size() ) {
		proposedGameplayModsClient.at( index ).voters.push_back( { 255, READ_STRING() } );
	}

	m_iFlags |= HUD_ACTIVE;

	return 1;
}