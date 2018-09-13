//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//	
//******************************************************************************
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
//******************************************************************************
#include "StdAfx.h"

#include "Basic/Dimension.h"
#include "Basic/UtilMath.h"
#include "Basic/WeatherDefine.h"
#include "Simulation/Graph.h"

#include "WeatherBasedSimulationString.h"

using namespace std;
using namespace WBSF::DIMENSION;
using namespace WBSF::HOURLY_DATA;


namespace WBSF
{

	static const CGraphSerie DEFAULT_WEATHER_SERIES[NB_VAR_H] =
	{
		{ "Tmin", 0, H_TMIN, MEAN, CGraphSerie::LEFT_AXIS, false, 2, RGB(200, 200, 200), CGraphSerie::stNone, 6, 6, RGB(000, 000, 255), true, RGB(255, 255, 255), CGraphSerie::lsSolid, 1, RGB(000, 000, 255), false, CGraphSerie::fsNone, CGraphSerie::FILL_BOTTOM, RGB(245, 245, 245) },
		{ "Tair", 0, H_TAIR, MEAN, CGraphSerie::LEFT_AXIS, false, 2, RGB(200, 200, 200), CGraphSerie::stNone, 6, 6, RGB(000, 000, 000), true, RGB(255, 255, 255), CGraphSerie::lsSolid, 1, RGB(145, 45, 170), false, CGraphSerie::fsNone, CGraphSerie::FILL_BOTTOM, RGB(245, 245, 245) },
		{ "Tmax", 0, H_TMAX, MEAN, CGraphSerie::LEFT_AXIS, false, 2, RGB(200, 200, 200), CGraphSerie::stNone, 6, 6, RGB(255, 000, 000), true, RGB(255, 255, 255), CGraphSerie::lsSolid, 1, RGB(255, 000, 000), false, CGraphSerie::fsNone, CGraphSerie::FILL_BOTTOM, RGB(245, 245, 245) },
		{ "Prcp", 1, H_PRCP, SUM, CGraphSerie::LEFT_AXIS, false, 2, RGB(200, 200, 200), CGraphSerie::HIST_VERTICAL, 6, RGB(000, 192, 255), 1, RGB(0, 0, 0) },
		{ "Tdew", 0, H_TDEW, MEAN, CGraphSerie::LEFT_AXIS, false, 2, RGB(200, 200, 200), CGraphSerie::stNone, 6, 6, RGB(000, 150, 000), true, RGB(255, 255, 255), CGraphSerie::lsSolid, 1, RGB(060, 150, 030), false, CGraphSerie::fsNone, CGraphSerie::FILL_BOTTOM, RGB(245, 245, 245) },
		{ "RelH", 0, H_RELH, MEAN, CGraphSerie::RIGHT_AXIS, false, 2, RGB(200, 200, 200), CGraphSerie::stNone, 6, 6, RGB(000, 000, 000), true, RGB(255, 255, 255), CGraphSerie::lsSolid, 1, RGB(100, 100, 100), false, CGraphSerie::fsHatchDownDiag, CGraphSerie::FILL_BOTTOM, RGB(215, 215, 215) },
		{ "WndS", 2, H_WNDS, MEAN, CGraphSerie::LEFT_AXIS, false, 2, RGB(200, 200, 200), CGraphSerie::stNone, 6, 6, RGB(000, 000, 000), true, RGB(255, 255, 255), CGraphSerie::lsSolid, 1, RGB(120, 040, 020), false, CGraphSerie::fsHatchUpDiag, CGraphSerie::FILL_BOTTOM, RGB(200, 200, 200) },
		{ "WndD", 2, H_WNDD, MEAN, CGraphSerie::RIGHT_AXIS, false, 2, RGB(200, 200, 200), CGraphSerie::stElli, 4, 4, RGB(000, 000, 255), true, RGB(160, 160, 160), CGraphSerie::lsNone, 1, RGB(000, 000, 000), false, CGraphSerie::fsNone, CGraphSerie::FILL_BOTTOM, RGB(245, 245, 245) },
		{ "SRad", 3, H_SRAD, MEAN, CGraphSerie::LEFT_AXIS, false, 2, RGB(200, 200, 200), CGraphSerie::stNone, 6, 6, RGB(255, 192, 000), true, RGB(255, 255, 255), CGraphSerie::lsSolid, 1, RGB(255, 192, 000), false, CGraphSerie::fsNone, CGraphSerie::FILL_BOTTOM, RGB(245, 245, 125) },
		{ "Pres", 3, H_PRES, MEAN, CGraphSerie::RIGHT_AXIS, false, 2, RGB(200, 200, 200), CGraphSerie::stNone, 6, 6, RGB(255, 000, 000), true, RGB(255, 255, 255), CGraphSerie::lsSolid, 1, RGB(030, 100, 050), false, CGraphSerie::fsNone, CGraphSerie::FILL_BOTTOM, RGB(245, 245, 245) },
		{ "Snow", 1, H_SNOW, SUM, CGraphSerie::LEFT_AXIS, false, 2, RGB(200, 200, 200), CGraphSerie::HIST_VERTICAL, 6, RGB(192, 192, 192), 1, RGB(0, 0, 0) },
		{ "SnDh", 1, H_SNDH, MEAN, CGraphSerie::RIGHT_AXIS, false, 2, RGB(000, 000, 000), CGraphSerie::stNone, 6, 6, RGB(000, 000, 000), true, RGB(000, 255, 255), CGraphSerie::lsSolid, 1, RGB(120, 120, 245), false, CGraphSerie::fsHatchDiagCross, CGraphSerie::FILL_BOTTOM, RGB(120, 120, 245) },
		{ "SWE", 1, H_SWE, MEAN, CGraphSerie::LEFT_AXIS, false, 2, RGB(200, 200, 200), CGraphSerie::stRect, 6, 6, RGB(000, 000, 255), true, RGB(255, 255, 255), CGraphSerie::lsNone, 1, RGB(000, 000, 255), false, CGraphSerie::fsNone, CGraphSerie::FILL_BOTTOM, RGB(245, 245, 245) },
		{ "Wnd2", 2, H_WND2, MEAN, CGraphSerie::LEFT_AXIS, false, 2, RGB(200, 200, 200), CGraphSerie::stNone, 6, 6, RGB(255, 000, 000), true, RGB(255, 255, 255), CGraphSerie::lsSolid, 1, RGB(255, 000, 255), false, CGraphSerie::fsNone, CGraphSerie::FILL_BOTTOM, RGB(245, 245, 245) },
		{ "Add1", 4, H_ADD1, MEAN, CGraphSerie::LEFT_AXIS, false, 2, RGB(200, 200, 200), CGraphSerie::stRect, 6, 6, RGB(000, 255, 255), true, RGB(255, 255, 255), CGraphSerie::lsNone, 1, RGB(255, 000, 000), false, CGraphSerie::fsNone, CGraphSerie::FILL_BOTTOM, RGB(245, 245, 245) },
		{ "Add2", 4, H_ADD2, MEAN, CGraphSerie::RIGHT_AXIS, false, 2, RGB(200, 000, 200), CGraphSerie::stRect, 6, 6, RGB(255, 000, 255), true, RGB(255, 255, 255), CGraphSerie::lsNone, 1, RGB(255, 000, 000), false, CGraphSerie::fsNone, CGraphSerie::FILL_BOTTOM, RGB(245, 245, 245) },
	};

