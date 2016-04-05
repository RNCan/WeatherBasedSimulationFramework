// MyCommandLineInfo.h: interface for the CWeatherUpdaterCmdLine class.
//
//////////////////////////////////////////////////////////////////////
#pragma once

#include "UI/Common/StandardCmdLine.h"


class CWeatherUpdaterCmdLine : public CStdCmdLine
{
public:

	enum { FIRST_PARAM=CStdCmdLine::NB_PARAM, FILTER=FIRST_PARAM, STATION_NAME, STATION_ID, NB_ALL_PARAM, NB_PARAM=NB_ALL_PARAM-FIRST_PARAM };
	static const char* PARAM_NAME[NB_PARAM];

	CWeatherUpdaterCmdLine();
	virtual ~CWeatherUpdaterCmdLine();

protected :
	
	virtual short GetOptionIndex(LPCTSTR lpszParam);
	
};
