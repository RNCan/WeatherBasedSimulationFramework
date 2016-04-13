
#include "stdafx.h"

#include "Resource.h"

#include "StationsListWnd.h"
#include "NormalsEditor.h"
#include "NormalsEditorDoc.h"
#include "Basic/Registry.h"
#include "UI/Common/SYShowMessage.h"
#include "UI/WVariablesEdit.h"

using namespace std;
using namespace WBSF;



#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif



//*******************************************************************************************************

BEGIN_MESSAGE_MAP(CStationsListToolBar, CMFCToolBar)
	ON_WM_CREATE()
	ON_WM_SIZE()
END_MESSAGE_MAP()


//IMPLEMENT_SERIAL(CMFCToolBarYearsButton, CMFCToolBarEditBoxButton, 1)
IMPLEMENT_SERIAL(CStationsListToolBar, CMFCToolBar, 1)

BOOL CStationsListToolBar::LoadToolBarEx(UINT uiToolbarResID, CMFCToolBarInfo& params, BOOL bLocked)
{
	if (!CMFCToolBar::LoadToolBarEx(uiToolbarResID, params, bLocked))
		return FALSE;

	
	
	CMFCToolBarWVariablesButton filterCtrl(ID_STATION_LIST_FILTER, 3, 150);
	ReplaceButton(ID_STATION_LIST_FILTER, filterCtrl);

	
	return TRUE;
}

void CStationsListToolBar::OnSize(UINT nType, int cx, int cy)
{
	CMFCToolBar::OnSize(nType, cx, cy);
	
	//CObList buttons;

	int index = CommandToIndex(ID_STATION_LIST_FILTER);
	CMFCToolBarEditBoxButton* pCtrl = (CMFCToolBarEditBoxButton*) GetButton(index);

	if (pCtrl && pCtrl->GetEditBox())
	{
		CRect rect = pCtrl->Rect();
		rect.right = __max(rect.left+150, cx);
		
		pCtrl->SetRect(rect);
	}
	
}

//*****************************************************************************************************
//**********************************************************************************************************
static const int IDC_STATION_LIST_ID = 1002;
static const int ID_INDICATOR_NB_STATIONS = 0xE711;

static UINT indicators[] =
{
	ID_SEPARATOR,
	ID_INDICATOR_NB_STATIONS
};

IMPLEMENT_DYNCREATE(CStationsListWnd, CDockablePane)
BEGIN_MESSAGE_MAP(CStationsListWnd, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_SETTINGCHANGE()
	ON_UPDATE_COMMAND_UI_RANGE(ID_ADD_WEATHER_STATION, ID_STATION_LIST_FILTER, OnUpdateToolbar)
	ON_COMMAND_RANGE(ID_ADD_WEATHER_STATION, ID_STATION_LIST_FILTER, OnToolbarCommand)
	ON_CONTROL_RANGE(EN_KILLFOCUS, ID_STATION_LIST_FILTER, ID_STATION_LIST_FILTER, OnToolbarCommand)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_NB_STATIONS, OnUpdateStatusBar)
	ON_MESSAGE(CStationsListCtrl::UWM_SELECTION_CHANGE, OnSelectionChange)
END_MESSAGE_MAP()

CNormalsEditorDoc* CStationsListWnd::GetDocument()
{
	CNormalsEditorDoc* pDoc = NULL;
	CWinApp* pApp = AfxGetApp();
	if (pApp)
	{
		POSITION  pos = pApp->GetFirstDocTemplatePosition();
		CDocTemplate* docT = pApp->GetNextDocTemplate(pos);
		if (docT)
		{
			pos = docT->GetFirstDocPosition();
			pDoc = (CNormalsEditorDoc*)docT->GetNextDoc(pos);
		}
	}

	return pDoc;
}



CStationsListWnd::CStationsListWnd()
{

}

CStationsListWnd::~CStationsListWnd()
{
}





void CStationsListWnd::AdjustLayout()
{
	if (GetSafeHwnd () == NULL || (AfxGetMainWnd() != NULL && AfxGetMainWnd()->IsIconic()))
	{
		return;
	}

	CRect rectClient;
	GetClientRect(rectClient);


	int cyTlb = m_wndToolBar.CalcFixedLayout(FALSE, TRUE).cy;
	int cxTlb = m_wndToolBar.CalcFixedLayout(FALSE, TRUE).cx;
	
	m_wndToolBar.SetWindowPos(NULL, rectClient.left, rectClient.top, rectClient.Width(), cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);
	m_stationsList.SetWindowPos(NULL, rectClient.left, rectClient.top + cyTlb, rectClient.Width(), rectClient.Height() - 2*cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);
	m_wndStatusBar.SetWindowPos(NULL, rectClient.left, rectClient.Height() - cyTlb, rectClient.Width(), cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);
}

int CStationsListWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	CRect rectDummy;
	rectDummy.SetRectEmpty();

	if (!m_stationsList.CreateGrid(WS_CHILD | WS_VISIBLE | WS_TABSTOP, CRect(0, 0, 0, 0), this, IDC_STATION_LIST_ID))
	{
		TRACE0("Impossible de créer le controle\n");
		return -1;      // échec de la création
	}

	SetPropListFont();

	m_wndToolBar.Create(this, AFX_DEFAULT_TOOLBAR_STYLE | WS_TABSTOP, IDR_STATIONSLIST_TOOLBAR);
	m_wndToolBar.LoadToolBar(IDR_STATIONSLIST_TOOLBAR, 0, 0, TRUE);
	m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() | CBRS_TOOLTIPS | CBRS_FLYBY);
	m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() & ~(CBRS_GRIPPER | CBRS_SIZE_DYNAMIC | CBRS_BORDER_TOP | CBRS_BORDER_BOTTOM | CBRS_BORDER_LEFT | CBRS_BORDER_RIGHT));
	m_wndToolBar.SetOwner(this);
	m_wndToolBar.SetRouteCommandsViaFrame(FALSE);


	if (!m_wndStatusBar.Create(this))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}
	m_wndStatusBar.SetIndicators(indicators, sizeof(indicators) / sizeof(UINT));
	m_wndStatusBar.SetOwner(this);
	m_wndStatusBar.SetPaneInfo(0, ID_SEPARATOR, SBPS_STRETCH, 0);




	AdjustLayout();

	return 0;
}

void CStationsListWnd::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);
	AdjustLayout();
}
//
//void CStationsListWnd::OnSetFocus(CWnd* pOldWnd)
//{
//	CDockablePane::OnSetFocus(pOldWnd);
//	m_stationsList.SetFocus();
//}

void CStationsListWnd::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
	CDockablePane::OnSettingChange(uFlags, lpszSection);
	SetPropListFont();
}

void CStationsListWnd::SetPropListFont()
{
	::DeleteObject(m_fntPropList.Detach());

	LOGFONT lf;
	afxGlobalData.fontRegular.GetLogFont(&lf);

	NONCLIENTMETRICS info;
	info.cbSize = sizeof(info);

	afxGlobalData.GetNonClientMetrics(info);

	lf.lfHeight = info.lfMenuFont.lfHeight;
	lf.lfWeight = info.lfMenuFont.lfWeight;
	lf.lfItalic = info.lfMenuFont.lfItalic;

	m_fntPropList.CreateFontIndirect(&lf);
	
	m_stationsList.SetFont(&m_fntPropList);
}


void CStationsListWnd::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	BOOL bEnable = FALSE;
	CNormalsEditorDoc* pDoc = (CNormalsEditorDoc*)GetDocument();
	ASSERT(pDoc);
	bEnable = pDoc->GetDatabase()->IsOpen();


	if (lHint == CNormalsEditorDoc::INIT || lHint == CNormalsEditorDoc::STATION_LIST_PROPERTIES_FILTERS_CHANGE)
	{
		int index = m_wndToolBar.CommandToIndex(ID_STATION_LIST_FILTER);
		CMFCToolBarWVariablesButton* pCtrl = (CMFCToolBarWVariablesButton*)m_wndToolBar.GetButton(index); ASSERT(pCtrl);
		pCtrl->SetVariables(pDoc->GetFilters());
	
		CWeatherDatabasePtr pDB = pDoc->GetDatabase();
		

		m_stationsList.m_pDB = pDoc->GetDatabase();
		//m_stationsList.m_stationIndex = pDoc->GetCurStationIndex();
		m_stationsList.m_filter = pDoc->GetFilters();

		m_stationsList.Update();

	}
	else if (lHint == CNormalsEditorDoc::STATION_INDEX_CHANGE )
	{
		size_t index = pDoc->GetCurStationIndex();
		//m_stationsList.m_stationIndex = index;
		m_stationsList.SetStationIndex(pDoc->GetCurStationIndex());
		
		
	}
	else if (lHint == CNormalsEditorDoc::LOCATION_CHANGE)
	{
		size_t index = pDoc->GetCurStationIndex();
		m_stationsList.SetStationModified(index, pDoc->IsStationModified(index));
	}
	else if (lHint == CNormalsEditorDoc::DATA_PROPERTIES_EDITION_MODE_CHANGE)
	{
		m_stationsList.SetEditionMode(pDoc->GetDataInEdition());
	}
	
}


