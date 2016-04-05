
#pragma once

#include <map>
#include <boost\dynamic_bitset.hpp>
#include "UltimateGrid/UGCtrl.h"
#include "UltimateGrid/CellTypes/UGCTsarw.h" 
#include "Basic/WeatherDatabase.h"
#include "UI/Common/CommonCtrl.h"
#include "UI/WVariablesEdit.h"
#include "UI/StationsListCtrl.h"

class CHourlyEditorDoc;


class CMFCToolBarYearsButton : public CMFCToolBarEditBoxButton
{
public:

	DECLARE_SERIAL(CMFCToolBarYearsButton)

	CMFCToolBarYearsButton(){}
	CMFCToolBarYearsButton(UINT uiID, UINT uimageID, int iWidth = 0) : CMFCToolBarEditBoxButton(uiID, uimageID, ES_AUTOHSCROLL | ES_WANTRETURN | WS_TABSTOP, iWidth)
	{
	}

	void SetYears(const std::set<int>& years)
	{
		GetEditBox()->GetWindowText(m_strContents);
		std::string tmp = WBSF::to_string(years, " ");

		CString newStr(tmp.c_str());
		if (newStr != m_strContents)
		{
			m_strContents = newStr;
			GetEditBox()->SetWindowText(m_strContents);
		}
	}

	std::set<int> GetYears()
	{
		CString tmp;
		if (GetEditBox())
			GetEditBox()->GetWindowText(tmp);

		std::string str = CStringA(tmp);
		return WBSF::to_object<int, std::set<int>>(str, " ");
	}
	
};

class CStationsListToolBar : public CMFCToolBar
{
public:
	
	DECLARE_SERIAL(CStationsListToolBar)
	
	
	virtual void OnUpdateCmdUI(CFrameWnd* /*pTarget*/, BOOL bDisableIfNoHndler)
	{
		CMFCToolBar::OnUpdateCmdUI((CFrameWnd*) GetOwner(), bDisableIfNoHndler);
	}

	virtual BOOL LoadState(LPCTSTR lpszProfileName = NULL, int nIndex = -1, UINT uiID = (UINT)-1){ return TRUE; }
	virtual BOOL SaveState(LPCTSTR lpszProfileName = NULL, int nIndex = -1, UINT uiID = (UINT)-1){ return TRUE; }
	virtual BOOL AllowShowOnList() const { return FALSE; }
	virtual BOOL LoadToolBarEx(UINT uiToolbarResID, CMFCToolBarInfo& params, BOOL bLocked = FALSE);

	DECLARE_MESSAGE_MAP()
	
	afx_msg void OnSize(UINT nType, int cx, int cy);
};

//*************************************************************************************************************
class CStationsListStatusBar : public CStatusBar
{
	virtual void OnUpdateCmdUI(CFrameWnd* /*pTarget*/, BOOL bDisableIfNoHndler)
	{
		CStatusBar::OnUpdateCmdUI((CFrameWnd*)GetOwner(), bDisableIfNoHndler);
	}
};


class CStationsListView : public CView
{

	DECLARE_DYNCREATE(CStationsListView)
// Construction
public:
	CStationsListView();

	
	virtual ~CStationsListView();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	

	CHourlyEditorDoc* GetDocument() const;
	WBSF::CWeatherDatabasePtr GetDatabasePtr();
	void AdjustLayout();

	
protected:

	CFont m_fntPropList;
	CStationsListToolBar m_wndToolBar;
	WBSF::CStationsListCtrl m_stationsList;
	CStationsListStatusBar m_wndStatusBar;

	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
	afx_msg LRESULT OnPropertyChanged(__in WPARAM wparam, __in LPARAM lparam);
	afx_msg void OnUpdateToolbar(CCmdUI *pCmdUI);
	afx_msg void OnToolbarCommand(UINT ID);
	afx_msg void OnUpdateStatusBar(CCmdUI* pCmdUI);
	afx_msg LRESULT OnSelectionChange(WPARAM, LPARAM);
	

	void SetPropListFont();

	
};
