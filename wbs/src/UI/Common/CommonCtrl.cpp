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
#include "Basic/Statistic.h"
#include "Basic/GeoBasic.h"
#include "UI/Common/CommonCtrl.h"
#include "WeatherBasedSimulationString.h"
#include "WeatherBasedSimulationUI.h"

using namespace UtilWin;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



//**************************************************************************
//CCFLComboBox
CString CCFLComboBox::GetWindowText()const
{
	CString tmp;
	UINT style = GetStyle()&0x0003L;
	if( style == CBS_DROPDOWNLIST)
	{
		int pos = GetCurSel();
		if( pos != CB_ERR)
			GetLBText(pos, tmp);
	}
	else GetWindowText(tmp);

	return (LPCTSTR)tmp;
}

int CCFLComboBox::SelectStringExact(int nStartAfter, LPCTSTR lpszString, int defaultSel)
{
	int pos = CB_ERR;
	if( GetCount() > 0 )
	{
		ASSERT( defaultSel >= CB_ERR && defaultSel < GetCount());
		pos = FindStringExact(nStartAfter, lpszString);
		if( pos == CB_ERR)
			pos = defaultSel;
	
		SetCurSel(pos);
	}
	return pos;
}

int CCFLComboBox::GetItemDataIndex(int itemData)const
{
	int pos = CB_ERR;
	for(int i=0; i<GetCount(); i++)
	{
		if( GetItemData(i) == itemData)
		{
			pos = i;
			break;
		}
	}

	return pos;
}

int CCFLComboBox::GetCurItemData()const
{
	int itemData = CB_ERR;

	int pos = GetCurSel();
	if( pos != CB_ERR )
		itemData = (int)GetItemData(pos);
	
	return itemData;
}

int CCFLComboBox::SelectFromItemData(int itemData, int defaultSel)
{
	int pos = CB_ERR;
	if( GetCount() > 0 )
	{
		pos = GetItemDataIndex(itemData);
		if( pos == CB_ERR)
			pos = defaultSel;
	
		SetCurSel(pos);			
	}

	return pos;
}
void CCFLComboBox::FillList(const WBSF::StringVector& list, std::string selection)
{
	if (selection.empty())
		selection = GetString();
	
	ResetContent();

	for (size_t i = 0; i<list.size(); i++)
		AddString(list[i].c_str());

	SelectStringExact(0, selection);
}

//**************************************************************************
CDefaultComboBox::CDefaultComboBox(CString str)
{
	if (str == WBSF::STRDEFAULT)
		str = UtilWin::GetCString(IDS_STR_DEFAULT);

	m_defaultStr = str;
}

void CDefaultComboBox::FillList(const WBSF::StringVector& list, std::string selection)
{
	if (selection.empty())
		selection = GetString();

	ResetContent();

	for (size_t i = 0; i<list.size(); i++)
		AddString(list[i].c_str());

	InsertString(0, m_defaultStr);
	SelectStringExact(0, selection);

	ASSERT(GetCount()>0);
}

//**************************************************************************
//CCFLListBox
CString CCFLListBox::GetText(int i)const
{
	CString tmp;
	CListBox::GetText(i, tmp);
	return tmp;
}

int CCFLListBox::GetItemDataIndex(int itemData)const
{
	int pos = CB_ERR;
	for(int i=0; i<GetCount(); i++)
	{
		if( GetItemData(i) == itemData)
		{
			pos = i;
			break;
		}
	}

	return pos;
}
//**************************************************************************
//CIntEdit


CIntEdit::CIntEdit(short base) :
m_base(base)
{}

CIntEdit::~CIntEdit()
{}


BEGIN_MESSAGE_MAP(CIntEdit, CEdit)
	ON_WM_CHAR()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CIntEdit message handlers

void CIntEdit::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{

	if (((nChar >= '0') && (nChar <= '9')) ||
		(nChar == VK_BACK) || (nChar == VK_DELETE) || (nChar == '-'))
	{
		CEdit::OnChar(nChar, nRepCnt, nFlags);
	}
}

void CIntEdit::SetInt(__int64 val)
{
	TCHAR buffer[100] = { 0 };

	
	_i64tot_s(val, buffer, 100, m_base);
	SetWindowText(buffer);
}

__int64 CIntEdit::GetInt()const
{
	TCHAR* end=NULL;
	return _wcstoi64((LPCWSTR)GetWindowTextW(), &end, m_base);
}

//**************************************************************************
//CFloatEdit

BEGIN_MESSAGE_MAP(CFloatEdit, CEdit)
	ON_WM_CHAR()
END_MESSAGE_MAP()


void CFloatEdit::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{

	if (((nChar >= '0') && (nChar <= '9')) || (nChar == '.') || (nChar == 'e') || (nChar == 'E') ||
		(nChar == VK_BACK) || (nChar == VK_DELETE) || (nChar == '-'))
	{
		CEdit::OnChar(nChar, nRepCnt, nFlags);
	}

}

