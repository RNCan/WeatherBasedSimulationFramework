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

#include "Basic/Statistic.h"
#include "Basic/WeatherDefine.h"
#include "UI/Common/UtilWin.h"
#include "UI/Common/CommonCtrl.h"
#include "UI/Common/AppOption.h"
#include "ChartsProperties.h"
#include "UI/Common/FileNameProperty.h"


#include "WeatherBasedSimulationString.h"

//using namespace UtilWin;
using namespace WBSF::HOURLY_DATA;

namespace WBSF
{

#define AFX_PROP_HAS_LIST 0x0001

	//*****************************************************************************************************
	void CChartsListCtrl::FillCharts()
	{
		m_pWndList->DeleteAllItems();
		for (size_t i = 0; i < m_graphics.size(); i++)
		{
			AddItem(CString(m_graphics[i].m_name.c_str()));
		}
	}


	void CChartsListCtrl::OnSelectionChanged()
	{
		if (!m_bInProcess)
			m_seriesListCtrl.SetChartIndex(GetSelItem());
	}


	void CChartsListCtrl::OnAfterAddItem(int iItem)
	{
		CVSListBox::OnAfterAddItem(iItem);
		m_graphics.push_back(CGraph());

		m_seriesListCtrl.SetChartIndex(-1);
		m_seriesListCtrl.SetChartIndex(iItem);

	}

	BOOL CChartsListCtrl::OnBeforeRemoveItem(int iItem)
	{
		ASSERT(iItem >= 0 && iItem < m_graphics.size());
		m_graphics.erase(m_graphics.begin() + iItem);
		return TRUE;
	}

	void CChartsListCtrl::OnAfterRenameItem(int iItem)
	{
		ASSERT(iItem >= 0 && iItem < m_graphics.size());
		m_graphics[iItem].m_name = CStringA(GetItemText(iItem));
	}

	void CChartsListCtrl::OnAfterMoveItemUp(int iItem)
	{
		ASSERT(iItem >= 0 && iItem < m_graphics.size() - 1);

		CGraph graph = m_graphics[iItem + 1];
		m_graphics[iItem + 1] = m_graphics[iItem];
		m_graphics[iItem] = graph;
	}


	void CChartsListCtrl::OnAfterMoveItemDown(int iItem)
	{
		ASSERT(iItem > 0 && iItem < m_graphics.size());
		CGraph graph = m_graphics[iItem - 1];
		m_graphics[iItem - 1] = m_graphics[iItem];
		m_graphics[iItem] = graph;
	}


	void CChartsListCtrl::OnClickButton(int iButton)
	{
		m_bInProcess = true;
		CVSListBox::OnClickButton(iButton);
		m_bInProcess = false;

		//m_seriesListCtrl.SetChartIndex(-1);
		//m_seriesListCtrl.SetChartIndex(GetSelItem());
	}

	//*****************************************************************************************************
	void CSeriesListCtrl::SetChartIndex(int index)
	{
		if (index != m_chartIndex)
		{
			m_pWndList->DeleteAllItems();
			m_chartIndex = index;

			if (index >= 0)
			{
				const CGraphSerieVector& series = m_graphics[index].m_series;

				for (size_t i = 0; i < series.size(); i++)
				{
					AddItem(CString(series[i].m_name.c_str()));
				}
			}
		}


	}

	void CSeriesListCtrl::OnSelectionChanged()
	{
		if (!m_bInProcess)
			m_seriesPropertyGridCtrl.Set(m_chartIndex, GetSelItem());
	}

	void CSeriesListCtrl::OnAfterAddItem(int iItem)
	{
		ASSERT(m_chartIndex >= 0 && m_chartIndex < m_graphics.size());

		CGraphSerieVector& series = m_graphics[m_chartIndex].m_series;
		series.push_back(CGraphSerie());
		m_seriesPropertyGridCtrl.Set(m_chartIndex, GetSelItem(), true);

		ASSERT(iItem >= 0 && iItem < series.size());
	}

