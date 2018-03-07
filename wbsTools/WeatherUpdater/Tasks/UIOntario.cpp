#include "StdAfx.h"
#include "UIOntario.h"
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
	//www.affes.mnr.gov.on.ca/Extranet/Bulletin_Boards/WXProducts/WxHourly.csv
	const char* CUIOntario::SERVER_NAME = "www.affes.mnr.gov.on.ca";
	//const char* CUIOntario::SERVER_PATH = "Extranet/Bulletin_Boards/WXProducts/";
	const char* CUIOntario::SERVER_PATH = "Maps/WX/";
	


	//*********************************************************************
	const char* CUIOntario::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "FirstYear", "LastYear", "Network"};
	const size_t CUIOntario::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_STRING, T_STRING, T_STRING_SELECT};
	const UINT CUIOntario::ATTRIBUTE_TITLE_ID = IDS_UPDATER_ONTARIO_P;
	const UINT CUIOntario::DESCRIPTION_TITLE_ID = ID_TASK_ONTARIO;

	const char* CUIOntario::CLASS_NAME(){ static const char* THE_CLASS_NAME = "Ontario";  return THE_CLASS_NAME; }
	CTaskBase::TType CUIOntario::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUIOntario::CLASS_NAME(), (createF)CUIOntario::create);



	CUIOntario::CUIOntario(void)
	{}

	CUIOntario::~CUIOntario(void)
	{}



	std::string CUIOntario::Option(size_t i)const
	{
		string str;
		switch (i)
		{
		case NETWORK:	str = "Ontario Fire Weather Program"; break;
		//case DATA_TYPE:	str = GetString(IDS_STR_WDATA_TYPE); break;
		};
		return str;
	}

	std::string CUIOntario::Default(size_t i)const
	{
		string str;

		switch (i)
		{
		case WORKING_DIR: str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()) + "Ontario\\"; break;
		case FIRST_YEAR:
		case LAST_YEAR:	str = ToString(CTRef::GetCurrentTRef().GetYear()); break;
		//case DATA_TYPE: str = "0"; break;
		};

		return str;
	}

	//****************************************************
	ERMsg CUIOntario::GetFileList(CFileInfoVector& fileList, CCallback& callback)const
	{
		ERMsg msg;


		fileList.clear();

		//open a connection on the server
		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;
		
		ERMsg msgTmp = GetHttpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS);
		if (msgTmp)
		{
			pSession->SetOption(INTERNET_OPTION_RECEIVE_TIMEOUT, 15000);
			//msgTmp = FindFiles(pConnection, string(SERVER_PATH) + "WxHourly.csv", fileList);
			msgTmp = FindFiles(pConnection, string(SERVER_PATH) + "wxhourly.xml", fileList);
		}
		
		return msg;
	}



	ERMsg CUIOntario::Execute(CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);
		msg = CreateMultipleDir(workingDir);


		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(workingDir, 1);
		callback.AddMessage(GetString(IDS_UPDATE_FROM));
		callback.AddMessage(string(SERVER_NAME) + "/" + SERVER_PATH, 1);
		callback.AddMessage("");

		//string fileName = "WxHourly.csv";
		string fileName = "wxhourly.xml";
		string remoteFilePath = SERVER_PATH + fileName;
		string outputFilePath = workingDir + fileName;
		

		int nbRun = 0;
		bool bDownloaded = false;

		while (!bDownloaded && nbRun < 5 && msg)
		{
			nbRun++;

			CInternetSessionPtr pSession;
			CHttpConnectionPtr pConnection;

			ERMsg msgTmp = GetHttpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS);
			if (msgTmp)
			{
				TRY
				{
					callback.PushTask(GetString(IDS_UPDATE_FILE), NOT_INIT);
					msgTmp += CopyFile(pConnection, remoteFilePath, outputFilePath, INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_TRANSFER_BINARY);
					callback.PopTask();

					//split data in seperate files
					if (msgTmp)
					{
						ASSERT(FileExists(outputFilePath));
						SplitStationsXML(outputFilePath, callback);
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


	string CUIOntario::GetOutputFilePath(const string& stationName, int year)const
	{
		string type = "Hourly";// (as<size_t>(DATA_TYPE) == HOURLY_WEATHER) ? "Hourly" : "Daily";
		return GetDir(WORKING_DIR) + type + "\\" + ToString(year) + "\\" + stationName + ".csv";
	}

	ERMsg CUIOntario::SplitStationsXML(const string& outputFilePath, CCallback& callback)
	{
		ERMsg msg;

		
		size_t nbStationsAdded = 0;
		//std::map<string, CWeatherYears> data;

		ifStream file;
		msg = file.open(outputFilePath);
		string source;
		if (msg)
			source = file.GetText();

		if (msg)
		{
			

			

			enum THourlyColumns{ STATION_CODE, OBSDATE, OBSTIME, TEMPERATURE, RELATIVE_HUMIDITY, WIND_SPEED, RAINFALL, WIND_DIRECTION, NB_COLUMNS };
			static const char* COL_NAME[NB_COLUMNS] = { "WSTNCODE", "Date", "Time", "temp", "RH", "speed", "precip", "WindDir" };
			static const TVarH  COL_POS_H[NB_COLUMNS] = { H_SKIP, H_SKIP, H_SKIP, H_TAIR2, H_RELH, H_WNDS, H_PRCP, H_WNDD };

			try
			{


				zen::XmlDoc doc = zen::parse(source);

				zen::XmlIn in(doc.root());

				size_t nbStations = std::distance(in.get()->getChildren().first, in.get()->getChildren().second);
				callback.PushTask("Split Ontario data file", nbStations);

				for (zen::XmlIn marker = in["marker"]; marker&&msg; marker.next())
				{
					string oldID;
					marker.attribute("label", oldID);
					oldID = oldID.substr(0, 3);



					string ID;
					CWeatherYears data(true);
					for (zen::XmlIn obs = marker["obs"]; obs&&msg; obs.next())
					{
						string str;
						obs.attribute(COL_NAME[OBSDATE], str);
						StringVector date(str, "-");
						obs.attribute(COL_NAME[OBSTIME], str);
						StringVector time(str, ":");

						ASSERT(date.size() == 3);
						ASSERT(time.size() == 2);

						int year = ToInt(date[0]);
						size_t m = ToInt(date[1]) - 1;
						size_t d = ToInt(date[2]) - 1;
						size_t h = ToInt(time[0]);

						ASSERT(m >= 0 && m < 12);
						ASSERT(d >= 0 && d < GetNbDayPerMonth(year, m));
						ASSERT(h >= 0 && h < 24);

						//CTRef TRef = CTRef(year, m, d, h);
						obs.attribute(COL_NAME[STATION_CODE], ID);
						if (ID[0] == '4')//strang thiink with lable begging with 4
							ID = oldID;
						
						if (!data.IsYearInit(year))
						{
							//data.CreateYear(year);
							//try to load old data before changing it...
							string filePath = GetOutputFilePath(ID, year);
							data.LoadData(filePath, -999, false);//don't erase other years when multiple years
						}

						for (size_t vv = TEMPERATURE; vv < NB_COLUMNS; vv++)
						{
							TVarH v = COL_POS_H[vv];
							ASSERT(v < NB_VAR_H);


							string str;
							obs.attribute(COL_NAME[vv], str);
							double value = ToDouble(str);
							if (!str.empty() && value > -99)
							{
								data[year][m][d][h][v] = value;
								if (v == H_RELH)
								{
									double T = data[year][m][d][h][H_TAIR2];
									if (!WEATHER::IsMissing(T))
										data[year][m][d][h][H_TDEW] = Hr2Td(T, value);
								}
							}
						}//for all variables 
						

					
						msg += callback.StepIt(0);
					}//for all observations

					//if (stat.GetTRef().IsInit() && data.find(lastID) != data.end())
					
					//data[lastID][stat.GetTRef()].SetData(stat);


					//save data
					for (auto it = data.begin(); it != data.end(); it++)
					{
						ASSERT(!ID.empty());
						string filePath = GetOutputFilePath(ID, it->first);
						string outputPath = GetPath(filePath);
						CreateMultipleDir(outputPath);
						it->second->SaveData(filePath, CTM::HOURLY);
					}


					msg += callback.StepIt();
					nbStationsAdded++;
				}//for all stations
			}
			catch (const zen::XmlParsingError& e)
			{
				// handle error
				msg.ajoute("Error parsing XML file: col=" + ToString(e.col) + ", row=" + ToString(e.row));
			}
		}//if open file

		callback.AddMessage(GetString(IDS_NB_STATIONS) + ToString(nbStationsAdded), 1);
		callback.PopTask();

		return msg;
	}
	

	std::string CUIOntario::GetStationListFilePath()const
	{
		
		return WBSF::GetApplicationPath() + "Layers\\OntarioStations.csv";
	}

	ERMsg CUIOntario::GetStationList(StringVector& stationList, CCallback& callback)
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

	ERMsg CUIOntario::GetWeatherStation(const std::string& ID, CTM TM, CWeatherStation& station, CCallback& callback)
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




	ERMsg CUIOntario::SplitStationsCSV(const string& outputFilePath, CCallback& callback)
	{
		ERMsg msg;

		CTM TM(CTM::HOURLY);

		std::map<string, CWeatherYears> data;

		ifStream file;
		msg = file.open(outputFilePath);
		if (msg)
		{
			callback.PushTask("Split data", file.length());

			CWeatherAccumulator stat(TM);
			string lastID;


			enum THourlyColumns{ STATION_CODE, OBSTIME, TEMPERATURE, RELATIVE_HUMIDITY, WIND_SPEED, WIND_DIRECTION, RAINFALL, NB_COLUMNS };
			static const WBSF::HOURLY_DATA::TVarH COL_POS_H[NB_COLUMNS] = { H_SKIP, H_SKIP, H_TAIR2, H_RELH, H_WNDS, H_WNDD, H_PRCP };

			for (CSVIterator loop(file); loop != CSVIterator() && msg; ++loop)
			{
				if (!loop->empty())
				{
					StringVector time((*loop)[OBSTIME], "-: ");
					ASSERT(time.size() == 5);

					int year = ToInt(time[0]);
					size_t month = WBSF::GetMonthIndex(time[1].c_str());
					size_t day = ToInt(time[2]) - 1;
					size_t hour = ToInt(time[3]);

					ASSERT(month >= 0 && month < 12);
					ASSERT(day >= 0 && day < GetNbDayPerMonth(year, month));
					ASSERT(hour >= 0 && hour < 24);

					CTRef TRef = CTRef(year, month, day, hour);
					string ID = (*loop)[STATION_CODE];
					//if (lastID.empty())
						//lastID = ID;


					if (ID != lastID)
					{
						if (data.find(ID) == data.end())
						{
							data[ID] = CWeatherYears(true);
							//try to load old data before changing it...
							string filePath = GetOutputFilePath(ID, year);
							data[ID].LoadData(filePath, -999, false);//don't erase other years when multiple years
						}
						
						lastID = ID;
					}

					if (stat.TRefIsChanging(TRef))
					{
						data[lastID][stat.GetTRef()].SetData(stat);
					}


					for (size_t v = 0; v < loop->size(); v++)
					{
						size_t cPos = COL_POS_H[v];
						if (cPos < NB_VAR_H)
						{
							double value = ToDouble((*loop)[v]);
							if (value > -99)
							{
								stat.Add(TRef, cPos, value);
								if (cPos == H_RELH)
								{
									double T = ToDouble((*loop)[TEMPERATURE]);
									double Hr = ToDouble((*loop)[RELATIVE_HUMIDITY]);
									if (T > -99 && Hr > 0)
										stat.Add(TRef, H_TDEW, Hr2Td(T, Hr));
								}
							}
						}
					}
				}//empty

				msg += callback.StepIt(loop->GetLastLine().length() + 2);
			}//for all line (


			if (stat.GetTRef().IsInit() && data.find(lastID) != data.end())
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

}