void CFloatEdit::SetFloat(const double fVal, int pres)
{
	SetWindowText(UtilWin::ToCString(fVal, pres));
}

double CFloatEdit::GetFloat()const
{
	return _tstof((LPCTSTR)GetWindowText());
}

/////////////////////////////////////////////////////////////////////////////
// CReadOnlyEdit

BEGIN_MESSAGE_MAP(CReadOnlyEdit, CEdit)
	ON_WM_CONTEXTMENU()
	ON_WM_WINDOWPOSCHANGING()
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateToolBar)
	ON_UPDATE_COMMAND_UI(ID_EDIT_SELECT_ALL, OnUpdateToolBar)
	ON_UPDATE_COMMAND_UI(ID_EDIT_CLEAR, OnUpdateToolBar)
	ON_COMMAND_EX(ID_EDIT_COPY, OnToolBarCommand)
	ON_COMMAND_EX(ID_EDIT_SELECT_ALL, OnToolBarCommand)
	ON_COMMAND_EX(ID_EDIT_CLEAR, OnToolBarCommand)

END_MESSAGE_MAP()

CReadOnlyEdit::CReadOnlyEdit()
{
}

CReadOnlyEdit::~CReadOnlyEdit()
{
}

/////////////////////////////////////////////////////////////////////////////
// Gestionnaires de messages de COutputList

//void CReadOnlyEdit::OnContextMenu(CWnd* pWnd, CPoint point)
//{
//	CEdit::OnContextMenu(pWnd, point);
//	/*CMenu menu;
//	menu.LoadMenu(IDR_OUTPUT_POPUP);
//
//	CMenu* pSumMenu = menu.GetSubMenu(0);
//
//	if (AfxGetMainWnd()->IsKindOf(RUNTIME_CLASS(CMDIFrameWndEx)))
//	{
//		CMFCPopupMenu* pPopupMenu = new CMFCPopupMenu;
//
//		if (!pPopupMenu->Create(this, point.x, point.y, (HMENU)pSumMenu->m_hMenu, FALSE, TRUE))
//			return;
//
//		((CMDIFrameWndEx*)AfxGetMainWnd())->OnShowPopupMenu(pPopupMenu);
//		UpdateDialogControls(this, FALSE);
//	}
//
//	SetFocus();*/
//}

//#define ME_SELECTALL    WM_USER + 0x7000

void CReadOnlyEdit::OnContextMenu(CWnd* pWnd, CPoint point)
{
	SetFocus();

	CContextMenuManager* pMM = ((CWinAppEx*)AfxGetApp())->GetContextMenuManager();
	HMENU hMenu = pMM->GetMenuByName(_T("Edit1"));
	if (hMenu != NULL)
	{
		//pMM->AddMenu(_T("Edit1"), IDR_MENU_EDIT);


		CMenu* pMenu = CMenu::FromHandle(hMenu);
		ASSERT(pMenu);
		//menu.LoadMenu(IDR_MENU_EDIT);

		CMenu* pSumMenu = pMenu->GetSubMenu(0);
		ASSERT(pSumMenu);
		//pSumMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON, point.x, point.y, this);

		pMM->TrackPopupMenu(*pSumMenu, point.x, point.y, this);
		//((CMDIFrameWndEx*)AfxGetMainWnd())->OnShowPopupMenu(pSumMenu);
	}

	//
	//	if (AfxGetMainWnd()->IsKindOf(RUNTIME_CLASS(CMDIFrameWndEx)))
	//	{
	//		CMFCPopupMenu* pPopupMenu = new CMFCPopupMenu;
	//
	//		if (!pPopupMenu->Create(this, point.x, point.y, (HMENU)pSumMenu->m_hMenu, FALSE, TRUE))
	//			return;
	//
	//		((CMDIFrameWndEx*)AfxGetMainWnd())->OnShowPopupMenu(pPopupMenu);
	//		UpdateDialogControls(this, FALSE);
	//	}
	//
	//	SetFocus();*/

}


BOOL CReadOnlyEdit::PreTranslateMessage(MSG* pMsg)
{
	//read only editor
	if (pMsg->message == WM_KEYDOWN)
	{
		bool bCTRL = GetKeyState(VK_CONTROL) < 0;
		if (bCTRL && (pMsg->wParam == 'C' || pMsg->wParam == 'A'))
			return CEdit::PreTranslateMessage(pMsg);

		if (pMsg->wParam >= VK_PRIOR && pMsg->wParam <= VK_DOWN)
			return CEdit::PreTranslateMessage(pMsg);

		return TRUE;
	}

	return CEdit::PreTranslateMessage(pMsg);
}

void CReadOnlyEdit::OnUpdateToolBar(CCmdUI *pCmdUI)
{
	DWORD sel = GetSel();
	int len = GetWindowTextLength();

	switch (pCmdUI->m_nID)
	{
	case ID_EDIT_COPY:   pCmdUI->Enable(LOWORD(sel) != HIWORD(sel)); break;
	case ID_EDIT_SELECT_ALL:   pCmdUI->Enable(len != 0 && !(LOWORD(sel) == 0 && HIWORD(sel) == len)); break;
	case ID_EDIT_CLEAR:	pCmdUI->Enable(len>0); break;
	}
}


