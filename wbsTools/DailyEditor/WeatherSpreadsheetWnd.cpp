
// BioSIMView.cpp : implementation of the CWeatherSpreadsheetWnd class
//

#include "stdafx.h"
#include "Basic/Registry.h"
#include "UI/Common/AppOption.h"
#include "UI/Common/SYShowMessage.h"
#include "UI/WVariablesEdit.h"

#include "DailyEditor.h"
#include "DailyEditorDoc.h"
#include "WeatherSpreadsheetWnd.h"
#include "resource.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


using namespace std;
using namespace WBSF;





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


BEGIN_MESSAGE_MAP(CWeatherSpreadsheetToolBar, CSplittedToolBar)
END_MESSAGE_MAP()


IMPLEMENT_SERIAL(CWeatherSpreadsheetToolBar, CSplittedToolBar, 1)
BOOL CWeatherSpreadsheetToolBar::LoadToolBarEx(UINT uiToolbarResID, CMFCToolBarInfo& params, BOOL bLocked)
{
	if (!CSplittedToolBar::LoadToolBarEx(uiToolbarResID, params, bLocked))
		return FALSE;
	
	//*****************************
	CMFCToolBarButton periodEnabled(ID_TABLE_PERIOD_ENABLED, 4);
	periodEnabled.SetStyle((WS_TABSTOP | TBBS_CHECKBOX) & ~WS_GROUP);
	ReplaceButton(ID_TABLE_PERIOD_ENABLED, periodEnabled);

	//*****************************
	CMFCToolBarDateTimeCtrl periodBegin(ID_TABLE_PERIOD_BEGIN, 5, WS_TABSTOP | DTS_SHORTDATECENTURYFORMAT, 175);
	ReplaceButton(ID_TABLE_PERIOD_BEGIN, periodBegin);
	//*****************************
	CMFCToolBarDateTimeCtrl periodEnd(ID_TABLE_PERIOD_END, 6, WS_TABSTOP | DTS_SHORTDATECENTURYFORMAT, 175);
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
	CMFCToolBarComboBoxButton TMButton(ID_TABLE_TM_TYPE, 9, WS_TABSTOP | CBS_DROPDOWNLIST);
	for (int i = 0; i <= CTM::DAILY; i++)
		TMButton.AddItem(CString(CTM::GetTypeTitle(i)));

	TMButton.SelectItem(0, FALSE);
	ReplaceButton(ID_TABLE_TM_TYPE, TMButton);

	UpdateTooltips();
	SetWindowPos(NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE | SWP_NOMOVE);

	return TRUE;
}

//**************************************************************************************************************************************

// CWeatherSpreadsheetWnd
IMPLEMENT_DYNCREATE(CWeatherSpreadsheetWnd, CDockablePane)

static const int IDC_GRID_ID = 1002;


BEGIN_MESSAGE_MAP(CWeatherSpreadsheetWnd, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_WINDOWPOSCHANGED()
	ON_UPDATE_COMMAND_UI_RANGE(ID_TABLE_MODE_VISUALISATION, ID_TABLE_TM_TYPE, OnUpdateToolbar)
	ON_COMMAND_RANGE(ID_TABLE_MODE_VISUALISATION, ID_TABLE_TM_TYPE, OnToolbarCommand)
	ON_CONTROL_RANGE(LBN_SELCHANGE, ID_TABLE_MODE_VISUALISATION, ID_TABLE_TM_TYPE, OnToolbarCommand)
	ON_CONTROL_RANGE(EN_CHANGE, ID_TABLE_MODE_VISUALISATION, ID_TABLE_TM_TYPE, OnToolbarCommand)

END_MESSAGE_MAP()


// CWeatherSpreadsheetWnd construction/destruction
CWeatherSpreadsheetWnd::CWeatherSpreadsheetWnd()
{
	m_bMustBeUpdated=false;
	m_bEnableMessage = false;
}

CWeatherSpreadsheetWnd::~CWeatherSpreadsheetWnd()
{
}

// CWeatherSpreadsheetWnd drawing

int CWeatherSpreadsheetWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	CreateToolBar();

	m_grid.CreateGrid(WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), this, IDC_GRID_ID);

	AdjustLayout();

	return 0;
}

