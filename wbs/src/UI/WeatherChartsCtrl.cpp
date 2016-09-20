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


#include "UI/Common/ChartTRefAxis.h"
#include "ChartCtrl/ChartAxisLabel.h"
#include "ChartCtrl/ChartPointsExSerie.h"
#include "ChartCtrl/ChartBarSerie.h"
#include "ChartCtrl/ChartCandlestickSerie.h"


#include "Basic/Dimension.h"
#include "Basic/WeatherDefine.h"
#include "Basic/Registry.h"

#include "ChartsProperties.h"
#include "WeatherChartsCtrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


using namespace std;
using namespace UtilWin;
using namespace WBSF::HOURLY_DATA;


namespace WBSF
{

	static const int STD_GAP = 8;
	static const int CHART_BASE_ID = 1000;
	static const int SPLITTER_BASE_ID = 2000;

	//*************************************************************************************************************
	//CWeatherChartsCtrl

	const TCHAR* CWeatherChartsCtrl::WINDOW_CLASS_NAME = _T("WeatherChartsCtrl");  // Window class name

	BEGIN_MESSAGE_MAP(CWeatherChartsCtrl, CWnd)
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



	CWeatherChartsCtrl::CWeatherChartsCtrl()
	{
		m_TM = CTM(CTM::HOURLY);
		m_stat = WBSF::MEAN;
		m_lastPoint = CPoint(-100, -100);
		m_zoom = 1;
		m_lastZoom = 0;
		m_bPeriodEnable = false;


		RegisterWindowClass();

		// Create the scroll helper class and attach to this wnd.
		m_scrollHelper.reset(new CScrollHelper);
		m_scrollHelper->AttachWnd(this);

	}



	bool CWeatherChartsCtrl::RegisterWindowClass()
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

	int CWeatherChartsCtrl::Create(CWnd *pParentWnd, const RECT &rect, UINT nID, DWORD dwStyle)
	{

		dwStyle |= WS_CLIPCHILDREN;
		dwStyle |= WS_HSCROLL;
		dwStyle |= WS_VSCROLL;
		int Result = CWnd::CreateEx(0, WINDOW_CLASS_NAME, _T(""), dwStyle, rect, pParentWnd, nID);

		return Result;
	}


	string CWeatherChartsCtrl::GetGraphisFilesPath(bool bHourly)
	{
		WBSF::CreateMultipleDir(WBSF::GetUserDataPath() + "WeatherEditor\\");
		return WBSF::GetUserDataPath() + "WeatherEditor\\" + (bHourly ?"Hourly":"Daily") +"WeatherGraphics.xml";
	}

