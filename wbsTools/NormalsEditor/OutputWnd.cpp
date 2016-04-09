
#include "stdafx.h"

#include "OutputWnd.h"
#include "Resource.h"
#include "MainFrm.h"
#include "NormalsEditorDoc.h"

using namespace WBSF;


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


static CNormalsEditorDoc* GetDocument()
{
	CWinApp* pApp = AfxGetApp();
	if (pApp)
	{
		CFrameWnd * pFrame = (CFrameWnd *)(pApp->m_pMainWnd);
		if (pFrame && pFrame->GetSafeHwnd() != NULL)
			return (CNormalsEditorDoc*)(pFrame->GetActiveDocument());
	}
	return NULL;

}

static CWeatherDatabasePtr GetDatabasePtr()
{
	CWeatherDatabasePtr pDB;
	CNormalsEditorDoc* pDocument = GetDocument();

	if (pDocument)
		pDB = pDocument->GetDatabase();


	return  pDB;
}
/////////////////////////////////////////////////////////////////////////////
// COutputBar

COutputWnd::COutputWnd()
{
}

COutputWnd::~COutputWnd()
{
}

BEGIN_MESSAGE_MAP(COutputWnd, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
END_MESSAGE_MAP()

static const UINT OUTPUT_TEXT_CTRL_ID = 4000;
int COutputWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	
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
	if (lHint == CNormalsEditorDoc::INIT || lHint == CNormalsEditorDoc::OUTPUT_CHANGE)
	{
		CNormalsEditorDoc* pDoc = GetDocument();
		m_wndOutput.SetWindowText(CString(pDoc->GetOutputText().c_str()));
	}
}

BOOL COutputWnd::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	//let the trl to route command
	CWnd* pFocus = GetFocus();
	if (pFocus)
	{
		CWnd* pParent = pFocus->GetParent();

		if (pFocus == &m_wndOutput || pParent == &m_wndOutput)
		{
			if (m_wndOutput.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
				return TRUE;
		}
	}

	return CDockablePane::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}
