
#pragma once

#include "UI/Common/CommonCtrl.h"

class CBioSIMDoc;
/////////////////////////////////////////////////////////////////////////////
// COutputWnd window


class COutputWnd : public CDockablePane
{
	

public:

	enum TOutput { OUTPUT_MESSAGE, VALIDATION, NB_OUTPUT };
	static CBioSIMDoc* GetDocument();


	COutputWnd();
	virtual ~COutputWnd();

	void SetOutputType(short outputType){m_outputType = outputType; }
	void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	void AdjustLayout();

	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);

protected:

	
	
	DECLARE_MESSAGE_MAP()

	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg int OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message);
	afx_msg void OnSize(UINT nType, int cx, int cy);

	CReadOnlyEdit m_editCtrl;
	CFont m_font;
	short m_outputType;


};
