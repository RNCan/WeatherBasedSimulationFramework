
// BioSIMView.cpp : implementation of the CNormalsListCtrl class
//

#include "stdafx.h"


#include "resource.h"
#include "UI/Common/AppOption.h"
#include "UI/Common/SYShowMessage.h"
#include "Basic/Registry.h"
#include "UI/WVariablesEdit.h"
#include "MatchStationDoc.h"
#include "ObservationMatchWnd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


using namespace std;
using namespace UtilWin;

using namespace WBSF;



static CMatchStationDoc* GetDocument()
{
	CWinApp* pApp = AfxGetApp();
	if (pApp)
	{
		CFrameWnd * pFrame = (CFrameWnd *)(pApp->m_pMainWnd);
		if (pFrame && pFrame->GetSafeHwnd() != NULL)
			return (CMatchStationDoc*)(pFrame->GetActiveDocument());
	}
	return NULL;

}
//
//IMPLEMENT_SERIAL(CNormalsListToolBar, CMFCToolBar, 1)
//BOOL CNormalsListToolBar::LoadToolBarEx(UINT uiToolbarResID, CMFCToolBarInfo& params, BOOL bLocked)
//{
//	if (!CMFCToolBar::LoadToolBarEx(uiToolbarResID, params, bLocked))
//		return FALSE;
//	
//	UpdateButton();
//
//	return TRUE;
//}
//
//void CNormalsListToolBar::UpdateButton()
//{
//
//	//*****************************
//	CMFCToolBarButton periodEnabled(ID_TABLE_PERIOD_ENABLED, 4);
//	periodEnabled.SetStyle((WS_TABSTOP | TBBS_CHECKBOX) & ~WS_GROUP);
//	ReplaceButton(ID_TABLE_PERIOD_ENABLED, periodEnabled);
//
//	//*****************************
//	CMFCToolBarDateTimeCtrl periodBegin(ID_TABLE_PERIOD_BEGIN, 5, WS_TABSTOP | DTS_SHORTDATECENTURYFORMAT);
//	ReplaceButton(ID_TABLE_PERIOD_BEGIN, periodBegin);
//	//*****************************
//	CMFCToolBarDateTimeCtrl periodEnd(ID_TABLE_PERIOD_END, 6, WS_TABSTOP | DTS_SHORTDATECENTURYFORMAT);
//	ReplaceButton(ID_TABLE_PERIOD_END, periodEnd);
//	//*****************************
//	CMFCToolBarWVariablesButton filterCtrl(ID_TABLE_FILTER, 7, 150);
//	ReplaceButton(ID_TABLE_FILTER, filterCtrl);
//
//	//*****************************
//	CMFCToolBarComboBoxButton statButton(ID_TABLE_STAT, 8, WS_TABSTOP | CBS_DROPDOWNLIST);
//	for (int i = 0; i < NB_STAT_TYPE; i++)
//		statButton.AddItem(CString(CStatistic::GetTitle(i)));
//
//	statButton.SelectItem(0, FALSE);
//	ReplaceButton(ID_TABLE_STAT, statButton);
//	
//	//*****************************
//	CMFCToolBarComboBoxButton TMButton(ID_TABLE_TM_TYPE, 9, WS_TABSTOP | CBS_DROPDOWNLIST, 100);
//	for (int i = 0; i <= CTM::HOURLY; i++)
//		TMButton.AddItem(CString(CTM::GetTypeTitle(i)));
//
//	TMButton.SelectItem(0, FALSE);
//	ReplaceButton(ID_TABLE_TM_TYPE, TMButton);
//
//	UpdateTooltips();
//	SetWindowPos(NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE | SWP_NOMOVE);
//	
//}
//
//void CNormalsListToolBar::AdjustLocations()
//{
//	ASSERT_VALID(this);
//
//	if (m_Buttons.IsEmpty() || GetSafeHwnd() == NULL)
//	{
//		return;
//	}
//
//	CRect rectClient;
//	GetClientRect(rectClient);
//
//	CClientDC dc(this);
//	CFont* pOldFont = SelectDefaultFont(&dc);
//	ENSURE(pOldFont != NULL);
//
//	int iStartOffset = rectClient.left + 1;
//	int iOffset = iStartOffset;
//
//	CSize sizeGrid(GetColumnWidth(), GetRowHeight());
//	CSize sizeCustButton(0, 0);
//
//	BOOL bPrevWasSeparator = FALSE;
//	int nRowActualWidth = 0;
//	for (POSITION pos = m_Buttons.GetHeadPosition(); pos != NULL;)
//	{
//		POSITION posSave = pos;
//
//		CMFCToolBarButton* pButton = (CMFCToolBarButton*)m_Buttons.GetNext(pos);
//		if (pButton == NULL)
//		{
//			break;
//		}
//		ASSERT_VALID(pButton);
//
//		BOOL bVisible = TRUE;
//
//		CSize sizeButton = pButton->OnCalculateSize(&dc, sizeGrid, TRUE);
//
//		if (pButton->m_bTextBelow )
//		{
//			sizeButton.cy = sizeGrid.cy;
//		}
//
//		if (pButton->m_nStyle & TBBS_SEPARATOR)
//		{
//			if (iOffset == iStartOffset || bPrevWasSeparator)
//			{
//				sizeButton = CSize(0, 0);
//				bVisible = FALSE;
//			}
//			else
//			{
//				bPrevWasSeparator = TRUE;
//			}
//		}
//
//		int iOffsetPrev = iOffset;
//
//		CRect rectButton;
//		rectButton.left = iOffset;
//		rectButton.right = rectButton.left + sizeButton.cx;
//		rectButton.top = rectClient.top;
//		rectButton.bottom = rectButton.top + sizeButton.cy;
//
//		iOffset += sizeButton.cx;
//		nRowActualWidth += sizeButton.cx;
//
//		pButton->Show(bVisible);
//		pButton->SetRect(rectButton);
//
//		if (bVisible)
//		{
//			bPrevWasSeparator = (pButton->m_nStyle & TBBS_SEPARATOR);
//		}
//
//		if (pButton->m_nID == ID_TABLE_SENDTO_EXCEL)
//		{
//			int size = 0;
//			//compute total length of the right control
//			for (POSITION pos2 = pos; pos2 != NULL;)
//			{
//				CMFCToolBarButton* pButton2 = (CMFCToolBarButton*)m_Buttons.GetNext(pos2);
//				if (pButton2 == NULL)
//				{
//					break;
//				}
//
//				CSize sizeButton2 = pButton2->OnCalculateSize(&dc, sizeGrid, TRUE);
//				size += sizeButton2.cx;
//
//			}
//
//			//add delta
//			int delta = Max(0,  rectClient.Width() - iOffset - 1 - size);
//			iOffset += delta;
//		}
//
//	}
//
//
//	dc.SelectObject(pOldFont);
//	UpdateTooltips();
//	RedrawCustomizeButton();
//}


