
// BioSIMView.cpp : implementation of the CResultGraphWnd class
//

#include "stdafx.h"

#include "ChartCtrl/ChartLineSerie.h"
#include "ChartCtrl/ChartPointsSerie.h"
#include "ChartCtrl/ChartSurfaceSerie.h"
#include "ChartCtrl/ChartGrid.h"
#include "ChartCtrl/ChartBarSerie.h"
#include "ChartCtrl/ChartCandlestickSerie.h"
#include "ChartCtrl/ChartLabel.h"
#include "ChartCtrl/ChartAxisLabel.h"
#include "ChartCtrl/ChartStandardAxis.h"
#include "ChartCtrl/ChartCrossHairCursor.h"
#include "ChartCtrl/ChartDragLineCursor.h"

#include "UI/GraphDefineDlg.h"
#include "UI/Common/ChartTRefAxis.h"
#include "UI/Common/AppOption.h"
#include "BioSIM.h"
#include "BioSIMDoc.h"
#include "ResultGraphWnd.h"


using namespace std;
using namespace WBSF;
using namespace UtilWin;
using namespace WBSF::DIMENSION;


#ifdef _DEBUG
#define new DEBUG_NEW
#endif



CBioSIMDoc* CResultGraphWnd::GetDocument()
{
	CDocument* pDoc = NULL;
	CWinApp* pApp = AfxGetApp();
	if (pApp)
	{
		POSITION  pos = pApp->GetFirstDocTemplatePosition();
		CDocTemplate* docT = pApp->GetNextDocTemplate(pos);
		if (docT)
		{
			pos = docT->GetFirstDocPosition();
			pDoc = docT->GetNextDoc(pos);
		}
	}

	return static_cast<CBioSIMDoc*>(pDoc);
}


BEGIN_MESSAGE_MAP(CResultChartCtrl, CChartCtrl)
ON_WM_PAINT()
END_MESSAGE_MAP()

CResultChartCtrl::CResultChartCtrl()
{
	m_bShow = false;
}

void CResultChartCtrl::SetGraph(const CGraph& graph)
{
	m_graph = graph;
}

void CResultChartCtrl::SetData(CResultPtr pResult)
{
	m_pResult = pResult;
}

void CResultChartCtrl::UpdateGraph()
{
	RemoveAllSeries();
	
	 

	if( m_graph.m_series.empty() ||
		m_pResult == NULL || 
		!m_pResult->Open() )
	{
		m_bShow = false;
		return;
	}
	
	m_bShow = true;
	
	
	EnableRefresh(false);
	GetTitle()->RemoveAll();
	GetTitle()->AddString( (LPCWSTR)CString(m_graph.m_title.c_str()) );
	
	GetLegend()->SetVisible(m_graph.m_bShowLegend);
	
	CVariableDefine XAxis = m_graph.m_XAxis;
	if( XAxis.m_dimension == TIME_REF )
	{
		CChartTRefAxis* pAxisX = new CChartTRefAxis;
		AttachCustomAxis(pAxisX, BottomAxis);

		
		int firstLine = max(0, min(int(m_pResult->GetNbRows())-1, m_graph.m_firstLine) );
		CTRef TRef(m_pResult->GetDataValue(firstLine, TIME_REF, 0, MEAN));
	
		pAxisX->SetReferenceTick(TRef);
		XAxis.m_field = 1;//Get reference
	}
	else
	{
		CreateStandardAxis(BottomAxis);
	}
	CChartAxis* pAxisX = GetAxis(BottomAxis);
	ENSURE(pAxisX);
	pAxisX->SetAutomatic(true);
	
	CChartAxis* pAxisY = GetAxis(LeftAxis);
	if( pAxisY==NULL)
	{
		pAxisY = CreateStandardAxis(LeftAxis);
		pAxisY->SetAutomatic(true);
	}

	ENSURE(pAxisY);

	//for the moement the graph is recreated each time but optimisation can be done
	for(int i=0; i<m_graph.m_series.size(); i++)
	{
		AddSerie(XAxis, m_graph.m_series[i]);
	}

	EnableRefresh(true);
	RefreshCtrl();
	
	m_pResult->Close();
}

