
#include "stdafx.h"
#include "Basic/Registry.h"
#include "FileManager/FileManager.h"
#include "Simulation/Mapping.h"
#include "Simulation/BioSIMProject.h"
#include "Simulation/WeatherGeneration.h"
#include "UI/Common/SYShowMessage.h"
#include "WeatherBasedSimulationUI.h"

#include "mainfrm.h"
#include "ProjectWnd.h"
#include "Resource.h"
#include "BioSIMDoc.h"

using namespace std;
using namespace UtilWin;
using namespace WBSF;



#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//IMPLEMENT_DYNCREATE(CProjectWnd, CView)


static const UINT ID_TREE_CTRL = 1000;

/////////////////////////////////////////////////////////////////////////////
// CProjectWnd
static const int ID_INDICATOR_NB_EXECUTE = 0xE711;

static UINT indicators[] =
{
	ID_SEPARATOR,
	ID_INDICATOR_NB_EXECUTE
};

CProjectWnd::CProjectWnd()
{
}

CProjectWnd::~CProjectWnd()
{
}

IMPLEMENT_DYNCREATE(CProjectWnd, CDockablePane)
BEGIN_MESSAGE_MAP(CProjectWnd, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_CONTEXTMENU()
	ON_WM_PAINT()
	ON_WM_SETFOCUS()

	ON_COMMAND(ID_EXECUTE_MENU, OnExecute)
	ON_COMMAND_EX(ID_OPEN_WORKING_DIR, OnOpenWorkingDir)
	ON_COMMAND_EX(ID_OPEN_OUTPUT_MAP_DIR, OnOpenWorkingDir)
	ON_COMMAND(ID_SHOWMAPS, OnShowMaps)
	ON_COMMAND(ID_SHOWLOC, OnShowLoc)
	ON_COMMAND(ID_MATCH_STATION, OnMatchStation)
	
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_NB_EXECUTE, OnUpdateNbExecute)
	ON_UPDATE_COMMAND_UI(ID_EXECUTE_MENU, OnUpdateToolBar)
	ON_UPDATE_COMMAND_UI(ID_OPEN_WORKING_DIR, OnUpdateToolBar)
	ON_UPDATE_COMMAND_UI(ID_OPEN_OUTPUT_MAP_DIR, OnUpdateToolBar)
	ON_UPDATE_COMMAND_UI(ID_SHOWMAPS, OnUpdateToolBar)
	ON_UPDATE_COMMAND_UI(ID_SHOWLOC, OnUpdateToolBar)
	ON_UPDATE_COMMAND_UI(ID_MATCH_STATION, OnUpdateToolBar)
	
	
	ON_REGISTERED_MESSAGE(WM_XHTMLTREE_CHECKBOX_CLICKED, OnCheckbox)
	ON_REGISTERED_MESSAGE(WM_XHTMLTREE_ITEM_EXPANDED, OnItemExpanded)
	ON_REGISTERED_MESSAGE(WM_XHTMLTREE_BEGIN_DRAG, OnBeginDrag)
	ON_REGISTERED_MESSAGE(WM_XHTMLTREE_END_DRAG, OnEndDrag)
	ON_REGISTERED_MESSAGE(WM_XHTMLTREE_DROP_HOVER, OnDropHover)
	ON_NOTIFY(TVN_SELCHANGED, ID_TREE_CTRL, OnSelChange)

END_MESSAGE_MAP()

LRESULT CProjectWnd::OnCheckbox(WPARAM wParam, LPARAM lParam)
{
	return m_projectCtrl.OnCheckbox(wParam, lParam);
}

LRESULT CProjectWnd::OnItemExpanded(WPARAM wParam, LPARAM lParam)
{
	return m_projectCtrl.OnItemExpanded(wParam, lParam);
}

LRESULT CProjectWnd::OnBeginDrag(WPARAM wParam, LPARAM lParam)
{
	return m_projectCtrl.OnBeginDrag(wParam, lParam);
}

LRESULT CProjectWnd::OnEndDrag(WPARAM wParam, LPARAM lParam)
{
	return m_projectCtrl.OnEndDrag(wParam, lParam);
}

LRESULT CProjectWnd::OnDropHover(WPARAM wParam, LPARAM lParam)
{
	return m_projectCtrl.OnDropHover(wParam, lParam);
}


