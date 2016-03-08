
#pragma once

#include <map>
#include "Basic/weatherDatabase.h"



namespace WBSF
{

	class CMatchStationDoc;
	//typedef std::tr1::shared_ptr<CWeatherDatabase> CWeatherDatabasePtr;

	class CPropertiesToolBar : public CMFCToolBar
	{
	public:
		virtual void OnUpdateCmdUI(CFrameWnd* /*pTarget*/, BOOL bDisableIfNoHndler)
		{
			CMFCToolBar::OnUpdateCmdUI((CFrameWnd*)GetOwner(), bDisableIfNoHndler);
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
			m_lastIndex = UNKNOWN_POS;
		}
		virtual BOOL ValidateItemData(CMFCPropertyGridProperty* /*pProp*/);
		virtual void OnPropertyChanged(CMFCPropertyGridProperty* pProp) const;
		virtual BOOL PreTranslateMessage(MSG* pMsg);


		//void SetProject(CWeatherDatabasePtr& pProject);
		void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);


		void EnableProperties(BOOL bEnable);
		void EnableProperties(CMFCPropertyGridProperty* pProp, BOOL bEnable);



	protected:

		size_t m_lastIndex;
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


		//CMatchStationDoc* GetDocument();
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
		//CPropertiesToolBar m_wndToolBar;
		CDataPropertyCtrl m_wndPropList;

		// Implémentation
	public:
		virtual ~CPropertiesWnd();
		virtual BOOL PreTranslateMessage(MSG* pMsg);

	protected:

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


		DECLARE_MESSAGE_MAP()
	};

}