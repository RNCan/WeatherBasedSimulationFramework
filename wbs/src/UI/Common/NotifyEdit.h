//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once
// NotifyEdit.h : header file
//


/////////////////////////////////////////////////////////////////////////////
// CNotifyEdit window

class CNotifyEdit : public CEdit
{
// Construction
public:

	enum TMessage { MY_ENTER = WM_USER + 1 };		// the enter key on the keybord


	CNotifyEdit();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNotifyEdit)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CNotifyEdit();

	// Generated message map functions
protected:
	//{{AFX_MSG(CNotifyEdit)
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

