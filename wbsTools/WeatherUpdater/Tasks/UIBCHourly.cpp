#include "stdafx.h"
#include "UIBCHourly.h"

#include "basic/WeatherStation.h"
#include "UI/Common/SYShowMessage.h"

#include "TaskFactory.h"
#include "../Resource.h"
#include "WeatherBasedSimulationString.h"

using namespace WBSF::HOURLY_DATA;
using namespace std;
using namespace UtilWWW;

namespace WBSF
{

	//*********************************************************************

	static const DWORD FLAGS = INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_PRAGMA_NOCACHE | INTERNET_FLAG_TRANSFER_BINARY;
	const char* CUIBCHourly::ATTRIBUTE_NAME[NB_ATTRIBUTES] = {"WorkingDir", "FirstYear", "LastYear", "Type", "UpdateStationList" };
	const size_t CUIBCHourly::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_STRING, T_STRING, T_COMBO_POSITION, T_BOOL };
	const UINT CUIBCHourly::ATTRIBUTE_TITLE_ID = IDS_UPDATER_BC_P;
	const UINT CUIBCHourly::DESCRIPTION_TITLE_ID = ID_TASK_BC;

	const char* CUIBCHourly::CLASS_NAME(){ static const char* THE_CLASS_NAME = "BCWeather";  return THE_CLASS_NAME; }
	CTaskBase::TType CUIBCHourly::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUIBCHourly::CLASS_NAME(), (createF)CUIBCHourly::create);


	const char* CUIBCHourly::SERVER_NAME = "tools.pacificclimate.org";

	const char* CUIBCHourly::NETWORK_NAME[NB_NETWORKS] = { "AGRI", "ARDA", "BCH", "EC", "EC_raw", "ENV-AQN", "ENV_ASP", "FLNRO-FERN", "FLNRO-WMB", "FRBC", "MoTIe", "MoTIm", "RTA" };
	const char* CUIBCHourly::TYPE_NAME[NB_TYPES] = { "1-hourly", "daily" /*"12-hourly", "irregular"*/ };
	const bool CUIBCHourly::AVAILABILITY[NB_NETWORKS][NB_TYPES] =
	{
		false, true,//AGR
		false, true,//ARDA
		true, true,	//BCH
		false, true,//EC
		true, true,	//EC_raw
		true, false,//ENV-AQN
		false, true,//ENV_ASP
		true, false,//FLNRO-FERN
		true, true, //FLNRO-WMB
		true, false,//FRBC
		true, false,//MoTIe
		false, true,//MoTIm 
		false, true,//RTA
	};


	CUIBCHourly::CUIBCHourly(void)
	{}

	CUIBCHourly::~CUIBCHourly(void)
	{}

	bool CUIBCHourly::IsHourly()const
	{ 
		return as<size_t>(TEMPORAL_TYPE)==T_HOURLY;
	}
	

	std::string CUIBCHourly::Option(size_t i)const
	{
		string str;
		switch (i)
		{
		case TEMPORAL_TYPE:	str = "Hourly|Daily"; break;
		};
		return str;
	}

	std::string CUIBCHourly::Default(size_t i)const
	{
		string str;

		switch (i)
		{
		case FIRST_YEAR:
		case LAST_YEAR:	str = ToString(CTRef::GetCurrentTRef().GetYear()); break;
		case TEMPORAL_TYPE: str = "1"; break;
		case UPDATE_STATION_LIST:	str = "1"; break;
		};
		return str;
	}
