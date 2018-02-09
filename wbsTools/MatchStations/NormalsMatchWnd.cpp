
// NormalsMatchView.cpp : implementation of the CNormalsListCtrl class
//

#include "stdafx.h"


#include "Basic/Registry.h"
#include "UI/Common/AppOption.h"
#include "UI/Common/SYShowMessage.h"
#include "UI/WVariablesEdit.h"


#include "MatchStationApp.h"
#include "MatchStationDoc.h"
#include "NormalsMatchWnd.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


using namespace std;
using namespace UtilWin;
using namespace WBSF;

static CMatchStationDoc* GetDocument()
{
	CWinApp* pApp = AfxGetApp();
	if (pApp)
	{
		CFrameWnd * pFrame = (CFrameWnd *)(pApp->m_pMainWnd);
		if (pFrame && pFrame->GetSafeHwnd() != NULL)
			return (CMatchStationDoc*)(pFrame->GetActiveDocument());
	}
	return NULL;

}

IMPLEMENT_SERIAL(CNormalsListToolBar, CMFCToolBar, 1)
	BOOL CNormalsListToolBar::LoadToolBarEx(UINT uiToolbarResID, CMFCToolBarInfo& params, BOOL bLocked)
{
	if (!CMFCToolBar::LoadToolBarEx(uiToolbarResID, params, bLocked))
		return FALSE;

	UpdateButton();

	return TRUE;
}

void CNormalsListToolBar::UpdateButton()
{

	//*****************************
	CMFCToolBarButton periodEnabled(ID_TABLE_PERIOD_ENABLED, 4);
	periodEnabled.SetStyle((WS_TABSTOP | TBBS_CHECKBOX) & ~WS_GROUP);
	ReplaceButton(ID_TABLE_PERIOD_ENABLED, periodEnabled);

	//*****************************
	CMFCToolBarDateTimeCtrl periodBegin(ID_TABLE_PERIOD_BEGIN, 5, WS_TABSTOP | DTS_SHORTDATECENTURYFORMAT);
	ReplaceButton(ID_TABLE_PERIOD_BEGIN, periodBegin);
	//*****************************
	CMFCToolBarDateTimeCtrl periodEnd(ID_TABLE_PERIOD_END, 6, WS_TABSTOP | DTS_SHORTDATECENTURYFORMAT);
	ReplaceButton(ID_TABLE_PERIOD_END, periodEnd);
	//*****************************
	CMFCToolBarWVariablesButton filterCtrl(ID_TABLE_FILTER, 7, 150);
	ReplaceButton(ID_TABLE_FILTER, filterCtrl);

	//*****************************
	CMFCToolBarComboBoxButton statButton(ID_TABLE_STAT, 8, WS_TABSTOP | CBS_DROPDOWNLIST);
	for (int i = 0; i < NB_STAT_TYPE; i++)
		statButton.AddItem(CString(CStatistic::GetTitle(i)));

	statButton.SelectItem(0, FALSE);
	ReplaceButton(ID_TABLE_STAT, statButton);

	//*****************************
	CMFCToolBarComboBoxButton TMButton(ID_TABLE_TM_TYPE, 9, WS_TABSTOP | CBS_DROPDOWNLIST, 100);
	for (int i = 0; i <= CTM::HOURLY; i++)
		TMButton.AddItem(CString(CTM::GetTypeTitle(i)));

	TMButton.SelectItem(0, FALSE);
	ReplaceButton(ID_TABLE_TM_TYPE, TMButton);

	UpdateTooltips();
	SetWindowPos(NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE | SWP_NOMOVE);

}

