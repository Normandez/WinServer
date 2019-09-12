#include "CLogger.h"
#include "SInputParams.h"

#include "CApplication.h"

static const char* s_main_menu = "CONTROL PANEL:\n\t0 - exit\n\t1 - print work threads num\n\t2 - print work threads\n\t3 - stop all work threads\n\n";
static const std::string s_log_file_name = "./main.log";

static CRITICAL_SECTION log_critical_sec;

int main( int argc, char** argv )
{
	CLogger logger(s_log_file_name);
	::InitializeCriticalSection(&log_critical_sec);

	try
	{
		// Input params parsing
		SInputParams input_params( argc, argv );
		if(!input_params.m_is_valid)
		{
			logger.MakeErrorLog( "Bad input params: " + input_params.m_error_reason );
		
			::DeleteCriticalSection(&log_critical_sec);
			std::getchar();
			return 1;
		}

		// CONTROL PANEL print
		::EnterCriticalSection(&log_critical_sec);
		std::cout << s_main_menu << std::endl;
		::LeaveCriticalSection(&log_critical_sec);

		CApplication app( input_params.m_threads_num, input_params.m_port, &logger, &log_critical_sec );
		app.Listen();
		
		short res = 255;
		while(std::cin)
		{
			std::cin >> res;

			if( res == 0 )
			{
				break;
			}
			else if( res == 1 )
			{
				std::string str_threads_num = std::to_string( app.GetWorkThreadsNum() );

				::EnterCriticalSection(&log_critical_sec);
				logger.MakeLog( "Work threads num = " + str_threads_num );
				::LeaveCriticalSection(&log_critical_sec);
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
				::EnterCriticalSection(&log_critical_sec);
				logger.MakeLog("Invalid choice. Try again.");
				::LeaveCriticalSection(&log_critical_sec);
			}
		}
	}
	catch( const std::runtime_error& rt_ex )
	{
		logger.MakeErrorLog( "runtime_error exception handled in MAIN: " + std::string( rt_ex.what() ) );

		::DeleteCriticalSection(&log_critical_sec);
		std::getchar();
		return 2;
	}
	catch( const std::logic_error& lg_ex )
	{
		logger.MakeErrorLog( "logic_error exception handled in MAIN: " + std::string( lg_ex.what() ) );

		::DeleteCriticalSection(&log_critical_sec);
		std::getchar();
		return 3;
	}
	catch( const std::exception& ex )
	{
		logger.MakeErrorLog( "Common exception handled in MAIN: " + std::string( ex.what() ) );

		::DeleteCriticalSection(&log_critical_sec);
		std::getchar();
		return 4;
	}
	catch(...)
	{
		logger.MakeErrorLog("Unknown exception handled in MAIN");

		::DeleteCriticalSection(&log_critical_sec);
		std::getchar();
		return 5;
	}

	::DeleteCriticalSection(&log_critical_sec);
	std::getchar();
	return 0;
}
