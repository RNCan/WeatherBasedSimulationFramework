
#pragma once

#include "UI/Common/CommonCtrl.h"

/////////////////////////////////////////////////////////////////////////////
// Fenêtre de COutputList
class COutputWnd : public CDockablePane
{
// Construction
public:
	COutputWnd();

	void UpdateFonts();
	void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);

// Attributs
protected:
	//CMFCTabCtrl	m_wndTabs;

	CReadOnlyEdit m_wndOutput;

protected:

	void AdjustHorzScroll(CListBox& wndListBox);

// Implémentation
public:
	virtual ~COutputWnd();

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);

	DECLARE_MESSAGE_MAP()
};

