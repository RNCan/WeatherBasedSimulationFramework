// OptionGeneral.cpp : implementation file
//

#include "stdafx.h"
#include "OptionsDPT.h"
#include "Registry.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace CFL;

/////////////////////////////////////////////////////////////////////////////
// COptionsDPT property page 

//IMPLEMENT_DYNCREATE(COptionsDPT, CMFCPropertyPage)

COptionsDPT::COptionsDPT() : CMFCPropertyPage(COptionsDPT::IDD)
{
 //   CRegistry option;

 //   m_bSave = option.GetProfileBool("SaveAtRun", false);
	//m_bExportAllLines = option.GetProfileBool("ExportAllLines", false);
	////m_bAddBioKrigingButton = option.GetProfileBool("AddBioKrigingLink",false);
	//m_bAddClimZoneButton = option.GetProfileBool("AddMatchStationLink",false);
	
}

COptionsDPT::~COptionsDPT()
{
}

void COptionsDPT::DoDataExchange(CDataExchange* pDX)
{
	CMFCPropertyPage::DoDataExchange(pDX);
	//DDX_Check(pDX, IDC_OPTION_SAVEATRUN, m_bSave);
	//DDX_Check(pDX, IDC_OPTION_EXPORT_ALL_LINES, m_bExportAllLines);
	//
	////DDX_Check(pDX, IDC_OPTION_LINKBIOKRIGING, m_bAddBioKrigingButton);
	//DDX_Check(pDX, IDC_OPTION_LINKCLIMZONES, m_bAddClimZoneButton);
	//
}


BEGIN_MESSAGE_MAP(COptionsDPT, CMFCPropertyPage)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COptionsDPT message handlers



void COptionsDPT::OnOK() 
{
    UpdateData();

//    CRegistry option;
//	
//    option.WriteProfileBool("SaveAtRun", m_bSave!=0);
//	option.WriteProfileBool("ExportAllLines", m_bExportAllLines!=0);
////	option.WriteProfileBool("AddBioKrigingLink",m_bAddBioKrigingButton!=0);
//	option.WriteProfileBool("AddMatchStationLink",m_bAddClimZoneButton!=0);


    CMFCPropertyPage::OnOK();
}