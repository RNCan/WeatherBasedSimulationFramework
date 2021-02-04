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
		DDX_Control(pDX, IDC_FIT_FIXE_TB, m_fixeTbCtrl);
		DDX_Control(pDX, IDC_FIT_TB, m_TbCtrl);
		DDX_Control(pDX, IDC_FIT_FIXE_TO, m_fixeToCtrl);
		DDX_Control(pDX, IDC_FIT_TO, m_ToCtrl);
		DDX_Control(pDX, IDC_FIT_FIXE_TM, m_fixeTmCtrl);
		DDX_Control(pDX, IDC_FIT_TM, m_TmCtrl);
		DDX_Control(pDX, IDC_FIT_TYPE, m_fitTypeCtrl);
		DDX_Control(pDX, IDC_FIT_USE_OUTPUT_AS_INPUT, m_useOutputAsInputCtrl);
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
	
			m_sa.m_bFixeTb = m_fixeTbCtrl.GetCheck();
			m_sa.m_Tb = ToDouble(m_TbCtrl.GetString());
			m_sa.m_bFixeTo = m_fixeToCtrl.GetCheck();
			m_sa.m_To = ToDouble(m_ToCtrl.GetString());
			m_sa.m_bFixeTm = m_fixeTmCtrl.GetCheck();
			m_sa.m_Tm = ToDouble(m_TmCtrl.GetString());


			m_sa.m_bUseOutputAsInput = m_useOutputAsInputCtrl.GetCheck();
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

			FillInputFile();
			m_inputFileNameCtrl.SelectString(0, m_sa.m_inputFileName);
			m_TobsFileNameCtrl.SelectString(0, m_sa.m_TobsFileName);
			m_ouputFileNameCtrl.SetString(m_sa.m_outputFileName);
			//m_calibOnCtrl.SetCurSel((int)m_sa.m_calibOn);
			m_fitTypeCtrl.SetCurSel((int)m_sa.m_fitType);
			m_sa.m_eqDevRate.SetSelection(m_eqDevRateCtrl.GetSelection());
			m_sa.m_eqSurvival.SetSelection(m_eqSurvivalCtrl.GetSelection());
			//m_converge01Ctrl.SetCheck(m_sa.m_bConverge01);
			//m_calibSigmaCtrl.SetCheck(m_sa.m_bCalibSigma);
			//m_fixeSigmaCtrl.SetCheck(m_sa.m_bFixeSigma);
			m_fixeTbCtrl.SetCheck(m_sa.m_bFixeTb);
			m_TbCtrl.SetString(ToString(m_sa.m_Tb ));
			m_fixeToCtrl.SetCheck(m_sa.m_bFixeTo);
			m_ToCtrl.SetString(ToString(m_sa.m_To));
			m_fixeTmCtrl.SetCheck(m_sa.m_bFixeTm);
			m_TmCtrl.SetString(ToString(m_sa.m_Tm));

			m_useOutputAsInputCtrl.SetCheck(m_sa.m_bUseOutputAsInput);
			m_ShowTraceCtrl.SetCheck(m_sa.m_bShowTrace);

		}

	}


	BEGIN_MESSAGE_MAP(CInsectParameterizationDlg, CDialogEx)
		ON_BN_CLICKED(IDC_FIT_SA, &OnEditSACtrl)
		ON_BN_CLICKED(IDC_FIT_EQ_OPTIONS, &OnEditEqOptions)
		ON_BN_CLICKED(IDC_FIT_FIXE_TB, &UpdateCtrl)
		ON_BN_CLICKED(IDC_FIT_FIXE_TO, &UpdateCtrl)
		ON_BN_CLICKED(IDC_FIT_FIXE_TM, &UpdateCtrl)
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

		bool bDev = type == CInsectParameterization::F_DEV_TIME_WTH_SIGMA || type == CInsectParameterization::F_DEV_TIME_ONLY;
		bool bSurvival = type == CInsectParameterization::F_SURVIVAL;
		bool bOvip = type == CInsectParameterization::F_OVIPOSITION;

		m_eqDevRateCtrl.ShowWindow(bDev? SW_SHOW : SW_HIDE);
		m_eqSurvivalCtrl.ShowWindow(bSurvival ? SW_SHOW : SW_HIDE);
		
		m_TbCtrl.EnableWindow(m_fixeTbCtrl.GetCheck());
		m_ToCtrl.EnableWindow(m_fixeToCtrl.GetCheck());
		m_TmCtrl.EnableWindow(m_fixeTmCtrl.GetCheck());

		m_fixeTbCtrl.ShowWindow(bDev ? SW_SHOW : SW_HIDE);
		m_fixeToCtrl.ShowWindow(bDev ? SW_SHOW : SW_HIDE);
		m_fixeTmCtrl.ShowWindow(bDev ? SW_SHOW : SW_HIDE);
		m_TbCtrl.ShowWindow(bDev ? SW_SHOW : SW_HIDE);
		m_ToCtrl.ShowWindow(bDev ? SW_SHOW : SW_HIDE);
		m_TmCtrl.ShowWindow(bDev ? SW_SHOW : SW_HIDE);

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
