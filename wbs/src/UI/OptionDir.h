//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once


#include "UI/Common/CommonCtrl.h"
#include "WeatherBasedSimulationUI.h"

namespace WBSF
{

	class CPathListBox : public CVSListBox
	{
		DECLARE_DYNAMIC(CPathListBox)

		// Construction
	public:
		CPathListBox()
		{}

		void EnableBrowse();
		void DoDataExchange(CDataExchange* pDX);

		virtual void OnBrowse();


		virtual BOOL EditItem(int iIndex)
		{
			if (!__super::EditItem(iIndex))
				return FALSE;

			return TRUE;
		}

		DECLARE_MESSAGE_MAP()
	};
	/////////////////////////////////////////////////////////////////////////////
	// COptionDir dialog

	class COptionDir : public CMFCPropertyPage
	{

		// Construction
	public:
		COptionDir();
		~COptionDir();



		// Dialog Data

		enum { IDD = IDD_CMN_OPTION_DIR };
		COpenDirEditCtrl	m_appPathCtrl;
		COpenDirEditCtrl	m_modelCtrl;
		CPathListBox		m_weatherPathCtrl;
		CPathListBox		m_mapPathCtrl;
		CVSListBox			m_mapExtCtrl;
		COpenDirEditCtrl	m_projectMapPathCtrl;
		COpenDirEditCtrl	m_projectWeatherPathCtrl;


		// Implementation
	protected:


		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		virtual void OnOK();
		virtual BOOL OnInitDialog();
		// Generated message map functions

		DECLARE_MESSAGE_MAP()
		afx_msg void OnOptionWeatherBrowse();

	};

}