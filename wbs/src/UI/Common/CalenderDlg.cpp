//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
//****************************************************************************
#include "stdafx.h"
#include "Basic/UtilTime.h"
#include "UI/Common/CalenderDlg.h"
  
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCalendrierDlg dialog 


CCalendrierDlg::CCalendrierDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CCalendrierDlg::IDD, pParent)
{
	CTime t = CTime::GetCurrentTime();
	m_day = t.GetDay();
	m_month = t.GetMonth();
	m_year = t.GetYear();
	
}
 

void CCalendrierDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_CALENDAR, m_calendar);
}


BEGIN_MESSAGE_MAP(CCalendrierDlg, CDialog)
	ON_NOTIFY(MCN_SELECT, IDC_CALENDAR, OnSelectDate)
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCalendrierDlg message handlers

void CCalendrierDlg::OnOK() 
{
    SYSTEMTIME sysTime;
    m_calendar.GetCurSel( &sysTime );
    

	m_day = sysTime.wDay;
	m_month = sysTime.wMonth;
	m_year = sysTime.wYear;

	
	CDialog::OnOK();
}

void CCalendrierDlg::OnSelectDate(NMHDR* pNMHDR, LRESULT* pResult) 
{
	
	OnOK();	
	*pResult = 0;
}

size_t CCalendrierDlg::GetJDay()
{
	// call global function GetDay
	return WBSF::GetJDay(m_year, m_month, m_day);
}

BOOL CCalendrierDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

    CTime refDateTime(m_year, int(m_month)+1, int(m_day)+1,0,0,0 );
    m_calendar.SetCurSel( refDateTime );
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CCalendrierDlg::OnEraseBkgnd(CDC* pDC) 
{
    
    CBrush brush(PS_SOLID, RGB(255,255,255) );

    CRect rect;
    GetClientRect(rect);
    pDC->FillRect(rect, &brush);
	
    return TRUE;
}
