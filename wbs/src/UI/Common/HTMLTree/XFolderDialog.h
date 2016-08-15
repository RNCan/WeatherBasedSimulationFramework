// XFolderDialog.h  Version 1.4
//
// Author:  Hans Dietrich
//          hdietrich@gmail.com
//
// This software is released into the public domain.  You are free to use
// it in any way you like, except that you may not sell this source code.
//
// This software is provided "as is" with no expressed or implied warranty.
// I accept no liability for any damage or loss of business that this
// software may cause.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef XFOLDERDIALOG_H
#define XFOLDERDIALOG_H

#include "XHistoryCombo.h"
#include "XFileOpenListView.h"

// Windows 2000 version of OPENFILENAME.
// The new version has three extra members.
// This is copied from commdlg.h
//
struct OPENFILENAMEEX_FOLDER : public OPENFILENAME
{
	void *	pvReserved;
	DWORD	dwReserved;
	DWORD	FlagsEx;
};

///////////////////////////////////////////////////////////////////////////////
// CXFolderDialog: Encapsulate Windows-2000 style open dialog.
//
class CXFolderDialog : public CFileDialog
{
	DECLARE_DYNAMIC(CXFolderDialog)

// Construction
public:
	
	CXFolderDialog(LPCTSTR lpszInitialFolder = NULL,
				   DWORD dwFlags = 0,
				   CWnd* pParentWnd = NULL);
	virtual ~CXFolderDialog();

// Dialog Data
	//{{AFX_DATA(CXFolderDialog)
	CHistoryCombo	m_cmbRecentFolders;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CXFolderDialog)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Operations
public:
	// override
	virtual INT_PTR DoModal();

// Attributes
public:

	enum XFILEDIALOG_OS_VERSION
	{
		XFILEDIALOG_AUTO_DETECT_OS_VERSION = 0,
		XFILEDIALOG_OS_VERSION_4,
		XFILEDIALOG_OS_VERSION_5
	};

	CString GetPath() { return m_strPath; }
	enum XFILEDIALOG_OS_VERSION GetOsVersion() { return m_eOsVersion; }
	void SetOsVersion(enum XFILEDIALOG_OS_VERSION eOsVersion) { m_eOsVersion = eOsVersion; }
	void SetTitle(LPCTSTR lpszTitle) { m_strTitle = lpszTitle; }
	void EnableRegistry(BOOL bEnable) { m_bUseRegistry = bEnable; }
	int GetViewMode() { return m_nViewMode; }
	void SetViewMode(int cmd);
	//LPCTSTR GetFilter()const{return m_strFilter;}
	//void SetFilter(LPCTSTR strFilter = NULL){ m_strFilter = strFilter ; }
// Implementation
protected:
	OPENFILENAMEEX_FOLDER		m_ofnEx;	// new Windows 2000 version of OPENFILENAME
	UINT				m_nIdFileNameStatic;
	UINT				m_nIdFileNameCombo;
	UINT				m_nIdFilesOfTypeStatic;
	UINT				m_nIdFilesOfTypeCombo;
	UINT				m_nIdPlaceBar;
	UINT				m_nViewMode;
	int					m_nStaticLeftMargin;
	CString				m_strTitle;
	DWORD				m_dwFlags;
	CString				m_strInitialFolder;
	//CString				m_strFilter;
	CString				m_strPath;
	CXFileOpenListView	m_wndListView;
	BOOL				m_bUseRegistry;		// TRUE = read/write registry for list view mode
	BOOL				m_bPersist;			// TRUE = persist view
	BOOL				m_bFirstTime;
	BOOL				m_bVistaFlag;

	enum XFILEDIALOG_OS_VERSION m_eOsVersion;

	CString GetPath(UINT nMessage);
	BOOL IsMinSize(int& cx, int& cy);
	BOOL IsWin2000();
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	virtual void OnOK();

	//{{AFX_MSG(CXFolderDialog)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSelendokMruCombo();
	afx_msg void OnCbnKillfocusMruCombo();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	//afx_msg LRESULT OnPostInit(WPARAM wp, LPARAM lp);

	DECLARE_MESSAGE_MAP()
};

#endif //XFOLDERDIALOG_H
