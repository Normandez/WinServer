#include "CHttpParser.h"

CHttpParser::CHttpParser()
{ }

CHttpParser::CHttpParser( const char* request_buf )
{
	LoadRequest(request_buf);
}

CHttpParser::~CHttpParser()
{ }

void CHttpParser::LoadRequest( const char* request_buf )
{
	std::string buf_str(request_buf);
	char delim = '\n';
	std::string token = "";

	std::stringstream buf_str_strm(buf_str);
	while( std::getline( buf_str_strm, token, delim ) )
	{
		m_request_lines.push_back(token);
	}
}
