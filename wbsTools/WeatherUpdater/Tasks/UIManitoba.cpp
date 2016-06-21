#include "StdAfx.h"
#include "UIManitoba.h"
#include <boost\dynamic_bitset.hpp>
#include <boost\filesystem.hpp>

#include "Basic/DailyDatabase.h"
#include "Basic/FileStamp.h"
#include "UI/Common/SYShowMessage.h"
#include "Basic\CSV.h"

#include "TaskFactory.h"
#include "../Resource.h"
#include "WeatherBasedSimulationString.h"


using namespace std; 
using namespace WBSF::HOURLY_DATA;
using namespace UtilWWW;
using namespace boost;


////HG - water level
//TW - water temperature
//TA - air temperature
//UD - wind direction
//US - wind speed
//UG - wind gust
//PC - precipitation
//XR - relative humidity
//PA - atmospheric pressure
//https://www.hydro.mb.ca/hydrologicalData/static/stations/05KL701/Parameter/TA/DayMeanYear.html
//https://www.hydro.mb.ca/hydrologicalData/static/data/graph.json?v=20160613054508
//view-source:https://www.hydro.mb.ca/hydrologicalData/static/stations/05KL701/station.html?v=20160513054508
//https://www.hydro.mb.ca/hydrologicalData/static/

//Table60ElementPos	Table60ElementName	Table60ElementDesc	Table60ElementUnits
//1	TMSTAMP	time stamp of record at interval completion	CDST "yyyy-mm-dd hh:mm"
//2	RECNBR	seq table record number from prog change / start	int
//3	StnID	unique 3 digit Station Identifier for MAWP	int
//4	BatMin	minimum battery voltage prev 1 hour	volts(v)
//5	Air_T	avg of 5 sec temp vals prev 1 min	deg C(°C)
//6	AvgAir_T	avg of 5 sec temp vals prev 1 hour	deg C(°C)
//7	MaxAir_T	maximum of 1 min avg temp vals prev 1 hour	deg C(°C)
//8	MinAir_T	minimum of 1 min avg temp vals prev 1 hour	deg C(°C)
//9	RH	avg of 5 sec RH vals prev 1 min	% RH(%)
//10	AvgRH	avg of 5 sec RH vals prev 1 hour	% RH(%)
//11	Rain	total rain measured prev 1 hour(Primary gauge)	millimetres(m.m.)
//12	Rain24RT	total rain running total since midnight CDST	millimetres(m.m.)
//13	WS_10Min	scalar mean wind speed prev 10 min	metres per second(m / s)
//14	WD_10Min	derived vector wind dir prev 10 min	degrees true (°)
//15	AvgWS	scalar mean wind speed prev 1 hour	metres per second(m / s)
//16	AvgWD	derived vector wind direction prev 1 hour	degrees true (°)
//17	AvgSD	derived std deviation of vector dir prev 1 hour	degrees true (°)
//18	MaxWS_10	maximum 10 min mean wind speed prev 1 hour	metres per second(m / s)
//19	MaxWD_10	derived vector wind direction for max 10min mean	degrees true (°)
//20	MaxWS	maximum wind speed of 5 sec vals prev 1 hour	metres per second(m / s)
//21	HmMaxWS	timestamp of occurance of MaxWS	CDST "yyyy-mm-dd hh:mm:ss.msms"
//22	MaxWD	wind direction at MaxWS	degrees true (°)
//23	Max5WS_10	maximum wind speed of 5 sec vals prev 10 min	metres per second(m / s)
//24	Max5WD_10	wind direction at Max5WS_10	degrees true (°)
//25	WS_2Min	scalar mean wind speed prev 2 minutes	metres per second(m / s)
//26	WD_2Min	derived vector wind direction prev 2 minutes	degrees true (°)
//27	Soil_T05	avg of 5 sec temp vals prev 1 min	deg C(°C)
//28	AvgRS_kw	avg hourly solar flux density prev 1 hour	KW / m²
//29	TotRS_MJ	total hourly solar fluxes prev 1 hour	MJ / m²
//30	Rain2	total rain measured prev 1 hour(Secondary gauge)	millimetres(m.m.)
//31	Rain24RT2	total rain running total since midnight CDST(Secondary gauge)	millimetres(m.m.)




