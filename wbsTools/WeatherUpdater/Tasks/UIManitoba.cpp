#include "StdAfx.h"
#include "UIManitoba.h"
#include <boost\dynamic_bitset.hpp>
#include <boost\filesystem.hpp>

#include "UI/Common/UtilWin.h"
#include "Basic/DailyDatabase.h"
#include "Basic/FileStamp.h"
#include "UI/Common/SYShowMessage.h"
#include "Basic\CSV.h"
#include "json\json11.hpp"

#include "TaskFactory.h"
#include "../Resource.h"
#include "WeatherBasedSimulationString.h"


using namespace std; 
using namespace WBSF::HOURLY_DATA;
using namespace UtilWWW;
using namespace boost;
using namespace json11;

//plus de donn…es sur:
//ftp://mawpvs.dyndns.org/Partners/AI/
//ftp://mawpvs.dyndns.org/DailySummary/



////HG - water level
//TW - water temperature
//TA - air temperature
//UD - wind direction
//US - wind speed
//UG - wind gust
//PC - precipitation
//XR - relative humidity
//PA - atmospheric pressure
//https://www.hydro.mb.ca/hydrologicalData/static/

//liste des stations
//https ://www.hydro.mb.ca/hydrologicalData/static/data/stationdata.json?

//coordonner des stations
//https://www.hydro.mb.ca/hydrologicalData/static/stations/05TG746/station.html?



//<table border="0"  cellspacing="0" cellpadding="0">
//</table>

//https://www.hydro.mb.ca/hydrologicalData/static/stations/05TG746/Parameter/HG/ContinuousWeek.xls
//https ://www.hydro.mb.ca/hydrologicalData/static/stations/05TG746/station.html?



//https://www.hydro.mb.ca/hydrologicalData/static/stations/05KL701/Parameter/TA/DayMeanYear.html
//https://www.hydro.mb.ca/hydrologicalData/static/stations/05KL701/Parameter/TA/DayMeanYear.html
//https://www.hydro.mb.ca/hydrologicalData/static/data/graph.json?v=20160613054508
//view-source:https://www.hydro.mb.ca/hydrologicalData/static/stations/05KL701/station.html?v=20160513054508
//https://www.hydro.mb.ca/hydrologicalData/static/

//Table60ElementPos	Table60ElementName	Table60ElementDesc	Table60ElementUnits
//1	TMSTAMP	time stamp of record at interval completion	CDST "yyyy-mm-dd hh:mm"
//2	RECNBR	seq table record number from prog change / start	int
//3	StnID	unique 3 digit Station Identifier for MAWP	int
//4	BatMin	minimum battery voltage prev 1 hour	volts(v)
//5	Air_T	avg of 5 sec temp vals prev 1 min	deg C(∞C)
//6	AvgAir_T	avg of 5 sec temp vals prev 1 hour	deg C(∞C)
//7	MaxAir_T	maximum of 1 min avg temp vals prev 1 hour	deg C(∞C)
//8	MinAir_T	minimum of 1 min avg temp vals prev 1 hour	deg C(∞C)
//9	RH	avg of 5 sec RH vals prev 1 min	% RH(%)
//10	AvgRH	avg of 5 sec RH vals prev 1 hour	% RH(%)
//11	Rain	total rain measured prev 1 hour(Primary gauge)	millimetres(m.m.)
//12	Rain24RT	total rain running total since midnight CDST	millimetres(m.m.)
//13	WS_10Min	scalar mean wind speed prev 10 min	metres per second(m / s)
//14	WD_10Min	derived vector wind dir prev 10 min	degrees true (∞)
//15	AvgWS	scalar mean wind speed prev 1 hour	metres per second(m / s)
//16	AvgWD	derived vector wind direction prev 1 hour	degrees true (∞)
//17	AvgSD	derived std deviation of vector dir prev 1 hour	degrees true (∞)
//18	MaxWS_10	maximum 10 min mean wind speed prev 1 hour	metres per second(m / s)
//19	MaxWD_10	derived vector wind direction for max 10min mean	degrees true (∞)
//20	MaxWS	maximum wind speed of 5 sec vals prev 1 hour	metres per second(m / s)
//21	HmMaxWS	timestamp of occurance of MaxWS	CDST "yyyy-mm-dd hh:mm:ss.msms"
//22	MaxWD	wind direction at MaxWS	degrees true (∞)
//23	Max5WS_10	maximum wind speed of 5 sec vals prev 10 min	metres per second(m / s)
//24	Max5WD_10	wind direction at Max5WS_10	degrees true (∞)
//25	WS_2Min	scalar mean wind speed prev 2 minutes	metres per second(m / s)
//26	WD_2Min	derived vector wind direction prev 2 minutes	degrees true (∞)
//27	Soil_T05	avg of 5 sec temp vals prev 1 min	deg C(∞C)
//28	AvgRS_kw	avg hourly solar flux density prev 1 hour	KW / m≤
//29	TotRS_MJ	total hourly solar fluxes prev 1 hour	MJ / m≤
//30	Rain2	total rain measured prev 1 hour(Secondary gauge)	millimetres(m.m.)
//31	Rain24RT2	total rain running total since midnight CDST(Secondary gauge)	millimetres(m.m.)




//Table24ElementPos	Table24ElementName	Table24ElementDesc	Table24ElementUnits
//1	TMSTAMP	time stamp of record at interval completion	CDST("yyyy-mm-dd hh:mm" - 1)
//2	RECNBR	sequential tabrecord number from prog change / start	int
//3	StnID	unique 3 digit Station Identifier for MAWP	int
//4	BatMin	minimum battery voltage prev 24 hours	volts(v)
//5	ProgSig	set by logger, changes if prog changes	int
//6	AvgAir_T	avg of prev 24 hours of 1 min avg's	deg C (∞C)
//7	MaxAir_T	max of prev 24 hours of 1 min avg's	deg C (∞C)
//8	HmMaxAir_T	timestamp of occurance of max temp	CDST "yyyy-mm-dd hh:mm:ss.msms"
//9	MinAir_T	min of prev 24 hours of 1 min avg's	deg C (∞C)
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
//20	AvgWD	derived vector wind direction prev 24 hours	degrees true (∞)
//21	AvgSD	derived std deviation of vector dir prev 24 hours	degrees true (∞)
//22	AvgSoil_T05	avg of 5 sec vals prev 24 hours	deg C(∞C)
//23	MaxSoil_T05	maximum of 5 sec vals prev 24 hours	deg C(∞C)
//24	MinSoil_T05	minimum of 5 sec vals prev 24 hours	deg C(∞C)
//25	AvgRS_kw	avg hourly solar flux density	KW / m≤
//26	MaxRS_kw	maximum hourly solar flux density	KW / m≤
//27	HmMaxRS	timestamp of maximum hourly solar flux density	CDST "yyyy-mm-dd hh:mm:ss.msms"
//28	TotRS_MJ	total solar fluxes previous 24 hours	MJ / m≤
//29	Rain2	total rain measured prev 24 hours(secondary)	millimetres(m.m.)


namespace WBSF
{

	const char* CUIManitoba::SUBDIR_NAME[NB_NETWORKS] = { "Agriculture", "Agriculture(historical)", "Fire", "Hydro" };
	const char* CUIManitoba::NETWORK_NAME[NB_NETWORKS] = { "Manitoba Agriculture", "Manitoba Agriculture (historical)", "Manitoba Fire", "Manitoba Hydro" };
	const char* CUIManitoba::SERVER_NAME[NB_NETWORKS] = { "mawpvs.dyndns.org", "tgs.gov.mb.ca", "www.gov.mb.ca", "www.hydro.mb.ca" };
	const char* CUIManitoba::SERVER_PATH[NB_NETWORKS] = { "Tx_DMZ/", "climate/", "sd/fire/", "hydrologicalData/static/stations" };

