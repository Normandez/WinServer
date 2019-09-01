#ifndef CAPPLICATION_H
#define CAPPLICATION_H

class CApplication
{
public:
	explicit CApplication();
	CApplication( const CApplication& other ) = delete;
	CApplication( CApplication&& other ) = delete;
	~CApplication();

	void operator=( const CApplication& other ) = delete;
	void operator=( CApplication&& other ) = delete;

};

#endif //CAPPLICATION_H
