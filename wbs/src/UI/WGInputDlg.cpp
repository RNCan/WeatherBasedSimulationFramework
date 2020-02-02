//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
// 02/08/2011	Rémi Saint-Amant	Update with new interface
//****************************************************************************
#include "stdafx.h"

#include "FileManager/FileManager.h"
#include "UI/Common/AppOption.h"
#include "UI/Common/SYShowMessage.h"
#include "UI/WGInputDlg.h"
#include "UI/SearchRadiusDlg.h"
#include "WeatherBasedSimulationString.h"
#include "UI/GenerateWUDlg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWGInputDlg dialog
using namespace UtilWin;
using namespace std;



namespace WBSF
{
	CWGInputDlg::CWGInputDlg(CWnd* pParent /*=NULL*/) :
		CDialogEx(IDD_WG_INPUT, pParent)
	{
		//m_bDefaultModel = true;
	}


	void CWGInputDlg::DoDataExchange(CDataExchange* pDX)
	{
		CDialogEx::DoDataExchange(pDX);


		DDX_Control(pDX, IDC_WG_VARIABLES, m_variablesCtrl);
		DDX_Control(pDX, IDC_WG_SOURCE_TYPE, m_sourceTypeCtrl);
		DDX_Control(pDX, IDC_WG_GENERATION_TYPE, m_generationTypeCtrl);

		DDX_Control(pDX, IDC_WG_NORMALS_DBNAME, m_normalDBNameCtrl);
		DDX_Control(pDX, IDC_WG_NORMALS_LINK, m_normalLinkCtrl);
		DDX_Control(pDX, IDC_WG_NORMALS_NB_STATIONS, m_normalsNbStationsCtrl);

		DDX_Control(pDX, IDC_WG_DAILY_DBNAME, m_dailyDBNameCtrl);
		DDX_Control(pDX, IDC_WG_DAILY_LINK, m_dailyLinkCtrl);
		DDX_Control(pDX, IDC_WG_DAILY_NB_STATIONS, m_dailyNbStationsCtrl);

		DDX_Control(pDX, IDC_WG_HOURLY_DBNAME, m_hourlyDBNameCtrl);
		DDX_Control(pDX, IDC_WG_HOURLY_LINK, m_hourlyLinkCtrl);
		DDX_Control(pDX, IDC_WG_HOURLY_NB_STATIONS, m_hourlyNbStationsCtrl);
		DDX_Control(pDX, IDC_WG_GRIBS_NB_POINTS, m_gribNbPointsCtrl);


		DDX_Control(pDX, IDC_WG_GRIBS_DBNAME, m_gribsDBNameCtrl);
		DDX_Control(pDX, IDC_WG_GRIBS_LINK, m_gribsLinkCtrl);
		DDX_Control(pDX, IDC_WG_USE_GRIBS, m_useGribCtrl);
		//DDX_Control(pDX, IDC_WG_AT_SURFACE, m_atSurfaceCtrl); 


		DDX_Control(pDX, IDC_WG_NB_YEARS, m_nbYearsCtrl);
		DDX_Control(pDX, IDC_WG_FIRST_YEAR, m_firstYearCtrl);
		DDX_Control(pDX, IDC_WG_LAST_YEAR, m_lastYearCtrl);

		DDX_Control(pDX, IDC_WG_USE_FORECASTS, m_useForecastCtrl);
		DDX_Control(pDX, IDC_WG_USE_RADARS_PRCP, m_useRadarPrcpCtrl);
		DDX_Control(pDX, IDC_WG_ALBEDO, m_albedoTypeCtrl);
		DDX_Control(pDX, IDC_WG_SEED, m_seedTypeCtrl);
		DDX_Control(pDX, IDC_WG_ALLOWED_DERIVED_VARIABLES, m_allowedDerivedVariablesCtrl);
		DDX_Control(pDX, IDC_WG_XVALIDATION, m_XValidationCtrl);
		DDX_Control(pDX, IDC_WG_SKIP_VERIFY, m_skipVerifyCtrl);
		DDX_Control(pDX, IDC_WG_NO_FILL_MISSING, m_noFillMissingCtrl);
		DDX_Control(pDX, IDC_WG_USE_SHORE, m_useShoreCtrl);


		if (!pDX->m_bSaveAndValidate)
		{
			//FillModel
			m_variablesCtrl.m_bShowFromModel = true;
			m_variablesCtrl.m_models = WBSF::GetFM().Model().GetFilesList();
		}

	}



