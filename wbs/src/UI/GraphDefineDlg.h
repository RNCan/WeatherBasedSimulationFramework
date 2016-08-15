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
#include "Simulation/Executable.h"
#include "ModelBase/model.h"
#include "UI/Common/CommonCtrl.h"

#include "WeatherBasedSimulationUI.h"

namespace WBSF
{

	class CGraphSerieSelectionCtrl : public CXHtmlTree
	{
	public:

		//	CVariableSelectionCtrl();

		virtual int			DrawItemText(CDC *pDC, HTREEITEM hItem, LPCTSTR lpszText,
			COLORREF crText, COLORREF crTextBackground,
			COLORREF crBackground, CRect& rect);

		void GetData(CGraphSerieVector& data);
		void SetData(const CGraphSerieVector& data);

		void SetOutputDefenition(const CModelOutputVariableDefVector& outputVar)
		{
			m_outputVar = outputVar;
			InitTree();
		}
		const CModelOutputVariableDefVector& GetOutputDefenition()const{ return m_outputVar; }

		int GetNbField(int dimension)const;
		CString GetFieldName(int d, int f)const{ return GetFieldTitle(d, f, FALSE); }
		CString GetFieldTitle(int d, int f, bool bTitle = true)const;

	protected:

		virtual void Init();
		virtual void PreSubclassWindow();

		void InitTree();

		HTREEITEM GetItem(int d, int f)const;
		CGraphSerie GetItemInfo(HTREEITEM hItem);

		CModelOutputVariableDefVector m_outputVar;

		CMap<HTREEITEM, HTREEITEM, CGraphSerie, CGraphSerie&> m_graphDefineMap;

		afx_msg void OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult);
		afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
		DECLARE_MESSAGE_MAP()

	};

	void AFXAPI DDX_Selection(CDataExchange* pDX, int ID, CVariableDefineVector& selection);


	//**********************************************************************************
	// CGraphDlg dialog


	class CGraphDlg : public CDialog
	{
		// Construction
	public:
		CGraphDlg(CExecutablePtr pExecutable, CWnd* pParentWnd);   // standard constructor
		CGraphDlg::~CGraphDlg();

		CGraph m_graph;
		CExecutablePtr m_pExecutable;

	protected:

		// Dialog Data
		enum { IDD = IDD_SIM_GRAPH };

		DECLARE_MESSAGE_MAP()

		void SetVariableName();

		CCFLEdit m_nameCtrl;
		CCFLEdit m_titleCtrl;
		CCFLEdit m_titleXCtrl;
		CCFLEdit m_titleYCtrl;
		CComboBox	m_XAxisCtrl;
		CButton	m_showLegendCtrl;
		CGraphSerieSelectionCtrl m_seriesCtrl;
		CCFLEdit m_firstLineCtrl;
		CCFLEdit m_lastLineCtrl;

		//	afx_msg void OnExecute();
		//afx_msg void OnUpdateExecute(CCmdUI *pCmdUI);

		virtual void DoDataExchange(CDataExchange* pDX);
		virtual void OnOK();

		//void RepositionChildControl( CWnd *pWnd, const int dx, const int dy, const UINT uAnchor );

		//void UpdateTree(CDimension dimension, CGraph& theGraph);
		void GetGraphFromInterface(CGraph& oGraph);
		void SetGraphToInterface(const CGraph& oGraph);

		//CString m_iName;
		//public:
		//virtual BOOL OnInitDialog();
	};

}