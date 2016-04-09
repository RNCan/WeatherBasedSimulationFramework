
// BioSIMView.cpp : implementation of the CNormalsChartWnd class
//

#include "stdafx.h"
#include "NormalsEditor.h"

#include "NormalsEditorDoc.h"
#include "NormalsChartWnd.h"
#include "UI/Common/AppOption.h"
#include "UI/Common/SYShowMessage.h"
#include "UI/ChartsProperties.h"
#include "UI/WVariablesEdit.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


using namespace std;
using namespace WBSF;
using namespace UtilWin;
using namespace WBSF::HOURLY_DATA;



static const int STD_GAP = 8;
static const int CHART_BASE_ID = 1000;
static const int SPLITTER_BASE_ID = 2000;

IMPLEMENT_SERIAL(CNormalsChartToolBar, CMFCToolBar, 1)
BOOL CNormalsChartToolBar::LoadToolBarEx(UINT uiToolbarResID, CMFCToolBarInfo& params, BOOL bLocked)
{
	if (!CMFCToolBar::LoadToolBarEx(uiToolbarResID, params, bLocked))
		return FALSE;

//*****************************
	//CMFCButton;
	//CMFCToolBarComboBoxButton zoomButton(ID_GRAPH_ZOOM, 3, WS_TABSTOP | CBS_DROPDOWNLIST, 75);
	//for (int i = 0; i < 8; i++)
	//{
	//	CString str;
	//	str.Format(_T("%dx"), 1 << i);
	//	zoomButton.AddItem(str);
	//}
	//
	//zoomButton.SelectItem(0, FALSE);
	//ReplaceButton(ID_GRAPH_ZOOM, zoomButton);
//*****************************

	UpdateTooltips();

	return TRUE;
}


//**********************************************************************
// CNormalsChartWnd construction/destruction
CNormalsEditorDoc* CNormalsChartWnd::GetDocument()
{
	CWinApp* pApp = AfxGetApp();
	if (pApp)
	{
		CFrameWnd * pFrame = (CFrameWnd *)(pApp->m_pMainWnd);
		if (pFrame && pFrame->GetSafeHwnd() != NULL)
			return (CNormalsEditorDoc*)(pFrame->GetActiveDocument());
	}
	return NULL;

}

CWeatherDatabasePtr CNormalsChartWnd::GetDatabasePtr()
{
	CWeatherDatabasePtr pDB;
	CNormalsEditorDoc* pDocument = GetDocument();

	if (pDocument)
		pDB = pDocument->GetDatabase();


	return  pDB;
}


static const int ID_CHART = 1004;

BEGIN_MESSAGE_MAP(CNormalsChartWnd, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_WINDOWPOSCHANGED()

	ON_UPDATE_COMMAND_UI_RANGE(ID_GRAPH_COPY, ID_GRAPH_ZOOM, OnUpdateToolbar)
	ON_COMMAND_RANGE(ID_GRAPH_COPY, ID_GRAPH_ZOOM, OnToolbarCommand)
	ON_CONTROL_RANGE(LBN_SELCHANGE, ID_GRAPH_COPY, ID_GRAPH_ZOOM, OnToolbarCommand)

END_MESSAGE_MAP()

CNormalsChartWnd::CNormalsChartWnd()
{
	m_bMustBeUpdated=false;
}

CNormalsChartWnd::~CNormalsChartWnd()
{}

BOOL CNormalsChartWnd::PreCreateWindow(CREATESTRUCT& cs)
{
	return CDockablePane::PreCreateWindow(cs);
}


int CNormalsChartWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	if (!m_normalsChartsCtrl.Create(this, CRect(0, 0, 0, 0), ID_CHART, WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | WS_VSCROLL))
		return FALSE;

	CreateToolBar();
	AdjustLayout();

	return 0;
}