//**************************************************************************************************************************************

// CNormalsListCtrl
//IMPLEMENT_DYNCREATE(CNormalsListCtrl, CStationsListCtrl)
//
//static const int IDC_GRID_ID = 1002;
//
//
//BEGIN_MESSAGE_MAP(CNormalsListCtrl, CStationsListCtrl)
//	ON_WM_CREATE()
//	ON_WM_SIZE()
//	ON_WM_WINDOWPOSCHANGED()
//END_MESSAGE_MAP()
//
//
//// CNormalsListCtrl construction/destruction
//CNormalsListCtrl::CNormalsListCtrl()
//{
//	m_bMustBeUpdated=false;
//}
//
//CNormalsListCtrl::~CNormalsListCtrl()
//{
//}
//
//// CNormalsListCtrl drawing
//
//void CNormalsListCtrl::OnDraw(CDC* pDC)
//{
//	CMatchStationDoc* pDoc = GetDocument();
//	ASSERT_VALID(pDoc);
//	if (!pDoc)
//		return;
//}
//
//int CNormalsListCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct)
//{
//	if (CStationsListCtrl::OnCreate(lpCreateStruct) == -1)
//		return -1;
//
//	return 0;
//}
//
//
//void CNormalsListCtrl::OnSize(UINT nType, int cx, int cy)
//{
//	CStationsListCtrl::OnSize(nType, cx, cy);
//	
//	AdjustLayout();
//}
//
//void CNormalsListCtrl::AdjustLayout()
//{
//	if (GetSafeHwnd() == NULL)
//	{
//		return;
//	}
//
//}
//
//void CNormalsListCtrl::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
//{
//	CMatchStationDoc* pDoc = (CMatchStationDoc*)GetDocument();
//	//CTPeriod period = pDoc->GetPeriod();
//	//bool bPeriodEnabled = pDoc->GetPeriodEnabled();
//
//	if (lHint == CMatchStationDoc::LANGUAGE_CHANGE)
//	{
//		//CStationsListCtrl::ReloadString();
//	}
//	
////	m_TM = pDoc->GetTM();
////	m_stat = pDoc->GetStatistic();
//	//m_bEditable = pDoc->GetDataInEdition();
//	//m_pStation = pDoc->GetCurStation();
//	//m_bPeriodEnabled = pDoc->GetPeriodEnabled();
//	//m_period = pDoc->GetPeriod();
//	m_filter = pDoc->GetVariables();
//
//	//std::set<int>	m_years;
//	//CWVariables		m_filter;
//
//
//	bool bVisible = IsWindowVisible();
//	if (bVisible)
//		UpdateData();
//	else
//		m_bMustBeUpdated=true;
//
//	
//}
//
//
//void CNormalsListCtrl::OnWindowPosChanged(WINDOWPOS* lpwndpos)
//{
//	CStationsListCtrl::OnWindowPosChanged(lpwndpos);
//
//	if( lpwndpos->flags & SWP_SHOWWINDOW )
//	{
//		if (m_bMustBeUpdated)
//			OnUpdate(NULL, CMatchStationDoc::INIT, NULL);
//	}
//}
//
//
//
//bool CNormalsListCtrl::SaveData()
//{
//	CWaitCursor wait;
//
//	CMatchStationDoc* pDoc = GetDocument();
//	CWeatherDatabasePtr& pDB = pDoc->GetNormalsDatabase();
//	ASSERT(pDB->IsOpen());
//
//	size_t index = pDoc->GetCurIndex();
//	if (index != UNKNOWN_POS)
//	{
//		const CLocation& location = pDoc->GetLocation(index);
//
//		//string fileName = pStation->GetDataFileName();
//		//string filePath = pDB->GetDataFilePath(fileName);
//	}
//		ERMsg msg;
//		//msg = pStation->SaveData(filePath, CTM(CTM::HOURLY));
//		//if (!msg)
//		//	SYShowMessage(msg, this);
//	
//	
//
//	return msg;
//}
//
//void CNormalsListCtrl::ExportToExcel()
//{
//	ERMsg msg;
//
//	CRegistry registry;
//	char separator = registry.GetListDelimiter();
//	
//	//if hourly data: send direcly to excel
//	CMatchStationDoc* pDoc = GetDocument();
//	CWeatherDatabasePtr& pDB = pDoc->GetNormalsDatabase();
//	ASSERT(pDB->IsOpen());
//	size_t index = pDoc->GetCurIndex();
//	if (index != UNKNOWN_POS)
//	{
//		const CLocation& location = pDoc->GetLocation(index);
//
//		//string fileName = pStation->GetDataFileName();
//
//		//string filePath;
//		//if (pDoc->GetTM() == CTM(CTM::HOURLY))
//		//{
//		//	filePath = pDB->GetDataFilePath(fileName);
//		//}
//		//else
//		//{
//		//	CWaitCursor wait;
//
//		//	//Save the data 
//		//	filePath = GetUserDataPath() + "tmp\\" + fileName;
//		//	CreateMultipleDir(GetPath(filePath));
//		//	msg += pStation->SaveData(filePath, pDoc->GetTM(), separator);
//		//}
//
//		//if (msg)
//		//{
//		//	msg = CallApplication(CRegistry::SPREADSHEET, filePath, GetSafeHwnd(), SW_SHOW, true);
//		//}
//
//		//if (!msg)
//		//	SYShowMessage(msg, this);
//	}
//}
//
//void CNormalsListCtrl::OnDateChange(UINT ID)
//{
//	ASSERT(ID == ID_TABLE_PERIOD_BEGIN || ID == ID_TABLE_PERIOD_END);
//
//	CMatchStationDoc* pDoc = (CMatchStationDoc*)GetDocument();
//
//	COleDateTime oFirstDate;
//	COleDateTime oLastDate;
//	CMFCToolBarDateTimeCtrl* pCtrl1 = CMFCToolBarDateTimeCtrl::GetByCmd(ID_TABLE_PERIOD_BEGIN); ASSERT(pCtrl1);
//	CMFCToolBarDateTimeCtrl* pCtrl2 = CMFCToolBarDateTimeCtrl::GetByCmd(ID_TABLE_PERIOD_END); ASSERT(pCtrl2);
//
//	pCtrl1->GetTime(oFirstDate);
//	pCtrl2->GetTime(oLastDate);
//
//	CTPeriod period;
//	if (oFirstDate.GetStatus() == COleDateTime::valid &&
//		oLastDate.GetStatus() == COleDateTime::valid)
//	{
//		CTRef begin(oFirstDate.GetYear(), oFirstDate.GetMonth() - 1, oFirstDate.GetDay() - 1);
//		CTRef end(oLastDate.GetYear(), oLastDate.GetMonth() - 1, oLastDate.GetDay() - 1);
//		if (begin > end)
//		{
//			if (ID == ID_TABLE_PERIOD_BEGIN)
//				end = begin;
//			else
//				begin = end;
//		}
//		period = CTPeriod(begin, end);
//	}
//
//	pDoc->SetPeriod(period);
//
//}


