// TaskBarDialog.cpp : implementation file
//

#include "stdafx.h"
#include "TDate.h"
#include "TaskBarDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTaskBarDialog dialog


CTaskBarDialog::CTaskBarDialog(int iconID, int menuID):
m_hIcon(NULL),
m_iconID(iconID),
m_menuID(menuID),
m_uID(1000),
m_bInit(false)
{
}


void CTaskBarDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CTaskBarDialog, CDialog)
	ON_WM_DESTROY()
    ON_MESSAGE(MYWM_NOTIFYICON, OnNotifyIcon)
END_MESSAGE_MAP()



/////////////////////////////////////////////////////////////////////////////
// CTaskBarDialog message handlers


BOOL CTaskBarDialog::OnInitDialog() 
{
	CDialog::OnInitDialog();
    

    m_hIcon = AfxGetApp()->LoadIcon(m_iconID);
    ASSERT(m_hIcon);

    if( m_pszTip.IsEmpty() )
        m_pszTip = AfxGetAppName();

    
	//
	// Add Tray Notify Icon
	//

	NotifyAdd();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


LRESULT CTaskBarDialog::OnNotifyIcon(WPARAM no, LPARAM lParam)
{
    switch (lParam)
    {
    case WM_MOUSEMOVE: break;
    case WM_LBUTTONDOWN: break;

    
    
    //
    // Diplay the dialog box on Left mouse dblclick
    // this is how the user gets the dialog back if it is hidden
    //

    case WM_LBUTTONDBLCLK:
    {
        ShowWindow (SW_RESTORE);
        SetForegroundWindow ();
        break;
    }

    //
    // Handle popup here when user right mouse clicks tray icon
    //
    case WM_RBUTTONDOWN:
    {
         CPoint point;
         GetCursorPos (&point);
         HandlePopupMenu (point);

         break;

    }

   
	default: break;

	}  // lParam switch


    return 0;
}

void CTaskBarDialog::OnDestroy() 
{
    NotifyDelete();
    if( m_hIcon )
	    DestroyIcon(m_hIcon);

    
    CDialog::OnDestroy();
}

void CTaskBarDialog::OnOK() 
{
    //Do notting
}

void CTaskBarDialog::OnCancel() 
{
	//cahce la fenetre
    ShowWindow(SW_HIDE);
}

 /****************************************************************************
 *                                                                          
 *  FUNCTION   : TrayMessage (HWND hDlg, DWORD dwMessage, UINT uID, 
 *                            HICON hIcon, PSTR pszTip )
 *                                                                          
 *  PURPOSE    : Creates, Modifies or deletes the tray icon 
 *               If pszTip is not null, it uses that for the tip
 *               otherwise it sets a default tip
 *                                                                          
 \****************************************************************************/
BOOL CTaskBarDialog::TrayMessage ( DWORD dwMessage  )

{
    BOOL res;
    NOTIFYICONDATA tnd;

    //
    // Get the Tray Icon
    //

    tnd.cbSize		= sizeof(NOTIFYICONDATA);
    tnd.hWnd		= m_hWnd;
    tnd.uID		    = m_uID;
    tnd.uFlags		= NIF_MESSAGE|NIF_ICON|NIF_TIP;
    tnd.uCallbackMessage= MYWM_NOTIFYICON;
    tnd.hIcon		= m_hIcon;
	
    //
    // If there is a specific tip, set it
    // otherwise use SetDisp as the default
    //


    ASSERT(m_pszTip.GetLength() < 64);
    ASSERT( !m_pszTip.IsEmpty() );
    lstrcpyn(tnd.szTip, (LPCTSTR)m_pszTip, m_pszTip.GetLength() + 1);
	
    //
    //	Use the Shell_NotifyIcon API to setup the tray icon
    //

    res = Shell_NotifyIcon(dwMessage, &tnd);


    return res;

}


 /****************************************************************************
 *                                                                          
 *  FUNCTION   : NotifyDelete (HWND hDlg, UINT uIndex)
 *                                                                          
 *  PURPOSE    : Deletes a tray icon based on the uIndex .
 *               In this SetDisp sample, it is only used for a single icon
 *                                                                          
 \****************************************************************************/

void CTaskBarDialog::NotifyDelete (void)
{
    TrayMessage(NIM_DELETE);
    m_bInit = false;
}


 /****************************************************************************
 *                                                                          
 *  FUNCTION   : NotifyAdd (HWND hDlg, UINT uIndex)
 *                                                                          
 *  PURPOSE    : Creates tray icons based on the uIndex .
 *               In this SetDisp sample, it is only used for a single icon
 *                                                                          
 \****************************************************************************/

void CTaskBarDialog::NotifyAdd(void)
{

    TrayMessage(NIM_ADD);
    m_bInit = true;

}

void CTaskBarDialog::NotifyUpdate(void)
{
    TrayMessage(NIM_MODIFY);
}

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : HandlePopupMenu (hwnd, point)                              *
 *                                                                          *
 *  PURPOSE    : Handles the display of the "floating" popup that appears   *
 *               on a mouse click in the app's client area.                 *
 *                                                                          *
 ****************************************************************************/
void CTaskBarDialog::HandlePopupMenu (CPoint& point)
{
    if( m_menuID != -1)
    {
        //
        // Get the menu for the windows
        //

        CMenu menu;
		menu.LoadMenu(m_menuID);
        ASSERT(menu.m_hMenu);

		CMenu* pContextMenu = menu.GetSubMenu(0);
        ASSERT(pContextMenu->m_hMenu);


        SetForegroundWindow ();
		pContextMenu->TrackPopupMenu( TPM_LEFTALIGN | TPM_LEFTBUTTON |TPM_RIGHTBUTTON,
		point.x, point.y, this );

        PostMessage (WM_USER, 0, 0);
    }

}


void CTaskBarDialog::SetToolTip(const CString& toolTip)
{
    m_pszTip = toolTip;

    if( m_bInit )
        NotifyUpdate();
    
}