// OptionPropSheet.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "editorOptionDlg.h"
#include "AppOption.h"


//a enlever
#include "DBEditorRes.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBioSIMOptionDlg

IMPLEMENT_DYNAMIC(CBioSIMOptionDlg, CMFCPropertySheet)

CBioSIMOptionDlg::CBioSIMOptionDlg(CWnd* pParentWnd)
	:CMFCPropertySheet(IDS_OPTION_CAPTION, pParentWnd)
{
	m_psh.dwFlags |= PSH_NOAPPLYNOW;
	m_psh.dwFlags &= ~(PSH_HASHELP);

	//Load the icon what we will set later for the program
	m_hIcon = AfxGetApp()->LoadIcon(IDI_OPTION);

	//Set look and icon
	SetLook(CMFCPropertySheet::PropSheetLook_OutlookBar);
 	SetIconsList(IDB_OPTION_ICON, 48, RGB(192,192,192) );
	

	
    AddPage(&m_links);

    CAppOption option;
	SetActivePage( option.GetProfileInt(_T("Option Panel"), 0) );
	
}

CBioSIMOptionDlg::~CBioSIMOptionDlg()
{
}


BEGIN_MESSAGE_MAP(CBioSIMOptionDlg, CMFCPropertySheet)
	//{{AFX_MSG_MAP(CBioSIMOptionDlg)
	ON_BN_CLICKED(IDOK, OnFinish)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBioSIMOptionDlg message handlers
void CBioSIMOptionDlg::OnFinish()
{
	
	
	//if( !m_directory.OnFinish() )return;
    //if( !m_links.OnFinish() )return;


	m_nModalResult = IDOK;
	PressButton( PSBTN_OK );
}


void CBioSIMOptionDlg::OnDestroy() 
{
	
    CAppOption option;	
    option.WriteProfileInt( _T("Option Panel"), GetActiveIndex() );	
    
    
    CMFCPropertySheet::OnDestroy();
}

BOOL CBioSIMOptionDlg::OnInitDialog()
{
	BOOL bResult = CMFCPropertySheet::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE); // Set big icon
	SetIcon(m_hIcon, FALSE); // Set small icon
	//SetWindowText( GetString(IDS_MODELS_CAPTION) + m_model.GetName() );

	
	return bResult;
}