//
//#ifdef _DEBUG
//void CNormalsListCtrl::AssertValid() const
//{
//	//CStationsListCtrl::AssertValid();
//}
//
//void CNormalsListCtrl::Dump(CDumpContext& dc) const
//{
//	//CStationsListCtrl::Dump(dc);
//}
//
//CMatchStationDoc* CNormalsListCtrl::GetDocument() const // non-debug version is inline
//{
//	//ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CMatchStationDoc)));
//	return (CMatchStationDoc*)m_pDocument;
//}
//#endif //_DEBUG



//*********************************************************************************

static const int IDC_CONTROL_ID = 1002;

CObservationMatchWnd::CObservationMatchWnd()
{
	m_bMustBeUpdated = false;
}

CObservationMatchWnd::~CObservationMatchWnd()
{
}


BEGIN_MESSAGE_MAP(CObservationMatchWnd, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_WINDOWPOSCHANGED()
END_MESSAGE_MAP()



/////////////////////////////////////////////////////////////////////////////
// Gestionnaires de messages de CResourceViewBar

void CObservationMatchWnd::AdjustLayout()
{
	if (GetSafeHwnd() == NULL || (AfxGetMainWnd() != NULL && AfxGetMainWnd()->IsIconic()))
	{
		return;
	}

	CRect rectClient;
	GetClientRect(rectClient);


	//int cyTlb = m_wndToolBar.CalcFixedLayout(FALSE, TRUE).cy;
	//int cxTlb = m_wndToolBar.CalcFixedLayout(FALSE, TRUE).cx;

	//m_wndToolBar.SetWindowPos(NULL, rectClient.left, rectClient.top, rectClient.Width(), cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);
	m_matchStationCtrl.SetWindowPos(NULL, rectClient.left, rectClient.top, rectClient.Width(), rectClient.Height(), SWP_NOACTIVATE | SWP_NOZORDER);
	//m_wndStatusBar.SetWindowPos(NULL, rectClient.left, rectClient.Height() - cyTlb, rectClient.Width(), cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);
}

int CObservationMatchWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	m_matchStationCtrl.CreateGrid(WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), this, IDC_CONTROL_ID);


	SetPropListFont();
	AdjustLayout();

	return 0;
}