void CNormalsChartWnd::CreateToolBar()
{
	if (m_wndToolBar.GetSafeHwnd())
		m_wndToolBar.DestroyWindow();

	m_wndToolBar.Create(this, AFX_DEFAULT_TOOLBAR_STYLE | CBRS_SIZE_DYNAMIC, IDR_TABLE_TOOLBAR);
	m_wndToolBar.LoadToolBar(IDR_GRAPH_TOOLBAR, 0, 0, TRUE);
	m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() | CBRS_TOOLTIPS | CBRS_FLYBY);
	m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() & ~(CBRS_GRIPPER | CBRS_SIZE_DYNAMIC | CBRS_BORDER_TOP | CBRS_BORDER_BOTTOM | CBRS_BORDER_LEFT | CBRS_BORDER_RIGHT));
	m_wndToolBar.SetOwner(this);
	m_wndToolBar.SetRouteCommandsViaFrame(FALSE);
	

}

void CNormalsChartWnd::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);
	
	AdjustLayout();
}

void CNormalsChartWnd::AdjustLayout()
{
	if (GetSafeHwnd() == NULL)
	{
		return;
	}

	CRect rectClient;
	GetClientRect(rectClient);

	CSize tlb = m_wndToolBar.CalcFixedLayout(FALSE, TRUE);
	

	m_wndToolBar.SetWindowPos(NULL, rectClient.left, rectClient.top, rectClient.Width(), tlb.cy, SWP_NOACTIVATE | SWP_NOZORDER);
	m_normalsChartsCtrl.SetWindowPos(NULL, rectClient.left, rectClient.top + tlb.cy, rectClient.Width(), rectClient.Height() - tlb.cy, SWP_NOACTIVATE | SWP_NOZORDER);
}

void CNormalsChartWnd::OnCopyGraph()
{
	
}

void CNormalsChartWnd::OnSaveGraph()
{
	CAppOption option;

	CString filter = GetExportImageFilter();
	option.SetCurrentProfile(_T("LastOpenPath"));
	CString lastDir = option.GetProfileString(_T("ExportImage" ));

    CFileDialog openDlg(FALSE, NULL, NULL, OFN_HIDEREADONLY, filter, this);//, sizeof(OPENFILENAME), true);
    openDlg.m_ofn.lpstrInitialDir = lastDir;
	option.SetCurrentProfile(_T("Settings"));
    openDlg.m_ofn.nFilterIndex = option.GetProfileInt(_T("ImageSaveFilterIndex"), 2);


    if( openDlg.DoModal() == IDOK)
    {
		option.SetCurrentProfile(_T("LastOpenPath"));
		option.WriteProfileString(_T("ExportImage"), openDlg.m_ofn.lpstrInitialDir);
		option.SetCurrentProfile(_T("Settings"));
		option.WriteProfileInt(_T("ImageSaveFilterIndex"), openDlg.m_ofn.nFilterIndex);
		
		string strFilename = CStringA(openDlg.GetPathName());
		if( GetFileExtension(strFilename).empty() )
			strFilename += CStringA(GetDefaultFilterExt(filter, openDlg.m_ofn.nFilterIndex-1));

		m_normalsChartsCtrl.SaveAsImage(strFilename);
    }
	
}

void CNormalsChartWnd::OnGraphOptions()
{
	CWVariables variables;
	variables.set();//emulate full variables

	CChartsProperties dlg;
	dlg.m_graphics = CNormalsChartsCtrl::GetCharts(variables);

	if (dlg.DoModal()==IDOK)
	{
		string filePath = CNormalsChartsCtrl::GetGraphisFilesPath();
		ERMsg msg = zen::SaveXML(filePath, "Graphics", "1", dlg.m_graphics);

		if (msg)
			m_normalsChartsCtrl.Update();
		else
			SYShowMessage(msg, this);
	}

}


