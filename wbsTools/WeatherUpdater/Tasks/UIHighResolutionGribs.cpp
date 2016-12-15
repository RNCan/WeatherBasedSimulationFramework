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

	//static const CGeoRect DEFAULT_BOUDINGBOX(-180, -90, 180, 90, PRJ_WGS_84);


	
	
	//canada
	//http://www.nco.ncep.noaa.gov/pmb/products/cmcens/cmc_gep01.t00z.pgrb2af00.shtml

	
	//sur ftp
	//ftp://ftp.ncep.noaa.gov/pub/data/nccf/com/hrrr/prod/

	//*********************************************************************
	const char* CUIHighResolutionGribs::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "Sources" };
	const size_t CUIHighResolutionGribs::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_STRING_SELECT };
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

	std::string CUIHighResolutionGribs::Option(size_t i)const
	{
		string str;
		switch (i)
		{
		case SOURCES:	str = "HRDPS=HRDPS (canada)|HRRR=HRRR (USA)"; break;
		};
		return str;
	}

	std::string CUIHighResolutionGribs::Default(size_t i)const
	{
		std::string str;
		switch (i)
		{
		case WORKING_DIR: str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()) + "HRG\\"; break;
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
				msg = HRDPS.Execute(callback);
				
			}
			else if (ss == N_HRRR)
			{
				CHRRR HRRR(workingDir + string(SOURCES_NAME[ss]) + "\\");
				msg = HRRR.Execute(callback);
			}
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

	CTRef CUIHighResolutionGribs::GetTRef(string filePath)
	{
		string name = GetFileTitle(filePath);
		int year = WBSF::as<int>(name.substr(8, 4));
		size_t m = WBSF::as<int>(name.substr(12, 2))-1;
		size_t d = WBSF::as<int>(name.substr(14, 2))-1;
		size_t h = WBSF::as<int>(name.substr(17, 2));

		return CTRef(year,m,d,h);
	}
}