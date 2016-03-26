
#include "stdafx.h"
#include "mainfrm.h"
#include "ProjectView.h"
#include "WeatherUpdaterDoc.h"

#include "Tasks/UIEnvCanHourly.h"
#include "Tasks/UIEnvCanDaily.h"
#include "Tasks/UIEnvCanHourlyForecast.h"
#include "Tasks/UIEnvCanRadar.h"
#include "Tasks/UIEnvCanPrcpRadar.h"
#include "Tasks/UIGHCN.h"
#include "Tasks/UIGSOD.h"
#include "Tasks/UIISDLite.h"
#include "Tasks/UISnowTel.h"
#include "Tasks/UIRapidUpdateCycle.h"
#include "Tasks/UISolutionMesonetHourly.h"
#include "Tasks/UISolutionMesonetDaily.h"
#include "Tasks/UICIPRA.h"

#include "Tasks/CreateHourlyDB.h"
#include "Tasks/CreateDailyDB.h"
#include "Tasks/CreateNormalsDB.h"
#include "Tasks/MergeWeather.h"
#include "Tasks/AppendWeather.h"
#include "Tasks/ClipWeather.h"
#include "Tasks/CopyFTP.h"
#include "Tasks/ConvertDB.h"
#include "Tasks/ZipUnzip.h"

#include "Tasks/TaskFactory.h"


#include "UI/Common/SYShowMessage.h"
#include "Resource.h"
#include "WeatherBasedSimulationString.h"


using namespace std;
using namespace UtilWin;
using namespace WBSF;



#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

IMPLEMENT_SERIAL(CProjectWndToolBar, CMFCToolBar, 1)
//BOOL CProjectWndToolBar::LoadToolBarEx(UINT uiToolbarResID, CMFCToolBarInfo& params, BOOL bLocked)
//{
//	if (!CMFCToolBar::LoadToolBarEx(uiToolbarResID, params, bLocked))
//		return FALSE;
//
//	UpdateTooltips();
//	SetWindowPos(NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE | SWP_NOMOVE);
//
//	return TRUE;
//}
//


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

	ON_UPDATE_COMMAND_UI_RANGE(ID_TASK_FIRST_UPDATER, ID_TASK_OTHER_MMG, OnUpdateToolBar)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateToolBar)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdateToolBar)
	ON_UPDATE_COMMAND_UI(ID_EDIT_DUPLICATE, OnUpdateToolBar)
	ON_UPDATE_COMMAND_UI(ID_REMOVE, OnUpdateToolBar)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_NB_TASK_CHECKED, OnUpdateStatusBar)

	ON_COMMAND_RANGE(ID_TASK_FIRST_UPDATER, ID_TASK_OTHER_MMG, OnAdd)
	ON_COMMAND(ID_REMOVE, &OnRemove)
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


CTaskWnd::CTaskWnd(size_t t, UINT toolbarID1, UINT toolbarID2)
{
	m_type = t;
	m_toolbarID1 = toolbarID1;
	m_toolbarID2 = toolbarID2;
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
	int nbT = m_wndToolBar2.GetSafeHwnd() ? 2:1;

	m_wndToolBar1.SetWindowPos(NULL, rectClient.left, rectClient.top, rectClient.Width(), cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);
	if (m_wndToolBar2.GetSafeHwnd())
		m_wndToolBar2.SetWindowPos(NULL, rectClient.left, rectClient.top + cyTlb, rectClient.Width(), cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);

	m_taskCtrl.SetWindowPos(NULL, rectClient.left, rectClient.top + nbT*cyTlb, rectClient.Width(), rectClient.Height() - (nbT + 1) * cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);
	m_wndStatusBar.SetWindowPos(NULL, rectClient.left, rectClient.Height() - cyTlb, rectClient.Width(), cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);
}



