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

#include "Simulation/Mapping.h"
#include "Simulation/Executable.h"
#include "UI/Common/CommonCtrl.h"
#include "UI/Common/OpenDirEditCtrl.h"

#include "WeatherBasedSimulationUI.h"

namespace WBSF
{

//	class CSimulation;

	/////////////////////////////////////////////////////////////////////////////
	// CMappingDlg dialog

	class CMappingDlg : public CDialog
	{
		// Construction
	public:
		
		CMappingDlg(const CExecutablePtr& pParent, CWnd* pParentWnd);   // standard constructor

		CExecutablePtr GetExecutable()const{ return m_mapping.CopyObject(); }
		void SetExecutable(CExecutablePtr pExecute){ m_mapping = GetMapping(pExecute); }
		CMapping& GetMapping(const CExecutablePtr& pItem){ ASSERT(pItem); return dynamic_cast<CMapping&>(*pItem); }


		CCFLEdit		m_nameCtrl;
		CCFLEdit		m_descriptionCtrl;
		CComboBox		m_interpolMethodCtrl;
		CCFLComboBox	m_DEMCtrl;
		CCFLEdit		m_TEMCtrl;
		CCFLEdit		m_tranfoInfoCtrl;
		COpenDirEditCtrl	m_defaultDirCtrl;
		CEdit			m_dimensionCtrl;
		CButton			m_XValOnlyCtrl;
		CButton			m_useHxGridCtrl;


		CMapping m_mapping;
		CExecutablePtr m_pParent;

	protected:

		// Dialog Data
		enum { IDD = IDD_SIM_MAPPING };

		virtual BOOL OnInitDialog();
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support


		// Implementation
	protected:

		void GetMappingFromInterface();
		void SetMappingToInterface();

		// Generated message map functions
		afx_msg void OnMapEditor();
		afx_msg void OnEditTransfo();
		afx_msg void OnEditAdvanced();
		afx_msg void UpdateCtrl();

		DECLARE_MESSAGE_MAP()

		void InitDEMList(void);
		void InitDefaultDir(void);
		
		CParentInfo m_parentInfo;

	};
}