	BOOL CSeriesListCtrl::OnBeforeRemoveItem(int iItem)
	{
		ASSERT(m_chartIndex >= 0 && m_chartIndex < m_graphics.size());
		CGraphSerieVector& series = m_graphics[m_chartIndex].m_series;

		ASSERT(iItem >= 0 && iItem < series.size());
		series.erase(series.begin() + iItem);

		return CVSListBox::OnBeforeRemoveItem(iItem);
	}

	void CSeriesListCtrl::OnAfterRenameItem(int iItem)
	{
		CGraphSerieVector& series = m_graphics[m_chartIndex].m_series;
		ASSERT(iItem >= 0 && iItem < series.size());
		ASSERT(m_chartIndex >= 0 && m_chartIndex < m_graphics.size());

		CVSListBox::OnAfterRenameItem(iItem);
	}

	void CSeriesListCtrl::OnAfterMoveItemUp(int iItem)
	{
		CGraphSerieVector& series = m_graphics[m_chartIndex].m_series;
		ASSERT(iItem >= 0 && iItem < series.size() - 1);
		ASSERT(m_chartIndex >= 0 && m_chartIndex < m_graphics.size());


		CGraphSerie serie = series[iItem + 1];
		series[iItem + 1] = series[iItem];
		series[iItem] = serie;

		CVSListBox::OnAfterMoveItemUp(iItem);

	}

	void CSeriesListCtrl::OnAfterMoveItemDown(int iItem)
	{
		CGraphSerieVector& series = m_graphics[m_chartIndex].m_series;
		ASSERT(iItem > 0 && iItem < series.size());
		ASSERT(m_chartIndex >= 0 && m_chartIndex < m_graphics.size());

		CGraphSerie serie = series[iItem - 1];
		series[iItem - 1] = series[iItem];
		series[iItem] = serie;

		CVSListBox::OnAfterMoveItemDown(iItem);
	}

	void CSeriesListCtrl::OnClickButton(int iButton)
	{
		m_bInProcess = true;
		CVSListBox::OnClickButton(iButton);
		m_bInProcess = false;

		SelectItem(GetSelItem());
	}

	//*****************************************************************************************************


	//class CSymbolProperty : public CStdComboPosProperty
	//{
	//public:

	//	//CString GetOptionText(int i){ return CString(CGraphSerie::GetSyboleTypeTitle(i).c_str()); }
	//	CSymbolProperty(const std::string& strName, size_t index = -1, const std::string& lpszDescr = "", size_t dwData = 0) : CStdComboPosProperty(strName, index, lpszDescr, GetString(IDS_WG_GRAPH_SYMBOL_TYPE), false, dwData)
	//	{
	//		m_bAllowEdit = false;

	//		for (int i = -1; i < CGraphSerie::NB_SYMBOL_TYPE; i++)
	//			AddOption(GetOptionText(i));

	//		AllowEdit(FALSE);

	//		SetOriginalValue(GetOptionText(index));
	//	}


	//	virtual CComboBox* CreateCombo(CWnd* pWndParent, CRect rect)
	//	{
	//		//new CLineStyleCB(;
	//		return CMFCPropertyGridProperty::CreateCombo(pWndParent, rect);
	//	}

	//	//int GetIndex()const	{ return m_pWndCombo->GetCurSel() - 1; }
	//	//void SetIndex(int index){ CMFCPropertyGridProperty::SetValue(GetOptionText(index)); }
	//};

	//class CLineStyleProperty : public CMFCPropertyGridProperty
	//{
	//public:

		//CString GetOptionText(int i){ return CString(CGraphSerie::GetLineStyleTitle(i).c_str()); }
	//	CLineStyleProperty(const CString& strName, int index = -1, LPCTSTR lpszDescr = NULL, DWORD_PTR dwData = 0) : CMFCPropertyGridProperty(strName, _T(""), lpszDescr, dwData)
	//	{
	//		m_bAllowEdit = false;

	//		for (int i = -1; i < CGraphSerie::NB_LINE_STYLE; i++)
	//			AddOption(GetOptionText(i));

	//		AllowEdit(FALSE);

	//		SetOriginalValue(GetOptionText(index));
	//	}


	//	virtual CComboBox* CreateCombo(CWnd* pWndParent, CRect rect)
	//	{
	//		//new CLineStyleCB(;
	//		return CMFCPropertyGridProperty::CreateCombo(pWndParent, rect);
	//	}

