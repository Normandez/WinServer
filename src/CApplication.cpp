#include "CApplication.h"

#define DEFAULT_PORT "27015"
#define DEFAULT_BUFSIZE 8192

namespace
{
	static bool s_termination_flag = false;
}

CApplication::CApplication( int num_of_threads, const std::string& listen_port, CLogger* logger, CRITICAL_SECTION* p_log_critical_sec )
	: m_listen_sock(INVALID_SOCKET),
	  m_is_wsa_init(false),
	  m_is_listen_sock_init(false),
	  m_is_thread_pool_init(false)
{
	m_logger = logger;
	m_log_critical_sec = p_log_critical_sec;

	// Set work threads num
	if( num_of_threads == 0 )
	{
		SYSTEM_INFO sys_info;
		::GetSystemInfo(&sys_info);
		m_num_of_threads = (int)sys_info.dwNumberOfProcessors * 2 - 1;

		m_thread_pool.reserve(m_num_of_threads);
	}
	else
	{
		m_num_of_threads = num_of_threads;

		m_thread_pool.reserve(m_num_of_threads);
	}
	std::string port = ( listen_port.empty() ) ? DEFAULT_PORT : listen_port;

	// WinSock2 init
	int wsa_init_res = ::WSAStartup( MAKEWORD(2,0), &m_wsa_data );
	if(wsa_init_res)
	{
		m_logger->MakeErrorLog( "WSADATA init failed with code: " + std::to_string(wsa_init_res) );
		return;
	}
	else
	{
		m_is_wsa_init = true;
	}

	// Get address params for listened socket
	addrinfo* addrinfo_res = NULL;
	addrinfo addrinfo_params;
	::ZeroMemory( &addrinfo_params, sizeof(addrinfo_params) );
	addrinfo_params.ai_family = AF_INET;
	addrinfo_params.ai_socktype = SOCK_STREAM;
	addrinfo_params.ai_protocol = IPPROTO_TCP;
	addrinfo_params.ai_flags = AI_PASSIVE;
	int getaddrinfo_res = getaddrinfo( NULL, port.c_str(), &addrinfo_params, &addrinfo_res );
	if(getaddrinfo_res)
	{
		m_logger->MakeErrorLog( "ADDRINFO getting failed with code: " + std::to_string(getaddrinfo_res) );
		return;
	}

	// Init listened socket
	m_listen_sock = socket( addrinfo_res->ai_family, addrinfo_res->ai_socktype, addrinfo_res->ai_protocol );
	if( m_listen_sock == INVALID_SOCKET )
	{
		m_logger->MakeErrorLog( "Listen socket init failed with error: " + std::to_string( ::WSAGetLastError() ) );
		::freeaddrinfo(addrinfo_res);
		return;
	}
	else
	{
		m_is_listen_sock_init = true;
	}

	// Bind address to listened socket
	int bind_res = ::bind( m_listen_sock, addrinfo_res->ai_addr, (int)addrinfo_res->ai_addrlen );
	if( bind_res == SOCKET_ERROR )
	{
		m_logger->MakeErrorLog( "Listen socket binding failed with error: " + std::to_string( ::WSAGetLastError() ) );
		::freeaddrinfo(addrinfo_res);
		return;
	}
	::freeaddrinfo(addrinfo_res);
}

CApplication::~CApplication()
{
	if(!s_termination_flag) StopAllThreads();

	if(m_is_listen_sock_init) ::closesocket(m_listen_sock);
	if(m_is_wsa_init) ::WSACleanup();

	// Working thread pool cleanup
	if(m_is_thread_pool_init)
	{
		for( size_t count = 0; count < m_thread_pool.size(); count++ )
		{
			::CloseHandle( m_thread_pool.at(count).first );
		}
	}
}

void CApplication::Listen()
{
	if( !m_is_wsa_init || !m_is_listen_sock_init )
	{
		return;
	}

	// Start listening
	int listen_res = ::listen( m_listen_sock, SOMAXCONN );
	if( listen_res == SOCKET_ERROR )
	{
		m_logger->MakeErrorLog( "Listen socket listening failed with error: " + std::to_string( ::WSAGetLastError() ) );
		return;
	}

	// Compress thread args into struct
	m_thread_args.listen_sock = &m_listen_sock;
	m_thread_args.logger = m_logger;
	m_thread_args.log_critical_sec = m_log_critical_sec;

	// Init thread pool
	for( size_t count = 0; count < m_num_of_threads; count++ )
	{
		std::pair<HANDLE, DWORD> new_pair;
		new_pair.first = ::CreateThread( NULL, 0, &CApplication::ProceedResponse, &m_thread_args, 0, &new_pair.second );
		m_thread_pool.push_back(new_pair);
	}
	m_is_thread_pool_init = true;
}

