//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once


#include <afxcmn.h>
/////////////////////////////////////////////////////////////////////////////
// CMyToolTipCtrl window

class CMyToolTipCtrl : public CToolTipCtrl
{
// Construction
public:
	CMyToolTipCtrl();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMyToolTipCtrl)
	//}}AFX_VIRTUAL

// Implementation
public:
	BOOL AddRectTool( CWnd* pWnd, const LPCTSTR pszText, LPCRECT lpRect, UINT nIdTool);
	BOOL AddWindowTool( CWnd* pWnd, LPCTSTR pszText);
	virtual ~CMyToolTipCtrl();

	// Generated message map functions
protected:
	//{{AFX_MSG(CMyToolTipCtrl)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