	enum { NB_CHARTS = 5, NB_VAR_MAX = 5 };


	static const int DEFAULT_HOURLY_CHARTS[NB_CHARTS][NB_VAR_MAX] =
	{
		{ H_RELH, H_TDEW, H_TAIR, -1, -1 },//air
		{ H_SNDH, H_SWE, H_SNOW, H_PRCP, -1 },//ground
		{ H_WNDS, H_WND2, H_WNDD, -1, -1 },
		{ H_SRAD, H_PRES, -1, -1, -1 },
		{ H_ADD1, H_ADD2, -1, -1, -1 },
	};

	static const int DEFAULT_DAILY_CHARTS[NB_CHARTS][NB_VAR_MAX] =
	{
		{ H_RELH, H_TDEW, H_TMIN, H_TMAX, -1 },//air
		{ H_SNDH, H_SWE, H_SNOW, H_PRCP, -1 },//ground
		{ H_WNDS, H_WND2, H_WNDD, -1, -1 },
		{ H_SRAD, H_PRES, -1, -1, -1 },
		{ H_ADD1, H_ADD2, -1, -1, -1 },
	};

	CGraphVector GetDefaultWeatherGraphVector(bool bHourly)
	{
		string XAxisTitle = GetString(IDS_WG_WEATHER_XAXIS_TITLE);
		StringVector graphName(GetString(IDS_WG_WEATHER_GRAPH_TITLE), ";|");
		StringVector leftYAxisTitle(GetString(IDS_WG_WEATHER_YAXIS_TITLE1), ";|");
		StringVector rightYAxisTitle(IDS_WG_WEATHER_YAXIS_TITLE2, ";|");


		CGraphVector graphics(NB_CHARTS);
		//load empty
		for (size_t c = 0; c < NB_CHARTS; c++)
		{

			graphics[c].m_name = graphName[c];
			//graphics[c].m_title = graphName[c];
			//graphics[c].m_Xtitle = XAxisTitle;
			graphics[c].m_Ytitle1 = leftYAxisTitle[c];
			graphics[c].m_Ytitle2 = rightYAxisTitle[c];
			graphics[c].m_bShowLegend = true;

			for (size_t v = 0; v < NB_VAR_MAX; v++)
			{
				int vv = bHourly ? DEFAULT_HOURLY_CHARTS[c][v] : DEFAULT_DAILY_CHARTS[c][v];
				if (vv >= 0)
					graphics[c].m_series.push_back(DEFAULT_WEATHER_SERIES[vv]);
			}
		}


		return graphics;
	}

