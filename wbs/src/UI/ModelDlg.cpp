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

#include "ModelBase/Model.h"
#include "UI/Common/UtilWin.h"


#include "ModelDlg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



namespace WBSF
{
	 
	//using namespace UtilWin;
	/////////////////////////////////////////////////////////////////////////////
	// CModelDlg

	BEGIN_MESSAGE_MAP(CModelDlg, CMFCPropertySheet)
	END_MESSAGE_MAP()


	CModelDlg::CModelDlg(CWnd* pParentWnd, UINT iSelectPage) :
		CMFCPropertySheet(_T("Model Editor"), pParentWnd, iSelectPage),
		m_generalPage(m_model),
		m_WGInputPage(m_model),
		m_SSIPage(m_model),
		m_inputPage(m_model),
		m_outputPage(m_model),
		m_creditPage(m_model)
	{
		m_psh.dwFlags |= PSH_NOAPPLYNOW;
		m_psh.dwFlags &= ~(PSH_HASHELP);

		//Load the icon what we will set later for the program
		m_hIcon = AfxGetApp()->LoadIcon(IDI_DBEDIT_MODEL);

		//Set look and icon
		SetLook(CMFCPropertySheet::PropSheetLook_OutlookBar);
		SetIconsList(IDB_UI_MODEL_ICONS, 48); 


		AddPage(&m_generalPage);
		AddPage(&m_WGInputPage);
		AddPage(&m_SSIPage);
		AddPage(&m_inputPage);
		AddPage(&m_outputPage);
		AddPage(&m_creditPage);

	}


	CModelDlg::~CModelDlg()
	{}


	BOOL CModelDlg::OnInitDialog()
	{
		BOOL bResult = CMFCPropertySheet::OnInitDialog();

		// Set the icon for this dialog.  The framework does this automatically
		//  when the application's main window is not a dialog
		SetIcon(m_hIcon, TRUE); // Set big icon
		SetIcon(m_hIcon, FALSE); // Set small icon
		SetWindowText(UtilWin::GetCString(IDS_MODELS_CAPTION) + UtilWin::Convert(m_model.GetName()));


		return bResult;
	}

}