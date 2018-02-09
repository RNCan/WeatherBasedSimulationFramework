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

/////////////////////////////////////////////////////////////////////////////
// CWeatherGenerationDlg dialog

#include "Simulation/WeatherGeneration.h"
#include "Simulation/WeatherGenerator.h"
#include "UI/Common/CommonCtrl.h"

#include "WeatherBasedSimulationUI.h"

namespace WBSF
{

	class CWeatherGenerationDlg : public CDialog
	{
		// Construction
	public:

		friend CWeatherGeneration;

		CWeatherGenerationDlg(const CExecutablePtr& pParent, CWnd* pParentWnd);   // standard constructor

		virtual void SetExecutable(CExecutablePtr pExecutable){ m_WG = GetWeatherGeneration(pExecutable); }
		virtual CExecutablePtr GetExecutable()const{ return CExecutablePtr(new CWeatherGeneration(m_WG)); }

	protected:

		enum { IDD = IDD_SIM_WEATHER_GENERATION };
		CButton		m_linkClimZone;

		CCFLEdit	m_nameCtrl;
		CCFLEdit	m_internalNameCtrl;
		CCFLEdit	m_descriptionCtrl;


		CDefaultComboBox	m_WGInputNameCtrl;
		CCFLComboBox		m_locationsNameCtrl;
		CCFLEdit			m_replicationsCtrl;
		CCFLEdit			m_durationCtrl;
		//CButton				m_useHxGridCtrl;
		CComboBox			m_behaviourCtrl;

		// Overrides
	protected:
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

		// Implementation


		// Generated message map functions
		virtual void OnOK();
		virtual BOOL OnInitDialog();


		afx_msg void OnEditWGInput();
		afx_msg void OnEditLocations();
		afx_msg void OnLocationsChange();
		//afx_msg void OnParametersVariationsChange();


		DECLARE_MESSAGE_MAP()

	private:

		void UpdateCtrl(void);
		void FillWGInput(void);
		void FillLocations(void);
		void UpdateNbLocations();
		//void UpdateNbParametersVariations();
		void GetWGFromInterface();
		void SetWGToInterface();


		CExecutablePtr m_pParent;
		CWeatherGeneration m_WG;
		CWeatherGeneration& GetWeatherGeneration(const CExecutablePtr& pItem){ ASSERT(pItem); return dynamic_cast<CWeatherGeneration&>(*pItem); }

		// to compute number of runs
		size_t m_nbVariations;
		size_t m_nbLocation;


	};


}