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
#include "FileManager/FileManager.h"
#include "UI/Common/UtilWin.h"
#include "UI/Common/FileNameProperty.h"
#include "UI/Common/AppOption.h"
#include "UI/Common/CommonCtrl.h"

#include "DispersalDlg.h"

#include "WeatherBasedSimulationString.h"

using namespace UtilWin;
using namespace std;

namespace WBSF
{


	typedef CStdIndexProperty < IDS_SIM_DISPERSAL_WT> CWeatherTypeProperty;
	typedef CStdIndexProperty < IDS_SIM_SEED_TYPE> CSeedTypeProperty;
	typedef CStdIndexProperty < IDS_SIM_LIFTOFF_TYPE> CLiftoffTypeProperty;
	typedef CStdIndexProperty < IDS_SIM_DURATION_TYPE> CDurationTypeProperty;
	typedef CStdIndexProperty < IDS_SIM_HEIGHT_TYPE> CHeightTypeProperty;
	typedef CStdIndexProperty < IDS_SIM_BROOD_TYPE> CBroodTypeProperty;
	typedef CStdIndexProperty < IDS_SIM_PRCP_TYPE> CPrcpTypeProperty;

	
	//*****************************************************************************************************
	
	enum TDispersalProperties
	{
		WEATHER_TYPE, SIMULATION_PERIOD, TIME_STEP, SEED_TYPE, REVERSED, USE_SPATIAL_INTERPOL, USE_TIME_INTERPOL, USE_PREDICTOR_CORRECTOR_METHOD, ADD_TURBULENCE, MAXIMUM_FLYERS, MAXIMUM_FLIGHTS,
		DEM, GRIBS, HOURLY_DB, HOST, DEFOLIATION, WATER,
		T_BROOD, P_TYPE, P_MAX, W_MIN, 
		HEIGHT_TYPE, WING_BEAT_SCALE, W_HORZ, W_HORZ_SD, W_DESCENT, W_DESCENT_SD, 
		OUTPUT_SUB_HOURLY, OUTPUT_FILE_TITLE, OUTPUT_TIME_FREQUENCY, CREATE_EGG_MAPS, EGG_MAP_TITLE, CREATE_EGG_RES, NB_PROPERTIES
	};

	BEGIN_MESSAGE_MAP(CDispersalPropertyGridCtrl, CMFCPropertyGridCtrl)
		//ON_WM_CREATE()
	END_MESSAGE_MAP()

	CDispersalPropertyGridCtrl::CDispersalPropertyGridCtrl()
	{
	}

