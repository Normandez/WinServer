#ifndef CHTTPPARSER_H
#define CHTTPPARSER_H

#include <string>
#include <sstream>
#include <vector>

class CHttpParser
{
public:
	CHttpParser();
	CHttpParser( const char* request_buf );
	~CHttpParser();

	CHttpParser( const CHttpParser& other ) = delete;
	CHttpParser( CHttpParser&& other ) = delete;
	void operator=( const CHttpParser& other ) = delete;
	void operator=( CHttpParser&& other ) = delete;

	void LoadRequest( const char* request_buf );

private:
	std::vector<std::string> m_request_lines;

};

#endif //CHTTPPARSER_H
