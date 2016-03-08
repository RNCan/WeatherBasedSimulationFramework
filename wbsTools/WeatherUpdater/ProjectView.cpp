
#include "stdafx.h"
#include "Simulation/BioSIMProject.h"
#include "Simulation/BioSIMProject.h"
#include "UI/Common\SYShowMessage.h"
#include "WeatherBasedSimulationUI.h"

#include "mainfrm.h"
#include "ProjectView.h"
#include "Resource.h"
#include "WeatherUpdaterDoc.h"

using namespace std;
using namespace UtilWin;
using namespace WBSF;



#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif



static const int ID_INDICATOR_NB_TASK_CHECKED = 0xE711;
static UINT indicators[] =
{
	ID_SEPARATOR,
	ID_INDICATOR_NB_TASK_CHECKED
};


BEGIN_MESSAGE_MAP(CTaskWnd, CWnd)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_CONTEXTMENU()
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_NB_TASK_CHECKED, OnUpdateStatusBar)
	ON_REGISTERED_MESSAGE(WM_XHTMLTREE_CHECKBOX_CLICKED, OnCheckbox)
	ON_REGISTERED_MESSAGE(WM_XHTMLTREE_ITEM_EXPANDED, OnItemExpanded)
	ON_REGISTERED_MESSAGE(WM_XHTMLTREE_BEGIN_DRAG, OnBeginDrag)
	ON_REGISTERED_MESSAGE(WM_XHTMLTREE_END_DRAG, OnEndDrag)
	ON_REGISTERED_MESSAGE(WM_XHTMLTREE_DROP_HOVER, OnDropHover)

END_MESSAGE_MAP()


CTaskWnd::CTaskWnd(UINT ctrlID, UINT toolbarID)
{
	m_ctrlID = ctrlID;
	m_toolbarID = toolbarID;
}


int CTaskWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	const DWORD dwStyle = TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT |
		TVS_EDITLABELS | TVS_SHOWSELALWAYS |
		WS_CHILD | WS_VISIBLE | WS_GROUP | WS_TABSTOP | WS_BORDER;

	// Create the list control.  Don't worry about specifying
	// correct coordinates.  That will be handled in OnSize()
	BOOL bResult = m_taskCtrl.Create(dwStyle, CRect(0, 0, 0, 0), this, m_ctrlID);
//	m_taskCtrl.SetDragOps(XHTMLTREE_DO_SCROLL_NORMAL);
	

	m_wndToolBar.Create(this, AFX_DEFAULT_TOOLBAR_STYLE | CBRS_SIZE_DYNAMIC, m_toolbarID);
	m_wndToolBar.LoadToolBar(m_toolbarID, 0, 0, TRUE /* Is locked */);
	m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() | CBRS_TOOLTIPS | CBRS_FLYBY);
	m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() & ~(CBRS_GRIPPER | CBRS_BORDER_TOP | CBRS_BORDER_BOTTOM | CBRS_BORDER_LEFT | CBRS_BORDER_RIGHT));
	m_wndToolBar.SetOwner(this);//| CBRS_SIZE_DYNAMIC 
	m_wndToolBar.SetRouteCommandsViaFrame(FALSE);

	if (!m_wndStatusBar.Create(this))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

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

	int cyTlb = m_wndToolBar.CalcFixedLayout(FALSE, TRUE).cy;
	int cxTlb = m_wndToolBar.CalcFixedLayout(FALSE, TRUE).cx;

	m_wndToolBar.SetWindowPos(NULL, rectClient.left, rectClient.top, rectClient.Width(), cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);
	m_taskCtrl.SetWindowPos(NULL, rectClient.left, rectClient.top + cyTlb, rectClient.Width(), rectClient.Height() - 2 * cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);
	m_wndStatusBar.SetWindowPos(NULL, rectClient.left, rectClient.Height() - cyTlb, rectClient.Width(), cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);
}



