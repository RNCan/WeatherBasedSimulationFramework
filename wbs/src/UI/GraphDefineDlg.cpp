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
#include "FileManager/FileManager.h"
#include "UI/Common/SYShowMessage.h"

#include "GraphDefineDlg.h"
#include "GraphPropertyDlg.h"


#include "WeatherBasedSimulationString.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


using namespace WBSF::DIMENSION;


namespace WBSF
{

	//*************************************************************************************************
	BEGIN_MESSAGE_MAP(CGraphSerieSelectionCtrl, CXHtmlTree)
		ON_WM_CREATE()
		ON_NOTIFY_REFLECT(NM_DBLCLK, &OnNMDblclk)
		//	ON_MESSAGE(UWM_XHTMLTREE_CHECKBOX, OnCheckClick)
	END_MESSAGE_MAP()


	//LRESULT CGraphSerieSelectionCtrl::OnCheckClick(WPARAM wp, LPARAM lp)
	//{
	//	BOOL bCheck = GetCheck(hItem);
	//
	//	NMHDR NMHDR;
	//	LRESULT result;
	//	BOOL bRep = CXHtmlTree::OnClick(&NMHDR, &result);
	//
	//	BOOL bCheck = GetCheck(hItem);
	//	return bRep;
	//}

	void CGraphSerieSelectionCtrl::PreSubclassWindow()
	{
		CXHtmlTree::PreSubclassWindow();

		_AFX_THREAD_STATE* pThreadState = AfxGetThreadState();
		if (pThreadState->m_pWndInit == NULL)
		{
			Init();
		}
	}