/////////////////////////////////////////////////////////////////////////////
// CWorkspaceBar message handlers

int CProjectWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	// Create view:
	const DWORD dwStyle = TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT |
		TVS_EDITLABELS | TVS_SHOWSELALWAYS |
		WS_CHILD | WS_VISIBLE | WS_GROUP | WS_TABSTOP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

	//| WS_BORDER 

	// Create the list control.  Don't worry about specifying
	// correct coordinates.  That will be handled in OnSize()
	BOOL bResult = m_projectCtrl.Create(dwStyle, CRect(0, 0, 0, 0), this, ID_TREE_CTRL);
	m_projectCtrl.SetDragOps(XHTMLTREE_DO_SCROLL_NORMAL);


	m_wndToolBar1.Create(this, AFX_DEFAULT_TOOLBAR_STYLE|CBRS_SIZE_DYNAMIC, IDR_PROJECT_TOOLBAR1);
	m_wndToolBar1.LoadToolBar(IDR_PROJECT_TOOLBAR1, 0, 0, TRUE /* Is locked */);
	m_wndToolBar1.SetPaneStyle(m_wndToolBar1.GetPaneStyle() | CBRS_TOOLTIPS | CBRS_FLYBY);
	m_wndToolBar1.SetPaneStyle(m_wndToolBar1.GetPaneStyle() & ~(CBRS_GRIPPER | CBRS_SIZE_DYNAMIC | CBRS_BORDER_TOP | CBRS_BORDER_BOTTOM | CBRS_BORDER_LEFT | CBRS_BORDER_RIGHT));
	m_wndToolBar1.SetOwner(this);
	m_wndToolBar1.SetRouteCommandsViaFrame(FALSE);
	
	
	m_wndToolBar2.Create(this, AFX_DEFAULT_TOOLBAR_STYLE|CBRS_SIZE_DYNAMIC, IDR_PROJECT_TOOLBAR2);
	m_wndToolBar2.LoadToolBar(IDR_PROJECT_TOOLBAR2, 0, 0, TRUE /* Is locked */);
	m_wndToolBar2.SetPaneStyle(m_wndToolBar2.GetPaneStyle() | CBRS_TOOLTIPS | CBRS_FLYBY);
	m_wndToolBar2.SetPaneStyle(m_wndToolBar2.GetPaneStyle() & ~(CBRS_GRIPPER | CBRS_SIZE_DYNAMIC | CBRS_BORDER_TOP | CBRS_BORDER_BOTTOM | CBRS_BORDER_LEFT | CBRS_BORDER_RIGHT));
	m_wndToolBar2.SetOwner(this);
	m_wndToolBar2.SetRouteCommandsViaFrame(FALSE); 

	if (!m_wndStatusBar.Create(this))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

	m_wndStatusBar.SetIndicators(indicators, sizeof(indicators)/sizeof(UINT));
	m_wndStatusBar.SetOwner(this);
	m_wndStatusBar.SetPaneInfo(0, ID_SEPARATOR, SBPS_STRETCH, 0);
	

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
	{
		return;
	}

	CRect rectClient;
	GetClientRect(rectClient);

	int cyTlb = m_wndToolBar1.CalcFixedLayout(FALSE, TRUE).cy;
	

	m_wndToolBar1.SetWindowPos(NULL, rectClient.left, rectClient.top, rectClient.Width(), cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);
	m_wndToolBar2.SetWindowPos(NULL, rectClient.left, rectClient.top+ cyTlb, rectClient.Width(), cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);
	m_projectCtrl.SetWindowPos(NULL, rectClient.left + 1, rectClient.top + 2 * cyTlb + 1, rectClient.Width() - 2, rectClient.Height() - 3 * (cyTlb + 1), SWP_NOACTIVATE | SWP_NOZORDER);
	m_wndStatusBar.SetWindowPos(NULL, rectClient.left+1, rectClient.Height()-cyTlb, rectClient.Width()-2, cyTlb-1, SWP_NOACTIVATE | SWP_NOZORDER);
}

void CProjectWnd::OnContextMenu(CWnd* pWnd, CPoint point)
{
	if (pWnd == &m_projectCtrl)
	{
		((CWinAppEx*)AfxGetApp())->ShowPopupMenu(IDR_POPUP, point, this);
	}
	else
	{
		CDockablePane::OnContextMenu(pWnd, point);
	}

}