void CWeatherSpreadsheetWnd::CreateToolBar()
{
	if (m_wndToolBar.GetSafeHwnd())
		m_wndToolBar.DestroyWindow();

	m_bEnableMessage = false;
	m_wndToolBar.Create(this, AFX_DEFAULT_TOOLBAR_STYLE | CBRS_SIZE_DYNAMIC, IDR_TABLE_TOOLBAR);
	m_wndToolBar.LoadToolBar(IDR_TABLE_TOOLBAR, 0, 0, TRUE);
	m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() | CBRS_TOOLTIPS | CBRS_FLYBY);
	m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() & ~(CBRS_GRIPPER | CBRS_SIZE_DYNAMIC | CBRS_BORDER_TOP | CBRS_BORDER_BOTTOM | CBRS_BORDER_LEFT | CBRS_BORDER_RIGHT));
	m_wndToolBar.SetOwner(this);
	m_wndToolBar.SetRouteCommandsViaFrame(FALSE);
	m_bEnableMessage = true;
}

void CWeatherSpreadsheetWnd::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);
	
	AdjustLayout();
}

void CWeatherSpreadsheetWnd::AdjustLayout()
{
	if (GetSafeHwnd() == NULL)
	{
		return;
	}

	CRect rectClient;
	GetClientRect(rectClient);

	int cxTlb = m_wndToolBar.CalcFixedLayout(FALSE, TRUE).cx;
	int cyTlb = m_wndToolBar.CalcFixedLayout(FALSE, TRUE).cy;

	m_wndToolBar.SetWindowPos(NULL, rectClient.left, rectClient.top, rectClient.Width(), cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);
	m_grid.SetWindowPos(NULL, rectClient.left, rectClient.top + cyTlb, rectClient.Width(), rectClient.Height() -cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);
}

void CWeatherSpreadsheetWnd::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	m_bEnableMessage = false;

	CDailyEditorDoc* pDoc = (CDailyEditorDoc*)GetDocument();
	CTPeriod period = pDoc->GetPeriod();
	bool bPeriodEnabled = pDoc->GetPeriodEnabled();

	if (lHint == CDailyEditorDoc::LANGUAGE_CHANGE)
	{
		CWeatherDataGridCtrl::ReloadString();

		CreateToolBar();
		AdjustLayout();
	}

	if (lHint == CDailyEditorDoc::INIT || lHint == CDailyEditorDoc::DATA_PROPERTIES_ENABLE_PERIOD_CHANGE)
	{
		int index = m_wndToolBar.CommandToIndex(ID_TABLE_PERIOD_ENABLED);
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
		int index = m_wndToolBar.CommandToIndex(ID_TABLE_FILTER);
		CMFCToolBarWVariablesButton* pCtrl = (CMFCToolBarWVariablesButton*)m_wndToolBar.GetButton(index); ASSERT(pCtrl);
		pCtrl->SetVariables(pDoc->GetVariables());
	}

	if (lHint == CDailyEditorDoc::INIT || lHint == CDailyEditorDoc::DATA_PROPERTIES_STAT_CHANGE || lHint == CDailyEditorDoc::LANGUAGE_CHANGE)
	{
		int index = m_wndToolBar.CommandToIndex(ID_TABLE_STAT);
		CMFCToolBarComboBoxButton* pCtrl = (CMFCToolBarComboBoxButton*)m_wndToolBar.GetButton(index); ASSERT(pCtrl);
		pCtrl->SelectItem(int(pDoc->GetStatistic()));
	}

	if (lHint == CDailyEditorDoc::INIT || lHint == CDailyEditorDoc::PROPERTIES_TM_CHANGE || lHint == CDailyEditorDoc::LANGUAGE_CHANGE)
	{
		int index = m_wndToolBar.CommandToIndex(ID_TABLE_TM_TYPE);
		CMFCToolBarComboBoxButton* pCtrl = (CMFCToolBarComboBoxButton*)m_wndToolBar.GetButton(index); ASSERT(pCtrl);
		pCtrl->SelectItem(int(pDoc->GetTM().Type()));
	}
	if (lHint == CDailyEditorDoc::DATA_PROPERTIES_EDITION_MODE_CHANGE)
	{
		m_grid.SetIsModified(false);
	}

	m_grid.m_TM = pDoc->GetTM();
	m_grid.m_stat = pDoc->GetStatistic();
	m_grid.m_bEditable = pDoc->GetDataInEdition();
	m_grid.m_pStation = pDoc->GetCurStation();
	m_grid.m_bPeriodEnabled = pDoc->GetPeriodEnabled();
	m_grid.m_period = pDoc->GetPeriod();
	m_grid.m_variables = pDoc->GetVariables();


	bool bVisible = IsWindowVisible();
	if( bVisible )
		m_grid.Update();
	else 
		m_bMustBeUpdated = true;

	m_bEnableMessage = true;
}

