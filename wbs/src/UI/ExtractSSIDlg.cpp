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
	ON_BN_CLICKED(IDC_MAP_EXTRACT_FROM_DEM, &UpdateCtrl)
	ON_BN_CLICKED(IDC_MAP_EXTRACT_FROM_GOOGLE, &UpdateCtrl)

END_MESSAGE_MAP()


IMPLEMENT_DYNCREATE(CExtractSSIDlg, CDialog)
/////////////////////////////////////////////////////////////////////////////

CExtractSSIDlg::CExtractSSIDlg(CWnd* pParent): 
CDialog(CExtractSSIDlg::IDD, pParent)
{
	CAppOption option(_T("ExtractSSI"));

	m_extractFrom = (TExtractFrom)option.GetProfileInt(_T("ExtractFrom"), 0);
	m_gridFilePath = option.GetProfileString(_T("GridFilePath"));
	m_bExtractElev = option.GetProfileBool(_T("ExtractElev"),true);
	m_bExtractSlopeAspect = option.GetProfileBool(_T("ExtractSlopeAspect"), false);
	m_bMissingOnly = option.GetProfileBool(_T("OnlyMissing"), true);
	m_bExtractShoreDistance = option.GetProfileBool(_T("ExtractShoreDistance"), false);
	m_bExtractGoogleName = option.GetProfileBool(_T("ExtractGoogleName"), false);
	m_bExtractGoogleElvation = option.GetProfileBool(_T("ExtractGoogleElevation"), false);
	m_googleMapsAPIKey = option.GetProfileString(_T("GoogleMapsAPIKey"));
	m_interpolationType = option.GetProfileInt(_T("InterpMethod"), 0);

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
	option.WriteProfileBool(_T("ExtractGoogleName"), m_bExtractGoogleName);
	option.WriteProfileBool(_T("ExtractGoogleElevation"), m_bExtractGoogleElvation);
	option.WriteProfileString(_T("GoogleMapsAPIKey"), m_googleMapsAPIKey);
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
	DDX_Text(pDX, IDC_MAP_EXTRACT_GOOGLE_KEY, m_googleMapsAPIKey);
	//DDX_Radio(pDX, IDC_MAP_EXTRACT_FROM_DEM, )
	
	if (pDX->m_bSaveAndValidate)
	{
		m_extractFrom = GetExtractFrom();
	}
	else
	{
		CString fileFilter = UtilWin::GetCString(IDS_STR_FILTER_RASTER);
		m_gridFilePathCtrl.EnableFileBrowseButton(_T(".tif"), fileFilter);

		DDX_CBIndex(pDX, IDC_SSI_METHOD, m_interpolationType);
		SetExtractFrom(m_extractFrom);
		UpdateCtrl();
	}

	
}

CExtractSSIDlg::TExtractFrom  CExtractSSIDlg::GetExtractFrom()const
{
	return (TExtractFrom) (GetCheckedRadioButton(IDC_MAP_EXTRACT_FROM_DEM, IDC_MAP_EXTRACT_FROM_GOOGLE)- IDC_MAP_EXTRACT_FROM_DEM);
}


void CExtractSSIDlg::SetExtractFrom(TExtractFrom  no)
{
	CheckRadioButton(IDC_MAP_EXTRACT_FROM_DEM, IDC_MAP_EXTRACT_FROM_GOOGLE, IDC_MAP_EXTRACT_FROM_DEM + no);
}

void CExtractSSIDlg::UpdateCtrl()
{
	CExtractSSIDlg::TExtractFrom  from = GetExtractFrom();
	
	GetDlgItem(IDC_MAP_EXTRACT_ELEV)->EnableWindow(from == FROM_DEM);
	GetDlgItem(IDC_MAP_EXTRACT_EXPOSITION)->EnableWindow(from == FROM_DEM);
	GetDlgItem(IDC_GRID_FILE_PATH)->EnableWindow(from == FROM_DEM);
	GetDlgItem(IDC_CMN_STATIC1)->EnableWindow(from == FROM_DEM);

	GetDlgItem(IDC_MAP_EXTRACT_GOOGLE_NAME)->EnableWindow(from == FROM_GOOGLE);
	GetDlgItem(IDC_MAP_EXTRACT_GOOGLE_ELEVATION)->EnableWindow(from == FROM_GOOGLE);
	GetDlgItem(IDC_MAP_EXTRACT_GOOGLE_KEY)->EnableWindow(from == FROM_GOOGLE);
	GetDlgItem(IDC_CMN_STATIC2)->EnableWindow(from == FROM_GOOGLE);
	GetDlgItem(IDC_CMN_STATIC3)->EnableWindow(from == FROM_GOOGLE);
	


}