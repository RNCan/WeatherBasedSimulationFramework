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
// CLocationVectorCtrl 

#include <boost\dynamic_bitset.hpp>
#include "Basic/Location.h"
#include "UI/Common/CommonCtrl.h"
#include "UltimateGrid/UGCtrl.h"
#include "UltimateGrid/CellTypes\UGCTsarw.h" 


namespace WBSF
{

	class CWGInput;


	class CLocationVectorCtrl : public CUGCtrl
	{
	public:

		static const WORD UWM_SELECTION_CHANGE = (WM_USER + 2); // a custom Windows message


		CLocationVectorCtrl();


		void Update();
		void EnableList(BOOL bEnable);

		virtual void OnSetup();

		virtual void OnCellChange(int oldcol, int newcol, long oldrow, long newrow);
		virtual int OnCanSizeCol(int) { return TRUE; }
		virtual int OnCanSizeRow(long) { return FALSE; }
		virtual int OnCanSizeTopHdg() { return FALSE; }
		virtual int OnCanSizeSideHdg() { return TRUE; }
		virtual void OnGetCell(int col, long row, CUGCell *cell);
		virtual void OnTH_LClicked(int col, long row, int updn, RECT *rect, POINT *point, BOOL processed = 0);
		virtual void OnCB_LClicked(int updn, RECT *rect, POINT *point, BOOL processed = 0);
		virtual void OnSH_LClicked(int col, long row, int updn, RECT *rect, POINT *point, BOOL processed = 0);
		virtual void OnColSized(int col, int *width);
		virtual int OnHint(int col, long row, int section, CString *string);
		virtual int OnVScrollHint(long row, CString *string);
		virtual COLORREF OnGetDefBackColor(int section);





		//CWVariables GetVariables(){ return m_variables; }
		void SetLocationVector(CLocationVectorPtr pLocations){ m_pLocations = pLocations; }
		void	SetCurIndex(size_t in);
		size_t	GetCurIndex()const;

		void SetStationModified(size_t i, bool bModified){ bModified ? m_stationModified.set(i) : m_stationModified.reset(i); }
		int RedrawRow(long index);
		void SetEditionMode(bool bInEdition);

		inline int OtherDir(int dir){ return (dir == UGCT_SORTARROWUP) ? UGCT_SORTARROWDOWN : UGCT_SORTARROWUP; }

	protected:

		CFont m_font;
		CFont m_fontBold;
		CPen m_cellBorderPen;

		std::string		m_lastFilePath;
		size_t			m_lastStationIndex;

		//locations
		CLocationVectorPtr m_pLocations;
		boost::dynamic_bitset<size_t> m_selection;


		//edition
		void CreateBoldFont();
		boost::dynamic_bitset<size_t> m_stationModified;
		bool			m_bInEdition;


		//sort
		void SortInfo(int col, int dir);
		std::vector<std::pair<std::string, size_t>> m_sortInfo;
		CUGSortArrowType m_sortArrow;
		int m_curSortCol;
		int m_sortDir;

		DECLARE_MESSAGE_MAP()
	};

}