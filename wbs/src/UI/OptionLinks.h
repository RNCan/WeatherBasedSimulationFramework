//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once


#include "UI/Common/MFCEditBrowseCtrlEx.h"
#include "WeatherBasedSimulationUI.h"

namespace WBSF
{

	/////////////////////////////////////////////////////////////////////////////
	// COptionLinks dialog

	class CAppLinkPropertyGridCtrl : public CMFCPropertyGridCtrl
	{
	public:

		enum TLink
		{
			L_BIOSIM, L_SHOWMAP, L_HOURLY_EDITOR, L_DAILY_EDITOR, L_NORMAL_EDITOR, L_MODEL_EDITOR,
			L_MATCH_STATION, L_WEATHER_UPDATER, L_FTP_TRANSFER, L_TDATE, L_MERGEFILE,
			L_TEXT_EDITOR, L_XML_EDITOR, L_SPREADSHEET1, L_SPREADSHEET2, L_GIS, L_R_SCRIPT,
			NB_LINKS
		};


		CAppLinkPropertyGridCtrl();


		void EnableProperties(BOOL bEnable);
		void EnableProperties(CMFCPropertyGridProperty* pProp, BOOL bEnable);

		virtual BOOL PreTranslateMessage(MSG* pMsg);
		virtual void OnPropertyChanged(CMFCPropertyGridProperty* pProp) const;
		virtual void Init();
		void Initialize();

		std::array<std::string, NB_LINKS> m_filePath;

	protected:


		DECLARE_MESSAGE_MAP()
		afx_msg void OnDestroy();

	};


	class COptionLinks : public CMFCPropertyPage
	{

		// Construction
	public:
		COptionLinks();
		~COptionLinks();


		// Dialog Data
		enum { IDD = IDD_CMN_OPTION_LINK };

		CAppLinkPropertyGridCtrl m_ctrl;
		

		// Implementation
	protected:
		// Generated message map functions

		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		virtual BOOL OnInitDialog();
		virtual void OnOK();
		

		DECLARE_MESSAGE_MAP()
	};

}