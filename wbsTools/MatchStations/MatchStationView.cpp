
// BioSIMView.cpp : implementation of the CDPTView class
//

#include "stdafx.h"

#include "MatchStationDoc.h"
#include "MatchStationView.h"
#include "resource.h"
#include "CommonRes.h"
//#include "StudiesDefinitionsDlg.h"
#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


using namespace UtilWin;
using namespace CFL;
using namespace std;

// CDPTView
IMPLEMENT_DYNCREATE(CDPTView, CView)

const int IDC_STATISTIC_ID = 1001;
const int IDC_VALIDITY_ID = 1002;
const int IDC_GRID_ID = 1003;

BEGIN_MESSAGE_MAP(CDPTView, CView)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_WINDOWPOSCHANGED()
	ON_CBN_SELCHANGE(IDC_STATISTIC_ID, OnStatisticChange )
	ON_COMMAND_RANGE(ID_STUDIES_LIST, ID_MODE_EDIT, &CDPTView::OnCommandUI)
	ON_UPDATE_COMMAND_UI_RANGE(ID_STUDIES_LIST, ID_MODE_EDIT, &CDPTView::OnUpdateUI)
	ON_COMMAND(ID_EDIT_COPY, OnCopy)
	ON_COMMAND(ID_EDIT_PASTE, OnPaste)
END_MESSAGE_MAP()

// CDPTView construction/destruction

CDPTView::CDPTView()
{
//	m_bMustBeUpdated=false;
	//m_bEnable=false;
}

CDPTView::~CDPTView()
{
}

BOOL CDPTView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}

// CDPTView drawing

void CDPTView::OnDraw(CDC* pDC)
{
	//CBioSIMDoc* pDoc = GetDocument();
	//ASSERT_VALID(pDoc);
	//if (!pDoc)
		//return;
}

//void CDPTView::OnRButtonUp(UINT nFlags, CPoint point)
//{
//	ClientToScreen(&point);
//	OnContextMenu(this, point);
//}

//void CDPTView::OnContextMenu(CWnd* pWnd, CPoint point)
//{
	//theApp.GetContextMenuManager()->ShowPopupMenu(IDR_POPUP_EDIT, point.x, point.y, this, TRUE);
//}


// CDPTView diagnostics



// CDPTView message handlers

//CBCGPGridCtrl* CDPTView::CreateGrid ()
CUGCtrl* CDPTView::CreateGrid ()
{ 
	return (CUGCtrl*) new CResultCtrl();
}

int CDPTView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;

	
	//m_font.CreateStockObject(DEFAULT_GUI_FONT);
	//m_statisticCtrl.Create(WS_GROUP|WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_BORDER|CBS_DROPDOWNLIST , CRect(0,0,0,0), this, IDC_STATISTIC_ID );
	//m_statisticCtrl.SetCurSel(MEAN);
	//
	//m_wndToolBar.Create(this, AFX_DEFAULT_TOOLBAR_STYLE|CBRS_SIZE_DYNAMIC, IDR_RESULT_TOOLBAR);
	//m_wndToolBar.LoadToolBar(IDR_RESULT_TOOLBAR, 0, 0, TRUE /* Is locked */);
	//m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() | CBRS_TOOLTIPS | CBRS_FLYBY);
	//m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() & ~(CBRS_GRIPPER | CBRS_SIZE_DYNAMIC | CBRS_BORDER_TOP | CBRS_BORDER_BOTTOM | CBRS_BORDER_LEFT | CBRS_BORDER_RIGHT));
	//m_wndToolBar.SetOwner(this);
	//m_wndToolBar.SetRouteCommandsViaFrame(FALSE); 
	//

//	CMatchStationDoc* pDoc = GetDocument();
	m_grid.CreateGrid( WS_CHILD|WS_VISIBLE, CRect(0,0,0,0), this, IDC_GRID_ID );    
	AdjustLayout();



	return 0;
}

void CDPTView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);
	
	AdjustLayout();
}

void CDPTView::AdjustLayout()
{
	if (GetSafeHwnd() == NULL || !IsWindow(GetSafeHwnd()))
	{
		return;
	}

	CRect rectClient;
	GetClientRect(rectClient);

	
	//int cxTlb = m_wndToolBar.CalcFixedLayout(FALSE, TRUE).cx+10;
	//int cyTlb = m_wndToolBar.CalcFixedLayout(FALSE, TRUE).cy;

	//m_wndToolBar.SetWindowPos(NULL, rectClient.left, rectClient.top, rectClient.Width(), cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);
	//m_statisticCtrl.SetWindowPos(NULL, rectClient.Width()-200, rectClient.top+2, 200, cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);
	//m_grid.SetWindowPos(NULL, rectClient.left, rectClient.top + cyTlb, rectClient.Width(), rectClient.Height() -(cyTlb), SWP_NOACTIVATE | SWP_NOZORDER);
	m_grid.SetWindowPos(NULL, rectClient.left, rectClient.top, rectClient.Width(), rectClient.Height(), SWP_NOACTIVATE | SWP_NOZORDER);
}

void CDPTView::OnStatisticChange()
{
	//m_grid.SetStatType( m_statisticCtrl.GetCurSel() );
}



