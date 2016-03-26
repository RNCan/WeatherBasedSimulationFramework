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

#include "ModelFormItem.h"
#include "ModelPages.h"

#include "ModelFormDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace WBSF
{

	/////////////////////////////////////////////////////////////////////////////
	// CModelFormDialog dialog


	CModelFormDialog::CModelFormDialog() :
		m_pParent(NULL)
	{
		//{{AFX_DATA_INIT(CModelFormDialog)
		//}}AFX_DATA_INIT
	}

	CModelFormDialog::~CModelFormDialog()
	{
		int nCount = (int)m_ItemsList.GetSize();

		for (int i = 0; i < nCount; i++)
			delete m_ItemsList[i];

		m_ItemsList.RemoveAll();
	}


	void CModelFormDialog::DoDataExchange(CDataExchange* pDX)
	{
		CDialog::DoDataExchange(pDX);
		//{{AFX_DATA_MAP(CModelFormDialog)
		// NOTE: the ClassWizard will add DDX and DDV calls here
		//}}AFX_DATA_MAP
	}


	BEGIN_MESSAGE_MAP(CModelFormDialog, CDialog)
		//{{AFX_MSG_MAP(CModelFormDialog)
		ON_WM_DESTROY()
		ON_WM_LBUTTONDOWN()
		//}}AFX_MSG_MAP
	END_MESSAGE_MAP()

	/////////////////////////////////////////////////////////////////////////////
	// CModelFormDialog message handlers

	void CModelFormDialog::OnCancel()
	{
	}

	void CModelFormDialog::OnOK()
	{
	}

	BOOL CModelFormDialog::OnInitDialog()
	{
		CDialog::OnInitDialog();

		ASSERT(m_hWnd);
		//GetClientRect(m_rect );

		return TRUE;  // return TRUE unless you set the focus to a control
		// EXCEPTION: OCX Property Pages should return FALSE
	}

	void CModelFormDialog::OnDestroy()
	{
		ASSERT(m_hWnd);
		m_rect = GetRect();

		CDialog::OnDestroy();
	}

	const CRect& CModelFormDialog::GetRect()
	{
		if (GetSafeHwnd())
			GetWindowRect(m_rect);

		return m_rect;
	}


	void CModelFormDialog::OnLButtonDown(UINT nFlags, CPoint point)
	{
		m_pParent->SendMessage(MY_LCLICK, nFlags, (LPARAM)&point);

		CDialog::OnLButtonDown(nFlags, point);
	}


	void CModelFormDialog::AddItem(CModelInputParameterDef& refInVar)
	{
		CModelFormItem* pItem = (CModelFormItem*) new CModelFormItem(refInVar);
		ASSERT(pItem);

		int pos = (int)m_ItemsList.Add(pItem);

		// create the item window;
		VERIFY(pItem->Create(this, pos));
	}

	void CModelFormDialog::UpdateItem(int nID)
	{
		ASSERT(nID >= 0 && nID < m_ItemsList.GetSize());
		m_ItemsList[nID]->Invalidate();
	}

	void CModelFormDialog::SetCurSel(int nID)
	{
		if (nID < m_ItemsList.GetSize() && m_ItemsList[nID])
		{
			m_ItemsList[nID]->SetFocus();
		}
	}

	void CModelFormDialog::DeleteItem(int nID)
	{
		delete m_ItemsList[nID];
		m_ItemsList.RemoveAt(nID);

		Invalidate();
	}

	void CModelFormDialog::DeleteAllItem()
	{
		for (int i = (int)m_ItemsList.GetSize() - 1; i >= 0; i--)
		{
			delete m_ItemsList[i];
			m_ItemsList.RemoveAt(i);
		}

		Invalidate();
	}

	BOOL CModelFormDialog::Create(const RECT& rect, UINT nIDTemplate, CModelInputParameterCtrl* pParentWnd)
	{
		ASSERT(pParentWnd);
		m_pParent = pParentWnd; // because Create is unable to associate a parent

		BOOL rep = CDialog::Create(nIDTemplate, pParentWnd);

		m_rect = rect;

		//m_rect += CPoint(parentRect.right+20, (parentRect.Height()/2));
		MoveWindow(m_rect);

		return rep;
	}

	// the id is not valid after a DeleteItem
	int CModelFormDialog::GetItemId(CString& name)const
	{
		int nID = -1;
		int nCount = (int)m_ItemsList.GetSize();

		for (int i = 0; i < nCount; i++)
		{
			if (m_ItemsList[i]->GetName() == name)
			{
				nID = i;
				break;
			}
		}

		ASSERT(nID != -1);

		return nID;
	}


	void CModelFormDialog::OnItemActivation(const CString& name)const
	{
		ASSERT(m_pParent);
		m_pParent->OnItemActivation(name);
	}

	void CModelFormDialog::OnUpdateParent(const CString& name)
	{
		m_pParent->UpdateProperties(name);
	}

}