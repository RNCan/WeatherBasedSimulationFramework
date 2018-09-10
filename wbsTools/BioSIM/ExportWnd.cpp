
// BioSIMView.cpp : implementation of the CExportView class
//

#include "stdafx.h"

#include "Basic/Registry.h"
#include "FileManager/FileManager.h"
#include "UI/Common/SYShowMessage.h"
#include "UI/Common/ProgressStepDlg.h"
#include "OutputView.h"
#include "MainFrm.h"
#include "BioSIMDoc.h"
#include "ExportWnd.h"
#include "BioSIM.h"


using namespace std;
using namespace WBSF;
using namespace WBSF::DIMENSION;
using namespace UtilWin;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CBioSIMDoc* CExportWnd::GetDocument()
{
	CDocument* pDoc = NULL;
	CWinApp* pApp = AfxGetApp();
	if (pApp)
	{
		POSITION  pos = pApp->GetFirstDocTemplatePosition();
		CDocTemplate* docT = pApp->GetNextDocTemplate(pos);
		if (docT)
		{
			pos = docT->GetFirstDocPosition();
			pDoc = docT->GetNextDoc(pos);
		}
	}

	return static_cast<CBioSIMDoc*>(pDoc);
}


//********************************************************************************************
BEGIN_MESSAGE_MAP(CExportWndToolBar2, CMFCToolBar)
	ON_WM_SIZE()
END_MESSAGE_MAP()


BOOL CExportWndToolBar2::LoadToolBarEx(UINT uiToolbarResID, CMFCToolBarInfo& params, BOOL bLocked)
{
	if (!CMFCToolBar::LoadToolBarEx(uiToolbarResID, params, bLocked))
		return FALSE;

	CMFCToolBarButton checkCtrl(ID_EXPORT_CHECK, 0);
	checkCtrl.SetStyle(TBBS_CHECKBOX);
	ReplaceButton(ID_EXPORT_CHECK, checkCtrl);
	

	//CMFCToolBarEditBoxButton nameCtrl(ID_EXPORT_NAME, 2);
	CMFCToolBarEditBoxButton nameCtrl(ID_EXPORT_NAME, 2);
	//ReplaceButton(ID_EXPORT_NAME, nameCtrl);
	InsertButton(nameCtrl);

	return TRUE;
}

void CExportWndToolBar2::OnSize(UINT nType, int cx, int cy)
{
	CMFCToolBar::OnSize(nType, cx, cy);

	int index = CommandToIndex(ID_EXPORT_NAME);
	CMFCToolBarButton* pCtrl = GetButton(index);

	if (pCtrl && pCtrl->GetHwnd()!=NULL)
	{
		CRect rect = pCtrl->Rect();
		rect.right = max(10, cx);
		pCtrl->SetRect(rect);
	}

}

//********************************************************************************************
//static const UINT ID_VARABLES_CTRL = 1000;
//static const UINT ID_STATISTICS_CTRL = 1001;



//***********************************************************************
CExportWnd::CExportWnd()
{
	m_bDesableUpdate = true;
}

CExportWnd::~CExportWnd()
{
}

