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
#include "LocationsListWnd.h"
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

//IMPLEMENT_SERIAL(CMainToolBar, CMFCToolBar, 1)
//BOOL CMainToolBar::LoadToolBarEx(UINT uiToolbarResID, CMFCToolBarInfo& params, BOOL bLocked)
//{
//	if (!CMFCToolBar::LoadToolBarEx(uiToolbarResID, params, bLocked))
//		return FALSE;
//
//	//*****************************
//	CMFCToolBarEditBoxButton nbStationsButton(ID_NB_STATIONS, 2, WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 100);
//	ReplaceButton(ID_NB_STATIONS, nbStationsButton);
//
//	//SetButtonText(0, _T("Nb Station"));
//
//	//*****************************
//	CMFCToolBarComboBoxButton varButton(ID_STATION_VARIABLES, 3, WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST, 200);
//	for (size_t i = 0; i < NB_VAR_H; i++)
//	varButton.AddItem(CString(GetVariableTitle(i)));
//
//	varButton.SelectItem(0, FALSE);
//	ReplaceButton(ID_STATION_VARIABLES, varButton);
//	//SetButtonText(1, _T("Variable"));
//	//*****************************
//	CMFCToolBarEditBoxButton yearButton(ID_STATION_YEAR, 4, WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 100);
//	ReplaceButton(ID_STATION_YEAR, yearButton);
//
//	UpdateTooltips();
//
//	return TRUE;
//}
//	

/////////////////////////////////////////////////////////////////////////////
// CLocationsListWnd

static const int ID_INDICATOR_NB_STATIONS = 0xE711;

static UINT indicators[] =
{
	ID_SEPARATOR,
	ID_INDICATOR_NB_STATIONS
};
IMPLEMENT_DYNCREATE(CLocationsListWnd, CDockablePane)

BEGIN_MESSAGE_MAP(CLocationsListWnd, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_MESSAGE(CLocationVectorCtrl::UWM_SELECTION_CHANGE, OnSelectionChange)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_NB_LOCATIONS, OnUpdateStatusBar)
	/*ON_COMMAND_RANGE(ID_NB_STATIONS, ID_STATION_YEAR, OnSearchPropertyChange)
	ON_CONTROL_RANGE(CBN_SELCHANGE, ID_NB_STATIONS, ID_STATION_YEAR, OnSearchPropertyChange)
	ON_CONTROL_RANGE(EN_CHANGE, ID_NB_STATIONS, ID_STATION_YEAR, OnSearchPropertyChange)
	ON_UPDATE_COMMAND_UI_RANGE(ID_NB_STATIONS, ID_STATION_YEAR, OnUpdateSearchProperty)
*/
END_MESSAGE_MAP()



CMatchStationDoc* CLocationsListWnd::GetDocument()
{
	CMatchStationDoc* pDoc = NULL;
	CWinApp* pApp = AfxGetApp();
	if (pApp)
	{
		POSITION  pos = pApp->GetFirstDocTemplatePosition();
		CDocTemplate* docT = pApp->GetNextDocTemplate(pos);
		if (docT)
		{
			pos = docT->GetFirstDocPosition();
			pDoc = (CMatchStationDoc*)docT->GetNextDoc(pos);
		}
	}

	return pDoc;
}


CLocationsListWnd::CLocationsListWnd()
{}

CLocationsListWnd::~CLocationsListWnd()
{}


/////////////////////////////////////////////////////////////////////////////
// CLocationsListWnd message handlers
void CLocationsListWnd::OnContextMenu(CWnd*, CPoint point)
{
	//	theApp.ShowPopupMenu (IDR_CONTEXT_MENU, point, this);
}

int CLocationsListWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
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



BOOL CLocationsListWnd::PreTranslateMessage(MSG* pMsg)
{

	return CDockablePane::PreTranslateMessage(pMsg);
}


