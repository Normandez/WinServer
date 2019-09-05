#ifndef CAPPLICATION_H
#define CAPPLICATION_H

#include <iostream>
#include <vector>

#include <string>
#include <sstream>
#include <algorithm>
#include <iterator>

#include <WinSock2.h>
#include <WS2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_PORT "27015"
#define DEFAULT_BUF_LENGTH 512

class CApplication
{
public:
	explicit CApplication();
	CApplication( const CApplication& other ) = delete;
	CApplication( CApplication&& other ) = delete;
	~CApplication();

	void operator=( const CApplication& other ) = delete;
	void operator=( CApplication&& other ) = delete;

	void Listen();
	void StopAllThreads();

	size_t GetWorkThreadNum() const;
	void PrintWorkThreads() const;

private:
	std::vector<std::pair<HANDLE, DWORD>> m_thread_pool;

	WSADATA m_wsa_data;
	SOCKET m_listen_sock;

	bool m_is_wsa_init;
	bool m_is_listen_sock_init;
	bool m_is_thread_pool_init;

	static DWORD WINAPI ProceedResponse( LPVOID param );

	static bool IsPostRequest( const std::vector<std::string>& recv_splitted );
	static bool IsJsonContentType( const std::vector<std::string>& recv_splitted );
	static std::string ConstructResponse( const std::vector<std::string>& recv_splitted, const std::string& response_data );

};

#endif //CAPPLICATION_H
