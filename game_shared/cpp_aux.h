#pragma once

#include <string>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <vector>
#include <regex>
#include <type_traits>
#include <valarray>
#include <set>
#include <map>
#include <functional>
#include <random>
#include <fstream>
#include "fs_aux.h"

namespace aux {

	namespace str {
		inline void ltrim( std::string *str ) {
			str->erase( str->begin(), std::find_if( str->begin(), str->end(), []( int ch ) {
				return !std::isspace( ch );
			} ) );
		}
		inline std::string ltrim( std::string str ) {
			ltrim( &str );
			return str;
		}
		inline void rtrim( std::string *str ) {
			str->erase( std::find_if( str->rbegin(), str->rend(), []( int ch ) {
				return !std::isspace( ch );
			} ).base(), str->end() );
		}
		inline std::string rtrim( std::string str ) {
			rtrim( &str );
			return str;
		}
		inline void trim( std::string *str ) {
			ltrim( str );
			rtrim( str );
		}
		inline std::string trim( const std::string &str ) {
			return rtrim( ltrim( str ) );
		}

		inline void toUppercase( std::string *str ) {
			std::transform( str->begin(), str->end(), str->begin(), ::toupper );
		}
		inline std::string toUppercase( std::string str ) {
			toUppercase( &str );
			return str;
		}
		inline void toLowercase( std::string *str ) {
			std::transform( str->begin(), str->end(), str->begin(), ::tolower );
		}
		inline std::string toLowercase( std::string str ) {
			toLowercase( &str );
			return str;
		}

		inline std::string replace( const std::string &str, const std::regex &re, const std::string &newSubstring ) {
			return std::regex_replace( str, re, newSubstring );
		}
		inline std::string replace( const std::string &str, const std::string &re, const std::string &newSubstring ) {
			return replace( str, std::regex( re ), newSubstring );
		}

		template<template <typename...> class Container = std::vector>
		Container<std::string> split( const std::string &s ) {
			if ( s.empty() ) {
				return { "" };
			}

			Container<std::string> strings;
			for ( auto &chr : s ) {
				strings.insert( std::end( strings ), std::string( 1, chr ) );
			}
			return strings;
		}

		template<template <typename...> class Container = std::vector>
		Container<std::string> split( const std::string &s, char delimiter ) {
			Container<std::string> strings;
			std::string token;
			std::istringstream tokenStream( s );
			while ( std::getline( tokenStream, token, delimiter ) ) {
				strings.insert( std::end( strings ), token );
			}
			return strings;
		}

		template<template <typename...> class Container = std::vector>
		Container<std::string> split( const std::string &s, const std::regex &regex ) {
			Container<std::string> strings;

			std::copy(
				std::sregex_token_iterator( s.begin(), s.end(), regex, -1 ),
				std::sregex_token_iterator(),
				std::inserter( strings, std::end( strings ) )
			);

			return strings;
		}

		template<template <typename...> class Container = std::vector>
		Container<std::string> split( const std::string &s, const std::string &regex ) {
			return split<Container>( s, std::regex( regex ) );
		}

		inline bool startsWith( const std::string &str, const std::string &containedStr ) {
			return str.size() >= containedStr.size() && 0 == str.compare( 0, containedStr.size(), containedStr );
		}

		inline bool endsWith( const std::string &str, const std::string &containedStr ) {
			return str.size() >= containedStr.size() && 0 == str.compare( str.size() - containedStr.size(), containedStr.size(), containedStr );
		}

		inline bool includes( const std::string &str, const std::string &containedStr ) {
			return str.find( containedStr ) != std::string::npos;
		}

		inline std::vector<std::string> getCommandLineArguments( const std::string &input ) {
			std::vector<std::string> args;

			std::string arg;

			bool quotes = false;
			bool writingWord = false;

			for ( const auto &letter : input ) {
				switch ( letter ) {
				case '"': {
					if ( writingWord ) {
						if ( quotes ) {
							quotes = false;
							args.push_back( arg );
							arg = "";
							writingWord = false;
							quotes = false;
						} else {
							writingWord = true;
							quotes = true;
						}
					} else {
						quotes = true;
					}

					break;
				}

				case ' ': {
					if ( quotes ) {
						arg += letter;
					} else if ( writingWord ) {
						args.push_back( arg );
						arg.clear();

						writingWord = false;
					}

					break;
				}

				default: {
					writingWord = true;
					arg += letter;

					break;
				}
				}
			}

			if ( !arg.empty() ) {
				args.push_back( arg );
			}

			return args;
		}

