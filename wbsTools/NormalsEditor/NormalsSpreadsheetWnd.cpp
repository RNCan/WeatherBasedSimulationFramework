
// BioSIMView.cpp : implementation of the CNormalsSpreadsheetWnd class
//

#include "stdafx.h"
#include "NormalsEditor.h"

#include "NormalsEditorDoc.h"
#include "NormalsSpreadsheetWnd.h"
#include "Basic/Registry.h"
#include "UI/Common/AppOption.h"
#include "UI/Common/SYShowMessage.h"
#include "UI/WVariablesEdit.h"

#include "resource.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


using namespace std;
using namespace UtilWin;
using namespace WBSF;






IMPLEMENT_SERIAL(CNormalsSpreadsheetToolBar, CMFCToolBar, 1)
BOOL CNormalsSpreadsheetToolBar::LoadToolBarEx(UINT uiToolbarResID, CMFCToolBarInfo& params, BOOL bLocked)
{
	if (!CMFCToolBar::LoadToolBarEx(uiToolbarResID, params, bLocked))
		return FALSE;

	UpdateTooltips();

	return TRUE;
}


//**************************************************************************************************************************************
// CNormalsSpreadsheetWnd
//IMPLEMENT_DYNCREATE(CNormalsSpreadsheetWnd, CDockablePane)



CNormalsEditorDoc* CNormalsSpreadsheetWnd::GetDocument()
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

CWeatherDatabasePtr CNormalsSpreadsheetWnd::GetDatabasePtr()
{
	CWeatherDatabasePtr pDB;
	CNormalsEditorDoc* pDocument = GetDocument();

	if (pDocument)
		pDB = pDocument->GetDatabase();


	return  pDB;
}

static const int IDC_GRID_ID = 1002;


BEGIN_MESSAGE_MAP(CNormalsSpreadsheetWnd, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_WINDOWPOSCHANGED()
	ON_UPDATE_COMMAND_UI_RANGE(ID_TABLE_MODE_VISUALISATION, ID_TABLE_SENDTO_EXCEL, OnUpdateToolbar)
	ON_COMMAND_RANGE(ID_TABLE_MODE_VISUALISATION, ID_TABLE_SENDTO_EXCEL, OnToolbarCommand)
	ON_CONTROL_RANGE(LBN_SELCHANGE, ID_TABLE_MODE_VISUALISATION, ID_TABLE_SENDTO_EXCEL, OnToolbarCommand)

END_MESSAGE_MAP()


// CNormalsSpreadsheetWnd construction/destruction
CNormalsSpreadsheetWnd::CNormalsSpreadsheetWnd()
{
	m_bMustBeUpdated=false;
}

CNormalsSpreadsheetWnd::~CNormalsSpreadsheetWnd()
{
}

// CNormalsSpreadsheetWnd drawing

int CNormalsSpreadsheetWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	CreateToolBar();

	m_grid.CreateGrid(WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), this, IDC_GRID_ID);

	AdjustLayout();

	return 0;
}

void CNormalsSpreadsheetWnd::CreateToolBar()
{
	if (m_wndToolBar.GetSafeHwnd())
		m_wndToolBar.DestroyWindow();

	m_wndToolBar.Create(this, AFX_DEFAULT_TOOLBAR_STYLE | CBRS_SIZE_DYNAMIC, IDR_TABLE_TOOLBAR);
	m_wndToolBar.LoadToolBar(IDR_TABLE_TOOLBAR, 0, 0, TRUE);
	m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() | CBRS_TOOLTIPS | CBRS_FLYBY);
	m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() & ~(CBRS_GRIPPER | CBRS_SIZE_DYNAMIC | CBRS_BORDER_TOP | CBRS_BORDER_BOTTOM | CBRS_BORDER_LEFT | CBRS_BORDER_RIGHT));
	m_wndToolBar.SetOwner(this);
	m_wndToolBar.SetRouteCommandsViaFrame(FALSE);

}

void CNormalsSpreadsheetWnd::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);
	
	AdjustLayout();
}

void CNormalsSpreadsheetWnd::AdjustLayout()
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

