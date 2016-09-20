
// BioSIMView.cpp : implementation of the CGradientChartCtrl class
//

#include "stdafx.h"
#include "Basic/Dimension.h"
#include "Basic/WeatherDefine.h"
#include "Basic/Registry.h"

#include "UI/Common/AppOption.h"
#include "UI/Common/SYShowMessage.h"
#include "UI/Common/ChartTRefAxis.h"

#include "UI/ChartsProperties.h"
#include "UI/ChartsProperties.h"
#include "UI/WVariablesEdit.h"


#include "ChartCtrl/ChartPointsExSerie.h"
#include "ChartCtrl/ChartPointsSerie.h"
#include "ChartCtrl/ChartGrid.h"
#include "ChartCtrl/ChartLabel.h"
#include "ChartCtrl/ChartAxisLabel.h"
#include "ChartCtrl/ChartStandardAxis.h"
#include "ChartCtrl/ChartCrossHairCursor.h"
#include "ChartCtrl/ChartDragLineCursor.h"
#include "ChartCtrl/ChartBarSerie.h"
#include "ChartCtrl/ChartCandlestickSerie.h"


#include "MatchStationApp.h"
#include "MatchStationDoc.h"
#include "WeightWnd.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


using namespace std;
using namespace UtilWin;
using namespace WBSF::HOURLY_DATA;
using namespace WBSF;


static const int STD_GAP = 8;
static const int CHART_BASE_ID = 1000;
static const int SPLITTER_BASE_ID = 2000;

static const CGraphSerie DEFAULT_WEIGHT_SERIES =
{ "StationName", 0, 0, 0, CGraphSerie::LEFT_AXIS, false, 2, RGB(200, 200, 200), CGraphSerie::stNone, 6, 6, RGB(000, 000, 255), true, RGB(255, 255, 255), CGraphSerie::lsSolid, 1, RGB(000, 000, 255), false, CGraphSerie::fsNone, CGraphSerie::FILL_BOTTOM, RGB(245, 245, 245) };

static const size_t MAX_WEIGHT_STATION = 10;
static const COLORREF LINE_COLOR[MAX_WEIGHT_STATION] = { RGB(0, 0, 0), RGB(0, 0, 0), RGB(0, 0, 0), RGB(0, 0, 0), RGB(0, 0, 0), RGB(0, 0, 0), RGB(0, 0, 0), RGB(0, 0, 0), RGB(0, 0, 0), RGB(0, 0, 0) };

//*************************************************************************************************************
//CWeightChartCtrl

const TCHAR* CWeightChartCtrl::WINDOW_CLASS_NAME = _T("WeightChartsCtrl");  // Window class name

BEGIN_MESSAGE_MAP(CWeightChartCtrl, CWnd)
	ON_WM_MOUSEACTIVATE()
	ON_WM_MOUSEMOVE()
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_WM_MOUSEWHEEL()
	ON_WM_SIZE()
	ON_WM_SETCURSOR()
	ON_NOTIFY_RANGE(SPN_DELTA, 2000, 2999, OnSplitterDelta)
	ON_MESSAGE(WM_MOUSELEAVE, OnMouseLeave)
END_MESSAGE_MAP()



CWeightChartCtrl::CWeightChartCtrl()
{
	//m_TM = CTM(CTM::HOURLY);
	m_lastPoint = CPoint(-100, -100);
	m_zoom = 1;
	m_lastZoom = 0;
	m_bPeriodEnable = false;

	RegisterWindowClass();

	// Create the scroll helper class and attach to this wnd.
	m_scrollHelper.reset(new CScrollHelper);
	m_scrollHelper->AttachWnd(this);

}



bool CWeightChartCtrl::RegisterWindowClass()
{
	WNDCLASS wndcls;
	HINSTANCE hInst = AfxGetInstanceHandle();

	if (!(::GetClassInfo(hInst, WINDOW_CLASS_NAME, &wndcls)))
	{
		memset(&wndcls, 0, sizeof(WNDCLASS));

		wndcls.hInstance = hInst;
		wndcls.lpfnWndProc = ::DefWindowProc;
		wndcls.hCursor = NULL;
		wndcls.hIcon = 0;
		wndcls.lpszMenuName = NULL;
		wndcls.hbrBackground = (HBRUSH)::GetStockObject(LTGRAY_BRUSH);
		wndcls.style = CS_DBLCLKS;
		wndcls.cbClsExtra = 0;
		wndcls.cbWndExtra = 0;
		wndcls.lpszClassName = WINDOW_CLASS_NAME;

		if (!RegisterClass(&wndcls))
		{
			//  AfxThrowResourceException();
			return false;
		}
	}

	return true;

}

int CWeightChartCtrl::Create(CWnd *pParentWnd, const RECT &rect, UINT nID, DWORD dwStyle)
{

	dwStyle |= WS_CLIPCHILDREN;
	dwStyle |= WS_HSCROLL;
	dwStyle |= WS_VSCROLL;
	int Result = CWnd::CreateEx(0, WINDOW_CLASS_NAME, _T(""), dwStyle, rect, pParentWnd, nID);

	return Result;
}


string CWeightChartCtrl::GetGraphisFilesPath()
{
	CreateMultipleDir(GetUserDataPath() + "WeatherEditor\\");
	return GetUserDataPath() + "WeatherEditor\\WeightGraphics.xml";
}


CGraphVector CWeightChartCtrl::GetChartDefine()const
{
	CGraphVector graphics;

	string filePath = GetGraphisFilesPath();
	zen::LoadXML(filePath, "Graphics", "1", graphics);

	if (graphics.empty())
	{
		
		if (!m_observationStations.empty())
		{
			CGraph graph;
			graph.m_name = GetVariableName(m_variable);
			graph.m_Ytitle1 = GetVariableUnits(m_variable);
			graph.m_bShowLegend = true;

			for (size_t s = 0; s < min(size_t(10), m_observationStations.size()); s++)
			{
				CGraphSerie serie = DEFAULT_WEIGHT_SERIES;
				serie.m_name = m_observationStations[s].m_name;
				serie.m_variable = (int)m_variable;
				serie.m_statistic = (int)s;
				serie.m_lineColor = LINE_COLOR[s];
				graph.m_series.push_back(serie);
			}

			graphics.push_back(graph);
		}
	}


	return graphics;
}

static void GetClientRectSB(CWnd* pWnd, CRect& clientRect)
{
	ASSERT(pWnd != NULL);

	CRect winRect;
	pWnd->GetWindowRect(&winRect);
	pWnd->ScreenToClient(&winRect);

	pWnd->GetClientRect(&clientRect);

	int cxSB = ::GetSystemMetrics(SM_CXVSCROLL);
	int cySB = ::GetSystemMetrics(SM_CYHSCROLL);

	if (winRect.right >= (clientRect.right + cxSB))
		clientRect.right += cxSB;
	if (winRect.bottom >= (clientRect.bottom + cySB))
		clientRect.bottom += cySB;
}