BOOL CReadOnlyEdit::OnToolBarCommand(UINT ID)
{
	
	switch (ID)
	{
	case ID_EDIT_COPY:			SendMessage(LOWORD(WM_COPY)); break;
	case ID_EDIT_SELECT_ALL:	SendMessage(EM_SETSEL, 0, -1); break;
	case ID_EDIT_CLEAR:			SetWindowText(_T("")); break;
	}
	
	return TRUE;
}
//**************************************************************************
//CStatisticComboBox
BEGIN_MESSAGE_MAP(CStatisticComboBox, CComboBox)
	ON_WM_CREATE()
END_MESSAGE_MAP()

void CStatisticComboBox::PreSubclassWindow()
{
	CComboBox::PreSubclassWindow();

	_AFX_THREAD_STATE* pThreadState = AfxGetThreadState();
	if (pThreadState->m_pWndInit == NULL)
	{
		Init();
	}
}

int CStatisticComboBox::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CComboBox::OnCreate(lpCreateStruct) == -1)
		return -1;

	Init();
	return 0;
}

void CStatisticComboBox::ReloadContent()
{
	int sel = GetCurSel();
	Init();
	SetCurSel(sel);
}

void CStatisticComboBox::Init()
{
	m_font.CreateStockObject(DEFAULT_GUI_FONT);
	SetFont(&m_font, FALSE);

	ResetContent();

	for(int i=0; i<WBSF::NB_STAT_TYPE; i++)
		AddString(GetStatisticTitle(i));
	
}


CString CStatisticComboBox::GetStatisticTitle(int i)
{
	return CString(WBSF::CStatistic::GetTitle(i));
}

//**************************************************************************
//CTMTypeComboBox
BEGIN_MESSAGE_MAP(CTMTypeComboBox, CComboBox)
	ON_WM_CREATE()
END_MESSAGE_MAP()

CTMTypeComboBox::CTMTypeComboBox()
{
	m_nbTypeAvailable = WBSF::CTM::NB_REFERENCE;
}
void CTMTypeComboBox::PreSubclassWindow()
{
	CComboBox::PreSubclassWindow();

	_AFX_THREAD_STATE* pThreadState = AfxGetThreadState();
	if (pThreadState->m_pWndInit == NULL)
	{
		Init();
	}
}

int CTMTypeComboBox::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CComboBox::OnCreate(lpCreateStruct) == -1)
		return -1;

	Init();
	return 0;
}

void CTMTypeComboBox::ReloadContent()
{
	int sel = GetCurSel();
	WBSF::CTM::ReloadString();
	Init();
	SetCurSel(sel);
}

void CTMTypeComboBox::Init()
{
	m_font.CreateStockObject(DEFAULT_GUI_FONT);
	SetFont(&m_font, FALSE);

	ResetContent();
	
	for (size_t i = 0; i<m_nbTypeAvailable; i++)
	{
		AddString(GetTitle(short(i)));
	}
}


CString CTMTypeComboBox::GetTitle(short i)
{
	return CString(WBSF::CTM::GetTypeTitle(i));
}

//**************************************************************************
//CTMTypeComboBox
BEGIN_MESSAGE_MAP(CTMModeComboBox, CComboBox)
	ON_WM_CREATE()
END_MESSAGE_MAP()

void CTMModeComboBox::PreSubclassWindow()
{
	CComboBox::PreSubclassWindow();

	_AFX_THREAD_STATE* pThreadState = AfxGetThreadState();
	if (pThreadState->m_pWndInit == NULL)
	{
		Init();
	}
}

int CTMModeComboBox::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CComboBox::OnCreate(lpCreateStruct) == -1)
		return -1;

	Init();
	return 0;
}

void CTMModeComboBox::ReloadContent()
{
	int sel = GetCurSel();
	WBSF::CTM::ReloadString();
	Init();
	SetCurSel(sel);
}

void CTMModeComboBox::Init()
{
	m_font.CreateStockObject(DEFAULT_GUI_FONT);
	SetFont(&m_font, FALSE);

	ResetContent();

	for (int i = 0; i<WBSF::CTM::NB_MODE; i++)
	{
		AddString(GetTitle(i));
	}
}


CString CTMModeComboBox::GetTitle(short i)
{
	return CString(WBSF::CTM::GetModeTitle(i));
}

//**************************************************************************
//CTransparentCheckBox

//**************************************************************************
//CTransparentCheckBox
BEGIN_MESSAGE_MAP(CTransparentCheckBox, CButton)
	ON_WM_ERASEBKGND()
	ON_WM_CTLCOLOR_REFLECT()
END_MESSAGE_MAP()

BOOL CTransparentCheckBox::OnEraseBkgnd(CDC* /*pDC*/)
{
	// Prevent from default background erasing.
	return FALSE;
}

