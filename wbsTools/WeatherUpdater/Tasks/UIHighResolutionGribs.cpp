#include "StdAfx.h"
#include "UIHighResolutionGribs.h"
#include "Basic/FileStamp.h"
//#include "Basic/CSV.h"
//#include "Basic/WeatherStation.h"
#include "UI/Common/SYShowMessage.h"

#include "HRRR.h"
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

	
	//sur ftp
	//ftp://ftp.ncep.noaa.gov/pub/data/nccf/com/hrrr/prod/

	//*********************************************************************
	const char* CUIHighResolutionGribs::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "Sources", "HRDPSVars" , "BuildHRDPSVRT"};
	const size_t CUIHighResolutionGribs::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_STRING_SELECT, T_STRING_SELECT, T_BOOL };
	const UINT CUIHighResolutionGribs::ATTRIBUTE_TITLE_ID = IDS_UPDATER_HRG_P;
	const UINT CUIHighResolutionGribs::DESCRIPTION_TITLE_ID = ID_TASK_HRG;

	const char* CUIHighResolutionGribs::CLASS_NAME(){ static const char* THE_CLASS_NAME = "HighResolutionGribs";  return THE_CLASS_NAME; }
	CTaskBase::TType CUIHighResolutionGribs::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUIHighResolutionGribs::CLASS_NAME(), (createF)CUIHighResolutionGribs::create);


	
	//HRDPS Canada 4 km
	//http://dd.weather.gc.ca/model_hrdps/continental/grib2
	//HRRR USA  3km 
	//http://nomads.ncep.noaa.gov/pub/data/nccf/com/hrrr/prod

	

	//const char* CUIHighResolutionGribs::SERVER_NAME[NB_SOURCES] = { "dd.weather.gc.ca", "nomads.ncep.noaa.gov" };
	//const char* CUIHighResolutionGribs::INPUT_FORMAT[NB_SOURCES] = { "/model_hrdps/continental/grib2", "/pub/data/nccf/com/hrrr/prod" };
	const char* CUIHighResolutionGribs::SOURCES_NAME[NB_SOURCES] = { "HRDPS", "HRRR"};


	size_t CUIHighResolutionGribs::GetSourcesIndex(const std::string& name)
	{
		size_t s = NOT_INIT;
		for (size_t ss = 0; ss < NB_SOURCES&&s == NOT_INIT; ss++)
		{
			if (IsEqual(name, SOURCES_NAME[ss]))
				s = ss;
		}

		assert(s < NB_SOURCES);

		return s;
	}




	CUIHighResolutionGribs::CUIHighResolutionGribs(void)
	{}
	
	CUIHighResolutionGribs::~CUIHighResolutionGribs(void)
	{}

	string CUIHighResolutionGribs::GetHRDPSSelectionString()
	{
		string select;
		for (size_t i = 0; i < NB_HRDPS_VARIABLES; i++)
			select += string(CHRDPSVariables::GetName(i)) + "=" + string(CHRDPSVariables::GetName(i)) + ":" + CHRDPSVariables::GetDescription(i) + "|";
		
		return select;
	}

	std::string CUIHighResolutionGribs::Option(size_t i)const
	{
		string str;
		switch (i)
		{
		case SOURCES:	str = "HRDPS=HRDPS (canada)|HRRR=HRRR (USA)"; break;
		case HRDPS_VARS: str = GetHRDPSSelectionString(); break;
		};
		return str;
	}

	std::string CUIHighResolutionGribs::Default(size_t i)const
	{
		std::string str;
		switch (i)
		{
		case WORKING_DIR: str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()) + "HRG\\"; break;
		case HRDPS_VARS: str = ""; break;
		};

		return str;
	}
		
	

	//************************************************************************************************************
	//Load station definition list section


	//*************************************************************************************************

	CTPeriod CUIHighResolutionGribs::GetPeriod()const
	{
		CTPeriod p;

		/*StringVector t1(Get(FIRST_DATE), "-/");
		StringVector t2(Get(LAST_DATE), "-/");
		if (t1.size() == 3 && t2.size() == 3)
			p = CTPeriod(CTRef(ToInt(t1[0]), ToSizeT(t1[1]) - 1, ToSizeT(t1[2]) - 1, FIRST_HOUR), CTRef(ToInt(t2[0]), ToSizeT(t2[1]) - 1, ToSizeT(t2[2]) - 1, LAST_HOUR));
*/
		return p;
	}


	ERMsg CUIHighResolutionGribs::Execute(CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);
		CreateMultipleDir(workingDir);


		StringVector sources = as<StringVector>(SOURCES);
		if (sources.empty())
			sources = StringVector("HRDPS|HRRR","|");

	
		size_t nbFileFound = 0;
		for (size_t s = 0; s < sources.size(); s++)
		{
			size_t ss = GetSourcesIndex(sources[s]);
			if (ss > NB_SOURCES)
			{
				callback.AddMessage("WARNING: Invalid source " + sources[s]);
				continue;
			}
			
			if (ss == N_HRDPS)
			{
				CHRDPS HRDPS(workingDir + string(SOURCES_NAME[ss]) + "\\");
				HRDPS.m_variables = Get(HRDPS_VARS);
				msg = HRDPS.Execute(callback);
				
			}
			else if (ss == N_HRRR)
			{
				CHRRR HRRR(workingDir + string(SOURCES_NAME[ss]) + "\\");
				msg = HRRR.Execute(callback);
			}

			if (callback.GetUserCancel())
				break;
		}


		return msg;
	}


	ERMsg CUIHighResolutionGribs::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;

		return msg;
	}

	ERMsg CUIHighResolutionGribs::GetWeatherStation(const std::string& stationName, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;
		return msg;
	}

	ERMsg CUIHighResolutionGribs::GetGribsList(CTPeriod p, std::map<CTRef, std::string>& gribsList, CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);


		StringVector sources = as<StringVector>(SOURCES);
		if (sources.empty())
			sources = StringVector("HRDPS", "|");

		
		int firstYear = p.Begin().GetYear();
		int lastYear = p.End().GetYear();
		size_t nbYears = lastYear - firstYear + 1;
		size_t ss = GetSourcesIndex(sources.front());//take only the first databaset


		
		for (size_t y = 0; y < nbYears; y++)
		{
			int year = firstYear + int(y);


			StringVector list;
			if (ss == N_HRDPS)
				list = GetFilesList(workingDir + string(SOURCES_NAME[ss]) + "\\" + ToString(year) + "\\*.vrt", FILE_PATH, true);
			else if ( ss == N_HRRR)
				list = GetFilesList(workingDir + string(SOURCES_NAME[ss]) + "\\" + ToString(year) + "\\*.grib2", FILE_PATH, true);

			
			for (size_t i = 0; i < list.size(); i++)
			{
				CTRef TRef = GetTRef(ss, list[i]);
				if (p.IsInside(TRef))
					gribsList[TRef] = list[i];
			}
		}



		return msg;
	}

	CTRef CUIHighResolutionGribs::GetTRef(size_t source, string filePath)
	{
		CTRef TRef;
		if (source == N_HRDPS)
		{
			string name = GetFileTitle(filePath);
			string str = name.substr(name.size() - 19, 15);
			
			int year = WBSF::as<int>(str.substr(0, 4));
			size_t m = WBSF::as<int>(str.substr(4, 2)) - 1;
			size_t d = WBSF::as<int>(str.substr(6, 2)) - 1;
			size_t h = WBSF::as<int>(str.substr(8, 2));
			size_t hh = WBSF::as<int>(str.substr(12, 3));
			
			TRef = CTRef(year, m, d, h+hh);

		}
		else if (source == N_HRRR)
		{
			string name = GetFileTitle(filePath);
			filePath = GetPath(filePath);
			string dir1 = WBSF::GetLastDirName(filePath);
			while (WBSF::IsPathEndOk(filePath))
				filePath = filePath.substr(0, filePath.length() - 1);
			filePath = GetPath(filePath);
			string dir2 = WBSF::GetLastDirName(filePath);
			while (WBSF::IsPathEndOk(filePath))
				filePath = filePath.substr(0, filePath.length() - 1);
			filePath = GetPath(filePath);
			string dir3 = WBSF::GetLastDirName(filePath);
			
			int year = WBSF::as<int>(dir3);
			size_t m = WBSF::as<int>(dir2) - 1;
			size_t d = WBSF::as<int>(dir1) - 1;
			size_t h = WBSF::as<int>(name.substr(6, 2));
			TRef = CTRef(year, m, d, h);
		}


		return TRef;
	}
}