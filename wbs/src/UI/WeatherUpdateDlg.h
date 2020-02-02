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


#include "Simulation/WeatherUpdater.h"
#include "UI/Common/CommonCtrl.h"
#include "WeatherBasedSimulationUI.h"

namespace WBSF
{

	/////////////////////////////////////////////////////////////////////////////
	// CWeatherUpdateDlg dialog

	class CWeatherUpdateDlg : public CDialogEx
	{
		// Construction
	public:

		CWeatherUpdateDlg(const CExecutablePtr& pParent, CWnd* pParentWnd);   // standard constructor
		CExecutablePtr GetExecutable()const{ return m_weatherUpdate.CopyObject(); }
		void SetExecutable(CExecutablePtr pExecute){ m_weatherUpdate = GetAnalysis(pExecute); }
		CWeatherUpdate & GetAnalysis(const CExecutablePtr& pItem){ ASSERT(pItem); return dynamic_cast<CWeatherUpdate&>(*pItem); }

	protected:

		// Dialog Data
		enum { IDD = IDD_SIM_WEATHER_UPDATE };

		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

		CWeatherUpdate m_weatherUpdate;
		CExecutablePtr m_pParent;

		// Generated message map functions
		DECLARE_MESSAGE_MAP()
		afx_msg void OnWeatherUpdaterManager();
		afx_msg void OnGenerateWeatherUpdater();
		afx_msg void UpdateCtrl();
		
		CDefaultComboBox m_projectNameCtrl;
		COpenDirEditCtrl m_filePathCtrl;

	};

}