//void CObservationMatchWnd::CreateToolBar()
//{
//	if (m_wndToolBar.GetSafeHwnd())
//		m_wndToolBar.DestroyWindow();
//
//	m_wndToolBar.Create(this, AFX_DEFAULT_TOOLBAR_STYLE | CBRS_SIZE_DYNAMIC | CBRS_TOOLTIPS | CBRS_FLYBY, IDR_TABLE_TOOLBAR);
//	m_wndToolBar.LoadToolBar(IDR_TABLE_TOOLBAR, 0, 0, TRUE);
//	m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() | CBRS_TOOLTIPS | CBRS_FLYBY);
//	m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() & ~(CBRS_GRIPPER | CBRS_SIZE_DYNAMIC | CBRS_BORDER_TOP | CBRS_BORDER_BOTTOM | CBRS_BORDER_LEFT | CBRS_BORDER_RIGHT));
//	m_wndToolBar.SetOwner(this);
//	m_wndToolBar.SetRouteCommandsViaFrame(FALSE);
//}

void CObservationMatchWnd::OnSize(UINT nType, int cx, int cy)
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

void CObservationMatchWnd::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
	CDockablePane::OnSettingChange(uFlags, lpszSection);
	SetPropListFont();
}

void CObservationMatchWnd::SetPropListFont()
{
	::DeleteObject(m_font.Detach());

	LOGFONT lf;
	afxGlobalData.fontRegular.GetLogFont(&lf);

	NONCLIENTMETRICS info;
	info.cbSize = sizeof(info);

	afxGlobalData.GetNonClientMetrics(info);

	lf.lfHeight = info.lfMenuFont.lfHeight;
	lf.lfWeight = info.lfMenuFont.lfWeight;
	lf.lfItalic = info.lfMenuFont.lfItalic;

	m_font.CreateFontIndirect(&lf);

	m_matchStationCtrl.SetFont(&m_font);
}