	void CDispersalPropertyGridCtrl::Init()
	{
		CMFCPropertyGridCtrl::Init();

		RemoveAll();

		static CString String;
		static COLORREF Color;

		CRect rect;
		GetClientRect(rect);

		//add all attribute
		CAppOption options;
		CStringArrayEx propertyHeader(UtilWin::GetCString(IDS_STR_PROPERTY_HEADER));
		m_nLeftColumnWidth = options.GetProfileInt(_T("DispersalPropertiesSplitterPos"), rect.Width() / 2);
		SetBoolLabels(UtilWin::GetCString(IDS_STR_TRUE), UtilWin::GetCString(IDS_STR_FALSE));

		EnableHeaderCtrl(true, propertyHeader[0], propertyHeader[1]);
		EnableDescriptionArea(true);
		SetVSDotNetLook(true);
		MarkModifiedProperties(true);
		SetAlphabeticMode(false);
		SetShowDragContext(true);
		EnableWindow(true);
		AlwaysShowUserToolTip();

		//General
		CStringArrayEx section(IDS_SIM_DISPERSAL_SECTION);
		WBSF::StringVector name(IDS_SIM_DISPERSAL_PROPERTIES, "|");
		WBSF::StringVector description(IDS_SIM_DISPERSAL_DESCRIPTION, "|");
		ASSERT(name.size() == NB_PROPERTIES);
		ASSERT(description.size() == NB_PROPERTIES);

		CMFCPropertyGridProperty* pGeneral = new CMFCPropertyGridProperty(section[0], -1);
		pGeneral->AddSubItem(new CWeatherTypeProperty(name[WEATHER_TYPE], 0, description[WEATHER_TYPE], WEATHER_TYPE));
		pGeneral->AddSubItem(new CStdTPeriodProperty(name[SIMULATION_PERIOD], "", description[SIMULATION_PERIOD], SIMULATION_PERIOD));
		pGeneral->AddSubItem(new CStdGridProperty(name[TIME_STEP], "", description[TIME_STEP], TIME_STEP));
		pGeneral->AddSubItem(new CSeedTypeProperty(name[SEED_TYPE], 0, description[SEED_TYPE], SEED_TYPE));
		pGeneral->AddSubItem(new CStdBoolGridProperty(name[REVERSED], true, description[REVERSED], REVERSED));
		pGeneral->AddSubItem(new CStdBoolGridProperty(name[USE_SPATIAL_INTERPOL], true, description[USE_SPATIAL_INTERPOL], USE_SPATIAL_INTERPOL));
		pGeneral->AddSubItem(new CStdBoolGridProperty(name[USE_TIME_INTERPOL], true, description[USE_TIME_INTERPOL], USE_TIME_INTERPOL));
		pGeneral->AddSubItem(new CStdBoolGridProperty(name[USE_PREDICTOR_CORRECTOR_METHOD], true, description[USE_PREDICTOR_CORRECTOR_METHOD], USE_PREDICTOR_CORRECTOR_METHOD));
		pGeneral->AddSubItem(new CStdBoolGridProperty(name[ADD_TURBULENCE], true, description[ADD_TURBULENCE], ADD_TURBULENCE));
		pGeneral->AddSubItem(new CStdGridProperty(name[MAXIMUM_FLYERS], "", description[MAXIMUM_FLYERS], MAXIMUM_FLYERS));
		pGeneral->AddSubItem(new CStdGridProperty(name[MAXIMUM_FLIGHTS], false, description[MAXIMUM_FLIGHTS], MAXIMUM_FLIGHTS));
		
		AddProperty(pGeneral);

		CMFCPropertyGridProperty* pInput = new CMFCPropertyGridProperty(section[1], -1);
		pInput->AddSubItem(new CFileNameProperty(name[DEM], "", WBSF::GetFM().MapInput(), description[DEM], DEM));
		pInput->AddSubItem(new CFileNameProperty(name[GRIBS], "", WBSF::GetFM().Gribs(), description[GRIBS], GRIBS));
		pInput->AddSubItem(new CFileNameProperty(name[HOURLY_DB], "", WBSF::GetFM().Hourly(), description[HOURLY_DB], HOURLY_DB));
		pInput->AddSubItem(new CFileNameProperty(name[HOST], "", WBSF::GetFM().MapInput(), description[HOST], HOST));
		pInput->AddSubItem(new CFileNameProperty(name[DEFOLIATION], "", WBSF::GetFM().MapInput(), description[DEFOLIATION], DEFOLIATION));
		pInput->AddSubItem(new CFileNameProperty(name[WATER], "", WBSF::GetFM().MapInput(), description[WATER], WATER));
		AddProperty(pInput);

		CMFCPropertyGridProperty* pWeather = new CMFCPropertyGridProperty(section[2], -1);
		
		pWeather->AddSubItem(new CBroodTypeProperty(name[T_BROOD], "", description[T_BROOD], T_BROOD));
		pWeather->AddSubItem(new CPrcpTypeProperty(name[P_TYPE], "", description[P_TYPE], P_TYPE));
		pWeather->AddSubItem(new CStdGridProperty(name[P_MAX], "", description[P_MAX], P_MAX));
		pWeather->AddSubItem(new CStdGridProperty(name[W_MIN], "", description[W_MIN], W_MIN));
		AddProperty(pWeather);

		CMFCPropertyGridProperty* pHeight = new CMFCPropertyGridProperty(section[3], -1);
		pHeight->AddSubItem(new CHeightTypeProperty(name[HEIGHT_TYPE], "", description[HEIGHT_TYPE], HEIGHT_TYPE));
		pHeight->AddSubItem(new CStdGridProperty(name[WING_BEAT_SCALE], "", description[WING_BEAT_SCALE], WING_BEAT_SCALE));

		AddProperty(pHeight);

		CMFCPropertyGridProperty* pVelocity = new CMFCPropertyGridProperty(section[4], -1);
		pVelocity->AddSubItem(new CStdGridProperty(name[W_HORZ], "", description[W_HORZ], W_HORZ));
		pVelocity->AddSubItem(new CStdGridProperty(name[W_HORZ_SD], "", description[W_HORZ_SD], W_HORZ_SD));
		pVelocity->AddSubItem(new CStdGridProperty(name[W_DESCENT], "", description[W_DESCENT], W_DESCENT));
		pVelocity->AddSubItem(new CStdGridProperty(name[W_DESCENT_SD], "", description[W_DESCENT_SD], W_DESCENT_SD));
		AddProperty(pVelocity);

		CMFCPropertyGridProperty* pOutput = new CMFCPropertyGridProperty(section[5], -1);
		pOutput->AddSubItem(new CStdBoolGridProperty(name[OUTPUT_SUB_HOURLY], false, description[OUTPUT_SUB_HOURLY], OUTPUT_SUB_HOURLY));
		pOutput->AddSubItem(new CStdGridProperty(name[OUTPUT_FILE_TITLE], "", description[OUTPUT_FILE_TITLE], OUTPUT_FILE_TITLE));
		pOutput->AddSubItem(new CStdGridProperty(name[OUTPUT_TIME_FREQUENCY], 600, description[OUTPUT_TIME_FREQUENCY], OUTPUT_TIME_FREQUENCY));
		AddProperty(pOutput);

		CMFCPropertyGridProperty* pEggs = new CMFCPropertyGridProperty(section[6], -1);
		pEggs->AddSubItem(new CStdBoolGridProperty(name[CREATE_EGG_MAPS], false, description[CREATE_EGG_MAPS], CREATE_EGG_MAPS));
		pEggs->AddSubItem(new CStdGridProperty(name[EGG_MAP_TITLE], "", description[EGG_MAP_TITLE], EGG_MAP_TITLE));
		pEggs->AddSubItem(new CStdGridProperty(name[CREATE_EGG_RES], "", description[EGG_MAP_TITLE], CREATE_EGG_RES));
		
		
		AddProperty(pEggs);

		

	}