BOOL CTransparentCheckBox::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.dwExStyle |= WS_EX_TRANSPARENT;
	return CButton::PreCreateWindow(cs);
}

HBRUSH CTransparentCheckBox::CtlColor(CDC* pDC, UINT /*nCtlColor*/)
{
	pDC->SetBkMode(TRANSPARENT);
	return reinterpret_cast<HBRUSH>(GetStockObject(NULL_BRUSH));
}

void CTransparentCheckBox::PreSubclassWindow()
{
	CButton::PreSubclassWindow();

	const LONG_PTR exStyle = GetWindowLongPtr(m_hWnd, GWL_EXSTYLE);
	SetWindowLongPtr(m_hWnd, GWL_EXSTYLE, exStyle | WS_EX_TRANSPARENT);
}
//*************************************************************************
BEGIN_MESSAGE_MAP(CTransparentStatic, CStatic)
	ON_WM_ERASEBKGND()
	ON_WM_CTLCOLOR_REFLECT()
END_MESSAGE_MAP()


BOOL CTransparentStatic::OnEraseBkgnd(CDC* /*pDC*/)
{
	// Prevent from default background erasing.
	return FALSE;
}

BOOL CTransparentStatic::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.dwExStyle |= WS_EX_TRANSPARENT;
	return CStatic::PreCreateWindow(cs);
}

HBRUSH CTransparentStatic::CtlColor(CDC* pDC, UINT /*nCtlColor*/)
{
	pDC->SetBkMode(TRANSPARENT);
	return reinterpret_cast<HBRUSH>(GetStockObject(NULL_BRUSH));
}

void CTransparentStatic::PreSubclassWindow()
{
	CStatic::PreSubclassWindow();

	const LONG_PTR exStyle = GetWindowLongPtr(m_hWnd, GWL_EXSTYLE);
	SetWindowLongPtr(m_hWnd, GWL_EXSTYLE, exStyle | WS_EX_TRANSPARENT);
}

//************************************************************
BEGIN_MESSAGE_MAP(CTransparentEdit, CCFLEdit)
	ON_WM_ERASEBKGND()
	ON_WM_CTLCOLOR_REFLECT()
END_MESSAGE_MAP()


BOOL CTransparentEdit::OnEraseBkgnd(CDC* /*pDC*/)
{
	// Prevent from default background erasing.
	return FALSE;
}

BOOL CTransparentEdit::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.dwExStyle |= WS_EX_TRANSPARENT;
	return CCFLEdit::PreCreateWindow(cs);
}

HBRUSH CreateCtrlBrush( CWnd* a_pDlg, CWnd* a_pCtrl, CBrush& a_brush)
{
	CRect ctrlRect;
	CRect dlgRect;

	CDC* pTempDC = a_pDlg->GetDC();

	a_pCtrl->GetWindowRect(&ctrlRect);
	a_pDlg->GetWindowRect(&dlgRect);

	// Map control rect to parent background
	int x = ctrlRect.left - dlgRect.left;
	int y = ctrlRect.top - dlgRect.top;
	int w = ctrlRect.Width();
	int h = ctrlRect.Height();

	// Bitmap to hold background for control
	CBitmap bkgndBitmap;
	bkgndBitmap.CreateCompatibleBitmap(pTempDC, w, h);

	// And a DC used to fill it in.
	CDC dcBitmap;
	dcBitmap.CreateCompatibleDC(pTempDC);
	dcBitmap.SelectObject(bkgndBitmap);

	// Copy background into bitmap
	dcBitmap.BitBlt(0, 0, w, h, pTempDC, x, y, SRCCOPY);

	if (a_brush.m_hObject)
		a_brush.DeleteObject();

	a_brush.CreatePatternBrush(&bkgndBitmap);

	a_pDlg->ReleaseDC(pTempDC);

	return (HBRUSH)a_brush;
}

HBRUSH CTransparentEdit::CtlColor(CDC* pDC, UINT /*nCtlColor*/)
{
	if ((HBRUSH)a_brush == NULL)
		CreateCtrlBrush(GetParent(), this, a_brush);

	return (HBRUSH)a_brush;

//	pDC->SetBkMode(TRANSPARENT);
	//return reinterpret_cast<HBRUSH>(GetStockObject(NULL_BRUSH));
}

