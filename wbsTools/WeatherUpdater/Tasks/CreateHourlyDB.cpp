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

	const char* CCreateHourlyDB::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "Input", "Forecast", "LongForecast", "OutputFilepath", "FirstYear", "LastYear", "BoundingBox", "DailyCompleteness", "MonthlyCompleteness", "AnnualCompleteness" };
	const size_t CCreateHourlyDB::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_UPDATER, T_UPDATER, T_UPDATER, T_FILEPATH, T_STRING, T_STRING, T_GEORECT, T_STRING, T_STRING, T_STRING };
	const UINT CCreateHourlyDB::ATTRIBUTE_TITLE_ID = IDS_TOOL_CREATE_HOURLY_P;
	const UINT CCreateHourlyDB::DESCRIPTION_TITLE_ID = ID_TASK_CREATE_HOURLY;


	const char* CCreateHourlyDB::CLASS_NAME(){ static const char* THE_CLASS_NAME = "CreateHourly";  return THE_CLASS_NAME; }
	CTaskBase::TType CCreateHourlyDB::ClassType()const { return CTaskBase::TOOLS; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CCreateHourlyDB::CLASS_NAME(), (createF)CCreateHourlyDB::create);
	//static size_t OLD_CLASS_ID = CTaskFactory::RegisterTask("HOURLY_DB_CREATOR", (createF)CCreateHourlyDB::create);

	

	CCreateHourlyDB::CCreateHourlyDB(void)
	{}

	CCreateHourlyDB::~CCreateHourlyDB(void)
	{}


	std::string CCreateHourlyDB::Option(size_t i)const
	{
		string str;
		
		switch (i)
		{
		case INPUT:				str = GetUpdaterList(CUpdaterTypeMask(true, false, false, true)); break;
		case FORECAST1:			str = GetUpdaterList(CUpdaterTypeMask(true, false, true, true)); break;
		case FORECAST2:			str = GetUpdaterList(CUpdaterTypeMask(true, false, true, true)); break;
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
		case FIRST_YEAR:
		case LAST_YEAR:	str = ToString(CTRef::GetCurrentTRef().GetYear()); break;
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

		string outputFilePath = Get(OUTPUT_FILEPATH);
		if (outputFilePath.empty())
		{
			msg.ajoute(GetString(IDS_BSC_NAME_EMPTY));
			return msg;
		}

		SetFileExtension(outputFilePath, CHourlyDatabase::DATABASE_EXT);

		msg = CreateMultipleDir(GetPath(outputFilePath));
		msg += CHourlyDatabase::DeleteDatabase(outputFilePath, callback);
		if (msg)
		{
			//load the WeatherUpdater
			CTaskPtr pTask = m_pProject->GetTask(UPDATER, Get(INPUT));


			if (pTask.get() != NULL)
			{
				CTaskPtr pForecastTask1;
				CTaskPtr pForecastTask2;
				
				if (!Get(FORECAST1).empty())
				{
					pForecastTask1 = m_pProject->GetTask(UPDATER, Get(FORECAST1));
					if (pForecastTask1.get())
						pForecastTask1->Initialize(TOOLS, callback);
				}
				if (!Get(FORECAST2).empty())
				{
					pForecastTask2 = m_pProject->GetTask(UPDATER, Get(FORECAST2));
					if (pForecastTask2.get())
						pForecastTask2->Initialize(TOOLS, callback);
				}


				string firstYear = pTask->Get("FirstYear"); ASSERT(!firstYear.empty());
				string lastYear = pTask->Get("LastYear");

				pTask->Set("FirstYear", Get("FirstYear"));
				pTask->Set("LastYear", Get("LastYear"));

				msg = CreateDatabase(outputFilePath, pTask, pForecastTask1, pForecastTask2, callback);

				pTask->Set("FirstYear", firstYear);
				pTask->Set("LastYear", lastYear);

				if (pForecastTask1.get())
					pForecastTask1->Finalize(TOOLS, callback);

				if (pForecastTask2.get())
					pForecastTask2->Finalize(TOOLS, callback);
			}
			else
			{
				msg.ajoute(FormatMsg(IDS_TASK_NOT_EXIST, Get(INPUT)));
			}
		}

		return msg;
	}



	static size_t GetDefaultStat(size_t v)
	{
		size_t s = MEAN;
		if (v == H_PRCP || v == H_SNOW )
			s = SUM;

		return s;
	}


	void CCreateHourlyDB::CleanSparse(CWeatherStation& station)const
	{
		if (!station.HaveData())
			return;


		CWVariables variables = station.GetVariables();
		array<double, NB_VAR_H> factor;
		factor.fill(24);
		factor[H_TMIN] = 1;//Tmin and Tmax always have only one value event from hourly compilation
		factor[H_TMAX] = 1;

		double dailyCompleteness = as<double>(DAILY_COMPLETENESS);
		double monthlyCompleteness = as<double>(MONTHLY_COMPLETENESS);
		double annualCompleteness = as<double>(ANNUAL_COMPLETENESS);

		if (dailyCompleteness> 0)
		{
			CTPeriod p = station.GetEntireTPeriod(CTM(CTM::DAILY));
			assert(p.GetTM().Type() == CTM::DAILY);

			for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
			{
				CWVariablesCounter count = station[TRef].GetVariablesCount();
				for (TVarH v = H_FIRST_VAR; v < NB_VAR_H; v++)
				{
					//clear variable if not enaugh data
					if (variables[v])
					{
						//double completeness = 100.0 * station[TRef][v][NB_VALUE] / factor[v];
						double completeness = 100.0 * count[v].first / 24;
						assert(completeness >= 0 && completeness <= 100);
						if (completeness < dailyCompleteness)
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
		}
		

		if (monthlyCompleteness> 0)
		{
			
			CTPeriod p = station.GetEntireTPeriod(CTM(CTM::MONTHLY));
			CTRef now = CTRef::GetCurrentTRef(CTM(CTM::MONTHLY));
			assert(p.Begin() <= now);
			if (p.End() >= now)
				p.End() = now - 1;

			for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
			{
				CWVariablesCounter count = station[TRef].GetVariablesCount(true);
				for (TVarH v = H_FIRST_VAR; v < NB_VAR_H; v++)
				{
					if (variables[v])
					{
						//double completeness = 100.0 * station[TRef][v][NB_VALUE] / (factor[v] * TRef.GetNbDayPerMonth());
						double completeness = 100.0 * count[v].first / TRef.GetNbDayPerMonth();
						assert(completeness >= 0 && completeness <= 100);
						if (completeness < monthlyCompleteness)
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
		}

		if (annualCompleteness > 0)
		{
			CTPeriod p = station.GetEntireTPeriod(CTM(CTM::ANNUAL));
			CTRef now = CTRef::GetCurrentTRef(CTM(CTM::ANNUAL));
			assert(p.Begin() <= now);
			if (p.End() >= now)
				p.End() = now - 1;

			for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
			{
				CWVariablesCounter count = station[TRef].GetVariablesCount(true);
				for (TVarH v = H_FIRST_VAR; v < NB_VAR_H; v++)
				{
					if (variables[v])
					{
						double completeness = 100.0 * count[v].first / TRef.GetNbDaysPerYear();
						//double completeness = 100.0 * station[TRef][v][NB_VALUE] / (factor[v] * TRef.GetNbDaysPerYear());
						assert(completeness >= 0 && completeness <= 100);
						if (completeness < annualCompleteness)
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

			station.ResetStat();
		}

	}

	static string GetOutputFilePath(string filePath, string ext)
	{
		SetFileExtension(filePath, ext);
		return filePath;
	}

	ERMsg CCreateHourlyDB::CreateDatabase(const std::string& outputFilePath, CTaskPtr& pTask, CTaskPtr& pForecastTask1, CTaskPtr& pForecastTask2, CCallback& callback)const
	{
		ERMsg msg;

		CTimer timer(true);
		CTimer timerRead;
		CTimer timerWrite;


		pTask->Initialize(TOOLS, callback);

		string hourlyDBFilePath = GetOutputFilePath(Get(OUTPUT_FILEPATH), CHourlyDatabase::DATABASE_EXT);
		msg += CHourlyDatabase::DeleteDatabase(hourlyDBFilePath, callback);
		Sleep(500);

		msg += CreateMultipleDir(GetPath(hourlyDBFilePath));


		callback.AddMessage(GetString(IDS_CREATE_DB));
		callback.AddMessage(hourlyDBFilePath, 1);

		CHourlyDatabase DB;
		msg += DB.Open(hourlyDBFilePath, CHourlyDatabase::modeWrite);
		assert(DirectoryExists(DB.GetDataPath(hourlyDBFilePath)));

		if (!msg)
			return msg;


		CStatistic::SetVMiss(-999);
		int nbStationAdded = 0;



		//find all station in the directories
		StringVector stationList;
		msg = pTask->GetStationList(stationList, callback);


		if (msg)
		{
			string comment = GetString(IDS_CREATE_DB) + GetFileName(hourlyDBFilePath) + " (Extracting " + ToString(stationList.size()) + " stations)";
			callback.PushTask(comment, stationList.size());
			callback.AddMessage(comment);

			for (size_t i = 0; i < stationList.size() && msg; i++)
			{
				CWeatherStation station(true);

				timerRead.Start();
				ERMsg messageTmp = pTask->GetWeatherStation(stationList[i], CTM(CTM::HOURLY), station, callback);
				timerRead.Stop();

				if (messageTmp)
				{
					CleanSparse(station);


					if (station.HaveData())
					{
						ASSERT(!station.m_name.empty());
						string newName = DB.GetUniqueName(station.m_ID, station.m_name);
						if (newName != station.m_name)
						{
							station.m_name = newName;
							station.SetDataFileName("");
						}
						
						//force the creation of the DataFileName column
						station.SetDataFileName(station.GetDataFileName());
						station.UseIt(true);

						//Get forecast
						if (pForecastTask1)
							pForecastTask1->GetWeatherStation("", CTM(CTM::HOURLY), station, callback);

						if (pForecastTask2)
							pForecastTask2->GetWeatherStation("", CTM(CTM::HOURLY), station, callback);

						timerWrite.Start();
						messageTmp += DB.Add(station);
						timerWrite.Stop();

						if (messageTmp)
							nbStationAdded++;
					}
				}//if msg
				


				if (!messageTmp)
					callback.AddMessage(messageTmp, 1);
				
				msg += callback.StepIt();
			}//for all station

			DB.Close();
			timer.Stop();
			callback.PopTask();

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
		}

		pTask->Finalize(TOOLS, callback);

		return msg;
	}


}