	//	int GetIndex()const	{ return m_pWndCombo->GetCurSel() - 1; }
	//	void SetIndex(int index){ CMFCPropertyGridProperty::SetValue(GetOptionText(index)); }
	//};


	//class CFillStyleProperty : public CMFCPropertyGridProperty
	//{
	//public:

		//CString GetOptionText(int i){ return CString(CGraphSerie::GetFillStyleTitle(i).c_str()); }
	//	CFillStyleProperty(const CString& strName, int index = -1, LPCTSTR lpszDescr = NULL, DWORD_PTR dwData = 0) : CMFCPropertyGridProperty(strName, _T(""), lpszDescr, dwData)
	//	{
	//		m_bAllowEdit = false;

	//		for (int i = -1; i < CGraphSerie::NB_FILL_STYLE; i++)
	//			AddOption(GetOptionText(i));

	//		AllowEdit(FALSE);

	//		SetOriginalValue(GetOptionText(index));
	//	}


	//	virtual CComboBox* CreateCombo(CWnd* pWndParent, CRect rect)
	//	{
	//		//new CLineStyleCB(;
	//		return CMFCPropertyGridProperty::CreateCombo(pWndParent, rect);
	//	}

	//	int GetIndex()const	{ return m_pWndCombo->GetCurSel() - 1; }
	//	void SetIndex(int index){ CMFCPropertyGridProperty::SetValue(GetOptionText(index)); }
	//};

	typedef CStdIndexProperty < IDS_WG_GRAPH_SYMBOL_TYPE, true > CSymbolProperty;
	typedef CStdIndexProperty < IDS_WG_GRAPH_LINE_STYLE, true > CLineStyleProperty;
	typedef CStdIndexProperty < IDS_WG_GRAPH_FILL_STYLE, true > CFillStyleProperty;

	//*****************************************************************************************************


	BEGIN_MESSAGE_MAP(CSeriesPropertyGridCtrl, CMFCPropertyGridCtrl)
		//ON_WM_CREATE()
	END_MESSAGE_MAP()

	CSeriesPropertyGridCtrl::CSeriesPropertyGridCtrl(CGraphVector& graphics) :
		m_graphics(graphics)
	{
		m_chartIndex = -1;
		m_serieIndex = -1;
	}


	enum TGraph
	{
		TITLE, X_AXIS_TITLE, Y_LEFT_AXIS_TITLE, Y_RIGHT_AXIS_TITLE, SHOW_LEGEND,
		VARIABLE, STATISTIC, SERIE_TYPE, Y_AXIS, ENABLE_SHADOW, SHADOW_DEPTH, SHADOW_COLOR,
		SYMBOL_TYPE, SYMBOL_SIZE_X, SYMBOL_SIZE_Y, SYMBOL_COLOR, SYMBOL_FILLED, SYMBOL_FILL_COLOR,
		LINE_STYLE, LINE_WIDTH, LINE_COLOR, LINE_SMOOTHED,
		FILL_STYLE, FILL_DIRECTION, FILL_COLOR,
		HIST_DIRECTION, HIST_BAR_WIDTH, HIST_BAR_COLOR, HIST_BORDER_WIDTH, HIST_BORDER_COLOR,
		NB_GRAPH_WEATHER_PROPERTIES
	};



