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

#include "Simulation/ModelExecution.h"
#include "ModelBase/Model.h"
#include "UI/Common/CommonCtrl.h"

#include "WeatherBasedSimulationUI.h"

namespace WBSF
{

	/////////////////////////////////////////////////////////////////////////////
	// CModelExecutionDlg dialog
	class CModelExecutionDlg : public CDialog
	{
		// Construction
	public:

		friend CModelExecution;

		CModelExecutionDlg(const CExecutablePtr& pParent, CWnd* pParentWnd = NULL);   // standard constructor
		


		virtual void SetExecutable(CExecutablePtr pExecutable){ m_modelExecution = GetModelGeneration(pExecutable); }
		virtual CExecutablePtr GetExecutable()const{ return CExecutablePtr(new CModelExecution(m_modelExecution)); }
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		virtual void OnOK();
		virtual BOOL OnInitDialog();

	protected:

		enum { IDD = IDD_SIM_MODEL_EXECUTION };
		CButton	m_linkClimZone;

		CCFLEdit	m_nameCtrl;
		CCFLEdit	m_internalNameCtrl;
		CCFLEdit	m_descriptionCtrl;

		CCFLComboBox	m_modelCtrl;
		CMFCButton		m_helpModelCtrl;
		CCFLEdit		m_aboutModelCtrl;

		CDefaultComboBox m_modelInputNameCtrl;
		CDefaultComboBox m_parametersVariationsNameCtrl;
		CCFLComboBox	m_seedCtrl;
		CCFLEdit		m_nbReplicationsCtrl;
		CCFLEdit		m_durationCtrl;
		CButton			m_useHxGridCtrl;
		CComboBox		m_behaviourCtrl;


		DECLARE_MESSAGE_MAP()
		afx_msg void OnEditModelInput();
		afx_msg void OnEditParametersVariations();
		afx_msg void OnParametersVariationsChange();
		afx_msg void OnModelChange();
		afx_msg void OnReplicationsChange();
		afx_msg void OnModelHelp();
		afx_msg void OnModelDescription();

	

		void UpdateCtrl(void);
		void FillModel();
		void FillModelInput(void);
		void FillParametersVariations(void);
		void UpdateNbLocations();
		void GetModelExecutionFromInterface();
		void SetModelExecutionToInterface();

		CModelExecution m_modelExecution;
		CModelExecution& GetModelGeneration(const CExecutablePtr& pItem){ ASSERT(pItem); return dynamic_cast<CModelExecution&>(*pItem); }

		std::string m_lastModelLoad;

		size_t m_nbLocations;// to calculate number of file generated
		size_t m_nbVariations;// to calculate number of file generated

		CExecutablePtr m_pParent;
		CModel m_model;
		CParentInfo m_info;
	};

}