
// MainFrm.h : interface de la classe CMainFrame
//

#pragma once

//#include "OutputWnd.h"
#include "PropertiesWnd.h"
#include "UI/Common/MFCEditBrowseCtrlEx.h"
#include "NormalsMatchWnd.h"
#include "ObservationMatchWnd.h"
#include "WeightWnd.h"
#include "InputWnd.h"
#include "NormalsGradientWnd.h"
#include "NormalsCorrectionWnd.h"
#include "NormalsEstimateWnd.h"
#include "ObservationEstimateWnd.h"
#include "LocationsListWnd.h"


//**************************************************************************************************************************************


class CMainFrame : public CFrameWndEx
{

protected: // création à partir de la sérialisation uniquement
	DECLARE_DYNCREATE(CMainFrame)
	CMainFrame();


	// Opérations
public:

	virtual ~CMainFrame();

	//void ActivateFrame(int nCmdShow);
	void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	void LoadtBasicCommand();

	CComPtr<ITaskbarList3>& GetTaskbarList(){ return m_pTaskbarList; }


protected:  // membres incorporés de la barre de contrôle

	CMFCMenuBar       m_wndMenuBar;
	CMFCToolBar       m_wndToolBar;
	CMFCStatusBar     m_wndStatusBar;
	
	CInputWnd				m_inputWnd;
	CLocationsListWnd		m_locationsWnd;
	CPropertiesWnd			m_propertiesWnd;
	CNormalsMatchWnd		m_normalsWnd;
	CObservationMatchWnd	m_observationWnd;
	CGradientChartWnd		m_weightChartsWnd;
	CNormalsGradientWnd		m_gradientWnd;
	CNormalsCorrectionWnd	m_correctionWnd;
	CNormalsEstimateWnd		m_estimateWnd;
	CObservationEstimateWnd m_obsEstimateWnd;

	static const UINT m_uTaskbarBtnCreatedMsg;
	CComPtr<ITaskbarList3> m_pTaskbarList;

	


	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnApplicationLook(UINT id);
	afx_msg void OnUpdateApplicationLook(CCmdUI* pCmdUI);
	afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
	afx_msg void OnLanguageChange(UINT id);
	afx_msg void OnLanguageUI(CCmdUI* pCmdUI);
	afx_msg void OnEditOptions();
	afx_msg LRESULT OnTaskbarProgress(WPARAM wParam, LPARAM lParam);
	afx_msg void OnNbNormalsChange();


	BOOL CreateDockingWindows();
	void SetDockingWindowIcons(BOOL bHiColorIcons);
	void AdjustDockingLayout(HDWP hdwp);

	DECLARE_MESSAGE_MAP()
	


#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
};


