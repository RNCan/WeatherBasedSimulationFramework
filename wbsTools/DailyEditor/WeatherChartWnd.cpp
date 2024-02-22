
// BioSIMView.cpp : implementation of the CWeatherChartWnd class
//

#include "stdafx.h"
#include "DailyEditor.h"

#include "DailyEditorDoc.h"
#include "WeatherChartWnd.h"

#include "Basic/Dimension.h"
#include "Basic/WeatherDefine.h"
#include "UI/Common/AppOption.h"
#include "UI/Common/SYShowMessage.h"
#include "UI/WVariablesEdit.h"
#include "UI/ChartsProperties.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


using namespace std;
using namespace WBSF;
using namespace UtilWin;
using namespace WBSF::HOURLY_DATA;



static CDailyEditorDoc* GetDocument()
{
	CWinApp* pApp = AfxGetApp();
	if (pApp)
	{
		CFrameWnd * pFrame = (CFrameWnd *)(pApp->m_pMainWnd);
		if (pFrame && pFrame->GetSafeHwnd() != NULL)
			return (CDailyEditorDoc*)(pFrame->GetActiveDocument());
	}
	return NULL;

}

static CWeatherDatabasePtr GetDatabasePtr()
{
	CWeatherDatabasePtr pDB;
	CDailyEditorDoc* pDocument = GetDocument();

	if (pDocument)
		pDB = pDocument->GetDatabase();


	return  pDB;
}



static const int STD_GAP = 8;
static const int CHART_BASE_ID = 1000;
static const int SPLITTER_BASE_ID = 2000;

BEGIN_MESSAGE_MAP(CWeatherChartToolBar, CSplittedToolBar)
END_MESSAGE_MAP()

IMPLEMENT_SERIAL(CWeatherChartToolBar, CSplittedToolBar, 1)
BOOL CWeatherChartToolBar::LoadToolBarEx(UINT uiToolbarResID, CMFCToolBarInfo& params, BOOL bLocked)
{
	if (!CSplittedToolBar::LoadToolBarEx(uiToolbarResID, params, bLocked))
		return FALSE;

//*****************************
	CMFCToolBarComboBoxButton zoomButton(ID_GRAPH_ZOOM, 3, CBS_DROPDOWNLIST, 75);
	for (int i = 0; i < 12; i++)
	{
		CString str;
		str.Format(_T("%dx"), 1 << i);
		zoomButton.AddItem(str);
	}
	
	zoomButton.SelectItem(0, FALSE);
	ReplaceButton(ID_GRAPH_ZOOM, zoomButton);
//*****************************
	CMFCToolBarButton periodEnabled(ID_GRAPH_PERIOD_ENABLED, 4);
	periodEnabled.SetStyle(TBBS_CHECKBOX);
	ReplaceButton(ID_GRAPH_PERIOD_ENABLED, periodEnabled);
//*****************************
	CMFCToolBarDateTimeCtrl periodBegin(ID_GRAPH_PERIOD_BEGIN, 5, DTS_SHORTDATECENTURYFORMAT, 175);
	ReplaceButton(ID_GRAPH_PERIOD_BEGIN, periodBegin);
//*****************************
	CMFCToolBarDateTimeCtrl periodEnd(ID_GRAPH_PERIOD_END, 6,  DTS_SHORTDATECENTURYFORMAT, 175);
	ReplaceButton(ID_GRAPH_PERIOD_END, periodEnd);
//*****************************
	CMFCToolBarWVariablesButton filterCtrl(ID_GRAPH_FILTER, 7, 150);
	ReplaceButton(ID_GRAPH_FILTER, filterCtrl);
//*****************************
	CMFCToolBarComboBoxButton statButton(ID_GRAPH_STAT, 8, CBS_DROPDOWNLIST);
	for (int i = 0; i < NB_STAT_TYPE; i++)
		statButton.AddItem(CString(CStatistic::GetTitle(i)));

	statButton.SelectItem(0, FALSE);
	ReplaceButton(ID_GRAPH_STAT, statButton);

//*****************************
	CMFCToolBarComboBoxButton TMButton(ID_GRAPH_TM_TYPE, 9, CBS_DROPDOWNLIST, 100);
	for (int i = 0; i <= CTM::DAILY; i++)
		TMButton.AddItem(CString(CTM::GetTypeTitle(i)));
	
	TMButton.SelectItem(0, FALSE);
	ReplaceButton(ID_GRAPH_TM_TYPE, TMButton);
	

	UpdateTooltips();

	return TRUE;
}