void CTaskWnd::OnUpdateStatusBar(CCmdUI* pCmdUI)
{

	if (pCmdUI->m_nID == ID_INDICATOR_NB_TASK_CHECKED)
	{
		long nbRows = m_taskCtrl.GetCheckedCount();

		CString str = _T("Selected = ");//GetString(IDS_NB_SELECTED_TASK);
		CString text = str + UtilWin::ToCString(nbRows);

		pCmdUI->SetText(text);

		CDC* pDC = GetDC();
		if (pDC)
		{
			CSize size = pDC->GetTextExtent(text);

			UINT nStyle = m_wndStatusBar.GetPaneStyle(1);
			m_wndStatusBar.SetPaneInfo(1, ID_INDICATOR_NB_TASK_CHECKED, nStyle, size.cx);
		}
	}


}

UINT CTaskWnd::CtrlBaseID(UINT ID)
{
	ASSERT(ID >= ID_TASK_FIRST && ID <= ID_TASK_LAST);

	return ID - ID_TASK_FIRST;
}


UINT CTaskWnd::CtrlID(const std::string& className)
{
	UINT index = UINT(-1);
	for (UINT i = ID_TASK_FIRST; i <= ID_TASK_LAST && index == UINT(-1); i++)
	{
		if (ClassName(i) == className)
			index = i;
	}

	return index;
}

string CTaskWnd::ClassName(UINT ID)
{
	string className;

	switch (ID)
	{
	case ID_TASK_EC_DAILY:		className = CUIEnvCanDaily::CLASS_NAME(); break;
	case ID_TASK_EC_HOURLY:		className = CUIEnvCanHourly::CLASS_NAME(); break;
	case ID_TASK_EC_FORECAST:	className = CUIEnvCanHourlyForecast::CLASS_NAME(); break;
	case ID_TASK_EC_RADAR:		className = CUIEnvCanRadar::CLASS_NAME(); break;
	case ID_TASK_EC_PRCP_RADAR: className = CUIEnvCanPrcpRadar::CLASS_NAME(); break;
	case ID_TASK_NOAA_GHCND:	className = CUIGHCND::CLASS_NAME(); break;
	case ID_TASK_NOAA_GSOD:		className = CUIGSOD::CLASS_NAME(); break;
	case ID_TASK_NOAA_ISD_LITE: className = CUIISDLite::CLASS_NAME(); break;
	case ID_TASK_SNOTEL:		className = CUISnoTel::CLASS_NAME(); break;
	case ID_TASK_NOMAD_RUC:		className = CUIRapidUpdateCycle::CLASS_NAME(); break;
	case ID_TASK_SM_DAILY:		className = CUISolutionMesonetDaily::CLASS_NAME(); break;
	case ID_TASK_SM_HOURLY:		className = CUISolutionMesonetHourly::CLASS_NAME(); break;
	case ID_TASK_SM_CIPRA_HOURLY: className = CUICIPRA::CLASS_NAME(); break;
		//case ID_TASK_OTHER_DOWNLOADER: str = ::CLASS_NAME(); break;
		//case ID_TASK_EC_GRIB_FORECAST: str = ::CLASS_NAME(); break;
		//case ID_TASK_ACIS: str = ; break;
		//case ID_TASK_BC_PAWS: str = ; break;
		//case ID_TASK_BC_SNOWPILLOW: str = ; break;
	case ID_TASK_CREATE_HOURLY:	className = CCreateHourlyDB::CLASS_NAME(); break;
	case ID_TASK_CREATE_DAILY:	className = CCreateDailyDB::CLASS_NAME(); break;
	case ID_TASK_CREATE_NORMALS:className = CCreateNormalsDB::CLASS_NAME(); break;
	case ID_TASK_MERGE_DATABASE:className = CMergeWeather::CLASS_NAME(); break;
	case ID_TASK_APPEND_DATABASE:className = CAppendWeather::CLASS_NAME(); break;
	case ID_TASK_CROP_DATABASE:	className = CClipWeather::CLASS_NAME(); break;
	case ID_TASK_ZIP_UNZIP:		className = CZipUnzip::CLASS_NAME(); break;
	case ID_TASK_DOWNLOAD_UPLOAD:className = CCopyFTP::CLASS_NAME(); break;
	case ID_TASK_CONVERT_DATABASE:className = CConvertDB::CLASS_NAME(); break;
		//case ID_TASK_OTHER_TOOLS:className = ; break;
		//case ID_TASK_RCM22:className = ; break;
		//case ID_TASK_GCM10:className = ; break;
		//case ID_TASK_GCM4:className = ; break;
		//case ID_TASK_HADGEM2_10:className = ; break;
		//case ID_TASK_MIROC_10:className = ; break;
		//case ID_TASK_CESM1_CAM5_10:className = ; break;

		//default: ASSERT(false);
	}

	return className;
}

