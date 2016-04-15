#pragma once

#include "ResultCtrl.h"
#include "UI/Common/CommonCtrl.h"


class CResultToolBar : public CSplittedToolBar
{
public:
	DECLARE_SERIAL(CResultToolBar)
	virtual BOOL LoadToolBarEx(UINT uiToolbarResID, CMFCToolBarInfo& params, BOOL bLocked = FALSE);
};



class CResultDataWnd : public CDockablePane
{
// Attributes
public:
	static CBioSIMDoc* GetDocument();


	CResultDataWnd();
	virtual ~CResultDataWnd();

	void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	
//	CResultCtrl* GetGridCtrl ()	{ return &m_grid;	}

// Overrides
protected:
	


	DECLARE_MESSAGE_MAP()
	afx_msg void OnToolbarCommand(UINT ID);
	afx_msg void OnUpdateToolbar(CCmdUI *pCmdUI);
	afx_msg void OnWindowPosChanged(WINDOWPOS* lpwndpos);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnExecute();
	afx_msg void OnUpdateExecute(CCmdUI *pCmdUI);
	
	
	void CreateToolBar();
	void AdjustLayout();


	CFont m_font;
	CResultCtrl m_grid;
	CResultToolBar m_wndToolBar;
	//CStatisticComboBox m_statisticCtrl;
	//CStatic m_validityCtrl;

	bool m_bMustBeUpdated;



};