//**********************************************************************
// CWeatherChartWnd construction/destruction
static const int ID_CHART = 1004;

BEGIN_MESSAGE_MAP(CWeatherChartWnd, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_WINDOWPOSCHANGED()

	ON_UPDATE_COMMAND_UI_RANGE(ID_GRAPH_COPY, ID_GRAPH_TM_TYPE, OnUpdateToolbar)
	ON_COMMAND_RANGE(ID_GRAPH_COPY, ID_GRAPH_TM_TYPE, OnToolbarCommand)
	ON_CONTROL_RANGE(LBN_SELCHANGE, ID_GRAPH_COPY, ID_GRAPH_TM_TYPE, OnToolbarCommand)
	ON_CONTROL_RANGE(EN_CHANGE, ID_GRAPH_COPY, ID_GRAPH_TM_TYPE, OnToolbarCommand)
	
END_MESSAGE_MAP()

CWeatherChartWnd::CWeatherChartWnd()
{
	m_bMustBeUpdated=false;
	m_bEnableMessage = false;
}

CWeatherChartWnd::~CWeatherChartWnd()
{}

BOOL CWeatherChartWnd::PreCreateWindow(CREATESTRUCT& cs)
{
	return CDockablePane::PreCreateWindow(cs);
}


int CWeatherChartWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	
	if (!m_weatherChartsCtrl.Create(this, CRect(0, 0, 0, 0), ID_CHART, WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | WS_VSCROLL))
		return FALSE;

	CreateToolBar();
	AdjustLayout();

	return 0;
}

void CWeatherChartWnd::CreateToolBar()
{
	
	if (m_wndToolBar.GetSafeHwnd())
		m_wndToolBar.DestroyWindow();

	m_bEnableMessage = false;
	m_wndToolBar.Create(this, AFX_DEFAULT_TOOLBAR_STYLE | CBRS_SIZE_DYNAMIC, IDR_TABLE_TOOLBAR);
	m_wndToolBar.LoadToolBar(IDR_GRAPH_TOOLBAR, 0, 0, TRUE);
	m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() | CBRS_TOOLTIPS | CBRS_FLYBY);
	m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() & ~(CBRS_GRIPPER | CBRS_SIZE_DYNAMIC | CBRS_BORDER_TOP | CBRS_BORDER_BOTTOM | CBRS_BORDER_LEFT | CBRS_BORDER_RIGHT));
	m_wndToolBar.SetOwner(this);
	m_wndToolBar.SetRouteCommandsViaFrame(FALSE);
	
	m_bEnableMessage = true;
}

void CWeatherChartWnd::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);
	
	AdjustLayout();
}

void CWeatherChartWnd::AdjustLayout()
{
	if (GetSafeHwnd() == NULL)
	{
		return;
	}

	CRect rectClient;
	GetClientRect(rectClient);

	CSize tlb = m_wndToolBar.CalcFixedLayout(FALSE, TRUE);
	

	m_wndToolBar.SetWindowPos(NULL, rectClient.left, rectClient.top, rectClient.Width(), tlb.cy, SWP_NOACTIVATE | SWP_NOZORDER);
	m_weatherChartsCtrl.SetWindowPos(NULL, rectClient.left, rectClient.top + tlb.cy, rectClient.Width(), rectClient.Height() - tlb.cy, SWP_NOACTIVATE | SWP_NOZORDER);
}

void CWeatherChartWnd::OnCopyGraph()
{
	
}