void CTaskWnd::OnUpdateToolBar(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(true);
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

void CTaskWnd::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	m_bInUpdate = true;

	if (lHint == CWeatherUpdaterDoc::INIT)
	{
		m_taskCtrl.DeleteAllItems();
		
		UINT imageIndex = ID_TASK_FIRST_TYPE - ID_TASK_FIRST + UINT(m_type);
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
			if (pTask->ClassType() == m_type)
			{
				UINT ID = CtrlID(pTask->ClassName());
				UINT imageIndex = ID - ID_TASK_FIRST;
				m_taskCtrl.InsertTask(pTask, imageIndex, m_taskCtrl.GetLastItem(m_taskCtrl.GetRootItem()));
			}
		}
	}
	else if (lHint == CWeatherUpdaterDoc::ADD_TASK)
	{
		//size_t p = pDoc->GetCurPos(m_type);
		//HTREEITEM hItem = m_taskCtrl.GetSelectedItem();
		//size_t p = m_taskCtrl.GetPosition(hItem);

		//const CTaskPtr& pTask = pDoc->GetTask(m_type, p);
		//UINT ID = CtrlID(pTask->ClassName());
		//UINT imageIndex = ID - ID_TASK_FIRST;

		//HTREEITEM hItem = m_taskCtrl.FindItem(p);
		//m_taskCtrl.InsertTask(pTask, imageIndex, hItem);
	}
	else if (lHint == CWeatherUpdaterDoc::REMOVE_TASK)
	{
		//size_t p = pDoc->GetCurPos(m_type);
		//HTREEITEM hItem = m_taskCtrl.FindItem(p);
		//ASSERT(hItem == m_taskCtrl.GetSelectedItem());
		//m_taskCtrl.DeleteItem(hItem);
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
	//else if (lHint == CWeatherUpdaterDoc::TASK_CHANGE)
	//{
	//	//only execute can change
	//	size_t p = pDoc->GetCurPos(m_type);
	//	ASSERT(p != NOT_INIT);

	//	HTREEITEM hItem = m_taskCtrl.FindItem(p);
	//	CTaskPtr& pTask = pDoc->GetTask(m_type, p);
	//	BOOL bChecked = ToBool(pTask->Get(CTaskBase::EXECUTE));
	//	if (m_taskCtrl.GetCheck(hItem) != bChecked)
	//		m_taskCtrl.SetCheck(hItem, bChecked);

	//	std::string name = pTask->Get(CTaskBase::NAME);
	//	//if (m_taskCtrl.GetIte.GetCheck(hItem) != name)
	//		//m_taskCtrl.SetCheck(hItem, bChecked);
	//	
	//}

	m_bInUpdate = false;
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
		UINT imageIndex = ID - ID_TASK_FIRST;
		
		CTaskPtr pTask = CTaskFactory::CreateObject(className);
		ENSURE(pTask.get());

		pTask->Init();
		size_t size = pDoc->GetTaskVector(m_type).size();
		pTask->m_name = /*pTask->GetTypeTitle(m_type)*/ className + to_string(size + 1);
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
	ASSERT(p != NOT_INIT);

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

	//pDoc->GetTask()->CopyToClipBoard();
}

