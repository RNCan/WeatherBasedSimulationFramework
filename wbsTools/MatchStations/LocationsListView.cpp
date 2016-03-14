// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#include "stdafx.h"
#include "LocationsListView.h"
#include "Resource.h"
#include "MatchStationDoc.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



using namespace WBSF;


static const int IDC_STATION_LIST_ID = 1003;
using namespace HOURLY_DATA;


//***********************************************************************************************************************************

IMPLEMENT_SERIAL(CMainToolBar, CMFCToolBar, 1)
BOOL CMainToolBar::LoadToolBarEx(UINT uiToolbarResID, CMFCToolBarInfo& params, BOOL bLocked)
{
	if (!CMFCToolBar::LoadToolBarEx(uiToolbarResID, params, bLocked))
		return FALSE;


	//*****************************

	//CMFCToolBarButton periodEnabled(ID_TABLE_PERIOD_ENABLED, 4);
	//periodEnabled.SetStyle((WS_TABSTOP | TBBS_CHECKBOX) & ~WS_GROUP);
	//ReplaceButton(ID_TABLE_PERIOD_ENABLED, periodEnabled);

	//*****************************
	//CMFCToolBarDateTimeCtrl periodBegin(ID_TABLE_PERIOD_BEGIN, 5, WS_TABSTOP | DTS_SHORTDATECENTURYFORMAT);
	//ReplaceButton(ID_TABLE_PERIOD_BEGIN, periodBegin);
	//*****************************
	//CMFCToolBarDateTimeCtrl periodEnd(ID_TABLE_PERIOD_END, 6, WS_TABSTOP | DTS_SHORTDATECENTURYFORMAT);
	//ReplaceButton(ID_TABLE_PERIOD_END, periodEnd);

	//EnableTextLabels(TRUE);

	//*****************************
	CMFCToolBarEditBoxButton nbStationsButton(ID_NB_STATIONS, 2, WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 100);
	ReplaceButton(ID_NB_STATIONS, nbStationsButton);

	//SetButtonText(0, _T("Nb Station"));

	//*****************************
	CMFCToolBarComboBoxButton varButton(ID_STATION_VARIABLES, 3, WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST, 200);
	for (size_t i = 0; i < NB_VAR_H; i++)
	varButton.AddItem(CString(GetVariableTitle(i)));

	varButton.SelectItem(0, FALSE);
	ReplaceButton(ID_STATION_VARIABLES, varButton);
	//SetButtonText(1, _T("Variable"));
	//*****************************
	CMFCToolBarEditBoxButton yearButton(ID_STATION_YEAR, 4, WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 100);
	ReplaceButton(ID_STATION_YEAR, yearButton);

	UpdateTooltips();

	return TRUE;
}
	

/////////////////////////////////////////////////////////////////////////////
// CLocationsListView

IMPLEMENT_DYNCREATE(CLocationsListView, CView)

BEGIN_MESSAGE_MAP(CLocationsListView, CView)
	ON_WM_CREATE()
	ON_WM_SIZE()
	//ON_WM_CONTEXTMENU()
	ON_MESSAGE(WM_SETTEXT, OnSetText)
	ON_MESSAGE(CLocationVectorCtrl::UWM_SELECTION_CHANGE, OnSelectionChange)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_NB_LOCATIONS, OnUpdateStatusBar)
	ON_COMMAND_RANGE(ID_NB_STATIONS, ID_STATION_YEAR, OnSearchPropertyChange)
	ON_CONTROL_RANGE(CBN_SELCHANGE, ID_NB_STATIONS, ID_STATION_YEAR, OnSearchPropertyChange)
	ON_CONTROL_RANGE(EN_CHANGE, ID_NB_STATIONS, ID_STATION_YEAR, OnSearchPropertyChange)
	ON_UPDATE_COMMAND_UI_RANGE(ID_NB_STATIONS, ID_STATION_YEAR, OnUpdateSearchProperty)

END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLocationsListView construction/destruction
static const int ID_INDICATOR_NB_STATIONS = 0xE711;

