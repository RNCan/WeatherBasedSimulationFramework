
#pragma once

#include "UI/Common/CommonCtrl.h"
#include "UI/Common/ProgressWnd.h"

class CBioSIMDoc;


class COutputView : public CView
{
	DECLARE_DYNCREATE(COutputView)

public:

	COutputView();
	~COutputView();

	virtual CBioSIMDoc* GetDocument() const;
	
	virtual void OnInitialUpdate(); // called first time after construct
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	virtual void OnDraw(CDC* pDC);
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);

	CProgressWnd& GetProgressWnd(){ return m_progressWnd; }
	void AdjustLayout();

protected:

	CProgressWnd	m_progressWnd;
	CReadOnlyEdit	m_messageWnd;

	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif


	CFont m_font;
};

