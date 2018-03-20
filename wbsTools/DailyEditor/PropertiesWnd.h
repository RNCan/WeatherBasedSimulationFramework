
#pragma once

#include <map>
#include "Basic/weatherDatabase.h"

class CDailyEditorDoc;

class CPropertiesToolBar : public CMFCToolBar
{
public:
	virtual void OnUpdateCmdUI(CFrameWnd* /*pTarget*/, BOOL bDisableIfNoHndler)
	{
		CMFCToolBar::OnUpdateCmdUI((CFrameWnd*) GetOwner(), bDisableIfNoHndler);
	}

	DECLARE_SERIAL(CPropertiesToolBar)

	virtual BOOL LoadState(LPCTSTR lpszProfileName = NULL, int nIndex = -1, UINT uiID = (UINT)-1) { return TRUE; }
	virtual BOOL SaveState(LPCTSTR lpszProfileName = NULL, int nIndex = -1, UINT uiID = (UINT)-1) { return TRUE; }
	virtual BOOL AllowShowOnList() const { return FALSE; }
	virtual BOOL LoadToolBarEx(UINT uiToolbarResID, CMFCToolBarInfo& params, BOOL bLocked = FALSE);
};

class CDataPropertyCtrl : public CMFCPropertyGridCtrl
{
public:

	CDataPropertyCtrl()
	{
		m_nLeftColumnWidth = 150;
		m_lastCol = -1;
		m_lastRow = -1;
		m_lastStationIndex = UNKNOWN_POS;
	}
	virtual BOOL ValidateItemData(CMFCPropertyGridProperty* /*pProp*/);
	virtual void OnPropertyChanged(CMFCPropertyGridProperty* pProp) const;
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);


	void EnableProperties(BOOL bEnable);
	void EnableProperties(CMFCPropertyGridProperty* pProp, BOOL bEnable);
	
	

protected:

	size_t m_lastStationIndex;
	int m_lastRow;
	int m_lastCol;

	
	void OnEditKillFocus();
	DECLARE_MESSAGE_MAP()
};


class CPropertiesWnd : public CDockablePane
{
// Construction
public:
	CPropertiesWnd();

	void AdjustLayout();
	void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);


// Attributs
public:

	void SetVSDotNetLook(BOOL bSet)
	{
		m_wndPropList.SetVSDotNetLook(bSet);
		m_wndPropList.SetGroupNameFullWidth(bSet);
	}

	
protected:
	CFont m_fntPropList;
	CDataPropertyCtrl m_wndPropList;

// Implémentation
public:
	virtual ~CPropertiesWnd();
	virtual BOOL PreTranslateMessage(MSG* pMsg);

protected:

	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
	afx_msg LRESULT OnSetText(WPARAM wParam, LPARAM lParam);
	

	void SetPropListFont();


	DECLARE_MESSAGE_MAP()
};

