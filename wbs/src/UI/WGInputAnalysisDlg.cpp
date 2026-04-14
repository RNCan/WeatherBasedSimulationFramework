//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
// 20-06/2018	Rémi Saint-Amant	Add export button
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
//****************************************************************************
#include "stdafx.h"
#include "UI/Common/SYShowMessage.h"
#include "UI/Common/CustomDDX.h"
#include "UI/Common/UtilWin.h"
#include "WGInputAnalysisDlg.h"

using namespace std;
using namespace WBSF::DIMENSION;


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace WBSF
{


	//*************************************************************************************************
	// CWGInputAnalysisDlg dialog

	BEGIN_MESSAGE_MAP(CWGInputAnalysisDlg, CDialog)
		ON_CBN_SELCHANGE(IDC_KIND, &UpdateCtrl)
		ON_BN_CLICKED(IDC_EXPORT_MATCH, &UpdateCtrl)
		ON_BN_CLICKED(IDC_EXPORT_NORMALS, &UpdateCtrl)
		
	END_MESSAGE_MAP()


	CWGInputAnalysisDlg::CWGInputAnalysisDlg(const CExecutablePtr& pParent, CWnd* pParentWnd/*=NULL*/) :
		CDialog(CWGInputAnalysisDlg::IDD, pParentWnd),
		m_pParent(pParent)
	{
	}



	void CWGInputAnalysisDlg::DoDataExchange(CDataExchange* pDX)
	{
		CDialog::DoDataExchange(pDX);

		DDX_Text(pDX, IDC_NAME, m_analysis.m_name);
		DDX_Text(pDX, IDC_DESCRIPTION, m_analysis.m_description);
		DDX_Text(pDX, IDC_INTERNAL_NAME, m_analysis.m_internalName);
		DDX_CBIndex(pDX, IDC_KIND, m_analysis.m_kind);
		DDX_Check(pDX, IDC_EXPORT_MATCH, m_analysis.m_bExportMatch);
		DDX_Check(pDX, IDC_EXPORT_NORMALS, m_analysis.m_bExportNormal);
		DDX_Text(pDX, IDC_MATCH_NAME, m_analysis.m_matchName);
		DDX_Text(pDX, IDC_DB_NAME, m_analysis.m_normalsName);
		DDX_Control(pDX, IDC_KIND, m_kindCtrl);
		DDX_Control(pDX, IDC_EXPORT_MATCH, m_exportCtrl);
		DDX_Control(pDX, IDC_EXPORT_NORMALS, m_exportNormalCtrl);

		UpdateCtrl();
	}



	void CWGInputAnalysisDlg::UpdateCtrl()
	{
		bool bShowExportNormal = m_kindCtrl.GetCurSel() == CWGInputAnalysis::EXTRACT_NORMALS;
		bool bEnable = m_kindCtrl.GetCurSel() == CWGInputAnalysis::MATCH_STATION_NORMALS || m_kindCtrl.GetCurSel() == CWGInputAnalysis::MATCH_STATION_OBSERVATIONS;
		GetDlgItem(IDC_CMN_STATIC1)->EnableWindow(bEnable);
		GetDlgItem(IDC_EXPORT_MATCH)->EnableWindow(bEnable);

		GetDlgItem(IDC_EXPORT_MATCH)->ShowWindow(bShowExportNormal ? SW_HIDE : SW_SHOW);
		GetDlgItem(IDC_MATCH_NAME)->ShowWindow(bShowExportNormal ? SW_HIDE : SW_SHOW);
		GetDlgItem(IDC_EXPORT_NORMALS)->ShowWindow(bShowExportNormal ? SW_SHOW: SW_HIDE);
		GetDlgItem(IDC_DB_NAME)->ShowWindow(bShowExportNormal ? SW_SHOW : SW_HIDE);


		bool bEnableMatch = bEnable && m_exportCtrl.GetCheck() != 0 || m_exportNormalCtrl.GetCheck() != 0;
		GetDlgItem(IDC_CMN_STATIC2)->EnableWindow(bEnableMatch);
		GetDlgItem(IDC_MATCH_NAME)->EnableWindow(bEnableMatch);

		

	}

}