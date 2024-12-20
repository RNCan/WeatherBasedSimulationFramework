
#include "stdafx.h"
#include "mainfrm.h"
#include "ProjectWnd.h"
#include "WeatherUpdaterDoc.h"
#include "Tasks/TaskFactory.h"

#include "UI/Common/SYShowMessage.h"
#include "Resource.h"
#include "WeatherBasedSimulationString.h"
#include "WeatherBasedSimulationUI.h"


using namespace std;
using namespace UtilWin;
using namespace WBSF;



#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

IMPLEMENT_SERIAL(CProjectWndToolBar, CMFCToolBar, 1)

static CWeatherUpdaterDoc* GetDocument()
{
	CWeatherUpdaterDoc* pDoc = NULL;
	CWinApp* pApp = AfxGetApp();
	if (pApp)
	{
		POSITION  pos = pApp->GetFirstDocTemplatePosition();
		CDocTemplate* docT = pApp->GetNextDocTemplate(pos);
		if (docT)
		{
			pos = docT->GetFirstDocPosition();
			pDoc = (CWeatherUpdaterDoc*)docT->GetNextDoc(pos);
		}
	}

	return pDoc;
}

static const int ID_INDICATOR_NB_TASK_CHECKED = 0xE711;
static UINT indicators[] =
{
	ID_SEPARATOR,
	ID_INDICATOR_NB_TASK_CHECKED
};

static const UINT ID_TASK_CTRL = 1000;

BEGIN_MESSAGE_MAP(CTaskWnd, CWnd)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_CONTEXTMENU()

	ON_UPDATE_COMMAND_UI_RANGE(ID_TASK_FIRST, ID_TASK_LAST, OnUpdateToolBar)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateToolBar)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdateToolBar)
	ON_UPDATE_COMMAND_UI(ID_EDIT_DUPLICATE, OnUpdateToolBar)
	ON_UPDATE_COMMAND_UI(ID_TASK_DELETE_UPDATE, OnUpdateToolBar) 
	ON_UPDATE_COMMAND_UI(ID_TASK_DELETE_TOOL, OnUpdateToolBar)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_NB_TASK_CHECKED, OnUpdateStatusBar)

	ON_COMMAND_RANGE(ID_TASK_FIRST, ID_TASK_LAST, OnToolBarClick)
	ON_COMMAND_RANGE(ID_TASK_DELETE_UPDATE, ID_TASK_DELETE_TOOL, &OnToolBarClick)
	
	ON_COMMAND(ID_EDIT_COPY, &OnEditCopy)
	ON_COMMAND(ID_EDIT_PASTE, &OnEditPaste)
	ON_COMMAND(ID_EDIT_DUPLICATE, &OnEditDuplicate)

	ON_REGISTERED_MESSAGE(WM_XHTMLTREE_CHECKBOX_CLICKED, OnCheckbox)
	ON_REGISTERED_MESSAGE(WM_XHTMLTREE_BEGIN_DRAG, OnBeginDrag)
	ON_REGISTERED_MESSAGE(WM_XHTMLTREE_END_DRAG, OnEndDrag)
	ON_REGISTERED_MESSAGE(WM_XHTMLTREE_DROP_HOVER, OnDropHover)
	ON_NOTIFY(TVN_SELCHANGED, ID_TASK_CTRL, OnSelChange)
	ON_NOTIFY(TVN_ENDLABELEDIT, ID_TASK_CTRL, OnNameChange)

END_MESSAGE_MAP()




CWeatherUpdaterDoc* CTaskWnd::GetDocument()
{
	CWeatherUpdaterDoc* pDoc = NULL;
	CWinApp* pApp = AfxGetApp();
	if (pApp)
	{
		POSITION  pos = pApp->GetFirstDocTemplatePosition();
		CDocTemplate* docT = pApp->GetNextDocTemplate(pos);
		if (docT)
		{
			pos = docT->GetFirstDocPosition();
			pDoc = (CWeatherUpdaterDoc*)docT->GetNextDoc(pos);
		}
	}

	return pDoc;
}


CTaskWnd::CTaskWnd(size_t t, UINT toolbarID1, UINT toolbarID2, UINT toolbarID3)
{
	m_type = t;
	m_toolbarID1 = toolbarID1;
	m_toolbarID2 = toolbarID2;
	m_toolbarID3 = toolbarID3;
	m_bInUpdate = false;
}


int CTaskWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	const DWORD dwStyle = TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT |
		TVS_EDITLABELS | TVS_SHOWSELALWAYS |
		WS_CHILD | WS_VISIBLE | WS_GROUP | WS_TABSTOP | WS_BORDER;

	VERIFY(m_taskCtrl.Create(dwStyle, CRect(0, 0, 0, 0), this, 1000));

	VERIFY(m_wndToolBar1.Create(this, AFX_DEFAULT_TOOLBAR_STYLE | CBRS_SIZE_DYNAMIC, m_toolbarID1));
	VERIFY(m_wndToolBar1.LoadToolBar(m_toolbarID1, 0, 0, TRUE /* Is locked */));
	m_wndToolBar1.SetPaneStyle(m_wndToolBar1.GetPaneStyle() | CBRS_TOOLTIPS | CBRS_FLYBY);
	m_wndToolBar1.SetPaneStyle(m_wndToolBar1.GetPaneStyle() & ~(CBRS_GRIPPER | CBRS_BORDER_TOP | CBRS_BORDER_BOTTOM | CBRS_BORDER_LEFT | CBRS_BORDER_RIGHT));
	m_wndToolBar1.SetOwner(this);//| CBRS_SIZE_DYNAMIC 
	m_wndToolBar1.SetRouteCommandsViaFrame(FALSE);
	
	if (m_toolbarID2!=-1)
	{
		VERIFY(m_wndToolBar2.Create(this, AFX_DEFAULT_TOOLBAR_STYLE | CBRS_SIZE_DYNAMIC, m_toolbarID2));
		VERIFY(m_wndToolBar2.LoadToolBar(m_toolbarID2, 0, 0, TRUE /* Is locked */));
		m_wndToolBar2.SetPaneStyle(m_wndToolBar2.GetPaneStyle() | CBRS_TOOLTIPS | CBRS_FLYBY);
		m_wndToolBar2.SetPaneStyle(m_wndToolBar2.GetPaneStyle() & ~(CBRS_GRIPPER | CBRS_BORDER_TOP | CBRS_BORDER_BOTTOM | CBRS_BORDER_LEFT | CBRS_BORDER_RIGHT));
		m_wndToolBar2.SetOwner(this);//| CBRS_SIZE_DYNAMIC 
		m_wndToolBar2.SetRouteCommandsViaFrame(FALSE);
	}

	 
	if (m_toolbarID3 != -1)
	{
		VERIFY(m_wndToolBar3.Create(this, AFX_DEFAULT_TOOLBAR_STYLE | CBRS_SIZE_DYNAMIC, m_toolbarID3));
		VERIFY(m_wndToolBar3.LoadToolBar(m_toolbarID3, 0, 0, TRUE /* Is locked */));
		m_wndToolBar3.SetPaneStyle(m_wndToolBar3.GetPaneStyle() | CBRS_TOOLTIPS | CBRS_FLYBY);
		m_wndToolBar3.SetPaneStyle(m_wndToolBar3.GetPaneStyle() & ~(CBRS_GRIPPER | CBRS_BORDER_TOP | CBRS_BORDER_BOTTOM | CBRS_BORDER_LEFT | CBRS_BORDER_RIGHT));
		m_wndToolBar3.SetOwner(this);//| CBRS_SIZE_DYNAMIC 
		m_wndToolBar3.SetRouteCommandsViaFrame(FALSE);
	}


	VERIFY(m_wndStatusBar.Create(this)); 
	m_wndStatusBar.SetIndicators(indicators, sizeof(indicators) / sizeof(UINT));
	m_wndStatusBar.SetOwner(this);
	m_wndStatusBar.SetPaneInfo(0, ID_SEPARATOR, SBPS_STRETCH, 0);

	return 0;
}



void CTaskWnd::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);
	AdjustLayout();
}