	void CSeriesPropertyGridCtrl::Init()
	{
		CMFCPropertyGridCtrl::Init();

		RemoveAll();

		//static COleVariant Bool((short)VARIANT_FALSE, VT_BOOL);
		//static COleVariant Int(0l, VT_I4);
		//static CString String;
		//static COLORREF Color;

		CRect rect;
		GetClientRect(rect);

		//add all attribute
		CAppOption options;
		StringVector propertyHeader(GetString(IDS_STR_PROPERTY_HEADER), ";|");
		m_nLeftColumnWidth = options.GetProfileInt(_T("WeatherChartsPropertiesSplitterPos"), rect.Width() / 2);

		EnableHeaderCtrl(true, CString(propertyHeader[0].c_str()), CString(propertyHeader[1].c_str()));
		EnableDescriptionArea(true);
		SetVSDotNetLook(true);
		MarkModifiedProperties(true);
		SetAlphabeticMode(false);
		SetShowDragContext(true);
		EnableWindow(true);
		AlwaysShowUserToolTip();



		//General

		//
		//
		StringVector section(IDS_DBEDIT_GRAPH_WEATHER, ";|");
		StringVector name(IDS_DBEDIT_GRAPH_WEATHER_PROPERTIES, ";|");
		StringVector description(IDS_DBEDIT_GRAPH_WEATHER_DESCRIPTION, ";|");


		CMFCPropertyGridProperty* pGroup = new CStdGridProperty(section[0]);
		


		ASSERT(name.size() == NB_GRAPH_WEATHER_PROPERTIES);
		ASSERT(description.size() == NB_GRAPH_WEATHER_PROPERTIES);


		CMFCPropertyGridProperty* pGraph = new CStdGridProperty(section[1]);
		pGraph->AddSubItem(new CStdGridProperty(name[TITLE], "", description[TITLE], TITLE));
		pGraph->AddSubItem(new CStdGridProperty(name[X_AXIS_TITLE], "", description[X_AXIS_TITLE], X_AXIS_TITLE));
		pGraph->AddSubItem(new CStdGridProperty(name[Y_LEFT_AXIS_TITLE], "", description[Y_LEFT_AXIS_TITLE], Y_LEFT_AXIS_TITLE));
		pGraph->AddSubItem(new CStdGridProperty(name[Y_RIGHT_AXIS_TITLE], "", description[Y_RIGHT_AXIS_TITLE], Y_RIGHT_AXIS_TITLE));
		pGraph->AddSubItem(new CStdBoolGridProperty(name[SHOW_LEGEND], true, description[SHOW_LEGEND], SHOW_LEGEND));
		pGroup->AddSubItem(pGraph);


		CMFCPropertyGridProperty* pSeries = new CStdGridProperty(section[2]);

		CMFCPropertyGridProperty* pGeneral = new CStdGridProperty(section[3]);
		pGeneral->AddSubItem(new CWeatherVariableProperty(name[VARIABLE], size_t(H_TAIR2), description[VARIABLE], VARIABLE));
		pGeneral->AddSubItem(new CStatisticProperty(name[STATISTIC], WBSF::MEAN, description[STATISTIC], STATISTIC));
		pGeneral->AddSubItem(new CSerieTypeProperty(name[SERIE_TYPE], WBSF::MEAN, description[SERIE_TYPE], SERIE_TYPE));
		pGeneral->AddSubItem(new CYAxisTypeProperty(name[Y_AXIS], CGraphSerie::LEFT_AXIS, description[Y_AXIS], Y_AXIS));
		pGeneral->AddSubItem(new CStdBoolGridProperty(name[ENABLE_SHADOW], false, description[ENABLE_SHADOW], ENABLE_SHADOW));
		pGeneral->AddSubItem(new CStdGridProperty(name[SHADOW_DEPTH], 3, description[SHADOW_DEPTH], SHADOW_DEPTH));
		pGeneral->AddSubItem(new CStdColorProperty(name[SHADOW_COLOR], RGB(220,220,220), description[SHADOW_COLOR], SHADOW_COLOR));
		pSeries->AddSubItem(pGeneral);

		CMFCPropertyGridProperty* pSymbol = new CStdGridProperty(section[4]);
		pSymbol->AddSubItem(new CSymbolProperty(name[SYMBOL_TYPE], 0, description[SYMBOL_TYPE], SYMBOL_TYPE));
		pSymbol->AddSubItem(new CStdColorProperty(name[SYMBOL_COLOR], RGB(0,0,0), description[SYMBOL_COLOR], SYMBOL_COLOR));
		pSymbol->AddSubItem(new CStdGridProperty(name[SYMBOL_SIZE_X], 6, description[SYMBOL_SIZE_X], SYMBOL_SIZE_X));
		pSymbol->AddSubItem(new CStdGridProperty(name[SYMBOL_SIZE_Y], 6, description[SYMBOL_SIZE_Y], SYMBOL_SIZE_Y));
		pSymbol->AddSubItem(new CStdBoolGridProperty(name[SYMBOL_FILLED], false, description[SYMBOL_FILLED], SYMBOL_FILLED));
		pSymbol->AddSubItem(new CStdColorProperty(name[SYMBOL_FILL_COLOR], RGB(255,255,255), description[SYMBOL_FILL_COLOR], SYMBOL_FILL_COLOR));
		pSeries->AddSubItem(pSymbol);

		CMFCPropertyGridProperty* pLine = new CStdGridProperty(section[5]);
		pLine->AddSubItem(new CLineStyleProperty(name[LINE_STYLE], CGraphSerie::lsNone, description[LINE_STYLE], LINE_STYLE));
		pLine->AddSubItem(new CStdColorProperty(name[LINE_COLOR], RGB(0,0,0), description[LINE_COLOR], LINE_COLOR));
		pLine->AddSubItem(new CStdGridProperty(name[LINE_WIDTH], 1, description[LINE_WIDTH], LINE_WIDTH));
		pLine->AddSubItem(new CStdBoolGridProperty(name[LINE_SMOOTHED], false, description[LINE_SMOOTHED], LINE_SMOOTHED));
		pSeries->AddSubItem(pLine);

		CMFCPropertyGridProperty* pFill = new CStdGridProperty(section[6]);
		pFill->AddSubItem(new CFillStyleProperty(name[FILL_STYLE], 0, description[FILL_STYLE], FILL_STYLE));
		pFill->AddSubItem(new CFillDirectionProperty(name[FILL_DIRECTION], CGraphSerie::FILL_BOTTOM, description[FILL_DIRECTION], FILL_DIRECTION));
		pFill->AddSubItem(new CStdColorProperty(name[FILL_COLOR], RGB(220,220,220), description[FILL_COLOR], FILL_COLOR));
		pSeries->AddSubItem(pFill);

		CMFCPropertyGridProperty* pHist = new CStdGridProperty(section[7]);
		pHist->AddSubItem(new CHistDirectionProperty(name[HIST_DIRECTION], 0, description[HIST_DIRECTION], HIST_DIRECTION));
		pHist->AddSubItem(new CStdGridProperty(name[HIST_BAR_WIDTH], 12, description[HIST_BAR_WIDTH], HIST_BAR_WIDTH));
		pHist->AddSubItem(new CStdColorProperty(name[HIST_BAR_COLOR], RGB(0,100,255), description[HIST_BAR_COLOR], HIST_BAR_COLOR));
		pHist->AddSubItem(new CStdGridProperty(name[HIST_BORDER_WIDTH], 1, description[HIST_BORDER_WIDTH], HIST_BORDER_WIDTH));
		pHist->AddSubItem(new CStdColorProperty(name[HIST_BORDER_COLOR], RGB(0,0,0), description[HIST_BORDER_COLOR], HIST_BORDER_COLOR));
		pSeries->AddSubItem(pHist);

		pGroup->AddSubItem(pSeries);


		AddProperty(pGroup);

	}

