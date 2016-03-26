//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once

#include <string>

void AFXAPI DDV_StringNotEmpty(CDataExchange* pDX, CString const& value);
void AFXAPI DDV_TextFromBrowseCtrl(CDataExchange* pDX, int nIDC, CString& value);
void AFXAPI DDX_Check(CDataExchange* pDX, int nIDC, bool& bValue);
void AFXAPI DDX_Text(CDataExchange* pDX, int nIDC, std::string& value);

