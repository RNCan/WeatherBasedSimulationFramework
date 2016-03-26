#include "stdafx.h"   
#include <locale.h>   
#include "MFCPropertyGridDateTimeProperty.h"   

#ifdef _DEBUG   
#define new DEBUG_NEW   
#undef THIS_FILE   
static char THIS_FILE[] = __FILE__;
#endif   

/////////////////////////////////////////////////////////////////////////////   
// CNotifyEdit   
//
//CNotifyEdit::CNotifyEdit()
//{
//}
//
//CNotifyEdit::~CNotifyEdit()
//{
//}
//
//BEGIN_MESSAGE_MAP(CNotifyEdit, CEdit)
//END_MESSAGE_MAP()
//
//LRESULT CNotifyEdit::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
//{
//	if (message == WM_KILLFOCUS)
//		GetParent()->PostMessage(message, wParam, lParam);
//
//	return CEdit::WindowProc(message, wParam, lParam);
//}

/////////////////////////////////////////////////////////////////////////////   
// CPropDateTimeCtrl   

CPropDateTimeCtrl::CPropDateTimeCtrl(CMFCPropertyGridDateTimeProperty *pProp, COLORREF clrBack)
{
	m_clrBack = clrBack;
	m_brBackground.CreateSolidBrush(m_clrBack);
	m_pProp = pProp;
}

BEGIN_MESSAGE_MAP(CPropDateTimeCtrl, CDateTimeCtrl)
	ON_WM_KILLFOCUS()
	ON_WM_CTLCOLOR_REFLECT()
	ON_WM_HSCROLL_REFLECT()
END_MESSAGE_MAP()

void CPropDateTimeCtrl::OnKillFocus(CWnd* pNewWnd)
{
	if (pNewWnd != NULL && IsChild(pNewWnd))
		return;
	CDateTimeCtrl::OnKillFocus(pNewWnd);
}

HBRUSH CPropDateTimeCtrl::CtlColor(CDC* pDC, UINT /*nCtlColor*/)
{
	pDC->SetBkColor(m_clrBack);
	return m_brBackground;
}

void CPropDateTimeCtrl::HScroll(UINT /*nSBCode*/, UINT /*nPos*/)
{
	ASSERT_VALID(m_pProp);

	m_pProp->OnUpdateValue();
	m_pProp->Redraw();
}

LRESULT CPropDateTimeCtrl::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message >= WM_MOUSEFIRST && message <= WM_MOUSELAST)
	{
		POINT pt;
		GetCursorPos(&pt);
		CWnd *wnd = NULL;
		CRect rect;
		wnd = GetWindow(GW_CHILD);
		while (wnd)
		{
			wnd->GetWindowRect(rect);
			if (rect.PtInRect(pt))
			{
				wnd->SendMessage(message, wParam, lParam);
				return TRUE;
			}
			wnd = wnd->GetWindow(GW_HWNDNEXT);
		}
	}
	if (message == WM_DESTROY)
		SetFont(NULL, FALSE);

	return CDateTimeCtrl::WindowProc(message, wParam, lParam);
}

/////////////////////////////////////////////////////////////////////////////   
// CMFCPropertyGridDateTimeProperty   

IMPLEMENT_DYNAMIC(CMFCPropertyGridDateTimeProperty, CMFCPropertyGridProperty)

CMFCPropertyGridDateTimeProperty::CMFCPropertyGridDateTimeProperty(const CString& strName, COleDateTime &nValue, LPCTSTR lpszDescr, DWORD dwData, BOOL style, BOOL updown, LPCTSTR setFormat, LPCTSTR format) :
CMFCPropertyGridProperty(strName, nValue, lpszDescr, dwData)
{
	m_style = style;
	m_updown = updown;
	m_setformat = setFormat;
	m_format = format;
}

CWnd* CMFCPropertyGridDateTimeProperty::CreateInPlaceEdit(CRect rectEdit, BOOL& bDefaultFormat)
{
	CPropDateTimeCtrl* pWndDateTime = new CPropDateTimeCtrl(this, m_pWndList->GetBkColor());

	rectEdit.InflateRect(4, 2, 0, 3);

	DWORD style = WS_VISIBLE | WS_CHILD;
	if (m_style == TRUE)
		style |= DTS_SHORTDATEFORMAT | m_updown ? DTS_UPDOWN : 0;
	else
		style |= DTS_TIMEFORMAT | DTS_UPDOWN;

	pWndDateTime->Create(style, rectEdit, m_pWndList, AFX_PROPLIST_ID_INPLACE);

	if (!m_setformat.IsEmpty())
		pWndDateTime->SetFormat(m_setformat);

	pWndDateTime->SetTime(m_varValue);

	bDefaultFormat = TRUE;
	return pWndDateTime;
}

BOOL CMFCPropertyGridDateTimeProperty::OnUpdateValue()
{
	ASSERT_VALID(this);
	ASSERT_VALID(m_pWndInPlace);
	ASSERT_VALID(m_pWndList);
	ASSERT(::IsWindow(m_pWndInPlace->GetSafeHwnd()));

	COleDateTime lCurrValue = COleDateTime(m_varValue.date);

	CDateTimeCtrl* pProp = (CDateTimeCtrl*)m_pWndInPlace;

	COleDateTime datetime;
	pProp->GetTime(datetime);
	m_varValue.date = datetime;

	if (lCurrValue != COleDateTime(m_varValue.date))
		m_pWndList->OnPropertyChanged(this);

	return TRUE;
}

BOOL CMFCPropertyGridDateTimeProperty::IsValueChanged() const
{
	ASSERT_VALID(this);

	if (m_varValueOrig.vt != m_varValue.vt)
		return FALSE;

	const COleDateTime var(m_varValue);
	const COleDateTime var1(m_varValueOrig);

	if (m_varValue.vt == VT_DATE)
		return var != var1;

	return FALSE;
}

CString CMFCPropertyGridDateTimeProperty::FormatProperty()
{
	CString strVal;
	if (m_format.IsEmpty())
	{
		setlocale(LC_ALL, "");
		strVal = COleDateTime(m_varValue).Format(m_style ? _T("%x") : _T("%X"));
	}
	else
		strVal = COleDateTime(m_varValue).Format(m_format);

	return strVal;
}

CString CMFCPropertyGridDateTimeProperty::FormatOriginalProperty()
{
	CString strVal;
	if (m_format.IsEmpty())
	{
		setlocale(LC_ALL, "");
		strVal = COleDateTime(m_varValueOrig).Format(m_style ? _T("%x") : _T("%X"));
	}
	else
		strVal = COleDateTime(m_varValueOrig).Format(m_format);

	return strVal;
}