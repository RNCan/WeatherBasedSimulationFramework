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
	CGenerateWUProjectDlg::CGenerateWUProjectDlg(bool bGenerate, CWnd* pParent) :
		m_bGenerate(bGenerate),
		CDialogEx(CGenerateWUProjectDlg::IDD, pParent)
	{
	}

	CGenerateWUProjectDlg::~CGenerateWUProjectDlg()
	{
	}


	void CGenerateWUProjectDlg::DoDataExchange(CDataExchange* pDX)
	{
		CDialogEx::DoDataExchange(pDX);

		DDX_Control(pDX, IDC_WU_PROJECT_TITLE, m_WeatherUpdaterProjectTitleCtrl);
		DDX_Control(pDX, IDC_DATABASE_TYPE, m_databaseTypeCtrl);
		DDX_Control(pDX, IDC_FTP_FILE_NAME, m_FTPFileNameCtrl);
		DDX_Control(pDX, IDC_FTP_FILEPATH, m_FTPFilePathCtrl);
		DDX_Control(pDX, IDC_LOCALE_DIRECTORY, m_localeDirectoryCtrl);
		
		
		
		if (m_bGenerate)
		{
			//do not select output directoy when generate
			GetDlgItem(IDC_LOCALE_DIRECTORY)->EnableWindow(FALSE);
		}
		else
		{
			CString title;
			GetDlgItemText(IDC_STATIC1, title);
			SetWindowText(title);

			CWnd* pOKCtrl = GetDlgItem(IDOK);
			ASSERT(pOKCtrl);
			GetDlgItemText(IDC_STATIC2, title);
			pOKCtrl->SetWindowText(title);


			GetDlgItem(IDC_STATIC3)->ShowWindow(SW_HIDE);
			m_WeatherUpdaterProjectTitleCtrl.ShowWindow(SW_HIDE);
		}
	}


	BEGIN_MESSAGE_MAP(CGenerateWUProjectDlg, CDialogEx)

		ON_CBN_SELCHANGE(IDC_DATABASE_TYPE, &OnTypeChange)
		ON_CBN_SELCHANGE(IDC_FTP_FILE_NAME, &OnNameChange)
		ON_CBN_SELCHANGE(IDC_LOCALE_DIRECTORY, &OnLocaleDirectoryChange)
	END_MESSAGE_MAP()

	static const char* SERVER_NAME = "ftp.cfl.scf.rncan.gc.ca";

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
		StringVector paths (FM.GetWeatherPath(), "|");
		paths.insert(paths.begin(), FM.GetProjectPath() + "Weather\\");
		
		
		m_localeDirectoryCtrl.FillList(paths, paths[0]);
		
		//UpdateCtrl();

		return TRUE;  // return TRUE unless you set the focus to a control
	}

	void CGenerateWUProjectDlg::UpdateCtrl()
	{

		BOOL bEnable = m_FTPFileNameCtrl.GetCurSel() != CB_ERR;

		CWnd* pOKCtrl = GetDlgItem(IDOK);
		ASSERT(pOKCtrl);

		pOKCtrl->EnableWindow(bEnable);



		string FTP_path = CGenerateWUProjectDlg::GetFTPPath();
		string file_name = m_FTPFileNameCtrl.GetString();

		string filePath = SERVER_NAME + FTP_path + file_name;
		ReplaceString(filePath, "\\", "/");

		m_FTPFilePathCtrl.SetWindowText("ftp://" + filePath);
	}

	string CGenerateWUProjectDlg::GetFTPPath()
	{


		//Fill FTP database available
		int type = m_databaseTypeCtrl.GetCurSel();
		string FTP_path;

		switch (type)
		{
		case T_HOURLY: FTP_path = "/regniere/Data11/Weather/Hourly/"; break;
		case T_DAILY: FTP_path = "/regniere/Data11/Weather/Daily/"; break;
		case T_NORMALS_PAST: FTP_path = "/regniere/Data11/Weather/Normals/past/"; break;
		case T_NORMALS_CURRENT: FTP_path = "/regniere/Data11/Weather/Normals/"; break;
		case T_NORMALS_FUTURE: FTP_path = "/regniere/Data11/Weather/Normals/ClimateChange/"; break;
		case T_GRIBS: FTP_path = "/regniere/Data11/Weather/Gribs/"; break;
		default: ASSERT(false);
		}

		return FTP_path;
	}

	void CGenerateWUProjectDlg::OnTypeChange()
	{
		string FTP_path = GetFTPPath();

		CFileInfoVector file_list;
		ERMsg msg = GetFTPFileList(FTP_path, file_list);
		if (msg)
		{
			m_FTPFileNameCtrl.ResetContent();
			for (size_t i = 0; i < file_list.size(); i++)
				m_FTPFileNameCtrl.AddString(CString(GetFileName(file_list[i].m_filePath).c_str()));
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
			string file_name = m_FTPFileNameCtrl.GetString();
			SetFileExtension(file_name, "");
			m_WeatherUpdaterProjectTitleCtrl.SetString(file_name);
		}


		UpdateCtrl();
	}

	ERMsg CGenerateWUProjectDlg::GetFTPFileList(const string& FTP_path, CFileInfoVector& fileList)
	{

		ERMsg msg;

		CFtpConnectionPtr pConnection;
		CInternetSessionPtr pSession;


		msg = GetFtpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", false, 5);
		if (msg)
		{
			msg = FindFiles(pConnection, FTP_path + "*.*", fileList, false, CCallback::DEFAULT_CALLBACK);
			pConnection->Close();
			pSession->Close();
		}

		return msg;
	}

	void CGenerateWUProjectDlg::OnOK()
	{
		ERMsg msg;

		CString fileName = m_WeatherUpdaterProjectTitleCtrl.GetWindowText();
		m_project_name = (LPCSTR)CStringA(fileName);
		if (!m_project_name.empty())
			SetFileExtension(m_project_name, WBSF::GetFM().WeatherUpdate().GetExtensions());


		m_FTP_path = GetFTPPath();
		m_FTP_file_name = m_FTPFileNameCtrl.GetString();
		m_FTP_file_path = m_FTP_path + m_FTP_file_name;
		m_locale_path = m_localeDirectoryCtrl.GetString();


		if (m_bGenerate)
		{
			if (!m_project_name.empty())
			{
				if (!m_FTP_file_name.empty())
				{
					string WU_file_path = WBSF::GetFM().WeatherUpdate().GetLocalPath() + m_project_name;

					bool bSave = true;
					if (FileExists(WU_file_path))
					{
						CString sOutMessage;
						AfxFormatString1(sOutMessage, IDS_RT_ERRFILEEXIST, fileName);
						int retCode = MessageBox(sOutMessage, AfxGetAppName(), MB_ICONQUESTION | MB_OKCANCEL);
						bSave = retCode == IDOK;
					}

					if (bSave)
					{
						msg = CWeatherUpdate::GenerateWUProject(WU_file_path, SERVER_NAME, m_FTP_file_path);

						if (msg)
						{
							CDialogEx::OnOK();
						}
					}
				}
				else
				{
					msg.ajoute(GetString(IDS_BSC_NAME_EMPTY));
				}
			}
			else
			{
				msg.ajoute(GetString(IDS_BSC_NAME_EMPTY));
			}

			if (!msg)
				SYShowMessage(msg, this);
		}
		else
		{
			CDialogEx::OnOK();
		}
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
