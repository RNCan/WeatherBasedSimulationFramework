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



CProgressStepDlg::CProgressStepDlg(CWnd* pParent, bool bShowPause, bool bShowMinimize)
{
	m_pParentWnd = pParent;
	m_bShowPause = bShowPause;
	m_bShowMinimize = bShowMinimize;

	m_nCurrentTask=-1;
	m_nCurrentNbTasks=-1;
	m_nCurrentStepPos = 101;
	m_ptrThread=NULL;
	m_bIsThreadSuspended=false;
	m_bIsIconic = false;

	m_callback.SetUserCancelMessage(WBSF::GetString(IDS_BSC_USER_BREAK));
}



void CProgressStepDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CMN_STEP_PROGRESS, m_progressCtrl);
    DDX_Control(pDX, IDC_CMN_STEP_MESSAGE, m_messageCtrl);
    DDX_Control(pDX, IDC_CMN_STEP_POURCENTAGE, m_pourcentageCtrl);
    DDX_Control(pDX, IDC_CMN_STEP_DESCRIPTION, m_descriptionCtrl);
    DDX_Control(pDX, IDC_CMN_STEPNO, m_stepNoCtrl);
   // DDX_Control(pDX, IDC_CMN_STEP_BITMAP, m_bitmap);
	DDX_Control(pDX, IDCANCEL, m_cancelCtrl);
	DDX_Control(pDX, IDC_CMN_PAUSE, m_pauseCtrl);

	m_progressCtrl.SetRange(0, 100);
}



BEGIN_MESSAGE_MAP(CProgressStepDlg, CDialog)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_DESTROY()
	ON_MESSAGE(WM_MY_THREAD_MESSAGE, OnThreadMessage)
	ON_BN_CLICKED(IDC_CMN_PAUSE, &OnBnClickedPause)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CProgressStepDlg message handlers

ERMsg CProgressStepDlg::Execute(AFX_THREADPROC pfnThreadProc, CProgressStepDlgParam* pParam)
{
	ERMsg msg;

	//create windows
	ENSURE( Create(m_pParentWnd) );

	//prepare callback and message
	pParam->m_pCallback = &m_callback;
	pParam->m_pMsg = &msg;

	//create thread 
	m_ptrThread = AfxBeginThread(pfnThreadProc, pParam, 0, 0, CREATE_SUSPENDED);
	ASSERT( m_ptrThread );

	m_ptrThread->m_bAutoDelete=FALSE;//don't delete thread at exit
	m_ptrThread->ResumeThread();//start thread

	//wait 0.5 seconds for very short task
	if( WaitForSingleObject(m_ptrThread->m_hThread,500)== WAIT_TIMEOUT)
	{
		//if the task is not finish, show progress bar dialog 
		ShowWindow(SW_SHOW);

		//wait until task finish
		while(WaitForSingleObject(m_ptrThread->m_hThread,50) == WAIT_TIMEOUT)
			PumpMessage();
	}

	//clean up memory
	delete m_ptrThread;
	m_ptrThread=NULL;

	//destroy window and return message
	DestroyWindow();

	//update title 
	CWnd* pMain = ::AfxGetMainWnd();
	CString tmp;
	pMain->SetWindowText(CString(m_title.c_str()));
	
	if ( m_pTaskbar && pMain)
		m_pTaskbar->SetProgressState( pMain->GetSafeHwnd(), TBPF_NOPROGRESS );

	return msg;
}