		// Based on https://www.rosettacode.org/wiki/Word_wrap#C.2B.2B
		inline std::vector<std::string> wordWrap( const char *text, float lineLength, std::function<float( const std::string & )> textLengthCalculator ) {
			std::istringstream words( text );
			std::ostringstream wrapped;
			std::string word;

			std::vector<std::string> result;

			if ( words >> word ) {
				wrapped << word;
				float space_left = lineLength - textLengthCalculator( word );

				while ( words >> word ) {
					float wordLength = textLengthCalculator( word );
					if ( space_left < wordLength + 1 ) {
						result.push_back( wrapped.str() );
						wrapped = std::ostringstream(); // to reset 'wrapped' stream
						wrapped << word;
						space_left = lineLength - wordLength;
					} else {
						wrapped << ' ' << word;
						space_left -= wordLength + 1;
					}
				}

				result.push_back( wrapped.str() );
			}

			return result;
		}
	}

	namespace ctr {
		template<typename Container, typename UnaryOperation>
		inline void map( Container *cont, const UnaryOperation &func ) {
			std::transform( cont->begin(), cont->end(), cont->begin(), func );
		}

		template<typename OutputContainer, typename Container, typename UnaryOperation>
		inline OutputContainer map( Container &cont, const UnaryOperation &func ) {
			OutputContainer result;
			for ( const auto &val : cont ) {
				result.insert( std::end( result ), func( val ) );
			}

			return result;
		}

		template<typename Container, typename Value>
		inline bool includes( const Container &cont, const Value &value ) {
			return std::find( cont.begin(), cont.end(), value ) != cont.end();
		}

		template<typename Container, typename UnaryPredicate>
		inline void filter( Container *cont, const UnaryPredicate &func ) {
			for ( auto it = cont->begin(); it != cont->end(); ) {
				if ( !func( *it ) ) {
					it = cont->erase( it );
				} else {
					it++;
				}
			}
		}
		template<typename Container, typename UnaryPredicate>
		inline Container filter( Container cont, const UnaryPredicate &func ) {
			filter( &cont, func );
			return cont;
		}
	}

	namespace rand {
		static std::random_device rd;
		static std::mt19937 gen( rd() );

		template<typename T = int>
		T uniformInt( T min, T max ) {
			std::uniform_int_distribution<T> dis( min, max );
			return dis( gen );
		}

		inline float uniformFloat( float min, float max ) {
			std::uniform_real_distribution<float> dis( min, max );
			return dis( gen );
		}

		inline double uniformDouble( double min, double max ) {
			std::uniform_real_distribution<double> dis( min, max );
			return dis( gen );
		}

		template<typename Iter>
		Iter choice( Iter start, Iter end ) {
			std::uniform_int_distribution<> dis( 0, std::distance( start, end ) - 1 );
			std::advance( start, dis( gen ) );
			return start;
		}

		template<template <typename...> class C = std::vector, typename E>
		const E& choice( const C<E> &container ) {
			if ( container.empty() ) {
				throw std::invalid_argument( "container contents are empty" );
			}

			size_t size = container.size();
			auto it = container.begin();
			if ( size == 1 ) {
				return *it;
			}

			int random_index = uniformInt<size_t>( 0, size - 1 );
			std::advance( it, random_index );

			return *it;
		}

		inline int discreteIndex( const std::vector<double> &values ) {
			std::discrete_distribution<> dis( values.begin(), values.end() );
			return dis( gen );
		}

		template<typename T>
		inline T discreteChoice( const std::map<T, double> &choices ) {
			std::discrete_distribution<> dis( values.begin(), values.end() );
			return dis( gen );
		}
	}

	namespace twitch {
		inline void saveCredentialsToFile( const std::string &user, const std::string &password ) {
			std::ofstream out( FS_ResolveModPath( "twitch_credentials.cfg" ) );
			out << user << "\n" << password;
		}

		inline std::pair<std::string, std::string> readCredentialsFromFile() {
			std::string user;
			std::string password;

			std::ifstream inp( FS_ResolveModPath( "twitch_credentials.cfg" ) );
			if ( !inp.is_open() ) {
				return { user, password };
			}

			std::getline( inp, user );
			std::getline( inp, password );

			return { aux::str::toLowercase( user ), password };
		}
	}
}