void CTransparentEdit::PreSubclassWindow()
{
	CCFLEdit::PreSubclassWindow();

	const LONG_PTR exStyle = GetWindowLongPtr(m_hWnd, GWL_EXSTYLE);
	SetWindowLongPtr(m_hWnd, GWL_EXSTYLE, exStyle | WS_EX_TRANSPARENT);
}
//**************************************************************************
// Reusable function for all of your dialogs 
/*

// CTransparentCheckBox message handlers 
HBRUSH CTransparentCheckBox::CtlColor(CDC* pDC, UINT nCtlColor) 
{ 
	//static CBrush m_brush;

	//if ( nCtlColor == CTLCOLOR_STATIC && (HBRUSH)m_brush == NULL)
	//{     
		//return CreateCtrlBrush( GetParent(), this, m_brush );
	//}
	
   //if((HBRUSH)brush == NULL)
     //   brush.CreateSolidBrush(RGB(255,0,0));
//   pDC->SetBkMode(TRANSPARENT); 
  // ::DrawThemeParentBackground(this, hdc, GetClientRect());

   //return (HBRUSH)GetStockObject(NULL_BRUSH);
	return (HBRUSH)GetStockObject(WHITE_BRUSH); 
} 

LRESULT CTransparentCheckBox::OnControlColorStatic(UINT uMsg, UINT wParam, LONG lParam)
{
	SetTextColor(hdc, RGB(0xFF, 0xFF, 0xFF));

	CRect crect(s_crectRememberMeCheckBox);
	crect.MoveToXY(0, 0);

	TCHAR rgch[4];
	lstrcpy(rgch, _T("foo"));
	::DrawText(hdc, rgch, -1, crect, DT_CENTER | DT_SINGLELINE);

	::DrawThemeParentBackground(this, hdc, crect);
	return (LRESULT)GetStockObject(HOLLOW_BRUSH);
}
*/

//**************************************************************************
//CAutoEnableStatic
CAutoEnableStatic::CAutoEnableStatic(bool bShowButton, int buttonType)
{
	
	m_bShowButton = bShowButton;
	m_buttonType = buttonType;
}

CAutoEnableStatic::~CAutoEnableStatic()
{
}


BEGIN_MESSAGE_MAP(CAutoEnableStatic, CStatic)
	ON_WM_CREATE()
	ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAutoEnableStatic message handlers


void CAutoEnableStatic::Init()
{
		// Get the static size
	GetWindowRect(&m_rcStatic);
	ScreenToClient(&m_rcStatic);

	//EnumChildWindows(GetParent()->GetSafeHwnd(),EnumChildProc, LPARAM(this));


	// Grab the caption off the static
	// We are going to put it on the button so it is aligned nicely
	
	GetWindowText(m_caption);
	SetWindowText(_T(""));

	// Now figure out how long the string is
	CClientDC dc(this);
	dc.SelectObject( GetFont() );
	CSize size = dc.GetTextExtent(m_caption);

	CRect rect(m_rcStatic.left + 10, m_rcStatic.top , m_rcStatic.left + 10 + size.cx + 20, m_rcStatic.top + size.cy);
	//ClientToScreen(rect);
	//GetParent()->ScreenToClient(rect);

	//dc.LPtoDP(&size);

	// Add a bit for the button itself
	//if (size.cy < 20) size.cy = 20;
	//size.cx += 30;

	static int ID_STATICCHECKBOX = 999;

	
	//CWnd* pParent=this;
	//if( GetParent() )
		//pParent=GetParent();
	//m_checkbox.CreateEx(WS_EX_TRANSPARENT, _T("BUTTON"), _T("&Transparency?"), WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | BS_CHECKBOX, CRect(m_rcStatic.left + 10, m_rcStatic.top , m_rcStatic.left + size.cx, m_rcStatic.top + size.cy),
		//this, ID_STATICCHECKBOX--);
	
	if(!m_checkbox.Create(m_caption, WS_CHILD | WS_VISIBLE  | WS_TABSTOP | BS_CHECKBOX, 
		rect,
		this, ID_STATICCHECKBOX--))
		AfxMessageBox(_T("Failed to create CheckedGroupBox"));

	
	//m_checkbox.ModifyStyleEx(0, WS_EX_TRANSPARENT);

	//m_caption
	//if(!m_radio.Create(m_caption, WS_CHILD | WS_VISIBLE  | WS_TABSTOP | BS_RADIOBUTTON, 
	//	//CRect(m_rcStatic.left + 10, m_rcStatic.top, 20, 20 ),
	//	CRect(m_rcStatic.left + 10, m_rcStatic.top , m_rcStatic.left + 10 + size.cx, m_rcStatic.top + size.cy),
	//	this, ID_STATICCHECKBOX--))
	//	AfxMessageBox("Failed to create CheckedGroupBox");
	
	//m_radio.ModifyStyleEx(0, WS_EX_LAYERED|WS_EX_TRANSPARENT);


	//SetWindowText(m_bShowButton?"":m_caption);
	m_checkbox.ShowWindow(m_bShowButton&&m_buttonType==CHECKBOX?SW_SHOW:SW_HIDE);
	//m_radio.ShowWindow(m_bShowButton&&m_buttonType==RADIO_BUTTON?SW_SHOW:SW_HIDE);
	//if( !m_bShowButton )
	//{
	//	SetWindowText(m_caption);
	//	m_checkbox.ShowWindow(SW_HIDE);
	//	m_radio.ShowWindow(SW_HIDE);
	//}

	// All of the above would be useless if we did not make sure both buttons had the same font in them...
	m_checkbox.SetFont(GetFont());
	//m_radio.SetFont(GetFont());
	
	// Assume that the dialog starts active, a SetCheck function is provided where this is not the case
	m_checkbox.SetCheck(true);
	//m_radio.SetCheck(true);
	// Build a list of dialog item ID's

//	CClientDC dc(this);
	
	//BOOL rep = (BOOL)GetParent()->SendMessage(WM_ERASEBKGND, (WPARAM)dc.GetSafeHdc(), 0);
	//HBRUSH hBrush = (HBRUSH)GetParent()->SendMessage(WM_CTLCOLOR, (WPARAM)(GetParent()->GetDC()->GetSafeHdc()), MAKELONG(GetSafeHwnd(), 0));
	//HBRUSH hBrush = (HBRUSH)GetParent()->SendMessage(WM_CTLCOLORDLG, (WPARAM)NULL,(LPARAM)MAKELONG(GetSafeHwnd(), 0));
	//m_br.CreateSolidBrush(dc.GetBkColor());
}

