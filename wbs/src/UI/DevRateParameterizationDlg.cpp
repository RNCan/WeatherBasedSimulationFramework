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
#include "UI/FitEquationOptionDlg.h"

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
		CDialogEx(IDD, pParentWnd),
		m_TobsFileNameCtrl(_T(""))
	{
	}


	void CDevRateParameterizationDlg::DoDataExchange(CDataExchange* pDX)
	{
		CDialogEx::DoDataExchange(pDX);

		DDX_Control(pDX, IDC_SIM_NAME, m_nameCtrl);
		DDX_Control(pDX, IDC_SIM_INTERNAL_NAME, m_internalNameCtrl);
		DDX_Control(pDX, IDC_SIM_DESCRIPTION, m_descriptionCtrl);
		DDX_Control(pDX, IDC_FIT_INPUT_NAME, m_inputFileNameCtrl);
		DDX_Control(pDX, IDC_FIT_OBS_TEMPERATURE, m_TobsFileNameCtrl);
		DDX_Control(pDX, IDC_FIT_OUTPUT_NAME, m_ouputFileNameCtrl);
		//DDX_Control(pDX, IDC_FIT_CALIB_ON, m_calibOnCtrl);
		DDX_Control(pDX, IDC_FIT_EQ_DEV_RATE, m_eqDevRateCtrl);
		DDX_Control(pDX, IDC_FIT_EQ_MORTALITY, m_eqMortalityCtrl);
		//DDX_Control(pDX, IDC_FIT_LIMIT_DEVRATE, m_converge01Ctrl);
		DDX_Control(pDX, IDC_FIT_CALIB_SIGMA, m_calibSigmaCtrl);
		DDX_Control(pDX, IDC_FIT_FIXED_SIGMA, m_fixeSigmaCtrl);
		DDX_Control(pDX, IDC_FIT_TYPE, m_fitTypeCtrl);
		
		

		
		if (pDX->m_bSaveAndValidate)
		{
			m_sa.SetName(m_nameCtrl.GetString());
			m_sa.SetDescription(m_descriptionCtrl.GetString());
			m_sa.m_inputFileName = m_inputFileNameCtrl.GetString();
			m_sa.m_TobsFileName = m_TobsFileNameCtrl.GetString();
			m_sa.m_outputFileName = m_ouputFileNameCtrl.GetString();
//			m_sa.m_calibOn = m_calibOnCtrl.GetCurSel();
			m_sa.m_fitType = m_fitTypeCtrl.GetCurSel();
			m_sa.m_eqDevRate.SetSelection(m_eqDevRateCtrl.GetSelection());
			m_sa.m_eqMortality.SetSelection(m_eqMortalityCtrl.GetSelection());
			//m_sa.m_bConverge01 = m_converge01Ctrl.GetCheck();
			m_sa.m_bCalibSigma = m_calibSigmaCtrl.GetCheck();
			m_sa.m_bFixeSigma = m_fixeSigmaCtrl.GetCheck();
		}
		else
		{
			m_nameCtrl.SetWindowText(m_sa.GetName());
			m_internalNameCtrl.SetWindowText(m_sa.GetInternalName());
			m_descriptionCtrl.SetWindowText(m_sa.GetDescription());

			std::string possibleValues1;
			for (size_t e = 0; e < CDevRateEquation::NB_EQUATIONS; e++)
			{
				if (!possibleValues1.empty())
					possibleValues1 += "|";

				possibleValues1 += CDevRateEquation::GetEquationName(CDevRateEquation::eq(e));
			}
			
			std::string possibleValues2;
			for (size_t e = 0; e < CMortalityEquation::NB_EQUATIONS; e++)
			{
				if (!possibleValues2.empty())
					possibleValues2 += "|";

				possibleValues2 += CMortalityEquation::GetEquationName(CMortalityEquation::eq(e));
			}



			m_eqDevRateCtrl.SetPossibleValues(possibleValues1);
			m_eqDevRateCtrl.SetSelection(m_sa.m_eqDevRate.GetSelection());
			m_eqMortalityCtrl.SetPossibleValues(possibleValues2);
			m_eqMortalityCtrl.SetSelection(m_sa.m_eqMortality.GetSelection());

			FillInputFile();
			m_inputFileNameCtrl.SelectString(0, m_sa.m_inputFileName);
			m_TobsFileNameCtrl.SelectString(0, m_sa.m_TobsFileName);
			m_ouputFileNameCtrl.SetString(m_sa.m_outputFileName);
			//m_calibOnCtrl.SetCurSel((int)m_sa.m_calibOn);
			m_fitTypeCtrl.SetCurSel((int)m_sa.m_fitType);
			m_sa.m_eqDevRate.SetSelection(m_eqDevRateCtrl.GetSelection());
			m_sa.m_eqMortality.SetSelection(m_eqMortalityCtrl.GetSelection());
			//m_converge01Ctrl.SetCheck(m_sa.m_bConverge01);
			m_calibSigmaCtrl.SetCheck(m_sa.m_bCalibSigma);
			m_fixeSigmaCtrl.SetCheck(m_sa.m_bFixeSigma);

		}

	}


	BEGIN_MESSAGE_MAP(CDevRateParameterizationDlg, CDialogEx)
		ON_BN_CLICKED(IDC_FIT_SA, &OnEditSACtrl)
		ON_BN_CLICKED(IDC_FIT_EQ_OPTIONS, &OnEditEqOptions)
		ON_CBN_SELCHANGE(IDC_FIT_TYPE, &OnFitTypeChange)
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

		UpdateCtrl();

		return TRUE;  // return TRUE unless you set the focus to a control
		// EXCEPTION: OCX Property Pages should return FALSE
	}



	void CDevRateParameterizationDlg::UpdateCtrl(void)
	{
		size_t type = (size_t)m_fitTypeCtrl.GetCurSel();
		m_eqDevRateCtrl.ShowWindow((type == CDevRateParameterization::F_DEV_RATE) ? SW_SHOW : SW_HIDE);
		m_calibSigmaCtrl.ShowWindow((type == CDevRateParameterization::F_DEV_RATE) ? SW_SHOW : SW_HIDE);
		m_fixeSigmaCtrl.ShowWindow((type == CDevRateParameterization::F_DEV_RATE) ? SW_SHOW : SW_HIDE);

		m_eqMortalityCtrl.ShowWindow((type == CDevRateParameterization::F_MORTALITY) ? SW_SHOW : SW_HIDE);
		
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
		m_TobsFileNameCtrl.FillList(list);
	}

	
}


void WBSF::CDevRateParameterizationDlg::OnEditEqOptions()
{
	size_t fitType = m_fitTypeCtrl.GetCurSel();
	CFitInputParamDlg dlg(fitType, this);
	dlg.m_eq_options = m_sa.m_eq_options;

	if (dlg.DoModal() == IDOK)
	{
		m_sa.m_eq_options = dlg.m_eq_options;
	}

}


void WBSF::CDevRateParameterizationDlg::OnFitTypeChange()
{
	UpdateCtrl();
}