	//****************************************************************************************************
	const char* CGraphSerie::XML_FLAG = "Variable";

	StringVector CGraphSerie::SERIE_TYPE_TITLE;
	std::string CGraphSerie::GetSerieTypeTitle(size_t i)
	{
		assert(i < NB_SERIE_TYPE);

		if (SERIE_TYPE_TITLE.empty())
			SERIE_TYPE_TITLE.LoadString(IDS_WG_GRAPH_SERIE_TYPE_TITLE, ";|");

		assert(SERIE_TYPE_TITLE.size() == NB_SERIE_TYPE);

		return SERIE_TYPE_TITLE[i];
	}

	StringVector CGraphSerie::Y_AXIS_TYPE_TITLE;
	std::string CGraphSerie::GetYAxisTypeTitle(size_t i)
	{
		assert(i < NB_YAXIS_TYPE);

		if (Y_AXIS_TYPE_TITLE.empty())
			Y_AXIS_TYPE_TITLE.LoadString(IDS_STR_LEFT_RIGHT, ";|");

		assert(Y_AXIS_TYPE_TITLE.size() == NB_YAXIS_TYPE);

		return Y_AXIS_TYPE_TITLE[i];
	}


	StringVector CGraphSerie::FILL_DIRECTION;
	const char* CGraphSerie::GetFillDirectionTitle(size_t i)
	{
		assert(i < NB_FILL_DIRECTION_TYPE);

		if (FILL_DIRECTION.empty())
			FILL_DIRECTION.LoadString(IDS_WG_GRAPH_FILL_DIR_TITLE, ";|");

		assert(FILL_DIRECTION.size() == NB_FILL_DIRECTION_TYPE);

		return FILL_DIRECTION[i].c_str();
	}

	StringVector CGraphSerie::SYMBOL_TYPE_TITLE;
	std::string CGraphSerie::GetSyboleTypeTitle(int i)
	{
		assert(i >= -1 && i < NB_SYMBOL_TYPE);

		if (SYMBOL_TYPE_TITLE.empty())
			SYMBOL_TYPE_TITLE.LoadString(IDS_WG_GRAPH_SYMBOL_TYPE, ";|");

		assert(SYMBOL_TYPE_TITLE.size() == NB_SYMBOL_TYPE + 1);

		return SYMBOL_TYPE_TITLE[i + 1];
	}
	StringVector CGraphSerie::LINE_STYLE_TITLE;
	std::string CGraphSerie::GetLineStyleTitle(int i)
	{
		assert(i >= -1 && i < NB_LINE_STYLE);

		if (LINE_STYLE_TITLE.empty())
			LINE_STYLE_TITLE.LoadString(IDS_WG_GRAPH_LINE_STYLE, ";|");

		assert(LINE_STYLE_TITLE.size() == NB_LINE_STYLE + 1);

		return LINE_STYLE_TITLE[i + 1];
	}
	StringVector CGraphSerie::FILL_STYLE_TITLE;
	std::string CGraphSerie::GetFillStyleTitle(int i)
	{
		assert(i >= -1 && i < NB_FILL_STYLE);

		if (FILL_STYLE_TITLE.empty())
			FILL_STYLE_TITLE.LoadString(IDS_WG_GRAPH_FILL_STYLE, ";|");

		assert(FILL_STYLE_TITLE.size() == NB_FILL_STYLE + 1);

		return FILL_STYLE_TITLE[i + 1].c_str();
	}

