//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
// 31-01-2020	Rémi Saint-Amant	create
//****************************************************************************
#include "stdafx.h"


#include "Basic/GoogleDrive.h"
#include "Basic/CallcURL.h"
#include "FileManager/FileManager.h"
#include "Simulation/WeatherUpdater.h"
#include "UI/Common/AppOption.h"
#include "UI/Common/UtilWin.h"
#include "UI/Common/SYShowMessage.h"
#include "UI/Common/UtilWWW.h"
#include "UI/GenerateWUDlg.h"



#include "WeatherBasedSimulationString.h"



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace UtilWin;
using namespace UtilWWW;
using namespace std;


namespace WBSF
{

	

	//************************************************************************
	CGenerateWUProjectDlg::CGenerateWUProjectDlg( CWnd* pParent) :
		//m_bGenerate(bGenerate),
		CDialogEx(CGenerateWUProjectDlg::IDD, pParent)
	{
		bool m_bShowOutputDir = true;
		bool m_bShowProject = true;
	}

	CGenerateWUProjectDlg::~CGenerateWUProjectDlg()
	{
	}


	void CGenerateWUProjectDlg::DoDataExchange(CDataExchange* pDX)
	{
		CDialogEx::DoDataExchange(pDX);

		DDX_Control(pDX, IDC_WU_PROJECT_TITLE, m_WeatherUpdaterProjectTitleCtrl);
		DDX_Control(pDX, IDC_DATABASE_TYPE, m_databaseTypeCtrl);
		DDX_Control(pDX, IDC_FTP_FILE_NAME, m_fileListCtrl);
		DDX_Control(pDX, IDC_FTP_FILEPATH, m_folderURLCtrl);
		DDX_Control(pDX, IDC_LOCALE_DIRECTORY, m_weatherDirectoryCtrl);



		//if (m_bGenerate)
		//{
		//	//do not select output directory when generate
		//	GetDlgItem(IDC_LOCALE_DIRECTORY)->EnableWindow(FALSE);
		//}
		//else
		//{
		CString title;
		GetDlgItemText(IDC_STATIC1, title);
		SetWindowText(title);

		//CWnd* pOKCtrl = GetDlgItem(IDOK);
		//ASSERT(pOKCtrl);
		//GetDlgItemText(IDC_STATIC2, title);
		//pOKCtrl->SetWindowText(title);

		//bool m_bShowOutputDir = false;
		GetDlgItem(IDC_STATIC4)->ShowWindow(m_bShowOutputDir ? SW_SHOW : SW_HIDE);
		m_weatherDirectoryCtrl.ShowWindow(m_bShowOutputDir ? SW_SHOW : SW_HIDE);


			
		//bool m_bShowProject = false;
		GetDlgItem(IDC_STATIC3)->ShowWindow(m_bShowProject ? SW_SHOW : SW_HIDE);
		m_WeatherUpdaterProjectTitleCtrl.ShowWindow(m_bShowProject ? SW_SHOW : SW_HIDE);

			
			
		//}
	}


	BEGIN_MESSAGE_MAP(CGenerateWUProjectDlg, CDialogEx)

		ON_CBN_SELCHANGE(IDC_DATABASE_TYPE, &OnTypeChange)
		ON_CBN_SELCHANGE(IDC_FTP_FILE_NAME, &OnNameChange)
		ON_CBN_SELCHANGE(IDC_LOCALE_DIRECTORY, &OnLocaleDirectoryChange)
	END_MESSAGE_MAP()

	//static const char* SERVER_NAME = "ftp.cfl.scf.rncan.gc.ca";

	/////////////////////////////////////////////////////////////////////////////
	// CGenerateWUProjectDlg msg handlers