void CWeatherSpreadsheetWnd::OnWindowPosChanged(WINDOWPOS* lpwndpos)
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


void CWeatherSpreadsheetWnd::OnUpdateToolbar(CCmdUI *pCmdUI)
{
	CDailyEditorDoc* pDoc = GetDocument();
	if (!pDoc)
		return;


	bool bInit = pDoc->GetCurStationIndex() != UNKNOWN_POS;
	bool bPeriodEnable = pDoc->GetPeriodEnabled();

	switch (pCmdUI->m_nID)
	{
	case ID_TABLE_MODE_VISUALISATION:	pCmdUI->Enable(bInit); pCmdUI->SetCheck(!pDoc->GetDataInEdition()); break;
	case ID_TABLE_MODE_EDITION:			pCmdUI->Enable(bInit); pCmdUI->SetCheck(pDoc->GetDataInEdition()); break;
	case ID_TABLE_SAVE:					pCmdUI->Enable(bInit && pDoc->GetDataInEdition()); break;
	case ID_TABLE_SENDTO_EXCEL:			pCmdUI->Enable(bInit && !pDoc->GetDataInEdition()); break;
	case ID_TABLE_PERIOD_ENABLED:		pCmdUI->SetCheck(bPeriodEnable);  pCmdUI->Enable(bInit); break;
	case ID_TABLE_PERIOD_BEGIN:			pCmdUI->Enable(bInit&&bPeriodEnable); break;
	case ID_TABLE_PERIOD_END:			pCmdUI->Enable(bInit&&bPeriodEnable); break;
	case ID_TABLE_FILTER:				pCmdUI->Enable(bInit); break;
	case ID_TABLE_STAT:					pCmdUI->Enable(bInit && !pDoc->GetDataInEdition()); break;
	case ID_TABLE_TM_TYPE:				pCmdUI->Enable(bInit && !pDoc->GetDataInEdition()); break;
	}

}

void CWeatherSpreadsheetWnd::OnToolbarCommand(UINT ID)
{
	if (m_bEnableMessage)
	{
		CDailyEditorDoc* pDoc = GetDocument();
		if (!pDoc)
			return;

		int index = m_wndToolBar.CommandToIndex(ID);
		CMFCToolBarButton* pCtrl = m_wndToolBar.GetButton(index); ASSERT(pCtrl);

		switch (ID)
		{
		case ID_TABLE_MODE_VISUALISATION:
		{
			if (pDoc->GetDataInEdition())
			{
				if (m_grid.IsModified())
				{
					int rep = AfxMessageBox(IDS_SAVE_DATA, MB_YESNOCANCEL | MB_ICONQUESTION);
					if (rep == IDYES)
					{
						if (SaveData())
							pDoc->SetDataInEdition(false);
					}
					else if (rep == IDNO)
					{
						//reload station in document
						if (pDoc->CancelDataEdition())
						{
							//reload data in interface
							pDoc->SetDataInEdition(false);
						}
					}
				}
				else
				{
					pDoc->SetDataInEdition(false);
				}
			}

			break;
		}
		case ID_TABLE_MODE_EDITION:
		{
			if (!pDoc->GetDataInEdition())
			{
				pDoc->SetTM(CTM::DAILY);
				pDoc->SetStatistic(MEAN);
				pDoc->SetDataInEdition(true);
			}
			break;
		}
		case ID_TABLE_SAVE:			SaveData();  break;
		case ID_TABLE_SENDTO_EXCEL:	ExportToExcel(); break;
		case ID_TABLE_PERIOD_ENABLED:	pDoc->SetPeriodEnabled(!(pCtrl->m_nStyle & TBBS_CHECKED)); break;
		case ID_TABLE_PERIOD_BEGIN:		OnDateChange(ID); break;
		case ID_TABLE_PERIOD_END:		OnDateChange(ID); break;
		case ID_TABLE_FILTER:			pDoc->SetVariables(((CMFCToolBarWVariablesButton*)pCtrl)->GetVariables()); break;
		case ID_TABLE_STAT:			pDoc->SetStatistic(((CMFCToolBarComboBoxButton*)pCtrl)->GetCurSel()); break;
		case ID_TABLE_TM_TYPE:		pDoc->SetTM(CTM(((CMFCToolBarComboBoxButton*)pCtrl)->GetCurSel())); break;

		}
	}
}

