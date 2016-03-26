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

#include "FileManager/FileManager.h"
#include "UI/Common/SYShowMessage.h"
#include "UI/Common/CustomDDX.h"
#include "UI/Common/UtilWin.h"
#include "UI/Common/CommonCtrl.h"
#include "ScriptDlg.h"

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
	// CScriptDlg dialog

	BEGIN_MESSAGE_MAP(CScriptDlg, CDialog)
	END_MESSAGE_MAP()


	CScriptDlg::CScriptDlg(const CExecutablePtr& pParent, CWnd* pParentWnd/*=NULL*/) :
		CDialog(CScriptDlg::IDD, pParentWnd),
		m_pParent(pParent)
	{
	}



	void CScriptDlg::DoDataExchange(CDataExchange* pDX)
	{
		CDialog::DoDataExchange(pDX);

		if (!pDX->m_bSaveAndValidate)
		{
			WBSF::StringVector list = WBSF::GetFM().Script().GetFilesList();
			CCFLComboBox* pList = (CCFLComboBox*)GetDlgItem(IDC_KIND);
			pList->FillList(list);
			pList->SelectStringExact(0, m_script.m_fileTitle);
		}


		DDX_Text(pDX, IDC_NAME, m_script.m_name);
		DDX_Text(pDX, IDC_DESCRIPTION, m_script.m_description);
		DDX_Text(pDX, IDC_INTERNAL_NAME, m_script.m_internalName);
		DDX_Text(pDX, IDC_KIND, m_script.m_fileTitle);

	}



}