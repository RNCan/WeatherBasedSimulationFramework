
#pragma once

#include "Basic/Callback.h"
#include "UI/Common/ProListCtrl.h"
#include "UI/Common/CommonCtrl.h"



class CProgressStepDlgParam
{
public:
	CProgressStepDlgParam(void* pThis = NULL, void* pFilepath = NULL, void* pExtra = NULL)
	{
		m_pThis = pThis;
		m_pFilepath = pFilepath;
		m_pExtra = pExtra;
		m_pCallback = NULL;
		m_pMsg = NULL;
	}

	void* m_pThis;
	void* m_pFilepath;
	void* m_pExtra;
	WBSF::CCallback* m_pCallback;
	ERMsg* m_pMsg;
};




class CProgressToolBar : public CMFCToolBar
{
	virtual void OnUpdateCmdUI(CFrameWnd* /*pTarget*/, BOOL bDisableIfNoHndler)
	{
		CMFCToolBar::OnUpdateCmdUI((CFrameWnd*) GetOwner(), bDisableIfNoHndler);
	}

	virtual BOOL AllowShowOnList() const { return FALSE; }
};

class CProgressWnd : public CWnd
{

	DECLARE_DYNCREATE(CProgressWnd)


// Construction
public:

	CProgressWnd();
	virtual ~CProgressWnd();
	
	void SetTaskbarList(CComPtr<ITaskbarList3> pTaskbarList){ m_pTaskbar = pTaskbarList; }
	ERMsg Execute(AFX_THREADPROC pfnThreadProc, CProgressStepDlgParam* pParam);
	
	void AdjustLayout();
	WBSF::CCallback& GetCallback(){ return m_callback; }
	void SetMessageCtrl(CEdit* pEdit){ m_pEdit = pEdit; }

	virtual void PreSubclassWindow();

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
	CProListCtrl m_progressListCtrl;

	WBSF::CCallback m_callback;
	std::string m_comment;
	CEdit* m_pEdit;
	
	CComPtr<ITaskbarList3> m_pTaskbar;
	CWinThread *m_ptrThread;
};

