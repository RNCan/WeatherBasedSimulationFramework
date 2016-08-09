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

#include "UI/Common/UtilWin.h"

#include "ModelFormItem.h"
#include "ModelFormDlg.h"

#include "WeatherBasedSimulationString.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace std;


namespace WBSF
{

	const int CModelFormItem::kGridedDis = 5;

	/////////////////////////////////////////////////////////////////////////////
	// CModelFormItem
	HCURSOR CModelFormItem::m_hFourArrow = NULL;
	HCURSOR CModelFormItem::m_hHorArrow = NULL;
	HCURSOR CModelFormItem::m_hArrow = NULL;

	BEGIN_MESSAGE_MAP(CModelFormItem, CWnd)
		//{{AFX_MSG_MAP(CModelFormItem)
		ON_WM_MOUSEMOVE()
		ON_WM_LBUTTONDOWN()
		ON_WM_LBUTTONUP()
		ON_WM_PAINT()
		ON_WM_SETCURSOR()
		ON_WM_GETMINMAXINFO()
		ON_WM_SIZING()
		ON_WM_ERASEBKGND()
		//}}AFX_MSG_MAP
	END_MESSAGE_MAP()


	CModelFormItem::CModelFormItem(CModelInputParameterDef& inVar) :
		m_param(inVar), m_bBeginSFTrack(false), m_bBeginSize(false),
		m_bMoveWnd(false)
	{
		if (m_hFourArrow == NULL)
			m_hFourArrow = ::LoadCursor(NULL, IDC_SIZEALL);
		if (m_hHorArrow == NULL)
			m_hHorArrow = ::LoadCursor(NULL, IDC_SIZEWE);
		if (m_hArrow == NULL)
			m_hArrow = ::LoadCursor(NULL, IDC_ARROW);
	}

	CModelFormItem::~CModelFormItem()
	{}

	//void CModelFormItem::SetParam(CModelInputParameterDef& in)
	//{
	//	m_param = in;
	//}
	/////////////////////////////////////////////////////////////////////////////
	// CModelFormItem message handlers

	void CModelFormItem::OnMouseMove(UINT nFlags, CPoint point)
	{
		CRect rect;
		GetClientRect(rect);

		if (m_bBeginSFTrack)
		{
			int RightDist;

			if (m_param.GetType() == CModelInputParameterDef::kMVFile)RightDist = CModelInputParameterDef::WIDTH_BUTTON_BROWSE + CModelInputParameterDef::MARGIN_HORZ;
			else RightDist = CModelInputParameterDef::MARGIN_HORZ;

			if ((point.x > CModelInputParameterDef::MARGIN_HORZ * 2) && (point.x < (rect.Width() - RightDist)))
			{
				if (!(nFlags & MK_CONTROL)) GetGridedPoint(point);
				m_param.SetBeginSecondField(point.x);
				Invalidate();
			}
		}
		else if (m_bBeginSize)
		{
			CPoint pt2(point);
			ClientToScreen(&pt2);
			GetParent()->ScreenToClient(&pt2);

			CRect ParentRect;
			GetParent()->GetClientRect(ParentRect);

			if (!(nFlags & MK_CONTROL))	GetGridedPoint(pt2);

			if ((pt2.x > 0) && (pt2.x < ParentRect.Width()) &&
				(pt2.y > 0) && (pt2.y < ParentRect.Height()))
			{
				CRect pos;
				GetWindowRect(pos);
				GetParent()->ScreenToClient(pos);



				pos.right -= m_lastPoint.x - pt2.x;

				if (!(nFlags & MK_CONTROL)) GetGridedPoint(pos);
				Invalidate();
				MoveWindow(pos, true);
				m_lastPoint.x = pt2.x;
				m_lastPoint.y = pt2.y;

				m_param.SetRect(pos);
			}
		}
		else if (m_bMoveWnd)
		{
			CPoint pt2(point);
			ClientToScreen(&pt2);
			GetParent()->ScreenToClient(&pt2);

			CRect ParentRect;
			GetParent()->GetClientRect(ParentRect);

			if (!(nFlags & MK_CONTROL))	GetGridedPoint(pt2);

			if ((pt2.x > 0) && (pt2.x < ParentRect.Width()) &&
				(pt2.y > 0) && (pt2.y < ParentRect.Height()))
			{
				CRect pos;
				GetWindowRect(pos);
				GetParent()->ScreenToClient(pos);

				pos -= CPoint(m_lastPoint.x - pt2.x, m_lastPoint.y - pt2.y);

				if (!(nFlags & MK_CONTROL)) GetGridedPoint(pos);
				Invalidate();
				MoveWindow(pos, true);
				m_lastPoint.x = pt2.x;
				m_lastPoint.y = pt2.y;

				m_param.SetRect(pos);
			}
		}

		CWnd::OnMouseMove(nFlags, point);
	}

