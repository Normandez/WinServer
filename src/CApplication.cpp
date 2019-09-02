#include "CApplication.h"

static const size_t s_max_thread_pool_size = 15;
static CRITICAL_SECTION critical_sec;

CApplication::CApplication()
{
	::InitializeCriticalSection(&critical_sec);

	// Working thread pool startup
	m_thread_pool.reserve(15);
	for( size_t count = 0; count < s_max_thread_pool_size; count++ )
	{
		DWORD id_sub_thread = (DWORD)count + 1;
		m_thread_pool.push_back( ::CreateThread( NULL, 0, &CApplication::ProceedResponse, NULL, 0, &id_sub_thread ) );
	}
}

CApplication::~CApplication()
{
	// Working thread pool cleanup
	for( size_t count = 0; count < s_max_thread_pool_size; count++ )
	{
		::CloseHandle( m_thread_pool.at(count) );
	}
}

void CApplication::Listen()
{

}

DWORD WINAPI CApplication::ProceedResponse( LPVOID param )
{
	// Working thread startup condition
	if(!param)
	{
		::EnterCriticalSection(&critical_sec);
		std::cout << "Work thread created" << std::endl;
		::LeaveCriticalSection(&critical_sec);

		return 0;
	}

	return 0;
}