	void CSeriesPropertyGridCtrl::Set(int chartIndex, int serieIndex, bool bForceReload)
	{

		if (chartIndex != m_chartIndex ||
			serieIndex != m_serieIndex ||
			bForceReload)
		{
			if (m_pSel != NULL && m_pSel->IsInPlaceEditing() != NULL && m_pSel->IsEnabled())
			{
				if (m_pSel->GetOptionCount() > 0)
					OnSelectCombo();

				EndEditItem();
			}

			m_chartIndex = chartIndex;
			m_serieIndex = serieIndex;

			if (m_chartIndex >= 0 && m_chartIndex < m_graphics.size())
			{
				const CGraph& graph = m_graphics[m_chartIndex];
				if (m_serieIndex >= 0 && m_serieIndex < graph.m_series.size())
				{
					const CGraphSerie& serie = graph.m_series[m_serieIndex];

					for (int i = 0; i < NB_GRAPH_WEATHER_PROPERTIES; i++)
					{
						std::string str;

						switch (i)
						{
						case TITLE:				str = graph.m_title; break;
						case X_AXIS_TITLE:		str = graph.m_Xtitle; break;
						case Y_LEFT_AXIS_TITLE: str = graph.m_Ytitle1; break;
						case Y_RIGHT_AXIS_TITLE:str = graph.m_Ytitle2; break;
						case SHOW_LEGEND:		str = ToString(graph.m_bShowLegend); break;
												
						case VARIABLE:			str = ToString(serie.m_variable); break;
						case STATISTIC:			str = ToString(serie.m_statistic); break;
						case SERIE_TYPE:		str = ToString(serie.m_type); break;
						case Y_AXIS:			str = ToString(serie.m_YAxis); break;
						case ENABLE_SHADOW:		str = ToString(serie.m_bEnableShadow); break;
						case SHADOW_DEPTH:		str = ToString(serie.m_shadowDepth); break;
						case SHADOW_COLOR:		str = ToString(serie.m_shadowColor); break;
												
						case SYMBOL_TYPE:		str = ToString(serie.m_symbolType); break;
						case SYMBOL_SIZE_X:		str = ToString(serie.m_symbolWidth); break;
						case SYMBOL_SIZE_Y:		str = ToString(serie.m_symbolHeight); break;
						case SYMBOL_COLOR:		str = ToString(serie.m_symbolColor); break;
						case SYMBOL_FILLED:		str = ToString(serie.m_bSymbolFilled); break;
						case SYMBOL_FILL_COLOR: str = ToString(serie.m_symbolFillColor); break;
												
						case LINE_STYLE:		str = ToString(serie.m_lineStyle); break;
						case LINE_WIDTH:		str = ToString(serie.m_lineWidth); break;
						case LINE_COLOR:		str = ToString(serie.m_lineColor); break;
						case LINE_SMOOTHED:		str = ToString(serie.m_bLineSmoothed); break;
												
						case FILL_STYLE:		str = ToString(serie.m_fillStyle); break;
						case FILL_DIRECTION:	str = ToString(serie.m_fillDirection); break;
						case FILL_COLOR:		str = ToString(serie.m_fillColor); break;
												
						case HIST_DIRECTION:	str = ToString(serie.m_histDirection); break;
						case HIST_BAR_WIDTH:	str = ToString(serie.m_histBarWidth); break;
						case HIST_BAR_COLOR:	str = ToString(serie.m_histBarColor); break;
						case HIST_BORDER_WIDTH:	str = ToString(serie.m_histBorderWidth); break;
						case HIST_BORDER_COLOR:	str = ToString(serie.m_histBorderColor); break;
						default: ASSERT(false);
						}

						CStdGriInterface* pProp = dynamic_cast<CStdGriInterface*>(FindItemByData(i));
						ASSERT(pProp);
						pProp->set_string(str);

					}//for all member
				}
			}

			EnableProperties(m_chartIndex >= 0 && m_serieIndex >= 0);
		}
	}


