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


#include "ModelBase/ParametersVariations.h"
#include "UI/Common/UtilWin.h"
#include "UI/Common/CommonCtrl.h"
#include "UI/Common/HTMLTree/XInfoTip.h"

#include "WeatherBasedSimulationUI.h"


namespace WBSF
{


	class CParametersVariationsProperties : public CMFCPropertyGridCtrl
	{
	public:

		CParametersVariationsProperties(const CModelInputParameterDefVector& parametersDefinition, CParametersVariationsDefinition& parameters);


		void EnableProperties(BOOL bEnable);
		void EnableProperties(CMFCPropertyGridProperty* pProp, BOOL bEnable);

		virtual BOOL PreTranslateMessage(MSG* pMsg);
		virtual void OnPropertyChanged(CMFCPropertyGridProperty* pProp) const;
		virtual BOOL ValidateItemData(CMFCPropertyGridProperty* /*pProp*/);

		void Init();
		size_t GetSelection()const{ return m_curP; }
		void SetSelection(size_t i);


	protected:

		const CModelInputParameterDefVector& m_parametersDefinition;
		CParametersVariationsDefinition& m_parametersVariations;
		size_t m_curP;

		DECLARE_MESSAGE_MAP()
		afx_msg void OnEnable(BOOL bEnable);
	};
	

	class CParametersVariationsDlg : public CDialog
	{
		// Construction
	public:


		//public member
		bool m_bShowVariationType;
		CModelInputParameterDefVector m_parametersDefinition;
		CParametersVariationsDefinition m_parametersVariations;

		CParametersVariationsDlg(CWnd* pParent = NULL, bool bShowVariationType = true);   // standard constructor


		enum { IDD = IDD_PARAMETERS_VARIATIONS };

		CSelectionCtrl m_parametersCtrl;
		CParametersVariationsProperties m_propertiesCtrl;
		CCFLComboBox	m_generationTypeCtrl;
		CCFLEdit		m_nbVariationsCtrl;


		virtual void OnOK();
		virtual void OnCancel();

		virtual BOOL OnInitDialog();
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		virtual BOOL PreTranslateMessage(MSG* pMsg);

		void FillParameters();
		void SelectParameters();
		void Enable(BOOL bEnable);

		void OnSelectionChange(NMHDR* pNMHDR, LRESULT* pResult);
		LRESULT OnCheckbox(WPARAM wParam, LPARAM lParam);

		DECLARE_MESSAGE_MAP()
		afx_msg void UpdateCtrl(void);
		afx_msg void OnDestroy();
		afx_msg void OnEnable(BOOL bEnable);
		afx_msg void OnGeneratioTypeChange();
		afx_msg void OnNbVariationChange();
	};


	//********************************************************************************************************************


}