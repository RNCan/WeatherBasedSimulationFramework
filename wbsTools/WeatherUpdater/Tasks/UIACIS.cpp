#include "StdAfx.h"
#include "UIACIS.h"

#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/copy.hpp>

#include "Basic/HourlyDatabase.h"
#include "Basic/CSV.h"
#include "Basic/FileStamp.h"
#include "UI/Common/SYShowMessage.h"
#include "TaskFactory.h"

#include "../Resource.h"
#include "WeatherBasedSimulationString.h"

//min et max
//http://wildfire.alberta.ca/wildfire-status/fire-weather/forecasts-observations/default.aspx
//Average Air Temperature							    AT		[°C]
//Instantaneous Air Temperature							ATA		[°C]
//Maximum Air Temperature								TX		[°C]
//Minimum Air Temperature								TN		[°C]
//Average Relative Humidity								HUA		[%]
//Instantaneous Relative Humidity						HU		[%]
//Accumulated Precipitation in Gauge					PR		[kg/m²]
//Incoming Solar Radiation								IR		[W/m2]
//Average Wind Direction at 10 meter height				WDA		[°]
//Instantaneous Wind Direction at 10 meter height		WD		[°]
//Average Wind Direction at 2 meter height				UA		[°]
//Instantaneous Wind Speed at 2 meter height			US		[km/h]
//Average Wind Speed at 10 meter height					WSA		[km/h]
//Instantaneous Wind Speed at 10 meter height			WS		[km/h]
//Accumulated Precipitation in Gauge        			P1		[mm]
//Precipitation											PC		[mm]

//{"AT", "ATA", "TX", "TN", "HUA", "HU", "PR", "IR", "WDA", "WD", "UA", "US", "WSA", "WS", "P1", "PC"};


//stations list
//


//meta data of station
//https://agriculture.alberta.ca/acis/station/metadata?resource=complete&station=4508


using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace UtilWWW;

namespace WBSF
{




	//*********************************************************************
	static const DWORD FLAGS = INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_PRAGMA_NOCACHE;

