// MyCommandLineInfo.h: interface for the CWeatherCmdLine class.
//
//////////////////////////////////////////////////////////////////////
#pragma once

#include "StandardCmdLine.h"
#include "ERMsg.h"

class CWeatherCmdLine : public CStdCmdLine
{
public:

	enum { FIRST_PARAM=CStdCmdLine::NB_PARAM, FILTER=FIRST_PARAM, STATION_NAME, STATION_ID, NB_ALL_PARAM, NB_PARAM=NB_ALL_PARAM-FIRST_PARAM };
	static const char* PARAM_NAME[NB_PARAM];
	//static const char* DEFAULT_PARAM[NB_PARAM];
	//static const short NEED_QUOTE[NB_PARAM];
	
	//static bool NeedQuote(short i);


	CWeatherCmdLine();
	virtual ~CWeatherCmdLine();
    
	
    //virtual void ParseParam( LPCTSTR lpszParam, BOOL bFlag, BOOL bLast );
	//CString GetCommandLine();


	//virtual ERMsg IsValid();

protected :
	
	virtual short GetOptionIndex(LPCTSTR lpszParam);
	
};