void CTaskWnd::OnEditPaste()
{
	CWeatherUpdaterDoc* pDoc = (CWeatherUpdaterDoc*)GetDocument();
	ASSERT(pDoc);

	
	//ASSERT(p != NOT_INIT);
	
	CTaskPtr pTask = CTaskFactory::CreateFromClipbord();

	//CTaskPtr pItem = pParent->CopyFromClipBoard();
	if (pTask)
	{
		HTREEITEM hItem = m_taskCtrl.GetSelectedItem();
		size_t p = m_taskCtrl.GetPosition(hItem);

		size_t pp = (p + 1);
		pDoc->InsertTask(m_type, pp, pTask);



		//pParent->InsertItem(pItem);
		//m_taskCtrl.InsertItem(pTask, hItem);

		//pDoc->UpdateAllViews(this, CBioSIMDoc::PROJECT_CHANGE);
		//Invalidate();

		//else
		//{
		//CString 
		//AfxMessageBox(IDS_CANT_PASTE_HERE, pItem->GetClassName(), pParent->GetClassName());
		//}
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

//LRESULT CTaskWnd::OnItemExpanded(WPARAM wParam, LPARAM lParam)
//{
//	XHTMLTREEMSGDATA *pData = (XHTMLTREEMSGDATA *)wParam;
//	ASSERT(pData);
//
//	if (pData)
//	{
//		HTREEITEM hItem = pData->hItem;
//		ASSERT(hItem);
//
//		string iName = m_taskCtrl.GetInternalName(hItem);
//		bool bExpanded = m_taskCtrl.IsExpanded(hItem);
//
//		if (m_pProjectState)
//		{
//			if (bExpanded)
//				m_pProjectState->m_expendedItems.insert(iName);
//			else
//				m_pProjectState->m_expendedItems.erase(iName);
//		}
//			
//	}
//
//	return 0;
//}

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
		/*
		CString strCopyMove = _T("move");
		if (pData->bCopyDrag)
		strCopyMove = _T("copy");
		CString strItem = m_Tree.GetItemText(pData->hItem);
		TRACE(_T("starting %s drag on '%s'\n"), strCopyMove, strItem);

		if (strItem == _T("Longdog"))
		lResult = 1;

		if (m_bLog && (lResult == 0))
		m_List.Printf(CXListBox::Blue, CXListBox::White, 0,
		_T("%04d  starting %s drag on '%s'"),
		m_nLineNo++, strCopyMove, strItem);
		else if (m_bLog && (lResult == 1))
		m_List.Printf(CXListBox::Red, CXListBox::White, 0,
		_T("%04d  rejecting drag of '%s'"),
		m_nLineNo++, strItem);
		*/
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

	if (pMsg && pData && pData->hItem /*&& pDoc->IsInit()*/)
	{
		HTREEITEM hItem = pData->hItem;
		HTREEITEM hAfter = pData->hAfter;
		bool bAfter = ((UINT_PTR)pData->hAfter & 0xFFFF0000) == 0xFFFF0000;

		//HTREEITEM hParent = m_taskCtrl.GetParentItem(pData->hItem);
		//HTREEITEM hParentPasteOn = NULL;
		//if (((UINT_PTR)pData->hAfter & 0xFFFF0000) == 0xFFFF0000)
		//{
		//	hParentPasteOn = pData->hNewParent;
		//	hAfter = NULL;
		//}
		//else
		//{
		//	hParentPasteOn = m_taskCtrl.GetParentItem(pData->hAfter);
		//	bool bExpended = m_taskCtrl.IsExpanded(pData->hAfter);
		//	if (bExpended)
		//		hParentPasteOn = NULL;
		//}
		
		//ASSERT(CTaskTreeCtrl::GetParentItem(pData->hAfter) == hParent);
		//HTREEITEM hParentPasteOn = CTaskTreeCtrl::GetParentItem(pData->hAfter);

		//HTREEITEM hParentPasteOn = CTaskTreeCtrl::GetParentItem(hPasteOnItem);
		//if( !CanPaste(pItem, hPasteOnItem ) )
		//if (hParentPasteOn == hParent)
		if (hAfter)
		{
			CWeatherUpdaterDoc* pDoc = GetDocument();

			size_t pFrom = m_taskCtrl.GetPosition(hItem);
			size_t pTo = m_taskCtrl.GetPosition(hAfter);
			//pDoc->GetCurPos(m_type);
			//WBSF::CTaskPtr pTask = pDoc->GetTask(m_type, p);
			pDoc->Move(pFrom, pTo, bAfter);


			//m_pRoot->MoveItem(iName, iAfterName, pData->bCopyDrag);


			//HTREEITEM hPasteOnItem = NULL;
			//if (((UINT_PTR)pData->hAfter & 0xFFFF0000) == 0xFFFF0000)
			//hPasteOnItem = pData->hNewParent;
			//else if (pData->hAfter)
			//hPasteOnItem = pData->hAfter;
		}
		else
		{
			lResult = 1;
		}

		/*if( CanPaste(pItem, hPasteOnItem ) )
		{
		}
		else
		{
		lResult = 1;
		}*/
		//}
		//else
		//{
		// lParam = 0 ==> drag was terminated by user (left button up
		// when not on item, ESC key, right mouse button down)
		//if (m_bLog)
		//m_List.Printf(CXListBox::Red, CXListBox::White, 0, 
		//_T("%04d  drag terminated by user"), m_nLineNo++);
		//}
	}

	return lResult;	// return 0 to allow drop
}

LRESULT CTaskWnd::OnDropHover(WPARAM wParam, LPARAM lParam)
{

	XHTMLTREEMSGDATA *pMsg = (XHTMLTREEMSGDATA *)wParam;
	ASSERT(pMsg);

	XHTMLTREEDRAGMSGDATA *pData = (XHTMLTREEDRAGMSGDATA *)lParam;
	//CBioSIMDoc* pDoc = GetDocument();


	LRESULT lResult = 0;

	//if (pMsg)
	//{
	//	m_taskCtrl

	//	//CBioSIMProject& project = pDoc->GetProject();

	//	//string iName = CTaskTreeCtrl::GetInternalName(pData->hItem);
	//	//CTaskPtr pItem = project.FindItem(iName);
	//	HTREEITEM hParent = GetParentItem(pData->hItem);
	//	//HTREEITEM hPasteOnItem = pData->hAfter;
	//	//if (((UINT_PTR)pData->hAfter & 0xFFFF0000) == 0xFFFF0000)
	//	//hPasteOnItem = pData->hNewParent;
	//	//else if (pData->hAfter)
	//	//hPasteOnItem = pData->hAfter;
	//	HTREEITEM hParentPasteOn = GetParentItem(pData->hAfter);
	//	bool bExpended = IsExpanded(pData->hAfter);
	//	if (bExpended)
	//		hParentPasteOn = pData->hAfter;
	//	//{
	//	//	//look to se if it the last item
	//	//	HTREEITEM hNextItem = CTaskTreeCtrl::GetNextItem(pData->hAfter);
	//	//	HTREEITEM hParentNextItem = CTaskTreeCtrl::GetParentItem(hNextItem);
	//	//	hParentPasteOn = hParentNextItem;
	//	//}


	//	//if( !CanPaste(pItem, hPasteOnItem ) )
	//	if (hParent != hParentPasteOn)
	//		lResult = 1;

	//	CString strItem = GetItemText(hParent);
	//	CString strTextHover = GetItemText(hParentPasteOn);
	//	CString proposedNewParent = GetItemText(pData->hNewParent);
	//	TRACE(_T("*********old parent '%s' new parent '%s' proposed new parent '%s'\n"), strItem, strTextHover, pData->hNewParent ? proposedNewParent : _T("NULL"));

	//	//TRACE(_T("dragging '%s' over '%s'\n"), strItem, strTextHover);

	//	//if (strTextHover == _T("Longdog"))
	//	//lResult = 1;
	//}
	//else
	//{
	//	TRACE(_T("ERROR bad param\n"));
	//	ASSERT(FALSE);
	//}

	return lResult;
}

void CTaskWnd::OnContextMenu(CWnd* pWnd, CPoint point)
{

	CTaskTreeCtrl* pWndTree = (CTaskTreeCtrl*)(this);
	ASSERT_VALID(pWndTree);

	int classType = -1;
	if (point != CPoint(-1, -1))
	{
		// Select clicked item:
		CPoint ptTree = point;
		pWndTree->ScreenToClient(&ptTree);

		UINT flags = 0;
		HTREEITEM hTreeItem = m_taskCtrl.HitTest(ptTree, &flags);
		if (hTreeItem != NULL)
		{
			CContextMenuManager* pMM = ((CWinAppEx*)AfxGetApp())->GetContextMenuManager();
			HMENU hMenu = pMM->GetMenuByName(_T("Edit1"));
			if (hMenu != NULL)
			{
				CMenu* pMenu = CMenu::FromHandle(hMenu);
				ASSERT(pMenu);

				CMenu* pSumMenu = pMenu->GetSubMenu(0);
				ASSERT(pSumMenu);

				pMM->TrackPopupMenu(*pSumMenu, point.x, point.y, this);
			}
		}

		m_taskCtrl.SelectItem(hTreeItem);
	}

	m_taskCtrl.SetFocus();
	
}


/////////////////////////////////////////////////////////////////////////////
// CProjectView
IMPLEMENT_DYNCREATE(CProjectView, CView)

BEGIN_MESSAGE_MAP(CProjectView, CView)
	ON_WM_CREATE()
	ON_WM_SIZE()
	//ON_WM_SETFOCUS()

//	ON_COMMAND_RANGE(ID_ADD_EC_DAILY, ID_ADD_BC_SNOWPILLOW, OnAddTask)
	//ON_UPDATE_COMMAND_UI_RANGE(ID_ADD_EC_DAILY, ID_ADD_BC_SNOWPILLOW, OnUpdateToolBar)

	
END_MESSAGE_MAP()

CProjectView::CProjectView():
m_wnd1(CTaskBase::UPDATER, IDR_TASK_TOOLBAR1, IDR_TASK_TOOLBAR2),
m_wnd2(CTaskBase::TOOLS, IDR_TASK_TOOLBAR3)
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
	if (pDoc)
	{
		m_wnd1.OnUpdate(pSender, lHint, pHint);
		m_wnd2.OnUpdate(pSender, lHint, pHint);
	}

}