void CTaskWnd::OnUpdateStatusBar(CCmdUI* pCmdUI)
{

	if (pCmdUI->m_nID == ID_INDICATOR_NB_TASK_CHECKED)
	{
		//	CWeatherDatabasePtr pDB = GetDatabasePtr();
		long nbRows = m_taskCtrl.GetCheckedCount();

		CString str = _T("Selected = ");//GetString(IDS__NB_STATION);
		CString text = str + UtilWin::ToCString(nbRows);

		pCmdUI->SetText(text);

		CDC* pDC = GetDC();
		ASSERT(pDC);
		CSize size = pDC->GetTextExtent(text);

		UINT nStyle = m_wndStatusBar.GetPaneStyle(1);
		m_wndStatusBar.SetPaneInfo(1, ID_INDICATOR_NB_TASK_CHECKED, nStyle, size.cx);
	}


}

//
//void CProjectView::OnContextMenu(CWnd* pWnd, CPoint point)
//{
//	if (pWnd == &m_projectCtrl)
//	{
//		((CWinAppEx*)AfxGetApp())->ShowPopupMenu(IDR_POPUP, point, this);
//		//m_projectCtrl.OnContextMenu(pWnd, point);
//	}
//	else
//	{
//		CView::OnContextMenu(pWnd, point);
//	}
//
//}

LRESULT CTaskWnd::OnCheckbox(WPARAM wParam, LPARAM lParam)
{
	return 0; // return m_taskCtrl.OnCheckbox(wParam, lParam);
}

LRESULT CTaskWnd::OnItemExpanded(WPARAM wParam, LPARAM lParam)
{
	return 0; // return m_taskCtrl.OnItemExpanded(wParam, lParam);
}

LRESULT CTaskWnd::OnBeginDrag(WPARAM wParam, LPARAM lParam)
{
	return 0; // return m_taskCtrl.OnBeginDrag(wParam, lParam);
}

LRESULT CTaskWnd::OnEndDrag(WPARAM wParam, LPARAM lParam)
{
	return 0; // return m_taskCtrl.OnEndDrag(wParam, lParam);
}

LRESULT CTaskWnd::OnDropHover(WPARAM wParam, LPARAM lParam)
{
	return 0; // return m_taskCtrl.OnDropHover(wParam, lParam);
}



static const UINT ID_TASK_CTRL1 = 1001;
static const UINT ID_TASK_CTRL2 = 1002;


/////////////////////////////////////////////////////////////////////////////
// CProjectView
IMPLEMENT_DYNCREATE(CProjectView, CView)

BEGIN_MESSAGE_MAP(CProjectView, CView)
	ON_WM_CREATE()
	ON_WM_SIZE()
	//ON_WM_SETFOCUS()

//	ON_COMMAND_RANGE(ID_ADD_EC_DAILY, ID_ADD_BC_SNOWPILLOW, OnAddTask)
	//ON_UPDATE_COMMAND_UI_RANGE(ID_ADD_EC_DAILY, ID_ADD_BC_SNOWPILLOW, OnUpdateToolBar)

	ON_NOTIFY(TVN_SELCHANGED, ID_TASK_CTRL1, OnSelChange)
	ON_NOTIFY(TVN_SELCHANGED, ID_TASK_CTRL2, OnSelChange)

END_MESSAGE_MAP()

CProjectView::CProjectView():
m_wnd1(ID_TASK_CTRL1, IDR_TASK_TOOLBAR1),
m_wnd2(ID_TASK_CTRL2, IDR_TASK_TOOLBAR2)
{

}

CProjectView::~CProjectView()
{
}

/////////////////////////////////////////////////////////////////////////////
// Gestionnaires de messages de CResourceViewBar
// CWeatherChartView drawing
void CProjectView::OnDraw(CDC* pDC)
{
	//do nothing
}




/////////////////////////////////////////////////////////////////////////////
// CWorkspaceBar message handlers

int CProjectView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;

	const DWORD dwStyle = TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT |
		TVS_EDITLABELS | TVS_SHOWSELALWAYS |
		WS_CHILD | WS_VISIBLE | WS_GROUP | WS_TABSTOP | WS_BORDER;

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



