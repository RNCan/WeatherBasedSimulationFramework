
// BioSIMView.h : interface of the CWeatherTableWnd class
//


#pragma once


#include "UI/Common/CommonCtrl.h"
#include "UI/NormalsDataGridCtrl.h"


class CNormalsEditorDoc;

class CNormalsSpreadsheetToolBar : public CMFCToolBar
{
public:

	DECLARE_SERIAL(CNormalsSpreadsheetToolBar)

	virtual void OnUpdateCmdUI(CFrameWnd* /*pTarget*/, BOOL bDisableIfNoHndler)
	{
		CMFCToolBar::OnUpdateCmdUI((CFrameWnd*) GetOwner(), bDisableIfNoHndler);
	}
	virtual BOOL LoadState(LPCTSTR lpszProfileName = NULL, int nIndex = -1, UINT uiID = (UINT) -1){	return TRUE;}
	virtual BOOL SaveState(LPCTSTR lpszProfileName = NULL, int nIndex = -1, UINT uiID = (UINT) -1){	return TRUE;}
	virtual BOOL AllowShowOnList() const { return FALSE; }

	//virtual BOOL LoadToolBarEx(UINT uiToolbarResID, CMFCToolBarInfo& params, BOOL bLocked = FALSE);
	//virtual void AdjustLocations();
	//void UpdateButton();
};

//**************************************************************************************************************************************

class CNormalsSpreadsheetWnd : public CDockablePane
{

public:


	static CNormalsEditorDoc* GetDocument();
	static WBSF::CWeatherDatabasePtr GetDatabasePtr();

	CNormalsSpreadsheetWnd();
	virtual ~CNormalsSpreadsheetWnd();


	void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	
	
// Generated message map functions
protected:
	
	void AdjustLayout();
	void UpdateResult(LPARAM lHint=NULL);
	void CreateToolBar();
	bool SaveData();
	void ExportToExcel();
	WBSF::CTM GetTM();


	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnToolbarCommand(UINT ID);
	afx_msg void OnUpdateToolbar(CCmdUI *pCmdUI);
	afx_msg void OnWindowPosChanged(WINDOWPOS* lpwndpos);
	


	CNormalsSpreadsheetToolBar m_wndToolBar;
	WBSF::CNormalsDataGridCtrl m_grid;
	bool m_bMustBeUpdated;

	
};