	void CModelFormItem::OnLButtonDown(UINT nFlags, CPoint point)
	{
		int eff = m_param.GetEndFirstField();
		int bsf = m_param.GetBeginSecondField();
		int right = CRect(m_param.GetRect()).Width();
		CString tmp = CString(m_param.GetName().c_str());
		m_pParent->OnItemActivation(tmp);


		if (((point.x >= eff) && (point.x <= bsf)))
		{
			if (m_param.GetType() != CModelInputParameterDef::kMVLine &&
				m_param.GetType() != CModelInputParameterDef::kMVTitle &&
				m_param.GetType() != CModelInputParameterDef::kMVStaticText)
			{
				m_bBeginSFTrack = true;
				SetCapture();
			}
		}
		else
		{
			//ClientToScreen(&point);
			//GetParent()->ScreenToClient(&point);

			m_lastPoint = point;
			ClientToScreen(&m_lastPoint);
			GetParent()->ScreenToClient(&m_lastPoint);
			if (!(nFlags & MK_CONTROL))
				GetGridedPoint(m_lastPoint);

			TRACE2("m_lastPoint x : %d, y : %d\n", m_lastPoint.x, m_lastPoint.y);
			
			if (((point.x >= right - 4) && (point.x <= right + 4)))
				m_bBeginSize = true;
			else 
				m_bMoveWnd = true;

			Invalidate();
			SetCapture();
		}

		

		CWnd::OnLButtonDown(nFlags, point);
	}

	void CModelFormItem::OnLButtonUp(UINT nFlags, CPoint point)
	{
		if (m_bBeginSFTrack || m_bBeginSize || m_bMoveWnd)
		{
			m_bBeginSFTrack = m_bBeginSize = m_bMoveWnd = false;
			ReleaseCapture();
			Invalidate(true);
			CString tmp = CString(m_param.GetName().c_str());
			m_pParent->OnUpdateParent(tmp);
		}

		CWnd::OnLButtonUp(nFlags, point);
	}


	BOOL CModelFormItem::Create(CModelFormDialog* pParentWnd, UINT nID)
	{
		ASSERT(&m_param != NULL);

		BOOL rep = CWnd::Create(NULL, _T(""), WS_CHILD | WS_VISIBLE/*| WS_THICKFRAME*/, m_param.GetRect(), pParentWnd, nID, NULL);
		m_pParent = pParentWnd;

		return rep;
	}