//**********************************************************************************************************************************
	ERMsg CUIBCHourly::Execute(CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);

		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(workingDir, 1);
		callback.AddMessage("");
		

		CTRef today = CTRef::GetCurrentTRef();

		//open a connection on the server
		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;

		msg = GetHttpConnection(SERVER_NAME, pConnection, pSession);
		if (!msg)
			return msg;

		if (as<bool>(UPDATE_STATION_LIST))
		{
			UpdateStationList(pConnection, callback);
		}

		callback.AddMessage(GetString(IDS_UPDATE_FILE));
		int nbDownload = 0;

		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = lastYear - firstYear + 1;

		callback.PushTask(FormatA("%04d"), nbYears);
		

		for (size_t y = 0; y < nbYears&&msg; y++)
		{
			int year = firstYear + int(y);

			size_t type = as<size_t>(TEMPORAL_TYPE); ASSERT(type < NB_TYPES);
			callback.PushTask(FormatA("%04d", year), type == T_HOURLY ? 7 : 9);
			for (size_t n = 0; n < NB_NETWORKS && msg; n++)
			{
				if (AVAILABILITY[n][type])
				{
				
					static const char* URL_FORMAT =
						"dataportal/data/pcds/agg/?"
						"from-date=%4d%%2F01%%2F01&"
						"to-date=%4d%%2F12%%2F31&"
						"input-polygon=&"
						"input-var=&"
						"network-name=%s&"
						"input-freq=%s&"
						"data-format=ascii&"
						"cliptodate=cliptodate&"
						"download-timeseries=Timeseries";

					//string URL = "dataportal/data/pcds/agg/?from-date=2015%2F01%2F01&to-date=2015%2F12%2F31&input-polygon=&input-var=&network-name=FLNRO-WMB&input-freq=1-hourly&data-format=ascii&cliptodate=cliptodate&download-timeseries=Timeseries";	
					//string URL = "dataportal/data/pcds/agg/?from-date=2016%2F01%2F01&to-date=2016%2F12%2F31&input-polygon=&input-var=&network-name=BCH&input-freq=daily&data-format=ascii&cliptodate=cliptodate&download-timeseries=Timeseries
					string URL = FormatA(URL_FORMAT, year, year, NETWORK_NAME[n], TYPE_NAME[type]);

					string ouputPath = workingDir + TYPE_NAME[type] + "\\" + ToString(year) + "\\";// +NETWORK_NAME[n] + "\\";
					CreateMultipleDir(ouputPath);

					
					string  filePathZip = ouputPath + "tmp.zip"; 

					
					callback.PushTask("Downloading " + string(NETWORK_NAME[n]) + " " + ToString(year) + "...", NOT_INIT);
					msg += UtilWWW::CopyFile(pConnection, URL, filePathZip, FLAGS, "", "", callback);
					callback.PopTask();
					if (msg)
					{
						if (FileExists(filePathZip) )
						{
							ifStream tmp;
							tmp.open(filePathZip);
							size_t length = tmp.length();
							tmp.close();
							if (length == 2)
							{
								RemoveFile(filePathZip);
							}
							else
							{
								msg = sevenZ(filePathZip, ouputPath, callback);

								if (msg)
								{
									RemoveFile(filePathZip);
									nbDownload++;
								}
							}
						}

						msg += callback.StepIt();
					}
					else
					{
					
						pConnection->Close();
						pSession->Close();

						//wait 5 seconds 
						callback.PushTask("Waiting 5 seconds...", 50);
						for (size_t s = 0; s < 50 && msg; s++)
						{
							Sleep(100);
							msg += callback.StepIt();
						}
						callback.PopTask();

						msg = GetHttpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS);
						
					}
					
					
				}//if available
			}//for all networks

			callback.PopTask();
		}//for all years


		callback.PopTask();
		callback.AddMessage(GetString(IDS_NB_FILES_DOWNLOADED) + ToString(nbDownload), 2);


		pConnection->Close();
		pSession->Close();


		return msg;
	}


	ERMsg CUIBCHourly::sevenZ(const string& filePathZip, const string& workingDir, CCallback& callback)
	{
		ERMsg msg;

		callback.PushTask(GetString(IDS_UNZIP_FILE), NOT_INIT);

		string command = GetApplicationPath() + "External\\7z.exe x \"" + filePathZip + "\" -y";

		DWORD exitCode = 0;
		msg = WinExecWait(command, workingDir, SW_HIDE, &exitCode);
		if (msg && exitCode != 0)
			msg.ajoute("7z.exe as exit with error code " + ToString(int(exitCode)));


		callback.PopTask();


		return msg;
	}

	std::string CUIBCHourly::GetStationListFilePath()const
	{
		return GetDir(WORKING_DIR) + "stations.csv";
	}



	ERMsg CUIBCHourly::UpdateStationList(CHttpConnectionPtr& pConnection, CCallback& callback)const
	{
		ERMsg msg;

		static const char* URL = "geoserver/CRMP/ows?service=WFS&version=1.1.0&request=GetFeature&typeName=CRMP%3Acrmp_network_geoserver&outputFormat=csv&srsname=epsg%3A4326&filter=%3Cogc%3AFilter+xmlns%3Aogc%3D%22http%3A%2F%2Fwww.opengis.net%2Fogc%22%3E%3Cogc%3AAnd%2F%3E%3C%2Fogc%3AFilter%3E";
		
		string localFilePath = GetStationListFilePath();
		CreateMultipleDir(GetPath(localFilePath));

		callback.PushTask("Downloading station list", NOT_INIT);
		msg = UtilWWW::CopyFile(pConnection, URL, localFilePath.c_str(), FLAGS, "", "", callback);
		callback.PopTask();


		return msg;

	}


	ERMsg CUIBCHourly::LoadStationList(CCallback& callback)
	{
		ERMsg msg;

		m_stations.clear();
//		native_id	station_name

		//ifStream file;
		//file.imbue(std::locale(std::locale::classic(), new std::codecvt_utf8<char>()));
		//msg = file.open(GetStationListFilePath());
		//if (msg)
		//{
		//	try
		//	{
		//		string str = file.GetText();
		//		std::replace(str.begin(), str.end(), '\'', 'Ã');

		//		zen::XmlDoc doc = zen::parse(str);

		//		zen::XmlIn in(doc.root());
		//		for (zen::XmlIn child = in["station"]; child; child.next())
		//		{
		//			CLocation tmp;
		//			child.attribute("fullname", tmp.m_name);
		//			std::replace(tmp.m_name.begin(), tmp.m_name.end(), 'Ã', '\'');
		//			child.attribute("name", tmp.m_ID);
		//			child.attribute("lat", tmp.m_lat);
		//			child.attribute("lon", tmp.m_lon);
		//			child.attribute("elev", tmp.m_elev);
		//			m_stations[tmp.m_ID] = tmp;
		//		}

		//	}
		//	catch (const zen::XmlFileError& e)
		//	{
		//		// handle error
		//		msg.ajoute(GetErrorDescription(e.lastError));
		//	}
		//	catch (const zen::XmlParsingError& e)
		//	{
		//		// handle error
		//		msg.ajoute("Error parsing XML file: col=" + ToString(e.col) + ", row=" + ToString(e.row));
		//	}
		//}

		return msg;
	}

	ERMsg CUIBCHourly::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;

		msg = LoadStationList(callback);
		if (!msg)
			return msg;


		set<string> fileList;

		string workingDir = GetDir(WORKING_DIR);
		CTRef today = CTRef::GetCurrentTRef();

		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = lastYear - firstYear + 1;
		size_t type = as<size_t>(TEMPORAL_TYPE);
	
		callback.PushTask(GetString(IDS_LOAD_STATION_LIST), nbYears);

		//find all station available that meet criterious
		for (size_t y = 0; y < nbYears&&msg; y++)
		{
			int year = firstYear + int(y);
			string command = workingDir + TYPE_NAME[type] + "\\" + ToString(year) + "\\*.ascii";
			
			//callback.PushTask(GetString(IDS_LOAD_STATION_LIST), NOT_INIT);
			StringVector fileListTmp = GetFilesList(command, 2, true);

			for (size_t i = 0; i < fileListTmp.size(); i++)
			{
				string fileTitle = GetFileTitle(fileListTmp[i]);
				MakeLower(fileTitle);
				fileList.insert(fileTitle);
			}

			msg += callback.StepIt();
		}

		stationList.insert(stationList.begin(), fileList.begin(), fileList.end());
		callback.PopTask();

		return msg;
	}

	ERMsg CUIBCHourly::GetWeatherStation(const string& stationID, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;

		CLocationMap::const_iterator it = m_stations.find(stationID);
		if (it != m_stations.end())
		{
			((CLocation&)station) = it->second;
		}
		else
		{
			msg.ajoute(FormatMsg(IDS_NO_STATION_INFORMATION, stationID));
			return msg;
		}

		string workingDir = GetDir(WORKING_DIR);
		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = lastYear - firstYear + 1;


		size_t nbFiles = 0;
		for (size_t y = 0; y < nbYears; y++)
		{
			int year = firstYear + int(y);
			
			for (size_t m = 0; m < 12 && msg; m++)
			{
				for (size_t d = 0; d < GetNbDayPerMonth(year, m) && msg; d++)
				{
					string filePath = workingDir + FormatA("%4d\\%02d\\%02d\\%4d%02d%02d%s.mts", year, m + 1, d + 1, year, m + 1, d + 1, stationID.c_str());
					if (FileExists(filePath))
						nbFiles++;
				}
			}
		}

		//now extract data 
		callback.PushTask(station.m_name, nbFiles);
		for (size_t y = 0; y < nbYears&&msg; y++)
		{
			int year = firstYear + int(y);

			for (size_t m = 0; m < 12 && msg; m++)
			{
				for (size_t d = 0; d < GetNbDayPerMonth(year, m) && msg; d++)
				{
					string filePath = workingDir + FormatA("%4d\\%02d\\%02d\\%4d%02d%02d%s.mts", year, m + 1, d + 1, year, m + 1, d + 1, stationID.c_str());
					if (FileExists(filePath))
					{
						msg = ReadData(filePath, TM, station[year], callback);
						if (msg)
							msg += callback.StepIt();
					}
				}
			}
		}

		if (msg)
		{
			//verify station is valid
			if (station.HaveData())
			{
				msg = station.IsValid();
			}
		}

		callback.PopTask();

		return msg;
	}

	
	ERMsg CUIBCHourly::ReadData(const string& filePath, CTM TM, CYear& data, CCallback& callback)const
	{
		ERMsg msg;

//		CTRef startDate = FromFormatedString(Get(START_DATE));
	//	CTRef endDate = FromFormatedString(Get(END_DATE));
		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);

