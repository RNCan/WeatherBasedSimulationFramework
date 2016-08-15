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
#include "Basic/Registry.h"
#include "GeoRectCtrl.h"
#include "UtilWin.h"



namespace WBSF
{

	BEGIN_MESSAGE_MAP(CGeoPointCtrl, CMFCEditBrowseCtrlEx)
		ON_CONTROL_REFLECT(EN_SETFOCUS, &CGeoPointCtrl::OnEnSetfocus)
	END_MESSAGE_MAP()

	int CGeoPointCtrl::GetType()
	{
		WBSF::CRegistry registry;
		std::string value = registry.GetProfileString("GeoCoordType", "0");

		return WBSF::ToInt(value);
	}

	void CGeoPointCtrl::SetType(int type)
	{
		WBSF::CRegistry registry;
		registry.WriteProfileString("GeoCoordType", WBSF::ToString(type));
	}

	const TCHAR* CGeoPointCtrl::GetButtonLable()
	{
		int type = GetType();
		return type == 0 ? _T("°") : _T("°s");
	}

	CGeoPointCtrl::CGeoPointCtrl()
	{
		m_Mode = BrowseMode_Default;
		m_strLabel = GetButtonLable();
	}

	CGeoPointCtrl::~CGeoPointCtrl(void)
	{
	}

	double CGeoPointCtrl::GetCoord()const
	{
		std::string tmp = GetString();
		double coord = StringToCoord(tmp);

		return coord;
	}

	void CGeoPointCtrl::SetCoord(double coord)
	{
		m_coord = coord;
		UpdateCtrl();
	}

	void CGeoPointCtrl::OnBrowse()
	{
		//m_coord = GetCoord();

		int type = GetType();
		type = type == 0 ? 1 : 0;

		SetType(type);
		UpdateText();


		//send notification to the parent
		NMHDR nm;
		nm.hwndFrom = m_hWnd;
		nm.idFrom = GetDlgCtrlID();
		nm.code = GEN_TYPE_CHANGE;

		GetParent()->SendMessage(WM_NOTIFY, 0, (LPARAM)&nm);
	}

	void CGeoPointCtrl::UpdateText()
	{
		m_coord = GetCoord();
		UpdateCtrl();
	}

	void CGeoPointCtrl::UpdateCtrl()
	{
		ASSERT(GetSafeHwnd());
		CString oldText = GetWindowText();

		int type = GetType();
		CString newStrLable = GetButtonLable();
		std::string txt = type == 0 ? ToString(m_coord, 5) : CoordToString(m_coord, true);
		CString newText(txt.c_str());


		if (newStrLable != m_strLabel ||
			newText != oldText)
		{
			m_strLabel = newStrLable;
			SetWindowText(newText);

			//update window
			CRect rect;
			GetWindowRect(rect);
			GetParent()->ScreenToClient(rect);
			GetParent()->InvalidateRect(rect);
			GetParent()->UpdateWindow();
		}
	}


	void CGeoPointCtrl::OnEnSetfocus()
	{
		UpdateText();
	}

	void AFXAPI DDX_Control(CDataExchange* pDX, int nIDC, CGeoPointCtrl& rControl)
	{
		::DDX_Control(pDX, nIDC, (CWnd&)rControl);
		rControl.EnableBrowseButton();
	}
	void AFXAPI DDX_Coord(CDataExchange* pDX, int nIDC, double& coord)
	{
		CGeoPointCtrl* wndCtrl = (CGeoPointCtrl*)pDX->m_pDlgWnd->GetDlgItem(nIDC);
		ASSERT(wndCtrl);

		if (pDX->m_bSaveAndValidate)
		{
			coord = wndCtrl->GetCoord();
		}
		else
		{
			wndCtrl->SetCoord(coord);
		}
	}

	//****************************************************************

	BEGIN_MESSAGE_MAP(CGeoRectCtrl, CAutoEnableStatic)
		ON_NOTIFY_RANGE(GEN_TYPE_CHANGE, 1001, 1004, GeoCoordTypeChange)
		//	ON_WM_ENABLE()
	END_MESSAGE_MAP()

