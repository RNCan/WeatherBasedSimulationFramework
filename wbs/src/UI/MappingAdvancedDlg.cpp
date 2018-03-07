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
		
		DDX_Control(pDX, IDC_MAP_OPTIONS, m_GDALOptionsCtrl);

		//Regrssion
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

		DDX_Control(pDX, IDC_MAP_GLOBAL_LIMIT, m_globalLimitCtrl);
		DDX_Control(pDX, IDC_MAP_GLOBAL_LIMIT_SD, m_globalLimitSDCtrl);
		DDX_Control(pDX, IDC_MAP_GLOBAL_LIMIT_TO_BOUND, m_globalLimitToBoundCtrl);

		DDX_Control(pDX, IDC_MAP_GLOBAL_MINMAX_LIMIT, m_globalLimitMinMaxCtrl);
		DDX_Control(pDX, IDC_MAP_GLOBAL_LIMIT_MIN, m_globalMinLimitCtrl);
		DDX_Control(pDX, IDC_MAP_GLOBAL_LIMIT_MAX, m_globalMaxLimitCtrl);
		DDX_Control(pDX, IDC_MAP_GLOBAL_MINMAX_LIMIT_TO_BOUND, m_globalMinMaxLimitToBoundCtrl);


		//Thin Plate Spline
		DDX_Control(pDX, IDC_MAP_TPS_MAX_ERROR, m_TPSMaxErrorCtrl);

		DDX_Control(pDX, IDC_CMN_STATIC1, m_static1Ctrl);
		DDX_Control(pDX, IDC_CMN_STATIC2, m_static2Ctrl);
		DDX_Control(pDX, IDC_CMN_STATIC3, m_static3Ctrl);
		DDX_Control(pDX, IDC_CMN_STATIC4, m_static4Ctrl);


		if (pDX->m_bSaveAndValidate)
			GetFromInterface();
		else
			SetToInterface();


	}



	BEGIN_MESSAGE_MAP(CMappingAdvancedDlg, CDialog)
		ON_CBN_SELCHANGE(IDC_MAP_VARIOGRAM_MODEL, &UpdateCtrl)
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

		m_nbPointCtrl.EnableWindow(bEnableKriging || bEnableIWD);
		m_maxDistanceCtrl.EnableWindow(bEnableKriging || bEnableIWD || bEnableTPS);
		
		m_static1Ctrl.EnableWindow(bEnableRegression, TRUE);
		m_static2Ctrl.EnableWindow(bEnableKriging, TRUE);
		m_static3Ctrl.EnableWindow(bEnableIWD, TRUE);
		m_static4Ctrl.EnableWindow(bEnableTPS, TRUE);

		m_regionalLimitCtrl.EnableWindow(bEnableKriging);


	}

	void CMappingAdvancedDlg::GetFromInterface()
	{
		m_pParam->m_nbPoints = ToInt(m_nbPointCtrl.GetString());
		m_pParam->m_noData = ToFloat(m_noDataCtrl.GetString());
		m_pParam->m_maxDistance = ToFloat(m_maxDistanceCtrl.GetString()) * 1000;
		m_pParam->m_XvalPoints = ToFloat(m_XvalPointCtrl.GetString())/100;
		
		m_pParam->m_GDALOptions = m_GDALOptionsCtrl.GetString();
		m_pParam->m_regressCriticalR2 = ToDouble(m_R²Ctrl.GetString());
		m_pParam->m_variogramModel = m_variogramModelCtrl.GetCurSel() - 1;
		m_pParam->m_lagDist = ToDouble(m_lagDistanceCtrl.GetString());
		m_pParam->m_nbLags = ToInt(m_nbLagsCtrl.GetString());
		m_pParam->m_detrendingModel = m_detrendingCtrl.GetCurSel() - 1;
		m_pParam->m_externalDrift = m_externalDriftCtrl.GetCurSel() - 1;
		m_pParam->m_bOutputVariogramInfo = m_outputVariogramCtrl.GetCheck();
		m_pParam->m_bRegionalLimit = m_regionalLimitCtrl.GetCheck();
		m_pParam->m_regionalLimitSD = ToFloat(m_regionalLimitSDCtrl.GetString());
		m_pParam->m_bRegionalLimitToBound = m_regionalLimitToBoundCtrl.GetCheck();

		m_pParam->m_bGlobalLimit = m_globalLimitCtrl.GetCheck();
		m_pParam->m_globalLimitSD = ToFloat(m_globalLimitSDCtrl.GetString());
		m_pParam->m_bGlobalLimitToBound = m_globalLimitToBoundCtrl.GetCheck();
		m_pParam->m_bGlobalMinMaxLimit = m_globalLimitMinMaxCtrl.GetCheck();
		m_pParam->m_globalMinLimit = ToFloat(m_globalMinLimitCtrl.GetString());
		m_pParam->m_globalMaxLimit = ToFloat(m_globalMaxLimitCtrl.GetString());
		m_pParam->m_bGlobalMinMaxLimitToBound = m_globalMinMaxLimitToBoundCtrl.GetCheck();

		//TPS
		
		m_pParam->m_TPSMaxError = ToFloat(m_TPSMaxErrorCtrl.GetString());

		//IWD
		m_pParam->m_IWDModel = m_IWDModelCtrl.GetCurSel() - 1;
		m_pParam->m_power = ToDouble(m_powerCtrl.GetString());

	}

	void CMappingAdvancedDlg::SetToInterface()
	{
		m_nbPointCtrl.SetWindowText(ToString(m_pParam->m_nbPoints));
		m_noDataCtrl.SetWindowText(ToString(m_pParam->m_noData));
		m_maxDistanceCtrl.SetWindowText(ToString(m_pParam->m_maxDistance / 1000, 0));
		m_XvalPointCtrl.SetWindowText(ToString(m_pParam->m_XvalPoints * 100, 0));
		m_GDALOptionsCtrl.SetWindowText(m_pParam->m_GDALOptions);

		m_R²Ctrl.SetWindowText(ToString(m_pParam->m_regressCriticalR2));

		m_variogramModelCtrl.SetCurSel(m_pParam->m_variogramModel + 1);
		m_lagDistanceCtrl.SetWindowText(ToString(m_pParam->m_lagDist, 6));
		m_nbLagsCtrl.SetWindowText(ToString(m_pParam->m_nbLags));
		m_detrendingCtrl.SetCurSel(m_pParam->m_detrendingModel + 1);
		m_externalDriftCtrl.SetCurSel(m_pParam->m_externalDrift + 1);
		m_outputVariogramCtrl.SetCheck(m_pParam->m_bOutputVariogramInfo);


		m_regionalLimitCtrl.SetCheck(m_pParam->m_bRegionalLimit);
		m_regionalLimitSDCtrl.SetWindowText(ToString(m_pParam->m_regionalLimitSD));
		m_regionalLimitToBoundCtrl.SetCheck(m_pParam->m_bRegionalLimitToBound);
		m_globalLimitCtrl.SetCheck(m_pParam->m_bGlobalLimit);
		m_globalLimitSDCtrl.SetWindowText(ToString(m_pParam->m_globalLimitSD));
		m_globalLimitToBoundCtrl.SetCheck(m_pParam->m_bGlobalLimitToBound);
		m_globalLimitMinMaxCtrl.SetCheck(m_pParam->m_bGlobalMinMaxLimit);
		m_globalMinLimitCtrl.SetWindowText(ToString(m_pParam->m_globalMinLimit));
		m_globalMaxLimitCtrl.SetWindowText(ToString(m_pParam->m_globalMaxLimit));
		m_globalMinMaxLimitToBoundCtrl.SetCheck(m_pParam->m_bGlobalMinMaxLimitToBound);

		//TPS
		m_TPSMaxErrorCtrl.SetWindowText(ToString(m_pParam->m_TPSMaxError, 10));

		m_IWDModelCtrl.SetCurSel(m_pParam->m_IWDModel + 1);
		if (m_pParam->m_power > 0)
			m_powerCtrl.SetWindowText(ToString(m_pParam->m_power));
		else m_powerCtrl.SetCurSel(0); //best power
	}

}