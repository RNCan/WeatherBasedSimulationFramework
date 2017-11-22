
// BioSIMView.h : interface of the CResultGraphWnd class
//


#pragma once

#include "Simulation/Result.h"
#include "Simulation/Executable.h"
#include "UI/Common/CommonCtrl.h"
#include "ChartCtrl/ChartCtrl.h"
#include "Simulation/Graph.h"

class CBioSIMDoc;




class CGraphToolBar : public CMFCToolBar
{
public:
	virtual void OnUpdateCmdUI(CFrameWnd* /*pTarget*/, BOOL bDisableIfNoHndler)
	{
		CMFCToolBar::OnUpdateCmdUI((CFrameWnd*) GetOwner(), bDisableIfNoHndler);
	}
	virtual BOOL LoadState(LPCTSTR lpszProfileName = NULL, int nIndex = -1, UINT uiID = (UINT) -1){	return TRUE;}
	virtual BOOL SaveState(LPCTSTR lpszProfileName = NULL, int nIndex = -1, UINT uiID = (UINT) -1){	return TRUE;}
	virtual BOOL AllowShowOnList() const { return FALSE; }
};


class CResultChartCtrl : public CChartCtrl
{
public:

	CResultChartCtrl();
	
	void SetGraph(const WBSF::CGraph& graph);
	void SetData(WBSF::CResultPtr pResult);
	void UpdateGraph();

	//int GetFirstLine()const{ return m_firstLine; }
	//void SetFirstLine(int in){ m_firstLine=in; }
	//int GetLastLine()const{ return m_lastLine; }
	//void SetLastLine(int in){ m_lastLine=in; }
	
protected:

	afx_msg void OnPaint();
	DECLARE_MESSAGE_MAP()
	
	void AddSerie(const WBSF::CVariableDefine& XSerie, const WBSF::CGraphSerie& YSerie);

	//int m_firstLine;
	//int m_lastLine;
	WBSF::CGraph		m_graph;
	WBSF::CResultPtr m_pResult;
	bool		m_bShow;
};



class CResultGraphWnd : public CDockablePane//: public CBCGPGridView
{

public:

	static CBioSIMDoc* GetDocument();


	CResultGraphWnd();
	~CResultGraphWnd();
	void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);

// Generated message map functions
protected:

	void AdjustLayout();
	void FillGraphList();
	void UpdateResult();
	const WBSF::CGraphVector& GetGraphArray()const;
	void SetGraphArray(const WBSF::CGraphVector& graphArray);
	void GetAllOtherGraphArray(WBSF::CGraphVector& allOtherGraph, WBSF::CExecutablePtr pExec = WBSF::CExecutablePtr(), std::string iName = "");
	
	virtual void OnWindowPosChanged(WINDOWPOS* lpwndpos);


	CCFLComboBox m_graphListCtrl;
	CMFCButton m_addCtrl;
	CMFCButton m_removeCtrl;
	CResultChartCtrl m_chartCtrl;
	CGraphToolBar m_wndToolBar;

	bool m_bMustBeUpdated;

	int m_baseIndex;
	WBSF::CGraphVector m_allOtherGraphArray;


	DECLARE_MESSAGE_MAP()
	afx_msg void OnGraphChange();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnUpdateToolbar(CCmdUI *pCmdUI);
	afx_msg BOOL OnEditGraph(UINT ID);
	afx_msg void OnRemoveGraph();
	afx_msg void OnSaveGraph();
	afx_msg void OnPrintGraph();
	afx_msg void OnDestroy();

};
