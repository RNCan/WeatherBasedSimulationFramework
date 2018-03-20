
// DPT.h : fichier d'en-tête principal pour l'application DPT
//
#pragma once

#ifndef __AFXWIN_H__
	#error "incluez 'stdafx.h' avant d'inclure ce fichier pour PCH"
#endif

#include "resource.h"       // symboles principaux



// CNormalsEditorApp:
// Consultez DPT.cpp pour l'implémentation de cette classe
//

class CNormalsEditorApp : public CWinAppEx
{
public:
	CNormalsEditorApp();


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

	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()


protected:

	ULONG_PTR m_nGdiplusToken;


};

extern CNormalsEditorApp theApp;
