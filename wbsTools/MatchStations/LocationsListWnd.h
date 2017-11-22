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



class CMatchStationDoc;




//**************************************************************************************************************************************
//class CMainToolBar : public CMFCToolBar
//{
//public:
//
//	DECLARE_SERIAL(CMainToolBar)
//
//	virtual void OnUpdateCmdUI(CFrameWnd* /*pTarget*/, BOOL bDisableIfNoHndler)
//	{
//		CMFCToolBar::OnUpdateCmdUI((CFrameWnd*)GetOwner(), bDisableIfNoHndler);
//	}
////	virtual BOOL LoadState(LPCTSTR lpszProfileName = NULL, int nIndex = -1, UINT uiID = (UINT)-1){ return TRUE; }
//	//virtual BOOL SaveState(LPCTSTR lpszProfileName = NULL, int nIndex = -1, UINT uiID = (UINT)-1){ return TRUE; }
//	virtual BOOL AllowShowOnList() const { return TRUE; }
//	virtual BOOL LoadToolBarEx(UINT uiToolbarResID, CMFCToolBarInfo& params, BOOL bLocked = FALSE);
//};

//*************************************************************************************************************
class CStationsListStatusBar : public CStatusBar
{
	virtual void OnUpdateCmdUI(CFrameWnd* /*pTarget*/, BOOL bDisableIfNoHndler)
	{
		CStatusBar::OnUpdateCmdUI((CFrameWnd*)GetOwner(), bDisableIfNoHndler);
	}
};
//*************************************************************************************************************

class CLocationsListWnd : public CDockablePane
{
	DECLARE_DYNCREATE(CLocationsListWnd)

	// Attributes
public:

	static CMatchStationDoc* GetDocument();

	CLocationsListWnd();
	virtual ~CLocationsListWnd();

	void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	void AdjustLayout();

protected:

	WBSF::CLocationVectorCtrl	m_locationVectorCtrl;
	CStationsListStatusBar		m_statusCtrl;

	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnContextMenu(CWnd*, CPoint point);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnUpdateStatusBar(CCmdUI* pCmdUI);
	afx_msg LRESULT  OnSelectionChange(WPARAM, LPARAM);


	virtual BOOL PreTranslateMessage(MSG* pMsg);
	

};


