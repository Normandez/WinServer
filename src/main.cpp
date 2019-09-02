#include "CApplication.h"

int main( int argc, char** argv )
{
	try
	{
		CApplication app;
		int res = 255;
		while(std::cin)
		{
			std::cin >> res;
			if(!res) break;
			if( res == 1 ) app.Listen();
		}

	}
	catch( const std::runtime_error& rt_ex )
	{
		std::cout << rt_ex.what() << std::endl;
	}

	return 0;
}
