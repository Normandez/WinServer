#include "CApplication.h"

int main( int argc, char** argv )
{
	try
	{
		CApplication app;
		int res = 1;
		while(std::cin)
		{
			std::cin >> res;
			if(!res) break;
		}

	}
	catch( const std::runtime_error& rt_ex )
	{
		std::cout << rt_ex.what() << std::endl;
	}

	return 0;
}
