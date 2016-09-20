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

#include "Basic/UtilZen.h"
#include "Basic/Statistic.h"
#include "Simulation/ExportDefine.h"

namespace WBSF
{

	class CGraphSerie
	{
	public:

		enum TSerie{ XY, HISTOGRAM, STICK, BOXPLOT, NB_SERIE_TYPE };
		enum TYAxis{ LEFT_AXIS, RIGHT_AXIS, NB_YAXIS_TYPE };
		enum TFillDirection{ FILL_LEFT, FILL_BOTTOM, NB_FILL_DIRECTION_TYPE };
		enum TSymbol{ stNone = -1, stEllipse, stElli = stEllipse, stRectangle, stRect = stRectangle, stTriangle, stTrgl = stTriangle, NB_SYMBOL_TYPE };
		enum LineStyle{ lsNone = -1, lsSolid, lsDash, lsDot, ltDashDot, lsDashDotDot, NB_LINE_STYLE };
		enum FillStyle{ fsNone = -1, fsSolid, fsHatchDownDiag, fsHatchUpDiag, fsHatchCross, fsHatchDiagCross, fsHatchHorizontal, fsHatchVertical, NB_FILL_STYLE };
		enum THistDirection{ HIST_HORIZONTAL, HIST_VERTICAL, NB_HIST_DIRECTION_TYPE };

		static void ReloadString();
		static std::string GetSerieTypeTitle(size_t i);
		static std::string GetYAxisTypeTitle(size_t i);
		static std::string GetSyboleTypeTitle(int i);
		static std::string GetLineStyleTitle(int i);
		static std::string GetFillStyleTitle(int i);
		static const char* GetFillDirectionTitle(size_t i);
		static const char* GetHistDirectionTitle(size_t i);

		static const char* GetXMLFlag(){ return XML_FLAG; }
		static const char* XML_FLAG;

		CGraphSerie(std::string name = "", int dimension = -1, int var = -1, int statistic = MEAN);
		CGraphSerie(std::string name, int dimension, int var, int statistic, int YAxis, bool bEnableShadow, int shadowDepth, COLORREF shadowColor, int histDirection, int histBarWidth, COLORREF histBarColor, int histBorderWidth, COLORREF histBorderColor);
		CGraphSerie(std::string name, int dimension, int var, int statistic, int YAxis, bool bEnableShadow, int shadowDepth, COLORREF shadowColor, int symbolType, int symbolHeight, int symbolWide, COLORREF symbolColor, bool bSymbolFilled, COLORREF symbolFillColor, int lineStyle, int lineWidth, COLORREF lineColor, bool lineSmoothed, int fillStyle, int fillDirection, COLORREF fillColor);

		void Reset();
		CGraphSerie& operator=(const CGraphSerie& in);
		bool operator==(const CGraphSerie& in)const;
		bool operator!=(const CGraphSerie& in)const{ return !operator==(in); }


		void LoadDefault();
		void SaveDefault();

		//general option
		int m_type; //type of series : 3 type : XY, histogram, CandelStick et ajouter BoxPlot

		std::string m_name;//serie name for edition
		int m_dimension;
		int m_variable;
		int m_statistic;
		int m_YAxis;
		bool m_bEnableShadow;
		int m_shadowDepth;
		COLORREF m_shadowColor;


		//point 
		int m_symbolType;
		int m_symbolHeight;
		int m_symbolWidth;
		COLORREF m_symbolColor;
		bool m_bSymbolFilled;
		COLORREF m_symbolFillColor;


		//line
		int m_lineStyle;
		int m_lineWidth;
		COLORREF m_lineColor;
		bool m_bLineSmoothed;

		//surface
		int m_fillStyle;
		int m_fillDirection;
		COLORREF m_fillColor;

		//histograpm
		int m_histDirection;
		int m_histBarWidth;
		COLORREF m_histBarColor;
		int m_histBorderWidth;
		COLORREF m_histBorderColor;

		//candleStick option
		//????
	protected:

		static StringVector SERIE_TYPE_TITLE;
		static StringVector Y_AXIS_TYPE_TITLE;
		static StringVector SYMBOL_TYPE_TITLE;
		static StringVector LINE_STYLE_TITLE;
		static StringVector FILL_STYLE_TITLE;
		static StringVector FILL_DIRECTION;
		static StringVector HIST_DIRECTION;
	};