void CTaskWnd::AdjustLayout()
{
	if (GetSafeHwnd() == NULL || (AfxGetMainWnd() != NULL && AfxGetMainWnd()->IsIconic()))
		return;

	CRect rectClient;
	GetClientRect(rectClient);

	int cyTlb = m_wndToolBar1.CalcFixedLayout(FALSE, TRUE).cy;
	int cxTlb = m_wndToolBar1.CalcFixedLayout(FALSE, TRUE).cx;
	int nbT = 1;
	
	if(m_wndToolBar2.GetSafeHwnd())
		nbT++;
	
	if (m_wndToolBar3.GetSafeHwnd())
		nbT++;

	m_wndToolBar1.SetWindowPos(NULL, rectClient.left, rectClient.top, rectClient.Width(), cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);
	if (m_wndToolBar2.GetSafeHwnd())
		m_wndToolBar2.SetWindowPos(NULL, rectClient.left, rectClient.top + cyTlb, rectClient.Width(), cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);
	if (m_wndToolBar3.GetSafeHwnd())
		m_wndToolBar3.SetWindowPos(NULL, rectClient.left, rectClient.top + 2*cyTlb, rectClient.Width(), cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);

	m_taskCtrl.SetWindowPos(NULL, rectClient.left, rectClient.top + nbT*cyTlb, rectClient.Width(), rectClient.Height() - (nbT + 1) * cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);
	m_wndStatusBar.SetWindowPos(NULL, rectClient.left, rectClient.Height() - cyTlb, rectClient.Width(), cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);
}



void CTaskWnd::OnUpdateStatusBar(CCmdUI* pCmdUI)
{

	if (pCmdUI->m_nID == ID_INDICATOR_NB_TASK_CHECKED)
	{
		long nbRows = m_taskCtrl.GetCheckedCount();

		CString str = GetCString(IDS_NB_SELECTED_TASK);
		CString text = str + UtilWin::ToCString(nbRows);

		pCmdUI->SetText(text);
		
		
		CDC* pDC = GetDC();
		if (pDC)
		{
			CSize size = pDC->GetTextExtent(text);
			size.cx *= 1.1;

			UINT nStyle = m_wndStatusBar.GetPaneStyle(1);
			m_wndStatusBar.SetPaneInfo(1, ID_INDICATOR_NB_TASK_CHECKED, nStyle, size.cx);

			ReleaseDC(pDC);
		}
		
	}


}



UINT CTaskWnd::CtrlID(const std::string& className)
{
	UINT index = UINT(-1);
	CTaskPtr pTask = CTaskFactory::CreateObject(className);
	if (pTask)
		index = pTask->GetDescriptionStringID();


	return index;
}

string CTaskWnd::ClassName(UINT ID)
{ 
	string className = CTaskFactory::ClassNameFromResourceID(ID);
	ASSERT(!className.empty());
	

	return className;
}

void CTaskWnd::OnUpdateToolBar(CCmdUI *pCmdUI)
{

	HTREEITEM hItem = m_taskCtrl.GetSelectedItem();
	size_t p = m_taskCtrl.GetPosition(hItem);
	switch(pCmdUI->m_nID)
	{
	case ID_EDIT_COPY:
	case ID_EDIT_DUPLICATE:
	case ID_TASK_DELETE_UPDATE:	pCmdUI->Enable(p != NOT_INIT); break;
	case ID_TASK_DELETE_TOOL:pCmdUI->Enable(p != NOT_INIT); break;
		
	default: pCmdUI->Enable(true);
	}
	
}