void CResultChartCtrl::AddSerie(const CVariableDefine& XSerie, const CGraphSerie& YSerie)
{

	if (XSerie.m_dimension == -1 || XSerie.m_field == -1)
		return;

	if (XSerie.m_dimension == VARIABLE && XSerie.m_field >= m_pResult->GetNbField(XSerie.m_dimension))
		return;

	if( YSerie.m_dimension==-1 || YSerie.m_variable==-1 )
		return;

	if( YSerie.m_dimension==VARIABLE && YSerie.m_variable>=m_pResult->GetNbField(YSerie.m_dimension) )
		return;

	int Type = YSerie.m_type;
	TChartString YName = CStringW(m_pResult->GetFieldTitle(YSerie.m_dimension, YSerie.m_variable, WBSF::MEAN).c_str());

	TChartString XTitle = CStringW(m_pResult->GetFieldTitle(XSerie.m_dimension, XSerie.m_field, WBSF::MEAN).c_str());
	if( !m_graph.m_Xtitle.empty() )
		XTitle = CStringW(m_graph.m_Xtitle.c_str());

	GetAxis(BottomAxis)->GetLabel()->SetText(XTitle);
	
	TChartString YTitle = CStringW(m_graph.m_Ytitle1.c_str());
	GetAxis(LeftAxis)->GetLabel()->SetText(YTitle);
	
	int firstLine = max(0, min(int(m_pResult->GetNbRows())-1, m_graph.m_firstLine) );
	int lastLine = max(0, min(int(m_pResult->GetNbRows()) - 1, m_graph.m_lastLine));
	int NumberPoints = lastLine-firstLine+1;//Min(m_maxPointPlot, m_pResult->GetNbRows());

	switch (Type)
	{
	// Points series
	case 0:
		{
			CChartPointsSerie* pPointsSeries = CreatePointsSerie();

			pPointsSeries->SetPointType((CChartPointsSerie::PointType) YSerie.m_symbolType);
			pPointsSeries->SetPointSize(YSerie.m_symbolWidth,YSerie.m_symbolHeight);
			pPointsSeries->SetName(YName);
			pPointsSeries->SetColor(YSerie.m_symbolColor);

			SChartXYPoint* pPoints = new SChartXYPoint[NumberPoints];
			int ii=0;
			for (int i=0; i<NumberPoints; i++)
			{
				double x = m_pResult->GetDataValue(firstLine + i, XSerie.m_dimension, XSerie.m_field, MEAN);
				double y = m_pResult->GetDataValue(firstLine + i, YSerie.m_dimension, YSerie.m_variable, MEAN);
				if( x> VMISS && y > VMISS )
				{
					pPoints[ii].X = x;
					pPoints[ii].Y = y;
					ii++;
				}
			}

			pPointsSeries->SetSeriesOrdering(poNoOrdering);
			pPointsSeries->SetPoints(pPoints,ii);
			delete pPoints;


			
		}
		break;

			// Line series
	case 1:
		{
			CChartLineSerie* pLineSeries = CreateLineSerie();
			pLineSeries->SetWidth(YSerie.m_lineWidth);
			pLineSeries->SetPenStyle(YSerie.m_lineStyle);
			pLineSeries->SetSmooth(YSerie.m_bLineSmoothed);

			pLineSeries->SetName(YName);
			pLineSeries->SetColor(YSerie.m_lineColor);

			SChartXYPoint* pPoints = new SChartXYPoint[NumberPoints];
			int ii=0;
			for (int i=0; i<NumberPoints; i++)
			{
				double x = m_pResult->GetDataValue(firstLine+i, XSerie.m_dimension, XSerie.m_field, MEAN);
				double y = m_pResult->GetDataValue(firstLine + i, YSerie.m_dimension, YSerie.m_variable, MEAN);
				if( x> VMISS && y > VMISS )
				{
					pPoints[ii].X = x;
					pPoints[ii].Y = y;
					ii++;
				}
			}

			pLineSeries->SetSeriesOrdering(poNoOrdering);
			pLineSeries->SetPoints(pPoints,ii);
			delete pPoints;
		}
		break;

	// Surface series
	case 2:
		{
			CChartSurfaceSerie* pSurfSeries = CreateSurfaceSerie();
			pSurfSeries->SetFillStyle(CChartSurfaceSerie::fsHatchCross);
			pSurfSeries->SetName(YName);
			pSurfSeries->SetColor(YSerie.m_fillColor);

			//int NumberPoints = Min(m_maxPointPlot, m_pResult->GetNbRows());
			double* XValues = new double[NumberPoints];
			double* YValues = new double[NumberPoints];
			int ii=0;
			for (int i=0; i<NumberPoints; i++)
			{
				double x = m_pResult->GetDataValue(firstLine + i, XSerie.m_dimension, XSerie.m_field, MEAN);
				double y = m_pResult->GetDataValue(firstLine + i, YSerie.m_dimension, YSerie.m_variable, MEAN);
				if( x> VMISS && y > VMISS )
				{
					XValues[ii] = x;
					YValues[ii] = y;
					ii++;
				}
			}

			pSurfSeries->SetPoints(XValues,YValues,ii);
			delete[] XValues;
			delete[] YValues;

		}
		break;
		//candleStick
	case 3:
		{
			CChartCandlestickSerie* pCadleStickSerie = CreateCandlestickSerie();
			pCadleStickSerie->SetWidth(YSerie.m_lineWidth);

			pCadleStickSerie->SetName(YName);
			pCadleStickSerie->SetColor(YSerie.m_symbolColor);
			for (int i=0; i<NumberPoints; i++)
			{
				double X = m_pResult->GetDataValue(firstLine + i, XSerie.m_dimension, XSerie.m_field, MEAN);
				double Y = m_pResult->GetDataValue(firstLine + i, YSerie.m_dimension, YSerie.m_variable, MEAN);
				double low = m_pResult->GetDataValue(firstLine + i, YSerie.m_dimension, YSerie.m_variable, LOWEST);
				double STD = m_pResult->GetDataValue(firstLine + i, YSerie.m_dimension, YSerie.m_variable, STD_DEV);
				double high = m_pResult->GetDataValue(firstLine + i, YSerie.m_dimension, YSerie.m_variable, HIGHEST);

				if( X>VMISS && Y>VMISS)
					pCadleStickSerie->AddPoint(X,low,high,Y-STD,Y+STD);
			}
		}
		break;
		//histopgram
		case 4:
		{
			CChartBarSerie* pBarSerie = CChartCtrl::CreateBarSerie();
			pBarSerie->SetName(YName);
			pBarSerie->SetColor(YSerie.m_symbolColor);

			for (int i=0; i<NumberPoints; i++)
			{
				double X = m_pResult->GetDataValue(firstLine + i, XSerie.m_dimension, XSerie.m_field, MEAN);
				double Y = m_pResult->GetDataValue(firstLine + i, YSerie.m_dimension, YSerie.m_variable, MEAN);
				
				if( X>VMISS && Y>VMISS)
					pBarSerie->AddPoint(X,Y);
			}
		}
		break;
	}

}

