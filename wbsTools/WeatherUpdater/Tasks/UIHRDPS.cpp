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
	const char* CUIHRDPS::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "Variables" , "BuildVRT"};
	const size_t CUIHRDPS::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_STRING_SELECT, T_BOOL };
	const UINT CUIHRDPS::ATTRIBUTE_TITLE_ID = IDS_UPDATER_HRDPS_P; 
	const UINT CUIHRDPS::DESCRIPTION_TITLE_ID = ID_TASK_HRDPS;

	const char* CUIHRDPS::CLASS_NAME(){ static const char* THE_CLASS_NAME = "HRDPS";  return THE_CLASS_NAME; }
	CTaskBase::TType CUIHRDPS::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUIHRDPS::CLASS_NAME(), (createF)CUIHRDPS::create);


	CUIHRDPS::CUIHRDPS(void)
	{}
	
	CUIHRDPS::~CUIHRDPS(void)
	{}

	string CUIHRDPS::GetHRDPSSelectionString()
	{
		string select;
		for (size_t i = 0; i < NB_HRDPS_VARIABLES; i++)
			select += string(CHRDPSVariables::GetName(i)) + "=" + string(CHRDPSVariables::GetName(i)) + ":" + CHRDPSVariables::GetDescription(i) + "|";
		
		return select;
	}

	std::string CUIHRDPS::Option(size_t i)const
	{
		string str;
		switch (i)
		{
		case HRDPS_VARS: str = GetHRDPSSelectionString(); break;
		};
		return str;
	}

	std::string CUIHRDPS::Default(size_t i)const
	{
		std::string str;
		switch (i)
		{
		case WORKING_DIR: str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()) + "HRDPS\\"; break;
		case HRDPS_VARS: str = ""; break;
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
		HRDPS.m_variables = Get(HRDPS_VARS);
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

	ERMsg CUIHRDPS::GetGribsList(CTPeriod p, std::map<CTRef, std::string>& gribsList, CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);
		
		int firstYear = p.Begin().GetYear();
		int lastYear = p.End().GetYear();
		size_t nbYears = lastYear - firstYear + 1;
		
		for (size_t y = 0; y < nbYears; y++)
		{
			int year = firstYear + int(y);


			StringVector list;
			list = GetFilesList(workingDir + ToString(year) + "\\*.vrt", FILE_PATH, true);
			
			for (size_t i = 0; i < list.size(); i++)
			{
				CTRef TRef = GetTRef(list[i]);
				if (p.IsInside(TRef))
					gribsList[TRef] = list[i];
			}
		}



		return msg;
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