void CTaskWnd::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	m_bInUpdate = true;

	if (lHint == CWeatherUpdaterDoc::INIT)
	{
		m_taskCtrl.DeleteAllItems();
		
		//UINT imageIndex = ID_TASK_LAST - ID_TASK_FIRST + UINT(m_type);
		UINT ID = m_type == CTaskBase::UPDATER ? ID_TASK_UPDATE : ID_TASK_TOOLS;
		UINT imageIndex = m_taskCtrl.ID2ImagesIndex(ID);
		m_taskCtrl.AddRoot(m_type, imageIndex);
	}
	
	
	

	CWeatherUpdaterDoc* pDoc = (CWeatherUpdaterDoc*)GetDocument(); 
	if (!pDoc)
		return;
	
	if (lHint == CWeatherUpdaterDoc::INIT)
	{
	
		const CTaskPtrVector& tasks = pDoc->GetTaskVector(m_type);
		for (size_t p = 0; p < tasks.size(); p++)
		{
			const CTaskPtr& pTask = tasks[p];
			ASSERT(pTask->ClassType() == m_type);
			{
				UINT ID = CtrlID(pTask->ClassName());
				UINT imageIndex = m_taskCtrl.ID2ImagesIndex(ID);
				//UINT imageIndex = ID - ID_TASK_FIRST;

				m_taskCtrl.InsertTask(pTask, imageIndex, m_taskCtrl.GetLastItem(m_taskCtrl.GetRootItem()));
			}
		}
	}
	else if (lHint == CWeatherUpdaterDoc::ADD_TASK)
	{
	}
	else if (lHint == CWeatherUpdaterDoc::REMOVE_TASK)
	{
	}
	else if (lHint == CWeatherUpdaterDoc::SELECTION_CHANGE)
	{
		//remove selection if it's not the current pane
		if (pDoc->GetCurT() != m_type)
		{
			size_t p = pDoc->GetCurP(m_type);
			HTREEITEM hItem = m_taskCtrl.GetSelectedItem();
			HTREEITEM hNewItem = m_taskCtrl.FindItem(p);
			if (hNewItem != hItem)
				m_taskCtrl.Select(hNewItem, TVGN_CARET);
		}
	}
	

	m_bInUpdate = false;
}

void CTaskWnd::OnToolBarClick(UINT ID)
{
	if (ID == ID_TASK_DELETE_UPDATE || ID == ID_TASK_DELETE_TOOL)
		OnRemove();
	else
		OnAdd(ID);
}

void CTaskWnd::OnAdd(UINT ID)
{
	CWeatherUpdaterDoc* pDoc = (CWeatherUpdaterDoc*)GetDocument(); ASSERT(pDoc);
	if (pDoc == NULL || pDoc->GetFilePath().empty())
	{
		
		if (AfxMessageBox(IDS_BSC_ASK_CREATE_PROJECT, MB_OKCANCEL) == IDOK)
			AfxGetMainWnd()->SendMessage(WM_COMMAND, ID_FILE_SAVE);

		if (pDoc == NULL || pDoc->GetFilePath().empty())
			return;
	}

	string className = ClassName(ID);
	if (!className.empty())
	{
		HTREEITEM hItem = m_taskCtrl.GetSelectedItem();
		size_t p = m_taskCtrl.GetPosition(hItem);
		UINT imageIndex = m_taskCtrl.ID2ImagesIndex(ID);
		//UINT imageIndex = ID - ID_TASK_FIRST;
		
		CTaskPtr pTask = CTaskFactory::CreateObject(className);
		ENSURE(pTask.get());

		pTask->Init(&pDoc->GetProject());
		
		pTask->m_name = className;
		pTask->m_bExecute = true;

		size_t pp = (p + 1);
		pDoc->InsertTask(m_type, pp, pTask);
		m_taskCtrl.InsertTask(pTask, imageIndex, (p==NOT_INIT)?NULL:hItem);
	}
}

void CTaskWnd::OnRemove()
{
	CWeatherUpdaterDoc* pDoc = (CWeatherUpdaterDoc*)GetDocument();
	ASSERT(pDoc);
	 
	HTREEITEM hItem = m_taskCtrl.GetSelectedItem();
	ASSERT(hItem != NULL);

	size_t p = m_taskCtrl.GetPosition(hItem);
	ENSURE(p != NOT_INIT);

	pDoc->RemoveTask(m_type, p);
	m_taskCtrl.DeleteItem(hItem);
}


void CTaskWnd::OnSelChange(NMHDR* pNMHDR, LRESULT* pResult)
{
	if (!m_bInUpdate)
	{
		CWeatherUpdaterDoc* pDoc = (CWeatherUpdaterDoc*)GetDocument();
		NMTREEVIEW* pNMTreeView = (NMTREEVIEW*)pNMHDR;
		HTREEITEM hItem = pNMTreeView->itemNew.hItem;

		size_t p = m_taskCtrl.GetPosition(hItem);
		pDoc->SetCurP(m_type, p);
	}
}

