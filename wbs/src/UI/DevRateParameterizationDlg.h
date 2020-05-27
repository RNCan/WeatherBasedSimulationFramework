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

#include "Simulation/DevRateParameterization.h"
#include "ModelBase/Model.h"
#include "UI/Common/CommonCtrl.h"

#include "WeatherBasedSimulationUI.h"


namespace WBSF
{

	class CDevRateParameterizationDlg : public CDialogEx
	{
		// Construction
	public:

		//friend CDevRateParameterization;

		CDevRateParameterizationDlg(const CExecutablePtr& pParent, CWnd* pParentWnd);   // standard constructor

		virtual void SetExecutable(CExecutablePtr pExecutable){ m_sa = GetSA(pExecutable); }
		virtual CExecutablePtr GetExecutable()const{ return CExecutablePtr(new CDevRateParameterization(m_sa)); }

	protected:

		enum { IDD = IDD_SIM_FIT_EQUATION};

		CCFLEdit	m_nameCtrl;
		CCFLEdit	m_internalNameCtrl;
		CCFLEdit	m_descriptionCtrl;
		
		CCFLComboBox	m_inputFileNameCtrl;
		CDefaultComboBox	m_TobsFileNameCtrl;
		CCFLEdit		m_ouputFileNameCtrl;
		
		CButton			m_calibSigmaCtrl;
		CButton			m_fixeSigmaCtrl;

		CCFLComboBox	m_fitTypeCtrl;
		CSelectionCtrl 	m_eqDevRateCtrl;
		CSelectionCtrl 	m_eqMortalityCtrl;
		
		//CButton			m_converge01Ctrl;
		

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


		DECLARE_MESSAGE_MAP()

	private:

		void UpdateCtrl(void);

		CDevRateParameterization m_sa;
		CDevRateParameterization& GetSA(const CExecutablePtr& pItem){ ASSERT(pItem); return dynamic_cast<CDevRateParameterization&>(*pItem); }



	};
}