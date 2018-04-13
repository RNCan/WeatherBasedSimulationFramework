//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
//****************************************************************************
#include "stdafx.h"
#include "PropertySheetSize.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif








//************************************************************************************************************

/////////////////////////////////////////////////////////////////////////////
// CResizablePropertySheet

IMPLEMENT_DYNAMIC(CResizablePropertySheet, CMFCPropertySheet)

CResizablePropertySheet::CResizablePropertySheet() :
	m_bInitialized(FALSE)
{
}

CResizablePropertySheet::CResizablePropertySheet(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage) :
	CMFCPropertySheet(nIDCaption, pParentWnd, iSelectPage),
	m_bInitialized(FALSE)
{
}

CResizablePropertySheet::CResizablePropertySheet(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage) :
	CMFCPropertySheet(pszCaption, pParentWnd, iSelectPage),
	m_bInitialized(FALSE)
{
}


CResizablePropertySheet::~CResizablePropertySheet()
{
}

BEGIN_MESSAGE_MAP(CResizablePropertySheet, CMFCPropertySheet)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_GETMINMAXINFO()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CResizablePropertySheet message handlers

BOOL CResizablePropertySheet::OnInitDialog()
{
	BOOL bResult = CMFCPropertySheet::OnInitDialog();

	/*GetClientRect(&m_rCrt);
	m_bInitialized = TRUE;*/

	return bResult;
}

BOOL CResizablePropertySheet::PreCreateWindow(CREATESTRUCT cs)
{
	cs.style |= WS_CLIPCHILDREN | WS_CLIPSIBLINGS;

	return CMFCPropertySheet::PreCreateWindow(cs);
}


int CResizablePropertySheet::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CMFCPropertySheet::OnCreate(lpCreateStruct) == -1)
		return -1;

	// keep client area
	//CRect rect;
	//GetClientRect(&rect);

	// set resizable style
	ModifyStyle(DS_MODALFRAME, WS_POPUP | WS_THICKFRAME);
	//ModifyStyleEx(0, WS_EX_TOOLWINDOW);
	//m_wndTab.AutoSizeWindow(TRUE);


	// adjust size to reflect new style
	//::AdjustWindowRectEx(&rect, GetStyle(), ::IsMenu(GetMenu()->GetSafeHmenu()), GetExStyle());
	//SetWindowPos(NULL, 0, 0, rect.Width(), rect.Height(), SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREPOSITION);

	return 0;
}


void CResizablePropertySheet::OnSize(UINT nType, int cx, int cy)
{
	CMFCPropertySheet::OnSize(nType, cx, cy);

	AdjustControlsLayout(cx, cy);
}
void CResizablePropertySheet::AdjustControlsLayout(int cx, int cy)
{
	//if (!m_bInitialized)
		//return;


	CWnd* pParentWnd = GetParent();
	//ASSERT(pParentWnd != NULL && pParentWnd->IsWindowVisible());
	if (pParentWnd == NULL || !pParentWnd->IsWindowVisible() || GetTabControl() == NULL || m_wndOutlookBar.GetSafeHwnd() == NULL)
		return;

	//ASSERT(pParentWnd != NULL && pParentWnd->IsWindowVisible());


	LockWindowUpdate();

	//int dx = cx - m_rCrt.Width();
	//int dy = cy - m_rCrt.Height();
	//GetClientRect(&m_rCrt);


	CTabCtrl* pTab = GetTabControl();
	ASSERT_VALID(pTab);

	CRect rectTabItem;
	pTab->GetItemRect(0, rectTabItem);
	pTab->MapWindowPoints(this, &rectTabItem);


	const int nVertMargin = 5;
	const int nHorzMargin = 5;
	const int nTabsHeight = rectTabItem.Height() + nVertMargin;

	CRect rectClient;
	GetClientRect(rectClient);
	//pTab->SetWindowPos(&wndTop, 0, 0, rectClient.Width()+dx, rectClient.Height()+dy, SWP_NOMOVE | SWP_NOACTIVATE);
	pTab->MoveWindow(m_nBarWidth, -nTabsHeight, rectClient.right, rectClient.bottom - 2 * nVertMargin);


	CRect rectTab;
	pTab->GetWindowRect(rectTab);
	ScreenToClient(rectTab);

	CRect rectNavigator = rectClient;
	rectNavigator.right = rectNavigator.left + m_nBarWidth;
	rectNavigator.bottom = rectTab.bottom;
	rectNavigator.DeflateRect(1, 1);

	m_wndOutlookBar.SetWindowPos(&wndTop, rectNavigator.left, rectNavigator.top, rectNavigator.Width(), rectNavigator.Height(), SWP_NOACTIVATE);

	CPropertyPage* pppg = GetActivePage();

	SetActivePage(pppg);//update page
	CRect rectPage = rectClient;
	rectPage.left = rectNavigator.right;
	rectPage.bottom = rectTab.bottom;
	rectPage.DeflateRect(1, 1);

	pppg->SetWindowPos(NULL, 0, 0, rectPage.Width(), rectPage.Height(), SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOACTIVATE);


	ReposButtons(TRUE);

	UnlockWindowUpdate();



}

int CResizablePropertySheet::ReposButtons(BOOL bRedraw)
{
	const BOOL bIsRTL = (GetExStyle() & WS_EX_LAYOUTRTL);
	const int nHorzMargin = 5;
	const int nVertMargin = 5;

	int nButtonsHeight = 0;

	CRect rectClient;
	GetClientRect(rectClient);

	int ids[] = { IDOK, ID_WIZBACK, ID_WIZNEXT, ID_WIZFINISH, IDCANCEL, ID_APPLY_NOW, IDHELP };

	int nTotalButtonsWidth = 0;

	for (int iStep = 0; iStep < (bIsRTL ? 1 : 2); iStep++)
	{
		for (int i = 0; i < sizeof(ids) / sizeof(ids[0]); i++)
		{
			CWnd* pButton = GetDlgItem(ids[i]);

			if (pButton != NULL )//&& pButton->IsWindowVisible()
			{
				if (ids[i] == IDHELP && (m_psh.dwFlags &PSH_HASHELP) == 0)
				{
					continue;
				}

				if (ids[i] == ID_APPLY_NOW && (m_psh.dwFlags&PSH_NOAPPLYNOW))
				{
					continue;
				}

				CRect rectButton;
				pButton->GetWindowRect(rectButton);
				ScreenToClient(rectButton);

				if (iStep == 0)
				{
					// Align buttons at the bottom
					pButton->SetWindowPos(&wndTop, rectButton.left, rectClient.bottom - rectButton.Height() - nVertMargin, -1, -1, SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
					nTotalButtonsWidth = rectButton.right;
					nButtonsHeight = std::max(nButtonsHeight, rectButton.Height());
				}
				else
				{
					// Right align the buttons
					pButton->SetWindowPos(&wndTop, rectButton.left + rectClient.right - nTotalButtonsWidth - nHorzMargin, rectButton.top, -1, -1, SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
				}

				if (bRedraw)
				{
					pButton->RedrawWindow();
				}
			}
		}
	}

	return nButtonsHeight;
}

void CResizablePropertySheet::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI)
{
	CMFCPropertySheet::OnGetMinMaxInfo(lpMMI);
	lpMMI->ptMinTrackSize.x = 300;
	lpMMI->ptMinTrackSize.y = 300;
}

