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
		ON_BN_CLICKED(IDC_SCIPT_MANAGER, &OnSciptManager)
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
			list1.insert(list1.begin(), "");
			m_listCtrl.FillList(list1, m_script.m_scriptFileName);

			WBSF::StringVector list2 = GetFilesList(WBSF::GetFM().GetOutputPath() + "*.*", FILE_NAME);
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

		std::string file_name = m_listCtrl.GetString();

		while (file_name.empty())
		{
			CNewNameDlg dlg(this);
			if (dlg.DoModal() != IDOK)
				return;

			file_name = dlg.m_name;
			SetFileExtension(file_name, ".R");

			if (!WBSF::GetFM().Script().FileExists(file_name))
			{
				ofStream f;
				string filePath = WBSF::GetFM().Script().GetLocalPath() + file_name;
				msg = f.open(filePath);
				if (msg)
				{
					string path = GetPath(filePath);
					ReplaceString(path, "\\", "/");

					string data_name = m_inputCtrl.GetString();
					if (data_name.empty())
						data_name = "DataName.csv";

					f << "cat(\"\\014\")" << endl;
					f << "rm(list=ls())" << endl;
					f << "graphics.off()" << endl;
					f << "Resolution=300" << endl;
					f << "\n" << endl;
					f << "GetScriptPath <- function()" << endl;
					f << "{" << endl;
					f << "    argv <- commandArgs(trailingOnly = FALSE)" << endl;
					f << "    if (any(grepl(\"--interactive\", argv))) {" << endl;
					f << "        GetScriptPath <-\"" + path + "\"" << endl;
					f << "    } else {" << endl;
					f << "        GetScriptPath <- paste(dirname(substring(argv[grep(\"--file=\", argv)],8)), \"/\", sep='')" << endl;
					f << "    }" << endl;
					f << "}" << endl;
					f << "\n" << endl;
					f << "GetFilePath <- function(name)" << endl;
					f << "{" << endl;
					f << "    GetFilePath <- paste(GetScriptPath(), name, sep='')" << endl;
					f << "}" << endl;
					f << "\n" << endl;
					f << "GetInputFilePath <- function()" << endl;
					f << "{" << endl;
					f << "    argv <- commandArgs(trailingOnly = FALSE)" << endl;
					f << "    if (any(grepl(\"--interactive\", argv))) { " << endl;
					f << "         GetInputFilePath <- GetFilePath(\"../output/" + data_name + "\")" << endl;
					f << "    } else { GetInputFilePath <- argv[grep(\"--args\", argv)+1] " << endl;
					f << "    }" << endl;
					f << "}" << endl;
					f << "\n" << endl;
					f << "dir.create(GetFilePath(\"../Images/\"), showWarnings = FALSE)" << endl;
					f << "Esim <- read.csv(GetInputFilePath())" << endl;
					f << "str(Esim)" << endl;
					f.close();
				}
			}

			if (!msg)
				UtilWin::SYShowMessage(msg, this);
		}

		ENSURE(!file_name.empty());

		string filePath;
		msg = WBSF::GetFM().Script().GetFilePath(file_name, filePath);
		if (msg)
			msg = CallApplication(CRegistry::TEXT_EDITOR, filePath, NULL, SW_SHOW);

		if (!msg)
			UtilWin::SYShowMessage(msg, this);

		WBSF::StringVector list1 = WBSF::GetFM().Script().GetFilesList();
		list1.insert(list1.begin(), "");
		m_listCtrl.FillList(list1, file_name);
	}


}





