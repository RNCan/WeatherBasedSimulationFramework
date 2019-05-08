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

#include "Simulation/ModelParameterization.h"
#include "ModelBase/Model.h"
#include "UI/Common/CommonCtrl.h"

#include "WeatherBasedSimulationUI.h"


namespace WBSF
{

	class CModelParameterizationDlg : public CDialog
	{
		// Construction
	public:

		//friend CModelParameterization;

		CModelParameterizationDlg(const CExecutablePtr& pParent, CWnd* pParentWnd);   // standard constructor

		virtual void SetExecutable(CExecutablePtr pExecutable){ m_sa = GetSA(pExecutable); }
		virtual CExecutablePtr GetExecutable()const{ return CExecutablePtr(new CModelParameterization(m_sa)); }

	protected:

		enum { IDD = IDD_SIM_SIMULATED_ANNEALING };

		CCFLEdit	m_nameCtrl;
		CCFLEdit	m_internalNameCtrl;
		CCFLEdit	m_descriptionCtrl;


		CCFLComboBox	m_modelCtrl;
		CDefaultComboBox	m_modelInputNameCtrl;
		CCFLComboBox	m_parametersVariationsNameCtrl;
		CComboBox		m_feedbackCtrl;
		CCFLComboBox	m_resultFileNameCtrl;
		CCFLComboBox	m_locIDFieldCtrl;
		CButton			m_useHxGridCtrl;
		CCFLEdit		m_replicationCtrl;
		CCFLEdit		m_modelInputOptNameCtrl;

		// Overrides
		// ClassWizard generated virtual function overrides

	protected:
		virtual void OnOK();
		virtual BOOL OnInitDialog();
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

		// Generated message map functions

		afx_msg void OnEditModelInput();
		afx_msg void FillParametersVariations(void);

		afx_msg void OnEditParametersVariations();
		afx_msg void OnEditSACtrl();
		afx_msg void OnModelChange();
		afx_msg void OnModelDescription();


		DECLARE_MESSAGE_MAP()

	private:

		void UpdateCtrl(void);
		void FillModel();
		void FillModelInput(void);
		void FillResultFile();
		void FillLOCIDField();


		void GetSimulationFromInterface();
		void SetSimulationToInterface();

		CModelParameterization m_sa;
		CModelParameterization& GetSA(const CExecutablePtr& pItem){ ASSERT(pItem); return dynamic_cast<CModelParameterization&>(*pItem); }

		std::string m_lastModelLoad;
		CModel m_model;


	};
}