void CNormalsListToolBar::AdjustLocations()
{
	ASSERT_VALID(this);

	if (m_Buttons.IsEmpty() || GetSafeHwnd() == NULL)
	{
		return;
	}

	CRect rectClient;
	GetClientRect(rectClient);

	CClientDC dc(this);
	CFont* pOldFont = SelectDefaultFont(&dc);
	ENSURE(pOldFont != NULL);

	int iStartOffset = rectClient.left + 1;
	int iOffset = iStartOffset;

	CSize sizeGrid(GetColumnWidth(), GetRowHeight());
	CSize sizeCustButton(0, 0);

	BOOL bPrevWasSeparator = FALSE;
	int nRowActualWidth = 0;
	for (POSITION pos = m_Buttons.GetHeadPosition(); pos != NULL;)
	{
		POSITION posSave = pos;

		CMFCToolBarButton* pButton = (CMFCToolBarButton*)m_Buttons.GetNext(pos);
		if (pButton == NULL)
		{
			break;
		}
		ASSERT_VALID(pButton);

		BOOL bVisible = TRUE;

		CSize sizeButton = pButton->OnCalculateSize(&dc, sizeGrid, TRUE);

		if (pButton->m_bTextBelow)
		{
			sizeButton.cy = sizeGrid.cy;
		}

		if (pButton->m_nStyle & TBBS_SEPARATOR)
		{
			if (iOffset == iStartOffset || bPrevWasSeparator)
			{
				sizeButton = CSize(0, 0);
				bVisible = FALSE;
			}
			else
			{
				bPrevWasSeparator = TRUE;
			}
		}

		int iOffsetPrev = iOffset;

		CRect rectButton;
		rectButton.left = iOffset;
		rectButton.right = rectButton.left + sizeButton.cx;
		rectButton.top = rectClient.top;
		rectButton.bottom = rectButton.top + sizeButton.cy;

		iOffset += sizeButton.cx;
		nRowActualWidth += sizeButton.cx;

		pButton->Show(bVisible);
		pButton->SetRect(rectButton);

		if (bVisible)
		{
			bPrevWasSeparator = (pButton->m_nStyle & TBBS_SEPARATOR);
		}

		if (pButton->m_nID == ID_TABLE_SENDTO_EXCEL)
		{
			int size = 0;
			//compute total length of the right control
			for (POSITION pos2 = pos; pos2 != NULL;)
			{
				CMFCToolBarButton* pButton2 = (CMFCToolBarButton*)m_Buttons.GetNext(pos2);
				if (pButton2 == NULL)
				{
					break;
				}

				CSize sizeButton2 = pButton2->OnCalculateSize(&dc, sizeGrid, TRUE);
				size += sizeButton2.cx;

			}

			//add delta
			int delta = max(0, rectClient.Width() - iOffset - 1 - size);
			iOffset += delta;
		}

	}


	dc.SelectObject(pOldFont);
	UpdateTooltips();
	RedrawCustomizeButton();
}


//**************************************************************************************************************************************

static const int IDC_NORMALS_MATCH_ID = 1002;


CNormalsMatchWnd::CNormalsMatchWnd()
{
	m_bMustBeUpdated = false;
}

CNormalsMatchWnd::~CNormalsMatchWnd()
{
}


BEGIN_MESSAGE_MAP(CNormalsMatchWnd, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_WINDOWPOSCHANGED()
END_MESSAGE_MAP()



/////////////////////////////////////////////////////////////////////////////
// Gestionnaires de messages de CResourceViewBar

void CNormalsMatchWnd::AdjustLayout()
{
	if (GetSafeHwnd() == NULL || (AfxGetMainWnd() != NULL && AfxGetMainWnd()->IsIconic()))
	{
		return;
	}

	CRect rectClient;
	GetClientRect(rectClient);

	m_wndNormalsList.SetWindowPos(NULL, rectClient.left, rectClient.top, rectClient.Width(), rectClient.Height(), SWP_NOACTIVATE | SWP_NOZORDER);
}

int CNormalsMatchWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	m_wndNormalsList.CreateGrid(WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), this, IDC_NORMALS_MATCH_ID);

	SetPropListFont();
	AdjustLayout();

	return 0;
}

void CNormalsMatchWnd::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);
	AdjustLayout();
}

void CNormalsMatchWnd::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
	CDockablePane::OnSettingChange(uFlags, lpszSection);
	SetPropListFont();
}