void CWeatherChartWnd::OnSaveGraph()
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
		CString strFilename = openDlg.GetPathName();
		if( GetFileExtension(strFilename).IsEmpty() )
			strFilename += GetDefaultFilterExt(filter, openDlg.m_ofn.nFilterIndex-1);

		m_weatherChartsCtrl.SaveAsImage((LPCSTR)CStringA(strFilename));
    }
	
}

void CWeatherChartWnd::OnGraphOptions()
{
	CWVariablesCounter variables;
	variables.fill(1);//emulate full variables

	CChartsProperties dlg(false);
	dlg.m_graphics = CWeatherChartsCtrl::GetCharts(variables, false);

	if (dlg.DoModal()==IDOK)
	{
		string filePath = CWeatherChartsCtrl::GetGraphisFilesPath(false);
		ERMsg msg = zen::SaveXML(filePath, "Graphics", "1", dlg.m_graphics);

		if (msg)
			m_weatherChartsCtrl.Update();
		else
			SYShowMessage(msg, this);
	}

}


void CWeatherChartWnd::OnUpdateToolbar(CCmdUI *pCmdUI)
{
	CDailyEditorDoc* pDoc = (CDailyEditorDoc*)GetDocument();
	if (!pDoc)
		return;

	bool bInit = pDoc->GetCurStationIndex() != UNKNOWN_POS && !pDoc->GetDataInEdition();
	bool bPeriodEnable = pDoc->GetPeriodEnabled();

	switch (pCmdUI->m_nID)
	{
	case ID_GRAPH_COPY:				pCmdUI->Enable(bInit&&false); break;
	case ID_GRAPH_SAVE:				pCmdUI->Enable(bInit ); break;
	case ID_GRAPH_OPTIONS:			pCmdUI->Enable(bInit); break;
	case ID_GRAPH_ZOOM:				pCmdUI->Enable(bInit); break;
	case ID_GRAPH_PERIOD_ENABLED:	pCmdUI->SetCheck(bPeriodEnable);  pCmdUI->Enable(bInit); break;
	case ID_GRAPH_PERIOD_BEGIN:		pCmdUI->Enable(bInit&&bPeriodEnable); break;
	case ID_GRAPH_PERIOD_END:		pCmdUI->Enable(bInit&&bPeriodEnable); break;
	case ID_GRAPH_FILTER:			pCmdUI->Enable(bInit); break;
	case ID_GRAPH_STAT:				pCmdUI->Enable(bInit); break;
	case ID_GRAPH_TM_TYPE:			pCmdUI->Enable(bInit); break;
	}
}

void CWeatherChartWnd::OnToolbarCommand(UINT ID, NMHDR *pNMHDR, LRESULT *pResult)
{
	OnToolbarCommand(ID);
}

void CWeatherChartWnd::OnToolbarCommand(UINT ID)
{
	if (m_bEnableMessage)
	{
		CDailyEditorDoc* pDoc = (CDailyEditorDoc*)GetDocument();
		if (!pDoc)
			return;

		bool bInit = pDoc->GetCurStationIndex() != UNKNOWN_POS && !pDoc->GetDataInEdition();
		bool bPeriodEnable = pDoc->GetPeriodEnabled();
		int index = m_wndToolBar.CommandToIndex(ID);
		CMFCToolBarButton* pCtrl = m_wndToolBar.GetButton(index); ASSERT(pCtrl);

		if (bInit)
		{
			switch (ID)
			{
			case ID_GRAPH_COPY:				OnCopyGraph(); break;
			case ID_GRAPH_SAVE:				OnSaveGraph(); break;
			case ID_GRAPH_OPTIONS:			OnGraphOptions(); break;
			case ID_GRAPH_ZOOM:				pDoc->SetChartsZoom(((CMFCToolBarComboBoxButton*)pCtrl)->GetCurSel()); break;
			case ID_GRAPH_PERIOD_ENABLED:	pDoc->SetPeriodEnabled(!(pCtrl->m_nStyle & TBBS_CHECKED)); break;
			case ID_GRAPH_PERIOD_BEGIN:		OnDateChange(ID); break;
			case ID_GRAPH_PERIOD_END:		OnDateChange(ID); break;
			case ID_GRAPH_FILTER:			pDoc->SetVariables(((CMFCToolBarWVariablesButton*)pCtrl)->GetVariables()); break;
			case ID_GRAPH_STAT:				pDoc->SetStatistic(((CMFCToolBarComboBoxButton*)pCtrl)->GetCurSel()); break;
			case ID_GRAPH_TM_TYPE:			pDoc->SetTM(CTM(((CMFCToolBarComboBoxButton*)pCtrl)->GetCurSel())); break;
			}
		}
	}
}