void CApplication::StopAllThreads()
{
	// Set termination flag
	s_termination_flag = true;
	
	// Stop listening
	::closesocket(m_listen_sock);
	m_is_listen_sock_init = false;

	// Threads termination
	size_t threads_num = m_thread_pool.size();
	if(threads_num)
	{
		HANDLE* lp_threads_handles = new HANDLE[threads_num];
		for( auto it : m_thread_pool )
		{
			*lp_threads_handles++ = it.first;
		}
		lp_threads_handles -= threads_num;
		DWORD dw_wait_res = ::WaitForMultipleObjects( (DWORD)threads_num, lp_threads_handles, TRUE, 10000 );
		if( dw_wait_res == WAIT_TIMEOUT )
		{
			lp_threads_handles -= threads_num;
			for( size_t it = 0; it < threads_num; it++ )
			{
				::TerminateThread( *lp_threads_handles++, 2 );
			}
		}
		delete[] lp_threads_handles;
	}
}

size_t CApplication::GetWorkThreadsNum() const
{
	return s_termination_flag ? 0 : m_thread_pool.size();
}

void CApplication::PrintWorkThreads() const
{
	if( m_thread_pool.empty() || s_termination_flag )
	{
		::EnterCriticalSection(m_log_critical_sec);
		m_logger->MakeLog("ThreadPool is empty");
		::LeaveCriticalSection(m_log_critical_sec);

		return;
	}

	::EnterCriticalSection(m_log_critical_sec);
	for( size_t it = 0; it < m_thread_pool.size(); it++ )
	{
		std::string log_line = "WorkThread #" 
			+ std::to_string(it)
			+ ", ID = " 
			+ std::to_string( m_thread_pool.at(it).second );

		m_logger->MakeLog(log_line);
	}
	::LeaveCriticalSection(m_log_critical_sec);
}

