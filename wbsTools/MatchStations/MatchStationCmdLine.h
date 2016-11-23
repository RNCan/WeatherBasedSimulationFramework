// MyCommandLineInfo.h: interface for the CMatchStationCmdLine class. 
//
//////////////////////////////////////////////////////////////////////
#pragma once

#include "Basic/ERMsg.h"
#include "UI/Common/StandardCmdLine.h"


class CMatchStationCmdLine : public CStdCmdLine
{
public:

	enum { FIRST_PARAM = CStdCmdLine::NB_PARAM, VARIABLE = FIRST_PARAM, YEAR, NB_STATIONS, SEARCH_RADIUS, OBS_TYPE, SKIP_VERIFY, NORMALS_FILEPATH, DAILY_FILEPATH, HOURLY_FILEPATH, NB_ALL_PARAM, NB_PARAM = NB_ALL_PARAM - FIRST_PARAM };
	static const char* PARAM_NAME[NB_PARAM];

	CMatchStationCmdLine();
	virtual ~CMatchStationCmdLine();

	virtual void ParseParam(const TCHAR* pszParam, BOOL bFlag, BOOL bLast);
	virtual short GetOptionIndex(LPCTSTR lpszParam);
	

protected :
	
	
	
};