void CTaskWnd::OnEditCopy()
{
	CWeatherUpdaterDoc* pDoc = (CWeatherUpdaterDoc*)GetDocument();
	ASSERT(pDoc);

	HTREEITEM hItem = m_taskCtrl.GetSelectedItem();
	ASSERT(hItem != NULL);

	size_t p = m_taskCtrl.GetPosition(hItem);
	ASSERT(p != NOT_INIT);

	pDoc->GetTask(m_type, p)->CopyToClipBoard();
}

void CTaskWnd::OnEditPaste()
{
	CWeatherUpdaterDoc* pDoc = (CWeatherUpdaterDoc*)GetDocument();
	ASSERT(pDoc);
	
	CTaskPtr pTask = CTaskFactory::CreateFromClipbord();

	if (pTask)
	{
		pTask->m_name = GenerateNewName(pTask->m_name);

		HTREEITEM hItem = m_taskCtrl.GetSelectedItem();
		size_t p = m_taskCtrl.GetPosition(hItem);

		size_t pp = (p + 1);
		pDoc->InsertTask(m_type, pp, pTask);

		UINT ID = CtrlID(pTask->ClassName());
		UINT imageIndex = m_taskCtrl.ID2ImagesIndex(ID);
		//UINT imageIndex = ID - ID_TASK_FIRST;
		m_taskCtrl.InsertTask(pTask, imageIndex, (p == NOT_INIT) ? NULL : hItem);
	}

}

void CTaskWnd::OnEditDuplicate()
{
	OnEditCopy();
	OnEditPaste();
}

void CTaskWnd::OnNameChange(NMHDR* pNMHDR, LRESULT* pResult)
{
	CWeatherUpdaterDoc* pDoc = (CWeatherUpdaterDoc*)GetDocument();
	ASSERT(pDoc);

	*pResult = TRUE;			// return TRUE to accept edit

	NMTVDISPINFO* pTVDispInfo = (NMTVDISPINFO*)pNMHDR;
	HTREEITEM hItem = pTVDispInfo->item.hItem;
	ASSERT(hItem);

	LPTSTR pszText = pTVDispInfo->item.pszText;
	if (pszText)
	{
		size_t p = m_taskCtrl.GetPosition(hItem);
		if (*pszText != _T('\0') && p != NOT_INIT)
		{
			CTaskPtr& pTask = pDoc->GetTask(m_type, p);
			ASSERT(pTask.get());
			pTask->m_name = string(CStringA(pszText));

			//pDoc->UpdateAllViews(NULL, CWeatherUpdaterDoc::TASK_CHANGE);
		}
		else
		{
			*pResult = FALSE;		// don't allow empty label
		}
	}
}


LRESULT CTaskWnd::OnCheckbox(WPARAM wParam, LPARAM lParam)
{
	XHTMLTREEMSGDATA *pData = (XHTMLTREEMSGDATA *)wParam;
	ASSERT(pData);

	BOOL bChecked = lParam;

	if (pData)
	{
		HTREEITEM hItem = pData->hItem;

		if (hItem)
		{
			ASSERT(m_type < CTaskBase::NB_TYPES);
			size_t p = m_taskCtrl.GetPosition(hItem);

			if (p != NOT_INIT)
			{
				CWeatherUpdaterDoc* pDoc = GetDocument();
				//size_t p = pDoc->GetCurP(m_type);
				WBSF::CTaskPtr pTask = pDoc->GetTask(m_type, p);
				pTask->m_bExecute = bChecked;
			}
			//pDoc->UpdateAllViews(NULL, CWeatherUpdaterDoc::TASK_CHANGE, NULL);
		}
	}

	return 0;
}




LRESULT CTaskWnd::OnBeginDrag(WPARAM wParam, LPARAM lParam)
{
	XHTMLTREEMSGDATA *pMsg = (XHTMLTREEMSGDATA *)wParam;
	ASSERT(pMsg);

	XHTMLTREEDRAGMSGDATA *pData = (XHTMLTREEDRAGMSGDATA *)lParam;
	LRESULT lResult = 0;

	if (pMsg && pData && pData->hItem)
	{
		lResult = m_taskCtrl.GetPosition(pData->hItem)==NOT_INIT?1:0;
	}
	else
	{
		TRACE(_T("ERROR bad param\n"));
		ASSERT(FALSE);
	}

	return lResult;	// return 0 to allow drag
}