BOOL CAutoEnableStatic::EnableWindow(BOOL bEnable)
{
	BOOL bRep = CStatic::EnableWindow(bEnable);

	m_checkbox.EnableWindow(bEnable);
	//m_radio.EnableWindow(bEnable);
	UpdateChild(bEnable);

	return bRep;
}


BOOL CALLBACK CAutoEnableStatic::EnumChildProc(HWND hwndChild, LPARAM lParam) 
{
	CAutoEnableStatic* pMe = (CAutoEnableStatic*)lParam;
	ASSERT(pMe);
	CWnd* pParent = pMe->GetParent();
	ASSERT(pParent);

	CWnd* pChild = CWnd::FromHandle(hwndChild);
	ASSERT(pChild);
	CRect rect;
	pChild->GetWindowRect(&rect);
	pMe->ScreenToClient(&rect);
	
	if(pMe->m_rcStatic.PtInRect(rect.TopLeft()))
	{
		int ID = pChild->GetDlgCtrlID();
		if(ID==-1)
		{
			static int NEW_ID = 900;
			pChild->SetDlgCtrlID(NEW_ID);
			ID = NEW_ID++;
		}
		ASSERT(ID>0);
		
		pMe->m_IDList.Add(ID);
	}


	return TRUE;
}


BOOL CAutoEnableStatic::OnCommand(WPARAM wParam, LPARAM lParam) 
{
	if (wParam == m_checkbox.GetDlgCtrlID() )
	{
		BOOL check = m_checkbox.GetCheck();
		m_checkbox.SetCheck(!check);

		UpdateChild(!check);

		
		GetParent()->SendMessage(WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(), BN_CLICKED), (LPARAM)m_hWnd);
	
		return TRUE;
	}

	//if (wParam == m_radio.GetDlgCtrlID() )
	//{
	//	BOOL check = m_radio.GetCheck();
	//	m_radio.SetCheck(!check);

	//	UpdateChild(!check);
	//
	//	return TRUE;
	//}

	return CStatic::OnCommand(wParam, lParam);
}

int CAutoEnableStatic::AddItem(int ID)
{
	m_ItemID.Add(ID);
	return (int)m_ItemID.GetSize();
}

void CAutoEnableStatic::ClearItems()
{
	m_ItemID.RemoveAll();
}

void CAutoEnableStatic::SetCheck(BOOL check)
{
	if( m_buttonType == CHECKBOX)
		m_checkbox.SetCheck(check);
	//else m_radio.SetCheck(check);
	
	UpdateChild(check);
}

BOOL CAutoEnableStatic::GetCheck()
{
	BOOL bCheck = FALSE;
	if( m_buttonType == CHECKBOX)
		bCheck = m_checkbox.GetCheck();
	//else bCheck = m_radio.GetCheck();

	return bCheck;
}

void CAutoEnableStatic::UpdateChild(BOOL check)
{
	if (m_ItemID.GetSize() > 0)
	{
		for (int i = 0; i < m_ItemID.GetSize(); i++)
			GetParent()->GetDlgItem(m_ItemID[i])->EnableWindow(check);
	}
	else
	{
		EnumChildWindows(GetParent()->GetSafeHwnd(),EnableParentChild, LPARAM(this));
	}
}

BOOL CALLBACK CAutoEnableStatic::EnableChild(HWND hwndChild, LPARAM lParam) 
{
	CAutoEnableStatic* pMe = (CAutoEnableStatic*)lParam;

	bool bEnable = pMe->IsWindowEnabled() && (!pMe->m_bShowButton || pMe->GetCheck() );
	CWnd* pChild = CWnd::FromHandle(hwndChild);
	pChild->EnableWindow( bEnable );

	return TRUE;
}