	StringVector CGraphSerie::HIST_DIRECTION;
	const char* CGraphSerie::GetHistDirectionTitle(size_t i)
	{
		assert(i >= -1 && i < NB_HIST_DIRECTION_TYPE);

		if (HIST_DIRECTION.empty())
			HIST_DIRECTION.LoadString(IDS_WG_GRAPH_HIST_DIR_TITLE, ";|");

		assert(HIST_DIRECTION.size() == NB_HIST_DIRECTION_TYPE);

		return HIST_DIRECTION[i].c_str();
	}

	void CGraphSerie::ReloadString()
	{

	}

	CGraphSerie::CGraphSerie(string name, int dimension, int field, int statistic)
	{
		Reset();

		m_name = name;
		m_dimension = dimension;
		m_variable = field;
		m_statistic = statistic;
	}

	CGraphSerie::CGraphSerie(string name, int dimension, int var, int statistic, int YAxis, bool bEnableShadow, int shadowDepth, COLORREF shadowColor, int histDirection, int histBarWidth, COLORREF histBarColor, int histBorderWidth, COLORREF histBorderColor)
	{
		m_type = HISTOGRAM;
		m_name = name;
		m_dimension = dimension;
		m_variable = var;
		m_statistic = statistic;
		m_YAxis = YAxis;
		m_bEnableShadow = bEnableShadow;
		m_shadowDepth = shadowDepth;
		m_shadowColor = shadowColor;

		m_histDirection = histDirection;
		m_histBarWidth = histBarWidth;
		m_histBarColor = histBarColor;
		m_histBorderWidth = histBorderWidth;
		m_histBorderColor = histBorderColor;
	}


	CGraphSerie::CGraphSerie(string name, int dimension, int var, int statistic, int YAxis, bool bEnableShadow, int shadowDepth, COLORREF shadowColor, int symbolType, int symbolHeight, int symbolWide, COLORREF symbolColor, bool bSymbolFilled, COLORREF symbolFillColor, int lineStyle, int lineWidth, COLORREF lineColor, bool lineSmoothed, int fillStyle, int fillDirection, COLORREF fillColor)
	{
		m_type = XY;
		m_name = name;
		m_dimension = dimension;
		m_variable = var;
		m_statistic = statistic;
		m_YAxis = YAxis;
		m_bEnableShadow = bEnableShadow;
		m_shadowDepth = shadowDepth;
		m_shadowColor = shadowColor;

		m_symbolType = symbolType;
		m_symbolHeight = symbolHeight;
		m_symbolWidth = symbolWide;
		m_symbolColor = symbolColor;
		m_bSymbolFilled = bSymbolFilled;
		m_symbolFillColor = symbolFillColor;

		m_lineStyle = lineStyle;
		m_lineWidth = lineWidth;
		m_lineColor = lineColor;
		m_bLineSmoothed = lineSmoothed;

		m_fillStyle = fillStyle;
		m_fillDirection = fillDirection;
		m_fillColor = fillColor;


	}

	void CGraphSerie::Reset()
	{
		m_type = XY;
		m_name.clear();
		m_dimension = -1;
		m_variable = -1;
		m_statistic = MEAN;
		m_YAxis = LEFT_AXIS;
		m_bEnableShadow = false;
		m_shadowDepth = 2;
		m_shadowColor = RGB(100, 100, 100);

		m_symbolType = stNone;
		m_symbolHeight = 5;
		m_symbolWidth = 5;
		m_symbolColor = RGB(Rand(255), Rand(255), Rand(255));
		m_bSymbolFilled = false;
		m_symbolFillColor = RGB(Rand(255), 0, 0);

		m_lineStyle = lsNone;
		m_lineWidth = 1;
		m_lineColor = m_symbolColor;
		m_bLineSmoothed = false;

		m_fillStyle = fsNone;
		m_fillDirection = FILL_BOTTOM;
		m_fillColor = RGB(250, 250, 250);

		m_histDirection = HS_VERTICAL;
		m_histBarWidth = 1;
		m_histBarColor = RGB(0, 0, 0);
		m_histBorderWidth = 5;
		m_histBorderColor = RGB(0, 0, 255);
	}