void CProjectView::OnSetFocus(CWnd* pOldWnd)
{
	CView::OnSetFocus(pOldWnd);

	m_wnd1.m_taskCtrl.SetFocus();
}


//
//void CProjectView::OnSelChange(NMHDR* pNMHDR, LRESULT* pResult)
//{
//	CWeatherUpdaterDoc* pDoc = (CWeatherUpdaterDoc*)GetDocument();
//
//	NMTREEVIEW* pNMTreeView = (NMTREEVIEW*)pNMHDR;
//	
//	//HTREEITEM hItem = pNMTreeView->itemNew.hItem;
////	ASSERT(hItem);
//
//	if (pNMHDR->idFrom == ID_TASK_CTRL1)
//	{
//
//	}
//	else if (pNMHDR->idFrom == ID_TASK_CTRL2)
//	{
//
//	}
//	else
//	{
//		ASSERT(false);
//	}
//
//	//string iName = m_projectCtrl.GetInternalName(hItem);
//	//pDoc->SetCurSel(iName);
//	////pDoc->UpdateAllViews(this, CWeatherUpdaterDoc::SEL_CHANGE);
//
//	*pResult = 0;
//}

void CProjectView::OnExecute()
{
	AfxGetMainWnd()->PostMessage(WM_COMMAND, ID_EXECUTE);
}


BOOL CProjectView::OnOpenWorkingDir(UINT ID)
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


void CProjectView::OnInitialUpdate()
{
	CWeatherUpdaterDoc* pDoc = static_cast<CWeatherUpdaterDoc*>(GetDocument());
	ASSERT(pDoc);
	pDoc->OnInitialUpdate();
}

BOOL CProjectView::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	
	CWnd* pFocus = GetFocus();
	if (pFocus)
	{
		CWnd* pParent = pFocus->GetParent();
		//CWnd* pOwner = pFocus->GetParentOwner();

		if (pFocus == &m_wnd1 || pParent == &m_wnd1)
		{
			if (m_wnd1.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
				return TRUE;
		}

		if(pFocus == &m_wnd2 || pParent == &m_wnd2)
		{
			if (m_wnd2.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
				return TRUE;
		}
	}
	

	return CView::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}
