#include "StdAfx.h"
#include "UISaskatchewan.h"
#include <boost\dynamic_bitset.hpp>
#include <boost\filesystem.hpp>

#include "UI/Common/UtilWin.h"
#include "Basic/DailyDatabase.h"
#include "Basic/FileStamp.h"
#include "UI/Common/SYShowMessage.h"
#include "Basic\CSV.h"
//#include "json\json11.hpp"

#include "TaskFactory.h"
#include "../Resource.h"
#include "WeatherBasedSimulationString.h"


using namespace std; 
using namespace WBSF::HOURLY_DATA;
using namespace UtilWWW;
using namespace boost;
//using namespace json11;

//plus de donn…es sur:
//http://fmfpweb2.serm.gov.sk.ca/weather/serm_only.html
//http ://fmfpweb2.serm.gov.sk.ca/currentcondsquick.php?station=SWIFT


//WeatherFarm
//http://weatherfarm.com/historical-data/
//http://weatherfarm.com/weather-analysis/?station_id=P0328#.WNGPq281-Uk
//http://weatherfarm.com/weather/current/SK/Endeavour/
//http://weatherfarm.com/historical-data/date-range=last2days&station-id=P0179
//http://weatherfarm.com/?share=P0179
//http://weatherfarm.com/historical-data/date-range=last2days&station-id=P0551

namespace WBSF
{

	const char* UISaskatchewan::SUBDIR_NAME[NB_NETWORKS] = { "Fire" };
	const char* UISaskatchewan::NETWORK_NAME[NB_NETWORKS] = { "Saskatchewan Fire" };
	const char* UISaskatchewan::SERVER_NAME[NB_NETWORKS] = { "fmfpweb2.serm.gov.sk.ca"};
	const char* UISaskatchewan::SERVER_PATH[NB_NETWORKS] = { "/"};

	size_t UISaskatchewan::GetNetwork(const string& network)
	{
		size_t n = NOT_INIT;

		for (size_t i = 0; i <NB_NETWORKS && n == NOT_INIT; i++)
		{
			if (IsEqualNoCase(network, NETWORK_NAME[i]))
				n = i;
		}

		return n;
	}