	CGraphSerie& CGraphSerie::operator=(const CGraphSerie& in)
	{
		if (&in != this)
		{
			m_type = in.m_type;
			m_name = in.m_name;
			m_dimension = in.m_dimension;
			m_variable = in.m_variable;
			m_statistic = in.m_statistic;
			m_YAxis = in.m_YAxis;
			m_bEnableShadow = in.m_bEnableShadow;
			m_shadowDepth = in.m_shadowDepth;
			m_shadowColor = in.m_shadowColor;

			m_symbolType = in.m_symbolType;
			m_symbolHeight = in.m_symbolHeight;
			m_symbolWidth = in.m_symbolWidth;
			m_symbolColor = in.m_symbolColor;
			m_bSymbolFilled = in.m_bSymbolFilled;
			m_symbolFillColor = in.m_symbolFillColor;

			m_lineStyle = in.m_lineStyle;
			m_lineWidth = in.m_lineWidth;
			m_lineColor = in.m_lineColor;
			m_bLineSmoothed = in.m_bLineSmoothed;

			m_fillStyle = in.m_fillStyle;
			m_fillDirection = in.m_fillDirection;
			m_fillColor = in.m_fillColor;

			m_histDirection = in.m_histDirection;
			m_histBarWidth = in.m_histBarWidth;
			m_histBarColor = in.m_histBarColor;
			m_histBorderWidth = in.m_histBorderWidth;
			m_histBorderColor = in.m_histBorderColor;
		}

		ASSERT(*this == in);

		return *this;
	}

	bool CGraphSerie::operator==(const CGraphSerie& in)const
	{
		bool bEqual = true;
		if (m_name != in.m_name) bEqual = false;
		if (m_dimension != in.m_dimension) bEqual = false;
		if (m_variable != in.m_variable) bEqual = false;
		if (m_statistic != in.m_statistic) bEqual = false;
		if (m_type != in.m_type) bEqual = false;
		if (m_YAxis != in.m_YAxis) bEqual = false;
		if (m_bEnableShadow != in.m_bEnableShadow) bEqual = false;
		if (m_shadowDepth != in.m_shadowDepth) bEqual = false;
		if (m_shadowColor != in.m_shadowColor) bEqual = false;

		if (m_symbolType != in.m_symbolType) bEqual = false;
		if (m_symbolHeight != in.m_symbolHeight) bEqual = false;
		if (m_symbolWidth != in.m_symbolWidth) bEqual = false;
		if (m_symbolColor != in.m_symbolColor) bEqual = false;
		if (m_bSymbolFilled != in.m_bSymbolFilled) bEqual = false;
		if (m_symbolFillColor != in.m_symbolFillColor) bEqual = false;

		if (m_lineStyle != in.m_lineStyle) bEqual = false;
		if (m_lineWidth != in.m_lineWidth) bEqual = false;
		if (m_lineColor != in.m_lineColor) bEqual = false;
		if (m_bLineSmoothed != in.m_bLineSmoothed) bEqual = false;

		if (m_fillStyle != in.m_fillStyle) bEqual = false;
		if (m_fillDirection != in.m_fillDirection) bEqual = false;
		if (m_fillColor != in.m_fillColor) bEqual = false;

		if (m_histDirection != in.m_histDirection) bEqual = false;
		if (m_histBarWidth != in.m_histBarWidth) bEqual = false;
		if (m_histBarColor != in.m_histBarColor) bEqual = false;
		if (m_histBorderWidth != in.m_histBorderWidth) bEqual = false;
		if (m_histBorderColor != in.m_histBorderColor) bEqual = false;

		return bEqual;
	}

