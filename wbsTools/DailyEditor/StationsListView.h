
#pragma once

#include <map>
#include <boost\dynamic_bitset.hpp>
#include "UltimateGrid/UGCtrl.h"
#include "UltimateGrid/CellTypes/UGCTsarw.h" 
#include "Basic/WeatherDatabase.h"
#include "UI/Common/CommonCtrl.h"
#include "UI/WVariablesEdit.h"
#include "UI/StationsListCtrl.h"

class CDailyEditorDoc;

//


class CMFCToolBarYearsButton : public CMFCToolBarEditBoxButton
{
public:

	DECLARE_SERIAL(CMFCToolBarYearsButton)

	CMFCToolBarYearsButton(){}
	CMFCToolBarYearsButton(UINT uiID, UINT uimageID, int iWidth = 0) : CMFCToolBarEditBoxButton(uiID, uimageID, ES_AUTOHSCROLL | ES_WANTRETURN | WS_TABSTOP, iWidth)
	{
	}

	void SetYears(const std::set<int>& years)
	{
		GetEditBox()->GetWindowText(m_strContents);
		std::string tmp = WBSF::to_string(years, " ");

		CString newStr(tmp.c_str());
		if (newStr != m_strContents)
		{
			m_strContents = newStr;
			GetEditBox()->SetWindowText(m_strContents);
		}
	}

	std::set<int> GetYears()
	{
		CString tmp;
		if (GetEditBox())
			GetEditBox()->GetWindowText(tmp);

		std::string str = CStringA(tmp);
		return WBSF::to_object<int, std::set<int>>(str," ");
	}
	
};

class CStationsListToolBar : public CMFCToolBar
{
public:
	
	DECLARE_SERIAL(CStationsListToolBar)
	
	
	virtual void OnUpdateCmdUI(CFrameWnd* /*pTarget*/, BOOL bDisableIfNoHndler)
	{
		CMFCToolBar::OnUpdateCmdUI((CFrameWnd*) GetOwner(), bDisableIfNoHndler);
	}

	virtual BOOL LoadState(LPCTSTR lpszProfileName = NULL, int nIndex = -1, UINT uiID = (UINT)-1){ return TRUE; }
	virtual BOOL SaveState(LPCTSTR lpszProfileName = NULL, int nIndex = -1, UINT uiID = (UINT)-1){ return TRUE; }
	virtual BOOL AllowShowOnList() const { return FALSE; }
	virtual BOOL LoadToolBarEx(UINT uiToolbarResID, CMFCToolBarInfo& params, BOOL bLocked = FALSE);

	DECLARE_MESSAGE_MAP()
	
	afx_msg void OnSize(UINT nType, int cx, int cy);
};

//*************************************************************************************************************
class CStationsListStatusBar : public CStatusBar
{
	virtual void OnUpdateCmdUI(CFrameWnd* /*pTarget*/, BOOL bDisableIfNoHndler)
	{
		CStatusBar::OnUpdateCmdUI((CFrameWnd*)GetOwner(), bDisableIfNoHndler);
	}
};

//*************************************************************************************************************

//class CStationsListCtrl : public CUGCtrl
//{
//public:
//
//	CStationsListCtrl();
//	
//	void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
//	void EnableStationsList(BOOL bEnable);
//
//	virtual void OnSetup();
//	
//	virtual void OnCellChange(int oldcol, int newcol, long oldrow, long newrow);
//	virtual int OnCanSizeCol(int) { return TRUE; }
//	virtual int OnCanSizeRow(long) { return FALSE; }
//	virtual int OnCanSizeTopHdg() { return FALSE; }
//	virtual int OnCanSizeSideHdg() { return TRUE; }
//	virtual void OnGetCell(int col, long row, CUGCell *cell);
//	virtual void OnTH_LClicked(int col, long row, int updn, RECT *rect, POINT *point, BOOL processed = 0);
//	virtual void OnCB_LClicked(int updn, RECT *rect, POINT *point, BOOL processed = 0);
//	virtual void OnSH_LClicked(int col, long row, int updn, RECT *rect, POINT *point, BOOL processed = 0);
//	virtual void OnColSized(int col, int *width);
//	virtual int OnHint(int col, long row, int section, CString *string);
//	virtual int OnVScrollHint(long row, CString *string);
//	virtual COLORREF OnGetDefBackColor(int section);
//	virtual void OnSelectionChanged(int startCol, long startRow, int endCol, long endRow, int blockNum);
//
//protected:
//	
//	CFont m_font;
//	CFont m_fontBold;
//	CPen m_cellBorderPen;
//
//	void CreateBoldFont();
//	void SortInfo(int col, int dir);
//	std::vector<std::pair<std::string, size_t>> m_sortInfo;
//	CUGSortArrowType m_sortArrow;
//	int m_curSortCol;
//	int m_sortDir;
//
//	std::vector<boost::dynamic_bitset<>> m_bDataEdited;
//
//	DECLARE_MESSAGE_MAP()
//};
//

class CStationsListView : public CView
{
	DECLARE_DYNCREATE(CStationsListView)

// Construction
public:
	CStationsListView();
	virtual ~CStationsListView();

	void AdjustLayout();
	void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);



	
protected:

	CFont m_fntPropList;
	CStationsListToolBar m_wndToolBar;
	WBSF::CStationsListCtrl m_stationsList;
	CStationsListStatusBar m_wndStatusBar;


	virtual void OnDraw(CDC* pDC);
	virtual BOOL PreTranslateMessage(MSG* pMsg);


	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
	afx_msg LRESULT OnPropertyChanged(__in WPARAM wparam, __in LPARAM lparam);
	afx_msg void OnUpdateToolbar(CCmdUI *pCmdUI);
	afx_msg void OnToolbarCommand(UINT ID);
	afx_msg void OnUpdateStatusBar(CCmdUI* pCmdUI);
	afx_msg LRESULT  OnSelectionChange(WPARAM, LPARAM);

	void SetPropListFont();


};

