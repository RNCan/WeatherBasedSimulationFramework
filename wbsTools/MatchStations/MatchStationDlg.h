// OptionPropSheet.h : header file
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CBioSIMOptionDlg
#include "..\Simulation\OptionLinks.h"


class CBioSIMOptionDlg : public CMFCPropertySheet
{
	DECLARE_DYNAMIC(CBioSIMOptionDlg)

    //enum TPane{LAST_OPEN=-1, BIOSIM, DIR, LINKS, ADVANCED, ERROR_LEVEL};

// Construction
public:
	CBioSIMOptionDlg(CWnd* pParentWnd = NULL);
	//CBioSIMOptionDlg(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = LAST_OPEN);
	virtual ~CBioSIMOptionDlg();

// Attributes
public:
	
	
    COptionLinks m_links;

    

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBioSIMOptionDlg)
	//}}AFX_VIRTUAL

// Implementation
public:


	// Generated message map functions
protected:
	//{{AFX_MSG(CBioSIMOptionDlg)
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	
	void OnFinish();

private:

	HICON	m_hIcon;

    
public:
	virtual BOOL OnInitDialog();
};

