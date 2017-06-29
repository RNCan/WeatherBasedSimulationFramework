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

//#include "Basic.h"
#include "Simulation/Analysis.h"
#include "UI/Common/UtilWin.h"
#include "UI/Common/HtmlTree/XHtmlTree.h"
#include "UI/Common/CommonCtrl.h"
#include "WeatherBasedSimulationUI.h"


namespace WBSF
{


	class CTemporalReferenceEdit : public CMFCMaskedEdit
	{
	public:


		CTemporalReferenceEdit(CTM TM = CTM(CTM::DAILY));

		virtual void PreSubclassWindow();
		CTRef GetTemporalReference()const;
		void SetTemporalReference(CTRef ref);

		CTM m_TM;
		void UpdateMask();

		DECLARE_MESSAGE_MAP()
	};


	/////////////////////////////////////////////////////////////////////////////
	// CAnalysisGeneralPage dialog
	class CAnalysisPage : public CMFCPropertyPage
	{
	public:
		CAnalysisPage(UINT nIDTemplate, UINT nIDCaption = 0);


		~CAnalysisPage();

		virtual void Initialise(const CExecutablePtr& pParent, CAnalysis& analysis);


	protected:
		DECLARE_MESSAGE_MAP()

		CExecutablePtr m_pParent;
		CAnalysis* m_pAnalysis;

		CParentInfo m_info;

	private:


	};


	class CAnalysisGeneralPage : public CAnalysisPage
	{
		//	DECLARE_DYNCREATE(CAnalysisGeneralPage)

		// Construction
	public:
		CAnalysisGeneralPage();
		~CAnalysisGeneralPage();



		// Dialog Data
		enum { IDD = IDD_SIM_ANALYSIS_GENERAL };

		// Overrides
	protected:
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

		// Implementation
	protected:
		DECLARE_MESSAGE_MAP()

		void GetAnalysisFromInterface(CAnalysis& analysis);
		void SetAnalysisToInterface(const CAnalysis& analysis);

		CCFLEdit m_nameCtrl;
		CCFLEdit m_internalNameCtrl;
		CCFLEdit m_descriptionCtrl;
	};


	/////////////////////////////////////////////////////////////////////////////
	// CAnalysisWherePage dialog

	class CAnalysisWherePage : public CAnalysisPage
	{
		//	DECLARE_DYNCREATE(CAnalysisWherePage)

		// Construction
	public:
		enum { IDD = IDD_SIM_ANALYSIS_WHERE };

		CAnalysisWherePage();
		~CAnalysisWherePage();



		// Overrides
	protected:

		DECLARE_MESSAGE_MAP()

		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

		void GetAnalysisFromInterface(CAnalysis& analysis);
		void SetAnalysisToInterface(const CAnalysis& analysis);
		void FillLocation();
		void UpdateCtrl();

		CButton m_selectCtrl;
		CSelectionCtrl m_locationCtrl;


		afx_msg void OnSelectChange();
	};


	/////////////////////////////////////////////////////////////////////////////
	// CAnalysisWhenPage dialog

	class CAnalysisWhenPage : public CAnalysisPage
	{
	public:
		enum { IDD = IDD_SIM_ANALYSIS_WHEN };

		CAnalysisWhenPage();
		~CAnalysisWhenPage();



		// Implementation
	protected:
		DECLARE_MESSAGE_MAP()

		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		afx_msg void OnUpdateCtrl(NMHDR *pNMHDR, LRESULT *pResult);

		void InitialiseCtrl();
		void GetAnalysisFromInterface(CAnalysis& analysis);
		void SetAnalysisToInterface(const CAnalysis& analysis);
		void UpdateCtrl();

		CButton m_selectPeriodCtrl;
		CTemporalReferenceEdit m_beginCtrl;
		CTemporalReferenceEdit m_endCtrl;

		CButton m_useCurrentDataCtrl1;
		CCFLEdit m_shiftCtrl1;
		CButton m_useCurrentDataCtrl2;
		CCFLEdit m_shiftCtrl2;

		CMFCButton m_continuousCtrl;
		CMFCButton m_yearByYearCtrl;
		short m_inversed;

		static const short NB_STATIC = 4;
		CWnd& GetStatic(short i){ return *GetDlgItem(IDC_CMN_STATIC1 + i); }

	};



	/////////////////////////////////////////////////////////////////////////////
	// CAnalysisWhatPage dialog

	class CAnalysisWhatPage : public CAnalysisPage
	{
	public:

		enum { IDD = IDD_SIM_ANALYSIS_WHAT };

		CAnalysisWhatPage();
		~CAnalysisWhatPage();


	protected:

		DECLARE_MESSAGE_MAP()

		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		void GetAnalysisFromInterface(CAnalysis& analysis);
		void SetAnalysisToInterface(const CAnalysis& analysis);
		void FillParametersVariations();
		void UpdateCtrl();

		CButton m_selectCtrl;
		CSelectionCtrl m_parametersetCtrl;
	};

	/////////////////////////////////////////////////////////////////////////////
	// CAnalysisWhichPage dialog

	class CAnalysisWhichPage : public CAnalysisPage
	{
	public:

		enum { IDD = IDD_SIM_ANALYSIS_WHICH };

		CAnalysisWhichPage();
		~CAnalysisWhichPage();


	protected:

		DECLARE_MESSAGE_MAP()

		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		void GetAnalysisFromInterface(CAnalysis& analysis);
		void SetAnalysisToInterface(const CAnalysis& analysis);
		void FillVariable();
		void UpdateCtrl();


		CButton m_selectCtrl;
		CSelectionCtrl m_variableCtrl;


	};

	class CMFCImage : public CMFCButton
	{
	protected:
		DECLARE_MESSAGE_MAP()
		afx_msg void OnLButtonDown(UINT nFlags, CPoint point){}
		afx_msg void OnMouseMove(UINT nFlags, CPoint point){}

	};

	class CAnalysisHowPage : public CAnalysisPage
	{
	public:
		enum { IDD = IDD_SIM_ANALYSIS_HOW };

		CAnalysisHowPage();
		~CAnalysisHowPage();

	protected:

		DECLARE_MESSAGE_MAP()

		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		virtual void OnOK();


		afx_msg void OnTTypeChange();
		afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);


		void GetAnalysisFromInterface(CAnalysis& analysis);
		void SetAnalysisToInterface(const CAnalysis& analysis);
		short GetComputationKind();
		void SetComputationKind(short type);
		void UpdateCtrl();
		void FillType();
		void FillMode(short type);
		CTM GetTM();
		void SetTM(CTM TM);

		CStatisticComboBox m_previousStatisticCtrl;
		CButton m_statisticCtrl;
		CButton m_eventCtrl;
		CCFLComboBox m_CTTypeCtrl;
		CCFLComboBox m_CTModeCtrl;
		CCFLComboBox m_TTTypeCtrl;
		CCFLComboBox m_TTModeCtrl;
		CStatisticComboBox  m_statisticTypeCtrl;
		CCFLComboBox m_eventTypeCtrl;
		CCFLEdit m_KCtrl;
		CAutoEnableStatic m_selectCtrl;

		CButton m_meanOverReplicationCtrl;
		CButton m_meanOverParameterSetCtrl;
		CButton m_meanOverLocationCtrl;
		CMFCImage m_imageModeCtrl;
		CTM m_sourceTM;

		static const short NB_STATIC = 4;
		CWnd& GetStatic(short i){ return *GetDlgItem(IDC_CMN_STATIC1 + i); }
	
	};

}