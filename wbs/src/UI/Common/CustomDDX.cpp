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
#include "CustomDDX.h"
#include "WeatherBasedSimulationString.h"

void AFXAPI DDV_StringNotEmpty(CDataExchange* pDX, CString const& value)
{
	if (pDX->m_bSaveAndValidate && value.IsEmpty())
	{
		CString prompt;
		prompt.LoadString( IDS_BSC_NAME_EMPTY );
		AfxMessageBox(prompt, MB_ICONEXCLAMATION, AFX_IDP_PARSE_STRING_SIZE);
		prompt.Empty(); // exception prep
		pDX->Fail();
	}
}

//
//void AFXAPI DDV_TextFromBrowseCtrl(CDataExchange* pDX, int nIDC, CString& value)
//{
//	CBrowseCtrl* wndCtrl = (CBrowseCtrl*)pDX->m_pDlgWnd->GetDlgItem(nIDC);
//	ASSERT( wndCtrl );
//
//	if (pDX->m_bSaveAndValidate)
//	{
//		value = wndCtrl->GetPathName();
//	}
//	else
//	{
//		wndCtrl->SetPathName(value);
//	}
//}

void AFXAPI DDX_Check(CDataExchange* pDX, int nIDC, bool& bValue)
{
	int value = bValue;
	DDX_Check(pDX, nIDC, value);
	bValue=value!=0;
}

void AFXAPI DDX_Text(CDataExchange* pDX, int nIDC, std::string& value)
{
	CString str(value.c_str());
	DDX_Text(pDX, nIDC, str);
	value = CStringA(str);
}

void AFXAPI DDX_Text(CDataExchange* pDX, int nIDC, bool& bValue)
{
	BOOL b = bValue?TRUE:FALSE;
	DDX_Text(pDX, nIDC, b);
	bValue = b?true:false;

}