void CProjectView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);
	AdjustLayout();
}

void CProjectView::AdjustLayout()
{
	if (GetSafeHwnd() == NULL)
		return;
	
	CRect rectClient;
	GetClientRect(rectClient);
	
	m_wndSplitter.SetRowInfo(0, rectClient.Height() / 2, 25);
	m_wndSplitter.SetRowInfo(1, rectClient.Height() / 2, 25);
	m_wndSplitter.SetWindowPos(NULL, rectClient.left, rectClient.top, rectClient.Width(), rectClient.Height(), SWP_NOACTIVATE | SWP_NOZORDER);


}

//void CProjectView::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
//{
//	CView::OnSettingChange(uFlags, lpszSection);
//	SetPropListFont();
//}
//
//void CProjectView::SetPropListFont()
//{
//	::DeleteObject(m_fntPropList.Detach());
//
//	LOGFONT lf;
//	afxGlobalData.fontRegular.GetLogFont(&lf);
//
//	NONCLIENTMETRICS info;
//	info.cbSize = sizeof(info);
//
//	afxGlobalData.GetNonClientMetrics(info);
//
//	lf.lfHeight = info.lfMenuFont.lfHeight;
//	lf.lfWeight = info.lfMenuFont.lfWeight;
//	lf.lfItalic = info.lfMenuFont.lfItalic;
//
//	m_fntPropList.CreateFontIndirect(&lf);
//
//	m_wnd1.SetFont(&m_fntPropList);
//	m_wnd2.SetFont(&m_fntPropList);
//}


void CProjectView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	
	CWeatherUpdaterDoc* pDoc = (CWeatherUpdaterDoc*)GetDocument(); ASSERT(pDoc);
	string ID = pDoc->GetCurTaskID();

	if (lHint == CWeatherUpdaterDoc::INIT)
	{
		/*CExecutablePtr& pExecutable = pDoc->GetTask();
		CProjectStatePtr& pProjectState = pDoc->GetProjectState();
		m_projectCtrl.SetExecutable(pExecutable, pProjectState);*/
	}
	else if (lHint == CWeatherUpdaterDoc::SELECTION_CHANGE)
	{
		//HTREEITEM hItem = m_projectCtrl.GetSelectedItem();
		//HTREEITEM hNewItem = m_projectCtrl.FindItem(iName);
		//if (hNewItem != hItem)
		//	m_projectCtrl.Select(hNewItem, TVGN_CARET);
	}
}


void CProjectView::OnUpdateToolbar(CCmdUI *pCmdUI)
{
	CWeatherUpdaterDoc* pDoc = (CWeatherUpdaterDoc*)GetDocument();
	//bool bInit = pDoc->GetDatabase()->IsOpen() && !pDoc->GetDataInEdition();

	/*switch (pCmdUI->m_nID)
	{
	case ID_ADD_WEATHER_STATION:pCmdUI->Enable(bInit); break;
	case ID_SENDTO_SHOWMAP:
	case ID_SENDTO_EXCEL:
	case ID_STATION_LIST_FILTER:pCmdUI->Enable(bInit); break;
	default: ASSERT(false);
	}*/


}


