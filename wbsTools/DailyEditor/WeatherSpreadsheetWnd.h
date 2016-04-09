
// BioSIMView.h : interface of the CWeatherTableWnd class
//


#pragma once


#include "UI/WeatherDataGridCtrl.h"
#include "UI/Common/CommonCtrl.h"


class CDailyEditorDoc;

class CWeatherSpreadsheetToolBar : public CSplittedToolBar
{
public:

	DECLARE_SERIAL(CWeatherSpreadsheetToolBar)
	virtual BOOL LoadToolBarEx(UINT uiToolbarResID, CMFCToolBarInfo& params, BOOL bLocked = FALSE);
	
};

//**************************************************************************************************************************************

class CWeatherSpreadsheetWnd : public CDockablePane
{
	DECLARE_DYNCREATE(CWeatherSpreadsheetWnd)


public:

	CWeatherSpreadsheetWnd();
	virtual ~CWeatherSpreadsheetWnd();


	void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	
	
// Generated message map functions
protected:
	
	void AdjustLayout();
	void CreateToolBar();
	bool SaveData();
	void ExportToExcel();
	WBSF::CTM GetTM();


	
	CWeatherSpreadsheetToolBar m_wndToolBar;
	WBSF::CWeatherDataGridCtrl m_grid;
	bool m_bMustBeUpdated;

	


	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnExecute();
	afx_msg void OnUpdateExecute(CCmdUI *pCmdUI);
	afx_msg void OnToolbarCommand(UINT ID);
	afx_msg void OnUpdateToolbar(CCmdUI *pCmdUI);
	afx_msg void OnWindowPosChanged(WINDOWPOS* lpwndpos);
	afx_msg void OnDateChange(UINT ID);


	
	
};

