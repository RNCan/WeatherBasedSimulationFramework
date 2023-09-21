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

	const char* CCreateDailyDB::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "Input", "Forecast", "LongForecast","OutputFilePath", "FirstYear", "LastYear", "BoundingBox", "MonthlyCompleteness", "AnnualCompleteness", "Validation" };
	const size_t CCreateDailyDB::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_UPDATER, T_UPDATER, T_UPDATER, T_FILEPATH, T_STRING, T_STRING, T_GEORECT, T_STRING, T_STRING, T_BOOL };
	const UINT CCreateDailyDB::ATTRIBUTE_TITLE_ID = IDS_TOOL_CREATE_DAILY_P;
	const UINT CCreateDailyDB::DESCRIPTION_TITLE_ID = ID_TASK_CREATE_DAILY;


	const char* CCreateDailyDB::CLASS_NAME() { static const char* THE_CLASS_NAME = "CreateDaily";  return THE_CLASS_NAME; }
	CTaskBase::TType CCreateDailyDB::ClassType()const { return CTaskBase::TOOLS; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CCreateDailyDB::CLASS_NAME(), CCreateDailyDB::create);
	//static size_t OLD_CLASS_ID = CTaskFactory::RegisterTask("DailyDatabase", (createF)CCreateDailyDB::create);


	CCreateDailyDB::CCreateDailyDB(void)
	{}

	CCreateDailyDB::~CCreateDailyDB(void)
	{}

	std::string CCreateDailyDB::Option(size_t i)const
	{
		string str;

		switch (i)
		{
		case INPUT:				str = GetUpdaterList(CUpdaterTypeMask(false, true, false, true)); break;
		case FORECAST1:			str = GetUpdaterList(CUpdaterTypeMask(false, true, true, true)); break;
		case FORECAST2:			str = GetUpdaterList(CUpdaterTypeMask(false, true, true, true)); break;
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
		case FIRST_YEAR:
		case LAST_YEAR:	str = ToString(CTRef::GetCurrentTRef().GetYear()); break;
		case MONTHLY_COMPLETENESS:	str = "80"; break;
		case ANNUAL_COMPLETENESS:	str = "25"; break;
		case VALIDATION: str = "1"; break;
		};

		return str;
	}

	ERMsg CCreateDailyDB::Execute(CCallback& callback)
	{
		ASSERT(m_pProject);//parent must be set for creator

		ERMsg msg;


		string outputFilePath = Get(OUTPUT_FILEPATH);
		if (outputFilePath.empty())
		{
			msg.ajoute(GetString(IDS_BSC_NAME_EMPTY));
			return msg;
		}


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


				string firstYear = pTask->Get("FirstYear");
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


	ERMsg CCreateDailyDB::CreateDatabase(const std::string& outputFilePath, CTaskPtr& pTask, CTaskPtr& pForecastTask1, CTaskPtr& pForecastTask2, CCallback& callback)const
	{
		ERMsg msg;

		CTimer timer(true);
		CTimer timerRead;
		CTimer timerWrite;

		pTask->Initialize(TOOLS, callback);

		callback.AddMessage(GetString(IDS_CREATE_DB));
		callback.AddMessage(outputFilePath, 1);

		//Get the data for each station
		CDailyDatabase dailyDB;
		msg = dailyDB.Open(outputFilePath, CDailyDatabase::modeWrite);

		ofStream log_file;
		if (msg && as<bool>(VALIDATION))
		{
			string log_filepath = outputFilePath;
			SetFileExtension(log_filepath, ".validation.csv");
			
			if (msg)
			{
				msg = log_file.open(log_filepath);
				if (msg)
					log_file << "ID,Name,Date,Variable,Error,Value" << endl;
			}
		}

		
		if (!msg)
			return msg;

		int nbStationAdded = 0;

		StringVector stationList;
		msg = pTask->GetStationList(stationList, callback);

		if (msg)
		{
			callback.PushTask(GetString(IDS_CREATE_DB) + GetFileName(outputFilePath) + " (Extracting " + ToString(stationList.size()) + " stations)", stationList.size());


			for (size_t i = 0; i < stationList.size() && msg; i++)
			{
				CWeatherStation station;

				timerRead.Start();
				ERMsg messageTmp = pTask->GetWeatherStation(stationList[i], CTM(CTM::DAILY), station, callback);
				timerRead.Stop();

				if (messageTmp)
				{
					//clean-up all SSI values
					string provider = station.GetSSI("Provider");
					string network = station.GetSSI("Network");
					string country = station.GetSSI("Country");
					string subDivisions = station.GetSSI("SubDivision");
					station.m_siteSpeceficInformation.clear();
					station.SetSSI("Provider", provider);
					station.SetSSI("Network", network);
					station.SetSSI("Country", country);
					station.SetSSI("SubDivision", subDivisions);


					if (station.IsHourly())
					{
						//transform hourly data to daily data
						station.GetStat(H_TAIR);//compute daily stat
						station.SetHourly(false);//remove hourly values
					}
					
					if(as<bool>(VALIDATION))
						msg += BasicValidation(station, log_file, callback);


					if(msg)
						CleanSparse(station);

					if (msg && station.HaveData())
					{
						station.m_name = WBSF::UTF8_ANSI(station.m_name);//try to remove UTF8 characters
						station.m_name = RemoveAccented(station.m_name);//remove all accent characters;

						//remove the added number "2" at the end of the name
						if (!station.m_name.empty() && station.m_name.back() == '2')
							station.m_name = station.m_name.substr(0, station.m_name.length() - 1);

						

						string newName = dailyDB.GetUniqueName(station.m_ID, station.m_name);
						if (newName != station.m_name)
						{
							station.m_name = newName;
							station.SetDataFileName("");
						}


						
						//Force write file name in the file
						station.SetDataFileName(station.GetDataFileName());
						station.UseIt(true);

						//Get forecast
						if (pForecastTask1)
							msg += pForecastTask1->GetWeatherStation("", CTM(CTM::DAILY), station, callback);

						if (pForecastTask2)
							msg += pForecastTask2->GetWeatherStation("", CTM(CTM::DAILY), station, callback);


						timerWrite.Start();
						messageTmp = dailyDB.Add(station);
						timerWrite.Stop();

						if (messageTmp)
							nbStationAdded++;
					}
				}
				else
				{
					if (callback.GetUserCancel())
						msg += messageTmp;
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

		if (as<bool>(VALIDATION))
			log_file.close();

		pTask->Finalize(TOOLS, callback);

		return msg;
	}



	static size_t GetDefaultStat(size_t v)
	{
		size_t s = MEAN;
		if (v == H_PRCP || v == H_SNOW)
			s = SUM;

		return s;
	}


	void CCreateDailyDB::CleanSparse(CWeatherStation& station)const
	{
		if (!station.HaveData())
			return;

		CWVariables variables = station.GetVariables();
		


		double monthlyCompleteness = as<double>(MONTHLY_COMPLETENESS);
		double annualCompleteness = as<double>(ANNUAL_COMPLETENESS);


		if (monthlyCompleteness > 0)
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
						double completeness = 100.0 * count[v].first / TRef.GetNbDayPerMonth();
						assert(completeness >= 0 && completeness <= 100);
						if (completeness < monthlyCompleteness)
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
		}

		if (annualCompleteness > 0)
		{
			
			CTPeriod p = station.GetEntireTPeriod(CTM(CTM::ANNUAL));
			CTRef this_year = CTRef::GetCurrentTRef(CTM(CTM::ANNUAL));
			CTRef today = CTRef::GetCurrentTRef(CTM(CTM::DAILY));
			assert(p.Begin() <= this_year);
			//if (p.End() >= now)
				//p.End() = now - 1;

			for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
			{
				size_t NbDaysPerYear = TRef >= this_year ? (today.GetJDay() + 1) : TRef.GetNbDaysPerYear();

				CWVariablesCounter count = station[TRef].GetVariablesCount(true);
				for (TVarH v = H_FIRST_VAR; v < NB_VAR_H; v++)
				{
					if (variables[v])
					{
						double completeness = 100.0 * count[v].first / NbDaysPerYear;
						assert(completeness >= 0 && completeness <= 100);
						if (completeness < annualCompleteness)
						{
							bool bClear = true;
							if (TRef == this_year)
							{
								
								if (today.GetJDay() < 15)//check for minimum days after the 15 of january
								//{
									//minimum of 7 days
								//	if( count[v].first >= 7)
									//	bClear = false;
								//}
								//else
								{
									bClear = false;
								}
								
							}
							//reset year
							if (bClear)
							{
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

			station.ResetStat();
		}

	}

	
	ERMsg CCreateDailyDB::BasicValidation(CWeatherStation& station, ofstream& log_file, CCallback& callback)const
	{
		ASSERT(station.IsDaily());
		ERMsg msg;

		bool bInvalidData = false;
		for (size_t y = 0; y < station.size() && msg; y++)
		{
			for (size_t m = 0; m < station[y].size() && msg; m++)//for all months
			{
				for (size_t d = 0; d < station[y][m].size() && msg; d++)//for all days
				{
					for (TVarH v = H_FIRST_VAR; v < NB_VAR_H && msg; v++)//for all variables
					{

						if (station[y][m][d][v].IsInit())
						{
							float value = station[y][m][d][v][MEAN];

							if (v == H_RELH && value > 100 && value <= 105)
							{
								value = 100;
								station[y][m][d].SetStat(v, value);
							}

							if (v == H_SNDH && value < 0)
							{
								value = 0;
								station[y][m][d].SetStat(v, value);
							}
							
							bool bValid = IsBasicCheckValid(v, value);
							if (!bValid)
							{
								log_file << station.m_ID << "," << station.m_name << "," << station[y][m][d].GetTRef().GetFormatedString() << "," << GetVariableAbvr(v) << "," << "Outliers" << "," << ToString(value) << endl;
								station[y][m][d].SetStat(v, CStatistic());//reset
								bInvalidData = true;
							}
							else
							{
								if (v == H_TMIN || v == H_TAIR || v == H_TMAX)
								{
									//Value(0) is at least 25°C larger or at least 25°C smaller than value(21) and value(1)
									const CWeatherDay& prev = station[y][m][d].GetPrevious();
									const CWeatherDay& next = station[y][m][d].GetNext();
									if (prev[v].IsInit() && next[v].IsInit())
									{
										if(abs(value-prev[v][MEAN])>25 && abs(value - next[v][MEAN]) > 25)
										{
											//string warning = "Warning: spike data" + station.m_name + " (" + station.m_ID + ") " + station[y][m][d].GetTRef().GetFormatedString() + " " + GetVariableAbvr(v) + " " + ToString(value);
											//callback.AddMessage(warning);

											log_file << station.m_ID << "," << station.m_name << "," << station[y][m][d].GetTRef().GetFormatedString() << "," << GetVariableAbvr(v) << "," << "Spike" << "," << ToString(value) << endl;
											station[y][m][d].SetStat(H_TMIN, CStatistic());//reset
											station[y][m][d].SetStat(H_TAIR, CStatistic());//reset
											station[y][m][d].SetStat(H_TMAX, CStatistic());//reset
											bInvalidData = true;

										}
										else
										{
											bool bValid = true;
											if (v == H_TMIN && prev[H_TAIR].IsInit() && next[H_TAIR].IsInit())
											{
												if (value <= min(prev[H_TAIR][MEAN], next[H_TAIR][MEAN]) - 40)
													bValid = false;
											}
											
											if (bValid && v == H_TMIN && prev[H_TMAX].IsInit() && next[H_TMAX].IsInit())
											{
												if (value <= min(prev[H_TMAX][MEAN], next[H_TMAX][MEAN]) - 40)
													bValid = false;
											}

											if (bValid && v == H_TAIR && prev[H_TMIN].IsInit() && next[H_TMIN].IsInit())
											{
												if (value >= max(prev[H_TMIN][MEAN], next[H_TMIN][MEAN]) + 40)
													bValid = false;
											}
											if (bValid && v == H_TAIR && prev[H_TMAX].IsInit() && next[H_TMAX].IsInit())
											{
												if (value <= min(prev[H_TMAX][MEAN], next[H_TMAX][MEAN]) - 40)
													bValid = false;
											}

											if (bValid && v == H_TMAX && prev[H_TMIN].IsInit() && next[H_TMIN].IsInit())
											{
												if (value >= max(prev[H_TMIN][MEAN], next[H_TMIN][MEAN]) + 40)
													bValid = false;
											}

											if (bValid && v == H_TMAX && prev[H_TAIR].IsInit() && next[H_TAIR].IsInit())
											{
												if (value >= max(prev[H_TAIR][MEAN], next[H_TAIR][MEAN]) + 40)
													bValid = false;
											}

											if( !bValid )
											{
												log_file << station.m_ID << "," << station.m_name << "," << station[y][m][d].GetTRef().GetFormatedString() << "," << GetVariableAbvr(v) << "," << "Inconsistency" << "," << ToString(value) << endl;
												//string warning = "Warning: spike data" + station.m_name + " (" + station.m_ID + ") " + station[y][m][d].GetTRef().GetFormatedString() + " " + GetVariableAbvr(v) + " " + ToString(value);
												//callback.AddMessage(warning);

												station[y][m][d].SetStat(H_TMIN, CStatistic());//reset
												station[y][m][d].SetStat(H_TAIR, CStatistic());//reset
												station[y][m][d].SetStat(H_TMAX, CStatistic());//reset
												bInvalidData = true;
											}
										}
									}
								}


								
							}
						}

						msg += callback.StepIt(0);
					}
				}//for all days
			}//for all months

			//msg += callback.StepIt();
		}//for all years

		if (bInvalidData)
		{
			string warning = "Warning: invalid data for station: " + station.m_name + " (" + station.m_ID + ") ";
			callback.AddMessage(warning);
		}

		return msg;
	}

	bool CCreateDailyDB::IsBasicCheckValid(size_t v, float value)
	{
		bool bValid = true;

		switch (v)
		{
		case H_TMIN:
		case H_TAIR:
		case H_TMAX:
		case H_TDEW: bValid = value >= -60 && value <= 60; break;
		case H_PRCP: bValid = value >= 0 && value <= 250; break;
		case H_RELH: bValid = value >= 0 && value <= 100; break;
		case H_WNDS:
		case H_WND2: bValid = value >= 0 && value <= 200; break;
		case H_WNDD: bValid = value >= 0 && value <= 360; break;
		case H_SRAD: bValid = value >= 0 && value <= 400; break;
		case H_PRES: bValid = value >= 800 && value <= 1200; break;
		case H_SNOW: bValid = value >= 0 && value <= 500; break;
		case H_SNDH: bValid = value >= 0 && value <= 5000; break;
		case H_SWE:	 bValid = value >= 0 && value <= 5000; break;
		default: ASSERT(false);

		}

		return bValid;
	}


}