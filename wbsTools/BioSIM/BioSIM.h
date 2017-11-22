
// BioSIM.h : main header file for the BioSIM application
//
#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"       // main symbols
//#include "FileManager.h"

// CBioSIMApp:
// See BioSIM.cpp for the implementation of this class
//

class CBioSIMApp : public CWinAppEx
{
public:
	CBioSIMApp();
	virtual ~CBioSIMApp();

	
// Overrides
public:
	virtual BOOL InitInstance();
	virtual void ParseCommandLine(CCommandLineInfo& rCmdInfo);
	virtual int ExitInstance();
// Implementation
	UINT	m_nAppLook;
	BOOL	m_bHiColorIcons;
	int		m_exitCode;
	ULONG_PTR m_gdiplusToken;
	

	virtual void PreLoadState();
	virtual void LoadCustomState();
	virtual void SaveCustomState();
	

	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()

};

extern CBioSIMApp theApp;


class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX2 };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support


	CStatic m_versionCtrl;

	// Implementation
protected:
	DECLARE_MESSAGE_MAP()
public:
	
};
