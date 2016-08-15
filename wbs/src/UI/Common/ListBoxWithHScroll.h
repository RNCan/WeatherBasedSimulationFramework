//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once


#include "UI/Common/UtilWin.h"

/////////////////////////////////////////////////////////////////////////////
// CListBoxWithHScroll window

class CListBoxWithHScroll : public CListBox
{
// Construction
public:
	CListBoxWithHScroll();

// Attributes
public:

// Operations
public:

	int AddString(const std::string& in){ return AddString( UtilWin::Convert(in.c_str()) ); }
	
	using CListBox::GetText;
	std::string GetText(int nIndex){ CString tmp; CListBox::GetText(nIndex, tmp); return UtilWin::ToUTF8(tmp); }

	int GetListboxStringExtent();
	void UpdateHScroll();
	int GetStringExtent(LPCTSTR lpszItem);
	int AddString( LPCTSTR lpszItem );
	int DeleteString( UINT nIndex );
	int InsertString( int nIndex, LPCTSTR lpszItem );
	void ResetContent( );

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CListBoxWithHScroll)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CListBoxWithHScroll();

	// Generated message map functions
protected:
	
	//{{AFX_MSG(CListBoxWithHScroll)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

