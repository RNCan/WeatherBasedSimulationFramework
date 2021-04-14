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
#include "Basic/Registry.h"

namespace WBSF
{

	/////////////////////////////////////////////////////////////////////////////
	// COptionLayerPage dialog

	class CLayersPropertyGridCtrl : public CMFCPropertyGridCtrl
	{
	public:

		

		CLayersPropertyGridCtrl();


		void EnableProperties(BOOL bEnable);
		void EnableProperties(CMFCPropertyGridProperty* pProp, BOOL bEnable);

		virtual BOOL PreTranslateMessage(MSG* pMsg);
		virtual void OnPropertyChanged(CMFCPropertyGridProperty* pProp) const;
		virtual void Init();
		void Initialize();

		std::array<std::string, NB_GEO_LAYERS_KEY> m_filePath;

	protected:


		DECLARE_MESSAGE_MAP()
		afx_msg void OnDestroy();

	};


	class COptionLayerPage : public CMFCPropertyPage
	{

		// Construction
	public:
		COptionLayerPage();
		~COptionLayerPage();


		// Dialog Data
		enum { IDD = IDD_CMN_OPTION_LINK };

		CLayersPropertyGridCtrl m_ctrl;
		

		// Implementation
	protected:
		// Generated message map functions

		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		virtual BOOL OnInitDialog();
		virtual void OnOK();
		

		DECLARE_MESSAGE_MAP()
	};

}