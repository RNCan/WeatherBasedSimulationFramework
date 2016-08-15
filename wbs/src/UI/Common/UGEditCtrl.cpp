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
#include "UI/Common/UGEditCtrl.h"


CUGEditCtrl::CUGEditCtrl(void)
{
	m_bHaveChange=false;

}

CUGEditCtrl::~CUGEditCtrl(void)
{
}


void CUGEditCtrl::OnDClicked(int col,long row,RECT *rect,POINT *point,BOOL processed)
{
	UNREFERENCED_PARAMETER(col);
	UNREFERENCED_PARAMETER(row);
	UNREFERENCED_PARAMETER(*rect);
	UNREFERENCED_PARAMETER(*point);
	UNREFERENCED_PARAMETER(processed);
	// Start edit when user double clicks in one of the cells
	StartEdit();
}				 

void CUGEditCtrl::OnCharDown(UINT *vcKey,BOOL processed)
{
	UNREFERENCED_PARAMETER(processed);
	// Start edit when a cell has focus and user types a character, this version of StartEdit allows us to pass information on the character user pressed.
	StartEdit( *vcKey );
}

void CUGEditCtrl::OnSetCell(int col,long row,CUGCell *cell)
{ 
	
	if( col>=0 && row>= 0)
	{
		CUGCell oldCell;
		GetCellIndirect(col, row, &oldCell);
		
		CString oldStr(oldCell.GetText());
		CString newStr(cell->GetText());
		if( oldStr != newStr )
			m_bHaveChange=true;
	}
} 

void CUGEditCtrl::OnKeyUp(UINT *vcKey,BOOL processed)
{
	UNREFERENCED_PARAMETER(processed);

	if( *vcKey == VK_F2)
		StartEdit( NULL ); 
}



void CUGEditCtrl::DeleteSelection()
{

	int col;
	long row;
	CUGCell cell;

	//enum selected items and add them to the string
	int rt = m_GI->m_multiSelect->EnumFirstSelected(&col, &row);
	long lastrow = row;
	while (rt == UG_SUCCESS)
	{
		//get the selected cell then copy the string
		GetCellIndirect(col, row, &cell);

		//check the cut flag
		if (cell.GetReadOnly() != TRUE)
		{
			//cell.ClearAll();
			cell.SetText(_T(""));
			SetCell(col, row, &cell);

			CRect rect;
			GetCellRect(col, row, rect);
			InvalidateRect(rect);
		}

		//update the last row flag
		lastrow = row;

		//find the next selected item
		rt = m_GI->m_multiSelect->EnumNextSelected(&col, &row);
	}
}