DWORD WINAPI CApplication::ProceedResponse( LPVOID param )
{
	SThreadArgs* thread_args = (SThreadArgs*)param;

	char buf[DEFAULT_BUFSIZE];
	int buf_size = DEFAULT_BUFSIZE;
	SOCKET accept_sock = INVALID_SOCKET;
	CHttpParser http_parser;

	std::string str_wsa_last_error = "";
	const std::string str_current_thread_id = std::to_string( ::GetCurrentThreadId() );

	try
	{
		while(true)
		{
			// Thread termination condition
			if(s_termination_flag)
			{
				::shutdown( accept_sock, SD_SEND );
				::closesocket(accept_sock);

				break;
			}

			// Listen accepting
			accept_sock = ::accept( ( (SOCKET*)thread_args->listen_sock )[0], NULL, NULL );
			if( accept_sock == INVALID_SOCKET && !s_termination_flag )
			{
				str_wsa_last_error = std::to_string( ::WSAGetLastError() );

				::EnterCriticalSection(thread_args->log_critical_sec);
				thread_args->logger->MakeErrorLog( "WorkThread ID = " + str_current_thread_id + ": Invalid listen_sock: " + str_wsa_last_error );
				::LeaveCriticalSection(thread_args->log_critical_sec);
				
				::closesocket(accept_sock);
				return 1;
			}
			else if(s_termination_flag)
			{
				::shutdown( accept_sock, SD_SEND );
				::closesocket(accept_sock);

				break;
			}

			// Data receiving
			int recv_res = ::recv( accept_sock, buf, buf_size, 0 );
			if( recv_res == 0 )
			{
				::shutdown( accept_sock, SD_BOTH );
				continue;
			}
			else if( recv_res == SOCKET_ERROR )
			{
				str_wsa_last_error = std::to_string( ::WSAGetLastError() );

				::EnterCriticalSection(thread_args->log_critical_sec);
				thread_args->logger->MakeErrorLog( "WorkThread ID = " + str_current_thread_id + ": Receiving error: " + str_wsa_last_error );
				::LeaveCriticalSection(thread_args->log_critical_sec);

				::shutdown( accept_sock, SD_BOTH );

				continue;
			}

			// Data processing
			std::string response_data = "NOT_SET";
			{
				http_parser.LoadRequest(buf);

				if( http_parser.IsPostRequest() )
				{
					if( http_parser.IsJsonContentType() )
					{
						std::regex data_regex("\"data\": ?\"(.*)\"");
						std::smatch data_match;
						if( std::regex_search( http_parser.ReadRequestLines().back(), data_match, data_regex ) )
						{
							std::string data = data_match[1].str();
							std::reverse( data.begin(), data.end() );

							response_data = http_parser.ConstructResponse( "\"{\\\"data\\\":\\\"" + data + "\\\"}\"", true );
						}
						else
						{
							response_data = http_parser.ConstructResponse( "\"{\\\"error\\\":\\\"'data' field not found\\\"}\"", false );
						}
					}
					else
					{
						response_data = http_parser.ConstructResponse( "\"{\\\"error\\\":\\\"Not application/json content type\\\"}\"", false );
					}
				}
				else
				{
					response_data = http_parser.ConstructResponse( "\"{\\\"error\\\":\\\"Not POST request\\\"}\"", false );
				}
			}

			// Data sending
			int send_res = ::send( accept_sock, response_data.c_str(), (int)response_data.size(), 0 );
			if( send_res == SOCKET_ERROR )
			{
				str_wsa_last_error = std::to_string( ::WSAGetLastError() );

				::EnterCriticalSection(thread_args->log_critical_sec);
				thread_args->logger->MakeErrorLog( "WorkThread ID = " + str_current_thread_id + ": Sending error: " + str_wsa_last_error );
				::LeaveCriticalSection(thread_args->log_critical_sec);

				::shutdown( accept_sock, SD_BOTH );

				continue;
			}

			// Disconnect
			int shutdown_res = ::shutdown( accept_sock, SD_SEND );
			if( shutdown_res == SOCKET_ERROR )
			{
				str_wsa_last_error = std::to_string( ::WSAGetLastError() );

				::EnterCriticalSection(thread_args->log_critical_sec);
				thread_args->logger->MakeErrorLog( "WorkThread ID = " + str_current_thread_id + ": Disconnect error: " + str_wsa_last_error );
				::LeaveCriticalSection(thread_args->log_critical_sec);

				continue;
			}
		}
	}
	catch( const std::runtime_error& rt_ex )
	{
		std::string log_line = "runtime_error exception handled in WorkThread with ID = " + str_current_thread_id + " :" + rt_ex.what();

		::EnterCriticalSection(thread_args->log_critical_sec);
		thread_args->logger->MakeErrorLog(log_line);
		::LeaveCriticalSection(thread_args->log_critical_sec);
		
		::shutdown( accept_sock, SD_SEND );
		::closesocket(accept_sock);

		return 1;
	}
	catch( const std::logic_error& lg_ex )
	{
		std::string log_line = "logic_error exception handled in WorkThread with ID = " + str_current_thread_id + " :" + lg_ex.what();

		::EnterCriticalSection(thread_args->log_critical_sec);
		thread_args->logger->MakeErrorLog(log_line);
		::LeaveCriticalSection(thread_args->log_critical_sec);

		::shutdown( accept_sock, SD_SEND );
		::closesocket(accept_sock);

		return 2;
	}
	catch( const std::exception& ex )
	{
		std::string log_line = "Common exception handled in WorkThread with ID = " + str_current_thread_id + " :" + ex.what();

		::EnterCriticalSection(thread_args->log_critical_sec);
		thread_args->logger->MakeErrorLog(log_line);
		::LeaveCriticalSection(thread_args->log_critical_sec);

		::shutdown( accept_sock, SD_SEND );
		::closesocket(accept_sock);

		return 3;
	}
	catch(...)
	{
		std::string log_line = "Unknown exception handled in WorkThread with ID = " + str_current_thread_id;

		::EnterCriticalSection(thread_args->log_critical_sec);
		thread_args->logger->MakeErrorLog(log_line);
		::LeaveCriticalSection(thread_args->log_critical_sec);
		
		::shutdown( accept_sock, SD_SEND );
		::closesocket(accept_sock);

		return 4;
	}
	
	return 0;
}
