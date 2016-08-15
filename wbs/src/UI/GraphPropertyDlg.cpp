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
#include "GraphPropertyDlg.h"


namespace WBSF
{

	// CGraphPropertyDlg dialog

	IMPLEMENT_DYNAMIC(CGraphPropertyDlg, CDialog)

		CGraphPropertyDlg::CGraphPropertyDlg(CWnd* pParent /*=NULL*/)
		: CDialog(IDD, pParent)
	{
		//  m_pointSymbol = 0;
		//  m_lineStyle = 0;
	}

	CGraphPropertyDlg::~CGraphPropertyDlg()
	{
	}

	void CGraphPropertyDlg::DoDataExchange(CDataExchange* pDX)
	{
		CDialog::DoDataExchange(pDX);
		DDX_Control(pDX, IDC_GRAPH_TYPE, m_typeCtrl);
		DDX_Control(pDX, IDC_GRAPH_COLOR, m_colorCtrl);
		DDX_Control(pDX, IDC_GRAPH_POINT_SYMBOL, m_pointSymbolCtrl);
		DDX_Control(pDX, IDC_GRAPH_POINT_SIZE, m_pointSizeCtrl);
		DDX_Control(pDX, IDC_GRAPH_LINE_STYLE, m_lineStyleCtrl);
		DDX_Control(pDX, IDC_GRAPH_LINE_WIDTH, m_lineWithCtrl);
		DDX_Control(pDX, IDC_GRAPH_SMOOTH, m_smoothCtrl);

		m_colorCtrl.EnableOtherButton(_T("other"));
		if (pDX->m_bSaveAndValidate)
		{
			//m_graphDefine.m_bXAxis = m_XAxisCtrl.GetCheck();

			m_graphDefine.m_type = m_typeCtrl.GetCurSel();
			m_graphDefine.m_symbolColor = m_colorCtrl.GetColor();
			m_graphDefine.m_symbolType = m_pointSymbolCtrl.GetCurSel();
			m_graphDefine.m_symbolHeight = m_graphDefine.m_lineWidth = ToInt(m_pointSizeCtrl.GetString());
			m_graphDefine.m_lineWidth = ToInt(m_lineWithCtrl.GetString());
			m_graphDefine.m_lineColor = m_colorCtrl.GetColor();
			m_graphDefine.m_lineStyle = m_lineStyleCtrl.GetCurSel();
			m_graphDefine.m_bLineSmoothed = m_smoothCtrl.GetCheck();
		}
		else
		{
			//m_XAxisCtrl.SetCheck(m_graphDefine.m_bXAxis);
			m_typeCtrl.SetCurSel(m_graphDefine.m_type);
			m_colorCtrl.SetColumnsNumber(10);
			m_colorCtrl.SetColor(m_graphDefine.m_symbolColor);
			m_pointSymbolCtrl.SetCurSel(m_graphDefine.m_symbolType);
			m_pointSizeCtrl.SetWindowText(ToString(m_graphDefine.m_symbolHeight));
			m_lineStyleCtrl.SetCurSel(m_graphDefine.m_lineStyle);
			m_lineWithCtrl.SetWindowText(ToString(m_graphDefine.m_lineWidth));
			m_smoothCtrl.SetCheck(m_graphDefine.m_bLineSmoothed);
		}
	}


	BEGIN_MESSAGE_MAP(CGraphPropertyDlg, CDialog)
	END_MESSAGE_MAP()



}