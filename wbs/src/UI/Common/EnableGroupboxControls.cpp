// EnableGroupboxControls.cpp  Version 1.0 - see article at CodeProject.com
//
// Author:  Hans Dietrich
//          hdietrich@gmail.com
//
// Description:
//     The EnableGroupboxControls function enables or disables all the controls 
//     contained within a groupbox.
//
// History
//     Version 1.0 - 2008 April 9
//     - Initial public release
//
// License:
//     This software is released under the Code Project Open License (CPOL),
//     which may be found here:  http://www.codeproject.com/info/eula.aspx
//     You are free to use this software in any way you like, except that you 
//     may not sell this source code.
//
//     This software is provided "as is" with no expressed or implied warranty.
//     I accept no liability for any damage or loss of business that this 
//     software may cause.
//
///////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "windows.h"
#include "tchar.h"
#include "EnableGroupboxControls.h"

//=============================================================================
//
// EnableGroupboxControls()
//
// Purpose:     This function enables/disables all the controls that are
//              completely contained with a groupbox.
//
// Parameters:  hWnd    - HWND of groupbox control
//              bEnable - TRUE = enable controls within groupbox
//
// Returns:     int     - number of controls enabled/disabled.  If zero is
//                        returned, it means that no controls lie within the
//                        rect of the groupbox.
//
int EnableGroupboxControls(HWND hWnd, BOOL bEnable)
{
	int rc = 0;

	if (::IsWindow(hWnd))
	{
		// get class name
		TCHAR szClassName[MAX_PATH];
		szClassName[0] = _T('\0');
		::GetClassName(hWnd, szClassName, sizeof(szClassName)/sizeof(TCHAR)-2);

		// get window style
		LONG lStyle = ::GetWindowLong(hWnd, GWL_STYLE);

		if ((_tcsicmp(szClassName, _T("Button")) == 0) &&
			((lStyle & BS_GROUPBOX) == BS_GROUPBOX))
		{
			// this is a groupbox

			RECT rectGroupbox;
			::GetWindowRect(hWnd, &rectGroupbox);

			// get first child control

			HWND hWndChild = 0;
			HWND hWndParent = ::GetParent(hWnd);
			if (IsWindow(hWndParent))
				hWndChild = ::GetWindow(hWndParent, GW_CHILD);

			while (hWndChild)
			{
				RECT rectChild;
				::GetWindowRect(hWndChild, &rectChild);

				// check if child rect is entirely contained within groupbox
				if ((rectChild.left >= rectGroupbox.left) &&
					(rectChild.right <= rectGroupbox.right) &&
					(rectChild.top >= rectGroupbox.top) &&
					(rectChild.bottom <= rectGroupbox.bottom))
				{
					//TRACE(_T("found child window 0x%X\n"), hWndChild);
					::EnableWindow(hWndChild, bEnable);
					rc++;
				}

				// get next child control
				hWndChild = ::GetWindow(hWndChild, GW_HWNDNEXT);
			}

			// if any controls were affected, invalidate the parent rect
			if (rc && IsWindow(hWndParent))
			{
				::InvalidateRect(hWndParent, NULL, FALSE);
			}
		}
	}
	return rc;
}