	void CDispersalPropertyGridCtrl::Set(const CDispersalParamters& in)
	{
		m_parameters = in;

		
		for (int i = 0; i < NB_PROPERTIES; i++)
		{
			string str;
			switch (i)
			{
			case WEATHER_TYPE:		str = WBSF::ToString(in.m_world.m_weather_type); break;
			case SIMULATION_PERIOD: str = in.m_world.m_simulationPeriod.GetFormatedString("%1, %2", "%Y-%m-%d"); break;
			case TIME_STEP:			str = WBSF::ToString(in.m_world.m_time_step); break;
			case SEED_TYPE:			str = WBSF::ToString(in.m_world.m_seed); break;
			case REVERSED:			str = WBSF::ToString(in.m_world.m_bReversed); break;
			case USE_SPATIAL_INTERPOL:		str = WBSF::ToString(in.m_world.m_bUseSpaceInterpolation); break;
			case USE_TIME_INTERPOL:		str = WBSF::ToString(in.m_world.m_bUseTimeInterpolation); break;
			case USE_PREDICTOR_CORRECTOR_METHOD: str = WBSF::ToString(in.m_world.m_bUsePredictorCorrectorMethod); break;
			case ADD_TURBULENCE:		str = WBSF::ToString(in.m_world.m_bUseTurbulance); break;
			case MAXIMUM_FLYERS:	str = WBSF::ToString(in.m_world.m_maxFliyers); break;
			case MAXIMUM_FLIGHTS:		str = WBSF::ToString(in.m_world.m_maxFlights); break;
			case DEM:				str = in.m_world.m_DEM_name; break;
			case HOURLY_DB:			str = in.m_world.m_hourly_DB_name; break;
			case GRIBS:				str = in.m_world.m_gribs_name; break;
			case HOST:				str = in.m_world.m_host_name; break;
			case DEFOLIATION:		str = in.m_world.m_defoliation_name; break;
			case WATER:				str = in.m_world.m_water_name; break;
				
			case T_BROOD:			str = WBSF::ToString(in.m_ATM.m_broodTSource); break;
			case P_TYPE:			str = WBSF::ToString(in.m_ATM.m_PSource); break;
			case P_MAX:				str = WBSF::ToString(in.m_ATM.m_Pmax); break;
			case W_MIN:				str = WBSF::ToString(in.m_ATM.m_Wmin); break;
			case HEIGHT_TYPE:		str = WBSF::ToString(in.m_ATM.m_height_type); break;
			case WING_BEAT_SCALE:	str = WBSF::ToString(in.m_ATM.m_w_α); break;
			case W_HORZ:			str = WBSF::ToString(in.m_ATM.m_w_horizontal); break;
			case W_HORZ_SD:			str = WBSF::ToString(in.m_ATM.m_w_horizontal_σ); break;
			case W_DESCENT:			str = WBSF::ToString(in.m_ATM.m_w_descent); break;
			case W_DESCENT_SD:		str = WBSF::ToString(in.m_ATM.m_w_descent_σ); break;
			case OUTPUT_SUB_HOURLY: str = WBSF::ToString(in.m_world.m_bOutputSubHourly); break;
			case OUTPUT_FILE_TITLE:	str = in.m_world.m_outputFileTitle; break;
			case OUTPUT_TIME_FREQUENCY:	str = WBSF::ToString(in.m_world.m_outputFrequency); break;
			case CREATE_EGG_MAPS:	str = WBSF::ToString(in.m_world.m_bCreateEggMaps); break;
			case EGG_MAP_TITLE:		str = in.m_world.m_eggMapsTitle; break;
			case CREATE_EGG_RES:	str = WBSF::ToString(in.m_world.m_eggMapsResolution); break;
			default: ASSERT(false);
			}

			CStdGridProperty* pProp = static_cast<CStdGridProperty*>(FindItemByData(i));
			pProp->set_string(str);

			ASSERT(pProp);
		}
	}

