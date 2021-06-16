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

#include "Basic/Registry.h"
#include "Basic/UtilStd.h"
#include "ModelBase/WGInput.h"
#include "UI/Common/SYShowMessage.h"
#include "UI/Common/UtilWin.h"

#include "LocationVectorCtrl.h"
#include "WeatherBasedSimulationString.h"

using namespace std;



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace WBSF
{


	enum { C_ID, C_NAME, C_LATITUDE, C_LONGITUDE, C_ALTITUDE, C_SHORE_DISTANCE, NB_LOCATIONS_COLUMNS};

	//*******************************************************************************************************

	BEGIN_MESSAGE_MAP(CLocationVectorCtrl, CUGCtrl)
	END_MESSAGE_MAP()

	CLocationVectorCtrl::CLocationVectorCtrl(void)
	{
		m_curSortCol = -999;
		m_sortDir = UGCT_SORTARROWUP;

		m_bInEdition = false;
		//m_lastStationIndex=-1;
	}


	void CLocationVectorCtrl::CreateBoldFont()
	{
		if (m_fontBold.GetSafeHandle() != NULL)
		{
			m_fontBold.DeleteObject();
		}

		CFont* pFont = &m_font;
		ASSERT_VALID(pFont);

		LOGFONT lf;
		memset(&lf, 0, sizeof(LOGFONT));

		pFont->GetLogFont(&lf);

		lf.lfWeight = FW_BOLD;
		m_fontBold.CreateFontIndirect(&lf);
	}

	void CLocationVectorCtrl::OnSetup()
	{
		SetDefRowHeight(MulDiv(m_GI->m_defRowHeight, UtilWin::GetWindowDPI(GetSafeHwnd()), 96));
		m_font.CreateStockObject(DEFAULT_GUI_FONT);
	

		CreateBoldFont();
		m_cellBorderPen.CreatePen(PS_SOLID, 1, RGB(157, 157, 161));


		m_CUGHint->SetFont(&m_font);
		m_CUGTab->ShowScrollbars(true);

		// add and set heading's default celltype
		CUGCell cell;
		GetHeadingDefault(&cell);
		cell.SetAlignment(UG_ALIGNCENTER);
		cell.SetBorder(UG_BDR_RTHIN | UG_BDR_BTHIN);
		cell.SetBackColor(RGB(239, 237, 242));
		cell.SetHBackColor(RGB(162, 192, 248));

		cell.SetBorderColor(&m_cellBorderPen);
		cell.SetFont(&m_font);
		SetHeadingDefault(&cell);

		// create a font and set it as Grid Default
		GetGridDefault(&cell);
		cell.SetAlignment(UG_ALIGNCENTER);
		cell.SetFont(&m_font);
		cell.SetBorderColor(&m_cellBorderPen);
		cell.SetBorder(UG_BDR_RTHIN | UG_BDR_BTHIN);
		SetGridDefault(&cell);
		// create a font and set it as Heading Default

		// set default properties
		UseHints(FALSE);
		UseVScrollHints(TRUE);

		EnableExcelBorders(TRUE);
		SetHighlightRow(UG_MULTISELECT_ROW, FALSE);
		SetMultiSelectMode(UG_MULTISELECT_OFF);
		SetCurrentCellMode(3);	//focus and highlighting
		SetDoubleBufferMode(TRUE);
		SetVScrollMode(UG_SCROLLTRACKING);
		SetHScrollMode(UG_SCROLLTRACKING);

		AddCellType(&m_sortArrow);


	}


	class FindByIndex
	{
	public:
		FindByIndex(size_t index) :
			m_index(index)
		{}

		bool operator ()(const std::pair<std::string, size_t>& in)const{ return in.second == m_index; }

	protected:
		size_t m_index;
	};

	void CLocationVectorCtrl::Update()
	{
		string filePath;
		if (m_pLocations.get())
			filePath = m_pLocations->GetFilePath();

		if (filePath != m_lastFilePath)
		{
			m_stationModified.clear();
			m_curSortCol = -999;
			m_sortDir = UGCT_SORTARROWUP;
			m_lastFilePath = filePath;

			if (!filePath.empty())
			{
				m_enableUpdate = FALSE;
				m_stationModified.resize(m_pLocations->size());
				SortInfo(AfxGetApp()->GetProfileInt(_T("LocationVectorCtrl"), _T("SortCol"), 0), AfxGetApp()->GetProfileInt(_T("LocationVectorCtrl"), _T("SortDir"), 0));
				
				SetNumberCols(NB_LOCATIONS_COLUMNS, FALSE);
				SetNumberRows((long)m_sortInfo.size(), FALSE);

				for (int i = -1; i < NB_LOCATIONS_COLUMNS; i++)
				{
					int width = AfxGetApp()->GetProfileInt(_T("LocationVectorCtrl"), _T("ColWidth ") + UtilWin::ToCString(i), 50);
					SetColWidth(i, width);
				}
				
				//if (newRow >= m_sortInfo.size())
				//Select(-1, -1);
//				else GotoRow((long)newRow);

				m_enableUpdate = TRUE;

			}//is open
			else
			{
				SetNumberRows(0, FALSE);
				SetNumberCols(0, FALSE);
				m_sortInfo.clear();
			}

			Invalidate();


			//reload station index if they are change
			//long newRow = long((GetCurrentRow() == -1) ? -1 : m_sortInfo[GetCurrentRow()].second);
			//if (GetCurrentRow() != m_lastStationIndex)
			GetParent()->SendMessage(UWM_SELECTION_CHANGE);
		}

	}

	void CLocationVectorCtrl::SetEditionMode(bool bInEdition)
	{
		if (bInEdition != m_bInEdition)
		{
			m_bInEdition = bInEdition;

			EnableWindow(!m_bInEdition);
			m_stationModified.reset();

			RedrawRow(GetCurrentRow());
		}
	}



	void CLocationVectorCtrl::OnGetCell(int col, long row, CUGCell *cell)
	{
		//row >= -1 && col >= -1 &&
		if ( m_enableUpdate)
		{
			string text;
			COLORREF backColor = cell->GetBackColor();
			COLORREF textColor = cell->GetTextColor();
			CFont* pFont = cell->GetFont();

			if (col == -1)
			{
				if (row == -1)
					text = "No";
				else
					text = ToString(m_sortInfo[row].second + 1);
			}
			else
			{
				if (row == -1)
				{
					
					if (col < C_SHORE_DISTANCE)
						text = CLocation::GetMemberTitle(col);
					else
						text = GetString(IDS_STR_SHORE_DISTANCE);
				}
				else
				{

					if (m_pLocations.get())
					{
						size_t index = m_sortInfo[row].second;
						const CLocation& location = m_pLocations->at(index);

						textColor = m_bInEdition ? RGB(100, 100, 100) : RGB(0, 0, 0);
						backColor = m_bInEdition ? ((row % 2) ? RGB(255, 235, 235) : RGB(250, 250, 250)) : ((row % 2) ? RGB(235, 235, 255) : RGB(255, 255, 255));

						assert(index < m_stationModified.size());
						if (m_stationModified[index] && !m_bInEdition)
						{
							textColor = RGB(50, 90, 230);
							pFont = &m_fontBold;
						}

						if (!location.IsValid())
						{
							backColor = RGB(255, 50, 40);
							textColor = RGB(255, 255, 255);
						}

						if (col < C_SHORE_DISTANCE)
							text = location.GetMember(col);
						else
							text = ToString(location.GetShoreDistance()/1000, 1);
					}
				}
			}


			if (row == -1 && col == m_curSortCol)
			{   // set default values to the top heading   
				cell->SetCellType(m_sortArrow.GetID());
				cell->SetCellTypeEx(m_sortDir);
			}

			cell->SetBackColor(backColor);
			cell->SetTextColor(textColor);
			cell->SetText(CString(text.c_str()));
			cell->SetFont(pFont);
		}//enable update

		CUGCtrl::OnGetCell(col, row, cell);
	}


	/***************************************************
	OnCellChange
	Sent whenever the current cell changes
	Params:
	oldcol, oldrow - coordinates of cell that is loosing the focus
	newcol, newrow - coordinates of cell that is gaining the focus
	Return:
	<none>
	****************************************************/
	void CLocationVectorCtrl::OnCellChange(int oldcol, int newcol, long oldrow, long newrow)
	{
		ASSERT(newrow >= -1 && newrow < GetNumberRows());
		if (m_enableUpdate)
			GetParent()->SendMessage(UWM_SELECTION_CHANGE);
	}

	/////////////////////////////////////////////////////////////////////////////
	//	OnGetDefBackColor
	//		Sent when the area behind the grid needs to be paindted.
	//	Params:
	//		section - Id of the grid section that requested this color
	//				  possible sections:
	//						UG_TOPHEADING, UG_SIDEHEADING, UG_GRID
	//	Return:
	//		RGB value representing the color of choice
	COLORREF CLocationVectorCtrl::OnGetDefBackColor(int section)
	{
		UNREFERENCED_PARAMETER(section);

		return GetSysColor(COLOR_APPWORKSPACE);
	}



	int CLocationVectorCtrl::OnHint(int col, long row, int section, CString *string)
	{
		if (m_pLocations)
		{
			if (section == UG_GRID)
			{
				if (col >= 0 && row >= 0)
				{
					if (col < C_SHORE_DISTANCE)
						*string = (*m_pLocations)[row].GetMember(col).c_str();
					else
						*string = UtilWin::ToCString((*m_pLocations)[row].GetShoreDistance()/1000,1);

				}
			}
		}


		return !string->IsEmpty();
	}

	int CLocationVectorCtrl::OnVScrollHint(long row, CString *string)
	{
		ASSERT(m_pLocations.get());

		size_t curRow = row == -1 ? -1 : m_sortInfo[row].second;
		*string = (*m_pLocations)[curRow].m_name.c_str();

		return  !string->IsEmpty();
	}


	//inline int OtherDir(int dir){ return (dir == UGCT_SORTARROWUP) ? UGCT_SORTARROWDOWN : UGCT_SORTARROWUP; }
	/////////////////////////////////////////////////////////////////////////////   
	//  OnTH_LClicked   
	//      Sent whenever the user clicks the left mouse button within the top heading   
	//      this message is sent when the button goes down then again when the button goes up   
	//  Params:   
	//      col, row    - coordinates of a cell that received mouse click event   
	//      updn        - is TRUE if mouse button is 'down' and FALSE if button is 'up'   
	//      processed   - indicates if current event was processed by other control in the grid.   
	//      rect        - represents the CDC rectangle of cell in question   
	//      point       - represents the screen point where the mouse event was detected   
	//  Return:   
	//      <none>   
	void CLocationVectorCtrl::OnTH_LClicked(int col, long row, int updn, RECT *rect, POINT *point, BOOL processed)
	{
		UNREFERENCED_PARAMETER(row);
		UNREFERENCED_PARAMETER(*rect);
		UNREFERENCED_PARAMETER(*point);
		UNREFERENCED_PARAMETER(processed);

		if (!updn)
			return;

		BeginWaitCursor();

		row = GetCurrentRow();
		size_t index = row >= 0 ? m_sortInfo[row].second : NOT_INIT;
		SortInfo(col, col != m_curSortCol ? m_sortDir : OtherDir(m_sortDir));

		size_t newRow = std::distance(m_sortInfo.begin(), std::find_if(m_sortInfo.begin(), m_sortInfo.end(), FindByIndex(index)));
		if (newRow < m_sortInfo.size())
			GotoRow((long)newRow);
		else
			Select(-1, -1);
		//if ( >= m_sortInfo.size())
			//Select(-1, -1);//force changing

		RedrawAll();
		EndWaitCursor();

		
		

		
		
	}

	void CLocationVectorCtrl::OnCB_LClicked(int updn, RECT *rect, POINT *point, BOOL processed)
	{
		UNREFERENCED_PARAMETER(*rect);
		UNREFERENCED_PARAMETER(*point);
		UNREFERENCED_PARAMETER(processed);

		if (!updn)
			return;

		BeginWaitCursor();

		int row = GetCurrentRow();
		size_t index = row >= 0 ? m_sortInfo[row].second : NOT_INIT;
		SortInfo(-1, m_curSortCol != -1 ? m_sortDir : OtherDir(m_sortDir));



		size_t newRow = std::distance(m_sortInfo.begin(), std::find_if(m_sortInfo.begin(), m_sortInfo.end(), FindByIndex(index)));
		if (newRow < m_sortInfo.size())
			GotoRow((long)newRow);
		else 
			Select(-1, -1); 
		


		RedrawAll();
		EndWaitCursor();

	}
	


	void CLocationVectorCtrl::SortInfo(int col, int dir)
	{
		ASSERT(m_pLocations);

		if (m_pLocations)
		{
			if (col != m_curSortCol)
			{
				m_curSortCol = col;
				m_sortInfo.clear();

				m_sortInfo.resize(m_pLocations->size());
				for (CLocationVector::const_iterator it = m_pLocations->begin(); it != m_pLocations->end(); it++)
				{
					size_t pos = std::distance(m_pLocations->cbegin(), it);
					const CLocation& loc = *it;

					string str;

					switch (m_curSortCol)
					{
					case -1:   str = to_string(pos + 1); break;
					case C_ID:   str = loc.m_ID; break;
					case C_NAME: str = loc.m_name; break;
					case C_LATITUDE:  str = FormatA("%015.6lf", loc.m_lat); break;
					case C_LONGITUDE:  str = FormatA("%015.6lf", loc.m_lon); break;
					case C_ALTITUDE: str = FormatA("%010.1lf", loc.m_elev); break;
					case C_SHORE_DISTANCE: str = FormatA("%010.1lf", loc.GetShoreDistance()/1000); break;
					default: ASSERT(false);
					}

					m_sortInfo[pos] = std::make_pair(str, pos);
				}


				if (m_curSortCol == CLocation::ID || m_curSortCol == CLocation::NAME)
					std::sort(m_sortInfo.begin(), m_sortInfo.end(), CCompareString());
				else
					std::sort(m_sortInfo.begin(), m_sortInfo.end(), CompareNumber);

				m_sortDir = UGCT_SORTARROWUP;
				//}
			}

			if (dir != m_sortDir)
			{
				std::reverse(m_sortInfo.begin(), m_sortInfo.end());
				m_sortDir = dir;
			}

			AfxGetApp()->WriteProfileInt(_T("LocationVectorCtrl"), _T("SortCol"), m_curSortCol);
			AfxGetApp()->WriteProfileInt(_T("LocationVectorCtrl"), _T("SortDir"), m_sortDir);
		}
		else
		{
			m_curSortCol = -999;
			m_sortDir = UGCT_SORTARROWUP;
		}
	}

	void CLocationVectorCtrl::OnColSized(int col, int *width)
	{
		AfxGetApp()->WriteProfileInt(_T("LocationVectorCtrl"), _T("ColWidth ") + UtilWin::ToCString(col), *width);
	}

	//int CLocationVectorCtrl::RedrawRow(long index)
	//{
	//	long row = (long)std::distance(m_sortInfo.begin(), std::find_if(m_sortInfo.begin(), m_sortInfo.end(), FindByIndex(index)));
	//	return CUGCtrl::RedrawRow(row);
	//}

	void	CLocationVectorCtrl::SetCurIndex(size_t index)
	{
		long row = (long)std::distance(m_sortInfo.begin(), std::find_if(m_sortInfo.begin(), m_sortInfo.end(), FindByIndex(index)));
		GotoRow(row);
		CUGCtrl::RedrawRow(row);
	}

	size_t	CLocationVectorCtrl::GetCurIndex()const
	{
		long row = const_cast<CLocationVectorCtrl*>(this)->GetCurrentRow();
		size_t newPos = row >= 0 ? m_sortInfo[row].second : UNKNOWN_POS;
		return newPos;
	}

}