BOOL CALLBACK CAutoEnableStatic::EnableParentChild(HWND hwndChild, LPARAM lParam) 
{
	CAutoEnableStatic* pMe = (CAutoEnableStatic*)lParam;
	ASSERT(pMe);
	CWnd* pParent = pMe->GetParent();
	ASSERT(pParent);

	CWnd* pChild = CWnd::FromHandle(hwndChild);
	ASSERT(pChild);
	CRect rect;
	pChild->GetWindowRect(&rect);
	pMe->ScreenToClient(&rect);
	
	if( pMe->m_rcStatic.PtInRect(rect.TopLeft()) && 
		hwndChild != pMe->m_checkbox.GetSafeHwnd() && 
		//hwndChild != pMe->m_radio.GetSafeHwnd() && 
		hwndChild != pMe->GetSafeHwnd() )
	{
		bool bEnable = pMe->IsWindowEnabled() && (!pMe->m_bShowButton || pMe->GetCheck() );
		pChild->EnableWindow( bEnable );
	}


	return TRUE;
}



int CAutoEnableStatic::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CStatic::OnCreate(lpCreateStruct) == -1)
		return -1;

	Init();

	return 0;
}

void CAutoEnableStatic::PreSubclassWindow()
{
	CStatic::PreSubclassWindow();

	Init();
}

void AFXAPI CAutoEnableStatic::DDX_Check(CDataExchange* pDX, int nIDC, bool& value)
{
	pDX->PrepareCtrl(nIDC);

	CAutoEnableStatic* pWnd = dynamic_cast<CAutoEnableStatic*>(pDX->m_pDlgWnd->GetDlgItem(nIDC));
	if( pWnd )
	{
		if (pDX->m_bSaveAndValidate)
		{
			value = pWnd->GetCheck()!=0;
		}
		else
		{
			 pWnd->SetCheck(value);
		}
	}
}

HBRUSH CAutoEnableStatic::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	if( pWnd == &m_checkbox)
	{
		CClientDC dc(this);
		//CRect rect;
		//pWnd->GetWindowRect(rect);
		COLORREF rgb = dc.GetPixel(0, 0);
		//COLORREF rgb = pDC->GetPixel(0, 0);
		if((HBRUSH)m_br == NULL)
			m_br.CreateSolidBrush(rgb);

		return m_br;
	}

	return CStatic::OnCtlColor(pDC, pWnd, nCtlColor);
}

//*****************************************************************************************************************
IMPLEMENT_DYNCREATE(CMFCTabCtrl24, CMFCTabCtrl)

BEGIN_MESSAGE_MAP(CPaneSplitter, CSplitterWndEx)
	ON_REGISTERED_MESSAGE(WM_XHTMLTREE_CHECKBOX_CLICKED, OnCheckbox)
END_MESSAGE_MAP()

LRESULT CPaneSplitter::OnCheckbox(WPARAM wParam, LPARAM lParam)
{
	return GetParent()->SendMessage(WM_XHTMLTREE_CHECKBOX_CLICKED, wParam, lParam);
}


//*****************************************************************************************************************
//slit toolbar at separator
IMPLEMENT_SERIAL(CSplittedToolBar, CMFCToolBar, 1)

void CSplittedToolBar::AdjustLocations()
{
	ASSERT_VALID(this);

	if (m_Buttons.IsEmpty() || GetSafeHwnd() == NULL)
	{
		return;
	}

	BOOL bHorz =  GetCurrentAlignment() & CBRS_ORIENT_HORZ ? TRUE : FALSE;

	CRect rectClient;
	GetClientRect(rectClient);

	int xRight = rectClient.right;

	CClientDC dc(this);
	CFont* pOldFont;
	if (bHorz)
	{
		pOldFont = SelectDefaultFont(&dc);
	}
	else
	{
		pOldFont = (CFont*)dc.SelectObject(&(GetGlobalData()->fontVert));
	}

	ENSURE(pOldFont != NULL);

	int iStartOffset;
	if (bHorz)
	{
		iStartOffset = rectClient.left + 1;
	}
	else
	{
		iStartOffset = rectClient.top + 1;
	}

	int iOffset = iStartOffset;
	int y = rectClient.top;

	CSize sizeGrid(GetColumnWidth(), GetRowHeight());

	CSize sizeCustButton(0, 0);

	BOOL bPrevWasSeparator = FALSE;
	int nRowActualWidth = 0;
	for (POSITION pos = m_Buttons.GetHeadPosition(); pos != NULL;)
	{
		POSITION posSave = pos;

		CMFCToolBarButton* pButton = (CMFCToolBarButton*)m_Buttons.GetNext(pos);
		if (pButton == NULL)
		{
			break;
		}

		

		ASSERT_VALID(pButton);

		BOOL bVisible = TRUE;

		CSize sizeButton = pButton->OnCalculateSize(&dc, sizeGrid, bHorz);
		
		if (pButton->m_bTextBelow && bHorz)
		{
			sizeButton.cy = sizeGrid.cy;
		}

		if (pButton->m_nStyle & TBBS_SEPARATOR)
		{
			if (iOffset == iStartOffset || bPrevWasSeparator)
			{
				sizeButton = CSize(0, 0);
				bVisible = FALSE;
			}
			else
			{
				bPrevWasSeparator = TRUE;
			}
		}

		int iOffsetPrev = iOffset;

		CRect rectButton;
		if (bHorz)
		{
			rectButton.left = iOffset;
			rectButton.right = rectButton.left + sizeButton.cx;
			rectButton.top = y;
			rectButton.bottom = rectButton.top + sizeButton.cy;

			iOffset += sizeButton.cx;
			nRowActualWidth += sizeButton.cx;
		}
		else
		{
			rectButton.left = rectClient.left;
			rectButton.right = rectClient.left + sizeButton.cx;
			rectButton.top = iOffset;
			rectButton.bottom = iOffset + sizeButton.cy;

			iOffset += sizeButton.cy;
		}

		pButton->Show(bVisible);
		pButton->SetRect(rectButton);

		if (bVisible)
		{
			bPrevWasSeparator = (pButton->m_nStyle & TBBS_SEPARATOR);
		}

		//if (pButton->m_nID == ID_GRAPH_ZOOM)
		if (pButton->m_nStyle & TBBS_SEPARATOR)
		{
			int size = 0;
			//compute total length of the right control
			for (POSITION pos2 = pos; pos2 != NULL;)
			{
				CMFCToolBarButton* pButton2 = (CMFCToolBarButton*)m_Buttons.GetNext(pos2);
				if (pButton2 == NULL)
				{
					break;
				}

				CSize sizeButton2 = pButton2->OnCalculateSize(&dc, sizeGrid, bHorz);
				size += bHorz ? sizeButton2.cx : sizeButton2.cy;

			}

			//add delta
			int delta = std::max(0, (bHorz ? rectClient.Width() : rectClient.Height()) - iOffset - 1 - size);
			iOffset += delta;
		}
		
	}


	dc.SelectObject(pOldFont);
	UpdateTooltips();
	RedrawCustomizeButton();
}

