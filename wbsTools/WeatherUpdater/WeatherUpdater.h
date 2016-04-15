
// DPT.h : fichier d'en-tête principal pour l'application DPT
//
#pragma once

#ifndef __AFXWIN_H__
	#error "incluez 'stdafx.h' avant d'inclure ce fichier pour PCH"
#endif

#include "resource.h"       // symboles principaux



// CWeatherUpdaterApp:
// Consultez DPT.cpp pour l'implémentation de cette classe
//

class CWeatherUpdaterApp : public CWinAppEx
{
public:

	static UINT ExecuteTasks(void* pParam);



	CWeatherUpdaterApp();

	virtual BOOL InitInstance();

// Implémentation
	UINT  m_nAppLook;
	BOOL  m_bHiColorIcons;
	int	  m_exitCode;

	virtual void PreLoadState();
	virtual void LoadCustomState();
	virtual void SaveCustomState();

	
	DECLARE_MESSAGE_MAP()
	afx_msg void OnAppAbout();
	afx_msg void OnAppUpdaterReference();


protected:

	ULONG_PTR m_nGdiplusToken;
public:
	virtual int ExitInstance();
};

extern CWeatherUpdaterApp theApp;