	void CDispersalPropertyGridCtrl::OnPropertyChanged(CMFCPropertyGridProperty* pPropIn) const
	{


		CStdGridProperty* pProp = static_cast<CStdGridProperty*>(pPropIn);
		CDispersalPropertyGridCtrl& me = const_cast<CDispersalPropertyGridCtrl&>(*this);
		std::string str = pProp->get_string();


		int i = (int)pProp->GetData();
		switch (i)
		{
		case WEATHER_TYPE:		me.m_parameters.m_world.m_weather_type = WBSF::ToInt(str); break;
		case SIMULATION_PERIOD: me.m_parameters.m_world.m_simulationPeriod.FromFormatedString(str, "%1, %2", "%Y-%m-%d"); break;
		case 1000 * SIMULATION_PERIOD + 1: me.m_parameters.m_world.m_simulationPeriod.Begin().FromFormatedString(str, "%Y-%m-%d"); break;
		case 1000 * SIMULATION_PERIOD + 2: me.m_parameters.m_world.m_simulationPeriod.End().FromFormatedString(str, "%Y-%m-%d"); break;
		case TIME_STEP:			me.m_parameters.m_world.m_time_step = WBSF::ToInt(str); break;
		case SEED_TYPE:			me.m_parameters.m_world.m_seed = WBSF::ToInt(str); break;
		case REVERSED:			me.m_parameters.m_world.m_bReversed = WBSF::ToBool(str); break;
		case USE_SPATIAL_INTERPOL:	me.m_parameters.m_world.m_bUseSpaceInterpolation = WBSF::ToBool(str); break;
		case USE_TIME_INTERPOL:	me.m_parameters.m_world.m_bUseTimeInterpolation = WBSF::ToBool(str); break;
		case USE_PREDICTOR_CORRECTOR_METHOD: me.m_parameters.m_world.m_bUsePredictorCorrectorMethod = WBSF::ToBool(str); break;
		case ADD_TURBULENCE:	me.m_parameters.m_world.m_bUseTurbulance = WBSF::ToBool(str); break;
		case MAXIMUM_FLYERS:	me.m_parameters.m_world.m_maxFliyers = WBSF::ToSizeT(str); break;
		case MAXIMUM_FLIGHTS:	me.m_parameters.m_world.m_maxFlights = WBSF::ToSizeT(str); break;
		//case DEFOLIATION_THRESHOLD:	me.m_parameters.m_world.m_defoliationThreshold = WBSF::ToDouble(str); break;
		//case DISTRACTION_THRESHOLD:	me.m_parameters.m_world.m_distractionThreshold = WBSF::ToDouble(str); break;
		//case HOST_THRESHOLD:	me.m_parameters.m_world.m_hostThreshold = WBSF::ToDouble(str); break;
		case DEM:				me.m_parameters.m_world.m_DEM_name = str; break;
		case GRIBS:				me.m_parameters.m_world.m_gribs_name = str; break;
		case HOURLY_DB:			me.m_parameters.m_world.m_hourly_DB_name = str; break;
		
		//case DISTRACTION:		me.m_parameters.m_world.m_distraction_name = str; break;
		case HOST:				me.m_parameters.m_world.m_host_name = str; break;
		case DEFOLIATION:		me.m_parameters.m_world.m_defoliation_name = str; break;
		case WATER:				me.m_parameters.m_world.m_water_name = str; break;
		case T_BROOD:			me.m_parameters.m_ATM.m_broodTSource = WBSF::ToSizeT(str); break;
		case P_TYPE:			me.m_parameters.m_ATM.m_PSource = WBSF::ToSizeT(str); break;
		case P_MAX:				me.m_parameters.m_ATM.m_Pmax = WBSF::ToDouble(str); break;
		case W_MIN:				me.m_parameters.m_ATM.m_Wmin = WBSF::ToDouble(str); break;
		case HEIGHT_TYPE:		me.m_parameters.m_ATM.m_height_type = WBSF::ToInt(str); break;
		case WING_BEAT_SCALE:	me.m_parameters.m_ATM.m_w_α = WBSF::ToDouble(str); break;
		case W_HORZ:			me.m_parameters.m_ATM.m_w_horizontal = WBSF::ToDouble(str); break;
		case W_HORZ_SD:			me.m_parameters.m_ATM.m_w_horizontal_σ = WBSF::ToDouble(str); break;
		case W_DESCENT:			me.m_parameters.m_ATM.m_w_descent = WBSF::ToDouble(str); break;
		case W_DESCENT_SD:		me.m_parameters.m_ATM.m_w_descent_σ = WBSF::ToDouble(str); break;
		case OUTPUT_SUB_HOURLY: me.m_parameters.m_world.m_bOutputSubHourly = WBSF::ToBool(str); break;
		case OUTPUT_FILE_TITLE:	me.m_parameters.m_world.m_outputFileTitle = str; break;
		case OUTPUT_TIME_FREQUENCY:	me.m_parameters.m_world.m_outputFrequency = WBSF::ToSizeT(str); break;
		case CREATE_EGG_MAPS:	me.m_parameters.m_world.m_bCreateEggMaps = WBSF::ToBool(str); break;
		case EGG_MAP_TITLE:		me.m_parameters.m_world.m_eggMapsTitle = str; break;
		case CREATE_EGG_RES:	me.m_parameters.m_world.m_eggMapsResolution = WBSF::ToDouble(str); break;

		default: ASSERT(false);
		}
	}