	typedef std::vector<CGraphSerie> CGraphSerieVector;


	class CGraph
	{
	public:

		enum{ XY, HISTOGRAM, CANDLE_STICK, BOX_PLOT };
		enum TMember { NAME, TITLE, X_TITLE, Y_TITLE1, Y_TITLE2, X_AXIS, SHOW_LEGEND, SERIES, FIRST_LINE, LAST_LINE, NB_MEMBER };

		static const char* GetMemberName(int i){ ASSERT(i >= 0 && i < NB_MEMBER); return MEMBER_NAME[i]; }
		static const char* GetXMLFlag(){ return XML_FLAG; }

		CGraph();
		CGraph(const CGraph& in);
		virtual ~CGraph();

		void Reset();
		CGraph& operator=(const CGraph& in);
		bool operator==(const CGraph& in)const;
		bool operator!=(const CGraph& in)const{ return !operator==(in); }


		int GetXSerie()const;

		std::string m_name;
		std::string m_title;
		std::string m_Xtitle;
		std::string m_Ytitle1;
		std::string m_Ytitle2;
		CVariableDefine m_XAxis;

		bool m_bShowLegend;
		CGraphSerieVector	m_series;
		int m_firstLine;
		int m_lastLine;


	protected:

		static const char* XML_FLAG;
		static const char* MEMBER_NAME[NB_MEMBER];

	};


	typedef std::vector<CGraph> CGraphVector;

	CGraphVector GetDefaultWeatherGraphVector(bool bHourly);
}





namespace zen
{

	template <> inline
		void writeStruc(const WBSF::CGraphSerie& in, XmlElement& output)
	{
		//General
		output.setAttribute("Name", in.m_name);
		output.setAttribute("Dimension", in.m_dimension);
		output.setAttribute("Variable", in.m_variable);
		output.setAttribute("Statistic", in.m_statistic);
		output.setAttribute("Type", in.m_type);
		output.setAttribute("YAxis", in.m_YAxis);
		output.setAttribute("Shadow", in.m_bEnableShadow);
		output.setAttribute("ShadowDepth", in.m_shadowDepth);
		output.setAttribute("ShadowColor", in.m_shadowColor);

		//points
		output.setAttribute("SymbolColor", in.m_symbolColor);
		output.setAttribute("SymbolType", in.m_symbolType);
		output.setAttribute("SymbolWidth", in.m_symbolWidth);
		output.setAttribute("SymbolHeight", in.m_symbolHeight);
		output.setAttribute("SymbolFilled", in.m_bSymbolFilled);
		output.setAttribute("SymbolFillColor", in.m_symbolFillColor);

		//line
		output.setAttribute("LineStyle", in.m_lineStyle);
		output.setAttribute("LineWidth", in.m_lineWidth);
		output.setAttribute("LineColor", in.m_lineColor);
		output.setAttribute("LineSmoothed", in.m_bLineSmoothed);

		//surface
		output.setAttribute("FillStyle", in.m_fillStyle);
		output.setAttribute("FillHorizontal", in.m_fillDirection);
		output.setAttribute("FillColor", in.m_fillColor);

		//histogram
		output.setAttribute("HistDirection", in.m_histDirection);
		output.setAttribute("HistBarWidth", in.m_histBarWidth);
		output.setAttribute("HistBarColor", in.m_histBarColor);
		output.setAttribute("HistBorderWidth", in.m_histBorderWidth);
		output.setAttribute("HistBorderColor", in.m_histBorderColor);

	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CGraphSerie& out)
	{
		//general
		input.getAttribute("Name", out.m_name);
		input.getAttribute("Dimension", out.m_dimension);
		input.getAttribute("Variable", out.m_variable);
		input.getAttribute("Statistic", out.m_statistic);
		input.getAttribute("Type", out.m_type);
		input.getAttribute("YAxis", out.m_YAxis);
		input.getAttribute("Shadow", out.m_bEnableShadow);
		input.getAttribute("ShadowDepth", out.m_shadowDepth);
		input.getAttribute("ShadowColor", out.m_shadowColor);

		//symbols
		input.getAttribute("SymbolType", out.m_symbolType);
		input.getAttribute("SymbolWidth", out.m_symbolWidth);
		input.getAttribute("SymbolHeight", out.m_symbolHeight);
		input.getAttribute("SymbolColor", out.m_symbolColor);
		input.getAttribute("SymbolFilled", out.m_bSymbolFilled);
		input.getAttribute("SymbolFillColor", out.m_symbolFillColor);
		//lines
		input.getAttribute("LineStyle", out.m_lineStyle);
		input.getAttribute("LineWidth", out.m_lineWidth);
		input.getAttribute("LineColor", out.m_lineColor);
		input.getAttribute("LineSmoothed", out.m_bLineSmoothed);
		//surface
		input.getAttribute("FillStyle", out.m_fillStyle);
		input.getAttribute("FillHorizontal", out.m_fillDirection);
		input.getAttribute("FillColor", out.m_fillColor);
		//histogram
		input.getAttribute("HistDirection", out.m_histDirection);
		input.getAttribute("HistBarWidth", out.m_histBarWidth);
		input.getAttribute("HistBarColor", out.m_histBarColor);
		input.getAttribute("HistBorderWidth", out.m_histBorderWidth);
		input.getAttribute("HistBorderColor", out.m_histBorderColor);

		return true;
	}

