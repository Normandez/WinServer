#include "CApplication.h"

#define DEFAULT_PORT "27015"
#define DEFAULT_BUFSIZE 1024

namespace
{
	static const size_t s_max_thread_pool_size = 15;
	static CRITICAL_SECTION critical_sec;
	static bool s_termination_flag = false;
}

CApplication::CApplication()
	: m_listen_sock(INVALID_SOCKET),
	  m_is_wsa_init(false),
	  m_is_listen_sock_init(false),
	  m_is_thread_pool_init(false)
{
	::InitializeCriticalSection(&critical_sec);
	m_thread_pool.reserve(s_max_thread_pool_size);

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
	int getaddrinfo_res = getaddrinfo( NULL, DEFAULT_PORT, &addrinfo_params, &addrinfo_res );
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
	for( size_t count = 0; count < s_max_thread_pool_size; count++ )
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
	::EnterCriticalSection(&critical_sec);
	s_termination_flag = true;
	::LeaveCriticalSection(&critical_sec);
	
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

DWORD WINAPI CApplication::ProceedResponse( LPVOID param )
{
	SOCKET accept_sock = INVALID_SOCKET;

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
			::EnterCriticalSection(&critical_sec);
			std::cout << "Invalid listen_sock. ERROR: " << ::WSAGetLastError() << std::endl;
			::LeaveCriticalSection(&critical_sec);
			::closesocket(accept_sock);
			return 1;
		}
		else if(s_termination_flag)
		{
			::shutdown( accept_sock, SD_SEND );
			::closesocket(accept_sock);

			break;
		}
		
		char buf[DEFAULT_BUFSIZE];
		int buf_size = DEFAULT_BUFSIZE;

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
		std::string buf_str(buf);
		char delim = '\n';
		std::string token = "";

		std::stringstream buf_str_strm(buf_str);
		std::vector<std::string> buf_str_lines;
		while( std::getline( buf_str_strm, token, delim ) )
		{
			buf_str_lines.push_back(token);
		}

		if( IsPostRequest(buf_str_lines) )
		{
			if( IsJsonContentType(buf_str_lines) )
			{
				std::regex data_regex("\"data\": ?\"(.*)\"");
				std::smatch data_match;
				if( std::regex_search( buf_str_lines.back(), data_match, data_regex ) )
				{
					std::string data = data_match[1].str();
					std::reverse( data.begin(), data.end() );

					response_data = ConstructResponse( "\"{\\\"data\\\":\\\"" + data + "\\\"}\"", true );
				}
				else
				{
					response_data = ConstructResponse( "\"{\\\"error\\\":\\\"'data' field not found\\\"}\"", false );
				}
			}
			else
			{
				response_data = ConstructResponse( "\"{\\\"error\\\":\\\"Not application/json content type\\\"}\"", false );
			}
		}
		else
		{
			response_data = ConstructResponse( "\"{\\\"error\\\":\\\"Not POST request\\\"}\"", false );
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
	
	return 0;
}

bool CApplication::IsPostRequest( const std::vector<std::string>& recv_splitted )
{
	const std::string post_str = "POST";
	std::string first_line = recv_splitted.at(0);
	
	std::transform( first_line.begin(), first_line.end(), first_line.begin(), ::toupper );

	return ( first_line.substr( 0, post_str.size() ) == post_str );
}

bool CApplication::IsJsonContentType( const std::vector<std::string>& recv_splitted )
{
	const std::string header_content_type_str = "content-type:";
	const std::string json_content_type_str = "application/json";

	for( std::string line : recv_splitted )
	{
		std::transform( line.begin(), line.end(), line.begin(), ::tolower );
		if( line.substr( 0, header_content_type_str.size() ) == header_content_type_str )
		{
			std::string content_type = line.substr( header_content_type_str.size() );
			if( content_type.back() == '\r' )
			{
				content_type.resize( content_type.size() - 1 );
			}
			if( content_type == json_content_type_str || content_type == ( " " + json_content_type_str ) )
			{
				return true;
			}
		}
	}

	return false;
}

std::string CApplication::ConstructResponse( const std::string& response_data, bool is_success )
{
	std::string constructed_response = "";

	if(is_success) constructed_response += "HTTP /1.1 200 OK\n";
	else constructed_response += "HTTP /1.1 400 Bad Request\n";
	constructed_response += "Content-Type: application/json\n";
	constructed_response += "Content-Length: " + std::to_string( (int)response_data.size() );
	constructed_response += "\n";
	constructed_response += "Server: WinServer\n\n";
	constructed_response += response_data;

	return constructed_response;
}

size_t CApplication::GetWorkThreadNum() const
{
	return s_termination_flag ? 0 : m_thread_pool.size();
}

void CApplication::PrintWorkThreads() const
{
	::EnterCriticalSection(&critical_sec);
	if( m_thread_pool.empty() || s_termination_flag )
	{
		std::cout << "ThreadPool is empty" << std::endl;
		return;
	}
	for( size_t it = 0; it < m_thread_pool.size(); it++ )
	{
		std::cout << "WorkThread #" << it << ", HANDLE = " << m_thread_pool.at(it).first << ", ID = " << m_thread_pool.at(it).second << std::endl;
	}
	::LeaveCriticalSection(&critical_sec);
}