void CResultChartCtrl::OnPaint() 
{
	
	if( GetSafeHwnd() && !m_bShow )
	{
		CPaintDC dc(this); // device context for painting

		CRect ClientRect;
		GetClientRect(&ClientRect);
		DrawBackground(&dc, ClientRect);
		//CBrush BrushBack;
		//BrushBack.CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
		//pDC->SetBkColor(GetSysColor(COLOR_BTNFACE));
		//m_chartCtrl.ClientToScreen(&ClientRect);
		//ScreenToClient(ClientRect);
		//pDC->FillRect(ClientRect,&BrushBack);
	}
	else
	{
		CChartCtrl::OnPaint();
	}
	
}
//**********************************************************************
// CResultGraphWnd construction/destruction

//IMPLEMENT_DYNCREATE(CResultGraphWnd, CDockablePane)

const int IDC_GRAPH_LIST_ID = 1001;
const int IDC_FIRST_LINE_ID = 1002;
const int IDC_LAST_LINE_ID = 1003;
const int IDC_CHART_ID = 1004;

BEGIN_MESSAGE_MAP(CResultGraphWnd, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_WINDOWPOSCHANGED()
	ON_CBN_SELCHANGE(IDC_GRAPH_LIST_ID, OnGraphChange )
	ON_EN_CHANGE(IDC_FIRST_LINE_ID, OnGraphChange)
	ON_EN_CHANGE(IDC_LAST_LINE_ID, OnGraphChange)
	ON_COMMAND_EX(ID_ADD_GRAPH, OnEditGraph) 
	ON_COMMAND_EX(ID_EDIT_GRAPH, OnEditGraph) 
	ON_COMMAND(ID_REMOVE_GRAPH, OnRemoveGraph)
	ON_COMMAND(ID_SAVE_GRAPH, OnSaveGraph)
	ON_COMMAND(ID_PRINT_GRAPH, OnPrintGraph)
	ON_COMMAND(IDC_FIRST_LINE_ID, OnPrintGraph)
	ON_UPDATE_COMMAND_UI_RANGE(ID_ADD_GRAPH,ID_PRINT_GRAPH,OnUpdateToolbar)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

CResultGraphWnd::CResultGraphWnd()
{
	m_bMustBeUpdated=false;
	m_baseIndex=0;

	//CAppOption option("Settings");
	
	//m_chartCtrl.SetFirstLine(option.GetProfileInt("FirstLine", 0));
	//m_chartCtrl.SetLastLine(option.GetProfileInt("LastLine", 365));
	
	
}

CResultGraphWnd::~CResultGraphWnd()
{}

//BOOL CResultGraphWnd::PreCreateWindow(CREATESTRUCT& cs)
//{
	//return CDockablePane::PreCreateWindow(cs);
//}

// CResultGraphWnd drawing


//void CResultGraphWnd::OnRButtonUp(UINT nFlags, CPoint point)
//{
//	ClientToScreen(&point);
//	OnContextMenu(this, point);
//}
//
//void CResultGraphWnd::OnContextMenu(CWnd* pWnd, CPoint point)
//{
//	theApp.GetContextMenuManager()->ShowPopupMenu(IDR_POPUP_EDIT, point.x, point.y, this, TRUE);
//}


// CResultGraphWnd diagnostics



// CResultGraphWnd message handlers

//CBCGPGridCtrl* CResultGraphWnd::CreateGrid ()
//CUGCtrl* CResultGraphWnd::CreateGrid ()
//{ 
	//return new CResultCtrl(); 
//}

int CResultGraphWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	m_graphListCtrl.Create(WS_GROUP|WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_BORDER|CBS_DROPDOWNLIST, CRect(0,0,0,0), this, IDC_GRAPH_LIST_ID );
	m_graphListCtrl.SetFont(CFont::FromHandle((HFONT) GetStockObject(DEFAULT_GUI_FONT)));
	if( m_graphListCtrl.GetCount() > 0)
		m_graphListCtrl.SetCurSel(0);

	//m_firstLineCtrl.Create(WS_GROUP|WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_BORDER, CRect(0,0,0,0), this, IDC_FIRST_LINE_ID );
	//m_firstLineCtrl.SetFont(CFont::FromHandle((HFONT) GetStockObject(DEFAULT_GUI_FONT)));
	//m_firstLineCtrl.SetWindowText(ToString(GetChartCtrl()->GetFirstLine()) );
	
	//m_lastLineCtrl.Create(WS_GROUP|WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_BORDER, CRect(0,0,0,0), this, IDC_LAST_LINE_ID );
	//m_lastLineCtrl.SetFont(CFont::FromHandle((HFONT) GetStockObject(DEFAULT_GUI_FONT)));
	//m_lastLineCtrl.SetWindowText(ToString(GetChartCtrl()->GetLastLine()) );

	//m_addCtrl.Create("Add",WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_BORDER, CRect(0,0,0,0), this, IDC_ADD_ID );
	//m_removeCtrl.Create("Remove",WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_BORDER, CRect(0,0,0,0), this, IDC_REMOVE_ID );

	//m_showMapCtrl.Create(GetString(IDS_TO_SHOWMAP),WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_BORDER,CRect(0,0,0,0),this,IDC_SHOWMAP_ID);
	//m_showMapCtrl.SetFont(&m_font);
	
	
	m_chartCtrl.Create(this, CRect(0,0,0,0), IDC_CHART_ID, WS_CHILD|WS_VISIBLE );
	//CChartStandardAxis* pBottomAxis = m_chartCtrl.CreateStandardAxis(CChartCtrl::BottomAxis);
	//pBottomAxis->SetAutomatic(true);
	//CChartStandardAxis* pLeftAxis = m_chartCtrl.CreateStandardAxis(CChartCtrl::LeftAxis);
	//pLeftAxis->SetAutomatic(true);

	
	m_wndToolBar.Create(this, AFX_DEFAULT_TOOLBAR_STYLE|CBRS_SIZE_DYNAMIC, IDR_GRAPH_TOOLBAR);
	m_wndToolBar.LoadToolBar(IDR_GRAPH_TOOLBAR, 0, 0, TRUE);
	m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() | CBRS_TOOLTIPS | CBRS_FLYBY);
	m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() & ~(CBRS_GRIPPER | CBRS_SIZE_DYNAMIC | CBRS_BORDER_TOP | CBRS_BORDER_BOTTOM | CBRS_BORDER_LEFT | CBRS_BORDER_RIGHT));
	m_wndToolBar.SetOwner(this);
	// All commands will be routed via this control , not via the parent frame:
	m_wndToolBar.SetRouteCommandsViaFrame(FALSE);

	AdjustLayout();


	return 0;
}

