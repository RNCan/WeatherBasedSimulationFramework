
#pragma once

#include "Basic/Callback.h"
#include "UI/Common/ProListCtrl.h"
#include "UI/Common/ProgressStepDlg.h"
#include "UI/Common/CommonCtrl.h"

class CProgressToolBar : public CMFCToolBar
{
	virtual void OnUpdateCmdUI(CFrameWnd* /*pTarget*/, BOOL bDisableIfNoHndler)
	{
		CMFCToolBar::OnUpdateCmdUI((CFrameWnd*) GetOwner(), bDisableIfNoHndler);
	}

	virtual BOOL AllowShowOnList() const { return FALSE; }
};





class CProgressDockablePane : public CDockablePane
{
// Construction
public:

	CProgressDockablePane();
	virtual ~CProgressDockablePane();
	
	void SetTaskbarList(CComPtr<ITaskbarList3> pTaskbarList){ m_pTaskbar = pTaskbarList; }
	ERMsg Execute(AFX_THREADPROC pfnThreadProc, CProgressStepDlgParam* pParam);


	void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	void AdjustLayout();
	void OnChangeVisualStyle();
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);

	WBSF::CCallback& GetCallback(){ return m_callback; }

	
	
// Attributes
protected:



	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnProperties();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnCancel();
	afx_msg void OnPauseResume();
	afx_msg void OnItemHeadChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg LRESULT OnThreadMessage(WPARAM wParam, LPARAM);
	afx_msg void OnUpdateToolbar(CCmdUI* pCmdUI);

	CProgressToolBar m_toolbarCtrl;
	CPaneSplitter m_wndSplitter;
	CProListCtrl m_progressCtrl;
	CReadOnlyEdit m_messageCtrl;
	CFont m_font;
	WBSF::CCallback m_callback;
	std::string m_comment;
	
	CComPtr<ITaskbarList3> m_pTaskbar;
	CWinThread *m_ptrThread;
};