	BEGIN_MESSAGE_MAP(CWGInputDlg, CDialogEx)
		ON_CBN_SELCHANGE(IDC_WG_SOURCE_TYPE, &UpdateCtrl)
		ON_CBN_SELCHANGE(IDC_WG_GENERATION_TYPE, &UpdateCtrl)
		ON_BN_CLICKED(IDC_WG_USE_GRIBS, &UpdateCtrl)
		ON_BN_CLICKED(IDC_WG_NORMALS_LINK, &OnNormalsLink)
		ON_BN_CLICKED(IDC_WG_DAILY_LINK, &OnDailyLink)
		ON_BN_CLICKED(IDC_WG_HOURLY_LINK, &OnHourlyLink)
		ON_BN_CLICKED(IDC_WG_GRIBS_LINK, &OnGribsLink)
		ON_BN_CLICKED(IDC_WG_SEARCH_RADIUS, &OnSearchRadiusClick)
		ON_WM_DESTROY()
		ON_WM_ENABLE()
		ON_BN_CLICKED(IDC_WG_DOWNLOAD_WEATHER, &CWGInputDlg::OnDownloadWeather)
	END_MESSAGE_MAP()

	/////////////////////////////////////////////////////////////////////////////
	// CWGInputDlg message handlers


	BOOL CWGInputDlg::OnInitDialog()
	{
		CDialogEx::OnInitDialog();

		CWaitCursor cursor;
		FillNormalsDBNameList();
		FillDailyDBNameList();
		FillHourlyDBNameList();
		FillGribsDBNameList();

		//CAppOption appOption;

		if (m_normalDBNameCtrl.GetCount() > 0)
			m_normalDBNameCtrl.SetCurSel(0);

		//set button image and tooltips
		m_normalLinkCtrl.SetImage(IDB_CMN_LINK32);
		m_dailyLinkCtrl.SetImage(IDB_CMN_LINK32);
		m_hourlyLinkCtrl.SetImage(IDB_CMN_LINK32);
		m_gribsLinkCtrl.SetImage(IDB_CMN_LINK32);

		// Create the tool tip
		m_testToolTips.Create(this);

		CString nbStationStr = UtilWin::GetCString(IDS_DB_MATCH_TOOLTIP);
		CString linkStr = UtilWin::GetCString(IDS_DB_LINK_TOOLTIP);

		// Add the tools
		m_testToolTips.AddWindowTool(&m_normalsNbStationsCtrl, nbStationStr);
		m_testToolTips.AddWindowTool(&m_dailyNbStationsCtrl, nbStationStr);
		m_testToolTips.AddWindowTool(&m_hourlyNbStationsCtrl, nbStationStr);
		//m_testToolTips.AddWindowTool(&m_gribsNbPointsCtrl, nbStationStr);

		m_testToolTips.AddWindowTool(&m_normalLinkCtrl, linkStr);
		m_testToolTips.AddWindowTool(&m_dailyLinkCtrl, linkStr);
		m_testToolTips.AddWindowTool(&m_hourlyLinkCtrl, linkStr);
		m_testToolTips.AddWindowTool(&m_gribsLinkCtrl, linkStr);

		return TRUE;  // return TRUE unless you set the focus to a control
	}

