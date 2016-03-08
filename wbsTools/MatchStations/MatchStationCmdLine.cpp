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

const char* CMatchStationCmdLine::PARAM_NAME[NB_PARAM]={"V", "Y", "N","","",""};
//const char* CMatchStationCmdLine::DEFAULT_PARAM[NB_PARAM]={"","",""};



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

	if (!bFlag && m_lastOption==-1 )
	{
		CString ext = UtilWin::GetFileExtension(pszParam);
		short index = -1;
		if (ext.CompareNoCase(_T(".NormalsStations")) == 0)
			index = NORMALS_FILEPATH;
		else if (ext.CompareNoCase(_T(".DailyStations")) == 0)
			index = DAILY_FILEPATH;
		else if (ext.CompareNoCase(_T(".HourlyStations")) == 0)
			index = HOURLY_FILEPATH;

		if (index != -1)
		{
			m_bParam[index] = true;
			m_param[index] = pszParam;
			ParseLast(bLast);
			return;
		}
	}

	return CStdCmdLine::ParseParam(pszParam, bFlag, bLast);
}
//void CMatchStationCmdLine::ParseParam( LPCTSTR lpszParam, BOOL bFlag, BOOL bLast )
//{
//	CStdCmdLine::ParseParam( lpszParam, bFlag, bLast );
//}

//ERMsg CMatchStationCmdLine::IsValid()
//{
//	ERMsg msg = CStdCmdLine::IsValid();
//
//	if( msg )
//	{
//		//working directory is specified we make the mixte
//		int nbPoints = atoi(m_param[NB_POINTS]);
//		double power = atof(m_param[POWER]);
//		if( nbPoints<1 || nbPoints > 100)
//			m_msg.ajoute( GetString( IDS_BAD_NBPOINTS) );
//		if( power < 0 || power > 5)
//			m_msg.ajoute( GetString( IDS_BAD_POWER) );
//		if( m_param[TRANING_SET].IsEmpty() ||
//			m_param[INPUT_MAP].IsEmpty() ||
//			m_param[OUTPUT_MAP].IsEmpty() )
//		{
//			m_msg.ajoute(GetString( IDS_INPUT_EMPTY_NAME));
//		}
//		
//		short ouputtype = atoi(m_param[OUTPUT_TYPE]);
//		if( ouputtype<0 || ouputtype>3)
//		{
//			m_msg.ajoute(GetString( IDS_INPUT_BAD_OUTPUT_TYPE));
//		}
//
//	}
//	else
//	{
//		m_msg.ajoute(" "); 
//		m_msg.ajoute(GetString(IDS_INPUT_DATASET) + m_param[TRANING_SET]);
//		m_msg.ajoute(GetString(IDS_INPUT_MAP) + m_param[INPUT_MAP]);
//		m_msg.ajoute(GetString(IDS_OUTPUT_MAP) + m_param[OUTPUT_MAP]);
//		m_msg.ajoute(GetString(IDS_MASK_USED) + m_param[MASK]);
//		m_msg.ajoute("K= " + m_param[NB_POINTS]);
//		m_msg.ajoute("T= " + m_param[POWER]);
//		m_msg.ajoute(GetString(IDS_NO_DATA) + m_param[NO_DATA_VALUE]);
//		m_msg.ajoute(GetString(IDS_OUTPUT_TYPE) + m_param[OUTPUT_TYPE]);
//
//		m_msg.ajoute(" ");
//		m_msg.ajoute(GetString(IDS_CMD_USAGE));
//	}
//
//	return m_msg;
//}

//bool CMatchStationCmdLine::NeedQuote(short i)
//{
//	ASSERT( i>=0 && i<NB_PARAM);
//	return NEED_QUOTE[i];
//}

//CString CMatchStationCmdLine::GetCommandLine()
//{
//	ASSERT( IsValid() );
//
////	CAppOption option;
//
//	CString cmdLine = GetApplicationPath() + "KNNMapping.exe ";
//	for(int i=0; i<NB_PARAM; i++)
//	{
//		if( !m_param[i].IsEmpty() )
//		{
//			CString tmp;
//
//			if( NeedQuote(i) )
//				tmp.Format("/%s \"%s\" ", PARAM_NAME[i], m_param[i]);
//			else tmp.Format("/%s %s ", PARAM_NAME[i], m_param[i]);
//
//			cmdLine+=tmp;
//		}
//	}
//
//	return cmdLine;
//}