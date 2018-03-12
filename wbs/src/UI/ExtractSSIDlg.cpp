//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//	
//******************************************************************************
#include "stdafx.h"


#include "Basic/Location.h"
#include "UI/ExtractSSIDlg.h"
#include "UI/Common/AppOption.h"

#include "WeatherBasedSimulationString.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

 
BEGIN_MESSAGE_MAP(CExtractSSIDlg, CDialog)
END_MESSAGE_MAP()


IMPLEMENT_DYNCREATE(CExtractSSIDlg, CDialog)
/////////////////////////////////////////////////////////////////////////////

CExtractSSIDlg::CExtractSSIDlg(CWnd* pParent): 
CDialog(CExtractSSIDlg::IDD, pParent)
{
	CAppOption option(_T("ExtractSSI"));

	m_gridFilePath = option.GetProfileString(_T("GridFilePath"));
	m_bExtractElev = option.GetProfileBool(_T("ExtractElev"),true);
	m_bExtractSlopeAspect = option.GetProfileBool(_T("ExtractSlopeAspect"), false);
	m_bMissingOnly = option.GetProfileBool(_T("OnlyMissing"), true);
	m_bExtractShoreDistance = option.GetProfileBool(_T("ExtractShoreDistance"), false);
	m_bExtractGoogleName = option.GetProfileBool(_T("ExtractGoogleName"), false);
	m_bExtractGoogleElvation = option.GetProfileBool(_T("ExtractGoogleElevation"), false);
	m_googleMapAPIKey = option.GetProfileString(_T("GoogleMapAPIKey"));
	m_googleGeoCodeAPIKey = option.GetProfileString(_T("GoogleGeoCodeAPIKey"));
	m_interpolationType = option.GetProfileInt(_T("InterpMethod"), 0);

}

CExtractSSIDlg::~CExtractSSIDlg()
{
	CAppOption option(_T("ExtractSSI"));

	option.WriteProfileString(_T("GridFilePath"), m_gridFilePath);
	option.WriteProfileBool(_T("ExtractElev"), m_bExtractElev);
	option.WriteProfileBool(_T("ExtractSlopeAspect"), m_bExtractSlopeAspect);
	option.WriteProfileBool(_T("OnlyMissing"), m_bMissingOnly);
	option.WriteProfileBool(_T("ExtractShoreDistance"), m_bExtractShoreDistance);
	option.WriteProfileBool(_T("ExtractGoogleName"), m_bExtractGoogleName);
	option.WriteProfileBool(_T("ExtractGoogleElevation"), m_bExtractGoogleElvation);
	option.WriteProfileString(_T("GoogleMapAPIKey"), m_googleMapAPIKey);
	option.WriteProfileString(_T("GoogleGeoCodeAPIKey"), m_googleGeoCodeAPIKey);
	option.WriteProfileInt(_T("InterpMethod"), m_interpolationType);
}


void CExtractSSIDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_GRID_FILE_PATH, m_gridFilePathCtrl);

	DDX_Text(pDX, IDC_GRID_FILE_PATH, m_gridFilePath);
	DDX_Check(pDX, IDC_MAP_EXTRACT_ELEV, m_bExtractElev);
	DDX_Check(pDX, IDC_MAP_EXTRACT_EXPOSITION, m_bExtractSlopeAspect);
	DDX_Check(pDX, IDC_MAP_EXTRACT_MISSING, m_bMissingOnly);
	DDX_Check(pDX, IDC_MAP_EXTRACT_SHORE_DISTANCE, m_bExtractShoreDistance);
	DDX_Check(pDX, IDC_MAP_EXTRACT_GOOGLE_NAME, m_bExtractGoogleName);
	DDX_Check(pDX, IDC_MAP_EXTRACT_GOOGLE_ELEVATION, m_bExtractGoogleElvation);
	DDX_Text(pDX, IDC_MAP_EXTRACT_ELEVATION_KEY, m_googleMapAPIKey);
	DDX_Text(pDX, IDC_MAP_EXTRACT_GEOCODE_KEY, m_googleGeoCodeAPIKey);

	DDX_CBIndex(pDX, IDC_SSI_METHOD, m_interpolationType);
	
	if( !pDX->m_bSaveAndValidate )
	{
		CString fileFilter = UtilWin::GetCString(IDS_STR_FILTER_RASTER);
		m_gridFilePathCtrl.EnableFileBrowseButton(_T(".tif"), fileFilter);
	}
}


ERMsg msg;

