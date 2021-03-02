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

/////////////////////////////////////////////////////////////////////////////
// CSimAnnealingDlg dialog

#include "Simulation/InsectParameterization.h"
#include "ModelBase/Model.h"
#include "UI/Common/CommonCtrl.h"

#include "WeatherBasedSimulationUI.h"


namespace WBSF
{

	class CInsectParameterizationDlg : public CDialogEx
	{
		// Construction
	public:

		CInsectParameterizationDlg(const CExecutablePtr& pParent, CWnd* pParentWnd);   // standard constructor

		virtual void SetExecutable(CExecutablePtr pExecutable) { m_sa = GetSA(pExecutable); }
		virtual CExecutablePtr GetExecutable()const { return CExecutablePtr(new CInsectParameterization(m_sa)); }

	protected:

		enum { IDD = IDD_SIM_FIT_EQUATION };

		CCFLEdit	m_nameCtrl;
		CCFLEdit	m_internalNameCtrl;
		CCFLEdit	m_descriptionCtrl;

		CCFLComboBox	m_inputFileNameCtrl;
		CDefaultComboBox	m_TobsFileNameCtrl;
		CCFLEdit		m_ouputFileNameCtrl;

		CCFLComboBox	m_fitTypeCtrl;
		CSelectionCtrl 	m_eqDevRateCtrl;
		CSelectionCtrl 	m_eqSurvivalCtrl;
		CSelectionCtrl 	m_eqFecundityCtrl;

		CButton			m_fixeTbCtrl;
		std::array<CCFLEdit, 3> m_TbCtrl;
		CButton			m_fixeToCtrl;
		std::array < CCFLEdit, 3>m_ToCtrl;
		CButton			m_fixeTmCtrl;
		std::array < CCFLEdit, 3>m_TmCtrl;
		CButton			m_fixeF0Ctrl;
		std::array<CCFLEdit, 3>		m_F0Ctrl;

		CButton			m_useOutputAsInputCtrl;
		CCFLComboBox	m_outputAsInputCtrl;
		CButton			m_ShowTraceCtrl;
		// Overrides

	protected:
		virtual void OnOK();
		virtual BOOL OnInitDialog();
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

		// Generated message map functions
		afx_msg void OnEditSACtrl();
		afx_msg void OnEditEqOptions();
		afx_msg void OnFitTypeChange();
		void FillInputFile();
		void FillOutputAsInputFile();


		DECLARE_MESSAGE_MAP()

	private:

		void UpdateCtrl(void);

		CInsectParameterization m_sa;
		CInsectParameterization& GetSA(const CExecutablePtr& pItem) { ASSERT(pItem); return dynamic_cast<CInsectParameterization&>(*pItem); }



	};
}