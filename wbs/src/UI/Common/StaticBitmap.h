//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once

// StaticBitmap.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CStaticBitmap window

class CStaticBitmap : public CStatic
{
// Construction
public:
	CStaticBitmap();
    DECLARE_DYNCREATE(CStaticBitmap)
// Attributes
public:

    inline COLORREF GetTransparentColor()const;
    inline void SetTransparentColor( COLORREF transparentColor);

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CStaticBitmap)
	protected:
	virtual void PreSubclassWindow();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CStaticBitmap();

	// Generated message map functions
protected:
	//{{AFX_MSG(CStaticBitmap)
	afx_msg void OnPaint();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

private:

    void DrawTransparent (CDC* pDC, HBITMAP hBitmap, COLORREF crColor);
    COLORREF m_transparentColor;
};

inline COLORREF CStaticBitmap::GetTransparentColor()const
{
    return m_transparentColor;
}

inline void CStaticBitmap::SetTransparentColor( COLORREF transparentColor)
{
    m_transparentColor = transparentColor;
}