void CProgressStepDlg::UpdateCtrl() 
{
	//const string& description = m_callback.GetCurrentTaskMessageText();
	//if ( !description.empty())
	//{
		//m_lastDescription = description;
		//m_descriptionCtrl.SetWindowTextW(CString(description.c_str()));
		//m_callback.SetCurrentDescription("");//delete description
	//}

    const string& message = m_callback.GetMessages();
    if( !message.empty() )
    {
		string tmp = message.data();
		std::remove(tmp.begin(), tmp.end(), '\r');
		WBSF::ReplaceString(tmp, "\n", "\r\n"); 
        
		m_comment += tmp;
        m_messageCtrl.SetWindowTextW( CString(m_comment.c_str()) );
        m_messageCtrl.SetSel( (int)m_comment.length(), -1 );
		m_callback.DeleteMessages();
    }

	CWnd* pMain = ::AfxGetMainWnd();
	bool bIsIconic = pMain?pMain->IsIconic()!=0:false;

	if (m_callback.GetCurrentStepPercent() < m_nCurrentStepPos)
	{
		//Set m_nCurrentStepPos to be shure to update it
		m_nCurrentStepPos = -1;
	}

	string stepPourcentage;
	if (m_callback.GetCurrentStepPercent() != m_nCurrentStepPos ||
		bIsIconic != m_bIsIconic)
	{
		//string stepNo = "?/?";
		//if (m_callback.GetCurrentTaskNo() != -1)
			//stepNo = WBSF::FormatA("%d/%d", m_callback.GetCurrentTaskNo() + 1, m_callback.GetNbTask());

		//m_stepNoCtrl.SetWindowTextW(CString(stepNo.c_str()));

		m_bIsIconic = bIsIconic;
		//get current step pourcent
		m_nCurrentStepPos = (int)m_callback.GetCurrentStepPercent();

		//update progres bar
		m_progressCtrl.SetPos(m_nCurrentStepPos);
		//update taskbar progress if available
		if (m_pTaskbar && pMain)
			m_pTaskbar->SetProgressValue(pMain->GetSafeHwnd(), m_nCurrentStepPos, 100);

		//update pourcent text
		stepPourcentage = WBSF::FormatA("%3d%%", m_nCurrentStepPos);
		m_pourcentageCtrl.SetWindowTextW(CString(stepPourcentage.c_str()));
		UpdateMainWindowText();
	}

	//if (m_nCurrentTask == -1 || m_nCurrentNbTasks == -1 || 
	//	m_nCurrentTask != m_callback.GetCurrentTaskNo() || m_nCurrentNbTasks != m_callback.GetNbTask())
	//{
		
		//update window title
		//if( pMain )
		//{
		//	if( m_bIsIconic )
		//	{
		//		string  stepNo = FormatA("%d/%d", m_callback.GetCurrentTaskNo()+1, m_callback.GetNbTask() );
		//		pMain->SetWindowText( UtilWin::ToUTF16(stepNo + " : " + stepPourcentage) );
		//	}
		//	else 
		//	{
		//		pMain->SetWindowTextW(UtilWin::ToUTF16(m_title));
		//	}
		//}
		//
		//m_nCurrentTask = m_callback.GetCurrentTaskNo();
		//m_nCurrentNbTasks = m_callback.GetNbTask();
//	}
}

void CProgressStepDlg::UpdateMainWindowText()
{
	CWnd* pMain = ::AfxGetMainWnd();
	
    if(  pMain)
	{
		if( m_bIsIconic )
		{
			//string stepNo = WBSF::FormatA("%d/%d", m_callback.GetCurrentTaskNo() + 1, m_callback.GetNbTask());
			string stepPourcentage = WBSF::FormatA("%3d%%", m_nCurrentStepPos);
			//string str = stepNo + " : " + stepPourcentage;
			pMain->SetWindowTextW(CString(stepPourcentage.c_str()));
		}
		else
		{
			CStringW title;
			pMain->GetWindowTextW(title);
			if( title != CStringW(m_title.c_str()) )
				pMain->SetWindowText(CString(m_title.c_str()));
		}
		
		//m_nCurrentTask = m_callback.GetCurrentTaskNo();
		//m_nCurrentNbTasks = m_callback.GetNbTask();
	}
}

void CProgressStepDlg::OnCancel() 
{
	GetDlgItem(IDCANCEL)->EnableWindow(FALSE);

	// Let thread know about this event
	m_callback.SetUserCancel();

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

LRESULT CProgressStepDlg::OnThreadMessage(WPARAM, LPARAM)
{
	UpdateCtrl();
	return 0;
}

void CProgressStepDlg::PostNcDestroy() 
{
	CDialog::PostNcDestroy();
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

void CProgressStepDlg::GetMessageArray(CStringArray& messageArray)
{
    messageArray.RemoveAll();

    CString message;
    GetMessage(message);

    while( !message.IsEmpty() )
    {
        int pos = message.Find(_T("\r\n"));
        CString tmp = message.Left(pos);
        if( pos + 2 > message.GetLength() )
            pos = message.GetLength();
        else pos+=2;

        message = message.Mid(pos);

        messageArray.Add(tmp);
    }
}

void CProgressStepDlg::GetMessage(CString& message)
{
    m_comment += m_callback.GetMessages().data();
    m_callback.DeleteMessages();

    if( m_messageCtrl.m_hWnd != NULL)
        m_messageCtrl.SetWindowText(CString(m_comment.c_str()));

    message = CString(m_comment.c_str());
}

BOOL CProgressStepDlg::Create(CWnd* pParentWnd) 
{
	m_pParentWnd = pParentWnd;
	BOOL bRep = CDialog::Create(IDD_CMN_PROGRESS_STEP, m_pParentWnd);
	if( bRep )
	{
		//associate the callback to this windows
		m_callback.SetWnd(&m_hWnd);

		if( m_bShowPause  )
			GetDlgItem(IDC_CMN_PAUSE)->ShowWindow(SW_SHOW);
	
		if(m_bShowMinimize)
			ModifyStyle(0, WS_MINIMIZEBOX, 0 );
	}

	return bRep;
}


UINT ProcessMessage(void* pParam)
{
	CProgressStepDlg* pThis = (CProgressStepDlg*)pParam;

	CWinThread* pTest = ::AfxGetThread();
	if( WaitForSingleObject(::AfxGetThread()->m_hThread,500)== WAIT_TIMEOUT)
	{
		//Sleep(1000);
		//if the task is not finish, show progress bar dialog 
		pThis->ShowWindow(SW_SHOW);

		//wait until task finish
		while(WaitForSingleObject(::AfxGetThread()->m_hThread,50) == WAIT_TIMEOUT)
		//while(1)
		{
			pThis->PumpMessage();
			PostMessage(pThis->m_hWnd, WM_MY_THREAD_MESSAGE, 0, 0);
			Sleep(100);
		}
	}
	
	return 0;
}

BOOL CProgressStepDlg::Create() 
{
	
	BOOL bRep = Create(m_pParentWnd);
	
	ShowWindow(SW_SHOW);

	//must process message when single thread
	m_callback.SetPumpMessage(true);
	//create thread 
	//m_ptrThread = AfxBeginThread(&ProcessMessage, this, 0, 0, CREATE_SUSPENDED);
	//ASSERT( m_ptrThread );

	//m_ptrThread->m_bAutoDelete=FALSE;//don't delete thread at exit
	//m_ptrThread->ResumeThread();//start thread
	

	return bRep;
}

void CProgressStepDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);
	AdjustLayout();
	// TODO: Add your message handler code here
}