	void CWGInputDlg::SetWGInput(const CWGInput& WGInput)
	{
		m_variablesCtrl.SetVariables(WGInput.m_variables);
		m_searchRadius = WGInput.m_searchRadius;

		m_sourceTypeCtrl.SetCurSel(WGInput.m_sourceType);
		m_generationTypeCtrl.SetCurSel(WGInput.m_generationType);

		if (!WGInput.m_normalsDBName.empty())
		{
			if (m_normalDBNameCtrl.SelectStringExact(-1, WGInput.m_normalsDBName) == CB_ERR)
			{
				string filePath;
				ERMsg message = WBSF::GetFM().Normals().GetFilePath(WGInput.m_normalsDBName, filePath);
				ASSERT(filePath.empty()); //introuvable

				SYShowMessage(message, this);
			}
		}
		else
		{
			m_normalDBNameCtrl.SetCurSel(0);
		}

		if (!WGInput.m_dailyDBName.empty())
		{
			if (m_dailyDBNameCtrl.SelectStringExact(-1, WGInput.m_dailyDBName) == CB_ERR)
			{
				string filePath;
				ERMsg message = WBSF::GetFM().Daily().GetFilePath(WGInput.m_dailyDBName, filePath);
				ASSERT(filePath.empty()); //introuvable

				SYShowMessage(message, this);
			}
		}
		else
		{
			m_dailyDBNameCtrl.SetCurSel(0);
		}


		if (!WGInput.m_hourlyDBName.empty())
		{
			if (m_hourlyDBNameCtrl.SelectStringExact(-1, WGInput.m_hourlyDBName.c_str()) == CB_ERR)
			{
				string filePath;
				ERMsg message = WBSF::GetFM().Hourly().GetFilePath(WGInput.m_hourlyDBName, filePath);
				ASSERT(filePath.empty()); //introuvable

				SYShowMessage(message, this);
			}
		}
		else
		{
			m_hourlyDBNameCtrl.SetCurSel(0);
		}

		if (!WGInput.m_gribsDBName.empty())
		{
			if (m_gribsDBNameCtrl.SelectStringExact(-1, WGInput.m_gribsDBName.c_str()) == CB_ERR)
			{
				string filePath;
				ERMsg message = WBSF::GetFM().Gribs().GetFilePath(WGInput.m_gribsDBName, filePath);
				ASSERT(filePath.empty()); //introuvable

				SYShowMessage(message, this);
			}
		}
		else
		{
			m_gribsDBNameCtrl.SetCurSel(0);
		}

		m_normalsNbStationsCtrl.SetString(to_string(WGInput.m_nbNormalsStations));
		m_dailyNbStationsCtrl.SetString(to_string(WGInput.m_nbDailyStations));
		m_hourlyNbStationsCtrl.SetString(to_string(WGInput.m_nbHourlyStations));
		m_gribNbPointsCtrl.SetString(to_string(WGInput.m_nbGribPoints));
		m_useGribCtrl.SetCheck(WGInput.m_bUseGribs);
		//m_atSurfaceCtrl.SetCheck(WGInput.m_bAtSurfaceOnly);

		m_nbYearsCtrl.SetString(to_string(WGInput.m_nbNormalsYears));
		m_firstYearCtrl.SetString(to_string(WGInput.m_firstYear));
		m_lastYearCtrl.SetString(to_string(WGInput.m_lastYear));

		m_useForecastCtrl.SetCheck(WGInput.m_bUseForecast);
		m_useRadarPrcpCtrl.SetCheck(WGInput.m_bUseRadarPrcp);

		m_albedoTypeCtrl.SetCurSel((int)WGInput.m_albedo);
		m_seedTypeCtrl.SetCurSel((int)WGInput.m_seed);
		m_allowedDerivedVariablesCtrl.SetVariables(WGInput.m_allowedDerivedVariables);
		m_XValidationCtrl.SetCheck(WGInput.m_bXValidation);
		m_skipVerifyCtrl.SetCheck(WGInput.m_bSkipVerify);
		m_noFillMissingCtrl.SetCheck(WGInput.m_bNoFillMissing);
		m_useShoreCtrl.SetCheck(WGInput.m_bUseShore);

		//EnableWindow(!bDefault);
		//UpdateCtrl();
	}

