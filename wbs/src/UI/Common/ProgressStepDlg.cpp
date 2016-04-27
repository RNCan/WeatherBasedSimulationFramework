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
#include "ProgressStepDlg.h"
#include "Basic/UtilStd.h"
#include "WeatherBasedSimulationString.h"
#include "WeatherBasedSimulationUI.h"


using namespace std;


  
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



BEGIN_MESSAGE_MAP(CProgressStepDlg, CDialog)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_DESTROY()
END_MESSAGE_MAP()


CProgressStepDlg::CProgressStepDlg(bool bShowPause, bool bShowMinimize)
{
	m_bShowMinimize = bShowMinimize;

	m_ptrThread=NULL;
	m_bIsThreadSuspended=false;
	m_bIsIconic = false;
}



void CProgressStepDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CMN_STEP_PROGRESS, m_progressCtrl);
    DDX_Control(pDX, IDC_CMN_STEP_MESSAGE, m_messageCtrl);
	
	if (!pDX->m_bSaveAndValidate)
	{
		m_progressCtrl.SetMessageCtrl(&m_messageCtrl);
	}

	
}



int CProgressStepDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDialog::OnCreate(lpCreateStruct) == -1)
		return -1;

	CWnd* pMain = ::AfxGetMainWnd();
	if (pMain)
	{
		CString tmp;
		pMain->GetWindowText(tmp);
		m_title = CStringA(tmp);
	}

	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CProgressStepDlg message handlers


void CProgressStepDlg::UpdateMainWindowText()
{
	CWnd* pMain = ::AfxGetMainWnd();
	
    if(  pMain)
	{
		if( m_bIsIconic )
		{
			string stepPourcentage = WBSF::FormatA("%3d%%", m_progressCtrl.GetCallback().GetCurrentStepPercent());
			pMain->SetWindowTextW(CString(stepPourcentage.c_str()));
		}
		else
		{
			CStringW title;
			pMain->GetWindowTextW(title);
			if( title != CStringW(m_title.c_str()) )
				pMain->SetWindowText(CString(m_title.c_str()));
		}
	}
}

void CProgressStepDlg::OnCancel() 
{
	//GetDlgItem(IDCANCEL)->EnableWindow(FALSE);

	// Let thread know about this event
	//m_callback.SetUserCancel();

	// thread may be suspended, so resume before shutting down
	if( m_ptrThread )
	{
		m_ptrThread->ResumeThread();
		CWaitCursor cursor;
		// Wait till target thread responds
		if( WaitForSingleObject(m_ptrThread->m_hThread, 60000) == WAIT_TIMEOUT)
		{
			//if the thread does'nt respons in 20 second we ast to the user
			if (MessageBoxW(CString(WBSF::GetString(IDS_CMN_NOT_RESPONDING).c_str()), 0, MB_OKCANCEL) == IDOK)
			{
				//try agait to see if responding
				if( WaitForSingleObject(m_ptrThread->m_hThread, 1000) == WAIT_TIMEOUT)
				{
					ASSERT( m_ptrThread->m_hThread != NULL);
					//TerminateThread(m_ptrThread->m_hThread, -2);
					::AfxGetMainWnd()->SendMessageW( WM_COMMAND,  MAKEWPARAM (ID_FILE_SAVE, 1), 0L );
					::ExitProcess(-1);
				}
			}
			else
			{
				GetDlgItem(IDCANCEL)->EnableWindow(TRUE);
			}
		}
	}
}

void CProgressStepDlg::OnOK() 
{
}



BOOL CProgressStepDlg::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{

	if (m_messageCtrl.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
		return TRUE;

	if (m_progressCtrl.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
		return TRUE;


	return CDialog::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

BOOL CProgressStepDlg::Create(CWnd* pParentWnd) 
{
	m_pParentWnd = pParentWnd;
	BOOL bRep = CDialog::Create(IDD_CMN_PROGRESS_STEP, m_pParentWnd);
	if( bRep )
	{
		if(m_bShowMinimize)
			ModifyStyle(0, WS_MINIMIZEBOX, 0 );
	}

	return bRep;
}



void CProgressStepDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);
	AdjustLayout();

}

void CProgressStepDlg::AdjustLayout()
{
	if (GetSafeHwnd() == NULL || m_progressCtrl.GetSafeHwnd() == NULL)
	{
		return;
	}

	CRect rect;
	GetClientRect(rect); 

	static const int MARGE = 10;
	rect.top += MARGE;
	rect.left += MARGE;
	rect.bottom -= 2 * MARGE;
	rect.right -= 2 * MARGE;

	m_progressCtrl.SetWindowPos(NULL, rect.left, rect.top, rect.Width(), rect.Height() / 2, SWP_NOACTIVATE | SWP_NOZORDER );
	m_messageCtrl.SetWindowPos(NULL, rect.left, rect.top+ rect.Height() / 2, rect.Width(), rect.Height() / 2, SWP_NOACTIVATE | SWP_NOZORDER);

}

ERMsg CProgressStepDlg::Execute(AFX_THREADPROC pfnThreadProc, CProgressStepDlgParam* pParam)
{
	return m_progressCtrl.Execute(pfnThreadProc, pParam);

}
