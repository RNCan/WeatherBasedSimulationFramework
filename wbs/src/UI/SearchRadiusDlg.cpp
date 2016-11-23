//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
// 11-11-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
//****************************************************************************
#include "stdafx.h"
#include "FileManager/FileManager.h"
#include "UI/Common/UtilWin.h"
#include "UI/Common/FileNameProperty.h"
#include "UI/Common/AppOption.h"
#include "UI/Common/CommonCtrl.h"

#include "SearchRadiusDlg.h"

#include "WeatherBasedSimulationString.h"

using namespace UtilWin;
using namespace std;
using namespace WBSF::HOURLY_DATA;

namespace WBSF
{

	//*****************************************************************************************************

	BEGIN_MESSAGE_MAP(CSearchRadiusPropertyGridCtrl, CMFCPropertyGridCtrl)
		//ON_WM_CREATE()
	END_MESSAGE_MAP()

	CSearchRadiusPropertyGridCtrl::CSearchRadiusPropertyGridCtrl(CSearchRadius& searchRadius):
		m_searchRadius(searchRadius)
	{
	}

	void CSearchRadiusPropertyGridCtrl::Init()
	{
		CMFCPropertyGridCtrl::Init();

		RemoveAll();


		CRect rect;
		GetClientRect(rect);

		//add all attribute
		CAppOption options;
		CStringArrayEx propertyHeader(UtilWin::GetCString(IDS_STR_PROPERTY_HEADER));
		m_nLeftColumnWidth = options.GetProfileInt(_T("SearchRadiusSplitterPos"), rect.Width() / 2);
		SetBoolLabels(UtilWin::GetCString(IDS_STR_TRUE), UtilWin::GetCString(IDS_STR_FALSE));

		EnableHeaderCtrl(true, propertyHeader[0], propertyHeader[1] + " [km]");
		EnableDescriptionArea(false);
		SetVSDotNetLook(true);
		MarkModifiedProperties(true);
		SetAlphabeticMode(true);
		SetShowDragContext(true);
		EnableWindow(true);
		AlwaysShowUserToolTip();

		//General
		CMFCPropertyGridProperty* pVariables = new CMFCPropertyGridProperty(GetCString(IDS_STR_NB_VARIABLES), -1);
		for (size_t v = 0; v < NB_VAR_H; v++)
		{
			pVariables->AddSubItem(new CStdGridProperty(HOURLY_DATA::GetVariableTitle(v), m_searchRadius[v]>=0 ? m_searchRadius[v] / 1000 : -1, GetVariableUnits(v), v));
		}
		
		AddProperty(pVariables);
	}

