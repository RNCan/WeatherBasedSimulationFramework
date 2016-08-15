//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once

// ColorListBox.h : header file
//
#include "MyPalette.h"
class CColorProfile;

/////////////////////////////////////////////////////////////////////////////
// CColorListBox window

class CColorListBox : public CListBox
{
// Construction
public:
    enum TColorListType { RANGE, FIXED };

	CColorListBox(TColorListType type=RANGE);

// Attributes
public:

// Operations
public:

    
    const CMyPalette& GetPalette();
    void SetPalette(const CMyPalette& palette);
    void AddNew();
    void DeleteSelected();
    void SelectionUp();
    void SelectionDown();
    bool SelectionDownEnable()const;
    bool SelectionUpEnable()const;
    
    // Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CColorListBox)
	public:
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	virtual void MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CColorListBox();

	// Generated message map functions
protected:
	//{{AFX_MSG(CColorListBox)
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnDestroy();
	//}}AFX_MSG

    void SetSelection(int* pSel, int nSize, int offset);
    void MoveString( int oldPos, int newPos );
    inline CRect GetColorRect(const CRect& rcItem)const;
    inline CRect GetBeginRect(const CRect& rcItem)const;
    inline CRect GetEndRect(const CRect& rcItem)const;
    inline CRect GetProgressRect(const CRect& rcItem)const;
    inline COLORREF GetColor( int index )const;
    inline void SetColor( int index, COLORREF color);
    inline CColorProfile& GetColorProfile( int index );

    void RetrievePalette();
    void FreeMemory();


    TColorListType m_type;
    CMyPalette m_palette;
    


	DECLARE_MESSAGE_MAP()
};



inline CRect CColorListBox::GetColorRect(const CRect& rcItem)const
{
    CRect rect(rcItem);
    rect.DeflateRect(2,2);
    return rect;

}
inline CRect CColorListBox::GetBeginRect(const CRect& rcItem)const
{
    CRect rect(rcItem);
    rect.DeflateRect(2,2);
    rect.right = rect.left + rect.Width()/4;
    return rect;
}

inline CRect CColorListBox::GetEndRect(const CRect& rcItem)const
{
    CRect rect(rcItem);
    rect.DeflateRect(2,2);
    rect.left = rect.right - rect.Width()/4;
    return rect;
}

inline CRect CColorListBox::GetProgressRect(const CRect& rcItem)const
{
    int size = rcItem.Width()/4 + 2;
    CRect rect(rcItem);
    rect.DeflateRect(size+2, 2, size+2, 2);
    
    return rect;
}

inline COLORREF CColorListBox::GetColor( int index )const
{
    ASSERT( index >=0 && index < GetCount() );
    ASSERT( m_type == FIXED );
    return (COLORREF )GetItemData(index);
}

inline void CColorListBox::SetColor( int index , COLORREF color)
{
    ASSERT( index >=0 && index < GetCount() );
    ASSERT( m_type == FIXED );
    SetItemData(index, (DWORD)color);
}


inline CColorProfile& CColorListBox::GetColorProfile( int index )
{
    ASSERT( index >=0 && index < GetCount() );
    CColorProfile* pInfo = (CColorProfile*)GetItemDataPtr(index);
    ASSERT( pInfo );

    return *pInfo;
}