	void CSeriesPropertyGridCtrl::OnPropertyChanged(CMFCPropertyGridProperty* pPropIn) const
	{
		ASSERT(m_chartIndex >= 0 && m_serieIndex >= 0);

		if (m_chartIndex >= 0 && m_serieIndex >= 0)
		{
			CStdGridProperty* pProp = static_cast<CStdGridProperty*>(pPropIn);

			CGraph& graph = m_graphics[m_chartIndex];
			CGraphSerie& serie = graph.m_series[m_serieIndex];

			std::string value = pProp->get_string();

			int i = (int)pProp->GetData();
			switch (i)
			{
			case TITLE:				graph.m_title = value; break;
			case X_AXIS_TITLE:		graph.m_Xtitle = value; break;
			case Y_LEFT_AXIS_TITLE: graph.m_Ytitle1 = value; break;
			case Y_RIGHT_AXIS_TITLE:graph.m_Ytitle2 = value; break;
			case SHOW_LEGEND:		graph.m_bShowLegend = as<bool>(value); break;

			case VARIABLE:			serie.m_variable = as<int>(value); break;
			case STATISTIC:			serie.m_statistic = as<int>(value);
			case SERIE_TYPE:		serie.m_type = as<int>(value); break;
			case Y_AXIS:			serie.m_YAxis = as<int>(value); break;
			case ENABLE_SHADOW:		serie.m_bEnableShadow = as<bool>(value); break;
			case SHADOW_DEPTH:		serie.m_shadowDepth = as<int>(value); break;
			case SHADOW_COLOR:		serie.m_shadowColor = ToCOLORREF(value); break;

			case SYMBOL_TYPE:		serie.m_symbolType = as<int>(value); break;
			case SYMBOL_COLOR:		serie.m_symbolColor = ToCOLORREF(value); break;
			case SYMBOL_SIZE_X:		serie.m_symbolWidth = as<int>(value); break;
			case SYMBOL_SIZE_Y:		serie.m_symbolHeight = as<int>(value); break;
			case SYMBOL_FILLED:		serie.m_bSymbolFilled = as<int>(value); break;
			case SYMBOL_FILL_COLOR: serie.m_symbolFillColor = ToCOLORREF(value); break;

			case LINE_STYLE:		serie.m_lineStyle = as<int>(value); break;
			case LINE_WIDTH:		serie.m_lineWidth = as<int>(value); break;
			case LINE_COLOR:		serie.m_lineColor = ToCOLORREF(value); break;
			case LINE_SMOOTHED:		serie.m_bLineSmoothed = as<bool>(value); break;

			case FILL_STYLE:		serie.m_fillStyle = as<int>(value); break;
			case FILL_DIRECTION:	serie.m_fillDirection = as<int>(value); break;
			case FILL_COLOR:		serie.m_fillColor = ToCOLORREF(value); break;

			case HIST_DIRECTION:	serie.m_histDirection = as<int>(value); break;
			case HIST_BAR_WIDTH:	serie.m_histBarWidth = as<int>(value); break;
			case HIST_BAR_COLOR:	serie.m_histBarColor = ToCOLORREF(value); break;
			case HIST_BORDER_WIDTH:	serie.m_histBorderWidth = as<int>(value); break;
			case HIST_BORDER_COLOR:	serie.m_histBorderColor = ToCOLORREF(value); break;

			default: ASSERT(false);
			}
		}
	}