	/*void CSearchRadiusPropertyGridCtrl::Set(const CDispersalParamters& in)
	{
		m_parameters = in;

		
		for (int i = 0; i < NB_PROPERTIES; i++)
		{
			string str;
			switch (i)
			{
			case WEATHER_TYPE:		str = WBSF::ToString(in.m_world.m_weather_type); break;
			case TIME_STEP:			str = WBSF::ToString(in.m_world.m_time_step); break;
			case SEED_TYPE:			str = WBSF::ToString(in.m_world.m_seed); break;
			case REVERSED:			str = WBSF::ToString(in.m_world.m_bReversed); break;
			case USE_SPATIAL_INTERPOL:		str = WBSF::ToString(in.m_world.m_bUseSpaceInterpolation); break;
			case USE_TIME_INTERPOL:		str = WBSF::ToString(in.m_world.m_bUseTimeInterpolation); break;
			case USE_PREDICTOR_CORRECTOR_METHOD: str = WBSF::ToString(in.m_world.m_bUsePredictorCorrectorMethod); break;
			case ADD_TURBULENCE:		str = WBSF::ToString(in.m_world.m_bUseTurbulance); break;
			case EVENT_THRESHOLD:	str = WBSF::ToString(in.m_world.m_eventThreshold); break;
			case DEFOLIATION_THRESHOLD:	str = WBSF::ToString(in.m_world.m_defoliationThreshold); break;
			case DISTRACTION_THRESHOLD:	str = WBSF::ToString(in.m_world.m_distractionThreshold); break;
			case HOST_THRESHOLD:	str = WBSF::ToString(in.m_world.m_hostThreshold); break;
			case DEM:				str = in.m_world.m_DEM_name; break;
			case HOURLY_DB:			str = in.m_world.m_hourly_DB_name; break;
			case GRIBS:				str = in.m_world.m_gribs_name; break;
			case DEFOLIATION:		str = in.m_world.m_defoliation_name; break;
			case HOST:				str = in.m_world.m_host_name; break;
			case WATER:				str = in.m_world.m_water_name; break;
			case DISTRACTION:		str = in.m_world.m_distraction_name; break;
			case T_MIN:				str = WBSF::ToString(in.m_ATM.m_Tmin); break;
			case T_MAX:				str = WBSF::ToString(in.m_ATM.m_Tmax); break;
			case P_MAX:				str = WBSF::ToString(in.m_ATM.m_Pmax); break;
			case W_MIN:				str = WBSF::ToString(in.m_ATM.m_Wmin); break;
			case LIFTOFF_TYPE:		str = WBSF::ToString(in.m_ATM.m_t_liftoff_type); break;
			case LIFTOFF_BEGIN:		str = WBSF::ToString(in.m_ATM.m_t_liftoff_begin); break;
			case LIFTOFF_END:		str = WBSF::ToString(in.m_ATM.m_t_liftoff_end); break;
			case LIFTOFF_CORRECTION:str = WBSF::ToString(in.m_ATM.m_t_liftoff_correction); break;
			case LIFTOFF_SIGMA:		str = WBSF::ToString(in.m_ATM.m_t_liftoff_σ_correction); break;
			case DURATION_TYPE:		str = WBSF::ToString(in.m_ATM.m_duration_type); break;
			case DURATION_MEAN:		str = WBSF::ToString(in.m_ATM.m_duration); break;
			case DURATION_SD:		str = WBSF::ToString(in.m_ATM.m_duration_σ); break;
			case HEIGHT_TYPE:		str = WBSF::ToString(in.m_ATM.m_height_type); break;
			case HEIGHT_MIN:		str = WBSF::ToString(in.m_ATM.m_height_lo); break;
			case HEIGHT_MEAN:		str = WBSF::ToString(in.m_ATM.m_height); break;
			case HEIGHT_SD:			str = WBSF::ToString(in.m_ATM.m_height_σ); break;
			case HEIGHT_MAX:		str = WBSF::ToString(in.m_ATM.m_height_hi); break;
			case W_ASCENT:			str = WBSF::ToString(in.m_ATM.m_w_ascent); break;
			case W_ASCENT_SD:		str = WBSF::ToString(in.m_ATM.m_w_ascent_σ); break;
			case W_HORZ:			str = WBSF::ToString(in.m_ATM.m_w_horizontal); break;
			case W_HORZ_SD:			str = WBSF::ToString(in.m_ATM.m_w_horizontal_σ); break;
			case W_DESCENT:			str = WBSF::ToString(in.m_ATM.m_w_descent); break;
			case W_DESCENT_SD:		str = WBSF::ToString(in.m_ATM.m_w_descent_σ); break;

			default: ASSERT(false);
			}

			CStdGridProperty* pProp = static_cast<CStdGridProperty*>(FindItemByData(i));
			pProp->set_string(str);

			ASSERT(pProp);

		}
	}
*/
	void CSearchRadiusPropertyGridCtrl::OnPropertyChanged(CMFCPropertyGridProperty* pPropIn) const
	{
		CStdGridProperty* pProp = static_cast<CStdGridProperty*>(pPropIn);
		CSearchRadiusPropertyGridCtrl& me = const_cast<CSearchRadiusPropertyGridCtrl&>(*this);
		std::string str = pProp->get_string();


		size_t v = (size_t)pProp->GetData();
		ASSERT(v < NB_VAR_H);
		
		double radius = ToDouble(str); //in km
		me.m_searchRadius[v] = radius>=0 ? radius * 1000:-1;//in meters
	}



	void CSearchRadiusPropertyGridCtrl::EnableProperties(BOOL bEnable)
	{
		for (int i = 0; i < GetPropertyCount(); i++)
		{
			CMFCPropertyGridProperty* pProp = GetProperty(i);
			EnableProperties(pProp, bEnable);
		}
	}

	void CSearchRadiusPropertyGridCtrl::EnableProperties(CMFCPropertyGridProperty* pProp, BOOL bEnable)
	{
		pProp->Enable(bEnable);

		for (int ii = 0; ii < pProp->GetSubItemsCount(); ii++)
			EnableProperties(pProp->GetSubItem(ii), bEnable);
	}


