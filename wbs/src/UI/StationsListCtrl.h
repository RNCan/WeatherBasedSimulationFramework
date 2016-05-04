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
#include <map>
#include <boost/dynamic_bitset.hpp>
#include "UltimateGrid/UGCtrl.h"
#include "UltimateGrid/CellTypes/UGCTsarw.h" 
#include "Basic/WeatherDatabase.h"
#include "UI/Common/CommonCtrl.h"




namespace WBSF
{


	//*************************************************************************************************************

	class CStationsListCtrl : public CUGCtrl
	{
	public:

		static const WORD UWM_SELECTION_CHANGE = (WM_USER + 2); // a custom Windows message


		static double GetCompleteness(const CWVariablesCounter& counter);


		CWeatherDatabasePtr m_pDB;


		std::set<int>	m_years;
		CWVariables		m_filter;
		size_t			m_initial_index;
		//	boost::dynamic_bitset<size_t> m_selection;//specific station selection;
		//bool			m_bStationModified;

		CStationsListCtrl();


		void Update();
		void EnableStationsList(BOOL bEnable);

		virtual void OnSetup();


		virtual int OnCanSizeCol(int) { return TRUE; }
		virtual int OnCanSizeRow(long) { return FALSE; }
		virtual int OnCanSizeTopHdg() { return FALSE; }
		virtual int OnCanSizeSideHdg() { return TRUE; }
		virtual void OnCellChange(int oldcol, int newcol, long oldrow, long newrow);
		virtual void OnGetCell(int col, long row, CUGCell *cell);
		virtual void OnTH_LClicked(int col, long row, int updn, RECT *rect, POINT *point, BOOL processed = 0);
		virtual void OnCB_LClicked(int updn, RECT *rect, POINT *point, BOOL processed = 0);
		virtual void OnSH_LClicked(int col, long row, int updn, RECT *rect, POINT *point, BOOL processed = 0);
		virtual void OnColSized(int col, int *width);
		virtual int OnHint(int col, long row, int section, CString *string);
		virtual int OnVScrollHint(long row, CString *string);
		virtual COLORREF OnGetDefBackColor(int section);





		//CWVariables GetVariables(){ return m_variables; }
		void	SetStationIndex(size_t in);
		size_t	GetStationIndex()const;

		void SetStationModified(size_t i, bool bModified){ bModified ? m_stationModified.set(i) : m_stationModified.reset(i); }
		//int RedrawRow(long index);
		void SetEditionMode(bool bInEdition);

		inline int OtherDir(int dir){ return (dir == UGCT_SORTARROWUP) ? UGCT_SORTARROWDOWN : UGCT_SORTARROWUP; }
	protected:

		CFont m_font;
		CFont m_fontBold;
		CPen m_cellBorderPen;

		std::string		m_lastFilePath;
		//size_t			m_lastStationIndex;
		std::set<int>	m_lastYears;
		CWVariables		m_lastFilter;

		void CreateBoldFont();
		void SortInfo(int col, int dir);
		std::vector<std::pair<std::string, size_t>> m_sortInfo;
		CUGSortArrowType m_sortArrow;
		int m_curSortCol;
		int m_sortDir;
		boost::dynamic_bitset<size_t> m_stationModified;
		bool			m_bInEdition;

		DECLARE_MESSAGE_MAP()
	};

	//*************************************************************************************************************

	class CMatchStationsCtrl : public CUGCtrl
	{
	public:

		static const WORD UWM_SELECTION_CHANGE = (WM_USER + 2); // a custom Windows message

		CWeatherDatabasePtr m_pDB;
		CLocation			m_location;
		CSearchResultVector	m_nearest;

		//	size_t	m_nbStations;
		//int		m_year;
		//size_t	m_variable;
		//bool		m_bXVal;

		CMatchStationsCtrl();


		void Update();
		void EnableStationsList(BOOL bEnable);

		virtual void OnSetup();


		virtual int OnCanSizeCol(int) { return TRUE; }
		virtual int OnCanSizeRow(long) { return FALSE; }
		virtual int OnCanSizeTopHdg() { return FALSE; }
		virtual int OnCanSizeSideHdg() { return TRUE; }
		virtual void OnCellChange(int oldcol, int newcol, long oldrow, long newrow);
		virtual void OnGetCell(int col, long row, CUGCell *cell);
		virtual void OnTH_LClicked(int col, long row, int updn, RECT *rect, POINT *point, BOOL processed = 0);
		virtual void OnCB_LClicked(int updn, RECT *rect, POINT *point, BOOL processed = 0);
		virtual void OnSH_LClicked(int col, long row, int updn, RECT *rect, POINT *point, BOOL processed = 0);


		virtual void OnDClicked(int col, long row, RECT *rect, POINT *point, BOOL processed);
		virtual void OnSH_DClicked(int col, long row, RECT *rect, POINT *point, BOOL processed = 0);




		virtual void OnColSized(int col, int *width);
		virtual int OnHint(int col, long row, int section, CString *string);
		virtual int OnVScrollHint(long row, CString *string);
		virtual COLORREF OnGetDefBackColor(int section);

		//CWVariables GetVariables(){ return m_variables; }
		void	SetStationIndex(size_t in);
		size_t	GetStationIndex()const;


		//int RedrawRow(long index);
		inline int OtherDir(int dir){ return (dir == UGCT_SORTARROWUP) ? UGCT_SORTARROWDOWN : UGCT_SORTARROWUP; }

	protected:

		CFont m_font;
		CFont m_fontBold;
		CPen m_cellBorderPen;

		std::string		m_lastFilePath;
		CLocation		m_lastLocation;
		CSearchResultVector	m_lastNearest;
		//int				m_lastYear;
		//size_t			m_lastVariable;
		//size_t			m_lastNbStations;
		//std::array<std::vector<double>, HOURLY_DATA::NB_VAR_H> m_weight;
		std::vector<double> m_shoreD;
		std::vector<double> m_deltaShore;
		std::vector<double> m_weight;

		void CreateBoldFont();
		void SortInfo(int col, int dir);
		std::vector<std::pair<std::string, size_t>> m_sortInfo;
		CUGSortArrowType m_sortArrow;
		int m_curSortCol;
		int m_sortDir;


		DECLARE_MESSAGE_MAP()
	};

}