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

#include "UI/Common/AppOption.h"
#include "UI/FileManagerPropSheet.h"
#include "WeatherBasedSimulationString.h"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

namespace WBSF
{
	

	IMPLEMENT_DYNAMIC(CDBManagerDlg, CResizablePropertySheet)


	BEGIN_MESSAGE_MAP(CDBManagerDlg, CResizablePropertySheet)
		ON_WM_DESTROY()
		
	END_MESSAGE_MAP()


	CDBManagerDlg::CDBManagerDlg(CWnd* pWndParent, int iSelection, bool bUseClose) :
		m_bUseClose(bUseClose),
		CResizablePropertySheet(IDS_DBEDITOR_CAPTION, pWndParent, iSelection)
	{
		m_psh.dwFlags |= PSH_NOAPPLYNOW;
		m_psh.dwFlags &= ~(PSH_HASHELP);
		
		//Load the icon what we will set later for the program
		m_hIcon = AfxGetApp()->LoadIcon(IDI_LINKED_DATA_EDITOR);

		//Set look and icon
		SetLook(CResizablePropertySheet::PropSheetLook_OutlookBar);
		SetIconsList(IDB_TEST_PNG, 48);
		
		//SetIconsList(IDB_DBEDITOR, 48);

		AddPage(&m_NormalsPage);
		AddPage(&m_dailyPage);
		AddPage(&m_hourlyPage);
		AddPage(&m_gribPage);
		AddPage(&m_mapPage);
		AddPage(&m_modelPage);
		AddPage(&m_weatherUpdatePage);
		AddPage(&m_scriptPage);

	}

	CDBManagerDlg::~CDBManagerDlg()
	{
	}




	/////////////////////////////////////////////////////////////////////////////
	// CDBManagerDlg message handlers

	void CDBManagerDlg::PostNcDestroy()
	{
		CResizablePropertySheet::PostNcDestroy();

		//if (m_bModeless)
			//delete this;
	}


	BOOL CDBManagerDlg::OnInitDialog()
	{
		BOOL bResult = CResizablePropertySheet::OnInitDialog();



		//return bResult;
		if (m_bUseClose)
		{
			CWnd* pOKButton = GetDlgItem(IDOK);
			ASSERT(pOKButton);
			pOKButton->ShowWindow(SW_HIDE);

			CWnd* pCANCELButton = GetDlgItem(IDCANCEL);
			ASSERT(pCANCELButton);
			pCANCELButton->SetWindowText(UtilWin::GetCString(IDS_STR_CLOSE));
			//pOKButton->ShowWindow(SW_SHOW);
		}

		SetIcon(m_hIcon, TRUE); // Set big icon
		SetIcon(m_hIcon, FALSE); // Set small icon


								 //resize window
		CAppOption option;

		CRect rectClient;
		GetWindowRect(rectClient);

		rectClient = option.GetProfileRect(_T("FileManagerDlgRect"), rectClient);
		UtilWin::EnsureRectangleOnDisplay(rectClient);
		//this->MoveWindow(rectClient);
		SetWindowPos(NULL, rectClient.left, rectClient.top, rectClient.Width(), rectClient.Height(), SWP_NOZORDER);
		//ReposButtons(TRUE);

		return bResult;
	}

	void CDBManagerDlg::OnDestroy()
	{

		CRect rectClient;
		GetWindowRect(rectClient);

		CAppOption option;
		option.WriteProfileRect(_T("FileManagerDlgRect"), rectClient);


		CResizablePropertySheet::OnDestroy();
	}

}