void CWeightChartCtrl::Update()
{

	CGraphVector chartsDefine = GetChartDefine();
	bool bValidPeriod = m_period.Begin() <= m_period.End();

	if (m_index != m_lastIndex ||
		chartsDefine != m_lastChartsDefine ||
		((m_period != m_lastPeriod) && bValidPeriod) ||
		m_zoom != m_lastZoom)
	{
		m_lastIndex = m_index;
		m_lastPeriod = m_period;

		m_charts.clear();
		m_splitters.clear();
		m_scrollHelper->ScrollToOrigin(true, true);

		
		CWaitCursor waitCursor;
		CRegistry registry("WeightCharts");

		//pre-compute total graphics height 
		int totalHeight = 0;
		for (CGraphVector::iterator it1 = chartsDefine.begin(); it1 != chartsDefine.end(); it1++)
			totalHeight += max(50, min(800, registry.GetValue<int>("height" + to_string(it1->m_series.front().m_variable), 150)));

		CRect rect;
		GetClientRectSB(this, rect);
		if (totalHeight > rect.Height())//if they have a scrollbar, remove width of crolbar
			rect.right -= ::GetSystemMetrics(SM_CXVSCROLL);

		CTPeriod entirePeriod;
		
		for (size_t s = 0; s < min(size_t(10), m_observationStations.size()); s++)
				entirePeriod += m_observationStations[s].GetEntireTPeriod();


		CTPeriod period = entirePeriod;
		if (m_bPeriodEnable && m_period.IsInit())
		{
			period = m_period;
			period.Transform(entirePeriod);
		}
		else
		{
			int MAX_DATA_SIZE = registry.GetValue<int>("MaxDataSize", 10000);
			if (period.GetNbRef() > MAX_DATA_SIZE)
				period.End() = period.Begin() + MAX_DATA_SIZE;
		}



		int width = int(rect.Width()*m_zoom);// -10;
		int height = 0;


		int top = 0;
		for (CGraphVector::iterator it1 = chartsDefine.begin(); it1 != chartsDefine.end(); it1++)
		{
			int firstVar = it1->m_series.front().m_variable;
			int height = max(50, min(800, registry.GetValue<int>("height" + to_string(firstVar), 150)));

			CChartCtrlPtr pChart;
			pChart.reset(new CChartCtrl);
			pChart->Create(this, CRect(0, top, width, top + height), CHART_BASE_ID + firstVar, WS_CHILD | WS_VISIBLE);
			top += height;

			pChart->EnableRefresh(false);

			TChartString title = ToUTF16(it1->m_title);
			pChart->GetTitle()->SetVisible(title.empty());
			if (!title.empty())
				pChart->GetTitle()->AddString(title);

			pChart->GetLegend()->SetVisible(it1->m_bShowLegend);
			pChart->GetLegend()->DockLegend(CChartLegend::dsDockBottomLeft);
			pChart->GetLegend()->EnableShadow(false);
			pChart->GetLegend()->EnableBorder(false);
			pChart->GetLegend()->SetTransparent(true);

			pChart->SetPanEnabled(false);
			pChart->SetZoomEnabled(false);
			pChart->SetBackGradient(RGB(250, 250, 250), RGB(200, 200, 200), gtVertical);
			pChart->SetLineInfoEnabled(true);

			//****************
			//X

			CChartTRefAxis* pAxisX = (CChartTRefAxis*)pChart->GetAxis(CChartCtrl::BottomAxis);
			if (pAxisX == NULL)
			{
				pAxisX = new CChartTRefAxis;
				pAxisX->SetMinMax(period.Begin().GetRef(), period.End().GetRef());
				pAxisX->SetReferenceTick(period.Begin());

				pAxisX->SetPanZoomEnabled(false);
				pAxisX->EnableScrollBar(false);
				pAxisX->SetAutoHideScrollBar(false);
				pAxisX->GetGrid()->SetBackColor(gtAlternate2, RGB(235, 235, 255), RGB(245, 245, 255));
				pChart->AttachCustomAxis(pAxisX, CChartCtrl::BottomAxis);
				TChartString lable = ToUTF16(it1->m_Xtitle);
				if (!lable.empty())
					pAxisX->GetLabel()->SetText(lable);
			}

			ENSURE(pAxisX);

			
			CWeightVector weight = m_observationStations.GetWeight((TVarH)m_variable, m_target);

			for (CGraphSerieVector::iterator it2 = it1->m_series.begin(); it2 != it1->m_series.end(); it2++)
			{
				size_t s = it2->m_statistic;

				//****************
				//Y
				CChartCtrl::EAxisPos axis = it2->m_YAxis == 0 ? CChartCtrl::LeftAxis : CChartCtrl::RightAxis;
				CChartAxis* pAxisY = pChart->GetAxis(axis);
				if (pAxisY == NULL)
				{
					pAxisY = pChart->CreateStandardAxis(axis);
					pAxisY->SetAutomatic(true);
					pAxisY->SetPanZoomEnabled(false);
					pAxisY->EnableScrollBar(false);
					pAxisY->SetAutoHideScrollBar(false);
					TChartString lable = ToUTF16((it2->m_YAxis == 0) ? it1->m_Ytitle1 : it1->m_Ytitle2);
					pAxisY->GetLabel()->SetText(lable);

					if (it2->m_YAxis == 1)
						pAxisY->GetGrid()->SetVisible(false);
				}

				ENSURE(pAxisY);

				//****************
				//Series
				CChartXYSerie * pTheSerie = NULL;
				if (it2->m_type == CGraph::XY)
				{
					CChartPointsExSerie* pSerie = new CChartPointsExSerie(pChart.get());
					pChart->AttachCustomSerie(pSerie, false, it2->m_YAxis != 0);

					//general
					TChartString varName = convert(it2->m_name);//convert(GetVariableName(it2->m_variable));
					pSerie->SetName(varName);
					pSerie->EnableShadow(it2->m_bEnableShadow);
					pSerie->SetShadowDepth(it2->m_shadowDepth);
					pSerie->SetShadowColor(it2->m_shadowColor);

					//point
					pSerie->SetPointType((CChartPointsExSerie::PointType)it2->m_symbolType);
					pSerie->SetPointSize(it2->m_symbolWidth, it2->m_symbolHeight);
					pSerie->SetColor(it2->m_symbolColor);
					pSerie->SetFillPoint(it2->m_bSymbolFilled);
					pSerie->SetPointFillColor(it2->m_symbolFillColor);
					//line
					pSerie->SetLineStyle((CChartPointsExSerie::LineType)it2->m_lineStyle);
					pSerie->SetLineWidth(it2->m_lineWidth);
					pSerie->SetLineColor(it2->m_lineColor);
					pSerie->SetLineSmooth(it2->m_bLineSmoothed);
					//surface
					pSerie->SetSurfaceFillStyle((CChartPointsExSerie::FillStyle)it2->m_fillStyle);
					pSerie->SetSurfaceFillColor(it2->m_fillColor);
					pSerie->EnableTooltip(true);

					pTheSerie = pSerie;
				}


				if (it2->m_type == CGraph::XY || it2->m_type == CGraph::HISTOGRAM)
				{
					TVarH var = TVarH(it2->m_variable);

					SChartXYPoint* pPoints = new SChartXYPoint[period.size()];
					bool bIsFirstMissing = false;

					int ii = 0;
					for (CTRef TRef = period.Begin(); TRef <= period.End() /*&& ii < numberPoints*/; TRef++)
					{
						bool bInside = entirePeriod.IsInside(TRef);
						bool bIsInit = bInside && weight[var][s][TRef] > -999;

						double x = bIsInit ? TRef.GetRef() : CHART_MISSING_VALUE;
						double y = bIsInit ? weight[var][s][TRef] : CHART_MISSING_VALUE;
						//it2->m_statistic
						if (bIsInit || bIsFirstMissing)
						{
							pPoints[ii].X = x;
							pPoints[ii].Y = y;
							ii++;

							bIsFirstMissing = bIsInit;
						}
					}

					pTheSerie->SetSeriesOrdering(poNoOrdering);
					pTheSerie->SetPoints(pPoints, (int)ii);

					delete pPoints;
				}
				else
				{

				}
			}

			m_charts.push_back(pChart);


			//add splitter
			CSplitterControlPtr pSplitter(new CSplitterControl);
			pSplitter->Create(WS_CHILD | WS_VISIBLE, CRect(-m_scrollHelper->GetScrollPos().cx, top, -m_scrollHelper->GetScrollPos().cx + width, top + STD_GAP), this, SPLITTER_BASE_ID + firstVar, SPS_HORIZONTAL | SPS_DELTA_NOTIFY | SPS_DOWN_MOVE);
			m_splitters.push_back(pSplitter);
			top += STD_GAP;
		

			ASSERT(m_splitters.size() == m_charts.size());
			for (int i = 0; i < m_splitters.size(); i++)
			{
				m_splitters[i]->RegisterLinkedWindow(SPLS_LINKED_UP, m_charts[i].get());

				for (int j = i + 1; j < m_charts.size(); j++)
				{
					m_splitters[i]->RegisterLinkedWindow(SPLS_LINKED_DOWN, m_charts[j].get());
					m_splitters[i]->RegisterLinkedWindow(SPLS_LINKED_DOWN, m_splitters[j].get());
				}
			}

			UpdateScrollHelper();

			//enable char redraw
			CWaitCursor cursor;
			//#pragma omp parrallel for
			for (CChartCtrlMap::iterator it = m_charts.begin(); it != m_charts.end(); it++)
			{
				(*it)->EnableRefresh(true);
			}
		}
	}
}



void CWeightChartCtrl::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	m_scrollHelper->OnHScroll(nSBCode, nPos, pScrollBar);
}

void CWeightChartCtrl::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	m_scrollHelper->OnVScroll(nSBCode, nPos, pScrollBar);
}

BOOL CWeightChartCtrl::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	BOOL wasScrolled = m_scrollHelper->OnMouseWheel(nFlags, zDelta, pt);
	return wasScrolled;
	//return FALSE;
}


BOOL CWeightChartCtrl::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	(void)::SetCursor(::LoadCursor(NULL, IDC_ARROW));
	return (TRUE);
}

int CWeightChartCtrl::OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message)
{
	int status = CWnd::OnMouseActivate(pDesktopWnd, nHitTest, message);

	// We handle this message so that when user clicks once in the
	// window, it will be given the focus, and this will allow
	// mousewheel messages to be directed to this window.
	SetFocus();

	return status;
}


void CWeightChartCtrl::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	AdjustLayout();

	m_scrollHelper->OnSize(nType, cx, cy);
}

void CWeightChartCtrl::AdjustLayout()
{
	if (GetSafeHwnd() == NULL )
		return;

	// Initialization: Set the display size if needed.

	if (m_scrollHelper->GetDisplaySize() == CSize(0, 0) && !m_charts.empty())
	{
		UpdateScrollHelper();
	}
}

void CWeightChartCtrl::OnSplitterDelta(UINT ID, NMHDR* pNMHDR, LRESULT* pResult)
{
	//  this function just want to show you how to use the delta event
	*pResult = 0;

	SPC_NM_DELTA* pDelta = (SPC_NM_DELTA*)pNMHDR;
	if (NULL == pDelta)
	{
		return;
	}

	int var = ID - SPLITTER_BASE_ID;
	CRect rect;
	GetDlgItem(var + CHART_BASE_ID)->GetClientRect(rect);

	CRegistry registry("Charts");
	registry.SetValue("height" + to_string(var), rect.Height());


	UpdateScrollHelper();
	Invalidate();
}

