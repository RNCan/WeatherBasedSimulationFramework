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




#include "Simulation/CopyExport.h"
#include "WeatherBasedSimulationUI.h"

namespace WBSF
{

	/////////////////////////////////////////////////////////////////////////////
	// CCopyExportDlg dialog

	class CCopyExportDlg : public CDialog
	{
		// Construction
	public:
		CCopyExportDlg(const CExecutablePtr& pParent, CWnd* pParentWnd);   // standard constructor



		CExecutablePtr GetExecutable()const{ return m_component.CopyObject(); }
		void SetExecutable(CExecutablePtr pExecute){ m_component = GetAnalysis(pExecute); }
		CCopyExport & GetAnalysis(const CExecutablePtr& pItem){ ASSERT(pItem); return dynamic_cast<CCopyExport&>(*pItem); }

	protected:

		// Dialog Data
		enum { IDD = IDD_SIM_COPY_EXPORT };

		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

		// Generated message map functions
		DECLARE_MESSAGE_MAP()

		CCopyExport m_component;
		CExecutablePtr m_pParent;

	};

}