#pragma once


#include "UI/Common/CommonCtrl.h"


class COutputWnd : public CDockablePane
{
// Construction
public:
	
	COutputWnd();
	virtual ~COutputWnd();


	void UpdateFonts();
	void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	void AdjustHorzScroll(CListBox& wndListBox);

	

protected:

	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	

	CReadOnlyEdit m_wndOutput;
};