BEGIN_MESSAGE_MAP(CExportWnd, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_MOUSEACTIVATE()
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_UPDATE_COMMAND_UI_RANGE(ID_EXPORT_NOW, ID_EXPORT_NAME, OnUpdateToolbar)
	
	ON_COMMAND_RANGE(ID_EXPORT_NOW, ID_EXPORT_SPREADSHEET2, OnExport)
	ON_COMMAND(ID_EXPORT_OPEN_DIRECTORY, OnOpenDirectory)
	ON_BN_CLICKED(ID_EXPORT_CHECK, OnExportCheked)
	ON_EN_CHANGE(ID_EXPORT_NAME, OnNameChange)
	ON_REGISTERED_MESSAGE(WM_XHTMLTREE_CHECKBOX_CLICKED, OnCheckbox)
END_MESSAGE_MAP()

int CExportWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;
	

	m_wndToolBar1.Create(this, AFX_DEFAULT_TOOLBAR_STYLE|CBRS_SIZE_DYNAMIC, IDR_EXPORT_TOOLBAR);
	m_wndToolBar1.LoadToolBar(IDR_EXPORT_TOOLBAR, 0, 0, TRUE /* Is locked */);
	m_wndToolBar1.SetPaneStyle(m_wndToolBar1.GetPaneStyle() | CBRS_TOOLTIPS | CBRS_FLYBY);
	m_wndToolBar1.SetPaneStyle(m_wndToolBar1.GetPaneStyle() & ~(CBRS_GRIPPER | CBRS_SIZE_DYNAMIC | CBRS_BORDER_TOP | CBRS_BORDER_BOTTOM | CBRS_BORDER_LEFT | CBRS_BORDER_RIGHT));
	m_wndToolBar1.SetOwner(&m_variablesCtrl);
	m_wndToolBar1.SetRouteCommandsViaFrame(FALSE);

	m_wndToolBar2.Create(this, AFX_DEFAULT_TOOLBAR_STYLE | CBRS_SIZE_DYNAMIC, IDR_EXPORT_TOOLBAR2);
	m_wndToolBar2.LoadToolBar(IDR_EXPORT_TOOLBAR2, 0, 0, TRUE /* Is locked */);
	m_wndToolBar2.SetPaneStyle(m_wndToolBar2.GetPaneStyle() | CBRS_TOOLTIPS | CBRS_FLYBY);
	m_wndToolBar2.SetPaneStyle(m_wndToolBar2.GetPaneStyle() & ~(CBRS_GRIPPER | CBRS_SIZE_DYNAMIC | CBRS_BORDER_TOP | CBRS_BORDER_BOTTOM | CBRS_BORDER_LEFT | CBRS_BORDER_RIGHT));
	m_wndToolBar2.SetOwner(&m_variablesCtrl);
	m_wndToolBar2.SetRouteCommandsViaFrame(FALSE);


	DWORD dwStyle = WS_CHILD | WS_VISIBLE | TVS_CHECKBOXES | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
	DWORD dwStyleEx = 0;
	m_wndSplitter.CreateStatic(this, 2, 1);
	
	if (!m_wndSplitter.AddWindow(0, 0, &m_variablesCtrl, WC_TREEVIEW, dwStyle, dwStyleEx, CSize(100, 100)))
		return -1;
	
	if (!m_wndSplitter.AddWindow(1, 0, &m_statisticCtrl, WC_TREEVIEW, dwStyle, dwStyleEx, CSize(100, 100)))
		return -1;

	
	return 0;
}


void CExportWnd::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);

	if (GetSafeHwnd() == NULL)
		return;

	AdjustLayout();
}


void CExportWnd::AdjustLayout()
{
	CRect rectClient;
	GetClientRect(rectClient);

	int cyTlb = m_wndToolBar1.CalcFixedLayout(FALSE, TRUE).cy;
	

	m_wndToolBar1.SetWindowPos(NULL, rectClient.left, rectClient.top, rectClient.Width(), cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);
	m_wndToolBar2.SetWindowPos(NULL, rectClient.left, rectClient.top + cyTlb, rectClient.Width(), cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);
	m_wndSplitter.SetRowInfo(0, max(10, (rectClient.Height() - 2 * cyTlb) / 2), 25);
	m_wndSplitter.SetRowInfo(1, max(10, (rectClient.Height() - 2 * cyTlb) / 2), 25);
	m_wndSplitter.SetWindowPos(NULL, rectClient.left, rectClient.top + 2*cyTlb, rectClient.Width(), rectClient.Height() - 2*cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);

}