void CResultGraphWnd::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);
	
	//m_graphListCtrl.MoveWindow( 4, 4, Min(cx, 200), 22 );       // size the combo box
	//m_addCtrl.MoveWindow( Min(cx, 204), 4, 22, 22 );
	//m_removeCtrl.MoveWindow( Min(cx, 232), 4, 22, 22 );
	//m_chartCtrl.MoveWindow( 4, 26, cx-4, cy-30 );       // size the graph control

	AdjustLayout();
}

void CResultGraphWnd::AdjustLayout()
{
	if (GetSafeHwnd() == NULL)
	{
		return;
	}

	CRect rectClient;//,rectCombo, rectMaxLine;
	GetClientRect(rectClient);
	//m_graphListCtrl.GetWindowRect(&rectCombo);
	//m_maxPointPlotCtrl.GetWindowRect(&rectMaxLine);

	//int cyCmb = rectCombo.Size().cy;
	int cxTlb = m_wndToolBar.CalcFixedLayout(FALSE, TRUE).cx+10;
	int cyTlb = m_wndToolBar.CalcFixedLayout(FALSE, TRUE).cy;

	m_wndToolBar.SetWindowPos(NULL, rectClient.left, rectClient.top, rectClient.Width(), cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);
	m_graphListCtrl.SetWindowPos(NULL, rectClient.Width()-200, rectClient.top+2, 200, cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);
	//m_graphListCtrl.SetWindowPos(NULL, rectClient.left+cxTlb, rectClient.top+1, rectClient.Width()-cxTlb-100, cyTlb-2, SWP_NOACTIVATE | SWP_NOZORDER);
	//m_firstLineCtrl.SetWindowPos(NULL, rectClient.left+rectClient.Width()-100+2, rectClient.top+1, 50-2, cyTlb-2, SWP_NOACTIVATE | SWP_NOZORDER);
	//m_lastLineCtrl.SetWindowPos(NULL, rectClient.left+rectClient.Width()-50+2, rectClient.top+1, 50-2, cyTlb-2, SWP_NOACTIVATE | SWP_NOZORDER);
	
	//m_pPropertiesView->SetWindowPos(NULL, rectClient.left, rectClient.top + cyCmb + cyTlb, rectClient.Width(), rectClient.Height() -(cyCmb+cyTlb), SWP_NOACTIVATE | SWP_NOZORDER);
	m_chartCtrl.SetWindowPos(NULL, rectClient.left, rectClient.top + cyTlb, rectClient.Width(), rectClient.Height() -(cyTlb), SWP_NOACTIVATE | SWP_NOZORDER);
}

