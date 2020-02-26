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

//	GetClientRect(&m_rCrt);
	AdjustControlsLayout(0,0);
	m_bInitialized = TRUE;

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

	// set resizable style
	ModifyStyle(DS_MODALFRAME, WS_POPUP | WS_THICKFRAME);

	return 0;
}


void CResizablePropertySheet::OnSize(UINT nType, int cx, int cy)
{
	CMFCPropertySheet::OnSize(nType, cx, cy);

	if (m_bInitialized)
		AdjustControlsLayout(cx, cy);
}
void CResizablePropertySheet::AdjustControlsLayout(int cx, int cy)
{
	CWnd* pParentWnd = GetParent();

	if (pParentWnd == NULL || !pParentWnd->IsWindowVisible() || GetTabControl() == NULL || m_wndOutlookBar.GetSafeHwnd() == NULL)
		return;


	LockWindowUpdate();

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
	pTab->MoveWindow(m_nBarWidth, -nTabsHeight, rectClient.right, rectClient.bottom - 2 * nVertMargin);


	CRect rectTab;
	pTab->GetWindowRect(rectTab);
	ScreenToClient(rectTab);

	CRect rectNavigator = rectClient;
	rectNavigator.right = rectNavigator.left + m_nBarWidth;
	rectNavigator.bottom = rectTab.bottom;
	rectNavigator.DeflateRect(1, 1);

	m_wndOutlookBar.SetWindowPos(&wndTop, rectNavigator.left, rectNavigator.top, rectNavigator.Width(), rectNavigator.Height(), SWP_NOACTIVATE);

	//adjust size of all pages
	m_rectPage = rectClient;
	m_rectPage.left = rectNavigator.right;
	m_rectPage.bottom = rectTab.bottom;
	m_rectPage.DeflateRect(1, 1);

	CPropertyPage* pppg = GetActivePage();
	//resize the active windows
	pppg->SetWindowPos(NULL, 0, 0, m_rectPage.Width(), m_rectPage.Height(), SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOACTIVATE);


	//update current window
	pppg->SetModified();
	pppg->UpdateWindow();


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

void CResizablePropertySheet::OnActivatePage(CPropertyPage* pPage)
{
	CMFCPropertySheet::OnActivatePage(pPage);
	
	if (m_bInitialized)
	{
		//resize activated page
		pPage->SetWindowPos(NULL, 0, 0, m_rectPage.Width(), m_rectPage.Height(), SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOACTIVATE);
	}
}
