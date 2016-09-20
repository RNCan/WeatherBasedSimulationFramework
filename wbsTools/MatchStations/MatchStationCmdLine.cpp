// MyCommandLineInfo.cpp: implementation of the CMatchStationCmdLine class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MatchStationCmdLine.h"
#include "Resource.h"
#include "UI/Common/UtilWin.h"
#include "UI/Common/AppOption.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

using namespace UtilWin;
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

const char* CMatchStationCmdLine::PARAM_NAME[NB_PARAM]={"V", "Y", "K", "T", "N", "D", "H"};


CMatchStationCmdLine::CMatchStationCmdLine():
CStdCmdLine(NB_ALL_PARAM)
{}


CMatchStationCmdLine::~CMatchStationCmdLine()
{}


short CMatchStationCmdLine::GetOptionIndex(LPCTSTR lpszParam)
{
	short index = CStdCmdLine::GetOptionIndex(lpszParam);
	if (index != -1)
		return index;

	CString tmp(lpszParam);
	tmp.MakeUpper();
	
	for(int i=0; i<NB_PARAM; i++)
	{
		if( tmp == PARAM_NAME[i] )
		{
			index=FIRST_PARAM+i;
			break;
		}
	}

	
	return index;
}

void CMatchStationCmdLine::ParseParam(const TCHAR* pszParam, BOOL bFlag, BOOL bLast)
{
	return CStdCmdLine::ParseParam(pszParam, bFlag, bLast);
}
