// OptionPropSheet.cpp : implementation file
//

#include "stdafx.h"
#include "Basic/Registry.h"
#include "UI/Common/AppOption.h"
#include "WeatherUpdaterOptionsDlg.h"
#include "Resource.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



/////////////////////////////////////////////////////////////////////////////
// CWeatherUpdaterOptionsDlg

IMPLEMENT_DYNAMIC(CWeatherUpdaterOptionsDlg, CMFCPropertySheet)

CWeatherUpdaterOptionsDlg::CWeatherUpdaterOptionsDlg(CWnd* pParentWnd, UINT iSelectPage)
	:CMFCPropertySheet(IDS_OPTION_CAPTION, pParentWnd, LINKS)
{
	m_psh.dwFlags |= PSH_NOAPPLYNOW;
	m_psh.dwFlags &= ~(PSH_HASHELP);

	//Load the icon what we will set later for the program
	m_hIcon = AfxGetApp()->LoadIcon(IDI_OPTION);


	//Set look and icon
	SetLook(CMFCPropertySheet::PropSheetLook_OutlookBar);
	SetIconsList(IDB_OPTION_ICON, 48);
	

    AddPage(&m_links);
	AddPage(&m_regional);
	AddPage(&m_layers);
	

    CAppOption option;
	SetActivePage( option.GetProfileInt(_T("OptionPanel"), 0) );
	
}

CWeatherUpdaterOptionsDlg::~CWeatherUpdaterOptionsDlg()
{
}


BEGIN_MESSAGE_MAP(CWeatherUpdaterOptionsDlg, CMFCPropertySheet)
	ON_BN_CLICKED(IDOK, OnFinish)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWeatherUpdaterOptionsDlg message handlers
void CWeatherUpdaterOptionsDlg::OnFinish()
{
	m_nModalResult = IDOK;
	PressButton( PSBTN_OK );
}


void CWeatherUpdaterOptionsDlg::OnDestroy() 
{
	
    CAppOption option;	
    option.WriteProfileInt( _T("OptionPanel"), GetActiveIndex() );	
    
    
    CMFCPropertySheet::OnDestroy();
}

BOOL CWeatherUpdaterOptionsDlg::OnInitDialog()
{
	BOOL bResult = CMFCPropertySheet::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE); // Set big icon
	SetIcon(m_hIcon, FALSE); // Set small icon
	
	return bResult;
}
