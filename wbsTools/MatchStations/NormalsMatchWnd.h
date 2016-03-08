
// BioSIMView.h : interface of the CNormalsListCtrl class
//


#pragma once


#include "UI/StationsListCtrl.h"
#include "UI/Common/CommonCtrl.h"


//**************************************************************************************************************************************

class CNormalsListToolBar : public CMFCToolBar
{
public:

	DECLARE_SERIAL(CNormalsListToolBar)

	virtual void OnUpdateCmdUI(CFrameWnd* /*pTarget*/, BOOL bDisableIfNoHndler)
	{
		CMFCToolBar::OnUpdateCmdUI((CFrameWnd*)GetOwner(), bDisableIfNoHndler);
	}
	virtual BOOL LoadState(LPCTSTR lpszProfileName = NULL, int nIndex = -1, UINT uiID = (UINT)-1){ return TRUE; }
	virtual BOOL SaveState(LPCTSTR lpszProfileName = NULL, int nIndex = -1, UINT uiID = (UINT)-1){ return TRUE; }
	virtual BOOL AllowShowOnList() const { return FALSE; }

	virtual BOOL LoadToolBarEx(UINT uiToolbarResID, CMFCToolBarInfo& params, BOOL bLocked = FALSE);
	virtual void AdjustLocations();
	void UpdateButton();
};



//**************************************************************************************************************************************
class CNormalsMatchWnd : public CDockablePane
{
	// Construction
public:
	CNormalsMatchWnd();
	virtual ~CNormalsMatchWnd();
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	void AdjustLayout();
	void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	
	WBSF::CMatchStationsCtrl m_wndNormalsList;

protected:

	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
	afx_msg void OnWindowPosChanged(WINDOWPOS* lpwndpos);

	CFont m_font;
	bool m_bMustBeUpdated;
	void SetPropListFont();

	DECLARE_MESSAGE_MAP()
};

