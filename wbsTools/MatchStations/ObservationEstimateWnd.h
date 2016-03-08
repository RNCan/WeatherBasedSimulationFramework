
// BioSIMView.h : interface of the CNormalsListCtrl class
//


#pragma once


#include "UI/StationsListCtrl.h"
#include "UI/Common/CommonCtrl.h"





//**************************************************************************************************************************************
class CObservationEstimateCtrl : public CUGCtrl
{
public:

	static const WORD UWM_SELECTION_CHANGE = (WM_USER + 2); // a custom Windows message

	CObservationEstimateCtrl();
	virtual ~CObservationEstimateCtrl();

	void Update();

	size_t		m_variable;
	WBSF::CNormalsStation m_mean;
	WBSF::CNormalsStation m_estimate;



	virtual void OnSetup();
	virtual int OnCanSizeCol(int) { return TRUE; }
	virtual int OnCanSizeRow(long) { return FALSE; }
	virtual int OnCanSizeTopHdg() { return FALSE; }
	virtual int OnCanSizeSideHdg() { return TRUE; }


	virtual void OnCellChange(int oldcol, int newcol, long oldrow, long newrow);
	virtual void OnGetCell(int col, long row, CUGCell *cell);
	virtual void OnTH_LClicked(int col, long row, int updn, RECT *rect, POINT *point, BOOL processed = 0);
	virtual void OnCB_LClicked(int updn, RECT *rect, POINT *point, BOOL processed = 0);
	virtual void OnSH_LClicked(int col, long row, int updn, RECT *rect, POINT *point, BOOL processed = 0);
	virtual void OnColSized(int col, int *width);
	virtual COLORREF OnGetDefBackColor(int section);



protected:

	CFont m_font;
	CFont m_fontBold;
	CPen m_cellBorderPen;


	std::string GetDataText(int col, long row)const;

	void CreateBoldFont();
	void SortInfo(int col, int dir);
	inline int OtherDir(int dir){ return (dir == UGCT_SORTARROWUP) ? UGCT_SORTARROWDOWN : UGCT_SORTARROWUP; }
	std::vector<std::pair<std::string, size_t>> m_sortInfo;
	CUGSortArrowType m_sortArrow;
	int m_curSortCol;
	int m_sortDir;



	DECLARE_MESSAGE_MAP()
	afx_msg void OnWindowPosChanged(WINDOWPOS* lpwndpos);


};


//**************************************************************************************************************************************
class CObservationEstimateWnd : public CDockablePane
{
	// Construction
public:
	CObservationEstimateWnd();
	virtual ~CObservationEstimateWnd();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);

	void AdjustLayout();
	void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);

	CObservationEstimateCtrl m_estimateCtrl;

protected:


	bool m_bMustBeUpdated;

	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
	afx_msg void OnUpdateToolbar(CCmdUI *pCmdUI);
	afx_msg void OnToolbarCommand(UINT ID);
	afx_msg void OnDateChange(UINT ID);
	afx_msg void OnWindowPosChanged(WINDOWPOS* lpwndpos);

	void SetPropListFont();
	void CreateToolBar();


	DECLARE_MESSAGE_MAP()
};