//Table24ElementPos	Table24ElementName	Table24ElementDesc	Table24ElementUnits
//1	TMSTAMP	time stamp of record at interval completion	CDST("yyyy-mm-dd hh:mm" - 1)
//2	RECNBR	sequential tabrecord number from prog change / start	int
//3	StnID	unique 3 digit Station Identifier for MAWP	int
//4	BatMin	minimum battery voltage prev 24 hours	volts(v)
//5	ProgSig	set by logger, changes if prog changes	int
//6	AvgAir_T	avg of prev 24 hours of 1 min avg's	deg C (°C)
//7	MaxAir_T	max of prev 24 hours of 1 min avg's	deg C (°C)
//8	HmMaxAir_T	timestamp of occurance of max temp	CDST "yyyy-mm-dd hh:mm:ss.msms"
//9	MinAir_T	min of prev 24 hours of 1 min avg's	deg C (°C)
//10	HmMinAir_T	timestamp of occurance of min temp	CDST "yyyy-mm-dd hh:mm:ss.msms"
//11	AvgRH	avg of prev 24 hours of 1 min avg's	% RH (%)
//12	MaxRH	maximum of prev 24 hours of 1 min avg's	% RH (%)
//13	HmMaxRH	timestamp of occurance of max RH	CDST "yyyy-mm-dd hh:mm:ss.msms"
//14	MinRH	minimum of prev 24 hours of 1 min avg's	% RH (%)
//15	HmMinRH	timestamp of occurance of min RH	CDST "yyyy-mm-dd hh:mm:ss.msms"
//16	Rain	total rain measured prev 24 hours(primary)	millimetres(m.m.)
//17	MaxWS	max of 5 sec vals prev 24 hours	metres per second(m / s)
//18	HmMaxWS	timestamp of occurance of max wind	CDST "yyyy-mm-dd hh:mm:ss.msms"
//19	AvgWS	scalar mean wind speed prev 24 hours	metres per second(m / s)
//20	AvgWD	derived vector wind direction prev 24 hours	degrees true (°)
//21	AvgSD	derived std deviation of vector dir prev 24 hours	degrees true (°)
//22	AvgSoil_T05	avg of 5 sec vals prev 24 hours	deg C(°C)
//23	MaxSoil_T05	maximum of 5 sec vals prev 24 hours	deg C(°C)
//24	MinSoil_T05	minimum of 5 sec vals prev 24 hours	deg C(°C)
//25	AvgRS_kw	avg hourly solar flux density	KW / m²
//26	MaxRS_kw	maximum hourly solar flux density	KW / m²
//27	HmMaxRS	timestamp of maximum hourly solar flux density	CDST "yyyy-mm-dd hh:mm:ss.msms"
//28	TotRS_MJ	total solar fluxes previous 24 hours	MJ / m²
//29	Rain2	total rain measured prev 24 hours(secondary)	millimetres(m.m.)


namespace WBSF
{

	const char* CUIManitoba::SERVER_NAME = "mawpvs.dyndns.org";
	const char* CUIManitoba::SERVER_PATH = "Tx_DMZ/";


