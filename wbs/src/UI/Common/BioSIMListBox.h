//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once

#include "UI/Common/PropertiesListBox.h"
#include "FileManager/DirectoryManager.h"


namespace WBSF
{

	//***********************************************************************
	//CBioSIMListBox

#define AFX_VSLISTBOX_BTN_EDIT		0x0010
#define AFX_VSLISTBOX_BTN_COPY		0x0020
#define AFX_VSLISTBOX_BTN_SHOW_INFO	0x0040
#define AFX_VSLISTBOX_BTN_SHOWMAP	0x0080
#define AFX_VSLISTBOX_BTN_EXCEL	0x00100
#define AFX_VSLISTBOX_BTN_SET_DEFAULT	0x0200
#define AFX_VSLISTBOX_BTN_LINK	0x0400
#define AFX_VSLISTBOX_BTN_OPTION	0x0800

#define AFX_VSLISTBOX_BTN_EDIT_ID		(UINT)(-15)
#define AFX_VSLISTBOX_BTN_COPY_ID		(UINT)(-16)
#define AFX_VSLISTBOX_BTN_SHOW_INFO_ID	(UINT)(-17)
#define AFX_VSLISTBOX_BTN_SHOWMAP_ID	(UINT)(-18)
#define AFX_VSLISTBOX_BTN_EXCEL_ID	(UINT)(-19)
#define AFX_VSLISTBOX_BTN_SET_DEFAULT_ID	(UINT)(-20)
#define AFX_VSLISTBOX_BTN_LINK_ID	(UINT)(-21)
#define AFX_VSLISTBOX_BTN_OPTION_ID	(UINT)(-22)


#define ON_BLB_SELCHANGE	WM_USER + 0x0001
#define ON_BLB_NAMECHANGE	WM_USER + 0x0002


	class CBioSIMListBox : public CVSListBoxMS
	{
	public:

		CBioSIMListBox(bool bAddDefault = false, int fileInfoType = WBSF::FILE_TITLE);

		BOOL SetStandardButtons(UINT uiBtns);
		int GetStdButtonNum(UINT uiStdBtn) const;

		//CDirectoryManagerPtr m_pManager;
		virtual const WBSF::CDirectoryManager& GetManager() = 0;
		virtual CString GetItemText(int iIndex) const;

		CString GetWindowText()const;
		bool SelectString(const CString& txt);

		bool IsDefault()const { return m_bAddDefault && GetSelectedCount()==1 && GetSelItem() == 0; }


	protected:

		virtual void OnInitList();
		virtual void OnKey(WORD wKey, BYTE fFlags);
		virtual void OnEndEditLabel(LPCTSTR lpszLabel);
		virtual void OnClickButton(int iButton);
		virtual BOOL PreTranslateMessage(MSG* pMsg);
		virtual bool CanEditLabel(int iItem)const;
		virtual void CopyItem(int iItem);
		virtual void OnSelectionChanged();
		virtual BOOL OnBeforeRemoveItem(int iItem);
		virtual BOOL OnBeforeRenameItem(int iItem, CString newName);
		virtual BOOL OnBeforeCopyItem(int iItem, CString newName);

		virtual void OnEditItem(int iItem);
		virtual void OnAfterEditItem(){}
		virtual void OnShowMap(int iItem);
		virtual void OnExcel(int iItem);
		virtual void OnAfterShowMap(){}
		virtual void OnSetAsDefault(int iItem){}
		virtual void OnAfterSetAsDefault(){}
		virtual void OnLink(){}
		virtual void OnShowInfo(int iItem){}
		virtual void OnOption();


		BOOL NameExist(CString newName);
		CString GetNewName(CString name);
		virtual void UpdateTitle();


		bool m_bCopyItem;
		int m_lastCopyItem;


		DECLARE_MESSAGE_MAP()
		afx_msg void OnDblclkList(NMHDR* /*pNMHDR*/, LRESULT* pResult);

		bool m_bAddDefault;


	};

}