#include "StdAfx.h"
#include "UINewfoundland.h"
#include <boost\dynamic_bitset.hpp>
#include <boost\filesystem.hpp>


#include "Basic/DailyDatabase.h"
#include "Basic/FileStamp.h"
#include "Basic\CSV.h"
#include "Basic/CallcURL.h"
#include "UI/Common/UtilWin.h"
#include "UI/Common/SYShowMessage.h"

#include "TaskFactory.h"
#include "../Resource.h"
#include "WeatherBasedSimulationString.h"


using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace UtilWWW;
using namespace boost;


namespace WBSF
{

	//autre reseau: Water Resources Management Division (WRMD) Automatic Data Retrieval System (ADRS)
	//https://www.mae.gov.nl.ca/wrmd/ADRS/v6/Graphs_List.asp
	//https://www.mae.gov.nl.ca/wrmd/ADRS/v6/template_station.asp?station=nlencl0004
	//https://www.mae.gov.nl.ca/wrmd/ADRS/v6/Data/NLENCL0004_Line.csv
	//https://www.gov.nl.ca/eccm/waterres/cycle/hydrologic/weather/
	//https://www.mae.gov.nl.ca/wrmd/ADRS/v6/ADRS_Stations_Dec_08_2020.kmz

	const char* CUINewfoundland::SERVER_NAME = "Ftpque.nrcan.gc.ca";
	const char* CUINewfoundland::SERVER_PATH = "/NLWeather/";