	int CGraphSerieSelectionCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct)
	{
		if (CXHtmlTree::OnCreate(lpCreateStruct) == -1)
			return -1;

		Init();
		return 0;
	}

	int CGraphSerieSelectionCtrl::GetNbField(int dimension)const
	{
		int nbField = 0;
		switch (dimension)
		{
		case LOCATION:	nbField = CLocation::NB_MEMBER; break;
		case PARAMETER:	nbField = 1; break;
		case REPLICATION:nbField = 1; break;
		case TIME_REF:	nbField = 1; break;
		case VARIABLE:	nbField = (int)m_outputVar.size(); break;
		default: ASSERT(false);
		}

		return nbField;
	}

	CString CGraphSerieSelectionCtrl::GetFieldTitle(int d, int f, bool bTitle)const
	{
		std::string title;
		switch (d)
		{
		case LOCATION:	title = CLocation::GetMemberTitle(f); break;
		case PARAMETER:
		case REPLICATION:
		case TIME_REF:	title = CDimension::GetDimensionTitle(d); break;
		case VARIABLE:	title = "Variable" + ToString(f + 1) + " (" + (bTitle ? m_outputVar[f].m_title : m_outputVar[f].m_name) + ")"; break;
		default: ASSERT(false);
		}

		return CString(title.c_str());
	}

	HTREEITEM CGraphSerieSelectionCtrl::GetItem(int d, int f)const
	{
		HTREEITEM hResultItem = NULL;
		HTREEITEM hItem = GetRootItem();
		if (hItem)
		{
			HTREEITEM hChild = GetChildItem(hItem);
			while (hChild && d > 0)
			{
				d--;
				hChild = GetNextSiblingItem(hChild);
			}

			ASSERT(d == 0);
			HTREEITEM hSubChild = GetChildItem(hChild);
			while (hSubChild && f > 0)
			{
				f--;
				hSubChild = GetNextSiblingItem(hSubChild);
			}

			if (f == 0)
				hResultItem = hSubChild;
		}


		return hResultItem;
	}

	void CGraphSerieSelectionCtrl::Init()
	{
		ASSERT(GetSafeHwnd());

		Initialize(TRUE, TRUE);
		SetSmartCheckBox(TRUE);
		SetSelectFollowsCheck(TRUE);
		SetAutoCheckChildren(TRUE);
		SetHtml(FALSE);
		SetImages(FALSE);

		InitTree();
	}

	void CGraphSerieSelectionCtrl::InitTree()
	{
		if (GetSafeHwnd())
		{
			DeleteAllItems();

			HTREEITEM hItem = InsertItem(UtilWin::GetCString(IDS_STR_SELECT_UNSELECT));
			for (int i = 0; i < NB_DIMENSION; i++)
			{
				HTREEITEM hDimItem = InsertItem(convert(CDimension::GetDimensionTitle(i)).c_str(), hItem);
				for (int j = 0; j < GetNbField(i); j++)
				{
					InsertItem(GetFieldTitle(i, j), hDimItem);
				}
			}

			ExpandAll();
		}
	}

	void CGraphSerieSelectionCtrl::SetData(const CGraphSerieVector& data)
	{
		ASSERT(GetSafeHwnd());
		m_graphDefineMap.RemoveAll();

		for (size_t i = 0; i < data.size(); i++)
		{
			int d = data[i].m_dimension;
			int f = data[i].m_variable;
			HTREEITEM hItem = GetItem(d, f);

			if (hItem)
			{
				SetCheck(hItem, TRUE);
				m_graphDefineMap[hItem] = data[i];
				//if( i == 0)
				EnsureVisible(hItem);
			}
		}
	}

	void CGraphSerieSelectionCtrl::GetData(CGraphSerieVector& data)
	{
		ASSERT(GetSafeHwnd());

		data.clear();

		HTREEITEM hItem = GetRootItem();
		//if(hItem)
		//	hItem = GetChildItem(hItem); 

		//int nbDim = GetChildrenCount(hItem);
		//ASSERT( nbDim == NB_DIMENSION);
		HTREEITEM hDimItem = GetChildItem(hItem);

		for (int d = 0; d < NB_DIMENSION; d++)
		{
			int nbField = GetChildrenCount(hDimItem);
			HTREEITEM hSubItem = GetChildItem(hDimItem);
			for (int f = 0; f < nbField; f++)
			{
				if (GetCheck(hSubItem))
				{
					CGraphSerie graphDefine;// = m_graphDefineMap[hSubItem];
					if (!m_graphDefineMap.Lookup(hSubItem, graphDefine))
					{
						graphDefine.LoadDefault();
					}
					graphDefine.m_dimension = d;
					graphDefine.m_variable = f;
					data.push_back(graphDefine);
				}

				hSubItem = GetNextSiblingItem(hSubItem);
			}

			hDimItem = GetNextSiblingItem(hDimItem);
		}
	}

	int CGraphSerieSelectionCtrl::DrawItemText(CDC *pDC, HTREEITEM hItem, LPCTSTR lpszText, COLORREF crText, COLORREF crTextBackground, COLORREF crBackground, CRect& rect)
	{
		return CXHtmlTree::DrawItemText(pDC, hItem, lpszText, crText, crTextBackground, crBackground, rect);
	}

	void CGraphSerieSelectionCtrl::OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult)
	{
		CPoint point;
		::GetCursorPos(&point);
		ScreenToClient(&point);

		UINT flags = 0;
		HTREEITEM hItem = HitTest(point, &flags);

		if (hItem &&
			GetChildrenCount(hItem) == 0 &&
			//GetCheck(hItem) && 
			(flags & (TVHT_ONITEM /*| TVHT_ONITEMBUTTON | TVHT_ONITEMSTATEICON*/))
			)
		{
			if (!GetCheck(hItem))
				SetCheck(hItem); // if the item isn't selected, we select it...

			//int index = m_index[hItem];
			//ASSERT( index>=0 && index<m_graphDefineArray.GetSize
			CGraphPropertyDlg dlg;
			if (!m_graphDefineMap.Lookup(hItem, dlg.m_graphDefine))
			{
				dlg.m_graphDefine.LoadDefault();
			}
			//dlg.m_graphDefine = m_graphDefineMap[hItem];
			if (dlg.DoModal() == IDOK)
			{
				//save current graph
				//CAppOption option("LastGraph");
				m_graphDefineMap[hItem] = dlg.m_graphDefine;
				dlg.m_graphDefine.SaveDefault();
			}
		}

		*pResult = TRUE;//don't allow defaut processing
	}

	void AFXAPI DDX_Selection(CDataExchange* pDX, int ID, CGraphSerieVector& selection)
	{
		CGraphSerieSelectionCtrl* pCtrl = dynamic_cast<CGraphSerieSelectionCtrl*>(pDX->m_pDlgWnd->GetDlgItem(ID));
		ASSERT(pCtrl);

		if (pDX->m_bSaveAndValidate)
		{
			pCtrl->GetData(selection);
		}
		else
		{
			pCtrl->SetData(selection);
		}
	}

	//**************************************************************************
	//*****************************************************************************************
	// CGraphDlg


	void CGraphDlg::DoDataExchange(CDataExchange* pDX)
	{
		CDialog::DoDataExchange(pDX);

		DDX_Control(pDX, IDC_GRAPH_NAME, m_nameCtrl);
		DDX_Control(pDX, IDC_GRAPH_TITLE, m_titleCtrl);
		DDX_Control(pDX, IDC_GRAPH_X_TITLE, m_titleXCtrl);
		DDX_Control(pDX, IDC_GRAPH_Y_TITLE, m_titleYCtrl);
		DDX_Control(pDX, IDC_GRAPH_XAXIS, m_XAxisCtrl);
		DDX_Control(pDX, IDC_GRAPH_SHOW_LEGEND, m_showLegendCtrl);
		DDX_Control(pDX, IDC_GRAPH_VARIABLE, m_seriesCtrl);
		DDX_Control(pDX, IDC_GRAPH_FIRST_LINE, m_firstLineCtrl);
		DDX_Control(pDX, IDC_GRAPH_LAST_LINE, m_lastLineCtrl);

		if (pDX->m_bSaveAndValidate)
		{
			GetGraphFromInterface(m_graph);
		}
		else
		{
			SetVariableName();
			SetGraphToInterface(m_graph);
		}
		//	AdjustLayout();
	}


	BEGIN_MESSAGE_MAP(CGraphDlg, CDialog)
	END_MESSAGE_MAP()

	//BOOL CGraphDlg::OnInitDialog()
	//{
	//	CDialog::OnInitDialog();
	//
	//	return TRUE;  // return TRUE unless you set the focus to a control
	//}

	// CGraphSerieView construction/destruction

	CGraphDlg::CGraphDlg(CExecutablePtr pExecutable, CWnd* pParentWnd) :
		m_pExecutable(pExecutable),
		CDialog(IDD, pParentWnd)
	{}

	CGraphDlg::~CGraphDlg()
	{}

	void CGraphDlg::SetVariableName()
	{
		if (m_pExecutable)
		{
			//initialize output variable
			//CModelOutputVariableDefVector outputVar;
			CParentInfo info;
			m_pExecutable->GetParentInfo(WBSF::GetFM(), info, VARIABLE);
			m_seriesCtrl.SetOutputDefenition(info.m_variables);

			ASSERT(CLocation::NB_MEMBER == 6);
			//while( m_XAxisCtrl.GetCount() > CLocation::NB_MEMBER+3)???
			//m_XAxisCtrl.DeleteString(CLocation::NB_MEMBER+3);
			for (size_t i = 0; i < info.m_variables.size(); i++)
				m_XAxisCtrl.AddString(CString(_T("Variable")) + UtilWin::ToCString(i + 1) + _T("(") + convert(info.m_variables[i].m_name).c_str() + _T(")"));
		}
	}
	//
	//
	//void CGraphDlg::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
	//{
	//	CFormView::OnUpdate(pSender, lHint, pHint);
	//	CBioSIMDoc* pDoc = (CBioSIMDoc* )GetDocument();
	//	CBioSIMProject& project = pDoc->GetProject();
	//
	//	//save last change
	//	CExecutablePtr pExec = project.FindItem(m_iName);
	//	if( pExec )
	//	{
	//		CGraph oGraph;
	//		GetGraphFromInterface(oGraph);
	//		if( oGraph != m_graph )//if user have made change. Elsewhere we don't change export
	//			pExec->SetGraph(oGraph);
	//	}
	//
	//	m_iName = pDoc->GetCurSel();
	//	pExec = project.FindItem(m_iName);
	//
	//
	//}


	void CGraphDlg::GetGraphFromInterface(CGraph& oGraph)
	{
		//oGraph.m_bAutoExport = m_autoExportCtrl.GetCheck();

		oGraph.m_name = m_nameCtrl.GetString();
		oGraph.m_title = m_titleCtrl.GetString();
		oGraph.m_Xtitle = m_titleXCtrl.GetString();
		oGraph.m_Ytitle1 = m_titleYCtrl.GetString();

		int pos = m_XAxisCtrl.GetCurSel();
		oGraph.m_XAxis.m_dimension = 0;
		for (int d = 0; d < VARIABLE&&pos >= m_seriesCtrl.GetNbField(d); d++)
		{
			oGraph.m_XAxis.m_dimension++;
			pos -= m_seriesCtrl.GetNbField(d);
		}
		ASSERT(pos < m_seriesCtrl.GetNbField(oGraph.m_XAxis.m_dimension));
		oGraph.m_XAxis.m_field = pos;
		oGraph.m_bShowLegend = m_showLegendCtrl.GetCheck();
		m_seriesCtrl.GetData(oGraph.m_series);
		oGraph.m_firstLine = ToInt(m_firstLineCtrl.GetString()) - 1;
		oGraph.m_lastLine = ToInt(m_lastLineCtrl.GetString()) - 1;

	}

	void CGraphDlg::SetGraphToInterface(const CGraph& oGraph)
	{
		//	m_autoExportCtrl.SetCheck(oGraph.m_bAutoExport);
		m_nameCtrl.SetWindowText(oGraph.m_name);
		m_titleCtrl.SetWindowText(oGraph.m_title);
		m_titleXCtrl.SetWindowText(oGraph.m_Xtitle);
		m_titleYCtrl.SetWindowText(oGraph.m_Ytitle1);

		//m_resolutionFactorCtrl.SetWindowText(ToString(oGraph.m_resolutionFactor));
		int pos = oGraph.m_XAxis.m_field;
		for (int d = 0; d < oGraph.m_XAxis.m_dimension; d++)
			pos += m_seriesCtrl.GetNbField(d);

		m_XAxisCtrl.SetCurSel(pos);
		m_showLegendCtrl.SetCheck(oGraph.m_bShowLegend);
		m_seriesCtrl.SetData(oGraph.m_series);
		m_firstLineCtrl.SetWindowText(ToString(oGraph.m_firstLine + 1));
		m_lastLineCtrl.SetWindowText(ToString(oGraph.m_lastLine + 1));
	}

	void CGraphDlg::OnOK()
	{
		/*if( m_graph.m_name.IsEmpty() )
			m_graph.m_name = ;
			*/
		//save current graph to the registry


		CDialog::OnOK();
	}

	//void CGraphSerieView::UpdateTree(CDimension dimension,CGraph& theGraph)
	//{
	//	if( theGraph.m_series.IsEmpty() )
	//	{
	//		for(int d=0; d<NB_DIMENSION; d++)
	//		{
	//			if( dimension[d]>1 || d==VARIABLE)
	//			{
	//				//for(int f=0; f<m_seriesCtrl.GetNbField(d); f++)
	//				//{
	//					//theGraph.m_series.Add( CGraphSerie(d,f) );
	//				//}
	//			}
	//		}
	//	}
	//}
	//


}