void CStationsListWnd::OnUpdateToolbar(CCmdUI *pCmdUI)
{
	CNormalsEditorDoc* pDoc = (CNormalsEditorDoc*)GetDocument();
	bool bInit = pDoc->GetDatabase()->IsOpen() && !pDoc->GetDataInEdition();
	
	switch (pCmdUI->m_nID)
	{
	case ID_ADD_WEATHER_STATION:pCmdUI->Enable(bInit); break;
	case ID_SENDTO_SHOWMAP:
	case ID_SENDTO_EXCEL:
	case ID_STATION_LIST_FILTER:pCmdUI->Enable(bInit); break;
	default: ASSERT(false);
	}

	
}


void CStationsListWnd::OnToolbarCommand(UINT ID)
{
	CNormalsEditorDoc* pDoc = (CNormalsEditorDoc*)GetDocument();
	ASSERT(pDoc);

	CNormalsDatabasePtr& pDB = pDoc->GetDatabase();

	if (pDB && pDB->IsOpen())
	{
		
		ASSERT(pDoc);
		
		if (ID == ID_ADD_WEATHER_STATION)
		{
			MessageBox(_T("Ça reste à faire..."));
		}
		else if (ID == ID_SENDTO_EXCEL || ID == ID_SENDTO_SHOWMAP)
		{
			WBSF::CRegistry registry;
			char separator = registry.GetListDelimiter();

			CSearchResultVector searchResultArray;

			CWVariables filter = pDoc->GetFilters();

			ERMsg msg;
			msg = pDB->GetStationList(searchResultArray, filter, WBSF::YEAR_NOT_INIT, true, false);

			CLocationVector loc = pDB->GetLocations(searchResultArray);
			//remove coma in name...
			loc.TrimData(separator);
			
			string filePath = WBSF::GetUserDataPath() + "tmp\\" + WBSF::GetFileTitle(pDB->GetFilePath()) + ".csv";
			WBSF::CreateMultipleDir(WBSF::GetPath(filePath));

			msg += (loc.Save(filePath, separator));
			if (msg)
			{
				if (ID == ID_SENDTO_EXCEL)
					msg += WBSF::CallApplication(WBSF::CRegistry::SPREADSHEET1, filePath, GetSafeHwnd(), SW_SHOW);
				else if (ID == ID_SENDTO_SHOWMAP)
					msg += WBSF::CallApplication(WBSF::CRegistry::SHOWMAP, filePath, GetSafeHwnd(), SW_SHOW);
			}

			//pDoc->SetOutputText(SYGetText(msg));
			if (!msg)
				UtilWin::SYShowMessage(msg, this);
		}
		else if (ID == ID_STATION_LIST_FILTER)
		{
			int index = m_wndToolBar.CommandToIndex(ID);
			CMFCToolBarWVariablesButton* pCtrl = (CMFCToolBarWVariablesButton*)m_wndToolBar.GetButton(index); ASSERT(pCtrl);
			pDoc->SetFilters(pCtrl->GetVariables());
		}
	}
}

BOOL CStationsListWnd::PreTranslateMessage(MSG* pMsg)
{
	//GetKeyState
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN )
	{
		int index2 = m_wndToolBar.CommandToIndex(ID_STATION_LIST_FILTER);
		CMFCToolBarWVariablesButton* pCtrl2 = (CMFCToolBarWVariablesButton*)m_wndToolBar.GetButton(index2); ASSERT(pCtrl2);
		if (pMsg->hwnd == pCtrl2->GetEditBox()->GetSafeHwnd())
		{
			// handle return pressed in edit control
			OnToolbarCommand(ID_STATION_LIST_FILTER);
			return TRUE; // this doesn't need processing anymore
		}

	}

	return CDockablePane::PreTranslateMessage(pMsg); // all other cases still need default processing
}



void CStationsListWnd::OnUpdateStatusBar(CCmdUI* pCmdUI)
{

	if (pCmdUI->m_nID == ID_INDICATOR_NB_STATIONS)
	{
		long nbRows = m_stationsList.GetNumberRows();

		CString str = _T("Stations = ");//GetString(IDS_NB_STATION);
		CString text = str + UtilWin::ToCString(nbRows);

		pCmdUI->SetText(text);

		CDC* pDC = GetDC();
		if (pDC)
		{
			CSize size = pDC->GetTextExtent(text);
			UINT nStyle = m_wndStatusBar.GetPaneStyle(1);
			m_wndStatusBar.SetPaneInfo(1, ID_INDICATOR_NB_STATIONS, nStyle, size.cx);
			ReleaseDC(pDC);
		}
	}


}

LRESULT  CStationsListWnd::OnSelectionChange(WPARAM, LPARAM)
{
	CNormalsEditorDoc* pDoc = (CNormalsEditorDoc*)GetDocument();
	ASSERT(pDoc);

	size_t	index = m_stationsList.GetStationIndex();
	pDoc->SetCurStationIndex(index);

	return 0;
}