void CWeightChartCtrl::UpdateScrollHelper()
{
	int width = 0;
	int height = 0;

	for (size_t i = 0; i < m_charts.size(); i++)
	{
		CRect rect;
		m_charts[i]->GetWindowRect(&rect);
		width = max(width, rect.Width());
		height += rect.Height();
		height += STD_GAP;
	}


	if (width > 0 && height > 0)
		m_scrollHelper->SetDisplaySize(width, height);
}

class CTrackMouseEvent : public tagTRACKMOUSEEVENT
{
public:
	CTrackMouseEvent(CWnd* pWnd, DWORD dwFlags = TME_LEAVE, DWORD dwHoverTime = HOVER_DEFAULT)
	{
		ASSERT_VALID(pWnd);
		ASSERT(::IsWindow(pWnd->m_hWnd));
		this->cbSize = sizeof(TRACKMOUSEEVENT);
		this->dwFlags = dwFlags;
		this->hwndTrack = pWnd->m_hWnd;
		this->dwHoverTime = dwHoverTime;
	}

	BOOL Track()
	{
		return _TrackMouseEvent(this);
	}
};

void DrawLine(CDC *cdc, const CRect& rect, const CPoint& point)
{
	int old = cdc->SetROP2(R2_NOT);
	cdc->MoveTo(point.x, rect.top);
	cdc->LineTo(point.x, point.y - 1);
	cdc->MoveTo(point.x, point.y + 1);
	cdc->LineTo(point.x, rect.bottom);
	cdc->SetROP2(old);

}

LRESULT CWeightChartCtrl::DrawDragLine(const CPoint& oldPt, const CPoint& newPt, int w)
{
	// Draw last rect, but no new one (erase old rect)
	CDC *cdc = GetDC();

	static const CRect RECT_NULL = CRect(0, 0, 0, 0);

	CRect rectRange;
	GetClientRect(&rectRange);
	CRect oldRect(oldPt.x - w, rectRange.top, oldPt.x + w, rectRange.bottom);
	CRect newRect(newPt.x - w, rectRange.top, newPt.x + w, rectRange.bottom);
	bool bOldValid = oldPt.x >= 0 && oldPt.y >= 0;
	bool bNewValid = newPt.x >= 0 && newPt.y >= 0;

	cdc->DrawDragRect(bNewValid ? newRect : RECT_NULL, CSize(w, w), bOldValid ? oldRect : RECT_NULL, CSize(w, w));

	ReleaseDC(cdc);

	return TRUE;
}

LRESULT CWeightChartCtrl::OnMouseLeave(WPARAM wParam, LPARAM lParam)
{
	DrawDragLine(m_lastPoint, CPoint(-100, -100), 2);
	return TRUE;
}

void CWeightChartCtrl::OnMouseMove(UINT nFlags, CPoint point)
{
	m_lastPoint = point;
}

ERMsg CWeightChartCtrl::SaveAsImage(const string& strFilename)
{
	ERMsg msg;

	int i = 1;
	for (CChartCtrlMap::iterator it = m_charts.begin(); it != m_charts.end(); it++, i++)
	{
		string filePath(strFilename);
		SetFileTitle(filePath, GetFileTitle(filePath) + to_string(i));
		TChartString strFileName = ToUTF16(filePath);
		(*it)->SaveAsImage(strFileName, CRect(), 32, GUID_NULL);
	}

	return msg;
}



void CWeightChartCtrl::CopyToClipboard()
{
	for (CChartCtrlMap::iterator it = m_charts.begin(); it != m_charts.end(); it++)
	{
		//(*it)->GetOpenClipboardWindow()
	}

}

//**********************************************************************************************************************************


static CMatchStationDoc* GetDocument()
{
	CWinApp* pApp = AfxGetApp();
	if (pApp)
	{
		CFrameWnd * pFrame = (CFrameWnd *)(pApp->m_pMainWnd);
		if (pFrame && pFrame->GetSafeHwnd() != NULL)
			return (CMatchStationDoc*)(pFrame->GetActiveDocument());
	}
	return NULL;

}

static CWeatherDatabasePtr GetDatabasePtr()
{
	CWeatherDatabasePtr pDatabase;
	CMatchStationDoc* pDocument = GetDocument();

	if (pDocument)
		pDatabase = pDocument->GetNormalsDatabase();


	return  pDatabase;
}

IMPLEMENT_SERIAL(CGraphToolBar, CMFCToolBar, 1)
BOOL CGraphToolBar::LoadToolBarEx(UINT uiToolbarResID, CMFCToolBarInfo& params, BOOL bLocked)
{
	if (!CMFCToolBar::LoadToolBarEx(uiToolbarResID, params, bLocked))
		return FALSE;

//*****************************
	CMFCButton;
	CMFCToolBarComboBoxButton zoomButton(ID_GRAPH_ZOOM, 3, WS_TABSTOP | CBS_DROPDOWNLIST, 75);
	for (int i = 0; i < 8; i++)
	{
		CString str;
		str.Format(_T("%dx"), 1 << i);
		zoomButton.AddItem(str);
	}
	
	zoomButton.SelectItem(0, FALSE);
	ReplaceButton(ID_GRAPH_ZOOM, zoomButton);

//*****************************
	CMFCToolBarButton periodEnabled(ID_GRAPH_PERIOD_ENABLED, 4);
	periodEnabled.SetStyle((WS_TABSTOP | TBBS_CHECKBOX) & ~WS_GROUP);
	ReplaceButton(ID_GRAPH_PERIOD_ENABLED, periodEnabled);
//*****************************
	CMFCToolBarDateTimeCtrl periodBegin(ID_GRAPH_PERIOD_BEGIN, 5, WS_TABSTOP | DTS_SHORTDATECENTURYFORMAT);
	ReplaceButton(ID_GRAPH_PERIOD_BEGIN, periodBegin);
//*****************************
	CMFCToolBarDateTimeCtrl periodEnd(ID_GRAPH_PERIOD_END, 6, WS_TABSTOP | DTS_SHORTDATECENTURYFORMAT);
	ReplaceButton(ID_GRAPH_PERIOD_END, periodEnd);
//*****************************
	CMFCToolBarWVariablesButton filterCtrl(ID_GRAPH_FILTER, 7,150);
	ReplaceButton(ID_GRAPH_FILTER, filterCtrl);
//*****************************
	CMFCToolBarComboBoxButton statButton(ID_GRAPH_STAT, 8, WS_TABSTOP | CBS_DROPDOWNLIST);
	for (int i = 0; i < NB_STAT_TYPE; i++)
	{
		statButton.AddItem(CString(CStatistic::GetTitle(i)));
	}

	statButton.SelectItem(0, FALSE);
	ReplaceButton(ID_GRAPH_STAT, statButton);

//*****************************
	CMFCToolBarComboBoxButton TMButton(ID_GRAPH_TM_TYPE, 9, WS_TABSTOP | CBS_DROPDOWNLIST,100);
	for (int i = 0; i < 4; i++)
		TMButton.AddItem(CString(CTM::GetTypeTitle(i)));
	
	TMButton.SelectItem(0, FALSE);
	ReplaceButton(ID_GRAPH_TM_TYPE, TMButton);
	

	UpdateTooltips();
	SetWindowPos(NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE | SWP_NOMOVE);

	return TRUE;
}

void CGraphToolBar::AdjustLocations()
{
	ASSERT_VALID(this);

	if (m_Buttons.IsEmpty() || GetSafeHwnd() == NULL)
	{
		return;
	}

	BOOL bHorz =  GetCurrentAlignment() & CBRS_ORIENT_HORZ ? TRUE : FALSE;

	CRect rectClient;
	GetClientRect(rectClient);

	int xRight = rectClient.right;

	CClientDC dc(this);
	CFont* pOldFont;
	if (bHorz)
	{
		pOldFont = SelectDefaultFont(&dc);
	}
	else
	{
		pOldFont = (CFont*)dc.SelectObject(&(GetGlobalData()->fontVert));
	}

	ENSURE(pOldFont != NULL);

	int iStartOffset;
	if (bHorz)
	{
		iStartOffset = rectClient.left + 1;
	}
	else
	{
		iStartOffset = rectClient.top + 1;
	}

	int iOffset = iStartOffset;
	int y = rectClient.top;

	CSize sizeGrid(GetColumnWidth(), GetRowHeight());

	CSize sizeCustButton(0, 0);

	BOOL bPrevWasSeparator = FALSE;
	int nRowActualWidth = 0;
	for (POSITION pos = m_Buttons.GetHeadPosition(); pos != NULL;)
	{
		POSITION posSave = pos;

		CMFCToolBarButton* pButton = (CMFCToolBarButton*)m_Buttons.GetNext(pos);
		if (pButton == NULL)
		{
			break;
		}

		

		ASSERT_VALID(pButton);

		BOOL bVisible = TRUE;

		CSize sizeButton = pButton->OnCalculateSize(&dc, sizeGrid, bHorz);
		
		if (pButton->m_bTextBelow && bHorz)
		{
			sizeButton.cy = sizeGrid.cy;
		}

		if (pButton->m_nStyle & TBBS_SEPARATOR)
		{
			if (iOffset == iStartOffset || bPrevWasSeparator)
			{
				sizeButton = CSize(0, 0);
				bVisible = FALSE;
			}
			else
			{
				bPrevWasSeparator = TRUE;
			}
		}

		int iOffsetPrev = iOffset;

		CRect rectButton;
		if (bHorz)
		{
			rectButton.left = iOffset;
			rectButton.right = rectButton.left + sizeButton.cx;
			rectButton.top = y;
			rectButton.bottom = rectButton.top + sizeButton.cy;

			iOffset += sizeButton.cx;
			nRowActualWidth += sizeButton.cx;
		}
		else
		{
			rectButton.left = rectClient.left;
			rectButton.right = rectClient.left + sizeButton.cx;
			rectButton.top = iOffset;
			rectButton.bottom = iOffset + sizeButton.cy;

			iOffset += sizeButton.cy;
		}

		pButton->Show(bVisible);
		pButton->SetRect(rectButton);

		if (bVisible)
		{
			bPrevWasSeparator = (pButton->m_nStyle & TBBS_SEPARATOR);
		}

		if (pButton->m_nID == ID_GRAPH_ZOOM)
		{
			int size = 0;
			//compute total length of the right control
			for (POSITION pos2 = pos; pos2 != NULL;)
			{
				CMFCToolBarButton* pButton2 = (CMFCToolBarButton*)m_Buttons.GetNext(pos2);
				if (pButton2 == NULL)
				{
					break;
				}

				CSize sizeButton2 = pButton2->OnCalculateSize(&dc, sizeGrid, bHorz);
				size += bHorz ? sizeButton2.cx : sizeButton2.cy;

			}

			//add delta
			int delta = max(0, (bHorz ? rectClient.Width() : rectClient.Height()) - iOffset - 1 - size);
			iOffset += delta;
		}
		
	}


	dc.SelectObject(pOldFont);
	UpdateTooltips();
	RedrawCustomizeButton();
}

