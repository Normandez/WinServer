#ifndef CAPPLICATION_H
#define CAPPLICATION_H

#include <iostream>

#include <iterator>
#include <regex>

#pragma comment(lib, "Ws2_32.lib")
#include <WinSock2.h>
#include <WS2tcpip.h>

#include "CHttpParser.h"
#include "CLogger.h"

class CApplication
{
public:
	explicit CApplication( int num_of_threads, const std::string& listen_port, CLogger* logger, CRITICAL_SECTION* p_log_critical_sec );
	CApplication( const CApplication& other ) = delete;
	CApplication( CApplication&& other ) = delete;
	~CApplication();

	void operator=( const CApplication& other ) = delete;
	void operator=( CApplication&& other ) = delete;

	void Listen();
	void StopAllThreads();

	size_t GetWorkThreadsNum() const;
	void PrintWorkThreads() const;

private:
	CLogger* m_logger;
	CRITICAL_SECTION* m_log_critical_sec;

	std::vector<std::pair<HANDLE, DWORD>> m_thread_pool;
	int m_num_of_threads;

	WSADATA m_wsa_data;
	SOCKET m_listen_sock;

	bool m_is_wsa_init;
	bool m_is_listen_sock_init;
	bool m_is_thread_pool_init;

	struct SThreadArgs
	{
		SOCKET* listen_sock = nullptr;
		CLogger* logger = nullptr;
		CRITICAL_SECTION* log_critical_sec = nullptr;
	} m_thread_args;
	static DWORD WINAPI ProceedResponse( LPVOID param );

};

#endif //CAPPLICATION_H