	void CModelFormItem::OnPaint()
	{
		CPaintDC dc(this); // device context for painting

		CRect rectItem;
		//GetWindowRect(rectItem);
		//ScreenToClient(rectItem);
		//rectItem-=rectItem.TopLeft();

		//CRect clientRect;
		GetClientRect(rectItem);
		//rectItem+=CPoint(2,2);

		HFONT hfnt, hOldFont;

		hfnt = (HFONT)GetStockObject(DEFAULT_GUI_FONT); // SYSTEM_FONT ANSI_VAR_FONT
		hOldFont = (HFONT)dc.SelectObject(hfnt);


		if (m_param.GetType() != CModelInputParameterDef::kMVLine && 
			m_param.GetType() != CModelInputParameterDef::kMVTitle &&
			m_param.GetType() != CModelInputParameterDef::kMVStaticText)
		{

			CRect rectCaption(rectItem);
			rectCaption.left += CModelInputParameterDef::MARGIN_HORZ;
			rectCaption.right = m_param.GetEndFirstField();

			dc.DrawText(CString(m_param.GetCaption().c_str()), rectCaption, DT_VCENTER | DT_SINGLELINE | DT_LEFT);
		}


		switch (m_param.GetType())
		{
		case CModelInputParameterDef::kMVInt:
		case CModelInputParameterDef::kMVReal:
		case CModelInputParameterDef::kMVString:
		{
			CRect rectValue(rectItem);
			rectValue.left = m_param.GetBeginSecondField() + 2;
			rectValue.right -= CModelInputParameterDef::MARGIN_HORZ;

			dc.DrawText(CString(m_param.GetTypeName()), rectValue, DT_VCENTER | DT_SINGLELINE | DT_LEFT);

			DrawField(dc);

			break;
		}


		case CModelInputParameterDef::kMVBool:
		case CModelInputParameterDef::kMVListByPos:
		case CModelInputParameterDef::kMVListByString:
		{
			CRect rectValue(rectItem);

			rectValue.bottom -= 1;
			rectValue.left = m_param.GetBeginSecondField();
			rectValue.right = rectItem.right - CModelInputParameterDef::MARGIN_HORZ;


			dc.MoveTo(rectValue.left, rectValue.top);
			dc.LineTo(rectValue.right, rectValue.top);
			dc.LineTo(rectValue.right, rectValue.bottom);
			dc.LineTo(rectValue.left, rectValue.bottom);
			dc.LineTo(rectValue.left, rectValue.top);

			rectValue.right -= 16;
			if (rectValue.right - rectValue.left >= 0)
			{
				dc.MoveTo(rectValue.right, rectValue.top);
				dc.LineTo(rectValue.right, rectValue.bottom);

				POINT lpPoints[3] =
				{
					{ rectValue.right + 4, rectValue.top + 4 },
					{ rectValue.right + 12, rectValue.top + 4 },
					{ rectValue.right + 8, rectValue.bottom - 4 }
				};

				CBrush brush(RGB(0, 0, 0));
				CBrush* oldBrush = (CBrush*)dc.SelectObject(&brush);
				dc.Polygon(lpPoints, 3);
				dc.SelectObject(oldBrush);
			}

			rectValue.left += 2;
			rectValue.top += 1;
			rectValue.bottom -= 1;
			if (m_param.GetType() == CModelInputParameterDef::kMVBool)
			{
				CString strValue;
				bool bDefault = WBSF::ToBool(m_param.m_default);

				if (bDefault) strValue.LoadString(IDS_STR_TRUE);
				else strValue.LoadString(IDS_STR_FALSE);


				dc.DrawText(strValue, rectValue, DT_VCENTER | DT_SINGLELINE | DT_LEFT);
			}
			else  // it's a list
			{
				//CString strValue;
				string str;
				int defPos = WBSF::ToInt(m_param.m_default);
				StringVector listOfParam = m_param.GetList();

				if (defPos < (int)listOfParam.size())
				{
					str = listOfParam[defPos];
				}


				dc.DrawText(UtilWin::Convert(str), rectValue, DT_VCENTER | DT_SINGLELINE | DT_LEFT);
			}


			DrawField(dc);

			break;
		}
		case CModelInputParameterDef::kMVFile:
		{
			CRect rectValue(rectItem);
			rectValue.left = m_param.GetBeginSecondField() + 2;
			rectValue.right = rectItem.right - CModelInputParameterDef::MARGIN_HORZ - CModelInputParameterDef::WIDTH_BUTTON_BROWSE;




			CString strValue;
			strValue.LoadString(IDS_STR_FILENAME);
			dc.DrawText(strValue, rectValue, DT_VCENTER | DT_SINGLELINE | DT_LEFT);

			rectValue.left = rectValue.right;
			rectValue.right = rectItem.right - CModelInputParameterDef::MARGIN_HORZ;
			rectValue.bottom -= 1;
			rectValue.top += 1;

			CBrush brush(RGB(0, 0, 0));
			dc.FrameRect(rectValue, &brush);

			rectValue.bottom -= 1;
			rectValue.top += 1;

			dc.DrawText(_T("···"), rectValue, DT_VCENTER | DT_SINGLELINE | DT_CENTER);

			DrawField(dc);

			break;
		}

		case CModelInputParameterDef::kMVLine:
		{
			dc.MoveTo(0, rectItem.Height() / 2);
			dc.LineTo(rectItem.right, rectItem.Height() / 2);
			break;
		}

		case CModelInputParameterDef::kMVTitle:
		case CModelInputParameterDef::kMVStaticText:
		{
			CRect rectCaption(rectItem);
			rectCaption.left += CModelInputParameterDef::MARGIN_HORZ;
			rectCaption.right -= CModelInputParameterDef::MARGIN_HORZ;

			dc.DrawText(UtilWin::Convert(m_param.GetCaption()), rectCaption, DT_VCENTER | DT_SINGLELINE | DT_LEFT);

			break;
		}

		default: TRACE("Erreur: mauvait type. CModelFormItem::OnPaint().\n");
		}

		dc.SelectObject(hOldFont);
		// Do not call CWnd::OnPaint() for painting messages
	}

