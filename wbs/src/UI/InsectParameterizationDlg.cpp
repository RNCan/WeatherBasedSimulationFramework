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
#include "UI/InsectParameterizationDlg.h"
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
	// CInsectParameterizationDlg dialog

	CInsectParameterizationDlg::CInsectParameterizationDlg(const CExecutablePtr& pParent, CWnd* pParentWnd) :
		CDialogEx(IDD, pParentWnd),
		m_TobsFileNameCtrl(_T(""))
	{
	}


	void CInsectParameterizationDlg::DoDataExchange(CDataExchange* pDX)
	{
		CDialogEx::DoDataExchange(pDX);

		DDX_Control(pDX, IDC_SIM_NAME, m_nameCtrl);
		DDX_Control(pDX, IDC_SIM_INTERNAL_NAME, m_internalNameCtrl);
		DDX_Control(pDX, IDC_SIM_DESCRIPTION, m_descriptionCtrl);
		DDX_Control(pDX, IDC_FIT_INPUT_NAME, m_inputFileNameCtrl);
		DDX_Control(pDX, IDC_FIT_OBS_TEMPERATURE, m_TobsFileNameCtrl);
		DDX_Control(pDX, IDC_FIT_OUTPUT_NAME, m_ouputFileNameCtrl);
		DDX_Control(pDX, IDC_FIT_EQ_DEV_RATE, m_eqDevRateCtrl);
		DDX_Control(pDX, IDC_FIT_EQ_MORTALITY, m_eqSurvivalCtrl);
		DDX_Control(pDX, IDC_FIT_EQ_FECUNDITY, m_eqFecundityCtrl);
		
		DDX_Control(pDX, IDC_FIT_FIXE_TB, m_fixeTbCtrl);
		DDX_Control(pDX, IDC_FIT_TB_FROM, m_TbCtrl[0]);
		DDX_Control(pDX, IDC_FIT_TB_TO, m_TbCtrl[1]);
		DDX_Control(pDX, IDC_FIT_TB_BY, m_TbCtrl[2]);
		DDX_Control(pDX, IDC_FIT_FIXE_TM, m_fixeTmCtrl);
		DDX_Control(pDX, IDC_FIT_TM_FROM, m_TmCtrl[0]);
		DDX_Control(pDX, IDC_FIT_TM_TO, m_TmCtrl[1]);
		DDX_Control(pDX, IDC_FIT_TM_BY, m_TmCtrl[2]);

		DDX_Control(pDX, IDC_FIT_LIMIT_TB, m_ConstrainTloCtrl);
		DDX_Control(pDX, IDC_FIT_LIMIT_TLO_FROM, m_TloCtrl[0]);
		DDX_Control(pDX, IDC_FIT_LIMIT_TLO_TO, m_TloCtrl[1]);

		DDX_Control(pDX, IDC_FIT_LIMIT_TM, m_ConstrainThiCtrl);
		DDX_Control(pDX, IDC_FIT_LIMIT_THI_FROM, m_ThiCtrl[0]);
		DDX_Control(pDX, IDC_FIT_LIMIT_THI_TO, m_ThiCtrl[1]);


		DDX_Control(pDX, IDC_FIT_FIXE_F0, m_fixeF0Ctrl);
		DDX_Control(pDX, IDC_FIT_F0_FROM, m_F0Ctrl);
		

		DDX_Control(pDX, IDC_FIT_LIMIT_MAX_RATE, m_limitMaxRateCtrl);
		DDX_Control(pDX, IDC_FIT_LIMIT_MAX_RATE_F, m_LimitMaxRatePCtrl);

		DDX_Control(pDX, IDC_FIT_AVOID_INF_TIME_IN_OBS_RANGE, m_avoidNullRateInTobsCtrl);


		DDX_Control(pDX, IDC_FIT_TYPE, m_fitTypeCtrl);
		DDX_Control(pDX, IDC_FIT_USE_OUTPUT_AS_INPUT, m_useOutputAsInputCtrl);
		DDX_Control(pDX, IDC_FIT_OUTPUT_AS_INPUT, m_outputAsInputCtrl);
		DDX_Control(pDX, IDC_FIT_SHOW_TRACE, m_ShowTraceCtrl);
		
		
		if (pDX->m_bSaveAndValidate)
		{
			m_sa.SetName(m_nameCtrl.GetString());
			m_sa.SetDescription(m_descriptionCtrl.GetString());
			m_sa.m_inputFileName = m_inputFileNameCtrl.GetString();
			m_sa.m_TobsFileName = m_TobsFileNameCtrl.GetString();
			m_sa.m_outputFileName = m_ouputFileNameCtrl.GetString();
//			m_sa.m_calibOn = m_baseOnCtrl.GetCurSel();
			m_sa.m_fitType = m_fitTypeCtrl.GetCurSel();
			m_sa.m_eqDevRate.SetSelection(m_eqDevRateCtrl.GetSelection());
			m_sa.m_eqSurvival.SetSelection(m_eqSurvivalCtrl.GetSelection());
			m_sa.m_eqFecundity.SetSelection(m_eqFecundityCtrl.GetSelection());
			
	
			m_sa.m_bFixeTb = m_fixeTbCtrl.GetCheck();

			m_sa.m_bFixeTm = m_fixeTmCtrl.GetCheck();
			m_sa.m_bFixeF0 = m_fixeF0Ctrl.GetCheck();
			m_sa.m_bLimitMaxRate = m_limitMaxRateCtrl.GetCheck();

			for (size_t i = 0; i < 3; i++)
			{
				m_sa.m_Tb[i] = ToDouble(m_TbCtrl[i].GetString());
				m_sa.m_Tm[i] = ToDouble(m_TmCtrl[i].GetString());
			}

			m_sa.m_bConstrainTlo = m_ConstrainTloCtrl.GetCheck();
			m_sa.m_Tlo[0] = ToDouble(m_TloCtrl[0].GetString());
			m_sa.m_Tlo[1] = ToDouble(m_TloCtrl[1].GetString());

			m_sa.m_bConstrainThi = m_ConstrainThiCtrl.GetCheck();
			m_sa.m_Thi[0] = ToDouble(m_ThiCtrl[0].GetString());
			m_sa.m_Thi[1] = ToDouble(m_ThiCtrl[1].GetString());



			m_sa.m_F0 = ToDouble(m_F0Ctrl.GetString());
			m_sa.m_LimitMaxRateP = ToDouble(m_LimitMaxRatePCtrl.GetString());

			m_sa.m_bAvoidNullRateInTobs = m_avoidNullRateInTobsCtrl.GetCheck();
			m_sa.m_bUseOutputAsInput = m_useOutputAsInputCtrl.GetCheck();
			m_sa.m_outputAsIntputFileName = m_outputAsInputCtrl.GetString();
			m_sa.m_bShowTrace = m_ShowTraceCtrl.GetCheck();
			

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
			for (size_t e = 0; e < CSurvivalEquation::NB_EQUATIONS; e++)
			{
				if (!possibleValues2.empty())
					possibleValues2 += "|";

				possibleValues2 += CSurvivalEquation::GetEquationName(CSurvivalEquation::eq(e));
			}



			m_eqDevRateCtrl.SetPossibleValues(possibleValues1);
			m_eqDevRateCtrl.SetSelection(m_sa.m_eqDevRate.GetSelection());
			m_eqSurvivalCtrl.SetPossibleValues(possibleValues2);
			m_eqSurvivalCtrl.SetSelection(m_sa.m_eqSurvival.GetSelection());
			m_eqFecundityCtrl.SetPossibleValues(possibleValues1);
			m_eqFecundityCtrl.SetSelection(m_sa.m_eqFecundity.GetSelection());
			

			FillInputFile();
			FillOutputAsInputFile();
			m_inputFileNameCtrl.SelectString(0, m_sa.m_inputFileName);
			m_TobsFileNameCtrl.SelectString(0, m_sa.m_TobsFileName);
			m_ouputFileNameCtrl.SetString(m_sa.m_outputFileName);
			//m_calibOnCtrl.SetCurSel((int)m_sa.m_calibOn);
			m_fitTypeCtrl.SetCurSel((int)m_sa.m_fitType);
			m_sa.m_eqDevRate.SetSelection(m_eqDevRateCtrl.GetSelection());
			m_sa.m_eqSurvival.SetSelection(m_eqSurvivalCtrl.GetSelection());
			m_sa.m_eqFecundity.SetSelection(m_eqFecundityCtrl.GetSelection());
			//m_converge01Ctrl.SetCheck(m_sa.m_bConverge01);
			//m_calibSigmaCtrl.SetCheck(m_sa.m_bCalibSigma);
			//m_fixeSigmaCtrl.SetCheck(m_sa.m_bFixeSigma);
			m_fixeTbCtrl.SetCheck(m_sa.m_bFixeTb);
			m_fixeTmCtrl.SetCheck(m_sa.m_bFixeTm);
			m_fixeF0Ctrl.SetCheck(m_sa.m_bFixeF0);
			m_limitMaxRateCtrl.SetCheck(m_sa.m_bLimitMaxRate);

			for (size_t i = 0; i < 3; i++)
			{
				m_TbCtrl[i].SetString(ToString(m_sa.m_Tb[i]));
				m_TmCtrl[i].SetString(ToString(m_sa.m_Tm[i]));
			}

			m_ConstrainTloCtrl.SetCheck(m_sa.m_bConstrainTlo);
			m_TloCtrl[0].SetString(ToString(m_sa.m_Tlo[0]));
			m_TloCtrl[1].SetString(ToString(m_sa.m_Tlo[1]));

			m_ConstrainThiCtrl.SetCheck(m_sa.m_bConstrainThi);
			m_ThiCtrl[0].SetString(ToString(m_sa.m_Thi[0]));
			m_ThiCtrl[1].SetString(ToString(m_sa.m_Thi[1]));

			m_F0Ctrl.SetString(ToString(m_sa.m_F0));
			m_LimitMaxRatePCtrl.SetString(ToString(m_sa.m_LimitMaxRateP));


			m_avoidNullRateInTobsCtrl.SetCheck(m_sa.m_bAvoidNullRateInTobs);
			m_useOutputAsInputCtrl.SetCheck(m_sa.m_bUseOutputAsInput);
			m_outputAsInputCtrl.SetWindowText(m_sa.m_outputAsIntputFileName);
			m_ShowTraceCtrl.SetCheck(m_sa.m_bShowTrace);

		}

	}


	BEGIN_MESSAGE_MAP(CInsectParameterizationDlg, CDialogEx)
		ON_BN_CLICKED(IDC_FIT_SA, &OnEditSACtrl)
		ON_BN_CLICKED(IDC_FIT_EQ_OPTIONS, &OnEditEqOptions)
		ON_BN_CLICKED(IDC_FIT_FIXE_TB, &UpdateCtrl)
		ON_BN_CLICKED(IDC_FIT_FIXE_TM, &UpdateCtrl)
		ON_BN_CLICKED(IDC_FIT_LIMIT_TB, &UpdateCtrl)
		ON_BN_CLICKED(IDC_FIT_LIMIT_TM, &UpdateCtrl)
		ON_BN_CLICKED(IDC_FIT_FIXE_F0, &UpdateCtrl)
		ON_BN_CLICKED(IDC_FIT_LIMIT_MAX_RATE, &UpdateCtrl)
		ON_BN_CLICKED(IDC_FIT_USE_OUTPUT_AS_INPUT, &UpdateCtrl)
		ON_CBN_SELCHANGE(IDC_FIT_TYPE, &OnFitTypeChange)
	END_MESSAGE_MAP()

	/////////////////////////////////////////////////////////////////////////////
	// CInsectParameterizationDlg message handlers




	void CInsectParameterizationDlg::OnOK()
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

	BOOL CInsectParameterizationDlg::OnInitDialog()
	{
		CDialogEx::OnInitDialog();

		UpdateCtrl();

		return TRUE;  // return TRUE unless you set the focus to a control
		// EXCEPTION: OCX Property Pages should return FALSE
	}



	void CInsectParameterizationDlg::UpdateCtrl(void)
	{
		size_t type = (size_t)m_fitTypeCtrl.GetCurSel();

		bool bDev = type == CInsectParameterization::F_DEV_TIME;
		bool bSurvival = type == CInsectParameterization::F_SURVIVAL;
		bool bOvip = type == CInsectParameterization::F_FECUNDITY;


		m_eqDevRateCtrl.ShowWindow(bDev? SW_SHOW : SW_HIDE);
		m_eqSurvivalCtrl.ShowWindow(bSurvival ? SW_SHOW : SW_HIDE);
		m_eqFecundityCtrl.ShowWindow(bOvip ? SW_SHOW : SW_HIDE);
		
		m_outputAsInputCtrl.EnableWindow(m_useOutputAsInputCtrl.GetCheck());

		m_fixeTbCtrl.EnableWindow(bDev||bOvip);
		m_fixeTmCtrl.EnableWindow(bDev||bOvip);
		m_fixeF0Ctrl.EnableWindow(bOvip);
		m_limitMaxRateCtrl.EnableWindow(bDev || bOvip);


		for (size_t i = 0; i < 3; i++)
		{
			m_TbCtrl[i].EnableWindow((bDev||bOvip)&&m_fixeTbCtrl.GetCheck());
			m_TmCtrl[i].EnableWindow((bDev||bOvip)&&m_fixeTmCtrl.GetCheck());
		}

		m_ConstrainTloCtrl.EnableWindow(bDev || bOvip);
		m_TloCtrl[0].EnableWindow((bDev || bOvip) && m_ConstrainTloCtrl.GetCheck());
		m_TloCtrl[1].EnableWindow((bDev || bOvip) && m_ConstrainTloCtrl.GetCheck());
		
		m_ConstrainThiCtrl.EnableWindow(bDev || bOvip);
		m_ThiCtrl[0].EnableWindow((bDev || bOvip) && m_ConstrainThiCtrl.GetCheck());
		m_ThiCtrl[1].EnableWindow((bDev || bOvip) && m_ConstrainThiCtrl.GetCheck());


		m_F0Ctrl.EnableWindow(bOvip && m_fixeF0Ctrl.GetCheck());
		m_LimitMaxRatePCtrl.EnableWindow(bDev && m_limitMaxRateCtrl.GetCheck());

		m_avoidNullRateInTobsCtrl.EnableWindow(bDev || bOvip);
	}

	void CInsectParameterizationDlg::OnEditSACtrl()
	{
		CSimulatedAnnealingCtrlDlg dlg;

		dlg.m_ctrl = m_sa.GetControl();

		if (dlg.DoModal() == IDOK)
		{
			m_sa.SetControl(dlg.m_ctrl);
		}
	}

	void CInsectParameterizationDlg::FillInputFile()
	{
		WBSF::StringVector list = WBSF::GetFM().Input().GetFilesList();
		m_inputFileNameCtrl.FillList(list);
		m_TobsFileNameCtrl.FillList(list);
	}

	
	void CInsectParameterizationDlg::FillOutputAsInputFile()
	{
		WBSF::StringVector list = WBSF::GetFilesList( WBSF::GetFM().GetOutputPath()+"*.csv", FILE_NAME);
		m_outputAsInputCtrl.FillList(list);
		
	}

}


void WBSF::CInsectParameterizationDlg::OnEditEqOptions()
{
	size_t fitType = m_fitTypeCtrl.GetCurSel();
	CFitInputParamDlg dlg(fitType, this);
	dlg.m_eq_options = m_sa.m_eq_options;

	if (dlg.DoModal() == IDOK)
	{
		m_sa.m_eq_options = dlg.m_eq_options;
	}

}


void WBSF::CInsectParameterizationDlg::OnFitTypeChange()
{
	UpdateCtrl();
}