void CObservationMatchWnd::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{

	CMatchStationDoc* pDoc = (CMatchStationDoc*)GetDocument();
	ASSERT(pDoc);



	//CTPeriod period = pDoc->GetPeriod();
	//bool bPeriodEnabled = pDoc->GetPeriodEnabled();

	if (lHint == CMatchStationDoc::LANGUAGE_CHANGE)
	{
		//	CStationsListCtrl::ReloadString();

		//CreateToolBar();
		AdjustLayout();
	}


	/*if (lHint == CMatchStationDoc::INIT || lHint == CMatchStationDoc::DATA_PROPERTIES_ENABLE_PERIOD_CHANGE)
	{
	int index = m_wndToolBar.CommandToIndex(ID_TABLE_PERIOD_ENABLED);
	CMFCToolBarButton* pCtrl = m_wndToolBar.GetButton(index); ASSERT(pCtrl);
	pCtrl->SetImage(bPeriodEnabled ? ID_TABLE_PERIOD_ENABLED - ID_TABLE_MODE_VISUALISATION : ID_TABLE_PERIOD_DESABLED - ID_TABLE_MODE_VISUALISATION);
	pCtrl->SetStyle(bPeriodEnabled ? (pCtrl->m_nStyle | TBBS_CHECKED) : (pCtrl->m_nStyle& ~TBBS_CHECKED));
	}*/

	/*if (lHint == CMatchStationDoc::INIT || lHint == CMatchStationDoc::DATA_PROPERTIES_PERIOD_CHANGE)
	{
	COleDateTime oFirstDate;
	COleDateTime oLastDate;
	if (period.IsInit())
	{
	oFirstDate = COleDateTime(period.Begin().GetYear(), (int)period.Begin().GetMonth() + 1, (int)period.Begin().GetDay() + 1, 0, 0, 0);
	oLastDate = COleDateTime(period.End().GetYear(), (int)period.End().GetMonth() + 1, (int)period.End().GetDay() + 1, 23, 59, 59);
	}
	else
	{
	oFirstDate.SetStatus(COleDateTime::null);
	oLastDate.SetStatus(COleDateTime::null);
	}

	CMFCToolBarDateTimeCtrl* pCtrl1 = CMFCToolBarDateTimeCtrl::GetByCmd(ID_TABLE_PERIOD_BEGIN); ASSERT(pCtrl1);
	CMFCToolBarDateTimeCtrl* pCtrl2 = CMFCToolBarDateTimeCtrl::GetByCmd(ID_TABLE_PERIOD_END); ASSERT(pCtrl2);

	pCtrl1->SetTime(oFirstDate);
	pCtrl2->SetTime(oLastDate);
	}*/

	//if (lHint == CMatchStationDoc::INIT || lHint == CMatchStationDoc::PROPERTIES_CHANGE)
	//{
	//	//int index = m_wndToolBar.CommandToIndex(ID_TABLE_FILTER);
	//	CMFCToolBarWVariablesButton* pCtrl = (CMFCToolBarWVariablesButton*)m_wndToolBar.GetButton(index); ASSERT(pCtrl);
	//	pCtrl->SetVariables(pDoc->GetVariables());
	//}

	/*if (lHint == CMatchStationDoc::INIT || lHint == CMatchStationDoc::DATA_PROPERTIES_STAT_CHANGE || lHint == CMatchStationDoc::LANGUAGE_CHANGE)
	{
	int index = m_wndToolBar.CommandToIndex(ID_TABLE_STAT);
	CMFCToolBarComboBoxButton* pCtrl = (CMFCToolBarComboBoxButton*)m_wndToolBar.GetButton(index); ASSERT(pCtrl);
	pCtrl->SelectItem(int(pDoc->GetStatistic()));
	}

	if (lHint == CMatchStationDoc::INIT || lHint == CMatchStationDoc::PROPERTIES_TM_CHANGE || lHint == CMatchStationDoc::LANGUAGE_CHANGE)
	{
	int index = m_wndToolBar.CommandToIndex(ID_TABLE_TM_TYPE);
	CMFCToolBarComboBoxButton* pCtrl = (CMFCToolBarComboBoxButton*)m_wndToolBar.GetButton(index); ASSERT(pCtrl);
	pCtrl->SelectItem(int(pDoc->GetTM().Type()));
	}*/


	m_matchStationCtrl.m_pDB = pDoc->GetObservationDatabase();
	//m_matchStationCtrl.m_variable = pDoc->GetVariable();
	m_matchStationCtrl.m_nearest = pDoc->GetObservationsMatch();
	//m_matchStationCtrl.m_year = pDoc->GetYear();

	if (pDoc->GetCurIndex() != UNKNOWN_POS)
		m_matchStationCtrl.m_location = pDoc->GetLocation(pDoc->GetCurIndex());




	if (IsWindowVisible())
		m_matchStationCtrl.Update();
	else
		m_bMustBeUpdated = true;

}



