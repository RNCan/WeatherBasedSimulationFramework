
// BioSIMView.h : interface of the CExportView class
//


#pragma once

#include "Basic\Dimension.h"
#include "UI/Common/CommonCtrl.h"
#include "UI/VariableSelectionCtrl.h"
#include "Resource.h"


class CExportWndToolBar1 : public CMFCToolBar
{
	virtual void OnUpdateCmdUI(CFrameWnd* /*pTarget*/, BOOL bDisableIfNoHndler)
	{
		CMFCToolBar::OnUpdateCmdUI((CFrameWnd*) GetOwner(), bDisableIfNoHndler);
	}

	virtual BOOL LoadState(LPCTSTR lpszProfileName = NULL, int nIndex = -1, UINT uiID = (UINT) -1){	return TRUE;}
	virtual BOOL SaveState(LPCTSTR lpszProfileName = NULL, int nIndex = -1, UINT uiID = (UINT) -1){	return TRUE;}
	virtual BOOL AllowShowOnList() const { return FALSE; }


};


class CExportWndToolBar2 : public CMFCToolBar
{
	virtual void OnUpdateCmdUI(CFrameWnd* /*pTarget*/, BOOL bDisableIfNoHndler)
	{
		CMFCToolBar::OnUpdateCmdUI((CFrameWnd*)GetOwner(), bDisableIfNoHndler);
	}

	virtual BOOL LoadState(LPCTSTR lpszProfileName = NULL, int nIndex = -1, UINT uiID = (UINT)-1){ return TRUE; }
	virtual BOOL SaveState(LPCTSTR lpszProfileName = NULL, int nIndex = -1, UINT uiID = (UINT)-1){ return TRUE; }
	virtual BOOL AllowShowOnList() const { return FALSE; }
	virtual BOOL LoadToolBarEx(UINT uiToolbarResID, CMFCToolBarInfo& params, BOOL bLocked);

	DECLARE_MESSAGE_MAP()
	afx_msg	void OnSize(UINT nType, int cx, int cy);
};

class CExportWnd : public CDockablePane
{
// Construction
public:

	static CBioSIMDoc* GetDocument();


	CExportWnd();
	~CExportWnd();	


	std::string m_iName;
	WBSF::CExport m_export;


	void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	void OnContextMenu(CWnd* pWnd, CPoint point);
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	
	//void OnCheck(int ID, bool bChecked);
protected:

	WBSF::CVariableSelectionCtrl m_variablesCtrl;
	WBSF::CStatisticSelectionCtrl m_statisticCtrl;
	
	CExportWndToolBar1 m_wndToolBar1;
	CExportWndToolBar2 m_wndToolBar2;
	CPaneSplitter m_wndSplitter;
	

	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnExport(UINT ID);
	afx_msg void OnExportToShowMap();
	afx_msg void OnOpenDirectory();
	afx_msg void OnNameChange();
	afx_msg void OnUpdateToolbar(CCmdUI *pCmdUI);
	afx_msg void OnExportCheked();
	LRESULT OnCheckbox(WPARAM wParam, LPARAM lParam);
	

	void AdjustLayout();
	void RepositionChildControl(CWnd *pWnd, const int dx, const int dy, const UINT uAnchor);
	void UpdateExport(WBSF::CDimension dimension, WBSF::CExport& theExport);
	//void GetExportFromInterface(WBSF::CExport& oExport);
	//void SetExportToInterface(const WBSF::CExport& oExport);
	void ChangeView();
	static UINT ExportTask(void* pParam);

	bool m_bDesableUpdate;
};
