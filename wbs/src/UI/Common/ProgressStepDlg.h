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
#include "UI/Common/ProgressWnd.h"



class CProgressStepDlg : public CDialog
{

public:

	// Construction
    CProgressStepDlg(bool bShowPause=false, bool bShowMinimize=false);   // standard constructor
	void SetTaskbarList(CComPtr<ITaskbarList3> pTaskbarList){ m_progressCtrl.SetTaskbarList(pTaskbarList); }

	virtual BOOL Create(CWnd* pParentWnd);

	ERMsg Execute(AFX_THREADPROC pfnThreadProc, CProgressStepDlgParam* pParam);

   // void GetMessageArray(CStringArray& messageArray);
    //void GetMessage(CString& message);

	WBSF::CCallback& GetCallback(){ return m_progressCtrl.GetCallback(); }
	void UpdateMainWindowText();

	
// Overrides
protected:

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);


	//virtual void PostNcDestroy();
	virtual void OnCancel();
	virtual void OnOK();

	// Generated message map functions
	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//afx_msg LRESULT OnThreadMessage(WPARAM wParam, LPARAM);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//afx_msg void OnBnClickedPause();


	//void UpdateCtrl();
	void AdjustLayout();

    //WBSF::CCallback m_callback;
	
    //std::string m_comment;
	std::string m_title;
	CWnd* m_pParentWnd;
	//bool m_bShowPause;
	bool m_bShowMinimize;
	bool m_bIsIconic;

	bool m_bIsThreadSuspended;
	CWinThread *m_ptrThread;


	//CComPtr<ITaskbarList3> m_pTaskbar;
	
	CProgressWnd	m_progressCtrl;
	CEdit			m_messageCtrl;

};
