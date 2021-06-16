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

#include "ModelOutputVarDefGridCtrl.h"
#include "WeatherBasedSimulationString.h"
#include "WeatherBasedSimulationUI.h"

using namespace UtilWin;


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace WBSF
{


	CModelOutputVarDefGridCtrl::CModelOutputVarDefGridCtrl(int nbColsShow)
	{
		m_nbColsShow = nbColsShow;
	}


	void CModelOutputVarDefGridCtrl::OnSetup()
	{
		if (OUTPUT_TYPE_NAME.IsEmpty())
			OUTPUT_TYPE_NAME.LoadString(IDS_STR_TM_TYPE);

		if (EXTENDED_TYPE_NAME.IsEmpty())
			EXTENDED_TYPE_NAME.LoadString(IDS_STR_TM_MODE);


		SetDefRowHeight(MulDiv(m_GI->m_defRowHeight, GetWindowDPI(GetSafeHwnd()), 96));
		SetTH_Height(MulDiv(m_GI->m_topHdgHeight, GetWindowDPI(GetSafeHwnd()), 96));
		SetHS_Height(MulDiv(m_GI->m_hScrollHeight, GetWindowDPI(GetSafeHwnd()), 96));


		//change font of header
		CUGCell cell;
		GetHeadingDefault(&cell);
		CFont* pFont = GetParent()->GetFont();
		cell.SetFont(pFont);
		SetHeadingDefault(&cell);


		CStringArrayEx title(IDS_OUTPUT_VAR_TITLE);


		EnableExcelBorders(TRUE);
		EnableMenu(TRUE);
		SetMultiSelectMode(TRUE);


		SetNumberCols(m_nbColsShow);
		SetNumberRows(50);

		ASSERT(title.GetSize() >= m_nbColsShow);
		for (int i = 0; i < m_nbColsShow; i++)
		{
			QuickSetText(i, -1, title[i]);
		}


		CString typeLable = OUTPUT_TYPE_NAME.ToString(_T("\n"), false);
		//CString extendedLable = EXTENDED_TYPE_NAME.ToString("\n",false);

		for (int i = 0; i < GetNumberRows(); i++)
		{
			QuickSetCellType(CModelOutputVariableDef::TIME_MODE, i, UGCT_DROPLIST);
			QuickSetLabelText(CModelOutputVariableDef::TIME_MODE, i, typeLable);
		}

		SetColWidth(-1, 25);

		EnableToolTips();

	}

	int CModelOutputVarDefGridCtrl::OnEditStart(int col, long row, CWnd **edit)
	{
		UNREFERENCED_PARAMETER(row);
		UNREFERENCED_PARAMETER(**edit);

		ASSERT(CModelOutputVariableDef::NB_MEMBERS == 8);
		bool bEdit = true;// (col >= CModelOutputVariableDef::NAME && col <= CModelOutputVariableDef::PRECISION);
		return bEdit;
	}


	int CModelOutputVarDefGridCtrl::OnMenuStart(int col, long row, int section)
	{
		if (section == UG_GRID)
		{
			//****** Empty the Menu!!
			EmptyMenu();

			//******* Add the Menu Items
			CWnd* pParent = GetParent();
			if (pParent)
			{
				AddMenuItem(1001, _T("Paste"));
			}
		}

		return TRUE;

	}


	void CModelOutputVarDefGridCtrl::OnMenuCommand(int col, long row, int section, int item)
	{
		if (section == UG_GRID)
		{
			//****** The user has selected the 'Copy' option
			if (item == 1001)
				Paste();
		}
	}

	void CModelOutputVarDefGridCtrl::OnGetCell(int col, long row, CUGCell *cell)
	{
		if (col == -1)
		{
			cell->SetText(UtilWin::ToCString(row + 1));
		}

	}

	void CModelOutputVarDefGridCtrl::OnSetCell(int col, long row, CUGCell *cell)
	{
	}

	int CModelOutputVarDefGridCtrl::GetSelection(int col, CString str)
	{
		ASSERT(CModelOutputVariableDef::NB_MEMBERS == 8);
		ASSERT(col == CModelOutputVariableDef::TIME_MODE /*|| col==CModelOutputVariableDef::BEFORE_VALUE || col==CModelOutputVariableDef::AFTER_VALUE*/);

		str.Trim();

		int sel = -1;
		if (col == CModelOutputVariableDef::TIME_MODE)
		{
			assert(false);
			/*sel = CTM(CTM::DAILY, CTM::FOR_EACH_YEAR).;
			if( !str.IsEmpty() )
			{
			int t =
			sel = OUTPUT_TYPE_NAME.Find(str, false);
			}
			ASSERT(sel.IsInit());
			*/



		}



		return sel;
	}

	CString CModelOutputVarDefGridCtrl::GetSelectionString(int col, int sel)
	{
		ASSERT(CModelOutputVariableDef::NB_MEMBERS == 8);
		ASSERT(col == CModelOutputVariableDef::TIME_MODE);

		CString str;
		if (col == CModelOutputVariableDef::TIME_MODE)
			str = OUTPUT_TYPE_NAME[sel];


		return str;
	}


	void CModelOutputVarDefGridCtrl::GetData(CModelOutputVariableDefVector& data)
	{
		ASSERT(GetNumberCols() == CModelOutputVariableDef::NB_MEMBERS);
		data.clear();


		for (int i = 0; i < GetNumberRows(); i++)
		{
			CModelOutputVariableDef varDef;
			GetVariableDef(i, varDef);

			if (!varDef.m_name.empty())
				data.push_back(varDef);
		}
	}

	void CModelOutputVarDefGridCtrl::SetData(const CModelOutputVariableDefVector& data)
	{
		ASSERT(GetNumberCols() == CModelOutputVariableDef::NB_MEMBERS);

		int nbRows = GetNumberRows();
		for (int i = 0; i < nbRows&&i < data.size(); i++)
			SetVariableDef(i, data[i]);

		if (data.size() > 0)
			BestFit(0, 1, 0, UG_BESTFIT_TOPHEADINGS);

		RedrawAll();
	}

	void CModelOutputVarDefGridCtrl::GetVariableDef(int row, CModelOutputVariableDef& varDef)
	{
		ASSERT(GetNumberCols() >= CModelOutputVariableDef::NB_MEMBERS);
		for (int i = 0; i < CModelOutputVariableDef::NB_MEMBERS; i++)
			varDef.SetMember(i, ToUTF8(QuickGetText(i, row)));
	}

	void CModelOutputVarDefGridCtrl::SetVariableDef(int row, const CModelOutputVariableDef& varDef)
	{
		ASSERT(GetNumberCols() >= CModelOutputVariableDef::NB_MEMBERS);
		for (int i = 0; i < CModelOutputVariableDef::NB_MEMBERS; i++)
			QuickSetText(i, row, Convert(varDef.GetMember(i)));
	}

}