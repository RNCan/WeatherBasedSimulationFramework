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


#include "Simulation/MergeExecutable.h"
#include "ExecutableTree.h"

#include "WeatherBasedSimulationUI.h"

namespace WBSF
{

	/////////////////////////////////////////////////////////////////////////////
	// CMergeExecutableDlg dialog

	class CMergeExecutableDlg : public CDialog
	{
		// Construction
	public:
		CMergeExecutableDlg(const CExecutablePtr& pParent, CWnd* pParentWnd);   // standard constructor

		CExecutablePtr GetExecutable()const{ return m_merge.CopyObject(); }
		void SetExecutable(CExecutablePtr pExecute){ m_merge = GetMergeExecutable(pExecute); }
		CMergeExecutable& GetMergeExecutable(const CExecutablePtr& pItem){ ASSERT(pItem); return dynamic_cast<CMergeExecutable&>(*pItem); }

	protected:

		// Dialog Data
		enum { IDD = IDD_SIM_MERGE_EXECUTABLE };

		CExecutableTree m_executableCtrl;


		//	virtual BOOL OnInitDialog ();
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

		// Generated message map functions
		DECLARE_MESSAGE_MAP()
		afx_msg LRESULT OnCheckbox(WPARAM wParam, LPARAM lParam);

		CMergeExecutable m_merge;
		CExecutablePtr m_pParent;
		
	};

}