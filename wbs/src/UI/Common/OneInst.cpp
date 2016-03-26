////////////////////////////////////////////////////////////////
// MSDN -- September 2000
// If this code works, it was written by Paul DiLascia.
// If not, I don't know who wrote it.
// Compiles with Visual C++ 6.0, runs on Windows 98 and probably NT too.
//
#include "StdAfx.h"
#include "OneInst.h"
#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//////////////////
// COnstruct: need class name, message ID for ident message and ID for
// WM_COPYDATA message.
//
COneInstance::COneInstance(LPCTSTR lpszClassName, UINT iIdentMsg, DWORD dwCopyDataOpen)
{
	m_sClassName = lpszClassName;
	m_iIdentMsg  = iIdentMsg;
	m_dwCopyDataOpen = dwCopyDataOpen;
}

COneInstance::~COneInstance() 
{
}

//////////////////
// Enumerator function to find other top-level window instance
//
BOOL WINAPI COneInstance::EnumWindowsProc(HWND hwnd, LPARAM lp)
{
	COneInstance* poi = (COneInstance*)lp;
	WCHAR classname[512] = { 0 };
	::GetClassNameW(hwnd, classname, sizeof(classname));
	if (_tcscmp(classname, (LPCTSTR)poi->m_sClassName) == 0 &&
		SendMessage(hwnd, poi->m_iIdentMsg, 0, 0)) {
		poi->m_hwndFound = hwnd; // found 
		return FALSE;				 // stop enumerating
	}
	return TRUE; // keep enumerating
}

//////////////////
// Find other instance. Can't use FindWindow because may need to search
// several windows with same class name, eg, for dialogs which all have
// the same class name.
// 
CWnd* COneInstance::FindOtherInstance()
{
	m_hwndFound = NULL; // assume not found
	EnumWindows(EnumWindowsProc, (LPARAM)this);
	return m_hwndFound ? CWnd::FromHandle(m_hwndFound) : NULL;
}

//////////////////
// Open file in other instance, if there is one. Returns ptr to other
// window, if found.
//
CWnd* COneInstance::OpenOtherInstance(LPCTSTR lpFileName, BOOL bShow)
{
	CWnd* pWnd = FindOtherInstance();
	if (pWnd) 
	{
		COPYDATASTRUCT cds;
		cds.dwData = m_dwCopyDataOpen;
		cds.lpData = (PVOID)lpFileName;
		cds.cbData = (DWORD)_tcslen(lpFileName);
		if (pWnd->SendMessage(WM_COPYDATA, NULL, (LPARAM)&cds)) {
			if (bShow) {
				// activate other instance
				if( pWnd->IsIconic() )
					pWnd->ShowWindow(SW_RESTORE);
				else pWnd->ShowWindow(SW_SHOW);
				//pWnd->SetWindowPos();
				pWnd->SetForegroundWindow();
				pWnd->SetFocus();
			}
		} else 
		{
			pWnd = NULL;
		}
	}
	return pWnd;
}

//////////////////
// Window proc used to subclass main window. Processes ident message to
// return TRUE and WM_COPYDATA message to open file
//
LRESULT COneInstance::WindowProc(UINT msg, WPARAM wp, LPARAM lp)
{
	if (msg==m_iIdentMsg) {
		return 1; // it's me!

	} else if (msg==WM_COPYDATA) {
		COPYDATASTRUCT* pcd = (COPYDATASTRUCT*)lp;
		if (pcd && pcd->dwData==m_dwCopyDataOpen) 
		{
			// get file name from COPYDATASTRUCT
			TCHAR buf[1024];
			int len = std::min(pcd->cbData,(DWORD)(sizeof(buf)-1));
			_tcsncpy(buf,(LPCTSTR)pcd->lpData,len);
			buf[len] = 0;
			CString sFileName = buf;
			if( !sFileName.IsEmpty() )
			{
				// now open it
				if( AfxGetApp()->OpenDocumentFile(sFileName) )
				{
					AfxGetApp()->AddToRecentFileList(sFileName);
				}
			}
			return TRUE;
		}
	}
	return CSubclassWnd::WindowProc(msg, wp, lp);
}
