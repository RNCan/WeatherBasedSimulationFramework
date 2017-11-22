
// BioSIMView.h : interface of the CDPTView class
//


#pragma once

#include "ResultCtrl.h"
#include "CommonCtrl.h"



class CResultToolBar : public CMFCToolBar
{
public:
	virtual void OnUpdateCmdUI(CFrameWnd* /*pTarget*/, BOOL bDisableIfNoHndler)
	{
		CMFCToolBar::OnUpdateCmdUI((CFrameWnd*) GetOwner(), bDisableIfNoHndler);
	}
	virtual BOOL LoadState(LPCTSTR lpszProfileName = NULL, int nIndex = -1, UINT uiID = (UINT) -1){	return TRUE;}
	virtual BOOL SaveState(LPCTSTR lpszProfileName = NULL, int nIndex = -1, UINT uiID = (UINT) -1){	return TRUE;}
	virtual BOOL AllowShowOnList() const { return FALSE; }
};



class CDPTView : public CView//: public CBCGPGridView
{
protected: // create from serialization only
	CDPTView();
	virtual ~CDPTView();

	DECLARE_DYNCREATE(CDPTView)

// Attributes
public:
	CMatchStationDoc* GetDocument() const;

	CResultCtrl* GetGridCtrl ()	{		return &m_grid;	}

// Overrides
protected:
	virtual void OnInitialUpdate(); // called first time after construct
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	
	//virtual CScrollBar* GetScrollBarCtrl(int nBar) const;

	//void OnWindowPosChanging(WINDOWPOS FAR* lpwndpos) ;
	//void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	//CMFCTabCtrl* GetParentTab () const;

// Implementation
protected:

// Generated message map functions
protected:
	//afx_msg void OnFilePrintPreview();
	//afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	//afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnStatisticChange();
	afx_msg void OnWindowPosChanged(WINDOWPOS* lpwndpos);
	afx_msg void OnCopy();
	afx_msg void OnPaste();
	void AdjustLayout();
	//void UpdateResult();

	DECLARE_MESSAGE_MAP()

	//virtual CBCGPGridCtrl* CreateGrid ();
	virtual CUGCtrl* CreateGrid ();

	//CFont m_font;
	CResultCtrl m_grid;
	//CStatisticComboBox m_statisticCtrl;
	//CStatic m_validityCtrl;
	//CResultToolBar m_wndToolBar;

	//bool m_bMustBeUpdated;

public:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnExecute();
	afx_msg void OnUpdateExecute(CCmdUI *pCmdUI);
	afx_msg void OnCommandUI(UINT ID);
	afx_msg void OnUpdateUI(CCmdUI *pCmdUI);
	//void OnShowMapClicked();

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	
	
};

#ifndef _DEBUG  // debug version in BioSIMView.cpp
inline CMatchStationDoc* CDPTView::GetDocument() const
{
	return reinterpret_cast<CMatchStationDoc*>(m_pDocument);
}
#endif

