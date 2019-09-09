#include "SInputParams.h"
#include "CApplication.h"

static const char* s_main_menu = "CONTROL PANEL:\n\t0 - exit\n\t1 - print work threads num\n\t2 - print work threads\n\t3 - stop all work threads\n\n";

int main( int argc, char** argv )
{
	try
	{
		// Input params parsing
		SInputParams input_params( argc, argv );
		if(!input_params.m_is_valid)
		{
			std::cout << "Bad input params.\n" << input_params.m_error_reason.c_str() << std::endl;
		
			std::getchar();
			return 1;
		}

		CApplication app( input_params.m_threads_num, input_params.m_port );
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
				std::cout << "Work threads num = " << app.GetWorkThreadsNum() << std::endl;
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
	catch( const std::runtime_error& rt_ex )
	{
		std::cout << "runtime_error exception handled in MAIN: " << rt_ex.what() << std::endl;

		std::getchar();
		return 2;
	}
	catch( const std::logic_error& lg_ex )
	{
		std::cout << "logic_error exception handled in MAIN: " << lg_ex.what() << std::endl;

		std::getchar();
		return 3;
	}
	catch( const std::exception& ex )
	{
		std::cout << "Common exception handled in MAIN: " << ex.what() << std::endl;

		std::getchar();
		return 4;
	}
	catch(...)
	{
		std::cout << "Unknown exception handled in MAIN" << std::endl;

		std::getchar();
		return 5;
	}

	std::getchar();
	return 0;
}
