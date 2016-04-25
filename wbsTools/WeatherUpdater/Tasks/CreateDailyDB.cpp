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
	const UINT CCreateDailyDB::ATTRIBUTE_TITLE_ID = IDS_TOOL_CREATE_DAILY_P;
	const UINT CCreateDailyDB::DESCRIPTION_TITLE_ID = ID_TASK_CREATE_DAILY;


	const char* CCreateDailyDB::CLASS_NAME(){ static const char* THE_CLASS_NAME = "CreateDaily";  return THE_CLASS_NAME; }
	CTaskBase::TType CCreateDailyDB::ClassType()const { return CTaskBase::TOOLS; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CCreateDailyDB::CLASS_NAME(), CCreateDailyDB::create);
	static size_t OLD_CLASS_ID = CTaskFactory::RegisterTask("DailyDatabase", (createF)CCreateDailyDB::create);
	

	CCreateDailyDB::CCreateDailyDB(void)
	{}

	CCreateDailyDB::~CCreateDailyDB(void)
	{}

	std::string CCreateDailyDB::Option(size_t i)const
	{
		string str;

		switch (i)
		{
		case INPUT:				str = GetUpdaterList(true, true, false, false);	break;
		case FORECAST:			str = GetUpdaterList(true, true, true, false); break;//forecast is only hourly for the moment
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

	
		string outputFilePath = Get(OUTPUT_FILEPATH);
		SetFileExtension(outputFilePath, CDailyDatabase::DATABASE_EXT);

		msg = CreateMultipleDir(GetPath(outputFilePath));
		msg += CDailyDatabase::DeleteDatabase(outputFilePath, callback);
		if (msg)
		{
			//load the WeatherUpdater
			CTaskPtr pTask = m_pProject->GetTask(UPDATER, Get(INPUT));

			if (pTask.get() != NULL)
			{
				//Get forecast if any
				CTaskPtr pForecastTask;
				if (!Get(FORECAST).empty())
					pForecastTask = m_pProject->GetTask(UPDATER, Get(FORECAST));

				
				string firstYear = pTask->Get("FirstYear"); ASSERT(!firstYear.empty());
				string lastYear = pTask->Get("LastYear");

				pTask->Set("FirstYear", Get("FirstYear"));
				pTask->Set("LastYear", Get("LastYear"));
			
				msg = CreateDatabase(outputFilePath, pTask, pForecastTask, callback);

				pTask->Set("FirstYear", firstYear);
				pTask->Set("LastYear", lastYear);
			}
			else
			{
				msg.ajoute(FormatMsg(IDS_TASK_NOT_EXIST, Get(INPUT)));
			}

		}
		
		return msg;
	}


	ERMsg CCreateDailyDB::CreateDatabase(const std::string& outputFilePath, CTaskPtr& pTask, CTaskPtr& pForecastTask, CCallback& callback)const
	{
		ERMsg msg;

		CTimer timer(true);
		CTimer timerRead;
		CTimer timerWrite;

		
		callback.AddMessage(GetString(IDS_CREATE_DB));
		callback.AddMessage(outputFilePath, 1);

		//Get the data for each station
		CDailyDatabase dailyDB;
		msg = dailyDB.Open(outputFilePath, CDailyDatabase::modeWrite);
		if (!msg)
			return msg;

		int nbStationAdded = 0;

		StringVector stationList;
		msg = pTask->GetStationList(stationList, callback);

		if (msg)
		{
			callback.PushTask(GetString(IDS_CREATE_DB) + outputFilePath, stationList.size());


			for (size_t i = 0; i < stationList.size() && msg; i++)
			{
				CWeatherStation station;

				timerRead.Start();
				ERMsg messageTmp = pTask->GetWeatherStation(stationList[i], CTM(CTM::DAILY), station, callback);
				timerRead.Stop();

				if (messageTmp)
				{
					CleanSparse(station);

					if (station.HaveData())
					{
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
						if (pForecastTask)
							pForecastTask->GetWeatherStation("", CTM(CTM::DAILY), station, callback);
						

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

			msg += dailyDB.Close();
			timer.Stop();
			callback.PopTask();


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

	void CCreateDailyDB::CleanSparse(CWeatherStation& station)const
	{
		if (!station.HaveData())
			return;

		CWVariables variables = station.GetVariables();

		if (as<double>(MONTHLY_COMPLETENESS) > 0 )
		{
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
		}

		if (as<double>(ANNUAL_COMPLETENESS) > 0)
		{
			station.ResetStat();
			CTPeriod p = station.GetEntireTPeriod(CTM(CTM::ANNUAL));
			CTRef now = CTRef::GetCurrentTRef(CTM(CTM::ANNUAL));
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
	}
	

}