void CWeatherChartWnd::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	m_bEnableMessage = false;

	if (lHint == CDailyEditorDoc::LANGUAGE_CHANGE)
	{
		CreateToolBar();
		AdjustLayout();
	}

	CDailyEditorDoc* pDoc = (CDailyEditorDoc*)GetDocument();
	CWeatherDatabasePtr pDB = pDoc->GetDatabase();
	CTPeriod period = pDoc->GetPeriod();
	bool bPeriodEnabled = pDoc->GetPeriodEnabled();


	if (lHint == CDailyEditorDoc::INIT || lHint == CDailyEditorDoc::CHARTS_PROPERTIES_ZOOM_CHANGE)
	{
		CMFCToolBarComboBoxButton* pCtrl = CMFCToolBarComboBoxButton::GetByCmd(ID_GRAPH_ZOOM); ASSERT(pCtrl);
		pCtrl->SelectItem(int(pDoc->GetChartsZoom()));
	}

	if (lHint == CDailyEditorDoc::INIT || lHint == CDailyEditorDoc::DATA_PROPERTIES_ENABLE_PERIOD_CHANGE)
	{
		int index = m_wndToolBar.CommandToIndex(ID_GRAPH_PERIOD_ENABLED);
		CMFCToolBarButton* pCtrl = m_wndToolBar.GetButton(index); ASSERT(pCtrl);
		pCtrl->SetImage(bPeriodEnabled ? ID_TABLE_PERIOD_ENABLED - ID_TABLE_MODE_VISUALISATION : ID_TABLE_PERIOD_DESABLED - ID_TABLE_MODE_VISUALISATION);
		pCtrl->SetStyle(bPeriodEnabled ? (pCtrl->m_nStyle | TBBS_CHECKED) : (pCtrl->m_nStyle& ~TBBS_CHECKED));
	}

	if (lHint == CDailyEditorDoc::INIT || lHint == CDailyEditorDoc::DATA_PROPERTIES_PERIOD_CHANGE)
	{
		if (period.IsInit())
		{
			COleDateTime oFirstDate(period.Begin().GetYear(), (int)period.Begin().GetMonth() + 1, (int)period.Begin().GetDay() + 1, 0, 0, 0);
			COleDateTime oLastDate(period.End().GetYear(), (int)period.End().GetMonth() + 1, (int)period.End().GetDay() + 1, 23, 59, 59);
			
			CMFCToolBarDateTimeCtrl* pCtrl1 = CMFCToolBarDateTimeCtrl::GetByCmd(ID_TABLE_PERIOD_BEGIN); ASSERT(pCtrl1);
			CMFCToolBarDateTimeCtrl* pCtrl2 = CMFCToolBarDateTimeCtrl::GetByCmd(ID_TABLE_PERIOD_END); ASSERT(pCtrl2);

			try
			{
				pCtrl1->SetTime(oFirstDate);
				pCtrl2->SetTime(oLastDate);
			}
			catch (...)
			{
			}
		}
	}

	if (lHint == CDailyEditorDoc::INIT || lHint == CDailyEditorDoc::DATA_PROPERTIES_VARIABLES_CHANGE)
	{
		int index = m_wndToolBar.CommandToIndex(ID_GRAPH_FILTER);
		CMFCToolBarWVariablesButton* pCtrl = (CMFCToolBarWVariablesButton*)m_wndToolBar.GetButton(index); ASSERT(pCtrl);
		pCtrl->SetVariables(pDoc->GetVariables());
	}

	if (lHint == CDailyEditorDoc::INIT || lHint == CDailyEditorDoc::DATA_PROPERTIES_STAT_CHANGE || lHint == CDailyEditorDoc::LANGUAGE_CHANGE)
	{
		int index = m_wndToolBar.CommandToIndex(ID_GRAPH_STAT);
		CMFCToolBarComboBoxButton* pCtrl = (CMFCToolBarComboBoxButton*)m_wndToolBar.GetButton(index); ASSERT(pCtrl);
		pCtrl->SelectItem(int(pDoc->GetStatistic()));
	}

	if (lHint == CDailyEditorDoc::INIT || lHint == CDailyEditorDoc::PROPERTIES_TM_CHANGE)
	{
		CMFCToolBarComboBoxButton* pCtrl = CMFCToolBarComboBoxButton::GetByCmd(ID_GRAPH_TM_TYPE); ASSERT(pCtrl);
		pCtrl->SelectItem(int(pDoc->GetTM().Type()));
	}


	m_weatherChartsCtrl.m_pStation = pDoc->GetCurStation();
	m_weatherChartsCtrl.m_zoom = 1 << pDoc->GetChartsZoom();
	m_weatherChartsCtrl.m_bPeriodEnable = pDoc->GetPeriodEnabled();
	m_weatherChartsCtrl.m_period = pDoc->GetPeriod();
	m_weatherChartsCtrl.m_variables = pDoc->GetVariables();
	m_weatherChartsCtrl.m_stat = pDoc->GetStatistic();
	m_weatherChartsCtrl.m_TM = pDoc->GetTM();


	

	bool bVisible = IsWindowVisible();
	if( bVisible  )
		m_weatherChartsCtrl.Update();
	else
		m_bMustBeUpdated=true;


	m_bEnableMessage = true;
}


