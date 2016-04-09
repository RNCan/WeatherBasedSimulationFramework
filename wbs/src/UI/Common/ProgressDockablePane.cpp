
#include "stdafx.h"
#include "ProgressDockablePane.h"
#include "WeatherBasedSimulationUI.h"
#include "WeatherBasedSimulationString.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


static const UINT ID_MESSAGE_CTRL = 1000;
/////////////////////////////////////////////////////////////////////////////
// CProgressDockablePane

CProgressDockablePane::CProgressDockablePane():
	m_ptrThread(NULL)
{
}

CProgressDockablePane::~CProgressDockablePane()
{
}

BEGIN_MESSAGE_MAP(CProgressDockablePane, CDockablePane)
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
// CWorkspaceBar message handlers

int CProgressDockablePane::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	CRect rectDummy;
	rectDummy.SetRectEmpty();

	// Create view:
	DWORD dwStyleEx = 0;
	m_wndSplitter.CreateStatic(this, 2, 1);

	if (!m_wndSplitter.AddWindow(0, 0, &m_progressCtrl, WC_LISTVIEW, WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_NOSORTHEADER, dwStyleEx, CSize(100, 100)))
		return -1;

	if (!m_wndSplitter.AddWindow(1, 0, &m_messageCtrl, WC_EDIT, WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_WANTRETURN | ES_AUTOVSCROLL | ES_AUTOHSCROLL | WS_VSCROLL | WS_HSCROLL, dwStyleEx, CSize(100, 100)))
		return -1;



	//VERIFY(m_progressCtrl.Create(dwViewStyle, rectDummy, this, 4));

	m_progressCtrl.InsertColumn(0, _T("Progress"), LVCFMT_LEFT, 250);
	m_progressCtrl.InsertColumn(1, _T("Description"), LVCFMT_LEFT, 250);
	
	
	//m_messageCtrl.Create(, CRect(), this, ID_MESSAGE_CTRL);
	m_font.CreateStockObject(DEFAULT_GUI_FONT);
	m_messageCtrl.SetFont(&m_font);
	m_messageCtrl.SetTabStops(8);


	// Load view images:
	m_toolbarCtrl.Create(this, AFX_DEFAULT_TOOLBAR_STYLE, IDR_CMN_PROGRESS_TOOLBAR);
	m_toolbarCtrl.LoadToolBar(IDR_CMN_PROGRESS_TOOLBAR, 0, 0, TRUE /* Is locked */);

	OnChangeVisualStyle();

	m_toolbarCtrl.SetPaneStyle(m_toolbarCtrl.GetPaneStyle() | CBRS_TOOLTIPS | CBRS_FLYBY);
	m_toolbarCtrl.SetPaneStyle(m_toolbarCtrl.GetPaneStyle() & ~(CBRS_GRIPPER | CBRS_SIZE_DYNAMIC | CBRS_BORDER_TOP | CBRS_BORDER_BOTTOM | CBRS_BORDER_LEFT | CBRS_BORDER_RIGHT));
	m_toolbarCtrl.SetOwner(this);
	m_toolbarCtrl.SetRouteCommandsViaFrame(FALSE);

	
	// Fill in some static tree view data (dummy code, nothing magic here)
	AdjustLayout();

	return 0;
}

void CProgressDockablePane::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);
	AdjustLayout();
}



void CProgressDockablePane::AdjustLayout()
{
	if (GetSafeHwnd() == NULL)
	{
		return;
	}

	CRect rectClient;
	GetClientRect(rectClient);

	int cyTlb = m_toolbarCtrl.CalcFixedLayout(FALSE, TRUE).cy;
	//int progressHeight = (rectClient.Height() - cyTlb - 2) / 3;



	m_toolbarCtrl.SetWindowPos(NULL, rectClient.left, rectClient.top, rectClient.Width(), cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);

	m_wndSplitter.SetRowInfo(0, std::max(10, (rectClient.Height() - cyTlb) / 2), 25);
	m_wndSplitter.SetRowInfo(1, std::max(10, (rectClient.Height() - cyTlb) / 2), 25);
	m_wndSplitter.SetWindowPos(NULL, rectClient.left, rectClient.top + cyTlb, rectClient.Width(), rectClient.Height() - cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);
	m_progressCtrl.SetColumnWidth(1, rectClient.Width() - m_progressCtrl.GetColumnWidth(0)-22);
}




void CProgressDockablePane::OnChangeVisualStyle()
{
	m_toolbarCtrl.CleanUpLockedImages();
	m_toolbarCtrl.LoadBitmap(IDR_CMN_PROGRESS_TOOLBAR, 0, 0, TRUE /* Locked */);
}


BOOL CProgressDockablePane::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	//if (GetFocus() == m_progressCtrl.GetSafeHwnd())
	if (m_progressCtrl.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo) )
		return TRUE;


	return CDockablePane::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}


