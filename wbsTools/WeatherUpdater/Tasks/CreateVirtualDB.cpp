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
	const char* CCreateVirtualDB::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "GribsFilePath", "LocationsFilePath", "OutputFilePath", "Variables", "OutputType", "ExportType", "FirstYear", "LastYear", "Incremental" };
	const size_t CCreateVirtualDB::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_FILEPATH, T_FILEPATH, T_FILEPATH, T_COMBO_INDEX,  T_COMBO_INDEX, T_STRING, T_STRING, T_BOOL };
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


		callback.AddMessage(GetString(IDS_CREATE_DB));
		callback.AddMessage(outputFilePath, 1);

		CGribsDB gribs;
		msg += gribs.load(inputFilePath);

		//Get the data for each station
		CWeatherDatabasePtr pDB = CreateWeatherDatabase(outputFilePath);
		if (pDB.get() == NULL)
			msg.ajoute("Unknown output database type");

		if(msg)
			msg += pDB->Open(outputFilePath, CDailyDatabase::modeWrite);


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

		CWeatherStationVector stations;

		size_t outputType = as<size_t>(OUTPUT_TYPE);
		SetFileExtension(outputFilePath, (outputType == OT_HOURLY) ? CHourlyDatabase::DATABASE_EXT : CDailyDatabase::DATABASE_EXT);

		msg = CreateMultipleDir(GetPath(outputFilePath));
		bool bIncremental = as<bool>(INCREMENTAL);

		CIncementalDB incremental;
		if (bIncremental)
		{
			if(FileExists(outputFilePath + ".inc"))
				msg = incremental.load(outputFilePath + ".inc");

			if (pDB->empty())
			{
				stations.resize(locationsII.size());
				for (size_t i = 0; i < locationsII.size(); i++)
					((CLocation&)stations[i]) = locationsII[i];
			}
			else
			{
				if (pDB->size() == locationsII.size())
				{
					//load database
					for (size_t i = 0; i < locationsII.size(); i++)
						msg += pDB->Get(stations[i], i);
				}
				else
				{
					msg.ajoute("The number of station in the database is not the same as the ... Do not use incremental.");
				}
			}
		}
		else
		{
			//delete incremental file
			WBSF::RemoveFile(outputFilePath + ".inc");
			//pDB->DeleteDatabase()
			//delete database
			if (outputType == OT_HOURLY)
			{
				msg += CHourlyDatabase::DeleteDatabase(outputFilePath, callback);
			}
			else
			{
				msg += CDailyDatabase::DeleteDatabase(outputFilePath, callback);
			}

			stations.resize(locationsII.size());
			for (size_t i = 0; i < locationsII.size(); i++)
				((CLocation&)stations[i]) = locationsII[i];
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

				callback.PushTask(GetString(IDS_CREATE_DB) + GetFileName(outputFilePath) + " (Extracting " + ToString(locations.size()) + " virtual stations)", locations.size());

				//init coord and info
				callback.PushTask("Extract weather form gribs", invalid.size()*stations.size());

				for (std::set<CTRef>::const_iterator it = invalid.begin(); it != invalid.end()&&msg; it++)
				{
					timerRead.Start();
					msg = ExtractStation(*it, gribs[*it], stations, callback);
					timerRead.Stop();
				}

				for (CWeatherStationVector::iterator it = stations.begin(); it != stations.end(); it++)
				{
					if (msg)
					{
						if (it->HaveData())
						{
							string newName = pDB->GetUniqueName(it->m_name);
							if (newName != it->m_name)
							{
								it->m_name = newName;
								it->SetDataFileName("");
							}

							//Force write file name in the file
							it->SetDataFileName(it->GetDataFileName());
							it->UseIt(true);


							timerWrite.Start();
							msg = pDB->Set(std::distance(stations.begin(), it), *it);
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

				msg += pDB->Close();
				timer.Stop();
				callback.PopTask();


				if (msg)
				{
					msg = pDB->Open(outputFilePath, CDailyDatabase::modeRead, callback);
					pDB->Close();
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

	ERMsg CCreateVirtualDB::ExtractStation(CTRef TRef, const std::string& file_path, CWeatherStationVector& stations, CCallback& callback)
	{
		ERMsg msg;
		CSfcDatasetCached sfcDS;

		msg = sfcDS.open(file_path, true);
		if (msg)
		{
			for (size_t i = 0; i < stations.size()&&msg; i++)
			{
				if (sfcDS.GetExtents().IsInside(stations[i]))
				{
					CHourlyData& data = stations[i].GetHour(TRef);
					sfcDS.get_weather(stations[i], data);

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