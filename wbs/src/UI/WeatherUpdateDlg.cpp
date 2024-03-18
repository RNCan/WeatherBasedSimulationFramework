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

#include "Basic/UtilStd.h"
#include "Basic/Registry.h"
#include "FileManager/FileManager.h"
#include "UI/Common/SYShowMessage.h"
#include "UI/Common/CustomDDX.h"
#include "UI/Common/UtilWin.h"
#include "UI/Common/NewNameDlg.h"
#include "UI/Common/SYShowMessage.h"
#include "UI/GenerateWUDlg.h"

#include "UI/WeatherUpdateDlg.h"
#include "WeatherBasedSimulationString.h"

using namespace std;
using namespace UtilWin;


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace WBSF
{


	//*************************************************************************************************
	// CWeatherUpdateDlg dialog

	BEGIN_MESSAGE_MAP(CWeatherUpdateDlg, CDialogEx)
		ON_BN_CLICKED(IDC_WEATHER_UPDATER_MANAGER, &CWeatherUpdateDlg::OnWeatherUpdaterManager)
		ON_BN_CLICKED(IDC_WU_GENERATE, &CWeatherUpdateDlg::OnGenerateWeatherUpdater)
		ON_CBN_SELCHANGE(IDC_WU_PROJECT, &UpdateCtrl)
	END_MESSAGE_MAP()


	CWeatherUpdateDlg::CWeatherUpdateDlg(const CExecutablePtr& pParent, CWnd* pParentWnd) :
		CDialogEx(CWeatherUpdateDlg::IDD, pParentWnd),
		m_pParent(pParent),
		m_projectNameCtrl(_T(""))
	{
	}



	void CWeatherUpdateDlg::DoDataExchange(CDataExchange* pDX)
	{
		CDialogEx::DoDataExchange(pDX);

		if (!pDX->m_bSaveAndValidate)
		{
			DDX_Control(pDX, IDC_WU_PROJECT, m_projectNameCtrl);
			DDX_Control(pDX, IDC_WU_FILEPATH, m_filePathCtrl);

			WBSF::StringVector list = WBSF::GetFM().WeatherUpdate().GetFilesList();
			m_projectNameCtrl.FillList(list, m_weatherUpdate.m_fileTitle);
			
			UpdateCtrl();
		}

		DDX_Text(pDX, IDC_NAME, m_weatherUpdate.m_name);
		DDX_Text(pDX, IDC_DESCRIPTION, m_weatherUpdate.m_description);
		DDX_Text(pDX, IDC_WU_PROJECT, m_weatherUpdate.m_fileTitle);
		DDX_Check(pDX, IDC_SHOW_APP, m_weatherUpdate.m_bShowApp);
		DDX_Text(pDX, IDC_INTERNAL_NAME, m_weatherUpdate.m_internalName);
	}

	
	void CWeatherUpdateDlg::OnGenerateWeatherUpdater()
	{
		CGenerateWUProjectDlg dlg(this);
		if (dlg.DoModal() == IDOK)
		{
			ERMsg msg;

			if (!dlg.m_project_name.empty())
			{
				if (!dlg.m_file_name.empty())
				{
					string WU_file_path = WBSF::GetFM().WeatherUpdate().GetLocalPath() + dlg.m_project_name;

					bool bSave = true;
					if (FileExists(WU_file_path))
					{
						CString sOutMessage;
						AfxFormatString1(sOutMessage, IDS_RT_ERRFILEEXIST, CString(dlg.m_project_name.c_str()));
						int retCode = MessageBox(sOutMessage, AfxGetAppName(), MB_ICONQUESTION | MB_OKCANCEL);
						bSave = retCode == IDOK;
					}

					if (bSave)
					{
						msg = CWeatherUpdate::GenerateWUProject(WU_file_path, dlg.m_file_id, dlg.m_weather_path);
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
		

			if (msg)
			{
				WBSF::StringVector list = WBSF::GetFM().WeatherUpdate().GetFilesList();
				m_projectNameCtrl.FillList(list, dlg.m_project_name);
				UpdateCtrl();
			}
		}

	}

	void CWeatherUpdateDlg::OnWeatherUpdaterManager()
	{
		ERMsg msg;

		std::string WU_project_name = m_projectNameCtrl.GetString();

		while (WU_project_name.empty())
		{
			CNewNameDlg dlg(this);
			if (dlg.DoModal() != IDOK)
				return;

			SetFileExtension(dlg.m_name, WBSF::GetFM().WeatherUpdate().GetExtensions());

			if (!WBSF::GetFM().WeatherUpdate().FileExists(dlg.m_name))
			{
				ofStream f;
				string filePath = WBSF::GetFM().WeatherUpdate().GetLocalPath() + dlg.m_name;
				msg = f.open(filePath);
				if (msg)
				{
					f << "<?xml version=\"1.0\" encoding=\"Windows-1252\"?>" << endl;
					f << "<WeatherUpdater version=\"2\">" << endl;
					f << "</WeatherUpdater>" << endl;
					f.close();


					WU_project_name = dlg.m_name;
					m_projectNameCtrl.AddString(dlg.m_name);
					m_projectNameCtrl.SelectStringExact(0, dlg.m_name);

				}
			}

			if (!msg)
				UtilWin::SYShowMessage(msg, this);
		}

		ENSURE(!WU_project_name.empty());

		string filePath;
		msg = WBSF::GetFM().WeatherUpdate().GetFilePath(WU_project_name, filePath);
		if (msg)
			msg = CallApplication(CRegistry::WEATHER_UPDATER, filePath, NULL, SW_SHOW);

		if (!msg)
			UtilWin::SYShowMessage(msg, this);
	}


	void CWeatherUpdateDlg::UpdateCtrl()
	{
		string project_name = m_projectNameCtrl.GetString();
		string filePath = WBSF::GetFM().WeatherUpdate().GetFilePath(project_name);
		m_filePathCtrl.SetWindowText(filePath);

	}



}





