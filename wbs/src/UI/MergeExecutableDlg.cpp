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
		ON_NOTIFY(NM_DBLCLK, IDC_EXECUTABLE_TREE, OnDblClk)
		ON_REGISTERED_MESSAGE(WM_XHTMLTREE_CHECKBOX_CLICKED, OnCheckbox)
	END_MESSAGE_MAP()


	CMergeExecutableDlg::CMergeExecutableDlg(const CExecutablePtr& pParent, CWnd* pParentWnd) :
		CDialog(CMergeExecutableDlg::IDD, pParentWnd),
		m_pParent(pParent->CopyObject())
	{
	}

	void SetCheck(CExecutablePtr& pItem, CMergeExecutable& merge)
	{
		for (int i = 0; i < pItem->GetNbItem(); i++)
		{
			CExecutablePtr pItem = pItem->GetItemAt(i);
			string iName = pItem->GetInternalName();
			bool bCheck = merge.m_mergedArray.Find(iName) != UNKNOWN_POS;
			pItem->SetExecute(bCheck);
			SetCheck(pItem, merge);
		}
	}
	
	void GetCheck(CExecutablePtr& pItem, CMergeExecutable& merge)
	{
		for (int i = 0; i < pItem->GetNbItem(); i++)
		{
			CExecutablePtr pItem = pItem->GetItemAt(i);
			string iName = pItem->GetInternalName();
			if (pItem->GetExecute())
				merge.m_mergedArray.push_back(iName);
			
			
			GetCheck(pItem, merge);
		}
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


		if (pDX->m_bSaveAndValidate)
		{
			m_merge.m_mergedArray.clear();
			//GetCheck(m_pParent, m_merge);
			m_executableCtrl.GetCheckedItem(m_merge.m_mergedArray);
		}
		else
		{
			//SetCheck(m_pParent, m_merge);
			
				


			m_executableCtrl.SetHideItem(m_merge.GetInternalName());
			//CProjectStatePtr pProjectState = make_shared<CProjectState>();
			
			//pProjectState->m_expendedItems.insert(m_pParent->GetInternalName());
			m_executableCtrl.SetExecutable(m_pParent, CProjectStatePtr());
			m_executableCtrl.ExpandAll();

			m_executableCtrl.SetCheckedItem(m_merge.m_mergedArray);
		}

	}

	void CMergeExecutableDlg::OnDblClk(NMHDR *pNMHDR, LRESULT *pResult)
	{
		*pResult = TRUE;
		//do nothing
	}

	LRESULT CMergeExecutableDlg::OnCheckbox(WPARAM wParam, LPARAM lParam)
	{
		//m_merge
		/*XHTMLTREEMSGDATA *pData = (XHTMLTREEMSGDATA *)wParam;
		ASSERT(pData);

		BOOL bChecked = lParam;

		if (pData)
		{
			HTREEITEM hItem = pData->hItem;

			if (hItem)
			{
				string iName = m_executableCtrl.GetInternalName(hItem);

				CExecutablePtr pItem = (iName == m_pParent->GetInternalName()) ? m_pRoot : m_pRoot->FindItem(iName);
				ASSERT(pItem);
				pItem->SetExecute(bChecked);

			}
		}*/

		return 0;
		//return m_executableCtrl.OnCheckbox(wParam, lParam);
	}

}