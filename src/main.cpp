#include "CApplication.h"

static const char* s_main_menu = "CONTROL PANEL:\n\t0 - exit\n\t1 - print work threads num\n\t2 - print work threads\n\t3 - stop all work threads\n\n";

int main( int argc, char** argv )
{
	try
	{
		CApplication app;
		app.Listen();
		
		short res = 255;
		std::cout << s_main_menu << std::endl;
		while(std::cin)
		{
			std::cin >> res;

			if( res == 0 )
			{
				break;
			}
			else if( res == 1 )
			{
				std::cout << "Work threads num = " << app.GetWorkThreadNum() << std::endl;
			}
			else if( res == 2 )
			{
				app.PrintWorkThreads();
			}
			else if( res == 3 )
			{
				app.StopAllThreads();
			}
			else
			{
				std::cout << "Invalid choice. Try again." << std::endl;
			}
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
