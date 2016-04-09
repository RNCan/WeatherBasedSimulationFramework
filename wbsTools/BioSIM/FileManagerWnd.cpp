
#include "stdafx.h"
#include "Simulation/BioSIMProject.h"
#include "Simulation/BioSIMProject.h"
#include "UI/Common\SYShowMessage.h"
#include "WeatherBasedSimulationUI.h"

#include "mainfrm.h"
#include "FileManagerWnd.h"
#include "Resource.h"
#include "BioSIMDoc.h"

using namespace std;
using namespace UtilWin;
using namespace WBSF;



#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


BEGIN_MESSAGE_MAP(CFileManagerWnd, CDockablePane)
	ON_WM_SIZE()
	ON_WM_CREATE()
	ON_WM_WINDOWPOSCHANGED()
END_MESSAGE_MAP()


CBioSIMDoc* CFileManagerWnd::GetDocument()
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

CFileManagerWnd::CFileManagerWnd()
{
	m_bMustBeUpdated = false;
}

int CFileManagerWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	//m_wnd.Create(this, WS_GROUP | WS_CHILD | WS_VISIBLE | WS_TABSTOP);//, WS_EX_CONTROLPARENT| WS_EX_TOOLWINDOW
	//m_wnd.ShowWindow(SW_SHOW);
	//CNormalMPage* pNew = new CNormalMPage();
	//m_wndOutlookBar.Create(_T(""), this, CRect(), 1000, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN );
	//m_wndOutlookBar.AddTab(pNew);

	return 0;
}

BOOL CFileManagerWnd::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	//let the trl to route command
	//if (m_wnd.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
	//	return TRUE;

	return CDockablePane::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

void CFileManagerWnd::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{

}

void CFileManagerWnd::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);
	AdjustLayout();
}

void CFileManagerWnd::AdjustLayout()
{
	if (GetSafeHwnd() == NULL/* || m_wndOutlookBar.GetSafeHwnd() == NULL*/)
		return;

	/*CRect rectClient;
	GetClientRect(rectClient);
	m_wndOutlookBar.SetWindowPos(NULL, 0, 0, rectClient.Width(), rectClient.Height(), SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);
*/
}


void CFileManagerWnd::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
	CDockablePane::OnWindowPosChanged(lpwndpos);

	if (lpwndpos->flags & SWP_SHOWWINDOW)
	{
		if (m_bMustBeUpdated)
		{
			//m_wnd.Update();
			m_bMustBeUpdated = false;
		}
	}
}

