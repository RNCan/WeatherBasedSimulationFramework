//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once

#include "WeatherBasedSimulationUI.h"
/////////////////////////////////////////////////////////////////////////////
// COptionAdvanced dialog

namespace WBSF
{

	class COptionAdvanced : public CMFCPropertyPage
	{
		//	DECLARE_DYNCREATE(COptionAdvanced)

		// Construction
	public:
		COptionAdvanced();
		~COptionAdvanced();


		// Dialog Data

		enum { IDD = IDD_CMN_OPTION_ADVANCED };

		int		m_maxDistFromLOC;
		int		m_maxDistFromPoint;
		BOOL    m_bRunEvenFar;
		BOOL	m_bRunWithMissingYear;
		BOOL	m_bKeepTmpFile;
		int		m_nbMaxThreads;
		BOOL	m_bUseHxGrid;
		int		m_nbLagMin;
		int		m_nbLagMax;
		int		m_nbLagStep;
		float	m_lagDistMin;
		float	m_lagDistMax;
		float	m_lagDistStep;

		// Implementation
	protected:

		virtual void OnOK();
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

		// Generated message map functions
		DECLARE_MESSAGE_MAP()

	};

}