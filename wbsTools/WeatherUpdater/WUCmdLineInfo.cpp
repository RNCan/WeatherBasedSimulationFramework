// MyCommandLineInfo.cpp: implementation of the CMyCommandLineInfo class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MyCommandLineInfo.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMyCommandLineInfo::CMyCommandLineInfo():
m_bOpenLOG(false),
m_bExecute(false)
{}

CMyCommandLineInfo::~CMyCommandLineInfo()
{}

void CMyCommandLineInfo::ParseParam( LPCTSTR lpszParam, BOOL bFlag, BOOL bLast )
{
    if( bFlag)
    {
        CString tmp(lpszParam);
        tmp.MakeUpper();

        if( tmp.Find(_T("LOG")) != -1)
            m_bOpenLOG = true;
		else if( tmp.Find(_T("EXEC")) != -1)
			m_bExecute = true;
        
    }
    else if( m_bOpenLOG )
    {
        m_bOpenLOG = false;
        m_LOGFileName = lpszParam;
    }
	//else if( m_bInterface )

    
    if( bLast ) 
        if( !m_LOGFileName.IsEmpty() )
            m_bOpenLOG = true;

    CCommandLineInfo::ParseParam( lpszParam, bFlag, bLast );
}