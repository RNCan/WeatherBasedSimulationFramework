
#pragma once

#include <map>
#include <boost\dynamic_bitset.hpp>
#include "UltimateGrid/UGCtrl.h"
#include "UltimateGrid/CellTypes/UGCTsarw.h" 
#include "Basic/WeatherDatabase.h"
#include "UI/Common/CommonCtrl.h"
#include "UI/WVariablesEdit.h"
#include "UI/StationsListCtrl.h"

class CDailyEditorDoc;


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
		
		if (!years.empty())
		{
			if (*years.rbegin() - *years.begin() == years.size() - 1)
				tmp = std::to_string(*years.begin()) + "-" + std::to_string(*years.rbegin());
		}


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
		std::set<int> years = WBSF::to_object<int, std::set<int>>(str, " -");
		if (str.find('-') != std::string::npos)
		{
			for (int y = *years.begin(); y <= *years.rbegin(); y++)
				years.insert(y);
		}

		return years;
	}
	
};


class CMFCToolBarNameButton : public CMFCToolBarEditBoxButton
{
public:

	DECLARE_SERIAL(CMFCToolBarNameButton)

	CMFCToolBarNameButton() {}
	CMFCToolBarNameButton(UINT uiID, UINT uimageID, int iWidth = 0) : CMFCToolBarEditBoxButton(uiID, uimageID, ES_AUTOHSCROLL | ES_WANTRETURN | WS_TABSTOP, iWidth)
	{
	}

	void SetFilter(std::string filter)
	{
		GetEditBox()->GetWindowText(m_strContents);
		CString newStr(filter.c_str());
		if (newStr != m_strContents)
		{
			m_strContents = newStr;
			GetEditBox()->SetWindowText(m_strContents);
		}
	}

	std::string GetFilter()
	{
		CString tmp;
		if (GetEditBox())
			GetEditBox()->GetWindowText(tmp);

		std::string str = CStringA(tmp);
		return str;
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

//*************************************************************************************************************

class CStationsListWnd : public CDockablePane
{
	DECLARE_DYNCREATE(CStationsListWnd)

// Construction
public:


	static CDailyEditorDoc* GetDocument();


	CStationsListWnd();
	virtual ~CStationsListWnd();

	void AdjustLayout();
	void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);



	
protected:

	CFont m_fntPropList;
	CStationsListToolBar m_wndToolBar;
	WBSF::CStationsListCtrl m_stationsList;
	CStationsListStatusBar m_wndStatusBar;


	virtual BOOL PreTranslateMessage(MSG* pMsg);


	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
	afx_msg LRESULT OnPropertyChanged(__in WPARAM wparam, __in LPARAM lparam);
	afx_msg void OnUpdateToolbar(CCmdUI *pCmdUI);
	afx_msg void OnToolbarCommand(UINT ID);
	afx_msg void OnUpdateStatusBar(CCmdUI* pCmdUI);
	afx_msg LRESULT  OnSelectionChange(WPARAM, LPARAM);

	void SetPropListFont();


};