	template <> inline
		void writeStruc(const WBSF::CGraphSerieVector& in, XmlElement& output)
	{
		writeStruc2<WBSF::CGraphSerie>(in, output);
	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CGraphSerieVector& out)
	{
		return readStruc2<WBSF::CGraphSerie>(input, out);
	}


	template <> inline
		void writeStruc(const WBSF::CGraph& in, XmlElement& output)
	{
		
		XmlOut out(output);
		out[WBSF::CGraph::GetMemberName(WBSF::CGraph::NAME)](in.m_name);
		out[WBSF::CGraph::GetMemberName(WBSF::CGraph::TITLE)](in.m_title);
		out[WBSF::CGraph::GetMemberName(WBSF::CGraph::X_TITLE)](in.m_Xtitle);
		out[WBSF::CGraph::GetMemberName(WBSF::CGraph::Y_TITLE1)](in.m_Ytitle1);
		out[WBSF::CGraph::GetMemberName(WBSF::CGraph::Y_TITLE2)](in.m_Ytitle2);
		out[WBSF::CGraph::GetMemberName(WBSF::CGraph::X_AXIS)](in.m_XAxis);
		out[WBSF::CGraph::GetMemberName(WBSF::CGraph::SHOW_LEGEND)](in.m_bShowLegend);
		out[WBSF::CGraph::GetMemberName(WBSF::CGraph::SERIES)](in.m_series);
		out[WBSF::CGraph::GetMemberName(WBSF::CGraph::FIRST_LINE)](in.m_firstLine);
		out[WBSF::CGraph::GetMemberName(WBSF::CGraph::LAST_LINE)](in.m_lastLine);
	}

	template <> inline
	bool readStruc(const XmlElement& input, WBSF::CGraph& out)
	{
		XmlIn in(input);
		
		in[WBSF::CGraph::GetMemberName(WBSF::CGraph::NAME)](out.m_name);
		in[WBSF::CGraph::GetMemberName(WBSF::CGraph::TITLE)](out.m_title);
		in[WBSF::CGraph::GetMemberName(WBSF::CGraph::X_TITLE)](out.m_Xtitle);
		in[WBSF::CGraph::GetMemberName(WBSF::CGraph::Y_TITLE1)](out.m_Ytitle1);
		in[WBSF::CGraph::GetMemberName(WBSF::CGraph::Y_TITLE2)](out.m_Ytitle2);
		in[WBSF::CGraph::GetMemberName(WBSF::CGraph::X_AXIS)](out.m_XAxis);
		in[WBSF::CGraph::GetMemberName(WBSF::CGraph::SHOW_LEGEND)](out.m_bShowLegend);
		in[WBSF::CGraph::GetMemberName(WBSF::CGraph::SERIES)](out.m_series);
		in[WBSF::CGraph::GetMemberName(WBSF::CGraph::FIRST_LINE)](out.m_firstLine);
		in[WBSF::CGraph::GetMemberName(WBSF::CGraph::LAST_LINE)](out.m_lastLine);


		return true;
	}
}


