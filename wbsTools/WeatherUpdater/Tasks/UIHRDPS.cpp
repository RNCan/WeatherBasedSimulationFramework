#include "StdAfx.h"
#include "UIHRDPS.h"
#include "Basic/FileStamp.h"
#include "UI/Common/SYShowMessage.h"

#include "HRDPS.h"
#include "TaskFactory.h"
#include "WeatherBasedSimulationString.h"
#include "../Resource.h"


using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace UtilWWW;


namespace WBSF
{
	//canada
	//http://www.nco.ncep.noaa.gov/pmb/products/cmcens/cmc_gep01.t00z.pgrb2af00.shtml

	//HRDPS Canada 4 km
	//http://dd.weather.gc.ca/model_hrdps/continental/grib2
	



	//*********************************************************************
	const char* CUIHRDPS::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "VariablesSFC", "VariablesTGL", "VariablesISBL", "VariablesOthers",  "TGLHeight", "ISBLLevels", "ComputeHourlyPrecipitation", "UpdateLastNDays" };
	const size_t CUIHRDPS::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_STRING_SELECT, T_STRING_SELECT, T_STRING_SELECT, T_STRING_SELECT, T_STRING_SELECT, T_STRING_SELECT, T_BOOL, T_STRING };
	const UINT CUIHRDPS::ATTRIBUTE_TITLE_ID = IDS_UPDATER_HRDPS_P; 
	const UINT CUIHRDPS::DESCRIPTION_TITLE_ID = ID_TASK_HRDPS;

	const char* CUIHRDPS::CLASS_NAME(){ static const char* THE_CLASS_NAME = "HRDPS";  return THE_CLASS_NAME; }
	CTaskBase::TType CUIHRDPS::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUIHRDPS::CLASS_NAME(), (createF)CUIHRDPS::create);


	CUIHRDPS::CUIHRDPS(void)
	{}
	
	CUIHRDPS::~CUIHRDPS(void)
	{}

	

	std::string CUIHRDPS::Option(size_t i)const
	{
		string str;
		switch (i)
		{
		case HRDPS_VARS_SFC: str = CHRDPSVariables::GetHRDPSSelectionString(HRDPS_SFR); break;
		case HRDPS_VARS_AGL: str = CHRDPSVariables::GetHRDPSSelectionString(HRDPS_AGL); break;
		case HRDPS_VARS_ISBL: str = CHRDPSVariables::GetHRDPSSelectionString(HRDPS_ISBL); break;
		case HRDPS_VARS_OTHERS: for(size_t c= HRDPS_ISBY; c< NB_HRDPS_CATEGORY; c++)str += CHRDPSVariables::GetHRDPSSelectionString(c); break;
		case HRDPS_AGL_HEIGHTS: str = "2|10|40|80|120"; break;
		case HRDPS_ISBL_LEVELS: str = "1015|1000|0985|0970|0950|0925|0900|0875|0850|0800|0750|0700|0650|0600|0550|0500|0450|0400|0350|0300|0275|0250|0225|0200|0175|0150|0100|0050"; break;
		};
		return str;
}


	std::string CUIHRDPS::Default(size_t i)const
	{
		std::string str;
		switch (i)
		{
		case WORKING_DIR: str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()) + "HRDPS\\"; break;
		case HRDPS_VARS_SFC: str = "APCP_SFC|DSWRF_SFC|HGT_SFC|PRES_SFC|SNOD_SFC|TCDC_SFC|"; break;
		case HRDPS_VARS_AGL: str = "DPT_AGL|RH_AGL|TMP_AGL|WDIR_AGL|WIND_AGL"; break;
		case HRDPS_VARS_ISBL: str = "----"; break;
		case HRDPS_VARS_OTHERS: str = "----"; break;
		case HRDPS_AGL_HEIGHTS: str = "2|10"; break;
		case HRDPS_ISBL_LEVELS: str = "1015|1000|0985|0970|0950|0925|0900|0875|0850|0800|0750"; break;
		case COMPUTE_HOURLY_PRCP: str = "1"; break;
		case UPDATE_LAST_N_DAYS:  str = "0"; break;
		};

		return str;
	}
		
	

	//************************************************************************************************************
	//Load station definition list section


	//*************************************************************************************************


	ERMsg CUIHRDPS::Execute(CCallback& callback)
	{
		ERMsg msg;

		

		string workingDir = GetDir(WORKING_DIR);
		CreateMultipleDir(workingDir);
	
		CHRDPS HRDPS(workingDir);
		HRDPS.m_bHRDPA6h = as<bool>(COMPUTE_HOURLY_PRCP);
		HRDPS.m_update_last_n_days = as<size_t>(UPDATE_LAST_N_DAYS);
		//HRDPS.m_createHistiricalGeotiff = as<bool>(CREATE_HISTORICAL_GEOTIFF);
		HRDPS.m_bCreateDailyGeotiff = false;//create daily CanUS instead

		CHRDPSVariables sfc (Get(HRDPS_VARS_SFC));
		CHRDPSVariables alg (Get(HRDPS_VARS_AGL));
		CHRDPSVariables isbl( Get(HRDPS_VARS_ISBL));
		CHRDPSVariables others (Get(HRDPS_VARS_OTHERS));

		//if (HRDPS.m_bHRDPA6h)//add precipitation
			//sfc.set(APCP_SFC);


		HRDPS.m_variables = (sfc| alg| isbl| others);
		
		
		

		if (!Get(HRDPS_AGL_HEIGHTS).empty())
			HRDPS.m_heights = Get(HRDPS_AGL_HEIGHTS);
		else
			HRDPS.m_heights.FromString("2|10|40|80|120"); 

		if (!Get(HRDPS_ISBL_LEVELS).empty())
			HRDPS.m_levels = Get(HRDPS_ISBL_LEVELS);
		else
			HRDPS.m_levels.FromString("1015|1000|0985|0970|0950|0925|0900|0875|0850|0800|0750|0700|0650|0600|0550|0500|0450|0400|0350|0300|0275|0250|0225|0200|0175|0150|0100|0050");

		msg = HRDPS.Execute(callback);

		return msg;
	}


	ERMsg CUIHRDPS::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;

		return msg;
	}

	ERMsg CUIHRDPS::GetWeatherStation(const std::string& stationName, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;
		return msg;
	}

	ERMsg CUIHRDPS::GetGribsList(CTPeriod p, CGribsMap& gribsList, CCallback& callback)
	{ 
		ASSERT(p.GetTM() == CTM::HOURLY);
		
		string workingDir = GetDir(WORKING_DIR);
		CHRDPS HRDPS(workingDir);
		return HRDPS.GetGribsList(p, gribsList, callback);
	}

	CTRef CUIHRDPS::GetTRef(string filePath)
	{
		CTRef TRef;
		string name = GetFileTitle(filePath);
		string str = name.substr(name.size() - 19, 15);
			
		int year = WBSF::as<int>(str.substr(0, 4));
		size_t m = WBSF::as<int>(str.substr(4, 2)) - 1;
		size_t d = WBSF::as<int>(str.substr(6, 2)) - 1;
		size_t h = WBSF::as<int>(str.substr(8, 2));
		size_t hh = WBSF::as<int>(str.substr(12, 3));
			
		TRef = CTRef(year, m, d, h+hh);
	

		return TRef;
	}
}