//*************************************************************************************************************
//CWeightChartCtrl
//
//const TCHAR* CWeightChartCtrl::WINDOW_CLASS_NAME = _T("WeatherChartsCtrl");  // Window class name
//
//BEGIN_MESSAGE_MAP(CWeightChartCtrl, CWnd)
//	ON_WM_MOUSEACTIVATE()
//	ON_WM_MOUSEMOVE()
//	ON_WM_HSCROLL()
//	ON_WM_VSCROLL()
//	ON_WM_MOUSEWHEEL()
//	ON_WM_SIZE()
//	ON_WM_SETCURSOR()
//	ON_NOTIFY_RANGE(SPN_DELTA, 2000, 2999, OnSplitterDelta)
//	ON_MESSAGE(WM_MOUSELEAVE, OnMouseLeave)
//END_MESSAGE_MAP()
//
//
//
//CWeightChartCtrl::CWeightChartCtrl()
//{
//	
//
//	m_TM = CTM(CTM::HOURLY);
//	m_stat = MEAN;
//	m_lastPoint = CPoint(-100, -100);
//	m_zoom = 1;
//	m_lastZoom = 0;
//	m_bPeriodEnable = false;
//	
//
//	RegisterWindowClass();
//
//	// Create the scroll helper class and attach to this wnd.
//	m_scrollHelper.reset(new CScrollHelper);
//	m_scrollHelper->AttachWnd(this);
//
//}
//
//
//
//bool CWeightChartCtrl::RegisterWindowClass()
//{
//	WNDCLASS wndcls;
//	HINSTANCE hInst = AfxGetInstanceHandle();
//
//	if (!(::GetClassInfo(hInst, WINDOW_CLASS_NAME, &wndcls)))
//	{
//		memset(&wndcls, 0, sizeof(WNDCLASS));
//		
//		wndcls.hInstance = hInst;
//		wndcls.lpfnWndProc = ::DefWindowProc;
//		wndcls.hCursor = NULL; 
//		wndcls.hIcon = 0;
//		wndcls.lpszMenuName = NULL;
//		wndcls.hbrBackground = (HBRUSH)::GetStockObject(LTGRAY_BRUSH);
//		wndcls.style = CS_DBLCLKS;
//		wndcls.cbClsExtra = 0;
//		wndcls.cbWndExtra = 0;
//		wndcls.lpszClassName = WINDOW_CLASS_NAME;
//
//		if (!RegisterClass(&wndcls))
//		{
//			//  AfxThrowResourceException();
//			return false;
//		}
//	}
//
//	return true;
//
//}
//
//int CWeightChartCtrl::Create(CWnd *pParentWnd, const RECT &rect, UINT nID, DWORD dwStyle)
//{
//
//	dwStyle |= WS_CLIPCHILDREN;
//	dwStyle |= WS_HSCROLL;
//	dwStyle |= WS_VSCROLL;
//	int Result = CWnd::CreateEx(0, WINDOW_CLASS_NAME, _T(""), dwStyle, rect, pParentWnd, nID);
//
//	return Result;
//}
//
//
//
//CGraphVector CWeightChartCtrl::GetCharts(const CWVariablesCounter& variables)
//{
//	CGraphVector graphics;
//
//	string filePath = GetGraphisFilesPath();
//	zen::LoadXML(filePath, "Graphics", "1", graphics);
//
//	if (graphics.empty())
//	{ 
//		graphics = GetDefaultWeatherGraphVector();
//	}
//
//
//	//remove unused variable
//	for (CGraphVector::iterator it1 = graphics.begin(); it1 != graphics.end(); )
//	{
//		for (CGraphSerieVector::iterator it2 = it1->m_series.begin(); it2 != it1->m_series.end(); )
//		{
//			
//			if ( variables[it2->m_variable].first > 0)
//				it2++; 
//			else
//				it2 = it1->m_series.erase(it2);
//		}
//
//		if (it1->m_series.empty())
//			it1 = graphics.erase(it1); 
//		else
//			it1++;
//	}
//
//	return graphics;
//}
//
//static void GetClientRectSB(CWnd* pWnd, CRect& clientRect)
//{
//	ASSERT(pWnd != NULL);
//
//	CRect winRect;
//	pWnd->GetWindowRect(&winRect);
//	pWnd->ScreenToClient(&winRect);
//
//	pWnd->GetClientRect(&clientRect);
//
//	int cxSB = ::GetSystemMetrics(SM_CXVSCROLL);
//	int cySB = ::GetSystemMetrics(SM_CYHSCROLL);
//
//	if (winRect.right >= (clientRect.right + cxSB))
//		clientRect.right += cxSB;
//	if (winRect.bottom >= (clientRect.bottom + cySB))
//		clientRect.bottom += cySB;
//}
//
//void CWeightChartCtrl::UpdateGraphics()
//{
//	string ID;
//	CWVariablesCounter varCounts;
//	//CWVariables variables = m_variables;
//
//	if (m_pStation)
//	{
//		ID = m_pStation->m_ID;
//		varCounts = m_pStation->GetVariablesCount();
//		//variables &= varCounts.GetVariables();
//		for (size_t i = H_FIRST_VAR; i < NB_VAR_H; i++)
//			if (m_variables.any() && !m_variables[i])
//				varCounts[i] = CCountPeriod();
//		
//	}
//		
//	
//
//	CGraphVector chartsDefine = GetCharts(varCounts);
//	bool bValidPeriod = m_period.Begin() <= m_period.End();
//
//	if (ID != m_lastStationID ||
//		m_TM != m_lastTM ||
//		m_stat != m_lastStat ||
//		chartsDefine != m_lastChartsDefine ||
//		((m_period != m_lastPeriod) && bValidPeriod) ||
//		//variables != m_lastVariables ||
//		m_zoom != m_lastZoom)
//	{
//		
//
//		m_lastStationID = ID;
//		m_lastTM = m_TM;
//		m_lastStat = m_stat;
//		m_lastPeriod = m_period;
//		//m_lastVariables = variables;
//
//		m_charts.clear();
//		m_splitters.clear();
//		m_scrollHelper->ScrollToOrigin(true, true); 
//
//		if (m_pStation == NULL || ID.empty() )
//			return;
//
//
//		CWaitCursor waitCursor;
//		CRegistry registry("Charts");
//
//		//pre-compute total graphics height 
//		int totalHeight = 0;
//		for (CGraphVector::iterator it1 = chartsDefine.begin(); it1 != chartsDefine.end(); it1++)
//			totalHeight += max(50, min(800, registry.GetValue<int>("height" + to_string(it1->m_series.front().m_variable), 150)));
//		
//		CRect rect;
//		GetClientRectSB(this, rect);
//		if (totalHeight>rect.Height())//if they have a scrollbar, remove width of crolbar
//			rect.right -= ::GetSystemMetrics(SM_CXVSCROLL);
//		
//
//		
//
//		
//		CTPeriod entirePeriod = m_pStation->GetEntireTPeriod(m_TM);
//		CTPeriod period = entirePeriod;
//		if (m_bPeriodEnable && m_period.IsInit())
//		{
//			period = m_period;
//			period.Transform(m_TM);
//		}
//		else
//		{
//			int MAX_DATA_SIZE = registry.GetValue<int>("MaxDataSize", 10000);
//			if (period.GetNbRef()>MAX_DATA_SIZE)
//				period.End() = period.Begin() + MAX_DATA_SIZE;
//		}
//		
//		
//
//		int width = int(rect.Width()*m_zoom);// -10;
//		int height = 0;
//
//
//		int top = 0;
//		for (CGraphVector::iterator it1 = chartsDefine.begin(); it1 != chartsDefine.end(); it1++)
//		{
//			int firstVar = it1->m_series.front().m_variable;
//			int height = max(50, min(800, registry.GetValue<int>("height" + to_string(firstVar), 150)));
//
//			CChartCtrlPtr pChart;
//			pChart.reset(new CChartCtrl);
//			//pChart->Create(this, CRect(-m_scrollHelper->GetScrollPos().cx, top, -m_scrollHelper->GetScrollPos().cx + width, top + height), CHART_BASE_ID + firstVar, WS_CHILD | WS_VISIBLE);
//			pChart->Create(this, CRect(0, top, width, top + height), CHART_BASE_ID + firstVar, WS_CHILD | WS_VISIBLE);
//			top += height;
//			
//			pChart->EnableRefresh(false);
//			
//			TChartString title = ToUTF16(it1->m_title);
//			pChart->GetTitle()->SetVisible(title.empty());
//			if (!title.empty())
//				pChart->GetTitle()->AddString(title);
//
//			pChart->GetLegend()->SetVisible(it1->m_bShowLegend);
//			pChart->GetLegend()->DockLegend(CChartLegend::dsDockBottomLeft);
//			pChart->GetLegend()->EnableShadow(false);
//			pChart->GetLegend()->EnableBorder(false);
//			pChart->GetLegend()->SetTransparent(true);
//
//			pChart->SetPanEnabled(false);
//			pChart->SetZoomEnabled(false);
//			pChart->SetBackGradient(RGB(250, 250, 250), RGB(200, 200, 200), gtVertical);
//			pChart->SetLineInfoEnabled(true);
//
//			//****************
//			//X
//
//			CChartTRefAxis* pAxisX = (CChartTRefAxis*)pChart->GetAxis(CChartCtrl::BottomAxis);
//			if (pAxisX == NULL)
//			{
//				pAxisX = new CChartTRefAxis;
//				pAxisX->SetMinMax(period.Begin().GetRef(), period.End().GetRef());
//				pAxisX->SetReferenceTick(period.Begin());
//
//				pAxisX->SetPanZoomEnabled(false);
//				pAxisX->EnableScrollBar(false);
//				pAxisX->SetAutoHideScrollBar(false);
//				pAxisX->GetGrid()->SetBackColor(gtAlternate2, RGB(235, 235, 255), RGB(245, 245, 255));
//				pChart->AttachCustomAxis(pAxisX, CChartCtrl::BottomAxis);
//				TChartString lable = ToUTF16(it1->m_Xtitle);
//				if (!lable.empty())
//					pAxisX->GetLabel()->SetText(lable);
//			}
//
//			ENSURE(pAxisX);
//
//			for (CGraphSerieVector::iterator it2 = it1->m_series.begin(); it2 != it1->m_series.end(); it2++)
//			{
//				//****************
//				//Y
//				CChartCtrl::EAxisPos axis = it2->m_YAxis == 0 ? CChartCtrl::LeftAxis : CChartCtrl::RightAxis;
//				CChartAxis* pAxisY = pChart->GetAxis(axis);
//				if (pAxisY == NULL)
//				{
//					pAxisY = pChart->CreateStandardAxis(axis);
//					pAxisY->SetAutomatic(true);
//					pAxisY->SetPanZoomEnabled(false);
//					pAxisY->EnableScrollBar(false);
//					pAxisY->SetAutoHideScrollBar(false);
//					TChartString lable = ToUTF16((it2->m_YAxis == 0) ? it1->m_Ytitle1 : it1->m_Ytitle2);
//					pAxisY->GetLabel()->SetText(lable);
//
//					if (it2->m_YAxis == 1)
//						pAxisY->GetGrid()->SetVisible(false);
//				}
//
//				ENSURE(pAxisY);
//
//				//****************
//				//Series
//				CChartXYSerie * pTheSerie = NULL;
//				if (it2->m_type == CGraph::XY)
//				{
//					CChartPointsExSerie* pSerie = new CChartPointsExSerie(pChart.get()); 
//					pChart->AttachCustomSerie(pSerie, false, it2->m_YAxis != 0);
//
//					//general
//					TChartString varName = convert(GetVariableName(it2->m_variable));
//					pSerie->SetName(varName);
//					pSerie->EnableShadow(it2->m_bEnableShadow);
//					pSerie->SetShadowDepth(it2->m_shadowDepth);
//					pSerie->SetShadowColor(it2->m_shadowColor);
//
//					//point
//					pSerie->SetPointType((CChartPointsExSerie::PointType)it2->m_symbolType);
//					pSerie->SetPointSize(it2->m_symbolWidth, it2->m_symbolHeight);
//					pSerie->SetColor(it2->m_symbolColor);
//					pSerie->SetFillPoint(it2->m_bSymbolFilled);
//					pSerie->SetPointFillColor(it2->m_symbolFillColor);
//					//line
//					pSerie->SetLineStyle((CChartPointsExSerie::LineType)it2->m_lineStyle);
//					pSerie->SetLineWidth(it2->m_lineWidth);
//					pSerie->SetLineColor(it2->m_lineColor);
//					pSerie->SetLineSmooth(it2->m_bLineSmoothed);
//					//surface
//					pSerie->SetSurfaceFillStyle((CChartPointsExSerie::FillStyle)it2->m_fillStyle);
//					pSerie->SetSurfaceFillColor(it2->m_fillColor);
//					pSerie->EnableTooltip(true);
//
//					pTheSerie = pSerie;
//				}
//				else if (it2->m_type == CGraph::HISTOGRAM)
//				{
//					CChartBarSerie* pSerie = new CChartBarSerie(pChart.get()); 
//					pChart->AttachCustomSerie(pSerie, false, it2->m_YAxis != 0);
//
//					//general
//					TChartString varName = convert(GetVariableName(it2->m_variable));
//					pSerie->SetName(varName);
//					
//					//histogram
//					pSerie->SetHorizontal(it2->m_histDirection==CGraphSerie::HIST_HORIZONTAL);
//					pSerie->SetBarWidth(it2->m_histBarWidth);
//					pSerie->SetColor(it2->m_histBarColor);
//					pSerie->SetBorderWidth(it2->m_histBorderWidth);
//					pSerie->SetBorderColor(it2->m_histBorderColor);
//					pSerie->EnableTooltip(true);
//
//					pTheSerie = pSerie;
//				}
//				else if (it2->m_type == CGraph::CANDLE_STICK)
//				{
//					CChartCandlestickSerie* pSeries = new CChartCandlestickSerie(pChart.get());
//				}
//				else if (it2->m_type == CGraph::BOX_PLOT)
//				{
//					assert(false); //todo
//				}
//				
//				
//
//				if (it2->m_type == CGraph::XY || it2->m_type == CGraph::HISTOGRAM)
//				{
//					TVarH var = TVarH(it2->m_variable);
//
//					SChartXYPoint* pPoints = new SChartXYPoint[period.size()];
//					bool bIsFirstMissing = false;
//
//					int ii = 0;
//					for (CTRef TRef = period.Begin(); TRef <= period.End() /*&& ii < numberPoints*/; TRef++)
//					{
//						bool bInside = entirePeriod.IsInside(TRef);
//						bool bIsInit = bInside && (*m_pStation)[TRef][var].IsInit();//don't access data to not create over data
//						
//						double x = bIsInit ? TRef.GetRef() : CHART_MISSING_VALUE;
//						double y = bIsInit ? (*m_pStation)[TRef][var][m_stat] : CHART_MISSING_VALUE;
//						//it2->m_statistic
//						if (bIsInit || bIsFirstMissing)
//						{
//							pPoints[ii].X = x;
//							pPoints[ii].Y = y;
//							ii++;
//
//							bIsFirstMissing = bIsInit;
//						}
//					}
//					
//					pTheSerie->SetSeriesOrdering(poNoOrdering);
//					pTheSerie->SetPoints(pPoints, (int)ii);
//
//					delete pPoints;
//				}
//				else
//				{
//
//				}
//			}
//
//			m_charts.push_back(pChart);
//
//
//			//add splitter
//			CSplitterControlPtr pSplitter(new CSplitterControl);
//			pSplitter->Create(WS_CHILD | WS_VISIBLE, CRect(-m_scrollHelper->GetScrollPos().cx, top, -m_scrollHelper->GetScrollPos().cx + width, top + STD_GAP), this, SPLITTER_BASE_ID + firstVar, SPS_HORIZONTAL | SPS_DELTA_NOTIFY | SPS_DOWN_MOVE);
//			m_splitters.push_back(pSplitter);
//			top += STD_GAP;
//		}
//
//		ASSERT(m_splitters.size() == m_charts.size());
//		for (int i = 0; i < m_splitters.size(); i++)
//		{
//			m_splitters[i]->RegisterLinkedWindow(SPLS_LINKED_UP, m_charts[i].get());
//
//			for (int j = i + 1; j < m_charts.size(); j++)
//			{
//				m_splitters[i]->RegisterLinkedWindow(SPLS_LINKED_DOWN, m_charts[j].get());
//				m_splitters[i]->RegisterLinkedWindow(SPLS_LINKED_DOWN, m_splitters[j].get());
//			}
//		}
//
//		UpdateScrollHelper();
//
//		//enable char redraw
//		CWaitCursor cursor;
//		//#pragma omp parrallel for
//		for (CChartCtrlMap::iterator it = m_charts.begin(); it != m_charts.end(); it++)
//		{
//			(*it)->EnableRefresh(true);
//		}
//	}
//}
//
//
//void CWeightChartCtrl::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
//{
//	m_scrollHelper->OnHScroll(nSBCode, nPos, pScrollBar);
//}
//
//void CWeightChartCtrl::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
//{
//	m_scrollHelper->OnVScroll(nSBCode, nPos, pScrollBar);
//}
//
//BOOL CWeightChartCtrl::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
//{
//	BOOL wasScrolled = m_scrollHelper->OnMouseWheel(nFlags, zDelta, pt);
//	return wasScrolled;
//	//return FALSE;
//}
//
//
//BOOL CWeightChartCtrl::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
//{
//	(void)::SetCursor(::LoadCursor(NULL, IDC_ARROW));
//	return (TRUE);
//}
//
//int CWeightChartCtrl::OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message)
//{
//	int status = CWnd::OnMouseActivate(pDesktopWnd, nHitTest, message);
//
//	// We handle this message so that when user clicks once in the
//	// window, it will be given the focus, and this will allow
//	// mousewheel messages to be directed to this window.
//	SetFocus();
//
//	return status;
//}
//
//
////BOOL CWeightChartCtrl::OnEraseBkgnd(CDC* pDC)
////{
////	// Return TRUE to indicate further erasing is not needed.
////	return TRUE;
////}
//
//void CWeightChartCtrl::OnSize(UINT nType, int cx, int cy)
//{
//	CWnd::OnSize(nType, cx, cy);
//
//	AdjustLayout();
//
//	m_scrollHelper->OnSize(nType, cx, cy);
//}
//
//void CWeightChartCtrl::AdjustLayout()
//{
//	if (GetSafeHwnd() == NULL || !m_pStation)
//		return;
//	
//	// Initialization: Set the display size if needed.
//	
//	if (m_scrollHelper->GetDisplaySize() == CSize(0, 0) && !m_charts.empty())
//	{
//		UpdateScrollHelper();
//	}
//}
//
//void CWeightChartCtrl::OnSplitterDelta(UINT ID, NMHDR* pNMHDR, LRESULT* pResult)
//{
//	//  this function just want to show you how to use the delta event
//	*pResult = 0;
//
//	SPC_NM_DELTA* pDelta = (SPC_NM_DELTA*)pNMHDR;
//	if (NULL == pDelta)
//	{
//		return;
//	}
//
//	int var = ID - SPLITTER_BASE_ID;
//	CRect rect;
//	GetDlgItem(var+CHART_BASE_ID)->GetClientRect(rect);
//
//	CRegistry registry("Charts");
//	registry.SetValue("height" + to_string(var), rect.Height());
//	
//
//	UpdateScrollHelper();
//	Invalidate();
//}
//
//void CWeightChartCtrl::UpdateScrollHelper()
//{
//	int width = 0;
//	int height = 0;
//
//	for (size_t i = 0; i < m_charts.size(); i++)
//	{
//		CRect rect;
//		m_charts[i]->GetWindowRect(&rect);
//		width = max(width, rect.Width());
//		height += rect.Height();
//		height += STD_GAP;
//	}
//
//	//CRect rectAll(0, 0, width, height);
//
//	if (width > 0 && height > 0)
//		m_scrollHelper->SetDisplaySize(width/*+50*/, height);// + 150
//}
//
//class CTrackMouseEvent : public tagTRACKMOUSEEVENT
//{
//public:
//	CTrackMouseEvent(CWnd* pWnd, DWORD dwFlags = TME_LEAVE, DWORD dwHoverTime = HOVER_DEFAULT)
//	{
//		ASSERT_VALID(pWnd);
//		ASSERT(::IsWindow(pWnd->m_hWnd));
//		this->cbSize = sizeof(TRACKMOUSEEVENT);
//		this->dwFlags = dwFlags;
//		this->hwndTrack = pWnd->m_hWnd;
//		this->dwHoverTime = dwHoverTime;
//	}
//
//	BOOL Track()
//	{
//		return _TrackMouseEvent(this);
//	}
//};
//
//void DrawLine(CDC *cdc, const CRect& rect, const CPoint& point)
//{
//	int old = cdc->SetROP2(R2_NOT);
//	cdc->MoveTo(point.x, rect.top);
//	cdc->LineTo(point.x, point.y - 1);
//	cdc->MoveTo(point.x, point.y + 1);
//	cdc->LineTo(point.x, rect.bottom);
//	cdc->SetROP2(old);
//
//}
//
//LRESULT CWeightChartCtrl::DrawDragLine(const CPoint& oldPt, const CPoint& newPt, int w)
//{
//	// Draw last rect, but no new one (erase old rect)
//	CDC *cdc = GetDC();
//
//	static const CRect RECT_NULL = CRect(0, 0, 0, 0);
//
//	CRect rectRange;
//	GetClientRect(&rectRange);
//	CRect oldRect(oldPt.x - w, rectRange.top, oldPt.x + w, rectRange.bottom);
//	CRect newRect(newPt.x - w, rectRange.top, newPt.x + w, rectRange.bottom);
//	bool bOldValid = oldPt.x >= 0 && oldPt.y >= 0;
//	bool bNewValid = newPt.x >= 0 && newPt.y >= 0;
//
//	cdc->DrawDragRect(bNewValid ? newRect : RECT_NULL, CSize(w, w), bOldValid ? oldRect : RECT_NULL, CSize(w, w));
//
//	ReleaseDC(cdc);
//
//	return TRUE;
//}
//
//LRESULT CWeightChartCtrl::OnMouseLeave(WPARAM wParam, LPARAM lParam)
//{
//	DrawDragLine(m_lastPoint, CPoint(-100, -100), 2);
//	return TRUE;
//}
//
//void CWeightChartCtrl::OnMouseMove(UINT nFlags, CPoint point)
//{
//	m_lastPoint = point;
//}
//
//ERMsg CWeightChartCtrl::SaveAsImage(const string& strFilename)
//{
//	ERMsg msg;
//
//	int i = 1;
//	for (CChartCtrlMap::iterator it = m_charts.begin(); it != m_charts.end(); it++, i++)
//	{
//		string filePath(strFilename);
//		SetFileTitle(filePath, GetFileTitle(filePath) + to_wstring(i));
//		TChartString strFileName = ToUTF16(filePath);
//		(*it)->SaveAsImage(strFileName, CRect(), 32, GUID_NULL);
//	}
//
//	return msg;
//}
//
//
//
//void CWeightChartCtrl::CopyToClipboard()
//{
//	for (CChartCtrlMap::iterator it = m_charts.begin(); it != m_charts.end(); it++)
//	{
//		//(*it)->GetOpenClipboardWindow()
//	}
//	
//}
//
//**********************************************************************
// CGradientChartCtrl construction/destruction

