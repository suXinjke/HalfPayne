#include "wrect.h"
#include "cl_dll.h"
#include "parsemsg.h"

#include "subtitles.h"
#include "string_aux.h"
#include <map>
#include <fstream>
#include <sstream>

extern SDL_Window *window;

std::map<std::string, SubtitleOutput> subtitlesToDraw;
std::map<std::string, SubtitleColor> colors;
std::map<std::string, std::vector<Subtitle>> subtitleMap;

void Subtitles_Init() {
	gEngfuncs.pfnHookUserMsg( "OnSound", Subtitles_OnSound );
	gEngfuncs.pfnHookUserMsg( "SubtClear", Subtitles_SubtClear );

	std::ifstream inp( "half_payne/resource/subtitles_en.txt" );
	if ( !inp.is_open( ) ) {
		gEngfuncs.Con_DPrintf( "SUBTITLE PARSER ERROR: failed to read resources/subtitles_en.txt file\n" );
		return;
	}

	std::string line;
	int lineCount = 0;
	while ( std::getline( inp, line ) ) {
		lineCount++;

		line = Trim( line, " " );
		if ( line.size() == 0 ) {
			continue;
		}

		if ( line.find( "SUBTITLE" ) != 0 && line.find( "COLOR" ) != 0 ) {
			continue;
		}

		std::vector<std::string> parts = Split( line, '|' );

		if ( parts.at( 0 ) == "SUBTITLE" ) {
			if ( parts.size() < 6 ) {
				gEngfuncs.Con_DPrintf( "SUBTITLE PARSER ERROR ON Line %d: insufficient subtitle data\n", lineCount );
				continue;
			}
			std::string subtitleKey = Uppercase( parts.at( 1 ) );
			std::string colorKey = parts.at( 2 );
			std::string text = parts.at( 5 );
			float delay, duration;
			try {
				delay = std::stof( parts.at( 3 ) );
				duration = std::stof( parts.at( 4 ) );

				subtitleMap[subtitleKey].push_back( {
					delay,
					duration,
					colorKey,
					text
				} );
			} catch ( std::invalid_argument e ) {
				gEngfuncs.Con_DPrintf( "SUBTITLE PARSER ERROR ON Line %d: delay or duration is not a number\n", lineCount );
			}

		} else if ( parts.at( 0 ) == "COLOR" ) {
			if ( parts.size() < 5 ) {
				gEngfuncs.Con_DPrintf( "SUBTITLE PARSER ERROR ON Line %d: insufficient color data\n", lineCount );
				continue;
			}
			std::string colorKey = Uppercase( parts.at( 1 ) );
			float r, g, b;
			try {
				r = std::stof( parts.at( 2 ) );
				g = std::stof( parts.at( 3 ) );
				b = std::stof( parts.at( 4 ) );

				colors[colorKey] = { r, g, b };
			} catch ( std::invalid_argument e ) {
				gEngfuncs.Con_DPrintf( "SUBTITLE PARSER ERROR ON Line %d: some of color data is not a number\n", lineCount );
			}

		}
	}

}

// Based on https://www.rosettacode.org/wiki/Word_wrap#C.2B.2B
std::vector<std::string> Wrap( const char *text, size_t line_length = 72 )
{
	std::istringstream words( text );
	std::ostringstream wrapped;
	std::string word;

	std::vector<std::string> result;

	if ( words >> word ) {
		wrapped << word;
		size_t space_left = line_length - ImGui::CalcTextSize( word.c_str() ).x;
		while ( words >> word ) {
			float wordLength = ImGui::CalcTextSize( word.c_str() ).x;
			if ( space_left < wordLength + 1 ) {
				result.push_back( wrapped.str() );
				wrapped = std::ostringstream(); // to reset 'wrapped' stream
				wrapped << word;
				space_left = line_length - wordLength;
			} else {
				wrapped << ' ' << word;
				space_left -= wordLength + 1;
			}
		}
		
		result.push_back( wrapped.str() );
	}

	return result;
}

bool Subtitle_IsFarAwayFromPlayer( const SubtitleOutput &subtitle ) {
	if ( subtitle.ignoreLongDistances ) {
		return false;
	}

	Vector playerOrigin = gEngfuncs.GetLocalPlayer()->origin;
	float distance = ( subtitle.pos - playerOrigin ).Length();

	return distance >= 512;
}

