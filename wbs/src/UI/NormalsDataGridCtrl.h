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

/////////////////////////////////////////////////////////////////////////////

#include <boost\dynamic_bitset.hpp>
#include "basic/ERMsg.h"
#include "Basic/NormalsStation.h"
#include "UI/Common/UGEditCtrl.h"



namespace WBSF
{

	class CNormalsDataGridCtrl : public CUGEditCtrl
	{
	public:

		//properties
		bool	m_bEditable;
		CWVariables m_variables;

		CNormalsStationPtr  m_pStation;

		CNormalsDataGridCtrl()
		{
			m_bEditable = false;
			m_bLastEditable = false;
			m_bModified = false;
		}


		void Update();


		virtual void OnSetup();
		virtual void OnGetCell(int col, long row, CUGCell *cell);
		virtual void OnSetCell(int col, long row, CUGCell *cell);
		virtual void OnColSized(int col, int *width);
		virtual int  OnSideHdgSized(int *width);
		virtual COLORREF OnGetDefBackColor(int section);
		virtual int OnMenuStart(int col, long row, int section);
		virtual void OnMenuCommand(int col, long row, int section, int item);
		virtual int OnEditStart(int col, long row, CWnd **edit);
		virtual int OnCanSizeSideHdg() { return TRUE; }
		virtual int OnCanSizeCol(int col) { UNREFERENCED_PARAMETER(col); return TRUE; }


		virtual BOOL PreTranslateMessage(MSG* pMsg);

		CTRef GetTRef(long row);
		static void ReloadString();

		bool IsModified()const{ return m_bModified; }
		void SetIsModified(bool in = false){ m_bModified = in; }
	protected:


		CFont			m_font;
		CFont			m_fontBold;
		CPen			m_cellBorderPen;

		bool			m_bLastEditable;
		std::string		m_lastID;
		CWVariables		m_lastVariables;

		std::string m_filePath;
		bool m_bModified;

		std::vector<boost::dynamic_bitset<size_t>> m_bDataEdited;

		void CreateBoldFont();
		static WBSF::StringVector VARIABLES_TOOLTIPS;

	};
}