//
//static const int ID_CHART = 1004;
//
//BEGIN_MESSAGE_MAP(CGradientChartCtrl, CWeightChartCtrl)
//	ON_WM_CREATE()
//	ON_WM_SIZE()
//	ON_WM_WINDOWPOSCHANGED()
//	ON_WM_DESTROY()
//END_MESSAGE_MAP()
//
//CGradientChartCtrl::CGradientChartCtrl()
//{
//	m_bMustBeUpdated=false;
//	
//	CRegistry registry("Charts");
//	m_zoom = registry.GetValue<int>("Zoom", 1);
//}
//
//CGradientChartCtrl::~CGradientChartCtrl()
//{
//	CRegistry registry("Charts");
//	registry.SetValue("Zoom", m_zoom);
//}
//
//BOOL CGradientChartCtrl::PreCreateWindow(CREATESTRUCT& cs)
//{
//	return CWeightChartCtrl::PreCreateWindow(cs);
//}
//
//// CGradientChartCtrl drawing
//
//int CGradientChartCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct)
//{
//	if (CWeightChartCtrl::OnCreate(lpCreateStruct) == -1)
//		return -1;
//	
//	//if (!Create(this, CRect(0, 0, 0, 0), ID_CHART, WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | WS_VSCROLL))
//		//return FALSE;
//
//	//CreateToolBar();
//	//AdjustLayout();
//
//	return 0;
//}
//
//void CGradientChartCtrl::OnSize(UINT nType, int cx, int cy)
//{
//	CWeightChartCtrl::OnSize(nType, cx, cy);
//	
//	//AdjustLayout();
//}
//
//void CGradientChartCtrl::CopyGraphToClipboard()
//{
//}
//
//ERMsg CGradientChartCtrl::SaveGraph(const CString& filePath)
//{
//	ERMsg msg;
//	return msg;
//}
//
//void CGradientChartCtrl::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
//{
//	//if (lHint == CMatchStationDoc::LANGUAGE_CHANGE)
//	//{
//	//	CreateToolBar();
//	//	AdjustLayout();
//	//}
//
//	CMatchStationDoc* pDoc = (CMatchStationDoc*)GetDocument();
//
//	m_variables = pDoc->GetVariables();
//
//
//	bool bVisible = IsWindowVisible();
//	if (bVisible)
//		UpdateGraphics();
//	else
//		m_bMustBeUpdated = true;
//}

