#include "CLogger.h"
#include "SInputParams.h"

#include "CApplication.h"

static const char* s_main_menu = "CONTROL PANEL:\n\t0 - exit\n\t1 - print work threads num\n\t2 - print work threads\n\t3 - stop all work threads\n\n";
static const std::string s_log_file_name = "./main.log";

int main( int argc, char** argv )
{
	CLogger logger(s_log_file_name);

	try
	{
		// Input params parsing
		SInputParams input_params( argc, argv );
		if(!input_params.m_is_valid)
		{
			logger.MakeErrorLog( "Bad input params: " + input_params.m_error_reason );
		
			std::getchar();
			return 1;
		}

		CApplication app( input_params.m_threads_num, input_params.m_port, &logger );
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
				logger.MakeLog( "Work threads num = " + std::to_string( app.GetWorkThreadsNum() ) );
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
				logger.MakeLog("Invalid choice. Try again.");
			}
		}
	}
	catch( const std::runtime_error& rt_ex )
	{
		logger.MakeErrorLog( "runtime_error exception handled in MAIN: " + std::string( rt_ex.what() ) );

		std::getchar();
		return 2;
	}
	catch( const std::logic_error& lg_ex )
	{
		logger.MakeErrorLog( "logic_error exception handled in MAIN: " + std::string( lg_ex.what() ) );

		std::getchar();
		return 3;
	}
	catch( const std::exception& ex )
	{
		logger.MakeErrorLog( "Common exception handled in MAIN: " + std::string( ex.what() ) );

		std::getchar();
		return 4;
	}
	catch(...)
	{
		logger.MakeErrorLog("Unknown exception handled in MAIN");

		std::getchar();
		return 5;
	}

	std::getchar();
	return 0;
}
