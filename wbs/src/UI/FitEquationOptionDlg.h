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


#include "Simulation/Dispersal.h"
#include "UI/Common/CommonCtrl.h"

#include "WeatherBasedSimulationUI.h"

namespace WBSF
{


	class CFitInputParamPropertyGridCtrl : public CMFCPropertyGridCtrl
	{
	public:

		CFitInputParamPropertyGridCtrl();
			


		void EnableProperties(BOOL bEnable);
		void EnableProperties(CMFCPropertyGridProperty* pProp, BOOL bEnable);

		virtual BOOL PreTranslateMessage(MSG* pMsg);
		virtual BOOL ValidateItemData(CMFCPropertyGridProperty* /*pProp*/);
		virtual void Init();

		void Set(CSAParametersMap* in) { m_eq_options = in; }

	protected:

		CSAParametersMap* m_eq_options;

		DECLARE_MESSAGE_MAP()
	
	};



	class CFitInputParamDlg : public CDialogEx
	{
	public:

		//enum TFit { F_DEV_TIME, F_SURVIVAL, F_FECUNDITY, F_STAGES_TABLE, NB_FIT_TYPE };

		CFitInputParamDlg(size_t type, CWnd* pParentDlg = NULL);
		~CFitInputParamDlg();

		CSAParametersMap m_eq_options;

	protected:

		enum { IDD = IDD_SIM_FIT_EQUATION_OPTIONS};

		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		DECLARE_MESSAGE_MAP()
		void OnSize(UINT nType, int cx, int cy);
		void OnDestroy();
		void AdjustLayout();
		afx_msg void OnBnClickedResetDefault();
		afx_msg LRESULT OnPropertyChanged(__in WPARAM wparam, __in LPARAM lparam);

		size_t m_fitType;
		CFitInputParamPropertyGridCtrl m_propertiesCtrl;
	
		
	};

}