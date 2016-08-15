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
#include "PropertiesListBox.h"
#include "UI/Common/UtilWin.h"


#include "WeatherBasedSimulationString.h"
#include "WeatherBasedSimulationUI.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace UtilWin;


BEGIN_MESSAGE_MAP(CVSListBoxMS, CVSListBox)
	ON_NOTIFY(LVN_BEGINLABELEDIT, nListId, &CVSListBoxMS::OnBeginLabelEdit)
	ON_NOTIFY(LVN_INSERTITEM, nListId, &CVSListBoxMS::OnUpdateTitle)
	ON_NOTIFY(LVN_DELETEITEM, nListId, &CVSListBoxMS::OnUpdateTitle)
	ON_NOTIFY(LVN_DELETEALLITEMS, nListId, &CVSListBoxMS::OnUpdateTitle)
	  
END_MESSAGE_MAP()

BOOL CVSListBoxMS::SetStandardButtons(UINT uiBtns)
{
	if (GetSafeHwnd() == NULL)
	{
		ASSERT(FALSE);
		return FALSE;
	}
	 
	CString strButton;
	if (uiBtns & AFX_VSLISTBOX_BTN_NEW)
	{
		UINT ID = IDB_CMN_NEW32;
		ENSURE(strButton.LoadString(IDS_AFXBARRES_NEW));
		ENSURE(AddButton(IDB_CMN_NEW32, strButton, VK_INSERT,0,AFX_VSLISTBOX_BTN_NEW_ID));
	}
 
	if (uiBtns & AFX_VSLISTBOX_BTN_DELETE)
	{
		ENSURE(strButton.LoadString(IDS_AFXBARRES_DELETE));
		ENSURE(AddButton(IDB_CMN_DELETE32, strButton, VK_DELETE, 0, AFX_VSLISTBOX_BTN_DELETE_ID));
	}

	if (uiBtns & AFX_VSLISTBOX_BTN_UP)
	{
		ENSURE(strButton.LoadString(IDS_AFXBARRES_MOVEUP));
		ENSURE(AddButton(IDB_CMN_UP32, strButton, VK_UP, FALT, AFX_VSLISTBOX_BTN_UP_ID));
	}

	if (uiBtns & AFX_VSLISTBOX_BTN_DOWN)
	{
		ENSURE(strButton.LoadString(IDS_AFXBARRES_MOVEDN));
		ENSURE(AddButton(IDB_CMN_DOWN32, strButton, VK_DOWN, FALT, AFX_VSLISTBOX_BTN_DOWN_ID));
	}

	m_uiStandardBtns |= uiBtns;

	return TRUE;
}

void CVSListBoxMS::CreateNewItem()
{
	m_bNewItem = TRUE;

	int iLastItem = AddItem(_T(""));
	ASSERT(iLastItem >= 0);

	EditItem(iLastItem);
}

void CVSListBoxMS::OnClickButton(int iButton)
{
	UINT uiBtnID = GetButtonID(iButton);
	
	switch( uiBtnID )
	{
	case AFX_VSLISTBOX_BTN_NEW_ID: UnselectAll();CVSListBoxBase::OnClickButton(iButton); break;
	case AFX_VSLISTBOX_BTN_DELETE_ID: DeleteSelection(); OnAfterRemoveItem(); break;
	case AFX_VSLISTBOX_BTN_UP_ID:	MoveSelection (true); break;
	case AFX_VSLISTBOX_BTN_DOWN_ID: MoveSelection (false); break;
	}
}

int CVSListBoxMS::GetSelectedCount()const{ ENSURE(m_pWndList); return m_pWndList->GetSelectedCount( ); }
void CVSListBoxMS::GetSelection(CArray<int>& selection)
{
	selection.SetSize(m_pWndList->GetSelectedCount( ));
	if (selection.GetSize() >= 0)
	{
		POSITION pos = m_pWndList->GetFirstSelectedItemPosition();
		for ( int i=0; pos!=NULL; i++ )
		{
			selection[i] = m_pWndList->GetNextSelectedItem(pos);
		}
	}
}


