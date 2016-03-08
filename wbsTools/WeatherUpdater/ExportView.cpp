
// BioSIMView.cpp : implementation of the CExportView class
//

#include "stdafx.h"

#include "Basic/Registry.h"
#include "UICommon/SYShowMessage.h"
#include "UICommon/ProgressStepDlg.h"
#include "BioSIMDoc.h"
#include "ExportView.h"
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
	CWinApp* pApp = AfxGetApp();
	if (pApp)
	{
		CFrameWnd * pFrame = (CFrameWnd *)(pApp->m_pMainWnd);
		if (pFrame && pFrame->GetSafeHwnd() != NULL)
			return (CBioSIMDoc*)(pFrame->GetActiveDocument());
	}
	return NULL;
}


//********************************************************************************************
BEGIN_MESSAGE_MAP(CExportWndToolBar2, CMFCToolBar)
	ON_WM_SIZE()
END_MESSAGE_MAP()


BOOL CExportWndToolBar2::LoadToolBarEx(UINT uiToolbarResID, CMFCToolBarInfo& params, BOOL bLocked)
{
	if (!CMFCToolBar::LoadToolBarEx(uiToolbarResID, params, bLocked))
		return FALSE;


	CMFCToolBarEditBoxButton nameCtrl(ID_EXPORT_NAME, 0, 150);
	ReplaceButton(ID_EXPORT_NAME, nameCtrl);


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
	ON_COMMAND_EX(ID_TO_SPREADSHEET1, &OnExport)
	ON_COMMAND_EX(ID_TO_SPREADSHEET2, &OnExport)
	ON_COMMAND(ID_OPEN_EXPORT_DIRECTORY, &OnOpenDirectory)
	ON_EN_CHANGE(ID_EXPORT_NAME, OnNameChange)
	
END_MESSAGE_MAP()

int CExportWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;
	

	m_wndToolBar.Create(this, AFX_DEFAULT_TOOLBAR_STYLE|CBRS_SIZE_DYNAMIC, IDR_EXPORT_TOOLBAR);
	m_wndToolBar.LoadToolBar(IDR_EXPORT_TOOLBAR, 0, 0, TRUE /* Is locked */);
	m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() | CBRS_TOOLTIPS | CBRS_FLYBY);
	m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() & ~(CBRS_GRIPPER | CBRS_SIZE_DYNAMIC | CBRS_BORDER_TOP | CBRS_BORDER_BOTTOM | CBRS_BORDER_LEFT | CBRS_BORDER_RIGHT));
	m_wndToolBar.SetOwner(&m_variablesCtrl);
	m_wndToolBar.SetRouteCommandsViaFrame(FALSE);

	m_wndToolBar2.Create(this, AFX_DEFAULT_TOOLBAR_STYLE | CBRS_SIZE_DYNAMIC, IDR_EXPORT_TOOLBAR2);
	m_wndToolBar2.LoadToolBar(IDR_EXPORT_TOOLBAR2, 0, 0, TRUE /* Is locked */);
	m_wndToolBar2.SetPaneStyle(m_wndToolBar.GetPaneStyle() | CBRS_TOOLTIPS | CBRS_FLYBY);
	m_wndToolBar2.SetPaneStyle(m_wndToolBar.GetPaneStyle() & ~(CBRS_GRIPPER | CBRS_SIZE_DYNAMIC | CBRS_BORDER_TOP | CBRS_BORDER_BOTTOM | CBRS_BORDER_LEFT | CBRS_BORDER_RIGHT));
	m_wndToolBar2.SetOwner(&m_variablesCtrl);
	m_wndToolBar2.SetRouteCommandsViaFrame(FALSE);


	DWORD dwStyle = WS_CHILD | WS_VISIBLE;// | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_EDITLABELS;
	DWORD dwStyleEx = 0;
	m_wndSplitter.CreateStatic(this, 2, 1);
	
	if (!m_wndSplitter.AddWindow(0, 0, &m_variablesCtrl, WC_TREEVIEW, dwStyle, dwStyleEx, CSize(100, 100)))
		return -1;

	dwStyle = WS_CHILD | WS_VISIBLE;// | LVS_ICON | LVS_SHAREIMAGELISTS;
	if (!m_wndSplitter.AddWindow(1, 0, &m_statisticCtrl, WC_TREEVIEW, dwStyle, dwStyleEx, CSize(100, 100)))
		return -1;
	
//	AdjustLayout();

	return 0;
}


void CExportWnd::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);

	if (GetSafeHwnd() == NULL)
	{
		return;
	}

	AdjustLayout();
}

void CExportWnd::AdjustLayout()
{
	CRect rectClient;
	GetClientRect(rectClient);

	int cyTlb = m_wndToolBar.CalcFixedLayout(FALSE, TRUE).cy;
	

	m_wndToolBar.SetWindowPos(NULL, rectClient.left, rectClient.top, rectClient.Width(), cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);
	m_wndToolBar2.SetWindowPos(NULL, rectClient.left, rectClient.top + cyTlb, rectClient.Width(), cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);
	m_wndSplitter.SetRowInfo(0, (rectClient.Height() - 2 * cyTlb) / 2, 25);
	m_wndSplitter.SetRowInfo(1, (rectClient.Height() - 2 * cyTlb) / 2, 25);
	m_wndSplitter.SetWindowPos(NULL, rectClient.left, rectClient.top + 2*cyTlb, rectClient.Width(), rectClient.Height() - 2*cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);

}
//
//int CExportWnd::OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message)
//{
//    int nResult = 0;
//    CFrameWnd* pParentFrame = GetParentFrame();
//
//	if( pParentFrame == pDesktopWnd )
//    {
//        // When this is docked
//        nResult = CView::OnMouseActivate(pDesktopWnd, nHitTest, message);
//    }
//    else
//    {
//        // When this is not docked
//        // pDesktopWnd is the frame window for CDockablePane
//        nResult = CWnd::OnMouseActivate( pDesktopWnd, nHitTest, message );
//    }
//    return nResult;
//
//}


