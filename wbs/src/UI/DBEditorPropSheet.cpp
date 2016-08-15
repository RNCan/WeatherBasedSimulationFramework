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
#include "UI/DBEditorPropSheet.h"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

namespace WBSF
{
	

	IMPLEMENT_DYNAMIC(CDBManagerDlg, CResizablePropertySheet)


	BEGIN_MESSAGE_MAP(CDBManagerDlg, CResizablePropertySheet)
		ON_WM_SIZE()
	END_MESSAGE_MAP()


	CDBManagerDlg::CDBManagerDlg(CWnd* pWndParent, int iSelection) :
		CResizablePropertySheet(IDS_DBEDITOR_CAPTION, pWndParent, iSelection)
	{
		m_psh.dwFlags |= PSH_NOAPPLYNOW;
//		m_psh.dwFlags |= PSH_RESIZABLE;
	//	m_psh.dwFlags |= PSH_MODELESS;
		m_psh.dwFlags &= ~(PSH_HASHELP);
		
		//m_rCrt.SetRectEmpty();
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
		CWnd* pOKButton = GetDlgItem(IDOK);
		ASSERT(pOKButton);
		pOKButton->ShowWindow(SW_HIDE);

		CWnd* pCANCELButton = GetDlgItem(IDCANCEL);
		ASSERT(pCANCELButton);
		pCANCELButton->SetWindowText(_T("Close"));
		//pCANCELButton->ShowWindow(SW_HIDE);

		//// Set Flags for property sheet  
		////m_bModeless = FALSE;
		////m_nFlags |= WF_CONTINUEMODAL;
		//BOOL bResult = CResizablePropertySheet::OnInitDialog();
		////m_bModeless = TRUE;
		////m_nFlags &= ~WF_CONTINUEMODAL;

		//UpdateData(FALSE);
		// Set the icon for this dialog.  The framework does this automatically
		//  when the application's main window is not a dialog
		SetIcon(m_hIcon, TRUE); // Set big icon
		SetIcon(m_hIcon, FALSE); // Set small icon

		return bResult;
	}
}

