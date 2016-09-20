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



#include "UltimateGrid/ExcelTopHdg.h"
#include "UltimateGrid/ExcelSideHdg.h"
#include "Basic/Registry.h"
#include "UI/Common/SYShowMessage.h"
#include "UI/Common/UtilWin.h"
#include "Simulation/WeatherGradient.h"
#include "StationsListCtrl.h"

#include "WeatherBasedSimulationString.h"


using namespace std;
using namespace WBSF::HOURLY_DATA;


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

namespace WBSF
{
	enum { NB_TOTAL_VARIABLES = CLocation::SITE_SPECIFIC_INFORMATION, NB_YEARS, COMPLETENESS, NB_STATIONS_COLUMNS };

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

	//*******************************************************************************************************

	BEGIN_MESSAGE_MAP(CStationsListCtrl, CUGCtrl)
	END_MESSAGE_MAP()

	CStationsListCtrl::CStationsListCtrl(void)
	{
		m_curSortCol = -999;
		m_sortDir = UGCT_SORTARROWUP;
		m_initial_index = NOT_INIT;

		m_bInEdition = false;
	}


	void CStationsListCtrl::CreateBoldFont()
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

	void CStationsListCtrl::OnSetup()
	{

		m_font.CreateStockObject(DEFAULT_GUI_FONT);
		CreateBoldFont();
		m_cellBorderPen.CreatePen(PS_SOLID, 1, RGB(157, 157, 161));


		m_CUGHint->SetFont(&m_font);
		m_CUGTab->ShowScrollbars(true);

		// add and set heading's default celltype
		CUGCell cell;
		GetHeadingDefault(&cell);
		cell.SetAlignment(UG_ALIGNLEFT);
		cell.SetBorder(UG_BDR_RTHIN | UG_BDR_BTHIN);
		cell.SetBackColor(RGB(239, 237, 242));
		cell.SetHBackColor(RGB(162, 192, 248));

		cell.SetBorderColor(&m_cellBorderPen);
		cell.SetFont(&m_font);
		SetHeadingDefault(&cell);

		// create a font and set it as Grid Default
		GetGridDefault(&cell);
		cell.SetAlignment(UG_ALIGNLEFT);
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



	void CStationsListCtrl::Update()
	{
		if (m_pDB->GetFilePath() != m_lastFilePath || m_years != m_lastYears || m_filter != m_lastFilter)
		{
			m_stationModified.clear();
			m_curSortCol = -999;
			m_sortDir = UGCT_SORTARROWUP;
			m_lastFilePath = m_pDB->GetFilePath();

			if (m_pDB && m_pDB->IsOpen())
			{
				m_enableUpdate = FALSE;

				m_lastYears = m_years;
				m_lastFilter = m_filter;

				m_stationModified.resize(m_pDB->size());
				SortInfo(AfxGetApp()->GetProfileInt(_T("StationsListCtrl"), _T("SortCol"), 0), AfxGetApp()->GetProfileInt(_T("StationsListCtrl"), _T("SortDir"), 0));

				SetTH_NumberRows(1);
				SetSH_NumberCols(1);
				SetNumberCols(NB_STATIONS_COLUMNS, FALSE);
				SetNumberRows((long)m_sortInfo.size(), FALSE);

				for (int i = -1; i < NB_STATIONS_COLUMNS; i++)
				{
					int width = AfxGetApp()->GetProfileInt(_T("StationsListCtrl"), _T("ColWidth ") + UtilWin::ToCString(i), 50);
					SetColWidth(i, width);
				}

				if (m_initial_index < m_sortInfo.size())
				{
					long row = std::distance(m_sortInfo.begin(), std::find_if(m_sortInfo.begin(), m_sortInfo.end(), FindByIndex(m_initial_index)));
					GotoRow(row);
				}
				else
				{
					//Select(-1, -1);
				}

				m_enableUpdate = TRUE;

			}//is open
			else
			{
				SetTH_NumberRows(0);
				SetSH_NumberCols(0);
				SetNumberRows(0, FALSE);
				SetNumberCols(0, FALSE);
				m_sortInfo.clear();
			}

			Invalidate();

			//GetParent()->SendMessage(UWM_SELECTION_CHANGE);
		}

	}

	void CStationsListCtrl::SetEditionMode(bool bInEdition)
	{
		if (bInEdition != m_bInEdition)
		{
			m_bInEdition = bInEdition;

			EnableWindow(!m_bInEdition);
			m_stationModified.reset();

			RedrawRow(GetCurrentRow());
		}
	}



	double CStationsListCtrl::GetCompleteness(const CWVariablesCounter& counter)
	{
		WBSF::CStatistic stat;

		CWVariables vars = counter.GetVariables();
		for (size_t i = 0; i < counter.size(); i++)
		{
			if (vars[i])
			{
				CTPeriod p = counter[i].second;
				p.Begin().m_hour = WBSF::FIRST_HOUR;
				p.Begin().m_day = WBSF::FIRST_DAY;
				p.Begin().m_month = WBSF::FIRST_MONTH;

				p.End().m_hour = WBSF::LAST_HOUR;
				p.End().m_day = WBSF::LAST_DAY;
				p.End().m_month = WBSF::LAST_MONTH;


				double completeness = 0;
				if (counter[i].second.IsInit())
					completeness = 100.0 * counter[i].first / p.size();

				assert(completeness >= 0 && completeness <= 100);
				stat += completeness;
			}
		}

		return stat[WBSF::MEAN];
	}


	void CStationsListCtrl::OnGetCell(int col, long row, CUGCell *cell)
	{

		if (m_enableUpdate)
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
					if (col == NB_TOTAL_VARIABLES)
						text = GetString(IDS_STR_NB_VARIABLES);
					else if (col == NB_YEARS)
						text = GetString(IDS_STR_YEARS);
					else if (col == COMPLETENESS)
						text = GetString(IDS_STR_COMPLETENESS);
					else
						text = CLocation::GetMemberTitle(col);
				}
				else
				{

					if (m_pDB && m_pDB->IsOpen())
					{
						size_t index = m_sortInfo[row].second;
						const CLocation& location = m_pDB->GetLocation(index);

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

						if (col == NB_TOTAL_VARIABLES)
						{
							CWVariables vars = m_pDB->GetWVariables(index);
							text = ToString(vars.count());
						}
						else if (col == NB_YEARS)
						{
							std::set<int> years = m_pDB->GetYears(index);
							text = ToString(years.size());
						}
						else if (col == COMPLETENESS)
						{
							double completenedd = GetCompleteness(m_pDB->GetWVariablesCounter(index));
							text = ToString(completenedd, 1);
						}
						else
						{
							text = location.GetMember(col);
						}
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
		}

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
	void CStationsListCtrl::OnCellChange(int oldcol, int newcol, long oldrow, long newrow)
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
	COLORREF CStationsListCtrl::OnGetDefBackColor(int section)
	{
		UNREFERENCED_PARAMETER(section);

		return GetSysColor(COLOR_APPWORKSPACE);
	}



	int CStationsListCtrl::OnHint(int col, long row, int section, CString *string)
	{
		if (m_pDB)
		{
			if (section == UG_GRID)
			{
				if (col >= 0 && row >= 0)
				{
					size_t curRow = m_sortInfo[row].second;
					*string = (*m_pDB)[curRow].GetMember(col).c_str();
				}
			}
		}


		return !string->IsEmpty();
	}

	int CStationsListCtrl::OnVScrollHint(long row, CString *string)
	{
		ASSERT(m_pDB.get());

		if (row>=0)
		{
			size_t curRow = m_sortInfo[row].second;
			*string = (*m_pDB)[curRow].m_name.c_str();
		}

		return  !string->IsEmpty();
	}


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
	void CStationsListCtrl::OnTH_LClicked(int col, long row, int updn, RECT *rect, POINT *point, BOOL processed)
	{
		UNREFERENCED_PARAMETER(row);
		UNREFERENCED_PARAMETER(*rect);
		UNREFERENCED_PARAMETER(*point);
		UNREFERENCED_PARAMETER(processed);

		if (!updn)
			return;

		BeginWaitCursor();

		row = GetCurrentRow();
		size_t index = (row >= 0 && row<m_sortInfo.size()) ? m_sortInfo[row].second : NOT_INIT;
		SortInfo(col, col != m_curSortCol ? m_sortDir : OtherDir(m_sortDir));

		size_t newRow = std::distance(m_sortInfo.begin(), std::find_if(m_sortInfo.begin(), m_sortInfo.end(), FindByIndex(index)));
		if (newRow < m_sortInfo.size())
			GotoRow((long)newRow); 
		//else 
			//Select(-1, -1);

		RedrawAll();
		EndWaitCursor();


		//long newRow = std::distance(m_sortInfo.begin(), std::find_if(m_sortInfo.begin(), m_sortInfo.end(), FindByIndex(index)));
		//GotoRow(newRow);
	}

	void CStationsListCtrl::OnCB_LClicked(int updn, RECT *rect, POINT *point, BOOL processed)
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
		//else
			//Select(-1, -1);


		RedrawAll();
		EndWaitCursor();

		
		
		//long newRow = std::distance(m_sortInfo.begin(), std::find_if(m_sortInfo.begin(), m_sortInfo.end(), FindByIndex(index)));
		//GotoRow(newRow);
	}



	void CStationsListCtrl::OnSH_LClicked(int col, long row, int updn, RECT *rect, POINT *point, BOOL processed)
	{
		if (!updn)
			return;

		//long newRow = long((GetCurrentRow() == -1) ? -1 : m_sortInfo[GetCurrentRow()].first);
		GotoRow(row);
	}



	void CStationsListCtrl::SortInfo(int col, int dir)
	{
		ASSERT(m_pDB);

		if (m_pDB->IsOpen())
		{
			if (col != m_curSortCol)
			{
				m_curSortCol = col;
				m_sortInfo.clear();

				CSearchResultVector results;

				//set<int> years = pDoc->GetYears();

				ERMsg msg;
				msg = m_pDB->GetStationList(results, m_filter, m_years, true, false);

				if (msg)
				{
					CWeatherDatabaseOptimization const& zop = m_pDB->GetOptimization();
					m_sortInfo.resize(results.size());
					for (CSearchResultVector::const_iterator it = results.begin(); it != results.end(); it++)
					{
						size_t pos = std::distance(results.cbegin(), it);
						const CLocation& loc = m_pDB->GetLocation(it->m_index);

						string str;

						switch (m_curSortCol)
						{
						case -1:   str = to_string(pos + 1); break;
						case CLocation::ID:   str = loc.m_ID; break;
						case CLocation::NAME: str = loc.m_name; break;
						case CLocation::LAT:  str = FormatA("%015.6lf", loc.m_lat); break;
						case CLocation::LON:  str = FormatA("%015.6lf", loc.m_lon); break;
						case CLocation::ELEV: str = FormatA("%010.3lf", loc.m_elev); break;
						case NB_TOTAL_VARIABLES:str = FormatA("%d", m_pDB->GetWVariables(it->m_index).count()); break;
						case NB_YEARS:			str = FormatA("%03d", m_pDB->GetYears(it->m_index).size()); break;
						case COMPLETENESS:		str = FormatA("%010.3lf", GetCompleteness(m_pDB->GetWVariablesCounter(it->m_index))); break;

						default: ASSERT(false);
						}

						m_sortInfo[pos] = std::make_pair(str, it->m_index );
					}


					if (m_curSortCol == CLocation::ID || m_curSortCol == CLocation::NAME)
						std::sort(m_sortInfo.begin(), m_sortInfo.end(), CCompareString());
					else
						std::sort(m_sortInfo.begin(), m_sortInfo.end(), CompareNumber);

					m_sortDir = UGCT_SORTARROWUP;
				}
			}

			if (dir != m_sortDir)
			{
				std::reverse(m_sortInfo.begin(), m_sortInfo.end());
				m_sortDir = dir;
			}

			AfxGetApp()->WriteProfileInt(_T("StationsListCtrl"), _T("SortCol"), m_curSortCol);
			AfxGetApp()->WriteProfileInt(_T("StationsListCtrl"), _T("SortDir"), m_sortDir);
		}
		else
		{
			m_curSortCol = -999;
			m_sortDir = UGCT_SORTARROWUP;
		}
	}

	void CStationsListCtrl::OnColSized(int col, int *width)
	{
		AfxGetApp()->WriteProfileInt(_T("StationsListCtrl"), _T("ColWidth ") + UtilWin::ToCString(col), *width);
	}

	/*int CStationsListCtrl::RedrawRow(long index)
	{
		long row = std::distance(m_sortInfo.begin(), std::find_if(m_sortInfo.begin(), m_sortInfo.end(), FindByIndex(index)));
		return CUGCtrl::RedrawRow(row);
	}
*/
	void	CStationsListCtrl::SetStationIndex(size_t index)
	{
		long row = std::distance(m_sortInfo.begin(), std::find_if(m_sortInfo.begin(), m_sortInfo.end(), FindByIndex(index)));
		CUGCtrl::GotoRow(row);
		CUGCtrl::RedrawRow(row);
	}

	size_t	CStationsListCtrl::GetStationIndex()const
	{
		long row = const_cast<CStationsListCtrl*>(this)->GetCurrentRow();
		size_t newPos = row >= 0 ? m_sortInfo[row].second: UNKNOWN_POS;
		return newPos;
	}


	//********************************************************************************************************************************
	//*******************************************************************************************************

	enum TMatchStations{ M_ID, M_NAME, M_LAT, M_LON, M_ALT, M_SHORE_DISTANCE, M_DISTANCE, M_DELTA_ELEVATION, M_DELTA_SHORE, M_WEIGHT, NB_MATCH_STATIONS_COLUMNS };

	BEGIN_MESSAGE_MAP(CMatchStationsCtrl, CUGCtrl)
	END_MESSAGE_MAP()

	CMatchStationsCtrl::CMatchStationsCtrl(void)
	{
		m_curSortCol = -999;
		m_sortDir = UGCT_SORTARROWUP;
	}


	void CMatchStationsCtrl::CreateBoldFont()
	{
		if (m_fontBold.GetSafeHandle() != NULL)
			m_fontBold.DeleteObject();

		CFont* pFont = &m_font;
		ASSERT_VALID(pFont);

		LOGFONT lf;
		memset(&lf, 0, sizeof(LOGFONT));

		pFont->GetLogFont(&lf);

		lf.lfWeight = FW_BOLD;
		m_fontBold.CreateFontIndirect(&lf);
	}

	void CMatchStationsCtrl::OnSetup()
	{

		m_font.CreateStockObject(DEFAULT_GUI_FONT);
		CreateBoldFont();
		m_cellBorderPen.CreatePen(PS_SOLID, 1, RGB(157, 157, 161));


		m_CUGHint->SetFont(&m_font);
		m_CUGTab->ShowScrollbars(true);

		// add and set heading's default celltype
		CUGCell cell;
		GetHeadingDefault(&cell);
		cell.SetAlignment(UG_ALIGNLEFT);
		cell.SetBorder(UG_BDR_RTHIN | UG_BDR_BTHIN);
		cell.SetBackColor(RGB(239, 237, 242));
		cell.SetHBackColor(RGB(162, 192, 248));

		cell.SetBorderColor(&m_cellBorderPen);
		cell.SetFont(&m_font);
		SetHeadingDefault(&cell);

		// create a font and set it as Grid Default
		GetGridDefault(&cell);
		cell.SetAlignment(UG_ALIGNLEFT);
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



	void CMatchStationsCtrl::Update()
	{
		//CMatchStationDoc* pDoc = GetDocument();
		//CWeatherDatabasePtr pDB = pDoc->GetNormalsDatabase();

		string filePath;
		if (m_pDB.get() && m_pDB->IsOpen())
			filePath = m_pDB->GetFilePath();

		//size_t index = pDoc->GetCurIndex();

		if (filePath != m_lastFilePath ||
			m_location != m_lastLocation ||
			m_nearest != m_lastNearest)
		{
			m_curSortCol = -999;
			m_sortDir = UGCT_SORTARROWUP;
			m_lastFilePath = filePath;
			m_lastNearest = m_nearest;

			m_weight = m_nearest.GetStationWeight();
			m_shoreD.resize(m_weight.size());
			m_deltaShore.resize(m_weight.size());

			if (m_location.IsInit())
			{
				double dShore = CWeatherGradient::GetShoreDistance(m_location);
				for (size_t i = 0; i < m_shoreD.size(); i++)
				{
					m_shoreD[i] = CWeatherGradient::GetShoreDistance(m_nearest[i].m_location);
					m_deltaShore[i] = dShore - m_shoreD[i];
				}
			}
				

			if (m_pDB && m_pDB->IsOpen())
			{
				m_enableUpdate = FALSE;
				m_lastLocation = m_location;

				SortInfo(M_WEIGHT, UGCT_SORTARROWDOWN);
		
				SetNumberCols(NB_MATCH_STATIONS_COLUMNS, FALSE);
				SetNumberRows((long)m_sortInfo.size(), FALSE);

				for (int i = -1; i < NB_MATCH_STATIONS_COLUMNS; i++)
				{
					int width = AfxGetApp()->GetProfileInt(_T("MatchStationsListCtrl"), _T("ColWidth ") + UtilWin::ToCString(i), 50);
					SetColWidth(i, width);
				}

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
			GetParent()->SendMessage(UWM_SELECTION_CHANGE);
		}

	}



	void CMatchStationsCtrl::OnGetCell(int col, long row, CUGCell *cell)
	{

		if (m_enableUpdate)
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
					if (col == M_SHORE_DISTANCE)
						text = GetString(IDS_STR_SHORE_DISTANCE);
					else if (col == M_DISTANCE)
						text = GetString(IDS_STR_DISTANCE);
					else if (col == M_DELTA_ELEVATION)
						text = GetString(IDS_STR_DELTA_ELEV);
					else 	if (col == M_DELTA_SHORE)
						text = "Delta Shore";//text = GetString(IDS_STR_DELTA_SHORE);
					else if (col == M_WEIGHT)
						text = GetString(IDS_STR_WEIGHT);
					else
						text = CLocation::GetMemberTitle(col);
				}
				else
				{
					if (m_pDB.get() && m_pDB->IsOpen())
					{
						size_t index = m_sortInfo[row].second;
						//long newRow = std::distance(m_sortInfo.begin(), std::find_if(m_sortInfo.begin(), m_sortInfo.end(), FindByIndex(index)));
						//m_nearest.Find()
						textColor = RGB(0, 0, 0);
						backColor = (row % 2) ? RGB(235, 235, 255) : RGB(255, 255, 255);

						if (!m_location.IsValid())
						{
							backColor = RGB(255, 50, 40);
							textColor = RGB(255, 255, 255);
						}
						
						if (col == M_SHORE_DISTANCE)
							text = ToString(m_shoreD[index] / 1000, 1);
						else if (col == M_DISTANCE)
							text = ToString(m_nearest[index].m_distance / 1000, 1);
						else if (col == M_DELTA_ELEVATION)
							text = ToString(m_nearest[index].m_deltaElev, 0);
						else if (col == M_DELTA_SHORE)
							text = ToString(m_deltaShore[index] / 1000, 1);
						else if (col == M_WEIGHT)
							text = ToString(m_weight[index] * 100, 1);
						else
							text = m_nearest[index].m_location.GetMember(col);
					}//id db open
				}//if col
			}//if row


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
	void CMatchStationsCtrl::OnCellChange(int oldcol, int newcol, long oldrow, long newrow)
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
	COLORREF CMatchStationsCtrl::OnGetDefBackColor(int section)
	{
		UNREFERENCED_PARAMETER(section);

		return GetSysColor(COLOR_APPWORKSPACE);
	}



	int CMatchStationsCtrl::OnHint(int col, long row, int section, CString *string)
	{
		if (m_pDB)
		{
			if (section == UG_GRID)
			{
				if (col >= 0 && row >= 0)
				{
					size_t index = m_sortInfo[row].second;
					index = m_nearest[index].m_index;
					*string = (*m_pDB)[index].GetMember(col).c_str();
				}
			}
		}


		return !string->IsEmpty();
	}

	int CMatchStationsCtrl::OnVScrollHint(long row, CString *string)
	{
		ASSERT(m_pDB.get());

		if (row >= 0)
		{
			size_t index = m_sortInfo[row].second;
			index = m_nearest[index].m_index;
			*string = (*m_pDB)[index].m_name.c_str();
		}

		return  !string->IsEmpty();
	}


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
	void CMatchStationsCtrl::OnTH_LClicked(int col, long row, int updn, RECT *rect, POINT *point, BOOL processed)
	{
		UNREFERENCED_PARAMETER(row);
		UNREFERENCED_PARAMETER(*rect);
		UNREFERENCED_PARAMETER(*point);
		UNREFERENCED_PARAMETER(processed);

		if (!updn)
			return;

		BeginWaitCursor();

		//row = GetCurrentRow();
		//size_t index = (row>0) ? m_sortInfo[row].second : NOT_INIT;

		SortInfo(col, col != m_curSortCol ? m_sortDir : OtherDir(m_sortDir));


		//size_t newRow = std::distance(m_sortInfo.begin(), std::find_if(m_sortInfo.begin(), m_sortInfo.end(), FindByIndex(index)));
		//if (newRow >= m_sortInfo.size())
		//Select(-1, -1);
		//else GotoRow((long)newRow);

		//if (index >= m_sortInfo.size())
			//Select(-1, -1);//force changing

		RedrawAll();
		EndWaitCursor();

		
		
		//long newRow = std::distance(m_sortInfo.begin(), std::find_if(m_sortInfo.begin(), m_sortInfo.end(), FindByIndex(index)));
		//GotoRow(newRow);
	}

	void CMatchStationsCtrl::OnCB_LClicked(int updn, RECT *rect, POINT *point, BOOL processed)
	{
		UNREFERENCED_PARAMETER(*rect);
		UNREFERENCED_PARAMETER(*point);
		UNREFERENCED_PARAMETER(processed);

		if (!updn)
			return;

		BeginWaitCursor();

		//int row = GetCurrentRow();
		//size_t index = (row>0) ? m_sortInfo[row].second : NOT_INIT;
		SortInfo(-1, m_curSortCol != -1 ? m_sortDir : OtherDir(m_sortDir));
		//if (index >= m_sortInfo.size())
			//Select(-1, -1);


		//size_t newRow = std::distance(m_sortInfo.begin(), std::find_if(m_sortInfo.begin(), m_sortInfo.end(), FindByIndex(index)));
		//if (newRow >= m_sortInfo.size())
		//Select(-1, -1);
		//else GotoRow((long)newRow);

		RedrawAll();
		EndWaitCursor();

	
//
		//long newRow = std::distance(m_sortInfo.begin(), std::find_if(m_sortInfo.begin(), m_sortInfo.end(), FindByIndex(index)));
		//GotoRow(newRow);
	}



	void CMatchStationsCtrl::OnSH_LClicked(int col, long row, int updn, RECT *rect, POINT *point, BOOL processed)
	{
		if (!updn)
			return;


		/*BeginWaitCursor();


		size_t index = (row>0) ? m_sortInfo[row].second : NOT_INIT;
		SortInfo(-1, m_curSortCol != -1 ? m_sortDir : OtherDir(m_sortDir));
		if (GetCurrentRow() >= m_sortInfo.size())
			Select(-1, -1);

		RedrawAll();
		EndWaitCursor();
*/
		GotoRow(row);
	}



	void CMatchStationsCtrl::SortInfo(int col, int dir)
	{
		ASSERT(m_pDB);

		if (m_pDB.get() && m_pDB->IsOpen())
		{
			if (col != m_curSortCol)
			{
				m_curSortCol = col;
				m_sortInfo.clear();


				m_sortInfo.resize(m_nearest.size());
				for (CSearchResultVector::const_iterator it = m_nearest.begin(); it != m_nearest.end(); it++)
				{
					size_t pos = std::distance(m_nearest.cbegin(), it);
					const CLocation& loc = m_pDB->GetLocation(it->m_index);

					string str;

					switch (m_curSortCol)
					{
					case -1:			str = to_string(pos + 1); break;
					case M_ID:			str = loc.m_ID; break;
					case M_NAME:		str = loc.m_name; break;
					case M_LAT:			str = FormatA("%015.6lf", loc.m_lat); break;
					case M_LON:			str = FormatA("%015.6lf", loc.m_lon); break;
					case M_ALT:			str = FormatA("%010.3lf", loc.m_elev); break;
					case M_SHORE_DISTANCE:	str = ToString(m_shoreD[pos] / 1000, 1); break;
					case M_DISTANCE: 	str = ToString(it->m_distance / 1000, 1); break;
					case M_DELTA_ELEVATION:	str = ToString(it->m_deltaElev, 0); break;
					case M_DELTA_SHORE:	str = ToString(m_deltaShore[pos] / 1000, 1); break;
					case M_WEIGHT:		str = ToString(m_weight[pos] * 100, 1); break;
					default: ASSERT(false);
					}

					m_sortInfo[pos] = std::make_pair(str, pos);
				}


				if (m_curSortCol == CLocation::ID || m_curSortCol == CLocation::NAME)
					std::sort(m_sortInfo.begin(), m_sortInfo.end(), CCompareString());
				else
					std::sort(m_sortInfo.begin(), m_sortInfo.end(), CompareNumber);

				m_sortDir = UGCT_SORTARROWUP;


				AfxGetApp()->WriteProfileInt(_T("MatchStationsListCtrl"), _T("SortCol"), m_curSortCol);
				AfxGetApp()->WriteProfileInt(_T("MatchStationsListCtrl"), _T("SortDir"), m_sortDir);
			}

			if (dir != m_sortDir)
			{
				std::reverse(m_sortInfo.begin(), m_sortInfo.end());
				m_sortDir = dir;
			}

		}
		else
		{
			m_curSortCol = -999;
			m_sortDir = UGCT_SORTARROWUP;
		}
	}

	void CMatchStationsCtrl::OnColSized(int col, int *width)
	{
		AfxGetApp()->WriteProfileInt(_T("MatchStationsListCtrl"), _T("ColWidth ") + UtilWin::ToCString(col), *width);
	}

	/*int CMatchStationsCtrl::RedrawRow(long index)
	{
		long row = std::distance(m_sortInfo.begin(), std::find_if(m_sortInfo.begin(), m_sortInfo.end(), FindByIndex(index)));
		return CUGCtrl::RedrawRow(row);
	}*/

	void	CMatchStationsCtrl::SetStationIndex(size_t index)
	{
		long row = std::distance(m_sortInfo.begin(), std::find_if(m_sortInfo.begin(), m_sortInfo.end(), FindByIndex(index)));
		GotoRow(row);
		CUGCtrl::RedrawRow(row);
	}

	size_t	CMatchStationsCtrl::GetStationIndex()const
	{
		
		long row = const_cast<CMatchStationsCtrl*>(this)->GetCurrentRow();
		size_t newPos = row >= 0 ? m_nearest[m_sortInfo[row].second].m_index:UNKNOWN_POS;
		return newPos;
	}

//	inline bool IsEqual(const char* str1, const std::string& str2, bool bCase = false){ return IsEqual(str1, str2.c_str(), bCase); }
//	inline bool IsEqual(const std::string& str1, const char* str2, bool bCase = false){ return IsEqual(str1.c_str(), str2, bCase); }

	void CMatchStationsCtrl::OnDClicked(int col, long row, RECT *rect, POINT *point, BOOL processed)
	{
		if (row >= 0)
		{
			size_t index = m_sortInfo[row].second;
			index = m_nearest[index].m_index;
			const CLocation& location = m_pDB->GetLocation(index);
			string filePath = m_pDB->GetFilePath();
			string ext = GetFileExtension(filePath);

			string editor;
			if (IsEqual(ext, ".NormalsStations"))
				editor = CRegistry::NORMAL_EDITOR;
			else if (IsEqual(ext, ".DailyStations"))
				editor = CRegistry::DAILY_EDITOR;
			else if (IsEqual(ext, ".HourlyStations"))
				editor = CRegistry::HOURLY_EDITOR;

			if (!editor.empty())
			{
				string command = "\"" + filePath + "\" -ID " + location.m_ID;
				WBSF::CallApplication(editor, command, GetSafeHwnd(), SW_SHOW, false, false);
			}

		}

	}

	void CMatchStationsCtrl::OnSH_DClicked(int col, long row, RECT *rect, POINT *point, BOOL processed)
	{
		OnDClicked(col, row, rect, point, processed);
	}



}