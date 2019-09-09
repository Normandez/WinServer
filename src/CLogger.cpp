#include "CLogger.h"

CLogger::CLogger( const std::string& log_file_name )
	: m_enable_file_logging(true)
{
	m_log_file_strm.open( log_file_name, std::ios_base::out | std::ios_base::trunc );
	if( !m_log_file_strm.is_open() )
	{
		std::cout << "> Log file not opened! File logging disabled." << std::endl;

		m_enable_file_logging = false;
	}
}

CLogger::~CLogger()
{
	if(m_enable_file_logging) m_log_file_strm.close();
}

void CLogger::MakeLog( const std::string& log_line )
{
	std::cout << "> " << log_line << std::endl;
	if(m_enable_file_logging) m_log_file_strm << "> " << log_line.c_str() << std::endl;
}

void CLogger::MakeErrorLog( const std::string& err_lor_line )
{
	std::cerr << "> ERROR: " << err_lor_line << std::endl;
	if(m_enable_file_logging) m_log_file_strm << "> ERROR: " << err_lor_line << std::endl;
}
