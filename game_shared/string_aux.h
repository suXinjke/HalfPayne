#ifndef STRING_AUX_H
#define STRING_AUX_H

#include <string>
#include <vector>
#include <deque>
#include <functional>

std::string Trim( const std::string& str, const std::string& whitespace = " \t" );
std::vector<std::string> Split( const std::string &text, char sep, bool ignoreEmptyStrings = true );
std::deque<std::string> SplitDeque( const std::string &text, char sep );
std::string Uppercase( std::string text );
bool EndsWith( const std::string &fullString, const std::string &ending );
std::vector<std::string> NaiveCommandLineParse( const std::string &input );
std::vector<std::string> Wrap( const char *text, float line_length, std::function<float( const std::string & )> text_length_calculator );

#endif // STRING_AUX_H
