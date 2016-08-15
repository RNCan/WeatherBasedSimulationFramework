#pragma once



// ProListCtrl.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CProListCtrl window

class CProListCtrl : public CListCtrl
{
// Construction
public:
	
	CProListCtrl();
	virtual ~CProListCtrl();

	void InvalidateProgressCtrls();
	int InsertItem(_In_ int nItem, _In_z_ LPCTSTR lpszItem, bool bFinite = true);
	BOOL DeleteItem(int nItem);
	void SetItemText(int i, const CString& text);
	// Generated message map functions
protected:
	
	DECLARE_MESSAGE_MAP()
	afx_msg void OnCustomDraw( NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnHScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnVScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnDestroy();
	
	
};

