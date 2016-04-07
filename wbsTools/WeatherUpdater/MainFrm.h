// MainFrm.h : interface de la classe CMainFrame
//

#pragma once

#include "OutputWnd.h"
#include "ParametersWnd.h"
#include "UI/Common/ProgressDockablePane.h"


class CMainFrame : public CFrameWndEx
{

protected: // création à partir de la sérialisation uniquement
	
	DECLARE_DYNCREATE(CMainFrame)
	CMainFrame();

public:

	virtual ~CMainFrame();

	void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	void LoadtBasicCommand();

	CComPtr<ITaskbarList3>& GetTaskbarList(){ return m_pTaskbarList; }
	CProgressDockablePane& GetProgressPane(){ return m_progressWnd; }
	CDockablePane* GetActivePane();

protected:  // membres incorporés de la barre de contrôle

	CMFCMenuBar       m_wndMenuBar;
	CMFCToolBar       m_wndToolBar;
	CMFCStatusBar     m_wndStatusBar;
	COutputWnd        m_wndOutput;
	CTaskPropertyWnd  m_wndProperties;
	CProgressDockablePane m_progressWnd;


	static const UINT m_uTaskbarBtnCreatedMsg;
	CComPtr<ITaskbarList3> m_pTaskbarList;

	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	// Fonctions générées de la table des messages
protected:

	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnApplicationLook(UINT id);
	afx_msg void OnUpdateApplicationLook(CCmdUI* pCmdUI);
	afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
	afx_msg void OnLanguageChange(UINT id);
	afx_msg void OnUpdateToolbar(CCmdUI* pCmdUI);
	afx_msg void OnEditOptions();
	afx_msg LRESULT OnTaskbarProgress(WPARAM wParam, LPARAM lParam);
	


	BOOL CreateDockingWindows();
	void SetDockingWindowIcons(BOOL bHiColorIcons);



#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

};