	//*********************************************************************
	const char* CUIManitoba::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "FirstYear", "LastYear", "Network", "DataType" };
	const size_t CUIManitoba::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_STRING, T_STRING, T_STRING_SELECT, T_COMBO_INDEX };
	const UINT CUIManitoba::ATTRIBUTE_TITLE_ID = IDS_UPDATER_MANITOBA_P;
	const UINT CUIManitoba::DESCRIPTION_TITLE_ID = ID_TASK_MANITOBA;

	const char* CUIManitoba::CLASS_NAME(){ static const char* THE_CLASS_NAME = "Manitoba";  return THE_CLASS_NAME; }
	CTaskBase::TType CUIManitoba::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUIManitoba::CLASS_NAME(), (createF)CUIManitoba::create);



	CUIManitoba::CUIManitoba(void)
	{}

	CUIManitoba::~CUIManitoba(void)
	{}



	std::string CUIManitoba::Option(size_t i)const
	{
		string str;
		switch (i)
		{
		case NETWORK:	str = "Manitoba Agriculture Weather Program"; break;
		case DATA_TYPE:	str = GetString(IDS_STR_WDATA_TYPE); break;
		};
		return str;
	}

	std::string CUIManitoba::Default(size_t i)const
	{
		string str;

		switch (i)
		{
		case WORKING_DIR: str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()) + "Manitoba\\"; break;
		case FIRST_YEAR:
		case LAST_YEAR:	str = ToString(CTRef::GetCurrentTRef().GetYear()); break;
		case DATA_TYPE: str = "0"; break;
		};

		return str;
	}

	//****************************************************
	ERMsg CUIManitoba::GetFileList(CFileInfoVector& fileList, CCallback& callback)const
	{
		ERMsg msg;


		fileList.clear();

	

		//open a connection on the server
		CInternetSessionPtr pSession;
		CFtpConnectionPtr pConnection;
		
		ERMsg msgTmp = GetFtpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", true);
		if (msgTmp)
		{
			pSession->SetOption(INTERNET_OPTION_RECEIVE_TIMEOUT, 15000);
			string command = as<size_t>(DATA_TYPE) == HOURLY_WEATHER ? "*60raw.txt" : "*24raw.txt";
			msgTmp = FindFiles(pConnection, string(SERVER_PATH) + command, fileList, callback);
		}
		

//		int firstYear = as<int>(FIRST_YEAR);
//		int lastYear = as<int>(LAST_YEAR);
//		size_t nbYears = lastYear - firstYear + 1;
//
//		callback.PushTask(GetString(IDS_LOAD_FILE_LIST), nbYears);
////		callback.SetNbStep(nbYears);
//
//
//
//		dynamic_bitset<size_t> toDo(nbYears + 1);
//		toDo.set();
//		//toDo.InsertAt(0, true, nbYears+1);
//
//
//		size_t nbRun = 0;
//
//		CFileInfoVector dirList;
//		while (nbRun < 20 && toDo[nbYears] && msg)
//		{
//			nbRun++;
//
//			//open a connection on the server
//			CInternetSessionPtr pSession;
//			CFtpConnectionPtr pConnection;
//
//			ERMsg msgTmp = GetFtpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", true);
//			if (msgTmp)
//			{
//				pSession->SetOption(INTERNET_OPTION_RECEIVE_TIMEOUT, 15000);
//
//				if (toDo[0])
//				{
//					callback.PushTask(GetString(IDS_LOAD_FILE_LIST), NOT_INIT);
//					msgTmp = FindDirectories(pConnection, SERVER_PATH, dirList);
//					callback.PopTask();
//
//					if (msgTmp)
//						toDo[0] = false;
//				}
//
//				if (msgTmp)
//				{
//					for (size_t y = 0; y < dirList.size() && msg&&msgTmp; y++)
//					{
//						msg += callback.StepIt(0);
//
//						const CFileInfo& info = dirList[y];
//						string path = info.m_filePath;
//						int year = ToInt(path.substr(14, 4));
//						if (year >= firstYear && year <= lastYear)
//						{
//							int index = year - firstYear + 1;
//							if (toDo[index])
//							{
//								msgTmp = FindFiles(pConnection, string(info.m_filePath) + "*.op.gz", fileList, callback);
//								if (msgTmp)
//								{
//									toDo[index] = false;
//									msg += callback.StepIt();
//
//									nbRun = 0;
//								}
//							}
//						}
//					}
//				}
//
//				pConnection->Close();
//				pSession->Close();
//
//				if (!msgTmp)
//					callback.AddMessage(msgTmp);
//			}
//			else
//			{
//				if (nbRun > 1 && nbRun < 20)
//				{
//					callback.AddMessage("Waiting 30 seconds for server...");
//					for (int i = 0; i < 60 && msg; i++)
//					{
//						Sleep(500);//wait 500 milisec
//						msg += callback.StepIt(0);
//					}
//				}
//			}
		//}
//
//		callback.PopTask();
//
//		//remove unwanted file
//		if (msg)
//		{
//			if (toDo[nbYears])
//				callback.AddMessage(GetString(IDS_SERVER_BUSY));
//
//			callback.AddMessage(GetString(IDS_NB_FILES_FOUND) + ToString(fileList.size()), 1);
//
//			msg = CleanList(fileList, callback);
//		}

		return msg;
	}



	ERMsg CUIManitoba::Execute(CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);
		msg = CreateMultipleDir(workingDir);


		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(workingDir, 1);
		callback.AddMessage(GetString(IDS_UPDATE_FROM));
		callback.AddMessage(string(SERVER_NAME) + "/" + SERVER_PATH, 1);
		callback.AddMessage("");


		//leand station list
		//CFileInfoVector fileList;
		//msg = UpdateStationHistory();

		//if (msg)
			//msg = UpdateOptimisationStationFile(GetDir(WORKING_DIR), callback);

		//if (msg)
			//msg = LoadOptimisation();

		//if (msg)
			//msg = GetFileList(fileList, callback);

		//if (!msg)
			//return msg;


		//callback.PushTask(GetString(IDS_UPDATE_FILE), fileList.size());
		//callback.SetNbStep(fileList.size());

		size_t type = as<size_t>(DATA_TYPE);
		string fileName = type == HOURLY_WEATHER ? "mawp60raw.txt" : "mawp24raw.txt";;
		string remoteFilePath = "Tx_DMZ/" + fileName;
		string outputFilePath = workingDir + fileName;
		

		int nbRun = 0;
		bool bDownloaded = false;

		while (!bDownloaded && nbRun < 5 && msg)
		{
			nbRun++;

			CInternetSessionPtr pSession;
			CFtpConnectionPtr pConnection;

			ERMsg msgTmp = GetFtpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", true);
			if (msgTmp)
			{
				TRY
				{
					//for (int i = curI; i < fileList.size() && msgTmp && msg; i++)
					//{
					//msg += callback.StepIt(0);
					
					
					//string outputPath = GetPath(outputFilePath);
					//CreateMultipleDir(outputPath);

					callback.PushTask(GetString(IDS_UPDATE_FILE), NOT_INIT);
					msgTmp += CopyFile(pConnection, remoteFilePath, outputFilePath, INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_DONT_CACHE);
					callback.PopTask();

					//split data in seperate files
					if (msgTmp)
					{
						ASSERT(FileExists(outputFilePath));
						SplitStations(outputFilePath, callback);
						RemoveFile(outputFilePath);
							
						msg += callback.StepIt();
						bDownloaded = true;
					}
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
					callback.PushTask("Waiting 30 seconds for server...", 600);
					for (size_t i = 0; i < 600 && msg; i++)
					{
						Sleep(50);//wait 50 milisec
						msg += callback.StepIt();
					}
					callback.PopTask();
				}
			}
		}

		//callback.AddMessage(GetString(IDS_NB_FILES_DOWNLOADED) + ToString(curI), 1);
		//callback.PopTask();

		return msg;
	}


	string CUIManitoba::GetOutputFilePath(const string& stationName, int year)const
	{
		string type = (as<size_t>(DATA_TYPE) == HOURLY_WEATHER) ? "Hourly" : "Daily";
		return GetDir(WORKING_DIR) + type + "\\" + ToString(year) + "\\" + stationName + ".csv";
	}

	ERMsg CUIManitoba::SplitStations(const string& outputFilePath, CCallback& callback)
	{
		ERMsg msg;

		size_t type = as<size_t>(DATA_TYPE);
		CTM TM(type == HOURLY_WEATHER?CTM::HOURLY:CTM::DAILY);

		std::map<string, CWeatherYears> data;

		ifStream file;
		msg = file.open(outputFilePath);
		if (msg)
		{
			callback.PushTask("Slpit data", file.length());

			CWeatherAccumulator stat(TM);
			string lastID;

			enum TCommonyColumns{TMSTAMP, RECNBR, STNID, BATMIN, COMMON_COLUMNS};
			enum THourlyColumns{ AIR_T_H = COMMON_COLUMNS, AVGAIR_T_H, MAXAIR_T_H, MINAIR_T_H, RH_AVG_H, AVGRH_H, RAIN_H, RAIN24RT_H, WS_10MIN_H, WD_10MIN_H, AVGWS_H, AVGWD_H, AVGSD_H, MAXWS_10_H, MAXWD_10_H, MAXWS_H, HMMAXWS_H, MAXWD_H, MAX5WS_10_H, MAX5WD_10_H, WS_2MIN_H, WD_2MIN_H, SOIL_T05_H, AVGRS_KW_H, TOTRS_MJ_H, RAIN2_H, RAIN24RT2_H, NB_COLUMNS_H };
			static const size_t COL_POS_H[NB_COLUMNS_H] = { -1, -1, -1, -1, -1, H_TAIR, H_TAIR, H_TAIR, -1, H_RELH, H_PRCP, -1, -1, -01, H_WND2, H_WNDD, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, H_SRAD, -1, H_PRCP, -1 };
			enum TDailyColumns{ PROGSIG_D = COMMON_COLUMNS, AVGAIR_T_D, MAXAIR_T_D, HMMAXAIR_T_D, MINAIR_T_D, HMMINAIR_T_D, AVGRH_D, MAXRH_D, HMMAXRH_D, MINRH_D, HMMINRH_D, RAIN_D, MAXWS_D, HMMAXWS_D, AVGWS_D, AVGWD_D, AVGSD_D, AVGSOIL_T05_D, MAXSOIL_T05_D, MINSOIL_T05_D, AVGRS_KW_D, MAXRS_KW_D, HMMAXRS_D, TOTRS_MJ_D, RAIN2_D, NB_COLUMNS_D };
			static const size_t COL_POS_D[NB_COLUMNS_D] = { -1, -1, -1, -1, -1, -1, H_TAIR, -1, H_TAIR, -1, H_RELH, -1, H_RELH, H_RELH, -1, H_PRCP, -1, -1, -1, H_WND2, H_WNDD, -1, -1, -1, -1, H_SRAD, -1,-1, -1 };
			
			for (CSVIterator loop(file, ",", false); loop != CSVIterator()&&msg; ++loop)
			{
				if (!loop->empty())
				{
					StringVector time((*loop)[TMSTAMP], "\"-: ");
					ASSERT(time.size() == 6);

					int year = ToInt(time[0]);
					size_t month = ToInt(time[1]) - 1;
					size_t day = ToInt(time[2]) - 1;
					size_t hour = ToInt(time[3]);

					ASSERT(month >= 0 && month < 12);
					ASSERT(day >= 0 && day < GetNbDayPerMonth(year, month));
					ASSERT(hour >= 0 && hour < 24);

					CTRef TRef = type == HOURLY_WEATHER ? CTRef(year, month, day, hour) : CTRef(year, month, day);
					string ID = (*loop)[STNID];
					if (lastID.empty())
						lastID = ID;

					if (stat.TRefIsChanging(TRef) || ID != lastID)
					{
						if (data.find(lastID) == data.end())
						{
							data[lastID] = CWeatherYears(type == HOURLY_WEATHER);
							//try to load old data before changing it...
							string filePath = GetOutputFilePath(ID, year);
							data[lastID].LoadData(filePath, -999, false);//don't erase other years when multiple years

						}
						//data[lastID].HaveData()
						data[lastID][stat.GetTRef()].SetData(stat);
						lastID = ID;
					}

					for (size_t v = 0; v < loop->size(); v++)
					{
						size_t cPos = type == HOURLY_WEATHER ? COL_POS_H[v] : COL_POS_D[v];
						if (cPos < NB_VAR_H)
						{
							double value = ToDouble((*loop)[v]);
							if (value > -99)
							{
								if (cPos == H_WND2)
									value *= 3600 / 1000;//convert m/s into km/h

								stat.Add(TRef, cPos, value);
								/*if (v == RH_AVG)
								{
								double T = ToDouble((*loop)[AVGAIR_T]);
								if (T > -99)
								{
								stat.Add(TRef, H_TDEW, Hr2Td(T, value));
								}
								}*/
							}
						}
					}
				}//empty

				msg += callback.StepIt(loop->GetLastLine().length() + 2);
			}//for all line (


			if (stat.GetTRef().IsInit())
				data[lastID][stat.GetTRef()].SetData(stat);


			if (msg)
			{
				//save data
				for (auto it1 = data.begin(); it1 != data.end(); it1++)
				{
					for (auto it2 = it1->second.begin(); it2 != it1->second.end(); it2++)
					{
						string filePath = GetOutputFilePath(it1->first, it2->first);
						string outputPath = GetPath(filePath);
						CreateMultipleDir(outputPath);
						it2->second->SaveData(filePath, TM);
					}
				}
			}//if msg

			callback.AddMessage(GetString(IDS_NB_STATIONS) + ToString(data.size()), 1);
			callback.PopTask();
		}//if msg

		return msg;
	}

	std::string CUIManitoba::GetStationListFilePath()const
	{
		
		return WBSF::GetApplicationPath() + "Layers\\ManitobaAgStations.csv";
	}

	ERMsg CUIManitoba::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;

		msg = m_stations.Load(GetStationListFilePath());

		if (msg)
			msg += m_stations.IsValid();

		if (msg)
		{
			for (size_t i = 0; i < m_stations.size(); i++)
				stationList.push_back(m_stations[i].m_ID);
		}

		return msg;
	}

	ERMsg CUIManitoba::GetWeatherStation(const std::string& ID, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;

		//Get station information
		size_t it = m_stations.FindByID(ID);
		if (it == NOT_INIT)
		{
			msg.ajoute(FormatMsg(IDS_NO_STATION_INFORMATION, ID));
			return msg;
		}

		((CLocation&)station) = m_stations[it];
		//station.m_name = TraitFileName(station.m_name);

		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = lastYear - firstYear + 1;
		station.CreateYears(firstYear, nbYears);

		station.m_name = PurgeFileName(station.m_name);

		//now extract data 
		for (size_t y = 0; y < nbYears&&msg; y++)
		{
			int year = firstYear + int(y);

			string filePath = GetOutputFilePath(ID, year);
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
}