void CProgressDockablePane::OnTimer(UINT_PTR nIDEvent)
{
	//m_callback.Lock();
	int nbItem = m_progressCtrl.GetItemCount();
	if (nbItem < m_callback.GetNbTasks())
	{
		while (nbItem < m_callback.GetTasks().size())
		{
			m_progressCtrl.InsertItem(nbItem, CString(m_callback.GetTasks().c[nbItem].m_description.c_str()));
			//m_progressCtrl.SetItemData(nbItem, NULL);
			//m_progressCtrl.SetItemText(nbItem, 1, CString(m_callback.GetTasks().c[nbItem].m_description.c_str()));
			nbItem = m_progressCtrl.GetItemCount();
		}


	}
	else if (nbItem > m_callback.GetNbTasks())
	{
		while (nbItem > m_callback.GetNbTasks())
		{
			m_progressCtrl.DeleteItem(nbItem-1);
			nbItem = m_progressCtrl.GetItemCount();
		}
	}
	//m_callback.Unlock();

	if (m_progressCtrl.GetItemCount() > 0)
	{
		//get the last progress bar
		DWORD_PTR pItem = m_progressCtrl.GetItemData(m_progressCtrl.GetItemCount() - 1);
		if (pItem!=NULL)
		{
			CProgressCtrl* pCtrl = (CProgressCtrl*)(pItem);
			if (pCtrl && pCtrl->GetSafeHwnd())
			{
				pCtrl->SetPos((int)m_callback.GetCurrentStepPercent());

				CWnd* pMain = ::AfxGetMainWnd();
				if (m_pTaskbar && pMain)
					m_pTaskbar->SetProgressValue(pMain->GetSafeHwnd(), (int)m_callback.GetCurrentStepPercent(), 100);
			}
		}
	}
	
	CDockablePane::OnTimer(nIDEvent);
	
}



void CProgressDockablePane::OnItemHeadChanged(NMHDR* pNMHDR, LRESULT* pResult)
{
	HD_NOTIFY *phdn = (HD_NOTIFY *)pNMHDR;
	// TODO: Add your control notification handler code here
	*pResult = 0;
	if (phdn->iItem != 1)
		return;

	m_progressCtrl.InvalidateProgressCtrls();
}

void CProgressDockablePane::OnUpdateToolbar(CCmdUI* pCmdUI)
{
	//switch (pCmdUI->m_nID)
	//{
	//case ID_CMN_CANCEL:pCmdUI->Enable(!m_callback.GetTasks().empty() && !m_callback.GetUserCancel()); break;
	//case ID_CMN_PAUSE_RESUME: break;
	//}
}

void CProgressDockablePane::OnCancel()
{
	m_callback.SetUserCancel();
}


void CProgressDockablePane::OnPauseResume()
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

LRESULT CProgressDockablePane::OnThreadMessage(WPARAM, LPARAM)
{
	const std::string& message = m_callback.GetMessages();
	if (!message.empty())
	{
		std::string tmp = message.data();
		std::remove(tmp.begin(), tmp.end(), '\r');
		WBSF::ReplaceString(tmp, "\n", "\r\n");

		m_comment += tmp;
		m_messageCtrl.SetWindowTextW(CString(m_comment.c_str()));
		m_messageCtrl.SetSel((int)m_comment.length(), -1);
		m_callback.DeleteMessages();
	}

	return 0;
}

void CProgressDockablePane::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
}

ERMsg CProgressDockablePane::Execute(AFX_THREADPROC pfnThreadProc, CProgressStepDlgParam* pParam)
{
	ASSERT(GetSafeHwnd());

	ERMsg msg;

	m_callback.Reset();
	m_callback.SetWnd(&m_hWnd);
	m_messageCtrl.SetWindowText(NULL);

	//prepare callback and message
	pParam->m_pCallback = &m_callback;
	pParam->m_pMsg = &msg;

	SetTimer(0, 50, NULL);

	//create thread 
	m_ptrThread = AfxBeginThread(pfnThreadProc, pParam, 0, 0, CREATE_SUSPENDED);
	ASSERT(m_ptrThread);

	m_ptrThread->m_bAutoDelete = FALSE;//don't delete thread at exit
	m_ptrThread->ResumeThread();//start thread

	//wait 0.5 seconds for very short task
	//if (WaitForSingleObject(m_ptrThread->m_hThread, 100) == WAIT_TIMEOUT)
	//{
		//if the task is not finish, show progress bar dialog 
		//ShowWindow(SW_SHOW);
	

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
	//}


	KillTimer(0);

	//clean callback
	while (!m_callback.GetTasks().empty())
		m_callback.PopTask();
	
	while (m_progressCtrl.GetItemCount()>0)
		m_progressCtrl.DeleteItem(m_progressCtrl.GetItemCount() - 1);
		
	//m_messageCtrl.SetWindowText(NULL);
	m_progressCtrl.Invalidate();
	//m_messageCtrl.Invalidate();
	
	//clean up memory
	delete m_ptrThread;
	m_ptrThread = NULL;

	
	//update title 
	CWnd* pMain = ::AfxGetMainWnd();
	

	if (m_pTaskbar && pMain)
		m_pTaskbar->SetProgressState(pMain->GetSafeHwnd(), TBPF_NOPROGRESS);


	


	return msg;
}