void CResultGraphWnd::OnGraphChange()
{
	int curSel = m_graphListCtrl.GetCurSel();
	if( curSel>=0 && curSel<m_baseIndex )
	{
		const CGraphVector& graphArray = GetGraphArray();
		ASSERT( (graphArray.size() + m_allOtherGraphArray.size()) == m_graphListCtrl.GetCount() );
		ASSERT( curSel<graphArray.size());
		
		//GetChartCtrl()->SetFirstLine( ToInt(m_firstLineCtrl.GetWindowText()) );
		//GetChartCtrl()->SetLastLine( ToInt(m_lastLineCtrl.GetWindowText()) );
		m_chartCtrl.SetGraph(graphArray[curSel]);
		m_chartCtrl.UpdateGraph();
	}
	else if( curSel >= m_baseIndex )
	{
		OnEditGraph(ID_ADD_GRAPH);
	}
	else
	{
		m_chartCtrl.SetGraph(CGraph());
		m_chartCtrl.UpdateGraph();
	}
}

BOOL CResultGraphWnd::OnEditGraph(UINT ID)
{
	CBioSIMDoc* pDoc = (CBioSIMDoc* )GetDocument();
	if( !pDoc->IsInit() )
		return TRUE;

	CBioSIMProject& project = pDoc->GetProject();
	string iName = pDoc->GetCurSel();
	CExecutablePtr pExec = project.FindItem(iName);
	CGraphVector graphArray = pExec->GetGraph();
	

	CGraphDlg dlg(pExec, this);
	
	int curSel = m_graphListCtrl.GetCurSel();
	if( curSel >= m_baseIndex )
	{
		m_graphListCtrl.SetCurSel(-1);//unselect until user click OK
		dlg.m_graph = m_allOtherGraphArray[curSel-m_baseIndex];
	}
	else if( curSel>=0 )
		dlg.m_graph = graphArray[curSel];

	if(ID == ID_ADD_GRAPH )
	{
		curSel = (int)graphArray.size();
		//if( m_graph.m_name.IsEmpty() )
		dlg.m_graph.m_name = "Graph" + ToString(graphArray.size() + m_allOtherGraphArray.size());
	}
	
	ASSERT( curSel>= 0);
	if( dlg.DoModal() == IDOK)
	{
		ASSERT( curSel<=graphArray.size());
		graphArray.insert(graphArray.begin()+curSel, dlg.m_graph);
		pExec->SetGraph(graphArray);
		FillGraphList();
		m_graphListCtrl.SetCurSel(curSel);
		OnGraphChange();
	}

	return TRUE;
}