LRESULT CTaskWnd::OnEndDrag(WPARAM wParam, LPARAM lParam)
{
	XHTMLTREEMSGDATA *pMsg = (XHTMLTREEMSGDATA *)wParam;
	ASSERT(pMsg);

	XHTMLTREEDRAGMSGDATA *pData = (XHTMLTREEDRAGMSGDATA *)lParam;

	LRESULT lResult = 1;

	if (pMsg && pData && pData->hItem )
	{
		HTREEITEM hItem = pData->hItem;
		HTREEITEM hAfter = pData->hAfter;
		
		if (hAfter)
		{
			CWeatherUpdaterDoc* pDoc = GetDocument();
			ASSERT(pDoc);
			
			bool bCopyDrag = pData->bCopyDrag;
			
			size_t pFrom = m_taskCtrl.GetPosition(hItem);
			size_t pTo = m_taskCtrl.GetPosition(hAfter);


			if (bCopyDrag)
			{
				CTaskPtr pTask1 = pDoc->GetTask(m_type, pFrom);
				CTaskPtr pTask2 = CTaskFactory::CreateObject(pTask1->ClassName());
				*pTask2 = *pTask1;

				//pTask2->m_name = WBSF::GenerateNewName(pTask2->m_name);

				pDoc->InsertTask(m_type, pTo, pTask2);
			}
			else
			{
				pDoc->Move(m_type, pFrom, pTo, true);
			}
				


			lResult = 0;
		}
	}

	return lResult;	// return 0 to allow drop
}

LRESULT CTaskWnd::OnDropHover(WPARAM wParam, LPARAM lParam)
{

	XHTMLTREEMSGDATA *pMsg = (XHTMLTREEMSGDATA *)wParam;
	ASSERT(pMsg);

	XHTMLTREEDRAGMSGDATA *pData = (XHTMLTREEDRAGMSGDATA *)lParam;
	LRESULT lResult = 0;


	return lResult;
}

void CTaskWnd::OnContextMenu(CWnd* pWnd, CPoint point)
{

	//CTaskTreeCtrl* pWndTree = (CTaskTreeCtrl*)(this);
	//ASSERT_VALID(pWndTree);

	//int classType = -1;
	//if (point != CPoint(-1, -1))
	//{
	//	// Select clicked item:
	//	CPoint ptTree = point;
	//	m_taskCtrl.ScreenToClient(&ptTree);

	//	UINT flags = 0;
	//	HTREEITEM hTreeItem = m_taskCtrl.HitTest(ptTree, &flags);
	//	if (hTreeItem != NULL)
	//	{
	//		CContextMenuManager* pMM = ((CWinAppEx*)AfxGetApp())->GetContextMenuManager();
	//		HMENU hMenu = pMM->GetMenuByName(_T("Edit2"));
	//		if (hMenu != NULL)
	//		{
	//			CMenu* pMenu = CMenu::FromHandle(hMenu);
	//			ASSERT(pMenu);

	//			CMenu* pSumMenu = pMenu->GetSubMenu(0);
	//			ASSERT(pSumMenu);

	//			pMM->TrackPopupMenu(*pSumMenu, point.x, point.y, this);
	//		}
	//	}

	//	//m_taskCtrl.SelectItem(hTreeItem);
	//}

	//SetFocus();
	
	CMenu menu;
	menu.LoadMenu(IDR_POPUP_EDIT);

	CMenu* pSumMenu = menu.GetSubMenu(0);

	if (AfxGetMainWnd()->IsKindOf(RUNTIME_CLASS(CFrameWndEx)))
	{
		CMFCPopupMenu* pPopupMenu = new CMFCPopupMenu;

		if (!pPopupMenu->Create(this, point.x, point.y, (HMENU)pSumMenu->m_hMenu, FALSE, TRUE))
			return;

		((CFrameWndEx*)AfxGetMainWnd())->OnShowPopupMenu(pPopupMenu);
		//UpdateDialogControls(this, FALSE);
	}

	SetFocus();
}


/////////////////////////////////////////////////////////////////////////////
// CProjectWnd
IMPLEMENT_DYNAMIC(CProjectWnd, CDockablePane)

