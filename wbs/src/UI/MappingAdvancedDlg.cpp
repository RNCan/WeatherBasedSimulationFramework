//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
//****************************************************************************
#include "stdafx.h"

#include "Geomatic/GridInterpol.h"
#include "UI/Common/UtilWin.h"
#include "MappingAdvancedDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace WBSF
{

	/////////////////////////////////////////////////////////////////////////////
	// CMappingAdvancedDlg dialog


	CMappingAdvancedDlg::CMappingAdvancedDlg(CWnd* pParentWnd/*=NULL*/) :
		CDialog(CMappingAdvancedDlg::IDD, pParentWnd)
	{
		m_pParam.reset(new CGridInterpolParam);
		m_method = -1;
	}


	void CMappingAdvancedDlg::DoDataExchange(CDataExchange* pDX)
	{
		CDialog::DoDataExchange(pDX);
		

		DDX_Control(pDX, IDC_MAP_NBPOINT, m_nbPointCtrl);
		DDX_Control(pDX, IDC_MAP_NO_DATA, m_noDataCtrl);
		DDX_Control(pDX, IDC_MAP_MAX_DISTANCE, m_maxDistanceCtrl);
		DDX_Control(pDX, IDC_MAP_XVAL_POINTS, m_XvalPointCtrl);
		DDX_Control(pDX, IDC_MAP_OUTPUT_TYPE, m_outputTypeCtrl);
		
		DDX_Control(pDX, IDC_MAP_USE_ELEVATION, m_useElevationCtrl);
		DDX_Control(pDX, IDC_MAP_USE_EXPOSITION, m_useExpositionCtrl);
		DDX_Control(pDX, IDC_MAP_USE_SHORE_DISTANCE, m_useShoreCtrl);
		DDX_Control(pDX, IDC_MAP_OPTIONS, m_GDALOptionsCtrl);
		DDX_Control(pDX, IDC_MAP_GLOBAL_LIMIT, m_globalLimitCtrl);
		DDX_Control(pDX, IDC_MAP_GLOBAL_LIMIT_SD, m_globalLimitSDCtrl);
		DDX_Control(pDX, IDC_MAP_GLOBAL_LIMIT_TO_BOUND, m_globalLimitToBoundCtrl);

		DDX_Control(pDX, IDC_MAP_GLOBAL_MINMAX_LIMIT, m_globalLimitMinMaxCtrl);
		DDX_Control(pDX, IDC_MAP_GLOBAL_LIMIT_MIN, m_globalMinLimitCtrl);
		DDX_Control(pDX, IDC_MAP_GLOBAL_LIMIT_MAX, m_globalMaxLimitCtrl);
		DDX_Control(pDX, IDC_MAP_GLOBAL_MINMAX_LIMIT_TO_BOUND, m_globalMinMaxLimitToBoundCtrl);

		//Spatial Regression
		DDX_Control(pDX, IDC_MAP_REGRESSION_MODEL, m_regressOptCtrl);
		DDX_Control(pDX, IDC_MAP_ADD_TERM, m_R²Ctrl);

		//IDW
		DDX_Control(pDX, IDC_MAP_IWD_MODEL, m_IWDModelCtrl);
		DDX_Control(pDX, IDC_MAP_POWER, m_powerCtrl);

		//Kriging
		DDX_Control(pDX, IDC_MAP_VARIOGRAM_MODEL, m_variogramModelCtrl);
		DDX_Control(pDX, IDC_MAP_LAGDIST, m_lagDistanceCtrl);
		DDX_Control(pDX, IDC_MAP_NBLAGS, m_nbLagsCtrl);
		DDX_Control(pDX, IDC_MAP_DETRENDING, m_detrendingCtrl);
		DDX_Control(pDX, IDC_MAP_EXTERNAL_DRIFT, m_externalDriftCtrl);
		DDX_Control(pDX, IDC_MAP_OUTPUT_VARIOGRAM_INFO, m_outputVariogramCtrl);
		DDX_Control(pDX, IDC_MAP_REGIONAL_LIMIT, m_regionalLimitCtrl);
		DDX_Control(pDX, IDC_MAP_REGIONAL_LIMIT_SD, m_regionalLimitSDCtrl);
		DDX_Control(pDX, IDC_MAP_REGIONAL_LIMIT_TO_BOUND, m_regionalLimitToBoundCtrl);

		//Thin Plate Spline
		DDX_Control(pDX, IDC_MAP_TPS_MAX_ERROR, m_TPSMaxErrorCtrl);

		//Random Forest
		DDX_Control(pDX, IDC_MAP_RANDOM_FOREST_MODEL, m_RFTreeTypeCtrl);

		//static
		DDX_Control(pDX, IDC_CMN_STATIC1, m_static1Ctrl);
		DDX_Control(pDX, IDC_CMN_STATIC2, m_static2Ctrl);
		DDX_Control(pDX, IDC_CMN_STATIC3, m_static3Ctrl);
		DDX_Control(pDX, IDC_CMN_STATIC4, m_static4Ctrl);
		DDX_Control(pDX, IDC_CMN_STATIC5, m_static5Ctrl);


		if (pDX->m_bSaveAndValidate)
			GetFromInterface();
		else
			SetToInterface();


	}



	BEGIN_MESSAGE_MAP(CMappingAdvancedDlg, CDialog)
		ON_CBN_SELCHANGE(IDC_MAP_VARIOGRAM_MODEL, &UpdateCtrl)
		ON_CBN_SELCHANGE(IDC_MAP_EXTERNAL_DRIFT, &UpdateCtrl)
	END_MESSAGE_MAP()

	/////////////////////////////////////////////////////////////////////////////
	// CMappingAdvancedDlg msg handlers




	BOOL CMappingAdvancedDlg::OnInitDialog()
	{
		CDialog::OnInitDialog();
		UpdateCtrl();

		return TRUE;  // return TRUE unless you set the focus to a control
		// EXCEPTION: OCX Property Pages should return FALSE
	}

	void CMappingAdvancedDlg::UpdateCtrl()
	{
		bool bEnableRegression = m_method == CGridInterpol::SPATIAL_REGRESSION;
		bool bEnableKriging = m_method == CGridInterpol::UNIVERSAL_KRIGING;
		bool bEnableIWD = m_method == CGridInterpol::INVERT_WEIGHTED_DISTANCE;
		bool bEnableTPS = m_method == CGridInterpol::THIN_PLATE_SPLINE;
		bool bEnableRF = m_method == CGridInterpol::RANDOM_FOREST;

		m_nbPointCtrl.EnableWindow(bEnableKriging || bEnableIWD);
		m_maxDistanceCtrl.EnableWindow(bEnableKriging || bEnableIWD || bEnableTPS);
		bool bEnableStepWiseR²= bEnableKriging && (m_externalDriftCtrl.GetCurSel() - 1) == CGridInterpolParam::ED_STEPWISE;
		
		m_static1Ctrl.EnableWindow(bEnableRegression| bEnableStepWiseR², TRUE);
		m_static2Ctrl.EnableWindow(bEnableKriging, TRUE);
		m_static3Ctrl.EnableWindow(bEnableIWD, TRUE);
		m_static4Ctrl.EnableWindow(bEnableTPS, TRUE);
		m_static5Ctrl.EnableWindow(bEnableRF, TRUE);

		m_regionalLimitCtrl.EnableWindow(bEnableKriging);

		//bool bEnableUseElevation = true;
		bool bEnableUseExposition = !bEnableKriging && !bEnableIWD;
		//bool bEnableUseShore = !bEnableKriging;
		//m_useElevationCtrl.EnableWindow(bEnableUseElevation);
		m_useExpositionCtrl.EnableWindow(bEnableUseExposition);
		//m_useShoreCtrl.EnableWindow(bEnableUseShore);


	}

	void CMappingAdvancedDlg::GetFromInterface()
	{
		//General param
		m_pParam->m_nbPoints = ToInt(m_nbPointCtrl.GetString());
		m_pParam->m_noData = ToFloat(m_noDataCtrl.GetString());
		m_pParam->m_maxDistance = ToFloat(m_maxDistanceCtrl.GetString()) * 1000.0f;
		m_pParam->m_XvalPoints = ToFloat(m_XvalPointCtrl.GetString())/100.0f;
		m_pParam->m_outputType = m_outputTypeCtrl.GetCurSel();
		m_pParam->m_bUseElevation = m_useElevationCtrl.GetCheck();
		m_pParam->m_bUseExposition = m_useExpositionCtrl.GetCheck();
		m_pParam->m_bUseShore = m_useShoreCtrl.GetCheck();
		m_pParam->m_GDALOptions = m_GDALOptionsCtrl.GetString();

		m_pParam->m_bGlobalLimit = m_globalLimitCtrl.GetCheck();
		m_pParam->m_globalLimitSD = ToFloat(m_globalLimitSDCtrl.GetString());
		m_pParam->m_bGlobalLimitToBound = m_globalLimitToBoundCtrl.GetCheck();
		m_pParam->m_bGlobalMinMaxLimit = m_globalLimitMinMaxCtrl.GetCheck();
		m_pParam->m_globalMinLimit = ToFloat(m_globalMinLimitCtrl.GetString());
		m_pParam->m_globalMaxLimit = ToFloat(m_globalMaxLimitCtrl.GetString());
		m_pParam->m_bGlobalMinMaxLimitToBound = m_globalMinMaxLimitToBoundCtrl.GetCheck();


		//Spatial Regression
		m_pParam->m_regressOptimization = m_regressOptCtrl.GetCurSel();
		m_pParam->m_regressCriticalR2 = ToDouble(m_R²Ctrl.GetString());
		
		//Kriging
		m_pParam->m_variogramModel = m_variogramModelCtrl.GetCurSel() - 1;
		m_pParam->m_lagDist = ToDouble(m_lagDistanceCtrl.GetString());
		m_pParam->m_nbLags = ToInt(m_nbLagsCtrl.GetString());
		m_pParam->m_detrendingModel = m_detrendingCtrl.GetCurSel() - 1;
		m_pParam->m_externalDrift = m_externalDriftCtrl.GetCurSel() - 1;
		m_pParam->m_bOutputVariogramInfo = m_outputVariogramCtrl.GetCheck();
		m_pParam->m_bRegionalLimit = m_regionalLimitCtrl.GetCheck();
		m_pParam->m_regionalLimitSD = ToFloat(m_regionalLimitSDCtrl.GetString());
		m_pParam->m_bRegionalLimitToBound = m_regionalLimitToBoundCtrl.GetCheck();

		//IWD
		m_pParam->m_IWDModel = m_IWDModelCtrl.GetCurSel() - 1;
		m_pParam->m_power = ToDouble(m_powerCtrl.GetString());

		//TPS
		m_pParam->m_TPSMaxError = ToFloat(m_TPSMaxErrorCtrl.GetString());

		//Random Forest
		m_pParam->m_RFTreeType = m_RFTreeTypeCtrl.GetCurSel();

	}

	void CMappingAdvancedDlg::SetToInterface()
	{
		//General param
		m_nbPointCtrl.SetWindowText(ToString(m_pParam->m_nbPoints));
		m_noDataCtrl.SetWindowText(ToString(m_pParam->m_noData));
		m_maxDistanceCtrl.SetWindowText(ToString(m_pParam->m_maxDistance / 1000.0f, 0));
		m_XvalPointCtrl.SetWindowText(ToString(m_pParam->m_XvalPoints * 100.0f, 2));
		m_outputTypeCtrl.SetCurSel((int)m_pParam->m_outputType);
		m_useElevationCtrl.SetCheck(m_pParam->m_bUseElevation);
		m_useExpositionCtrl.SetCheck(m_pParam->m_bUseExposition);
		m_useShoreCtrl.SetCheck(m_pParam->m_bUseShore);
		m_GDALOptionsCtrl.SetWindowText(m_pParam->m_GDALOptions);

		m_globalLimitCtrl.SetCheck(m_pParam->m_bGlobalLimit);
		m_globalLimitSDCtrl.SetWindowText(ToString(m_pParam->m_globalLimitSD));
		m_globalLimitToBoundCtrl.SetCheck(m_pParam->m_bGlobalLimitToBound);
		m_globalLimitMinMaxCtrl.SetCheck(m_pParam->m_bGlobalMinMaxLimit);
		m_globalMinLimitCtrl.SetWindowText(ToString(m_pParam->m_globalMinLimit));
		m_globalMaxLimitCtrl.SetWindowText(ToString(m_pParam->m_globalMaxLimit));
		m_globalMinMaxLimitToBoundCtrl.SetCheck(m_pParam->m_bGlobalMinMaxLimitToBound);


		//Spatial Regression
		m_regressOptCtrl.SetCurSel(int(m_pParam->m_regressOptimization));
		m_R²Ctrl.SetWindowText(ToString(m_pParam->m_regressCriticalR2,10));

		//Kriging
		m_variogramModelCtrl.SetCurSel(m_pParam->m_variogramModel + 1);
		m_lagDistanceCtrl.SetWindowText(ToString(m_pParam->m_lagDist, 6));
		m_nbLagsCtrl.SetWindowText(ToString(m_pParam->m_nbLags));
		m_detrendingCtrl.SetCurSel(m_pParam->m_detrendingModel + 1);
		m_externalDriftCtrl.SetCurSel(m_pParam->m_externalDrift + 1);
		m_outputVariogramCtrl.SetCheck(m_pParam->m_bOutputVariogramInfo);
		m_regionalLimitCtrl.SetCheck(m_pParam->m_bRegionalLimit);
		m_regionalLimitSDCtrl.SetWindowText(ToString(m_pParam->m_regionalLimitSD));
		m_regionalLimitToBoundCtrl.SetCheck(m_pParam->m_bRegionalLimitToBound);

		//IWD
		m_IWDModelCtrl.SetCurSel(m_pParam->m_IWDModel + 1);
		if (m_pParam->m_power > 0)
			m_powerCtrl.SetWindowText(ToString(m_pParam->m_power));
		else m_powerCtrl.SetCurSel(0); //best power

		//TPS
		m_TPSMaxErrorCtrl.SetWindowText(ToString(m_pParam->m_TPSMaxError, 10));
		
		//Random Forest
		m_RFTreeTypeCtrl.SetCurSel((int)m_pParam->m_RFTreeType);
	}



}