
// MainFrm.h : interface de la classe CMainFrame
//

#pragma once

#include "OutputWnd.h"
#include "PropertiesWnd.h"
#include "UI/Common/MFCEditBrowseCtrlEx.h"
#include "NormalsMatchWnd.h"
#include "ObservationMatchWnd.h"
#include "ChartsWnd.h"
#include "FilePathWnd.h"
#include "NormalsGradientWnd.h"
#include "NormalsCorrectionWnd.h"
#include "NormalsEstimateWnd.h"
#include "ObservationEstimateWnd.h"
#include "LocationsListView.h"


//**************************************************************************************************************************************


class CMainFrame : public CFrameWndEx
{

protected: // création à partir de la sérialisation uniquement
	CMainFrame();


	// Attributs
public:

	// Opérations
public:

	//void ActivateFrame(int nCmdShow);
	void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	void LoadtBasicCommand();

	// Substitutions
public:
	//virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL LoadFrame(UINT nIDResource, DWORD dwDefaultStyle = WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, CWnd* pParentWnd = NULL, CCreateContext* pContext = NULL);

	// Implémentation
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:  // membres incorporés de la barre de contrôle

	CMFCMenuBar       m_wndMenuBar;
	CMainToolBar      m_wndToolBar;
	CFilePathWnd	  m_wndFilePath;
	CMFCStatusBar     m_wndStatusBar;


	CNormalsMatchWnd		m_normalsWnd;
	CObservationMatchWnd	m_observationWnd;
	CGradientChartWnd		m_weightChartsWnd;
	CNormalsGradientWnd		m_gradientWnd;
	CNormalsCorrectionWnd	m_correctionWnd;
	CNormalsEstimateWnd		m_estimateWnd;
	CObservationEstimateWnd m_obsEstimateWnd;

	COutputWnd        m_wndOutput;
	CPropertiesWnd    m_wndProperties;

	// Fonctions générées de la table des messages
protected:



	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnViewCustomize();
	//afx_msg LRESULT OnToolbarCreateNew(WPARAM wp, LPARAM lp);
	afx_msg void OnApplicationLook(UINT id);
	afx_msg void OnUpdateApplicationLook(CCmdUI* pCmdUI);
	afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
	afx_msg void OnLanguageChange(UINT id);
	afx_msg void OnLanguageUI(CCmdUI* pCmdUI);
	afx_msg void OnToolbarChange(UINT id);
	afx_msg void OnToolbarUI(CCmdUI* pCmdUI);
	afx_msg void OnEditOptions();
	afx_msg LRESULT OnToolbarReset(WPARAM wp, LPARAM);
	afx_msg void OnNbNormalsChange();


	BOOL CreateDockingWindows();
	void SetDockingWindowIcons(BOOL bHiColorIcons);
	void AdjustDockingLayout(HDWP hdwp);

	DECLARE_MESSAGE_MAP()
	DECLARE_DYNCREATE(CMainFrame)
};


