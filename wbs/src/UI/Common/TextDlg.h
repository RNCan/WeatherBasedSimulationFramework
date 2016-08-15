//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once

// TextDlg.h : header file
//

class CDialogItem
{
public:
	// define the enum with values to match whatever DLGITEMTEMPLATE requires

	DLGITEMTEMPLATE  m_dlgItemTemplate;

	enum            TControl{BUTTON = 0x0080, EDITCONTROL, STATICTEXT};
	TControl        m_controltype;
	CStringA         m_strCaption;

public:
	//CDialogItem(enum controltype cType);  // default constructor will fill in default values
	CDialogItem() {};  // default constructor, not to be called directly

	void Initialize(TControl ctrltype, CRect& prect, LPCTSTR pszCaption, UINT nID);
};

/////////////////////////////////////////////////////////////////////////////
// CTextDlg dialog

class CTextDlg : public CDialog
{
// Construction
public:
	CTextDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CTextDlg();
	static const int ID_TEXTE_CTRL;

// Dialog Data
	//{{AFX_DATA(CTextDlg)
//	enum { IDD = IDD_TEXT_DIALOG };
	CString	m_texte;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTextDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

// Implementation
protected:

	void InitTemplateHandle();
	// Generated message map functions
	//{{AFX_MSG(CTextDlg)
	afx_msg void OnOk();
	virtual void OnCancel();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg int OnCreate( LPCREATESTRUCT lpcs);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	CDialogItem m_dlgItem[1];
	inline CEdit& GetTextCtrl();

	HLOCAL m_hLocal;
	DLGTEMPLATE m_dlgTempl;
	CStringA m_strCaption;
};

inline CEdit& CTextDlg::GetTextCtrl()
{
    return (CEdit&) (*GetDlgItem(ID_TEXTE_CTRL));

}
