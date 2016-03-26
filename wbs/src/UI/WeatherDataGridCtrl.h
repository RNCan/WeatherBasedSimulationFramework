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

#include <boost\dynamic_bitset.hpp>
#include "basic/ERMsg.h"
#include "UI/Common/UGEditCtrl.h"
#include "Basic/WeatherStation.h"


namespace WBSF
{

	/////////////////////////////////////////////////////////////////////////////
	// CDailyDlg dialog



	class CWeatherDataGridCtrl : public CUGEditCtrl
	{
	public:

		//properties
		CTM		m_TM;
		size_t	m_stat;
		bool	m_bEditable;
		bool	m_bPeriodEnabled;
		CTPeriod m_period;
		CWVariables m_variables;

		CWeatherStationPtr  m_pStation;

		CWeatherDataGridCtrl()
		{
			m_TM = CTM(CTM::HOURLY);
			m_stat = WBSF::MEAN;
			m_bEditable = false;
			m_bPeriodEnabled = false;

			m_lastTM = CTM(CTM::HOURLY);
			m_lastStat = WBSF::MEAN;
			m_bLastEditable = false;
			m_bModified = false;
		}


		void Update();


		virtual void OnSetup();
		virtual void OnGetCell(int col, long row, CUGCell *cell);
		virtual void OnSetCell(int col, long row, CUGCell *cell);
		virtual void OnColSized(int col, int *width);
		virtual int  OnSideHdgSized(int *width);
		virtual int OnHint(int col, long row, int section, CString *string);
		virtual int OnVScrollHint(long row, CString *string);
		virtual COLORREF OnGetDefBackColor(int section);
		virtual int OnMenuStart(int col, long row, int section);
		virtual void OnMenuCommand(int col, long row, int section, int item);
		virtual int OnCanSizeSideHdg() { return TRUE; }
		virtual int OnCanSizeCol(int col) { UNREFERENCED_PARAMETER(col); return TRUE; }
		virtual int OnEditStart(int col, long row, CWnd **edit);

		virtual BOOL PreTranslateMessage(MSG* pMsg);

		CTRef GetTRef(long row);
		static void ReloadString();

		bool IsModified()const{ return m_bModified; }
		void SetIsModified(bool in = false){ m_bModified = in; }
	protected:


		CFont			m_font;
		CFont			m_fontBold;
		CPen			m_cellBorderPen;

		CTM				m_lastTM;
		size_t			m_lastStat;
		bool			m_bLastEditable;
		std::string		m_lastID;
		bool			m_bLastPeriodEnabled;
		CTPeriod		m_lastPeriod;
		CWVariables		m_lastVariables;

		std::vector<size_t> m_colMap;
		std::string m_filePath;
		bool m_bModified;

		std::vector<boost::dynamic_bitset<size_t>> m_bDataEdited;


		void CreateBoldFont();
		static WBSF::StringVector VARIABLES_TOOLTIPS;

	};
}