int CVSListBoxMS::GetSelItem() const
{
	if (GetSafeHwnd() == NULL || m_pWndList == NULL)
	{
		ASSERT(FALSE);
		return -1;
	}

	ASSERT_VALID(m_pWndList);
	if( GetSelectedCount()==1)
		return m_pWndList->GetNextItem(-1, LVNI_SELECTED);

	return -1;
}

CString CVSListBoxMS::GetWindowText()const
{
	CString txt;
	if( GetSelectedCount()==1)
		txt=GetItemText(GetSelItem());

	return txt;
}

bool CVSListBoxMS::SelectString(const CString& txt)
{
	LVFINDINFO info;
	
	info.flags = LVFI_STRING;
	info.psz = txt;

	int nIndex=m_pWndList->FindItem(&info);
	if( nIndex>=0)
		SelectItem(nIndex);

	return nIndex>=0;
}

void CVSListBoxMS::SelectAll()
{
	if (GetSafeHwnd() == NULL || m_pWndList == NULL)
	{
		ASSERT(FALSE);
		return;
	}

	ASSERT_VALID(m_pWndList);
	for(int i=0; i<m_pWndList->GetItemCount(); i++)
		m_pWndList->SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);

}

void CVSListBoxMS::UnselectAll()
{
	if (GetSafeHwnd() == NULL || m_pWndList == NULL)
	{
		ASSERT(FALSE);
		return;
	}

	ASSERT_VALID(m_pWndList);
	for(int i=0; i<m_pWndList->GetItemCount(); i++)
		m_pWndList->SetItemState(i, 0, LVIS_SELECTED);

}

void CVSListBoxMS::SetSelection(const CArray<int>& selection)
{
	if (GetSafeHwnd() == NULL || m_pWndList == NULL)
	{
		ASSERT(FALSE);
		return;
	}

	for(int i=0; i<m_pWndList->GetItemCount(); i++)
	{
		if( UtilWin::FindInArray(selection, i) >= 0)
			m_pWndList->SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
		else 
			m_pWndList->SetItemState(i, 0, LVIS_SELECTED);
	}
}


void CVSListBoxMS::DeleteSelection() 
{
	CArray<int> selection;
	GetSelection(selection);
    
	if( selection.GetSize() > 0)
    {
		for(INT_PTR i=selection.GetUpperBound(); i>=0; i--)
		{
			if( OnBeforeRemoveItem(selection[i]) )
				RemoveItem( selection[i] );
		}
    }
}

void CVSListBoxMS::MoveSelection(bool bIsUp) 
{
	ASSERT(m_pWndList);

	CArray<int> selection;
	GetSelection(selection);
    
	if( CanMove(selection, bIsUp))
    {
		SetRedraw(FALSE);
		for(int i=0; i<selection.GetSize(); i++)
		{
			if (bIsUp)
			{
				MoveItem( selection[i], selection[i]-1);
				OnAfterMoveItemUp(selection[i]-1);
			}
			else
			{
				INT_PTR ii = selection.GetUpperBound()-i;
				MoveItem( selection[ii], selection[ii]+1);
				OnAfterMoveItemDown(selection[ii]+1);
			}
		}

		SetRedraw();
		m_pWndList->Invalidate();
    }

	
}



void CVSListBoxMS::MoveItem( int iSelItem, int iNewSelItem )
{
	CString strLabel = GetItemText(iSelItem);
	DWORD_PTR dwData = GetItemData(iSelItem);
	BOOL bCheck = GetCheck(iSelItem);

	m_bIsActualDelete = FALSE;
	m_pWndList->DeleteItem(iSelItem);
	m_bIsActualDelete = TRUE;

	
	int iItem = m_pWndList->InsertItem(iNewSelItem, strLabel, I_IMAGECALLBACK);
	m_pWndList->SetItemData(iItem, dwData);
	m_pWndList->SetCheck(iItem, bCheck);
	SelectItem(iNewSelItem);
}

