#include "CApplication.h"

#define DEFAULT_PORT "27015"
#define DEFAULT_BUFSIZE 8192

namespace
{
	static bool s_termination_flag = false;
}

CApplication::CApplication( int num_of_threads, const std::string& listen_port, CLogger* logger )
	: m_listen_sock(INVALID_SOCKET),
	  m_is_wsa_init(false),
	  m_is_listen_sock_init(false),
	  m_is_thread_pool_init(false)
{
	m_logger = logger;

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
		std::cout << "WSADATA init failed with code: " << wsa_init_res << std::endl;
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
		std::cout << "ADDRINFO getting failed with code: " << getaddrinfo_res << std::endl;
		return;
	}

	// Init listened socket
	m_listen_sock = socket( addrinfo_res->ai_family, addrinfo_res->ai_socktype, addrinfo_res->ai_protocol );
	if( m_listen_sock == INVALID_SOCKET )
	{
		std::cout << "Listen socket init failed with error: " << ::WSAGetLastError() << std::endl;
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
		std::cout << "Listen socket binding failed with error: " << ::WSAGetLastError() << std::endl;
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
		std::cout << "Listen socket listening failed with error: " << WSAGetLastError() << std::endl;
		return;
	}

	// Init thread pool
	for( size_t count = 0; count < m_num_of_threads; count++ )
	{
		std::pair<HANDLE, DWORD> new_pair;
		new_pair.first = ::CreateThread( NULL, 0, &CApplication::ProceedResponse, &m_listen_sock, 0, &new_pair.second );
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
		std::cout << "ThreadPool is empty" << std::endl;
		return;
	}
	for( size_t it = 0; it < m_thread_pool.size(); it++ )
	{
		std::cout << "WorkThread #" << it << ", HANDLE = " << m_thread_pool.at(it).first << ", ID = " << m_thread_pool.at(it).second << std::endl;
	}
}

DWORD WINAPI CApplication::ProceedResponse( LPVOID param )
{
	char buf[DEFAULT_BUFSIZE];
	int buf_size = DEFAULT_BUFSIZE;
	SOCKET accept_sock = INVALID_SOCKET;
	CHttpParser http_parser;

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
			accept_sock = ::accept( ( (SOCKET*)param )[0], NULL, NULL );
			if( accept_sock == INVALID_SOCKET && !s_termination_flag )
			{
				std::cout << "Invalid listen_sock. ERROR: " << ::WSAGetLastError() << std::endl;
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
				std::cout << "Receiving error: " << WSAGetLastError() << std::endl;
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
				std::cout << "Sending error: " << WSAGetLastError() << std::endl;
				::shutdown( accept_sock, SD_BOTH );

				continue;
			}

			// Disconnect
			int shutdown_res = ::shutdown( accept_sock, SD_SEND );
			if( shutdown_res == SOCKET_ERROR )
			{
				std::cout << "Disconnect error: " << WSAGetLastError() << std::endl;

				continue;
			}
		}
	}
	catch( const std::runtime_error& rt_ex )
	{
		std::cout << "runtime_error exception handled in WorkThread with ID = " << ::GetCurrentThreadId() << " :" << rt_ex.what() << std::endl;

		::shutdown( accept_sock, SD_SEND );
		::closesocket(accept_sock);

		return 1;
	}
	catch( const std::logic_error& lg_ex )
	{
		std::cout << "logic_error exception handled in WorkThread with ID = " << ::GetCurrentThreadId() << " :" << lg_ex.what() << std::endl;

		::shutdown( accept_sock, SD_SEND );
		::closesocket(accept_sock);

		return 2;
	}
	catch( const std::exception& ex )
	{
		std::cout << "Common exception handled in WorkThread with ID = " << ::GetCurrentThreadId() << " :" << ex.what() << std::endl;

		::shutdown( accept_sock, SD_SEND );
		::closesocket(accept_sock);

		return 3;
	}
	catch(...)
	{
		std::cout << "Unknown exception handled in WorkThread with ID = " << ::GetCurrentThreadId() << " :" << std::endl;
		
		return 4;
	}
	
	return 0;
}
