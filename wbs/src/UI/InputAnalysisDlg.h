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


#include "Simulation/InputAnalysis.h"
#include "WeatherBasedSimulationUI.h"


namespace WBSF
{

	/////////////////////////////////////////////////////////////////////////////
	// CInputAnalysisDlg dialog

	class CInputAnalysisDlg : public CDialog
	{
		// Construction
	public:
		CInputAnalysisDlg(const CExecutablePtr& pParent, CWnd* pParentWnd);   // standard constructor

		CExecutablePtr GetExecutable()const{ return m_analysis.CopyObject(); }
		void SetExecutable(CExecutablePtr pExecute){ m_analysis = GetAnalysis(pExecute); }
		CInputAnalysis & GetAnalysis(const CExecutablePtr& pItem){ ASSERT(pItem); return dynamic_cast<CInputAnalysis&>(*pItem); }

	protected:

		// Dialog Data
		enum { IDD = IDD_SIM_INPUT_ANALYSIS };

		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

		// Generated message map functions
		DECLARE_MESSAGE_MAP()

		CInputAnalysis m_analysis;
		CExecutablePtr m_pParent;
	};

}