	//*********************************************************************
	const char* CUINewfoundland::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "UsderName", "Password", "WorkingDir", "FirstYear", "LastYear" };
	const size_t CUINewfoundland::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_STRING, T_PASSWORD, T_PATH, T_STRING, T_STRING };
	const UINT CUINewfoundland::ATTRIBUTE_TITLE_ID = IDS_UPDATER_NEWFOUNDLAND_P;
	const UINT CUINewfoundland::DESCRIPTION_TITLE_ID = ID_TASK_NEWFOUNDLAND;

	const char* CUINewfoundland::CLASS_NAME() { static const char* THE_CLASS_NAME = "Newfoundland";  return THE_CLASS_NAME; }
	CTaskBase::TType CUINewfoundland::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUINewfoundland::CLASS_NAME(), (createF)CUINewfoundland::create);


	CUINewfoundland::CUINewfoundland(void)
	{}

	CUINewfoundland::~CUINewfoundland(void)
	{}


	std::string CUINewfoundland::Option(size_t i)const
	{
		string str;
		//switch (i)
		//{
		//};
		return str;
	}

	std::string CUINewfoundland::Default(size_t i)const
	{
		string str;

		switch (i)
		{
		case WORKING_DIR: str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()) + "Newfoundland\\"; break;
		case FIRST_YEAR:
		case LAST_YEAR:	str = ToString(CTRef::GetCurrentTRef().GetYear()); break;
		};

		return str;
	}

	//****************************************************


	std::string CUINewfoundland::GetStationsListFilePath(size_t n)const
	{
		string file_path;
		if (n == DFFA_NETWORK)
			file_path = GetDir(WORKING_DIR) + "Station_Metadata.csv";
		else if (n == WRMD_NETWORK)
			file_path = WBSF::GetApplicationPath() + "Layers\\NLStationsWRMD.csv";

		return file_path;
	}

	string CUINewfoundland::GetOutputFilePath(size_t n, const string& fileTitle, int year)const
	{
		string file_path;
		if(n== DFFA_NETWORK)
			file_path = GetDir(WORKING_DIR) + ToString(year) + "\\" + fileTitle + ".csv";
		else if (n == WRMD_NETWORK)
			file_path = GetDir(WORKING_DIR) + "WRMD\\"+ ToString(year) + "\\" + fileTitle + ".csv";

		return file_path;
	}

	string CUINewfoundland::GetOutputFilePath(int year)const
	{
		return GetDir(WORKING_DIR) + ToString(year) + "\\NL_WeatherStations_" + ToString(year) + ".zip";
	}


	ERMsg CUINewfoundland::Execute(CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);
		msg = CreateMultipleDir(workingDir);


		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(workingDir, 1);
		callback.AddMessage(GetString(IDS_UPDATE_FROM));
		callback.AddMessage(string(SERVER_NAME) + "/", 1);
		callback.AddMessage("");


		StringVector fileList;

		msg += UpdateStationList(callback);

		if (!msg)
			return msg;


		size_t nbDownloads = 0;

		int firstYear = WBSF::as<int>(Get(FIRST_YEAR));
		int lastYear = WBSF::as<int>(Get(LAST_YEAR));
		size_t nbYears = lastYear - firstYear + 1;

		callback.AddMessage(GetString(IDS_NUMBER_FILES) + ToString(nbYears), 1);
		callback.AddMessage("");

		callback.PushTask("Update Newfoundland weather data (" + ToString(nbYears) + " files)", nbYears);

		//open a connection on the server
		CInternetSessionPtr pSession;
		CFtpConnectionPtr pConnection;

		msg = GetFtpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, Get(USER_NAME), Get(PASSWORD), true, 5, callback);
		if (msg)
		{
			pSession->SetOption(INTERNET_OPTION_RECEIVE_TIMEOUT, 45000);

			//add station list
			for (size_t y = 0; y < nbYears; y++)
			{
				int year = firstYear + int(y);

				string filter = "/hydromanitoba/NLWeather/" + ToString(year) + "/NL_WeatherStations_" + ToString(year) + ".zip";

				CFileInfoVector fileList;
				msg = FindFiles(pConnection, filter, fileList, false, callback);
				if (fileList.size() == 1)
				{
					string fileName = GetFileName(fileList.front().m_filePath);

					string outputFilePath = GetOutputFilePath(year);
					if (!IsFileUpToDate(fileList.front(), outputFilePath))
					{
						CreateMultipleDir(GetPath(outputFilePath));
						callback.AddMessage("Download Newfoundland file: " + fileName + " ...");
						msg = CopyFile(pConnection, fileList.front().m_filePath, outputFilePath, INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_DONT_CACHE);

						if (msg)
						{
							msg = sevenZ(outputFilePath, GetPath(outputFilePath), callback);
							if (msg)
							{
								nbDownloads++;
								msg += callback.StepIt();
							}
						}
					}
				}
			}


			pConnection->Close();
			pSession->Close();
		}



		callback.AddMessage("Number of file downloaded: " + ToString(nbDownloads));
		callback.PopTask();


		//now downlaod WRMD stations
		msg += ExecutePublicWRMD(callback);

		return msg;
	}

	enum TColums { C_STAT_NUM, C_WSC_NUM, C_NST_DATI, C_AIR_TEMP, C_REL_HUMIDITY, C_ATMOS_PRES, C_DEW_POINT, C_PRECIP_TB, C_RAIN, C_SNOW, C_SNOW_DEPTH, C_SNOW_DEPTH_NEW, C_RAD_SOLAR, C_SUNSHINE_HRS, C_WIND_SPEED, C_WIND_DIR, C_WIND_SPEED_GUST, C_WIND_DIR_GUST, C_SOIL_MOIS, C_BATT_VOLTAGE, C_AIR_TEMP_SCND, C_SWE_TL, C_SWE_K, C_WIND_CHILL, C_HUMIDEX, C_WATER_TEMP, NB_COLUMS };
	static TVarH GetVar(const string& header)
	{
		static const char* VAR_NAME[NB_COLUMS] = { "STAT_NUM", "WSC_NUM", "NST_DATI", "AIR_TEMP", "REL_HUMIDITY", "ATMOS_PRES", "DEW_POINT", "PRECIP_TB", "RAIN", "SNOW", "SNOW_DEPTH", "SNOW_DEPTH_NEW", "RAD_SOLAR", "SUNSHINE_HRS", "WIND_SPEED", "WIND_DIR", "WIND_SPEED_GUST", "WIND_DIR_GUST", "SOIL_MOIS", "BATT_VOLTAGE", "AIR_TEMP_SCND", "SWE_TL","SWE_K", "WIND_CHILL","HUMIDEX","WATER_TEMP" };
		static const TVarH VAR[NB_COLUMS] = { H_SKIP, H_SKIP, H_SKIP, H_TAIR, H_RELH, H_PRES, H_PRCP, H_TDEW, H_SKIP, H_SKIP, H_SNDH, H_SKIP, H_SRAD, H_SKIP, H_WNDS, H_WNDD, H_SKIP, H_SKIP, H_SKIP, H_SKIP, H_SKIP, H_SWE, H_SKIP, H_SKIP, H_SKIP, H_SKIP };


		TVarH var = H_SKIP;
		auto it = std::find_if(begin(VAR_NAME), end(VAR_NAME), [header](const char* name) {return IsEqual(header, name); });
		

		if (it != end(VAR_NAME))
			var = VAR[std::distance(begin(VAR_NAME), it)];

		return var;
	}


	ERMsg CUINewfoundland::ExecutePublicWRMD(CCallback& callback)
	{
		ERMsg msg;

		static const char* SERVER_NAME = "www.mae.gov.nl.ca";

		string workingDir = GetDir(WORKING_DIR) + "WRMD\\";
		msg = CreateMultipleDir(workingDir);

		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(workingDir, 1);
		callback.AddMessage(GetString(IDS_UPDATE_FROM));
		callback.AddMessage(string(SERVER_NAME), 1);
		callback.AddMessage("");

		
		CLocationVector location;
		msg = location.Load(GetStationsListFilePath(WRMD_NETWORK));

		callback.PushTask("Download Newfoundland public WRMD data (" + ToString(location.size()) + " stations)", location.size());
		callback.AddMessage("Download Newfoundland public WRMD data (" + ToString(location.size()) + " stations)");

		size_t nbFiles = 0;
		for (size_t i = 0; i < location.size() && msg; i++)
		{
			CWeatherYears data(true);


			string URL = "https://www.mae.gov.nl.ca/wrmd/ADRS/v6/Data/" + location[i].m_ID + "_Line.csv";
			string filePath = workingDir + location[i].m_ID + "_Line.csv";

			string argument = "-s -k \"" + URL + "\" --output \"" + filePath + "\"";
			string exe = GetApplicationPath() + "External\\curl.exe";
			string command = exe + " " + argument;

			DWORD exit_code;
			msg = WinExecWait(command, "", SW_HIDE, &exit_code);
			if (exit_code == 0 && FileExists(filePath))
			{
				ifStream file;
				msg += file.open(filePath);
				if (msg)
				{
					vector<TVarH> variables;

					string line;
					getline(file, line);//skip warning
					CSVIterator loop(file);
					++loop; //skip units.

					for (; loop != CSVIterator(); ++loop)
					{
						ASSERT(loop->size() >= 3);

						if (variables.empty())
						{
							for (size_t c = 0; c < loop.Header().size(); c++)
							{
								TVarH var = GetVar(loop.Header()[c]);
								variables.push_back(var);
							}
						}


						string date_time_str = (*loop)[C_NST_DATI];

						StringVector date_time(date_time_str, "/ :");
						ASSERT(date_time.size() == 7);

						size_t m = WBSF::as<size_t>(date_time[0]) - 1;
						size_t d = WBSF::as<size_t>(date_time[1]) - 1;
						int year = WBSF::as<int>(date_time[2]);
						size_t h = WBSF::as<size_t>(date_time[3]);
						size_t mm = WBSF::as<size_t>(date_time[4]);
						size_t ss = WBSF::as<size_t>(date_time[5]);
						
						CTRef TRef(year,m,d,h);
						//TRef.FromFormatedString(date_time, "%m/%d/%Y %H:%M:%S");
						if (date_time[6]=="AM" && TRef.GetHour() == 12)
							TRef -= 12;
						else if (date_time[6] == "PM" && TRef.GetHour() != 12)
							TRef += 12;

						
						ASSERT(TRef.IsValid());

						if (TRef.IsValid())
						{
							if (!data.IsYearInit(TRef.GetYear()))
							{
								//try to load old data before changing it...
								string filePath = GetOutputFilePath(WRMD_NETWORK, location[i].m_ID, TRef.GetYear());
								data.LoadData(filePath, -999, false);//don't erase other years when multiple years
							}


							for (size_t c = 0; c < loop->size(); c++)
							{
								if (variables[c] != H_SKIP && !loop->at(c).empty())
								{
									float value = ToFloat(loop->at(c));


									if (variables[c] == H_SRAD)
										value *= 1000.0f / (3600 * 24);//convert KJ/m² --> W/m²
									else if (variables[c] == H_PRES)
										value *= 10;//convert kPa --> hPa
									else if (variables[c] == H_SNDH)
										value = max(0.0, value*100.0);//convert m --> cm

									if (data.GetHour(TRef)[variables[c]] == -999)
										data.GetHour(TRef).SetStat(variables[c], value);
								}
							}
						}

					}//for all lines


					if (data.HaveData())
					{
						for (auto it = data.begin(); it != data.end(); it++)
						{
							string outputPath = GetOutputFilePath(WRMD_NETWORK, location[i].m_ID, it->first);
							CreateMultipleDir(GetPath(outputPath));
							it->second->SaveData(outputPath);
						}
					}
					else
					{
						callback.AddMessage(location[i].m_ID + " don't have data");
					}

					nbFiles++;
					msg += callback.StepIt();
					file.close();

					WBSF::RemoveFile(filePath);
				}//msg
			}//if msg

		}//for all locations




		callback.AddMessage(GetString(IDS_NB_FILES_DOWNLOADED) + ToString(nbFiles), 1);
		callback.PopTask();

		return msg;
	}

	ERMsg CUINewfoundland::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;

		for (size_t n = 0; n < NB_NETWORKS; n++)
		{
			msg += m_stations[n].Load(GetStationsListFilePath(n));
			msg += m_stations[n].IsValid();

			for (size_t i = 0; i < m_stations[n].size(); i++)
			{
				stationList.push_back(std::to_string(n) + "|" + m_stations[n][i].m_ID);
			}
				
		}

		//Update network


		return msg;
	}

	ERMsg CUINewfoundland::GetWeatherStation(const std::string& str_in, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;


		//Get station information
		StringVector tmp(str_in, "|");
		ASSERT(tmp.size() == 2);
		size_t n = WBSF::as <size_t>(tmp[0]);
		string ID = tmp[1];
		size_t it = m_stations[n].FindByID(ID);
		ASSERT(it != NOT_INIT);

		((CLocation&)station) = m_stations[n][it];
		int firstYear = WBSF::as<int>(Get(FIRST_YEAR));
		int lastYear = WBSF::as<int>(Get(LAST_YEAR));
		size_t nbYears = lastYear - firstYear + 1;

		station.CreateYears(firstYear, nbYears);
		station.m_name = PurgeFileName(station.m_name);

		//now extract data 
		for (size_t y = 0; y < nbYears&&msg; y++)
		{
			int year = firstYear + int(y);

			string title = (n == DFFA_NETWORK)? station.GetSSI("FileTitle"): station.m_ID;
			string filePath = GetOutputFilePath(n, title, year);
			if (FileExists(filePath))
			{
				if(n== DFFA_NETWORK)
					msg = ReadDataFile(filePath, TM, station, callback);
				else if(n== WRMD_NETWORK)
					msg = station.LoadData(filePath, -999, false);
			}

			msg += callback.StepIt(0);
		}

		//verify station is valid
		if (msg && station.HaveData())
		{
			//verify station is valid
			msg = station.IsValid();
		}

		static const char* NETWORK_NAME[NB_NETWORKS] = {"NL_DFFA", "NL_WRMD"};

		station.m_siteSpeceficInformation.clear();
		station.SetSSI("Provider", "Newfoundland");
		station.SetSSI("Network", NETWORK_NAME[n]);
		station.SetSSI("Country", "CAN");
		station.SetSSI("SubDivision", "NL");


		return msg;
	}


	//******************************************************************************************************

	ERMsg CUINewfoundland::UpdateStationList(CCallback& callback)
	{
		ERMsg msg;

		CInternetSessionPtr pSession;
		CFtpConnectionPtr pConnection;

		msg = GetFtpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, Get(USER_NAME), Get(PASSWORD), true, 5, callback);

		if (msg)
		{
			string path = "/hydromanitoba/NLWeather/Station_Metadata.csv";

			CFileInfoVector fileList;
			msg = FindFiles(pConnection, path, fileList, false, callback);

			if (msg)
			{
				ASSERT(fileList.size() == 1);

				string outputFilePath = GetStationsListFilePath(DFFA_NETWORK);
				if (!IsFileUpToDate(fileList.front(), outputFilePath))
				{
					CreateMultipleDir(GetPath(outputFilePath));
					msg = CopyFile(pConnection, fileList.front().m_filePath, outputFilePath, INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_DONT_CACHE);
				}

			}

			pConnection->Close();
			pSession->Close();

		}

		return msg;
	}

	ERMsg CUINewfoundland::sevenZ(const string& filePathZip, const string& workingDir, CCallback& callback)
	{
		ERMsg msg;

		callback.PushTask(GetString(IDS_UNZIP_FILE), NOT_INIT);

		string command = GetApplicationPath() + "External\\7za.exe x \"" + filePathZip + "\" -y";
		//UINT show = as<bool>(SHOW_APP) ? SW_SHOW : SW_HIDE;

		DWORD exitCode = 0;
		msg = WinExecWait(command, workingDir, SW_HIDE, &exitCode);
		if (msg && exitCode != 0)
			msg.ajoute("7za.exe as exit with error code " + ToString(int(exitCode)));


		callback.PopTask();


		return msg;
	}


	CTRef CUINewfoundland::GetTRef(string str)//const vector<size_t>& vars, const CSVIterator& loop)
	{
		CTRef TRef;

		//2017/01/01 00:00:00
		StringVector vec(str, " :/");
		if (vec.size() == 6)
		{
			int year = WBSF::as<int>(vec[0]);
			size_t month = WBSF::as<size_t>(vec[1]) - 1;
			size_t day = WBSF::as<size_t>(vec[2]) - 1;
			size_t hour = WBSF::as<size_t>(vec[3]);

			ASSERT(month >= 0 && month < 12);
			ASSERT(day >= 0 && day < GetNbDayPerMonth(year, month));
			ASSERT(hour >= 0 && hour < 24);


			TRef = CTRef(year, month, day, hour);
		}


		return TRef;

	}

	ERMsg CUINewfoundland::ReadDataFile(const string& filePath, CTM TM, CWeatherYears& data, CCallback& callback)const
	{
		ERMsg msg;

		enum TColumns { C_NAME, C_DATE_TIME, C_TAIR, CTMIN, C_TMAX, C_RELH, C_WNDS, C_WNDD, C_RAIN24, NB_COLUMNS };
		static const TVarH COL_VAR[NB_COLUMNS] = { H_SKIP, H_SKIP, H_TAIR, H_TAIR, H_TAIR, H_RELH, H_WNDS, H_WNDD, H_PRCP };

		//now extract data 
		ifStream file;

		msg = file.open(filePath);

		if (msg)
		{
			CWeatherAccumulator stat(TM);
			double lastPrcp = 0;

			for (CSVIterator loop(file); loop != CSVIterator(); ++loop)
			{
				CTRef TRef = GetTRef((*loop)[C_DATE_TIME]);
				if (TRef.IsInit())
				{
					if (stat.TRefIsChanging(TRef))
						data[stat.GetTRef()].SetData(stat);

					for (size_t c = 0; c < loop->size(); c++)
					{
						if (COL_VAR[c] != H_SKIP && !(*loop)[c].empty())
						{
							double value = ToDouble((*loop)[c]);

							if (c == C_RAIN24)
							{
								if (value < lastPrcp)
									lastPrcp = 0;

								value -= lastPrcp;
								lastPrcp = ToDouble((*loop)[c]);
							}

							stat.Add(TRef, COL_VAR[c], value);
							if (c == C_RELH)
							{
								double T = ToDouble((*loop)[C_TAIR]);
								stat.Add(TRef, H_TDEW, Hr2Td(T, value));
							}
						}//if valid value
					}//for all columns
				}//TRef is init

				msg += callback.StepIt(0);
			}//for all line (

			if (stat.GetTRef().IsInit())
				data[stat.GetTRef()].SetData(stat);

		}//if load 

		return msg;
	}


}