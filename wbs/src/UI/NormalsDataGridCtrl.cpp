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
#include "Basic/UtilTime.h"
#include "Basic/Registry.h"
#include "UI/Common/UtilWin.h"
#include "UI/NormalsDataGridCtrl.h"

#include "WeatherBasedSimulationString.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace WBSF
{

	using namespace WEATHER;
	using namespace NORMALS_DATA;
	using namespace std;

	WBSF::StringVector CNormalsDataGridCtrl::VARIABLES_TOOLTIPS;

	void CNormalsDataGridCtrl::CreateBoldFont()
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
	void CNormalsDataGridCtrl::OnSetup()
	{
		SetDefRowHeight(MulDiv(m_GI->m_defRowHeight, UtilWin::GetWindowDPI(GetSafeHwnd()), 96));
		SetTH_Height(MulDiv(m_GI->m_topHdgHeight, UtilWin::GetWindowDPI(GetSafeHwnd()), 96));
		SetHS_Height(MulDiv(m_GI->m_hScrollHeight, UtilWin::GetWindowDPI(GetSafeHwnd()), 96));


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

	int CNormalsDataGridCtrl::OnEditStart(int col, long row, CWnd **edit)
	{
		return m_bEditable;
	}


	void CNormalsDataGridCtrl::Update()
	{

		string ID;
		if (m_pStation)
			ID = m_pStation->m_ID;

		if (ID != m_lastID ||
			m_variables != m_lastVariables)
		{
			m_lastID = ID;
			m_lastVariables = m_variables;

			m_bDataEdited.clear();
			m_bModified = false;

			if (m_pStation != NULL && m_pStation->IsInit())
			{
				m_enableUpdate = FALSE;
				WBSF::CRegistry registry("ColumnWidth");
				int colWidth = registry.GetValue<int>("Month", 90);
				SetColWidth(-1, colWidth);

				SetTH_NumberRows(1);
				SetSH_NumberCols(1);
				SetNumberCols((int)NORMALS_DATA::NB_FIELDS, FALSE);
				SetNumberRows((long)12, FALSE);
				m_bDataEdited.resize(12);

				for (size_t i = 0; i < m_bDataEdited.size(); i++)
					m_bDataEdited[i].resize(NORMALS_DATA::NB_FIELDS);


				size_t col = 0;
				for (size_t f = 0; f < NORMALS_DATA::NB_FIELDS; f++)
				{
					size_t v = F2V(f);
					if ((m_variables.none() || m_variables[v]))
					{
						string name = GetFieldHeader(f);
						int colWidth = registry.GetValue<int>(name, 80);
						SetSH_ColWidth((int)col, colWidth);
						SetColWidth((int)col, colWidth);
						col++;
					}
				}

				m_enableUpdate = TRUE;
				Invalidate();
			}
			else
			{
				SetTH_NumberRows(0);
				SetSH_NumberCols(0);
				SetNumberCols(0, FALSE);
				SetNumberRows(0, FALSE);
			}



		}
		else if (m_bEditable != m_bLastEditable)
		{
			m_bLastEditable = m_bEditable;

			for (size_t i = 0; i < m_bDataEdited.size(); i++)
				m_bDataEdited[i].reset();

			Invalidate();
		}

	}

	CTRef CNormalsDataGridCtrl::GetTRef(long row)
	{
		ASSERT(m_pStation->IsInit());
		ASSERT(GetNumberRows() > 0);

		CTRef TRef(YEAR_NOT_INIT, (size_t)row);

		return TRef;
	}

	void CNormalsDataGridCtrl::OnGetCell(int col, long row, CUGCell *cell)
	{

		if (m_pStation != NULL&&m_enableUpdate)
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
					text = GetFieldTitle(col);
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
					bool bEdit = m_bEditable;

					textColor = bEdit ? RGB(0, 0, 0) : RGB(100, 100, 100);
					backColor = bEdit ? ((row % 2) ? RGB(255, 235, 235) : RGB(250, 250, 250)) : ((row % 2) ? RGB(235, 235, 255) : RGB(255, 255, 255));

					//CTRef TRef = GetTRef(row);

					float value = (*m_pStation)[row][col];
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

						if (value < GetLimitN(col, 0) ||
							value > GetLimitN(col, 1))
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


	void CNormalsDataGridCtrl::OnSetCell(int col, long row, CUGCell *cell)
	{
		ASSERT(col >= 0 && row >= -1);
		ASSERT(m_bEditable);

		if (row != -1 && col != -1)
		{
			size_t v = F2V(col);
			if (m_variables.none() || m_variables[v])
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

					float newValue = UtilWin::ToFloat(cell->GetText());
					m_bModified = true;

					(*m_pStation)[row][col] = newValue;
				}
			}
		}


		CUGEditCtrl::OnSetCell(col, row, cell);
	}


	int CNormalsDataGridCtrl::OnMenuStart(int col, long row, int section)
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
			WBSF::StringVector str(IDS_STR_EDIT_COMMAND);

			AddMenuItem(ID_EDIT_SELECT_ALL, CString(str[0].c_str()));
			AddMenuItem(ID_EDIT_COPY, CString(str[2].c_str()));

			if (m_bEditable)
			{
				AddMenuItem(ID_EDIT_PASTE, CString(str[3].c_str()));
				AddMenuItem(ID_EDIT_CLEAR, CString(str[4].c_str()));
			}
		}


		return TRUE;
	}

	void CNormalsDataGridCtrl::OnMenuCommand(int col, long row, int section, int item)
	{
		if (section == UG_CORNERBUTTON)
		{

		}
		else if (section == UG_TOPHEADING)
		{
			/*if (item == 1001)
			{
			CNewNameDlg dlg;
			dlg.m_name = QuickGetText(col, row);
			if (dlg.DoModal() == IDOK)
			{
			QuickSetText(col, row, dlg.m_name);
			Invalidate();
			}

			}*/
		}
		else if (section == UG_GRID || section == UG_SIDEHEADING)
		{
			//****** The user has selected the 'Copy' option
			if (item == ID_EDIT_SELECT_ALL)
			{
				SelectRange(0, 0, GetNumberCols(), GetNumberRows());
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


	BOOL CNormalsDataGridCtrl::PreTranslateMessage(MSG* pMsg)
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



	/////////////////////////////////////////////////////////////////////////////
	//	OnGetDefBackColor
	//		Sent when the area behind the grid needs to be paindted.
	//	Params:
	//		section - Id of the grid section that requested this color
	//				  possible sections:
	//						UG_TOPHEADING, UG_SIDEHEADING, UG_GRID
	//	Return:
	//		RGB value representing the color of choice
	COLORREF CNormalsDataGridCtrl::OnGetDefBackColor(int section)
	{
		UNREFERENCED_PARAMETER(section);

		return GetSysColor(COLOR_APPWORKSPACE);
	}



	void CNormalsDataGridCtrl::ReloadString()
	{
		WBSF::StringVector units(IDS_STR_WEATHER_VARIABLES_UNITS, ";|");
		WBSF::StringVector names(IDS_STR_WEATHER_VARIABLES_TITLE, ";|");
		assert(names.size() == units.size());

		VARIABLES_TOOLTIPS.resize(names.size());
		for (size_t i = 0; i < names.size(); i++)
			VARIABLES_TOOLTIPS[i] = names[i] + " [" + units[i] + "]";
	}


	void CNormalsDataGridCtrl::OnColSized(int col, int *width)
	{
		WBSF::CRegistry registry("ColumnWidth");
		string name = GetFieldHeader(col);
		registry.SetValue(name, *width);
	}

	int  CNormalsDataGridCtrl::OnSideHdgSized(int *width)
	{
		WBSF::CRegistry registry("ColumnWidth");
		registry.SetValue("Month", *width);
		return TRUE;
	}
}