void CObservationMatchWnd::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
	CDockablePane::OnWindowPosChanged(lpwndpos);

	if (lpwndpos->flags & SWP_SHOWWINDOW)
	{
		if (m_bMustBeUpdated)
		{
			m_matchStationCtrl.Update();
			m_bMustBeUpdated = false;
		}
	}
}


BOOL CObservationMatchWnd::PreTranslateMessage(MSG* pMsg)
{
	//GetAsyncKeyState(VK_RETURN)
	//GetKeyState
	//if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)
	//{
	//	int index1 = m_wndToolBar.CommandToIndex(ID_STATION_LIST_YEAR);
	//	CMFCToolBarYearsButton* pCtrl1 = (CMFCToolBarYearsButton*)m_wndToolBar.GetButton(index1); ASSERT(pCtrl1);
	//	if (pMsg->hwnd == pCtrl1->GetEditBox()->GetSafeHwnd())
	//	{
	//		// handle return pressed in edit control
	//		OnToolbarCommand(ID_STATION_LIST_YEAR);
	//		return TRUE; // this doesn't need processing anymore
	//	}

	//	int index2 = m_wndToolBar.CommandToIndex(ID_STATION_LIST_FILTER);
	//	CMFCToolBarWVariablesButton* pCtrl2 = (CMFCToolBarWVariablesButton*)m_wndToolBar.GetButton(index2); ASSERT(pCtrl2);
	//	if (pMsg->hwnd == pCtrl2->GetEditBox()->GetSafeHwnd())
	//	{
	//		// handle return pressed in edit control
	//		OnToolbarCommand(ID_STATION_LIST_FILTER);
	//		return TRUE; // this doesn't need processing anymore
	//	}

	//}

	//if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_TAB)
	//{
	//	int index2 = m_wndToolBar.CommandToIndex(ID_STATION_LIST_FILTER);
	//	CMFCToolBarWVariablesButton* pCtrl2 = (CMFCToolBarWVariablesButton*)m_wndToolBar.GetButton(index2); ASSERT(pCtrl2);
	//	if (pMsg->hwnd == pCtrl2->GetEditBox()->GetSafeHwnd())
	//	{
	//		// handle return pressed in edit control
	//		OnToolbarCommand(ID_STATION_LIST_FILTER);
	//		return TRUE; // this doesn't need processing anymore
	//	}
	//
	//	// handle return pressed in edit control
	//	return TRUE; // this doesn't need processing anymore
	//}

	return CDockablePane::PreTranslateMessage(pMsg); // all other cases still need default processing
}