void CResultGraphWnd::OnRemoveGraph()
{
	int curSel = m_graphListCtrl.GetCurSel();
	
	if( curSel>=0 )
	{
		ASSERT( curSel>=0 && curSel<m_baseIndex);
		CGraphVector graphArray = GetGraphArray();
		graphArray.erase(graphArray.begin()+curSel);
		SetGraphArray(graphArray);
	

		m_graphListCtrl.DeleteString(curSel);
		m_graphListCtrl.SetCurSel(curSel-1);
		m_baseIndex--;
		OnGraphChange();
	}
}


void CResultGraphWnd::OnSaveGraph()
{
	CAppOption option;

	CString filter = GetExportImageFilter();
	option.SetCurrentProfile(_T("LastOpenPath"));
	CString lastDir = option.GetProfileString(_T("ExportImage" ));

    CFileDialog openDlg(FALSE, NULL, NULL, OFN_HIDEREADONLY, filter, this);//, sizeof(OPENFILENAME), true);
    openDlg.m_ofn.lpstrInitialDir = lastDir;
	option.SetCurrentProfile(_T("Settings"));
    openDlg.m_ofn.nFilterIndex = option.GetProfileInt(_T("ImageSaveFilterIndex"), 2);


    if( openDlg.DoModal() == IDOK)
    {
		option.SetCurrentProfile(_T("LastOpenPath"));
		option.WriteProfileString(_T("ExportImage"), openDlg.m_ofn.lpstrInitialDir);
		option.SetCurrentProfile(_T("Settings"));
		option.WriteProfileInt(_T("ImageSaveFilterIndex"), openDlg.m_ofn.nFilterIndex);
		TChartString strFilename = openDlg.GetPathName();
		if( GetFileExtension(strFilename.c_str()).IsEmpty() )
			strFilename += GetDefaultFilterExt(filter, openDlg.m_ofn.nFilterIndex-1);

		m_chartCtrl.SaveAsImage(strFilename, CRect(), 32, GUID_NULL);
    }
	
}

