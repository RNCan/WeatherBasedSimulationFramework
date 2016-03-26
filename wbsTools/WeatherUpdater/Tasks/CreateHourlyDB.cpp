#include "StdAfx.h"
#include "CreateHourlyDB.h"
#include "Basic/WeatherStation.h"
#include "Basic/HourlyDatabase.h"
#include "Basic/Timer.h"
#include "UI/Common/SYShowMessage.h"
#include "TaskFactory.h"
#include "../resource.h"
#include "WeatherBasedSimulationString.h"


using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace WBSF::WEATHER;

namespace WBSF
{


	const int DEW_HOUR = 9;
	//*********************************************************************

	const char* CCreateHourlyDB::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "Input", "Forecast", "OutputFilepath", "FirstYear", "LastYear", "BoundingBox", "DailyCompleteness", "MonthlyCompleteness", "AnnualCompleteness" };
	const size_t CCreateHourlyDB::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_UPDATER, T_UPDATER, T_FILEPATH, T_STRING, T_STRING, T_GEORECT, T_STRING, T_STRING, T_STRING };
	const StringVector CCreateHourlyDB::ATTRIBUTE_TITLE(IDS_TOOL_CREATE_HOURLY_P, "|;");

	const char* CCreateHourlyDB::CLASS_NAME(){ static const char* THE_CLASS_NAME = "CreateHourly";  return THE_CLASS_NAME; }
	CTaskBase::TType CCreateHourlyDB::ClassType()const { return CTaskBase::TOOLS; }
	static size_t CLASS_ID = CTaskFactory::RegisterClass(CCreateHourlyDB::CLASS_NAME(), CCreateHourlyDB::create);
	static size_t OLD_CLASS_ID = CTaskFactory::RegisterClass("HOURLY_DB_CREATOR", CCreateHourlyDB::create);

	

	CCreateHourlyDB::CCreateHourlyDB(void)
	{}

	CCreateHourlyDB::~CCreateHourlyDB(void)
	{}


	std::string CCreateHourlyDB::Option(size_t i)const
	{
		string str;

		switch (i)
		{
		case INPUT:				str = GetUpdaterList(false); break;
		case FORECAST:			str = GetUpdaterList(true); break;
		case OUTPUT_FILEPATH:	str = GetString(IDS_STR_FILTER_HOURLY); break;
		case FIRST_YEAR:
		case LAST_YEAR:			str = ToString(CTRef::GetCurrentTRef().GetYear()); break;
		};

		return str;
	}

	std::string CCreateHourlyDB::Default(size_t i)const
	{
		string str;
		
		switch (i)
		{
		case DAILY_COMPLETENESS:	str = "50"; break;
		case MONTHLY_COMPLETENESS:	str = "80"; break;
		case ANNUAL_COMPLETENESS:	str = "25"; break;
		};

		return str;
	}

	ERMsg CCreateHourlyDB::Execute(CCallback& callback)
	{
		ASSERT(m_pProject);//parent must be set for creator

		ERMsg msg;

		//load the WeatherUpdater
		CTaskPtr pTask = m_pProject->GetTask(UPDATER, Get(INPUT));
		

		if (pTask.get() != NULL)
		{
			string firstYear = pTask->Get("FIRST_YEAR");
			string lastYear = pTask->Get("LAST_YEAR");
			msg = CreateDatabase(*pTask, callback);
			pTask->Set("FIRST_YEAR", firstYear);
			pTask->Set("LAST_YEAR", lastYear);
		}
		else
		{
			msg.ajoute(FormatMsg(IDS_TASK_NOT_EXIST, Get(INPUT)));
		}

		return msg;
	}



	static size_t GetDefaultStat(size_t v)
	{
		size_t s = MEAN;
		if (v == H_PRCP || v == H_SNOW || v == H_SRAD)
			s = SUM;

		return s;
	}


	void CCreateHourlyDB::CleanSparse(CWeatherStation& station)const
	{
		if (!station.HaveData())
			return;


		CWVariables variables = station.GetVariables();

		CTPeriod p = station.GetEntireTPeriod(CTM(CTM::DAILY));
		assert(p.GetTM().Type() == CTM::DAILY);

		for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
		{
			for (TVarH v = H_TAIR; v < NB_VAR_H; v++)
			{
				//clear variable if not enaugh data
				if (variables[v])
				{
					double completeness = 100.0 * station[TRef][v][NB_VALUE] / 24;
					assert(completeness >= 0 && completeness <= 100);
					if (completeness < as<double>(DAILY_COMPLETENESS))
					{
						//reset daily data
						int year = TRef.GetYear();
						size_t m = TRef.GetMonth();
						size_t d = TRef.GetDay();
						for (size_t h = 0; h < 24; h++)
							station[year][m][d][h][v] = WEATHER::MISSING;
					}
				}
			}
		}

		station.ResetStat();

		p = station.GetEntireTPeriod(CTM(CTM::MONTHLY));
		CTRef now = CTRef::GetCurrentTRef(CTM(CTM::MONTHLY));
		assert(p.Begin() <= now);
		if (p.End() >= now)
			p.End() = now - 1;

		for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
		{
			for (TVarH v = H_TAIR; v < NB_VAR_H; v++)
			{
				if (variables[v])
				{
					double completeness = 100.0 * station[TRef][v][NB_VALUE] / (24 * TRef.GetNbDayPerMonth());
					assert(completeness >= 0 && completeness <= 100);
					if (completeness < as<double>(MONTHLY_COMPLETENESS))
					{
						//reset month
						int year = TRef.GetYear();
						size_t m = TRef.GetMonth();
						for (size_t d = 0; d < TRef.GetNbDayPerMonth(); d++)
						{
							for (size_t h = 0; h < 24; h++)
								station[year][m][d][h][v] = WEATHER::MISSING;
						}
					}
				}
			}
		}

		station.ResetStat();

		p = station.GetEntireTPeriod(CTM(CTM::ANNUAL));
		now = CTRef::GetCurrentTRef(CTM(CTM::ANNUAL));
		assert(p.Begin() <= now);
		if (p.End() >= now)
			p.End() = now - 1;

		for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
		{
			for (TVarH v = H_TAIR; v < NB_VAR_H; v++)
			{
				if (variables[v])
				{
					double completeness = 100.0 * station[TRef][v][NB_VALUE] / (24 * TRef.GetNbDaysPerYear());
					assert(completeness >= 0 && completeness <= 100);
					if (completeness < as<double>(ANNUAL_COMPLETENESS))
					{
						//reset month
						int year = TRef.GetYear();
						for (size_t m = 0; m < 12; m++)
						{
							for (size_t d = 0; d < TRef.GetNbDayPerMonth(year, m); d++)
							{
								for (size_t h = 0; h < 24; h++)
									station[year][m][d][h][v] = WEATHER::MISSING;
							}
						}
					}
				}
			}
		}

	}

	static string GetOutputFilePath(string filePath, string ext)
	{
		SetFileExtension(filePath, ext);
		return filePath;
	}

	ERMsg CCreateHourlyDB::CreateDatabase(CTaskBase& task, CCallback& callback)const
	{
		ERMsg msg;

		CTimer timer(true);
		CTimer timerRead;
		CTimer timerWrite;


		/*CTask forecastTask;
		if (!m_forecastName.empty())
		{

			msg = forecastTask.LoadFromDoc(m_forecastName);

			if (msg)
			{
				CTaskBase& forecast = dynamic_cast<CTaskBase&>(forecastTask.GetP());
				msg = forecast.PreProcess(callback);
			}


			if (!msg)
				return msg;

		}*/

		//StringVector includeStation(Get(INCLUDE_STATIONS), ",;|");
		//StringVector exludeStation(Get(EXCLUDE_STATIONS), ",;|");


		string hourlyDBFilePath = GetOutputFilePath(Get(OUTPUT_FILEPATH), CHourlyDatabase::DATABASE_EXT);
		msg += CHourlyDatabase::DeleteDatabase(hourlyDBFilePath, callback);
		Sleep(500);

		msg += CreateMultipleDir(GetPath(hourlyDBFilePath));


		callback.AddMessage(GetString(IDS_CREATE_DATABASE));
		callback.AddMessage(hourlyDBFilePath, 1);

		CHourlyDatabase DB;
		msg += DB.Open(hourlyDBFilePath, CHourlyDatabase::modeWrite);
		assert(DirectoryExists(GetPath(hourlyDBFilePath) + GetFileTitle(hourlyDBFilePath) + "\\"));

		/*CLocationVector loc;
		if (msg && !m_locIncludeStation.empty())
			msg += loc.Load(m_locIncludeStation);*/

		if (!msg)
			return msg;



		CStatistic::SetVMiss(-999);
		int nbStationAdded = 0;


		callback.SetCurrentDescription("Load stations list");
		callback.SetNbStep(2);


		//find all station in the directories
		StringVector stationList;
		//msg = weatherUpdater.PreProcess(callback);
		//msg += callback.StepIt();
		//if (msg)
		//{
		msg = task.GetStationList(stationList, callback);
		msg += callback.StepIt();
		//}

		if (!msg)
			return msg;

	/*	if (!loc.empty())
		{
			for (StringVector::iterator it = stationList.begin(); it != stationList.end();)
			{
				string ID = task.GetStationIDFromName(*it);
				if (loc.FindByID(ID) == -1)
					it = stationList.erase(it);
				else
					it++;
			}
		}
*/
		callback.SetCurrentDescription(GetString(IDS_CREATE_DATABASE));
		callback.AddTask(1);
		callback.SetNbStep(stationList.size());
		callback.AddMessage("Extracting " + ToString(stationList.size()) + " stations");

		for (size_t i = 0; i < stationList.size() && msg; i++)
		{

			CWeatherStation station(true);

			timerRead.Start();
			ERMsg messageTmp = task.GetWeatherStation(stationList[i], CTM(CTM::HOURLY), station, callback);
			timerRead.Stop();

			if (messageTmp)
			{
				//if (m_bClearSparse)
				CleanSparse(station);


				if (station.HaveData())
				{
					ASSERT(!station.m_name.empty());
					string newName = DB.GetUniqueName(station.m_name);
					if (newName != station.m_name)
					{
						station.m_name = newName;
						station.SetDataFileName("");
					}

					//if (std::find(exludeStation.begin(), exludeStation.end(), station.m_ID) == exludeStation.end())
					//{
					station.UseIt(true);

					//Get forecast
					//if (!m_forecastName.empty())
					//{
					//	CTaskBase& forecast = dynamic_cast<CTaskBase&>(forecastTask.GetP());
					//	forecast.GetWeatherStation("", CTM(CTM::HOURLY), station, callback);
					//}
					/*}
					else
					{
						station.UseIt(false);
						callback.AddMessage("Station " + station.m_name + " (" + station.m_ID + ") was exclude by user", 1);
					}
*/
					timerWrite.Start();
					messageTmp += DB.Add(station);
					timerWrite.Stop();

					if (messageTmp)
						nbStationAdded++;
				}
			}//if msg


			if (!messageTmp)
				callback.AddMessage(messageTmp, 1);

			if (msg)
				msg += callback.StepIt();
		}//for all station

		DB.Close();
		timer.Stop();


		if (msg)
		{
			msg = DB.Open(hourlyDBFilePath, CHourlyDatabase::modeRead, callback);
			DB.Close();
		}


		if (msg)
		{
			callback.AddMessage(GetString(IDS_STATION_ADDED) + ToString(nbStationAdded), 1);
			callback.AddMessage(FormatMsg(IDS_BSC_TIME_READ, SecondToDHMS(timerRead.Elapsed())));
			callback.AddMessage(FormatMsg(IDS_BSC_TIME_WRITE, SecondToDHMS(timerWrite.Elapsed())));
			callback.AddMessage(FormatMsg(IDS_BSC_TOTAL_TIME, SecondToDHMS(timer.Elapsed())));
		}


		return msg;
	}


}