void CProjectWnd::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	CRect rectTree;
	m_projectCtrl.GetWindowRect(rectTree);
	ScreenToClient(rectTree);

	rectTree.InflateRect(1, 1);
	dc.Draw3dRect(rectTree, ::GetSysColor(COLOR_3DSHADOW), ::GetSysColor(COLOR_3DSHADOW));
	
	CRect rectStatus;
	m_wndStatusBar.GetWindowRect(rectStatus);
	ScreenToClient(rectStatus);
	rectStatus.InflateRect(1, 1);
	dc.Draw3dRect(rectStatus, ::GetSysColor(COLOR_3DSHADOW), ::GetSysColor(COLOR_3DSHADOW));
}

void CProjectWnd::OnSetFocus(CWnd* pOldWnd)
{
	CDockablePane::OnSetFocus(pOldWnd);

	m_projectCtrl.SetFocus();
}

void CProjectWnd::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	CBioSIMDoc* pDoc = (CBioSIMDoc*)GetDocument(); ASSERT(pDoc);
	string iName = pDoc->GetCurSel();

	if (lHint == CBioSIMDoc::INIT)
	{
		CExecutablePtr& pExecutable = pDoc->GetExecutable();
		CProjectStatePtr& pProjectState = pDoc->GetProjectState();
		m_projectCtrl.SetExecutable(pExecutable, pProjectState);
	}
	else if (lHint == CBioSIMDoc::SEL_CHANGE)
	{
		HTREEITEM hItem = m_projectCtrl.GetSelectedItem();
		HTREEITEM hNewItem = m_projectCtrl.FindItem(iName);
		if (hNewItem != hItem)
			m_projectCtrl.Select(hNewItem, TVGN_CARET);
	}
}




void CProjectWnd::OnSelChange(NMHDR* pNMHDR, LRESULT* pResult)
{
	CBioSIMDoc* pDoc = GetDocument();

	NMTREEVIEW* pNMTreeView = (NMTREEVIEW*)pNMHDR;
	HTREEITEM hItem = pNMTreeView->itemNew.hItem;
	//	ASSERT(hItem);

	string iName = m_projectCtrl.GetInternalName(hItem);
	pDoc->SetCurSel(iName);
	//pDoc->UpdateAllViews(this, CBioSIMDoc::SEL_CHANGE);

	*pResult = 0;
}

void CProjectWnd::OnExecute()
{
	AfxGetMainWnd()->PostMessage(WM_COMMAND, ID_EXECUTE);
}


BOOL CProjectWnd::OnOpenWorkingDir(UINT ID)
{
	CBioSIMDoc* pDoc = GetDocument();
	CBioSIMProject& project = pDoc->GetProject();

	
	HTREEITEM hItem = m_projectCtrl.GetSelectedItem();
	string iName = m_projectCtrl.GetInternalName(hItem);
	CExecutablePtr pItem = project.FindItem(iName);

	if (pItem)
	{
		string path;
		switch (ID)
		{
		case ID_OPEN_WORKING_DIR: path = pItem->GetPath(GetFM()); break;
		case ID_OPEN_OUTPUT_MAP_DIR: path = GetFM().GetOutputMapPath(); break;
		default: ASSERT(false);
		}

		ShellExecute(m_hWnd, _T("open"), Convert(path), NULL, NULL, SW_SHOW);
	}

	return TRUE;
}

void CProjectWnd::OnShowMaps()
{
	CExecutablePtr pItem = m_projectCtrl.GetSelectedExecutable();

	if (pItem)
	{
		CMapping* pMapping = dynamic_cast<CMapping*>(pItem.get());

		CExecutablePtr pParent;
		if (pMapping)
			pParent = pMapping->GetParent();

		CResultPtr pResult;
		if (pParent)
			pResult = pParent->GetResult(GetFM());

		if (pResult && pResult->Open())
		{
			size_t pSize = pMapping->GetNbMapParameterIndex(pResult);
			size_t rSize = pMapping->GetNbReplicationIndex(pResult);
			size_t tSize = pMapping->GetNbMapTimeIndex(pResult);
			size_t vSize = pMapping->GetNbMapVariableIndex(pResult);


			for (size_t p = 0; p<pSize; p++)
			{
				for (size_t r = 0; p < rSize; p++)
				{
					for (size_t t = 0; t < tSize; t++)
					{
						for (size_t v = 0; v < vSize; v++)
						{
							string fileName = pMapping->GetTEMName(pResult, p, r, t, v);
							string filePath = pMapping->GetTEMFilePath(GetFM(), fileName);
							if (FileExists(filePath) || DirectoryExists(filePath))
								WBSF::CallApplication(CRegistry::SHOWMAP, filePath, GetSafeHwnd(), SW_SHOW);
						}
					}
				}
			}
		}
	}
}


