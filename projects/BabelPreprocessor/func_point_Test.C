//#include "optionDeclaration.h"

class OptionDeclaration
{
	public:
		// Constructors designed to trigger auto-promotion of constructor arguments
		OptionDeclaration( char* s, int value );
		OptionDeclaration( char* s, double value );
		OptionDeclaration( char* s, char* value );
};

OptionDeclaration option1 ("VERSION",1.0);
OptionDeclaration option2 ("PACKAGE","BABEL_Preprocessor_4");


class A
   {
     public:
          int x;

		 void (*test1)(int a);
   };

