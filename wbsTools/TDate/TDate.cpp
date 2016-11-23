// TDate.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "TDate.h"
#include "TDateDlg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTDateApp

BEGIN_MESSAGE_MAP(CTDateApp, CWinApp)
	//{{AFX_MSG_MAP(CTDateApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTDateApp construction

CTDateApp::CTDateApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CTDateApp object

CTDateApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CTDateApp initialization

static const CString MyMainWndClassName = _T("Transforme Date");

/*BOOL CTDateApp::InitApplication() 
{
    // Call base class. Default version does nothing.
    CWinApp::InitApplication();

    WNDCLASS wndcls;
    // Start with NULL defaults.
    memset(&wndcls, 0, sizeof(WNDCLASS));   
    // Get class information for default window class.
    ::GetClassInfo(AfxGetInstanceHandle(),"AfxFrameOrView",&wndcls);
    // Substitute unique class name for new class.
    wndcls.lpszClassName = MyMainWndClassName;
    // Register new class and return the result code.
    return ::RegisterClass(&wndcls);
	
//	return CWinApp::InitApplication();
}
*/

BOOL CTDateApp::InitInstance()
{
    //Partir une seul instance
    if (!FirstInstance()) 
        return FALSE;


	AfxEnableControlContainer();
  	SetRegistryKey(_T("Centre de Foresterie des Laurentides"));


	m_pMainWnd = new CTDateDlg;
	int nResponse = ((CTDateDlg*)m_pMainWnd)->Create(IDD_TDATE_DIALOG);
    
    if( __argc != 1)
        m_nCmdShow = SW_HIDE;


    m_pMainWnd->ShowWindow(m_nCmdShow );


	return TRUE;
}





BOOL CTDateApp::FirstInstance()
{
    CWnd *PrevCWnd, *ChildCWnd;
    // Determine if another window with our class name exists.
 //   PrevCWnd = CWnd::FindWindow(MyMainWndClassName, NULL);
    PrevCWnd = CWnd::FindWindow(NULL, MyMainWndClassName);
    if (PrevCWnd != NULL)
    {
        if( m_nCmdShow != SW_HIDE)
        {
            // If so, does it have any pop-ups?
            ChildCWnd=PrevCWnd->GetLastActivePopup();

            PrevCWnd->ShowWindow(SW_RESTORE);
            // Bring the main window to the top.
            PrevCWnd->BringWindowToTop();
            // If iconic, restore the main window.
            //WINDOWPLACEMENT  wndpl;
            //PrevCWnd->GetWindowPlacement( &wndpl);


            //if (PrevCWnd->IsIconic() || wndpl.showCmd == SW_HIDE)
        
            // If there are pop-ups, bring them along too!
            if (PrevCWnd != ChildCWnd)
                ChildCWnd->BringWindowToTop();
            // Return FALSE. This isn't the first instance
            // and we are done activating the previous one.
        }
        return FALSE;
        
    }
    //else
        // First instance. Proceed as normal.
        return TRUE;
}