	void CWGInputDlg::GetWGInput(CWGInput& WGInput)
	{

		WGInput.m_variables = m_variablesCtrl.GetVariables();
		WGInput.m_searchRadius = m_searchRadius;
		WGInput.m_sourceType = m_sourceTypeCtrl.GetCurSel();
		WGInput.m_generationType = m_generationTypeCtrl.GetCurSel();

		WGInput.m_normalsDBName = m_normalDBNameCtrl.GetString();
		WGInput.m_nbNormalsStations = stoi(m_normalsNbStationsCtrl.GetString());
		WGInput.m_dailyDBName = m_dailyDBNameCtrl.GetString();
		WGInput.m_nbDailyStations = stoi(m_dailyNbStationsCtrl.GetString());
		WGInput.m_hourlyDBName = m_hourlyDBNameCtrl.GetString();
		WGInput.m_nbHourlyStations = stoi(m_hourlyNbStationsCtrl.GetString());
		WGInput.m_nbGribPoints = stoi(m_gribNbPointsCtrl.GetString());
		WGInput.m_gribsDBName = m_gribsDBNameCtrl.GetString();
		WGInput.m_bUseGribs = m_useGribCtrl.GetCheck();
		//		WGInput.m_bAtSurfaceOnly = m_atSurfaceCtrl.GetCheck();

		WGInput.m_nbNormalsYears = stoi(m_nbYearsCtrl.GetString());
		WGInput.m_firstYear = stoi(m_firstYearCtrl.GetString());
		WGInput.m_lastYear = stoi(m_lastYearCtrl.GetString());
		WGInput.m_bUseForecast = m_useForecastCtrl.GetCheck();
		WGInput.m_bUseRadarPrcp = m_useRadarPrcpCtrl.GetCheck();

		WGInput.m_albedo = m_albedoTypeCtrl.GetCurSel();
		WGInput.m_seed = m_seedTypeCtrl.GetCurSel();
		WGInput.m_allowedDerivedVariables = m_allowedDerivedVariablesCtrl.GetVariables();
		WGInput.m_bXValidation = m_XValidationCtrl.GetCheck();
		WGInput.m_bSkipVerify = m_skipVerifyCtrl.GetCheck();
		WGInput.m_bNoFillMissing = m_noFillMissingCtrl.GetCheck();
		WGInput.m_bUseShore = m_useShoreCtrl.GetCheck();

	}


	void CWGInputDlg::UpdateCtrl()
	{
		UpdateCtrl(true);
	}

	void CWGInputDlg::UpdateCtrl(bool bEnable)
	{
		EnumChildWindows(GetSafeHwnd(), EnableChildWindow, bEnable);

		if (bEnable)
		{
			bool bFromDisaggregations = m_sourceTypeCtrl.GetCurSel() == CWGInput::FROM_DISAGGREGATIONS;
			bool bFromObservations = m_sourceTypeCtrl.GetCurSel() == CWGInput::FROM_OBSERVATIONS;

			bool bGenerateDaily = bFromObservations && (m_generationTypeCtrl.GetCurSel() == CWGInput::GENERATE_DAILY);
			bool bGenerateHourly = bFromObservations && (m_generationTypeCtrl.GetCurSel() == CWGInput::GENERATE_HOURLY);

			GetStaticCtrl(1).EnableWindow(bFromDisaggregations);
			m_nbYearsCtrl.EnableWindow(bFromDisaggregations);

			GetStaticCtrl(2).EnableWindow(bFromObservations);
			m_firstYearCtrl.EnableWindow(bFromObservations);
			GetStaticCtrl(3).EnableWindow(bFromObservations);
			m_lastYearCtrl.EnableWindow(bFromObservations);
			m_useForecastCtrl.EnableWindow(bFromObservations);
			m_useRadarPrcpCtrl.EnableWindow(bFromObservations);

			m_dailyDBNameCtrl.EnableWindow(bGenerateDaily);
			m_dailyLinkCtrl.EnableWindow(bGenerateDaily);
			m_dailyNbStationsCtrl.EnableWindow(bGenerateDaily);

			m_hourlyDBNameCtrl.EnableWindow(bGenerateHourly);
			m_hourlyLinkCtrl.EnableWindow(bGenerateHourly);
			m_hourlyNbStationsCtrl.EnableWindow(bGenerateHourly);

			bool bUseGribs = bFromObservations && m_useGribCtrl.GetCheck();
			m_useGribCtrl.EnableWindow(bFromObservations && bGenerateHourly);
			m_gribsDBNameCtrl.EnableWindow(bUseGribs && bGenerateHourly);
			m_gribsLinkCtrl.EnableWindow(bUseGribs && bGenerateHourly);
			m_gribNbPointsCtrl.EnableWindow(bUseGribs && bGenerateHourly);
		}
	}


