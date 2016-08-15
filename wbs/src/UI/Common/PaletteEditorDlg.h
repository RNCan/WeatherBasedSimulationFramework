//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once

#include "ColorListBox.h"
#include "WeatherBasedSimulationUI.h"

/////////////////////////////////////////////////////////////////////////////
// CPaletteEditorDlg dialog

class CPaletteEditorDlg : public CDialog
{
// Construction
public:
	CPaletteEditorDlg(const CString& path, CWnd* pParent = NULL);   // standard constructor

    inline const CString& GetPaletteName();
    inline void SetPaletteName(const CString& palette);

// Dialog Data
	//{{AFX_DATA(CPaletteEditorDlg)
	enum { IDD = IDD_CMN_PALETTE_EDITOR };
	CButton	m_deleteCtrl;
	CColorListBox	m_rangeColorList;
	CColorListBox	m_fixedColorList;
	CListBox	m_paletteList;
	//}}AFX_DATA


    //CBitmapButton	m_upCtrl;
	//CBitmapButton	m_downCtrl;
//	CBitmapButton	m_deleteProfileCtrl;
//	CBitmapButton	m_newProfileCtrl;
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPaletteEditorDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

    CToolBar m_wndToolBar;
    inline CListBox& GetPaletteListCtrl();
    inline CColorListBox& GetRangeColorListCtrl();
    inline CColorListBox& GetFixedColorListCtrl();
	// Generated message map functions
	//{{AFX_MSG(CPaletteEditorDlg)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnNewPalette();
	afx_msg void OnDeletePalette();
	afx_msg void OnImportPalette();
	virtual BOOL OnInitDialog();
	afx_msg void OnSelPaletteChange();
	afx_msg void OnSelRangeColorChange();
	afx_msg void OnSelFixedColorChange();
	afx_msg void OnNewProfile();
	afx_msg void OnDeleteProfile();
	afx_msg void OnProfileUp();
	afx_msg void OnProfileDown();
    afx_msg void OnPaletteStyleChange(UINT ID);
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

    
private:

	inline CButton& GetUpCtrl();
	inline CButton& GetDownCtrl();
	inline CButton& GetNewCtrl();
	inline CButton& GetDeleteCtrl();

    void UpdateButton();
    void FillPaletteList();
    void FillColorList(const CMyPalette& palette);
    CString GetPaletteFilePath(const CString& m_name);

    bool SavePalette();


    CString m_path;
    CString m_paletteName;

};


inline const CString& CPaletteEditorDlg::GetPaletteName()
{
    return m_paletteName;
}

inline void CPaletteEditorDlg::SetPaletteName(const CString& paletteName)
{
    m_paletteName = paletteName;
}

inline CListBox& CPaletteEditorDlg::GetPaletteListCtrl()
{
    ASSERT( GetDlgItem( IDC_PE_LIST )); 
    return (CListBox&) *GetDlgItem( IDC_PE_LIST );
}

inline CColorListBox& CPaletteEditorDlg::GetRangeColorListCtrl()
{
    ASSERT( GetDlgItem( IDC_PE_RANGE_COLOR )); 
    return (CColorListBox&) *GetDlgItem( IDC_PE_RANGE_COLOR );
}

inline CColorListBox& CPaletteEditorDlg::GetFixedColorListCtrl()
{
    ASSERT( GetDlgItem( IDC_PE_FIXED_COLOR )); 
    return (CColorListBox&) *GetDlgItem( IDC_PE_FIXED_COLOR );
}


inline CButton& CPaletteEditorDlg::GetUpCtrl()
{
	ASSERT( GetDlgItem(IDC_PE_UP));
	return (CButton&)(*GetDlgItem(IDC_PE_UP));
}

inline CButton& CPaletteEditorDlg::GetDownCtrl()
{
	ASSERT( GetDlgItem(IDC_PE_DOWN));
	return (CButton&)(*GetDlgItem(IDC_PE_DOWN));
}

inline CButton& CPaletteEditorDlg::GetNewCtrl()
{
	ASSERT( GetDlgItem(IDC_PE_COLOR_NEW));
	return (CButton&)(*GetDlgItem(IDC_PE_COLOR_NEW));
}

inline CButton& CPaletteEditorDlg::GetDeleteCtrl()
{
	ASSERT( GetDlgItem(IDC_PE_COLOR_DELETE));
	return (CButton&)(*GetDlgItem(IDC_PE_COLOR_DELETE));
}

