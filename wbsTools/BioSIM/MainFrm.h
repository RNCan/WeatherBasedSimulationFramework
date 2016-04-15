
// MainFrm.h : interface of the CMainFrame class
//

#pragma once
#include "ProjectWnd.h"
#include "PropertiesWnd.h"
#include "ExportWnd.h"
#include "ResultGraphWnd.h"
#include "ResultDataWnd.h"
#include "UI/Common/ProgressWnd.h"

class CMainFrame : public CFrameWndEx
{
	
protected: // create from serialization only
	
	CMainFrame();
	DECLARE_DYNCREATE(CMainFrame)

// Overrides
public:

	virtual ~CMainFrame();
	void UpdateAllViews(CView* pSender, LPARAM lHint, CObject* pHint);
	void LoadBasicCommand();

	//virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	//virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);


	//CProgressWnd& GetProgressWnd(){ return m_progressWnd; }

	CComPtr<ITaskbarList3>& GetTaskBarList() { return m_pTaskbarList; }
protected:  // control bar embedded members
	
	CMFCMenuBar			m_wndMenuBar;
	CMFCToolBar			m_wndToolBar;
	CMFCStatusBar		m_wndStatusBar;
	CProjectWnd			m_projectWnd;
	CResultDataWnd		m_spreadsheetWnd;
	CResultGraphWnd		m_chartWnd;
	CPropertiesWnd		m_propertiesWnd;
	CExportWnd			m_exportWnd;

	//CPaneSplitter		m_wndSplitter;
	//CProgressWnd		m_progressWnd;
	//CProgressDockablePane m_progressWnd;
	//CFileManagerWnd		m_fileManagerWnd;
	
// Generated message map functions
protected:

	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//afx_msg LRESULT OnToolbarCreateNew(WPARAM wp, LPARAM lp);
	afx_msg void OnApplicationLook(UINT id);
	afx_msg void OnUpdateApplicationLook(CCmdUI* pCmdUI);
	afx_msg void OnToolsDbEditor(); 
	afx_msg void OnToolsOptions();
	afx_msg LRESULT OnToolsOptions2(WPARAM wParam, LPARAM lParam);
	afx_msg BOOL OnToolsRunApp(UINT id);
	afx_msg void OnRunApp(UINT id);
	afx_msg void OnExecute();
	afx_msg void OnUpdateExecute(CCmdUI *pCmdUI);
	afx_msg void OnLanguageChange(UINT id);
	afx_msg void OnLanguageUI(CCmdUI* pCmdUI);
	afx_msg BOOL OnHelp(UINT id);
	afx_msg void CleanUp();
	afx_msg LRESULT OnTaskbarBtnCreated ( WPARAM wParam, LPARAM lParam );
	afx_msg void OnUpdateToolbar(CCmdUI* pCmdUI);
	

	BOOL CreateDockingWindows();
	void SetDockingWindowIcons();



	static const UINT m_uTaskbarBtnCreatedMsg;
	CComPtr<ITaskbarList3> m_pTaskbarList;


#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	//static UINT ExecuteTask(void* pParam);
};
