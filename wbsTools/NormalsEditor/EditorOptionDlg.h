// OptionPropSheet.h : header file
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CNormalsEditorOptionsDlg
#include "..\Simulation\OptionLinks.h"


class CNormalsEditorOptionsDlg : public CMFCPropertySheet
{
	DECLARE_DYNAMIC(CNormalsEditorOptionsDlg)

    //enum TPane{LAST_OPEN=-1, BIOSIM, DIR, LINKS, ADVANCED, ERROR_LEVEL};

// Construction
public:
	CNormalsEditorOptionsDlg(CWnd* pParentWnd = NULL);
	//CNormalsEditorOptionsDlg(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = LAST_OPEN);
	virtual ~CNormalsEditorOptionsDlg();

// Attributes
public:
	
	
    COptionLinks m_links;

    

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNormalsEditorOptionsDlg)
	//}}AFX_VIRTUAL

// Implementation
public:


	// Generated message map functions
protected:
	//{{AFX_MSG(CNormalsEditorOptionsDlg)
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	
	void OnFinish();

private:

	HICON	m_hIcon;

    
public:
	virtual BOOL OnInitDialog();
};

