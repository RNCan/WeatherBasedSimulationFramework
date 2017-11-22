// OptionPropSheet.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "MatchStationOptionsDlg.h"
#include "AppOption.h"
#include "resource.h"
#include "Registry.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// COptionCharts property page
BEGIN_MESSAGE_MAP(COptionCharts, CMFCPropertyPage)

END_MESSAGE_MAP()


COptionCharts::COptionCharts() : CMFCPropertyPage(COptionCharts::IDD)
{
	m_bInit = false;
}


COptionCharts::~COptionCharts()
{
}

void COptionCharts::DoDataExchange(CDataExchange* pDX)
{
	CMFCPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CMN_OPTION_CHARTS_MAX_DATA, m_maxDataCtrl);
	DDX_Control(pDX, IDC_CMN_OPTION_NB_PIXEL_PER_X, m_nbPixelsPerXCtrl);

	CFL::CRegistry registry("Charts");



	if (pDX->m_bSaveAndValidate)
	{
		registry.WriteProfileString("MaxDataSize", m_maxDataCtrl.GetString() );
		registry.WriteProfileString("NbPixelsPerXUnit", m_nbPixelsPerXCtrl.GetString());
	}
	else
	{
		m_maxDataCtrl.SetWindowText(registry.GetProfileString("MaxDataSize", "10000"));
		m_nbPixelsPerXCtrl.SetWindowText(registry.GetProfileString("NbPixelsPerXUnit", "25"));
	}
	

}


BOOL COptionCharts::OnInitDialog()
{
	CMFCPropertyPage::OnInitDialog();
	m_bInit = true;
	
	return TRUE;  // return TRUE unless you set the focus to a control
}


bool COptionCharts::OnFinish()
{
	// if the dialog is never show return true
	if (!m_bInit) 
		return true;
	
	UpdateData();
	return true;
}

void COptionCharts::OnOK()
{
	OnFinish();

	CMFCPropertyPage::OnOK();
}

/////////////////////////////////////////////////////////////////////////////
// CMatchStationOptionsDlg

IMPLEMENT_DYNAMIC(CMatchStationOptionsDlg, CMFCPropertySheet)

CMatchStationOptionsDlg::CMatchStationOptionsDlg(CWnd* pParentWnd, UINT iSelectPage)
	:CMFCPropertySheet(IDS_OPTION_CAPTION, pParentWnd, LINKS)
{
	m_psh.dwFlags |= PSH_NOAPPLYNOW;
	m_psh.dwFlags &= ~(PSH_HASHELP);

	//Load the icon what we will set later for the program
	m_hIcon = AfxGetApp()->LoadIcon(IDI_OPTION);

	//Set look and icon
	SetLook(CMFCPropertySheet::PropSheetLook_OutlookBar);
	
	CBitmap bitmap;
	bitmap.LoadBitmap(IDB_OPTION_ICON);
	
	m_images.Create(48, 48, ILC_COLOR32 | ILC_MASK, 0, 0);
	m_images.Add(CBitmap::FromHandle((HBITMAP)bitmap), RGB(0, 0, 0));
	SetIconsList((HIMAGELIST)m_images);

    AddPage(&m_links);
	AddPage(&m_timeFormat);
    AddPage(&m_charts);
	

    CAppOption option;
	SetActivePage( option.GetProfileInt(_T("OptionPanel"), 0) );
	
}

CMatchStationOptionsDlg::~CMatchStationOptionsDlg()
{
}


BEGIN_MESSAGE_MAP(CMatchStationOptionsDlg, CMFCPropertySheet)
	ON_BN_CLICKED(IDOK, OnFinish)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMatchStationOptionsDlg message handlers
void CMatchStationOptionsDlg::OnFinish()
{
	m_nModalResult = IDOK;
	PressButton( PSBTN_OK );
}


void CMatchStationOptionsDlg::OnDestroy() 
{
	
    CAppOption option;	
    option.WriteProfileInt( _T("OptionPanel"), GetActiveIndex() );	
    
    
    CMFCPropertySheet::OnDestroy();
}

BOOL CMatchStationOptionsDlg::OnInitDialog()
{
	BOOL bResult = CMFCPropertySheet::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE); // Set big icon
	SetIcon(m_hIcon, FALSE); // Set small icon
	
	return bResult;
}
