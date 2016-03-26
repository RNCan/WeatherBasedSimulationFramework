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
#include "SelectionDlg.h"
//
//#include "WeatherBasedSimulationUI.h"
//
//
//short CSelectionDlgItemVector::m_maxItemSelected = 9999;
//// CSelectionDlg
//
//IMPLEMENT_DYNAMIC(CSelectionDlg, CDialog)
//CSelectionDlg::CSelectionDlg(CWnd* pParent):
//CDialog(IDD_CMN_SELECTION, pParent)
//{
//}
//
//CSelectionDlg::~CSelectionDlg()
//{
//}
//
//// recreate the list box by copying styles etc, and list items
//// and applying them to a newly created control
//BOOL RecreateListBox(CListBox* pList, LPVOID lpParam=NULL)
//{
//	if (pList == NULL)
//		return FALSE;
//	if (pList->GetSafeHwnd() == NULL)
//		return FALSE;
//
//	CWnd* pParent = pList->GetParent();
//	if (pParent == NULL)
//		return FALSE;
//
//	// get current attributes
//	DWORD dwStyle = pList->GetStyle();
//	DWORD dwStyleEx = pList->GetExStyle();
//	CRect rc;
//	pList->GetWindowRect(&rc);
//	pParent->ScreenToClient(&rc);	// map to client co-ords
//	UINT nID = pList->GetDlgCtrlID();
//	CFont* pFont = pList->GetFont();
//	CWnd* pWndAfter = pList->GetNextWindow(GW_HWNDPREV);
//
//	// create the new list box and copy the old list box items 
//	// into a new listbox along with each item's data, and selection state
//	CListBox listNew;
//	if (! listNew.CreateEx(dwStyleEx, _T("LISTBOX"), _T(""), dwStyle, 
//                                rc, pParent, nID, lpParam))
//	  return FALSE;
//	listNew.SetFont(pFont);
//	int nNumItems = pList->GetCount();
//	BOOL bMultiSel = (dwStyle & LBS_MULTIPLESEL || dwStyle & LBS_EXTENDEDSEL);
//	for (int n = 0; n < nNumItems; n++)
//	{
//		CString sText;
//		pList->GetText(n, sText);
//		int nNewIndex = listNew.AddString(sText);
//		listNew.SetItemData(nNewIndex, pList->GetItemData(n));
//		if (bMultiSel && pList->GetSel(n))
//			listNew.SetSel(nNewIndex);
//	}
//	if (! bMultiSel)
//	{
//		int nCurSel = pList->GetCurSel();
//		if (nCurSel != -1)
//		{
//			CString sSelText;
//			// get the selection in the old list
//			pList->GetText(nCurSel, sSelText);
//			// now find and select it in the new list
//			listNew.SetCurSel(listNew.FindStringExact(-1, sSelText));
//		}
//	}
//	// destroy the existing window, then attach the new one
//	pList->DestroyWindow();
//	HWND hwnd = listNew.Detach();
//	pList->Attach(hwnd);
//
//	// position correctly in z-order
//	pList->SetWindowPos(pWndAfter == NULL ? &CWnd::wndBottom
//                                 : pWndAfter, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
//
//	return TRUE;
//}
// 
//void CSelectionDlg::DoDataExchange(CDataExchange* pDX)
//{
//	CDialog::DoDataExchange(pDX);
//	//{{AFX_DATA_MAP(CMaskPage)
//	DDX_Control(pDX, IDC_CMN_SELECT_USED, m_usedCtrl);
//	DDX_Control(pDX, IDC_CMN_SELECT_UNUSED, m_unusedCtrl);
//	DDX_Control(pDX, IDC_CMN_SELECT_ADD, m_addCtrl);
//	DDX_Control(pDX, IDC_CMN_SELECT_REMOVE, m_removeCtrl);
//	//}}AFX_DATA_MAP
//
//	if( pDX->m_bSaveAndValidate )
//	{
//		GetItemList();
//	}
//	else 
//	{
//		if( m_usedCtrl.ModifyStyle(m_bSort?0:LBS_SORT, m_bSort?LBS_SORT:0) )
//		{
//			RecreateListBox(&m_usedCtrl);
//			m_usedCtrl.ShowScrollBar(SB_BOTH);
////			m_usedCtrl.SubclassDlgItem(IDC_SELECT_USED, this);
//		}
//		
//		if( m_unusedCtrl.ModifyStyle(m_bSort?0:LBS_SORT, m_bSort?LBS_SORT:0) )
//		{
//			RecreateListBox(&m_unusedCtrl);
//			m_unusedCtrl.ShowScrollBar(SB_BOTH);
////			m_unusedCtrl.SubclassDlgItem(IDC_SELECT_UNUSED, this);
//		}
//
//		FillItemList();	
//		UpdateCtrl();
//	}
//}
//
//BEGIN_MESSAGE_MAP(CSelectionDlg, CDialog)
//	ON_BN_CLICKED(IDC_CMN_SELECT_ADD, OnAdd)
//	ON_BN_CLICKED(IDC_CMN_SELECT_REMOVE, OnRemove)
//	ON_LBN_SELCHANGE(IDC_CMN_SELECT_UNUSED, UpdateCtrl)
//	ON_LBN_SELCHANGE(IDC_CMN_SELECT_USED, UpdateCtrl)
//	ON_LBN_DBLCLK(IDC_CMN_SELECT_USED, OnRemove)
//	ON_LBN_DBLCLK(IDC_CMN_SELECT_UNUSED, OnAdd)
//END_MESSAGE_MAP()
//
//
//
//// CSelectionDlg message handlers
//
//void CSelectionDlg::FillItemList()
//{
//	for (size_t i = 0; i<m_list.size(); i++)
//	{
//		if(m_list[i].m_bSelected)
//		{
//			int pos = m_usedCtrl.AddString( m_list[i].m_name );
//			m_usedCtrl.SetItemData(pos, m_list[i].m_index);
//		}
//		else
//		{
//			int pos = m_unusedCtrl.AddString( m_list[i].m_name );
//			m_unusedCtrl.SetItemData(pos, m_list[i].m_index);
//		}
//	}
//}
//
//void CSelectionDlg::GetItemList()
//{
//	m_list.clear();
//	for(int i=0; i<m_usedCtrl.GetCount(); i++)
//	{
//		CSelectionDlgItem item;
//		item.m_bSelected = true;
//		item.m_name = m_usedCtrl.GetText(i);
//		item.m_index = (int)m_usedCtrl.GetItemData(i);
//		m_list.push_back(item);
//	}
//
//	for(int i=0; i<m_unusedCtrl.GetCount(); i++)
//	{
//		CSelectionDlgItem item;
//		item.m_bSelected = false;
//		item.m_name = m_unusedCtrl.GetText(i);
//		item.m_index = (int)m_unusedCtrl.GetItemData(i);
//		m_list.push_back(item);
//	}
//
//
//}
//
//
//void CSelectionDlg::OnAdd() 
//{
//	if( m_addCtrl.IsWindowEnabled() )
//	{
//		int nSelCount = m_unusedCtrl.GetSelCount( );
//		int* pSel = new int[nSelCount];
//		m_unusedCtrl.GetSelItems(nSelCount, pSel);
//
//	    
//		for(int i=nSelCount-1; i>= 0; i--)
//		{
//			MoveString( m_unusedCtrl, m_usedCtrl, pSel[i]); 
//		}
//		
//		delete pSel;
//
//		UpdateCtrl();
//	}
//}
//
//
//
//void CSelectionDlg::OnRemove() 
//{
//    int nSelCount = m_usedCtrl.GetSelCount( );
//    int* pSel = new int[nSelCount];
//    m_usedCtrl.GetSelItems(nSelCount, pSel);
//
//    
//    for(int i=nSelCount-1; i>= 0; i--)
//    {
//        MoveString( m_usedCtrl, m_unusedCtrl, pSel[i]);
//    }
//
//    delete pSel;
//
//	UpdateCtrl();
//}
//
//
//
//void CSelectionDlg::MoveString( CListBoxWithHScroll& lisboxFrom, CListBoxWithHScroll& lisboxTo, int posFrom )
//{
//	CString text;
//	lisboxFrom.GetText( posFrom, text );
//	DWORD_PTR data = lisboxFrom.GetItemData(posFrom);
//
//	lisboxFrom.DeleteString( posFrom);
//	
//	int nCount = lisboxFrom.GetCount();
//	if( nCount > 0)
//	{
//		if(posFrom == nCount )lisboxFrom.SetCurSel(posFrom-1);
//		else lisboxFrom.SetCurSel(posFrom);
//	}
//	else lisboxFrom.SetCurSel(-1);
//
//
//	int pos = lisboxTo.AddString( text );
//	lisboxTo.SetItemData(pos, data);
//	lisboxTo.SetCurSel(pos);
//
//}
//
//void CSelectionDlg::UpdateCtrl()
//{
//	bool bEnable = true;
//
//	m_usedCtrl.EnableWindow(bEnable);
//	m_unusedCtrl.EnableWindow(bEnable); 
//
//	bool enableAdd = (m_unusedCtrl.GetSelCount()+m_usedCtrl.GetCount())<=CSelectionDlgItemVector::m_maxItemSelected&&m_usedCtrl.GetCount()<CSelectionDlgItemVector::m_maxItemSelected;
//	m_addCtrl.EnableWindow(bEnable&&enableAdd); 
//	m_removeCtrl.EnableWindow(bEnable&&m_usedCtrl.GetSelCount()!=0);
//
////	for(int i=0; i<kNbStatic; i++)
////		GetStaticCtrl(i).EnableWindow(bEnable);
//}
//
//BOOL CSelectionDlg::OnInitDialog()
//{
//	CDialog::OnInitDialog();
//
//
//
//	return TRUE;  // return TRUE unless you set the focus to a control
//}
