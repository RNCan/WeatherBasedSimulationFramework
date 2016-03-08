
// BioSIMView.h : interface of the CNormalsListCtrl class
//


#pragma once


#include "UI/StationsListCtrl.h"
#include "UI/Common/CommonCtrl.h"



namespace WBSF
{

	//
	//class CNormalsListToolBar : public CMFCToolBar
	//{
	//public:
	//
	//	DECLARE_SERIAL(CNormalsListToolBar)
	//
	//	virtual void OnUpdateCmdUI(CFrameWnd* /*pTarget*/, BOOL bDisableIfNoHndler)
	//	{
	//		CMFCToolBar::OnUpdateCmdUI((CFrameWnd*) GetOwner(), bDisableIfNoHndler);
	//	}
	//	virtual BOOL LoadState(LPCTSTR lpszProfileName = NULL, int nIndex = -1, UINT uiID = (UINT) -1){	return TRUE;}
	//	virtual BOOL SaveState(LPCTSTR lpszProfileName = NULL, int nIndex = -1, UINT uiID = (UINT) -1){	return TRUE;}
	//	virtual BOOL AllowShowOnList() const { return FALSE; }
	//
	//	virtual BOOL LoadToolBarEx(UINT uiToolbarResID, CMFCToolBarInfo& params, BOOL bLocked = FALSE);
	//	virtual void AdjustLocations();
	//	void UpdateButton();
	//};

	//**************************************************************************************************************************************

	//class CNormalsListCtrl : public CStationsListCtrl
	//{
	//public:
	//
	//
	//	CNormalsListCtrl();
	//	virtual ~CNormalsListCtrl();
	//
	//	void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//	void AdjustLayout();
	//
	//	
	//	bool m_bMustBeUpdated;
	//
	//	bool SaveData();
	//	void ExportToExcel();
	//
	//protected:
	//
	//	DECLARE_MESSAGE_MAP()
	//	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//	afx_msg void OnSize(UINT nType, int cx, int cy);
	//	//afx_msg void OnExecute();
	//	//afx_msg void OnUpdateExecute(CCmdUI *pCmdUI);
	//	//afx_msg void OnToolbarCommand(UINT ID);
	//	//afx_msg void OnUpdateToolbar(CCmdUI *pCmdUI);
	//	afx_msg void OnWindowPosChanged(WINDOWPOS* lpwndpos);
	//	
	//
	//	CTM GetTM();
	//};


	//**************************************************************************************************************************************
	class CObservationMatchWnd : public CDockablePane
	{
		// Construction
	public:
		CObservationMatchWnd();
		virtual ~CObservationMatchWnd();
		virtual BOOL PreTranslateMessage(MSG* pMsg);

		void AdjustLayout();
		void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);


		//CNormalsListToolBar m_wndToolBar;
		CMatchStationsCtrl m_matchStationCtrl;

	protected:

		afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
		afx_msg void OnSize(UINT nType, int cx, int cy);
		afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
		afx_msg void OnWindowPosChanged(WINDOWPOS* lpwndpos);
		//afx_msg void OnUpdateToolbar(CCmdUI *pCmdUI);
		//afx_msg void OnToolbarCommand(UINT ID);
		//afx_msg void OnDateChange(UINT ID);

		CFont m_font;
		bool m_bMustBeUpdated;
		void SetPropListFont();
		//void CreateToolBar();


		DECLARE_MESSAGE_MAP()
	};

}