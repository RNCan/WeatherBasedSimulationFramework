// TDate.h : main header file for the TDATE application
//

#if !defined(AFX_TDATE_H__DE4495D9_436E_11D3_8188_006008A9A18F__INCLUDED_)
#define AFX_TDATE_H__DE4495D9_436E_11D3_8188_006008A9A18F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CTDateApp:
// See TDate.cpp for the implementation of this class
//

class CTDateApp : public CWinApp
{
public:
	CTDateApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTDateApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL
    
    BOOL CTDateApp::FirstInstance();

// Implementation

	//{{AFX_MSG(CTDateApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TDATE_H__DE4495D9_436E_11D3_8188_006008A9A18F__INCLUDED_)