//		size_t nbYears = startDate.GetYear() - endDate.GetYear() + 1;

	//	int nbYear = m_lastYear - m_firstYear + 1;
		//enum { C_STID, C_STNM, C_MINUTES, C_LAT, C_LON, C_ELEV, C_TAIR, C_RELH, C_TDEW, C_WDIR, C_WSPD, C_WMAX, C_TAIR3HR, C_TDEW3HR, C_RAIN1HR, C_RAIN3HR, C_RAIN, C_PRES, C_PMSL, C_PMSL3HR, C_SNOW, C_SRAD, C_TAIRMIN, C_TAIRMAX, C_PT020H, C_PT040H, C_PT050H, NB_INPUT_HOURLY_COLUMN };
		//const int COL_POS[NB_VAR_H] = { C_TAIR, -1, C_RAIN1HR, C_TDEW, C_RELH, C_WSPD, C_WDIR, C_SRAD, /*C_PRES*/C_PMSL, -1, C_SNOW, -1, -1, -1, -1, -1, -1, -1 };

		//now extact data 
		ifStream file;

		msg = file.open(filePath);

		if (msg)
		{
			CWeatherAccumulator stat(TM);

			string title;
			getline(file, title);
			string headerStr;
			getline(file, headerStr);
			StringVector header = Tokenize(headerStr, ",", true);
			//wind_speed, temperature, wind_direction, relative_humidity, time, precipitation
			//time, ONE_DAY_PRECIPITATION, MAX_TEMP, MIN_TEMP
			//ONE_DAY_PRECIPITATION, ONE_DAY_RAIN, ONE_DAY_SNOW, time, MIN_TEMP, MAX_TEMP, SNOW_ON_THE_GROUND


			string dateTimeStr;
			StringVector dateTime = Tokenize(dateTimeStr, " ", true);

			int year = stoi(dateTime[1]);
			int month = stoi(dateTime[2]);
			int day = stoi(dateTime[3]);
			int hour = stoi(dateTime[4]);

			_tzset();
			tm _tm = { 0, 0, hour, day, month - 1, year - 1900, -1, -1, -1 };
			__time64_t ltime = _mkgmtime64(&_tm);

			_localtime64_s(&_tm, &ltime);
			year = _tm.tm_year + 1900;
			month = _tm.tm_mon + 1 - 1;
			day = _tm.tm_mday - 1;
			hour = _tm.tm_hour;

			//que faire pour le NB
			ASSERT(year >= firstYear - 1 && year <= lastYear);
			ASSERT(month >= 0 && month < 12);
			ASSERT(day >= 0 && day < GetNbDayPerMonth(year, month));
			ASSERT(hour >= 0 && hour < 24);


			size_t i = 0;

			string line;
			while (getline(file, line) && msg)
			{
				Trim(line);
				ASSERT(!line.empty());

				StringVector vars = Tokenize(line, ",");
				//ASSERT(vars.size() >= NB_INPUT_HOURLY_COLUMN);

				//int deltaHour = stoi(vars[C_MINUTES]) / 60;

				//CTRef TRef = CTRef(year, month, day, hour) + deltaHour;

				//if (stat.TRefIsChanging(TRef) && data.GetEntireTPeriod(CTM(CTM::HOURLY)).IsInside(stat.GetTRef()))
				//	data[stat.GetTRef()].SetData(stat);

				//bool bValid[NB_VAR_H] = { 0 };

				//for (size_t v = 0; v<NB_VAR_H; v++)
				//{
				//	if (COL_POS[v] >= 0)
				//	{
				//		double value = stod(vars[COL_POS[v]]);

				//		//if (value>-996)
				//			//stat.Add(TRef, v, Convert(v, value));
				//	}
				//}

				msg += callback.StepIt(0);
			}//for all line


			if (stat.GetTRef().IsInit() && data.GetEntireTPeriod(CTM(CTM::HOURLY)).IsInside(stat.GetTRef()))
				data[stat.GetTRef()].SetData(stat);

		}//if load 

		return msg;
	}


}