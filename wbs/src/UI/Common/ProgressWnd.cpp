
#include "stdafx.h"
#include "ProgressWnd.h"
#include "WeatherBasedSimulationUI.h"
#include "WeatherBasedSimulationString.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif



//******************************************************************************************************************************

static const UINT ID_PROGRESS_CTRL = 1001;


IMPLEMENT_DYNCREATE(CProgressWnd, CWnd)
BEGIN_MESSAGE_MAP(CProgressWnd, CWnd)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_TIMER()
	ON_NOTIFY(HDN_ITEMCHANGED, 0, OnItemHeadChanged)
	ON_UPDATE_COMMAND_UI_RANGE(ID_CMN_CANCEL, ID_CMN_PAUSE_RESUME, OnUpdateToolbar)
	ON_COMMAND(ID_CMN_CANCEL, OnCancel)
	ON_COMMAND(ID_CMN_PAUSE_RESUME, OnPauseResume)
	ON_MESSAGE(WM_MY_THREAD_MESSAGE, OnThreadMessage)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CProgressWnd

CProgressWnd::CProgressWnd():
	m_ptrThread(NULL),
	m_pEdit(NULL)
{
}

CProgressWnd::~CProgressWnd()
{
}

/////////////////////////////////////////////////////////////////////////////
// CWorkspaceBar message handlers

int CProgressWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;


	if (!m_progressListCtrl.Create(WS_CHILD | WS_VISIBLE | WS_BORDER |LVS_REPORT | LVS_NOSORTHEADER | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, CRect(), this, ID_PROGRESS_CTRL))
		return -1;


	CStringArrayEx str(IDS_CMN_PROG_HEADER, _T("|;"));
	m_progressListCtrl.InsertColumn(0, str[0], LVCFMT_LEFT, 200);
	m_progressListCtrl.InsertColumn(1, str[1], LVCFMT_LEFT);
	
	
	// Load view images:
	VERIFY(m_toolbarCtrl.Create(this, AFX_DEFAULT_TOOLBAR_STYLE, IDR_CMN_PROGRESS_TOOLBAR));
	VERIFY(m_toolbarCtrl.LoadToolBar(IDR_CMN_PROGRESS_TOOLBAR, 0, 0, TRUE /* Is locked */));
	m_toolbarCtrl.CleanUpLockedImages();//????
	m_toolbarCtrl.LoadBitmap(IDR_CMN_PROGRESS_TOOLBAR, 0, 0, TRUE /* Locked */);
	m_toolbarCtrl.SetPaneStyle(m_toolbarCtrl.GetPaneStyle() | CBRS_TOOLTIPS | CBRS_FLYBY);
	m_toolbarCtrl.SetPaneStyle(m_toolbarCtrl.GetPaneStyle() & ~(CBRS_GRIPPER | CBRS_SIZE_DYNAMIC | CBRS_BORDER_TOP | CBRS_BORDER_BOTTOM | CBRS_BORDER_LEFT | CBRS_BORDER_RIGHT));
	m_toolbarCtrl.SetOwner(this);
	m_toolbarCtrl.SetRouteCommandsViaFrame(FALSE);

	return 0;
}


void CProgressWnd::PreSubclassWindow()
{
	CWnd::PreSubclassWindow();

	_AFX_THREAD_STATE* pThreadState = AfxGetThreadState();
	if (pThreadState->m_pWndInit == NULL)
	{ 
		VERIFY(m_progressListCtrl.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | LVS_NOSORTHEADER | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, CRect(), this, ID_PROGRESS_CTRL));

		CStringArrayEx str(IDS_CMN_PROG_HEADER, _T("|;"));
		m_progressListCtrl.InsertColumn(0, str[0], LVCFMT_LEFT, 200);
		m_progressListCtrl.InsertColumn(1, str[1], LVCFMT_LEFT);

		// Load view images:
		VERIFY(m_toolbarCtrl.Create(this, AFX_DEFAULT_TOOLBAR_STYLE, IDR_CMN_PROGRESS_TOOLBAR));
		VERIFY(m_toolbarCtrl.LoadToolBar(IDR_CMN_PROGRESS_TOOLBAR, 0, 0, TRUE /* Is locked */));
		m_toolbarCtrl.CleanUpLockedImages();//????
		m_toolbarCtrl.LoadBitmap(IDR_CMN_PROGRESS_TOOLBAR, 0, 0, TRUE /* Locked */);
		m_toolbarCtrl.SetPaneStyle(m_toolbarCtrl.GetPaneStyle() | CBRS_TOOLTIPS | CBRS_FLYBY);
		m_toolbarCtrl.SetPaneStyle(m_toolbarCtrl.GetPaneStyle() & ~(CBRS_GRIPPER | CBRS_SIZE_DYNAMIC | CBRS_BORDER_TOP | CBRS_BORDER_BOTTOM | CBRS_BORDER_LEFT | CBRS_BORDER_RIGHT));
		m_toolbarCtrl.SetOwner(this);
		m_toolbarCtrl.SetRouteCommandsViaFrame(FALSE);
	}
}

