#include "util_aux.h"

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