	void CDispersalPropertyGridCtrl::EnableProperties(BOOL bEnable)
	{
		for (int i = 0; i < GetPropertyCount(); i++)
		{
			CMFCPropertyGridProperty* pProp = GetProperty(i);
			EnableProperties(pProp, bEnable);
		}
	}

	void CDispersalPropertyGridCtrl::EnableProperties(CMFCPropertyGridProperty* pProp, BOOL bEnable)
	{
		pProp->Enable(bEnable);

		for (int ii = 0; ii < pProp->GetSubItemsCount(); ii++)
			EnableProperties(pProp->GetSubItem(ii), bEnable);
	}


	BOOL CDispersalPropertyGridCtrl::PreTranslateMessage(MSG* pMsg)
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

	BOOL CDispersalPropertyGridCtrl::ValidateItemData(CMFCPropertyGridProperty* /*pProp*/)
	{
		return TRUE;
	}

	//afx_msg void OnHeaderEndTrack(NMHDR* pNMHDR, LRESULT* pResult);

	//**************************************************************************************************************

	//**********************************************************************************************
	void CDispersalDlg::DoDataExchange(CDataExchange* pDX)
	{
		CDialog::DoDataExchange(pDX);



		DDX_Control(pDX, IDC_NAME, m_nameCtrl);
		DDX_Control(pDX, IDC_DESCRIPTION, m_descriptionCtrl);
		DDX_Control(pDX, IDC_SIM_PROPERTIES, m_propertiesCtrl);
		DDX_Control(pDX, IDC_INTERNAL_NAME, m_internalNameCtrl);


		if (pDX->m_bSaveAndValidate)
		{
			m_dispersal.m_name = m_nameCtrl.GetString();
			m_dispersal.m_description = m_descriptionCtrl.GetString();
			m_dispersal.m_parameters = m_propertiesCtrl.Get();
		}
		else
		{
			m_nameCtrl.SetString(m_dispersal.m_name);
			m_descriptionCtrl.SetString(m_dispersal.m_description);
			m_propertiesCtrl.Set(m_dispersal.m_parameters);
			m_internalNameCtrl.SetString(m_dispersal.m_internalName);

			//resize window
			CRect rectClient;
			GetWindowRect(rectClient);

			CAppOption option;
			rectClient = option.GetProfileRect(_T("DispercalDlgRect"), rectClient);
			UtilWin::EnsureRectangleOnDisplay(rectClient);
			SetWindowPos(NULL, rectClient.left, rectClient.top, rectClient.Width(), rectClient.Height(), SWP_NOACTIVATE | SWP_NOZORDER);
		}



	}

