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
#include <boost/dynamic_bitset.hpp>
#include "Basic/UtilStd.h"
#include "UI/Common/AppOption.h"
#include "UI/Common/SelectionCtrl.h"
#include "UI/Common/UtilWin.h"

#include "WeatherBasedSimulationString.h"
#include "WeatherBasedSimulationUI.h"
 

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif



CSelectionCtrl::CSelectionCtrl(const std::string& in)
{
	SetPossibleValues(in);
}

CSelectionCtrl::~CSelectionCtrl()
{}

BEGIN_MESSAGE_MAP(CSelectionCtrl, CXHtmlTree)
	ON_WM_CREATE()
	ON_NOTIFY_REFLECT(NM_DBLCLK, &CSelectionCtrl::OnNMDblclk)
END_MESSAGE_MAP()


void CSelectionCtrl::PreSubclassWindow()
{
	CXHtmlTree::PreSubclassWindow();

	_AFX_THREAD_STATE* pThreadState = AfxGetThreadState();
	if (pThreadState->m_pWndInit == NULL)
	{
		Init();
	}
}

int CSelectionCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CXHtmlTree::OnCreate(lpCreateStruct) == -1)
		return -1;

	Init();
	return 0;
}

void CSelectionCtrl::SetPossibleValues(const std::string& in)
{
	m_possibleValues = in;
	if( GetSafeHwnd() )
	{
		Init();
	}
}

void CSelectionCtrl::Init()
{
	ASSERT(GetSafeHwnd());

	Initialize(TRUE, TRUE);
	SetSmartCheckBox(TRUE);
	SetSelectFollowsCheck(TRUE);
	SetAutoCheckChildren(TRUE);
	SetHtml(FALSE);
	SetImages(FALSE);


	DeleteAllItems();
	HTREEITEM hItem = InsertItem(CString(WBSF::GetString(IDS_STR_SELECT_UNSELECT).c_str()));
	WBSF::StringVector list(m_possibleValues, ";|");
	for(size_t i=0; i<list.size(); i++)
	{
		InsertItem( CString(list[i].c_str()), hItem);
	}
	Expand(hItem, TVE_EXPAND);

	UpdateCheck(FALSE);
	
	//ASSERT(GetRootItem() != NULL);
	//SelectItem(GetRootItem());
}

//void CSelectionCtrl::SetSelection(const CIntArrayEx& selection)
//void SetSelection(const std::string& selection)
void CSelectionCtrl::UpdateCheck(bool bSaveAndValidate)
{
	if (GetSafeHwnd())
	{
		

		if (bSaveAndValidate)
		{
			ASSERT(GetSafeHwnd());

			
			//m_selection.clear();

			HTREEITEM hItem = GetRootItem();
			boost::dynamic_bitset<size_t> checked(GetChildrenCount(hItem));

			if (hItem)
				hItem = GetChildItem(hItem);
			
			int pos = 0;
			while (hItem)
			{
				//if (GetCheck(hItem))
				//{
				//	if (!m_selection.empty())
				//		m_selection += "|";

				//	m_selection += std::to_string(pos);
				//}

				//m_selection.insert(m_selection.begin(), GetCheck(hItem) ? '1' : '0');
				checked[pos] = GetCheck(hItem)?true:false;
				//;
				pos++;
				hItem = GetNextItem(hItem);
			}

			boost::to_string(checked, m_selection);
		}
		else
		{
			//WBSF::StringVector sel(m_selection, ";|");
			boost::dynamic_bitset<size_t> checked(m_selection);
			HTREEITEM hItem = GetRootItem();
			if (hItem)
				hItem = GetChildItem(hItem);
			
			
			size_t pos = 0;
			while (hItem&&pos<checked.size())
			{
				SetCheck(hItem, checked[pos]);

				pos++;
				hItem = GetNextItem(hItem);
			}
		}
	}
}

void CSelectionCtrl::OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = TRUE;
}

//
//case WM_NOTIFY:
//{
//	LPNMHDR lpnmh = (LPNMHDR)lParam;
//	TVHITTESTINFO ht = { 0 };
//
//	if (lpnmh->code == NM_CLICK) && (lpnmh->idFrom == IDC_MYTREE))
//   {
//	   DWORD dwpos = GetMessagePos();
//
//	   // include <windowsx.h> and <windows.h> header files
//	   ht.pt.x = GET_X_LPARAM(dwpos);
//	   ht.pt.y = GET_Y_LPARAM(dwpos);
//	   MapWindowPoints(HWND_DESKTOP, lpnmh->hwndFrom, &ht.pt, 1);
//
//	   TreeView_HitTest(lpnmh->hwndFrom, &ht);
//
//	   if (TVHT_ONITEMSTATEICON & ht.flags)
//	   {
//
//		   PostMessage(hWnd, UM_CHECKSTATECHANGE, 0, (LPARAM)ht.hItem);
//	   }
//   }
//}
//break;
//
//case UM_CHECKSTATECHANGE:
//{
//	HTREEITEM   hItemChanged = (HTREEITEM)lParam;
//	/*
//	Retrieve the new checked state of the item and handle the notification.
//	*/
//}
//break;
//

