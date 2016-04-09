// MainFrm.h : interface de la classe CMainFrame
//

#pragma once

#include "OutputWnd.h"
#include "PropertiesWnd.h"

#include "WeatherChartWnd.h"
#include "WeatherSpreadsheetWnd.h"



class CMainFrame : public CFrameWndEx
{
	
protected: // création à partir de la sérialisation uniquement
	CMainFrame();


// Attributs
public:

// Opérations
public:

	void ActivateFrame(int nCmdShow);
	void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	void LoadtBasicCommand();

	virtual ~CMainFrame();


protected:  // membres incorporés de la barre de contrôle


	CMFCMenuBar       m_wndMenuBar;
	CMFCToolBar       m_wndToolBar;
	CMFCStatusBar     m_wndStatusBar;
	
	CWeatherSpreadsheetWnd	m_spreadsheetWnd;
	CWeatherChartWnd	m_chartWnd;

	COutputWnd        m_wndOutput;
	CPropertiesWnd    m_wndProperties;

// Fonctions générées de la table des messages
protected:
	
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
//	afx_msg void OnViewCustomize();
//	afx_msg LRESULT OnToolbarCreateNew(WPARAM wp, LPARAM lp);
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


