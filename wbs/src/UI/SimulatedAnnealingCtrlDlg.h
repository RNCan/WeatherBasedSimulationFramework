//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//	
//******************************************************************************
#pragma once

#include "Simulation/ModelParameterization.h"
#include "WeatherBasedSimulationUI.h"


namespace WBSF
{

	/////////////////////////////////////////////////////////////////////////////
	// CSimulatedAnnealingCtrlDlg dialog

	class CSimulatedAnnealingCtrlDlg : public CDialog
	{
		DECLARE_DYNCREATE(CSimulatedAnnealingCtrlDlg)

		// Construction
	public:
		CSimulatedAnnealingCtrlDlg(CWnd* pParent = NULL);
		~CSimulatedAnnealingCtrlDlg();

		CSAControl m_ctrl;

	protected:

		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		DECLARE_MESSAGE_MAP()
		void GetSACtrlFromInterface();
		void SetSACtrlToInterface();


		enum { IDD = IDD_SIM_SIMULATED_ANNEALING_PARAMETERS };
		CString	m_inputFilePath;
		double	m_initialTemperature;
		double	m_errorTolerence;
		int	m_nbCycles;
		int	m_nbIteration;
		int	m_nbSkipLoop;
		double	m_TReduction;
		double  m_TReduction2;
		long	m_maxEvaluation;
		long	m_nbEps;
		int		m_max;
		long	m_seed1;
		long	m_seed2;
	};

}