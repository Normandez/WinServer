#include "CApplication.h"

static const size_t s_max_thread_pool_size = 15;
static CRITICAL_SECTION critical_sec;

#define DEFAULT_PORT "27015"
#define DEFAULT_BUFSIZE 512

CApplication::CApplication()
	: m_listen_sock(INVALID_SOCKET)
{
	::InitializeCriticalSection(&critical_sec);
	m_thread_pool.reserve(15);

	int wsa_init_res = ::WSAStartup( MAKEWORD(2,0), &m_wsa_data );
	if(wsa_init_res)
	{
		std::cout << "WSADATA init failed with code: " << wsa_init_res << std::endl;
		return;
	}

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
		::WSACleanup();

		return;
	}

	m_listen_sock = socket( addrinfo_res->ai_family, addrinfo_res->ai_socktype, addrinfo_res->ai_protocol );
	if( m_listen_sock == INVALID_SOCKET )
	{
		std::cout << "Listen socket init failed with error: " << ::WSAGetLastError() << std::endl;
		::freeaddrinfo(addrinfo_res);
		::WSACleanup();

		return;
	}

	int bind_res = ::bind( m_listen_sock, addrinfo_res->ai_addr, (int)addrinfo_res->ai_addrlen );
	if( bind_res == SOCKET_ERROR )
	{
		std::cout << "Listen socket binding failed with error: " << ::WSAGetLastError() << std::endl;
		::freeaddrinfo(addrinfo_res);
		::closesocket(m_listen_sock);
		::WSACleanup();

		return;
	}
	::freeaddrinfo(addrinfo_res);
}

CApplication::~CApplication()
{
	::closesocket(m_listen_sock);
	::WSACleanup();

	// Working thread pool cleanup
	for( size_t count = 0; count < 2; count++ )
	{
		::CloseHandle( m_thread_pool.at(count).first );
	}
}

void CApplication::Listen()
{
	int listen_res = ::listen( m_listen_sock, SOMAXCONN );
	if( listen_res == SOCKET_ERROR )
	{
		std::cout << "Listen socket listenning failed with error: " << WSAGetLastError() << std::endl;
		::closesocket(m_listen_sock);
		::WSACleanup();

		return;
	}

	for( size_t count = 0; count < 2; count++ )
	{
		std::pair<HANDLE, DWORD> new_pair;
		new_pair.first = ::CreateThread( NULL, 0, &CApplication::ProceedResponse, &m_listen_sock, 0, &new_pair.second );
		m_thread_pool.push_back(new_pair);
	}

}

DWORD WINAPI CApplication::ProceedResponse( LPVOID param )
{
	SOCKET accept_sock = INVALID_SOCKET;
	while(true)
	{
		accept_sock = ::accept( ( (SOCKET*)param )[0], NULL, NULL );
		if ( accept_sock == INVALID_SOCKET )
		{
			std::cout << "Invalid listen_sock" << std::endl;
			std::cout << "ERROR: " << ::WSAGetLastError() << std::endl;
			::closesocket(accept_sock);

			return 1;
		}

		char buf[DEFAULT_BUFSIZE];
		int buf_size = DEFAULT_BUFSIZE;
		::recv( accept_sock, buf, buf_size, 0 );
		std::cout << "Received" << std::endl;
		::send( accept_sock, buf, buf_size, 0 );
		::shutdown( accept_sock, SD_SEND );
	}
	

	return 0;
}