void CExportWnd::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	CBioSIMDoc* pDoc = (CBioSIMDoc*)GetDocument();
	CBioSIMProject& project = pDoc->GetProject();
	
	if (lHint == CBioSIMDoc::ID_LAGUAGE_CHANGE)
	{
		m_variablesCtrl.DeleteAllItems();
		m_statisticCtrl.DeleteAllItems();
	}

	m_iName = pDoc->GetCurSel();
	CExecutablePtr pExec = project.FindItem(m_iName);

	if (pExec)
	{
		if (lHint == CBioSIMDoc::INIT )
		{
		}

		//set data
		m_export = pExec->GetExport();

		CParentInfo info;
		pExec->GetParentInfo(GetFM(), info, DIMENSION::VARIABLE);
		m_variablesCtrl.SetOutputDefinition(info.m_variables);

		//SetExportToInterface(m_export);
		m_bDesableUpdate = true;

		int index = m_wndToolBar2.CommandToIndex(ID_EXPORT_NAME);
		CMFCToolBarEditBoxButton* pCtrl = (CMFCToolBarEditBoxButton*)m_wndToolBar2.GetButton(index);
		ENSURE(pCtrl && pCtrl->GetEditBox());
		pCtrl->GetEditBox()->SetWindowText(CString(m_export.m_fileName.c_str()));

		m_variablesCtrl.SetData(m_export.m_variables);
		m_statisticCtrl.SetSelection(m_export.m_statistic);


		m_bDesableUpdate = false;

	}

}
//
//void CExportWnd::GetExportFromInterface(CExport& oExport)
//{
//	//int index = m_wndToolBar2.CommandToIndex(ID_EXPORT_NAME);
//	//CMFCToolBarEditBoxButton* pCtrl = (CMFCToolBarEditBoxButton*)m_wndToolBar2.GetButton(index);
//
//	//CString str;
//	//pCtrl->GetEditBox()->GetWindowText(str);
//	//oExport.m_fileName = CStringA(str);
//
//	//m_variablesCtrl.GetData(oExport.m_variables);
//	//oExport.m_statistic = m_statisticCtrl.GetSelection();
//}
//
//void CExportWnd::SetExportToInterface(const CExport& oExport)
//{
//	/*m_bDesableUpdate = true;
//
//	int index = m_wndToolBar2.CommandToIndex(ID_EXPORT_NAME);
//	CMFCToolBarEditBoxButton* pCtrl = (CMFCToolBarEditBoxButton*)m_wndToolBar2.GetButton(index);
//	ENSURE(pCtrl && pCtrl->GetEditBox());
//	pCtrl->GetEditBox()->SetWindowText(CString(oExport.m_fileName.c_str()));
//	
//	m_variablesCtrl.SetData(oExport.m_variables);
//	m_statisticCtrl.SetSelection(oExport.m_statistic);
//
//
//	m_bDesableUpdate = false;*/
//}




void CExportWnd::UpdateExport(CDimension dimension, CExport& theExport)
{
	if (theExport.m_variables.empty())
	{
		for (int d = 0; d<NB_DIMENSION; d++)
		{
			if (dimension[d]>1 || d == VARIABLE)
			{
				for (int f = 0; f<m_variablesCtrl.GetNbField(d); f++)
				{
					theExport.m_variables.push_back(CVariableDefine(d, f));
				}
			}
		}
	}
}


