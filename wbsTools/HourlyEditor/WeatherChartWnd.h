
// BioSIMView.h : interface of the CWeatherChartWnd class
//


#pragma once

#include <unordered_map>
#include "Basic/WeatherDatabase.h"
#include "ChartCtrl/ChartCtrl.h"
#include "Simulation/Graph.h"
#include "UI/Common/CommonCtrl.h"
#include "UI/Common/ScrollHelper.h"
#include "UI/Common/SplitterControl.h"
#include "UI/WeatherChartsCtrl.h"

class CHourlyEditorDoc;

//**************************************************************************************************************************************

class CGraphToolBar : public CSplittedToolBar
{
public:
	
	DECLARE_SERIAL(CGraphToolBar)
	virtual BOOL LoadToolBarEx(UINT uiToolbarResID, CMFCToolBarInfo& params, BOOL bLocked = FALSE);
};


class CWeatherChartWnd : public CDockablePane
{
	// Attributes
public:


	static CHourlyEditorDoc* GetDocument();


	CWeatherChartWnd();

	virtual ~CWeatherChartWnd();
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	virtual void OnWindowPosChanged(WINDOWPOS* lpwndpos);
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);

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
	

	void AdjustLayout();
	void FillGraphList();
	void CreateToolBar();
	

	WBSF::CWeatherChartsCtrl m_weatherChartsCtrl;
	CGraphToolBar m_wndToolBar;
	

	bool m_bMustBeUpdated;
	bool m_bEnableMessage;

};



