
#include "stdafx.h"

#include "FileManager/FileManager.h"
#include "OutputWnd.h"
#include "Resource.h"
#include "MainFrm.h"
#include "BioSIMDoc.h"

#include "WeatherbasedSimulationUI.h"

using namespace std;
using namespace WBSF;


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static const UINT ID_EDIT_CTRL = 1000;

CBioSIMDoc* COutputWnd::GetDocument()
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


BEGIN_MESSAGE_MAP(COutputWnd, CDockablePane)
	ON_WM_CREATE()
	ON_WM_MOUSEACTIVATE()
	ON_WM_SIZE()
END_MESSAGE_MAP()


COutputWnd::COutputWnd()
{
	m_outputType=OUTPUT_MESSAGE;
}

COutputWnd::~COutputWnd()
{}




int COutputWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	m_editCtrl.Create(WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_WANTRETURN | ES_AUTOVSCROLL | ES_AUTOHSCROLL | WS_VSCROLL | WS_HSCROLL, CRect(), this, ID_EDIT_CTRL);
	m_font.CreateStockObject(DEFAULT_GUI_FONT);
	m_editCtrl.SetFont(&m_font);
	m_editCtrl.SetTabStops(8);


	return 0;
}

void COutputWnd::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);
	AdjustLayout();
}

void COutputWnd::AdjustLayout()
{
	if (GetSafeHwnd() == NULL)
		return;
	
	CRect rectClient;
	GetClientRect(rectClient);

	m_editCtrl.SetWindowPos(NULL, rectClient.left, rectClient.top, rectClient.Width(), rectClient.Height(), SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE);
}




void COutputWnd::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	CBioSIMDoc* pDoc = GetDocument();
	
	CBioSIMProject& project = pDoc->GetProject();

	string iName = pDoc->GetCurSel();
	
	CExecutablePtr pItem = project.FindItem(iName);
	
	if( pItem )
	{
		CString text;
		switch( m_outputType )
		{
		case OUTPUT_MESSAGE: text = CString(pItem->GetOutputMessage(GetFileManager()).c_str()); break;
		case VALIDATION: text = CString(pItem->GetValidationMessage(GetFileManager()).c_str()); break;
		default:ASSERT(false);
		}

		text.Remove('\r');
		text.Replace(_T("\n"),_T("\r\n"));
		m_editCtrl.SetWindowText(text);
	}
}

int COutputWnd::OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message)
{
    int nResult = 0;
    CFrameWnd* pParentFrame = GetParentFrame();

	if( pParentFrame == pDesktopWnd )
    {
        // When this is docked
		nResult = CDockablePane::OnMouseActivate(pDesktopWnd, nHitTest, message);
    }
    else
    {
        // When this is not docked
        // pDesktopWnd is the frame window for CDockablePane
        nResult = CWnd::OnMouseActivate( pDesktopWnd, nHitTest, message );
    }
    return nResult;

}

BOOL COutputWnd::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	//let the trl to route command
	CWnd* pFocus = GetFocus();
	if (pFocus)
	{
		CWnd* pParent = pFocus->GetParent();
		CWnd* pOwner = pFocus->GetParentOwner();


		if (pFocus == &m_editCtrl || pParent == &m_editCtrl || pOwner == &m_editCtrl)
		{
			if (m_editCtrl.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
				return TRUE;
		}
	}

	return CDockablePane::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