void AFXAPI DDX_Selection(CDataExchange* pDX, int ID, std::string& selection)
{
	CSelectionCtrl* pCtrl = dynamic_cast<CSelectionCtrl*>(pDX->m_pDlgWnd->GetDlgItem(ID));
	ASSERT( pCtrl );

	if( pDX->m_bSaveAndValidate ) 
	{
		//pCtrl->UpdateCheck(TRUE);
		selection = pCtrl->GetSelection();

	}
	else 
	{
		pCtrl->SetSelection( selection );
		//pCtrl->UpdateCheck(FALSE);
	}
}

//****************************************************************************************************************

BEGIN_MESSAGE_MAP(CSelectionDlg, CDialog)
	ON_WM_SIZE()
	ON_WM_GETMINMAXINFO()
	ON_WM_DESTROY()
END_MESSAGE_MAP()


CSelectionDlg::CSelectionDlg(CWnd* pParent) :
CDialog(IDD_CMN_SELECTION, pParent)
{
}

CSelectionDlg::~CSelectionDlg()
{
}


BOOL CSelectionDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	
	


	CAppOption option(_T("WindowsPosition"));
	CRect rect = option.GetProfileRect(_T("SelectionDlg"), CRect());
	if (rect.IsRectEmpty())
		GetWindowRect(rect);

	UtilWin::EnsureRectangleOnDisplay(rect);
	MoveWindow(rect);
	//SetWindowPos(NULL, rect.left, rect.top, rect.Width(), rect.Height(), SWP_NOZORDER);

	return TRUE;
}

void CSelectionDlg::OnDestroy()
{
	CRect rect;
	GetWindowRect(rect);

	CAppOption option(_T("WindowsPosition"));
	//CPoint pt = rect.TopLeft();
	option.WriteProfileRect(_T("SelectionDlg"), rect);

	CDialog::OnDestroy();
}

void CSelectionDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	
	DDX_Control(pDX, IDC_CMN_SELECTION, m_selectionCtrl);


	if (pDX->m_bSaveAndValidate)
	{
		m_selection = m_selectionCtrl.GetSelection();
	}
	else
	{
		m_selectionCtrl.SetPossibleValues(m_possibleValues);
		m_selectionCtrl.SetSelection(m_selection);
	}
}

void CSelectionDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	
	if (m_selectionCtrl.GetSafeHwnd() != NULL)
	{
		static const int MARGE = 10;


		CRect rect;
		GetClientRect(rect);


		CRect rectCancel;
		GetDlgItem(IDCANCEL)->GetWindowRect(rectCancel); ScreenToClient(rectCancel);
		rectCancel.top = rect.bottom - MARGE - rectCancel.Height();
		rectCancel.bottom = rect.bottom - MARGE;
		rectCancel.left = rect.right - MARGE - rectCancel.Width();
		rectCancel.right = rect.right - MARGE;

		CRect rectOK;
		GetDlgItem(IDOK)->GetWindowRect(rectOK); ScreenToClient(rectOK);
		rectOK.top = rect.bottom - MARGE - rectOK.Height();
		rectOK.bottom = rect.bottom - MARGE;
		rectOK.left = rect.right - MARGE - rectCancel.Width() - MARGE - rectOK.Width();
		rectOK.right = rect.right - MARGE - rectCancel.Width() - MARGE;
		
		
		m_selectionCtrl.SetWindowPos(NULL, 0, 0, rect.Width()-2*MARGE, rect.Height() - rectOK.Height()-3*MARGE, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE);
		GetDlgItem(IDOK)->SetWindowPos(NULL, rectOK.left, rectOK.top, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE);
		GetDlgItem(IDCANCEL)->SetWindowPos(NULL, rectCancel.left, rectCancel.top, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE);
	}
}


void CSelectionDlg::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI)
{
	CDialog::OnGetMinMaxInfo(lpMMI);
	lpMMI->ptMinTrackSize.x = 300;
	lpMMI->ptMinTrackSize.y = 400;
}
