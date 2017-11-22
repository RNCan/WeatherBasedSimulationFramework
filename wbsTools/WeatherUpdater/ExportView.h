
// BioSIMView.h : interface of the CExportView class
//


#pragma once

//#include "CreateView.h"
#include "UICommon/CommonCtrl.h"
#include "UI/VariableSelectionCtrl.h"
#include "Resource.h"


class CExportWndToolBar : public CMFCToolBar
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
//
//class CExportView : public CWnd
//{
//	
//
//public:
//
//	CExportView();
//	virtual ~CExportView();
//
//
//	//CBioSIMDoc* GetDocument() const;
//
//	/*static CExportView* CreateView(CWnd* pParent, CCreateContext *pContext=NULL)
//	{
//		return (CExportView*)CreateViewEx(RUNTIME_CLASS(CExportView), pParent, pContext);
//	}
//*/
//	
//	
//
//protected:
//	//virtual void OnDraw(CDC* pDC);  // overridden to draw this view
//	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
//	//virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
//	void Update();
//
//	// Implementation
//protected:
//
//// Generated message map functions
//protected:
//	
//	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
//	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
//
//	void AdjustLayout();
//	
//	DECLARE_MESSAGE_MAP()
//
//	//CButton m_autoExportCtrl;
//	//CCFLEdit m_fileNameCtrl;
//	//CCFLComboBox m_scriptCtrl;
//	
//	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
//	afx_msg void OnSize(UINT nType, int cx, int cy);
//	afx_msg void OnExecute();
//	afx_msg void OnUpdateExecute(CCmdUI *pCmdUI);
//	afx_msg int OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message);
//
//	virtual void DoDataExchange(CDataExchange* pDX);
//
//
//	DECLARE_DYNCREATE(CExportView)
//};


class CPaneSplitter : public CSplitterWndEx
{
public:
	BOOL AddWindow(int row, int col, CWnd* pWnd, CString clsName, DWORD dwStyle, DWORD dwStyleEx, SIZE sizeInit)
	{
		m_pColInfo[col].nIdealSize = sizeInit.cx;
		m_pRowInfo[row].nIdealSize = sizeInit.cy;
		CRect rect(CPoint(0, 0), sizeInit);
		if (!pWnd->CreateEx(dwStyleEx, clsName, NULL, dwStyle, rect, this, IdFromRowCol(row, col)))
			return FALSE;
		return TRUE;
	}
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

protected:

	WBSF::CVariableSelectionCtrl m_variablesCtrl;
	WBSF::CStatisticSelectionCtrl m_statisticCtrl;
	
	CExportWndToolBar m_wndToolBar;
	CExportWndToolBar2 m_wndToolBar2;
	CPaneSplitter m_wndSplitter;
	

	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnExport(UINT ID);
	afx_msg void OnExportToShowMap();
	afx_msg void OnOpenDirectory();
	afx_msg void OnNameChange();
	//afx_msg void OnVariableChange()
	//afx_msg void OnVariableChange()

	void AdjustLayout();
	void RepositionChildControl(CWnd *pWnd, const int dx, const int dy, const UINT uAnchor);
	void UpdateExport(WBSF::CDimension dimension, WBSF::CExport& theExport);
	void GetExportFromInterface(WBSF::CExport& oExport);
	void SetExportFromInterface(const WBSF::CExport& oExport);
	void ChangeView();
	
	



	
};
