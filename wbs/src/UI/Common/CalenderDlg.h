//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once



#include "WeatherBasedSimulationUI.h"

/////////////////////////////////////////////////////////////////////////////
// CCalendrierDlg dialog

class CCalendrierDlg : public CDialog
{
// Construction
public:
	CCalendrierDlg(CWnd* pParent = NULL);   // standard constructor

	size_t GetJDay();
	size_t m_day;
	size_t m_month;
	int m_year;


// Dialog Data
	enum { IDD = IDD_CALENDRIER };
	CMonthCalCtrl	m_calendar;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCalendrierDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CCalendrierDlg)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnSelectDate(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()


    
};