void CNormalsChartWnd::OnUpdateToolbar(CCmdUI *pCmdUI)
{
	CNormalsEditorDoc* pDoc = (CNormalsEditorDoc*)GetDocument();
	bool bInit = pDoc->GetCurStationIndex() != UNKNOWN_POS && !pDoc->GetDataInEdition();

	switch (pCmdUI->m_nID)
	{
	case ID_GRAPH_COPY:				pCmdUI->Enable(bInit&&false); break;
	case ID_GRAPH_SAVE:				pCmdUI->Enable(bInit); break;
	case ID_GRAPH_OPTIONS:			pCmdUI->Enable(true); break;
	//case ID_GRAPH_ZOOM:				pCmdUI->Enable(bInit); break;
	//case ID_GRAPH_FILTER:			pCmdUI->Enable(bInit); break;
	}
}

void CNormalsChartWnd::OnToolbarCommand(UINT ID, NMHDR *pNMHDR, LRESULT *pResult)
{
	OnToolbarCommand(ID);
}

void CNormalsChartWnd::OnToolbarCommand(UINT ID)
{
	CNormalsEditorDoc* pDoc = (CNormalsEditorDoc*)GetDocument();
	bool bInit = pDoc->GetCurStationIndex() != UNKNOWN_POS && !pDoc->GetDataInEdition();

	int index = m_wndToolBar.CommandToIndex(ID);
	CMFCToolBarButton* pCtrl = m_wndToolBar.GetButton(index); ASSERT(pCtrl);

	if (bInit)
	{
		switch (ID)
		{
		case ID_GRAPH_COPY:				OnCopyGraph(); break;
		case ID_GRAPH_SAVE:				OnSaveGraph(); break;
		case ID_GRAPH_OPTIONS:			OnGraphOptions(); break;
		/*case ID_GRAPH_ZOOM:				
			m_normalsChartsCtrl.m_zoom = 1 << ((CMFCToolBarComboBoxButton*)pCtrl)->GetCurSel();
			m_normalsChartsCtrl.Update();
			break;*/
			
		//case ID_GRAPH_FILTER:			pDoc->SetVariables(((CMFCToolBarWVariablesButton*)pCtrl)->GetVariables()); break;
		}
	}
}


void CNormalsChartWnd::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	if (lHint == CNormalsEditorDoc::LANGUAGE_CHANGE)
	{
		CreateToolBar();
		AdjustLayout();
	}

	CNormalsEditorDoc* pDoc = (CNormalsEditorDoc*)GetDocument();
	CWeatherDatabasePtr pDB = pDoc->GetDatabase();

	int index = m_wndToolBar.CommandToIndex(ID_GRAPH_ZOOM);
	CMFCToolBarComboBoxButton* pCtrl = (CMFCToolBarComboBoxButton*)m_wndToolBar.GetButton(index); ASSERT(pCtrl);
	
	m_normalsChartsCtrl.m_pStation = pDoc->GetCurStation();
	m_normalsChartsCtrl.m_zoom = 1;// 1 << pCtrl->GetCurSel();

	
	bool bVisible = IsWindowVisible();
	if( bVisible  )
		m_normalsChartsCtrl.Update();
	else
		m_bMustBeUpdated = true;
	
}


void CNormalsChartWnd::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
	CDockablePane::OnWindowPosChanged(lpwndpos);

	if( lpwndpos->flags & SWP_SHOWWINDOW )
	{
		if( m_bMustBeUpdated )
		{
			m_normalsChartsCtrl.Update();
			m_bMustBeUpdated=false;
		}
	}
}


BOOL CNormalsChartWnd::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	//let the trl to route command
	CWnd* pFocus = GetFocus();
	if (pFocus)
	{
		CWnd* pParent = pFocus->GetParent();

		if (pFocus == &m_normalsChartsCtrl || pParent == &m_normalsChartsCtrl)
		{
			if (m_normalsChartsCtrl.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
				return TRUE;
		}
	}

	return CDockablePane::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}