void CWeatherChartWnd::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
	CDockablePane::OnWindowPosChanged(lpwndpos);

	if( lpwndpos->flags & SWP_SHOWWINDOW )
	{
		if( m_bMustBeUpdated )
		{
			m_weatherChartsCtrl.Update();
			m_bMustBeUpdated=false;
		}
	}
}



void CWeatherChartWnd::OnDateChange(UINT ID)
{
	ASSERT(ID == ID_GRAPH_PERIOD_BEGIN || ID == ID_GRAPH_PERIOD_END);

	CDailyEditorDoc* pDoc = (CDailyEditorDoc*)GetDocument();

	COleDateTime oFirstDate;
	COleDateTime oLastDate;
	CMFCToolBarDateTimeCtrl* pCtrl1 = CMFCToolBarDateTimeCtrl::GetByCmd(ID_GRAPH_PERIOD_BEGIN); ASSERT(pCtrl1);
	CMFCToolBarDateTimeCtrl* pCtrl2 = CMFCToolBarDateTimeCtrl::GetByCmd(ID_GRAPH_PERIOD_END); ASSERT(pCtrl2);

	pCtrl1->GetTime(oFirstDate);
	pCtrl2->GetTime(oLastDate);

	CTPeriod period;
	if (oFirstDate.GetStatus() == COleDateTime::valid &&
		oLastDate.GetStatus() == COleDateTime::valid)
	{
		CTRef begin(oFirstDate.GetYear(), oFirstDate.GetMonth() - 1, oFirstDate.GetDay() - 1);
		CTRef end(oLastDate.GetYear(), oLastDate.GetMonth() - 1, oLastDate.GetDay() - 1);
		if (begin > end)
		{
			if (ID == ID_GRAPH_PERIOD_BEGIN)
				end = begin;
			else
				begin = end;
		}
		period = CTPeriod(begin, end);
	}

	pDoc->SetPeriod(period);

}


BOOL CWeatherChartWnd::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	//let the trl to route command
	CWnd* pFocus = GetFocus();
	if (pFocus)
	{
		CWnd* pParent = pFocus->GetParent();

		if (pFocus == &m_weatherChartsCtrl || pParent == &m_weatherChartsCtrl)
		{
			if (m_weatherChartsCtrl.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
				return TRUE;
		}
	}

	return CDockablePane::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