void CExportWnd::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	CBioSIMDoc* pDoc = (CBioSIMDoc*)GetDocument();
	CBioSIMProject& project = pDoc->GetProject();

	//save last change
	CExecutablePtr pExec = project.FindItem(m_iName);
	if (pExec)
	{
		CExport oExport;
		GetExportFromInterface(oExport);
		if (oExport != m_export)//if user have made change. Elsewhere we don't change export
			pExec->SetExport(oExport);
	}

	m_iName = pDoc->GetCurSel();
	pExec = project.FindItem(m_iName);

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

		SetExportFromInterface(m_export);
	}

}

void CExportWnd::GetExportFromInterface(CExport& oExport)
{
	int index = m_wndToolBar2.CommandToIndex(ID_EXPORT_NAME);
	CMFCToolBarEditBoxButton* pCtrl = (CMFCToolBarEditBoxButton*)m_wndToolBar2.GetButton(index);

	oExport.m_fileName = CStringA(pCtrl->m_strText);
	m_variablesCtrl.GetData(oExport.m_variables);
	oExport.m_statistic = m_statisticCtrl.GetSelection();
}

void CExportWnd::SetExportFromInterface(const CExport& oExport)
{
	int index = m_wndToolBar2.CommandToIndex(ID_EXPORT_NAME);
	CMFCToolBarEditBoxButton* pCtrl = (CMFCToolBarEditBoxButton*)m_wndToolBar2.GetButton(index);

	pCtrl->m_strText = oExport.m_fileName.c_str();
	m_variablesCtrl.SetData(oExport.m_variables);
	m_statisticCtrl.SetSelection(oExport.m_statistic);

}

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

BOOL CExportWnd::OnExport(UINT ID)
{
	CBioSIMDoc* pDoc = GetDocument();

	if (!pDoc->IsInit())
		return FALSE;


	CBioSIMProject& project = pDoc->GetProject();
	string iName = pDoc->GetCurSel();
	CExecutablePtr pExec = project.FindItem(iName);
	if (pExec)
	{
		CExport oExport;
		GetExportFromInterface(oExport);
		pExec->SetExport(oExport);

		CProgressStepDlg dlg(this);
		dlg.Create();



		int format = CExecutable::EXPORT_CSV;
		if (ID == ID_TO_SPREADSHEET2)
			format = CExecutable::EXPORT_CSV_LOC;

		pExec->LoadDefaultCtrl();
		ERMsg msg = pExec->Export(GetFM(), format, dlg.GetCallback());
		if (msg)
		{
			if (ID == ID_TO_SPREADSHEET1 || ID == ID_TO_SPREADSHEET2)
			{
				
				string appName = CRegistry::SPREADSHEET1;
				if (ID == ID_TO_SPREADSHEET2)
					appName = CRegistry::SPREADSHEET2;

				string argument = pExec->GetExportFilePath(GetFM(), format);
				CallApplication(appName, argument, GetSafeHwnd(), SW_SHOW);
			}
			else if (ID == ID_TO_SCRIPT)
			{
				////string scripName = m_scriptCtrl.GetString();
				//if (!scripName.empty())
				//{
				//	string argument = "\"" + GetFM().Script().GetFilePath(scripName);
				//	argument += "\" \"" + pExec->GetExportFilePath(GetFM(), CExecutable::EXPORT_CSV) + "\"";
				//	CallApplication(CRegistry::R_SCRIPT, argument, GetSafeHwnd(), SW_HIDE, false);
				//}
			}

			//if( ID == ID_TO_SPREADSHEET )
			//{
			//tring argument = pExec->GetExportFilePath(fileManager, format);
			//CallApplication(CRegistry::SPREADSHEET, argument, this);
			//}
			//else if( ID_TO_SHOW_MAP )
			//{
			//	CString argument = pExec->GetExportFilePath(fileManager, format);
			//	CallApplication(CRegistry::SHOWMAP, argument, this);
			//}
		}
		else
		{
			SYShowMessage(msg, this);
		}
	}

	return TRUE;
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


void CExportWnd::OnNameChange()
{
	CBioSIMDoc* pDoc = GetDocument();
	ASSERT(pDoc->IsInit());
		

	CBioSIMProject& project = pDoc->GetProject();
	string iName = pDoc->GetCurSel();
	CExecutablePtr pExec = project.FindItem(iName);
	if (pExec)
	{
		int index = m_wndToolBar2.CommandToIndex(ID_EXPORT_NAME);
		CMFCToolBarEditBoxButton* pCtrl = (CMFCToolBarEditBoxButton*)m_wndToolBar2.GetButton(index);
		pExec->m_export.m_fileName = CStringA(pCtrl->m_strText);
	}
}

//void CExportWnd::OnRButtonUp(UINT nFlags, CPoint point)
//{
//	ClientToScreen(&point);
//	OnContextMenu(this, point);
//}
