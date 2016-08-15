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
#include "Basic/Registry.h"
#include "UI/Common/UtilWin.h"
#include "UI/Common/BioSIMListBox.h"

#include "WeatherBasedSimulationUI.h"
#include "WeatherBasedSimulationString.h"

using namespace UtilWin;



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace WBSF
{



	//***********************************************************************
	//CBioSIMListBox

	static UINT WM_CMN_TOOLS_OPTIONS = ::RegisterWindowMessage(_T("WM_CMN_TOOLS_OPTIONS"));


	BEGIN_MESSAGE_MAP(CBioSIMListBox, CVSListBoxMS)
		ON_NOTIFY(NM_DBLCLK, nListId, &OnDblclkList)
	END_MESSAGE_MAP()

	CBioSIMListBox::CBioSIMListBox(bool bAddDefault, int fileInfoType)
	{
		m_bAddDefault = bAddDefault;
		m_bCopyItem = FALSE;
		m_lastCopyItem = -1;
	}

	void CBioSIMListBox::OnClickButton(int iButton)
	{
		UINT uiBtnID = GetButtonID(iButton);
		int iSelItem = GetSelItem();

		switch (uiBtnID)
		{
		case AFX_VSLISTBOX_BTN_EDIT_ID: if (iSelItem >= 0) { OnEditItem(iSelItem); }break;
		case AFX_VSLISTBOX_BTN_COPY_ID: if (iSelItem >= 0) { UnselectAll(); CopyItem(iSelItem); } break;
		case AFX_VSLISTBOX_BTN_SHOWMAP_ID: if (iSelItem >= 0) { OnShowMap(iSelItem); }break;
		case AFX_VSLISTBOX_BTN_SHOW_INFO_ID: if (iSelItem >= 0) { OnShowInfo(iSelItem); }break;
		case AFX_VSLISTBOX_BTN_EXCEL_ID: if (iSelItem >= 0) { OnExcel(iSelItem); }break;
		case AFX_VSLISTBOX_BTN_SET_DEFAULT_ID: if (iSelItem >= 0) { OnSetAsDefault(iSelItem); } break;
		case AFX_VSLISTBOX_BTN_LINK_ID: OnLink(); break;
		case AFX_VSLISTBOX_BTN_OPTION_ID: OnOption(); break;
		default: CVSListBoxMS::OnClickButton(iButton); break;
		}

		if (uiBtnID == AFX_VSLISTBOX_BTN_COPY_ID ||
			uiBtnID == AFX_VSLISTBOX_BTN_NEW_ID ||
			uiBtnID == AFX_VSLISTBOX_BTN_DELETE_ID)
		{
			Invalidate(true);
		}

	}


	//void CBioSIMListBox::OnUpdateTitle(NMHDR* /*pNMHDR*/, LRESULT* pResult)
	//{
	//	UpdateTitle();
	//}

	void CBioSIMListBox::OnDblclkList(NMHDR* /*pNMHDR*/, LRESULT* pResult)
	{
		*pResult = 0;

		int iSelItem = GetSelItem();

		if ((m_uiStandardBtns & AFX_VSLISTBOX_BTN_NEW) && iSelItem == -1)
		{
			CreateNewItem();
		}
		else if (iSelItem >= 0)
		{
			if (!m_bAddDefault || iSelItem != 0)
				OnEditItem(iSelItem);
		}
	}

	int CBioSIMListBox::GetStdButtonNum(UINT uiStdBtn) const
	{
		switch (uiStdBtn)
		{
		case AFX_VSLISTBOX_BTN_EDIT:
			return GetButtonNum(AFX_VSLISTBOX_BTN_EDIT_ID);

		case AFX_VSLISTBOX_BTN_COPY:
			return GetButtonNum(AFX_VSLISTBOX_BTN_COPY_ID);

		case AFX_VSLISTBOX_BTN_SHOWMAP:
			return GetButtonNum(AFX_VSLISTBOX_BTN_SHOWMAP_ID);

		case AFX_VSLISTBOX_BTN_SHOW_INFO:
			return GetButtonNum(AFX_VSLISTBOX_BTN_SHOW_INFO_ID);

		case AFX_VSLISTBOX_BTN_EXCEL:
			return GetButtonNum(AFX_VSLISTBOX_BTN_EXCEL_ID);

		case AFX_VSLISTBOX_BTN_SET_DEFAULT:
			return GetButtonNum(AFX_VSLISTBOX_BTN_SET_DEFAULT_ID);

		case AFX_VSLISTBOX_BTN_LINK:
			return GetButtonNum(AFX_VSLISTBOX_BTN_LINK_ID);

		case AFX_VSLISTBOX_BTN_OPTION:
			return GetButtonNum(AFX_VSLISTBOX_BTN_OPTION_ID);

		}

		return CVSListBoxBase::GetStdButtonNum(uiStdBtn);
	}

	//CString GetEllipsisString(CString str, int width)
	//{
	//	int pos = int((width-3)/2);
	//	return str.Left(pos) + "..." + str.Right(pos + (width-3) mod 2);
	//}

	BOOL CBioSIMListBox::OnBeforeRemoveItem(int iItem)
	{
		ASSERT(iItem >= 0);

		BOOL bRep = FALSE;
		if (!m_bAddDefault || iItem != 0)
		{
			CString confirm;
			confirm.FormatMessage(IDS_BSC_CONFIRM_DELETE, GetItemText(iItem));
			bRep = MessageBox(confirm, AfxGetAppName(), MB_OKCANCEL) == IDOK;


			if (bRep)
				bRep = GetManager().DeleteFile(ToUTF8(GetItemText(iItem))) != 0;
		}

		return bRep;
	}

	BOOL CBioSIMListBox::OnBeforeRenameItem(int iItem, CString newName)
	{
		ASSERT(iItem >= 0);
		ASSERT(!m_bAddDefault || iItem != 0);

		if (!m_bAddDefault || iItem != 0)
		{
			return GetManager().RenameFile(ToUTF8(GetItemText(iItem)), ToUTF8(newName)) != 0;
		}

		return FALSE;
	}

	BOOL CBioSIMListBox::OnBeforeCopyItem(int iItem, CString newName)
	{
		ASSERT(iItem >= 0);
		if (!m_bAddDefault || iItem != 0)
		{
			return GetManager().CopyFile(ToUTF8(GetItemText(iItem)), ToUTF8(newName)) != 0;
		}

		return FALSE;
	}

	void CBioSIMListBox::OnEditItem(int iItem)
	{
		ASSERT(iItem >= 0);

		if (!m_bAddDefault || iItem != 0)
		{
			std::string filePath = GetManager().GetFilePath(ToUTF8(GetItemText(iItem)));
			WBSF::CallApplication(WBSF::CRegistry::TEXT_EDITOR, filePath, GetSafeHwnd(), SW_SHOW, true, false);
		}
	}



	void CBioSIMListBox::OnShowMap(int iItem)
	{
		ASSERT(iItem >= 0);
		ASSERT(!m_bAddDefault || iItem != 0);

		if (!m_bAddDefault || iItem != 0)
		{
			std::string filePath = GetManager().GetFilePath(ToUTF8(GetItemText(iItem)));
			WBSF::CallApplication(WBSF::CRegistry::SHOWMAP, filePath, GetSafeHwnd(), SW_SHOW);
		}
	}

	void CBioSIMListBox::OnExcel(int iItem)
	{
		ASSERT(iItem >= 0);
		ASSERT(!m_bAddDefault || iItem != 0);

		if (!m_bAddDefault || iItem != 0)
		{
			std::string filePath = GetManager().GetFilePath(ToUTF8(GetItemText(iItem)));
			WBSF::CallApplication(WBSF::CRegistry::SPREADSHEET1, filePath, GetSafeHwnd(), SW_SHOW);
		}
	}



	void CBioSIMListBox::OnInitList()
	{
		DeleteAllItems();

		if (m_pWndList->GetSafeHwnd())
		{
			WBSF::StringVector list = GetManager().GetFilesList();
			for (int i = 0; i < (int)list.size(); i++)
			{
				m_pWndList->InsertItem(i, Convert(list[i]));
			}

			if (m_bAddDefault)
			{
				m_pWndList->InsertItem(0, UtilWin::GetCString(IDS_STR_DEFAULT));
			}

			UpdateTitle();
		}
	}

	void CBioSIMListBox::UpdateTitle()
	{
		m_bDefaultCaption = false;
		int nbItem = m_pWndList->GetItemCount();
		if (m_bAddDefault)
			nbItem--;

		ASSERT(nbItem >= 0);
		m_strCaption = UtilWin::ToCString(nbItem);

		Invalidate();
		UpdateWindow();
	}

	BOOL CBioSIMListBox::NameExist(CString newName)
	{
		BOOL bRep = FALSE;
		for (int i = 0; i < GetCount(); i++)
		{
			if (GetItemText(i).CompareNoCase(newName) == 0)
			{
				bRep = TRUE;
				break;
			}
		}
		return bRep;
	}

	CString CBioSIMListBox::GetNewName(CString name)
	{
		while (NameExist(name))
		{
			int no = GetEndNumber(name);
			if (no == 0)
			{
				name += _T("2");//ToString(no);
			}
			else
			{
				name.Replace(UtilWin::ToCString(no), UtilWin::ToCString(no + 1));
			}
		}

		return name;
	}

	void CBioSIMListBox::CopyItem(int iItem)
	{
		ASSERT(iItem >= 0);

		if (!m_bAddDefault || iItem != 0)
		{
			CString name = GetItemText(iItem);
			CString newName = GetNewName(name);


			m_bCopyItem = TRUE;
			m_lastCopyItem = iItem;


			int iLastItem = AddItem(name);
			ASSERT(iLastItem >= 0);

			//if th eitem is insert before the copied item, we have to take the next one
			if (iLastItem <= m_lastCopyItem)
				m_lastCopyItem++;


			EditItem(iLastItem);

			m_wndEdit.SetWindowText(newName);
		}
	}

	//void CBioSIMListBox::OnItemChanged(NMHDR* pNMHDR, LRESULT* pResult)
	//{
	//	ENSURE(pNMHDR != NULL);
	//
	//	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	//	ENSURE(pNMListView != NULL);
	//
	//	if (pNMListView->uChanged == LVIF_STATE && (pNMListView->uOldState & LVIS_SELECTED) != (pNMListView->uNewState & LVIS_SELECTED))
	//	{
	//		if( !m_bCopyItem )
	//			OnSelectionChanged();
	//	}
	//
	//	*pResult = 0;
	//}

	void CBioSIMListBox::OnEndEditLabel(LPCTSTR lpszLabel)
	{
		int iSelItem = GetSelItem();
		//ASSERT(!m_bAddDefault || iItem != 0 );

		if (iSelItem < 0 || (m_bAddDefault && iSelItem == 0))
		{
			ASSERT(FALSE);
			return;
		}

		CString strLabel = (lpszLabel != NULL) ? lpszLabel : _T("");

		if (!strLabel.IsEmpty() && !NameExist(strLabel))
		{


			if (m_bNewItem)
			{
				SetItemText(iSelItem, strLabel);
				OnAfterAddItem(iSelItem);
			}
			else if (m_bCopyItem)
			{
				SetItemText(iSelItem, strLabel);
				if (!OnBeforeCopyItem(m_lastCopyItem, strLabel))
				{
					RemoveItem(iSelItem);
				}

				m_bCopyItem = FALSE;
				m_lastCopyItem = -1;
			}
			else
			{
				if (OnBeforeRenameItem(iSelItem, strLabel))
				{
					SetItemText(iSelItem, strLabel);
					OnAfterRenameItem(iSelItem);
				}
			}

			//notify parent of the chnage of a item
			NMHDR nm;
			nm.hwndFrom = m_hWnd;
			nm.idFrom = GetDlgCtrlID();
			nm.code = ON_BLB_NAMECHANGE;

			GetParent()->SendMessage(WM_NOTIFY, 0, (LPARAM)&nm);
		}
		else
		{
			if (m_bNewItem || m_bCopyItem)
			{
				RemoveItem(iSelItem);
			}

			if (!strLabel.IsEmpty())
			{
				CString error;
				error.FormatMessage(IDS_BSC_NAME_EXIST, strLabel);
				MessageBox(error);
			}

		}

		m_bNewItem = FALSE;
	}


	BOOL CBioSIMListBox::SetStandardButtons(UINT uiBtns)
	{
		if (!CVSListBoxMS::SetStandardButtons(uiBtns))
			return FALSE;
		 

		CString strButton;
		if (uiBtns & AFX_VSLISTBOX_BTN_EDIT)
		{
			ENSURE(strButton.LoadString(IDS_CMN_AFXBARRES_EDIT));
			ENSURE(AddButton(IDB_CMN_EDIT32, strButton, VK_RETURN, 0, AFX_VSLISTBOX_BTN_EDIT_ID));
		}

		if (uiBtns & AFX_VSLISTBOX_BTN_COPY)
		{
			ENSURE(strButton.LoadString(IDS_CMN_AFXBARRES_COPY));
			ENSURE(AddButton(IDB_CMN_COPY32, strButton, VK_INSERT, FSHIFT, AFX_VSLISTBOX_BTN_COPY_ID));
		}


		if (uiBtns & AFX_VSLISTBOX_BTN_SHOWMAP)
		{
			ENSURE(strButton.LoadString(IDS_CMN_AFXBARRES_SHOWMAP));
			ENSURE(AddButton(IDB_CMN_SHOWMAP32, strButton, 0, 0, AFX_VSLISTBOX_BTN_SHOWMAP_ID));
		}

		if (uiBtns & AFX_VSLISTBOX_BTN_SHOW_INFO)
		{
			ENSURE(strButton.LoadString(IDS_CMN_AFXBARRES_SHOW_INFO));
			ENSURE(AddButton(IDB_CMN_INFO32, strButton, 0, 0, AFX_VSLISTBOX_BTN_SHOW_INFO_ID));
		}

		if (uiBtns & AFX_VSLISTBOX_BTN_EXCEL)
		{
			ENSURE(strButton.LoadString(IDS_CMN_AFXBARRES_EXCEL));
			ENSURE(AddButton(IDB_CMN_EXCEL32, strButton, 0, 0, AFX_VSLISTBOX_BTN_EXCEL_ID));
		}

		if (uiBtns & AFX_VSLISTBOX_BTN_SET_DEFAULT)
		{
			ENSURE(strButton.LoadString(IDS_CMN_AFXBARRES_SET_DEFAULT));
			ENSURE(AddButton(IDB_CMN_SET_DEFAULT, strButton, 0, 0, AFX_VSLISTBOX_BTN_SET_DEFAULT_ID));
		}

		if (uiBtns & AFX_VSLISTBOX_BTN_LINK)
		{ 
			ENSURE(strButton.LoadString(IDS_CMN_AFXBARRES_LINK));
			ENSURE(AddButton(IDB_CMN_LINK32, strButton, 0, 0, AFX_VSLISTBOX_BTN_LINK_ID));
		}

		if (uiBtns & AFX_VSLISTBOX_BTN_OPTION)
		{
			ENSURE(strButton.LoadString(IDS_CMN_AFXBARRES_OPTION));
			ENSURE(AddButton(IDB_CMN_OPTION32, strButton, 0, 0, AFX_VSLISTBOX_BTN_OPTION_ID));
		}


		m_uiStandardBtns |= uiBtns;

		return TRUE;
	}

	void CBioSIMListBox::OnSelectionChanged()
	{

		CVSListBoxMS::OnSelectionChanged();
		UpdateTitle();


		NMHDR nm;
		nm.hwndFrom = m_hWnd;
		nm.idFrom = GetDlgCtrlID();
		nm.code = ON_BLB_SELCHANGE;

		GetParent()->SendMessage(WM_NOTIFY, 0, (LPARAM)&nm);
	}

	//by default CBioSIMListBox Do an OnEdit when DblClick
	BOOL CBioSIMListBox::PreTranslateMessage(MSG* pMsg)
	{
		if (m_pWndList != NULL)
		{
			if (pMsg->message == WM_LBUTTONDOWN && m_pWndList->GetEditControl() == NULL && m_pWndList->GetSafeHwnd() == CWnd::GetFocus()->GetSafeHwnd())
			{
				ASSERT_VALID(m_pWndList);

				m_ptClick = CPoint(-1, -1);

				CPoint ptClick = pMsg->pt;
				m_pWndList->ScreenToClient(&ptClick);

				UINT uFlags;
				int iItem = m_pWndList->HitTest(ptClick, &uFlags);
				if (iItem >= 0 && (uFlags & LVHT_ONITEMLABEL))
				{
					UINT uiMask = LVIS_FOCUSED | LVIS_SELECTED;
					if ((m_pWndList->GetItemState(iItem, uiMask) & uiMask) == uiMask)
					{
						// Secondary click on selected item:
						m_ptClick = ptClick;

						SetCapture();
						return TRUE;
					}
				}
			}
			else if (pMsg->message == WM_LBUTTONUP && m_ptClick != CPoint(-1, -1))
			{
				ASSERT_VALID(m_pWndList);

				ReleaseCapture();

				CPoint ptClick = pMsg->pt;
				m_pWndList->ScreenToClient(&ptClick);

				int iItem = m_pWndList->HitTest(ptClick);

				BOOL bEditItem = iItem >= 0 && (abs(ptClick.x - m_ptClick.x) < ::GetSystemMetrics(SM_CXDRAG) && abs(ptClick.y - m_ptClick.y) < ::GetSystemMetrics(SM_CYDRAG));

				m_ptClick = CPoint(-1, -1);

				//if (bEditItem)
				//{
				//	EditItem(iItem);
				//}

				return TRUE;
			}
		}

		return CVSListBoxBase::PreTranslateMessage(pMsg);
	}


	void CBioSIMListBox::OnKey(WORD wKey, BYTE fFlags)
	{
		CVSListBoxMS::OnKey(wKey, fFlags);

		int iSelItem = GetSelItem();
		TCHAR cKey = (TCHAR)LOWORD(::MapVirtualKey(wKey, 2));

		if (fFlags == 0 && wKey == VK_F5)
		{
			OnInitList();
		}
	}

	bool CBioSIMListBox::CanEditLabel(int iItem)const
	{
		return !m_bAddDefault || iItem > 0;
	}

	//void CBioSIMListBox::OnPaint()
	//{
	//	//UpdateTitle();
	//
	//	CVSListBoxMS::OnPaint();
	//}


	void CBioSIMListBox::OnOption()
	{
		//AfxGetMainWnd()->SendMessage(WM_COMMAND, MAKEWPARAM(ID_CMN_TOOLS_OPTIONS, BN_CLICKED), (LPARAM)m_hWnd );
		AfxGetMainWnd()->SendMessage(WM_CMN_TOOLS_OPTIONS, 1, 0);
		OnInitList();
		//((CDBEditorPropSheet*)GetParent())->OnFileManagerChange();
	}

	CString CBioSIMListBox::GetWindowText()const
	{
		CString tmp = CVSListBoxMS::GetWindowText();

		//override default string 
		if (m_bAddDefault && GetSelItem() == 0)
			tmp = STRDEFAULT;


		return tmp;
	}

	bool CBioSIMListBox::SelectString(const CString& txt)
	{
		bool bRep = true;
		//override default string 
		if (m_bAddDefault && (txt.IsEmpty() || txt == STRDEFAULT))
			SelectItem(0);
		else
			bRep = CVSListBoxMS::SelectString(txt);


		return bRep;
	}

	CString CBioSIMListBox::GetItemText(int iIndex) const
	{
		CString tmp = CVSListBoxMS::GetItemText(iIndex);

		//override default string 
		if (m_bAddDefault && iIndex == 0)
			tmp = STRDEFAULT;


		return tmp;
	}

}