// MainFrm.h : interface de la classe CMainFrame
//

#pragma once

#include "OutputWnd.h"
#include "TaskPropertiesWnd.h"



class CMainFrame : public CFrameWndEx
{

protected: // création à partir de la sérialisation uniquement
	
	DECLARE_DYNCREATE(CMainFrame)
	CMainFrame();

public:

	virtual ~CMainFrame();

	void ActivateFrame(int nCmdShow);
	void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	void LoadtBasicCommand();



//	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//virtual BOOL LoadFrame(UINT nIDResource, DWORD dwDefaultStyle = WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, CWnd* pParentWnd = NULL, CCreateContext* pContext = NULL);


protected:  // membres incorporés de la barre de contrôle

	CMFCMenuBar       m_wndMenuBar;
	CMFCToolBar       m_wndToolBar;
	CMFCStatusBar     m_wndStatusBar;

	COutputWnd        m_wndOutput;
	CTaskPropertyWnd  m_wndProperties;

	// Fonctions générées de la table des messages
protected:

	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//afx_msg void OnViewCustomize();
	//afx_msg LRESULT OnToolbarCreateNew(WPARAM wp, LPARAM lp);
	afx_msg void OnApplicationLook(UINT id);
	afx_msg void OnUpdateApplicationLook(CCmdUI* pCmdUI);
	afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
	afx_msg void OnLanguageChange(UINT id);
	afx_msg void OnLanguageUI(CCmdUI* pCmdUI);
	afx_msg void OnEditOptions();

	BOOL CreateDockingWindows();
	void SetDockingWindowIcons(BOOL bHiColorIcons);



#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

};


