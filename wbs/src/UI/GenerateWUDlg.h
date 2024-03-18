//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//	
//******************************************************************************
#pragma once

#include "UI/Common/OpenDirEditCtrl.h"
#include "UI/Common/CommonCtrl.h"

#include "WeatherBasedSimulationUI.h"

namespace WBSF
{


	

	//COpenDirEditCtrl m_FTPFilePathCtrl;


	class CGenerateWUProjectDlg : public CDialogEx
	{
		// Construction
	public:

		enum { T_HOURLY, T_DAILY, T_NORMALS_PAST, T_NORMALS_CURRENT, T_NORMALS_FUTURE, T_GRIBS, NB_DATABASE_TYPE };


		CGenerateWUProjectDlg(CWnd* pParent = NULL);   // standard constructor
		virtual ~CGenerateWUProjectDlg();


		
		std::string GetGoogleDriveFolderID();


		std::string m_project_name;
		//std::string m_file_URL;
		//std::string m_folder_URL;
		std::string m_folder_id;
		std::string m_file_id;
		std::string m_file_name;
		std::string m_weather_path;
		//std::string m_locale_path;
		CFileInfoVector m_file_list;

		bool m_bShowOutputDir;
		bool m_bShowProject;
		// Implementation
	protected:


		virtual BOOL OnInitDialog();
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		virtual void OnOK();
		virtual BOOL DestroyWindow();

		// Dialog Data
		enum { IDD = IDD_GENERATE_WU};

		

		DECLARE_MESSAGE_MAP()

	
		//bool m_bGenerate;

		void UpdateCtrl(void);
		afx_msg void OnTypeChange();
		afx_msg void OnNameChange();
		afx_msg void OnLocaleDirectoryChange();
		
		CCFLEdit	m_WeatherUpdaterProjectTitleCtrl;
		CCFLComboBox m_databaseTypeCtrl;
		CCFLComboBox m_fileListCtrl;
		CCFLComboBox m_weatherDirectoryCtrl;

		COpenDirEditCtrl m_folderURLCtrl;
		
	};

}