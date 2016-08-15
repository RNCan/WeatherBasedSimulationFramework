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
#include "LineStyleCB.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLineStyleCB

CLineStyleCB::CLineStyleCB()
{

}

CLineStyleCB::~CLineStyleCB()
{
}


BEGIN_MESSAGE_MAP(CLineStyleCB, CComboBox)
	//{{AFX_MSG_MAP(CLineStyleCB)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLineStyleCB message handlers

BOOL CLineStyleCB::PreCreateWindow(CREATESTRUCT& cs) 
{
	cs.style &= ~(CBS_OWNERDRAWVARIABLE | CBS_SORT);
	cs.style |= CBS_OWNERDRAWFIXED;
	cs.style &= ~CBS_DROPDOWN;
	cs.style |= CBS_DROPDOWNLIST;
	
	return CComboBox::PreCreateWindow(cs);
}

void CLineStyleCB::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
	CDC dc;
	dc.Attach( lpDrawItemStruct->hDC );

	CRect rect(lpDrawItemStruct->rcItem);
	UINT nIndex = lpDrawItemStruct->itemID;


	dc.FillSolidRect(rect, RGB(255,255,255));
	if( lpDrawItemStruct->itemState & ODS_SELECTED )
		dc.DrawFocusRect(rect);

	
	if( nIndex != -1)
	{
		int y = (rect.top + rect.bottom)/2;
		CPen pen;
        CreateNewPen(pen, nIndex, dc);
            
		
		CPen* pOldPen = dc.SelectObject(&pen);
		dc.MoveTo(rect.left, y);
		dc.LineTo(rect.right, y);
		dc.SelectObject(pOldPen);
	}




	dc.Detach();
}

void CLineStyleCB::CreateNewPen(CPen& pen, int linestyle, CDC& dc)
{
    switch( linestyle )
    {
    case DOT: linestyle = PS_DOT; break;
    case DASHDOT: linestyle = PS_DASH; break;
    case SHORT_DASH: linestyle = PS_DASHDOT; break;
    case LONG_DASH: linestyle = PS_DASHDOTDOT; break;
    case SOLID: linestyle = PS_SOLID; break;
	case NO_LINE: linestyle = PS_NULL; break;
    default: ASSERT(false);
    }

    pen.CreatePen( linestyle, 1, RGB(0,0,0));
}


void CLineStyleCB::AddItems()
{
	ResetContent();

	for (int i = 0; i<NB_LINE_STYLE; i++)
	{
		AddString(_T(""));
		SetItemData (i, i);
	}
}

void CLineStyleCB::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct) 
{
	lpMeasureItemStruct->itemHeight = 15;	
}
