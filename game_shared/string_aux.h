#ifndef STRING_AUX_H
#define STRING_AUX_H

#include <string>
#include <vector>

std::string Trim( const std::string& str, const std::string& whitespace = " \t" );
std::vector<std::string> Split( const std::string &text, char sep );
std::string Uppercase( std::string text );

#endif // STRING_AUX_H
