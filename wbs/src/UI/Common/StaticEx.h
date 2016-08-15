//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once

#include <afxtempl.h>

class CStaticEx : public CStatic
{
// Construction
public:
	CStaticEx();
	virtual ~CStaticEx();
	
	BOOL EnableWindow(BOOL bEnable = TRUE, BOOL bRecurseChildren = FALSE);
	
	DECLARE_MESSAGE_MAP()
	
	int EnableChildWindows(HWND hWnd, BOOL bEnable, BOOL bEnableParent);
	
};