void CNormalsMatchWnd::SetPropListFont()
{
	::DeleteObject(m_font.Detach());

	LOGFONT lf;
	afxGlobalData.fontRegular.GetLogFont(&lf);

	NONCLIENTMETRICS info;
	info.cbSize = sizeof(info);

	afxGlobalData.GetNonClientMetrics(info);

	lf.lfHeight = info.lfMenuFont.lfHeight;
	lf.lfWeight = info.lfMenuFont.lfWeight;
	lf.lfItalic = info.lfMenuFont.lfItalic;

	m_font.CreateFontIndirect(&lf);

	m_wndNormalsList.SetFont(&m_font);
}


void CNormalsMatchWnd::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{

	CMatchStationDoc* pDoc = (CMatchStationDoc*)GetDocument();
	ASSERT(pDoc);

	if (lHint == CMatchStationDoc::LANGUAGE_CHANGE)
	{
		AdjustLayout();
	}


	m_wndNormalsList.m_pDB = pDoc->GetNormalsDatabase();
	m_wndNormalsList.m_nearest = pDoc->GetNormalsMatch();

	if (pDoc->GetCurIndex() != UNKNOWN_POS)
		m_wndNormalsList.m_location = pDoc->GetLocation(pDoc->GetCurIndex());


	if (IsWindowVisible())
		m_wndNormalsList.Update();
	else
		m_bMustBeUpdated = true;

}



void CNormalsMatchWnd::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
	CDockablePane::OnWindowPosChanged(lpwndpos);

	if (lpwndpos->flags & SWP_SHOWWINDOW)
	{
		if (m_bMustBeUpdated)
		{
			m_wndNormalsList.Update();
			m_bMustBeUpdated = false;
		}

	}
}


BOOL CNormalsMatchWnd::PreTranslateMessage(MSG* pMsg)
{
	//GetAsyncKeyState(VK_RETURN)
	//GetKeyState
	//if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)
	//{
	//	int index1 = m_wndToolBar.CommandToIndex(ID_STATION_LIST_YEAR);
	//	CMFCToolBarYearsButton* pCtrl1 = (CMFCToolBarYearsButton*)m_wndToolBar.GetButton(index1); ASSERT(pCtrl1);
	//	if (pMsg->hwnd == pCtrl1->GetEditBox()->GetSafeHwnd())
	//	{
	//		// handle return pressed in edit control
	//		OnToolbarCommand(ID_STATION_LIST_YEAR);
	//		return TRUE; // this doesn't need processing anymore
	//	}

	//	int index2 = m_wndToolBar.CommandToIndex(ID_STATION_LIST_FILTER);
	//	CMFCToolBarWVariablesButton* pCtrl2 = (CMFCToolBarWVariablesButton*)m_wndToolBar.GetButton(index2); ASSERT(pCtrl2);
	//	if (pMsg->hwnd == pCtrl2->GetEditBox()->GetSafeHwnd())
	//	{
	//		// handle return pressed in edit control
	//		OnToolbarCommand(ID_STATION_LIST_FILTER);
	//		return TRUE; // this doesn't need processing anymore
	//	}

	//}

	//if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_TAB)
	//{
	//	int index2 = m_wndToolBar.CommandToIndex(ID_STATION_LIST_FILTER);
	//	CMFCToolBarWVariablesButton* pCtrl2 = (CMFCToolBarWVariablesButton*)m_wndToolBar.GetButton(index2); ASSERT(pCtrl2);
	//	if (pMsg->hwnd == pCtrl2->GetEditBox()->GetSafeHwnd())
	//	{
	//		// handle return pressed in edit control
	//		OnToolbarCommand(ID_STATION_LIST_FILTER);
	//		return TRUE; // this doesn't need processing anymore
	//	}
	//
	//	// handle return pressed in edit control
	//	return TRUE; // this doesn't need processing anymore
	//}

	return CDockablePane::PreTranslateMessage(pMsg); // all other cases still need default processing
}