static UINT indicators[] =
{
	ID_SEPARATOR,
	ID_INDICATOR_NB_STATIONS
};

CLocationsListView::CLocationsListView()
{}

CLocationsListView::~CLocationsListView()
{}


/////////////////////////////////////////////////////////////////////////////
// CLocationsListView message handlers
void CLocationsListView::OnContextMenu(CWnd*, CPoint point)
{
	//	theApp.ShowPopupMenu (IDR_CONTEXT_MENU, point, this);
}

int CLocationsListView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (!m_locationVectorCtrl.CreateGrid(WS_CHILD | WS_VISIBLE | WS_TABSTOP, CRect(0, 0, 0, 0), this, IDC_STATION_LIST_ID))
	{
		TRACE0("Impossible de créer le controle\n");
		return -1;      // échec de la création
	}

	if (!m_statusCtrl.Create(this))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}
	m_statusCtrl.SetIndicators(indicators, sizeof(indicators) / sizeof(UINT));
	m_statusCtrl.SetOwner(this);
	m_statusCtrl.SetPaneInfo(0, ID_SEPARATOR, SBPS_STRETCH, 0);


	return 0;
}

void CLocationsListView::OnDraw(CDC* pDC)
{}

LRESULT CLocationsListView::OnSetText(WPARAM wParam, LPARAM lParam)
{
	LRESULT Result = Default(); //let it do the default thing if you want

	return Result;
}

BOOL CLocationsListView::PreTranslateMessage(MSG* pMsg)
{

	return CView::PreTranslateMessage(pMsg);
}


void CLocationsListView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	BOOL bEnable = FALSE;
	CMatchStationDoc* pDoc = GetDocument();
	ASSERT(pDoc);

	//if (pDoc && pDoc->GetLocationVector())
	//{
	//bEnable = !pDoc->GetLocationVector()->GetFilePath().empty();

	if (lHint == CMatchStationDoc::INIT)
	{
		m_locationVectorCtrl.SetLocationVector(GetDocument()->GetLocationVector());
		m_locationVectorCtrl.Update();

	}

	if (lHint == CMatchStationDoc::LOCATION_INDEX_CHANGE)
	{
		m_locationVectorCtrl.SetCurIndex(pDoc->GetCurIndex());
	}


	if (lHint == CMatchStationDoc::INIT || lHint == CMatchStationDoc::PROPERTIES_CHANGE)
	{

		CMFCToolBarEditBoxButton::SetContentsAll(ID_NB_STATIONS, UtilWin::ToCString(pDoc->GetNbStation()));

		CMFCToolBarComboBoxButton* pCtrl = CMFCToolBarComboBoxButton::GetByCmd(ID_STATION_VARIABLES); ASSERT(pCtrl);
		pCtrl->SelectItem((int)pDoc->GetVariable());
		if (pDoc->GetYear() > -999)
			CMFCToolBarEditBoxButton::SetContentsAll(ID_STATION_YEAR, UtilWin::ToCString(pDoc->GetYear()));
	}
	//}


}

void CLocationsListView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);
	AdjustLayout();
}



void CLocationsListView::AdjustLayout()
{
	if (GetSafeHwnd() == NULL || (AfxGetMainWnd() != NULL && AfxGetMainWnd()->IsIconic()))
	{
		return;
	}

	CRect rect;
	GetClientRect(rect);
	int cyTlb = m_statusCtrl.CalcFixedLayout(FALSE, TRUE).cy;


	m_locationVectorCtrl.SetWindowPos(NULL, 0, 0, rect.Width(), rect.Height()-cyTlb, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE);
	m_statusCtrl.SetWindowPos(NULL, rect.left, rect.Height() - cyTlb, rect.Width(), cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);
	
}


