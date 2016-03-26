//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
//****************************************************************************
#include "stdafx.h"
#include "StandardCmdLine.h"

#include "Simulation.h"
#include "WeatherBasedSimulationString.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//using namespace UtilWin;
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

const char* CStdCmdLine::PARAM_NAME[NB_PARAM][2]=
{	
	{"EXEC","E"},
	{"LOG", "L"},
	{"SHOW", "SHOW"},
	{"HELP", "?"}
};



CStdCmdLine::CStdCmdLine(int nbParam)
{
	m_bParam.SetSize(nbParam);
    m_param.SetSize(nbParam);

	Reset();
}

void CStdCmdLine::Reset()
{
	m_lastOption=-1;
	for(int i=0; i<NB_PARAM; i++)
	{
		m_bParam[i]=false;
		m_param[i].Empty();
	}

	m_msg = ERMsg();
}

CStdCmdLine::~CStdCmdLine()
{}


short CStdCmdLine::GetOptionIndex(LPCTSTR lpszParam)
{
	CString tmp(lpszParam);
	tmp.MakeUpper();

	short index = -1;
	for(int i=0; i<NB_PARAM; i++)
	{
		if( tmp == PARAM_NAME[i][0] || tmp == PARAM_NAME[i][1])
		{
			index=i;
			break;
		}
	}

	return index;
}

void CStdCmdLine::ParseParam( LPCTSTR lpszParam, BOOL bFlag, BOOL bLast )
{
    if( bFlag)
    {
		int index = GetOptionIndex(lpszParam);
        if( index >= 0)
		{
			m_bParam[index]=true;
			m_lastOption=index;
		}
		else
		{
			//add msg
			std::string param = CStringA(lpszParam);
			std::string error = WBSF::FormatMsg(IDS_CMN_BAD_CMD_PARAM, param);
			m_msg.ajoute(error);
		}
        
    }
    else 
    {
		if( m_lastOption >= 0)
		{	
			m_param[m_lastOption] = lpszParam;
			m_param[m_lastOption].Trim();
			m_lastOption=-1;
			ParseLast(bLast);
			return;
		}
    }

    CCommandLineInfo::ParseParam( lpszParam, bFlag, bLast );
}

ERMsg CStdCmdLine::IsValid()
{
	//ASSERT( m_bExec );


	if( m_bParam[HELP] )
	{
		m_msg.ajoute(" ");
		m_msg.ajoute(WBSF::GetString(IDS_CMN_CMD_USAGE));
	}
	

	return m_msg;
}

