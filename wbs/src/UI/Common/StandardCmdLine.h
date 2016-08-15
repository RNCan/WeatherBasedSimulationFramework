//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once

#include "basic/ERMsg.h"

class CStdCmdLine : public CCommandLineInfo  
{
public:

	enum { EXECUTE, LOG, SHOW, HELP, NB_PARAM};
	static const char* PARAM_NAME[NB_PARAM][2];
	
	

	CStdCmdLine(int nbParam=NB_PARAM);
	virtual ~CStdCmdLine();
    
	void Reset();
    virtual void ParseParam( LPCTSTR lpszParam, BOOL bFlag, BOOL bLast );
	virtual ERMsg IsValid();

	
	bool Is(short i)const { ASSERT(i>=0&&i<m_bParam.GetSize());return m_bParam[i];}
	bool Have(short i)const { ASSERT(i >= 0 && i<m_bParam.GetSize()); return m_bParam[i]; }
	CString GetParam(short i)const { ASSERT(i >= 0 && i<m_param.GetSize()); return m_param[i]; }
	//bool ParseError()const { return m_msg==0; }
	//ERMsg GetLastError()const { return m_msg; }
	

protected:

	virtual short GetOptionIndex(LPCTSTR lpszParam);

	//bool m_bParam[NB_PARAM];
    //CString m_param[NB_PARAM];
	CArray<bool> m_bParam;
    CStringArray m_param;
	
	short m_lastOption;

	ERMsg m_msg;
};

//bool AttachConsole( DWORD dwProcessId );