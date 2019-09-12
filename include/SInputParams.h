#ifndef SINPUTPARAMS_H
#define SINPUTPARAMS_H

#include <string>

namespace
{
	static const std::string s_usage_line = "WinServer.exe [--threads-num=<number_of_work_threads>] [--port=<listen_port>]";
	static const std::string s_threads_num_param_pattern = "--threads-num=";
	static const std::string s_port_param_pattern = "--port=";
}

struct SInputParams
{
	SInputParams( int argc, char** argv )
		: m_is_valid(false),
		  m_error_reason(""),
		  m_threads_num(0),
		  m_port("")
	{
		// Input params number check
		if( argc > 3 )
		{
			m_error_reason = "A lot of input params. Usage:\n\t" + s_usage_line;
			return;
		}

		// Input params parsing
		std::string param = "";
		for( int it = 1; it < argc; it++ )
		{
			param = argv[it];

			if( param.find(s_threads_num_param_pattern) == 0 )	// Threads num parsing
			{
				try
				{
					m_threads_num = std::stoi( param.substr( s_threads_num_param_pattern.size() ) );
				}
				catch( const std::invalid_argument& inv_arg_ex )
				{
					(void)inv_arg_ex;
					m_error_reason = "Invalid '" + s_threads_num_param_pattern + "' value: not a digit.";
					return;
				}
			}
			else if( param.find(s_port_param_pattern) == 0 )	// Port parsing
			{
				int parsed_port;
				try
				{
					parsed_port = std::stoi( param.substr( s_port_param_pattern.size() ) );
				}
				catch( const std::invalid_argument& inv_arg_ex )
				{
					(void)inv_arg_ex;
					m_error_reason = "Invalid '" + s_port_param_pattern + "' value: not a digit.";
					return;
				}

				if( parsed_port >= 0 && parsed_port <= 65535 )
				{
					m_port = std::to_string(parsed_port);
				}
				else
				{
					m_error_reason = "Invalid port value: port must be a number in range [0, 65535]";
					return;
				}
			}
			else
			{
				m_error_reason = "Unknown parameter: '" + param + "'. Usage:\n\t" + s_usage_line;
				return;
			}
		}

		m_is_valid = true;
	}

	bool m_is_valid;
	std::string m_error_reason;

	int m_threads_num;
	std::string m_port;
};

#endif //SINPUTPARAMS_H