	// CDispersalDlg dialog

	//IMPLEMENT_DYNAMIC(CDispersalDlg, CDialog)

	BEGIN_MESSAGE_MAP(CDispersalDlg, CDialog)
		ON_WM_SIZE()
		ON_WM_DESTROY()
	END_MESSAGE_MAP()



	CDispersalDlg::CDispersalDlg(CExecutablePtr pParent, CWnd* pParentDlg) :
		CDialog(IDD, pParentDlg)
	{
	}


	CDispersalDlg::~CDispersalDlg()
	{
	}

	void CDispersalDlg::OnSize(UINT nType, int cx, int cy)
	{
		CDialog::OnSize(nType, cx, cy);

		AdjustLayout();
	}

	void CDispersalDlg::AdjustLayout()
	{
		static const int MARGE = 10;
		if (GetSafeHwnd() == NULL || m_propertiesCtrl.GetSafeHwnd() == NULL)
		{
			return;
		}

		CRect rectClient;
		GetClientRect(rectClient);

		CRect rectOK;
		GetDlgItem(IDOK)->GetWindowRect(rectOK); ScreenToClient(rectOK);

		rectOK.MoveToX(rectClient.right - 2 * MARGE - 2 * rectOK.Width());
		rectOK.MoveToY(rectClient.bottom - MARGE - rectOK.Height());

		CRect rectCancel;
		GetDlgItem(IDCANCEL)->GetWindowRect(rectCancel); ScreenToClient(rectCancel);
		rectCancel.left = rectClient.right - MARGE - rectCancel.Width();
		rectCancel.top = rectClient.bottom - MARGE - rectCancel.Height();

		CRect rectName;
		m_nameCtrl.GetWindowRect(rectName); ScreenToClient(rectName);
		rectName.right = rectClient.right - MARGE;

		CRect rectDescription;
		m_descriptionCtrl.GetWindowRect(rectDescription); ScreenToClient(rectDescription);
		rectDescription.right = rectClient.right - MARGE;

		CRect rectStatic;
		GetDlgItem(IDC_CMN_STATIC1)->GetWindowRect(rectStatic); ScreenToClient(rectStatic);
		rectStatic.MoveToY(rectClient.bottom - rectStatic.Height() - MARGE);
		//rectStatic.bottom = rectClient.bottom - MARGE;

		CRect rectInternalName;
		m_internalNameCtrl.GetWindowRect(rectInternalName); ScreenToClient(rectInternalName);
		rectInternalName.top = rectClient.bottom - rectDescription.Height() - MARGE;
		rectInternalName.bottom = rectClient.bottom - MARGE;

		CRect rect;
		m_propertiesCtrl.GetWindowRect(rect); ScreenToClient(rect);
		rect.right = rectClient.right - MARGE;
		rect.bottom = rectClient.bottom - MARGE - rectOK.Height() - MARGE;


		m_nameCtrl.SetWindowPos(NULL, 0, 0, rectName.Width(), rectName.Height(), SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE);
		m_descriptionCtrl.SetWindowPos(NULL, 0, 0, rectDescription.Width(), rectDescription.Height(), SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE);
		m_propertiesCtrl.SetWindowPos(NULL, 0, 0, rect.Width(), rect.Height(), SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE);
		GetDlgItem(IDC_CMN_STATIC1)->SetWindowPos(NULL, rectStatic.left, rectStatic.top, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE);
		m_internalNameCtrl.SetWindowPos(NULL, rectInternalName.left, rectInternalName.top, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE);
		GetDlgItem(IDOK)->SetWindowPos(NULL, rectOK.left, rectOK.top, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE);
		GetDlgItem(IDCANCEL)->SetWindowPos(NULL, rectCancel.left, rectCancel.top, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE);
	}


	void CDispersalDlg::OnDestroy()
	{
		CRect rectClient;
		GetWindowRect(rectClient);

		CAppOption option;
		option.WriteProfileRect(_T("DispercalDlgRect"), rectClient);

		CDialog::OnDestroy();
	}

}