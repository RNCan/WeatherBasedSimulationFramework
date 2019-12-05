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
	typedef CStdIndexProperty < IDS_SIM_BROOD_TYPE> CBroodTypeProperty;
	typedef CStdIndexProperty < IDS_SIM_PRCP_TYPE> CPrcpTypeProperty;

	
	//*****************************************************************************************************
	
	enum TDispersalProperties
	{
		WEATHER_TYPE, SIMULATION_PERIOD, TIME_STEP, SEED_TYPE, USE_SPATIAL_INTERPOL, USE_TIME_INTERPOL, USE_PREDICTOR_CORRECTOR_METHOD, MAXIMUM_FLYERS, MAX_MISS_WEATHER, FORCE_FIRST_FLIGHT, T_BROOD, P_TYPE,
		DEM, GRIBS, HOURLY_DB, DEFOLIATION, WATER,
		P_MAX, W_MIN, WING_BEAT_SCALE, REDUCTION_FACTOR, REDUCTION_HEIGHT, W_HORZ, W_HORZ_SD, W_DESCENT, W_DESCENT_SD, MAXIMUM_FLIGHTS, FLIGHT_AFTER_SUNRIZE, APPPLY_ATTRITION, MAX_ADULT_LONGEVITY, READY_SHIFT0, READY_SHIFT1,
		OUTPUT_SUB_HOURLY, OUTPUT_FILE_TITLE, CREATE_EGG_MAPS, EGG_MAP_TITLE, CREATE_EGG_RES, NB_PROPERTIES
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
		WBSF::StringVector sex(IDS_STR_SEX, "|");
		ASSERT(name.size() == NB_PROPERTIES);
		ASSERT(description.size() == NB_PROPERTIES);
		ASSERT(sex.size() == 2);
		 
		CMFCPropertyGridProperty* pGeneral = new CMFCPropertyGridProperty(section[0], -1);
		pGeneral->AddSubItem(new CWeatherTypeProperty(name[WEATHER_TYPE], 0, description[WEATHER_TYPE], WEATHER_TYPE));
		pGeneral->AddSubItem(new CStdTPeriodProperty(name[SIMULATION_PERIOD], "", description[SIMULATION_PERIOD], SIMULATION_PERIOD));
		//pGeneral->AddSubItem(new CStdTPeriodProperty(name[FLIGHT_PERIOD], "", description[FLIGHT_PERIOD], FLIGHT_PERIOD));
		//pGeneral->AddSubItem(new CStdGridProperty(name[EXTRA_SIMULATION], "", description[EXTRA_SIMULATION], EXTRA_SIMULATION));
		pGeneral->AddSubItem(new CStdGridProperty(name[TIME_STEP], "", description[TIME_STEP], TIME_STEP));
		pGeneral->AddSubItem(new CSeedTypeProperty(name[SEED_TYPE], 0, description[SEED_TYPE], SEED_TYPE));
		pGeneral->AddSubItem(new CStdBoolGridProperty(name[USE_SPATIAL_INTERPOL], true, description[USE_SPATIAL_INTERPOL], USE_SPATIAL_INTERPOL));
		pGeneral->AddSubItem(new CStdBoolGridProperty(name[USE_TIME_INTERPOL], true, description[USE_TIME_INTERPOL], USE_TIME_INTERPOL));
		pGeneral->AddSubItem(new CStdBoolGridProperty(name[USE_PREDICTOR_CORRECTOR_METHOD], true, description[USE_PREDICTOR_CORRECTOR_METHOD], USE_PREDICTOR_CORRECTOR_METHOD));
		pGeneral->AddSubItem(new CStdGridProperty(name[MAXIMUM_FLYERS], "", description[MAXIMUM_FLYERS], MAXIMUM_FLYERS));
		pGeneral->AddSubItem(new CStdGridProperty(name[MAX_MISS_WEATHER], "", description[MAX_MISS_WEATHER], MAX_MISS_WEATHER));
		pGeneral->AddSubItem(new CStdBoolGridProperty(name[FORCE_FIRST_FLIGHT], "", description[FORCE_FIRST_FLIGHT], FORCE_FIRST_FLIGHT));
		pGeneral->AddSubItem(new CBroodTypeProperty(name[T_BROOD], "", description[T_BROOD], T_BROOD));
		pGeneral->AddSubItem(new CPrcpTypeProperty(name[P_TYPE], "", description[P_TYPE], P_TYPE));
		AddProperty(pGeneral, FALSE, FALSE);

		CMFCPropertyGridProperty* pInput = new CMFCPropertyGridProperty(section[1], -1);
		pInput->AddSubItem(new CFileNameProperty(name[DEM], "", WBSF::GetFM().MapInput(), description[DEM], DEM));
		pInput->AddSubItem(new CFileNameProperty(name[GRIBS], "", WBSF::GetFM().Gribs(), description[GRIBS], GRIBS));
		pInput->AddSubItem(new CFileNameProperty(name[HOURLY_DB], "", WBSF::GetFM().Hourly(), description[HOURLY_DB], HOURLY_DB));
		pInput->AddSubItem(new CFileNameProperty(name[DEFOLIATION], "", WBSF::GetFM().MapInput(), description[DEFOLIATION], DEFOLIATION));
		pInput->AddSubItem(new CFileNameProperty(name[WATER], "", WBSF::GetFM().MapInput(), description[WATER], WATER));
		AddProperty(pInput, FALSE, FALSE);


		CMFCPropertyGridProperty* pMoth = new CMFCPropertyGridProperty(section[2], -1);
		pMoth->AddSubItem(new CStdGridProperty(name[P_MAX], "", description[P_MAX], P_MAX));
		pMoth->AddSubItem(new CStdGridProperty(name[W_MIN], "", description[W_MIN], W_MIN));
		pMoth->AddSubItem(new CStdGridProperty(name[WING_BEAT_SCALE], "", description[WING_BEAT_SCALE], WING_BEAT_SCALE));
		pMoth->AddSubItem(new CStdGridProperty(name[REDUCTION_FACTOR], "", description[REDUCTION_FACTOR], REDUCTION_FACTOR));
		pMoth->AddSubItem(new CStdGridProperty(name[REDUCTION_HEIGHT], "", description[REDUCTION_HEIGHT], REDUCTION_HEIGHT));
		pMoth->AddSubItem(new CStdGridProperty(name[W_HORZ], "", description[W_HORZ], W_HORZ));
		pMoth->AddSubItem(new CStdGridProperty(name[W_HORZ_SD], "", description[W_HORZ_SD], W_HORZ_SD));
		pMoth->AddSubItem(new CStdGridProperty(name[W_DESCENT], "", description[W_DESCENT], W_DESCENT));
		pMoth->AddSubItem(new CStdGridProperty(name[W_DESCENT_SD], "", description[W_DESCENT_SD], W_DESCENT_SD));
		pMoth->AddSubItem(new CStdGridProperty(name[MAXIMUM_FLIGHTS], "", description[MAXIMUM_FLIGHTS], MAXIMUM_FLIGHTS));
		pMoth->AddSubItem(new CStdGridProperty(name[FLIGHT_AFTER_SUNRIZE], "", description[FLIGHT_AFTER_SUNRIZE], FLIGHT_AFTER_SUNRIZE));
		pMoth->AddSubItem(new CStdBoolGridProperty(name[APPPLY_ATTRITION], true, description[APPPLY_ATTRITION], APPPLY_ATTRITION));
		pMoth->AddSubItem(new CStdGridProperty(name[MAX_ADULT_LONGEVITY], "", description[MAX_ADULT_LONGEVITY], MAX_ADULT_LONGEVITY));

		CMFCPropertyGridProperty* pShift = new CMFCPropertyGridProperty( CString(name[READY_SHIFT0].c_str()), -1);
		pShift->AddSubItem(new CStdGridProperty(sex[CSBWMothParameters::MALE], "", description[READY_SHIFT0], READY_SHIFT0));
		pShift->AddSubItem(new CStdGridProperty(sex[CSBWMothParameters::FEMALE], "", description[READY_SHIFT1], READY_SHIFT1));
		pMoth->AddSubItem(pShift);
		AddProperty(pMoth);

		CMFCPropertyGridProperty* pOutput = new CMFCPropertyGridProperty(section[3], -1);
		pOutput->AddSubItem(new CStdBoolGridProperty(name[OUTPUT_SUB_HOURLY], false, description[OUTPUT_SUB_HOURLY], OUTPUT_SUB_HOURLY));
		pOutput->AddSubItem(new CStdGridProperty(name[OUTPUT_FILE_TITLE], "", description[OUTPUT_FILE_TITLE], OUTPUT_FILE_TITLE));
		AddProperty(pOutput, FALSE, FALSE);

		CMFCPropertyGridProperty* pEggs = new CMFCPropertyGridProperty(section[4], -1);
		pEggs->AddSubItem(new CStdBoolGridProperty(name[CREATE_EGG_MAPS], false, description[CREATE_EGG_MAPS], CREATE_EGG_MAPS));
		pEggs->AddSubItem(new CStdGridProperty(name[EGG_MAP_TITLE], "", description[EGG_MAP_TITLE], EGG_MAP_TITLE));
		pEggs->AddSubItem(new CStdGridProperty(name[CREATE_EGG_RES], "", description[EGG_MAP_TITLE], CREATE_EGG_RES));
		AddProperty(pEggs, FALSE, FALSE);

		

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
			//case FLIGHT_PERIOD:		str = in.m_world.m_flightPeriod.GetFormatedString("%1, %2", "%Y-%m-%d"); break;
			//case EXTRA_SIMULATION:	str = WBSF::ToString(in.m_world.m_extra_simulation_days); break;
			case TIME_STEP:			str = WBSF::ToString(in.m_world.m_time_step); break;
			case SEED_TYPE:			str = WBSF::ToString(in.m_world.m_seed); break;
			case USE_SPATIAL_INTERPOL:		str = WBSF::ToString(in.m_world.m_bUseSpaceInterpolation); break;
			case USE_TIME_INTERPOL:		str = WBSF::ToString(in.m_world.m_bUseTimeInterpolation); break;
			case USE_PREDICTOR_CORRECTOR_METHOD: str = WBSF::ToString(in.m_world.m_bUsePredictorCorrectorMethod); break;
			case MAXIMUM_FLYERS:	str = WBSF::ToString(in.m_world.m_maxFlyers); break;
			case MAX_MISS_WEATHER:	str = WBSF::ToString(in.m_world.m_max_missing_weather, 2); break;
			case T_BROOD:			str = WBSF::ToString(in.m_world.m_broodTSource); break;
			case P_TYPE:			str = WBSF::ToString(in.m_world.m_PSource); break;
			case FORCE_FIRST_FLIGHT:	str = WBSF::ToString(in.m_world.m_bForceFirstFlight); break;

			case DEM:				str = in.m_world.m_DEM_name; break;
			case HOURLY_DB:			str = in.m_world.m_hourly_DB_name; break;
			case GRIBS:				str = in.m_world.m_gribs_name; break;
			case DEFOLIATION:		str = in.m_world.m_defoliation_name; break;
			case WATER:				str = in.m_world.m_water_name; break;
				
			
			case P_MAX:				str = WBSF::ToString(in.m_moths.m_Pmax); break;
			case W_MIN:				str = WBSF::ToString(in.m_moths.m_Wmin); break;
			case WING_BEAT_SCALE:	str = WBSF::ToString(in.m_moths.m_w_α); break;
			case REDUCTION_FACTOR:		str = WBSF::ToString(in.m_moths.m_Δv); break;
			case REDUCTION_HEIGHT:		str = WBSF::ToString(in.m_moths.m_Hv); break;
			case W_HORZ:			str = WBSF::ToString(in.m_moths.m_w_horizontal); break;
			case W_HORZ_SD:			str = WBSF::ToString(in.m_moths.m_w_horizontal_σ); break;
			case W_DESCENT:			str = WBSF::ToString(in.m_moths.m_w_descent); break;
			case W_DESCENT_SD:		str = WBSF::ToString(in.m_moths.m_w_descent_σ); break;
			case MAXIMUM_FLIGHTS:		str = WBSF::ToString(in.m_moths.m_maxFlights); break;
			case FLIGHT_AFTER_SUNRIZE:	str = WBSF::ToString(in.m_moths.m_flight_after_sunrise, 2); break;
			case APPPLY_ATTRITION:		str = WBSF::ToString(in.m_moths.m_bAppplyAttrition); break;
			case MAX_ADULT_LONGEVITY:	str = WBSF::ToString(in.m_moths.m_max_adult_longevity); break;
			case READY_SHIFT0:		str = WBSF::ToString(in.m_moths.m_ready_to_fly[0]); break;
			case READY_SHIFT1:		str = WBSF::ToString(in.m_moths.m_ready_to_fly[1]); break;


			case OUTPUT_SUB_HOURLY: str = WBSF::ToString(in.m_world.m_bOutputSubHourly); break;
			case OUTPUT_FILE_TITLE:	str = in.m_world.m_outputFileTitle; break;
			//case OUTPUT_TIME_FREQUENCY:	str = WBSF::ToString(in.m_world.m_outputFrequency); break;
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
		//case FLIGHT_PERIOD:		me.m_parameters.m_world.m_flightPeriod.FromFormatedString(str, "%1, %2", "%Y-%m-%d"); break;
		//case 1000 * FLIGHT_PERIOD + 1: me.m_parameters.m_world.m_flightPeriod.Begin().FromFormatedString(str, "%Y-%m-%d"); break;
		//case 1000 * FLIGHT_PERIOD + 2: me.m_parameters.m_world.m_flightPeriod.End().FromFormatedString(str, "%Y-%m-%d"); break;
		//case EXTRA_SIMULATION:	me.m_parameters.m_world.m_extra_simulation_days = WBSF::ToInt(str); break;
		case TIME_STEP:			me.m_parameters.m_world.m_time_step = WBSF::ToInt(str); break;
		case SEED_TYPE:			me.m_parameters.m_world.m_seed = WBSF::ToInt(str); break;
		case USE_SPATIAL_INTERPOL:	me.m_parameters.m_world.m_bUseSpaceInterpolation = WBSF::ToBool(str); break;
		case USE_TIME_INTERPOL:	me.m_parameters.m_world.m_bUseTimeInterpolation = WBSF::ToBool(str); break;
		case USE_PREDICTOR_CORRECTOR_METHOD: me.m_parameters.m_world.m_bUsePredictorCorrectorMethod = WBSF::ToBool(str); break;
		case MAXIMUM_FLYERS:	me.m_parameters.m_world.m_maxFlyers = WBSF::ToSizeT(str); break;
		case MAX_MISS_WEATHER:	me.m_parameters.m_world.m_max_missing_weather = WBSF::ToDouble(str); break;
		case T_BROOD:			me.m_parameters.m_world.m_broodTSource = WBSF::ToSizeT(str); break;
		case P_TYPE:			me.m_parameters.m_world.m_PSource = WBSF::ToSizeT(str); break;
		case FORCE_FIRST_FLIGHT:	me.m_parameters.m_world.m_bForceFirstFlight = WBSF::ToBool(str); break;

		case DEM:				me.m_parameters.m_world.m_DEM_name = str; break;
		case GRIBS:				me.m_parameters.m_world.m_gribs_name = str; break;
		case HOURLY_DB:			me.m_parameters.m_world.m_hourly_DB_name = str; break;
		case DEFOLIATION:		me.m_parameters.m_world.m_defoliation_name = str; break;
		case WATER:				me.m_parameters.m_world.m_water_name = str; break;

		case P_MAX:				me.m_parameters.m_moths.m_Pmax = WBSF::ToDouble(str); break;
		case W_MIN:				me.m_parameters.m_moths.m_Wmin = WBSF::ToDouble(str); break;
		case WING_BEAT_SCALE:	me.m_parameters.m_moths.m_w_α = WBSF::ToDouble(str); break;
		case REDUCTION_FACTOR:		me.m_parameters.m_moths.m_Δv = WBSF::ToDouble(str); break;
		case REDUCTION_HEIGHT:		me.m_parameters.m_moths.m_Hv = WBSF::ToDouble(str); break;
		case W_HORZ:			me.m_parameters.m_moths.m_w_horizontal = WBSF::ToDouble(str); break;
		case W_HORZ_SD:			me.m_parameters.m_moths.m_w_horizontal_σ = WBSF::ToDouble(str); break;
		case W_DESCENT:			me.m_parameters.m_moths.m_w_descent = WBSF::ToDouble(str); break;
		case W_DESCENT_SD:		me.m_parameters.m_moths.m_w_descent_σ = WBSF::ToDouble(str); break;
		case MAXIMUM_FLIGHTS:	me.m_parameters.m_moths.m_maxFlights = WBSF::ToSizeT(str); break;
		case FLIGHT_AFTER_SUNRIZE:	me.m_parameters.m_moths.m_flight_after_sunrise = WBSF::ToDouble(str); break;
		case APPPLY_ATTRITION:		me.m_parameters.m_moths.m_bAppplyAttrition = WBSF::ToBool(str); break;
		case MAX_ADULT_LONGEVITY:	me.m_parameters.m_moths.m_max_adult_longevity = WBSF::ToSizeT(str); break;
		case READY_SHIFT0:		me.m_parameters.m_moths.m_ready_to_fly[0] = WBSF::as<double>(str); break;
		case READY_SHIFT1:		me.m_parameters.m_moths.m_ready_to_fly[1] = WBSF::as<double>(str); break;


		case OUTPUT_SUB_HOURLY: me.m_parameters.m_world.m_bOutputSubHourly = WBSF::ToBool(str); break;
		case OUTPUT_FILE_TITLE:	me.m_parameters.m_world.m_outputFileTitle = str; break;
		//case OUTPUT_TIME_FREQUENCY:	me.m_parameters.m_world.m_outputFrequency = WBSF::ToSizeT(str); break;
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


	//**************************************************************************************************************

	//**********************************************************************************************
	void CDispersalDlg::DoDataExchange(CDataExchange* pDX)
	{
		CDialogEx::DoDataExchange(pDX);



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

	//IMPLEMENT_DYNAMIC(CDispersalDlg, CDialogEx)

	BEGIN_MESSAGE_MAP(CDispersalDlg, CDialogEx)
		ON_WM_SIZE()
		ON_WM_DESTROY()
	END_MESSAGE_MAP()



	CDispersalDlg::CDispersalDlg(CExecutablePtr pParent, CWnd* pParentDlg) :
		CDialogEx(IDD, pParentDlg)
	{
	}


	CDispersalDlg::~CDispersalDlg()
	{
	}

	void CDispersalDlg::OnSize(UINT nType, int cx, int cy)
	{
		CDialogEx::OnSize(nType, cx, cy);

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

		CDialogEx::OnDestroy();
	}

}