	size_t CUIManitoba::GetNetwork(const string& network)
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
		case NETWORK:	str = "0=Manitoba Agriculture|1=Manitoba Agriculture (historical)|2=Manitoba fire|3=Manitoba Hydro"; break;
		case DATA_TYPE:	str = GetString(IDS_STR_DATA_TYPE); break;
		};
		return str;
	}

	std::string CUIManitoba::Default(size_t i)const
	{
		string str;

		switch (i)
		{
		case WORKING_DIR: str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()) + "Manitoba\\"; break;
		case NETWORK:	str = "0|1|2|3"; break;
		case FIRST_YEAR:
		case LAST_YEAR:	str = ToString(CTRef::GetCurrentTRef().GetYear()); break;
		case DATA_TYPE: str = "0"; break;
		};

		return str;
	}

	//****************************************************


	std::string CUIManitoba::GetStationsListFilePath(size_t network)const
	{
		static const char* FILE_NAME[NB_NETWORKS] = { "ManitobaAgriStations.csv", "ManitobaAgriStations.csv", "ManitobaFireStations.csv", "ManitobaHydroStations.csv" };

		string filePath = WBSF::GetApplicationPath() + "Layers\\" + FILE_NAME[network];// ManitobaAgStations.csv";
		return filePath;
	}

	string CUIManitoba::GetOutputFilePath(size_t network, size_t type, const string& title, int year, size_t m)const
	{
		string strType = (type == HOURLY_WEATHER) ? "Hourly" : "Daily";
		return GetDir(WORKING_DIR) + SUBDIR_NAME[network] + "\\" + strType + "\\" + ToString(year) + "\\" + (m != NOT_INIT ? FormatA("%02d\\", m+1).c_str() : "") + title + ".csv";
	}


	ERMsg CUIManitoba::Execute(CCallback& callback)
	{
		ERMsg msg;

		
		StringVector networkV(Get(NETWORK), "|");
		bitset<NETWORK> network;
		for (size_t n = 0; n < NB_NETWORKS; n++)
			if (networkV.empty() || networkV.Find(ToString(n)) != NOT_INIT)
				network.set(n);


		callback.PushTask("Download Manitoba Network", network.count());

		for (size_t n = 0; n < NB_NETWORKS; n++)
		{
			if (network[n])
			{
				string workingDir = GetDir(WORKING_DIR) + SUBDIR_NAME[n] + "\\";
				msg = CreateMultipleDir(workingDir);


				callback.AddMessage(GetString(IDS_UPDATE_DIR));
				callback.AddMessage(workingDir, 1);
				callback.AddMessage(GetString(IDS_UPDATE_FROM));
				callback.AddMessage(string(SERVER_NAME[n]) + "/" + SERVER_PATH[n], 1);
				callback.AddMessage("");

				switch (n)
				{
				case AGRI: msg = ExecuteAgriculture(callback); break;
				case HAGRI: msg = ExecuteHistoricalAgriculture(callback); break;
				case FIRE:
				case HYDRO:	msg = ExecuteHydro(callback); break;
				default: ASSERT(false);
				}

				msg += callback.StepIt();
			}//if selected network
		}//for all newtwork


		callback.PopTask();


		return msg;
	}


	ERMsg CUIManitoba::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;

		StringVector networks( Get(NETWORK), "|") ;

		m_stations.clear();
		for (size_t n = 0; n < NB_NETWORKS; n++)
		{
			if (networks.empty() || networks.Find(to_string(n)) != NOT_INIT)
			{
				CLocationVector locations;
				msg = locations.Load(GetStationsListFilePath(n));

				if (msg)
					msg += locations.IsValid();

				//Update network
				for (size_t i = 0; i < locations.size(); i++)
					locations[i].SetSSI("Network", NETWORK_NAME[n]);

				m_stations.insert(m_stations.end(), locations.begin(), locations.end());
				
				for (size_t i = 0; i < locations.size(); i++)
					stationList.push_back(ToString(n)+"/"+locations[i].m_ID);
			}
		}

		if (msg)
		{
			
		}


		return msg;
	}

	ERMsg CUIManitoba::GetWeatherStation(const std::string& NID, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;

		size_t n = ToSizeT(NID.substr(0,1));
		string ID = NID.substr(2);

		//Get station information
		size_t it = m_stations.FindByID(ID);
		ASSERT(it != NOT_INIT);
		/*{
			msg.ajoute(FormatMsg(IDS_NO_STATION_INFORMATION, ID));
			return msg;
		}*/

		((CLocation&)station) = m_stations[it];
		//string network = station.GetSSI("Network");
		//size_t n = GetNetwork(network);
	/*	if (n == NOT_INIT)
		{
			msg.ajoute("Invalid network for: " + ID);
			return msg;
		}*/
		//station.m_name = TraitFileName(station.m_name);

		size_t type = as<size_t>(DATA_TYPE);
		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = lastYear - firstYear + 1;
		station.CreateYears(firstYear, nbYears);

		station.m_name = PurgeFileName(station.m_name);

		//now extract data 
		for (size_t y = 0; y < nbYears&&msg; y++)
		{
			int year = firstYear + int(y);

			if (n == HAGRI)
			{
				for (size_t m = 0; m < 12 && msg; m++)
				{
					string filePath = GetOutputFilePath(n, type, ID, year, m);
					if (FileExists(filePath))
						msg = station.LoadData(filePath, -999, false);

					msg += callback.StepIt(0);
				}
			}
			else
			{
				string filePath = GetOutputFilePath(n, type, ID, year);
				if (FileExists(filePath))
					msg = station.LoadData(filePath, -999, false);

				msg += callback.StepIt(0);
			}

			
		}

		station.CleanUnusedVariable("TN T TX P TD H WS WD R Z");

		//verify station is valid
		if (msg && station.HaveData())
		{
			//verify station is valid
			msg = station.IsValid();
		}

		return msg;
	}


	//**********************************************************************************************************
	ERMsg CUIManitoba::GetAgriFileList(CFileInfoVector& fileList, CCallback& callback)const
	{
		ERMsg msg;


		fileList.clear();

		//open a connection on the server
		CInternetSessionPtr pSession;
		CFtpConnectionPtr pConnection;

		ERMsg msgTmp = GetFtpConnection(SERVER_NAME[AGRI], pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", true);
		if (msgTmp)
		{
			pSession->SetOption(INTERNET_OPTION_RECEIVE_TIMEOUT, 15000);
			string command = as<size_t>(DATA_TYPE) == HOURLY_WEATHER ? "*60raw.txt" : "*24raw.txt";
			msgTmp = FindFiles(pConnection, string(SERVER_PATH[AGRI]) + command, fileList, callback);
		}


		return msg;
	}




	ERMsg CUIManitoba::ExecuteAgriculture(CCallback& callback)
	{
		ERMsg msg;


		size_t type = as<size_t>(DATA_TYPE);


		string fileName = type == HOURLY_WEATHER ? "mawp60raw.txt" : "mawp24raw.txt";;
		string remoteFilePath = "Tx_DMZ/" + fileName;
		string outputFilePath = GetDir(WORKING_DIR) + SUBDIR_NAME[AGRI] + "\\" + fileName;


		int nbRun = 0;
		bool bDownloaded = false;

		while (!bDownloaded && nbRun < 5 && msg)
		{
			nbRun++;

			CInternetSessionPtr pSession;
			CFtpConnectionPtr pConnection;

			ERMsg msgTmp = GetFtpConnection(SERVER_NAME[AGRI], pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", true);
			if (msgTmp)
			{
				TRY
				{
					callback.PushTask(GetString(IDS_UPDATE_FILE), NOT_INIT);
					msgTmp += CopyFile(pConnection, remoteFilePath, outputFilePath, INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_DONT_CACHE);
					callback.PopTask();

					//split data in seperate files
					if (msgTmp)
					{
						ASSERT(FileExists(outputFilePath));
						SplitAgriStations(outputFilePath, callback);
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

		return msg;
	}



	ERMsg CUIManitoba::SplitAgriStations(const string& outputFilePath, CCallback& callback)
	{
		ERMsg msg;

		if (m_stations.empty())
			msg = m_stations.Load(GetStationsListFilePath(AGRI));

		size_t type = as<size_t>(DATA_TYPE);
		CTM TM(type == HOURLY_WEATHER ? CTM::HOURLY : CTM::DAILY);

		std::map<string, CWeatherYears> data;

		ifStream file;
		msg = file.open(outputFilePath);
		if (msg)
		{
			callback.PushTask("Split Manitoba data", file.length());

			CWeatherAccumulator stat(TM);
			string lastID;

			enum TCommonyColumns{ TMSTAMP, RECNBR, STNID, BATMIN, COMMON_COLUMNS };
			enum THourlyColumns{ AIR_T_H = COMMON_COLUMNS, AVGAIR_T_H, MAXAIR_T_H, MINAIR_T_H, RH_AVG_H, AVGRH_H, RAIN_H, RAIN24RT_H, WS_10MIN_H, WD_10MIN_H, AVGWS_H, AVGWD_H, AVGSD_H, MAXWS_10_H, MAXWD_10_H, MAXWS_H, HMMAXWS_H, MAXWD_H, MAX5WS_10_H, MAX5WD_10_H, WS_2MIN_H, WD_2MIN_H, SOIL_T05_H, AVGRS_KW_H, TOTRS_MJ_H, RAIN2_H, RAIN24RT2_H, NB_COLUMNS_H };
			static const size_t COL_POS_H[NB_COLUMNS_H] = { -1, -1, -1, -1, -1, H_TAIR2, H_TMAX2, H_TMIN2, -1, H_RELH, H_PRCP, -1, -1, -01, H_WND2, H_WNDD, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, H_SRAD2, -1, H_PRCP, -1 };
			enum TDailyColumns{ PROGSIG_D = COMMON_COLUMNS, AVGAIR_T_D, MAXAIR_T_D, HMMAXAIR_T_D, MINAIR_T_D, HMMINAIR_T_D, AVGRH_D, MAXRH_D, HMMAXRH_D, MINRH_D, HMMINRH_D, RAIN_D, MAXWS_D, HMMAXWS_D, AVGWS_D, AVGWD_D, AVGSD_D, AVGSOIL_T05_D, MAXSOIL_T05_D, MINSOIL_T05_D, AVGRS_KW_D, MAXRS_KW_D, HMMAXRS_D, TOTRS_MJ_D, RAIN2_D, NB_COLUMNS_D };
			static const size_t COL_POS_D[NB_COLUMNS_D] = { -1, -1, -1, -1, -1, H_TAIR2, H_TMAX2, -1, H_TMIN2, -1, H_RELH, H_RELH, -1, H_RELH, -1, H_PRCP, -1, -1, H_WND2, H_WNDD, -1, -1, -1, -1, H_SRAD2, -1, -1, -1, -1 };

			bool b10m = true;
			for (CSVIterator loop(file, ",", false); loop != CSVIterator() && msg; ++loop)
			{
				size_t nbCols = type == HOURLY_WEATHER ? NB_COLUMNS_H : NB_COLUMNS_D;
				//ASSERT(loop->size() == nbCols || loop->size()==27);

				if (loop->size() == nbCols || loop->size() == 27)
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
					{
						lastID = ID;
						size_t it = m_stations.FindByID(ID);
						if (it != NOT_INIT)
							b10m = m_stations[it].GetSSI("WindHeight") == "10";

					}


					if (stat.TRefIsChanging(TRef) || ID != lastID)
					{
						if (data.find(lastID) == data.end())
						{
							data[lastID] = CWeatherYears(type == HOURLY_WEATHER);
							//try to load old data before changing it...
							string filePath = GetOutputFilePath(AGRI, type, ID, year);
							data[lastID].LoadData(filePath, -999, false);//don't erase other years when multiple years

						}
						//data[lastID].HaveData()
						data[lastID][stat.GetTRef()].SetData(stat);
						lastID = ID;
						size_t it = m_stations.FindByID(ID);
						if (it != NOT_INIT)
							b10m = m_stations[it].GetSSI("WindHeight") == "10";

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
								{
									value *= 3600 / 1000;//convert m/s into km/h
									//some station is at 2 meters and some at 10 meters
									if (b10m)
										cPos = H_WNDS;
								}

								if (cPos == H_RELH)
									value = max(1.0, min(100.0, value));


								if (cPos == H_WNDS || cPos == H_WND2)
								{
									ASSERT(value < 100);
								}

								if (cPos == H_TAIR2 || cPos == H_TMIN2 || cPos == H_TMAX2)
								{
									ASSERT(value > -60 && value < 60);
								}


								stat.Add(TRef, cPos, value);
								if (type == HOURLY_WEATHER && cPos == H_RELH)
								{
									double T = ToDouble((*loop)[AVGAIR_T_H]);
									double Hr = value;
									if (T > -99 && Hr > -99)
										stat.Add(TRef, H_TDEW, Hr2Td(T, Hr));
								}
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
						string filePath = GetOutputFilePath(AGRI, type, it1->first, it2->first);
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

	//******************************************************************************************************

	static double GetWindDir(string compass)
	{
		static const char* COMPASS[16] = { "N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE", "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW" };

		double wd = -999;
		for (size_t i = 0; i < 16; i++)
		{
			if (compass == COMPASS[i])
			{
				wd = i*22.5;
				break;
			}
		}

		return wd;
	}

	ERMsg CUIManitoba::DownloadStationData(CHttpConnectionPtr& pConnection, size_t type, const std::string& ID, CTRef TRef, std::string& text)
	{
		ERMsg msg;


		CString URL; 
		//CString strHeaders = _T("Content-Type: application/x-www-form-urlencoded\r\n");
		CStringA strParam;


		if (type == HOURLY_WEATHER)
		{
			URL = _T("climate/HourlyReport.aspx");
			string strDate = FormatA("%4d-%02d-%02d", TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1);
			strParam += "__VIEWSTATE=%2FwEPDwULLTEwOTU4OTMxNzYPZBYCZg9kFgICAw9kFgICAQ9kFgYCAQ9kFgQCAQ9kFgICAQ9kFgJmDxAPFgYeDURhdGFUZXh0RmllbGQFC1N0YXRpb25OYW1lHg5EYXRhVmFsdWVGaWVsZAUFU3RuSUQeC18hRGF0YUJvdW5kZ2QQFSgGQWx0b25hBkFyYm9yZwZCaXJ0bGUKQm9pc3NldmFpbglEZWxvcmFpbmUGRHVnYWxkCUVsbSBDcmVlawlFcmlrc2RhbGUJRXRoZWxiZXJ0B0ZvcnJlc3QJR2xhZHN0b25lCEdsZW5ib3JvCUdyYW5kdmlldwdIYW1pb3RhCUtpbGxhcm5leQlMZXRlbGxpZXIHTWFuaXRvdQxNZWxpdGEgU291dGgJTWlubmVkb3NhCU1vb3NlaG9ybgZNb3JyaXMHUGllcnNvbgxQb3J0YWdlIEVhc3QGUmVzdG9uB1J1c3NlbGwHU2Vsa2lyawhTb21lcnNldAZTb3VyaXMJU3QgUGllcnJlC1N0LiBBZG9scGhlCFN0YXJidWNrCVN0ZS4gUm9zZQlTdGVpbmJhY2gLU3dhbiBWYWxsZXkGVGV1bG9uCFRyZWhlcm5lBlZpcmRlbghXYXdhbmVzYQ1XaW5rbGVyIENNQ0RDCVdvb2RsYW5kcxUoAzI0NAMyMDYDMjE2AzIwOQMyNDEDMjE3AzIzNwMyMTgDMjEzAzIzMwMyMDQDMjM5AzIxOQMyMTQDMjIwAzIzOAMyNDIDNzQwAzIyMQMyMDUDMjIyAzIzMgMyMzUDMjQ1AzIxNQMyMTADMjQ2AzIwOAMyMDMDMjQzAzIwMgMyMTEDMjIzAzIzMQMyMDcDMjAxAzIyNAMyNDADMjMwAzIyNRQrAyhnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZGQCAg9kFgICAQ9kFgICAg8PZBYMHgtvbk1vdXNlT3ZlcgU8Q2hhbmdlTW91c2UoJ2N0bDAwX0RlZmF1bHRDb250ZW50X2ltZ0hSRGF0ZScsICdvbk1vdXNlT3ZlcicpHgpvbk1vdXNlT3V0BTtDaGFuZ2VNb3VzZSgnY3RsMDBfRGVmYXVsdENvbnRlbnRfaW1nSFJEYXRlJywgJ29uTW91c2VPdXQnKR4Lb25Nb3VzZURvd24FPUNoYW5nZUJvcmRlcignY3RsMDBfRGVmYXVsdENvbnRlbnRfaW1nSFJEYXRlJywgJ29uTW91c2VEb3duJykeCW9uTW91c2VVcAU7Q2hhbmdlQm9yZGVyKCdjdGwwMF9EZWZhdWx0Q29udGVudF9pbWdIUkRhdGUnLCAnb25Nb3VzZVVwJykeB29uQ2xpY2sFLlNob3dDYWxlbmRhcignY3RsMDBfRGVmYXVsdENvbnRlbnRfdHh0SFJEYXRlJykeCm9uS2V5UHJlc3MFLlNob3dDYWxlbmRhcignY3RsMDBfRGVmYXVsdENvbnRlbnRfdHh0SFJEYXRlJylkAgMPZBYEZg9kFgJmD2QWAmYPDxYCHgRUZXh0BSE8Yj5Ib3VybHkgUmF3IERhdGEgZm9yIEFsdG9uYTwvYj5kZAIBD2QWAmYPZBYCZg8PFgIfCQUYPGI%2BRmVicnVhcnkgMDEsIDIwMTc8L2I%2BZGQCBQ88KwANAGQYAQUjY3RsMDAkRGVmYXVsdENvbnRlbnQkZ2RIb3VybHlSZXBvcnQPZ2RqOfHRBZakZ7g%2FfO653G9kKVd%2Fqg%3D%3D&";
			strParam += "__EVENTVALIDATION=%2FwEWKwLgns6JDALx5%2BaKCAKvkdK9BAKvka69BAKO%2Ba6hCAKmo4nLCQKAusjKDgKAusDKDgK10IiUBgKc3fD9BQKc3cj9BQLx55aLCAKO%2BaKhCAKO%2BaqhCALx55KLCALNmpe%2BBwK10ICUBgK7tKvgAwLGmu%2B9BwKmo7HLCQLKiLCgAgK7tNPgAwK7tK%2FgAwLKiISgAgLKiICgAgLKiIygAgLNmpu%2BBwKvkaK9BAK10IyUBgKc3fT9BQKc3cT9BQK7tNvgAwKmo7XLCQKc3cz9BQKmo43LCQKAuszKDgKmo7nLCQLx5%2B6KCALNmu%2B9BwLNmpO%2BBwLKiIigAgKho5PqDALa8rvJA0HoHMqTYMMp8eVkb8x3GI60%2FxwX&";
			strParam += ("ctl00%24DefaultContent%24cboStationNames=" + ID + "&").c_str();
			strParam += ("ctl00%24DefaultContent%24txtHRDate=" + strDate + "&").c_str();
			strParam += "ctl00%24DefaultContent%24btn_HRSearch=Submit";
			//strParam = "__VIEWSTATE=%2FwEPDwULLTEwOTU4OTMxNzYPZBYCZg9kFgICAw9kFgICAQ9kFgYCAQ9kFgQCAQ9kFgICAQ9kFgJmDxAPFgYeDURhdGFUZXh0RmllbGQFC1N0YXRpb25OYW1lHg5EYXRhVmFsdWVGaWVsZAUFU3RuSUQeC18hRGF0YUJvdW5kZ2QQFSgGQWx0b25hBkFyYm9yZwZCaXJ0bGUKQm9pc3NldmFpbglEZWxvcmFpbmUGRHVnYWxkCUVsbSBDcmVlawlFcmlrc2RhbGUJRXRoZWxiZXJ0B0ZvcnJlc3QJR2xhZHN0b25lCEdsZW5ib3JvCUdyYW5kdmlldwdIYW1pb3RhCUtpbGxhcm5leQlMZXRlbGxpZXIHTWFuaXRvdQxNZWxpdGEgU291dGgJTWlubmVkb3NhCU1vb3NlaG9ybgZNb3JyaXMHUGllcnNvbgxQb3J0YWdlIEVhc3QGUmVzdG9uB1J1c3NlbGwHU2Vsa2lyawhTb21lcnNldAZTb3VyaXMJU3QgUGllcnJlC1N0LiBBZG9scGhlCFN0YXJidWNrCVN0ZS4gUm9zZQlTdGVpbmJhY2gLU3dhbiBWYWxsZXkGVGV1bG9uCFRyZWhlcm5lBlZpcmRlbghXYXdhbmVzYQ1XaW5rbGVyIENNQ0RDCVdvb2RsYW5kcxUoAzI0NAMyMDYDMjE2AzIwOQMyNDEDMjE3AzIzNwMyMTgDMjEzAzIzMwMyMDQDMjM5AzIxOQMyMTQDMjIwAzIzOAMyNDIDNzQwAzIyMQMyMDUDMjIyAzIzMgMyMzUDMjQ1AzIxNQMyMTADMjQ2AzIwOAMyMDMDMjQzAzIwMgMyMTEDMjIzAzIzMQMyMDcDMjAxAzIyNAMyNDADMjMwAzIyNRQrAyhnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZGQCAg9kFgICAQ9kFgICAg8PZBYMHgtvbk1vdXNlT3ZlcgU8Q2hhbmdlTW91c2UoJ2N0bDAwX0RlZmF1bHRDb250ZW50X2ltZ0hSRGF0ZScsICdvbk1vdXNlT3ZlcicpHgpvbk1vdXNlT3V0BTtDaGFuZ2VNb3VzZSgnY3RsMDBfRGVmYXVsdENvbnRlbnRfaW1nSFJEYXRlJywgJ29uTW91c2VPdXQnKR4Lb25Nb3VzZURvd24FPUNoYW5nZUJvcmRlcignY3RsMDBfRGVmYXVsdENvbnRlbnRfaW1nSFJEYXRlJywgJ29uTW91c2VEb3duJykeCW9uTW91c2VVcAU7Q2hhbmdlQm9yZGVyKCdjdGwwMF9EZWZhdWx0Q29udGVudF9pbWdIUkRhdGUnLCAnb25Nb3VzZVVwJykeB29uQ2xpY2sFLlNob3dDYWxlbmRhcignY3RsMDBfRGVmYXVsdENvbnRlbnRfdHh0SFJEYXRlJykeCm9uS2V5UHJlc3MFLlNob3dDYWxlbmRhcignY3RsMDBfRGVmYXVsdENvbnRlbnRfdHh0SFJEYXRlJylkAgMPZBYEZg9kFgJmD2QWAmYPDxYCHgRUZXh0BSE8Yj5Ib3VybHkgUmF3IERhdGEgZm9yIEFyYm9yZzwvYj5kZAIBD2QWAmYPZBYCZg8PFgIfCQUYPGI%2BRmVicnVhcnkgMDEsIDIwMTc8L2I%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%2BNvz9apC1x8iUpxMt3dM&__EVENTVALIDATION=%2FwEWKwL8idyADQLx5%2BaKCAKvkdK9BAKvka69BAKO%2Ba6hCAKmo4nLCQKAusjKDgKAusDKDgK10IiUBgKc3fD9BQKc3cj9BQLx55aLCAKO%2BaKhCAKO%2BaqhCALx55KLCALNmpe%2BBwK10ICUBgK7tKvgAwLGmu%2B9BwKmo7HLCQLKiLCgAgK7tNPgAwK7tK%2FgAwLKiISgAgLKiICgAgLKiIygAgLNmpu%2BBwKvkaK9BAK10IyUBgKc3fT9BQKc3cT9BQK7tNvgAwKmo7XLCQKc3cz9BQKmo43LCQKAuszKDgKmo7nLCQLx5%2B6KCALNmu%2B9BwLNmpO%2BBwLKiIigAgKho5PqDALa8rvJA7c1QqPsKYBHGBLaTpsDkbilYJJd&ctl00%24DefaultContent%24cboStationNames=240&ctl00%24DefaultContent%24txtHRDate=2017-02-01&ctl00%24DefaultContent%24btn_HRSearch=Submit";
		}
		else
		{
			URL = _T("climate/DailyReport.aspx");
			string strDate1 = FormatA("%4d-%02d-%02d", TRef.GetYear(), TRef.GetMonth() + 1, 1);
			string strDate2 = FormatA("%4d-%02d-%02d", TRef.GetYear(), TRef.GetMonth() + 1, GetNbDayPerMonth(TRef.GetYear(), TRef.GetMonth()));
			strParam += "__VIEWSTATE=%2FwEPDwUJMjE5Njc0MjAyD2QWAmYPZBYCAgMPZBYCAgEPZBYGAgEPZBYGAgEPZBYCAgEPZBYCZg8QDxYGHg1EYXRhVGV4dEZpZWxkBQtTdGF0aW9uTmFtZR4ORGF0YVZhbHVlRmllbGQFBVN0bklEHgtfIURhdGFCb3VuZGdkEBU%2BBkFsdG9uYQZBcmJvcmcGQmlydGxlCkJvaXNzZXZhaW4HQnJhbmRvbghDYXJiZXJyeQZDYXJtYW4NQ3lwcmVzcyBSaXZlcgdEYXVwaGluCURlbG9yYWluZQZEdWdhbGQJRWxtIENyZWVrC0VtZXJzb24gQXV0CUVyaWtzZGFsZQlFdGhlbGJlcnQNRmlzaGVyIEJyYW5jaAdGb3JyZXN0BUdpbWxpCUdsYWRzdG9uZQhHbGVuYm9ybwlHcmFuZHZpZXcGR3JldG5hB0hhbWlvdGEJS2lsbGFybmV5CUxldGVsbGllcgdNYW5pdG91CE1jQ3JlYXJ5Bk1lbGl0YQxNZWxpdGEgU291dGgJTWlubmVkb3NhCU1vb3NlaG9ybgpNb3JkZW4gQ0RBBk1vcnJpcwdQaWVyc29uC1BpbG90IE1vdW5kClBpbmF3YSBBdXQHUG9ydGFnZQxQb3J0YWdlIEVhc3QGUmVzdG9uClJvYmxpbiBBVVQHUnVzc2VsbAdTZWxraXJrClNob2FsIExha2UIU29tZXJzZXQGU291cmlzC1NwcmFndWUgQVVUCVN0IFBpZXJyZQtTdC4gQWRvbHBoZQhTdGFyYnVjawlTdGUuIFJvc2UJU3RlaW5iYWNoClN3YW4gUml2ZXILU3dhbiBWYWxsZXkGVGV1bG9uB1RoZSBQYXMIVHJlaGVybmUGVmlyZGVuCldhc2FnYW1pbmcIV2F3YW5lc2ENV2lua2xlciBDTUNEQxBXaW5uaXBlZyBBaXJwb3J0CVdvb2RsYW5kcxU%2BAzI0NAMyMDYDMjE2AzIwOQEyATQBNQE4ATkDMjQxAzIxNwMyMzcCMTEDMjE4AzIxMwI1NgMyMzMCMTQDMjA0AzIzOQMyMTkCMTcDMjE0AzIyMAMyMzgDMjQyAjI1AjI2Azc0MAMyMjEDMjA1AjI5AzIyMgMyMzICMzQCMzUCMzcDMjM1AzI0NQIzOAMyMTUDMjEwAjQwAzI0NgMyMDgCNDEDMjAzAzI0MwMyMDIDMjExAzIyMwI0NAMyMzEDMjA3AjQ1AzIwMQMyMjQCNTEDMjQwAzIzMAI1MgMyMjUUKwM%2BZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dkZAICD2QWAgIBD2QWAgICDw9kFgweC29uTW91c2VPdmVyBT9DaGFuZ2VNb3VzZSgnY3RsMDBfRGVmYXVsdENvbnRlbnRfaW1nUGxhbnREYXRlJywgJ29uTW91c2VPdmVyJykeCm9uTW91c2VPdXQFPkNoYW5nZU1vdXNlKCdjdGwwMF9EZWZhdWx0Q29udGVudF9pbWdQbGFudERhdGUnLCAnb25Nb3VzZU91dCcpHgtvbk1vdXNlRG93bgVAQ2hhbmdlQm9yZGVyKCdjdGwwMF9EZWZhdWx0Q29udGVudF9pbWdQbGFudERhdGUnLCAnb25Nb3VzZURvd24nKR4Jb25Nb3VzZVVwBT5DaGFuZ2VCb3JkZXIoJ2N0bDAwX0RlZmF1bHRDb250ZW50X2ltZ1BsYW50RGF0ZScsICdvbk1vdXNlVXAnKR4Hb25DbGljawUxU2hvd0NhbGVuZGFyKCdjdGwwMF9EZWZhdWx0Q29udGVudF90eHRQbGFudERhdGUnKR4Kb25LZXlQcmVzcwUxU2hvd0NhbGVuZGFyKCdjdGwwMF9EZWZhdWx0Q29udGVudF90eHRQbGFudERhdGUnKWQCAw9kFgICAQ9kFgICAg8PZBYMHwMFPUNoYW5nZU1vdXNlKCdjdGwwMF9EZWZhdWx0Q29udGVudF9pbWdFbmREYXRlJywgJ29uTW91c2VPdmVyJykfBAU8Q2hhbmdlTW91c2UoJ2N0bDAwX0RlZmF1bHRDb250ZW50X2ltZ0VuZERhdGUnLCAnb25Nb3VzZU91dCcpHwUFPkNoYW5nZUJvcmRlcignY3RsMDBfRGVmYXVsdENvbnRlbnRfaW1nRW5kRGF0ZScsICdvbk1vdXNlRG93bicpHwYFPENoYW5nZUJvcmRlcignY3RsMDBfRGVmYXVsdENvbnRlbnRfaW1nRW5kRGF0ZScsICdvbk1vdXNlVXAnKR8HBS9TaG93Q2FsZW5kYXIoJ2N0bDAwX0RlZmF1bHRDb250ZW50X3R4dEVuZERhdGUnKR8IBS9TaG93Q2FsZW5kYXIoJ2N0bDAwX0RlZmF1bHRDb250ZW50X3R4dEVuZERhdGUnKWQCBQ88KwANAGQCBw88KwANAGQYAgUpY3RsMDAkRGVmYXVsdENvbnRlbnQkZ2REYWlseVJlcG9ydFJlc3VsdHMPZ2QFKmN0bDAwJERlZmF1bHRDb250ZW50JGdkRGFpbHlTdW1tYXJ5UmVzdWx0cw9nZAb64crxyS8KcBo0%2BTjdcGPYy3va&";
			strParam += "__EVENTVALIDATION=%2FwEWQgKKp428CQLx5%2BaKCAKvkdK9BAKvka69BAKO%2Ba6hCALr27hqAunbuGoC7tu4agL927hqAvLbuGoCpqOJywkCgLrIyg4CgLrAyg4C6tv0aQK10IiUBgKc3fD9BQLu28BpApzdyP0FAurbyGkC8eeWiwgCjvmioQgCjvmqoQgC6tvcaQLx55KLCALNmpe%2BBwK10ICUBgK7tKvgAwLr28RpAuvbwGkCxprvvQcCpqOxywkCyoiwoAIC69uUagK7tNPgAwK7tK%2FgAwLo28hpAujbxGkC6NvcaQLKiISgAgLKiICgAgLo25hqAsqIjKACAs2am74HAunb%2BGkCr5GivQQCtdCMlAYC6dv0aQKc3fT9BQKc3cT9BQK7tNvgAwKmo7XLCQKc3cz9BQLp28hpAqajjcsJAoC6zMoOAunbxGkCpqO5ywkC8efuiggC7tv0aQLNmu%2B9BwLNmpO%2BBwLu2%2FBpAsqIiKACApiE9P8MAs%2F%2FtpIDAtryq8EC3k0aFtwMt4xr2THkdlTkbCJ2E7M%3D&";
			strParam += ("ctl00%24DefaultContent%24cboStationNames=" + ID + "&").c_str();
			strParam += ("ctl00%24DefaultContent%24txtPlantDate=" + strDate1 + "&").c_str();
			strParam += ("ctl00%24DefaultContent%24txtEndDate=" + strDate2 + "&").c_str();
			strParam += "ctl00%24DefaultContent%24btn_DRSearch=Submit";
		}

		DWORD HttpRequestFlags = INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE;
		CHttpFile* pURLFile = pConnection->OpenRequest(CHttpConnection::HTTP_VERB_POST, URL, NULL, 1, NULL, NULL, HttpRequestFlags);


		bool bRep = false;

		if (pURLFile != NULL)
		{
			{
				TRY
				{
					CString strContentL;
					strContentL.Format(_T("Content-Length: %d\r\n"), strParam.GetLength());
					

					//if (type == HOURLY_WEATHER)
					//{
					//	//pURLFile->AddRequestHeaders(CString("Host: tgs.gov.mb.ca"));
					//	//pURLFile->AddRequestHeaders(CString("Connection: keep-alive"));
					//	pURLFile->AddRequestHeaders(strContentL);
					//	//pURLFile->AddRequestHeaders(CString("Cache-Control: max-age=0"));
					//	//pURLFile->AddRequestHeaders(CString("Origin: http://tgs.gov.mb.ca"));
					//	//pURLFile->AddRequestHeaders(CString("Upgrade-Insecure-Requests: 1"));
					//	pURLFile->AddRequestHeaders(CString("Content-Type: application/x-www-form-urlencoded\r\n"));
					//	//pURLFile->AddRequestHeaders(CString("Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\n"));
					//	//pURLFile->AddRequestHeaders(CString("Referer: http://tgs.gov.mb.ca/climate/HourlyReport.aspx\r\n"));
					//	//pURLFile->AddRequestHeaders(CString("Accept-Encoding: gzip, deflate"));
					//	//pURLFile->AddRequestHeaders(CString("Accept-Language: fr-FR,fr;q=0.8,en-US;q=0.6,en;q=0.4"));
					//}
					//else
					//{
						pURLFile->AddRequestHeaders(strContentL);
						pURLFile->AddRequestHeaders(CString("Content-Type: application/x-www-form-urlencoded\r\n"));
					//}

					// send request
					bRep = pURLFile->SendRequest(0, 0, (void*)(const char*)strParam, strParam.GetLength()) != 0;
				}
					CATCH_ALL(e)

				{


					DWORD errnum = GetLastError();


					//DWORD errnum = GetLastError();
					if (errnum == 12002 || errnum == 12029)
					{
						msg = UtilWin::SYGetMessage(*e);
					}
					else if (errnum == 12031 || errnum == 12111)
					{
						//throw a exception: server reset
						//	THROW(new CInternetException(errnum));
						msg = UtilWin::SYGetMessage(*e);
					}
					else if (errnum == 12003)
					{
						msg = UtilWin::SYGetMessage(*e);

						DWORD size = 255;
						TCHAR cause[256] = { 0 };
						InternetGetLastResponseInfo(&errnum, cause, &size);
						if (_tcslen(cause) > 0)
							msg.ajoute(UtilWin::ToUTF8(cause));
					}
					else
					{
						CInternetException e(errnum);
						msg += UtilWin::SYGetMessage(e);
					}
				}

				END_CATCH_ALL
			}

			if (msg)
			{
				if (bRep)
				{
					const short MAX_READ_SIZE = 4096;
					pURLFile->SetReadBufferSize(MAX_READ_SIZE);

					std::string tmp;
					tmp.resize(MAX_READ_SIZE);
					UINT charRead = 0;
					while ((charRead = pURLFile->Read(&(tmp[0]), MAX_READ_SIZE)) > 0)
						text.append(tmp.c_str(), charRead);
				}
				else
				{
					CString tmp;
					tmp.FormatMessage(IDS_CMN_UNABLE_LOAD_PAGE, URL);
					msg.ajoute(UtilWin::ToUTF8(tmp));
				}
			}

			pURLFile->Close();
			delete pURLFile;
		}

		return msg;
	}

	ERMsg CUIManitoba::GetHistoricalStationList(size_t dataType, StringVector& fileList, CCallback& callback)
	{
		ERMsg msg;

		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;

		msg = GetHttpConnection(SERVER_NAME[HAGRI], pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS);
		if (msg)
		{
			string str;
			msg = UtilWWW::GetPageText(pConnection, "climate/" + string(dataType == HOURLY_WEATHER ? "HourlyReport.aspx" : "DailyReport.aspx"), str);
			if (msg)
			{
				string::size_type pos1 = str.find("<table");
				string::size_type pos2 = NOT_INIT;
				if (pos1 < str.size())
				{
					pos1 = str.find("<select", pos1);
					pos2 = str.find("</select>", pos1);
				}
				
				if (pos1 != string::npos && pos2 != string::npos)
				{
					try
					{
						string xml_str = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n" + str.substr(pos1, pos2 - pos1 + 9);
						zen::XmlDoc doc = zen::parse(xml_str);

						zen::XmlIn in(doc.root());
						for (zen::XmlIn it = in["option"]; it; it.next())
						{
							string value;
							it.get()->getAttribute("value", value);
							fileList.push_back(value);
						}//for all station
					}
					catch (const zen::XmlParsingError& e)
					{
						// handle error
						msg.ajoute("Error parsing XML file: col=" + WBSF::ToString(e.col) + ", row=" + WBSF::ToString(e.row));
					}
				}
			}


			pConnection->Close();
			pSession->Close();
		
		}

		return msg;
	}

	ERMsg CUIManitoba::SaveAgricultureDailyStation(const std::string& filePath, std::string str)
	{
		ERMsg msg;

		CWeatherYears data(false);

		enum THourlyColumns{ C_DATE, C_TMAX, C_TMIN, C_TAVG, C_PPT, C_GDD, C_CHU, NB_COLUMNS };
		static const TVarH DAILY_VAR[NB_COLUMNS] = { H_SKIP, H_TMAX2, H_TMIN2, H_TAIR2, H_PRCP, H_SKIP, H_SKIP};

		//enum THourlyColumns{ C_HOUR, C_TEMP,C_RH, C_RAIN, C_WIND_SPEED, C_WIND_DIR, C_PEAK_WIND, C_SOIL_TEMP, NB_COLUMNS };
		//static const TVarH COL_POS[NB_COLUMNS] = { H_SKIP, H_TAIR2, H_TMAX2, H_TMIN2, H_RELH, H_TDEW, H_WNDS, H_WNDD, H_PRCP, H_SKIP };

		try
		{
			int ID = ToInt(GetFileTitle(filePath));
			WBSF::ReplaceString(str, "\t", "");

			zen::XmlDoc doc = zen::parse(str);

			zen::XmlIn in(doc.root());
			for (zen::XmlIn itTR = in["tr"]; itTR; itTR.next())
			{
				StringVector tmp;
				for (zen::XmlIn itTD = itTR["td"]; itTD; itTD.next())
				{
					string value;
					itTD["font"](value);
					Trim(value);
					tmp.push_back(value);
				}//for all columns

				
				if (tmp.size() == NB_COLUMNS)
				{

					StringVector date(tmp[C_DATE], " ");

					if (date.size() == 3)
					{

						int year = ToInt(date[2]);
						size_t month = WBSF::GetMonthIndex(date[0].c_str());
						size_t day = ToInt(date[1]) - 1;

						CTRef TRef(year, month, day);
						ASSERT(TRef.IsValid());

						for (size_t i = 0; i < NB_COLUMNS; i++)
						{
							if (DAILY_VAR[i] != H_SKIP && !tmp[i].empty())
							{
								double value = ToDouble(tmp[i]);
								data.GetDay(TRef).SetStat(DAILY_VAR[i], value);
							}
						}
					}
				}
			}


			if (msg)
			{
				ASSERT(data.size() == 1);
				//save annual data
				for (auto it1 = data.begin(); it1 != data.end(); it1++)
				{
					msg += it1->second->SaveData(filePath);
				}
			}//if msg
		}
		catch (const zen::XmlParsingError& e)
		{
			// handle error
			msg.ajoute("Error parsing XML file: col=" + WBSF::ToString(e.col) + ", row=" + WBSF::ToString(e.row));
		}


		return msg;
	}


	ERMsg CUIManitoba::SaveAgricultureHourlyStation(const std::string& filePath, std::string str)
	{
		ERMsg msg;

		CWeatherYears data(true);
		
		enum THourlyColumns{ C_HOUR, C_TEMP, C_RH, C_RAIN, C_WIND_SPEED, C_WIND_DIR, C_PEAK_WIND, C_SOIL_TEMP, NB_COLUMNS };
		static const TVarH HOURLY_VARS[NB_COLUMNS] = { H_SKIP, H_TAIR2, H_RELH, H_PRCP, H_WNDS, H_WNDD, H_SKIP, H_ADD1 };

		try
		{
			int ID = ToInt(GetFileTitle(filePath));
			WBSF::ReplaceString(str, "\t", "");

			zen::XmlDoc doc = zen::parse(str);

			zen::XmlIn in(doc.root());
			for (zen::XmlIn itDay = in["table"]; itDay; itDay.next())
			{
				string date;
				itDay.get()->getAttribute("date", date);
				CTRef TRef;
				TRef.FromFormatedString(date);
				TRef.Transform(CTM(CTM::HOURLY));

				for (zen::XmlIn itTR = itDay["tr"]; itTR; itTR.next())
				{
					StringVector tmp;
					for (zen::XmlIn itTD = itTR["td"]; itTD; itTD.next())
					{
						string value;
						itTD["font"](value);
						Trim(value);
						tmp.push_back(value);
					}//for all columns


					if (tmp.size() == NB_COLUMNS)
					{
						string strHour = tmp[C_HOUR];
						size_t hour = ToSizeT(strHour.substr(0,2));

						if (strHour == "12AM")
							hour = 0;
						else if (strHour.substr(2) == "PM" && strHour!="12PM")
							hour += 12;
						
						TRef.m_hour = hour;
						ASSERT(TRef.IsValid());
						
						for (size_t i = 0; i < NB_COLUMNS; i++)
						{
							if (HOURLY_VARS[i] != H_SKIP && !tmp[i].empty())
							{
								double value = (HOURLY_VARS[i] == H_WNDD)?GetWindDir(tmp[i]):ToDouble(tmp[i]);
								data[TRef].SetStat(HOURLY_VARS[i], value);
							}
						}
					}
				}
			}

			if (msg && data.size() == 1)
			{
				//save annual data
				const CWeatherYear& d = data[size_t(0)];
				msg = d.SaveData(filePath);
			}//if msg
		}
		catch (const zen::XmlParsingError& e)
		{
			// handle error
			msg.ajoute("Error parsing XML file: col=" + WBSF::ToString(e.col) + ", row=" + WBSF::ToString(e.row));
		}


		return msg;
	}

	ERMsg CUIManitoba::ExecuteHistoricalAgriculture(CCallback& callback)
	{
		ERMsg msg;


		size_t type = as<size_t>(DATA_TYPE);
		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = lastYear - firstYear + 1;
		CTRef currentTRef = CTRef::GetCurrentTRef();


		StringVector fileList;
		GetHistoricalStationList(type, fileList, callback);

	
		int nbFiles = 0;
		int nbRun = 0;
		size_t curY = 0;
		

		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;

		while (nbRun < 5 && curY<nbYears && msg)
		{
			size_t totalFiles = (lastYear < currentTRef.GetYear()) ? fileList.size()*nbYears * 12 : fileList.size()*(nbYears - 1) * 12 + fileList.size()*(currentTRef.GetMonth() + 1);
			callback.PushTask("Download Manitoba hitorical agriculture data (" + ToString(totalFiles) + " files)", totalFiles);

			nbRun++;
			msg = GetHttpConnection(SERVER_NAME[HAGRI], pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS);
			if (msg)
			{
				for (size_t y = curY; y < nbYears &&msg; y++)
				{
					int year = firstYear + int(y);
					size_t nbMonths = year < currentTRef.GetYear() ? 12 : currentTRef.GetMonth() + 1;

					for (size_t m = 0; m < nbMonths && msg; m++)
					{
						for (size_t i = 0; i < fileList.size() && msg; i++)
						{
							bool bDownload = true;
							string filePath = GetOutputFilePath(HAGRI, type, fileList[i], year, m);
							CreateMultipleDir(GetPath(filePath));

							CFileStamp fileStamp(filePath);
							CTime lu = fileStamp.m_time;
							if (lu.GetTime() > 0)
							{
								int nbDays = CTRef(lu.GetYear(), lu.GetMonth() - 1, lu.GetDay() - 1) - CTRef(year, m, LAST_DAY);

								if (nbDays > 10)
									bDownload = false;
							}


							if (bDownload)
							{
								if (type == HOURLY_WEATHER)
								{
									string source;
									size_t nbDays = year < currentTRef.GetYear() || m < currentTRef.GetMonth() ? GetNbDayPerMonth(year, m) : currentTRef.GetDay() + 1;
									callback.PushTask("Update station (" + fileList[i] + ") for " + GetMonthTitle(m) + " (" + ToString(nbDays) + " days)", nbDays);


									for (size_t d = 0; d < nbDays && msg; d++)
									{
										string str;

										CTRef TRef(year, m, d);
										msg = DownloadStationData(pConnection, type, fileList[i], TRef, str);
										string::size_type pos1 = str.find("Hourly Raw Data for");

										if (pos1 < str.size())
										{
											pos1 = str.find("<table", pos1);
											if (pos1 < str.size())
											{
												string::size_type pos2 = str.find("</table>", pos1);

												if (pos1 != string::npos && pos2 != string::npos)
												{
													source += ("<table date=\"" + TRef.GetFormatedString("%Y-%m-%d") + "\" " + str.substr(pos1+6, pos2 - (pos1+6)) + "</table>");
													msg += callback.StepIt();
												}
											}
										}
									}



									//split data in seperate files
									if (msg)
									{
										string tmp = string("<?xml version=\"1.0\" encoding=\"Windows-1252\"?>\r\n") + "<tables>" + source + "</tables>";
										msg += SaveAgricultureHourlyStation(filePath, tmp);
										nbFiles++;
										nbRun = 0;
									}
									/*else
									{
									ofStream file;
									file.open(filePath);
									file.close();
									}*/

									callback.PopTask();
								}//if hourly
								else
								{
									string str;
									msg = DownloadStationData(pConnection, type, fileList[i], CTRef(year, m), str);

									//split data in seperate files
									if (msg)
									{

										string::size_type pos1 = str.find("ctl00_DefaultContent_gdDailySummaryResults");
										string::size_type pos2 = string::npos;

										if (pos1 < str.size())
											pos1 = str.find("<table", pos1);

										if (pos1 < str.size())
											pos2 = str.find("</table>", pos1);


										if (pos1 != string::npos && pos2 != string::npos)
										{
											string tmp = "<?xml version=\"1.0\" encoding=\"Windows-1252\"?>\r\n" + str.substr(pos1, pos2 - pos1 + 9);
											msg += SaveAgricultureDailyStation(filePath, tmp);
											nbFiles++;
											nbRun = 0;
										}
										//else
										//{
										//	//save empty file to avoid download it again
										//	ofStream file;
										//	file.open(filePath);
										//	file.close();
										//}
									}//if msg
								}//data type
							}//need download

							msg += callback.StepIt();
						}//for all station
					}//for all months

					if (msg)
						curY++;
				}//for all years

				//if an error occur: try again
				if (!msg && !callback.GetUserCancel())
				{
					if (nbRun < 5)
					{
						callback.AddMessage(msg);
						msg.asgType(ERMsg::OK);

						callback.PushTask("Waiting 30 seconds for server...", 600);
						for (int i = 0; i < 600 && msg; i++)
						{
							Sleep(50);//wait 50 milisec
							msg += callback.StepIt();
						}
						callback.PopTask();
					}
				}

				//clean connection
				pConnection->Close();
				pSession->Close();
			}//if get connection

			callback.AddMessage(GetString(IDS_NB_FILES_DOWNLOADED) + ToString(nbFiles), 1);
			callback.PopTask();
		}//while


		return msg;
	}


	//******************************************************************************************************


	//HG - water level
	//TW - water temperature
	//TA - air temperature
	//UD - wind direction
	//US - wind speed
	//UG - wind gust
	//PC - precipitation
	//XR - relative humidity
	//PA - atmospheric pressure

	enum TVariables { H_WATER_LEVEL, H_WATER_TEMPERATURE, H_AIR_TEMPERATURE, H_WIND_DIRECTION, H_WIND_SPEED, H_WIND_GUST, H_PRECIPITATION, H_RELATIVE_HUMIDITY, H_ATMOSPHERIC_PRESSURE, NB_VARS };
	static char* HYDRO_VAR_NAME[NB_VARS] = { "HG", "TW", "TA", "UD", "US", "UG", "PC", "XR", "PA" };
	static const size_t HYDRO_VAR[NB_VARS] = { H_ADD1, H_ADD2, H_TAIR2, H_WNDD, H_WNDS, -1, H_PRCP, H_RELH, H_PRES };
	static size_t GetVar(string filePath)
	{
		size_t var = NOT_INIT;

		string title = GetFileTitle(filePath);
		string varID = title.substr(0, 2);
		MakeUpper(varID);
		for (size_t i = 0; i <NB_VARS && var == NOT_INIT; i++)
		{
			if (varID == HYDRO_VAR_NAME[i])
				var = HYDRO_VAR[i];
		}

		return var;
	}




	void ExcelSerialDateToDMY(double fSerialDate, int &nYear, size_t &nMonth, size_t &nDay, size_t &nHour, size_t &nMinute)
	{

		int nSerialDate = int(fSerialDate);

		// Excel/Lotus 123 have a bug with 29-02-1900. 1900 is not a
		// leap year, but Excel/Lotus 123 think it is...
		if (nSerialDate == 60)
		{
			nDay = 29;
			nMonth = 2;
			nYear = 1900;

			return;
		}
		else if (nSerialDate < 60)
		{
			// Because of the 29-02-1900 bug, any serial date 
			// under 60 is one off... Compensate.
			nSerialDate++;
		}

		// Modified Julian to DMY calculation with an addition of 2415019
		int l = nSerialDate + 68569 + 2415019;
		int n = int((4 * l) / 146097);
		l = l - int((146097 * n + 3) / 4);
		int i = int((4000 * (l + 1)) / 1461001);
		l = l - int((1461 * i) / 4) + 31;
		int j = int((80 * l) / 2447);
		nDay = l - int((2447 * j) / 80);
		l = int(j / 11);
		nMonth = j + 2 - (12 * l);
		nYear = 100 * (n - 49) + i + l;


		fSerialDate -= nSerialDate;
		fSerialDate *= 24;
		nHour = int(fSerialDate);
		fSerialDate -= nHour;
		fSerialDate *= 60;
		nMinute = int(fSerialDate + 0.5);
		if (nMinute == 60)
		{
			nHour++;
			nMinute = 0;
		}

		nMonth--;
		nDay--;
	}

	ERMsg CUIManitoba::ExecuteHydro(CCallback& callback)
	{
		ERMsg msg;

		//liste des stations
		//https ://www.hydro.mb.ca/hydrologicalData/static/data/stationdata.json?

		//</table>
		//https://www.hydro.mb.ca
		//hydrologicalData/static/stations/05TG746/Parameter/HG/ContinuousWeek.xls


		CLocationVector locations;
		msg = locations.Load(GetStationsListFilePath(HYDRO));

		if (msg)
			msg += locations.IsValid();

		if (msg)
		{

			size_t type = as<size_t>(DATA_TYPE);

			string fileName = type == HOURLY_WEATHER ? "ContinuousWeek.xls" : "DayMeanYear.xls";


			callback.PushTask("Update Manitoba Hydro weather data (" + ToString(locations.size()) + " stations)", locations.size()*NB_VARS);
			size_t curI = 0;
			int nbRun = 0;
			//bool bDownloaded = false;

			while (curI < locations.size() && nbRun < 5 && msg)
			{
				nbRun++;

				CInternetSessionPtr pSession;
				CHttpConnectionPtr pConnection;

				ERMsg msgTmp = GetHttpConnection(SERVER_NAME[HYDRO], pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS);
				if (msgTmp)
				{
					TRY
					{

						for (size_t i = curI; i < locations.size() && msg; i++)
						{
							StringVector filePath;
							string ID = locations[i].m_ID;
							//string ID = "05UH737";
							for (size_t v = 0; v < NB_VARS&&msg; v++)
							{
								if (HYDRO_VAR[v] != NOT_INIT)
								{
									string remoteFilePath = "hydrologicalData/static/stations/" + ID + "/Parameter/" + HYDRO_VAR_NAME[v] + "/" + fileName;
									string outputFilePath = GetDir(WORKING_DIR) + SUBDIR_NAME[HYDRO] + "\\" + HYDRO_VAR_NAME[v] + "_" + fileName;

									if (FileExists(outputFilePath))
										msg += RemoveFile(outputFilePath);

									
									//callback.PushTask(GetString(IDS_UPDATE_FILE), NOT_INIT);
									msgTmp += CopyFile(pConnection, remoteFilePath, outputFilePath, INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_PRAGMA_NOCACHE | INTERNET_FLAG_TRANSFER_BINARY);
									//callback.PopTask();

									//split data in seperate files
									if (msgTmp && FileExists(outputFilePath))
									{
										ifStream file;
										if (file.open(outputFilePath))
										{
											string str = file.GetText();
											file.close();

											if (str.find("Sorry, the page was not found") == NOT_INIT)
											{
												string tmpFilePath = outputFilePath;
												SetFileExtension(tmpFilePath, ".csv");

												if (FileExists(tmpFilePath))
													msg += RemoveFile(tmpFilePath);

												if (msg)
												{
													string xls2csv = GetApplicationPath() + "External\\xml2csv.exe";
													string command = xls2csv + " \"" + outputFilePath + "\" \"" + tmpFilePath + "\"";
													WinExecWait(command);

													if (FileExists(tmpFilePath))
														filePath.push_back(tmpFilePath);
													else
														msg.ajoute("Unable to convert " + GetFileName(outputFilePath) + " to " + GetFileName(tmpFilePath));
												}

											}

											
										}//if valid file
									}//if file exist
								}//for all variables

								msg += callback.StepIt();
								
							}//for all var

							if (msg && !filePath.empty())
							{
								msg += SplitHydroData(locations[i].m_ID, filePath, callback);
								if (msg)
								{
									curI++;
									//remove files
									for (size_t i = 0; i < filePath.size(); i++)
									{
										//delete .csv file
										msg += RemoveFile(filePath[i]);
										//delete .xls file
										msg = RemoveFile(SetFileExtension(filePath[i], ".xls"));
									}
										
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


			callback.AddMessage(GetString(IDS_NB_STATIONS) + ToString(curI), 1);
			callback.PopTask();
		}

		

		return msg;
	}

	
	ERMsg CUIManitoba::SplitHydroData(const string& ID, const StringVector& outputFilePath, CCallback& callback)
	{
		ASSERT(!outputFilePath.empty());
		ERMsg msg;

		//if (m_stations.empty())
			//msg = m_stations.Load(GetStationsListFilePath(AGRI));

		

		//GetOutputFilePath(HYDRO, ID, year)
		size_t type = as<size_t>(DATA_TYPE);
		CTM TM(type == HOURLY_WEATHER ? CTM::HOURLY : CTM::DAILY);

		CWeatherYears data(type == HOURLY_WEATHER);

		vector<size_t> vars(outputFilePath.size());
		vector<ifStream> files(outputFilePath.size());
		for (size_t i = 0; i < outputFilePath.size(); i++)
		{
			msg += files[i].open(outputFilePath[i]);
			vars[i] = GetVar(outputFilePath[i]);
		}
			

		if (msg)
		{
			//callback.PushTask("Split Manitoba hydro data", files.size());

			CWeatherAccumulator stat(TM);

			for (size_t i = 0; i < outputFilePath.size()&&msg; i++)
			{
				string junk;
				for (int l = 0; l < 8; l++)
					getline(files[i], junk);

				for (CSVIterator loop(files[i]); loop != CSVIterator() && msg; ++loop)
				{
					if (loop->size() >= 2)
					{
						int year = 0;
						size_t month = 0;
						size_t day = 0;
						size_t hour = 0;
						size_t minute = 0;

						double fSerialDate = ToDouble((*loop)[0]);
						ExcelSerialDateToDMY(fSerialDate, year, month, day, hour, minute);
						
						
						ASSERT(month >= 0 && month < 12);
						ASSERT(day >= 0 && day < GetNbDayPerMonth(year, month));
						ASSERT(hour >= 0 && hour < 24);
						ASSERT(minute >= 0 && minute < 60);

						CTRef TRef = type == HOURLY_WEATHER ? CTRef(year, month, day, hour) : CTRef(year, month, day);

						if (stat.TRefIsChanging(TRef) )
						{

							if (!data.IsYearInit(TRef.GetYear()))
							{
								//try to load old data before changing it...
								string filePath = GetOutputFilePath(HYDRO, type, ID, year);
								data.LoadData(filePath, -999, false);//don't erase other years when multiple years
							}


							//data[lastID].HaveData()
							data[stat.GetTRef()].SetStat( (TVarH)vars[i], stat.GetStat(vars[i]) );
						}

						double value = ToDouble((*loop)[1]);
						if (value > -99)
						{

							if (vars[i] == H_PRES)
								value *= 10;//kPa --> hPa

							//if (cPos == H_RELH)
								//value = max(1.0, min(100.0, value));

							stat.Add(TRef, vars[i], value);
							
						}//if good
						
					}//size >= 2

					msg += callback.StepIt(0);
				}//for all line 
				
				if (stat.GetTRef().IsInit())
					data[stat.GetTRef()].SetStat((TVarH)vars[i], stat.GetStat(vars[i]));


			}//for all files (variables)


			if (msg)
			{
				//Compute Tdew if hourly data
				if (type == HOURLY_WEATHER )
				{
					CTPeriod p = data.GetEntireTPeriod();
					for (CTRef h = p.Begin(); h <= p.End(); h++)
					{
						const CStatistic& T = data[h][H_TAIR2];
						const CStatistic& Hr = data[h][H_RELH];
						if (T.IsInit() && Hr.IsInit())
							data[h].SetStat(H_TDEW, Hr2Td(T[MEAN], Hr[MEAN]));
					}
				}

				//save all years 
				for (auto it = data.begin(); it != data.end(); it++)
				{
					string filePath = GetOutputFilePath(HYDRO, type, ID, it->first);
					string outputPath = GetPath(filePath);
					CreateMultipleDir(outputPath);
					it->second->SaveData(filePath, TM);
				}
			}//if msg
			
			
			//callback.PopTask();
		}//if msg

		return msg;
	}

	ERMsg CUIManitoba::UpdateHydroStationsList(CCallback& callback)
	{
		ERMsg msg;

		//get stations 
		
		//<table border="0"  cellspacing="0" cellpadding="0">
		//</table>
		//https://www.hydro.mb.ca
		//hydrologicalData/static/stations/05TG746/Parameter/HG/ContinuousWeek.xls
		//hydrologicalData/static/stations/05TG746/Parameter/HG/DayMeanYear.xls

		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;
		// /hydrologicalData/static/
		msg = GetHttpConnection("www.hydro.mb.ca", pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS);
		if (msg)
		{

			string str;
			msg = UtilWWW::GetPageText(pConnection, "hydrologicalData/static/data/stationdata.json?", str);
			if (msg)
			{
				string error;
				std::vector<Json>& items = Json::parse_multi(str, error);
				//Json json = Json::parse(str, error);

				if (!error.empty())
				{
					msg.ajoute(error);
					return msg;
				}
				else if(items.size() == 1)
				{
					const Json& item = items.front();
					bool bTest = item.is_object();
					const Json& item2 = item["features"];
					CLocationVector locations;

					//const Json& item2 = item["features"];

					ASSERT( item2.is_array() );
					for (size_t j = 0; j < item2.array_items().size(); j++)
					{
						const Json& item3 = item2[j]["attributes"];
						const Json& item4 = item2[j]["geometry"];
						//:{"x":-96.101388888889, "y" : 56.086388888889}
						string test = item3["station_latitude"].string_value();


						string name = item3["station_name"].string_value();
						string ID = item3["station_no"].string_value();
						double lat = item4["y"].number_value();
						double lon = item4["x"].number_value();
						CLocation location(name, ID, lat, lon, -999);
						location.SetSSI("Network", NETWORK_NAME[HYDRO]);
						location.SetSSI("StationNo", item3["station_id"].string_value() );
						location.SetSSI("RiverName", item3["river_name"].string_value() );
						location.SetSSI("InstallDate", item3["gen.shelterinstall"].string_value());
						locations.push_back(location);

							msg += callback.StepIt();

					}

					
					//temporary file path without elevation
					string filePath = GetDir(WORKING_DIR) + SUBDIR_NAME[HYDRO] + "\\Stations.csv";
					msg = locations.Save(filePath);

				}
			}


			//clean connection
			pConnection->Close();
			pSession->Close();
		}




		//if (!FileExists(dstFilePath))
		//{
		//string srcFilePath = GetApplicationPath() + "Layers\\CIPRAStations.csv";

		//CString src(CStringA(srcFilePath.c_str()));
		//CString dst(CStringA(dstFilePath.c_str()));
		//CopyFile(src, dst, TRUE);
		//}


		return msg;

	}
}