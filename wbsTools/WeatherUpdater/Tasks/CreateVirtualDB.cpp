#include "StdAfx.h"
#include "CreateVirtualDB.h"
#include "Basic/WeatherStation.h"
#include "Basic/DailyDatabase.h"
#include "Basic/HourlyDatabase.h"
#include "Basic/WeatherDatabaseCreator.h"
#include "Basic/Timer.h"
#include "Basic/CSV.h"
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

	const char* CCreateVirtualDB::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "GribsFilePath", "OutputFilePath", "LocationsFilePath", "OutputType", "FirstYear", "LastYear", "Incremental" };
	const size_t CCreateVirtualDB::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_FILEPATH, T_FILEPATH, T_FILEPATH, T_COMBO_INDEX, T_STRING, T_STRING, T_BOOL };
	const UINT CCreateVirtualDB::ATTRIBUTE_TITLE_ID = IDS_TOOL_CREATE_VIRTUAL_P;
	const UINT CCreateVirtualDB::DESCRIPTION_TITLE_ID = ID_TASK_CREATE_VIRTUAL;


	const char* CCreateVirtualDB::CLASS_NAME(){ static const char* THE_CLASS_NAME = "CreateVirtualDB";  return THE_CLASS_NAME; }
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
		case OUTPUT_TYPE:		str = "Hourly|Daily"; break;
	//	case FIRST_YEAR:
		//case LAST_YEAR:			str = ToString(CTRef::GetCurrentTRef().GetYear()); break;
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

		std::map<CTRef, std::string> gribs;
		msg += load_gribs(inputFilePath, gribs);

		size_t outputType = as<size_t>(OUTPUT_TYPE);
		SetFileExtension(outputFilePath, (outputType == OT_HOURLY) ? CHourlyDatabase::DATABASE_EXT : CDailyDatabase::DATABASE_EXT);

		msg = CreateMultipleDir(GetPath(outputFilePath));
		bool bIncremental = as<bool>(INCREMENTAL);

		if (!bIncremental)
		{
			if (outputType == OT_HOURLY)
				msg += CHourlyDatabase::DeleteDatabase(outputFilePath, callback);
			else
				msg += CDailyDatabase::DeleteDatabase(outputFilePath, callback);
		}


		CLocationVector locations;

		msg += locations.Load(Get(LOCATIONS_FILEPATH));

		if (msg)
		{
			//load the WeatherUpdater
			//CTaskPtr pTask = m_pProject->GetTask(UPDATER, Get(INPUT));

			//if (pTask.get() != NULL)
			//{
			//	//Get forecast if any
			//	CTaskPtr pForecastTask;
			//	if (!Get(FORECAST).empty())
			//		pForecastTask = m_pProject->GetTask(UPDATER, Get(FORECAST));

			//	
			//	string firstYear = pTask->Get("FirstYear"); ASSERT(!firstYear.empty());
			//	string lastYear = pTask->Get("LastYear");

			//	pTask->Set("FirstYear", Get("FirstYear"));
			//	pTask->Set("LastYear", Get("LastYear"));

			CTimer timer(true);
			CTimer timerRead;
			CTimer timerWrite;


			callback.AddMessage(GetString(IDS_CREATE_DB));
			callback.AddMessage(outputFilePath, 1);

			//Get the data for each station
			CWeatherDatabasePtr pDB = CreateWeatherDatabase(outputFilePath);
			if (pDB.get() == NULL)
			{
				msg.ajoute("Unknown output database type");
				return msg;

			}

			msg = pDB->Open(outputFilePath, CDailyDatabase::modeWrite);
			if (!msg)
				return msg;

			int nbStationAdded = 0;

			//StringVector stationList;
			//msg = pTask->GetStationList(stationList, callback);

			//if (msg)
			//{
			callback.PushTask(GetString(IDS_CREATE_DB) + GetFileName(outputFilePath) + " (Extracting " + ToString(locations.size()) + " virtual stations)", locations.size());


			CWeatherStationVector stations;

			for (std::map<CTRef, std::string>::const_iterator it = gribs.begin(); it != gribs.end(); it++)
			{
				CTRef TRef = it->first;
				//if (p.IsInside(TRef))
				//{
				timerRead.Start();
				msg = ExtractPoint(it->second, stations, callback);
				timerRead.Stop();
				//}
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
						msg = pDB->Add(*it);
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


		//	pTask->Set("FirstYear", firstYear);
		//	pTask->Set("LastYear", lastYear);
		//}
		//else
		//{
		//	msg.ajoute(FormatMsg(IDS_TASK_NOT_EXIST, Get(INPUT)));
		//}



		return msg;
	}

	ERMsg CCreateVirtualDB::ExtractPoint(const std::string& outputFilePath, CWeatherStationVector& stations, CCallback& callback)
	{
		ERMsg msg;
		//CPointsExtractor pointsExtractor;
//		ERMsg msg = pointsExtractor.m_options.ParseOptions(argc, argv);

		
		//msg = pointsExtractor.Execute();

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

	//void CCreateVirtualDB::CleanSparse(CWeatherStation& station)const
	//{
	//	if (!station.HaveData())
	//		return;

	//	CWVariables variables = station.GetVariables();
	//	double factor = station.IsHourly() ? 24:1;

	//	if (as<double>(MONTHLY_COMPLETENESS) > 0 )
	//	{
	//		CTPeriod p = station.GetEntireTPeriod(CTM(CTM::MONTHLY));
	//		CTRef now = CTRef::GetCurrentTRef(CTM(CTM::MONTHLY));
	//		assert(p.Begin() <= now);
	//		if (p.End() >= now)
	//			p.End() = now - 1;

	//		for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
	//		{
	//			for (TVarH v = H_FIRST_VAR; v < NB_VAR_H; v++)
	//			{
	//				if (variables[v])
	//				{
	//					double completeness = 100.0 * station[TRef][v][NB_VALUE] / (TRef.GetNbDayPerMonth()*factor);
	//					assert(completeness >= 0 && completeness <= 100);
	//					if (completeness < as<double>(MONTHLY_COMPLETENESS))
	//					{
	//						//reset month
	//						int year = TRef.GetYear();
	//						size_t m = TRef.GetMonth();
	//						for (size_t d = 0; d < TRef.GetNbDayPerMonth(); d++)
	//						{
	//							station[year][m][d][v].clear();
	//						}
	//					}
	//				}
	//			}
	//		}
	//	}

	//	if (as<double>(ANNUAL_COMPLETENESS) > 0)
	//	{
	//		station.ResetStat();
	//		CTPeriod p = station.GetEntireTPeriod(CTM(CTM::ANNUAL));
	//		CTRef now = CTRef::GetCurrentTRef(CTM(CTM::ANNUAL));
	//		assert(p.Begin() <= now);
	//		if (p.End() >= now)
	//			p.End() = now - 1;

	//		for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
	//		{
	//			for (TVarH v = H_FIRST_VAR; v < NB_VAR_H; v++)
	//			{
	//				if (variables[v])
	//				{
	//					double completeness = 100.0 * station[TRef][v][NB_VALUE] / (TRef.GetNbDaysPerYear()*factor);
	//					assert(completeness >= 0 && completeness <= 100);
	//					if (completeness < as<double>(ANNUAL_COMPLETENESS))
	//					{
	//						//reset month
	//						int year = TRef.GetYear();
	//						for (size_t m = 0; m < 12; m++)
	//						{
	//							for (size_t d = 0; d < TRef.GetNbDayPerMonth(year, m); d++)
	//							{
	//								station[year][m][d][v].clear();
	//							}
	//						}
	//					}
	//				}
	//			}
	//		}
	//	}
	//}
	//

	ERMsg CCreateVirtualDB::load_gribs(const std::string& filepath, std::map<CTRef, std::string>& gribs)
	{
		ERMsg msg;

		ifStream file;
		msg = file.open(filepath);
		if (msg)
		{
			for (CSVIterator loop(file); loop != CSVIterator() && msg; ++loop)
			{
				if ((*loop).size() == 2)
				{
					CTRef TRef;
					TRef.FromFormatedString((*loop)[0], "", "-", 1);
					assert(TRef.IsValid());

					gribs[TRef] = (*loop)[1];
				}
			}
		}

		return msg;

	}
}