void CResultGraphWnd::OnPrintGraph()
{
	int curSel = m_graphListCtrl.GetCurSel();
	
	if( curSel>=0 )
	{
		CPrintDialog printDlg(false);
		printDlg.GetDefaults();
		printDlg.m_pd.Flags &= ~PD_RETURNDEFAULT;

		DEVMODE* dm=(DEVMODE*) GlobalLock(printDlg.m_pd.hDevMode);
		dm->dmFields|=DM_ORIENTATION;
		dm->dmOrientation=DMORIENT_LANDSCAPE;
		GlobalUnlock(printDlg.m_pd.hDevMode);

		if( printDlg.DoModal() == IDOK)
		{
			const CGraphVector& graphArray = GetGraphArray();
			TChartString title = CStringW(graphArray[curSel].m_name.c_str());
			m_chartCtrl.Print(title, &printDlg);
		}
	}
}

void CResultGraphWnd::OnUpdateToolbar(CCmdUI *pCmdUI)
{
	CBioSIMDoc* pDoc = (CBioSIMDoc* )GetDocument();
	CBioSIMProject& project = pDoc->GetProject();
	

	string iName = pDoc->GetCurSel();

	int curSel = m_graphListCtrl.GetCurSel();
	ASSERT( curSel>=-1 && curSel<m_graphListCtrl.GetCount());
	switch(pCmdUI->m_nID)
	{
	case ID_ADD_GRAPH: pCmdUI->Enable(pDoc->IsInit() && !iName.empty()); break;
	case ID_EDIT_GRAPH:
	case ID_REMOVE_GRAPH:
	case ID_SAVE_GRAPH:
	case ID_PRINT_GRAPH: pCmdUI->Enable(pDoc->IsInit() && !iName.empty()&&curSel>=0); break;
	default: ASSERT(false);
	}
}