void CProgressWnd::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);
	AdjustLayout();
}



void CProgressWnd::AdjustLayout()
{
	if (GetSafeHwnd() == NULL || m_toolbarCtrl.GetSafeHwnd()==NULL )
		return;

	CRect rect;
	GetClientRect(rect);

	int cyTlb = m_toolbarCtrl.CalcFixedLayout(FALSE, TRUE).cy;
		
	m_toolbarCtrl.SetWindowPos(NULL, rect.left, rect.top, rect.Width(), cyTlb, SWP_NOACTIVATE | SWP_NOZORDER );
	m_progressListCtrl.SetWindowPos(NULL, rect.left, rect.top + cyTlb, rect.Width(), rect.Height() - cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);
	m_progressListCtrl.SetColumnWidth(1, rect.Width() - m_progressListCtrl.GetColumnWidth(0));

}


void CProgressWnd::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == 1)
	{
		if (m_progressListCtrl.GetItemCount() > 0)
		{
			//get the last progress bar
			DWORD_PTR pItem = m_progressListCtrl.GetItemData(m_progressListCtrl.GetItemCount() - 1);
			if (pItem != NULL)
			{
				CProgressCtrl* pCtrl = (CProgressCtrl*)(pItem);
				if (pCtrl && pCtrl->GetSafeHwnd())
				{
					if (m_callback.GetNbStep() > 0)
						pCtrl->SetPos((int)m_callback.GetCurrentStepPercent());

					CWnd* pMain = ::AfxGetMainWnd();
					if (m_pTaskbar && pMain)
						m_pTaskbar->SetProgressValue(pMain->GetSafeHwnd(), (int)m_callback.GetCurrentStepPercent(), 100);
				}
			}
		}
	}
	

	CWnd::OnTimer(nIDEvent);
	
}



void CProgressWnd::OnItemHeadChanged(NMHDR* pNMHDR, LRESULT* pResult)
{
	HD_NOTIFY *phdn = (HD_NOTIFY *)pNMHDR;
	// TODO: Add your control notification handler code here
	*pResult = 0;
	if (phdn->iItem != 1)
		return;

	m_progressListCtrl.InvalidateProgressCtrls();
}

void CProgressWnd::OnUpdateToolbar(CCmdUI* pCmdUI)
{
	//switch (pCmdUI->m_nID)
	//{
	//case ID_CMN_CANCEL:pCmdUI->Enable(!m_callback.GetTasks().empty() && !m_callback.GetUserCancel()); break;
	//case ID_CMN_PAUSE_RESUME: break;
	//}
}

void CProgressWnd::OnCancel()
{
	m_callback.SetUserCancel();
}


void CProgressWnd::OnPauseResume()
{
	WBSF::StringVector title(WBSF::GetString(IDS_CMN_PAUSE), ",;|");
	ASSERT(title.size() == 2);

	//if (!m_bIsThreadSuspended)
	//{
	//	m_bIsThreadSuspended = true;
	//	//m_callback.m_bPause.ResetEvent();
	//	m_callback.SetPause(true);
	//	m_ptrThread->SuspendThread();

	//	GetDlgItem(IDCANCEL)->EnableWindow(FALSE);

	//	m_pauseCtrl.SetWindowText(CString(title[1].c_str()));

	//	CWnd* pMain = ::AfxGetMainWnd();
	//	if (m_pTaskbar && pMain)
	//		m_pTaskbar->SetProgressState(pMain->GetSafeHwnd(), TBPF_PAUSED);
	//}
	//else
	//{
	//	m_bIsThreadSuspended = false;
	//	GetDlgItem(IDCANCEL)->EnableWindow();
	//	m_pauseCtrl.SetWindowText(CString(title[0].c_str()));

	//	CWnd* pMain = ::AfxGetMainWnd();
	//	if (m_pTaskbar && pMain)
	//		m_pTaskbar->SetProgressState(pMain->GetSafeHwnd(), TBPF_NORMAL);

	//	m_ptrThread->ResumeThread();
	//	//m_callback.m_bPause.SetEvent();
	//	m_callback.SetPause(false);
	//}
}