// CBioSIMDoc commands
UINT CExportWnd::ExportTask(void* pParam)
{
	CProgressStepDlgParam* pMyParam = (CProgressStepDlgParam*)pParam;
	CBioSIMProject* pProject = (CBioSIMProject*)pMyParam->m_pThis;
	CFileManager* pFM = (CFileManager*)pMyParam->m_pExtra;

	ERMsg* pMsg = pMyParam->m_pMsg;
	CCallback* pCallback = pMyParam->m_pCallback;

	VERIFY(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED) == S_OK);
	TRY
		*pMsg = pProject->Export(*pFM, CExecutable::EXPORT_CSV, *pCallback);
	CATCH_ALL(e)
		*pMsg = ::SYGetMessage(*e);
	END_CATCH_ALL

	CoUninitialize();

	if (*pMsg)
		return 0;

	return -1;
}
void CExportWnd::OnExport(UINT ID)
{
	CBioSIMDoc* pDoc = GetDocument();

	if (!pDoc->IsInit())
		return;

	if (pDoc->IsExecute())
		return;


	ERMsg msg;

	CBioSIMProject& project = pDoc->GetProject();
	string iName = pDoc->GetCurSel();
	CExecutablePtr pExec = project.FindItem(iName);
	if (pExec)
	{
		CMainFrame* pMainFrm = (CMainFrame*)AfxGetMainWnd();
		COutputView* pView = (COutputView*)pMainFrm->GetActiveView();
		CProgressWnd& progressWnd = pView->GetProgressWnd();


		pDoc->SetIsExecute(true);
		pView->AdjustLayout();

		//GetProject().LoadDefaultCtrl();
		
		progressWnd.SetTaskbarList(pMainFrm->GetTaskBarList());

		CProgressStepDlgParam param(pExec.get(), NULL, &GetFM());

		TRY
		{
			pExec->LoadDefaultCtrl();
			msg = progressWnd.Execute(CExportWnd::ExportTask, &param);
		}
		CATCH_ALL(e)
		{
			msg = SYGetMessage(*e);
		}
		END_CATCH_ALL


		pDoc->SetIsExecute(false);
		pView->AdjustLayout();


		if (msg)
		{
			if (ID == ID_EXPORT_NOW)
			{
				//do nothing
			}
			else if (ID == ID_EXPORT_SPREADSHEET1 || ID == ID_EXPORT_SPREADSHEET2)
			{

				string appName = CRegistry::SPREADSHEET1;
				if (ID == ID_EXPORT_SPREADSHEET2)
					appName = CRegistry::SPREADSHEET2;

				string argument = pExec->GetExportFilePath(GetFM());
				CallApplication(appName, argument, GetSafeHwnd(), SW_SHOW);
			}
		}
		else
		{
			SYShowMessage(msg, this);
		}

		//CProgressStepDlg dlg(this);
		//dlg.Create();

		//
		//pExec->LoadDefaultCtrl();
		//ERMsg msg = pExec->Export(GetFM(), CExecutable::EXPORT_CSV, dlg.GetCallback());
		//if (msg)
		//{
		//	if (ID == ID_EXPORT_NOW)
		//	{
		//		//do nothing
		//	}
		//	else if (ID == ID_EXPORT_SPREADSHEET1 || ID == ID_EXPORT_SPREADSHEET2)
		//	{
		//		
		//		string appName = CRegistry::SPREADSHEET1;
		//		if (ID == ID_EXPORT_SPREADSHEET2)
		//			appName = CRegistry::SPREADSHEET2;

		//		string argument = pExec->GetExportFilePath(GetFM());
		//		CallApplication(appName, argument, GetSafeHwnd(), SW_SHOW);
		//	}
		//}
		//else
		//{
		//	SYShowMessage(msg, this);
		//}
	}
}

void CExportWnd::OnOpenDirectory()
{
	CBioSIMDoc* pDoc = GetDocument();

	if (!pDoc->IsInit())
		return;

	ShellExecute(NULL, _T("open"), Convert(GetFM().GetOutputPath()), NULL, NULL, SW_SHOW);
}


void CExportWnd::OnContextMenu(CWnd* pWnd, CPoint point)
{
	CDockablePane::OnContextMenu(pWnd, point);
	//theApp.GetContextMenuManager()->ShowPopupMenu(IDR_POPUP_EDIT, point.x, point.y, this, TRUE);
}

void CExportWnd::OnExportCheked()
{
	if (m_bDesableUpdate)
		return;

	CBioSIMDoc* pDoc = GetDocument();
	ASSERT(pDoc->IsInit());

	CBioSIMProject& project = pDoc->GetProject();
	string iName = pDoc->GetCurSel();
	CExecutablePtr pExec = project.FindItem(iName);
	if (pExec)
	{
		pExec->m_export.m_bAutoExport = !pExec->m_export.m_bAutoExport;
	}
}

