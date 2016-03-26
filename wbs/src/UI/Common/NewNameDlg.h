//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once


#include <string>
#include "WeatherBasedSimulationUI.h"
/////////////////////////////////////////////////////////////////////////////
// CNewNameDlg dialog

class CNewNameDlg : public CDialog
{
// Construction
public:
	CNewNameDlg(CWnd* pParent = NULL);   // standard constructor

    CString m_title;
// Dialog Data
	
	enum { IDD = IDD_CMN_NEW_NAME };
	std::string	m_name;
	


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNewNameDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CNewNameDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

