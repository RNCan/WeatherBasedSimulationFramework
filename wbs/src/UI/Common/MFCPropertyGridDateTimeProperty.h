//******************************************************************************
//Fisier: GridProperties.h
//Creat : M.N. / 18.09.2008
//
//		Clasele CMFCPropertyGridDateTimeProperty si CMFCPropertyGridIPAdressProperty extind
//		controlul MFC CMFCPropertyGridCtrl prin adaugarea de campuri de tip data si adresa IP.
//
//		Copyright(C) 2008 ny Mugurel Nechita(Web : nmsoft.3x.ro, Email : nechitam@gmail.com
//
//		Detalii de utilizare :
//******************************************************************************
#pragma once

//#include <Winsock2.h> 

//class CNotifyEdit : public CEdit
//{
//	// Construction 
//public:
//	CNotifyEdit();
//	virtual ~CNotifyEdit();
//public:
//	DECLARE_MESSAGE_MAP()
//protected:
//	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
//};

class CMFCPropertyGridDateTimeProperty;

class CPropDateTimeCtrl : public CDateTimeCtrl
{
	// Construction 
public:
	CPropDateTimeCtrl(CMFCPropertyGridDateTimeProperty *pProp, COLORREF clrBack);

protected:
	CBrush m_brBackground;
	COLORREF m_clrBack;
	CMFCPropertyGridDateTimeProperty *m_pProp;

public:
	DECLARE_MESSAGE_MAP()

	afx_msg HBRUSH CtlColor(CDC* pDC, UINT nCtlColor);
	afx_msg void HScroll(UINT nSBCode, UINT nPos);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnDestroy();
protected:
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
};

class CMFCPropertyGridDateTimeProperty : public CMFCPropertyGridProperty
{
	DECLARE_DYNAMIC(CMFCPropertyGridDateTimeProperty)

	BOOL m_style;
	BOOL m_updown;
	CString m_setformat;
	CString m_format;
public:
	CMFCPropertyGridDateTimeProperty(const CString& strName, COleDateTime &nValue, LPCTSTR lpszDescr = NULL, DWORD dwData = 0, BOOL style = TRUE, BOOL updown = FALSE, LPCTSTR setFormat = NULL, LPCTSTR format = NULL);

	virtual BOOL OnUpdateValue();
	virtual CString FormatProperty();
	virtual CString FormatOriginalProperty();

protected:
	virtual CWnd* CreateInPlaceEdit(CRect rectEdit, BOOL& bDefaultFormat);
	virtual BOOL OnSetCursor() const { return FALSE; /* Use default */ }
	BOOL IsValueChanged() const;
};
