#ifndef UTIL_AUX_H
#define UTIL_AUX_H

#include <vector>
#include <set>
#include <map>
#include <random>

extern std::random_device rd;
extern std::mt19937 gen;

int UniformInt( int min, int max );
float UniformFloat( float min, float max );
int IndexFromDiscreteDistribution( const std::vector<double> &values );

template<typename Iter>
Iter RandomFromContainer( Iter start, Iter end ) {
	std::uniform_int_distribution<> dis( 0, std::distance( start, end ) - 1 );
	std::advance( start, dis( gen ) );
	return start;
}

template <typename T>
const T& RandomFromVector( const std::vector<T> &vec ) {
	return *RandomFromContainer( vec.begin(), vec.end() );
}

template <typename T>
const T& RandomFromSet( const std::set<T> &set ) {
	return *RandomFromContainer( set.begin(), set.end() );
}

template <typename K, typename V>
const K& RandomKeyFromMap( const std::map<K, V> &map ) {
	return RandomFromContainer( map.begin(), map.end() )->first;
}

template <typename K, typename V>
const V& RandomValueFromMap( const std::map<K, V> &map ) {
	return RandomFromContainer( map.begin(), map.end() )->second;
}

void SaveTwitchCredentialsToFile( const std::string &user, const std::string &password );
std::pair<std::string, std::string> ReadTwitchCredentialsFromFile();

#endif // UTIL_AUX_H
