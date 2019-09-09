#ifndef CLOGGER_H
#define CLOGGER_H

#include <iostream>
#include <fstream>

#include <string>

class CLogger
{
public:
	explicit CLogger( const std::string& log_file_name );
	CLogger( const CLogger& other ) = delete;
	CLogger( CLogger&& other ) = delete;
	~CLogger();

	void operator=( const CLogger& other ) = delete;
	void operator=( CLogger&& other ) = delete;

	void MakeLog( const std::string& log_line );
	
private:
	std::ofstream m_log_file_strm;

	bool m_enable_file_logging;

};

#endif //CLOGGER_H
