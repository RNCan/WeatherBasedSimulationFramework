
#pragma once

//#include "ProjectView.h"
#include "Simulation/Executable.h"
#include "UI/ExecutableTree.h"
#include "UI/DBEditorPropSheet.h"

class CBioSIMDoc;
//
//class CProjectWndToolBar : public CMFCToolBar
//{
//	virtual void OnUpdateCmdUI(CFrameWnd* /*pTarget*/, BOOL bDisableIfNoHndler)
//	{
//		CMFCToolBar::OnUpdateCmdUI((CFrameWnd*)GetOwner(), bDisableIfNoHndler);
//	}
//
//	virtual BOOL LoadState(LPCTSTR lpszProfileName = NULL, int nIndex = -1, UINT uiID = (UINT)-1){ return TRUE; }
//	virtual BOOL SaveState(LPCTSTR lpszProfileName = NULL, int nIndex = -1, UINT uiID = (UINT)-1){ return TRUE; }
//
//	virtual BOOL AllowShowOnList() const { return FALSE; }
//};
//
//
//class CProjectWndStatusBar : public CStatusBar
//{
//	virtual void OnUpdateCmdUI(CFrameWnd* /*pTarget*/, BOOL bDisableIfNoHndler)
//	{
//		CStatusBar::OnUpdateCmdUI((CFrameWnd*)GetOwner(), bDisableIfNoHndler);
//	}
//
//};


class CFileManagerWnd: public CDockablePane
{
	
public:
	
	static CBioSIMDoc* GetDocument();
	
	CFileManagerWnd();
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);


	void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	void AdjustLayout();

protected:

	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnWindowPosChanged(WINDOWPOS* lpwndpos);
	
	//WBSF::CDBManagerDlg m_wnd;

	//CMFCOutlookBar  m_wndOutlookBar;
	//CImageList     m_Icons;
	bool m_bMustBeUpdated;
};
