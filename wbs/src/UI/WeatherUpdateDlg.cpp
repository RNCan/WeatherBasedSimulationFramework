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

#include "WeatherUpdateDlg.h"

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

	BEGIN_MESSAGE_MAP(CWeatherUpdateDlg, CDialog)
		ON_BN_CLICKED(IDC_WEATHER_UPDATER_MANAGER, &CWeatherUpdateDlg::OnWeatherUpdaterManager)
	END_MESSAGE_MAP()


	CWeatherUpdateDlg::CWeatherUpdateDlg(const CExecutablePtr& pParent, CWnd* pParentWnd) :
		CDialog(CWeatherUpdateDlg::IDD, pParentWnd),
		m_pParent(pParent),
		m_listCtrl(_T(""))
	{
	}



	void CWeatherUpdateDlg::DoDataExchange(CDataExchange* pDX)
	{
		CDialog::DoDataExchange(pDX);

		if (!pDX->m_bSaveAndValidate)
		{
			DDX_Control(pDX, IDC_KIND, m_listCtrl);
			WBSF::StringVector list = WBSF::GetFM().WeatherUpdate().GetFilesList();
			m_listCtrl.FillList(list, m_weatherUpdate.m_fileTitle);
		}

		DDX_Text(pDX, IDC_NAME, m_weatherUpdate.m_name);
		DDX_Text(pDX, IDC_DESCRIPTION, m_weatherUpdate.m_description);
		DDX_Text(pDX, IDC_INTERNAL_NAME, m_weatherUpdate.m_internalName);
		DDX_Text(pDX, IDC_KIND, m_weatherUpdate.m_fileTitle);
		DDX_Check(pDX, IDC_SHOW_APP, m_weatherUpdate.m_bShowApp);
	}

	void CWeatherUpdateDlg::OnWeatherUpdaterManager()
	{
		ERMsg msg;
		
		std::string updater = m_listCtrl.GetString();

		while(updater.empty())
		{
			CNewNameDlg dlg(this);
			if (dlg.DoModal() != IDOK)
				return;

			
			if (!WBSF::GetFM().WeatherUpdate().FileExists(dlg.m_name))
			{
				ofStream f;
				string filePath = WBSF::GetFM().WeatherUpdate().GetLocalPath() + dlg.m_name + WBSF::GetFM().WeatherUpdate().GetExtensions();
				msg = f.open(filePath);
				if (msg)
				{
					f << "<?xml version=\"1.0\" encoding=\"Windows-1252\"?>" << endl;
					f << "<WeatherUpdater version=\"2\">" << endl;
					f << "</WeatherUpdater>" << endl;
					f.close();


					updater = dlg.m_name;
					m_listCtrl.AddString(dlg.m_name);
					m_listCtrl.SelectStringExact(0, dlg.m_name);

				}
			}
			
			if(!msg)
				UtilWin::SYShowMessage(msg, this);
		}

		ENSURE(!updater.empty());

		string filePath;
		msg = WBSF::GetFM().WeatherUpdate().GetFilePath(updater, filePath);
		if (msg)
			msg = CallApplication(CRegistry::WEATHER_UPDATER, filePath, NULL, SW_SHOW);

		if (!msg)
			UtilWin::SYShowMessage(msg, this);
	}



}





	