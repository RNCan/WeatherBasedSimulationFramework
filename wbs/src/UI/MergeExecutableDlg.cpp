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

#include "Simulation/MergeExecutable.h"
#include "FileManager/FileManager.h"
#include "UI/Common/CustomDDX.h"
#include "UI/Common/UtilWin.h"

#include "MergeExecutableDlg.h"


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
	// CMergeExecutableDlg dialog

	BEGIN_MESSAGE_MAP(CMergeExecutableDlg, CDialog)
		ON_REGISTERED_MESSAGE(WM_XHTMLTREE_CHECKBOX_CLICKED, OnCheckbox)
	END_MESSAGE_MAP()


	CMergeExecutableDlg::CMergeExecutableDlg(const CExecutablePtr& pParent, CWnd* pParentWnd) :
		CDialog(CMergeExecutableDlg::IDD, pParentWnd),
		m_pParent(pParent->CopyObject()),
		m_executableCtrl(false)
	{
	}


	void CMergeExecutableDlg::DoDataExchange(CDataExchange* pDX)
	{
		CDialog::DoDataExchange(pDX);

		DDX_Control(pDX, IDC_EXECUTABLE_TREE, m_executableCtrl);

		DDX_Text(pDX, IDC_NAME, m_merge.m_name);
		DDX_Text(pDX, IDC_INTERNAL_NAME, m_merge.m_internalName);
		DDX_Text(pDX, IDC_DESCRIPTION, m_merge.m_description);
		DDX_Selection(pDX, IDC_EXECUTABLE_TREE, m_merge.m_mergedArray);
		DDX_CBIndex(pDX, IDC_DIMENSION_MERGED, m_merge.m_dimensionAppend);
		DDX_Check(pDX, IDC_SIM_ADD_NAME, m_merge.m_bAddName);
		


		if (pDX->m_bSaveAndValidate)
		{
			m_merge.m_mergedArray.clear();
			m_executableCtrl.GetCheckedItem(m_merge.m_mergedArray);
		}
		else
		{
			m_executableCtrl.SetHideItem(m_merge.GetInternalName());
			m_executableCtrl.SetExecutable(m_pParent, CProjectStatePtr());
			m_executableCtrl.ExpandAll();
			m_executableCtrl.SetCheckedItem(m_merge.m_mergedArray);
		}

	}

	
	LRESULT CMergeExecutableDlg::OnCheckbox(WPARAM wParam, LPARAM lParam)
	{
		return 0;
	}

}