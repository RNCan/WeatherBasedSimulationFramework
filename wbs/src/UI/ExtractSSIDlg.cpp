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

//#include "CommonUI/CustomDDX.h"
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
	m_bExtractOther = option.GetProfileBool(_T("ExtractOther"), false);
	m_interpolationType = option.GetProfileInt(_T("InterpMethod"), 0);

}

CExtractSSIDlg::~CExtractSSIDlg()
{
	CAppOption option(_T("ExtractSSI"));

	option.WriteProfileString(_T("GridFilePath"), m_gridFilePath);
	option.WriteProfileBool(_T("ExtractElev"), m_bExtractElev);
	option.WriteProfileBool(_T("ExtractSlopeAspect"), m_bExtractSlopeAspect);
	option.WriteProfileBool(_T("OnlyMissing"), m_bMissingOnly);
	option.WriteProfileBool(_T("ExtractOther"), m_bExtractOther);
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
	DDX_Check(pDX, IDC_MAP_EXTRACT_OTHER, m_bExtractOther);
	DDX_CBIndex(pDX, IDC_SSI_METHOD, m_interpolationType);
	
	if( !pDX->m_bSaveAndValidate )
	{
		CString fileFilter = UtilWin::GetCString(IDS_STR_FILTER_RASTER);
		m_gridFilePathCtrl.EnableFileBrowseButton(_T(".tif"), fileFilter);
	}
}

//
//void CShowMapView::OnRasterExtractElevation()
//{
//
//
//	CAppOption option("ExtractElevation");
//	CExtractElevDlg dlg;
//
//	dlg.m_filePathIn1 = option.GetProfileString("FilePathIn", "");
//	dlg.m_filePathIn2 = option.GetProfileString("FilePathDEM", "");
//	dlg.m_filePathOut = option.GetProfileString("FilepathOut", "");
//	dlg.m_interpolationType = option.GetProfileInt("Interpolation", 2);
//	dlg.m_bExtractElev = option.GetProfileBool("ExtractElevation", true);
//	dlg.m_bExtractExposition = option.GetProfileBool("ExtractSlopeAndAspect", false);
//	dlg.m_bMissingOnly = option.GetProfileBool("MissingOnly", true);
//
//	while (dlg.DoModal() != IDCANCEL)
//	{
//		if (UtilWin::FileExist(dlg.m_filePathOut))
//		{
//			CString msg;
//			msg.FormatMessage(IDS_CMN_OVERWRITE_EXISTING_FILE, dlg.m_filePathOut);
//			if (MessageBox(msg, AfxGetAppName(), MB_YESNO) == IDNO)
//				continue;
//		}
//
//		//CSCCallBack callBack;
//		CProgressStepDlg progressDlg(this);
//		progressDlg.Create();
//
//		progressDlg.GetCallback().SetNbStep(1);
//
//		CGeoResampling gr;
//
//
//		ERMsg msg =
//			gr.ExtractElevation(
//			dlg.m_filePathIn1,
//			dlg.m_filePathOut,
//			dlg.m_filePathIn2,
//			dlg.m_interpolationType,
//			dlg.m_bExtractElev,
//			dlg.m_bExtractExposition,
//			dlg.m_bMissingOnly,
//			(CSCCallBack&)progressDlg.GetCallback());
//
//		if (!msg)
//			SYShowMessage(msg, this);
//
//
//		break;
//	}
//
//	option.WriteProfileString("FilePathIn", dlg.m_filePathIn1);
//	option.WriteProfileString("FilePathDEM", dlg.m_filePathIn2);
//	option.WriteProfileString("FilePathOut", dlg.m_filePathOut);
//	option.WriteProfileInt("Interpolation", dlg.m_interpolationType);
//	option.WriteProfileBool("ExtractElevation", dlg.m_bExtractElev);
//	option.WriteProfileBool("ExtractSlopeAndAspect", dlg.m_bExtractExposition);
//	option.WriteProfileBool("MissingOnly", dlg.m_bMissingOnly);
//}
//
//
//
//using namespace std;
//using namespace WBSF;
//
//ERMsg ExtractSSI(CLocationsVector& locations,
//	const std::string& gridFilePath,
//	int resamplingType,
//	bool bExtractElev,
//	bool bExtractExpo,
//	bool bMissingOnly,
//	CCallback& callBack)
//{
//	
//	ERMsg message;
//
//	CGeoMap geoMapIn;
//	
//
//	message = geoMapIn.Open(gridFilePath, CGeoFileInterface::TOpenMode(CGeoFileInterface::modeRead | CGeoFileInterface::modeDirectAccess));
//	
//	if (message)
//	{
//
//
//		CString str;
//		str.LoadString(IDS_MAP_RESAMPLING_CAPTION);
//		
//		callBack.SetCurrentDescription((LPCTSTR)str);
//		callBack.SetNbStep(locations.size());
//
//		for (size_t i = 0; i<inputLOC.GetSize() && message; i++)
//		{
//			CGeoPointWP pt = inputLOC[i].GetCoordWP();
//			pt.Reproject(geoMapIn.GetProjection());
//
//			if (geoMapIn.GetBoundingBox().PtInRect(pt))
//			{
//				float result = 0;
//				switch (resamplingType)
//				{
//				case CGeoResamplingOption::NEAREST://** nearest result
//					result = geoMapIn.GetNearestCell(pt);
//					break;
//				case CGeoResamplingOption::BILINEAR://** object method Bilinear
//					result = geoMapIn.GetBilinearCell(pt);
//					break;
//				case CGeoResamplingOption::CUBIC://** object method Cubic
//					result = geoMapIn.GetCubicConvolutionCell(pt);
//					break;
//
//				default:ASSERT(false);
//				}
//
//				if (bExtractElev)
//				{
//					if (!bMissingOnly || inputLOC[i].GetElev() == -999)
//					{
//						if (result > geoMapIn.GetNoData())
//						{
//							inputLOC[i].SetElev((int)result);
//						}
//						else
//						{
//							inputLOC[i].SetElev(-999);
//						}
//					}
//				}
//
//				if (bExtractExpo)
//				{
//					if (!bMissingOnly || inputLOC[i].GetSlope() == -999 || inputLOC[i].GetAspect() == -999)
//					{
//						long col = 0;
//						long row = 0;
//						VERIFY(geoMapIn.CartoCoordToColRow(pt.m_lat(), pt.m_lon(), col, row, true));
//
//						CGeoMapPoint ptGeo;
//						if (geoMapIn.ReadCell(ptGeo, col, row))
//						{
//							inputLOC[i].SetExposition(ptGeo.GetSlope(), ptGeo.GetAspect());
//						}
//						else
//						{
//							inputLOC[i].SetExposition(-999, -999);
//						}
//					}
//				}
//			}
//			else
//			{
//				if (bExtractElev)
//					if (!bMissingOnly || inputLOC[i].GetElev() == -999)
//						inputLOC[i].SetElev(-999);
//
//				if (bExtractExpo)
//					if (!bMissingOnly || inputLOC[i].GetSlope() == -999 || inputLOC[i].GetAspect() == -999)
//						inputLOC[i].SetExposition(-999, -999);
//			}
//
//			message += callBack.StepIt();
//		}
//
//		message = inputLOC.Save((LPCTSTR)filePathOut);
//
//	}
//
//	geoMapIn.Close();
//
//
//	return message;
//}
