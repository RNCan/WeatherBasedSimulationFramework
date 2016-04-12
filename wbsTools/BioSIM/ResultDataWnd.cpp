
// BioSIMView.cpp : implementation of the CResultDataWnd class
//

#include "stdafx.h"
#include "BioSIM.h"
#include "FileManager/FileManager.h"
#include "BioSIMDoc.h"
#include "ResultDataWnd.h"
 

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


using namespace std;
using namespace WBSF;


CBioSIMDoc* CResultDataWnd::GetDocument()
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

// CResultDataWnd

IMPLEMENT_SERIAL(CResultToolBar, CSplittedToolBar, 1)
BOOL CResultToolBar::LoadToolBarEx(UINT uiToolbarResID, CMFCToolBarInfo& params, BOOL bLocked)
{
	if (!CMFCToolBar::LoadToolBarEx(uiToolbarResID, params, bLocked))
		return FALSE;

	
	//*****************************
	CMFCToolBarComboBoxButton statButton(ID_STATISTIC, 1, CBS_DROPDOWNLIST);
	for (int i = 0; i < NB_STAT_TYPE; i++)
		statButton.AddItem(CString(CStatistic::GetTitle(i)));

	statButton.SelectItem(1, FALSE);
	ReplaceButton(ID_STATISTIC, statButton);

	UpdateTooltips();
	SetWindowPos(NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE | SWP_NOMOVE);

	return TRUE;
}

const int IDC_GRID_ID = 1003;

BEGIN_MESSAGE_MAP(CResultDataWnd, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_WINDOWPOSCHANGED()
	ON_UPDATE_COMMAND_UI_RANGE(ID_STATISTIC, ID_STATISTIC, OnUpdateToolbar)
	ON_COMMAND_RANGE(ID_STATISTIC, ID_STATISTIC, OnToolbarCommand)

END_MESSAGE_MAP()

// CResultDataWnd construction/destruction

CResultDataWnd::CResultDataWnd()
{
	m_bMustBeUpdated=false;
}

CResultDataWnd::~CResultDataWnd()
{
}


int CResultDataWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	m_grid.CreateGrid(WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, CRect(0, 0, 0, 0), this, IDC_GRID_ID);
	m_font.CreateStockObject(DEFAULT_GUI_FONT);
	
	m_wndToolBar.Create(this, AFX_DEFAULT_TOOLBAR_STYLE|CBRS_SIZE_DYNAMIC, IDR_RESULT_TOOLBAR);
	m_wndToolBar.LoadToolBar(IDR_RESULT_TOOLBAR, 0, 0, TRUE /* Is locked */);
	m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() | CBRS_TOOLTIPS | CBRS_FLYBY);
	m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() & ~(CBRS_GRIPPER | CBRS_SIZE_DYNAMIC | CBRS_BORDER_TOP | CBRS_BORDER_BOTTOM | CBRS_BORDER_LEFT | CBRS_BORDER_RIGHT));
	m_wndToolBar.SetOwner(this);
	m_wndToolBar.SetRouteCommandsViaFrame(FALSE); 
	


	return 0;
}

void CResultDataWnd::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);
	
	AdjustLayout();
}

void CResultDataWnd::AdjustLayout()
{
	if (GetSafeHwnd() == NULL)
	{
		return;
	}

	CRect rect;
	GetClientRect(rect);

	int cxTlb = m_wndToolBar.CalcFixedLayout(FALSE, TRUE).cx+10;
	int cyTlb = m_wndToolBar.CalcFixedLayout(FALSE, TRUE).cy;

	m_wndToolBar.SetWindowPos(NULL, 0, 0, rect.Width(), cyTlb, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE);
	m_grid.SetWindowPos(NULL, rect.left, rect.top + cyTlb, rect.Width(), rect.Height() - cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);
}

void CResultDataWnd::OnUpdateToolbar(CCmdUI *pCmdUI)
{
	CBioSIMDoc* pDoc = GetDocument();
	pCmdUI->Enable(pDoc->IsInit());
	
}

void CResultDataWnd::OnToolbarCommand(UINT ID)
{
	int index = m_wndToolBar.CommandToIndex(ID);
	CMFCToolBarComboBoxButton* pCtrl = (CMFCToolBarComboBoxButton*)m_wndToolBar.GetButton(index); 
	ASSERT(pCtrl);

	int sel = pCtrl->GetCurSel(); 
	m_grid.SetStatType(sel);
}

//void CResultDataWnd::OnShowMapClicked()
//{
//}


void CResultDataWnd::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	CBioSIMDoc* pDoc = (CBioSIMDoc*)GetDocument();
	CBioSIMProject& project = pDoc->GetProject();


	string iName = pDoc->GetCurSel();
	CExecutablePtr pItem = project.FindItem(iName);

	CResultPtr result;
	if (pItem && !pDoc->IsExecute())
		result = pItem->GetResult(GetFileManager());
	
	m_grid.SetData(result);

	bool bVisible = IsWindowVisible();
	if (bVisible)
		m_grid.Update();
	else
		m_bMustBeUpdated=true;
	


}


void CResultDataWnd::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
	CDockablePane::OnWindowPosChanged(lpwndpos);

	if( lpwndpos->flags & SWP_SHOWWINDOW )
	{
		if( m_bMustBeUpdated )
		{
			m_grid.Update();
			m_bMustBeUpdated=false;
		}
	}
}

BOOL CResultDataWnd::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	//let the view to route command
	CWnd* pFocus = GetFocus();
	if (pFocus)
	{


		CWnd* pParent = pFocus->GetParent();
		CWnd* pOwner = pFocus->GetParentOwner();


		if (pFocus == &m_grid || pParent == &m_grid || pOwner == &m_grid)
		{
			if (m_grid.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
				return TRUE;
		}
	}
	
	return CDockablePane::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}
