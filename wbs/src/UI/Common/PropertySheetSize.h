//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once
//The solution is the following :
//
//1. Derive new PropertySheet class from CMFCPropertySheet
//2. Override OnInitDialog and PreCreateWindow methods
//3. Add WM_SIZE message handler
//4. Add members
//a.CSize m_sizePrev
//b.BOOL m_bInitialized
//5. Add methods
//a. void AdjustControlsLayout()
//b. int ReposButtons(BOOL bRedraw)
//Declaration listing :

/////////////////////////////////////////////////////////////////////////////
// CResizablePropertySheet


//#define WS_EX_LAYOUT_RTL	0x00400000
//
//class CResizableGrip
//{
//private:
//	SIZE m_sizeGrip;		// holds grip size
//
//	CScrollBar m_wndGrip;	// grip control
//
//private:
//	static BOOL IsRTL(HWND hwnd)
//	{
//		DWORD dwExStyle = ::GetWindowLong(hwnd, GWL_EXSTYLE);
//		return (dwExStyle & WS_EX_LAYOUT_RTL);
//	}
//
//	static LRESULT CALLBACK GripWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
//
//protected:
//	BOOL InitGrip();
//	void UpdateGripPos();
//	void ShowSizeGrip(BOOL bShow = TRUE);	// show or hide the size grip
//
//	virtual CWnd* GetResizableWnd() = 0;
//
//public:
//	CResizableGrip();
//	virtual ~CResizableGrip();
//
//};

class CResizablePropertySheet : public CMFCPropertySheet
{
	DECLARE_DYNAMIC(CResizablePropertySheet)

	// Construction
public:

	
	CResizablePropertySheet();
	CResizablePropertySheet(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	CResizablePropertySheet(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	
	virtual ~CResizablePropertySheet();
	virtual BOOL OnInitDialog();
	virtual BOOL PreCreateWindow(CREATESTRUCT cs);


	void AdjustControlsLayout(int cx, int cy);
	int ReposButtons(BOOL bRedraw);
	
	

protected:
	
	DECLARE_MESSAGE_MAP()
	afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg int OnCreate(LPCREATESTRUCT lpcs);


	
	BOOL	m_bInitialized;
	CRect	m_rCrt;
};