	BOOL CSearchRadiusPropertyGridCtrl::PreTranslateMessage(MSG* pMsg)
	{

		if (pMsg->message == WM_KEYDOWN)
		{
			BOOL bAlt = GetKeyState(VK_CONTROL) & 0x8000;
			if (pMsg->wParam == VK_RETURN)
			{
				if (m_pSel != NULL && m_pSel->IsInPlaceEditing() != NULL && m_pSel->IsEnabled())
				{
					if (m_pSel->GetOptionCount()>0)
						OnSelectCombo();

					EndEditItem();

					//select next item
					size_t ID = m_pSel->GetData();
					size_t newID = (ID + 1) % GetProperty(0)->GetSubItemsCount();
					CMFCPropertyGridProperty* pNext = FindItemByData(newID);
					if (pNext)
					{
						SetCurSel(pNext);
						EditItem(pNext);
					}

					return TRUE; // this doesn't need processing anymore
				}

			}
			else if (pMsg->wParam == VK_DOWN && bAlt)
			{
				m_pSel->OnClickButton(CPoint(-1, -1));
				return TRUE; // this doesn't need processing anymore
			}
		}

		return CMFCPropertyGridCtrl::PreTranslateMessage(pMsg); // all other cases still need default processing;
	}

	BOOL CSearchRadiusPropertyGridCtrl::ValidateItemData(CMFCPropertyGridProperty* /*pProp*/)
	{
		return TRUE;
	}

	//afx_msg void OnHeaderEndTrack(NMHDR* pNMHDR, LRESULT* pResult);

	//**************************************************************************************************************

	//**********************************************************************************************
	void CSearchRadiusDlg::DoDataExchange(CDataExchange* pDX)
	{
		CDialog::DoDataExchange(pDX);

		DDX_Control(pDX, IDC_VARIABLES_RADIUS, m_ctrl);

		if (pDX->m_bSaveAndValidate)
		{
		}
		else
		{
			//m_ctrl.Init();

			//resize window
			CRect rectClient;
			GetWindowRect(rectClient);

			CAppOption option;
			rectClient = option.GetProfileRect(_T("SearchRadiusDlgRect"), rectClient);
			UtilWin::EnsureRectangleOnDisplay(rectClient);
			SetWindowPos(NULL, rectClient.left, rectClient.top, rectClient.Width(), rectClient.Height(), SWP_NOACTIVATE | SWP_NOZORDER);
		}



	}

	
	BEGIN_MESSAGE_MAP(CSearchRadiusDlg, CDialog)
		ON_WM_SIZE()
		ON_WM_DESTROY()
	END_MESSAGE_MAP()



	CSearchRadiusDlg::CSearchRadiusDlg(CWnd* pParentDlg) :
		CDialog(IDD, pParentDlg),
		m_ctrl(m_searchRadius)
	{
	}


	CSearchRadiusDlg::~CSearchRadiusDlg()
	{
	}

	void CSearchRadiusDlg::OnSize(UINT nType, int cx, int cy)
	{
		CDialog::OnSize(nType, cx, cy);
		AdjustLayout();
	}

	void CSearchRadiusDlg::AdjustLayout()
	{
		static const int MARGE = 10;
		if (GetSafeHwnd() == NULL || m_ctrl.GetSafeHwnd() == NULL)
		{
			return;
		}

		CRect rectClient;
		GetClientRect(rectClient);

		CRect rectCancel;
		GetDlgItem(IDCANCEL)->GetWindowRect(rectCancel); ScreenToClient(rectCancel);
		rectCancel.left = rectClient.right - MARGE - rectCancel.Width();
		rectCancel.top = rectClient.bottom - MARGE - rectCancel.Height();

		CRect rectOK;
		GetDlgItem(IDOK)->GetWindowRect(rectOK); ScreenToClient(rectOK);
		rectOK.left = rectClient.right - 2 * MARGE - rectCancel.Width() - rectOK.Width();
		rectOK.top = rectClient.bottom - MARGE - rectOK.Height();

		CRect rect;
		m_ctrl.GetWindowRect(rect); ScreenToClient(rect);
		rect.right = rectClient.right - MARGE;
		rect.bottom = rectClient.bottom - MARGE - rectOK.Height() - MARGE;


		m_ctrl.SetWindowPos(NULL, 0, 0, rect.Width(), rect.Height(), SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE);
		GetDlgItem(IDOK)->SetWindowPos(NULL, rectOK.left, rectOK.top, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE);
		GetDlgItem(IDCANCEL)->SetWindowPos(NULL, rectCancel.left, rectCancel.top, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE);
	}


	void CSearchRadiusDlg::OnDestroy()
	{
		CRect rectClient;
		GetWindowRect(rectClient);

		CAppOption option;
		option.WriteProfileRect(_T("SearchRadiusDlgRect"), rectClient);

		CDialog::OnDestroy();
	}

}