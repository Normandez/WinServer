#ifndef CAPPLICATION_H
#define CAPPLICATION_H

#include <iostream>
#include <vector>

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

private:
	std::vector<HANDLE> m_thread_pool;

	static DWORD WINAPI ProceedResponse( LPVOID param );

};

#endif //CAPPLICATION_H
