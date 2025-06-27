#include "globals.hpp"

inline std::string make_step( double k, double c, int a = -100 )
{
	// k / ( 1 + exp( -a * ( x + c ) ) )
	std::string expression = "";
	expression += std::to_string( k );
	expression += "/(1+exp(";
	expression += std::to_string( a );
	expression += "*(x+";
	expression += std::to_string( c );
	expression += ")))";
	return expression;

}