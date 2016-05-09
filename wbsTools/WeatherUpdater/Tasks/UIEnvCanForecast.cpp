#include "StdAfx.h"
#include "UIEnvCanForecast.h"
#include "Basic/WeatherStation.h"
#include "Geomatic/ShapeFileBase.h"
#include "UI/Common/UtilWWW.h"
#include "TaskFactory.h"

#include "WeatherBasedSimulationString.h"
#include "../Resource.h"



using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace UtilWWW;

namespace WBSF
{
	const char* CUIEnvCanForecast::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "Method", "GribsType" };
	const size_t CUIEnvCanForecast::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_COMBO_INDEX, T_COMBO_INDEX };
	const UINT CUIEnvCanForecast::ATTRIBUTE_TITLE_ID = IDS_UPDATER_EC_FORECAST_P;
	const UINT CUIEnvCanForecast::DESCRIPTION_TITLE_ID = ID_TASK_EC_FORECAST;

	const char* CUIEnvCanForecast::CLASS_NAME(){ static const char* THE_CLASS_NAME = "EnvCanForecast";  return THE_CLASS_NAME; }
	CTaskBase::TType CUIEnvCanForecast::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUIEnvCanForecast::CLASS_NAME(), (createF)CUIEnvCanForecast::create);


	CUIEnvCanForecast::CUIEnvCanForecast(void)
	{}
	

	CUIEnvCanForecast::~CUIEnvCanForecast(void)
	{}


	std::string CUIEnvCanForecast::Option(size_t i)const
	{
		string str;
		switch (i)
		{
		case METHOD:	str = "MeteoCode|Gribs|Both"; break;
		case GRIBS_TYPE: str = "HRDPS (Canada at 2.5 km)|RDPS (North America at 10 km)"; break;
		};
		return str;
	}

	std::string CUIEnvCanForecast::Default(size_t i)const
	{
		std::string str;
		switch (i)
		{
		case WORKING_DIR: str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()) + "EnvCan\\Forecast\\"; break;
		case METHOD:	str = "2"; break;
		case GRIBS_TYPE: str = "0"; break;

		};

		return str;
	}


	//***********************************************************************************************************************
	// download section

	//******************************************************
	
	ERMsg CUIEnvCanForecast::Execute(CCallback& callback)
	{
		ERMsg msg;

		size_t type = as<size_t>(GRIBS_TYPE);
		m_meteoCode.m_workingDir = GetDir(WORKING_DIR) + "MeteoCode\\";
		m_gribs.m_workingDir = GetDir(WORKING_DIR) + ((type == GT_HRDPS) ? "HRDPS\\" : "RDPS\\");
		m_gribs.m_type = type;


		size_t method = as<size_t>(METHOD);
		if (method==M_BOTH)
			callback.PushTask("Download Forecast", 2);

		if (method == M_BOTH || method == M_METEO_CODE)
			m_meteoCode.Execute(callback);
			

		if (method == M_BOTH)
			msg += callback.StepIt();

		if (method == M_BOTH || method == M_GRIBS)
			m_gribs.Execute(callback);

		if (method == M_BOTH)
			callback.PopTask();

		return msg;
	}

	
	ERMsg CUIEnvCanForecast::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;

		stationList.clear();

		size_t type = as<size_t>(GRIBS_TYPE);
		m_meteoCode.m_workingDir = GetDir(WORKING_DIR) + "MeteoCode\\";
		m_gribs.m_workingDir = GetDir(WORKING_DIR) + ((type == GT_HRDPS) ? "HRDPS\\" : "RDPS\\");
		m_gribs.m_type = type;


		size_t method = as<size_t>(METHOD);
		if (method == M_BOTH)
			callback.PushTask("Download Forecast", 2);

		if (method == M_BOTH || method == M_METEO_CODE)
			msg += m_meteoCode.GetStationList(stationList, callback);


		if (method == M_BOTH)
			msg += callback.StepIt();

		if (msg && (method == M_BOTH || method == M_GRIBS) )
			msg += m_gribs.GetStationList(stationList, callback);

		if (method == M_BOTH)
			callback.PopTask();


		
		return msg;
	}




	ERMsg CUIEnvCanForecast::GetWeatherStation(const std::string& ID, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;

		size_t type = as<size_t>(GRIBS_TYPE);
		m_meteoCode.m_workingDir = GetDir(WORKING_DIR) + "MeteoCode\\";
		m_gribs.m_workingDir = GetDir(WORKING_DIR) + ((type == GT_HRDPS) ? "HRDPS\\" : "RDPS\\");
		m_gribs.m_type = type;


		size_t method = as<size_t>(METHOD);

		if (method == M_BOTH || method == M_METEO_CODE)
			msg += m_meteoCode.GetWeatherStation(ID, TM, station, callback);

		if (msg && (method == M_BOTH || method == M_GRIBS))
			msg += m_gribs.GetWeatherStation(ID, TM, station, callback);



		return msg;
	}

}