void CProjectWnd::OnShowLoc()
{
	CExecutablePtr pItem = m_projectCtrl.GetSelectedExecutable();

	if (pItem)
	{
		ERMsg msg;
		CParentInfo info;

		//CLocationVector loc;
		msg = pItem->GetParentInfo(GetFM(), info, DIMENSION::LOCATION);//pItem->GetLocationList(GetFM(), loc);
		if (msg)
		{
			string filePath = WBSF::GetTempPath() + "tmp.loc";
			msg = info.m_locations.Save(filePath);
			if (msg)
			{
				ASSERT(FileExists(filePath));
				CallApplication(CRegistry::SHOWMAP, filePath, GetSafeHwnd(), SW_SHOW);
			}
		}

		if (!msg)
			UtilWin::SYShowMessage(msg, this);
	}
}

void CProjectWnd::OnMatchStation()
{
	CExecutablePtr pItem = m_projectCtrl.GetSelectedExecutable();

	if (pItem)
	{
		ERMsg msg;

		CBioSIMDoc* pDoc = GetDocument();
		ENSURE(pDoc);
		CBioSIMProject& project = pDoc->GetProject();
		
		HTREEITEM hItem = m_projectCtrl.GetSelectedItem();

		short elemType = m_projectCtrl.GetClassType(hItem);
		short deepElemType = elemType;

		while (deepElemType != CExecutableTree::WEATHER_GENERATION)
		{
			hItem = m_projectCtrl.GetParentItem(hItem);
			if (hItem == NULL)
				break;

			deepElemType = m_projectCtrl.GetClassType(hItem);
		}
		
		if (deepElemType == CExecutableTree::WEATHER_GENERATION)
		{
			string iName = m_projectCtrl.GetInternalName(hItem);
			CExecutablePtr pExec = project.FindItem(iName);
			CWeatherGeneration* pWG = dynamic_cast<CWeatherGeneration*>(pExec.get());

			string locFilepath;
			string NormalsFilepath;
			string DailyFilepath;
			string HourlyFilepath;
			

			msg += GetFM().Loc().GetFilePath(pWG->m_locationsName, locFilepath);
			if (!pWG->m_WGInputName.empty())
			{
				string WGFilepath;
				msg += GetFM().WGInput().GetFilePath(pWG->m_WGInputName, WGFilepath);
				if (msg)
				{
					CWGInput WGInput;
					msg = WGInput.Load(WGFilepath);
					if (msg)
					{
						if (!WGInput.m_normalsDBName.empty())
							msg += GetFM().Normals().GetFilePath(WGInput.m_normalsDBName, NormalsFilepath);
						if (!WGInput.m_dailyDBName.empty())
							msg += GetFM().Daily().GetFilePath(WGInput.m_dailyDBName, DailyFilepath);
						if (!WGInput.m_hourlyDBName.empty())
							msg += GetFM().Hourly().GetFilePath(WGInput.m_hourlyDBName, HourlyFilepath);
					}
				}
			}

			if (msg)
			{
				ASSERT(FileExists(locFilepath));
			
				string command = "\"" + locFilepath + "\"";

				if (!NormalsFilepath.empty())
					command += " -n \"" + NormalsFilepath + "\"";
				
				if (!DailyFilepath.empty())
					command += " -d \"" + DailyFilepath + "\"";
				
				if (!HourlyFilepath.empty())
					command += " -h \"" + HourlyFilepath + "\"";
				
				msg = CallApplication(CRegistry::MATCH_STATION, command, GetSafeHwnd(), SW_SHOW, false);
			}
		}

		if (!msg)
			UtilWin::SYShowMessage(msg, this);
	}

}