//
//void CGradientChartCtrl::OnWindowPosChanged(WINDOWPOS* lpwndpos)
//{
//	CWeightChartCtrl::OnWindowPosChanged(lpwndpos);
//
//	if( lpwndpos->flags & SWP_SHOWWINDOW )
//	{
//		if( m_bMustBeUpdated )
//		{
//			UpdateGraphics();
//			m_bMustBeUpdated=false;
//		}
//	}
//}
//
//
//void CGradientChartCtrl::OnDestroy()
//{
//	//CAppOption option("Settings");
//	//option.WriteProfileInt("FirstLine", ToInt(m_firstLineCtrl.GetWindowText()));
//	//option.WriteProfileInt("LastLine", ToInt(m_lastLineCtrl.GetWindowText()));
//
//	CWeightChartCtrl::OnDestroy();
//}

//DWORD DateTime_GetSystemtime(	HWND hwndDP,	LPSYSTEMTIME lpSysTime	);




/////////////////////////////////////////////////////////////////////////////
// CGradientChartWnd

CGradientChartWnd::CGradientChartWnd()
{
	m_bMustBeUpdated = false;
}

CGradientChartWnd::~CGradientChartWnd()
{}

BEGIN_MESSAGE_MAP(CGradientChartWnd, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_WINDOWPOSCHANGED()
	ON_UPDATE_COMMAND_UI_RANGE(ID_GRAPH_COPY, ID_GRAPH_ZOOM, OnUpdateToolbar)
	ON_COMMAND_RANGE(ID_GRAPH_COPY, ID_GRAPH_ZOOM, OnToolbarCommand)
	ON_CONTROL_RANGE(LBN_SELCHANGE, ID_GRAPH_COPY, ID_GRAPH_ZOOM, OnToolbarCommand)
	ON_WM_DESTROY()

END_MESSAGE_MAP()

static const UINT CHART_CTRL_ID = 4000;
int CGradientChartWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	CreateToolBar();


	// Créer les volets de sortie :
	const DWORD dwStyle = LBS_NOINTEGRALHEIGHT | WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL;
	if (!m_chartCtrl.Create(this, CRect(0, 0, 0, 0), CHART_CTRL_ID, dwStyle))
	{
		TRACE0("Impossible de créer les fenêtres Sortie\n");
		return -1;      // échec de la création
	}

	UpdateFonts();
	


	return 0;
}

