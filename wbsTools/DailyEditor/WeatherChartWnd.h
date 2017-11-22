
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
	virtual BOOL LoadToolBarEx(UINT uiToolbarResID, CMFCToolBarInfo& params, BOOL bLocked = FALSE);
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
	afx_msg void OnGraphChange();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnUpdateToolbar(CCmdUI *pCmdUI);
	afx_msg void OnToolbarCommand(UINT ID, NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnToolbarCommand(UINT ID);
	afx_msg void OnCopyGraph();
	afx_msg void OnSaveGraph();
	afx_msg void OnGraphOptions();
	afx_msg void OnDateChange(UINT);
	afx_msg void OnDestroy();


	void AdjustLayout();
	void FillGraphList();
	void CreateToolBar();
	

	WBSF::CWeatherChartsCtrl m_weatherChartsCtrl;
	CWeatherChartToolBar m_wndToolBar;
	

	bool m_bMustBeUpdated;


};



