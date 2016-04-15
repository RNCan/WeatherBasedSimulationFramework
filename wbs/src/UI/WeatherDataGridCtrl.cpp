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
#include "UI/Common/UtilWin.h"
#include "UI/WeatherDataGridCtrl.h"
#include "WeatherBasedSimulationString.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace WBSF::WEATHER;
using namespace WBSF::HOURLY_DATA;
using namespace UtilWin;
using namespace std;


namespace WBSF
{
	WBSF::StringVector CWeatherDataGridCtrl::VARIABLES_TOOLTIPS;

	void CWeatherDataGridCtrl::CreateBoldFont()
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

	/////////////////////////////////////////////////////////////////////////////
	//	OnSetup
	//		This function is called just after the grid window 
	//		is created or attached to a dialog item.
	//		It can be used to initially setup the grid
	void CWeatherDataGridCtrl::OnSetup()
	{

		m_font.CreateStockObject(DEFAULT_GUI_FONT);
		CreateBoldFont();
		m_cellBorderPen.CreatePen(PS_SOLID, 1, RGB(157, 157, 161));
		// create and set new top-heading class
		CExcelTopHdg* pExcelTopHdg = new CExcelTopHdg;
		pExcelTopHdg->Create(NULL, _T(""), WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), this, 234752);
		SetNewTopHeadingClass(pExcelTopHdg);
		// create and set new side-heading class
		CExcelSideHdg* pExcelSideHdg = new CExcelSideHdg;
		pExcelSideHdg->Create(NULL, _T(""), WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), this, 234753);
		SetNewSideHeadingClass(pExcelSideHdg);
		m_CUGHint->SetFont(&m_font);
		m_CUGTab->ShowScrollbars(true);


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
		UseHints(TRUE);
		UseVScrollHints(TRUE);
		EnableMenu(TRUE);
		EnableExcelBorders(TRUE);


		SetDoubleBufferMode(TRUE);
		SetVScrollMode(UG_SCROLLNORMAL);//UG_SCROLLTRACKING
		SetHScrollMode(UG_SCROLLTRACKING);
		SetHighlightRow(FALSE, FALSE);
		SetMultiSelectMode(UG_MULTISELECT_CELL);//
		SetCurrentCellMode(3);



		SetTH_NumberRows(0);
		SetSH_NumberCols(0);
		SetNumberCols(0);
		SetNumberRows(0);


	}

	int CWeatherDataGridCtrl::OnEditStart(int col, long row, CWnd **edit)
	{
		const CWeatherFormat& format = m_pStation->GetFormat();
		return m_bEditable && format[m_colMap[col]].m_var != SKIP;
	}


	void CWeatherDataGridCtrl::Update()
	{

		string ID;
		if (m_pStation)
			ID = m_pStation->m_ID;

		if (ID != m_lastID || m_TM != m_lastTM ||
			m_bPeriodEnabled != m_bLastPeriodEnabled || m_period != m_lastPeriod ||
			m_variables != m_lastVariables)
		{
			m_lastTM = m_TM;
			m_lastID = ID;
			m_bLastPeriodEnabled = m_bPeriodEnabled;
			m_lastPeriod = m_period;
			m_lastVariables = m_variables;

			m_colMap.clear();
			m_bDataEdited.clear();
			m_bModified = false;

			if (m_pStation != NULL && m_pStation->IsInit())
			{
				m_enableUpdate = FALSE;

				const CWeatherFormat& format = m_pStation->GetFormat();
				WBSF::StringVector header = format.GetHeaderVector();
				CTPeriod period = m_pStation->GetEntireTPeriod(m_TM);
				if (m_bPeriodEnabled && m_period.IsInit())
				{
					CTPeriod p(m_period);
					p.Transform(m_TM);
					period = period.Intersect(p);
				}

				size_t nbCols = 0;
				for (int v = 0; v < format.size(); v++)
					if (format[v].m_var != SKIP && (m_variables.none() || m_variables[format[v].m_var]))
						nbCols++;


				WBSF::CRegistry registry("ColumnWidth");
				int colWidth = registry.GetValue<int>("Time", 90);
				SetColWidth(-1, colWidth);

				SetNumberCols((int)nbCols, FALSE);
				SetNumberRows((long)period.size(), FALSE);
				m_bDataEdited.resize(period.size());

				for (size_t i = 0; i < m_bDataEdited.size(); i++)
					m_bDataEdited[i].resize(nbCols);


				m_colMap.resize(nbCols);
				size_t col = 0;
				for (size_t v = 0; v < format.size(); v++)
				{
					if (format[v].m_var != SKIP && (m_variables.none() || m_variables[format[v].m_var]))
					{
						m_colMap[col] = v;

						string name = GetVariableName(format[v].m_var);
						int colWidth = registry.GetValue<int>(name, 80);
						SetSH_ColWidth((int)col, colWidth);
						SetColWidth((int)col, colWidth);
						col++;
					}
				}
				
				m_enableUpdate = TRUE;
			}
			else
			{
				SetNumberCols(0, FALSE);
				SetNumberRows(0, FALSE);
			}

			
			Invalidate();

		}
		else if (m_stat != m_lastStat)
		{
			m_lastStat = m_stat;
			Invalidate();
		}
		else if (m_bEditable != m_bLastEditable)
		{
			m_bLastEditable = m_bEditable;

			for (size_t i = 0; i < m_bDataEdited.size(); i++)
				m_bDataEdited[i].reset();

			Invalidate();
		}

	}

	CTRef CWeatherDataGridCtrl::GetTRef(long row)
	{
		ASSERT(m_pStation->IsInit());
		ASSERT(GetNumberRows() > 0);

		CTRef TRef;
		CTPeriod period = m_pStation->GetEntireTPeriod(m_TM);

		if (m_bPeriodEnabled && m_period.IsInit())
		{
			CTPeriod p(m_period);
			p.Transform(m_TM);
			period = period.Intersect(p);
		}

		ASSERT(period.IsInit());//don't call this method if not intersect
		ASSERT(row >= 0 && row < period.size());

		TRef = period[row];

		return TRef;
	}

	void CWeatherDataGridCtrl::OnGetCell(int col, long row, CUGCell *cell)
	{

		if (m_pStation != NULL && m_pStation->HaveData() && !m_colMap.empty() && m_enableUpdate)
		{
			string text;
			COLORREF backColor = cell->GetBackColor();
			COLORREF textColor = cell->GetTextColor();
			CFont* pFont = cell->GetFont();

			if (row == -1)
			{
				if (col == -1)
				{
					text = WBSF::GetString(IDS_WG_WEATHER_XAXIS_TITLE);
				}
				else
				{
					const CWeatherFormat& format = m_pStation->GetFormat();
					TVarH var = format[m_colMap[col]].m_var;


					ASSERT(col >= 0 && col < format.size());
					WBSF::StringVector header(format.GetHeader(), ",;\t");
					if (header.size() == format.size())
					{
						textColor = var < H_ADD1 ? RGB(0, 0, 0) : RGB(240, 0, 0);
						if (var != SKIP)
							text = GetVariableTitle(var);
						else
							text = header[m_colMap[col]];
					}
				}

			}
			else
			{
				if (col == -1)
				{
					CTRef TRef = GetTRef(row);
					text = TRef.GetFormatedString();
				}
				else
				{
					const CWeatherFormat& format = m_pStation->GetFormat();
					TVarH var = format[m_colMap[col]].m_var;
					size_t stat = format[m_colMap[col]].m_stat;

					bool bEdit = m_bEditable && var < H_ADD1;//var != HOURLY_DATA::SKIP 

					textColor = bEdit ? RGB(0, 0, 0) : RGB(100, 100, 100);
					backColor = bEdit ? ((row % 2) ? RGB(255, 235, 235) : RGB(250, 250, 250)) : ((row % 2) ? RGB(235, 235, 255) : RGB(255, 255, 255));


					CTRef TRef = GetTRef(row);
					CStatistic weaStat;
					if (var == H_TAIR && (stat == LOWEST || stat == HIGHEST))
					{
						if (stat == LOWEST)
							weaStat = (*m_pStation)[TRef].GetVarEx(H_TMIN);
						else 
							weaStat = (*m_pStation)[TRef].GetVarEx(H_TMAX);
					}
					else if (m_pStation->IsHourly() && TRef.GetType() == CTRef::DAILY)
					{
						const CWeatherDay& day = (*m_pStation).GetDay(TRef);
						for (size_t h = 0; h < 24; h++)
							if (!IsMissing(day[h][var]))
								weaStat += day[h][var];
					}
					else
					{
						weaStat = (*m_pStation)[TRef][var];
					}

					float value = float(weaStat[m_stat]);
					if (!IsMissing(value))
					{
						text = WBSF::ToString(value);

						if (bEdit)
						{
							if (m_bDataEdited[row][col])
							{
								textColor = RGB(50, 90, 230);
								pFont = &m_fontBold;
							}
						}

						if (m_stat == WBSF::MEAN &&
							(var >= H_ADD1 ||
							value < GetLimitH(var, 0) ||
							value > GetLimitH(var, 1)))
						{
							backColor = RGB(255, 50, 40);
							textColor = RGB(255, 255, 255);
						}
					}
				}
			}

			cell->SetBackColor(backColor);
			cell->SetTextColor(textColor);
			cell->SetText(CString(text.c_str()));
			cell->SetFont(pFont);
		}


		CUGEditCtrl::OnGetCell(col, row, cell);
	}


	void CWeatherDataGridCtrl::OnSetCell(int col, long row, CUGCell *cell)
	{
		ASSERT(col >= 0 && row >= -1);
		ASSERT(m_bEditable);

		if (row != -1 && col != -1)
		{
			const CWeatherFormat& format = m_pStation->GetFormat();
			TVarH var = format[m_colMap[col]].m_var;
			size_t stat = format[m_colMap[col]].m_stat;


			if (var != SKIP && (m_variables.none() || m_variables[var]))
			{
				CUGCell oldCell;
				GetCellIndirect(col, row, &oldCell);

				CTRef TRef = GetTRef(row);
				ASSERT(TRef.GetTM().Type() == CTM::HOURLY);

				std::string oldStr = CStringA(oldCell.GetText());
				std::string newStr = CStringA(cell->GetText());

				if (newStr != oldStr)
				{
					size_t pos = row*GetNumberCols() + row;
					m_bDataEdited[row].set(col);

					double newValue = UtilWin::ToDouble(cell->GetText());
					m_bModified = true;
					//m_pStation->SetModified(true);


					if (var == H_TAIR && (stat == LOWEST || stat == HIGHEST))
					{
						ASSERT(TRef.GetType() == CTRef::DAILY);
						ASSERT((*m_pStation)[TRef][H_TRNG].IsInit());
						double Tmin = (*m_pStation)[TRef].GetVarEx(H_TMIN);
						double Tmax = (*m_pStation)[TRef].GetVarEx(H_TMAX);
						if (stat == LOWEST)
							Tmin = newValue;
						else if (stat == HIGHEST)
							Tmax = newValue;

						(*m_pStation)[TRef].SetStat(H_TAIR, (Tmin + Tmax) / 2);
						(*m_pStation)[TRef].SetStat(H_TRNG, Tmax - Tmin);
					}
					else
					{
						(*m_pStation)[TRef].SetStat(var, newValue);
					}
				}
			}
		}


		CUGEditCtrl::OnSetCell(col, row, cell);
	}


	int CWeatherDataGridCtrl::OnMenuStart(int col, long row, int section)
	{
		//****** Empty the Menu!!
		EmptyMenu();

		if (section == UG_CORNERBUTTON)
		{

		}
		else if (section == UG_TOPHEADING)
		{
			//AddMenuItem(1001, _T("Changer le titre"));
		}
		else if (section == UG_GRID || section == UG_SIDEHEADING)
		{
			CStringArrayEx str(IDS_STR_EDIT_COMMAND);

			AddMenuItem(ID_EDIT_SELECT_ALL, str[0]);
			AddMenuItem(ID_EDIT_COPY, str[2]);

			const CWeatherFormat& format = m_pStation->GetFormat();
			if (m_bEditable && format[m_colMap[col]].m_var != SKIP)
			{
				AddMenuItem(ID_EDIT_PASTE, str[3]);
				AddMenuItem(ID_EDIT_CLEAR, str[4]);
			}
		}


		return TRUE;
	}

	void CWeatherDataGridCtrl::OnMenuCommand(int col, long row, int section, int item)
	{
		if (section == UG_CORNERBUTTON)
		{
		}
		else if (section == UG_TOPHEADING)
		{
		}
		else if (section == UG_GRID || section == UG_SIDEHEADING)
		{
			//****** The user has selected the 'Copy' option
			if (item == ID_EDIT_SELECT_ALL)
			{
				SelectRange(0, 0, GetNumberCols()-1, GetNumberRows()-1);
			}

			//****** The user has selected the 'Copy' option
			if (item == ID_EDIT_COPY)
			{
				CopySelected();
			}

			//****** The user has selected the 'Paste' option
			if (item == ID_EDIT_PASTE)
			{
				Paste();
			}

			//****** The user has selected the 'Clear' option
			if (item == ID_EDIT_CLEAR)
			{
				DeleteSelection();
			}
		}
	}


	BOOL CWeatherDataGridCtrl::PreTranslateMessage(MSG* pMsg)
	{
		if (GetKeyState(VK_CONTROL) < 0)
		{
			if (pMsg->message == WM_KEYDOWN)
			{
				if (pMsg->wParam == 67)
				{
					CopySelected();

					return TRUE; // this doesn't need processing anymore

				}
				else if (pMsg->wParam == 86)
				{
					if (m_bEditable)
					{
						//get the text from the clipboard
						Paste();

						return TRUE; // this doesn't need processing anymore
					}
				}
			}
		}

		if (pMsg->wParam == VK_DELETE)
		{
			DeleteSelection();
		}

		return CUGEditCtrl::PreTranslateMessage(pMsg); // all other cases still need default processing
	}


	int CWeatherDataGridCtrl::OnVScrollHint(long row, CString *string)
	{
		*string = QuickGetText(-1, row);

		return TRUE;
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
	COLORREF CWeatherDataGridCtrl::OnGetDefBackColor(int section)
	{
		UNREFERENCED_PARAMETER(section);

		return GetSysColor(COLOR_APPWORKSPACE);
	}



	void CWeatherDataGridCtrl::ReloadString()
	{
		WBSF::StringVector units(IDS_STR_WEATHER_VARIABLES_UNITS, ";|");
		WBSF::StringVector names(IDS_STR_WEATHER_VARIABLES_TITLE, ";|");
		assert(names.size() == units.size());

		VARIABLES_TOOLTIPS.resize(names.size());
		for (size_t i = 0; i < names.size(); i++)
			VARIABLES_TOOLTIPS[i] = names[i] + " [" + units[i] + "]";
	}

	int CWeatherDataGridCtrl::OnHint(int col, long row, int section, CString *string)
	{
		if (section == UG_TOPHEADING)
		{
			if (col >= 0)
			{
				if (VARIABLES_TOOLTIPS.empty())
					ReloadString();

				const CWeatherFormat& format = m_pStation->GetFormat();
				assert(m_colMap[col] < format.size());

				TVarH var = format[m_colMap[col]].m_var;
				*string = CString(VARIABLES_TOOLTIPS[var].c_str());

			}
		}


		return !string->IsEmpty();
	}

	void CWeatherDataGridCtrl::OnColSized(int col, int *width)
	{
		const CWeatherFormat& format = m_pStation->GetFormat();
		ASSERT(col >= -1 && col < format.size());

		WBSF::CRegistry registry("ColumnWidth");
		string name = GetVariableName(format[m_colMap[col]].m_var);
		registry.SetValue(name, *width);
	}

	int  CWeatherDataGridCtrl::OnSideHdgSized(int *width)
	{
		WBSF::CRegistry registry("ColumnWidth");
		registry.SetValue("Time", *width);
		return TRUE;
	}


}