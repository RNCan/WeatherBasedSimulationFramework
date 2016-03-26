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


#include "Simulation/Script.h"

#include "WeatherBasedSimulationUI.h"


namespace WBSF
{

	/////////////////////////////////////////////////////////////////////////////
	// CScriptDlg dialog

	class CScriptDlg : public CDialog
	{
		// Construction
	public:
		CScriptDlg(const CExecutablePtr& pParent, CWnd* pParentWnd);   // standard constructor



		CExecutablePtr GetExecutable()const{ return m_script.CopyObject(); }
		void SetExecutable(CExecutablePtr pExecute){ m_script = GetAnalysis(pExecute); }
		CScript & GetAnalysis(const CExecutablePtr& pItem){ ASSERT(pItem); return dynamic_cast<CScript&>(*pItem); }

	protected:

		// Dialog Data
		enum { IDD = IDD_SIM_SCRIPT };

		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

		// Generated message map functions
		DECLARE_MESSAGE_MAP()

		CScript m_script;
		CExecutablePtr m_pParent;


	};

}