/* 
 * Developed by Huzifa Terkawi 
 * http://www.codeproject.com/Members/huzifa30
 * All permission granted to use this code as long as you retain this notice
 * and refere to orginal Material when part of this code is re-puplished
*/

#pragma once

#include "resource.h"
#include "UI/Common/MFCEditBrowseCtrlEx.h"

class CMatchStationDoc;


class CFilePathDlg : public CDialog
{
	//DECLARE_DYNAMIC(CFilePathDlg)
	DECLARE_MESSAGE_MAP()

public:

	CMatchStationDoc* GetDocument();


	CFilePathDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CFilePathDlg();


	int Create(CWnd* pParent = NULL);
	void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);


protected:

	enum { IDD = IDD_DIALOGBAR };
	void DoDataExchange(CDataExchange* pDX);

	CMFCEditBrowseCtrlEx	m_normalsFilePathCtrl;
	CMFCEditBrowseCtrlEx	m_observationFilePathCtrl;

	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnChange(UINT ID);
};




//***********************************************************************
//
class CFilePathWnd : public CDockablePane
{
	DECLARE_DYNAMIC(CFilePathWnd)
	DECLARE_MESSAGE_MAP()

public:


	static CMatchStationDoc* GetDocument();

	CFilePathWnd();
	virtual ~CFilePathWnd();

	void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);



protected:

	afx_msg int OnCreate(LPCREATESTRUCT lp);
	afx_msg void OnSize(UINT nType, int cx, int cy);


private:
	CFilePathDlg m_wndDlg;
};
