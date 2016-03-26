////////////////////////////////////////////////////////////////
// MSDN -- September 2000
// If this code works, it was written by Paul DiLascia.
// If not, I don't know who wrote it.
// Compiles with Visual C++ 6.0, runs on Windows 98 and probably NT too.
//
#pragma once
#include "subclass.h"

//////////////////
// Class to implement "only one instance" feature for an application.
// To use it:
//
// - instantiate global instance in your main window (frame/dialog)
//
// - call COneInstance::HookWindow when your main window is created
//   (OnCreate or OnInitDialog)
//
// - call COneInstance::OpenOtherInstance from your app's InitInstance
//   function.
//
class COneInstance : public CSubclassWnd {
protected:
	CString m_sClassName;					 // registered window class name
	UINT	  m_iIdentMsg;						 // message to positively identify me
	DWORD	  m_dwCopyDataOpen;				 // unique WM_COPYDATA ID
	HWND	  m_hwndFound;						 // used to find other instance

	// used to enumerate top-level windows
	static BOOL WINAPI EnumWindowsProc(HWND hwnd, LPARAM lp);

	// subclass function for main window
	virtual LRESULT WindowProc(UINT msg, WPARAM wp, LPARAM lp);

public:
	COneInstance(LPCTSTR lpClassName, UINT iTestMsg, DWORD dwCopyDataOpen=100);
	~COneInstance();

	// public API
	BOOL Init(CWnd* pWnd) {
		return HookWindow(pWnd);
	}
	virtual CWnd* FindOtherInstance();
	virtual CWnd* OpenOtherInstance(LPCTSTR lpFileName, BOOL bShow=TRUE);
};