//*********************************************************************************************************
IMPLEMENT_DYNAMIC(CStdTRefProperty, CMFCPropertyGridProperty)

void CStdGriFolderProperty2::OnClickButton(CPoint point)

{
	CString strFolder = GetValue();
	if (strFolder.IsEmpty())
		strFolder = CStringA(WBSF::GetApplicationPath().c_str());

	CString strResult;

	if (afxShellManager->BrowseForFolder(strResult, m_pWndList, strFolder, 0, BIF_USENEWUI))//BIF_NEWDIALOGSTYLE
	{
		if (strResult != strFolder)
			SetValue(strResult);
	}

}

CStdGeoRectProperty::CStdGeoRectProperty(const std::string& name, const std::string& value, const std::string& description, size_t no) :
CStdGridProperty(name, no, true)
{

	WBSF::CGeoRect rect(-180, -90, 180, 90, WBSF::PRJ_WGS_84);
	if (!value.empty())
	{
		std::stringstream tmp(value);
		rect << tmp;
	}

	CStdGridProperty* pProp = NULL;

	pProp = new CStdGridProperty("Xmin", rect.m_xMin, "Specifies the window's height", m_dwData * 1000 + 1);
	AddSubItem(pProp);

	pProp = new CStdGridProperty("Xmax", rect.m_xMax, "Specifies the window's width", m_dwData * 1000 + 2);
	AddSubItem(pProp);

	pProp = new CStdGridProperty("Ymin", rect.m_yMin, "Specifies the window's height", m_dwData * 1000 + 3);
	AddSubItem(pProp);

	pProp = new CStdGridProperty("Ymax", rect.m_yMax, "Specifies the window's width", m_dwData * 1000 + 4);
	AddSubItem(pProp);
}


//*********************************************************************************************************


//**** pour créer des menu à partir de string  ********


//void CReadOnlyEdit::OnContextMenu(CWnd* pWnd, CPoint point)
//{
//CStringArrayEx str(IDS_STR_EDIT_COMMAND);

//CMenu menu;
//menu.CreatePopupMenu();

//DWORD sel = GetSel();
//DWORD flags = LOWORD(sel) == HIWORD(sel) ? MF_GRAYED : 0;
//menu.InsertMenu(0, MF_BYPOSITION | flags, WM_COPY, str[2]);

//int len = GetWindowTextLength();
//flags = (!len || (LOWORD(sel) == 0 && HIWORD(sel) ==len)) ? MF_GRAYED : 0;
//menu.InsertMenu(1, MF_BYPOSITION | flags, ID_EDIT_SELECT_ALL, str[0]);
//menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON |TPM_RIGHTBUTTON, point.x, point.y, this);
//
//SetFocus();
//}

//BOOL CReadOnlyEdit::OnCommand(WPARAM wParam, LPARAM lParam)
//{
//	switch (LOWORD(wParam))
//	{
//	case WM_CUT:
//	case WM_CLEAR:
//	case WM_PASTE:break;
//	case WM_COPY: return SendMessage(LOWORD(wParam))!=0;
//	case ID_EDIT_SELECT_ALL: return SendMessage(EM_SETSEL, 0, -1) != 0;
//	}
//
//	return CEdit::OnCommand(wParam, lParam);
//}