void CNormalsSpreadsheetWnd::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	CNormalsEditorDoc* pDoc = (CNormalsEditorDoc*)GetDocument();
	//CTPeriod period = pDoc->GetPeriod();
	//bool bPeriodEnabled = pDoc->GetPeriodEnabled();

	if (lHint == CNormalsEditorDoc::LANGUAGE_CHANGE)
	{
		CNormalsDataGridCtrl::ReloadString();

		CreateToolBar();
		AdjustLayout();
	}

	if (lHint == CNormalsEditorDoc::DATA_PROPERTIES_EDITION_MODE_CHANGE)
	{
		m_grid.SetIsModified(false);
	}

	m_grid.m_pStation = pDoc->GetCurStation();


	bool bVisible = IsWindowVisible();
	if( bVisible )
		m_grid.Update();
	else
		m_bMustBeUpdated=true;
	

}

void CNormalsSpreadsheetWnd::OnWindowPosChanged(WINDOWPOS* lpwndpos)
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


void CNormalsSpreadsheetWnd::OnUpdateToolbar(CCmdUI *pCmdUI)
{
	CNormalsEditorDoc* pDoc = GetDocument();
	bool bInit = pDoc->GetCurStationIndex() != UNKNOWN_POS;

	switch (pCmdUI->m_nID)
	{
	case ID_TABLE_MODE_VISUALISATION:pCmdUI->Enable(bInit); pCmdUI->SetCheck(!pDoc->GetDataInEdition()); break;
	case ID_TABLE_MODE_EDITION:	pCmdUI->Enable(bInit); pCmdUI->SetCheck(pDoc->GetDataInEdition()); break;
	case ID_TABLE_SAVE:			pCmdUI->Enable(bInit && pDoc->GetDataInEdition()); break;
	case ID_TABLE_SENDTO_EXCEL:	pCmdUI->Enable(bInit && !pDoc->GetDataInEdition()); break;
	//case ID_TABLE_FILTER:		pCmdUI->Enable(bInit); break;
	}

}

void CNormalsSpreadsheetWnd::OnToolbarCommand(UINT ID)
{
	CNormalsEditorDoc* pDoc = GetDocument();
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
					if( pDoc->CancelDataEdition() )
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
			pDoc->SetDataInEdition(true);
		break;
	}
	case ID_TABLE_SAVE:			SaveData();  break;
	case ID_TABLE_SENDTO_EXCEL:	ExportToExcel(); break;
	//case ID_TABLE_FILTER:			pDoc->SetVariables(((CMFCToolBarWVariablesButton*)pCtrl)->GetVariables()); break;
	}
}

bool CNormalsSpreadsheetWnd::SaveData()
{
	CWaitCursor wait;

	CNormalsEditorDoc* pDoc = GetDocument();
	CNormalsDatabasePtr& pDB = pDoc->GetDatabase();
	ASSERT(pDB->IsOpen());

	const CNormalsStationPtr& pStation = pDoc->GetCurStation();
	
	string fileName = pStation->GetDataFileName();
	string filePath = pDB->GetDataFilePath(fileName);
	
	ERMsg msg;
	msg = pStation->SaveData(filePath, CTM(CTM::DAILY));
	if (!msg)
		SYShowMessage(msg, this);

	return msg;
}

void CNormalsSpreadsheetWnd::ExportToExcel()
{
	ERMsg msg;

	CRegistry registry;
	char separator = registry.GetListDelimiter();
	
	//if Normals data: send direcly to excel
	CNormalsEditorDoc* pDoc = GetDocument();
	CNormalsDatabasePtr& pDB = pDoc->GetDatabase();
	ASSERT(pDB->IsOpen());
	const CNormalsStationPtr& pStation = pDoc->GetCurStation();
	string fileName = pStation->GetDataFileName();

	string filePath;
	
	CWaitCursor wait;

	//Save the data 
	filePath = WBSF::GetUserDataPath() + "tmp\\" + fileName;
	WBSF::CreateMultipleDir(WBSF::GetPath(filePath));
	msg += pStation->SaveData(filePath, CTM(CTM::MONTHLY, CTM::OVERALL_YEARS), separator);
	

	if (msg)
	{
		msg = WBSF::CallApplication(WBSF::CRegistry::SPREADSHEET1, filePath, GetSafeHwnd(), SW_SHOW, true);
	}

	if (!msg)
		SYShowMessage(msg, this);
}

BOOL CNormalsSpreadsheetWnd::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
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