void CGradientChartWnd::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);
	AdjustLayout();
}

//void CGradientChartWnd::AdjustHorzScroll(CListBox& wndListBox)
//{
//	CClientDC dc(this);
//	CFont* pOldFont = dc.SelectObject(&afxGlobalData.fontRegular);
//
//	int cxExtentMax = 0;
//
//	for (int i = 0; i < wndListBox.GetCount(); i++)
//	{
//		CString strItem;
//		wndListBox.GetText(i, strItem);
//
//		cxExtentMax = std::max(cxExtentMax, (int)dc.GetTextExtent(strItem).cx);
//	}
//
//	wndListBox.SetHorizontalExtent(cxExtentMax);
//	dc.SelectObject(pOldFont);
//}

void CGradientChartWnd::UpdateFonts()
{
	m_chartCtrl.SetFont(&afxGlobalData.fontRegular);
}


void CGradientChartWnd::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
	CDockablePane::OnWindowPosChanged(lpwndpos);

	if( lpwndpos->flags & SWP_SHOWWINDOW )
	{
		if( m_bMustBeUpdated )
		{
			m_chartCtrl.Update();
			m_bMustBeUpdated=false;
		}
	}
}

BOOL CGradientChartWnd::PreTranslateMessage(MSG* pMsg)
{
	//GetAsyncKeyState(VK_RETURN)
	//GetKeyState
	//if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)
	//{
	//	int index1 = m_wndToolBar.CommandToIndex(ID_STATION_LIST_YEAR);
	//	CMFCToolBarYearsButton* pCtrl1 = (CMFCToolBarYearsButton*)m_wndToolBar.GetButton(index1); ASSERT(pCtrl1);
	//	if (pMsg->hwnd == pCtrl1->GetEditBox()->GetSafeHwnd())
	//	{
	//		// handle return pressed in edit control
	//		OnToolbarCommand(ID_STATION_LIST_YEAR);
	//		return TRUE; // this doesn't need processing anymore
	//	}

	//	int index2 = m_wndToolBar.CommandToIndex(ID_STATION_LIST_FILTER);
	//	CMFCToolBarWVariablesButton* pCtrl2 = (CMFCToolBarWVariablesButton*)m_wndToolBar.GetButton(index2); ASSERT(pCtrl2);
	//	if (pMsg->hwnd == pCtrl2->GetEditBox()->GetSafeHwnd())
	//	{
	//		// handle return pressed in edit control
	//		OnToolbarCommand(ID_STATION_LIST_FILTER);
	//		return TRUE; // this doesn't need processing anymore
	//	}

	//}

	//if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_TAB)
	//{
	//	int index2 = m_wndToolBar.CommandToIndex(ID_STATION_LIST_FILTER);
	//	CMFCToolBarWVariablesButton* pCtrl2 = (CMFCToolBarWVariablesButton*)m_wndToolBar.GetButton(index2); ASSERT(pCtrl2);
	//	if (pMsg->hwnd == pCtrl2->GetEditBox()->GetSafeHwnd())
	//	{
	//		// handle return pressed in edit control
	//		OnToolbarCommand(ID_STATION_LIST_FILTER);
	//		return TRUE; // this doesn't need processing anymore
	//	}
	//
	//	// handle return pressed in edit control
	//	return TRUE; // this doesn't need processing anymore
	//}

	return CDockablePane::PreTranslateMessage(pMsg); // all other cases still need default processing
}


void CGradientChartWnd::CreateToolBar()
{
	if (m_wndToolBar.GetSafeHwnd())
		m_wndToolBar.DestroyWindow();

	m_wndToolBar.Create(this, AFX_DEFAULT_TOOLBAR_STYLE | CBRS_TOOLTIPS | CBRS_FLYBY, IDR_WEIGHT_TOOLBAR);
	m_wndToolBar.LoadToolBar(IDR_WEIGHT_TOOLBAR, 0, 0, TRUE);
	m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() | CBRS_TOOLTIPS | CBRS_FLYBY);
	m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() & ~(CBRS_GRIPPER | CBRS_SIZE_DYNAMIC | CBRS_BORDER_TOP | CBRS_BORDER_BOTTOM | CBRS_BORDER_LEFT | CBRS_BORDER_RIGHT));
	m_wndToolBar.SetOwner(this);
	m_wndToolBar.SetRouteCommandsViaFrame(FALSE);
}

void CGradientChartWnd::OnDateChange(UINT ID)
{
	//ASSERT(ID == ID_GRAPH_PERIOD_BEGIN || ID == ID_GRAPH_PERIOD_END);

	//CMatchStationDoc* pDoc = (CMatchStationDoc*)GetDocument();

	//COleDateTime oFirstDate;
	//COleDateTime oLastDate;
	//CMFCToolBarDateTimeCtrl* pCtrl1 = CMFCToolBarDateTimeCtrl::GetByCmd(ID_GRAPH_PERIOD_BEGIN); ASSERT(pCtrl1);
	//CMFCToolBarDateTimeCtrl* pCtrl2 = CMFCToolBarDateTimeCtrl::GetByCmd(ID_GRAPH_PERIOD_END); ASSERT(pCtrl2);

	//pCtrl1->GetTime(oFirstDate);
	//pCtrl2->GetTime(oLastDate);

	////	m_firstDay.GetTime(oFirstDate);
	////m_lastDay.GetTime(oLastDate);

	//CTPeriod period;
	//if (oFirstDate.GetStatus() == COleDateTime::valid &&
	//	oLastDate.GetStatus() == COleDateTime::valid)
	//{
	//	CTRef begin(oFirstDate.GetYear(), oFirstDate.GetMonth() - 1, oFirstDate.GetDay() - 1);
	//	CTRef end(oLastDate.GetYear(), oLastDate.GetMonth() - 1, oLastDate.GetDay() - 1);
	//	if (begin > end)
	//	{
	//		if (ID == ID_GRAPH_PERIOD_BEGIN)
	//			end = begin;
	//		else
	//			begin = end;
	//	}
	//	period = CTPeriod(begin, end);
	//}

	//pDoc->SetPeriod(period);

}

void CGradientChartWnd::AdjustLayout()
{
	if (GetSafeHwnd() == NULL)
	{
		return;
	}

	CRect rectClient;
	GetClientRect(rectClient);

	CSize tlb = m_wndToolBar.CalcFixedLayout(FALSE, TRUE);


	m_wndToolBar.SetWindowPos(NULL, rectClient.left, rectClient.top, rectClient.Width(), tlb.cy, SWP_NOACTIVATE | SWP_NOZORDER);
	m_chartCtrl.SetWindowPos(NULL, rectClient.left, rectClient.top + tlb.cy, rectClient.Width(), rectClient.Height() - tlb.cy, SWP_NOACTIVATE | SWP_NOZORDER);
}


