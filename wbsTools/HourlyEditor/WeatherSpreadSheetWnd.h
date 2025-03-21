
// BioSIMView.h : interface of the CWeatherSpreadsheetWnd class
//


#pragma once


#include "UI/WeatherDataGridCtrl.h"
#include "UI/Common/CommonCtrl.h"




class CResultToolBar : public CSplittedToolBar
{
public:

	DECLARE_SERIAL(CResultToolBar)
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

//**************************************************************************************************************************************

class CWeatherSpreadsheetWnd : public CDockablePane
{
	DECLARE_DYNCREATE(CWeatherSpreadsheetWnd)

public:

	static CHourlyEditorDoc* GetDocument();

	CWeatherSpreadsheetWnd();
	virtual ~CWeatherSpreadsheetWnd();

	
	void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	void AdjustLayout();
	void CreateToolBar();
	bool SaveData();
	void ExportToExcel();
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);


protected:

	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnToolbarCommand(UINT ID);
	afx_msg void OnUpdateToolbar(CCmdUI *pCmdUI);
	afx_msg void OnWindowPosChanged(WINDOWPOS* lpwndpos);
	afx_msg void OnDateChange(UINT ID);

	CResultToolBar m_wndToolBar;
	WBSF::CWeatherDataGridCtrl m_grid;
	bool m_bMustBeUpdated;
	bool m_bEnableMessage;
	
};