void Subtitles_Draw() {

	if ( subtitlesToDraw.size() == 0 ) {
		return;
	}

	int ScreenWidth, ScreenHeight;
	SDL_GetWindowSize( window, &ScreenWidth, &ScreenHeight );

	float FrameWidth = 0;
	float FrameHeight = 0;

	bool willDrawAtLeastOneSubtitle = false;

	float time = gEngfuncs.GetClientTime();

	auto i = subtitlesToDraw.begin();
	while ( i != subtitlesToDraw.end() ) {
		auto subtitle = i->second;
		
		if ( time >= subtitle.duration ) {
			i = subtitlesToDraw.erase( i );
			continue;
		}

		if ( time < subtitle.delay ) {
			i++;
			continue;
		}
		
		if ( Subtitle_IsFarAwayFromPlayer( subtitle ) ) {
			i++;
			continue;
		}

		ImVec2 textSize = ImGui::CalcTextSize( subtitle.text.c_str() );
		if ( textSize.x > FrameWidth ) {
			FrameWidth = textSize.x;
		}
		FrameHeight += textSize.y;

		willDrawAtLeastOneSubtitle = true;
		i++;
	}

	if ( !willDrawAtLeastOneSubtitle ) {
		return;
	}

	ImGui::SetNextWindowPos( ImVec2( ScreenWidth / 2.0f - FrameWidth / 2.0f, ScreenHeight / 1.3f ), 0 );
	ImGui::SetNextWindowSize( ImVec2( FrameWidth + 14, FrameHeight + 18 ) );

	ImGui::Begin( "Subtitles", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse );
	for ( auto pair : subtitlesToDraw ) {
		auto subtitle = pair.second;
		if ( time < subtitle.delay ) {
			continue;
		}

		if ( Subtitle_IsFarAwayFromPlayer( subtitle ) ) {
			continue;
		}

		ImVec2 textSize = ImGui::CalcTextSize( subtitle.text.c_str() );
		ImGui::SetCursorPosX( ImGui::GetWindowWidth() / 2.0f - textSize.x / 2.0f );
		ImGui::TextColored( ImVec4( subtitle.color[0], subtitle.color[1], subtitle.color[2], 1.0 ), "%s", subtitle.text.c_str() );
	}
	ImGui::End();
}

void Subtitles_Push( const std::string &key, const std::string &text, float duration, const Vector &color, const Vector &pos, float delay, bool ignoreLongDistances ) {
	if ( subtitlesToDraw.count( key ) ) {
		return;
	}
	
	int ScreenWidth;
	SDL_GetWindowSize( window, &ScreenWidth, NULL );

	std::vector<std::string> lines = Wrap( text.c_str(), ScreenWidth / 2 );

	for ( size_t i = 0 ; i < lines.size() ; i++ ) {
		std::string actualKey = key + std::to_string( i );

		float startTime = gEngfuncs.GetClientTime() + delay;

		subtitlesToDraw[actualKey] = {
			startTime,
			startTime + duration,
			ignoreLongDistances,
			color,
			pos,
			lines.at( i )
		};
	}
}

const std::vector<Subtitle> Subtitles_GetByKey( const std::string &key ) {
	std::string actualKey = Uppercase( key );
	std::vector<Subtitle> result;

	return subtitleMap.count( actualKey ) ?
		subtitleMap[actualKey] :
		std::vector<Subtitle>();
}

const SubtitleColor Subtitles_GetSubtitleColorByKey( const std::string &key ) {
	SubtitleColor defaultColor = {
		1.0,
		1.0,
		1.0
	};
	return colors.count( key ) ? colors[key] : defaultColor;
}

void Subtitles_Push( const std::string &key, bool ignoreLongDistances, const Vector &pos ) {

	int print_subtitles_cvar = gEngfuncs.pfnGetCvarFloat( "subtitles" );
	if ( print_subtitles_cvar <= 0 ) {
		return;
	}

	auto subtitles = Subtitles_GetByKey( key );

	for ( size_t i = 0 ; i < subtitles.size() ; i++ ) {
		auto subtitle = subtitles.at( i );
		auto color = Subtitles_GetSubtitleColorByKey( subtitle.colorKey );

		if ( subtitle.colorKey != "PAYNE" && print_subtitles_cvar < 2 ) {
			continue;
		}

		Subtitles_Push(
			key + std::to_string( i ),
			subtitle.text,
			subtitle.duration,
			Vector( color.r, color.g, color.b ),
			pos,
			subtitle.delay,
			ignoreLongDistances
		);
	}
}

int Subtitles_OnSound( const char *pszName,  int iSize, void *pbuf ) {
	BEGIN_READ( pbuf, iSize );

	std::string key = READ_STRING();
	bool ignoreLongDistances = READ_BYTE();
	float x = READ_COORD();
	float y = READ_COORD();
	float z = READ_COORD();

	// Dumb exception for Grunt cutscene, because player is actually far away from sound event
	if ( key.find( "!HG_DRAG" ) == 0 ) {
		ignoreLongDistances = true;
	}

	Subtitles_Push( key, ignoreLongDistances, Vector( x, y, z ) );

	return 1;
}

int Subtitles_SubtClear( const char *pszName,  int iSize, void *pbuf ) {
	BEGIN_READ( pbuf, iSize );
	subtitlesToDraw.clear();

	return 1;
}