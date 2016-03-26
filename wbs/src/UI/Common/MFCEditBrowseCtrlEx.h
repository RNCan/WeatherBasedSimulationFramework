//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once

#include "afxEditBrowseCtrl.h"
#include "UI/Common/UtilWin.h"


/////////////////////////////////////////////////////////////////////////////
// CMFCEditBrowseCtrlEx window

class CMFCEditBrowseCtrlEx : public CMFCEditBrowseCtrl 
{
// Construction
public:
	CMFCEditBrowseCtrlEx();

// Attributes
protected:
	int  m_nFilterIndex;
	CString m_defaultEntry;
 
// Operations
public:

	
	void EnableFileBrowseButton(LPCTSTR lpszDefExt, LPCTSTR lpszFilter, LPCTSTR defaultEntry, int nDefaultFilter=0);
	void EnableFileBrowseButton(LPCTSTR lpszDefExt = NULL, LPCTSTR lpszFilter = NULL, int nFilterIndex=0);
	void EnableFolderBrowseButton();

	using CMFCEditBrowseCtrl::GetWindowText;
	CString GetWindowText()const {CString tmp; GetWindowText(tmp); return tmp; }
	
	std::string GetString()const
	{
		CStringW tmp;
		CEdit::GetWindowTextW(tmp);
		return UtilWin::ToUTF8(tmp);
	}

	using CEdit::SetWindowText;
	void SetWindowText(const std::string& tmp)
	{
		CEdit::SetWindowTextW(UtilWin::ToUTF16(tmp));
	}
	void SetString(const std::string& tmp)
	{
		CEdit::SetWindowTextW(UtilWin::ToUTF16(tmp));
	}

// Overrides
public:

	virtual void OnBrowse();

// Implementation
public:
	virtual ~CMFCEditBrowseCtrlEx();

	DECLARE_MESSAGE_MAP()


};

