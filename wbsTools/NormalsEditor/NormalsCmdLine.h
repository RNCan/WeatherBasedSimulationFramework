// MyCommandLineInfo.h: interface for the CNormalsCmdLine class.
//
//////////////////////////////////////////////////////////////////////
#pragma once

#include "UI/Common/StandardCmdLine.h"


class CNormalsCmdLine : public CStdCmdLine
{
public:

	enum { FIRST_PARAM=CStdCmdLine::NB_PARAM, FILTER=FIRST_PARAM, STATION_NAME, STATION_ID, NB_ALL_PARAM, NB_PARAM=NB_ALL_PARAM-FIRST_PARAM };
	static const char* PARAM_NAME[NB_PARAM];

	CNormalsCmdLine();
	virtual ~CNormalsCmdLine();

protected :
	
	virtual short GetOptionIndex(LPCTSTR lpszParam);
	
};
