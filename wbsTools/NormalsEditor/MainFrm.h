// MainFrm.h : interface de la classe CMainFrame
//

#pragma once

#include "OutputWnd.h"
#include "PropertiesWnd.h"

#include "NormalsChartWnd.h"
#include "NormalsSpreadsheetWnd.h"
//
//class CMainFrameMenuBar : public CMFCMenuBar
//{
//	public:
//
//		DECLARE_SERIAL(CMainFrameMenuBar)
//		virtual BOOL LoadState(LPCTSTR lpszProfileName = NULL, int nIndex = -1, UINT uiID = (UINT)-1){ return TRUE; }
//		virtual BOOL SaveState(LPCTSTR lpszProfileName = NULL, int nIndex = -1, UINT uiID = (UINT)-1){ return TRUE; }
//		virtual BOOL AllowShowOnList() const { return FALSE; }
//
//};
//
//class CMainFrameToolBar : public CMFCToolBar
//{
//public:
//
//	DECLARE_SERIAL(CMainFrameMenuBar)
//	virtual BOOL LoadState(LPCTSTR lpszProfileName = NULL, int nIndex = -1, UINT uiID = (UINT)-1){ return TRUE; }
//	virtual BOOL SaveState(LPCTSTR lpszProfileName = NULL, int nIndex = -1, UINT uiID = (UINT)-1){ return TRUE; }
//	virtual BOOL AllowShowOnList() const { return FALSE; }
//
//};

class CMainFrame : public CFrameWndEx
{
	
protected: // création à partir de la sérialisation uniquement

	CMainFrame();


// Opérations
public:

	void ActivateFrame(int nCmdShow);
	void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	void LoadtBasicCommand();


// Implémentation
public:
	virtual ~CMainFrame();

protected:  // membres incorporés de la barre de contrôle

	CMFCMenuBar m_wndMenuBar;
	CMFCToolBar m_wndToolBar;
	CMFCStatusBar     m_wndStatusBar;
	
	CNormalsSpreadsheetWnd	m_spreadsheetWnd;
	CNormalsChartWnd	m_chartWnd;

	COutputWnd        m_wndOutput;
	CPropertiesWnd    m_wndProperties;

// Fonctions générées de la table des messages
protected:


	
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnApplicationLook(UINT id);
	afx_msg void OnUpdateApplicationLook(CCmdUI* pCmdUI);
	afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
	afx_msg void OnLanguageChange(UINT id);
	afx_msg void OnLanguageUI(CCmdUI* pCmdUI);
	afx_msg void OnEditOptions();

	BOOL CreateDockingWindows();
	void SetDockingWindowIcons(BOOL bHiColorIcons);

	
	DECLARE_MESSAGE_MAP()
	DECLARE_DYNCREATE(CMainFrame)

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

};


