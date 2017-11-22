
#pragma once


#include "Simulation/Executable.h"
#include "UI/ExecutableTree.h"

class CBioSIMDoc;

class CFileManagerWnd: public CDockablePane
{
	
public:
	
	static CBioSIMDoc* GetDocument();
	
	CFileManagerWnd();
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);


	void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	void AdjustLayout();

protected:

	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnWindowPosChanged(WINDOWPOS* lpwndpos);
	
	bool m_bMustBeUpdated;
};
