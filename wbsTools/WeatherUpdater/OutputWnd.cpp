
#include "stdafx.h"

#include "OutputWnd.h"
#include "Resource.h"
#include "MainFrm.h"
#include "WeatherUpdaterDoc.h"

using namespace WBSF;


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


static CWeatherUpdaterDoc* GetDocument()
{
	CWinApp* pApp = AfxGetApp();
	if (pApp)
	{
		CFrameWnd * pFrame = (CFrameWnd *)(pApp->m_pMainWnd);
		if (pFrame && pFrame->GetSafeHwnd() != NULL)
			return (CWeatherUpdaterDoc*)(pFrame->GetActiveDocument());
	}
	return NULL;

}

//**************************************************************************************************************
// COutputEdit

COutputEdit::COutputEdit()
{
}

COutputEdit::~COutputEdit()
{
}

BEGIN_MESSAGE_MAP(COutputEdit, CListBox)
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_COMMAND(ID_EDIT_CLEAR, OnEditClear)
	//ON_COMMAND(ID_OUTPUT_WND, OnViewOutput)
	ON_WM_WINDOWPOSCHANGING()
END_MESSAGE_MAP()
/////////////////////////////////////////////////////////////////////////////
// Gestionnaires de messages de COutputEdit

void COutputEdit::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
	CContextMenuManager* pMM = ((CWinAppEx*)AfxGetApp())->GetContextMenuManager();
	HMENU hMenu = pMM->GetMenuByName(_T("Edit1"));
	if (hMenu != NULL)
	{
		CMenu* pMenu = CMenu::FromHandle(hMenu);
		ASSERT(pMenu);

		CMenu* pSumMenu = pMenu->GetSubMenu(0);
		ASSERT(pSumMenu);

		pMM->TrackPopupMenu(*pSumMenu, point.x, point.y, this);
	}

	/*CMenu menu;
	menu.LoadMenu(IDR_EDIT_MENU);

	CMenu* pSumMenu = menu.GetSubMenu(0);

	if (AfxGetMainWnd()->IsKindOf(RUNTIME_CLASS(CMDIFrameWndEx)))
	{
		CMFCPopupMenu* pPopupMenu = new CMFCPopupMenu;

		if (!pPopupMenu->Create(this, point.x, point.y, (HMENU)pSumMenu->m_hMenu, FALSE, TRUE))
			return;

		((CMDIFrameWndEx*)AfxGetMainWnd())->OnShowPopupMenu(pPopupMenu);
		UpdateDialogControls(this, FALSE);
	}

	SetFocus();*/
}

void COutputEdit::OnEditCopy()
{
	MessageBox(_T("Copier la sortie"));
}

void COutputEdit::OnEditClear()
{
	MessageBox(_T("Effacer la sortie"));
}

void COutputEdit::OnViewOutput()
{
	CDockablePane* pParentBar = DYNAMIC_DOWNCAST(CDockablePane, GetOwner());
	CMDIFrameWndEx* pMainFrame = DYNAMIC_DOWNCAST(CMDIFrameWndEx, GetTopLevelFrame());

	if (pMainFrame != NULL && pParentBar != NULL)
	{
		pMainFrame->SetFocus();
		pMainFrame->ShowPane(pParentBar, FALSE, FALSE, FALSE);
		pMainFrame->RecalcLayout();

	}
}


//**************************************************************************************************************
static const UINT OUTPUT_TEXT_CTRL_ID = 1000;


BEGIN_MESSAGE_MAP(COutputWnd, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
END_MESSAGE_MAP()


COutputWnd::COutputWnd()
{
}

COutputWnd::~COutputWnd()
{
}



int COutputWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;


	// Créer la fenêtre d'onglets :
	//if (!m_wndTabs.Create(CMFCTabCtrl::STYLE_FLAT, rectDummy, this, 1))
	//{
	//	TRACE0("Impossible de créer la fenêtre d'onglets de sortie\n");
	//	return -1;      // échec de la création
	//}

	// Créer les volets de sortie :
	const DWORD dwStyle = LBS_NOINTEGRALHEIGHT | WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL;

	if (!m_wndOutput.Create(dwStyle, CRect(0, 0, 0, 0), this, OUTPUT_TEXT_CTRL_ID))
	{
		TRACE0("Impossible de créer les fenêtres Sortie\n");
		return -1;      // échec de la création
	}

	UpdateFonts();

	return 0;
}

void COutputWnd::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);

	// Le contrôle onglet doit couvrir toute la zone cliente :
	m_wndOutput.SetWindowPos(NULL, -1, -1, cx, cy, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
}

void COutputWnd::AdjustHorzScroll(CListBox& wndListBox)
{
	CClientDC dc(this);
	CFont* pOldFont = dc.SelectObject(&afxGlobalData.fontRegular);

	int cxExtentMax = 0;

	for (int i = 0; i < wndListBox.GetCount(); i ++)
	{
		CString strItem;
		wndListBox.GetText(i, strItem);

		cxExtentMax = std::max(cxExtentMax, (int)dc.GetTextExtent(strItem).cx);
	}

	wndListBox.SetHorizontalExtent(cxExtentMax);
	dc.SelectObject(pOldFont);
}

void COutputWnd::UpdateFonts()
{
	m_wndOutput.SetFont(&afxGlobalData.fontRegular); 
}

void COutputWnd::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	CWeatherUpdaterDoc* pDoc = (CWeatherUpdaterDoc*)GetDocument();
	if (!pDoc)
		return;


	if (lHint == CWeatherUpdaterDoc::INIT || lHint == CWeatherUpdaterDoc::SELECTION_CHANGE || lHint == CWeatherUpdaterDoc::TASK_CHANGE)
	{
		size_t t = pDoc->GetCurType();
		size_t p = pDoc->GetCurPos(t);
		m_wndOutput.SetWindowText(CString(pDoc->GetOutputText(t,p).c_str()));
	}
}

