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

#include "Basic/Dimension.h"
#include "Basic/Location.h"
#include "Basic/Statistic.h"
#include "VariableSelectionCtrl.h"

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
	BEGIN_MESSAGE_MAP(CStatisticSelectionCtrl, CSelectionCtrl)
	END_MESSAGE_MAP()

	CStatisticSelectionCtrl::CStatisticSelectionCtrl()
	{
		//get only basic statistics
		WBSF::StringVector stats(WBSF::GetString(IDS_STR_STATISTIC), "|;");
		std::string str;
		for (size_t i = 0; i < WBSF::NB_STAT_TYPE; i++)
			str += stats[i] + "|";

		SetPossibleValues(str);
	}

	BEGIN_MESSAGE_MAP(CVariableSelectionCtrl, CXHtmlTree)
		ON_WM_CREATE()
		ON_NOTIFY_REFLECT(NM_DBLCLK, &OnNMDblclk)
	END_MESSAGE_MAP()


	void CVariableSelectionCtrl::PreSubclassWindow()
	{
		CXHtmlTree::PreSubclassWindow();

		_AFX_THREAD_STATE* pThreadState = AfxGetThreadState();
		if (pThreadState->m_pWndInit == NULL)
		{
			Init();
		}
	}

	int CVariableSelectionCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct)
	{
		bool bHaveDrag = lpCreateStruct->style & TVS_DISABLEDRAGDROP;
		if (CXHtmlTree::OnCreate(lpCreateStruct) == -1)
			return -1;

		Init();
		return 0;
	}

	size_t CVariableSelectionCtrl::GetNbField(size_t dimension)const
	{
		size_t nbField = 0;
		switch (dimension)
		{
		case LOCATION:	nbField = CLocation::NB_MEMBER; break;
		case PARAMETER:	nbField = 1; break;
		case REPLICATION:nbField = 1; break;
		case TIME_REF:	nbField = 1; break;
		case VARIABLE:	nbField = m_outputVar.size(); break;
		default: ASSERT(false);
		}

		return nbField;
	}


	CString CVariableSelectionCtrl::GetFieldTitle(int d, int f, bool bTitle)const
	{
		CString title;
		switch (d)
		{
		case LOCATION:	title = CLocation::GetMemberTitle(f); break;
		case PARAMETER:
		case REPLICATION:
		case TIME_REF:	title = CDimension::GetDimensionTitle(d); break;
		case VARIABLE:	title = bTitle ? m_outputVar[f].m_title.c_str() : m_outputVar[f].m_name.c_str(); break;
		default: ASSERT(false);
		}

		return title;
	}

	HTREEITEM CVariableSelectionCtrl::GetItem(int d, int f)const
	{
		HTREEITEM hResultItem = NULL;
		HTREEITEM hItem = GetRootItem();
		if (hItem)
		{
			HTREEITEM hChild = GetChildItem(hItem);
			while (hChild && d > 0)
			{
				d--;
				hChild = GetNextSiblingItem(hChild);
			}

			ASSERT(d == 0);
			HTREEITEM hSubChild = GetChildItem(hChild);
			while (hSubChild && f > 0)
			{
				f--;
				hSubChild = GetNextSiblingItem(hSubChild);
			}

			if (f == 0)
				hResultItem = hSubChild;
		}


		return hResultItem;
	}


	void CVariableSelectionCtrl::Init()
	{
		ASSERT(GetSafeHwnd());

		Initialize(TRUE, TRUE);
		SetSmartCheckBox(TRUE);
		SetSelectFollowsCheck(TRUE);
		SetAutoCheckChildren(TRUE);
		SetHtml(FALSE);
		SetImages(FALSE);

		InitTree();
	}

	void CVariableSelectionCtrl::InitTree()
	{
		if (GetSafeHwnd())
		{
			DeleteAllItems();

			HTREEITEM hItem = InsertItem(UtilWin::GetCString(IDS_STR_SELECT_UNSELECT));
			for (int i = 0; i < NB_DIMENSION; i++)
			{
				HTREEITEM hDimItem = InsertItem(Convert(CDimension::GetDimensionTitle(i)), hItem);
				for (int j = 0; j < GetNbField(i); j++)
				{
					InsertItem(GetFieldTitle(i, j), hDimItem);
				}
			}

			ExpandAll();
		}
	}

	void CVariableSelectionCtrl::SetData(const CVariableDefineVector& data)
	{
		ASSERT(GetSafeHwnd());


		for (int i = 0; i < data.size(); i++)
		{
			int d = data[i].m_dimension;
			int f = data[i].m_field;
			HTREEITEM hItem = GetItem(d, f);
			if (hItem)
			{
				SetCheck(hItem, TRUE);
				//SetItemText(hItem, data[i].m_str);
			}
		}
	}

	void CVariableSelectionCtrl::GetData(CVariableDefineVector& data)
	{
		ASSERT(GetSafeHwnd());

		data.clear();

		HTREEITEM hItem = GetRootItem();
		//if(hItem)
		//	hItem = GetChildItem(hItem); 

		//int nbDim = GetChildrenCount(hItem);
		//ASSERT( nbDim == NB_DIMENSION);
		HTREEITEM hDimItem = GetChildItem(hItem);

		for (int d = 0; d < NB_DIMENSION; d++)
		{
			int nbField = GetChildrenCount(hDimItem);
			HTREEITEM hSubItem = GetChildItem(hDimItem);
			for (int f = 0; f < nbField; f++)
			{
				if (GetCheck(hSubItem))
					data.push_back(CVariableDefine(d, f/*,GetItemText(hSubItem)*/));

				hSubItem = GetNextSiblingItem(hSubItem);
			}

			hDimItem = GetNextSiblingItem(hDimItem);
		}
	}

	void CVariableSelectionCtrl::OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult)
	{
		*pResult = TRUE;
	}

	void AFXAPI DDX_Selection(CDataExchange* pDX, int ID, CVariableDefineVector& selection)
	{
		CVariableSelectionCtrl* pCtrl = dynamic_cast<CVariableSelectionCtrl*>(pDX->m_pDlgWnd->GetDlgItem(ID));
		ASSERT(pCtrl);

		if (pDX->m_bSaveAndValidate)
		{
			pCtrl->GetData(selection);
		}
		else
		{
			pCtrl->SetData(selection);
		}
	}

}