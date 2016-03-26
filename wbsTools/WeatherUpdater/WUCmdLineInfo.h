// MyCommandLineInfo.h: interface for the CMyCommandLineInfo class.
//
//////////////////////////////////////////////////////////////////////
#pragma once

class CMyCommandLineInfo : public CCommandLineInfo  
{
public:
	CMyCommandLineInfo();
	virtual ~CMyCommandLineInfo();
    
    virtual void ParseParam( LPCTSTR lpszParam, BOOL bFlag, BOOL bLast );

    bool m_bOpenLOG;
    CString m_LOGFileName;
	bool m_bExecute;
    

};