void CGradientChartWnd::OnGraphOptions()
{
	CWVariablesCounter variables;
	variables.fill(1);//emulate full variables

	CChartsProperties dlg;
	dlg.m_graphics = m_chartCtrl.GetChartDefine();

	if (dlg.DoModal() == IDOK)
	{
		string filePath = CWeightChartCtrl::GetGraphisFilesPath();
		ERMsg msg = zen::SaveXML(filePath, "Graphics", "1", dlg.m_graphics);

		if (msg)
			m_chartCtrl.Update();
		else
			SYShowMessage(msg, this);
	}

}

void CGradientChartWnd::OnToolbarCommand(UINT ID, NMHDR *pNMHDR, LRESULT *pResult)
{
	OnToolbarCommand(ID);
}


void CGradientChartWnd::OnUpdateToolbar(CCmdUI *pCmdUI)
{
	CMatchStationDoc* pDoc = GetDocument();
	bool bInit = pDoc->GetCurIndex() != UNKNOWN_POS;
	CWVariables variable = pDoc->GetVariable();
	//bool bPeriodEnable = pDoc->GetPeriodEnabled();
	

	switch (pCmdUI->m_nID)
	{
	case ID_GRAPH_SAVE:				pCmdUI->Enable(bInit); break;
	case ID_GRAPH_ZOOM:				pCmdUI->Enable(bInit); break;
	//case ID_GRAPH_FILTER:			pCmdUI->Enable(bInit); break;
	//case ID_GRAPH_PERIOD_ENABLED:	pCmdUI->SetCheck(bPeriodEnable);  pCmdUI->Enable(bInit); break;
	//case ID_GRAPH_PERIOD_BEGIN:		pCmdUI->Enable(bInit&&bPeriodEnable); break;
	//case ID_GRAPH_PERIOD_END:		pCmdUI->Enable(bInit&&bPeriodEnable); break;
	//case ID_GRAPH_STAT:				pCmdUI->Enable(bInit); break;
	//case ID_GRAPH_TM_TYPE:			pCmdUI->Enable(bInit); break;
	}
}


void CGradientChartWnd::OnToolbarCommand(UINT ID)
{
	CMatchStationDoc* pDoc = GetDocument();
	bool bInit = pDoc->GetCurIndex() != UNKNOWN_POS;
	//bool bPeriodEnable = pDoc->GetChartsPeriodEnabled();
	int index = m_wndToolBar.CommandToIndex(ID);
	CMFCToolBarButton* pCtrl = m_wndToolBar.GetButton(index); ASSERT(pCtrl);

	if (bInit)
	{
		switch (ID)
		{
		case ID_GRAPH_SAVE:				OnSaveGraph(); break;
		case ID_GRAPH_ZOOM:				OnUpdate(NULL, NULL, NULL); break;
		}
	}
}


void CGradientChartWnd::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	if (lHint == CMatchStationDoc::LANGUAGE_CHANGE)
	{
		CreateToolBar();
		AdjustLayout();
	}

	CMatchStationDoc* pDoc = (CMatchStationDoc*)GetDocument();
	//CWeatherDatabasePtr pDB = pDoc->GetNormalsDatabase();
	//CTPeriod period = pDoc->GetPeriod();
	//bool bPeriodEnabled = pDoc->GetPeriodEnabled();
	//CWVariables variable = pDoc->GetVariable();


	//if (lHint == CMatchStationDoc::INIT)
	//{
		//CMFCToolBarComboBoxButton* pCtrl = CMFCToolBarComboBoxButton::GetByCmd(ID_GRAPH_ZOOM); ASSERT(pCtrl);
		//pCtrl->SelectItem(int(pDoc->GetChartsZoom()));
	//}

	/*if (lHint == CMatchStationDoc::INIT || lHint == CMatchStationDoc::DATA_PROPERTIES_ENABLE_PERIOD_CHANGE)
	{
		int index = m_wndToolBar.CommandToIndex(ID_GRAPH_PERIOD_ENABLED);
		CMFCToolBarButton* pCtrl = m_wndToolBar.GetButton(index); ASSERT(pCtrl);
		pCtrl->SetImage(bPeriodEnabled ? ID_TABLE_PERIOD_ENABLED - ID_TABLE_MODE_VISUALISATION : ID_TABLE_PERIOD_DESABLED - ID_TABLE_MODE_VISUALISATION);
		pCtrl->SetStyle(bPeriodEnabled ? (pCtrl->m_nStyle | TBBS_CHECKED) : (pCtrl->m_nStyle& ~TBBS_CHECKED));
	}*/

	/*if (lHint == CMatchStationDoc::INIT || lHint == CMatchStationDoc::DATA_PROPERTIES_PERIOD_CHANGE)
	{
		COleDateTime oFirstDate;
		COleDateTime oLastDate;
		if (period.IsInit())
		{
			oFirstDate = COleDateTime(period.Begin().GetYear(), (int)period.Begin().GetMonth() + 1, (int)period.Begin().GetDay() + 1, 0, 0, 0);
			oLastDate = COleDateTime(period.End().GetYear(), (int)period.End().GetMonth() + 1, (int)period.End().GetDay() + 1, 23, 59, 59);
		}
		else
		{
			oFirstDate.SetStatus(COleDateTime::null);
			oLastDate.SetStatus(COleDateTime::null);
		}

		CMFCToolBarDateTimeCtrl* pCtrl1 = CMFCToolBarDateTimeCtrl::GetByCmd(ID_GRAPH_PERIOD_BEGIN); ASSERT(pCtrl1);
		CMFCToolBarDateTimeCtrl* pCtrl2 = CMFCToolBarDateTimeCtrl::GetByCmd(ID_GRAPH_PERIOD_END); ASSERT(pCtrl2);

		pCtrl1->SetTime(oFirstDate);
		pCtrl2->SetTime(oLastDate);
	}*/

	/*if (lHint == CMatchStationDoc::INIT || lHint == CMatchStationDoc::VARIABLES_CHANGE)
	{
		int index = m_wndToolBar.CommandToIndex(ID_GRAPH_FILTER);
		CMFCToolBarWVariablesButton* pCtrl = (CMFCToolBarWVariablesButton*)m_wndToolBar.GetButton(index); ASSERT(pCtrl);
		pCtrl->SetVariables(pDoc->GetVariables());
	}*/

	/*if (lHint == CMatchStationDoc::INIT || lHint == CMatchStationDoc::DATA_PROPERTIES_STAT_CHANGE || lHint == CMatchStationDoc::LANGUAGE_CHANGE)
	{
		int index = m_wndToolBar.CommandToIndex(ID_GRAPH_STAT);
		CMFCToolBarComboBoxButton* pCtrl = (CMFCToolBarComboBoxButton*)m_wndToolBar.GetButton(index); ASSERT(pCtrl);
		pCtrl->SelectItem(int(pDoc->GetStatistic()));
	}*/

	/*if (lHint == CMatchStationDoc::INIT || lHint == CMatchStationDoc::PROPERTIES_TM_CHANGE)
	{
		CMFCToolBarComboBoxButton* pCtrl = CMFCToolBarComboBoxButton::GetByCmd(ID_GRAPH_TM_TYPE); ASSERT(pCtrl);
		pCtrl->SelectItem(int(pDoc->GetTM().Type()));
	}
*/
	CMFCToolBarComboBoxButton* pCtrl = CMFCToolBarComboBoxButton::GetByCmd(ID_GRAPH_ZOOM); ASSERT(pCtrl);
	ENSURE(pCtrl);
	int zoom = pCtrl->GetCurSel();

	m_chartCtrl.m_index = pDoc->GetCurIndex();
	m_chartCtrl.m_variable = pDoc->GetVariable();
	m_chartCtrl.m_observationStations = pDoc->GetObservationStation();
	m_chartCtrl.m_zoom = 1 << zoom;

	if(pDoc->GetCurIndex()!=UNKNOWN_POS)
		m_chartCtrl.m_target = pDoc->GetLocation(pDoc->GetCurIndex());

	bool bVisible = IsWindowVisible();
	if (bVisible)
		m_chartCtrl.Update();
	else
		m_bMustBeUpdated = true;
	
}


void CGradientChartWnd::OnCopyGraph()
{
}

void CGradientChartWnd::OnSaveGraph()
{
	CAppOption option;

	CString filter = GetExportImageFilter();
	option.SetCurrentProfile(_T("LastOpenPath"));
	CString lastDir = option.GetProfileString(_T("ExportImage"));

	CFileDialog openDlg(FALSE, NULL, NULL, OFN_HIDEREADONLY, filter, this);//, sizeof(OPENFILENAME), true);
	openDlg.m_ofn.lpstrInitialDir = lastDir;
	option.SetCurrentProfile(_T("Settings"));
	openDlg.m_ofn.nFilterIndex = option.GetProfileInt(_T("ImageSaveFilterIndex"), 2);


	if (openDlg.DoModal() == IDOK)
	{
		option.SetCurrentProfile(_T("LastOpenPath"));
		option.WriteProfileString(_T("ExportImage"), openDlg.m_ofn.lpstrInitialDir);
		option.SetCurrentProfile(_T("Settings"));
		option.WriteProfileInt(_T("ImageSaveFilterIndex"), openDlg.m_ofn.nFilterIndex);
		TChartString strFilename = openDlg.GetPathName();

		if (UtilWin::GetFileExtension(strFilename.c_str()).IsEmpty())
			strFilename += GetDefaultFilterExt(filter, openDlg.m_ofn.nFilterIndex - 1);
	}

}
