//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once

#include "basic/ERMsg.h"
#include "Basic/Callback.h"
#include "StaticBitmap.h"



class CProgressStepDlgParam
{
public:
	CProgressStepDlgParam(void* pThis=NULL, void* pExtra=NULL)
	{
		m_pThis = pThis;
		m_pExtra = pExtra;
		m_pCallback = NULL;
		m_pMsg = NULL;
	}

	void* m_pThis;
	void* m_pExtra;
	WBSF::CCallback* m_pCallback;
	ERMsg* m_pMsg;
};


class CProgressStepDlg : public CDialog
{
// Construction
public:
	static void PumpMessage(void);


    CProgressStepDlg(CWnd* pParent = NULL, bool bShowPause=false, bool bShowMinimize=false);   // standard constructor
	void SetTaskbarList(CComPtr<ITaskbarList3> pTaskbarList){m_pTaskbar=pTaskbarList;}

	ERMsg Execute(AFX_THREADPROC pfnThreadProc, CProgressStepDlgParam* pParam);
	BOOL Create();

	void UpdateMainWindowText();
	

    void GetMessageArray(CStringArray& messageArray);
    void GetMessage(CString& message);

	//CWnd* pParentWnd, bool bShowPause=false, bool bShowMinimize=false
	virtual BOOL Create(CWnd* pParentWnd);

	WBSF::CCallback& GetCallback(){ return m_callback; }

// Overrides
protected:

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	virtual void OnCancel();
	virtual void OnOK();

	// Generated message map functions
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg LRESULT OnThreadMessage(WPARAM wParam, LPARAM);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDestroy();
	afx_msg void OnBnClickedPause();

	DECLARE_MESSAGE_MAP()


	void UpdateCtrl();
	void AdjustLayout();

    WBSF::CCallback m_callback;
	int m_nCurrentTask;
	int m_nCurrentNbTasks;
    int m_nCurrentStepPos;
	//std::string m_lastDescription;
    std::string m_comment;
	std::string m_title;
	CWnd* m_pParentWnd;
	bool m_bShowPause;
	bool m_bShowMinimize;
	bool m_bIsIconic;

	bool m_bIsThreadSuspended;
	CWinThread *m_ptrThread;


	CComPtr<ITaskbarList3> m_pTaskbar;

   	CStatic	m_stepNoCtrl;
	CStatic	m_pourcentageCtrl;
	CStatic	m_descriptionCtrl;
	CProgressCtrl	m_progressCtrl;
	CEdit	m_messageCtrl;
    CStaticBitmap m_bitmap;
	CButton m_cancelCtrl;
	CButton m_pauseCtrl;

};
//
//class CProgressStepDlg : public CProgressStepDlg 
//{
//public:
//	
//	CProgressStepDlg (CWnd* pParent = NULL):
//	  CProgressStepDlg (pParent)
//	  {}
//	
//	  CProgressStepDlg(WBSF::CCallback& callback, CWnd* pParent = NULL):
//	  CProgressStepDlg (pParent)
//	  {
//		  ASSERT(false);
//	  }
//};
