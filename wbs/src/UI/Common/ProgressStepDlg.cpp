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
//	m_bShowPause = bShowPause;
	m_bShowMinimize = bShowMinimize;

	m_ptrThread=NULL;
	m_bIsThreadSuspended=false;
	m_bIsIconic = false;

	//m_callback.SetUserCancelMessage(WBSF::GetString(IDS_BSC_USER_BREAK));
}



void CProgressStepDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CMN_STEP_PROGRESS, m_progressCtrl);
    DDX_Control(pDX, IDC_CMN_STEP_MESSAGE, m_messageCtrl);
	//DDX_Control(pDX, IDCANCEL, m_cancelCtrl);
	//DDX_Control(pDX, IDC_CMN_PAUSE, m_pauseCtrl);
	
	if (!pDX->m_bSaveAndValidate)
	{
		m_progressCtrl.SetMessageCtrl(&m_messageCtrl);
	}

	//m_progressCtrl.SetRange(0, 100);
}



int CProgressStepDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDialog::OnCreate(lpCreateStruct) == -1)
		return -1;

	//VERIFY(m_messageCtrl.Create(WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | WS_HSCROLL | WS_VSCROLL | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, CRect(), this, IDC_CMN_STEP_MESSAGE));
	//VERIFY(m_progressCtrl.Create(NULL, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, CRect(), this, IDC_CMN_STEP_PROGRESS));

	//m_progressCtrl.SetMessageCtrl(&m_messageCtrl);

	//m_cancelCtrl.Create("Cancel", , IDCANCEL);
	//m_pauseCtrl.Create(IDC_CMN_PAUSE);

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

//LRESULT CProgressStepDlg::OnThreadMessage(WPARAM, LPARAM)
//{
//	UpdateCtrl();
//	return 0;
//}
//
//void CProgressStepDlg::GetMessageArray(CStringArray& messageArray)
//{
//    messageArray.RemoveAll();
//
//    CString message;
//    GetMessage(message);
//
//    while( !message.IsEmpty() )
//    {
//        int pos = message.Find(_T("\r\n"));
//        CString tmp = message.Left(pos);
//        if( pos + 2 > message.GetLength() )
//            pos = message.GetLength();
//        else 
//			pos+=2;
//
//        message = message.Mid(pos);
//
//        messageArray.Add(tmp);
//    }
//}
//
//void CProgressStepDlg::GetMessage(CString& message)
//{
//	//m_comment += m_progressCtrl.GetCallback().GetMessages().data();
// //   m_callback.DeleteMessages();
//
// //   if( m_messageCtrl.m_hWnd != NULL)
// //       m_messageCtrl.SetWindowText(CString(m_comment.c_str()));
//
// //   message = CString(m_comment.c_str());
//}

BOOL CProgressStepDlg::Create(CWnd* pParentWnd) 
{
	m_pParentWnd = pParentWnd;
	BOOL bRep = CDialog::Create(IDD_CMN_PROGRESS_STEP, m_pParentWnd);
	if( bRep )
	{
		//associate the callback to this windows
		//m_callback.SetWnd(&m_hWnd);

		//if( m_bShowPause  )
			//GetDlgItem(IDC_CMN_PAUSE)->ShowWindow(SW_SHOW);
	
		if(m_bShowMinimize)
			ModifyStyle(0, WS_MINIMIZEBOX, 0 );
	}

	//ShowWindow(SW_SHOW);

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
//
//void CProgressStepDlg::OnBnClickedPause()
//{
////	WBSF::StringVector title(WBSF::GetString(IDS_CMN_PAUSE), ",;|");
////	ASSERT( title.size() == 2);
////
////	if( ! m_bIsThreadSuspended )
////	{
////		m_bIsThreadSuspended = true;
////		//m_callback.m_bPause.ResetEvent();
////		m_callback.SetPause(true);
////		m_ptrThread->SuspendThread();
////
////		GetDlgItem(IDCANCEL)->EnableWindow(FALSE);
////
//////		m_pauseCtrl.SetWindowText(CString(title[1].c_str()));
////
////		CWnd* pMain = ::AfxGetMainWnd();
////		if ( m_pTaskbar && pMain)
////			m_pTaskbar->SetProgressState( pMain->GetSafeHwnd(), TBPF_PAUSED );
////	}
////	else
////	{
////		m_bIsThreadSuspended = false;
////		GetDlgItem(IDCANCEL)->EnableWindow();
////		//m_pauseCtrl.SetWindowText(CString(title[0].c_str()));
////
////		CWnd* pMain = ::AfxGetMainWnd();
////		if ( m_pTaskbar && pMain)
////			m_pTaskbar->SetProgressState( pMain->GetSafeHwnd(), TBPF_NORMAL );
////
////		m_ptrThread->ResumeThread();
////		//m_callback.m_bPause.SetEvent();
////		m_callback.SetPause(false);
////	}
//}
//

ERMsg CProgressStepDlg::Execute(AFX_THREADPROC pfnThreadProc, CProgressStepDlgParam* pParam)
{
	return m_progressCtrl.Execute(pfnThreadProc, pParam);

	//prepare callback and message
	//pParam->m_pCallback = &m_callback;
	//pParam->m_pMsg = &msg;

	////create thread 
	//m_ptrThread = AfxBeginThread(pfnThreadProc, pParam, 0, 0, CREATE_SUSPENDED);
	//ASSERT(m_ptrThread);

	//m_ptrThread->m_bAutoDelete = FALSE;//don't delete thread at exit
	//m_ptrThread->ResumeThread();//start thread

	//	
	////wait until task finish
	//while (WaitForSingleObject(m_ptrThread->m_hThread, 50) == WAIT_TIMEOUT)
	//{
	//	MSG winMsg;
	//	while (PeekMessage((LPMSG)&winMsg, NULL, 0, 0, PM_REMOVE))
	//	{
	//		if ((winMsg.message != WM_QUIT)
	//			&& (winMsg.message != WM_CLOSE)
	//			&& (winMsg.message != WM_DESTROY)
	//			&& (winMsg.message != WM_NCDESTROY)
	//			&& (winMsg.message != WM_HSCROLL)
	//			&& (winMsg.message != WM_VSCROLL))
	//		{
	//			TranslateMessage((LPMSG)&winMsg);
	//			DispatchMessage((LPMSG)&winMsg);
	//		}
	//	}
	//}


	////clean up memory
	//delete m_ptrThread;
	//m_ptrThread = NULL;

	//
	//CWnd* pMain = ::AfxGetMainWnd();
	////if (pMain)
	////	pMain->SetWindowText(CString(m_title.c_str()));//update title 

	//if (m_pTaskbar && pMain)
	//	m_pTaskbar->SetProgressState(pMain->GetSafeHwnd(), TBPF_NOPROGRESS);


}
