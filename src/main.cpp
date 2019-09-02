#include "CApplication.h"

int main( int argc, char** argv )
{
	try
	{
		CApplication app;
		app.Listen();

		short res = 255;
		std::cout << "Enter '0' to exit..." << std::endl;
		while(std::cin)
		{
			std::cin >> res;
			if(!res) break;
		}
	}
	catch( const std::exception& rt_ex )
	{
		std::cout << "Common exception handled: " << rt_ex.what() << std::endl;

		std::getchar();
		return 1;
	}

	std::getchar();
	return 0;
}
