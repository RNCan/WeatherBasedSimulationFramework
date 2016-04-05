
// MainFrm.h : interface de la classe CMainFrame
//

#pragma once
#include "OutputWnd.h"
#include "PropertiesWnd.h"
#include "WeatherChartsWnd.h"
#include "WeatherTableWnd.h"

class CMFCToolBar2 : public CMFCToolBar
{
	//virtual int GetColumnWidth() const{ return 32; }

};

class CMainFrame : public CFrameWndEx
{
	
protected: // cr�ation � partir de la s�rialisation uniquement
	DECLARE_DYNCREATE(CMainFrame)
	CMainFrame();


public:

	void ActivateFrame(int nCmdShow);
	void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	void LoadBasicCommand();

	virtual ~CMainFrame();


protected:  // membres incorpor�s de la barre de contr�le
	CMFCMenuBar       m_wndMenuBar;
	CMFCToolBar       m_wndToolBar;
	CMFCStatusBar     m_wndStatusBar;

	CWeatherSpreadsheetWnd m_spreadsheetWnd;
	CWeatherChartWnd  m_chartWnd;
	COutputWnd        m_wndOutput;
	CPropertiesWnd    m_wndProperties;

// Fonctions g�n�r�es de la table des messages
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



#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif


};


