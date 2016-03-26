#include "StdAfx.h"
#include "CreateDailyDB.h"
#include "Basic/WeatherStation.h"
#include "Basic/DailyDatabase.h"
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
	//*********************************************************************

	const char* CCreateDailyDB::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "Input", "Forecast", "OutputFilePath", "FirstYear", "LastYear", "BoundingBox", "MonthlyCompleteness", "AnnualCompleteness" };
	const size_t CCreateDailyDB::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_UPDATER, T_UPDATER, T_FILEPATH, T_STRING, T_STRING, T_GEORECT, T_STRING, T_STRING };
	const StringVector CCreateDailyDB::ATTRIBUTE_TITLE(IDS_TOOL_CREATE_DAILY_P, "|;");

	const char* CCreateDailyDB::CLASS_NAME(){ static const char* THE_CLASS_NAME = "CreateDaily";  return THE_CLASS_NAME; }
	CTaskBase::TType CCreateDailyDB::ClassType()const { return CTaskBase::TOOLS; }
	static size_t CLASS_ID = CTaskFactory::RegisterClass(CCreateDailyDB::CLASS_NAME(), CCreateDailyDB::create);
	static size_t OLD_CLASS_ID = CTaskFactory::RegisterClass("DailyDatabase", CCreateDailyDB::create);
	

	CCreateDailyDB::CCreateDailyDB(void)
	{}

	CCreateDailyDB::~CCreateDailyDB(void)
	{}

	std::string CCreateDailyDB::Option(size_t i)const
	{
		string str;

		switch (i)
		{
		case INPUT:				str = GetUpdaterList(false); break;
		case FORECAST:			str = GetUpdaterList(true); break;
		case OUTPUT_FILEPATH:	str = GetString(IDS_STR_FILTER_DAILY); break;
		case FIRST_YEAR:
		case LAST_YEAR:			str = ToString(CTRef::GetCurrentTRef().GetYear()); break;

		};

		return str;
	}

	std::string CCreateDailyDB::Default(size_t i)const
	{
		string str;

		switch (i)
		{
		case MONTHLY_COMPLETENESS:	str = "80"; break;
		case ANNUAL_COMPLETENESS:	str = "25"; break;
		};

		return str;
	}

	ERMsg CCreateDailyDB::Execute(CCallback& callback)
	{
		ASSERT(m_pProject);//parent must be set for creator

		ERMsg msg;

		callback.PushLevel();

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


		callback.PopLevel();
		return msg;
	}


	static size_t GetDefaultStat(size_t v)
	{
		size_t s = MEAN;
		if (v == H_PRCP || v == H_SNOW || v == H_SRAD)
			s = SUM;

		return s;
	}

	void CCreateDailyDB::CleanSparse(CWeatherStation& station)const
	{
		CWVariables variables = station.GetVariables();

		CTPeriod p = station.GetEntireTPeriod(CTM(CTM::MONTHLY));
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
					double completeness = 100.0 * station[TRef][v][NB_VALUE] / TRef.GetNbDayPerMonth();
					assert(completeness >= 0 && completeness <= 100);
					if (completeness < as<double>(MONTHLY_COMPLETENESS))
					{
						//reset month
						int year = TRef.GetYear();
						size_t m = TRef.GetMonth();
						for (size_t d = 0; d < TRef.GetNbDayPerMonth(); d++)
						{
							station[year][m][d][v].clear();
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
					double completeness = 100.0 * station[TRef][v][NB_VALUE] / TRef.GetNbDaysPerYear();
					assert(completeness >= 0 && completeness <= 100);
					if (completeness < as<double>(ANNUAL_COMPLETENESS))
					{
						//reset month
						int year = TRef.GetYear();
						for (size_t m = 0; m < 12; m++)
						{
							for (size_t d = 0; d < TRef.GetNbDayPerMonth(year, m); d++)
							{
								station[year][m][d][v].clear();
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


	ERMsg CCreateDailyDB::CreateDatabase(CTaskBase& task, CCallback& callback)const
	{
		ERMsg msg;

		CTimer timer(true);
		CTimer timerRead;
		CTimer timerWrite;

		//CTaskBase forecastTask;
		//if (!Get(FORECAST).empty())
		//{
		//	msg = forecastTask.LoadFromDoc(m_forecastName);

		//	if (msg)
		//	{
		//		CTaskBase& forecast = dynamic_cast<CTaskBase&>(forecastTask.GetP());
		//		//msg = forecast.PreProcess(callback);
		//	}


		//	if (!msg)
		//		return msg;
		//}

		string outputFilePath = GetOutputFilePath(Get(OUTPUT_FILEPATH), CDailyDatabase::DATABASE_EXT);
		CreateMultipleDir(GetPath(outputFilePath));

		//StringVector exludeStation(m_exludeStation, ";|");

		callback.AddMessage(GetString(IDS_CREATE_DATABASE));
		callback.AddMessage(outputFilePath, 1);

		//Get the data for each station
		CDailyDatabase dailyDB;
		msg = dailyDB.Open(outputFilePath, CDailyDatabase::modeWrite);
		if (!msg)
			return msg;

		int nbStationAdded = 0;

		callback.SetCurrentDescription("Load stations list");
		callback.SetNbStep(2);

		//find all station in the directories
		//StringVector allStation;
		//msg = weatherUpdater.PreProcess(callback);
		//msg += callback.StepIt();
		//if (msg)
		//{

		StringVector stationList;
		msg = task.GetStationList(stationList, callback);
		msg += callback.StepIt();


		if (msg)
		{
			callback.SetCurrentDescription(GetString(IDS_CREATE_DATABASE) + outputFilePath);
			callback.SetNbStep(stationList.size());


			for (size_t i = 0; i < stationList.size() && msg; i++)
			{
				CWeatherStation station;

				timerRead.Start();
				ERMsg messageTmp = task.GetWeatherStation(stationList[i], CTM(CTM::DAILY), station, callback);
				timerRead.Stop();

				if (messageTmp)
				{
					//if (m_bClearSparse)
					CleanSparse(station);
					//RemoveInvalidYear(station);

					if (station.HaveData())
					{
						//if (std::find(exludeStation.begin(), exludeStation.end(), station.m_ID.c_str()) == exludeStation.end())
						//{
						string newName = dailyDB.GetUniqueName(station.m_name);
						if (newName != station.m_name)
						{
							station.m_name = newName;
							station.SetDataFileName("");
						}

						//Force write file name in the file
						station.SetDataFileName(station.GetDataFileName());
						station.UseIt(true);

						//Get forecast
					/*	if (!m_forecastName.empty())
						{
							CTaskBase& forecast = dynamic_cast<CTaskBase&>(forecastTask.GetP());
							forecast.GetWeatherStation("", CTM(CTM::DAILY), station, callback);
						}*/
						//}
						//else
						//{
						//	station.UseIt(false);
						//	callback.AddMessage( "Station "+ station.m_name + " (" + station.m_ID + ") was exclude by user", 1);
						//}

						timerWrite.Start();
						messageTmp = dailyDB.Add(station);
						timerWrite.Stop();

						if (messageTmp)
							nbStationAdded++;
					}
				}

				if (!messageTmp)
					callback.AddMessage(messageTmp, 1);

				msg += callback.StepIt();

			}
		}
	

		msg += dailyDB.Close();
		timer.Stop();

		if (msg)
		{
			msg = dailyDB.Open(outputFilePath, CDailyDatabase::modeRead, callback);
			dailyDB.Close();
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