void CProjectWnd::OnUpdateNbExecute(CCmdUI* pCmdUI)
{
	CBioSIMDoc* pDoc = GetDocument();
	ENSURE(pDoc);
	const CBioSIMProject& project = pDoc->GetProject();

	CString text;
	if (pCmdUI->m_nID == ID_INDICATOR_NB_EXECUTE)
	{
		//pCmdUI->Enable();
		CString text = GetCString(IDS_INDICATOR_NB_EXECUTE) + ToCString(project.GetNbExecute());
		pCmdUI->SetText(text);

		CDC* pDC = GetDC();
		if (pDC)
		{
			CSize size = pDC->GetTextExtent(text);

			UINT nStyle = m_wndStatusBar.GetPaneStyle(1);
			m_wndStatusBar.SetPaneInfo(1, ID_INDICATOR_NB_EXECUTE, nStyle, size.cx);
			ReleaseDC(pDC);
		}
	}
}

void CProjectWnd::OnUpdateToolBar(CCmdUI *pCmdUI)
{
	CBioSIMDoc* pDoc = (CBioSIMDoc*)GetDocument();
	ENSURE(pDoc);
	pCmdUI->Enable(pDoc->IsInit());
	
	if (pDoc->IsInit())
	{
		const CBioSIMProject& project = pDoc->GetProject();
		HTREEITEM hItem = m_projectCtrl.GetSelectedItem();
		HTREEITEM hParent = m_projectCtrl.GetParentItem(hItem);

		short elemType = m_projectCtrl.GetClassType(hItem);
		short deepElemType = elemType;
		while (deepElemType == CExecutableTree::GROUP)
		{
			hItem = m_projectCtrl.GetParentItem(hItem);
			if (hItem == NULL)
				break;

			deepElemType = m_projectCtrl.GetClassType(hItem);
		}

		switch (pCmdUI->m_nID)
		{
		case ID_EXECUTE:
		case ID_EXECUTE_MENU:		   pCmdUI->Enable(pDoc->IsInit()); break;
		case ID_MATCH_STATION:		   pCmdUI->Enable(pDoc->IsInit() && elemType != CExecutableTree::UNKNOWN); break;
		case ID_PROPERTIES:			   pCmdUI->Enable(pDoc->IsInit() && elemType != CExecutableTree::UNKNOWN); break;
		case ID_OPEN_WORKING_DIR:	   pCmdUI->Enable(pDoc->IsInit()); break;
		case ID_OPEN_OUTPUT_MAP_DIR:   pCmdUI->Enable(pDoc->IsInit() && elemType == CExecutableTree::MAPPING); break;
		case ID_SHOWMAPS:              pCmdUI->Enable(pDoc->IsInit() && elemType == CExecutableTree::MAPPING); break;
		case ID_SHOWLOC:               pCmdUI->Enable(pDoc->IsInit() && elemType != CExecutableTree::UNKNOWN); break;
		case ID_ITEM_EXPAND_ALL:       pCmdUI->Enable(pDoc->IsInit() && elemType != CExecutableTree::UNKNOWN); break;
		case ID_ITEM_COLLAPSE_ALL:     pCmdUI->Enable(pDoc->IsInit() && elemType != CExecutableTree::UNKNOWN); break;
		}
	}
	
}





CBioSIMDoc* CProjectWnd::GetDocument()
{
	CDocument* pDoc = NULL;
	CWinApp* pApp = AfxGetApp();
	if (pApp)
	{
		POSITION  pos = pApp->GetFirstDocTemplatePosition();
		CDocTemplate* docT = pApp->GetNextDocTemplate(pos);
		if (docT)
		{
			pos = docT->GetFirstDocPosition();
			pDoc = docT->GetNextDoc(pos);
		}
	}

	return static_cast<CBioSIMDoc*>(pDoc);
}

//void CProjectWnd::OnInitialUpdate()
//{
//	CBioSIMDoc* pDoc = GetDocument();
//	ASSERT(pDoc);
//	pDoc->OnInitialUpdate();
//}


BOOL CProjectWnd::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	//let the trl to route command
	/*CWnd* pFocus = GetFocus();
	if (pFocus)
	{
		CWnd* pParent = pFocus->GetParent();

		if (pFocus == &m_projectCtrl || pParent == &m_projectCtrl )
		{
			if (m_projectCtrl.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
				return TRUE;
		}
	}

	if (m_wndToolBar1.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
		return TRUE;

	if (m_wndToolBar2.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
		return TRUE;
*/
	if (m_projectCtrl.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
		return TRUE;

	return CDockablePane::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

