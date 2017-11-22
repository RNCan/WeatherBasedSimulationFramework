
// DPT.h : fichier d'en-tête principal pour l'application DPT
//
#pragma once

#ifndef __AFXWIN_H__
	#error "incluez 'stdafx.h' avant d'inclure ce fichier pour PCH"
#endif

#include "resource.h"       // symboles principaux
#include "MatchStationCmdLine.h"




// CMatchStationApp:
// Consultez DPT.cpp pour l'implémentation de cette classe
//

class CMatchStationApp : public CWinAppEx
{
public:
	CMatchStationApp();


	// Substitutions
public:
	virtual BOOL InitInstance();

	// Implémentation
	UINT  m_nAppLook;
	BOOL  m_bHiColorIcons;

	virtual void PreLoadState();
	virtual void LoadCustomState();
	virtual void SaveCustomState();
	virtual int ExitInstance();

	CMatchStationCmdLine m_cmdInfo;

protected:

		
	DECLARE_MESSAGE_MAP()
	afx_msg void OnAppAbout();


	
//	BOOL ProcessShellCommand(CMatchStationCmdLine& cmdInfo);
	
	ULONG_PTR m_nGdiplusToken;

};

extern CMatchStationApp theApp;