	void CSeriesPropertyGridCtrl::EnableProperties(BOOL bEnable)
	{
		for (int i = 0; i < GetPropertyCount(); i++)
		{
			CMFCPropertyGridProperty* pProp = GetProperty(i);
			EnableProperties(pProp, bEnable);
		}
	}

	void CSeriesPropertyGridCtrl::EnableProperties(CMFCPropertyGridProperty* pProp, BOOL bEnable)
	{
		pProp->Enable(bEnable);

		for (int ii = 0; ii < pProp->GetSubItemsCount(); ii++)
			EnableProperties(pProp->GetSubItem(ii), bEnable);
	}


	BOOL CSeriesPropertyGridCtrl::PreTranslateMessage(MSG* pMsg)
	{

		if (pMsg->message == WM_KEYDOWN)
		{
			BOOL bAlt = GetKeyState(VK_CONTROL) & 0x8000;
			if (pMsg->wParam == VK_RETURN)
			{
				if (m_pSel != NULL && m_pSel->IsInPlaceEditing() != NULL && m_pSel->IsEnabled())
				{
					if (m_pSel->GetOptionCount()>0)
						OnSelectCombo();

					EndEditItem();

					//select next item
					size_t ID = m_pSel->GetData();
					size_t newID = (ID + 1) % GetProperty(0)->GetSubItemsCount();
					CMFCPropertyGridProperty* pNext = FindItemByData(newID);
					if (pNext)
					{
						SetCurSel(pNext);
						EditItem(pNext);
					}

					return TRUE; // this doesn't need processing anymore
				}

			}
			else if (pMsg->wParam == VK_DOWN && bAlt)
			{
				m_pSel->OnClickButton(CPoint(-1, -1));
				return TRUE; // this doesn't need processing anymore
			}
		}

		return CMFCPropertyGridCtrl::PreTranslateMessage(pMsg); // all other cases still need default processing;
	}

	BOOL CSeriesPropertyGridCtrl::ValidateItemData(CMFCPropertyGridProperty* /*pProp*/)
	{
		return TRUE;
	}


	//**************************************************************************************************************

	// CChartsProperties dialog

	IMPLEMENT_DYNAMIC(CChartsProperties, CDialog)

