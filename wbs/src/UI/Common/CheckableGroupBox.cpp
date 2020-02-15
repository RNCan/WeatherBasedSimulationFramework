// CheckableGroupBox.cpp : implementation file
//

#include "stdafx.h"
#include "CheckableGroupBox.h"

#pragma warning( disable : 4786 )
#include <list>

using namespace std;

#define ID_TITLE	0xFFFF

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCheckableGroupBox
IMPLEMENT_DYNAMIC( CCheckableGroupBox, CButton )


CCheckableGroupBox::CCheckableGroupBox()
{
}

CCheckableGroupBox::~CCheckableGroupBox()
{
}



BEGIN_MESSAGE_MAP(CCheckableGroupBox, CButton)
	//{{AFX_MSG_MAP(CCheckableGroupBox)
	ON_WM_ENABLE()
	ON_WM_SETFOCUS()
	//}}AFX_MSG_MAP
	ON_COMMAND( ID_TITLE , OnClicked)
	ON_MESSAGE(BM_GETCHECK, OnGetCheck)
	ON_MESSAGE(BM_SETCHECK, OnSetCheck)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCheckableGroupBox message handlers


void CCheckableGroupBox::SetTitleStyle(UINT style)
{
	CString strText;
	GetWindowText(strText);

	CClientDC dc(this);
	CFont* pOldFont = dc.SelectObject(GetFont());
	CSize czText = dc.GetTextExtent(strText);
	dc.SelectObject(pOldFont);
	// Add some space for the checkbox and at the end
	czText.cx += 25;

	// Move the checkbox on top of the groupbox
	CRect rc;
	GetWindowRect(rc);	
	this->ScreenToClient(rc);
	rc.left += 5;
	rc.right = rc.left + czText.cx;
	rc.bottom = rc.top + czText.cy;

	if(style == BS_AUTOCHECKBOX || style == BS_AUTORADIOBUTTON)
	{
		m_TitleBox.Create(strText, style | WS_CHILD  , rc, this, ID_TITLE);
		m_TitleBox.SetFont(GetFont(), true);
		m_TitleBox.ShowWindow(SW_SHOW);
	}
}

/**
* If current m_TitleBox is a radio button, search all controls in current parent
* window, uncheck all other radio buttons if they have the same group id
*/
void CCheckableGroupBox::OnClicked() 
{
	CheckGroupboxControls();
	UINT style = m_TitleBox.GetButtonStyle();
	if(style == BS_AUTORADIOBUTTON)
	{
		CWnd* pWnd = GetParent()->GetWindow(GW_CHILD);
		CRect rcWnd, rcTest;

		while (pWnd)
		{
			if(pWnd->IsKindOf(RUNTIME_CLASS(CCheckableGroupBox)))
			{
				CCheckableGroupBox* pT = (CCheckableGroupBox*)pWnd;
				if(pT->GetGroupID() == m_nGroupID && pT != this)
				{
					pT->SetCheck(0);
				}
			}
			pWnd = pWnd->GetWindow(GW_HWNDNEXT);
		}		
	}
	
	::SendMessage(GetParent()->m_hWnd,
					WM_COMMAND, 
					MAKEWPARAM(::GetDlgCtrlID(m_hWnd), BN_CLICKED),
					(LPARAM)m_hWnd);
	
}

void CCheckableGroupBox::CheckGroupboxControls()
{
	int nCheck = m_TitleBox.GetCheck();
	CRect rcGroupbox;
	GetWindowRect(rcGroupbox);
	list<CCheckableGroupBox*> mapChkableGroupWnds;

	// Get first child control
	CWnd* pWnd = GetParent()->GetWindow(GW_CHILD);
	
	CRect rcWnd, rcTest;

	while (pWnd)
	{
		pWnd->GetWindowRect(rcWnd);

		if (rcTest.IntersectRect(rcGroupbox, rcWnd) && 
			(rcTest != rcGroupbox) &&
			pWnd != this && pWnd != &m_TitleBox)
		{
			if(pWnd->IsKindOf(RUNTIME_CLASS(CCheckableGroupBox)))
			{
				mapChkableGroupWnds.push_back((CCheckableGroupBox*)pWnd);
			}
			pWnd->EnableWindow(nCheck == 1 && m_TitleBox.IsWindowEnabled());
		}
		pWnd = pWnd->GetWindow(GW_HWNDNEXT);
	}
	
	list<CCheckableGroupBox*>::iterator iter;
	for(iter=mapChkableGroupWnds.begin(); iter!=mapChkableGroupWnds.end(); ++iter)
	{
		(*iter)->SetCheck((*iter)->GetCheck());
	}
}

int CCheckableGroupBox::GetCheck() const
{
	if(::IsWindow(m_TitleBox.m_hWnd))
	{
		return m_TitleBox.GetCheck();
	}
	return -1;
}

void CCheckableGroupBox::SetCheck(int nCheck)
{
	if(::IsWindow(m_TitleBox.m_hWnd))
	{
		m_TitleBox.SetCheck(nCheck);
		CheckGroupboxControls();
	}
}

void CCheckableGroupBox::SetGroupID(UINT nGroup)
{
	m_nGroupID = nGroup;
}


UINT CCheckableGroupBox::GetGroupID() const
{
	return m_nGroupID;
}


void CCheckableGroupBox::OnEnable(BOOL bEnable) 
{
	CButton::OnEnable(bEnable);
	//disable all child controls if group is disabled
	m_TitleBox.EnableWindow(bEnable);
	CheckGroupboxControls();
}


void CCheckableGroupBox::OnSetFocus(CWnd* pOldWnd) 
{
	m_TitleBox.SetFocus();
}


LRESULT CCheckableGroupBox::OnGetCheck(WPARAM wp, LPARAM lp)
{
	return GetCheck();
}

LRESULT CCheckableGroupBox::OnSetCheck(WPARAM wp, LPARAM lp)
{
	SetCheck((int)wp);
	return 0;
}