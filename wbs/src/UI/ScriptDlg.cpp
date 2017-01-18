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

#include "Basic/Registry.h"
#include "FileManager/FileManager.h"
#include "UI/Common/SYShowMessage.h"
#include "UI/Common/CustomDDX.h"
#include "UI/Common/UtilWin.h"
#include "UI/Common/NewNameDlg.h"
#include "UI/Common/SYShowMessage.h"
#include "ScriptDlg.h"

using namespace std;
using namespace WBSF::DIMENSION;


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace WBSF
{

	//*************************************************************************************************
	// CScriptDlg dialog

	BEGIN_MESSAGE_MAP(CScriptDlg, CDialog)
		ON_BN_CLICKED(IDC_SCIPT_MANAGER, &CScriptDlg::OnSciptManager)
	END_MESSAGE_MAP()


	CScriptDlg::CScriptDlg(const CExecutablePtr& pParent, CWnd* pParentWnd/*=NULL*/) :
		CDialog(CScriptDlg::IDD, pParentWnd),
		m_pParent(pParent)
	{
	}



	void CScriptDlg::DoDataExchange(CDataExchange* pDX)
	{
		CDialog::DoDataExchange(pDX);

		if (!pDX->m_bSaveAndValidate)
		{
			DDX_Control(pDX, IDC_SCRIPT, m_listCtrl);
			DDX_Control(pDX, IDC_INPUT, m_inputCtrl);
			DDX_Control(pDX, IDC_OUTPUT, m_outputCtrl);

			WBSF::StringVector list1 = WBSF::GetFM().Script().GetFilesList();
			m_listCtrl.FillList(list1, m_script.m_scriptFileName);

			WBSF::StringVector list2 = GetFilesList(WBSF::GetFM().GetOutputPath()+"*.*", FILE_NAME);
			m_inputCtrl.FillList(list2, m_script.m_inputFileName);
		}

		DDX_Text(pDX, IDC_NAME, m_script.m_name);
		DDX_Text(pDX, IDC_DESCRIPTION, m_script.m_description);
		DDX_Text(pDX, IDC_INTERNAL_NAME, m_script.m_internalName);
		DDX_Text(pDX, IDC_SCRIPT, m_script.m_scriptFileName);
		DDX_Text(pDX, IDC_INPUT, m_script.m_inputFileName);
		DDX_Text(pDX, IDC_OUTPUT, m_script.m_outputFileName);

	}


	void CScriptDlg::OnSciptManager()
	{
		ERMsg msg;

		std::string updater = m_listCtrl.GetString();

		while (updater.empty())
		{
			CNewNameDlg dlg(this);
			if (dlg.DoModal() != IDOK)
				return;


			if (!WBSF::GetFM().Script().FileExists(dlg.m_name))
			{
				updater = dlg.m_name;
				ofStream f;
				string filePath;
				msg = WBSF::GetFM().Script().GetFilePath(updater, filePath);
				if (msg)
				{
					f << "<?xml version=\"1.0\" encoding=\"Windows-1252\"?>" << endl;
					f << "<WeatherUpdater version=\"2\">" << endl;
					f << "</WeatherUpdater>" << endl;
					f.close();
				}
			}

			if (!msg)
				UtilWin::SYShowMessage(msg, this);
		}

		ENSURE(!updater.empty());

		string filePath;
		msg = WBSF::GetFM().Script().GetFilePath(updater, filePath);
		if (msg)
			msg = CallApplication(CRegistry::WEATHER_UPDATER, filePath, NULL, SW_SHOW);

		if (!msg)
			UtilWin::SYShowMessage(msg, this);
	}


}