LRESULT CProgressWnd::OnThreadMessage(WPARAM t, LPARAM)
{
	
	//m_callback.Lock();
	if (t == 0)
	{
		const std::string& message = m_callback.GetMessages();
		if (!message.empty())
		{

			std::string tmp = message.data();
			std::remove(tmp.begin(), tmp.end(), '\r');
			WBSF::ReplaceString(tmp, "\n", "\r\n");

			m_comment += tmp;
			
			if (m_pEdit)
			{
				m_pEdit->SetWindowTextW(CString(m_comment.c_str()));
				m_pEdit->SetSel((int)m_comment.length(), -1);
			}
				
			m_callback.DeleteMessages(false);
		}
	}
	else if (t == 1)
	{
		const WBSF::CCallbackTask& task = m_callback.GetTasks().top();// .c[m_progressCtrl.GetItemCount()];
		m_progressListCtrl.InsertItem(m_progressListCtrl.GetItemCount(), CString(task.m_description.c_str()), task.m_nbSteps > 0);
	}
	else if (t == 2)
	{
		if (m_progressListCtrl.GetItemCount() > 0)
			m_progressListCtrl.DeleteItem(m_progressListCtrl.GetItemCount() - 1);
	}

	//update text
	//m_callback.Unlock();
	

	return 0;
}

ERMsg CProgressWnd::Execute(AFX_THREADPROC pfnThreadProc, CProgressStepDlgParam* pParam)
{
	//ASSERT(GetSafeHwnd());

	ERMsg msg;

	AdjustLayout();

	m_callback.Reset();
	m_callback.SetWnd(&m_hWnd);
	if (m_pEdit)
		m_pEdit->SetWindowText(NULL);

	//prepare callback and message
	pParam->m_pCallback = &m_callback;
	pParam->m_pMsg = &msg;

	if (GetSafeHwnd())
		SetTimer(1, 200, NULL);

	//create thread 
	m_ptrThread = AfxBeginThread(pfnThreadProc, pParam, 0, 0, CREATE_SUSPENDED);
	ASSERT(m_ptrThread);

	m_ptrThread->m_bAutoDelete = FALSE;//don't delete thread at exit
	m_ptrThread->ResumeThread();//start thread

	//wait until task finish
	while (WaitForSingleObject(m_ptrThread->m_hThread, 50) == WAIT_TIMEOUT)
	{
		MSG winMsg;
		while (PeekMessage((LPMSG)&winMsg, NULL, 0, 0, PM_REMOVE))
		{
			if ((winMsg.message != WM_QUIT)
				&& (winMsg.message != WM_CLOSE)
				&& (winMsg.message != WM_DESTROY)
				&& (winMsg.message != WM_NCDESTROY)
				&& (winMsg.message != WM_HSCROLL)
				&& (winMsg.message != WM_VSCROLL))
			{
				TranslateMessage((LPMSG)&winMsg);
				DispatchMessage((LPMSG)&winMsg);
			}
		}
	} 

	if (GetSafeHwnd())
		KillTimer(1);

	//clean callback
	while (!m_callback.GetTasks().empty())
		m_callback.PopTask();
	
	while (GetSafeHwnd() && m_progressListCtrl.GetItemCount()>0)
		m_progressListCtrl.DeleteItem(m_progressListCtrl.GetItemCount() - 1);
		
	//m_progressListCtrl.Invalidate();
	
	//clean up memory
	delete m_ptrThread;
	m_ptrThread = NULL;

	
	//update title 
	CWnd* pMain = ::AfxGetMainWnd();
	

	if (m_pTaskbar && pMain)
		m_pTaskbar->SetProgressState(pMain->GetSafeHwnd(), TBPF_NOPROGRESS);


	return msg;
}


