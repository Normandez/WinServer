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

const std::vector<std::string>& CHttpParser::ReadRequestLines() const
{
	return m_request_lines;
}

bool CHttpParser::IsPostRequest() const
{
	const std::string post_str = "POST";
	std::string first_line = "";
	try
	{
		first_line = m_request_lines.at(0);
	}
	catch( const std::out_of_range& out_of_range_ex )
	{
		(void)out_of_range_ex;
		return false;
	}
	
	std::transform( first_line.begin(), first_line.end(), first_line.begin(), ::toupper );

	return ( first_line.substr( 0, post_str.size() ) == post_str );
}

bool CHttpParser::IsJsonContentType() const
{
	const std::string header_content_type_str = "content-type:";
	const std::string json_content_type_str = "application/json";

	for( std::string line : m_request_lines )
	{
		std::transform( line.begin(), line.end(), line.begin(), ::tolower );
		if( line.substr( 0, header_content_type_str.size() ) == header_content_type_str )
		{
			std::string content_type = line.substr( header_content_type_str.size() );
			if( content_type.back() == '\r' )
			{
				content_type.resize( content_type.size() - 1 );
			}
			if( content_type == json_content_type_str || content_type == ( " " + json_content_type_str ) )
			{
				return true;
			}
		}
	}

	return false;
}

std::string CHttpParser::ConstructResponse( const std::string& response_data, bool is_success ) const
{
	std::string constructed_response = "";

	if(is_success) constructed_response += "HTTP /1.1 200 OK\r\n";
	else constructed_response += "HTTP/1.1 400 Bad Request\r\n";
	constructed_response += "Content-Type: application/json\r\n";
	constructed_response += "Content-Length: " + std::to_string( (int)response_data.size() );
	constructed_response += "\r\n";
	constructed_response += "Server: WinServer\r\n";
	constructed_response += "Connection: Closed\r\n";
	constructed_response += "\r\n";
	constructed_response += response_data;

	return constructed_response;
}
