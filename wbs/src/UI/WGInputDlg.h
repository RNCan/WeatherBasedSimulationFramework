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



#include "ModelBase/WGInput.h"
#include "ModelBase/Model.h"
#include "UI/Common/UtilWin.h"
#include "UI/Common/CommonCtrl.h"
#include "UI/Common/MyToolTipCtrl.h"
#include "WVariablesEdit.h"


namespace WBSF
{


	/////////////////////////////////////////////////////////////////////////////
	// CWGInputDlg dialog

	class CWGInputDlg : public CDialog
	{
		// Construction
	public:
		CWGInputDlg(CWnd* pParent = NULL);   // standard constructor

		enum { IDD = IDD_WG_INPUT };

		void SetWGInput(const CWGInput& WGInput);
		void GetWGInput(CWGInput& WGInput);
		void UpdateCtrl(bool bEnable);

		// Overrides
		virtual BOOL Create(const CModel& model, CWnd* pParentWnd);
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		virtual BOOL OnInitDialog();
		virtual void OnOK();
		virtual void OnCancel();
		virtual BOOL PreTranslateMessage(MSG* pMsg);
		// Implementation
	protected:

		// Generated message map functions
		DECLARE_MESSAGE_MAP()
		afx_msg void OnDestroy();
		afx_msg void OnNormalsLink();
		afx_msg void OnDailyLink();
		afx_msg void OnHourlyLink();
		afx_msg void OnGribsLink();
		afx_msg void OnSearchRadiusClick();
		afx_msg void OnEnable(BOOL bEnable);
		afx_msg void UpdateCtrl();
		


		void FillNormalsDBNameList();
		void FillDailyDBNameList();
		void FillHourlyDBNameList();
		void FillGribsDBNameList();

		CWnd& GetStaticCtrl(int staticNo)
		{
			CWnd* pWnd = GetDlgItem(IDC_CMN_STATIC1 + staticNo - 1); 
			
			ASSERT(pWnd);
			return (CWnd&)*pWnd;
		}

		//bool m_bDefaultModel;
		CModel m_model;
		CSearchRadius m_searchRadius;

		CWVariablesEdit m_variablesCtrl;
		CCFLComboBox m_sourceTypeCtrl;
		CCFLComboBox m_generationTypeCtrl;

		CCFLComboBox m_normalDBNameCtrl;
		CMFCButton m_normalLinkCtrl;
		CCFLEdit m_normalsNbStationsCtrl;

		CCFLComboBox m_dailyDBNameCtrl;
		CMFCButton m_dailyLinkCtrl;
		CCFLEdit m_dailyNbStationsCtrl;

		CCFLComboBox m_hourlyDBNameCtrl;
		CMFCButton m_hourlyLinkCtrl;
		CCFLEdit m_hourlyNbStationsCtrl;

		CCFLComboBox m_gribsDBNameCtrl;
		CMFCButton m_gribsLinkCtrl;
		CButton m_useGribCtrl;
		CButton m_atSurfaceCtrl;

		CCFLEdit m_nbYearsCtrl;
		CCFLEdit m_firstYearCtrl;
		CCFLEdit m_lastYearCtrl;
		CButton m_useForecastCtrl;
		CButton m_useRadarPrcpCtrl;

		CCFLComboBox m_albedoTypeCtrl;
		CCFLComboBox m_seedTypeCtrl;
		CWVariablesEdit m_allowedDerivedVariablesCtrl;
		CButton m_XValidationCtrl;
		CButton m_skipVerifyCtrl;
		CButton m_noFillMissingCtrl;

		//CXInfoTip m_testToolTips;
		CMyToolTipCtrl m_testToolTips;

		static BOOL CALLBACK EnableChildWindow(HWND hwndChild, LPARAM lParam);
		
	};



}