
// BioSIMView.h : interface of the CNormalsChartWnd class
//


#pragma once

#include <unordered_map>
#include "ChartCtrl/ChartCtrl.h"
#include "Basic/WeatherDatabase.h"
#include "Simulation/Graph.h"
#include "UI/Common/CommonCtrl.h"
#include "UI/Common/ScrollHelper.h"
#include "UI/Common/SplitterControl.h"
#include "UI/NormalsChartsCtrl.h"

class CNormalsEditorDoc;

//**************************************************************************************************************************************

class CNormalsChartToolBar : public CMFCToolBar
{
public:
	
	DECLARE_SERIAL(CNormalsChartToolBar)
	virtual void OnUpdateCmdUI(CFrameWnd* /*pTarget*/, BOOL bDisableIfNoHndler)
	{
		CMFCToolBar::OnUpdateCmdUI((CFrameWnd*) GetOwner(), bDisableIfNoHndler);
	}
	virtual BOOL LoadState(LPCTSTR lpszProfileName = NULL, int nIndex = -1, UINT uiID = (UINT) -1){	return TRUE;}
	virtual BOOL SaveState(LPCTSTR lpszProfileName = NULL, int nIndex = -1, UINT uiID = (UINT) -1){	return TRUE;}
	virtual BOOL AllowShowOnList() const { return FALSE; }
	virtual BOOL LoadToolBarEx(UINT uiToolbarResID, CMFCToolBarInfo& params, BOOL bLocked = FALSE);
};



class CNormalsChartWnd : public CDockablePane
{
	DECLARE_DYNCREATE(CNormalsChartWnd)

public:

	static CNormalsEditorDoc* GetDocument();
	static WBSF::CWeatherDatabasePtr GetDatabasePtr();

	CNormalsChartWnd();
	virtual ~CNormalsChartWnd();
	
	void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnWindowPosChanged(WINDOWPOS* lpwndpos);
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);

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



	void AdjustLayout();
	void CreateToolBar();
	

	WBSF::CNormalsChartsCtrl m_normalsChartsCtrl;
	CNormalsChartToolBar m_wndToolBar;
	

	bool m_bMustBeUpdated;


};



