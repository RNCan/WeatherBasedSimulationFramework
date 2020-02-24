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

 
BEGIN_MESSAGE_MAP(CExtractSSIDlg, CDialogEx)
	ON_BN_CLICKED(IDC_MAP_EXTRACT_FROM_DEM, &UpdateCtrl)
	ON_BN_CLICKED(IDC_MAP_EXTRACT_FROM_WEB, &UpdateCtrl)
	ON_BN_CLICKED(IDC_MAP_EXTRACT_WEB_ELEVATION, &UpdateCtrl)

END_MESSAGE_MAP()


IMPLEMENT_DYNCREATE(CExtractSSIDlg, CDialogEx)
/////////////////////////////////////////////////////////////////////////////
//https://api.opentopodata.org/v1/srtm90m?locations=-43.5,172.5|27.6,1.98&interpolation=cubic
//au canada seulement:
//http://geogratis.gc.ca/services/elevation/cdem/altitude?lat=45.5&lon=-71.5
//reverse geocode
//https://nominatim.openstreetmap.org/reverse?format=json&lat=46.736497&lon=-71.450790


CExtractSSIDlg::CExtractSSIDlg(CWnd* pParent): 
CDialogEx(CExtractSSIDlg::IDD, pParent)
{
	CAppOption option(_T("ExtractSSI"));

	m_extractFrom = (TExtractFrom)option.GetProfileInt(_T("ExtractFrom"), 0);
	m_gridFilePath = option.GetProfileString(_T("GridFilePath"));
	m_bExtractElev = option.GetProfileBool(_T("ExtractElev"),true);
	m_bExtractSlopeAspect = option.GetProfileBool(_T("ExtractSlopeAspect"), false);
	m_bMissingOnly = option.GetProfileBool(_T("OnlyMissing"), true);
	m_bExtractShoreDistance = option.GetProfileBool(_T("ExtractShoreDistance"), false);
	m_bExtractWebElevation = option.GetProfileBool(_T("ExtractWebElevation"), false);
	m_webElevProduct = option.GetProfileInt(_T("WebElevProduct"), 1);
	m_interpolationType = option.GetProfileInt(_T("InterpMethod"), 0);

	m_bExtractWebName = option.GetProfileBool(_T("ExtractWebName"), false);
	m_bExtractWebState = option.GetProfileBool(_T("ExtractWebName2"), false);
	m_bExtractWebCountry = option.GetProfileBool(_T("ExtractWebName3"), false);
}

CExtractSSIDlg::~CExtractSSIDlg()
{
	CAppOption option(_T("ExtractSSI"));

	option.WriteProfileInt(_T("ExtractFrom"), m_extractFrom);
	option.WriteProfileString(_T("GridFilePath"), m_gridFilePath);
	option.WriteProfileBool(_T("ExtractElev"), m_bExtractElev);
	option.WriteProfileBool(_T("ExtractSlopeAspect"), m_bExtractSlopeAspect);
	option.WriteProfileBool(_T("OnlyMissing"), m_bMissingOnly);
	option.WriteProfileBool(_T("ExtractShoreDistance"), m_bExtractShoreDistance);
	
	option.WriteProfileBool(_T("ExtractWebElevation"), m_bExtractWebElevation);
	option.WriteProfileBool(_T("WebElevProduct"), m_webElevProduct);
	option.WriteProfileInt(_T("InterpMethod"), m_interpolationType);

	option.WriteProfileBool(_T("ExtractWebName"), m_bExtractWebName);
	option.WriteProfileBool(_T("ExtractWebName2"), m_bExtractWebState);
	option.WriteProfileBool(_T("ExtractWebName3"), m_bExtractWebCountry);
}


void CExtractSSIDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_GRID_FILE_PATH, m_gridFilePathCtrl);
	

	DDX_Text(pDX, IDC_GRID_FILE_PATH, m_gridFilePath);
	DDX_Check(pDX, IDC_MAP_EXTRACT_ELEV, m_bExtractElev);
	DDX_Check(pDX, IDC_MAP_EXTRACT_EXPOSITION, m_bExtractSlopeAspect);
	DDX_Check(pDX, IDC_MAP_EXTRACT_MISSING, m_bMissingOnly);
	DDX_Check(pDX, IDC_MAP_EXTRACT_SHORE_DISTANCE, m_bExtractShoreDistance);
	DDX_Check(pDX, IDC_MAP_EXTRACT_WEB_ELEVATION, m_bExtractWebElevation);
	DDX_CBIndex(pDX, IDC_MAP_EXTRACT_PRODUCT, m_webElevProduct);
	DDX_CBIndex(pDX, IDC_SSI_METHOD, m_interpolationType);
	DDX_Check(pDX, IDC_MAP_EXTRACT_WEB_NAME, m_bExtractWebName);
	DDX_Check(pDX, IDC_MAP_EXTRACT_WEB_NAME2, m_bExtractWebState);
	DDX_Check(pDX, IDC_MAP_EXTRACT_WEB_NAME3, m_bExtractWebCountry);
	
	if (pDX->m_bSaveAndValidate)
	{
		m_extractFrom = GetExtractFrom();
	}
	else
	{
		CString fileFilter = UtilWin::GetCString(IDS_STR_FILTER_RASTER);
		m_gridFilePathCtrl.EnableFileBrowseButton(_T(".tif"), fileFilter);

		//DDX_CBIndex(pDX, IDC_SSI_METHOD, m_interpolationType);
		SetExtractFrom(m_extractFrom);
		UpdateCtrl();
	}

	
}

CExtractSSIDlg::TExtractFrom  CExtractSSIDlg::GetExtractFrom()const
{
	return (TExtractFrom) (GetCheckedRadioButton(IDC_MAP_EXTRACT_FROM_DEM, IDC_MAP_EXTRACT_FROM_WEB)- IDC_MAP_EXTRACT_FROM_DEM);
}


void CExtractSSIDlg::SetExtractFrom(TExtractFrom  no)
{
	CheckRadioButton(IDC_MAP_EXTRACT_FROM_DEM, IDC_MAP_EXTRACT_FROM_WEB, IDC_MAP_EXTRACT_FROM_DEM + no);
}

void CExtractSSIDlg::UpdateCtrl()
{
	CExtractSSIDlg::TExtractFrom  from = GetExtractFrom();
	
	bool bFromDEM = from == FROM_DEM;
	bool bWebElev = IsDlgButtonChecked(IDC_MAP_EXTRACT_WEB_ELEVATION);
	GetDlgItem(IDC_MAP_EXTRACT_ELEV)->EnableWindow(bFromDEM);
	GetDlgItem(IDC_MAP_EXTRACT_EXPOSITION)->EnableWindow(bFromDEM);
	GetDlgItem(IDC_GRID_FILE_PATH)->EnableWindow(bFromDEM);
	GetDlgItem(IDC_CMN_STATIC1)->EnableWindow(bFromDEM);

	
	GetDlgItem(IDC_MAP_EXTRACT_WEB_ELEVATION)->EnableWindow(!bFromDEM);
	GetDlgItem(IDC_MAP_EXTRACT_PRODUCT)->EnableWindow(!bFromDEM && bWebElev);
	GetDlgItem(IDC_SSI_METHOD)->EnableWindow(!bFromDEM && bWebElev);
	GetDlgItem(IDC_CMN_STATIC2)->EnableWindow(!bFromDEM);
	GetDlgItem(IDC_MAP_EXTRACT_WEB_NAME)->EnableWindow(!bFromDEM);
	GetDlgItem(IDC_MAP_EXTRACT_WEB_NAME2)->EnableWindow(!bFromDEM);
	GetDlgItem(IDC_MAP_EXTRACT_WEB_NAME3)->EnableWindow(!bFromDEM);
	
	//GetDlgItem(IDC_CMN_STATIC3)->EnableWindow(!bFromDEM);
	


}