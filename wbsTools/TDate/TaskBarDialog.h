#pragma once

/////////////////////////////////////////////////////////////////////////////
// CTaskBarDialog dialog

#define MYWM_NOTIFYICON		(WM_APP+100)


class CTaskBarDialog : public CDialog
{
// Construction
public:

    CTaskBarDialog(int iconID, int menuID);   // standard constructor
    void SetToolTip(const CString& toolTip);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   	virtual void OnOK();
	virtual void OnCancel();

// Implementation
protected:

    virtual void HandlePopupMenu (CPoint& point);
    virtual void NotifyDelete (void);
    virtual void NotifyAdd(void);
    virtual void NotifyUpdate(void);
    virtual BOOL TrayMessage (DWORD dwMessage);
    


	// Generated message map functions
	afx_msg void OnDestroy();
	virtual BOOL OnInitDialog();
	afx_msg void OnClose();

	
	afx_msg LRESULT OnNotifyIcon(WPARAM no, LPARAM lParam);
    afx_msg void OnRButtonDown(UINT nFlags, CPoint point);

	DECLARE_MESSAGE_MAP()
    

    CString m_pszTip;
    HICON   m_hIcon;
    
    
    int m_iconID;
    bool m_bInit;
    int m_menuID;
    int m_uID;
};