bool CWeatherSpreadsheetWnd::SaveData()
{
	CWaitCursor wait;

	CDailyEditorDoc* pDoc = GetDocument();
	CWeatherDatabasePtr& pDB = pDoc->GetDatabase();
	ASSERT(pDB->IsOpen());

	const CWeatherStationPtr& pStation = pDoc->GetCurStation();
	
	string fileName = pStation->GetDataFileName();
	string filePath = pDB->GetDataFilePath(fileName);
	
	ERMsg msg;
	msg = pStation->SaveData(filePath, CTM(CTM::DAILY));
	if (!msg)
		UtilWin::SYShowMessage(msg, this);

	return msg;
}

void CWeatherSpreadsheetWnd::ExportToExcel()
{
	ERMsg msg;

	CRegistry registry;
	char separator = registry.GetListDelimiter();
	
	//if Daily data: send direcly to excel
	CDailyEditorDoc* pDoc = GetDocument();
	CWeatherDatabasePtr& pDB = pDoc->GetDatabase();
	ASSERT(pDB->IsOpen());
	const CWeatherStationPtr& pStation = pDoc->GetCurStation();
	string fileName = pStation->GetDataFileName();

	string filePath;
	if (pDoc->GetTM() == CTM(CTM::DAILY) )
	{
		filePath = pDB->GetDataFilePath(fileName);
	}
	else
	{
		CWaitCursor wait;

		//Save the data 
		filePath = GetUserDataPath() + "tmp\\" + fileName;
		CreateMultipleDir(GetPath(filePath));
		msg += pStation->SaveData(filePath, pDoc->GetTM(), separator);
	}

	if (msg)
	{
		msg = CallApplication(CRegistry::SPREADSHEET1, filePath, GetSafeHwnd(), SW_SHOW, true);
	}

	if (!msg)
		UtilWin::SYShowMessage(msg, this);
}

void CWeatherSpreadsheetWnd::OnDateChange(UINT ID)
{
	ASSERT(ID == ID_TABLE_PERIOD_BEGIN || ID == ID_TABLE_PERIOD_END);

	CDailyEditorDoc* pDoc = (CDailyEditorDoc*)GetDocument();

	COleDateTime oFirstDate;
	COleDateTime oLastDate;
	CMFCToolBarDateTimeCtrl* pCtrl1 = CMFCToolBarDateTimeCtrl::GetByCmd(ID_TABLE_PERIOD_BEGIN); ASSERT(pCtrl1);
	CMFCToolBarDateTimeCtrl* pCtrl2 = CMFCToolBarDateTimeCtrl::GetByCmd(ID_TABLE_PERIOD_END); ASSERT(pCtrl2);

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
			if (ID == ID_TABLE_PERIOD_BEGIN)
				end = begin;
			else
				begin = end;
		}
		period = CTPeriod(begin, end);
	}

	pDoc->SetPeriod(period);

}





BOOL CWeatherSpreadsheetWnd::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	//let the trl to route command
	CWnd* pFocus = GetFocus();
	if (pFocus)
	{
		CWnd* pParent = pFocus->GetParent();

		if (pFocus == &m_grid || pParent == &m_grid)
		{
			if (m_grid.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
				return TRUE;
		}
	}

	return CDockablePane::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

