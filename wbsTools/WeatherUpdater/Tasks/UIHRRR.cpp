#include "StdAfx.h"
#include "UIHRRR.h"
#include "Basic/FileStamp.h"
#include "UI/Common/SYShowMessage.h"

#include "HRRR.h"
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
	const char* CUIHRRR::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir" };
	const size_t CUIHRRR::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH };
	const UINT CUIHRRR::ATTRIBUTE_TITLE_ID = IDS_UPDATER_HRRR_P;
	const UINT CUIHRRR::DESCRIPTION_TITLE_ID = ID_TASK_HRRR;

	const char* CUIHRRR::CLASS_NAME(){ static const char* THE_CLASS_NAME = "HRRR";  return THE_CLASS_NAME; }
	CTaskBase::TType CUIHRRR::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUIHRRR::CLASS_NAME(), (createF)CUIHRRR::create);


	
	//HRDPS Canada 4 km
	//http://dd.weather.gc.ca/model_hrdps/continental/grib2
	//HRRR USA  3km 
	//http://nomads.ncep.noaa.gov/pub/data/nccf/com/hrrr/prod

	

	//const char* CUIHRRR::SERVER_NAME[NB_SOURCES] = { "dd.weather.gc.ca", "nomads.ncep.noaa.gov" };
	//const char* CUIHRRR::INPUT_FORMAT[NB_SOURCES] = { "/model_hrdps/continental/grib2", "/pub/data/nccf/com/hrrr/prod" };
	/*const char* CUIHRRR::SOURCES_NAME[NB_SOURCES] = { "HRDPS", "HRRR"};


	size_t CUIHRRR::GetSourcesIndex(const std::string& name)
	{
		size_t s = NOT_INIT;
		for (size_t ss = 0; ss < NB_SOURCES&&s == NOT_INIT; ss++)
		{
			if (IsEqual(name, SOURCES_NAME[ss]))
				s = ss;
		}

		assert(s < NB_SOURCES);

		return s;
	}*/




	CUIHRRR::CUIHRRR(void)
	{}
	
	CUIHRRR::~CUIHRRR(void)
	{}
	

	std::string CUIHRRR::Option(size_t i)const
	{
		string str;
		//switch (i)
		//{
		//case SOURCES:	str = "HRDPS=HRDPS (canada)|HRRR=HRRR (USA)"; break;
		//case HRDPS_VARS: str = GetHRDPSSelectionString(); break;
		//};
		return str;
	}

	std::string CUIHRRR::Default(size_t i)const
	{
		std::string str;
		switch (i)
		{
		case WORKING_DIR: str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()) + "HRRR\\"; break;
		};

		return str;
	}
		
	

	//************************************************************************************************************
	//Load station definition list section


	//*************************************************************************************************

	CTPeriod CUIHRRR::GetPeriod()const
	{
		CTPeriod p;

		/*StringVector t1(Get(FIRST_DATE), "-/");
		StringVector t2(Get(LAST_DATE), "-/");
		if (t1.size() == 3 && t2.size() == 3)
			p = CTPeriod(CTRef(ToInt(t1[0]), ToSizeT(t1[1]) - 1, ToSizeT(t1[2]) - 1, FIRST_HOUR), CTRef(ToInt(t2[0]), ToSizeT(t2[1]) - 1, ToSizeT(t2[2]) - 1, LAST_HOUR));
*/
		return p;
	}


	ERMsg CUIHRRR::Execute(CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);
		CreateMultipleDir(workingDir);

		CHRRR HRRR(workingDir);
		msg = HRRR.Execute(callback);


		return msg;
	}


	ERMsg CUIHRRR::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;

		return msg;
	}

	ERMsg CUIHRRR::GetWeatherStation(const std::string& stationName, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;
		return msg;
	}

	ERMsg CUIHRRR::GetGribsList(CTPeriod p, std::map<CTRef, std::string>& gribsList, CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);
		
		
		int firstYear = p.Begin().GetYear();
		int lastYear = p.End().GetYear();
		size_t nbYears = lastYear - firstYear + 1;
		
		for (size_t y = 0; y < nbYears; y++)
		{
			int year = firstYear + int(y);


			StringVector list1;
			list1 = GetFilesList(workingDir + ToString(year) + "\\*.grib2", FILE_PATH, true);
			StringVector list2;
			list2 = GetFilesList(workingDir + ToString(year) + "\\*.grib2.idx", FILE_PATH, true);


			std::set<CTRef> TRef2;
			for (size_t i = 0; i < list2.size(); i++)
			{
				CTRef TRef = GetTRef(list2[i]);
				TRef2.insert(TRef);
			}

			for (size_t i = 0; i < list1.size(); i++)
			{
				CTRef TRef = GetTRef(list1[i]);
				if (p.IsInside(TRef) && TRef2.find(TRef) != TRef2.end())
					gribsList[TRef] = list1[i];
			}
			
		}



		return msg;
	}

	CTRef CUIHRRR::GetTRef(string filePath)
	{
		CTRef TRef;
		
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

		return TRef;
	}
}