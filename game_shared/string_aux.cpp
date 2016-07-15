#include "string_aux.h"
#include <algorithm>

std::string Trim( const std::string& str, const std::string& whitespace )
{
	const auto strBegin = str.find_first_not_of( whitespace );
	if ( strBegin == std::string::npos ) {
		return "";    // no content
	}

	const auto strEnd = str.find_last_not_of( whitespace );
	const auto strRange = strEnd - strBegin + 1;

	return str.substr( strBegin, strRange );
}

std::vector<std::string> Split( const std::string &text, char sep )
{
	std::vector<std::string> tokens;
	std::size_t start = 0, end = 0;
	while ( ( end = text.find( sep, start ) ) != std::string::npos ) {
		std::string temp = text.substr( start, end - start );
		if ( temp != "" ) {
			tokens.push_back( temp );
		}
		start = end + 1;
	}
	std::string temp = text.substr( start );
	if ( temp != "" ) {
		tokens.push_back( temp );
	}
	return tokens;
}

std::string Uppercase( std::string text )
{
    std::transform(text.begin(), text.end(), text.begin(), ::toupper);

    return text;
}
