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

using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace UtilWWW;

namespace WBSF
{

	class Gzip 
	{
	public:
		static std::string compress(const std::string& data)
		{
			namespace bio = boost::iostreams;

			std::stringstream compressed;
			std::stringstream origin(data);

			bio::filtering_streambuf<bio::input> out;
			out.push(bio::gzip_compressor(bio::gzip_params(bio::gzip::best_compression)));
			out.push(origin);
			bio::copy(out, compressed);

			return compressed.str();
		}

		static std::string decompress(const std::string& data)
		{
			namespace bio = boost::iostreams;

			std::stringstream compressed(data);
			std::stringstream decompressed;

			bio::filtering_streambuf<bio::input> out;
			out.push(bio::gzip_decompressor());
			out.push(compressed);
			bio::copy(out, decompressed);

			return decompressed.str();
		}
	};


	//*********************************************************************
	static const DWORD FLAGS = INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_PRAGMA_NOCACHE;

	//*********************************************************************
	const char* CUIACIS::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "UserName", "Password", "WorkingDir", "DataType", "FirstYear", "LastYear", "UpdateStationsList", "IgnoreEnvCan", "MonthLag" };
	const size_t CUIACIS::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_STRING, T_PASSWORD, T_PATH, T_COMBO_INDEX, T_STRING, T_STRING, T_BOOL, T_BOOL, T_BOOL };
	const UINT CUIACIS::ATTRIBUTE_TITLE_ID = IDS_UPDATER_ALBERTA_P;
	const UINT CUIACIS::DESCRIPTION_TITLE_ID = ID_TASK_ALBERTA;

	const char* CUIACIS::CLASS_NAME(){ static const char* THE_CLASS_NAME = "ACISHourly";  return THE_CLASS_NAME; }
	CTaskBase::TType CUIACIS::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUIACIS::CLASS_NAME(), (createF)CUIACIS::create);

	const char* CUIACIS::SERVER_NAME = "agriculture.alberta.ca";

	
	//http://agriculture.alberta.ca/acis/rss/data?type=DAILY&stationId=2274&date=20160102
	
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
		case WORKING_DIR: str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()) + "ACIS\\"; break;
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
	

	ERMsg CUIACIS::DownloadStationList(CLocationVector& stationList, CCallback& callback)const
	{
		ERMsg msg;

		stationList.clear();

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
		bool bIgoneEC = as<bool>(IGNORE_ENV_CAN);
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

					bool bND = !TRef1.IsInit() || TRef1 - TRef2 < 2; //let 2 days to update the data if it's not the current month
					if (bND && bIgoneEC)
					{
						string ID = m_stations[i].GetSSI("EC_id");
						ID = ID.substr(0, 3);
						if (!ID.empty() && ID != "999")
							bND = false;
					}

					bNeedDownload[i][y][m] = bND;
					nbFilesToDownload += bND ? 1 : 0;

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

		callback.PushTask("Download ACIS data (" + ToString(nbFilesToDownload) + " files)", nbFilesToDownload);
		//msg += DownloadMonth(pConnection, 2016, AUGUST, "2154", "c:/tmp/text.csv", callback);
		//return msg;

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

	ERMsg CUIACIS::VerifyUserPass(CCallback& callback)
	{
		ERMsg msg;


		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;
		msg = GetHttpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, Get(USER_NAME), Get(PASSWORD));
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
			pConnection.release();
			pSession->Close();
			pSession.release();
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

		//stream << output_text;

		bool bFind = false;
		callback.PushTask("Update " + filePath, GetNbDayPerMonth(year, m));
		size_t nbDays = (TRef.GetYear() == year&&TRef.GetMonth() == m) ? TRef.GetDay() + 1 : GetNbDayPerMonth(year, m);
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
					msg.ajoute("Error parsing XML file: col=" + ToString(e.col) + ", row=" + ToString(e.row));
				}
			}//if is valid

			msg += callback.StepIt();
		}//for all day

		
		if (msg /*&& !output_text.empty()*/)//always save the file to avoid to download it
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
		return GetDir(WORKING_DIR) + (type == HOURLY_WEATHER ? "Hourly" : "Daily") + "\\" + ToString(year) + "\\" + FormatA("%02d", m + 1) + "\\" + ID + ".csv.gz";
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
				{
					string ID = m_stations[i].GetSSI("EC_id");
					ID = ID.substr(0, 3);
					if (!ID.empty() && ID != "999")
						bAdd = false;
				}

				if( bAdd )
					stationList.push_back(m_stations[i].m_ID);
			}

		}

		return msg;
	}
	
	
	ERMsg CUIACIS::GetWeatherStation(const std::string& ID, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;

		size_t type = as <size_t>(DATA_TYPE);
		if ( TM.Type() == CTM::DAILY && type != DAILY_WEATHER)
		{
			msg.ajoute("Daily from hourly is not supported"); 
			return msg;
		}

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
		station.SetHourly(TM.Type()==CTM::HOURLY);

		//now extact data
		CWeatherAccumulator accumulator(TM);


		//now extract data 
		for (size_t y = 0; y < nbYears&&msg; y++)
		{
			int year = firstYear + int(y);

			for (size_t m = 0; m < 12 && msg; m++)
			{
				string filePath = GetOutputFilePath(year, m, ID);
				if (FileExists(filePath))
				{
					msg = ReadDataFile(filePath, station, accumulator);
					msg += callback.StepIt(0);
				}
			}
		}

		/*if (accumulator.GetTRef().IsInit())
		{
			CTPeriod period(CTRef(firstYear, 0, 0, 0, TM), CTRef(lastYear, LAST_MONTH, LAST_DAY, LAST_HOUR, TM));
			if (period.IsInside(accumulator.GetTRef()))
				station[accumulator.GetTRef()].SetData(accumulator);
		}*/

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

	

	ERMsg CUIACIS::ReadDataFile(const string& filePath, CWeatherStation& station, CWeatherAccumulator& accumulator)
	{
		ERMsg msg;
		size_t type = as <size_t>(DATA_TYPE);
		bool bLagOneMonth = as<bool>(MONTH_LAG);
		
		CTRef today = CTRef::GetCurrentTRef();

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
					StringVector columns(line, ",");

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
							bUseIt = today - TRef.as(CTM::DAILY)>30;

						if (TRef.IsValid() && bUseIt)//some data have invalid TRef or we have to make a one month lag
						{
							string var_str = columns[c++];

							TVarH var = H_SKIP;
							if (var_str == "AT" || var_str == "ATA")
								var = H_TAIR2;
							else if (var_str == "TX")
								var = H_TMAX2;
							else if (var_str == "TN")
								var = H_TMIN2;
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
								var = H_SRAD2;
							/*else if (var_str == "PC")
								var = H_ADD1;
								else if (var_str == "P1")
								var = H_ADD2;*/


							if (var != H_SKIP)
							{
								string str = columns[c++];
								float value = value = WBSF::as<float>(str);
								if (var == H_SNDH)
									value /= 10;

								if (type == HOURLY_WEATHER)
								{
									//if (accumulator.TRefIsChanging(TRef))
									//station[accumulator.GetTRef()].SetData(accumulator);

									//accumulator.Add(TRef, var, value);

									station[TRef].SetStat(var, value);
									if (type == HOURLY_WEATHER && var == H_RELH && station[TRef][H_TAIR2])
										station[TRef].SetStat(H_TDEW, Hr2Td(station[TRef][H_TAIR2], value));

								}
								else
								{
									if (var == H_SRAD2 && type == DAILY_WEATHER)
										value *= 1000000.0f / (3600 * 24);//convert MJ/m² --> W/m²

									station[TRef].SetStat(var, value);

									string str_min = columns[c++];
									string str_max = columns[c++];

									if (var == H_TAIR2)
									{
										float Tmin = WBSF::as<float>(str_min);
										float Tmax = WBSF::as<float>(str_max);
										if (Tmin > -999 && Tmax > -999)
										{
											ASSERT(Tmin >= -70 && Tmin <= 70);
											ASSERT(Tmax >= -70 && Tmax <= 70);
											ASSERT(Tmin <= Tmax);

											station[TRef].SetStat(H_TMIN2, Tmin);
											station[TRef].SetStat(H_TMAX2, Tmax);

										}
									}
								}//if daily
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


		return msg;
	}






}