	CGeoRectCtrl::CGeoRectCtrl(bool bShowCheckbox) :
		CAutoEnableStatic(bShowCheckbox)
	{}

	void CGeoRectCtrl::GeoCoordTypeChange(UINT id, NMHDR * pNotifyStruct, LRESULT * result)
	{
		for (int i = 0; i < 4; i++)
			m_ctrl[i].UpdateText();

		//Invalidate();
		//UpdateWindow();
	}

	void CGeoRectCtrl::PreSubclassWindow()
	{
		//create sub item before  call parent
		CRect rect;

		GetClientRect(rect);
		int midX = rect.Width() / 2 + 2;
		int midY = rect.Height() / 2 + 4;
		int w = rect.Width() / 2 - 22;
		int w½ = w / 2;

		CRect pos[4] =
		{
			CRect(midX - w½, midY - 14 - 22, midX + w½, midY - 14),
			CRect(midX - w½, midY + 14, midX + w½, midY + 14 + 22),
			CRect(midX - w - 14, midY - 11, midX - 14, midY + 11),
			CRect(midX + 14, midY - 11, midX + 14 + w, midY + 11)
		};

		ModifyStyleEx(0, WS_EX_CONTROLPARENT);

		for (int i = 0; i < 4; i++)
		{
			DWORD style = WS_GROUP | WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_BORDER;

			m_ctrl[i].CreateEx(WS_EX_CLIENTEDGE, _T("EDIT"), NULL, style, pos[i], this, 1001 + i);
			m_ctrl[i].SetFont(GetFont());
			m_ctrl[i].SetCoord(0);
		}

		CAutoEnableStatic::PreSubclassWindow();

	}

	WBSF::CGeoRect CGeoRectCtrl::GetGeoRect()const
	{
		double coord[4] = { 0 };
		for (int i = 0; i < 4; i++)
			coord[i] = m_ctrl[i].GetCoord();

		WBSF::CGeoRect rect(coord[2], coord[0], coord[3], coord[1], WBSF::PRJ_WGS_84);
		rect.NormalizeRect();

		return rect;
	}

	void CGeoRectCtrl::SetGeoRect(const WBSF::CGeoRect& rect)
	{
		double coord[4] = { rect.m_yMax, rect.m_yMin, rect.m_xMin, rect.m_xMax };
		for (int i = 0; i < 4; i++)
			m_ctrl[i].SetCoord(coord[i]);

	}

	void CGeoRectCtrl::OnSetFocus(CWnd* pOldWnd)
	{
		//CAutoEnableStatic::OnSetFocus(pOldWnd);
		m_ctrl[0].SetFocus();
	}

	void AFXAPI DDX_GeoRect(CDataExchange* pDX, int nIDC, WBSF::CGeoRect& rect)
	{
		CGeoRectCtrl* wndCtrl = (CGeoRectCtrl*)pDX->m_pDlgWnd->GetDlgItem(nIDC);
		ASSERT(wndCtrl);

		if (pDX->m_bSaveAndValidate)
		{
			rect = wndCtrl->GetGeoRect();
		}
		else
		{
			wndCtrl->SetGeoRect(rect);
		}
	}

	void AFXAPI DDV_GeoRect(CDataExchange* pDX, WBSF::CGeoRect& rect)
	{
		if (pDX->m_bSaveAndValidate)
		{
			if (!rect.IsRectNormal())
			{
				CString prompt;
				prompt = "Bad coordinate"; //.LoadString( IDS_CMN_NAME_EMPTY );
				AfxMessageBox(prompt, MB_ICONEXCLAMATION, AFX_IDP_PARSE_STRING_SIZE);
				prompt.Empty(); // exception prep
				pDX->Fail();
			}
		}
	}


	//void CGeoRectCtrl::OnEnable(BOOL bEnable)
	//{
	//	CAutoEnableStatic::OnEnable(bEnable);
	//
	//	for(int i=0; i<4; i++)
	//		m_ctrl[i].EnableWindow(bEnable);
	//
	//}
}