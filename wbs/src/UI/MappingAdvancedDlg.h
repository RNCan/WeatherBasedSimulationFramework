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


#include "UI/Common/StaticEx.h"
#include "UI/Common/CommonCtrl.h"

#include "WeatherBasedSimulationUI.h"

namespace WBSF
{

	class CGridInterpolParam;
	/////////////////////////////////////////////////////////////////////////////
	// CMappingAdvancedDlg dialog

	class CMappingAdvancedDlg : public CDialog
	{
		// Construction
	public:
		CMappingAdvancedDlg(CWnd* pParentWnd);   // standard constructor

		int m_method;
		std::unique_ptr<CGridInterpolParam> m_pParam;


	protected:

		// Dialog Data
		enum { IDD = IDD_MAP_GRID_INTERPOL };

		CCFLEdit	m_nbPointCtrl;
		CCFLEdit	m_noDataCtrl;
		CCFLEdit	m_maxDistanceCtrl;
		CCFLEdit	m_XvalPointCtrl;
		CCFLEdit	m_GDALOptionsCtrl;
		CButton		m_useElevationCtrl;
		CButton		m_useExpositionCtrl;
		CButton		m_useShoreCtrl;

		//Regression
		//CComboBox	m_regressionModelCtrl;

		//Kriging
		CComboBox	m_variogramModelCtrl;
		CCFLEdit	m_lagDistanceCtrl;
		CCFLEdit	m_nbLagsCtrl;
		CComboBox	m_detrendingCtrl;
		CComboBox	m_externalDriftCtrl;
		CButton		m_outputVariogramCtrl;

		
		CAutoEnableStatic m_regionalLimitCtrl;
		CCFLEdit	m_regionalLimitSDCtrl;
		CButton		m_regionalLimitToBoundCtrl;


		CAutoEnableStatic m_globalLimitCtrl;
		CCFLEdit	m_globalLimitSDCtrl;
		CButton		m_globalLimitToBoundCtrl;
		CAutoEnableStatic m_globalLimitMinMaxCtrl;
		CCFLEdit	m_globalMinLimitCtrl;
		CCFLEdit	m_globalMaxLimitCtrl;
		CButton		m_globalMinMaxLimitToBoundCtrl;

		//IWD	
		CCFLEdit	m_R²Ctrl;
		CComboBox	m_IWDModelCtrl;
		CCFLComboBox	m_powerCtrl;

		//TPS
		CCFLComboBox	m_TPSMaxErrorCtrl;

		//Random Forest
		CComboBox	m_RFTreeTypeCtrl;


		CStaticEx	m_static1Ctrl;
		CStaticEx	m_static2Ctrl;
		CStaticEx	m_static3Ctrl;
		CStaticEx	m_static4Ctrl;
		CStaticEx	m_static5Ctrl;

		virtual BOOL OnInitDialog();
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

		void UpdateCtrl();
		void GetFromInterface();
		void SetToInterface();

		// Generated message map functions
		DECLARE_MESSAGE_MAP()

	};

}