	//*********************************************************************
	const char* UISaskatchewan::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "DownloadArchive"};
	const size_t UISaskatchewan::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_BOOL};
	const UINT UISaskatchewan::ATTRIBUTE_TITLE_ID = IDS_UPDATER_SASKATCHEWAN_P;
	const UINT UISaskatchewan::DESCRIPTION_TITLE_ID = ID_TASK_SASKATCHEWAN;

	const char* UISaskatchewan::CLASS_NAME(){ static const char* THE_CLASS_NAME = "Saskatchewan";  return THE_CLASS_NAME; }
	CTaskBase::TType UISaskatchewan::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(UISaskatchewan::CLASS_NAME(), (createF)UISaskatchewan::create);



	UISaskatchewan::UISaskatchewan(void)
	{}

	UISaskatchewan::~UISaskatchewan(void)
	{}



	std::string UISaskatchewan::Option(size_t i)const
	{
		string str;
		//switch (i)
		//{
		//};
		return str;
	}

	std::string UISaskatchewan::Default(size_t i)const
	{
		string str;

		switch (i)
		{
		case WORKING_DIR: str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()) + "Saskatchewan\\"; break;
		case DOWNLOAD_ARCHIVE:	str = "1"; break;
		};

		return str;
	}

	//****************************************************


	std::string UISaskatchewan::GetStationsListFilePath(size_t network)const
	{
		static const char* FILE_NAME[NB_NETWORKS] = { "SaskatchewanFireStations.csv" };

		string filePath = WBSF::GetApplicationPath() + "Layers\\" + FILE_NAME[network];
		return filePath;
	}

	string UISaskatchewan::GetOutputFilePath(size_t network, const string& title, int year, size_t m)const
	{
		return GetDir(WORKING_DIR) + SUBDIR_NAME[network] + "\\" + ToString(year) + "\\" + (m != NOT_INIT ? FormatA("%02d\\", m+1).c_str() : "") + title + ".csv";
	}


	ERMsg UISaskatchewan::Execute(CCallback& callback)
	{
		ERMsg msg;

		
		string workingDir = GetDir(WORKING_DIR) + SUBDIR_NAME[FIRE] + "\\";
		msg = CreateMultipleDir(workingDir);


		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(workingDir, 1);
		callback.AddMessage(GetString(IDS_UPDATE_FROM));
		callback.AddMessage(string(SERVER_NAME[FIRE]) + "/" + SERVER_PATH[FIRE], 1);
		callback.AddMessage("");

		msg = ExecuteFire(callback);

		return msg;
	}


	ERMsg UISaskatchewan::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;

	
		m_stations.clear();
	
		CLocationVector locations;
		msg = locations.Load(GetStationsListFilePath(FIRE));

		if (msg)
			msg += locations.IsValid();

		//Update network
		for (size_t i = 0; i < locations.size(); i++)
			locations[i].SetSSI("Network", NETWORK_NAME[FIRE]);

		m_stations.insert(m_stations.end(), locations.begin(), locations.end());
				
		for (size_t i = 0; i < locations.size(); i++)
			stationList.push_back(ToString(FIRE)+"/"+locations[i].m_ID);
	

		return msg;
	}

	ERMsg UISaskatchewan::GetWeatherStation(const std::string& NID, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;

		size_t n = ToSizeT(NID.substr(0,1));
		string ID = NID.substr(2);

		//Get station information
		size_t it = m_stations.FindByID(ID);
		ASSERT(it != NOT_INIT);

		((CLocation&)station) = m_stations[it];

		//size_t type = as<size_t>(DATA_TYPE);
		int firstYear = WBSF::as<int>(Get("FirstYear"));
		int lastYear = WBSF::as<int>(Get("LastYear"));
		size_t nbYears = lastYear - firstYear + 1;
		station.CreateYears(firstYear, nbYears);

		station.m_name = PurgeFileName(station.m_name);

		//now extract data 
		for (size_t y = 0; y < nbYears&&msg; y++)
		{
			int year = firstYear + int(y);

			string filePath = GetOutputFilePath(n, ID, year);
			if (FileExists(filePath))
				msg = station.LoadData(filePath, -999, false);

			msg += callback.StepIt(0);
		}

		//verify station is valid
		if (msg && station.HaveData())
		{
			//verify station is valid
			msg = station.IsValid();
		}

		return msg;
	}


	//******************************************************************************************************

	//TA - air temperature 
	//UD - wind direction
	//US - wind speed
	//UG - wind gust
	//PC - precipitation
	//XR - relative humidity
	//PA - atmospheric pressure

	enum TVariables { H_STATION_ID, H_DATE, H_TIME, H_TEMP, H_DEW, H_RELATIVE_HUMIDITY, H_DDIR, H_CDIR, H_WIND_SPEED, H_WIND_GUST, H_PRECIPITATION, H_WIND_DIR, H_MAX_SPEED, NB_VARS };
	//ffmc_h	isi_h	fwi_h	
	//date	time	temp	rh	dir	wspd	mx_spd	rn_1	telem	d_cell
	static const char* FIRE_VAR_NAME[NB_VARS] = { "fmfp_id", "date", "time", "temp", "dew", "rh", "ddir", "cdir", "wspd", "wgst", "rn_1", "dir", "mx_spd" };
	static const TVarH FIRE_VAR[NB_VARS] = { H_SKIP, TVarH(-3), TVarH(-2), H_TAIR2, H_TDEW, H_RELH, H_WNDD, H_SKIP, H_WNDS, H_SKIP, H_PRCP, H_WNDD, H_SKIP};
	static size_t GetVar(string name)
	{
		size_t var = NOT_INIT;

		for (size_t i = 0; i <NB_VARS && var == NOT_INIT; i++)
		{
			if (IsEqual(name, FIRE_VAR_NAME[i]) )
				var = FIRE_VAR[i];
		}

		return var;
	}


	ERMsg UISaskatchewan::ExecuteFire(CCallback& callback)
	{
		ERMsg msg;

		CLocationVector locations;
		msg = locations.Load(GetStationsListFilePath(FIRE));

		if (msg)
			msg += locations.IsValid();

		if (msg)
		{

			bool bArchive = as<bool>(DOWNLOAD_ARCHIVE);

			callback.PushTask("Update Saskatchewan fire weather data (" + ToString(locations.size()) + " stations)", locations.size() * (bArchive?2:1) );
			size_t curI = 0;
			int nbRun = 0;
			
			while (curI < locations.size() && nbRun < 5 && msg)
			{
				nbRun++;

				CInternetSessionPtr pSession;
				CHttpConnectionPtr pConnection;

				ERMsg msgTmp = GetHttpConnection(SERVER_NAME[FIRE], pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", false, 5, callback);
				if (msgTmp)
				{
					TRY
					{
						for (size_t i = curI; i < locations.size() && msg; i++)
						{
							string ID = locations[i].m_ID;

							if (bArchive)
							{
								string remoteFilePath = "exc_hourly_rts.php?station=" + ID + "&nhh=5000";
								string outputFilePath = GetDir(WORKING_DIR) + SUBDIR_NAME[FIRE] + "\\" + ID + "_archive.txt";
								if (FileExists(outputFilePath))
									msg += RemoveFile(outputFilePath);
								
								msgTmp += CopyFile(pConnection, remoteFilePath, outputFilePath, INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_PRAGMA_NOCACHE);
								if (msgTmp && FileExists(outputFilePath))
								{
									msg += SplitFireData(locations[i].m_ID, outputFilePath, callback);
									msg += RemoveFile(outputFilePath);
									msg += callback.StepIt();
								}
									
							}

							string remoteFilePath = "exc_hourly_stn.php?station=" + ID + "&nhh=10000";
							string outputFilePath = GetDir(WORKING_DIR) + SUBDIR_NAME[FIRE] + "\\" + ID + ".txt";

							if (FileExists(outputFilePath))
								msg += RemoveFile(outputFilePath);

							msgTmp += CopyFile(pConnection, remoteFilePath, outputFilePath, INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_PRAGMA_NOCACHE);

							//split data in seperate files
							if (msgTmp && FileExists(outputFilePath))
							{
								msg += SplitFireData(locations[i].m_ID, outputFilePath, callback);
								if (msg)
								{
									curI++;
									msg += RemoveFile(outputFilePath);
									msg += callback.StepIt();
								}
							}

						}//for all station
					}
					CATCH_ALL(e)
					{
						msgTmp = UtilWin::SYGetMessage(*e);
					}
					END_CATCH_ALL

						//clean connection
					pConnection->Close();
					pSession->Close();
				}
				else
				{
					if (nbRun > 1 && nbRun < 5)
					{
						msg += WaitServer(10, callback);
					}
				}
			}
			

			callback.AddMessage(GetString(IDS_NB_STATIONS) + ToString(curI), 1);
			callback.PopTask();
		}

		return msg;
	}
	


	static CTRef GetTRef(const vector<size_t>& vars, const CSVIterator& loop)
	{
		CTRef TRef;

		//find -3 (date) and -2 (hour)
		size_t pos = UNKNOWN_POS;
		for (size_t i = 0; i < vars.size() && pos == UNKNOWN_POS; i++)
			if (vars[i] == -3)
				pos = i;

		ASSERT(pos != UNKNOWN_POS);
		ASSERT(vars[pos+1] == -2);
		//21 Mar 2017	19

		StringVector str((*loop)[pos], " ");
		if (str.size() == 3)//eliminate NULL
		{

			size_t day = WBSF::as<size_t>(str[0]) - 1;
			size_t month = WBSF::GetMonthIndex(str[1].c_str());
			int year = WBSF::as<int>(str[2]);
			size_t hour = WBSF::as<size_t>((*loop)[pos + 1]);
			TRef = CTRef(year, month, day, hour);
		}


		return TRef;

	}
	
	ERMsg UISaskatchewan::SplitFireData(const string& ID, const std::string& outputFilePath, CCallback& callback)
	{
		ASSERT(!outputFilePath.empty());
		ERMsg msg;

		
		CTM TM(CTM::HOURLY);

		CWeatherYears data(true);

		
		ifStream files;
		msg += files.open(outputFilePath);

		if (msg)
		{
			vector<size_t> vars;
			CWeatherAccumulator stat(TM);

			for (CSVIterator loop(files,"\t"); loop != CSVIterator() && msg; ++loop)
			{
				if (vars.empty())
				{
					vars.resize(loop.Header().size());
					for (size_t i = 0; i < loop.Header().size(); i++)
						vars[i] = GetVar(loop.Header()[i]);
				}

				if (loop->size() == vars.size())
				{
					CTRef TRef = GetTRef(vars, loop);
					if (TRef.IsInit())
					{
						if (stat.TRefIsChanging(TRef))
						{
							if (!data.IsYearInit(TRef.GetYear()))
							{
								//try to load old data before changing it...
								string filePath = GetOutputFilePath(FIRE, ID, TRef.GetYear());
								data.LoadData(filePath, -999, false);//don't erase other years when multiple years
							}

							data[stat.GetTRef()].SetData(stat);
						}

						for (size_t i = 0; i < vars.size(); i++)
						{
							if (vars[i] < NB_VAR_H && (*loop)[i] != "NULL")
							{
								double value = ToDouble((*loop)[i]);
								ASSERT(value > -99);
								stat.Add(TRef, vars[i], value);
							}
						}
					}
				}

				msg += callback.StepIt(0);
			}//for all line 
				
			if (stat.GetTRef().IsInit())
				data[stat.GetTRef()].SetData(stat);

			if (msg)
			{
				//Compute Tdew if hourly data
				CTPeriod p = data.GetEntireTPeriod();
				for (CTRef h = p.Begin(); h <= p.End(); h++)
				{
					CStatistic T = data[h][H_TAIR2];
					CStatistic Hr = data[h][H_RELH];
					CStatistic Td = data[h][H_TDEW];
					if (T.IsInit() && Hr.IsInit())
					{
						if(!Td.IsInit())
							data[h].SetStat(H_TDEW, Hr2Td(T[MEAN], Hr[MEAN]));
					}
						
				}


				//save all years 
				for (auto it = data.begin(); it != data.end(); it++)
				{
					string filePath = GetOutputFilePath(FIRE, ID, it->first);
					string outputPath = GetPath(filePath);
					CreateMultipleDir(outputPath);
					it->second->SaveData(filePath, TM);
				}
			}//if msg
		}//if msg

		return msg;
	}

	
}