
#pragma once

/////////////////////////////////////////////////////////////////////////////
// Fenêtre de COutputList

class COutputEdit : public CEdit
{
// Construction
public:
	COutputEdit();
	virtual ~COutputEdit();
	

protected:

	DECLARE_MESSAGE_MAP()
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnEditCopy();
	afx_msg void OnEditClear();
	afx_msg void OnViewOutput();

	
};

class COutputWnd : public CDockablePane
{
// Construction
public:
	COutputWnd();
	virtual ~COutputWnd();


	void UpdateFonts();
	void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	void AdjustHorzScroll(CListBox& wndListBox);


protected:


	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);


	COutputEdit m_wndOutput;

};

