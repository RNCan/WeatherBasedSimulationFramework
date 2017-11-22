/* 
 * Developed by Huzifa Terkawi 
 * http://www.codeproject.com/Members/huzifa30
 * All permission granted to use this code as long as you retain this notice
 * and refere to orginal Material when part of this code is re-puplished
*/

#pragma once

#include "resource.h"
#include "UI/Common/CommonCtrl.h"

class CMatchStationDoc;


class CInputPropertyCtrl : public CMFCPropertyGridCtrl
{
public:

	enum TInput{ LOC_FILEPATH, OBSERVATION_TYPE, HOURLY_FILEPATH, DAILY_FILEPATH, NORMALS_FILEPATH, VARIABLES, YEAR, NB_STATIONS, SEARCH_RADIUS, SKIP_VERIFY, NB_INPUTS };
	enum TInputType{ IT_FILEPATH, IT_OBS_TYPE, IT_VAR, IT_STRING, IT_BOOL, NB_INPUT_TYPE };
	static const int INPUT_TYPE[NB_INPUTS];

	CInputPropertyCtrl();
	
	virtual void OnPropertyChanged(CMFCPropertyGridProperty* pProp) const;
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);

//	void EnableProperties(BOOL bEnable);
	//void EnableProperties(CMFCPropertyGridProperty* pProp, BOOL bEnable);
	void SetPropListFont();
	void SetLeftColumnWidth(int width) { m_nLeftColumnWidth = width; AdjustLayout(); }

protected:

	size_t m_lastIndex;
	int m_lastRow;
	int m_lastCol;
	CFont m_fntPropList;
	
	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	
};


class CInputWnd : public CDockablePane
{
	// Construction
public:
	
	

	static CMatchStationDoc* GetDocument();


	CInputWnd();
	virtual ~CInputWnd();
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	

	void AdjustLayout();
	void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);


protected:
	

	CInputPropertyCtrl m_inputCtrl;


	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
	afx_msg void OnKillFocus();
	afx_msg LRESULT OnPropertyChanged(__in WPARAM wparam, __in LPARAM lparam);
	afx_msg LRESULT OnSetText(WPARAM wParam, LPARAM lParam);
	afx_msg void OnDestroy();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);

	DECLARE_MESSAGE_MAP()
};



