#include "util_aux.h"
#include <fstream>
#include <string>

std::random_device rd;
std::mt19937 gen( rd() );

int UniformInt( int min, int max ) {
	std::uniform_int_distribution<> dis( min, max );
	return dis( gen );
}

float UniformFloat( float min, float max ) {
	std::uniform_real_distribution<float> dis( min, max );
	return dis( gen );
}

int IndexFromDiscreteDistribution( const std::vector<double> &values ) {
	std::discrete_distribution<> dis( values.begin(), values.end() );
	return dis( gen );
}

void SaveTwitchCredentialsToFile( const std::string &user, const std::string &password ) {
	std::ofstream out( "./half_payne/twitch_credentials.cfg" );
	out << user << "\n" << password;
}

std::pair<std::string, std::string> ReadTwitchCredentialsFromFile() {
	std::string user;
	std::string password;

	std::ifstream inp( "./half_payne/twitch_credentials.cfg" );
	if ( !inp.is_open() ) {
		return { user, password };
	}

	std::getline( inp, user );
	std::getline( inp, password );

	return { user, password };
}