bool CVSListBoxMS::CanMove(const CArray<int>& selection, bool bIsUp)const
{
	bool bCanMove = false;
	if( selection.GetSize() > 0 )
	{
		if( bIsUp )
		{
			if( selection[0] != 0 ) 
				bCanMove = true;
		}
		else 
		{
			if( selection[selection.GetUpperBound()] != m_pWndList->GetItemCount()-1)
				bCanMove = true;	
		}
	}

    return bCanMove;
}

DWORD CVSListBoxMS::GetStyle()const
{
	return WS_VISIBLE | WS_CHILD | LVS_REPORT |LVS_NOCOLUMNHEADER | LVS_EDITLABELS | LVS_SHOWSELALWAYS;
}

DWORD CVSListBoxMS::GetStyleEx()const
{
	return LVS_EX_FULLROWSELECT;
}


CWnd* CVSListBoxMS::OnCreateList()
{
	if (GetSafeHwnd() == NULL ||
		m_pWndList != NULL)
	{
		return FALSE;
	}

	ASSERT(GetStyle() & WS_CHILD);

	CRect rectEmpty;
	rectEmpty.SetRectEmpty();

	DWORD style = GetStyle();
	DWORD styleEx = GetStyleEx();
	 
	m_pWndList = new CListCtrl;
	m_pWndList->CWnd::CreateEx(WS_EX_CLIENTEDGE, _T("SysListView32"), _T(""), style,rectEmpty, this, nListId);

	m_pWndList->SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, styleEx);
	
	m_pWndList->InsertColumn(0, _T(""));
	SetFont( m_pWndList->GetFont() );

	//UpdateTitle();
	return m_pWndList;
}


BOOL CVSListBoxMS::PreTranslateMessage(MSG* pMsg) 
{
	// hook to catch ctrl-a key for "select all"
	// and ctrl-i key for "invert selection

	if ( pMsg->message == WM_CHAR )
	{
		TCHAR chr = static_cast<TCHAR>(pMsg->wParam);
		switch ( chr )
		{
			case 0x01:	// 0x01 is the key code for ctrl-a and also for ctrl-A
			{
				SelectAll();
				break;
			}
			//case 0x09:	// 0x09 is the key code for ctrl-i and ctrl-I
			//{
			//	m_pWndList->InvertSelection();
			//	break;
			//}
		}	
	}

	return CVSListBox::PreTranslateMessage(pMsg);
}

void CVSListBoxMS::OnItemChanged(NMHDR* pNMHDR, LRESULT* pResult)
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	if( pNMListView->uNewState & LVIS_STATEIMAGEMASK )
	{
		int selectedItem = pNMListView->iItem;
		//CArray<int> selection;
		//GetSelection(selection);
		//for(int i=0; i<selection.GetSize(); i++)
		//{
		//	if( selection[i] != selectedItem)
		//		SetCheck( i, !GetCheck(i) );
		//}
	}
	
	CVSListBox::OnItemChanged(pNMHDR, pResult);
	//UpdateTitle();
}



void CVSListBoxMS::OnBeginLabelEdit(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
	LV_ITEM& item = pDispInfo->item;
	
	*pResult = CanEditLabel(item.iItem)?FALSE:TRUE;
}

void CVSListBoxMS::OnUpdateTitle(NMHDR *pNMHDR, LRESULT *pResult)
{
	UpdateTitle();
}

bool CVSListBoxMS::CanEditLabel(int i)const
{
	return true;
}

void CVSListBoxMS::UpdateTitle()
{
	m_bDefaultCaption = false;
	int nbItem= m_pWndList->GetItemCount();

	ASSERT( nbItem>=0);
	m_strCaption=ToCString(nbItem);

	Invalidate();
	UpdateWindow();
}

void CVSListBoxMS::OnAfterRemoveItem()
{
	Invalidate();
	UpdateWindow();
}

//***********************************************************************
//CPListBox

BEGIN_MESSAGE_MAP(CPListBox, CVSListBoxMS)
END_MESSAGE_MAP()



//***********************************************************************
//CCFLPropertyGridCtrl