void CExportWnd::OnNameChange()
{
	if (m_bDesableUpdate)
		return;

	CBioSIMDoc* pDoc = GetDocument();
	ASSERT(pDoc->IsInit());

	CBioSIMProject& project = pDoc->GetProject();
	string iName = pDoc->GetCurSel();
	CExecutablePtr pExec = project.FindItem(iName);
	if (pExec)
	{
		int index = m_wndToolBar2.CommandToIndex(ID_EXPORT_NAME);
		CMFCToolBarEditBoxButton* pCtrl = (CMFCToolBarEditBoxButton*)m_wndToolBar2.GetButton(index);
		CString str;
		pCtrl->GetEditBox()->GetWindowText(str);
		pExec->m_export.m_fileName = CStringA(str);
	}
}


LRESULT CExportWnd::OnCheckbox(WPARAM wParam, LPARAM lParam)
{
	if (m_bDesableUpdate)
		return 0;

	XHTMLTREEMSGDATA *pData = (XHTMLTREEMSGDATA *)wParam;
	ASSERT(pData);

	BOOL bChecked = (BOOL)lParam;

	if (pData)
	{
		CBioSIMDoc* pDoc = GetDocument();
		ASSERT(pDoc->IsInit());

		CBioSIMProject& project = pDoc->GetProject();
		string iName = pDoc->GetCurSel();
		CExecutablePtr pExec = project.FindItem(iName);
		if (pExec)
		{
			int a1 = m_wndSplitter.IdFromRowCol(0, 0);
			int a2 = m_wndSplitter.IdFromRowCol(1, 0);

			if (pData->hCtrl == m_variablesCtrl.GetSafeHwnd())
				m_variablesCtrl.GetData(pExec->m_export.m_variables);
			else if (pData->hCtrl == m_statisticCtrl.GetSafeHwnd())
				pExec->m_export.m_statistic = m_statisticCtrl.GetSelection();
		}

	}

	return 0;
}

void CExportWnd::OnUpdateToolbar(CCmdUI *pCmdUI)
{
	CBioSIMDoc* pDoc = GetDocument();
	ASSERT(pDoc);
	
	bool bInit = pDoc->IsInit();
	pCmdUI->Enable(bInit);
	if (bInit)
	{
		string iName = pDoc->GetCurSel();

		CBioSIMProject& project = pDoc->GetProject();
		CExecutablePtr pExec = project.FindItem(iName);
		if (pExec)
		{
			if (pCmdUI->m_nID == ID_EXPORT_CHECK)
			{
				CMFCToolBar* pToolBar = (CMFCToolBar*)(pCmdUI->m_pOther);
				if (pToolBar != NULL)
					pToolBar->SetButtonInfo(0, ID_EXPORT_CHECK, pToolBar->GetButtonStyle(0), pExec->m_export.m_bAutoExport ? 1 : 0);

				pCmdUI->SetCheck(pExec->m_export.m_bAutoExport);
			}
		}//if pExec
	}
}

BOOL CExportWnd::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	//let the view to route command
	CWnd* pFocus = GetFocus();
	if (pFocus)
	{
		CWnd* pParent = pFocus->GetParent();

		if (GetFocus() == &m_wndSplitter && m_wndSplitter.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
			return TRUE;

		if (GetFocus() == &m_variablesCtrl || pParent == &m_variablesCtrl)
			if( m_variablesCtrl.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
				return TRUE;

		if (GetFocus() == &m_statisticCtrl || pParent == &m_statisticCtrl)
			if( m_statisticCtrl.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
				return TRUE;

		if (GetFocus() == &m_wndToolBar2 || pParent == &m_wndToolBar2)
			if( m_wndToolBar2 && m_wndToolBar2.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
				return TRUE;
	}

	return CDockablePane::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}