void CResultGraphWnd::GetAllOtherGraphArray(CGraphVector& allOtherGraph, CExecutablePtr pExec, string iName)
{
	if( pExec.get() == NULL)
	{
		CBioSIMDoc* pDoc = (CBioSIMDoc* )GetDocument();
		//CBioSIMProject& project = pDoc->GetProject();
		ASSERT( pDoc );
		iName = pDoc->GetCurSel();
		pExec = pDoc->GetExecutable();
		ASSERT( pExec.get() );

		allOtherGraph.clear();

		CGraph graph;
		graph.m_name = "--------------------------------------------------";
		allOtherGraph.push_back(graph);
	}

	

	for(int i=0; i<pExec->GetNbItem(); i++)
	{
		CExecutablePtr pChild = pExec->GetItemAt(i);
		if( pChild->GetInternalName() != iName)
			allOtherGraph.insert(allOtherGraph.begin(), pChild->GetGraph().begin(), pChild->GetGraph().end());
		
		GetAllOtherGraphArray(allOtherGraph, pChild, iName);
	}
}

const CGraphVector& CResultGraphWnd::GetGraphArray()const
{
	CBioSIMDoc* pDoc = (CBioSIMDoc* )GetDocument();
	CBioSIMProject& project = pDoc->GetProject();
	ASSERT( pDoc );
	string iName = pDoc->GetCurSel();
	CExecutablePtr pExec = project.FindItem(iName);
	ASSERT( pExec.get() );
	return pExec->GetGraph();
}


void CResultGraphWnd::SetGraphArray(const CGraphVector& graphArray)
{
	CBioSIMDoc* pDoc = (CBioSIMDoc* )GetDocument();
	CBioSIMProject& project = pDoc->GetProject();
	ASSERT( pDoc );
	string iName = pDoc->GetCurSel();
	CExecutablePtr pExec = project.FindItem(iName);
	ASSERT( pExec.get() );
	pExec->SetGraph(graphArray);
}

void CResultGraphWnd::FillGraphList()
{
	const CGraphVector& graphArray = GetGraphArray();
	GetAllOtherGraphArray(m_allOtherGraphArray);

	int curSel = m_graphListCtrl.GetCurSel();
	
	m_graphListCtrl.ResetContent();
	
	for(int i=0; i<graphArray.size(); i++)
		m_graphListCtrl.AddString(graphArray[i].m_name);

	m_baseIndex = m_graphListCtrl.GetCount();
	for(int i=0; i<m_allOtherGraphArray.size(); i++)
		m_graphListCtrl.AddString(m_allOtherGraphArray[i].m_name);

	if(curSel==-1 && m_baseIndex>0)
		curSel=0;

	if(curSel>=m_baseIndex)
		curSel=m_baseIndex-1;

	m_graphListCtrl.SetCurSel(curSel);
	
}

void CResultGraphWnd::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	CBioSIMDoc* pDoc = (CBioSIMDoc*)GetDocument();
	CBioSIMProject& project = pDoc->GetProject();
	string iName = pDoc->GetCurSel();
	CExecutablePtr pItem = project.FindItem(iName);

	FillGraphList();
	OnGraphChange();

	CResultPtr result;
	if (pItem && !pDoc->IsExecute())
		result = pItem->GetResult(GetFileManager());

	m_chartCtrl.SetData(result);


	bool bVisible = IsWindowVisible();
	if( bVisible  )
		m_chartCtrl.UpdateGraph();
	else
		m_bMustBeUpdated = true;
		
}

void CResultGraphWnd::UpdateResult()
{
	
}


void CResultGraphWnd::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
	CDockablePane::OnWindowPosChanged(lpwndpos);

	if( lpwndpos->flags & SWP_SHOWWINDOW )
	{
		if( m_bMustBeUpdated )
		{
			m_chartCtrl.UpdateGraph();
			m_bMustBeUpdated=false;
		}
	}
}


void CResultGraphWnd::OnDestroy()
{
	//CAppOption option("Settings");
	//option.WriteProfileInt("FirstLine", ToInt(m_firstLineCtrl.GetWindowText()));
	//option.WriteProfileInt("LastLine", ToInt(m_lastLineCtrl.GetWindowText()));

	CDockablePane::OnDestroy();
}


BOOL CResultGraphWnd::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	//let the view to route command
	if (GetFocus() == &m_chartCtrl && m_chartCtrl.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
		return TRUE;

	return CDockablePane::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}
