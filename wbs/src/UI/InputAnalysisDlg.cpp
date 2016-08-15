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
#include "UI/Common/SYShowMessage.h"
#include "UI/Common/CustomDDX.h"
#include "UI/Common/UtilWin.h"
#include "InputAnalysisDlg.h"

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
	// CInputAnalysisDlg dialog

	BEGIN_MESSAGE_MAP(CInputAnalysisDlg, CDialog)
	END_MESSAGE_MAP()


	CInputAnalysisDlg::CInputAnalysisDlg(const CExecutablePtr& pParent, CWnd* pParentWnd/*=NULL*/) :
		CDialog(CInputAnalysisDlg::IDD, pParentWnd),
		m_pParent(pParent)
	{
	}



	void CInputAnalysisDlg::DoDataExchange(CDataExchange* pDX)
	{
		CDialog::DoDataExchange(pDX);

		DDX_Text(pDX, IDC_NAME, m_analysis.m_name);
		DDX_Text(pDX, IDC_DESCRIPTION, m_analysis.m_description);
		DDX_Text(pDX, IDC_INTERNAL_NAME, m_analysis.m_internalName);
		DDX_CBIndex(pDX, IDC_KIND, m_analysis.m_kind);

	}



}