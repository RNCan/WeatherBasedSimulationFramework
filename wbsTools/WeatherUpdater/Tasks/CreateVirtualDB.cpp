#include "StdAfx.h"
#include "CreateVirtualDB.h"
#include "Basic/WeatherStation.h"
#include "Basic/DailyDatabase.h"
#include "Basic/HourlyDatabase.h"
#include "Basic/WeatherDatabaseCreator.h"
#include "Basic/Timer.h"
#include "Basic/CSV.h"
#include "Geomatic/SfcGribsDatabase.h"
#include "UI/Common/SYShowMessage.h"

#include "TaskFactory.h"
#include "../resource.h"
#include "WeatherBasedSimulationString.h"

//#include "..\GeomaticTools\PointsExtractor\PointsExtractor.cpp"

using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace WBSF::WEATHER;

namespace WBSF
{
	//*********************************************************************
	const char* CCreateVirtualDB::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "GribsFilePath", "LocationsFilePath", "OutputFilePath", "Variables", "OutputType", "ExportType", "FirstDate", "LastDate", "Incremental" };
	const size_t CCreateVirtualDB::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_FILEPATH, T_FILEPATH, T_FILEPATH, T_STRING_SELECT, T_COMBO_INDEX,  T_COMBO_INDEX, T_DATE, T_DATE, T_BOOL };
	const UINT CCreateVirtualDB::ATTRIBUTE_TITLE_ID = IDS_TOOL_CREATE_VIRTUAL_P;
	const UINT CCreateVirtualDB::DESCRIPTION_TITLE_ID = ID_TASK_CREATE_VIRTUAL;


	const char* CCreateVirtualDB::CLASS_NAME() { static const char* THE_CLASS_NAME = "CreateVirtualDB";  return THE_CLASS_NAME; }
	CTaskBase::TType CCreateVirtualDB::ClassType()const { return CTaskBase::TOOLS; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CCreateVirtualDB::CLASS_NAME(), CCreateVirtualDB::create);


	CCreateVirtualDB::CCreateVirtualDB(void)
	{}

	CCreateVirtualDB::~CCreateVirtualDB(void)
	{}

	std::string CCreateVirtualDB::Option(size_t i)const
	{
		string str;

		switch (i)
		{
		case INPUT_FILEPATH:	str = GetString(IDS_STR_FILTER_GRIBS); break;
		case OUTPUT_FILEPATH:	str = GetString(IDS_STR_FILTER_OBSERVATION); break;
		case LOCATIONS_FILEPATH:str = GetString(IDS_STR_FILTER_LOC); break;
		case VARIABLES:			str = "Tmin|Tair|Tmax|Prcp|Tdew|RelH|WinS|WinD|SRad|Pres"; break;
		case OUTPUT_TYPE:		str = "Hourly|Daily"; break;
		case EXTRACTION_TYPE:	str = "At location|Nearest point|4 nearest points"; break;
		};

		return str;
	}

	std::string CCreateVirtualDB::Default(size_t i)const
	{
		string str;

		switch (i)
		{
		case OUTPUT_TYPE:		str = "0"; break;
		case FIRST_YEAR:
		case LAST_YEAR:	str = ToString(CTRef::GetCurrentTRef().GetYear()); break;
		};

		return str;
	}

	ERMsg CCreateVirtualDB::Execute(CCallback& callback)
	{
		ERMsg msg;

		string inputFilePath = Get(INPUT_FILEPATH);
		if (inputFilePath.empty())
		{
			msg.ajoute(GetString(IDS_BSC_NAME_EMPTY));
			return msg;
		}

		string outputFilePath = Get(OUTPUT_FILEPATH);
		if (outputFilePath.empty())
		{
			msg.ajoute(GetString(IDS_BSC_NAME_EMPTY));
			return msg;
		}

		//SetFileExtension(outputFilePath, (outputType == OT_HOURLY) ? CHourlyDatabase::DATABASE_EXT : CDailyDatabase::DATABASE_EXT);
		SetFileExtension(outputFilePath, CHourlyDatabase::DATABASE_EXT);
		callback.AddMessage(GetString(IDS_CREATE_DB));
		callback.AddMessage(outputFilePath, 1);

		CGribsDB gribs;
		msg += gribs.load(inputFilePath);


		CLocationVector locations;
		if (msg)
			msg += locations.Load(Get(LOCATIONS_FILEPATH));


		if (!msg)
			return msg;

		CLocationVector locationsII;
		size_t extractionType = as<size_t>(EXTRACTION_TYPE);
		switch (extractionType)
		{
		case E_AT_LOCATION: locationsII = locations;  break;
			//case E_NEREST: locationsII = GetNearest(locations, 1); break;
			//case E_4NEAREST: locationsII = GetNearest(locations, 4); break;
		};

		//size_t outputType = as<size_t>(OUTPUT_TYPE);
		bool bIncremental = as<bool>(INCREMENTAL);

		CIncementalDB incremental;
		if (!bIncremental)

		{
			//delete incremental file
			WBSF::RemoveFile(outputFilePath + ".inc");
			//pDB->DeleteDatabase()
			//delete database
			//if (outputType == OT_HOURLY)
			//{
			msg += CHourlyDatabase::DeleteDatabase(outputFilePath, callback);
			//}
			//else
			//{
			//	msg += CDailyDatabase::DeleteDatabase(outputFilePath, callback);
			//}

			
		}


	
		
		
		msg = CreateMultipleDir(GetPath(outputFilePath));


		//Get the data for each station
		//CWeatherDatabasePtr pDB = CreateWeatherDatabase(outputFilePath);
		CHourlyDatabase DB;
		//if (pDB.get() == NULL)
			//msg.ajoute("Unknown output database type");

		//if (msg)
		msg += DB.Open(outputFilePath, CDailyDatabase::modeWrite);

		

		CWeatherStationVector stations;


		if (bIncremental)
		{
			if (FileExists(outputFilePath + ".inc"))
				msg = incremental.load(outputFilePath + ".inc");

			if (DB.empty())
			{
				stations.resize(locationsII.size());
				for (size_t i = 0; i < locationsII.size(); i++)
				{
					((CLocation&)stations[i]) = locationsII[i];
					stations[i].SetHourly(true);
				}
			}
			else
			{
				if (DB.size() == locationsII.size())
				{
					//load database
					for (size_t i = 0; i < locationsII.size(); i++)
						msg += DB.Get(stations[i], i);
				}
				else
				{
					msg.ajoute("The number of station in the database is not the same as the ... Do not use incremental.");
				}
			}
		}
		else
		{
			stations.resize(locationsII.size());
			for (size_t i = 0; i < locationsII.size(); i++)
			{
				((CLocation&)stations[i]) = locationsII[i];
				stations[i].SetHourly(true);
			}
		}

	

		//CTPeriod invalid_period;
		std::set<CTRef> invalid;
		incremental.GetInvalidTRef(gribs, invalid);
		if (!invalid.empty())//there is an invalid period, up-tu-date otherwise
		{
			
			if (msg)
			{
				CTimer timer(true);
				CTimer timerRead;
				CTimer timerWrite;
				
				int nbStationAdded = 0;
				callback.PushTask(GetString(IDS_CREATE_DB) + GetFileName(outputFilePath) + " (Extracting " + ToString(stations.size()) + " virtual stations)", invalid.size()*stations.size());

				//init coord and info
				for (std::set<CTRef>::const_iterator it = invalid.begin(); it != invalid.end()&&msg; it++)
				{
					timerRead.Start();
					msg = ExtractStation(*it, gribs[*it], stations, callback);
					timerRead.Stop();
				}

				callback.PushTask("Save weather to disk", invalid.size()*stations.size());
				for (CWeatherStationVector::iterator it = stations.begin(); it != stations.end(); it++)
				{
					if (msg)
					{
						if (it->HaveData())
						{
							string newName = DB.GetUniqueName(it->m_name);
							if (newName != it->m_name)
							{
								it->m_name = newName;
								it->SetDataFileName("");
							}

							//Force write file name in the file
							it->SetDataFileName(it->GetDataFileName());
							it->UseIt(true);


							timerWrite.Start();
							msg = DB.Set(std::distance(stations.begin(), it), *it);
							timerWrite.Stop();

							if (msg)
								nbStationAdded++;
						}
					}

					//if (!msg)
			//			callback.AddMessage(messageTmp, 1);
		//
					msg += callback.StepIt();

				}

				msg += DB.Close();
				timer.Stop();
				callback.PopTask();


				if (msg)
				{
					msg = DB.Open(outputFilePath, CDailyDatabase::modeRead, callback);
					DB.Close();
				}

				if (msg)
				{
					callback.AddMessage(GetString(IDS_STATION_ADDED) + ToString(nbStationAdded), 1);
					callback.AddMessage(FormatMsg(IDS_BSC_TIME_READ, SecondToDHMS(timerRead.Elapsed())));
					callback.AddMessage(FormatMsg(IDS_BSC_TIME_WRITE, SecondToDHMS(timerWrite.Elapsed())));
					callback.AddMessage(FormatMsg(IDS_BSC_TOTAL_TIME, SecondToDHMS(timer.Elapsed())));
				}
			}
		}

		return msg;
	}
	
	std::bitset< HOURLY_DATA::NB_VAR_ALL> GetVariables(string str)
	{
		std::bitset< HOURLY_DATA::NB_VAR_ALL> variables;
		if (str.empty())
		{
			variables.set();
		}
		else
		{
			StringVector vars(str, ",;| "); 
			for (size_t v = 0; v < vars.size(); v++)
			{
				TVarH var = HOURLY_DATA::GetVariableFromName(vars[v], true);
				if (var != SKIP)
					variables.set(var);
			}
		}

		return variables;
	}

	ERMsg CCreateVirtualDB::ExtractStation(CTRef TRef, const std::string& file_path, CWeatherStationVector& stations, CCallback& callback)
	{
		ERMsg msg;

		string str = Get(VARIABLES);
		std::bitset< HOURLY_DATA::NB_VAR_ALL> variables = GetVariables(str);

		CSfcDatasetCached sfcDS;
		sfcDS.set_variables(variables);

		msg = sfcDS.open(file_path, true);
		if (msg)
		{
			CProjectionTransformation GEO_2_WEA(PRJ_WGS_84, sfcDS.GetPrjID());
			for (size_t i = 0; i < stations.size()&&msg; i++)
			{
				CGeoPoint pt = stations[i];
				pt.Reproject(GEO_2_WEA);
				if (sfcDS.GetExtents().IsInside(pt))
				{
					CHourlyData& data = stations[i].GetHour(TRef);
					sfcDS.get_weather(pt, data);

					msg += callback.StepIt();
				}
			}

			sfcDS.close();
		}

		return msg;
	}
	//ERMsg CCreateVirtualDB::CreateDatabase(const std::string& outputFilePath, CTaskPtr& pTask, CTaskPtr& pForecastTask, CCallback& callback)const
	//{
	//	ERMsg msg;

	//	CTimer timer(true);
	//	CTimer timerRead;
	//	CTimer timerWrite;

	//	
	//	callback.AddMessage(GetString(IDS_CREATE_DB));
	//	callback.AddMessage(outputFilePath, 1);

	//	//Get the data for each station
	//	CDailyDatabase dailyDB;
	//	msg = dailyDB.Open(outputFilePath, CDailyDatabase::modeWrite);
	//	if (!msg)
	//		return msg;

	//	int nbStationAdded = 0;

	//	StringVector stationList;
	//	msg = pTask->GetStationList(stationList, callback);

	//	if (msg)
	//	{
	//		callback.PushTask(GetString(IDS_CREATE_DB) + GetFileName(outputFilePath) + " (Extracting " + ToString(stationList.size()) + " stations)", stationList.size());


	//		for (size_t i = 0; i < stationList.size() && msg; i++)
	//		{
	//			CWeatherStation station;

	//			timerRead.Start();
	//			ERMsg messageTmp = pTask->GetWeatherStation(stationList[i], CTM(CTM::DAILY), station, callback);
	//			timerRead.Stop();

	//			if (messageTmp)
	//			{
	//				CleanSparse(station);

	//				if (station.HaveData())
	//				{
	//					string newName = dailyDB.GetUniqueName(station.m_name);
	//					if (newName != station.m_name)
	//					{
	//						station.m_name = newName;
	//						station.SetDataFileName("");
	//					}

	//					//Force write file name in the file
	//					station.SetDataFileName(station.GetDataFileName());
	//					station.UseIt(true);

	//					//Get forecast
	//					if (pForecastTask)
	//						msg += pForecastTask->GetWeatherStation("", CTM(CTM::DAILY), station, callback);
	//					

	//					timerWrite.Start();
	//					messageTmp = dailyDB.Add(station);
	//					timerWrite.Stop();

	//					if (messageTmp)
	//						nbStationAdded++;
	//				}
	//			}
	//			else
	//			{
	//				if (callback.GetUserCancel())
	//					msg += messageTmp;
	//			}

	//			if (!messageTmp)
	//				callback.AddMessage(messageTmp, 1);

	//			msg += callback.StepIt();

	//		}

	//		msg += dailyDB.Close();
	//		timer.Stop();
	//		callback.PopTask();


	//		if (msg)
	//		{
	//			msg = dailyDB.Open(outputFilePath, CDailyDatabase::modeRead, callback);
	//			dailyDB.Close();
	//		}

	//		if (msg)
	//		{
	//			callback.AddMessage(GetString(IDS_STATION_ADDED) + ToString(nbStationAdded), 1);
	//			callback.AddMessage(FormatMsg(IDS_BSC_TIME_READ, SecondToDHMS(timerRead.Elapsed())));
	//			callback.AddMessage(FormatMsg(IDS_BSC_TIME_WRITE, SecondToDHMS(timerWrite.Elapsed())));
	//			callback.AddMessage(FormatMsg(IDS_BSC_TOTAL_TIME, SecondToDHMS(timer.Elapsed())));
	//		}


	//	}

	//	return msg;
	//}



	static size_t GetDefaultStat(size_t v)
	{
		size_t s = MEAN;
		if (v == H_PRCP || v == H_SNOW)
			s = SUM;

		return s;
	}


}