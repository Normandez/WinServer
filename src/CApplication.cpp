#include "CApplication.h"

#define DEFAULT_PORT "27015"
#define DEFAULT_BUFSIZE 512

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
	m_thread_pool.reserve(15);

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
		::recv( accept_sock, buf, buf_size, 0 );
		::send( accept_sock, buf, buf_size, 0 );
		::shutdown( accept_sock, SD_SEND );
	}
	
	return 0;
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
