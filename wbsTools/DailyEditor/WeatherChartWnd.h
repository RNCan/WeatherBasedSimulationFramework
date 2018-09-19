
// BioSIMView.h : interface of the CWeatherChartWnd class
//


#pragma once

#include <unordered_map>
#include "ChartCtrl/ChartCtrl.h"
#include "Simulation/Graph.h"
#include "Basic/WeatherDatabase.h"
#include "UI/Common/CommonCtrl.h"
#include "UI/Common/ScrollHelper.h"
#include "UI/Common/SplitterControl.h"
#include "UI/WeatherChartsCtrl.h"

class CDailyEditorDoc;

//**************************************************************************************************************************************

class CWeatherChartToolBar : public CSplittedToolBar
{
public:
	
	DECLARE_SERIAL(CWeatherChartToolBar)
	virtual void OnUpdateCmdUI(CFrameWnd* /*pTarget*/, BOOL bDisableIfNoHndler)
	{
		CSplittedToolBar::OnUpdateCmdUI((CFrameWnd*)GetOwner(), bDisableIfNoHndler);
	}
	virtual BOOL LoadState(LPCTSTR lpszProfileName = NULL, int nIndex = -1, UINT uiID = (UINT)-1) { return TRUE; }
	virtual BOOL SaveState(LPCTSTR lpszProfileName = NULL, int nIndex = -1, UINT uiID = (UINT)-1) { return TRUE; }
	virtual BOOL AllowShowOnList() const { return FALSE; }
	virtual BOOL LoadToolBarEx(UINT uiToolbarResID, CMFCToolBarInfo& params, BOOL bLocked = FALSE);

	DECLARE_MESSAGE_MAP()
};



class CWeatherChartWnd : public CDockablePane
{

public:

	CWeatherChartWnd();
	virtual ~CWeatherChartWnd();



	
	void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	virtual BOOL CWeatherChartWnd::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnWindowPosChanged(WINDOWPOS* lpwndpos);

// Generated message map functions
protected:

	DECLARE_MESSAGE_MAP()

	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnUpdateToolbar(CCmdUI *pCmdUI);
	afx_msg void OnToolbarCommand(UINT ID, NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnToolbarCommand(UINT ID);
	afx_msg void OnCopyGraph();
	afx_msg void OnSaveGraph();
	afx_msg void OnGraphOptions();
	afx_msg void OnDateChange(UINT);


	void AdjustLayout();
	void CreateToolBar();
	

	WBSF::CWeatherChartsCtrl m_weatherChartsCtrl;
	CWeatherChartToolBar m_wndToolBar;
	bool m_bMustBeUpdated;
	bool m_bEnableMessage;

};



