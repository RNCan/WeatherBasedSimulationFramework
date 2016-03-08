// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#pragma once

#include "UI/LocationVectorCtrl.h"
#include "UI/Common/CommonCtrl.h"



namespace WBSF
{

	class CMatchStationDoc;




	//**************************************************************************************************************************************
	class CMainToolBar : public CMFCToolBar
	{
	public:

		DECLARE_SERIAL(CMainToolBar)

		virtual void OnUpdateCmdUI(CFrameWnd* /*pTarget*/, BOOL bDisableIfNoHndler)
		{
			CMFCToolBar::OnUpdateCmdUI((CFrameWnd*)GetOwner(), bDisableIfNoHndler);
		}
		//virtual BOOL LoadState(LPCTSTR lpszProfileName = NULL, int nIndex = -1, UINT uiID = (UINT)-1){ return TRUE; }
		//virtual BOOL SaveState(LPCTSTR lpszProfileName = NULL, int nIndex = -1, UINT uiID = (UINT)-1){ return TRUE; }
		virtual BOOL AllowShowOnList() const { return TRUE; }

		virtual BOOL LoadToolBarEx(UINT uiToolbarResID, CMFCToolBarInfo& params, BOOL bLocked = FALSE);
		//virtual void AdjustLocations();
		void UpdateButton();
	};



	class CLocationsListView : public CView
	{
	protected: // create from serialization only
		CLocationsListView();
		DECLARE_DYNCREATE(CLocationsListView)

		// Attributes
	public:
		CMatchStationDoc* GetDocument();

		// Operations
	public:

		// Implementation
	public:
		virtual ~CLocationsListView();

#ifdef _DEBUG
		virtual void AssertValid() const;
		virtual void Dump(CDumpContext& dc) const;
#endif

	protected:

		CLocationVectorCtrl		m_locationVectorCtrl;


		DECLARE_MESSAGE_MAP()
		afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
		afx_msg void OnContextMenu(CWnd*, CPoint point);
		afx_msg LRESULT OnSetText(WPARAM wParam, LPARAM lParam);
		afx_msg void OnSize(UINT nType, int cx, int cy);
		afx_msg void OnUpdateStatusBar(CCmdUI* pCmdUI);
		afx_msg LRESULT  OnSelectionChange(WPARAM, LPARAM);
		afx_msg void OnSearchPropertyChange(UINT id);
		afx_msg void OnUpdateSearchProperty(CCmdUI* pCmdUI);


		virtual BOOL PreTranslateMessage(MSG* pMsg);
		virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
		virtual void OnInitialUpdate();

		void OnDraw(CDC* pDC);
		void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
		void AdjustLayout();
	};


	inline CMatchStationDoc* CLocationsListView::GetDocument()
	{
		return (CMatchStationDoc*)m_pDocument;
	}

}