BEGIN_MESSAGE_MAP(CProjectWnd, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
END_MESSAGE_MAP()

CProjectWnd::CProjectWnd():
m_wnd1(CTaskBase::UPDATER, IDR_TASK_TOOLBAR1, IDR_TASK_TOOLBAR2, IDR_TASK_TOOLBAR3),
m_wnd2(CTaskBase::TOOLS, IDR_TASK_TOOLBAR4, IDR_TASK_TOOLBAR5)
{

}

CProjectWnd::~CProjectWnd()
{
}



/////////////////////////////////////////////////////////////////////////////
// CWorkspaceBar message handlers

int CProjectWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	const DWORD dwStyle = TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT |
		TVS_EDITLABELS | TVS_SHOWSELALWAYS |
		WS_CHILD | WS_VISIBLE | WS_GROUP | WS_TABSTOP | WS_BORDER | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;

	DWORD dwStyleEx = 0;
	m_wndSplitter.CreateStatic(this, 2, 1);

	if (!m_wndSplitter.AddWindow(0, 0, &m_wnd1, WC_TREEVIEW, dwStyle, dwStyleEx, CSize(100, 100)))
		return -1;

	
	if (!m_wndSplitter.AddWindow(1, 0, &m_wnd2, WC_TREEVIEW, dwStyle, dwStyleEx, CSize(100, 100)))
		return -1;


	// All commands will be routed via this control , not via the parent frame:
	AdjustLayout();

	return 0;
}



void CProjectWnd::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);
	AdjustLayout();
}

void CProjectWnd::AdjustLayout()
{
	if (GetSafeHwnd() == NULL)
		return;
	
	CRect rectClient;
	GetClientRect(rectClient);
	
	m_wndSplitter.SetRowInfo(0, rectClient.Height() / 2, 25);
	m_wndSplitter.SetRowInfo(1, rectClient.Height() / 2, 25);
	m_wndSplitter.SetWindowPos(NULL, rectClient.left, rectClient.top, rectClient.Width(), rectClient.Height(), SWP_NOACTIVATE | SWP_NOZORDER);


}


void CProjectWnd::OnSetFocus(CWnd* pOldWnd)
{
	CDockablePane::OnSetFocus(pOldWnd);
	m_wnd1.m_taskCtrl.SetFocus();
}


void CProjectWnd::OnExecute()
{
	AfxGetMainWnd()->PostMessage(WM_COMMAND, ID_EXECUTE);
}


BOOL CProjectWnd::OnOpenWorkingDir(UINT ID)
{
	//CWeatherUpdaterDoc* pDoc = GetDocument();
	//CBioSIMProject& project = pDoc->GetProject();

	//
	//HTREEITEM hItem = m_projectCtrl.GetSelectedItem();
	//string iName = m_projectCtrl.GetInternalName(hItem);
	//CTaskPtr pItem = project.FindItem(iName);

	//if (pItem)
	//{
	//	string path;
	//	switch (ID)
	//	{
	//	case ID_OPEN_WORKING_DIR: path = pItem->GetPath(GetFM()); break;
	//	case ID_OPEN_OUTPUT_MAP_DIR: path = GetFM().GetOutputMapPath(); break;
	//	default: ASSERT(false);
	//	}

	//	ShellExecute(m_hWnd, _T("open"), Convert(path), NULL, NULL, SW_SHOW);
	//}

	return TRUE;
}



void CProjectWnd::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	CWeatherUpdaterDoc* pDoc = (CWeatherUpdaterDoc*)GetDocument(); ASSERT(pDoc);
	if (pDoc)
	{
		m_wnd1.OnUpdate(pSender, lHint, pHint);
		m_wnd2.OnUpdate(pSender, lHint, pHint);
	}

}


BOOL CProjectWnd::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	
	CWnd* pFocus = GetFocus();
	if (pFocus&& IsChild(pFocus))
	{
		//CWnd* pParent = pFocus->GetParent();

		//if (pFocus == &m_wnd1 || pParent == &m_wnd1)
		//{
		if (m_wnd1.IsChild(pFocus) && m_wnd1.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
			return TRUE;
		//}

		//if(pFocus == &m_wnd2 || pParent == &m_wnd2)
		//{
		if (m_wnd2.IsChild(pFocus) && m_wnd2.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
				return TRUE;
		//}
	}
	

	return CDockablePane::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}
