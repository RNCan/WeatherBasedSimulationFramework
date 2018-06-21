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

#include "basic/ERMsg.h"
#include "Basic/Callback.h"
#include "Basic/UtilStd.h"
#include "UI/Common/UtilWin.h"
#include "UI/Common/SYShowMessage.h"

#include "WeatherBasedSimulationString.h"

namespace UtilWin
{

	static WBSF::CCriticalSection CS;


	void SYShowMessage(ERMsg& message, CWnd* wnd)
	{
		CString messStr = SYGetText(message);

		if (wnd != NULL)
			wnd->MessageBox(messStr, AfxGetAppName(), MB_ICONSTOP | MB_OK);
		else AfxMessageBox(messStr, MB_ICONSTOP | MB_OK);
	}

	CString SYGetText(ERMsg& message)
	{
		CString messStr;

		CS.Enter();
		for (unsigned int i = 0; i < message.dimension(); i++)
		{
			messStr += message[i].data();
			messStr += "\n";
		}
		CS.Leave();

		return messStr;
	}

	ERMsg SYGetMessage(CException& e)
	{
		ERMsg message;

		TCHAR cause[255];
		e.GetErrorMessage(cause, 255);
		std::string tmp = UtilWin::ToUTF8(cause);
		while (!tmp.empty() && (tmp.back() == '\n' || tmp.back() == '\r'))
			tmp = tmp.substr(0, tmp.size() - 1);

		message.ajoute(tmp);

		return message;
	}

	//use _errno() and not errno to call this fonction
	ERMsg SYGetMessage(int* errNo)
	{
		ERMsg message;

		message.asgType(ERMsg::ERREUR);
		message.ajoute(strerror(*errNo));

		return message;
	}
	ERMsg SYGetMessage(DWORD errnum)
	{
		ERMsg message;
		TCHAR cause[255] = { 0 };

		FormatMessage(
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, errnum, 0, cause, 255, NULL);

		std::string tmp = UtilWin::ToUTF8(cause);
		while (!tmp.empty() && (tmp.back() == '\n' || tmp.back() == '\r'))
			tmp = tmp.substr(0, tmp.size() - 1);

		message.ajoute(tmp);

		return message;
	}

	void LoadStringArray(CStringArray& array, UINT nID)
	{
		CString tmp;
		VERIFY(tmp.LoadString(nID));

		array.RemoveAll();

		int pos = 0;
		CString item = tmp.Tokenize(_T("\r\n\t,;|"), pos);
		while (!item.IsEmpty())
		{
			array.Add(item);
			item = tmp.Tokenize(_T("\r\n\t,;|"), pos);
		}
	}

	ERMsg NotEqualMessage(const CString& varName, const CString& arg1, const CString& arg2)
	{
		ERMsg message;

		CString tmp;
		tmp.FormatMessage(IDS_BSC_NOT_EQUAL_MESSAGE, varName, arg2, arg1);
		message.ajoute(UtilWin::ToUTF8(tmp));

		return message;
	}

	ERMsg NotEqualMessage(const CString& varName, ULONGLONG arg1, ULONGLONG arg2)
	{
		return NotEqualMessage(varName, UtilWin::ToCString(int(arg1)), UtilWin::ToCString(int(arg2)));
	}


	template<class T>
	CString GetPString(T a)
	{
		CString str;
		const type_info& info = typeid(a);
		if (info.name() == "int")
		{

		}

		return str;
	}

	ERMsg NotEqualMessage(const CString& varName, double arg1, double arg2)
	{

		return NotEqualMessage(varName, UtilWin::ToCString(arg1), UtilWin::ToCString(arg2));
	}

	ERMsg NotEqualMessage(const CString& varName, int arg1, int arg2)
	{
		return NotEqualMessage(varName, UtilWin::ToCString(int(arg1)), UtilWin::ToCString(int(arg2)));
	}

	ERMsg NotEqualMessage(const CString& varName, float arg1, float arg2)
	{
		GetPString(arg1);
		return NotEqualMessage(varName, UtilWin::ToCString(arg1), UtilWin::ToCString(arg2));
	}

	CString SYGetOutputCString(ERMsg& message, WBSF::CCallback& callBack, bool bAllMessage)
	{
		CString tmp;

		CString strError;
		CString strSucces;
		CString strLastMessage;
		CString strLastComment;

		strError.LoadString(IDS_STR_ERROR);
		strSucces.LoadString(IDS_STR_SUCCESS);
		strLastMessage.LoadString(IDS_STR_LAST_ERROR);
		strLastComment.LoadString(IDS_STR_LAST_COMMENT);

		if (message)
			tmp = strLastMessage + strSucces + _T("\n\n");
		else tmp = strLastMessage + strError + _T(":") + SYGetText(message) + _T("\n\n");
		tmp += strLastComment + _T("\n\n");
		tmp += bAllMessage ? callBack.GetDlgMessageText().data() : callBack.GetCurrentTaskMessageText().data();
		tmp += _T("\n");

		return tmp;
	}

}