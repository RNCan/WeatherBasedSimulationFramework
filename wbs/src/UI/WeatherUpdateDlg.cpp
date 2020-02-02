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

using namespace std;



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
			WBSF::StringVector list = WBSF::GetFM().WeatherUpdate().GetFilesList();
			m_projectNameCtrl.FillList(list, m_weatherUpdate.m_fileTitle);
		}

		DDX_Text(pDX, IDC_NAME, m_weatherUpdate.m_name);
		DDX_Text(pDX, IDC_DESCRIPTION, m_weatherUpdate.m_description);
		//DDX_Control(pDX, IDC_DIRECT_FILE_NAME, m_filePathCtrl1);
		

		DDX_Text(pDX, IDC_WU_PROJECT, m_weatherUpdate.m_fileTitle);
		
		DDX_Control(pDX, IDC_WU_FILEPATH, m_filePathCtrl);

		//DDX_Control(pDX, IDC_WU_FILEPATH2, m_filePathCtrl2);
		DDX_Check(pDX, IDC_SHOW_APP, m_weatherUpdate.m_bShowApp);
		DDX_Text(pDX, IDC_INTERNAL_NAME, m_weatherUpdate.m_internalName);
	}

	
	void CWeatherUpdateDlg::OnGenerateWeatherUpdater()
	{
		CGenerateWUProjectDlg generateWUProjectDlg(true, this);
		if (generateWUProjectDlg.DoModal() == IDOK)
		{
			WBSF::StringVector list = WBSF::GetFM().WeatherUpdate().GetFilesList();
			m_projectNameCtrl.FillList(list, generateWUProjectDlg.m_project_name);
			UpdateCtrl();
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