		BEGIN_MESSAGE_MAP(CChartsProperties, CDialog)
			ON_WM_SIZE()
			ON_WM_DESTROY()
		END_MESSAGE_MAP()

		void CChartsProperties::DoDataExchange(CDataExchange* pDX)
		{
			CDialog::DoDataExchange(pDX);
			DDX_Control(pDX, IDC_CHARTS, m_chartsCtrl);
			DDX_Control(pDX, IDC_SERIES, m_seriesCtrl);
			DDX_Control(pDX, IDC_SERIES_PROPERTY, m_seriesPropertiesCtrl);


			if (!pDX->m_bSaveAndValidate)
			{
				m_chartsCtrl.FillCharts();

				//resize window
				CRect rectClient;
				GetWindowRect(rectClient);

				CAppOption option;
				rectClient = option.GetProfileRect(_T("WeatherChartsPropertiesRect"), rectClient);
				UtilWin::EnsureRectangleOnDisplay(rectClient);
				SetWindowPos(NULL, rectClient.left, rectClient.top, rectClient.Width(), rectClient.Height(), SWP_NOACTIVATE | SWP_NOZORDER);
			}


		}



		CChartsProperties::CChartsProperties(CWnd* pParent /*=NULL*/) :
			CDialog(CChartsProperties::IDD, pParent),
			m_seriesPropertiesCtrl(m_graphics),
			m_seriesCtrl(m_graphics, m_seriesPropertiesCtrl),
			m_chartsCtrl(m_graphics, m_seriesCtrl)
		{

		}

		CChartsProperties::~CChartsProperties()
		{
		}


		void CChartsProperties::OnSize(UINT nType, int cx, int cy)
		{
			CDialog::OnSize(nType, cx, cy);

			AdjustLayout();
		}

		void CChartsProperties::AdjustLayout()
		{
			static const int MARGE = 10;
			if (GetSafeHwnd() == NULL || m_seriesPropertiesCtrl.GetSafeHwnd() == NULL)
			{
				return;
			}

			CRect rectClient;
			GetClientRect(rectClient);

			CRect rectOK;
			GetDlgItem(IDOK)->GetWindowRect(rectOK); ScreenToClient(rectOK);

			rectOK.left = rectClient.right - MARGE - rectOK.Width();
			rectOK.right = rectClient.right - MARGE;

			CRect rectCancel;
			GetDlgItem(IDCANCEL)->GetWindowRect(rectCancel); ScreenToClient(rectCancel);
			rectCancel.left = rectClient.right - MARGE - rectCancel.Width();
			rectCancel.right = rectClient.right - MARGE;

			CRect rect1;
			m_chartsCtrl.GetWindowRect(rect1); ScreenToClient(rect1);
			rect1.bottom = rectClient.bottom - MARGE;

			CRect rect2;
			m_seriesCtrl.GetWindowRect(rect2); ScreenToClient(rect2);
			rect2.bottom = rectClient.bottom - MARGE;

			CRect rect3;
			m_seriesPropertiesCtrl.GetWindowRect(rect3); ScreenToClient(rect3);
			rect3.right = rectClient.right - MARGE - rectOK.Width() - MARGE;
			rect3.bottom = rectClient.bottom - MARGE;

			m_chartsCtrl.SetWindowPos(NULL, 0, 0, rect1.Width(), rect1.Height(), SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE);
			m_seriesCtrl.SetWindowPos(NULL, 0, 0, rect2.Width(), rect2.Height(), SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE);
			m_seriesPropertiesCtrl.SetWindowPos(NULL, 0, 0, rect3.Width(), rect3.Height(), SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE);

			GetDlgItem(IDOK)->SetWindowPos(NULL, rectOK.left, rectOK.top, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE);
			GetDlgItem(IDCANCEL)->SetWindowPos(NULL, rectCancel.left, rectCancel.top, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE);
		}


		void CChartsProperties::OnDestroy()
		{
			CRect rectClient;
			GetWindowRect(rectClient);

			CAppOption option;
			option.WriteProfileRect(_T("WeatherChartsPropertiesRect"), rectClient);

			CDialog::OnDestroy();
		}

}