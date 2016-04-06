// ProListCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "ProListCtrl.h"
#include "TextProgressCtrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CProListCtrl

CProListCtrl::CProListCtrl()
{}

CProListCtrl::~CProListCtrl()
{}


BEGIN_MESSAGE_MAP(CProListCtrl, CListCtrl)
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, OnCustomDraw)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CProListCtrl message handlers
void CProListCtrl::OnCustomDraw( NMHDR* pNMHDR, LRESULT* pResult )
{
	NMLVCUSTOMDRAW* pLVCD = (NMLVCUSTOMDRAW*)pNMHDR;

	*pResult = CDRF_DODEFAULT;

	if (CDDS_PREPAINT == pLVCD->nmcd.dwDrawStage)
	{
		*pResult = CDRF_NOTIFYITEMDRAW;
		return;
	}
	else if (CDDS_ITEMPREPAINT == pLVCD->nmcd.dwDrawStage)
	{
		*pResult = CDRF_NOTIFYSUBITEMDRAW;
		return;
	}
	else if ( (CDDS_SUBITEM | CDDS_ITEMPREPAINT) == pLVCD->nmcd.dwDrawStage )
	{
		*pResult = CDRF_NOTIFYPOSTPAINT;
		return;
	}
	else if ( (CDDS_SUBITEM | CDDS_ITEMPOSTPAINT) == pLVCD->nmcd.dwDrawStage )
	{
		int nItem = (int)pLVCD->nmcd.dwItemSpec;
		int nSubItem = pLVCD->iSubItem;
		if (nSubItem!=0)
			return;
		
		CRect rcSubItem;
		GetSubItemRect(nItem, nSubItem, LVIR_BOUNDS, rcSubItem);
		rcSubItem.right = GetColumnWidth(0);
		
		CTextProgressCtrl* pCtrl = (CTextProgressCtrl*)this->GetItemData(nItem);
		if (NULL == pCtrl)
		{
			pCtrl = new CTextProgressCtrl;
			//if (rcSubItem.Width() > 250)
				//rcSubItem.right = rcSubItem.left + 250;

			pCtrl->Create(WS_CHILD|WS_VISIBLE|PBS_SMOOTH, rcSubItem, this, 0x1000 + nItem);
			ASSERT(pCtrl->GetSafeHwnd());
			pCtrl->SetRange32(0, 100);
			pCtrl->SetPos( 0 );
			pCtrl->SetShowPercent(true);
			this->SetItemData(nItem, (DWORD)pCtrl);
		}
			
		//if (rcSubItem.Width() > 250)
			//rcSubItem.right = rcSubItem.left + 250;

		pCtrl->MoveWindow(rcSubItem);
		pCtrl->ShowWindow(SW_SHOW);
		*pResult = CDRF_SKIPDEFAULT;
		return;
	}
}

void CProListCtrl::OnHScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CListCtrl::OnHScroll(nSBCode, nPos, pScrollBar);
	InvalidateProgressCtrls();
}

void CProListCtrl::OnVScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CListCtrl::OnVScroll(nSBCode, nPos, pScrollBar);
	InvalidateProgressCtrls();
}

void CProListCtrl::InvalidateProgressCtrls()
{
	int nFirst = GetTopIndex();
	int nLast = nFirst + GetCountPerPage();

	//Hide the other items.
	int nCount = this->GetItemCount();
	CTextProgressCtrl* pCtrl;
	for (int i = 0; i < std::min(nFirst, nCount); i++)
	{
		pCtrl = (CTextProgressCtrl*)this->GetItemData(i);
		if (NULL != pCtrl)
			pCtrl->ShowWindow(SW_HIDE);		
	}
	for(int i = nLast; i < nCount; i++)
	{
		pCtrl = (CTextProgressCtrl*)this->GetItemData(i);
		if (NULL != pCtrl)
			pCtrl->ShowWindow(SW_HIDE);		
	}

	//Invalidate
	CRect rc(0,0,0,0);
	CRect rcSubItem;
	for(; nFirst < std::min(nLast,nCount); nFirst++)
	{
		GetSubItemRect(nFirst, 0, LVIR_BOUNDS, rcSubItem);
		VERIFY( rc.UnionRect(rc, rcSubItem) );
	}

	InvalidateRect(rc);
}

BOOL CProListCtrl::DeleteItem(int nItem)
{
	ASSERT(nItem >= 0 && nItem < GetItemCount());
	CTextProgressCtrl* pCtrl = (CTextProgressCtrl*)GetItemData(nItem);
	if (NULL != pCtrl)
		delete pCtrl;

	return CListCtrl::DeleteItem(nItem);
}

void CProListCtrl::OnDestroy()
{
	int nCount = GetItemCount();
	for (int i = nCount-1; i >= 0; i--)
		DeleteItem(i);
}
