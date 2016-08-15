//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once

/////////////////////////////////////////////////////////////////////////////
// CLineStyleCB window

class CLineStyleCB : public CComboBox
{
// Construction
public:

	enum TLineStyle{ DOT, DASHDOT, SHORT_DASH, LONG_DASH, SOLID, NO_LINE, NB_LINE_STYLE};
    
	
	CLineStyleCB();

// Attributes
public:

// Operations
public:

	void AddItems();
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLineStyleCB)
	public:
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	virtual void MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct);
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CLineStyleCB();

	// Generated message map functions
protected:
    void CreateNewPen(CPen& pen, int linestyle, CDC& dc);

	//{{AFX_MSG(CLineStyleCB)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

    
};

