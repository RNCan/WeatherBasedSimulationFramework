
#include "stdafx.h"

#include "FileManager/FileManager.h"
#include "OutputView.h"
#include "Resource.h"
#include "MainFrm.h"
#include "NormalsEditorDoc.h"

#include "WeatherbasedSimulationUI.h"

using namespace std;
using namespace WBSF;
using namespace UtilWin;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static const UINT ID_MESSAGE_WND = 1001;
static const UINT ID_PROGRESS_WND = 1002;



IMPLEMENT_DYNCREATE(COutputView, CView)
BEGIN_MESSAGE_MAP(COutputView, CView)
	ON_WM_CREATE()
	ON_WM_MOUSEACTIVATE()
	ON_WM_SIZE()
END_MESSAGE_MAP()


COutputView::COutputView()
{
//	m_outputType=OUTPUT_MESSAGE;
}

COutputView::~COutputView()
{}




int COutputView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	VERIFY(m_messageWnd.Create(WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | WS_HSCROLL | WS_VSCROLL | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, CRect(), this, ID_MESSAGE_WND));
	VERIFY(m_progressWnd.Create(NULL, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, CRect(), this, ID_PROGRESS_WND));

	m_progressWnd.SetMessageCtrl(&m_messageWnd);
	m_font.CreateStockObject(DEFAULT_GUI_FONT);
	m_messageWnd.SetFont(&m_font);
	m_messageWnd.SetTabStops(8);


	return 0;
}

void COutputView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);
	
	AdjustLayout();
}

void COutputView::AdjustLayout()
{
	if (GetSafeHwnd() == NULL)
		return;

	CRect rect;
	GetClientRect(rect);

	CNormalsEditorDoc* pDoc = GetDocument();
	if (pDoc && pDoc->IsExecute())
	{
		m_messageWnd.SetWindowPos(NULL, 0, 0, rect.Width(), rect.Height()/2, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE);
		m_progressWnd.SetWindowPos(NULL, rect.left, rect.top + rect.Height() / 2, rect.Width(), rect.Height() / 2, SWP_NOACTIVATE | SWP_NOZORDER);
	}
	else
	{
		m_messageWnd.SetWindowPos(NULL, 0, 0, rect.Width(), rect.Height(), SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE);
		m_progressWnd.SetWindowPos(NULL, 0,0,0,0, SWP_NOACTIVATE | SWP_NOZORDER);
	}

}



CNormalsEditorDoc* COutputView::GetDocument() const
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CNormalsEditorDoc)));
	return (CNormalsEditorDoc*)m_pDocument;
}
//
//void COutputView::OnInitialUpdate()
//{
//	CNormalsEditorDoc* pDoc = GetDocument();
//	ASSERT(pDoc);
//	pDoc->OnInitialUpdate();
//}


void COutputView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	CNormalsEditorDoc* pDoc = (CNormalsEditorDoc*)GetDocument();
	if (!pDoc)
		return;


	if (lHint == CNormalsEditorDoc::INIT || lHint == CNormalsEditorDoc::OUTPUT_CHANGE)//|| lHint == CNormalsEditorDoc::SELECTION_CHANGE || lHint == CNormalsEditorDoc::TASK_CHANGE
	{
		//ici on pourrait extraire des information suplémentaire
		CString str(pDoc->GetOutputText().c_str());
		str.Replace(_T("\r"), _T(""));
		str.Replace(_T("\n"), _T("\r\n"));

		m_messageWnd.SetWindowText(str);
	}

	/*WeatherUpdaterDoc* pDoc = GetDocument();
	if (!pDoc->IsExecute())
	{
		CTasksProject* pProject = pDoc->GetProject();

		string iName = pDoc->GetCurSel();

		CExecutablePtr pItem = project.FindItem(iName);

		if (pItem)
		{
			CString text = CString( pItem->GetOutputMessage(GetFileManager()).c_str() );
			text.Remove('\r');
			text.Replace(_T("\n"),_T("\r\n"));
			m_messageWnd.SetWindowText(text);
		}
	}*/
}

void COutputView::OnDraw(CDC* pDC)
{

}


BOOL COutputView::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	//let the trl to route command
	CWnd* pFocus = GetFocus();
	if (pFocus)
	{
		CWnd* pParent = pFocus->GetParent();

		if (pFocus == &m_progressWnd || pParent == &m_progressWnd )
		{
			if (m_progressWnd.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
				return TRUE;
		}

		if (pFocus == &m_messageWnd || pParent == &m_messageWnd)
		{
			if (m_messageWnd.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
				return TRUE;
		}

		
	}

	return CView::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

#ifdef _DEBUG
void COutputView::AssertValid() const
{
	CView::AssertValid();
}

void COutputView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

#endif //_DEBUG