	BOOL CGenerateWUProjectDlg::OnInitDialog()
	{
		CDialogEx::OnInitDialog();

		CAppOption option(_T("WindowsPosition"));

		CPoint pt = option.GetProfilePoint(_T("GenerateWUDlg"), CPoint(30, 30));
		UtilWin::EnsurePointOnDisplay(pt);
		SetWindowPos(NULL, pt.x, pt.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

		int curType = option.GetProfileInt(_T("WUGenerateDBType"), 1);
		m_databaseTypeCtrl.SetCurSel(curType);
		OnTypeChange();



		CFileManager& FM = GetFileManager();
		StringVector paths(FM.GetWeatherPath(), "|");
		paths.insert(paths.begin(), FM.GetProjectPath() + "Weather\\");


		m_weatherDirectoryCtrl.FillList(paths, paths[0]);

		//UpdateCtrl();

		return TRUE;  // return TRUE unless you set the focus to a control
	}

	void CGenerateWUProjectDlg::UpdateCtrl()
	{

		BOOL bEnable = m_fileListCtrl.GetCurSel() != CB_ERR;

		CWnd* pOKCtrl = GetDlgItem(IDOK);
		ASSERT(pOKCtrl);

		pOKCtrl->EnableWindow(bEnable);

		size_t pos = (size_t)m_fileListCtrl.GetCurSel();
		/*string id;
		if (pos != NOT_INIT)
		{
			ASSERT(pos < m_file_list.size());
			id = GetPartID(m_file_list[pos].m_filePath);
		}*/
		//m_FTPFilePathCtrl.SetWindowText("https://drive.google.com/file/d/" + id);

		m_folderURLCtrl.SetWindowText(CGoogleDrive::GetURLFromFolderID(GetGoogleDriveFolderID()) + "/");//add id of the file will be remove in 

	}

	string CGenerateWUProjectDlg::GetGoogleDriveFolderID()
	{


		//Fill FTP database available
		int type = m_databaseTypeCtrl.GetCurSel();
		string folder_id;

		switch (type)
		{
		case T_HOURLY: folder_id = "103Rw3VUQ3B93rp7xml14JUiq_LNLpQKH"; break;
		case T_DAILY: folder_id = "1KZBlYp54URTP3eoMHZcvGI_BO6LrxW1Q"; break;
		case T_NORMALS_PAST: folder_id = "1-ken5YCyE3dhEu3nKtJ1qVIiVpKpwCrI"; break;
		case T_NORMALS_CURRENT: folder_id = "1NKlamU4ytsq3ahSTzAISq4t6U5NIRl4G"; break;
		case T_NORMALS_FUTURE: folder_id = "15xe2hsTIq_kvKzoSuoVfVMxpXa7Gs44X"; break;
		case T_GRIBS: folder_id = "106zhz9deYMyKb9mnAinZGoyoMKD_i_LM"; break;

		default: ASSERT(false);
		}

		return folder_id;
	}

	void CGenerateWUProjectDlg::OnTypeChange()
	{
		//string link = GetGoogleDriveLink();
		string folder_id= GetGoogleDriveFolderID();

		m_file_list.clear();
		//ERMsg msg = GetGoogleDriveFileList(link, m_file_list);
		ERMsg msg = CGoogleDrive::GetFolderFileList(folder_id, m_file_list);
		if (msg)
		{
			m_fileListCtrl.ResetContent();
			for (size_t i = 0; i < m_file_list.size(); i++)
			{
				string name = CGoogleDrive::GetPartName(m_file_list[i].m_filePath);
				m_fileListCtrl.AddString(CString(name.c_str()));
			}
		}
		else
		{
			SYShowMessage(msg, this);
		}

		UpdateCtrl();
	}


	void CGenerateWUProjectDlg::OnNameChange()
	{
		string project_name = m_WeatherUpdaterProjectTitleCtrl.GetString();
		if (project_name.empty())
		{
			size_t pos = (size_t)m_fileListCtrl.GetCurSel();
			string name;

			if (pos != NOT_INIT)
			{
				ASSERT(pos < m_file_list.size());
				name = CGoogleDrive::GetPartName(m_file_list[pos].m_filePath);
				SetFileExtension(name, "");
			}

			m_WeatherUpdaterProjectTitleCtrl.SetString(name);
		}


		UpdateCtrl();
	}

	
	void CGenerateWUProjectDlg::OnOK()
	{
		ERMsg msg;

		CString fileName = m_WeatherUpdaterProjectTitleCtrl.GetWindowText();
		m_project_name = (LPCSTR)CStringA(fileName);
		if (!m_project_name.empty())
			SetFileExtension(m_project_name, WBSF::GetFM().WeatherUpdate().GetExtensions());

		size_t pos = (size_t)m_fileListCtrl.GetCurSel();
		string name;
		string id;

		if (pos != NOT_INIT)
		{
			ASSERT(pos < m_file_list.size());
			name = CGoogleDrive::GetPartName(m_file_list[pos].m_filePath);
			id = CGoogleDrive::GetPartID(m_file_list[pos].m_filePath);

		}
		
		m_folder_id = GetGoogleDriveFolderID();
		m_file_id = id;
		m_file_name = name;
		m_weather_path = m_weatherDirectoryCtrl.GetString();

		CDialogEx::OnOK();

	}

	BOOL CGenerateWUProjectDlg::DestroyWindow()
	{
		CRect rect;
		GetWindowRect(rect);

		CAppOption option(_T("WindowsPosition"));
		CPoint pt = rect.TopLeft();
		option.WriteProfilePoint(_T("GenerateWUDlg"), pt);

		int curType = m_databaseTypeCtrl.GetCurSel();
		option.WriteProfileInt(_T("WUGenerateDBType"), curType);




		BOOL bDestroy = CDialogEx::DestroyWindow();

		ASSERT(GetSafeHwnd() == NULL);

		return bDestroy;
	}


}

void WBSF::CGenerateWUProjectDlg::OnLocaleDirectoryChange()
{
	UpdateCtrl();
}
