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

#include "Basic/Location.h"
#include "UI/Common/CommonCtrl.h"
#include "UI/Common/GeoRectCtrl.h"
#include "UI/WVariablesEdit.h"
#include "WeatherBasedSimulationUI.h"


namespace WBSF
{


	/////////////////////////////////////////////////////////////////////////////
	// CLOCGeneratorDlg dialog

	class CLOCGeneratorDlg : public CDialog
	{
		// Construction
	public:

		enum TGenerationFrom{ FROM_DEM, FROM_WEATHER, NB_GENERATION_FROM };
		enum TWeatherStation{ NORMAL, DAILY, HOURLY, NB_WEATHER_STATION };
		enum TGenerationMethod{ REGULAR, RANDOM, NB_METHOD };
		enum TWeatherGenerationMethod{ ALL_STATIONS, MOST_COMPLETE_STATIONS, WELL_DISTRIBUTED_STATIONS, COMPLETE_AND_DISTRIBUTED_STATIONS, NB_WEATHER_METHOD };
		enum TExposition{ NO_EXPO, EXPO_FROM_DEM, EXPO_GENERATE, NB_EXPOSITION };
		enum TExpositionGeneration{ EXPO_UNIFORM, EXPO_NORMAL, NB_EXPOSITION_GENERATION };


		CLOCGeneratorDlg(CWnd* pParent = NULL);   // standard constructor

		const CLocationVector& GetLOC()const{ return m_locArray; }

	protected:
		// Dialog Data

		enum { IDD = IDD_SIM_LOC_GENERATE };
		CIntEdit	m_nbPointCtrl;
		CIntEdit	m_nbStationCtrl;
		CIntEdit	m_nbPointLatCtrl;
		CIntEdit	m_nbPointLonCtrl;
		CIntEdit	m_yearCtrl;
		CAutoEnableStatic m_exposureCtrl;
		CWVariablesEdit m_filterCtrl;

		int		m_exposition;
		int		m_generateFrom;
		int		m_generationMethod;
		int		m_weatherGenerationMethod;
		int		m_weatherStation;
		int		m_year;
		int		m_nbPoint;
		int		m_nbPointLat;
		int		m_nbPointLon;
		int		m_nbStations;
		int		m_genType;
		int		m_bElevExtrem;
		float	m_factor;
		bool	m_useBoundingBox;
		WBSF::CGeoRect m_rect;
		CString m_DEMName;
		CString m_normalDBName;
		CString m_dailyDBName;
		CString m_hourlyDBName;
		CWVariables m_filter;


		// Generated message map functions

		virtual BOOL OnInitDialog();
		virtual void OnOK();
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

		afx_msg void OnSelectDEM();
		afx_msg void OnGenerationMethodChange();
		afx_msg void OnGenerateFromChange();
		afx_msg void OnExpoTypeChange();
		afx_msg void OnDestroy();
		DECLARE_MESSAGE_MAP()


		void UpdateCtrl();
		void FillDEMList();
		void FillNormalDBList();
		void FillDailyDBList();
		void FillHourlyDBList();

		ERMsg GenerateFromDEM();
		ERMsg VerifyParameter();
		ERMsg GenerateFromWeatherStation();

		TGenerationFrom GetGenerateFromType()
		{
			ASSERT(GetGenerationFromCtrl().GetCurSel() >= 0 && GetGenerationFromCtrl().GetCurSel() < NB_GENERATION_FROM);
			return (TGenerationFrom)GetGenerationFromCtrl().GetCurSel();
		}

		TWeatherStation GetWeatherStationType()
		{
			ASSERT(GetWeatherStationTypeCtrl().GetCurSel() >= 0 && GetWeatherStationTypeCtrl().GetCurSel() < NB_WEATHER_STATION);
			return (TWeatherStation)GetWeatherStationTypeCtrl().GetCurSel();
		}
		TGenerationMethod GetGenerationMethod()
		{
			ASSERT(GetGenerationMethodCtrl().GetCurSel() >= 0 && GetGenerationMethodCtrl().GetCurSel() < NB_METHOD);
			return (TGenerationMethod)GetGenerationMethodCtrl().GetCurSel();
		}