	BOOL CWGInputDlg::Create(const CModel& model, CWnd* pParentWnd)
	{
		BOOL bRep = CDialogEx::Create(IDD, pParentWnd);

		if (bRep)
		{
			m_model = model;

			CAppOption option(_T("ModelsWindowsPosition"));

			CPoint pt = option.GetProfilePoint(_T("WGInputDlg"), CPoint(460, 30));
			UtilWin::EnsurePointOnDisplay(pt);
			SetWindowPos(NULL, pt.x, pt.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		}

		return bRep;
	}


	void CWGInputDlg::FillNormalsDBNameList()
	{
		string name = ToUTF8(m_normalDBNameCtrl.GetWindowText());
		m_normalDBNameCtrl.ResetContent();

		WBSF::StringVector fileList = WBSF::GetFM().Normals().GetFilesList();

		//string lastPath;
		for (size_t i = 0; i < fileList.size(); i++)
			m_normalDBNameCtrl.AddString(fileList[i]);

		if (m_normalDBNameCtrl.GetCount() == 0)
			m_normalDBNameCtrl.InsertString(0, _T(""));

		m_normalDBNameCtrl.SelectStringExact(0, name);
	}

	void CWGInputDlg::FillDailyDBNameList()
	{
		CString name = m_dailyDBNameCtrl.GetWindowText();
		m_dailyDBNameCtrl.ResetContent();

		WBSF::StringVector fileList = WBSF::GetFM().Daily().GetFilesList();

		for (int i = 0; i < fileList.size(); i++)
		{
			m_dailyDBNameCtrl.AddString(fileList[i].c_str());
		}

		m_dailyDBNameCtrl.InsertString(0, _T(""));

		m_dailyDBNameCtrl.SelectStringExact(0, name);
	}

	void CWGInputDlg::FillHourlyDBNameList()
	{
		CString name = m_hourlyDBNameCtrl.GetWindowText();
		m_hourlyDBNameCtrl.ResetContent();

		WBSF::StringVector fileList = WBSF::GetFM().Hourly().GetFilesList();

		for (int i = 0; i < fileList.size(); i++)
		{
			m_hourlyDBNameCtrl.AddString(fileList[i].c_str());
		}

		m_hourlyDBNameCtrl.InsertString(0, _T(""));
		m_hourlyDBNameCtrl.SelectStringExact(0, name);
	}

	void CWGInputDlg::FillGribsDBNameList()
	{
		CString name = m_gribsDBNameCtrl.GetWindowText();
		m_gribsDBNameCtrl.ResetContent();

		WBSF::StringVector fileList = WBSF::GetFM().Gribs().GetFilesList();

		for (int i = 0; i < fileList.size(); i++)
		{
			m_gribsDBNameCtrl.AddString(fileList[i].c_str());
		}

		m_gribsDBNameCtrl.InsertString(0, _T(""));
		m_gribsDBNameCtrl.SelectStringExact(0, name);
	}
	void CWGInputDlg::OnDestroy()
	{
		CAppOption option;

		CRect curRect;
		GetWindowRect(curRect);

		option.SetCurrentProfile(_T("ModelsWindowsPosition"));
		option.WriteProfilePoint(_T("WGInputDlg"), curRect.TopLeft());


		CDialogEx::OnDestroy();
	}


	//do nothing
	void CWGInputDlg::OnOK()
	{}

	void CWGInputDlg::OnCancel()
	{}

	void CWGInputDlg::OnNormalsLink()
	{
		CString filter = UtilWin::GetCString(IDS_STR_FILTER_NORMALS);
		CString sFolder;

		CFileDialog openDialog(true, NULL, _T(""), OFN_EXTENSIONDIFFERENT, filter, this);

		if (openDialog.DoModal() == IDOK)
		{
			string filePath = ToUTF8(openDialog.GetPathName());
			string path = GetPath(filePath);
			string title = GetFileTitle(filePath);

			if (!WBSF::GetFM().Normals().IsInDirectoryList(path))
			{
				string pathList = WBSF::GetFM().GetWeatherPath("|") + "|" + path;
				WBSF::GetFM().SetWeatherPath(pathList);
				WBSF::GetFM().SaveDefaultFileManager();

				FillNormalsDBNameList();
			}

			m_normalDBNameCtrl.SelectStringExact(0, title);
		}
	}


	void CWGInputDlg::OnDailyLink()
	{
		CString filter = UtilWin::GetCString(IDS_STR_FILTER_DAILY);
		CString sFolder;

		CFileDialog openDialog(true, NULL, _T(""), OFN_EXTENSIONDIFFERENT, filter, this);

		if (openDialog.DoModal() == IDOK)
		{
			string filePath = ToUTF8(openDialog.GetPathName());
			string path = GetPath(filePath);
			string title = GetFileTitle(filePath);

			if (!WBSF::GetFM().Normals().IsInDirectoryList(path))
			{
				string pathList = WBSF::GetFM().GetWeatherPath("|") + "|" + path;
				WBSF::GetFM().SetWeatherPath(pathList);
				WBSF::GetFM().SaveDefaultFileManager();

				FillDailyDBNameList();
			}

			m_dailyDBNameCtrl.SelectStringExact(0, title);
		}

	}

	void CWGInputDlg::OnHourlyLink()
	{
		CString filter = UtilWin::GetCString(IDS_STR_FILTER_HOURLY);
		CString sFolder;

		CFileDialog openDialog(true, NULL, _T(""), OFN_EXTENSIONDIFFERENT, filter, this);

		if (openDialog.DoModal() == IDOK)
		{
			string filePath = ToUTF8(openDialog.GetPathName());
			string path = GetPath(filePath);
			string title = GetFileTitle(filePath);

			if (!WBSF::GetFM().Normals().IsInDirectoryList(path))
			{
				string pathList = WBSF::GetFM().GetWeatherPath("|") + "|" + path;
				WBSF::GetFM().SetWeatherPath(pathList);
				WBSF::GetFM().SaveDefaultFileManager();

				FillHourlyDBNameList();
			}
			m_hourlyDBNameCtrl.SelectStringExact(0, title);
		}

	}

	void CWGInputDlg::OnGribsLink()
	{
		CString filter = UtilWin::GetCString(IDS_STR_FILTER_GRIBS);
		CString sFolder;

		CFileDialog openDialog(true, NULL, _T(""), OFN_EXTENSIONDIFFERENT, filter, this);

		if (openDialog.DoModal() == IDOK)
		{
			string filePath = ToUTF8(openDialog.GetPathName());
			string path = GetPath(filePath);
			string title = GetFileTitle(filePath);

			if (!WBSF::GetFM().Normals().IsInDirectoryList(path))
			{
				string pathList = WBSF::GetFM().GetWeatherPath("|") + "|" + path;
				WBSF::GetFM().SetWeatherPath(pathList);
				WBSF::GetFM().SaveDefaultFileManager();

				FillGribsDBNameList();
			}
			m_gribsDBNameCtrl.SelectStringExact(0, title);
		}

	}


	BOOL CWGInputDlg::PreTranslateMessage(MSG* pMsg)
	{
		m_testToolTips.RelayEvent(pMsg);

		return CDialogEx::PreTranslateMessage(pMsg);
	}


	BOOL CALLBACK CWGInputDlg::EnableChildWindow(HWND hwndChild, LPARAM lParam)
	{
		CWnd* pChild = CWnd::FromHandle(hwndChild);
		ASSERT(pChild);

		BOOL bEnable = (BOOL)lParam;
		pChild->EnableWindow(bEnable);

		return TRUE;
	}

	void CWGInputDlg::OnEnable(BOOL bEnable)
	{
		UpdateCtrl(bEnable);
		GetParent()->EnableWindow(bEnable);
	}


	void CWGInputDlg::OnSearchRadiusClick()
	{
		CSearchRadiusDlg dlg(this);
		dlg.m_searchRadius = m_searchRadius;
		if (dlg.DoModal() == IDOK)
		{
			m_searchRadius = dlg.m_searchRadius;
		}
	}

	/*int WinExecWait2(const std::string& command, std::string inputDir, UINT uCmdShow, LPDWORD pExitCode, CCallback& callback)
	{
		SECURITY_ATTRIBUTES sa;
		sa.nLength = sizeof(sa);
		sa.lpSecurityDescriptor = NULL;
		sa.bInheritHandle = TRUE;

		int t = rand();
		CString tmp = GetTempPath() + CString::Format( _T("tmp_File%06d"), t);
			

		HANDLE h = CreateFile(tmp,
			FILE_APPEND_DATA,
			FILE_SHARE_WRITE | FILE_SHARE_READ|
			FILE_ATTRIBUTE_TEMPORARY| FILE_FLAG_DELETE_ON_CLOSE,
			&sa,
			OPEN_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL);

		PROCESS_INFORMATION pi;
		STARTUPINFO si;
		BOOL ret = FALSE;
		DWORD flags = CREATE_NO_WINDOW;

		ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
		ZeroMemory(&si, sizeof(STARTUPINFO));
		si.cb = sizeof(STARTUPINFO);
		si.dwFlags |= STARTF_USESTDHANDLES;
		si.hStdInput = NULL;
		si.hStdError = h;
		si.hStdOutput = h;

		TCHAR cmd[] = TEXT("Test.exe 30");
		ret = CreateProcess(NULL, cmd, NULL, NULL, TRUE, flags, NULL, NULL, &si, &pi);

		if (ret)
		{
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
			return 0;
		}

		return -1;
	}
*/
	void CWGInputDlg::OnDownloadWeather()
	{
		CGenerateWUProjectDlg generateWUProjectDlg(false, this);
		if (generateWUProjectDlg.DoModal() == IDOK)
		{
			ERMsg msg;

			//WBSF::StringVector list = WBSF::GetFM().WeatherUpdate().GetFilesList();
			//m_projectNameCtrl.FillList(list, generateWUProjectDlg.m_project_name);

			//CTRef TRef = GetTRef(s, fileList[i].m_filePath);

			string junk = WBSF::GetFM().WeatherUpdate().GetLocalPath();
			string tmp_path = GetPath(junk) + "tmp\\";
			string wea_path = GetPath(junk) + "..\\Weather\\";
			string scriptFilePath = tmp_path + "script.txt";
			CreateMultipleDir(tmp_path);
			CreateMultipleDir(wea_path);


			ofStream stript;
			msg = stript.open(scriptFilePath);
			if (msg)
			{
				//string input_file_path = generateWUProjectDlg.m_FTP_file_path;
				string file_path_zip = tmp_path + generateWUProjectDlg.m_FTP_file_name;
				//string tmpFilePaht = path + GetFileName(fileList[i].m_filePath);
								//CreateMultipleDir(GetPath(outputFilePaht));

				stript << "open ftp://anonymous:anonymous%40example.com@ftp.cfl.scf.rncan.gc.ca" << endl;

				stript << "cd " << generateWUProjectDlg.m_FTP_path << endl;
				stript << "lcd " << tmp_path << endl;
				stript << "get " << generateWUProjectDlg.m_FTP_file_name << endl;
				stript << "exit" << endl;
				stript.close();

				//call WinSCP
				bool bShow = true;
				string command = "\"" + GetApplicationPath() + "External\\WinSCP.exe\" " + string(bShow ? "/console " : "") + "-timeout=300 -passive=on /log=\"" + scriptFilePath + ".log\" /ini=nul /script=\"" + scriptFilePath;
				DWORD exit_code = 0;
				msg = WBSF::WinExecWait(command, "", SW_SHOW, &exit_code);
				if (msg)
				{
					if (msg && exit_code != 0)
						msg.ajoute("WinSCP.exe was unable to download file: " + generateWUProjectDlg.m_FTP_file_path);

					if (exit_code == 0 && FileExists(file_path_zip))
					{
						//call 7z

						//unzip only .csv file because they are smaller than the zip file
						string command = GetApplicationPath() + "External\\7za.exe x \"" + file_path_zip + "\" -y -o\"" + wea_path + "\"";

						//string command = GetApplicationPath() + "External\\7za.exe e \"" + filePathZip + "\" -y -o\"" + outputPath + "\"" + " \"" + GetFileName(filePathData) + "\"";
						msg = WinExecWait(command, wea_path, SW_SHOW, &exit_code);

						if (msg && exit_code == 0)
						{
							//verify database
						}
						else
						{
							msg.ajoute("7za.exe was unable to unzip file: " + file_path_zip);
						}
					}
				}//if msg


			}//if msg

			if (!msg)
				SYShowMessage(msg, this);

		}//do modal
	}
}

