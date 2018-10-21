#ifndef ARGUMENT_H
#define ARGUMENT_H

#include <string>
#include <functional>

struct Argument {
	std::string name;
	std::string default;
	std::string string;
	std::string description;
	float number = NAN;

	bool isRequired = true;
	bool isNumber = false;
	bool mustBeNumber = false;
	float min = NAN;
	float max = NAN;

	float randomMin = NAN;
	float randomMax = NAN;

	typedef std::function<std::string( const std::string &, float )> ArgumentDescriptionFunction;

	ArgumentDescriptionFunction GetDescription = []( const std::string &string, float number ) {
		return "";
	};

	Argument( const std::string &name ) : name( name ) {};

	Argument &IsOptional() {
		isRequired = false;
		return *this;
	}

	Argument &IsNumber() {
		isNumber = true;
		mustBeNumber = true;
		return *this;
	}

	Argument &CanBeNumber() {
		isNumber = true;
		return *this;
	}

	Argument &MinMax( float min, float max = NAN ) {
		this->isNumber = true;
		this->min = min;
		this->max = max;
		return *this;
	}
	
	Argument &RandomMinMax( float min, float max ) {
		this->randomMin = min;
		this->randomMax = max;
		return *this;
	}

	Argument &Description( const ArgumentDescriptionFunction &description ) {
		this->GetDescription = description;
		return *this;
	}

	Argument &Default( const std::string &default ) {
		this->default = default;
		Init( default );
		return *this;
	}

	std::string Init( const std::string &value );
};

#endif // ARGUMENT_H