
// BioSIMView.cpp : implementation of the CNormalsListCtrl class
//

#include "stdafx.h"


#include "Basic/Registry.h"
#include "UI/Common/AppOption.h"
#include "UI/Common/SYShowMessage.h"
#include "UI/WVariablesEdit.h"


#include "MatchStationApp.h"
#include "MatchStationDoc.h"
#include "NormalsMatchWnd.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


using namespace std;
using namespace UtilWin;
namespace WBSF
{


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

	IMPLEMENT_SERIAL(CNormalsListToolBar, CMFCToolBar, 1)
		BOOL CNormalsListToolBar::LoadToolBarEx(UINT uiToolbarResID, CMFCToolBarInfo& params, BOOL bLocked)
	{
		if (!CMFCToolBar::LoadToolBarEx(uiToolbarResID, params, bLocked))
			return FALSE;

		UpdateButton();

		return TRUE;
	}

	void CNormalsListToolBar::UpdateButton()
	{

		//*****************************
		CMFCToolBarButton periodEnabled(ID_TABLE_PERIOD_ENABLED, 4);
		periodEnabled.SetStyle((WS_TABSTOP | TBBS_CHECKBOX) & ~WS_GROUP);
		ReplaceButton(ID_TABLE_PERIOD_ENABLED, periodEnabled);

		//*****************************
		CMFCToolBarDateTimeCtrl periodBegin(ID_TABLE_PERIOD_BEGIN, 5, WS_TABSTOP | DTS_SHORTDATECENTURYFORMAT);
		ReplaceButton(ID_TABLE_PERIOD_BEGIN, periodBegin);
		//*****************************
		CMFCToolBarDateTimeCtrl periodEnd(ID_TABLE_PERIOD_END, 6, WS_TABSTOP | DTS_SHORTDATECENTURYFORMAT);
		ReplaceButton(ID_TABLE_PERIOD_END, periodEnd);
		//*****************************
		CMFCToolBarWVariablesButton filterCtrl(ID_TABLE_FILTER, 7, 150);
		ReplaceButton(ID_TABLE_FILTER, filterCtrl);

		//*****************************
		CMFCToolBarComboBoxButton statButton(ID_TABLE_STAT, 8, WS_TABSTOP | CBS_DROPDOWNLIST);
		for (int i = 0; i < NB_STAT_TYPE; i++)
			statButton.AddItem(CString(CStatistic::GetTitle(i)));

		statButton.SelectItem(0, FALSE);
		ReplaceButton(ID_TABLE_STAT, statButton);

		//*****************************
		CMFCToolBarComboBoxButton TMButton(ID_TABLE_TM_TYPE, 9, WS_TABSTOP | CBS_DROPDOWNLIST, 100);
		for (int i = 0; i <= CTM::HOURLY; i++)
			TMButton.AddItem(CString(CTM::GetTypeTitle(i)));

		TMButton.SelectItem(0, FALSE);
		ReplaceButton(ID_TABLE_TM_TYPE, TMButton);

		UpdateTooltips();
		SetWindowPos(NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE | SWP_NOMOVE);

	}

