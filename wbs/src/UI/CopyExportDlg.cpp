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
#include "UI/Common/UtilWin.h"
#include "UI/Common/CustomDDX.h"
#include "CopyExportDlg.h"

#include "WeatherBasedSimulationString.h"


using namespace UtilWin;
using namespace WBSF::DIMENSION;


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace WBSF
{

	//*************************************************************************************************
	// CCopyExportDlg dialog

	BEGIN_MESSAGE_MAP(CCopyExportDlg, CDialog)
	END_MESSAGE_MAP()


	CCopyExportDlg::CCopyExportDlg(const CExecutablePtr& pParent, CWnd* pParentWnd/*=NULL*/) :
		CDialog(CCopyExportDlg::IDD, pParentWnd),
		m_pParent(pParent)
	{
	}



	void CCopyExportDlg::DoDataExchange(CDataExchange* pDX)
	{
		CDialog::DoDataExchange(pDX);

		DDX_Text(pDX, IDC_NAME, m_component.m_name);
		DDX_Text(pDX, IDC_DESCRIPTION, m_component.m_description);
		DDX_Text(pDX, IDC_INTERNAL_NAME, m_component.m_internalName);
		DDX_Text(pDX, IDC_OUTPUT_FILE_PATH, m_component.m_outputFilePath);
		DDX_Text(pDX, IDC_USER_NAME, m_component.m_userName);
		DDX_Text(pDX, IDC_PASSWORD, m_component.m_password);

	}



}