	BOOL CModelFormItem::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
	{

		if ((nHitTest == HTTOPRIGHT) || (nHitTest == HTTOPLEFT))
		{
			::SetCursor(m_hHorArrow);
			return true;
		}

		if ((nHitTest == HTTOP) || (nHitTest == HTBOTTOM))
		{
			::SetCursor(m_hArrow);
			return true;
		}

		if (nHitTest == HTCLIENT)
		{
			DWORD dwPos = GetMessagePos();
			CPoint point(LOWORD(dwPos), HIWORD(dwPos));
			ScreenToClient(&point);

			int eff = m_param.GetEndFirstField();
			int bsf = m_param.GetBeginSecondField();
			int right = CRect(m_param.GetRect()).Width();
			bool bTrack = m_param.GetType() != CModelInputParameterDef::kMVLine && m_param.GetType() != CModelInputParameterDef::kMVTitle && m_param.GetType() != CModelInputParameterDef::kMVStaticText && (point.x >= eff) && (point.x <= bsf);
			bool bSize = (point.x >= right - 4) && (point.x <= right + 4);
			if (bTrack || bSize)
				::SetCursor(m_hHorArrow);
			else 
				::SetCursor(m_hArrow);

			return true;
		}
	
	return CWnd::OnSetCursor(pWnd, nHitTest, message);

	}

	void CModelFormItem::DrawField(CPaintDC & dc)
	{
		CRect rectItem;
		GetClientRect(rectItem);
		int eff = m_param.GetEndFirstField();
		int bsf = m_param.GetBeginSecondField();

		dc.MoveTo(eff, rectItem.top + 2);
		dc.LineTo(eff, rectItem.bottom - 2);
		dc.MoveTo(bsf, rectItem.top + 2);
		dc.LineTo(bsf, rectItem.bottom - 2);

	}


	void CModelFormItem::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI)
	{
		CRect rect;
		GetParent()->GetClientRect(rect);

		lpMMI->ptMinTrackSize.x = 4 * CModelInputParameterDef::MARGIN_HORZ + CModelInputParameterDef::WIDTH_BUTTON_BROWSE;
		lpMMI->ptMinTrackSize.y = CModelInputParameterDef::DEFAULT_HEIGHT;
		lpMMI->ptMaxTrackSize.x = rect.Width();
		lpMMI->ptMaxTrackSize.y = CModelInputParameterDef::DEFAULT_HEIGHT;

		CWnd::OnGetMinMaxInfo(lpMMI);
	}


	CPoint& CModelFormItem::GetGridedPoint(CPoint& point)
	{
		point.x /= kGridedDis;
		point.x *= kGridedDis;
		point.y /= kGridedDis;
		point.y *= kGridedDis;

		return point;
	}

	CRect& CModelFormItem::GetGridedPoint(CRect& rect)
	{
		rect.left /= kGridedDis;
		rect.left *= kGridedDis;
		rect.right /= kGridedDis;
		rect.right *= kGridedDis;
		rect.top /= kGridedDis;
		rect.top *= kGridedDis;
		rect.bottom /= kGridedDis;
		rect.bottom *= kGridedDis;

		return rect;
	}




	void CModelFormItem::OnSizing(UINT fwSide, LPRECT pRect)
	{
		CWnd::OnSizing(fwSide, pRect);

		//	CRect rect(pRect);
		//GetParent()->ScreenToClient(rect);	

		//GetGridedPoint(rect);

		//if( rect != m_param.GetRect() )
		//{
		//m_param.SetRect(rect);
		//MoveWindow(rect);//???
		//m_pParent->OnUpdateParent(m_param.GetName());
		//}
	}


	BOOL CModelFormItem::OnEraseBkgnd(CDC* pDC)
	{
		CRect rectItem;
		GetClientRect(rectItem);
		//	CRect rectItem;
		//GetWindowRect(rectItem);
		//ScreenToClient(rectItem);



		pDC->FillSolidRect(rectItem, ::GetSysColor(COLOR_3DFACE));
		pDC->DrawFrameControl(rectItem, DFC_BUTTON, DFCS_BUTTONPUSH);
		//pDC->Draw3dRect(rectItem, ::GetSysColor(COLOR_3DHIGHLIGHT), ::GetSysColor(COLOR_3DSHADOW) );


		return true;
	}
}