//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once

#include "basic/ERMsg.h"
#include "Basic/Callback.h"

class CWnd;
class CException;

namespace UtilWin
{
	void SYShowMessage(ERMsg& message, CWnd* wnd);

	ERMsg SYGetMessage(CException& e);
	ERMsg SYGetMessage(int* errNo);
	ERMsg SYGetMessage(DWORD errnum);
	CString SYGetText(ERMsg& message);
	CString SYGetOutputCString(ERMsg& mesage, WBSF::CCallback& callBack = DEFAULT_CALLBACK, bool bAllMessage = true);

	void LoadStringArray(CStringArray& array, UINT nID);
	ERMsg NotEqualMessage(const CString& varName, const CString& arg1, const CString& arg2);
	ERMsg NotEqualMessage(const CString& varName, ULONGLONG arg1, ULONGLONG arg2);
	ERMsg NotEqualMessage(const CString& varName, int arg1, int arg2);
	ERMsg NotEqualMessage(const CString& varName, double arg1, double arg2);
	ERMsg NotEqualMessage(const CString& varName, float arg1, float arg2);
}