BEGIN_MESSAGE_MAP(CCFLPropertyGridCtrl, CMFCPropertyGridCtrl)
END_MESSAGE_MAP()

void CCFLPropertyGridCtrl::OnPropertyChanged(CMFCPropertyGridProperty* pProp) const
{
	m_pParentList->OnPropertyChanged(pProp);
}
//***********************************************************************
//CPropertiesListBox

BEGIN_MESSAGE_MAP(CPropertiesListBoxBase, CStatic)
END_MESSAGE_MAP()

BEGIN_MESSAGE_MAP(CPropertiesListBox, CPropertiesListBoxBase)
	 //ON_REGISTERED_MESSAGE(AFX_WM_PROPERTY_CHANGED, OnPropertyChanged)
END_MESSAGE_MAP()


CPropertiesListBox::CPropertiesListBox(int listSize)
{
	m_listSize = listSize;
	m_pWndList = NULL;
	m_pWndProperties = NULL;
	m_lastSel=-1;
}

CPropertiesListBox::~CPropertiesListBox()
{
	if (m_pWndList != NULL)
	{
		ASSERT_VALID(m_pWndList);
		delete m_pWndList;
	}

	if (m_pWndProperties != NULL)
	{
		ASSERT_VALID(m_pWndProperties);
		delete m_pWndProperties;
	}
}
void CPropertiesListBox::PreSubclassWindow()
{
	CStatic::PreSubclassWindow();

	_AFX_THREAD_STATE* pThreadState = AfxGetThreadState();
	if (pThreadState->m_pWndInit == NULL)
	{
		Init();
	}
}

int CPropertiesListBox::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CStatic::OnCreate(lpCreateStruct) == -1)
		return -1;

	Init();
	return 0;
}

void CPropertiesListBox::Init()
{
	ModifyStyle(0, SS_USERITEM);
	ModifyStyleEx(0, WS_EX_CONTROLPARENT);

	if (OnCreateList() == NULL)
	{
		TRACE0("CPropertiesListBox::Init(): Can not create list control\n");
		return;
	}

	if (OnCreateProperties() == NULL)
	{
		TRACE0("CPropertiesListBox::Init(): Can not create property control\n");
		return;
	}

	//AdjustLayout();
}

CWnd* CPropertiesListBox::OnCreateList()
{
	if (GetSafeHwnd() == NULL ||
		m_pWndList != NULL)
	{
		return FALSE;
	}

	ASSERT(GetStyle() & WS_CHILD);

	CRect rect;
	GetClientRect(&rect);
	if( m_listSize >= 0)
		rect.right = m_listSize;
	else 
		rect.right = rect.Width()/2;

	rect.DeflateRect(4,4,2,4);
	
	
	

	m_pWndList = new CPListBox(this);
	//if( !m_pWndList->CreateEx(WS_EX_CLIENTEDGE,_T("STATIC"),_T(""),WS_CHILD | WS_VISIBLE | WS_TABSTOP, rect, this, 1000) )
	if( !m_pWndList->Create(_T(""),WS_CHILD | WS_VISIBLE | WS_TABSTOP, rect, this, 1000) )
	{
		return FALSE;
	}

	m_pWndList->SetStandardButtons();

	return m_pWndList;
}

