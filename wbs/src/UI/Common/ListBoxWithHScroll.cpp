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
#include "ListBoxWithHScroll.h"
#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CListBoxWithHScroll

CListBoxWithHScroll::CListBoxWithHScroll()
{
}

CListBoxWithHScroll::~CListBoxWithHScroll()
{
}


BEGIN_MESSAGE_MAP(CListBoxWithHScroll, CListBox)
	//{{AFX_MSG_MAP(CListBoxWithHScroll)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CListBoxWithHScroll message handlers

void CListBoxWithHScroll::UpdateHScroll()
{
	this->SetHorizontalExtent( GetListboxStringExtent() );
}

int CListBoxWithHScroll::GetListboxStringExtent()
{
    CString tmp;
    CDC* pDC = GetDC();
    ASSERT( pDC != NULL );
    CFont* pFont = GetFont();
    if( pFont )
        pDC->SelectObject( pFont );

    int extent=0;
    for(int i=0; i<GetCount(); i++ )
    {
        GetText(i, tmp);
    
        
        // Add one average text width to insure that we see the end of the
        // string when scrolled horizontally.
         
        CSize size = pDC->GetTextExtent( tmp );
        
        if( size.cx > extent )
            extent = size.cx;
    }

    ReleaseDC(pDC);

    return extent + 5;
}

int CListBoxWithHScroll::GetStringExtent(LPCTSTR lpszItem)
{
	CDC* pDC = GetDC();
    ASSERT( pDC != NULL );
    CFont* pFont = GetFont();
    if( pFont )
        pDC->SelectObject( pFont );

    int extent=0;
     
    CSize size = pDC->GetTextExtent( lpszItem );

    ReleaseDC(pDC);

    return size.cx + 5;
}

BOOL CListBoxWithHScroll::PreCreateWindow(CREATESTRUCT& cs) 
{
	cs.dwExStyle |= WS_HSCROLL;
	return CListBox::PreCreateWindow(cs);
}

int CListBoxWithHScroll::AddString( LPCTSTR lpszItem )
{
	int extent = std::max( GetStringExtent(lpszItem ), GetHorizontalExtent() );
	SetHorizontalExtent( extent );

	return CListBox::AddString( lpszItem );
}

int CListBoxWithHScroll::DeleteString( UINT nIndex )
{
	CString tmp;
	GetText(nIndex, tmp);

	int rep = CListBox::DeleteString( nIndex );

	int extent = GetHorizontalExtent();
	if( GetStringExtent(tmp) >= extent )
		UpdateHScroll();

	return rep;
}

int CListBoxWithHScroll::InsertString( int nIndex, LPCTSTR lpszItem )
{
	int extent = std::max( GetStringExtent(lpszItem ), GetHorizontalExtent() );
	SetHorizontalExtent( extent );

	return CListBox::InsertString( nIndex, lpszItem );
}

void CListBoxWithHScroll::ResetContent( )
{
	SetHorizontalExtent( 0 );

	CListBox::ResetContent( );
}

