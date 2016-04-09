
// BioSIMView.h : interface of the CMatchStationView class
//


#pragma once

#include "ResultCtrl.h"
#include "afxcview.h"



class CMatchStationView : public CListView
{
protected: // create from serialization only
	CMatchStationView();
	virtual ~CMatchStationView();

	DECLARE_DYNCREATE(CMatchStationView)

// Attributes
public:
	CBioSIMDoc* GetDocument() const;

// Operations
public:

// Overrides
public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
protected:

// Implementation
protected:

// Generated message map functions
protected:
	afx_msg void OnFilePrintPreview();
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	DECLARE_MESSAGE_MAP()


//	CFont m_Font;
	//CResultCtrl m_grid;
	

public:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnExecute();
	afx_msg void OnUpdateExecute(CCmdUI *pCmdUI);

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

};

#ifndef _DEBUG  // debug version in BioSIMView.cpp
inline CBioSIMDoc* CMatchStationView::GetDocument() const
   { return reinterpret_cast<CBioSIMDoc*>(m_pDocument); }
#endif