CWnd* CPropertiesListBox::OnCreateProperties()
{
	if (GetSafeHwnd() == NULL ||
		m_pWndProperties != NULL)
	{
		return FALSE;
	}

	ASSERT(GetStyle() & WS_CHILD);


	CRect rect;
	GetClientRect(&rect);
	if( m_listSize >= 0)
		rect.left = m_listSize;
	else 
		rect.left = rect.Width()/2;

	rect.DeflateRect(2,4,4,4);
	
	CStringArrayEx propertyHeader(GetCString(IDS_STR_PROPERTY_HEADER) );

	m_pWndProperties = new CCFLPropertyGridCtrl(this);
	//if( !m_pWndProperties->CreateEx(WS_EX_CLIENTEDGE,afxGlobalData.RegisterWindowClass(_T("Afx:PropList")), _T(""),WS_CHILD | WS_VISIBLE | WS_TABSTOP, rect, this, 1001) )
	if( !m_pWndProperties->Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER , rect, this, 1001) )
		return FALSE;

	m_pWndProperties->EnableHeaderCtrl(true, propertyHeader[0], propertyHeader[1]);
	m_pWndProperties->EnableDescriptionArea(true);
	m_pWndProperties->SetVSDotNetLook(true);
	m_pWndProperties->MarkModifiedProperties(false);
	m_pWndProperties->SetAlphabeticMode(false);
	m_pWndProperties->SetShowDragContext(true);
	m_pWndProperties->EnableWindow(false);
	m_pWndProperties->AlwaysShowUserToolTip();

	return m_pWndProperties;
}



void CPropertiesListBox::OnSelectionChanged()
{
	TRACE0("OnSelectionChanged\n");

	////in the case of the creation of the first item, we receave
	////a OnSelectionChanged() but we would not process it.
	//if( m_pWndList->GetCount() > m_data.GetSize() )
	//{
	//	TRACE0("Contrl not init\n");
	//	return;
	//}
	
	int newSel = m_pWndList->GetSelItem();

	if( newSel != m_lastSel )
	{
		if( m_lastSel >= 0 && m_lastSel < m_pWndList->GetCount() )
			OnGetDataFromProperties(m_lastSel);
		
		//if( newSel != -1  && newSel<m_data.GetSize())
		OnSetDataToProperties(newSel);

		m_lastSel = newSel;
	}
}


BOOL CPropertiesListBox::OnBeforeRemoveItem(int iItem)
{

	DWORD_PTR pos = m_pWndList->GetItemData(iItem);
	
	for(int i=0; i<m_pWndList->GetCount(); i++)
	{
		if( m_pWndList->GetItemData(i) > pos )
			m_pWndList->SetItemData(i, m_pWndList->GetItemData(i)-1);
	}

	//reset the last selection because, this item is removed
	m_lastSel=-1;
	if(m_pWndList->GetCount()==1)
		OnSetDataToProperties(-1);


	TRACE0("OnBeforeRemoveItem\n");
	return TRUE;
}

void CPropertiesListBox::OnAfterAddItem(int iItem)
{
	ASSERT( iItem == m_pWndList->GetCount()-1 );

	m_pWndList->SetItemData(iItem, iItem);
	TRACE0("OnAfterAddItem\n");

	//Update the selection because we don't receve it from parent
	OnSelectionChanged();
}

void CPropertiesListBox::OnAfterRenameItem(int iItem)
{
	OnSetDataToProperties(iItem);
	TRACE0("OnAfterRenameItem\n");
}

//iItem is the new item position
void CPropertiesListBox::OnAfterMoveItemUp(int iItem)
{
	TRACE0("OnAfterMoveItemUp\n");
}

//iItem is the new item position
void CPropertiesListBox::OnAfterMoveItemDown(int iItem)
{
	TRACE0("OnAfterMoveItemDown\n");
}

BOOL CPropertiesListBox::PreTranslateMessage(MSG* pMsg)
{

	if( m_pWndList->GetSafeHwnd() != NULL &&
		m_pWndList->PreTranslateMessage(pMsg) )
		return TRUE;

	if( m_pWndProperties->GetSafeHwnd() != NULL  &&
		m_pWndProperties->PreTranslateMessage(pMsg) )
		return TRUE;

	return CPropertiesListBoxBase::PreTranslateMessage(pMsg);
}

void CPropertiesListBox::OnPropertyChanged(CMFCPropertyGridProperty* pProp) const
{    
//    CMFCPropertyGridProperty* pProp = (CMFCPropertyGridProperty*) lParam;
//	int iItem = m_pWndList->GetSelItem();
	//ASSERT( iItem>=0 &&  iItem<m_pWndList->GetCount() );

	//const_cast<CPropertiesListBox*>(this)->OnPropertiesChange(iItem, (UINT)0, pProp);
}