	void CNormalsListToolBar::AdjustLocations()
	{
		ASSERT_VALID(this);

		if (m_Buttons.IsEmpty() || GetSafeHwnd() == NULL)
		{
			return;
		}

		CRect rectClient;
		GetClientRect(rectClient);

		CClientDC dc(this);
		CFont* pOldFont = SelectDefaultFont(&dc);
		ENSURE(pOldFont != NULL);

		int iStartOffset = rectClient.left + 1;
		int iOffset = iStartOffset;

		CSize sizeGrid(GetColumnWidth(), GetRowHeight());
		CSize sizeCustButton(0, 0);

		BOOL bPrevWasSeparator = FALSE;
		int nRowActualWidth = 0;
		for (POSITION pos = m_Buttons.GetHeadPosition(); pos != NULL;)
		{
			POSITION posSave = pos;

			CMFCToolBarButton* pButton = (CMFCToolBarButton*)m_Buttons.GetNext(pos);
			if (pButton == NULL)
			{
				break;
			}
			ASSERT_VALID(pButton);

			BOOL bVisible = TRUE;

			CSize sizeButton = pButton->OnCalculateSize(&dc, sizeGrid, TRUE);

			if (pButton->m_bTextBelow)
			{
				sizeButton.cy = sizeGrid.cy;
			}

			if (pButton->m_nStyle & TBBS_SEPARATOR)
			{
				if (iOffset == iStartOffset || bPrevWasSeparator)
				{
					sizeButton = CSize(0, 0);
					bVisible = FALSE;
				}
				else
				{
					bPrevWasSeparator = TRUE;
				}
			}

			int iOffsetPrev = iOffset;

			CRect rectButton;
			rectButton.left = iOffset;
			rectButton.right = rectButton.left + sizeButton.cx;
			rectButton.top = rectClient.top;
			rectButton.bottom = rectButton.top + sizeButton.cy;

			iOffset += sizeButton.cx;
			nRowActualWidth += sizeButton.cx;

			pButton->Show(bVisible);
			pButton->SetRect(rectButton);

			if (bVisible)
			{
				bPrevWasSeparator = (pButton->m_nStyle & TBBS_SEPARATOR);
			}

			if (pButton->m_nID == ID_TABLE_SENDTO_EXCEL)
			{
				int size = 0;
				//compute total length of the right control
				for (POSITION pos2 = pos; pos2 != NULL;)
				{
					CMFCToolBarButton* pButton2 = (CMFCToolBarButton*)m_Buttons.GetNext(pos2);
					if (pButton2 == NULL)
					{
						break;
					}

					CSize sizeButton2 = pButton2->OnCalculateSize(&dc, sizeGrid, TRUE);
					size += sizeButton2.cx;

				}

				//add delta
				int delta = max(0, rectClient.Width() - iOffset - 1 - size);
				iOffset += delta;
			}

		}


		dc.SelectObject(pOldFont);
		UpdateTooltips();
		RedrawCustomizeButton();
	}


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

	static const int IDC_NORMALS_MATCH_ID = 1002;
	//static const int ID_INDICATOR_NB_STATIONS = 0xE711;
	//
	//static UINT indicators[] =
	//{
	//	ID_SEPARATOR,
	//	ID_INDICATOR_NB_STATIONS
	//};


	CNormalsMatchWnd::CNormalsMatchWnd()
	{
		m_bMustBeUpdated = false;
	}

	CNormalsMatchWnd::~CNormalsMatchWnd()
	{
	}


	BEGIN_MESSAGE_MAP(CNormalsMatchWnd, CDockablePane)
		ON_WM_CREATE()
		ON_WM_SIZE()
		ON_WM_WINDOWPOSCHANGED()
		//	ON_WM_SETTINGCHANGE()
		//ON_UPDATE_COMMAND_UI_RANGE(ID_TABLE_MODE_VISUALISATION, ID_TABLE_TM_TYPE, OnUpdateToolbar)
		//ON_COMMAND_RANGE(ID_TABLE_MODE_VISUALISATION, ID_TABLE_TM_TYPE, OnToolbarCommand)
		//ON_CONTROL_RANGE(LBN_SELCHANGE, ID_TABLE_MODE_VISUALISATION, ID_TABLE_TM_TYPE, OnToolbarCommand)

		//ON_UPDATE_COMMAND_UI(ID_INDICATOR_NB_STATIONS, OnUpdateStatusBar)
		//ON_MESSAGE(UWM_SELECTION_CHANGE, OnSelectionChange)
	END_MESSAGE_MAP()



	/////////////////////////////////////////////////////////////////////////////
	// Gestionnaires de messages de CResourceViewBar

