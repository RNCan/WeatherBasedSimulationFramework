
#pragma once

//#include "ProjectView.h"
#include "Simulation/Executable.h"
#include "UI/ExecutableTree.h"

class CBioSIMDoc;

class CProjectViewToolBar : public CMFCToolBar
{
	virtual void OnUpdateCmdUI(CFrameWnd* /*pTarget*/, BOOL bDisableIfNoHndler)
	{
		CMFCToolBar::OnUpdateCmdUI((CFrameWnd*)GetOwner(), bDisableIfNoHndler);
	}

	virtual BOOL LoadState(LPCTSTR lpszProfileName = NULL, int nIndex = -1, UINT uiID = (UINT)-1){ return TRUE; }
	virtual BOOL SaveState(LPCTSTR lpszProfileName = NULL, int nIndex = -1, UINT uiID = (UINT)-1){ return TRUE; }

	virtual BOOL AllowShowOnList() const { return FALSE; }
};


class CProjectViewStatusBar : public CStatusBar
{
	virtual void OnUpdateCmdUI(CFrameWnd* /*pTarget*/, BOOL bDisableIfNoHndler)
	{
		CStatusBar::OnUpdateCmdUI((CFrameWnd*)GetOwner(), bDisableIfNoHndler);
	}

};

class CProjectView : public CView
{
	DECLARE_DYNCREATE(CProjectView)

public:
	
	CProjectView();
	~CProjectView();


	virtual CBioSIMDoc* GetDocument() const;
	virtual void OnDraw(CDC* pDC);
	virtual void OnInitialUpdate(); // called first time after construct
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);

	void AdjustLayout();
	
protected:

	WBSF::CExecutableTree	m_projectCtrl;
	CProjectViewToolBar		m_wndToolBar1;
	CProjectViewToolBar		m_wndToolBar2;
	CProjectViewStatusBar	m_wndStatusBar;
	

	
	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnPaint();
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnSelChange(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnExecute();
	afx_msg BOOL OnOpenWorkingDir(UINT ID);
	afx_msg void OnShowMaps();
	afx_msg void OnShowLoc();
	afx_msg void OnMatchStation();
	afx_msg void OnUpdateNbExecute(CCmdUI* pCmdUI);
	afx_msg void OnUpdateToolBar(CCmdUI *pCmdUI);
	
	afx_msg LRESULT OnCheckbox(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnItemExpanded(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnBeginDrag(WPARAM, LPARAM);
	afx_msg LRESULT OnEndDrag(WPARAM, LPARAM);
	afx_msg LRESULT OnDropHover(WPARAM, LPARAM);


#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

};