void CLocationsListWnd::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	BOOL bEnable = FALSE;
	CMatchStationDoc* pDoc = GetDocument();
	ASSERT(pDoc);

	if (lHint == CMatchStationDoc::INIT)
	{
		m_locationVectorCtrl.SetLocationVector(GetDocument()->GetLocations());
		m_locationVectorCtrl.Update();

	}

	if (lHint == CMatchStationDoc::LOCATION_INDEX_CHANGE)
	{
		m_locationVectorCtrl.SetCurIndex(pDoc->GetCurIndex());
	}

}

void CLocationsListWnd::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);
	AdjustLayout();
}



void CLocationsListWnd::AdjustLayout()
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


void CLocationsListWnd::OnUpdateStatusBar(CCmdUI* pCmdUI)
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
		if (pDC)
		{
			CSize size = pDC->GetTextExtent(text);
			UINT nStyle = m_statusCtrl.GetPaneStyle(1);
			m_statusCtrl.SetPaneInfo(1, ID_INDICATOR_NB_STATIONS, nStyle, size.cx);
			ReleaseDC(pDC);
		}
	}


}



LRESULT CLocationsListWnd::OnSelectionChange(WPARAM, LPARAM)
{
	CMatchStationDoc* pDoc = GetDocument();
	ASSERT(pDoc);

	size_t	index = m_locationVectorCtrl.GetCurIndex();
	pDoc->SetCurIndex(index);

	return 0;
}

//
//void CLocationsListWnd::OnSearchPropertyChange(UINT id)
//{
//	CMatchStationDoc* pDoc = GetDocument();
//	ASSERT(pDoc);
//	bool bInit = pDoc->GetCurIndex() != UNKNOWN_POS;
//
//
//	switch (id)
//	{
//	case ID_NB_STATIONS:
//	{
//		//CMFCToolBarEditBoxButton* pCtrl = CMFCToolBarEditBoxButton::GetByCmd(ID_NB_STATIONS); ASSERT(pCtrl);
//		CString text = CMFCToolBarEditBoxButton::GetContentsAll(ID_NB_STATIONS);
//		size_t nbStations = (size_t)UtilWin::ToInt(text);
//		if (nbStations > 0)
//			pDoc->SetNbStation(nbStations);
//
//		break;
//	}
//
//	case ID_STATION_VARIABLES:
//	{
//		CMFCToolBarComboBoxButton* pCtrl = CMFCToolBarComboBoxButton::GetByCmd(ID_STATION_VARIABLES); ASSERT(pCtrl);
//		pDoc->SetVariable(HOURLY_DATA::TVarH(pCtrl->GetCurSel()));
//		break;
//	}
//	case ID_STATION_YEAR:
//	{
//		//CMFCToolBarEditBoxButton* pCtrl = CMFCToolBarEditBoxButton::GetByCmd(ID_STATION_YEAR); ASSERT(pCtrl);
//		CString text = CMFCToolBarEditBoxButton::GetContentsAll(ID_STATION_YEAR);
//		if (text.IsEmpty() || text.GetLength() == 4)
//			pDoc->SetYear(UtilWin::ToInt(text));
//		break;
//	}
//	default: ASSERT(false);
//	}
//
//
//	//size_t	index = m_locationVectorCtrl.GetCurIndex();
//	//pDoc->SetCurIndex(index);
//
//}
//
//void CLocationsListWnd::OnUpdateSearchProperty(CCmdUI* pCmdUI)
//{
//
//	CMatchStationDoc* pDoc = GetDocument();
//	ASSERT(pDoc);
//
//
//	/*switch (pCmdUI->m_nID)
//	{
//	case ID_NB_STATIONS:		pCmdUI->SetText(UtilWin::ToCString(pDoc->GetNbStation())); break;
//	case ID_STATION_VARIABLES:	pCmdUI->m_nIndex = pDoc->GetVariable(); break;
//	case ID_STATION_YEAR:		pCmdUI->SetText(UtilWin::ToCString(pDoc->GetYear())); break;
//	default: ASSERT(false);
//	}*/
//
//
//	pCmdUI->Enable(TRUE);
//}