	void CNormalsMatchWnd::AdjustLayout()
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
		m_wndNormalsList.SetWindowPos(NULL, rectClient.left, rectClient.top, rectClient.Width(), rectClient.Height(), SWP_NOACTIVATE | SWP_NOZORDER);
		//m_wndStatusBar.SetWindowPos(NULL, rectClient.left, rectClient.Height() - cyTlb, rectClient.Width(), cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);
	}

	int CNormalsMatchWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
	{
		if (CDockablePane::OnCreate(lpCreateStruct) == -1)
			return -1;


		//CreateToolBar();
		m_wndNormalsList.CreateGrid(WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), this, IDC_NORMALS_MATCH_ID);




		//if (!m_wndStatusBar.Create(this))
		//{
		//	TRACE0("Failed to create status bar\n");
		//	return -1;      // fail to create
		//}
		//m_wndStatusBar.SetIndicators(indicators, sizeof(indicators) / sizeof(UINT));
		//m_wndStatusBar.SetOwner(this);
		//m_wndStatusBar.SetPaneInfo(0, ID_SEPARATOR, SBPS_STRETCH, 0);


		SetPropListFont();
		AdjustLayout();

		return 0;
	}

	//void CNormalsMatchWnd::CreateToolBar()
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

	void CNormalsMatchWnd::OnSize(UINT nType, int cx, int cy)
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

	void CNormalsMatchWnd::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
	{
		CDockablePane::OnSettingChange(uFlags, lpszSection);
		SetPropListFont();
	}

	void CNormalsMatchWnd::SetPropListFont()
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

		m_wndNormalsList.SetFont(&m_font);
	}


	void CNormalsMatchWnd::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
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


		m_wndNormalsList.m_pDB = pDoc->GetNormalsDatabase();
		//	m_wndNormalsList.m_variable = pDoc->GetVariable();
		m_wndNormalsList.m_nearest = pDoc->GetNormalsMatch();
		//	m_wndNormalsList.m_year = pDoc->GetYear();

		if (pDoc->GetCurIndex() != UNKNOWN_POS)
			m_wndNormalsList.m_location = pDoc->GetLocation(pDoc->GetCurIndex());




		if (IsWindowVisible())
			m_wndNormalsList.Update();
		else
			m_bMustBeUpdated = true;

	}



	void CNormalsMatchWnd::OnWindowPosChanged(WINDOWPOS* lpwndpos)
	{
		CDockablePane::OnWindowPosChanged(lpwndpos);

		if (lpwndpos->flags & SWP_SHOWWINDOW)
		{
			if (m_bMustBeUpdated)
			{
				m_wndNormalsList.Update();
				m_bMustBeUpdated = false;
			}

		}
	}



	//void CNormalsMatchWnd::OnUpdateToolbar(CCmdUI *pCmdUI)
	//{
	//	CMatchStationDoc* pDoc = GetDocument();
	//	
	//	bool bInit = pDoc->GetCurIndex() != UNKNOWN_POS;
	//	//bool bPeriodEnable = pDoc->GetPeriodEnabled();
	//	CWVariables variables = pDoc->GetVariables();
	//
	//
	//	switch (pCmdUI->m_nID)
	//	{
	//	//case ID_TABLE_MODE_VISUALISATION:pCmdUI->Enable(bInit); pCmdUI->SetCheck(!pDoc->GetDataInEdition()); break;
	//	//case ID_TABLE_MODE_EDITION:	pCmdUI->Enable(bInit); pCmdUI->SetCheck(pDoc->GetDataInEdition()); break;
	//	case ID_TABLE_SAVE:			pCmdUI->Enable(bInit && pDoc->GetDataInEdition()); break;
	//	case ID_TABLE_SENDTO_EXCEL:	pCmdUI->Enable(bInit && !pDoc->GetDataInEdition()); break;
	//	case ID_TABLE_PERIOD_ENABLED:	pCmdUI->SetCheck(bPeriodEnable);  pCmdUI->Enable(bInit); break;
	//	case ID_TABLE_PERIOD_BEGIN:	pCmdUI->Enable(bInit&&bPeriodEnable); break;
	//	case ID_TABLE_PERIOD_END:	pCmdUI->Enable(bInit&&bPeriodEnable); break;
	//	case ID_TABLE_FILTER:		pCmdUI->Enable(bInit); break;
	//	case ID_TABLE_STAT:			pCmdUI->Enable(bInit && !pDoc->GetDataInEdition()); break;
	//	case ID_TABLE_TM_TYPE:		pCmdUI->Enable(bInit && !pDoc->GetDataInEdition()); break;
	//	}
	//
	//
	//}


	//void CNormalsMatchWnd::OnToolbarCommand(UINT ID)
	//{
	//	CMatchStationDoc* pDoc = GetDocument();
	//	int index = m_wndToolBar.CommandToIndex(ID);
	//	CMFCToolBarButton* pCtrl = m_wndToolBar.GetButton(index); ASSERT(pCtrl);
	//
	//
	//	switch (ID)
	//	{
	//	//case ID_TABLE_MODE_VISUALISATION:
	//	//{
	//	//	if (pDoc->GetDataInEdition())
	//	//	{
	//	//		if (m_wndNormalsList.IsModified())
	//	//		{
	//	//			int rep = AfxMessageBox(IDS_SAVE_DATA, MB_YESNOCANCEL | MB_ICONQUESTION);
	//	//			if (rep == IDYES)
	//	//			{
	//	//				if (m_wndNormalsList.SaveData())
	//	//					pDoc->SetDataInEdition(false);
	//	//			}
	//	//			else if (rep == IDNO)
	//	//			{
	//	//				//reload station in document
	//	//				if (pDoc->CancelDataEdition())
	//	//				{
	//	//					//reload data in interface
	//	//					pDoc->SetDataInEdition(false);
	//	//				}
	//	//			}
	//	//		}
	//	//		else
	//	//		{
	//	//			pDoc->SetDataInEdition(false);
	//	//		}
	//	//	}
	//
	//	//	break;
	//	//}
	//	//case ID_TABLE_MODE_EDITION:
	//	//{
	//	//	if (!pDoc->GetDataInEdition())
	//	//	{
	//	//		pDoc->SetTM(CTM::HOURLY);
	//	//		pDoc->SetStatistic(MEAN);
	//	//		pDoc->SetDataInEdition(true);
	//	//	}
	//	//	break;
	//	//}
	//	case ID_TABLE_SAVE:				m_wndNormalsList.SaveData();  break;
	//	case ID_TABLE_SENDTO_EXCEL:		m_wndNormalsList.ExportToExcel(); break;
	//	case ID_TABLE_PERIOD_ENABLED:	pDoc->SetPeriodEnabled(!(pCtrl->m_nStyle & TBBS_CHECKED)); break;
	//	case ID_TABLE_PERIOD_BEGIN:		OnDateChange(ID); break;
	//	case ID_TABLE_PERIOD_END:		OnDateChange(ID); break;
	//	case ID_TABLE_FILTER:			pDoc->SetVariables(((CMFCToolBarWVariablesButton*)pCtrl)->GetVariables()); break;
	//	case ID_TABLE_STAT:				pDoc->SetStatistic(((CMFCToolBarComboBoxButton*)pCtrl)->GetCurSel()); break;
	//	case ID_TABLE_TM_TYPE:			pDoc->SetTM(CTM(((CMFCToolBarComboBoxButton*)pCtrl)->GetCurSel())); break;
	//
	//	}
	//
	//
	//	//CWeatherDatabasePtr& pDB = GetDatabasePtr();
	//
	//
	//	//if (pDB && pDB->IsOpen())
	//	//{
	//	//	CMatchStationDoc* pDoc = GetDocument();
	//	//	ASSERT(pDoc);
	//
	//	//	if (ID == ID_ADD_WEATHER_STATION)
	//	//	{
	//	//		MessageBox(_T("Ça reste à faire..."));
	//	//	}
	//	//	else if (ID == ID_SENDTO_EXCEL || ID == ID_SENDTO_SHOWMAP)
	//	//	{
	//	//		CRegistry registry;
	//	//		char separator = registry.GetListDelimiter();
	//
	//	//		CSearchResultVector searchResultArray;
	//
	//	//		CWVariables filter = pDoc->GetFilters();
	//	//		set<int> years = pDoc->GetYears();
	//
	//	//		ERMsg msg;
	//	//		msg = pDB->GetStationList(searchResultArray, filter, years, true, false);
	//
	//	//		CLocationVector loc = pDB->GetLocations(searchResultArray);
	//
	//	//		string filePath = GetUserDataPath() + "tmp\\" + GetFileTitle(pDB->GetFilePath()) + ".csv";
	//	//		CreateMultipleDir(GetPath(filePath));
	//
	//	//		msg += (loc.Save(filePath, separator));
	//	//		if (msg)
	//	//		{
	//	//			if (ID == ID_SENDTO_EXCEL)
	//	//				msg += CallApplication(CRegistry::SPREADSHEET, filePath, GetSafeHwnd(), SW_SHOW);
	//	//			else if (ID == ID_SENDTO_SHOWMAP)
	//	//				msg += CallApplication(CRegistry::SHOWMAP, filePath, GetSafeHwnd(), SW_SHOW);
	//	//		}
	//
	//	//		//pDoc->SetOutputText(SYGetText(msg));
	//	//		if (!msg)
	//	//			SYShowMessage(msg, this);
	//	//	}
	//	//	else if (ID == ID_STATION_LIST_YEAR)
	//	//	{
	//	//		int index = m_wndToolBar.CommandToIndex(ID);
	//	//		CMFCToolBarYearsButton* pCtrl = (CMFCToolBarYearsButton*)m_wndToolBar.GetButton(index); ASSERT(pCtrl);
	//
	//	//		std::set<int> years = pCtrl->GetYears();
	//	//		pDoc->SetYears(years);
	//	//	}
	//	//	else if (ID == ID_STATION_LIST_FILTER)
	//	//	{
	//	//		int index = m_wndToolBar.CommandToIndex(ID);
	//	//		CMFCToolBarWVariablesButton* pCtrl = (CMFCToolBarWVariablesButton*)m_wndToolBar.GetButton(index); ASSERT(pCtrl);
	//	//		pDoc->SetFilters(pCtrl->GetVariables());
	//	//	}
	//	//}
	//}

	BOOL CNormalsMatchWnd::PreTranslateMessage(MSG* pMsg)
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



	//void CNormalsMatchWnd::OnUpdateStatusBar(CCmdUI* pCmdUI)
	//{

	//if (pCmdUI->m_nID == ID_INDICATOR_NB_STATIONS)
	//{
	//	CWeatherDatabasePtr pDB = GetDatabasePtr();
	//	//pCmdUI->->Enable(pDB && pDB->IsOpen());

	//	long nbRows = m_stationsList.GetNumberRows();

	//	CString str = _T("Stations = ");//GetCString(IDS_INDICATOR_NB_EXECUTE);
	//	CString text = str + UtilWin::ToCString(nbRows);

	//	pCmdUI->SetText(text);

	//	//resize space of text
	//	CDC* pDC = GetDC();
	//	ASSERT(pDC);
	//	CSize size = pDC->GetTextExtent(text);

	//	UINT nStyle = m_wndStatusBar.GetPaneStyle(1);
	//	m_wndStatusBar.SetPaneInfo(1, ID_INDICATOR_NB_STATIONS, nStyle, size.cx);
	//}


	//}

	//
	//LRESULT  CNormalsMatchWnd::OnSelectionChange(WPARAM, LPARAM)
	//{
	//	CMatchStationDoc* pDoc = GetDocument();
	//	ASSERT(pDoc);
	//
	//	size_t	index = m_stationsList.GetStationIndex();
	//	pDoc->SetCurStationIndex(index);
	//
	//	return 0;
	//}

	//
	//
	//void CNormalsMatchWnd::OnDateChange(UINT ID)
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

}