void CLocationsListView::OnUpdateStatusBar(CCmdUI* pCmdUI)
{

	if (pCmdUI->m_nID == ID_INDICATOR_NB_LOCATIONS)
	{
		CMatchStationDoc* pDoc = GetDocument();

		long nbRows = m_locationVectorCtrl.GetNumberRows();

		CString str = UtilWin::GetCString(ID_INDICATOR_NB_LOCATIONS);
		CString text = str + UtilWin::ToCString(nbRows);

		pCmdUI->SetText(text);

		//resize space of text
		CDC* pDC = GetDC();
		ASSERT(pDC);
		CSize size = pDC->GetTextExtent(text);

		//CMFCStatusBar* pStatusBar = static_cast <CMFCStatusBar*> (pCmdUI->m_pOther);
		//UINT nStyle = pStatusBar->GetPaneStyle(1);
		//pStatusBar->SetPaneInfo(1, ID_INDICATOR_NB_LOCATIONS, nStyle, size.cx);
		UINT nStyle = m_statusCtrl.GetPaneStyle(1);
		m_statusCtrl.SetPaneInfo(1, ID_INDICATOR_NB_STATIONS, nStyle, size.cx);
	}


}



LRESULT CLocationsListView::OnSelectionChange(WPARAM, LPARAM)
{
	CMatchStationDoc* pDoc = GetDocument();
	ASSERT(pDoc);

	size_t	index = m_locationVectorCtrl.GetCurIndex();
	pDoc->SetCurIndex(index);

	return 0;
}


void CLocationsListView::OnSearchPropertyChange(UINT id)
{
	CMatchStationDoc* pDoc = GetDocument();
	ASSERT(pDoc);
	bool bInit = pDoc->GetCurIndex() != UNKNOWN_POS;


	switch (id)
	{
	case ID_NB_STATIONS:
	{
		//CMFCToolBarEditBoxButton* pCtrl = CMFCToolBarEditBoxButton::GetByCmd(ID_NB_STATIONS); ASSERT(pCtrl);
		CString text = CMFCToolBarEditBoxButton::GetContentsAll(ID_NB_STATIONS);
		size_t nbStations = (size_t)UtilWin::ToInt(text);
		if (nbStations > 0)
			pDoc->SetNbStation(nbStations);

		break;
	}

	case ID_STATION_VARIABLES:
	{
		CMFCToolBarComboBoxButton* pCtrl = CMFCToolBarComboBoxButton::GetByCmd(ID_STATION_VARIABLES); ASSERT(pCtrl);
		pDoc->SetVariable(HOURLY_DATA::TVarH(pCtrl->GetCurSel()));
		break;
	}
	case ID_STATION_YEAR:
	{
		//CMFCToolBarEditBoxButton* pCtrl = CMFCToolBarEditBoxButton::GetByCmd(ID_STATION_YEAR); ASSERT(pCtrl);
		CString text = CMFCToolBarEditBoxButton::GetContentsAll(ID_STATION_YEAR);
		if (text.IsEmpty() || text.GetLength() == 4)
			pDoc->SetYear(UtilWin::ToInt(text));
		break;
	}
	default: ASSERT(false);
	}


	//size_t	index = m_locationVectorCtrl.GetCurIndex();
	//pDoc->SetCurIndex(index);

}

void CLocationsListView::OnUpdateSearchProperty(CCmdUI* pCmdUI)
{

	CMatchStationDoc* pDoc = GetDocument();
	ASSERT(pDoc);


	/*switch (pCmdUI->m_nID)
	{
	case ID_NB_STATIONS:		pCmdUI->SetText(UtilWin::ToCString(pDoc->GetNbStation())); break;
	case ID_STATION_VARIABLES:	pCmdUI->m_nIndex = pDoc->GetVariable(); break;
	case ID_STATION_YEAR:		pCmdUI->SetText(UtilWin::ToCString(pDoc->GetYear())); break;
	default: ASSERT(false);
	}*/


	pCmdUI->Enable(TRUE);
}

void CLocationsListView::OnInitialUpdate()
{
	CMatchStationDoc* pDoc = GetDocument();
	ASSERT(pDoc);
	pDoc->OnInitialUpdate();
	pDoc->UpdateAllViews(NULL, NULL, NULL);
}

/////////////////////////////////////////////////////////////////////////////
// CLocationsListView diagnostics

#ifdef _DEBUG
void CLocationsListView::AssertValid() const
{
	CView::AssertValid();
}

void CLocationsListView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

#endif //_DEBUG