	CGraphVector CWeatherChartsCtrl::GetCharts(const CWVariablesCounter& variables, bool bHourly)
	{
		CGraphVector graphics;

		string filePath = GetGraphisFilesPath(bHourly);
		zen::LoadXML(filePath, "Graphics", "1", graphics);

		if (graphics.empty())
		{
			graphics = GetDefaultWeatherGraphVector(bHourly);
		}


		//remove unused variable
		for (CGraphVector::iterator it1 = graphics.begin(); it1 != graphics.end();)
		{
			for (CGraphSerieVector::iterator it2 = it1->m_series.begin(); it2 != it1->m_series.end();)
			{

				if (variables[it2->m_variable].first > 0)
					it2++;
				else
					it2 = it1->m_series.erase(it2);
			}

			if (it1->m_series.empty())
				it1 = graphics.erase(it1);
			else
				it1++;
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

	void CWeatherChartsCtrl::Update()
	{
		string ID;
		CWVariablesCounter varCounts;
		bool bHourly = false;

		if (m_pStation)
		{
			ID = m_pStation->m_ID;
			varCounts = m_pStation->GetVariablesCount();
			//variables &= varCounts.GetVariables();
			for (size_t i = H_FIRST_VAR; i < NB_VAR_H; i++)
				if (m_variables.any() && !m_variables[i])
					varCounts[i] = CCountPeriod();

			bHourly = m_pStation->IsHourly();
		}



		CGraphVector chartsDefine = GetCharts(varCounts, bHourly);
		bool bValidPeriod = m_period.Begin() <= m_period.End();

		if (ID != m_lastStationID ||
			m_TM != m_lastTM ||
			m_stat != m_lastStat ||
			chartsDefine != m_lastChartsDefine ||
			((m_period != m_lastPeriod) && bValidPeriod) ||
			m_zoom != m_lastZoom)
		{


			m_lastStationID = ID;
			m_lastTM = m_TM;
			m_lastStat = m_stat;
			m_lastPeriod = m_period;

			m_charts.clear();
			m_splitters.clear();
			m_scrollHelper->ScrollToOrigin(true, true);

			if (m_pStation != NULL && !ID.empty())
			{


				CWaitCursor waitCursor;
				CRegistry registry("Charts");

				//pre-compute total graphics height 
				int totalHeight = 0;
				for (CGraphVector::iterator it1 = chartsDefine.begin(); it1 != chartsDefine.end(); it1++)
					totalHeight += max(50, min(800, registry.GetValue<int>("height" + to_string(it1->m_series.front().m_variable), 150)));

				CRect rect;
				GetClientRectSB(this, rect);
				if (totalHeight > rect.Height())//if they have a scrollbar, remove width of crolbar
					rect.right -= ::GetSystemMetrics(SM_CXVSCROLL);





				CTPeriod entirePeriod = m_pStation->GetEntireTPeriod(m_TM);
				CTPeriod period = entirePeriod;
				if (m_bPeriodEnable && m_period.IsInit())
				{
					period = m_period;
					period.Transform(m_TM);
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
					//pChart->Create(this, CRect(-m_scrollHelper->GetScrollPos().cx, top, -m_scrollHelper->GetScrollPos().cx + width, top + height), CHART_BASE_ID + firstVar, WS_CHILD | WS_VISIBLE);
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

					for (CGraphSerieVector::iterator it2 = it1->m_series.begin(); it2 != it1->m_series.end(); it2++)
					{
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

							//if (it2->m_YAxis == 1)
								//pAxisY->GetGrid()->SetVisible(false);
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
							TChartString varName = WBSF::convert(GetVariableName(it2->m_variable));
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
						else if (it2->m_type == CGraph::HISTOGRAM)
						{
							CChartBarSerie* pSerie = new CChartBarSerie(pChart.get());
							pChart->AttachCustomSerie(pSerie, false, it2->m_YAxis != 0);

							//general
							TChartString varName = WBSF::convert(GetVariableName(it2->m_variable));
							pSerie->SetName(varName);

							//histogram
							pSerie->SetHorizontal(it2->m_histDirection == CGraphSerie::HIST_HORIZONTAL);
							pSerie->SetBarWidth(it2->m_histBarWidth);
							pSerie->SetColor(it2->m_histBarColor);
							pSerie->SetBorderWidth(it2->m_histBorderWidth);
							pSerie->SetBorderColor(it2->m_histBorderColor);
							pSerie->EnableTooltip(true);

							pTheSerie = pSerie;
						}
						else if (it2->m_type == CGraph::CANDLE_STICK)
						{
							CChartCandlestickSerie* pSeries = new CChartCandlestickSerie(pChart.get());
						}
						else if (it2->m_type == CGraph::BOX_PLOT)
						{
							assert(false); //todo
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
								bool bIsInit = bInside && (*m_pStation)[TRef][var].IsInit();//don't access data to not create over data

								double x = bIsInit ? TRef.GetRef() : CHART_MISSING_VALUE;
								double y = bIsInit ? (*m_pStation)[TRef][var][m_stat] : CHART_MISSING_VALUE;
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
				}

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


	void CWeatherChartsCtrl::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
	{
		m_scrollHelper->OnHScroll(nSBCode, nPos, pScrollBar);
	}

	void CWeatherChartsCtrl::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
	{
		m_scrollHelper->OnVScroll(nSBCode, nPos, pScrollBar);
	}

	BOOL CWeatherChartsCtrl::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
	{
		BOOL wasScrolled = m_scrollHelper->OnMouseWheel(nFlags, zDelta, pt);
		return wasScrolled;
		//return FALSE;
	}


	BOOL CWeatherChartsCtrl::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
	{
		(void)::SetCursor(::LoadCursor(NULL, IDC_ARROW));
		return (TRUE);
	}

	int CWeatherChartsCtrl::OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message)
	{
		int status = CWnd::OnMouseActivate(pDesktopWnd, nHitTest, message);

		// We handle this message so that when user clicks once in the
		// window, it will be given the focus, and this will allow
		// mousewheel messages to be directed to this window.
		SetFocus();

		return status;
	}


	//BOOL CWeatherChartsCtrl::OnEraseBkgnd(CDC* pDC)
	//{
	//	// Return TRUE to indicate further erasing is not needed.
	//	return TRUE;
	//}

	void CWeatherChartsCtrl::OnSize(UINT nType, int cx, int cy)
	{
		CWnd::OnSize(nType, cx, cy);

		AdjustLayout();

		m_scrollHelper->OnSize(nType, cx, cy);
	}

	void CWeatherChartsCtrl::AdjustLayout()
	{
		if (GetSafeHwnd() == NULL || !m_pStation)
			return;

		// Initialization: Set the display size if needed.

		if (m_scrollHelper->GetDisplaySize() == CSize(0, 0) && !m_charts.empty())
		{
			UpdateScrollHelper();
		}
	}

	void CWeatherChartsCtrl::OnSplitterDelta(UINT ID, NMHDR* pNMHDR, LRESULT* pResult)
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

	void CWeatherChartsCtrl::UpdateScrollHelper()
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

	static void DrawLine(CDC *cdc, const CRect& rect, const CPoint& point)
	{
		int old = cdc->SetROP2(R2_NOT);
		cdc->MoveTo(point.x, rect.top);
		cdc->LineTo(point.x, point.y - 1);
		cdc->MoveTo(point.x, point.y + 1);
		cdc->LineTo(point.x, rect.bottom);
		cdc->SetROP2(old);

	}

	LRESULT CWeatherChartsCtrl::DrawDragLine(const CPoint& oldPt, const CPoint& newPt, int w)
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

	LRESULT CWeatherChartsCtrl::OnMouseLeave(WPARAM wParam, LPARAM lParam)
	{
		DrawDragLine(m_lastPoint, CPoint(-100, -100), 2);
		return TRUE;
	}

	void CWeatherChartsCtrl::OnMouseMove(UINT nFlags, CPoint point)
	{
		m_lastPoint = point;
	}

	ERMsg CWeatherChartsCtrl::SaveAsImage(const string& strFilename)
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



	void CWeatherChartsCtrl::CopyToClipboard()
	{
		for (CChartCtrlMap::iterator it = m_charts.begin(); it != m_charts.end(); it++)
		{
			//(*it)->GetOpenClipboardWindow()
		}

	}

}