void CProjectView::OnToolbarCommand(UINT ID)
{
	CWeatherUpdaterDoc* pDoc = (CWeatherUpdaterDoc*)GetDocument();
	ASSERT(pDoc);

	//CNormalsDatabasePtr& pDB = pDoc->GetDatabase();

	//if (pDB && pDB->IsOpen())
	//{

	//	ASSERT(pDoc);

	//	if (ID == ID_ADD_WEATHER_STATION)
	//	{
	//		MessageBox(_T("Ça reste à faire..."));
	//	}
	//	else if (ID == ID_SENDTO_EXCEL || ID == ID_SENDTO_SHOWMAP)
	//	{
	//		WBSF::CRegistry registry;
	//		char separator = registry.GetListDelimiter();

	//		CSearchResultVector searchResultArray;

	//		CWVariables filter = pDoc->GetFilters();

	//		ERMsg msg;
	//		msg = pDB->GetStationList(searchResultArray, filter, WBSF::YEAR_NOT_INIT, true, false);

	//		CLocationVector loc = pDB->GetLocations(searchResultArray);
	//		//remove coma in name...
	//		loc.TrimData(separator);

	//		string filePath = WBSF::GetUserDataPath() + "tmp\\" + WBSF::GetFileTitle(pDB->GetFilePath()) + ".csv";
	//		WBSF::CreateMultipleDir(WBSF::GetPath(filePath));

	//		msg += (loc.Save(filePath, separator));
	//		if (msg)
	//		{
	//			if (ID == ID_SENDTO_EXCEL)
	//				msg += WBSF::CallApplication(WBSF::CRegistry::SPREADSHEET1, filePath, GetSafeHwnd(), SW_SHOW);
	//			else if (ID == ID_SENDTO_SHOWMAP)
	//				msg += WBSF::CallApplication(WBSF::CRegistry::SHOWMAP, filePath, GetSafeHwnd(), SW_SHOW);
	//		}

	//		//pDoc->SetOutputText(SYGetText(msg));
	//		if (!msg)
	//			UtilWin::SYShowMessage(msg, this);
	//	}
	//	else if (ID == ID_STATION_LIST_FILTER)
	//	{
	//		int index = m_wndToolBar.CommandToIndex(ID);
	//		CMFCToolBarWVariablesButton* pCtrl = (CMFCToolBarWVariablesButton*)m_wndToolBar.GetButton(index); ASSERT(pCtrl);
	//		pDoc->SetFilters(pCtrl->GetVariables());
	//	}
	//}
}
//void CProjectView::OnPaint()
//{
//	CPaintDC dc(this); // device context for painting
//
//	CRect rectTree;
//	m_projectCtrl.GetWindowRect(rectTree);
//	ScreenToClient(rectTree);
//
//	rectTree.InflateRect(1, 1);
//	dc.Draw3dRect(rectTree, ::GetSysColor(COLOR_3DSHADOW), ::GetSysColor(COLOR_3DSHADOW));
//	
//	CRect rectStatus;
//	m_wndStatusBar.GetWindowRect(rectStatus);
//	ScreenToClient(rectStatus);
//	rectStatus.InflateRect(1, 1);
//	dc.Draw3dRect(rectStatus, ::GetSysColor(COLOR_3DSHADOW), ::GetSysColor(COLOR_3DSHADOW));
//}

void CProjectView::OnSetFocus(CWnd* pOldWnd)
{
	CView::OnSetFocus(pOldWnd);

	m_wnd1.m_taskCtrl.SetFocus();
}





void CProjectView::OnSelChange(NMHDR* pNMHDR, LRESULT* pResult)
{
	//CWeatherUpdaterDoc* pDoc = GetDocument();

	//NMTREEVIEW* pNMTreeView = (NMTREEVIEW*)pNMHDR;
	//HTREEITEM hItem = pNMTreeView->itemNew.hItem;
	////	ASSERT(hItem);

	//string iName = m_projectCtrl.GetInternalName(hItem);
	//pDoc->SetCurSel(iName);
	////pDoc->UpdateAllViews(this, CWeatherUpdaterDoc::SEL_CHANGE);

	*pResult = 0;
}

void CProjectView::OnExecute()
{
//	AfxGetMainWnd()->PostMessage(WM_COMMAND, ID_EXECUTE);
}


BOOL CProjectView::OnOpenWorkingDir(UINT ID)
{
	//CWeatherUpdaterDoc* pDoc = GetDocument();
	//CBioSIMProject& project = pDoc->GetProject();

	//
	//HTREEITEM hItem = m_projectCtrl.GetSelectedItem();
	//string iName = m_projectCtrl.GetInternalName(hItem);
	//CExecutablePtr pItem = project.FindItem(iName);

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


BOOL CProjectView::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	//let the trl to route command
	if (m_wnd1.m_taskCtrl.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
		return TRUE;
	
	if (m_wnd2.m_taskCtrl.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
		return TRUE;

	return CView::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}