void CDPTView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	//CMatchStationDoc* pDoc = (CMatchStationDoc*)GetDocument();
	//CMainFrame* pMainFrm = (CMainFrame*)AfxGetMainWnd();

	//if (lHint == CMatchStationDoc::INIT)
	//{
		//pMainFrm->SetDocument(pDoc);
	//}

	m_grid.OnUpdate(pSender, lHint, pHint);
	//pMainFrm->OnUpdate(pSender, lHint, pHint);
}

//void CDPTView::UpdateResult()
//{
//}

void CDPTView::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
	CView::OnWindowPosChanged(lpwndpos);

	if( lpwndpos->flags & SWP_SHOWWINDOW )
	{
		//if( m_bMustBeUpdated )
		//{
		//	//UpdateResult();
		//	//m_bMustBeUpdated=false;
		//}
	}
}


#ifdef _DEBUG
void CDPTView::AssertValid() const
{
	CView::AssertValid();
}

void CDPTView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CMatchStationDoc* CDPTView::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CMatchStationDoc)));
	return (CMatchStationDoc*)m_pDocument;
}
#endif //_DEBUG




void CDPTView::OnCommandUI(UINT nID)
{
	//CMatchStationDoc* pDocument = pDocument = GetDocument(); ASSERT(pDocument);
	//CWeatherDatabasePtr& pProject = pDocument->m_pProject; ASSERT(pProject);
	//
	//if (nID == ID_STUDIES_LIST)
	//{
	//	CStudiesDefinitionsDlg dlg(AfxGetMainWnd());

	//	dlg.m_studiesDefinitions = pProject->m_studiesDefinitions;
	//	dlg.m_properties = pProject->m_properties;

	//	if (dlg.DoModal() == IDOK)
	//	{
	//		if (pProject->m_studiesDefinitions != dlg.m_studiesDefinitions)
	//		{
	//			pProject->m_studiesDefinitions = dlg.m_studiesDefinitions;
	//		
	//			for (CStudiesDefinitions::iterator it = pProject->m_studiesDefinitions.begin(); it != pProject->m_studiesDefinitions.end(); it++)
	//			{
	//				CStudyData& data = pProject->m_studiesData[it->first];
	//				CStudyDefinition& study = it->second;
	//				if (data.size_y() != study.GetNbVials() || data.size_x() != study.m_fields.size())
	//				{
	//					data.resize(study.GetNbVials(), study.m_fields.size());
	//				}

	//				ASSERT(data.size_y() == study.GetNbVials() && data.size_x() == study.m_fields.size());
	//			}

	//			if (pProject->m_studiesDefinitions.find(pProject->m_properties.m_studyName) == pProject->m_studiesDefinitions.end())
	//			{
	//				//current study have chnaged name
	//				pProject->m_properties.m_studyName.clear();
	//			}
	//			
	//			//remove old data
	//			
	//			pDocument->UpdateAllViews(NULL, CMatchStationDoc::DEFINITION_CHANGE, NULL);
	//		}
	//		
	//	}
	//}
	////else if (nID == ID_SHOW_ALL || nID == ID_SHOW_EMPTY || nID == ID_SHOW_NON_EMPTY)
	////{
	////	//pDocument->m_showType = nID - ID_SHOW_ALL;
	////}
	//else if (nID == ID_READ_ONLY || nID == ID_MODE_EDIT)
	//{
	//	pProject->m_properties.m_bEditable = nID == ID_MODE_EDIT;
	//	GetDocument()->UpdateAllViews(NULL, CMatchStationDoc::PROPERTIES_CHANGE, NULL);
	//}
	
}

void CDPTView::OnUpdateUI(CCmdUI *pCmdUI)
{
	//bool bEnable = false;

	//CMatchStationDoc* pDocument = pDocument = GetDocument(); ASSERT(pDocument);
	//CWeatherDatabasePtr pProject = pDocument->m_pProject;
	//if (pProject)
	//{
	//	const CDPTProjectProperties& properties = pProject->m_properties;
	//	switch (pCmdUI->m_nID)
	//	{
	//	case ID_STUDIES_LIST: break;
	//		//case ID_SHOW_ALL: pCmdUI->SetRadio(); break;
	//		//case ID_SHOW_EMPTY: pCmdUI->SetRadio(); break;
	//		//case ID_SHOW_NON_EMPTY: pCmdUI->SetRadio(); break;
	//	case ID_READ_ONLY: pCmdUI->SetRadio(!properties.m_bEditable); break;
	//	case ID_MODE_EDIT: pCmdUI->SetRadio(properties.m_bEditable); break;
	//	default: ASSERT(FALSE);
	//	}
	//	
	//	bEnable = true;
	//}
	//
	//pCmdUI->Enable(bEnable);
}


void CDPTView::OnInitialUpdate()
{
	CMatchStationDoc* pDoc = (CMatchStationDoc*)GetDocument();
	pDoc->UpdateAllViews(NULL, 0, NULL);
	//CMainFrame* pMainFrm = (CMainFrame*)AfxGetMainWnd();
	//pMainFrm->OnUpdate(NULL, 0, NULL);

}


void CDPTView::OnCopy()
{
	m_grid.CopySelected();
}

void CDPTView::OnPaste()
{
	m_grid.Paste();
}
