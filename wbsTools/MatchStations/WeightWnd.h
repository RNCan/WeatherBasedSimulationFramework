
// BioSIMView.h : interface of the CWeightChartCtrl class
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


class CMatchStationDoc;


class CGraphToolBar : public CMFCToolBar
{
public:

	DECLARE_SERIAL(CGraphToolBar)
	virtual void OnUpdateCmdUI(CFrameWnd* /*pTarget*/, BOOL bDisableIfNoHndler)
	{
		CMFCToolBar::OnUpdateCmdUI((CFrameWnd*)GetOwner(), bDisableIfNoHndler);
	}
	virtual BOOL LoadState(LPCTSTR lpszProfileName = NULL, int nIndex = -1, UINT uiID = (UINT)-1){ return TRUE; }
	virtual BOOL SaveState(LPCTSTR lpszProfileName = NULL, int nIndex = -1, UINT uiID = (UINT)-1){ return TRUE; }
	virtual BOOL AllowShowOnList() const { return FALSE; }
	virtual BOOL LoadToolBarEx(UINT uiToolbarResID, CMFCToolBarInfo& params, BOOL bLocked = FALSE);
	virtual void AdjustLocations();

};




class CWeightChartCtrl : public CWnd
{
public:

	CWeightChartCtrl();
	int Create(CWnd *pParentWnd, const RECT &rect, UINT nID, DWORD dwStyle);

	//CTM m_TM;

	bool m_bPeriodEnable;
	WBSF::CTPeriod m_period;


	size_t		m_index;
	size_t		m_variable;
	float		m_zoom;
	WBSF::CLocation	m_target;
	WBSF::CWeatherStationVector m_observationStations;


	void Update();
	void UpdateScrollHelper();


	afx_msg int OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSplitterDelta(UINT ID, NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg LRESULT OnMouseLeave(WPARAM wParam, LPARAM lParam);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);

	LRESULT DrawDragLine(const CPoint& oldPt, const CPoint& newPt, int w = 2);

	WBSF::CChartCtrlMap& GetCharts(){ return m_charts; }

	ERMsg SaveAsImage(const std::string& strFilename);
	void CopyToClipboard();

	WBSF::CGraphVector GetChartDefine()const;
	static std::string GetGraphisFilesPath();

protected:



	WBSF::CTM		m_lastTM;
	size_t			m_lastIndex;
	WBSF::CTPeriod	m_lastPeriod;
	float			m_lastZoom;

	//internal use
	WBSF::CChartCtrlMap m_charts;
	WBSF::CSplitterControlPtrMap m_splitters;
	WBSF::CGraphVector m_lastChartsDefine;

	DECLARE_MESSAGE_MAP()

	void AdjustLayout();

	static bool RegisterWindowClass();
	static const TCHAR* WINDOW_CLASS_NAME;


protected:

	CPoint m_lastPoint;
	WBSF::CScrollHelperPtr m_scrollHelper;

};


//**************************************************************************************************************************************
class CWeightChartWnd : public CDockablePane
{
	// Construction
public:
	CWeightChartWnd();

	void AdjustLayout();
	void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	void CreateToolBar();


	// Attributs
public:

	void SetVSDotNetLook(BOOL bSet)
	{
		//m_wndPropList.SetVSDotNetLook(bSet);
		//m_wndPropList.SetGroupNameFullWidth(bSet);
	}


protected:

	CFont m_font;
	CWeightChartCtrl m_chartCtrl;
	CGraphToolBar m_wndToolBar;
	bool m_bMustBeUpdated;

public:
	virtual ~CWeightChartWnd();
	virtual BOOL PreTranslateMessage(MSG* pMsg);

protected:

	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);

	afx_msg void UpdateFonts();
	afx_msg void OnUpdateToolbar(CCmdUI *pCmdUI);
	afx_msg void OnToolbarCommand(UINT ID, NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnToolbarCommand(UINT ID);
	afx_msg void OnGraphOptions();
	afx_msg void OnDateChange(UINT);
	afx_msg void OnCopyGraph();
	afx_msg void OnSaveGraph();
	afx_msg void OnWindowPosChanged(WINDOWPOS* lpwndpos);

	DECLARE_MESSAGE_MAP()
};

