//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
//****************************************************************************
#include "stdafx.h"
#include "StaticEx.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CStaticEx


CStaticEx::CStaticEx()
{
}

CStaticEx::~CStaticEx()
{
}


BEGIN_MESSAGE_MAP(CStaticEx, CStatic)
END_MESSAGE_MAP()



//=============================================================================
//
// EnableChildWindows()
//
// Purpose:     This function enables/disables all the controls that are
//              completely contained within a parent.
//
// Parameters:  hWnd          - HWND of parent control
//              bEnable       - TRUE = enable controls within parent
//              bEnableParent - TRUE = also enable/disable parent window
//
// Returns:     int     - number of controls enabled/disabled.  If zero is
//                        returned, it means that no controls lie within the
//                        rect of the parent.
//
int CStaticEx::EnableChildWindows(HWND hWnd, BOOL bEnable, BOOL bEnableParent)
{
	int rc = 0;

	if (bEnableParent)
		::EnableWindow(hWnd, bEnable);

	RECT rectWindow;
	::GetWindowRect(hWnd, &rectWindow);

	// get first child control

	HWND hWndChild = 0;
	HWND hWndParent = ::GetParent(hWnd);
	if (IsWindow(hWndParent))
		hWndChild = ::GetWindow(hWndParent, GW_CHILD);

	while (hWndChild)
	{
		RECT rectChild;
		::GetWindowRect(hWndChild, &rectChild);

		// check if child rect is entirely contained within window
		if ((rectChild.left   >= rectWindow.left) &&
			(rectChild.right  <= rectWindow.right) &&
			(rectChild.top    >= rectWindow.top) &&
			(rectChild.bottom <= rectWindow.bottom))
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

	return rc;
}

//=============================================================================
BOOL CStaticEx::EnableWindow(BOOL bEnable /*= TRUE*/, 
							  BOOL bRecurseChildren /*= FALSE*/)
//=============================================================================
{
	BOOL rc = CStatic::EnableWindow(bEnable);

	if (bRecurseChildren)
		EnableChildWindows(m_hWnd, bEnable, FALSE);

	return rc;
}
