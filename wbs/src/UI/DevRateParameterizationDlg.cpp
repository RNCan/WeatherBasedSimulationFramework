//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
// 01-03-2019	Rémi Saint-Amant	Include into Weather-based simulation framework
//****************************************************************************
#include "stdafx.h"
#include "Basic/Registry.h"
#include "Basic/CSV.h"
#include "FileManager/FileManager.h"
#include "ModelBase/Model.h"
#include "ModelBase/WGInput.h"
#include "UI/Common/UtilWin.h" 
#include "UI/Common/TextDlg.h"
#include "UI/SimulatedAnnealingCtrlDlg.h"
#include "UI/DevRateParameterizationDlg.h"
#include "UI/ModelInputManagerDlg.h"
#include "UI/LOCEditDlg.h"

#include "WeatherBasedSimulationString.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace std;


namespace WBSF
{

	//******************************************************************
	//CExecuteDialog

	//******************************************************************
	// CDevRateParameterizationDlg dialog

	CDevRateParameterizationDlg::CDevRateParameterizationDlg(const CExecutablePtr& pParent, CWnd* pParentWnd) :
		CDialogEx(IDD, pParentWnd)
	{
	}


	void CDevRateParameterizationDlg::DoDataExchange(CDataExchange* pDX)
	{
		CDialogEx::DoDataExchange(pDX);

		DDX_Control(pDX, IDC_SIM_NAME, m_nameCtrl);
		DDX_Control(pDX, IDC_SIM_INTERNAL_NAME, m_internalNameCtrl);
		DDX_Control(pDX, IDC_SIM_DESCRIPTION, m_descriptionCtrl);
		DDX_Control(pDX, IDC_SIM_INPUT_NAME, m_inputFileNameCtrl);
		DDX_Control(pDX, IDC_SIM_OUTPUT_NAME, m_ouputFileNameCtrl);
		DDX_Control(pDX, IDC_SIM_EQUATION, m_equationsCtrl);
		DDX_Control(pDX, IDC_SIM_LIMIT_DEVRATE, m_converge01Ctrl);
	

		
		if (pDX->m_bSaveAndValidate)
		{
			m_sa.SetName(m_nameCtrl.GetString());
			m_sa.SetDescription(m_descriptionCtrl.GetString());
			m_sa.m_inputFileName = m_inputFileNameCtrl.GetString();
			m_sa.m_outputFileName = m_ouputFileNameCtrl.GetString();
			m_sa.m_equations.SetSelection(m_equationsCtrl.GetSelection());
			m_sa.m_bConverge01 = m_converge01Ctrl.GetCheck();
			

		}
		else
		{
			m_nameCtrl.SetWindowText(m_sa.GetName());
			m_internalNameCtrl.SetWindowText(m_sa.GetInternalName());
			m_descriptionCtrl.SetWindowText(m_sa.GetDescription());


			std::string possibleValues;
			for (size_t e = 0; e < CDevRateEquation::NB_EQUATIONS; e++)
			{
				if (!possibleValues.empty())
					possibleValues += "|";

				possibleValues += CDevRateEquation::GetEquationName(CDevRateEquation::e(e));
			}

			m_equationsCtrl.SetPossibleValues(possibleValues);
			m_equationsCtrl.SetSelection(m_sa.m_equations.GetSelection());

			FillInputFile();
			m_inputFileNameCtrl.SelectString(0, m_sa.m_inputFileName);
			m_ouputFileNameCtrl.SetString(m_sa.m_outputFileName);
			m_sa.m_equations.SetSelection(m_equationsCtrl.GetSelection());
			m_converge01Ctrl.SetCheck(m_sa.m_bConverge01);

		}

	}


	BEGIN_MESSAGE_MAP(CDevRateParameterizationDlg, CDialogEx)
		ON_BN_CLICKED(IDC_SIM_SA, OnEditSACtrl)
	END_MESSAGE_MAP()

	/////////////////////////////////////////////////////////////////////////////
	// CDevRateParameterizationDlg message handlers




	void CDevRateParameterizationDlg::OnOK()
	{
		/*if(m_LOCNameCtrl.GetWindowText().IsEmpty() )
		{
		MessageBox(UtilWin::GetCString(IDS_SIM_NOLOC), AfxGetAppName() , MB_ICONEXCLAMATION|MB_OK);
		return ;
		}*/

		

		//WBSF::CRegistry option;
		//option.WriteProfileString("LastModel", m_sa.GetModelName().c_str());

		CDialogEx::OnOK();

		//m_bInit = false;
	}

	BOOL CDevRateParameterizationDlg::OnInitDialog()
	{
		CDialogEx::OnInitDialog();

		//	m_inputFileNameCtrl.EnableFileBrowseButton(".csv","*.csv|*.csv||");


		SetSimulationToInterface();

		UpdateCtrl();

		return TRUE;  // return TRUE unless you set the focus to a control
		// EXCEPTION: OCX Property Pages should return FALSE
	}



	void CDevRateParameterizationDlg::UpdateCtrl(void)
	{
	}

	void CDevRateParameterizationDlg::OnEditSACtrl()
	{
		CSimulatedAnnealingCtrlDlg dlg;

		dlg.m_ctrl = m_sa.GetControl();

		if (dlg.DoModal() == IDOK)
		{
			m_sa.SetControl(dlg.m_ctrl);
		}
	}

	void CDevRateParameterizationDlg::FillInputFile()
	{
		WBSF::StringVector list = WBSF::GetFM().Input().GetFilesList();
		m_inputFileNameCtrl.FillList(list);
	}

	void CDevRateParameterizationDlg::GetSimulationFromInterface()
	{
		
	}

	void CDevRateParameterizationDlg::SetSimulationToInterface()
	{
	}

}