		TWeatherGenerationMethod GetWeatherGenerationMethod()
		{
			ASSERT(GetWeatherGenerationMethodCtrl().GetCurSel() >= 0 && GetWeatherGenerationMethodCtrl().GetCurSel() < NB_WEATHER_METHOD);
			return (TWeatherGenerationMethod)GetWeatherGenerationMethodCtrl().GetCurSel();
		}

		CCFLComboBox& GetGenerationFromCtrl()
		{
			ASSERT(GetDlgItem(IDC_GENLOC_FROM));
			return (CCFLComboBox&)*GetDlgItem(IDC_GENLOC_FROM);
		}
		CCFLComboBox& GetGenerationMethodCtrl()
		{
			ASSERT(GetDlgItem(IDC_GENLOC_METHOD));
			return (CCFLComboBox&)*GetDlgItem(IDC_GENLOC_METHOD);
		}
		CCFLComboBox& GetWeatherGenerationMethodCtrl()
		{
			ASSERT(GetDlgItem(IDC_GENLOC_METHOD_W));
			return (CCFLComboBox&)*GetDlgItem(IDC_GENLOC_METHOD_W);
		}
		CCFLComboBox& GetWeatherStationTypeCtrl()
		{
			ASSERT(GetDlgItem(IDC_GENLOC_STATION_TYPE));
			return (CCFLComboBox&)*GetDlgItem(IDC_GENLOC_STATION_TYPE);
		}

		CCFLComboBox& GetNormalDBNameCtrl()
		{
			ASSERT(GetDlgItem(IDC_GENLOC_NORMAL_NAME));
			return (CCFLComboBox&)*GetDlgItem(IDC_GENLOC_NORMAL_NAME);
		}

		CCFLComboBox& GetDailyDBNameCtrl()
		{
			ASSERT(GetDlgItem(IDC_GENLOC_DAILY_NAME));
			return (CCFLComboBox&)*GetDlgItem(IDC_GENLOC_DAILY_NAME);
		}
		CCFLComboBox& GetHourlyDBNameCtrl()
		{
			ASSERT(GetDlgItem(IDC_GENLOC_HOURLY_NAME));
			return (CCFLComboBox&)*GetDlgItem(IDC_GENLOC_HOURLY_NAME);
		}
		CCFLComboBox& GetFilterCtrl()
		{
			ASSERT(GetDlgItem(IDC_GENLOC_FILTER));
			return (CCFLComboBox&)*GetDlgItem(IDC_GENLOC_FILTER);
		}

		CStatic& GetStaticCtrl(int i)
		{
			ASSERT(i >= 1 && i <= 10);
			ASSERT(GetDlgItem(IDC_CMN_STATIC1 + i - 1));
			return (CStatic&)*GetDlgItem(IDC_CMN_STATIC1 + i - 1);
		}

		CCFLComboBox& GetDEMNameCtrl()
		{
			ASSERT(GetDlgItem(IDC_GENLOC_DEM_NAME));
			return (CCFLComboBox&)*GetDlgItem(IDC_GENLOC_DEM_NAME);
		}

		CButton& GetDEMBrowseCtrl()
		{
			ASSERT(GetDlgItem(IDC_GENLOC_SELECTDEM));
			return (CButton&)*GetDlgItem(IDC_GENLOC_SELECTDEM);
		}

		CButton& GetDEMExpositionCtrl()
		{
			ASSERT(GetDlgItem(IDC_GENLOC_EXPOSITION_FROM_DEM));
			return (CButton&)*GetDlgItem(IDC_GENLOC_EXPOSITION_FROM_DEM);
		}

		CButton& GetUniformExpositionCtrl()
		{
			ASSERT(GetDlgItem(IDC_GENLOC_UNIFORM));
			return (CButton&)*GetDlgItem(IDC_GENLOC_UNIFORM);
		}

		CButton& GetNormalExpositionCtrl()
		{
			ASSERT(GetDlgItem(IDC_GENLOC_NORMAL));
			return (CButton&)*GetDlgItem(IDC_GENLOC_NORMAL);
		}


		CLocationVector m_locArray;
		CGeoRectCtrl m_rectCtrl;



	};

}