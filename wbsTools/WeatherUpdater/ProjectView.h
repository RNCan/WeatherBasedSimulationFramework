#pragma once

//#include "Simulation/Executable.h"
#include "UI/VariableSelectionCtrl.h"


class CWeatherUpdaterDoc;

class CProjectWndToolBar : public CMFCToolBar
{
	virtual void OnUpdateCmdUI(CFrameWnd* /*pTarget*/, BOOL bDisableIfNoHndler)
	{
		CMFCToolBar::OnUpdateCmdUI((CFrameWnd*)GetOwner(), bDisableIfNoHndler);
	}

	virtual BOOL LoadState(LPCTSTR lpszProfileName = NULL, int nIndex = -1, UINT uiID = (UINT)-1){ return TRUE; }
	virtual BOOL SaveState(LPCTSTR lpszProfileName = NULL, int nIndex = -1, UINT uiID = (UINT)-1){ return TRUE; }

	virtual BOOL AllowShowOnList() const { return FALSE; }
};




class CProjectWndStatusBar : public CStatusBar
{
	virtual void OnUpdateCmdUI(CFrameWnd* /*pTarget*/, BOOL bDisableIfNoHndler)
	{
		CStatusBar::OnUpdateCmdUI((CFrameWnd*)GetOwner(), bDisableIfNoHndler);
	}

};



class CTaskWnd : public CWnd
{
public:

CTaskWnd(UINT ctrlID = 1001, UINT toolbarID = 1002);
		

UINT m_ctrlID;
UINT m_toolbarID;


CFont m_fntPropList;
CProjectWndToolBar		m_wndToolBar;
WBSF::CStatisticSelectionCtrl	m_taskCtrl;
CProjectWndStatusBar	m_wndStatusBar;


DECLARE_MESSAGE_MAP()
afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
afx_msg void OnSize(UINT nType, int cx, int cy);
afx_msg void OnUpdateStatusBar(CCmdUI* pCmdUI);
afx_msg LRESULT OnCheckbox(WPARAM wParam, LPARAM lParam);
afx_msg LRESULT OnItemExpanded(WPARAM wParam, LPARAM lParam);
afx_msg LRESULT OnBeginDrag(WPARAM, LPARAM);
afx_msg LRESULT OnEndDrag(WPARAM, LPARAM);
afx_msg LRESULT OnDropHover(WPARAM, LPARAM);

	
void AdjustLayout();
};

class CProjectView : public CView
{
	DECLARE_DYNCREATE(CProjectView)
		
public:

	CProjectView();
	~CProjectView();

	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	virtual void OnDraw(CDC* pDC);
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);


	void AdjustLayout();



protected:

		
	CTaskWnd m_wnd1;
	CTaskWnd m_wnd2;
	CPaneSplitter m_wndSplitter;

	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnSelChange(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnExecute();
	afx_msg BOOL OnOpenWorkingDir(UINT ID);
	afx_msg void OnUpdateNbExecute(CCmdUI* pCmdUI);
		
		
	afx_msg void OnUpdateToolbar(CCmdUI *pCmdUI);
	afx_msg void OnToolbarCommand(UINT ID);
	afx_msg void OnAddTask();

};

