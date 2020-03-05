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

//#include "Simulation/Mapping.h"
//#include "Simulation/Executable.h"

#include "UI/Common/CheckableGroupBox.h"
#include "Basic/QGISPalette.h"
#include "UI/Common/CommonCtrl.h"
//#include "UI/Common/OpenDirEditCtrl.h"
#include "WeatherBasedSimulationUI.h"

namespace WBSF
{

//	class CSimulation;

	/////////////////////////////////////////////////////////////////////////////
	// CCreateQGISStyleDlg dialog

	class CCreateQGISStyleDlg : public CDialogEx
	{
		// Construction
	public:
		
		CCreateQGISStyleDlg(CWnd* pParentWnd);   // standard constructor

		CCreateStyleOptions m_options;

		CCheckableGroupBox m_createStyle;
		CCFLComboBox	m_nameCtrl;
		CCFLComboBox	m_colorRampCtrl;
		CIntEdit		m_nbClassesCtrl;
		CFloatEdit		m_classesSizeCtrl;
		CFloatEdit		m_varFactorCtrl;
		CFloatEdit		m_userMinCtrl;
		CFloatEdit		m_userMaxCtrl;
		CButton			m_reversePaletteCtrl;
		CCFLEdit		m_numberFormatCtrl;
		CCFLEdit		m_dateFormatCtrl;
		CButton			m_ordinalDateCtrl;

		
		
		void InitPaletteNameList();

		size_t GetBreakType();
		size_t GetMinMaxType();


	protected:

		// Dialog Data
		enum { IDD = IDD_CREATE_STYLE};

		virtual BOOL OnInitDialog();
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support


		// Implementation
	protected:

		// Generated message map functions
		afx_msg void UpdateCtrl();
		afx_msg void OnGetFormatClick(NMHDR *pNMHDR, LRESULT *pResult);
		DECLARE_MESSAGE_MAP()

		
	};
}