	//*********************************************************************
	//"UserName", "Password", 
	const char* CUIACIS::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "DataType", "FirstYear", "LastYear", "UpdateStationsList", "IgnoreEnvCan", "MonthLag" };
	const size_t CUIACIS::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_COMBO_INDEX, T_STRING, T_STRING, T_BOOL, T_BOOL, T_BOOL };
	//T_STRING, T_PASSWORD, 
	const UINT CUIACIS::ATTRIBUTE_TITLE_ID = IDS_UPDATER_ALBERTA_P;
	const UINT CUIACIS::DESCRIPTION_TITLE_ID = ID_TASK_ALBERTA;

	const char* CUIACIS::CLASS_NAME() { static const char* THE_CLASS_NAME = "ACISHourly";  return THE_CLASS_NAME; }
	CTaskBase::TType CUIACIS::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUIACIS::CLASS_NAME(), (createF)CUIACIS::create);

	const char* CUIACIS::SERVER_NAME = "agriculture.alberta.ca";

	//"Alberta Agriculture and Forestry"	"AGCM"
	//"Alberta Sustainable Resource Development"	"ASRD"
	//"Alberta Agriculture and Forestry"	"AGDM"
	//"Environment and Parks"	"AENV"
	//"Nav Canada"	"NAV"
	//"Environment Canada"	"MSC"
	//"Environment Canada"	"PARKS"
	//"Alberta Agriculture and Forestry"	"IMCIN"
	//"Environment Canada"	"MSCRCS"
	//"Environment Canada"	"AAFC"
	//"Nav Canada"	"MSCRCS"
	//"Environment Canada"	"SYNC"
	//"Agriculture and Agri-Food Canada"	"MSCRCS"
	//"Alberta Agriculture and Forestry"	"RESEARCH"
	//"Hatfield Consultants"	
	//"Environment and Parks"	"AGDM"
	//"Alberta Agriculture and Forestry"	"AFTS"
	//"Agriculture and Agri-Food Canada"	"AAFC"

		//http://agriculture.alberta.ca/acis/rss/data?type=HOURLY&stationId=39271045&date=20180320

	CUIACIS::CUIACIS(void)
	{}

	CUIACIS::~CUIACIS(void)
	{}

	std::string CUIACIS::Option(size_t i)const
	{
		string str;
		switch (i)
		{
		case DATA_TYPE:	str = GetString(IDS_STR_DATA_TYPE); break;
		};
		return str;
	}



	std::string CUIACIS::Default(size_t i)const
	{
		string str;

		switch (i)
		{
		case WORKING_DIR: str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()) + "Alberta\\"; break;
		case DATA_TYPE: str = "1"; break;
		case FIRST_YEAR:
		case LAST_YEAR:	str = ToString(CTRef::GetCurrentTRef().GetYear()); break;
		case UPDATE_STATIONS_LIST: str = "0"; break;
		case IGNORE_ENV_CAN: str = "1"; break;
		case MONTH_LAG: str = "0"; break;
		};

		return str;
	}

	//Interface attribute index to attribute index
	static const char PageDataFormat[] = "acis/rss/data?type=%s&stationId=%s&date=%4d%02d%02d";

	static const bool OLD_CODE = true;


	ERMsg CUIACIS::Execute(CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);

		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(workingDir, 1);
		callback.AddMessage(GetString(IDS_UPDATE_FROM));
		callback.AddMessage(SERVER_NAME, 1);
		callback.AddMessage("");


		msg = VerifyUserPass(callback);//to protect user if they have not a good password to be locked
		if (!msg)
			return msg;


		bool bForeUpdate = as<bool>(UPDATE_STATIONS_LIST);
		if (FileExists(GetStationListFilePath()) && !bForeUpdate)
		{
			msg = m_stations.Load(GetStationListFilePath());
		}
		else
		{
			CreateMultipleDir(GetPath(GetStationListFilePath()));
			if (OLD_CODE)
			{
				msg = DownloadStationListII(m_stations, callback);
			}
			else
			{
				msg = DownloadStationList(m_stations, callback);
			}


			if (msg)
				msg = m_stations.Save(GetStationListFilePath());
		}

		if (!msg)
			return msg;


		if (OLD_CODE)
		{
			size_t type = as <size_t>(DATA_TYPE);
			if (type == HOURLY_WEATHER)
				msg = DownloadStationHourly(callback);
			else
				msg = DownloadStationDaily(callback);
		}
		else
		{
			msg = DownloadStation(callback);
		}


		return msg;
	}


	ERMsg CUIACIS::DownloadStationListII(CLocationVector& stationList, CCallback& callback)const
	{
		ERMsg msg;

		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;

		msg = GetHttpConnection(SERVER_NAME, pConnection, pSession);
		if (!msg)
			return msg;



		//Get cookie and page list
		//agriculture.alberta.ca/acis/alberta-weather-data-viewer.jsp
		string source;
		msg = GetPageText(pConnection, "acis/alberta-weather-data-viewer.jsp", source);
		if (msg)
		{

			vector<string> listID;
			string::size_type posBegin = source.find("Select Station(s):");
			//int posEnd = posBegin;
			while (msg&&posBegin != string::npos)
			{
				string ID = FindString(source, "<option value=", ">", posBegin);
				//string Name = FindString(source, "</option>", posBegin);

				if (ID != "-1")
				{
					listID.push_back(ID);

					posBegin = source.find("<option value=", posBegin);
					msg += callback.StepIt(0);
				}
			}

			callback.PushTask(GetString(IDS_LOAD_STATION_LIST), listID.size());

			for (size_t i = 0; i < listID.size() && msg; i++)
			{
				string metadata;
				msg = GetPageText(pConnection, ("acis/station/metadata?resource=complete&station=" + listID[i]).c_str(), metadata);
				string::size_type metaPosBegin = metadata.find("<name>");
				if (msg && metaPosBegin != string::npos)
				{
					//CLocation stationInfo;
					CLocation stationInfo;
					//last word is the network
					//StringVector tmp(TrimConst(FindString(metadata, "<name>", "</name>", metaPosBegin)), " ");
					/*ASSERT(tmp.size() >= 2);

					stationInfo.m_name = tmp[0];
					for(size_t j=1; j<tmp.size()-1; j++)
						stationInfo.m_name += " " + tmp[i];*/
					stationInfo.m_name = TrimConst(FindString(metadata, "<name>", "</name>", metaPosBegin));
					stationInfo.m_ID = TrimConst(listID[i]);
					stationInfo.m_lat = ToDouble(FindString(metadata, "<latitude>", "</latitude>", metaPosBegin));
					stationInfo.m_lon = ToDouble(FindString(metadata, "<longitude>", "</longitude>", metaPosBegin));
					stationInfo.m_alt = ToDouble(FindString(metadata, "<elevation>", "</elevation>", metaPosBegin));

					//<aliases><WMO>71285</WMO><TC>XAF</TC><EC>3010010</EC><AENV>ABEE</AENV><LOGGER_ID>1285</LOGGER_ID></aliases>
					string owner = FindString(metadata, "<owner><name>", "</name></owner>", metaPosBegin);
					if (!owner.empty())
						stationInfo.SetSSI("owner", owner);

					string oper = FindString(metadata, "<operator><name>", "</name></operator>", metaPosBegin);
					if (!oper.empty())
						stationInfo.SetSSI("operator", oper);


					StringVector aliases(FindString(metadata, "<aliases>", "</aliases>", metaPosBegin), "<>");
					ASSERT(aliases.size() % 3 == 0);
					for (size_t j = 0; j < aliases.size(); j += 3)
					{
						if (!aliases[j + 1].empty())
							stationInfo.SetSSI(aliases[j], aliases[j + 1]);
					}

					/*string WMO = FindString(aliases, "<WMO>", "</WMO>");
					if (!WMO.empty())
						stationInfo.SetSSI("WMO", WMO);
					string ECID = FindString(aliases, "<EC>", "</EC>");
					if (!ECID.empty())
						stationInfo.SetSSI("EC_id", ECID);
					string AENV = FindString(aliases, "<AENV>", "</AENV>");
					if (!AENV.empty())
						stationInfo.SetSSI("AENV", AENV);

					*/


					stationList.push_back(stationInfo);
					msg += callback.StepIt();
				}
			}
		}

		pConnection->Close();
		pSession->Close();



		//clear all non ACIS stations
		for (CLocationVector::iterator it = stationList.begin(); it != stationList.end();)
		{
			if (it->GetSSI("ClimateID").substr(0, 3) == "999")
				it++;
			else
				it = stationList.erase(it);
		}


		callback.AddMessage(GetString(IDS_NB_STATIONS) + ToString(stationList.size()));
		callback.PopTask();

		return msg;
	}


	string CUIACIS::GetSessiosnID(CHttpConnectionPtr& pConnection)
	{
		string sessionID;
		string page;

		if (GetPageText(pConnection, "acis/alberta-weather-data-viewer.jsp", page))
		{

			string::size_type posBegin = page.find("getSessionId()");
			if (posBegin >= 0)
				sessionID = FindString(page, "return \"", "\";", posBegin);
		}

		return sessionID;
	}

	//CTRef CUIACIS::GetTRefFromTime64(__time64_t t, CTM TM = CTM(CTM::DAILY))
	//{
	//	CTRef TRef;

	//	if (t > 0)
	//	{
	//		struct tm *theTime = _localtime64(&t);
	//		TRef = CTRef(1900 + theTime->tm_year, theTime->tm_mon, theTime->tm_mday - 1, theTime->tm_hour, TM);
	//	}

	//	return TRef;
	//}

	bool CUIACIS::IsInclude(const CLocation& station)
	{
		string owner = station.GetSSI("owner");
		string ID = station.GetSSI("EC");
		ID = ID.substr(0, 3);

		//		static const char* ENV_CAN_MISSING[] = { "3076069", "3055754", "3031875", "3053760" };

		bool bASRD = IsEqual(owner, "Alberta Sustainable Resource Development");
		bool bParck = IsEqual(owner, "Environment and Parks");
		//	bool bException = find(begin(ENV_CAN_MISSING), end(ENV_CAN_MISSING), station.m_ID)!= end(ENV_CAN_MISSING);
		bool bInclude = ID.empty() || ID == "999" || bASRD || bParck;
		return bInclude;
	}

	bool CUIACIS::NeedUpdate(const CLocation& station, int year, size_t m)
	{
		string filepath = GetOutputFilePath(year, m, station.m_ID);


		CTRef TRef1 = CWeatherYears::GetLastTref(filepath).as(CTM::DAILY);
		CTRef TRef2(year, m, LAST_DAY);
		CTimeRef last_update(GetFileStamp(filepath));
		//CTRef today = CTRef::GetCurrentTRef();
		bool bNotCompleted = TRef1.IsInit() && TRef1 - TRef2 < 0;
		bool bNot5days = !last_update.IsInit() || last_update - TRef2 < 5;

		bool bNeedUpdate = bNotCompleted || bNot5days; //let 5 days to update the data if it's not the current month
		return bNeedUpdate;
	}

	ERMsg CUIACIS::DownloadStationHourly(CCallback& callback)
	{
		ERMsg msg;

		static const char PageDataFormatH[] =
			"acis/api/v1/legacy/weather-data/timeseries?"
			"stations=%s&"
			"elements=PRCIP,ATAM,ATX,ATN,HUAM,IR,WSAM,WDAM,UA,SD,DEW&"
			"startdate=%4d%02d%02d&"
			"enddate=%4d%02d%02d&"
			"interval=HOURLY&"
			"format=csv&"
			"precipunit=mm&"
			"inclCompleteness=false&"
			"inclSource=false&"
			"inclComments=false&"
			"session=%s";


		CTRef today = CTRef::GetCurrentTRef();
		string workingDir = GetDir(WORKING_DIR);
		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = lastYear - firstYear + 1;

		bool bIgoneEC = as<bool>(IGNORE_ENV_CAN);
		size_t type = as <size_t>(DATA_TYPE);

		callback.PushTask("Clear stations list...", m_stations.size()*nbYears * 12);


		size_t nbFilesToDownload = 0;
		vector<vector<array<bool, 12>>> bNeedDownload(m_stations.size());
		for (size_t i = 0; i < m_stations.size() && msg; i++)
		{
			bNeedDownload[i].resize(nbYears);
			for (size_t y = 0; y < nbYears&&msg; y++)
			{
				int year = firstYear + int(y);

				size_t nbm = year == today.GetYear() ? today.GetMonth() + 1 : 12;
				for (size_t m = 0; m < nbm && msg; m++)
				{
					bool bNeedUpdate = NeedUpdate(m_stations[i], year, m);
					if (bNeedUpdate && bIgoneEC)
						bNeedUpdate = IsInclude(m_stations[i]);

					bNeedDownload[i][y][m] = bNeedUpdate;
					nbFilesToDownload += bNeedUpdate ? 1 : 0;
					msg += callback.StepIt();
				}
			}
		}

		callback.PopTask();

		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;
		msg += GetHttpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", false, 1, callback);
		if (!msg)
			return msg;

		//une protection empêche de charger plus de fichier
		size_t nbDownload = 0;
		size_t currentNbDownload = 0;
		size_t nbTry = 0;

		callback.PushTask("Download ACIS hourly data: " + to_string(nbFilesToDownload) + " station-month", nbFilesToDownload);
		callback.AddMessage("Download ACIS hourly data: " + to_string(nbFilesToDownload) + " station-month");

		for (size_t i = 0; i < m_stations.size() && msg; i++)
		{

			for (size_t y = 0; y < nbYears&&msg; y++)
			{
				int year = firstYear + int(y);
				for (size_t m = 0; m < 12 && msg; m++)
				{
					if (bNeedDownload[i][y][m])
					{

						string filePath = GetOutputFilePath(year, m, m_stations[i].m_ID);
						CreateMultipleDir(GetPath(filePath));

						string sessionID = GetSessiosnID(pConnection);
						string URL = FormatA(PageDataFormatH, m_stations[i].m_ID.c_str(), year, m + 1, 1, year, m + 1, GetNbDayPerMonth(year, m), sessionID.c_str());

						try
						{
							string source;
							msg = GetPageText(pConnection, URL, source, false, FLAGS | INTERNET_FLAG_FORMS_SUBMIT);
							if (msg)
							{
								if (source.find("An Error Has Occurred") == string::npos &&
									source.find("Page Not Found") == string::npos)
								{
									if (source.find("Too Many Requests") == string::npos)
									{
										msg += SaveData(type, filePath, source, callback);
										if (msg)
										{
											nbDownload++;
											currentNbDownload++;
											nbTry = 0;
										}

										//need to wait to not bust the maximum download by minute
										size_t pause_time = 10;
										for (size_t s = 0; s < pause_time && msg; s++)
										{
											Sleep(100);
											msg += callback.StepIt(0);
										}
									}
									else
									{


										nbTry++;
										if (nbTry < 5)
										{
											callback.AddMessage("Too Many Requests");
											//wait 120 seconds 
											msg += WaitServer(120, callback);
										}
										else
										{
											//msg = UtilWin::SYGetMessage(*e);
											msg.ajoute("Too Many Requests");
										}

									}
								}
								else
								{
									msg.ajoute(URL);
								}
							}

							if (msg)
								msg += callback.StepIt();
						}
						catch (CException* e)
						{
							//if an error occur: try again
							msg = UtilWin::SYGetMessage(*e);
						}
					}
				}
			}
		}


		//clean connection
		//pConnection->OpenRequest(CHttpConnection::HTTP_VERB_UNLINK, _T("acis/api/v1/legacy/weather-data/timeseries?"));
		pConnection->Close();
		pConnection.release();
		pSession->Close();
		pSession.release();


		callback.AddMessage(GetString(IDS_NB_FILES_DOWNLOADED) + ToString(nbDownload), 2);
		callback.PopTask();

		return msg;
	}

	ERMsg CUIACIS::DownloadStationDaily(CCallback& callback)
	{
		ERMsg msg;

		static const char PageDataFormatD[] =
			"acis/api/v1/legacy/weather-data/timeseries?"
			"stations=%s&"
			"elements=PRCIP,ATAM,ATX,ATN,HUAM,IR,WSAM,WDAM,UA,SD,DEW&"
			"startdate=%4d%02d%02d&"
			"enddate=%4d%02d%02d&"
			"interval=DAILY&"
			"format=csv&"
			"precipunit=mm&"
			"inclCompleteness=false&"
			"inclSource=false&"
			"inclComments=false&"
			"session=%s";

		CTRef today = CTRef::GetCurrentTRef();
		string workingDir = GetDir(WORKING_DIR);
		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = lastYear - firstYear + 1;

		bool bIgoneEC = as<bool>(IGNORE_ENV_CAN);
		size_t type = as <size_t>(DATA_TYPE);

		callback.PushTask("Clear stations list...", m_stations.size()*nbYears);


		size_t nbFilesToDownload = 0;
		vector<vector<bool>> bNeedDownload(m_stations.size());
		for (size_t i = 0; i < m_stations.size() && msg; i++)
		{
			bNeedDownload[i].resize(nbYears);
			for (size_t y = 0; y < nbYears&&msg; y++)
			{
				int year = firstYear + int(y);

				string filePath = GetOutputFilePath(year, NOT_INIT, m_stations[i].m_ID);
				bool bNeedUpdate = NeedUpdate(m_stations[i], year);
				if (bNeedUpdate && bIgoneEC)
					bNeedUpdate = IsInclude(m_stations[i]);



				bNeedDownload[i][y] = bNeedUpdate;
				nbFilesToDownload += bNeedUpdate ? 1 : 0;
				msg += callback.StepIt();

			}
		}

		callback.PopTask();

		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;
		msg += GetHttpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", false, 1, callback);
		if (!msg)
			return msg;

		//une protection empêche de charger plus de fichier
		size_t nbDownload = 0;
		size_t currentNbDownload = 0;
		size_t nbTry = 0;

		callback.PushTask("Download ACIS daily data: " + to_string(nbFilesToDownload) + " station-year", nbFilesToDownload);
		callback.AddMessage("Download ACIS daily data: " + to_string(nbFilesToDownload) + " station-year");

		for (size_t i = 0; i < m_stations.size() && msg; i++)
		{
			for (size_t y = 0; y < nbYears&&msg; y++)
			{
				int year = firstYear + int(y);
				if (bNeedDownload[i][y])
				{
					try
					{
						string filePath = GetOutputFilePath(year, NOT_INIT, m_stations[i].m_ID);
						CreateMultipleDir(GetPath(filePath));

						string sessionID = GetSessiosnID(pConnection);
						string URL = FormatA(PageDataFormatD, m_stations[i].m_ID.c_str(), year, 1, 1, year, 12, 31, sessionID.c_str());

						string source;
						msg = GetPageText(pConnection, URL, source, false, FLAGS | INTERNET_FLAG_FORMS_SUBMIT);

						if (msg)
						{
							if (source.find("An Error Has Occurred") == string::npos &&
								source.find("Page Not Found") == string::npos)
							{
								if (source.find("Too Many Requests") == string::npos)
								{
									msg += SaveData(type, filePath, source, callback);
									if (msg)
									{
										nbDownload++;
										currentNbDownload++;
										nbTry = 0;
									}

									//need to wait to not bust the maximum download by minute
									size_t pause_time = 10;
									for (size_t s = 0; s < pause_time && msg; s++)
									{
										Sleep(100);
										msg += callback.StepIt(0);
									}
								}
								else
								{
									nbTry++;
									if (nbTry < 5)
									{
										callback.AddMessage("Too Many Requests");
										//wait 120 seconds 
										msg += WaitServer(120, callback);
									}
									else
									{
										//msg = UtilWin::SYGetMessage(*e);
										msg.ajoute("Too Many Requests");
									}

								}
							}
							else
							{
								msg.ajoute(URL);
							}
						}

						if (msg)
							msg += callback.StepIt();
					}
					catch (CException* e)
					{
						//if an error occur: try again
						msg = UtilWin::SYGetMessage(*e);
					}
				}
			}
		}


		//clean connection
		//pConnection->OpenRequest(CHttpConnection::HTTP_VERB_UNLINK, _T("acis/api/v1/legacy/weather-data/timeseries?"));
		pConnection->Close();
		pConnection.release();
		pSession->Close();
		pSession.release();


		callback.AddMessage(GetString(IDS_NB_FILES_DOWNLOADED) + ToString(nbDownload), 2);
		callback.PopTask();


		return msg;
	}


	//******************************************************************************************************************************
	//******************************************************************************************************************************
	//******************************************************************************************************************************
	//******************************************************************************************************************************
	//rss part



	ERMsg CUIACIS::DownloadStation(CCallback& callback)
	{
		ERMsg msg;

		CTRef today = CTRef::GetCurrentTRef();
		string workingDir = GetDir(WORKING_DIR);
		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = lastYear - firstYear + 1;
		bool bIgoneEC = as<bool>(IGNORE_ENV_CAN);
		size_t type = as <size_t>(DATA_TYPE);

		size_t nbFilesToDownload = 0;
		vector<vector<array<bool, 12>>> bNeedDownload(m_stations.size());

		callback.PushTask("Clear stations list...", m_stations.size()*nbYears * 12);
		for (size_t i = 0; i < m_stations.size() && msg; i++)
		{
			bNeedDownload[i].resize(nbYears);
			for (size_t y = 0; y < nbYears&&msg; y++)
			{
				int year = firstYear + int(y);

				size_t nbm = year == today.GetYear() ? today.GetMonth() + 1 : 12;
				for (size_t m = 0; m < nbm && msg; m++)
				{
					string filePath = GetOutputFilePath(year, m, m_stations[i].m_ID);
					CTimeRef TRef1(GetFileStamp(filePath));
					CTRef TRef2(year, m, LAST_DAY);

					bool bND = !TRef1.IsInit() || TRef1 - TRef2 < 2; //let 2 days to update the data if it's not the current month
					if (bND && bIgoneEC)
					{
						//string network = m_stations[i].GetSSI("type");
						string ID = m_stations[i].GetSSI("EC_id");
						ID = ID.substr(0, 3);
						if (!ID.empty() && ID != "999"/* && network != "AENV"  && network != "PARC"*/)
							bND = false;
					}

					bNeedDownload[i][y][m] = bND;
					nbFilesToDownload += bND ? 1 : 0;

					msg += callback.StepIt();
				}
			}
		}

		callback.PopTask();

		size_t nbTry = 0;
		size_t curI = 0;
		size_t nbDownload = 0;


		callback.PushTask(string("Download ACIS ") + (type == HOURLY_WEATHER ? "hourly" : "daily") + " data (" + ToString(nbFilesToDownload) + " files)", nbFilesToDownload);
		callback.AddMessage("Nb stations months to downloaded: " + ToString(nbFilesToDownload));

		while (curI < m_stations.size() && msg)
		{
			nbTry++;

			CInternetSessionPtr pSession;
			CHttpConnectionPtr pConnection;
			msg = GetHttpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", false, 1, callback);

			if (msg)
			{
				try
				{
					pSession->SetOption(INTERNET_OPTION_RECEIVE_TIMEOUT, 15000);

					while (curI < m_stations.size() && msg)
					{
						for (size_t y = 0; y < nbYears && msg; y++)
						{
							int year = firstYear + int(y);
							for (size_t m = 0; m < 12 && msg; m++)
							{
								if (bNeedDownload[curI][y][m])
								{
									string filePath = GetOutputFilePath(year, m, m_stations[curI].m_ID);
									CreateMultipleDir(GetPath(filePath));

									msg += DownloadMonth(pConnection, year, m, m_stations[curI].m_ID, filePath, callback);
									if (msg)
									{
										nbDownload++;
										nbTry = 0;
										msg += callback.StepIt();
									}
								}//if need download
							}//for all months
						}//for all years

						curI++;
					}//for all station
				}
				catch (CException* e)
				{
					//if an error occur: try again
					if (nbTry < 5)
					{
						callback.AddMessage(UtilWin::SYGetMessage(*e));

						//msg = WaitServer(10, callback);
						//wait 5 seconds 
						callback.PushTask("Wait 5 seconds...", 50);
						for (size_t s = 0; s < 50 && msg; s++)
						{
							Sleep(100);
							msg += callback.StepIt();
						}
						callback.PopTask();
					}
					else
					{
						msg = UtilWin::SYGetMessage(*e);
					}
				}

				//clean connection
				pConnection->Close();
				pSession->Close();
			}//if msg
		}//while

		callback.AddMessage("Nb stations months downloaded: " + ToString(nbDownload));
		callback.PopTask();

		return msg;
	}

	ERMsg CUIACIS::DownloadStationList(CLocationVector& stationList, CCallback& callback)const
	{
		ERMsg msg;

		stationList.clear();

		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;

		msg = GetHttpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", false, 1, callback);
		if (!msg)
			return msg;


		//Get cookie and page list

		string source;
		msg = GetPageText(pConnection, "acis/rss/data?type=station&stationId=all", source);
		if (msg)
		{
			try
			{
				//replace all ' by space
				WBSF::ReplaceString(source, "'", " ");
				zen::XmlDoc doc = zen::parse(source);

				zen::XmlIn in(doc.root());
				for (zen::XmlIn child = in["station"]; child&&msg; child.next())
				{
					CLocation location;
					enum TAttributes { ACIS_STATION_ID, COMMENT, ELEVATION, LATITUDE, LONGITUDE, OPERATOR, OWNER, POSTAL_CODE, STATION_NAME, TYPE, NB_ATTRIBUTES };
					static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "acis_station_id", "comment", "elevation", "latitude", "longitude", "operator", "owner", "postal_code", "station_name", "type" };
					for (size_t i = 0; i < NB_ATTRIBUTES; i++)
					{
						string str;
						if (child.attribute(ATTRIBUTE_NAME[i], str))
						{
							std::replace(str.begin(), str.end(), ',', ' ');
							std::replace(str.begin(), str.end(), ';', ' ');

							switch (i)
							{
							case ACIS_STATION_ID:	location.m_ID = str; break;
							case STATION_NAME:		location.m_name = str; break;
							case LATITUDE:			location.m_lat = ToDouble(str); break;
							case LONGITUDE:			location.m_lon = ToDouble(str); break;
							case ELEVATION:			location.m_alt = ToDouble(str); break;
							default:				location.SetSSI(ATTRIBUTE_NAME[i], str);
							}
						}
					}//for all attributes

					const zen::XmlElement* pIds = child["station_ids"].get();
					if (pIds)
					{
						auto iterPair = pIds->getChildren();
						for (auto iter = iterPair.first; iter != iterPair.second; ++iter)
						{
							string name = iter->getNameAs<string>();
							string value;
							iter->getValue(value);
							location.SetSSI(name, value);
						}//for all stations ID
					}

					stationList.push_back(location);
				}//for all stations
			}
			catch (const zen::XmlParsingError& e)
			{
				// handle error
				msg.ajoute("Error parsing XML file: col=" + ToString(e.col) + ", row=" + ToString(e.row));
			}

		}//if msg

		pConnection->Close();
		pSession->Close();

		callback.AddMessage(GetString(IDS_NB_STATIONS) + ToString(stationList.size()));


		return msg;
	}

	ERMsg CUIACIS::VerifyUserPass(CCallback& callback)
	{
		ERMsg msg;

		if (OLD_CODE)
		{
			//if(Get(USER_NAME) != "12345" || Get(PASSWORD) != "12345")
				//msg.ajoute("Invalid user name/password");
		}
		else
		{
			CInternetSessionPtr pSession;
			CHttpConnectionPtr pConnection;
			msg = GetHttpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", false, 1, callback);
			if (msg)
			{
				size_t type = as <size_t>(DATA_TYPE);
				string pageURL = FormatA(PageDataFormat, type == HOURLY_WEATHER ? "HOURLY" : "DAILY", "2154", 2016, 1, 1);

				string source;
				msg = GetPageText(pConnection, pageURL, source, false, FLAGS);

				if (source.empty() || source.find("No Records Were Found") != string::npos || source.find("ACIS Error") != string::npos)
				{
					msg.ajoute("Invalid user name/password or server problem");
				}


				//clean connection
				pConnection->Close();
				pSession->Close();
			}
		}

		return msg;

	}

	size_t GetHour(const string& time)
	{
		size_t h = 0;

		StringVector v(time, ", :");
		ASSERT(v.size() == 8);
		if (v.size() == 8)
			h = WBSF::as<size_t>(v[4]);

		return h;
	}

	CTRef GetTRef(string str_date, CTM TM)
	{
		CTRef TRef;

		StringVector tmp(str_date, ", :");
		ASSERT(tmp.size() == 8);
		size_t d = ToSizeT(tmp[1]) - 1;
		size_t m = WBSF::GetMonthIndex(tmp[2].c_str());
		int year = ToInt(tmp[3]);
		size_t h = ToSizeT(tmp[4]);

		return CTRef(year, m, d, h, TM);
	}

	ERMsg CUIACIS::DownloadMonth(CHttpConnectionPtr& pConnection, int year, size_t m, const string& ID, const string& filePath, CCallback& callback)
	{
		ERMsg msg;

		size_t type = as <size_t>(DATA_TYPE);
		CTRef TRef = CTRef::GetCurrentTRef();

		//string output_text;
		std::stringstream stream;
		if (type == HOURLY_WEATHER)
			stream << "Year,Month,Day,Hour,Var,Value,Source\r\n";
		else
			stream << "Year,Month,Day,Var,Value,Min,Max,Source,Estimated,Manual,Missing,Rejected,Suspect\r\n";

		bool bFind = false;
		bool bSave = true;
		callback.PushTask("Update " + filePath, GetNbDayPerMonth(year, m));
		size_t nbDays = (TRef.GetYear() == year && TRef.GetMonth() == m) ? TRef.GetDay() + 1 : GetNbDayPerMonth(year, m);
		for (size_t d = 0; d < nbDays && msg; d++)
		{
			string pageURL = FormatA(PageDataFormat, type == HOURLY_WEATHER ? "HOURLY" : "DAILY", ID.c_str(), year, m + 1, d + 1);

			string source;
			msg = GetPageText(pConnection, pageURL, source, false, FLAGS);

			if (!source.empty() && source.find("No Records Were Found") == string::npos && source.find("ACIS Error") == string::npos)
			{
				try
				{
					WBSF::ReplaceString(source, "'", " ");
					zen::XmlDoc doc = zen::parse(source);

					string xml_name = (type == HOURLY_WEATHER) ? "element_value" : "aggregation_value";
					zen::XmlIn xml_in(doc.root());

					for (zen::XmlIn child = xml_in[xml_name]; child&&msg; child.next())
					{
						string var_str;
						if (child["element_cd"](var_str))
						{
							if (type == HOURLY_WEATHER)
							{
								string time;
								if (child["time"](time))
								{
									CTRef TRef = GetTRef(time, CTM::HOURLY);

									string src;
									child["source"](src);

									string value;
									if (child[(var_str == "PR") ? "delta" : "value"](value))
									{
										//output_text += FormatA("%4d,%02d,%02d,%02d,%s,%s,%s\r\n", TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, TRef.GetHour(), var_str.c_str(), value.c_str(), src.c_str());
										stream << FormatA("%4d,%02d,%02d,%02d,%s,%s,%s\r\n", TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, TRef.GetHour(), var_str.c_str(), value.c_str(), src.c_str());
									}

								}
							}
							else
							{
								string date;
								if (child["aggregation_date"](date))
								{
									CTRef TRef = GetTRef(date, CTM::DAILY);

									string value;
									if (child["value"](value))
									{

										//string checked;
										//child["checked"](checked);

										string actual;
										child["actual_percent"](actual);
										string estimated;
										child["estimated_percent"](estimated);
										string manual;
										child["manual_percent"](manual);
										string missing;
										child["missing_percent"](missing);
										string rejected;
										child["rejected_percent"](rejected);
										string suspect;
										child["suspect_percent"](suspect);

										string str_min, str_max;
										child["min"](str_min);
										child["max"](str_max);

										if (str_min.empty())
											str_min = "-999.0";

										if (str_max.empty())
											str_max = "-999.0";

										//output_text += FormatA("%4d,%02d,%02d,%s,", TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, var_str.c_str());
										//output_text += FormatA("%s,%s,%s,%s,%s,%s,%s,%s,%s\r\n", value.c_str(), str_min.c_str(), str_max.c_str(), actual.c_str(), estimated.c_str(), manual.c_str(), missing.c_str(), rejected.c_str(), suspect.c_str());
										stream << FormatA("%4d,%02d,%02d,%s,", TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, var_str.c_str());
										stream << FormatA("%s,%s,%s,%s,%s,%s,%s,%s,%s\r\n", value.c_str(), str_min.c_str(), str_max.c_str(), actual.c_str(), estimated.c_str(), manual.c_str(), missing.c_str(), rejected.c_str(), suspect.c_str());
									}
								}
							}//if hourly
						}//for all record of the day
					}
				}//try
				catch (const zen::XmlParsingError& e)
				{
					// handle error
					callback.AddMessage("Error parsing XML file: col=" + ToString(e.col) + ", row=" + ToString(e.row));
					callback.AddMessage("URL: " + pageURL);
					bSave = false;
				}
			}//if is valid

			msg += callback.StepIt();
		}//for all day


		if (msg && bSave)//always save the file to avoid to download it
		{
			ofStream  file;
			msg = file.open(filePath, ios_base::out | ios_base::binary);
			if (msg)
			{

				try
				{
					boost::iostreams::filtering_ostreambuf out;
					out.push(boost::iostreams::gzip_compressor());
					out.push(file);
					boost::iostreams::copy(stream, out);

				}//try
				catch (const boost::iostreams::gzip_error& exception)
				{
					int error = exception.error();
					if (error == boost::iostreams::gzip::zlib_error)
					{
						//check for all error code    
						msg.ajoute(exception.what());
					}
				}

				file.close();
			}//if msg

		}//if have data

		callback.PopTask();

		return msg;
	}



	std::string CUIACIS::GetStationListFilePath()const
	{
		return (std::string)GetDir(WORKING_DIR) + "StationsList.csv";
	}


	string CUIACIS::GetOutputFilePath(int year, size_t m, const string& ID)const
	{
		size_t type = as<size_t>(DATA_TYPE);
		string filepath = GetDir(WORKING_DIR) + (type == HOURLY_WEATHER ? "Hourly" : "Daily") + "\\" + ToString(year) + "\\";

		if (!OLD_CODE)
			filepath += FormatA("%02d", m + 1) + "\\";

		filepath += ID + ".csv";

		if (!OLD_CODE)
			filepath += ".gz";

		return filepath;
	}


	ERMsg CUIACIS::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;

		if (m_stations.empty())
			msg = m_stations.Load(GetStationListFilePath());

		if (msg)
			msg += m_stations.IsValid();

		if (msg)
		{
			bool bIgoneEC = as<bool>(IGNORE_ENV_CAN);

			for (size_t i = 0; i < m_stations.size(); i++)
			{

				bool bAdd = true;
				if (bIgoneEC)
					bAdd = IsInclude(m_stations[i]);
				//{
				//	//string network = m_stations[i].GetSSI("type");
				//	string ID = m_stations[i].GetSSI("EC_id");
				//	ID = ID.substr(0, 3);
				//	if (!ID.empty() && ID != "999")//&& network != "AENV"  && network != "PARC"
				//		bAdd = false;
				//}

				if (bAdd)
					stationList.push_back(m_stations[i].m_ID);
			}

		}

		return msg;
	}


	ERMsg CUIACIS::GetWeatherStation(const std::string& ID, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;

		size_t type = as <size_t>(DATA_TYPE);


		size_t pos = m_stations.FindByID(ID);
		if (pos == NOT_INIT)
		{
			msg.ajoute(FormatMsg(IDS_NO_STATION_INFORMATION, ID));
			return msg;
		}


		((CLocation&)station) = m_stations[pos];

		station.m_name = WBSF::PurgeFileName(station.m_name);
		//station.m_ID;// += "H";//add a "H" for hourly data

		for (SiteSpeceficInformationMap::iterator it = station.m_siteSpeceficInformation.begin(); it != station.m_siteSpeceficInformation.end(); it++)
		{
			WBSF::ReplaceString(it->second.first, ",", " ");
			WBSF::ReplaceString(it->second.first, ";", " ");
			WBSF::ReplaceString(it->second.first, "|", " ");
			WBSF::ReplaceString(it->second.first, "\t", " ");
			WBSF::ReplaceString(it->second.first, "\"", "'");
		}



		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = size_t(lastYear - firstYear + 1);
		station.CreateYears(firstYear, nbYears);
		station.SetHourly(type == HOURLY_WEATHER);

		//now extract data 
		for (size_t y = 0; y < nbYears&&msg; y++)
		{
			int year = firstYear + int(y);

			for (size_t m = 0; m < 12 && msg; m++)
			{
				string filePath = GetOutputFilePath(year, m, ID);
				if (FileExists(filePath))
				{
					msg = ReadDataFile(filePath, station);
					msg += callback.StepIt(0);
				}
			}
		}

		station.CleanUnusedVariable("TN T TX P TD H WS WD W2 R SD");

		if (msg)
		{
			//verify station is valid
			if (station.HaveData())
			{
				msg = station.IsValid();
			}
		}

		return msg;
	}

	static TVarH GetVar(const string& header)
	{

		TVarH var = H_SKIP;

		static const char* VAR_NAME[] = { "Air Temp. Min. (°C)", "Air Temp. Avg. (°C)", "Air Temp. Max. (°C)", "Precip. (mm)", "Est. Dew Point Temp. (°C)", "Humidity Avg. (%)", "Wind Speed 10 m Avg. (km/h)", "Wind Dir. 10 m Avg. (°)", "Incoming Solar Rad. (W/m2)", "Snow Depth (cm)", "Wind Speed 2 m Avg. (km/h)" };
		//PRCIP,ATAM,ATX,ATN,HUAM,IR,WSAM,WDAM,UA,SD,DEW
		static const TVarH VAR[] = { H_TMIN, H_TAIR, H_TMAX, H_PRCP, H_TDEW, H_RELH, H_WNDS, H_WNDD,H_SRAD, H_SNDH, H_WND2 };

		//auto a = begin(VAR_NAME);
		auto it = std::find_if(begin(VAR_NAME), end(VAR_NAME), [header](const char* name) {return IsEqual(header, name); });
		if (it != end(VAR_NAME))
			var = VAR[std::distance(begin(VAR_NAME), it)];

		return var;
	}


	ERMsg CUIACIS::ReadDataFile(const string& filePath, CWeatherStation& station)
	{
		ERMsg msg;

		size_t type = as <size_t>(DATA_TYPE);
		bool bLagOneMonth = as<bool>(MONTH_LAG);
		CTRef today = CTRef::GetCurrentTRef();

		if (IsEqual(GetFileExtension(filePath), ".gz"))
		{
			ifStream  file;

			msg = file.open(filePath, ios_base::in | ios_base::binary);
			if (msg)
			{

				try
				{
					boost::iostreams::filtering_istreambuf in;
					in.push(boost::iostreams::gzip_decompressor());
					in.push(file);
					std::istream incoming(&in);

					string line;
					std::getline(incoming, line);
					while (std::getline(incoming, line) && msg)
					{
						line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
						StringVector columns;
						columns.Tokenize(line, ",", false);//some field is empty

						size_t nbColumns = (type == HOURLY_WEATHER) ? 7 : 13;
						ASSERT(columns.size() == nbColumns);
						if (columns.size() == nbColumns)
						{
							size_t c = 0;
							int year = ToInt(columns[c++]);
							size_t m = ToInt(columns[c++]) - 1;
							size_t d = ToInt(columns[c++]) - 1;
							size_t h = 0;
							if (type == HOURLY_WEATHER)
								h = ToInt(columns[c++]);

							ASSERT(m >= 0 && m < 12);
							ASSERT(d >= 0 && d < GetNbDayPerMonth(year, m));

							CTRef TRef(year, m, d, h, type == HOURLY_WEATHER ? CTM::HOURLY : CTM::DAILY);
							ASSERT(TRef.IsValid());



							bool bUseIt = true;
							if (bLagOneMonth)
								bUseIt = today - TRef.as(CTM::DAILY) > 30;

							if (TRef.IsValid() && bUseIt)//some data have invalid TRef or we have to make a one month lag
							{
								string var_str = columns[c++];

								TVarH var = H_SKIP;
								if (var_str == "AT" || var_str == "ATA")
									var = H_TAIR;
								else if (var_str == "TX")
									var = H_TMAX;
								else if (var_str == "TN")
									var = H_TMIN;
								else if (var_str == "PR")
									var = H_PRCP;
								else if (var_str == "HU" || var_str == "HUA")
									var = H_RELH;
								else if (var_str == "WS" || var_str == "WSA")
									var = H_WNDS;
								else if (var_str == "WD" || var_str == "WDA")
									var = H_WNDD;
								else if (var_str == "US")
									var = H_WND2;
								else if (var_str == "IR")
									var = H_SRAD;
								/*else if (var_str == "PC")
									var = H_ADD1;
									else if (var_str == "P1")
									var = H_ADD2;*/


								if (var != H_SKIP)
								{
									string str = columns[c++];
									if (!str.empty())
									{
										float value = value = WBSF::as<float>(str);
										if (var == H_SNDH)
											value /= 10;

										if (type == HOURLY_WEATHER)
										{
											station[TRef].SetStat(var, value);
											if (var == H_RELH && station[TRef][H_TAIR].IsInit())
												station[TRef].SetStat(H_TDEW, Hr2Td(station[TRef][H_TAIR], value));
										}
										else
										{
											if (var == H_SRAD)
												value *= 1000000.0f / (3600 * 24);//convert MJ/m² --> W/m²

											station[TRef].SetStat(var, value);

											string str_min = columns[c++];
											string str_max = columns[c++];

											if (var == H_TAIR)
											{
												float Tmin = WBSF::as<float>(str_min);
												float Tmax = WBSF::as<float>(str_max);
												if (Tmin > -999 && Tmax > -999)
												{
													ASSERT(Tmin >= -70 && Tmin <= 70);
													ASSERT(Tmax >= -70 && Tmax <= 70);
													ASSERT(Tmin <= Tmax);

													station[TRef].SetStat(H_TMIN, Tmin);
													station[TRef].SetStat(H_TMAX, Tmax);

												}
											}
										}//if daily
									}//if not empty
								}//if good vars
							}//TRef is valid
						}//good number of column
					}//for all lines
				}//try
				catch (const boost::iostreams::gzip_error& exception)
				{
					int error = exception.error();
					if (error == boost::iostreams::gzip::zlib_error)
					{
						//check for all error code    
						msg.ajoute(exception.what());
					}
				}
			}//if msg
		}
		else
		{
			station.LoadData(filePath);
			if (type == HOURLY_WEATHER)
			{
				//Compute hourly Tair form Tmin and Tmax
				for (size_t y = 0; y < station.size(); y++)
				{
					for (size_t m = 0; m < station[y].size(); m++)
					{
						for (size_t d = 0; d < station[y][m].size(); d++)
						{
							for (size_t h = 0; h < station[y][m][d].size(); h++)
							{
								if (station[y][m][d][h][H_TAIR] == -999 && station[y][m][d][h][H_TMIN] != -999 && station[y][m][d][h][H_TMAX] != -999)
									station[y][m][d][h].SetStat(H_TAIR, (station[y][m][d][h][H_TMIN] + station[y][m][d][h][H_TMAX]) / 2.0);

								if (station[y][m][d][h][H_RELH] == -999 && station[y][m][d][h][H_TAIR] != -999 && station[y][m][d][h][H_TDEW] != -999)
									station[y][m][d][h].SetStat(H_RELH, Td2Hr(station[y][m][d][H_TAIR], station[y][m][d][h][H_TDEW]));

								if (station[y][m][d][h][H_TDEW] == -999 && station[y][m][d][h][H_TAIR] != -999 && station[y][m][d][h][H_RELH] != -999)
									station[y][m][d][h].SetStat(H_TDEW, Hr2Td(station[y][m][d][H_TAIR], station[y][m][d][h][H_RELH]));

							}
						}
					}
				}

			}
			//bool bUseIt = true;
			//if (bLagOneMonth)
				//bUseIt = today - TRef.as(CTM::DAILY) > 30;

			if (bLagOneMonth)//some data have invalid TRef or we have to make a one month lag
			{
				//reset last month data
				int ndRef = 30;

				if (type == HOURLY_WEATHER)
				{
					ndRef = 30 * 24;
					today.Transform(CTM(CTM::HOURLY), 0);
				}

				for (CTRef TRef = today - ndRef; TRef <= today; TRef++)
					station.Get(TRef).Reset();
			}
		}

		return msg;
	}


	ERMsg CUIACIS::SaveData(size_t type, const string& filePath, const string& str, CCallback& callback)
	{
		ERMsg msg;



		CWeatherYears data(type == HOURLY_WEATHER);
		//try to load old data, ignor error if the fiole does not exist
		if (type == HOURLY_WEATHER)
			data.LoadData(filePath);


		stringstream stream(str);

		vector<TVarH > variables;
		for (CSVIterator loop(stream, ",", true, true); loop != CSVIterator() && msg; ++loop)
		{
			if (variables.empty())
			{
				for (size_t c = 0; c < loop.Header().size(); c++)
				{
					TVarH var = GetVar(loop.Header()[c]);
					variables.push_back(var);
				}
			}


			if (loop->size() == variables.size())
			{
				StringVector tmp((*loop)[1], " -:");
				ASSERT(type == HOURLY_WEATHER || tmp.size() == 3);
				ASSERT(type == DAILY_WEATHER || tmp.size() == 5);

				int year = ToInt(tmp[2]);
				size_t m = CTRef::GetMonthIndex(tmp[1].c_str());
				size_t d = ToInt(tmp[0]) - 1;
				size_t h = 0;
				if (type == HOURLY_WEATHER)
					h = ToInt(tmp[3]);


				ASSERT(m >= 0 && m < 12);
				ASSERT(d >= 0 && d < GetNbDayPerMonth(year, m));

				CTRef TRef(year, m, d, h, type == HOURLY_WEATHER ? CTM::HOURLY : CTM::DAILY);
				ASSERT(TRef.IsValid());

				if (TRef.IsValid())//some data have invalid TRef or we have to make a one month lag
				{
					for (size_t c = 2; c < loop->size(); c++)
					{
						if (variables[c] != H_SKIP)
						{
							const string& str = (*loop)[c];
							if (!str.empty())
							{
								float value = value = WBSF::as<float>(str);
								//if (variables[c] == H_SNDH)
									//value /= 10;

								if (type == HOURLY_WEATHER)
								{
									data[TRef].SetStat(variables[c], value);
									//if (variables[c] == H_RELH && station[TRef][H_TAIR].IsInit())
										//station[TRef].SetStat(H_TDEW, Hr2Td(station[TRef][H_TAIR], value));
								}
								else
								{
									//if (var == H_SRAD)
										//value *= 1000000.0f / (3600 * 24);//convert MJ/m² --> W/m²

									data[TRef].SetStat(variables[c], value);

									/*string str_min = columns[c++];
									string str_max = columns[c++];

									if (var == H_TAIR)
									{
										float Tmin = WBSF::as<float>(str_min);
										float Tmax = WBSF::as<float>(str_max);
										if (Tmin > -999 && Tmax > -999)
										{
											ASSERT(Tmin >= -70 && Tmin <= 70);
											ASSERT(Tmax >= -70 && Tmax <= 70);
											ASSERT(Tmin <= Tmax);

											station[TRef].SetStat(H_TMIN, Tmin);
											station[TRef].SetStat(H_TMAX, Tmax);

										}
									}*/
								}//if daily
							}//if not empty
						}//if good vars
					}

				}//TRef is valid
			}//good number of column
		}//for all lines



		CreateMultipleDir(GetPath(filePath));
		msg = data.SaveData(filePath);

		return msg;
	}



}