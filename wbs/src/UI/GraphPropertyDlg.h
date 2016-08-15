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

#include "Simulation/Graph.h"
#include "UI/Common/CommonCtrl.h"

#include "WeatherBasedSimulationUI.h"

namespace WBSF
{

	// CGraphPropertyDlg dialog

	class CGraphPropertyDlg : public CDialog
	{
		DECLARE_DYNAMIC(CGraphPropertyDlg)

	public:
		CGraphPropertyDlg(CWnd* pParent = NULL);   // standard constructor
		virtual ~CGraphPropertyDlg();

		// Dialog Data
		enum { IDD = IDD_SIM_GRAPH_SERIE };

		CGraphSerie m_graphDefine;

	protected:
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

		DECLARE_MESSAGE_MAP()


		CComboBox m_typeCtrl;
		CMFCColorButton m_colorCtrl;
		CComboBox m_pointSymbolCtrl;
		CCFLEdit m_pointSizeCtrl;
		CComboBox m_lineStyleCtrl;
		CCFLEdit m_lineWithCtrl;
		CButton m_smoothCtrl;

	};

}