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


#include "Basic/Statistic.h"
#include "UI/SimulatedAnnealingCtrlDlg.h"



#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif


namespace WBSF
{

	IMPLEMENT_DYNCREATE(CSimulatedAnnealingCtrlDlg, CDialog)


		/////////////////////////////////////////////////////////////////////////////
		// CSimulatedAnnealingCtrlDlg property page

		CSimulatedAnnealingCtrlDlg::CSimulatedAnnealingCtrlDlg(CWnd* pParent) :
		CDialog(CSimulatedAnnealingCtrlDlg::IDD, pParent)
	{
		//{{AFX_DATA_INIT(CSimulatedAnnealingCtrlDlg)
		//}}AFX_DATA_INIT
	}


	CSimulatedAnnealingCtrlDlg::~CSimulatedAnnealingCtrlDlg()
	{
	}

	void CSimulatedAnnealingCtrlDlg::DoDataExchange(CDataExchange* pDX)
	{
		CDialog::DoDataExchange(pDX);

		if (!pDX->m_bSaveAndValidate)
			SetSACtrlToInterface();

	
		DDX_Text(pDX, IDC_INITIAL_TEMPERATURE, m_initialTemperature);
		DDV_MinMaxDouble(pDX, m_initialTemperature, 1e-8, 1e8);
		DDX_Text(pDX, IDC_ERROR_TOLERENCE, m_errorTolerence);
		DDX_Text(pDX, IDC_NB_CYCLES, m_nbCycles);
		DDV_MinMaxInt(pDX, m_nbCycles, 1, 1000);
		DDX_Text(pDX, IDC_NB_ITERATION, m_nbIteration);
		DDV_MinMaxInt(pDX, m_nbIteration, 1, 1000);
		DDX_Text(pDX, IDC_T_REDUCTION, m_TReduction);
		DDV_MinMaxDouble(pDX, m_TReduction, 0.00001, 1.0);
		DDX_Text(pDX, IDC_T_REDUCTION2, m_TReduction2);
		DDV_MinMaxDouble(pDX, m_TReduction2, 0.00001, 1.0);
		DDX_Text(pDX, IDC_MAX_ELVA, m_maxEvaluation);
		DDV_MinMaxLong(pDX, m_maxEvaluation, 1, 100000000);
		DDX_Text(pDX, IDC_NB_ESP, m_nbEps);
		DDV_MinMaxLong(pDX, m_nbEps, 1, 10);
		DDX_CBIndex(pDX, IDC_OPTIMISATION_TYPE, m_max);
		DDX_Text(pDX, IDC_SEED1, m_seed1);
		DDV_MinMaxLong(pDX, m_seed1, -1, 31328);
		DDX_Text(pDX, IDC_SEED2, m_seed2);
		DDV_MinMaxLong(pDX, m_seed2, -1, 30081);
		int curSel = m_ctrl.m_statisticType - WBSF::BIAS;
		DDX_CBIndex(pDX, IDC_STATISTIC_TYPE, curSel);
		m_ctrl.m_statisticType = curSel + WBSF::BIAS;
	

		if (pDX->m_bSaveAndValidate)
			GetSACtrlFromInterface();

	}


	BEGIN_MESSAGE_MAP(CSimulatedAnnealingCtrlDlg, CDialog)
	
	END_MESSAGE_MAP()

	void CSimulatedAnnealingCtrlDlg::SetSACtrlToInterface()
	{
		m_maxEvaluation = m_ctrl.MAXEVL();

		m_nbEps = m_ctrl.NEPS();
		m_max = m_ctrl.Max();
		m_seed1 = m_ctrl.Seed1();
		m_seed2 = m_ctrl.Seed2();

		m_initialTemperature = m_ctrl.T();
		m_errorTolerence = m_ctrl.EPS();

		m_nbCycles = (short)m_ctrl.NS();
		m_nbIteration = (short)m_ctrl.NT();
		m_TReduction = m_ctrl.m_RT;
		m_TReduction2 = m_ctrl.m_RT2;
	}

	void CSimulatedAnnealingCtrlDlg::GetSACtrlFromInterface()
	{
		m_ctrl.SetMAXEVL(m_maxEvaluation);
		m_ctrl.SetNEPS(m_nbEps);
		m_ctrl.SetMax(m_max != 0);
		m_ctrl.SetSeed1(m_seed1);
		m_ctrl.SetSeed2(m_seed2);

		m_ctrl.SetT(m_initialTemperature);
		m_ctrl.SetEPS(m_errorTolerence);
		m_ctrl.SetNS(m_nbCycles);
		m_ctrl.SetNT(m_nbIteration);
		m_ctrl.m_RT = m_TReduction;
		m_ctrl.m_RT2 = m_TReduction2;

	}

}