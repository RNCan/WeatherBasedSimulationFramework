// OptionGeneral.cpp : implementation file
//

#include "stdafx.h"

#include "Basic/Registry.h"
#include "UI/Common/AppOption.h"
#include "OptionBioSIM.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COptionBioSIM property page 

//IMPLEMENT_DYNCREATE(COptionBioSIM, CMFCPropertyPage)

COptionBioSIM::COptionBioSIM() : CMFCPropertyPage(COptionBioSIM::IDD)
{
    CAppOption option;

    m_bSave = option.GetProfileBool(_T("SaveAtRun"), false);
	m_bExportAllLines = option.GetProfileBool(_T("ExportAllLines"), false);
	//m_bAddBioKrigingButton = option.GetProfileBool("AddBioKrigingLink",false);
	m_bAddClimZoneButton = option.GetProfileBool(_T("AddMatchStationLink"),false);
	
}

COptionBioSIM::~COptionBioSIM()
{
}

void COptionBioSIM::DoDataExchange(CDataExchange* pDX)
{
	CMFCPropertyPage::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_OPTION_SAVEATRUN, m_bSave);
	DDX_Check(pDX, IDC_OPTION_EXPORT_ALL_LINES, m_bExportAllLines);
	
	//DDX_Check(pDX, IDC_OPTION_LINKBIOKRIGING, m_bAddBioKrigingButton);
	DDX_Check(pDX, IDC_OPTION_LINKCLIMZONES, m_bAddClimZoneButton);
	
}


BEGIN_MESSAGE_MAP(COptionBioSIM, CMFCPropertyPage)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COptionBioSIM message handlers



void COptionBioSIM::OnOK() 
{
    UpdateData();

    CAppOption option;
	
    option.WriteProfileBool(_T("SaveAtRun"), m_bSave!=0);
	option.WriteProfileBool(_T("ExportAllLines"), m_bExportAllLines!=0);
//	option.WriteProfileBool("AddBioKrigingLink",m_bAddBioKrigingButton!=0);
	option.WriteProfileBool(_T("AddMatchStationLink"),m_bAddClimZoneButton!=0);


    CMFCPropertyPage::OnOK();
}