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
#include "TaskTreeCtrl.h"
//#include "Tasks/TaskFactory.h"
//#include "ProjectView.h"

#include "WeatherBasedSimulationString.h"
#include "WeatherBasedSimulationUI.h"
#include "resource.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


using namespace std;
using namespace WBSF;

	/////////////////////////////////////////////////////////////////////////////
	// CTaskTreeCtrl

	BEGIN_MESSAGE_MAP(CTaskTreeCtrl, CXHtmlTree)
		ON_WM_CREATE()
		ON_UPDATE_COMMAND_UI(ID_ITEM_EXPAND_ALL, OnUpdateToolBar)
		ON_UPDATE_COMMAND_UI(ID_ITEM_COLLAPSE_ALL, OnUpdateToolBar)
		ON_NOTIFY_REFLECT(NM_DBLCLK, &OnDblClick)
	END_MESSAGE_MAP()


	CTaskTreeCtrl::CTaskTreeCtrl()
	{
	}

	CTaskTreeCtrl::~CTaskTreeCtrl()
	{
	}


	void CTaskTreeCtrl::PreSubclassWindow()
	{
		CXHtmlTree::PreSubclassWindow();

		_AFX_THREAD_STATE* pThreadState = AfxGetThreadState();
		if (pThreadState->m_pWndInit == NULL)
		{
			Init();
		}
	}

	int CTaskTreeCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct)
	{
		if (CXHtmlTree::OnCreate(lpCreateStruct) == -1)
			return -1;

		Init();
		return 0;
	}


	
	
	XHTMLTREEDATA CTaskTreeCtrl::GetExtraData()
	{
		XHTMLTREEDATA xhtd;

		xhtd.bChecked = TRUE;
		xhtd.bEnabled = FALSE;

		return xhtd;
	}

	TVINSERTSTRUCT CTaskTreeCtrl::GetInsertStruct(CString& name, int imageIndex, HTREEITEM hParent, HTREEITEM hInsertAfter)
	{
		TVINSERTSTRUCT tvis = { 0 };
		tvis.item.mask = TVIF_TEXT;
		tvis.item.pszText = name.LockBuffer();
		tvis.item.cchTextMax = name.GetLength();
		tvis.hParent = hParent;
		tvis.hInsertAfter = hInsertAfter != NULL ? hInsertAfter:TVI_FIRST;
		tvis.item.iImage = imageIndex;
		tvis.item.iSelectedImage = imageIndex;
		tvis.item.mask |= TVIF_IMAGE | TVIF_SELECTEDIMAGE;
		//tvis.item.mask          |= TVIF_PARAM;
		//tvis.item.lParam         = 0;
		return tvis;
	}
	
	void CTaskTreeCtrl::Init()
	{
		ASSERT(GetSafeHwnd()); 

		m_taskImages.Create(16, 16, ILC_COLOR32, 0, 0);

		CPngImage image;
		CBitmap bitmap;

		image.Load(IDR_TASK, AfxGetInstanceHandle());
		bitmap.Attach(image.Detach());
		m_taskImages.Add(&bitmap, RGB(255, 255, 255));
		bitmap.Detach();

		image.Load(IDR_TASK_TYPE, AfxGetInstanceHandle());
		bitmap.Attach(image.Detach());
		m_taskImages.Add(&bitmap, RGB(255, 255, 255));

		SetImageList(&m_taskImages, TVSIL_NORMAL);
		Initialize(TRUE, TRUE);
		SetSmartCheckBox(FALSE);
		SetSelectFollowsCheck(FALSE);
		SetAutoCheckChildren(TRUE);
		SetHtml(FALSE);
		SetImages(TRUE);
		SetDragOps(XHTMLTREE_DO_CTRL_KEY);
		SetDropCursors(IDC_CMN_NODROP, IDC_CMN_DROPCOPY, IDC_CMN_DROPMOVE);
	}

	//add root
	void CTaskTreeCtrl::AddRoot(size_t t, UINT iImageIndex, CString name)
	{
		if (name.IsEmpty())
			name = CString(CTaskBase::GetTypeTitle(t).c_str());

		TVINSERTSTRUCT tvis = GetInsertStruct(name, iImageIndex, NULL, NULL);
		XHTMLTREEDATA xhtd;
		xhtd.bExpanded = true;
		xhtd.bChecked = false;

		HTREEITEM hItem = InsertItem(&tvis, &xhtd);

	}
	
	size_t CTaskTreeCtrl::GetPosition(HTREEITEM hItemIn)const
	{
		size_t pos=NOT_INIT;
		CTaskTreeCtrl& me = const_cast<CTaskTreeCtrl&>(*this);
		
		HTREEITEM hItem = GetRootItem();
		while (hItem != NULL && hItem != hItemIn)
		{
			pos++;
			hItem = me.GetNextItem(hItem);
		}
		
		if (hItem == NULL)
			pos = UNKNOWN_POS;


		return pos;
	}

	
	HTREEITEM CTaskTreeCtrl::FindItem(size_t p)const
	{
		CTaskTreeCtrl& me = const_cast<CTaskTreeCtrl&>(*this);
		if (p == NOT_INIT)
			return NULL;

		size_t i = NOT_INIT;
		HTREEITEM hItem = GetRootItem();
		while (++i<=p && hItem!=NULL)
			hItem = me.GetNextItem(hItem);

		return hItem;
	}

	size_t CTaskTreeCtrl::InsertTask(WBSF::CTaskPtr pTask, UINT imageIndex, HTREEITEM hInsertAfter)
	{
		ASSERT(pTask!=NULL);
		ASSERT(imageIndex != -1);

		HTREEITEM hRoot = GetRootItem();

		CString name(pTask->m_name.c_str());
		bool bExecute = pTask->m_bExecute;
		bool bExpand = true;

		TVINSERTSTRUCT tvis = GetInsertStruct(name, imageIndex, hRoot, hInsertAfter);
		XHTMLTREEDATA xhtd;
		xhtd.bExpanded = bExpand;
		xhtd.bChecked = bExecute;

		//Add this element to the tree
		HTREEITEM hItem = InsertItem(&tvis, &xhtd);

		return GetPosition(hItem);

	}

	void CTaskTreeCtrl::OnUpdateToolBar(CCmdUI *pCmdUI)
	{
		pCmdUI->Enable(true);
	}

	void CTaskTreeCtrl::OnDblClick(NMHDR*, LRESULT* pResult)
	{
		*pResult = TRUE;
	}

	BOOL CTaskTreeCtrl::OnItemMove(UINT ID)
	{
		HTREEITEM hItem = CTaskTreeCtrl::GetSelectedItem();
		
		if( hItem && ID==ID_ITEM_EXPAND_ALL)
			CTaskTreeCtrl::Expand(hItem, TVE_EXPAND);
	
		if( hItem )
			hItem = CTaskTreeCtrl::GetChildItem(hItem);
	
		if( hItem )
		{
			do 
			{
				switch(ID)
				{
				case ID_ITEM_EXPAND_ALL: CTaskTreeCtrl::ExpandBranch(hItem); break;
				case ID_ITEM_COLLAPSE_ALL: CTaskTreeCtrl::CollapseBranch(hItem); break;
				default: ASSERT( false);
				}
	
			} while ((hItem = CTaskTreeCtrl::GetNextSiblingItem(hItem)) != NULL);
		}
	
		return TRUE;
	}

	LRESULT CTaskTreeCtrl::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
	{
		return CXHtmlTree::WindowProc(message, wParam, lParam);
	}

	BOOL CTaskTreeCtrl::OnCommand(WPARAM wParam, LPARAM lParam)
	{
		return CXHtmlTree::OnCommand(wParam, lParam);
	}

	void AFXAPI DDX_Selection(CDataExchange* pDX, int ID, WBSF::StringVector& data)
	{
		CTaskTreeCtrl* pCtrl = dynamic_cast<CTaskTreeCtrl*>(pDX->m_pDlgWnd->GetDlgItem(ID));
		ASSERT(pCtrl);

		/*if (pDX->m_bSaveAndValidate)
		{
			pCtrl->GetCheckedItem(data);
		}
		else
		{
			pCtrl->SetCheckedItem(data);
		}*/
	}

