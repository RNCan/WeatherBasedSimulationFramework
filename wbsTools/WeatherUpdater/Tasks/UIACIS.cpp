#include "StdAfx.h"
#include "UIACIS.h"



#include "Basic/HourlyDatabase.h"
#include "Basic/CSV.h"
#include "Basic/FileStamp.h"
#include "UI/Common/SYShowMessage.h"
#include "TaskFactory.h"

#include "../Resource.h"
#include "WeatherBasedSimulationString.h"

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
//Accumulated Precipitation in Gauge(snow ? ? )			P1		[mm]
//Precipitation											PC		[mm]


using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace UtilWWW;

namespace WBSF
{

	//*********************************************************************
	static const DWORD FLAGS = INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_PRAGMA_NOCACHE;

	//*********************************************************************
	const char* CUIACIS::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "UserName", "Password", "WorkingDir", "DataType", "FirstYear", "LastYear" };
	const size_t CUIACIS::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_STRING, T_PASSWORD, T_PATH, T_COMBO_INDEX, T_STRING, T_STRING };
	const UINT CUIACIS::ATTRIBUTE_TITLE_ID = IDS_UPDATER_ACIS_P;
	const UINT CUIACIS::DESCRIPTION_TITLE_ID = ID_TASK_ACIS;

	const char* CUIACIS::CLASS_NAME(){ static const char* THE_CLASS_NAME = "ACISHourly";  return THE_CLASS_NAME; }
	CTaskBase::TType CUIACIS::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUIACIS::CLASS_NAME(), (createF)CUIACIS::create);

	const char* CUIACIS::SERVER_NAME = "agriculture.alberta.ca";

	
	//http://agriculture.alberta.ca/acis/rss/data?type=DAILY&stationId=2274&date=20160102
	
	CUIACIS::CUIACIS(void)
	{}

	CUIACIS::~CUIACIS(void)
	{}

	/*long CUIACIS::GetNbDay(const CTime& t)
	{
		return GetNbDay(t.GetYear(), t.GetMonth() - 1, t.GetDay() - 1);
	}

	long CUIACIS::GetNbDay(int y, int m, int d)
	{
		ASSERT(m >= 0 && m < 12);
		ASSERT(d >= 0 && d < 31);

		return long(y * 365 + m*30.42 + d);
	}*/

	//StringVector typeList("Hourly|Daily", "|");

	

	std::string CUIACIS::Option(size_t i)const
	{
		string str;
		switch (i)
		{
		case DATA_TYPE:	str = GetString(IDS_STR_WDATA_TYPE); break;
		};
		return str;
	}



	std::string CUIACIS::Default(size_t i)const
	{
		string str;

		switch (i)
		{
		case WORKING_DIR: str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()) + "ACIS\\"; break;
		case DATA_TYPE: str = "1"; break;
		case FIRST_YEAR:
		case LAST_YEAR:	str = ToString(CTRef::GetCurrentTRef().GetYear()); break;
		};

		return str;
	}

	//Interface attribute index to attribute index
	static const char PageDataFormat[] = "acis/rss/data?type=%s&stationId=%s&date=%4d%02d%02d";
	

	ERMsg CUIACIS::DownloadStationList(CLocationVector& stationList, CCallback& callback)const
	{
		ERMsg msg;


		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;
		
		msg = GetHttpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, Get(USER_NAME), Get(PASSWORD));
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
				WBSF::ReplaceString(source, "'"," ");
				zen::XmlDoc doc = zen::parse(source);

				zen::XmlIn in(doc.root());
				for (zen::XmlIn child = in["station"]; child&&msg; child.next())
				{
					CLocation location;
					enum TAttributes{ ACIS_STATION_ID, COMMENT, ELEVATION, LATITUDE, LONGITUDE, OPERATOR, OWNER, POSTAL_CODE, STATION_NAME, TYPE, NB_ATTRIBUTES };
					static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "acis_station_id", "comment", "elevation", "latitude", "longitude", "operator", "owner ", "postal_code ", "station_name", "type" };
					for (size_t i = 0; i < NB_ATTRIBUTES; i++)
					{
						string str;
						if (child.attribute(ATTRIBUTE_NAME[i], str))
						{
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
						auto iterPair = pIds->getChildren();;
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

	ERMsg CUIACIS::Execute(CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);

		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(workingDir, 1);
		callback.AddMessage(GetString(IDS_UPDATE_FROM));
		callback.AddMessage(SERVER_NAME, 1);
		callback.AddMessage("");


		if (FileExists(GetStationListFilePath()))
		{
			msg = m_stations.Load(GetStationListFilePath());
		}
		else
		{
			CreateMultipleDir(GetPath(GetStationListFilePath()));
			msg = DownloadStationList(m_stations, callback);

			if (msg)
				msg = m_stations.Save(GetStationListFilePath());
		}

		if (!msg)
			return msg;


		
		msg = DownloadStation(callback);
		
		return msg;
	}



	ERMsg CUIACIS::DownloadStation(CCallback& callback)
	{
		ERMsg msg;


		CTRef today = CTRef::GetCurrentTRef();
		string workingDir = GetDir(WORKING_DIR);
		int nbFilesToDownload = 0;
		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = lastYear - firstYear + 1;
		callback.PushTask("Clear stations list...", m_stations.size()*nbYears * 12);

		vector<vector<array<bool, 12>>> bNeedDownload(m_stations.size());
		for (size_t i = 0; i < m_stations.size() && msg; i++)
		{
			bNeedDownload[i].resize(nbYears);
			for (size_t y = 0; y < nbYears&&msg; y++)
			{
				int year = firstYear + int(y);

				size_t nbm = year == today.GetYear() ? today.GetMonth()+1 : 12;
				for (size_t m = 0; m < nbm && msg; m++)
				{
					string filePath = GetOutputFilePath(year, m, m_stations[i].m_ID);
					CTimeRef TRef1(GetFileStamp(filePath));
					CTRef TRef2(year, m, LAST_DAY);

					bNeedDownload[i][y][m] = !TRef1.IsInit() || TRef1 - TRef2 < 2; //let 2 days to update the data if it's not the current month
					nbFilesToDownload += bNeedDownload[i][y][m] ? 1 : 0;

					msg += callback.StepIt();
				}
			}
		}

		callback.PopTask();

		

		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;
		msg = GetHttpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, Get(USER_NAME), Get(PASSWORD));
		if (!msg)
			return msg;

		pSession->SetOption(INTERNET_OPTION_RECEIVE_TIMEOUT, 15000);

		int nbDownload = 0;
		int currentNbDownload = 0;

		callback.PushTask("Download data", nbFilesToDownload);
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

						msg += DownloadMonth(pConnection, year, m, m_stations[i].m_ID, filePath, callback);
						if (msg || callback.GetUserCancel())
						{
							nbDownload++;
							currentNbDownload++;
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

							msg = GetHttpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, Get(USER_NAME), Get(PASSWORD));
							currentNbDownload = 0;
						}//if msg
					}//if need download
				}//for all months
			}//for all years
		}//for all station


		//clean connection
		pConnection->Close();
		pConnection.release();
		pSession->Close();
		pSession.release();


		callback.AddMessage(GetString(IDS_NB_FILES_DOWNLOADED) + ToString(nbDownload));
		callback.PopTask();

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

	ERMsg CUIACIS::DownloadMonth(CHttpConnectionPtr& pConnection, int year, size_t m, const string& ID, const string& filePath, CCallback& callback)
	{
		ERMsg msg;


		if (msg)
		{
			size_t type = as <size_t>(DATA_TYPE);


			CWeatherYears data(type == HOURLY_WEATHER);
			data.CreateYear(year);
			
			callback.PushTask("Update " + filePath, GetNbDayPerMonth(year, m));
			for (size_t d = 0; d < GetNbDayPerMonth(year, m) && msg; d++)
			{
				string pageURL = FormatA(PageDataFormat, type==HOURLY_WEATHER?"HOURLY":"DAILY", ID.c_str(), year, m + 1, d + 1);

				string source;
				msg = GetPageText(pConnection, pageURL, source, false, FLAGS);

				if (!source.empty() && source.find("No Records Were Found") == string::npos && source.find("ACIS Error") == string::npos)
				{
					try
					{
						//replace all ' by space
						WBSF::ReplaceString(source, "'", " ");
						zen::XmlDoc doc = zen::parse(source);
						
						string xml_name = (type == HOURLY_WEATHER) ? "element_value" : "aggregation_value";

						zen::XmlIn in(doc.root());
						for (zen::XmlIn child = in[xml_name]; child&&msg; child.next())
						{
							string var_str;
							if (child["element_cd"](var_str))
							{

								TVarH var = H_SKIP;
								if (var_str == "AT" || var_str == "ATA")
									var = H_TAIR;
								else if (var_str == "TX")
									var = H_TAIR;
								else if (var_str == "TN")
									var = H_TAIR;
								else if (var_str == "PR")
									var = H_PRCP;
								else if (var_str == "HU" || var_str == "HUA")
									var = H_RELH;
								else if (var_str == "WS" || var_str == "WSA")
									var = H_WNDS;
								else if (var_str == "WD" || var_str == "WDA")
									var = H_WNDD;
								else if (var_str == "US" )
									var = H_WND2;
								else if (var_str == "PC")
									var = H_SNOW;
								else if (var_str == "P1")
									var = H_SNDH;// en mm d'eau ou en mm de hauteur???

								if (var != H_SKIP)
								{
									if (type == HOURLY_WEATHER)
									{
										string time;
										if (child["time"](time))
										{
											CTRef TRef = CTRef(year, m, d, GetHour(time));

											string str;
											if (child[(var == H_PRCP) ? "delta" : "value"](str))
											{
												float value = value = WBSF::as<float>(str);
												if (var == H_SNDH)
													value /= 10;

												data[TRef].SetStat(var, value);
											}

										}
									}
									else
									{
										CTRef TRef(year, m, d);

										string str;
										if (child["value"](str))
										{
											float value = value = WBSF::as<float>(str);
											if (var == H_SNDH)
												value /= 10;
											data[TRef].SetStat(var, value);
										}
										
										if (var == H_TAIR)
										{
											string tmin, tmax;
											if (child["min"](tmin) && child["max"](tmax))
											{
												float Tmin = WBSF::as<float>(tmin);
												float Tmax = WBSF::as<float>(tmax);
												ASSERT(Tmin >= -70 && Tmin <= 70);
												ASSERT(Tmax >= -70 && Tmax <= 70);
												ASSERT(Tmin <= Tmax);

												data[TRef].SetStat(H_TAIR, (Tmin+Tmax)/2);
												data[TRef].SetStat(H_TRNG, Tmax-Tmin);
											}
										}// if Tair
									}//if hourly
								}//if good var
							}//if element
						}//for all record of the day
					}//try
					catch (const zen::XmlParsingError& e)
					{
						// handle error
						msg.ajoute("Error parsing XML file: col=" + ToString(e.col) + ", row=" + ToString(e.row));
					}

				}//if is valid

				msg+= callback.StepIt();
			}//for all day

			if (data.HaveData())
			{
				msg += data.SaveData(filePath);
			}
		}

		callback.PopTask();

		return msg;
	}
	//ERMsg CUIACIS::DownloadStationHourly(CCallback& callback)
	//{
	////msg += DownlaodFeeds();
	//////http://agriculture.alberta.ca/acis/rss/available-data?type=hourly&stationId=10540
	////<rss version = "2.0">
	////	<channel>
	////	<title>Hourly Weather Data For Abee AGDM< / title>
	////	<link>http://agriculture.alberta.ca/acis/rss</link>
	////<description>...< / description>
	////	<pubDate>Mon, 25 Apr 2016 00 : 00 : 00 MDT< / pubDate>
	////	<generator>jRSSGenerator by Henrique A.Viecili< / generator>
	////	<docs>http ://agriculture.alberta.ca/acis/references.jsp</docs>
	////	<item>
	////	<title>Apr 25, 2016 < / title >
	////	<link>
	////http ://agriculture.alberta.ca/acis/rss/data?type=HOURLY&stationId=10540&date=20160425
	////	  < / link>
	////	  <description>Hourly Weather Data for Abee AGDM April 25, 2016 < / description >
	////	  <author>Alberta Alberta Agriculture and Forestry< / author>
	////	  <pubDate>Mon, 25 Apr 2016 14 : 05 : 17 MDT< / pubDate>
	////	  < / item>

	//	string sessionID = GetSessiosnID(pConnection);
	//	int nbDownload = 0;
	//	int currentNbDownload = 0;

	//	callback.PushTask("Download data", nbFilesToDownload);
	//	for (size_t i = 0; i < m_stations.size() && msg; i++)
	//	{
	//		if (m_stations[i].GetSSI("ClimateID").substr(0, 3) == "999")
	//		{
	//			for (size_t y = 0; y < nbYears&&msg; y++)
	//			{
	//				int year = firstYear + int(y);
	//				for (size_t m = 0; m < 12 && msg; m++)
	//				{
	//					if (bNeedDownload[i][y][m])
	//					{

	//						string filePath = GetOutputFilePath(year, m, m_stations[i].m_ID);
	//						CreateMultipleDir(GetPath(filePath));

	//						string pageURL = FormatA(PageDataFormatH, m_stations[i].m_ID.c_str(), year, m + 1, 1, year, m + 1, GetNbDayPerMonth(year, m), sessionID.c_str());

	//						string source;
	//						msg = GetPageText(pConnection, pageURL, source, false, FLAGS | INTERNET_FLAG_FORMS_SUBMIT);
	//						if (msg)
	//						{
	//							if (source.find("An Error Has Occurred") == string::npos &&
	//								source.find("Page Not Found") == string::npos &&
	//								source.find("Too Many Requests") == string::npos)
	//							{
	//								ofStream file;
	//								msg = file.open(filePath);
	//								if (msg)
	//								{
	//									file << source;
	//									file.close();
	//								}


	//								nbDownload++;
	//								currentNbDownload++;
	//								msg += callback.StepIt();
	//							}
	//							else
	//							{
	//								msg.ajoute(source);
	//							}

	//						}
	//						else
	//						{
	//							pConnection->Close();
	//							pSession->Close();

	//							//wait 5 seconds 
	//							callback.PushTask("Waiting 5 seconds...", 50);
	//							for (size_t s = 0; s < 50 && msg; s++)
	//							{
	//								Sleep(100);
	//								msg += callback.StepIt();
	//							}
	//							callback.PopTask();

	//							msg = GetHttpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, Get(USER_NAME), Get(PASSWORD));
	//							string olID = sessionID;
	//							sessionID = GetSessiosnID(pConnection);

	//							assert(sessionID != olID);
	//							currentNbDownload = 0;
	//						}
	//					}
	//				}
	//			}
	//		}
	//	}


	//	//clean connection
	//	pConnection->Close();
	//	pConnection.release();
	//	pSession->Close();
	//	pSession.release();


	//	callback.AddMessage(GetString(IDS_NB_FILES_DOWNLOADED) + ToString(nbDownload), 2);
	//	callback.PopTask();

	//}
	
	std::string CUIACIS::GetStationListFilePath()const
	{
		return (std::string)GetDir(WORKING_DIR) + "StationsList.csv";
	}


	string CUIACIS::GetOutputFilePath(int year, size_t m, const string& ID)const
	{
		size_t type = as<size_t>(DATA_TYPE);
		return GetDir(WORKING_DIR) + (type == HOURLY_WEATHER ? "Hourly" : "Daily") + "\\" + ToString(year) + "\\" + FormatA("%02d", m + 1) + "\\" + ID + ".csv";
	}


	ERMsg CUIACIS::GetStationList(StringVector& stationList, CCallback& callback)
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
	
	
	ERMsg CUIACIS::GetWeatherStation(const std::string& ID, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;

		size_t pos = m_stations.FindByID(ID);
		if (pos == NOT_INIT)
		{
			msg.ajoute(FormatMsg(IDS_NO_STATION_INFORMATION, ID));
			return msg;
		}


		((CLocation&)station) = m_stations[pos];

		station.m_name = station.m_name;
		station.m_ID;// += "H";//add a "H" for hourly data

		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = size_t(lastYear - firstYear + 1);
		station.CreateYears(firstYear, nbYears);

		//if (nbYears > 10)
			//callback.PushTask();

		//now extract data 
		for (size_t y = 0; y < nbYears&&msg; y++)
		{
			int year = firstYear + int(y);

			for (size_t m = 0; m < 12 && msg; m++)
			{
				string filePath = GetOutputFilePath(year, m, ID);
				if (FileExists(filePath))
				{
					CWeatherYears data;
					msg = data.LoadData(filePath);
					if (msg)
						station[year][m]= data[year][m];

					msg += callback.StepIt(0);
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

		return msg;
	}

}