// OptionPropSheet.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "MatchStationOptionsDlg.h"
#include "AppOption.h"


//a enlever
#include "DBEditorRes.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMatchStationOptionsDlg

IMPLEMENT_DYNAMIC(CMatchStationOptionsDlg, CMFCPropertySheet)

CMatchStationOptionsDlg::CMatchStationOptionsDlg(CWnd* pParentWnd, UINT iSelectPage)
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

CMatchStationOptionsDlg::~CMatchStationOptionsDlg()
{
}


BEGIN_MESSAGE_MAP(CMatchStationOptionsDlg, CMFCPropertySheet)
	//{{AFX_MSG_MAP(CMatchStationOptionsDlg)
	ON_BN_CLICKED(IDOK, OnFinish)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMatchStationOptionsDlg message handlers
void CMatchStationOptionsDlg::OnFinish()
{
	
	
	//if( !m_directory.OnFinish() )return;
    //if( !m_links.OnFinish() )return;


	m_nModalResult = IDOK;
	PressButton( PSBTN_OK );
}


void CMatchStationOptionsDlg::OnDestroy() 
{
	
    CAppOption option;	
    option.WriteProfileInt( _T("Option Panel"), GetActiveIndex() );	
    
    
    CMFCPropertySheet::OnDestroy();
}

BOOL CMatchStationOptionsDlg::OnInitDialog()
{
	BOOL bResult = CMFCPropertySheet::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE); // Set big icon
	SetIcon(m_hIcon, FALSE); // Set small icon
	//SetWindowText( GetString(IDS_MODELS_CAPTION) + m_model.GetName() );

	
	return bResult;
}
