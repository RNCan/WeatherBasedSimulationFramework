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


#include "Simulation/WGInputAnalysis.h"
#include "WeatherBasedSimulationUI.h"


namespace WBSF
{

	/////////////////////////////////////////////////////////////////////////////
	// CWGInputAnalysisDlg dialog

	class CWGInputAnalysisDlg : public CDialog
	{
		// Construction
	public:
		CWGInputAnalysisDlg(const CExecutablePtr& pParent, CWnd* pParentWnd);   // standard constructor

		CExecutablePtr GetExecutable()const{ return m_analysis.CopyObject(); }
		void SetExecutable(CExecutablePtr pExecute){ m_analysis = GetAnalysis(pExecute); }
		CWGInputAnalysis & GetAnalysis(const CExecutablePtr& pItem){ ASSERT(pItem); return dynamic_cast<CWGInputAnalysis&>(*pItem); }

	protected:

		// Dialog Data
		enum { IDD = IDD_SIM_INPUT_ANALYSIS };

		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

		// Generated message map functions
		DECLARE_MESSAGE_MAP()

		CWGInputAnalysis m_analysis;
		CExecutablePtr m_pParent;

		void UpdateCtrl();
		CComboBox m_kindCtrl;
		CButton m_exportCtrl;
	};

}