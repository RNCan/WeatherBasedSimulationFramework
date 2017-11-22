
// BioSIMView.h : interface of the CNormalsListCtrl class
//


#pragma once


#include "UI/StationsListCtrl.h"
#include "UI/Common/CommonCtrl.h"



//**************************************************************************************************************************************
class CObservationMatchWnd : public CDockablePane
{
	// Construction
public:
	CObservationMatchWnd();
	virtual ~CObservationMatchWnd();
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	void AdjustLayout();
	void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	
	WBSF::CMatchStationsCtrl m_matchStationCtrl;

protected:

	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
	afx_msg void OnWindowPosChanged(WINDOWPOS* lpwndpos);

	CFont m_font;
	bool m_bMustBeUpdated;
	void SetPropListFont();

	DECLARE_MESSAGE_MAP()
};