void CProgressStepDlg::AdjustLayout()
{
	if( !GetSafeHwnd() || !m_messageCtrl.GetSafeHwnd())
		return;

	
	CRect rectClient;
	GetClientRect(rectClient);
	long cx = rectClient.Width();
	long cy = rectClient.Height();

	CRect rect;

	m_messageCtrl.GetWindowRect(rect);
	ScreenToClient(rect);
	rect.right = max(rect.left+100, cx-9);
	rect.bottom = max(rect.top+100, cy-9);
	m_messageCtrl.MoveWindow( rect );       

	
	m_descriptionCtrl.GetWindowRect(rect);
	ScreenToClient(rect);
	rect.right = max(rect.left+100, cx-9);
	m_descriptionCtrl.MoveWindow( rect );       
	
	m_progressCtrl.GetWindowRect(rect);
	ScreenToClient(rect);
	rect.right = max(rect.left+100, cx-9);
	m_progressCtrl.MoveWindow( rect );       

	m_pourcentageCtrl.GetWindowRect(rect);
	ScreenToClient(rect);
	rect.right = max(rect.left+100, cx-9);
	m_pourcentageCtrl.MoveWindow( rect );       

	CRect stepNoRect;
	m_stepNoCtrl.GetWindowRect(stepNoRect);
	ScreenToClient(stepNoRect);

	m_cancelCtrl.GetWindowRect(rect);
	ScreenToClient(rect);
	int width = rect.Width();
	rect.left = max(stepNoRect.right+9, cx-width-9);
	rect.right = rect.left+width;
	m_cancelCtrl.MoveWindow( rect );       

	//m_pauseCtrl.GetWindowRect(rect);
	//ScreenToClient(rect);
	//width = rect.Width();
	rect.left -= width+9;
	rect.right -= width+9;
	m_pauseCtrl.MoveWindow( rect );       
}


void CProgressStepDlg::OnDestroy()
{
	if( m_ptrThread )
	{
		TerminateThread(m_ptrThread->m_hThread, -2);
		delete m_ptrThread;
		m_ptrThread=NULL;
	}
	//m_pTaskbar.Release();
	CDialog::OnDestroy();
}

void CProgressStepDlg::OnBnClickedPause()
{
	WBSF::StringVector title(WBSF::GetString(IDS_CMN_PAUSE), ",;|");
	ASSERT( title.size() == 2);

	if( ! m_bIsThreadSuspended )
	{
		m_bIsThreadSuspended = true;
		//m_callback.m_bPause.ResetEvent();
		m_callback.SetPause(true);
		m_ptrThread->SuspendThread();

		GetDlgItem(IDCANCEL)->EnableWindow(FALSE);

		m_pauseCtrl.SetWindowText(CString(title[1].c_str()));

		CWnd* pMain = ::AfxGetMainWnd();
		if ( m_pTaskbar && pMain)
			m_pTaskbar->SetProgressState( pMain->GetSafeHwnd(), TBPF_PAUSED );
	}
	else
	{
		m_bIsThreadSuspended = false;
		GetDlgItem(IDCANCEL)->EnableWindow();
		m_pauseCtrl.SetWindowText(CString(title[0].c_str()));

		CWnd* pMain = ::AfxGetMainWnd();
		if ( m_pTaskbar && pMain)
			m_pTaskbar->SetProgressState( pMain->GetSafeHwnd(), TBPF_NORMAL );

		m_ptrThread->ResumeThread();
		//m_callback.m_bPause.SetEvent();
		m_callback.SetPause(false);
	}
}

void CProgressStepDlg::PumpMessage(void)
{
//	if( m_pLastDlg )
	//	m_pLastDlg->UpdateCtrl();
	
	MSG msg;
	while (PeekMessage( (LPMSG) &msg, NULL, 0, 0, PM_REMOVE) ) 
	{ 
		TranslateMessage( (LPMSG) &msg ); 
		DispatchMessage( (LPMSG) &msg ); 
	} 
}