	/*
	void CGraphSerie::GetXML(LPXNode& pRoot)const
	{
	ASSERT( pRoot != NULL );

	XNode* pNode = pRoot->AppendChild( GetXMLFlag(), "" );
	pNode->AppendAttr("Dimension", ToString(m_dimension) );
	pNode->AppendAttr("Field", ToString(m_variable) );
	pNode->AppendAttr("Type", ToString(m_type) );
	pNode->AppendAttr("Color", ToString(m_color) );
	pNode->AppendAttr("PointSymbol", ToString(m_symbolType )  );
	pNode->AppendAttr("PointSize", ToString(m_symbolSize ) );
	pNode->AppendAttr("LineStyle", ToString(m_lineStyle )  );
	pNode->AppendAttr("LineWidth", ToString(m_lineWidth) );
	pNode->AppendAttr("LineSmoothed", ToString(m_bLineSmoothed) );

	}

	void CGraphSerie::SetXML(const LPXNode pNode)
	{
	m_dimension = ToInt(pNode->attrValue("Dimension" ));
	m_variable = ToInt(pNode->attrValue("Field" ));

	m_type = ToInt(pNode->attrValue("Type") );
	m_color = ToCOLORREF(pNode->attrValue("Color") );
	m_symbolType = ToInt(pNode->attrValue("PointSymbol")  );
	m_symbolSize = ToInt(pNode->attrValue("PointSize") );
	m_lineStyle = ToInt(pNode->attrValue("LineStyle")  );
	m_lineWidth = ToInt(pNode->attrValue("LineWidth") );
	m_bLineSmoothed = ToInt(pNode->attrValue("LineSmoothed") );
	}
	*/
	void CGraphSerie::LoadDefault()
	{
		//CAppOption option("GraphSeries");

		//m_type = option.GetProfileInt("Type", m_type);
		//m_symbolType = option.GetProfileInt("PointSymbol", m_symbolType);
		//m_symbolSize = option.GetProfileInt("PointSize", m_symbolSize);
		//m_lineStyle = option.GetProfileInt("LineStyle", m_lineStyle);
		//m_lineWidth = option.GetProfileInt("LineWidth", m_lineWidth);
		//m_bLineSmoothed = option.GetProfileInt("LineSmoothed", m_bLineSmoothed);
	}

	void CGraphSerie::SaveDefault()
	{
		//CAppOption option("GraphSeries");
		//option.WriteProfileInt("Type", m_type);
		//option.WriteProfileInt("PointSymbol", m_symbolType);
		//option.WriteProfileInt("PointSize", m_symbolSize);
		//option.WriteProfileInt("LineStyle", m_lineStyle);
		//option.WriteProfileInt("LineWidth", m_lineWidth);
		//option.WriteProfileInt("LineSmoothed", m_bLineSmoothed);
	}

	//******************************************************************************************************************************
	const char* CGraph::XML_FLAG = "Graph";
	const char* CGraph::MEMBER_NAME[NB_MEMBER] = { "Name", "Title", "XTitle", "LeftYTitle", "RightYTitle", "Variable", "ShowLegend", "VariableArray", "FirstLine", "LastLine" };


	CGraph::CGraph()
	{
		Reset();
	}

	CGraph::CGraph(const CGraph& in)
	{
		operator=(in);
	}

	CGraph::~CGraph()
	{}

	void CGraph::Reset()
	{
		m_name.empty();
		m_title.empty();
		m_Xtitle.empty();
		m_Ytitle1.empty();
		m_Ytitle2.empty();
		m_XAxis.Reset();
		m_bShowLegend = false;
		m_series.clear();
		m_firstLine = 0;
		m_lastLine = 364;
	}

	CGraph& CGraph::operator=(const CGraph& in)
	{
		if (&in != this)
		{
			m_name = in.m_name;
			m_title = in.m_title;
			m_Xtitle = in.m_Xtitle;
			m_Ytitle1 = in.m_Ytitle1;
			m_Ytitle2 = in.m_Ytitle2;
			m_XAxis = in.m_XAxis;
			m_bShowLegend = in.m_bShowLegend;
			m_series = in.m_series;
			m_firstLine = in.m_firstLine;
			m_lastLine = in.m_lastLine;
		}


		return *this;
	}

	bool CGraph::operator==(const CGraph& in)const
	{
		bool bEqual = true;

		if (m_name != in.m_name) bEqual = false;
		if (m_title != in.m_title)bEqual = false;
		if (m_Xtitle != in.m_Xtitle)bEqual = false;
		if (m_Ytitle1 != in.m_Ytitle1)bEqual = false;
		if (m_Ytitle2 != in.m_Ytitle2)bEqual = false;
		if (m_XAxis != in.m_XAxis)bEqual = false;
		if (m_bShowLegend != in.m_bShowLegend)bEqual = false;
		if (m_series != in.m_series)bEqual = false;
		if (m_firstLine != in.m_firstLine)bEqual = false;
		if (m_lastLine != in.m_lastLine)bEqual = false;

		return bEqual;
	}


}

