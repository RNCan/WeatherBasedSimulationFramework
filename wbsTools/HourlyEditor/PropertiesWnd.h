
#pragma once

#include <map>
#include "Basic/weatherDatabase.h"

class CHourlyEditorDoc;

class CPropertiesToolBar : public CMFCToolBar
{
public:
	virtual void OnUpdateCmdUI(CFrameWnd* /*pTarget*/, BOOL bDisableIfNoHndler)
	{
		CMFCToolBar::OnUpdateCmdUI((CFrameWnd*) GetOwner(), bDisableIfNoHndler);
	}

	virtual BOOL AllowShowOnList() const { return FALSE; }
};


class CCodesListProperty : public CMFCPropertyGridProperty
{
	virtual CComboBox* CreateCombo(CWnd* pWndParent, CRect rect);
	virtual BOOL OnEdit(LPPOINT /*lptClick*/);

public:

	CCodesListProperty(const CString& strName, const COleVariant& varValue, LPCTSTR lpszDescr, DWORD_PTR dwData, const std::string& codes);

	virtual void OnSelectCombo();
	virtual BOOL PushChar(UINT nChar);

	
	std::map<std::string, std::string> m_codes;
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
	

	//void SetProject(CWeatherDatabasePtr& pProject);
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

	static CHourlyEditorDoc* GetDocument();
	static WBSF::CWeatherDatabasePtr GetDatabasePtr();

	CPropertiesWnd();
	virtual ~CPropertiesWnd();

	
	void AdjustLayout();
	void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	virtual BOOL CPropertiesWnd::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	void SetVSDotNetLook(BOOL bSet)
	{
		m_wndPropList.SetVSDotNetLook(bSet);
		m_wndPropList.SetGroupNameFullWidth(bSet);
	}

	
protected:


	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnExpandAllProperties();
	afx_msg void OnUpdateExpandAllProperties(CCmdUI* pCmdUI);
	afx_msg void OnSortProperties();
	afx_msg void OnUpdateSortProperties(CCmdUI* pCmdUI);
	afx_msg void OnProperties1();
	afx_msg void OnUpdateProperties1(CCmdUI* pCmdUI);
	afx_msg void OnProperties2();
	afx_msg void OnUpdateProperties2(CCmdUI* pCmdUI);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
	afx_msg void OnVialKillFocus();
	afx_msg LRESULT OnPropertyChanged(__in WPARAM wparam, __in LPARAM lparam);
	afx_msg LRESULT OnSetText(WPARAM wParam, LPARAM lParam);
	

	void SetPropListFont();

	CFont m_fntPropList;
	CDataPropertyCtrl m_wndPropList;


};

