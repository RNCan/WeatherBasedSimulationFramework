//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once

#include "UtilWin.h"

class COpenDirEditCtrl : public CMFCEditBrowseCtrl
{
public:

	using CMFCEditBrowseCtrl::GetWindowText;
	std::string GetWindowText()const
	{
		CString tmp;
		CMFCEditBrowseCtrl::GetWindowText(tmp);
		
		return UtilWin::ToUTF8(tmp);
	}

	using CMFCEditBrowseCtrl::SetWindowText;
	void SetWindowText(const std::string& str)
	{
		CMFCEditBrowseCtrl::SetWindowText(UtilWin::Convert(str));
	}
	
	virtual void OnBrowse()
	{
		CString filePath;
		GetWindowText(filePath);
		CString command = _T("Explorer.exe \"") + UtilWin::GetPath(filePath) + _T("\"");
		WinExec( UtilWin::ToUTF8(command).c_str(), SW_SHOW);
	}
};

//override DDX_Control to provide initialisation
void AFXAPI DDX_Control(CDataExchange* pDX, int nIDC, COpenDirEditCtrl& rControl);
