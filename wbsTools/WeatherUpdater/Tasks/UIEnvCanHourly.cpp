#include "StdAfx.h"
#include "UIEnvCanHourly.h"

#include <iostream>
#include <algorithm>
#include <iterator> 
#include "Basic/FileStamp.h"
#include "Basic/CSV.h"
#include "Basic/Statistic.h"
#include "Basic/UtilMath.h"

#include "Basic/Psychrometrics_SI.h"
#include "UI/Common/SYShowMessage.h"
#include "UI/Common/UtilWin.h"
#include "TaskFactory.h"
#include "Geomatic/TimeZones.h"
//#include "cctz\time_zone.h"




#include "../Resource.h"

using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace UtilWWW;

namespace WBSF
{

	//catalogue de toute les stations:
	//ftp://ftp.tor.ec.gc.ca/Pub/About_the_data/Station_catalogue/station_data_catalogue.txt

	//nouveau site:
	//http://beta.weatheroffice.gc.ca/observations/swob-ml/20170622/CACM/2017-06-22-0300-CACM-AUTO-swob.xml


	const char* CUIEnvCanHourly::SERVER_NAME = "climate.weather.gc.ca";
	//*********************************************************************

	const char* CUIEnvCanHourly::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "FirstYear", "LastYear", "Province", "Network" };
	const size_t CUIEnvCanHourly::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_STRING, T_STRING, T_STRING_SELECT, T_STRING_SELECT };
	const UINT CUIEnvCanHourly::ATTRIBUTE_TITLE_ID = IDS_UPDATER_EC_HOURLY_P;
	const UINT CUIEnvCanHourly::DESCRIPTION_TITLE_ID = ID_TASK_EC_HOURLY;

	const char* CUIEnvCanHourly::CLASS_NAME() { static const char* THE_CLASS_NAME = "EnvCanHourly";  return THE_CLASS_NAME; }
	CTaskBase::TType CUIEnvCanHourly::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUIEnvCanHourly::CLASS_NAME(), (createF)CUIEnvCanHourly::create);


	CUIEnvCanHourly::CUIEnvCanHourly(void)
	{}

	CUIEnvCanHourly::~CUIEnvCanHourly(void)
	{}


	std::string CUIEnvCanHourly::Option(size_t i)const
	{
		string str;
		switch (i)
		{
		case PROVINCE:	str = CProvinceSelection::GetAllPossibleValue(); break;
		case NETWORK:	str = "Hist=Historical|SWOB=SWOB"; break;
		};
		return str;
	}

	std::string CUIEnvCanHourly::Default(size_t i)const
	{
		std::string str;
		switch (i)
		{
		case WORKING_DIR:	str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()) + "EnvCan\\Hourly\\"; break;
		case FIRST_YEAR:
		case LAST_YEAR:		str = ToString(CTRef::GetCurrentTRef().GetYear()); break;
		};

		return str;
	}

	long CUIEnvCanHourly::GetNbDay(const CTime& t) { return GetNbDay(t.GetYear(), t.GetMonth() - 1, t.GetDay() - 1); }
	long CUIEnvCanHourly::GetNbDay(int year, size_t m, size_t d)
	{
		ASSERT(m >= 0 && m < 12);
		ASSERT(d >= 0 && d < 31);

		return long(year * 365 + m * 30.42 + d);
	}
	//
	//void CUIEnvCanHourly::Reset()
	//{
	//	CTaskBase::Reset();
	//
	//	m_selection.Reset();
	//	m_firstMonth=0;
	//	m_lastMonth=11;
	//	m_bForceDownload=false;
	//	m_bExtractWindDir=false;
	//	m_bExtractVaporPressure=false;
	//	m_bExtractPressure = false;
	//
	//	m_boundingBox = DEFAULT_BOUDINGBOX;
	//		
	//	m_nbDays = GetNbDay(CTime::GetCurrentTime());
	//
	//}


	static string CleanString(string str)
	{
		string output;

		//str = FindString(str, "<td>", "</td>");
		ReplaceString(str, "<abbr title=\"degrees\">", "");
		ReplaceString(str, "<abbr title=\"minute\">", "");
		ReplaceString(str, "<abbr title=\"second\">", "");
		ReplaceString(str, "<abbr title=\"North\">", "");
		ReplaceString(str, "<abbr title=\"West\">", "");
		ReplaceString(str, "<abbr title=\"metre\">", "");
		ReplaceString(str, "</abbr>", "");


		ReplaceString(str, "m", "");
		ReplaceString(str, "&deg;", " ");
		ReplaceString(str, "&quot;", " ");
		ReplaceString(str, "&#x09;", "");
		ReplaceString(str, "°", " ");
		ReplaceString(str, "'", " ");
		ReplaceString(str, "\"", " ");
		ReplaceString(str, "N", " ");
		ReplaceString(str, "W", " ");
		ReplaceString(str, ",", "");//remove thousand separator (,)

		Trim(str);


		return str;
	}

	static double GetCoordinate(string str)
	{
		float ld = 0, lm = 0, ls = 0;
		sscanf(str.c_str(), "%f %f %f", &ld, &lm, &ls);

		return ld + Signe(ld)*(lm / 60.0 + ls / 3600.0);
	}


	ERMsg CUIEnvCanHourly::DownloadStationList(CLocationVector& stationList, CCallback& callback)const
	{
		ERMsg msg;

		//Interface attribute index to attribute index
		//sample for alberta:
		//http://climate.weatheroffice.ec.gc.ca/advanceSearch/searchHistoricDataStations_f.html?timeframe=1&Prov=XX&StationID=99999&Year=2007&Month=10&Day=2&selRowPerPage=ALL&optlimit=yearRange&searchType=stnProv&startYear=2007&endYear=2007&lstProvince=ALTA&startRow=1

		static const char pageFormat[] =
			"historical_data/search_historic_data_stations_e.html?"
			"searchType=stnProv&"
			"timeframe=1&"
			"lstProvince=%s&"
			"optLimit=yearRange&"
			"StartYear=%d&"
			"EndYear=%d&"
			"Year=%d&"
			"Month=%d&"
			"Day=%d&"
			"selRowPerPage=%d&"
			"startRow=%d&";
		//"cmdProvSubmit=Search";

		static const short SEL_ROW_PER_PAGE = 100;

		CProvinceSelection selection;
		selection.FromString(Get(PROVINCE));
		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		callback.PushTask(GetString(IDS_LOAD_STATION_LIST), selection.any() ? selection.count() : CProvinceSelection::NB_PROVINCES);


		size_t nbRun = 0;
		size_t curI = 0;
		while (curI < CProvinceSelection::NB_PROVINCES && msg)
		{
			nbRun++;
			CInternetSessionPtr pSession;
			CHttpConnectionPtr pConnection;

			msg = GetHttpConnection(SERVER_NAME, pConnection, pSession);
			if (msg)
			{

				try
				{
					//loop on province
					for (size_t i = curI; i < CProvinceSelection::NB_PROVINCES&&msg; i++)
					{
						if (selection[i])
						{

							//first call
							CTime today = CTime::GetCurrentTime();
							string URL = FormatA(pageFormat, selection.GetName(i, CProvinceSelection::ABVR).c_str(), firstYear, lastYear, today.GetYear(), today.GetMonth(), today.GetDay(), SEL_ROW_PER_PAGE, 1);

							int nbStation = GetNbStation(pConnection, URL);

							if (nbStation != -1)
							{
								short nbPage = (nbStation - 1) / SEL_ROW_PER_PAGE + 1;

								callback.AddMessage(FormatMsg(IDS_LOAD_PAGE, selection.GetName(i, CProvinceSelection::NAME), ToString(nbPage)));

								for (int j = 0; j < nbPage&&msg; j++)
								{
									short startRow = j * SEL_ROW_PER_PAGE + 1;
									URL = FormatA(pageFormat, selection.GetName(i, CProvinceSelection::ABVR).c_str(), firstYear, lastYear, today.GetYear(), today.GetMonth(), today.GetDay(), SEL_ROW_PER_PAGE, startRow);
									msg = GetStationListPage(pConnection, URL, stationList);

									msg += callback.StepIt(1.0 / nbPage);
								}
							}
							else
							{
								throw(new UtilWin::CStringException(UtilWin::GetCString(IDS_SERVER_DOWN)));
								//msg.ajoute(GetString(IDS_SERVER_DOWN));
							}
						}



						curI++;
						nbRun = 0;

					}
				}
				catch (CException* e)
				{
					if (nbRun < 5)
					{
						callback.AddMessage(UtilWin::SYGetMessage(*e));

						callback.PushTask("Waiting 30 seconds for server...", 600);
						for (size_t i = 0; i < 600 && msg; i++)
						{
							Sleep(50);//wait 50 milisec
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
			}
		}

		callback.AddMessage(GetString(IDS_NB_STATIONS) + ToString(stationList.size()));
		callback.PopTask();

		return msg;
	}


	int CUIEnvCanHourly::GetNbStation(CHttpConnectionPtr& pConnection, const string& URL)const
	{
		int nbStation = -1;
		string source;

		if (GetPageText(pConnection, URL, source))
		{
			string::size_type posBegin = source.find("stations found");

			if (posBegin != string::npos)
			{
				posBegin -= 8;//return before hte requested number
				string tmp = FindString(source, ">", "stations found", posBegin);
				nbStation = ToInt(tmp);
			}
		}

		return nbStation;
	}

	ERMsg CUIEnvCanHourly::GetStationListPage(CHttpConnectionPtr& pConnection, const string& page, CLocationVector& stationList)const
	{
		ERMsg msg;
		string source;
		msg = GetPageText(pConnection, page, source);
		if (msg)
		{
			if (Find(source, "stations found"))
			{
				msg = ParseStationListPage(source, stationList);
			}
		}

		return msg;
	}

	CTPeriod CUIEnvCanHourly::String2Period(string period)
	{
		ASSERT(!period.empty() && period != "N/A" && period != "|");

		//ReplaceString(period, "-999", "00");

		StringVector str(period, "-|");
		ASSERT(str.size() == 6);

		int v[6] = { 1100, 1, 1, 2099, 12, 31 };

		if (str.size() == 6)
		{
			for (int i = 0; i < 6; i++)
				v[i] = ToInt(str[i]);
		}

		return CTPeriod(CTRef(v[0], v[1] - 1, v[2] - 1), CTRef(v[3], v[4] - 1, v[5] - 1));
	}

	static string PurgeQuote(string str)
	{
		WBSF::ReplaceString(str, "\"", "");
		return str;
	}

	ERMsg CUIEnvCanHourly::ParseStationListPage(const string& source, CLocationVector& stationList)const
	{
		ERMsg msg;
		string::size_type posBegin = 0;
		string::size_type posEnd = 0;

		FindString(source, "form action=", "method=\"post\"", posBegin, posEnd);

		while (posBegin != string::npos)
		{
			CLocation stationInfo;

			string period = PurgeQuote(FindString(source, "name=\"hlyRange\" value=", "/>", posBegin, posEnd));
			string internalID = PurgeQuote(FindString(source, "name=\"StationID\" value=", "/>", posBegin, posEnd));
			string prov = PurgeQuote(FindString(source, "name=\"Prov\" value=", "/>", posBegin, posEnd));
			string name = PurgeQuote(FindString(source, "<div class=\"col-lg-3 col-md-3 col-sm-3 col-xs-3\">", "</div>", posBegin, posEnd));

			if (posBegin != string::npos)
			{
				//when the station don't have hourly value, the period ios "|"
				if (!period.empty() && period != "N/A" && period != "|")
				{
					stationInfo.m_name = Trim(name);
					stationInfo.SetSSI("InternalID", internalID);
					stationInfo.SetSSI("Province", prov);
					stationInfo.SetSSI("Period", period);

					stationList.push_back(stationInfo);
				}

				FindString(source, "form action=", "method=\"post\"", posBegin, posEnd);
			}
		}

		return msg;
	}

	ERMsg CUIEnvCanHourly::UpdateStationList(CLocationVector& stationList, CLocationVector & stations, CCallback& callback)const
	{
		ERMsg msg;

		//update coordinates
		callback.PushTask("Update coordinates", stationList.size());
		//callback.SetNbStep(stationList.size());


		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;

		msg = GetHttpConnection(SERVER_NAME, pConnection, pSession);
		if (!msg)
			return msg;


		for (CLocationVector::iterator it = stationList.begin(); it != stationList.end(); it++)
		{
			//this station doesn't exist, we add it
			//CTPeriod period = String2Period(it->GetSSI("Period"));
			//
			//string ID = it->m_ID;
			////string internalID = it->GetSSI("InternalID");
			////__int64 ID = ToInt64(internalID);
			//CLocationMap::iterator it2 = stationMap.find(ID);
			//if (it2 == m_stations.end() || it2->second.m_lat == -999)
			//{
			//	stationMap[ID] = *it;
			//	__int64 internalID = WBSF::as<__int64>(it->GetSSI("InternalID"));
			//	msg += UpdateCoordinate(pConnection, internalID, period.End().GetYear(), period.End().GetMonth(), period.End().GetDay(), stationMap[ID]);
			//}
			//else
			//{
			//	stationMap[ID].SetSSI("Period", it->GetSSI("Period"));
			//}

			////now: update coordinate for stationList station
			//*it = stationMap[ID];

			CTPeriod period = String2Period(it->GetSSI("Period"));
			string internalID = it->GetSSI("InternalID");
			CLocationVector::iterator it2 = stations.FindBySSI("InternalID", internalID, false);
			if (it2 == stations.end() || it2->m_lat == -999)
			{
				__int64 internalID64 = ToInt64(internalID);

				stations.push_back(*it);
				ERMsg msgTmp = UpdateCoordinate(pConnection, internalID64, period.End().GetYear(), period.End().GetMonth(), period.End().GetDay(), stations.back());
				if (msgTmp)
					it2 = stations.FindBySSI("InternalID", internalID, false);
				else
					callback.AddMessage(msgTmp);
			}
			else
			{
				it2->SetSSI("Period", it->GetSSI("Period"));
			}

			//now: update coordinate for station that are not init
			*it = *it2;

			msg += callback.StepIt();
		}

		pConnection->Close();
		pSession->Close();
		callback.PopTask();

		return msg;
	}


	//because station coordinate in the csv file is lesser accurate than in the web page
	//we have to update coordinate from web page
	ERMsg CUIEnvCanHourly::UpdateCoordinate(CHttpConnectionPtr& pConnection, __int64 ID, int year, size_t m, size_t d, CLocation& station)const
	{
		static const char webPageDataFormat[] =
		{
			"climate_data/hourly_data_e.html?"
			"timeframe=1&"
			"StationID=%ld&"
			"Year=%d&"
			"Month=%d&"
			"Day=%d"
		};

		ERMsg msg;

		string URL = FormatA(webPageDataFormat, ID, year, m + 1, d + 1);

		string source;
		msg = GetPageText(pConnection, URL, source);
		if (msg)
		{
			string::size_type posBegin = source.find("latitude");
			string::size_type posEnd = 0;

			if (posBegin != string::npos)
			{
				//find latitude						   
				string latitude = FindString(source, "labelledby=\"latitude\">", "</div>", posBegin, posEnd);
				latitude = CleanString(latitude);

				//find longitude
				string longitude = FindString(source, "labelledby=\"longitude\">", "</div>", posBegin, posEnd);
				longitude = CleanString(longitude);
				longitude.insert(longitude.begin(), '-');

				//find elevation
				string elevation = FindString(source, "labelledby=\"elevation\">", "</div>", posBegin, posEnd);
				elevation = CleanString(elevation);

				string ClimateID = FindString(source, "labelledby=\"climateid\">", "</div>", posBegin, posEnd);
				ClimateID = CleanString(ClimateID);

				string WMOID = FindString(source, "labelledby=\"wmoid\">", "</div>", posBegin, posEnd);
				WMOID = CleanString(WMOID);

				string TCID = FindString(source, "labelledby=\"tcid\">", "</div>", posBegin, posEnd);
				TCID = CleanString(TCID);

				if (!latitude.empty() &&
					!longitude.empty() &&
					!elevation.empty())
				{
					station.m_lat = GetCoordinate(latitude);
					station.m_lon = GetCoordinate(longitude);
					station.m_elev = ToDouble(elevation);

					station.m_ID = ClimateID;
					if (!WMOID.empty())
						station.SetSSI("WorldMeteorologicalOrganizationID", WMOID);
					if (!TCID.empty())
						station.SetSSI("TransportCanadaID", TCID);

					ASSERT(station.IsValid());
				}
				else
				{
					msg.ajoute("EnvCan Hourly bad coordinate: " + URL);
				}
			}
		}

		return msg;
	}

	//******************************************************
	ERMsg CUIEnvCanHourly::CopyStationDataPage(CHttpConnectionPtr& pConnection, __int64 ID, int year, size_t m, const string& filePath)
	{
		ERMsg msg;

		static const char pageDataFormat[] =
		{
			"climate_data/bulk_data_e.html?"
			"format=csv&"
			"stationID=%ld&"
			"Year=%d&"
			"Month=%d&"
			"Day=1&"
			"timeframe=1&"
			"submit=Download+Data"
		};

		string URL = FormatA(pageDataFormat, ID, year, m + 1);

		string source;
		msg = GetPageText(pConnection, URL, source);
		if (msg)
		{
			ofStream file;
			msg = file.open(filePath);

			if (msg)
			{
				string::size_type posBegin = source.find("\"Date/Time\"", 0);
				//ASSERT(posBegin != string::npos);
				if (posBegin != string::npos)
				{
					file << source.substr(posBegin);
					file.close();
				}
				else
				{
					msg.ajoute("error loading page ID = " + ToString(ID) + ", year = " + ToString(year) + ", month = " + ToString(m + 1));
				}

			}
		}

		return msg;
	}


	ERMsg CUIEnvCanHourly::CleanStationList(CLocationVector& stationList, CCallback& callback)const
	{
		ERMsg msg;

		//CGeoRect boundingBox;

		//if (!boundingBox.IsRectEmpty() && boundingBox != DEFAULT_BOUDINGBOX)
		//{
		//	callback.SetCurrentDescription(GetString(IDS_CLEAN_LIST));
		//	callback.SetNbStep(stationList.size());

		//	for (CLocationVector::iterator it = stationList.begin(); it != stationList.end() && msg; )
		//	{
		//		if( boundingBox.PtInRect(*it))
		//			it++;
		//		else
		//			it = stationList.erase(it);

		//		msg += callback.StepIt();
		//	}
		//}

		return msg;
	}

	//***********************************************************************************************************


	std::bitset<CUIEnvCanHourly::NB_NETWORKS> CUIEnvCanHourly::GetNetWork()const
	{
		std::bitset<NB_NETWORKS> network;

		StringVector str(Get(NETWORK), "|;,");
		if (str.empty())
		{
			network.set();
		}
		else
		{
			StringVector net("HIST|SWOB", "|");
			for (size_t i = 0; i < str.size(); i++)
			{
				size_t n = net.Find(str[i], false);
				if (n < network.size())
					network.set(n);
			}
		}

		return network;
	}

	void TestTimeZone()
	{

	}


	ERMsg CUIEnvCanHourly::Execute(CCallback& callback)
	{
		ERMsg msg;

		TestTimeZone();


		std::bitset<NB_NETWORKS> network = GetNetWork();

		for (size_t n = 0; n < network.size(); n++)
		{
			if (network.test(n))
			{
				switch (n)
				{
				case N_HISTORICAL:	msg = ExecuteHistorical(callback); break;
				case N_SWOB:		msg = ExecuteSWOB(callback); break;
				default:	ASSERT(false);
				}
			}
		}

		return msg;
	}

	ERMsg CUIEnvCanHourly::ExecuteHistorical(CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);
		CreateMultipleDir(workingDir);

		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(workingDir, 1);
		callback.AddMessage(GetString(IDS_UPDATE_FROM));
		callback.AddMessage(SERVER_NAME, 1);
		callback.AddMessage("");


		//Get the stations list
		CLocationVector stationList;
		//local station list
		if (FileExists(GetStationListFilePath()))
			msg = m_stations.Load(GetStationListFilePath());

		//remote station list
		if (msg)
			msg = DownloadStationList(stationList, callback);

		if (msg)
			msg = UpdateStationList(stationList, m_stations, callback);

		//save event if they append an error...
		msg += m_stations.Save(GetStationListFilePath());

		if (msg)
			msg = CleanStationList(stationList, callback);

		if (!msg)
			return msg;



		callback.PushTask("Download EnvCan hourly data (" + ToString(stationList.size()) + " stations)", stationList.size());
		callback.AddMessage("Download EnvCan hourly data (" + ToString(stationList.size()) + " stations)");

		size_t nbFiles = 0;
		int nbRun = 0;
		size_t curI = 0;
		while (curI < stationList.size() && msg)
		{
			nbRun++;

			CInternetSessionPtr pSession;
			CHttpConnectionPtr pConnection;

			msg = GetHttpConnection(SERVER_NAME, pConnection, pSession);

			if (msg)
			{


				for (size_t i = curI; i < stationList.size() && msg; i++)
				{
					msg = DownloadStation(pConnection, stationList[i], callback);
					if (msg)
					{
						curI++;
						nbRun = 0;
						nbFiles++;
						msg += callback.StepIt();
					}
				}



				//if an error occur: try again
				if (!msg && !callback.GetUserCancel())
				{
					if (nbRun < 5)
					{
						callback.AddMessage(msg);
						msg = ERMsg();


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
			}
		}

		callback.AddMessage(GetString(IDS_NB_FILES_DOWNLOADED) + ToString(nbFiles), 1);
		callback.PopTask();

		return msg;
	}


	ERMsg CUIEnvCanHourly::DownloadStation(CHttpConnectionPtr& pConnection, const CLocation& station, CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);

		size_t nbFilesToDownload = 0;
		CProvinceSelection selection(Get(PROVINCE));
		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYear = lastYear - firstYear + 1;
		CTRef now = CTRef::GetCurrentTRef(CTM::MONTHLY);

		if (nbYear > 5)
			callback.PushTask("Get number of files to update for " + station.m_name, nbYear * 12, 1);

		vector< array<bool, 12> > bNeedDownload;
		bNeedDownload.resize(nbYear);

		for (size_t y = 0; y < nbYear&&msg; y++)
		{
			int year = firstYear + int(y);

			for (size_t m = 0; m < 12 && msg; m++)
			{
				if (CTRef(year, m) <= now)
				{
					CTPeriod period1 = String2Period(station.GetSSI("Period"));
					CTPeriod period2(CTRef(year, JANUARY, FIRST_DAY), CTRef(year, DECEMBER, LAST_DAY));
					if (period1.IsIntersect(period2))
					{
						string outputPath = GetOutputFilePath(N_HISTORICAL, station.GetSSI("Province"), year, m, station.GetSSI("InternalID"));
						bNeedDownload[y][m] = NeedDownload(outputPath, station, year, m);
						nbFilesToDownload += bNeedDownload[y][m] ? 1 : 0;
					}
				}

				if (nbYear > 5)
					msg += callback.StepIt();
			}
		}

		if (nbYear > 5)
			callback.PopTask();


		//
		if (nbFilesToDownload > 0)
		{
			if (nbFilesToDownload > 60)
				callback.PushTask("Update files for " + station.m_name + " (" + ToString(nbFilesToDownload) + ")", nbFilesToDownload);

			for (size_t y = 0; y < nbYear&&msg; y++)
			{
				int year = firstYear + int(y);

				for (size_t m = 0; m < 12 && msg; m++)
				{
					if (bNeedDownload[y][m])
					{
						string internalID = station.GetSSI("InternalID");
						string filePath = GetOutputFilePath(N_HISTORICAL, station.GetSSI("Province"), year, m, internalID);
						CreateMultipleDir(GetPath(filePath));

						TRY
							msg = CopyStationDataPage(pConnection, ToLong(internalID), year, m, filePath);
						CATCH_ALL(e)
							msg = UtilWin::SYGetMessage(*e);
						END_CATCH_ALL

							msg += callback.StepIt(nbFilesToDownload > 60 ? 1 : 0);
					}
				}
			}

			if (nbFilesToDownload > 60)
				callback.PopTask();
		}

		return msg;
	}


	bool CUIEnvCanHourly::NeedDownload(const string& filePath, const CLocation& info, int year, size_t m)const
	{
		bool bDownload = true;


		CFileStamp fileStamp(filePath);
		CTime lastUpdate = fileStamp.m_time;
		if (lastUpdate.GetTime() > 0)
		{
			int nbDays = GetNbDay(lastUpdate) - GetNbDay(year, m, 0);

			if (nbDays > 62)//update until 2 months after
				bDownload = false;
		}

		return bDownload;
	}

	//[vide] = Aucune donnée disponible 
	//M = Données manquantes 
	//E = Valeur estimée 
	//A = Valeur accumulée 
	//C = Précipitation, quantité incertaine 
	//L = des précipitation peuvent avoir eu lieu 
	//F = Valeur accumulée et estimée 
	//N = Température manquante, mais > 0 
	//Y = Température manquante, mais < 0 
	//S = À plus d'une reprise 
	//T = Trace 
	//* = La valeur affichée est basée sur des données incomplètes. 
	//= Ces données journalières n'ont subit qu'un contrôle de qualité préliminaire 

	//ERMsg CUIEnvCanHourly::PreProcess(CCallback& callback)
	//{
	//	ERMsg msg;
	//	msg = m_stations.Load(GetStationListFilePath());
	//	return msg;
	//}

	string CUIEnvCanHourly::GetOutputFilePath(size_t n, const string& prov, int year, size_t m, const string& ID)const
	{
		ASSERT(prov.length() < 4);
		std::stringstream month;
		month << std::setfill('0') << std::setw(2) << (m + 1);

		string filePath;
		if (n == N_HISTORICAL)
			filePath = GetDir(WORKING_DIR) + prov + "\\" + ToString(year) + "\\" + month.str() + "\\" + ID + ".csv";
		else if (n == N_SWOB)
			filePath = GetDir(WORKING_DIR) + "SWOB-ML\\" + ToString(year) + "\\" + month.str() + "\\" + ID + ".csv";

		return filePath;
	}


	ERMsg CUIEnvCanHourly::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;

		m_stations.clear();
		m_SWOBstations.clear();

		string workingDir = GetDir(WORKING_DIR);
		std::bitset<NB_NETWORKS> network = GetNetWork();
		CProvinceSelection selection(Get(PROVINCE));
		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);

		set<string> tmpList;

		for (size_t n = 0; n < network.size(); n++)
		{
			if (network[n])
			{
				switch (n)
				{
				case N_HISTORICAL:
				{
					msg += m_stations.Load(GetStationListFilePath());
					for (CLocationVector::const_iterator it = m_stations.begin(); it != m_stations.end() && msg; it++)
					{
						const CLocation& station = *it;
						//CTPeriod p = String2Period(station.GetSSI("Period"));
						//if (p.Begin().GetYear() <= lastYear && p.End().GetYear() >= firstYear) //Bug correction: By RSA 02/2012
						CTPeriod period1 = String2Period(station.GetSSI("Period"));
						CTPeriod period2(CTRef(firstYear, JANUARY, DAY_01), CTRef(lastYear, DECEMBER, DAY_31));
						if (period1.IsIntersect(period2))
						{
							string prov = station.GetSSI("Province");
							size_t p = selection.GetProvince(prov);
							ASSERT(p != -1);


							if (selection.none() || selection[p])
							{
								//string stationStr = ToString(n) + "\\" + prov + "\\" + ToString(station.GetSSI("InternalID"));
								//stationList.push_back(stationStr);

								tmpList.insert(station.m_ID);
							}
						}

						msg += callback.StepIt(0);
					}

					break;
				}

				case N_SWOB:
				{
					msg += m_SWOBstations.Load(GetSWOBStationsListFilePath());
					string filePath = workingDir + "SWOB-ML\\MissingStations.csv";
					CLocationVector missingLoc;
					if (missingLoc.Load(filePath))
						m_SWOBstations.insert(m_SWOBstations.end(), missingLoc.begin(), missingLoc.end());


					for (CLocationVector::const_iterator it = m_SWOBstations.begin(); it != m_SWOBstations.end() && msg; it++)
					{
						string prov = it->GetSSI("Province");
						if (selection.at(prov))
						{
							//string stationStr = ToString(n) + "\\" + prov + "\\" + ToString(it->m_ID);
							//stationList.push_back(stationStr);

							tmpList.insert(it->m_ID);
						}
					}
					break;
				}
				default:	ASSERT(false);
				}
			}
		}

		stationList.insert(stationList.end(), tmpList.begin(), tmpList.end());

		return msg;
	}

	std::bitset<CUIEnvCanHourly::NB_NETWORKS> CUIEnvCanHourly::GetStationInformation(const string& ID, CLocation& station)const
	{
		std::bitset<CUIEnvCanHourly::NB_NETWORKS> network = GetNetWork();

		if (network[N_HISTORICAL])
		{
			//CLocationMap::const_iterator it = m_stations.find(ID);
			size_t i = m_stations.FindByID(ID);
			if (i < m_stations.size())
				station = m_stations[i];
			else
				network.reset(N_HISTORICAL);
		}

		if (network[N_SWOB])
		{
			size_t pos = m_SWOBstations.FindByID(ID);
			if (pos != NOT_INIT)
			{
				network.set();
				if (station.IsInit())
					station.SetSSI("ICAO", m_SWOBstations[pos].GetSSI("ICAO"));
				else
					station = m_SWOBstations[pos];
			}
			else
			{
				network.reset(N_SWOB);
			}
		}


		return network;
	}

	static std::string TraitFileName(std::string name)
	{
		std::replace(name.begin(), name.end(), '(', ' ');
		std::replace(name.begin(), name.end(), ')', ' ');
		std::replace(name.begin(), name.end(), ',', '-');
		std::replace(name.begin(), name.end(), ';', '-');


		Trim(name);
		ReplaceString(name, "  ", " ");//replace double white space by only one white space
		UppercaseFirstLetter(name);
		ReplaceString(name, "St ", "St-");
		ReplaceString(name, "Ste ", "Ste-");
		ReplaceString(name, "St. ", "St-");
		ReplaceString(name, "Ste. ", "Ste-");

		return name;
	}

	ERMsg CUIEnvCanHourly::GetWeatherStation(const string& ID, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;

		//CProvinceSelection selection(Get(PROVINCE));
		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = size_t(lastYear - firstYear + 1);

		std::bitset<CUIEnvCanHourly::NB_NETWORKS> network = GetStationInformation(ID, station);

		string prov = station.GetSSI("Province");

		station.m_name = TraitFileName(station.m_name) + " (" + prov + ")";
		station.m_ID += "H";//add a "H" for hourly data

		station.SetSSI("FirstYear", "");
		station.SetSSI("LastYear", "");

		station.SetHourly(true);
		station.CreateYears(firstYear, nbYears);

		if (network[N_HISTORICAL])
		{
			string internalID = station.GetSSI("InternalID");
			//now extract data 
			for (size_t y = 0; y < nbYears&&msg; y++)
			{
				int year = firstYear + int(y);

				for (size_t m = 0; m < 12 && msg; m++)
				{
					string filePath = GetOutputFilePath(N_HISTORICAL, prov, year, m, internalID);
					if (FileExists(filePath))
						msg = ReadData(filePath, TM, station[year], callback);

					msg += callback.StepIt(0);
				}
			}
		}

		if (network[N_SWOB])
		{
			//cctz::time_zone zone;
//			CTimeZones::GetZone(station, zone);

			string ICAO_ID = station.GetSSI("ICAO");

			for (size_t y = 0; y < nbYears&&msg; y++)
			{
				int year = firstYear + int(y);

				for (size_t m = 0; m < 12 && msg; m++)
				{
					size_t size1 = sizeof(string);
					size_t size2 = sizeof(SWOBDataHour);
					size_t size3 = sizeof(SWOBData);
					string filePath = GetOutputFilePath(N_SWOB, station.GetSSI("Province"), year, m, ICAO_ID);
					if (FileExists(filePath))
						msg = ReadSWOBData(filePath, TM, station, callback);

					msg += callback.StepIt(0);
				}
			}

			CWAllVariables vars;
			vars.reset(H_ADD1);
			vars.reset(H_ADD2);
			station.CleanUnusedVariable(vars);
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



	int GetHour(const string& line)
	{
		string::size_type pos = 0;
		return ToInt(Tokenize(line, ":", pos));
	}



	ERMsg CUIEnvCanHourly::ReadData(const string& filePath, CTM TM, CYear& data, CCallback& callback)const
	{
		ERMsg msg;

		//int nbYear = m_lastYear-m_firstYear+1;

		enum { DATE_TIME, H_YEAR, H_MONTH, H_DAY, TIMEVAL, DATA_QUALITY, TEMPERATURE, TEMPERATURE_FLAG, DEWPOINT, DEWPOINT_FLAG, RELHUM, RELHUM_FLAG, WIND_DIR, WIND_DIR_FLAG, WIND_SPEED, WIND_SPEED_FLAG, VISIBILITY, VISIBILITY_FLAG, PRESSURE, PRESSURE_FLAG, HMDX, HMDX_FLAG, WIND_CHILL, WIND_CHILL_FLAG, WEATHER_INFO, NB_INPUT_HOURLY_COLUMN };

		const int COL_POS[NB_VAR_H] = { -1, TEMPERATURE, -1, -1, DEWPOINT, RELHUM, WIND_SPEED, WIND_DIR, -1, PRESSURE, -1, -1, -1, -1, -1 };
		const double FACTOR[NB_VAR_H] = { 0, 1, 0, 0, 1, 1, 1, 10, 0, 10, 0, 0, 0, 0, 0 };



		//now extact data 
		ifStream file;
		msg = file.open(filePath);

		if (msg)
		{
			for (CSVIterator loop(file, ",", true, true); loop != CSVIterator() && msg; ++loop)
			{
				//new file don't have the DATA_QUALITY flag
				__int64 fix = (loop.Header().size() == NB_INPUT_HOURLY_COLUMN) ? 0 : -1;
				if (loop.Header().size() != (NB_INPUT_HOURLY_COLUMN + fix))
				{
					msg.ajoute("Numbert of columns in Env Can hourly file" + to_string(loop.Header().size()) + "is not the number expected " + to_string(NB_INPUT_HOURLY_COLUMN + fix));
					msg.ajoute(filePath);
					return msg;
				}

				if (loop->size() == (NB_INPUT_HOURLY_COLUMN + fix))
				{
					int year = ToInt((*loop)[H_YEAR]);
					int month = ToInt((*loop)[H_MONTH]) - 1;
					int day = ToInt((*loop)[H_DAY]) - 1;
					int hour = GetHour((*loop)[TIMEVAL]);

					ASSERT(month >= 0 && month < 12);
					ASSERT(day >= 0 && day < GetNbDayPerMonth(year, month));
					ASSERT(hour >= 0 && hour < 24);

					CTRef TRef(year, month, day, hour);

					bool bValid[NB_VAR_H] = { 0 };
					bValid[H_TAIR2] = ((*loop)[TEMPERATURE_FLAG + fix].empty() || (*loop)[TEMPERATURE_FLAG + fix] == "E") && !(*loop)[TEMPERATURE + fix].empty();
					bValid[H_PRES] = (*loop)[PRESSURE_FLAG + fix].empty() && !(*loop)[PRESSURE + fix].empty();
					bValid[H_TDEW] = ((*loop)[DEWPOINT_FLAG + fix].empty() || (*loop)[DEWPOINT_FLAG + fix] != "M") && !(*loop)[DEWPOINT + fix].empty();
					bValid[H_RELH] = ((*loop)[RELHUM_FLAG + fix].empty() || (*loop)[RELHUM_FLAG + fix] != "M") && !(*loop)[RELHUM + fix].empty();
					bValid[H_WNDS] = ((*loop)[WIND_SPEED_FLAG + fix].empty() || (*loop)[WIND_SPEED_FLAG + fix] != "E") && !(*loop)[WIND_SPEED + fix].empty();
					bValid[H_WNDD] = ((*loop)[WIND_DIR_FLAG + fix].empty() || (*loop)[WIND_DIR_FLAG + fix] == "E") && !(*loop)[WIND_DIR + fix].empty();

					for (TVarH v = H_FIRST_VAR; v < NB_VAR_H; v++)
					{
						if (bValid[v])
						{
							if (COL_POS[v] >= 0)
							{
								double value = ToDouble((*loop)[COL_POS[v] + fix])*FACTOR[v];
								data[TRef].SetStat(v, value);
							}
						}
					}

					if (bValid[H_TAIR2] && (!bValid[H_TDEW] || !bValid[H_RELH]))
					{
						double Tair = ToDouble((*loop)[COL_POS[H_TAIR2]])*FACTOR[H_TAIR2];
						double Tdew = ToDouble((*loop)[COL_POS[H_TDEW]])*FACTOR[H_TDEW];
						double Hr = ToDouble((*loop)[COL_POS[H_RELH]])*FACTOR[H_RELH];
						if (Hr == -999 && Tdew != -999)
							data[TRef].SetStat(H_RELH, Td2Hr(Tair, Tdew));
						else if (Tdew == -999 && Hr != -999)
							data[TRef].SetStat(H_TDEW, Hr2Td(Tair, Hr));
					}
				}

				msg += callback.StepIt(0);
			}//for all line
		}//if load 

		return msg;
	}

	//**************************************************************************************************************************

	const char* CUIEnvCanHourly::SWOB_VARIABLE_NAME[NB_SWOB_VARIABLES] =
	{ "stn_pres", "air_temp", "rel_hum", "max_wnd_spd_10m_pst1hr", "max_wnd_spd_10m_pst1hr_tm",
	"wnd_dir_10m_pst1hr_max_spd", "avg_wnd_spd_10m_pst1hr", "avg_wnd_dir_10m_pst1hr", "avg_air_temp_pst1hr",
	"max_air_temp_pst1hr", "max_rel_hum_pst1hr", "min_air_temp_pst1hr", "min_rel_hum_pst1hr", "pcpn_amt_pst1hr",
	"snw_dpth", "rnfl_amt_pst1hr", "max_vis_pst1hr", "dwpt_temp", "tot_globl_solr_radn_pst1hr",
	"min_air_temp_pst24hrs", "max_air_temp_pst24hrs", "pcpn_amt_pst24hrs"
	};

	const char* CUIEnvCanHourly::DEFAULT_UNIT[NB_SWOB_VARIABLES] =
	{
		"hPa", "°C", "%", "km/h", "hhmm",
		"°", "km/h", "°", "°C",
		"°C", "%", "°C", "%", "mm",
		"cm", "mm", "km", "°C", "W/m²",
		"°C", "°C", "mm"
	};

	const TVarH CUIEnvCanHourly::VARIABLE_TYPE[NB_SWOB_VARIABLES] =
	{
		H_PRES, H_SKIP, H_SKIP, H_SKIP, H_SKIP,
		H_SKIP, H_WNDS, H_WNDD, H_TAIR2,
		H_TMAX2, H_ADD2, H_TMIN2, H_ADD1, H_PRCP,
		H_SNDH, H_SKIP, H_SKIP, H_TDEW, H_SRAD2,
		H_SKIP, H_SKIP, H_SKIP
	};


	ERMsg CUIEnvCanHourly::ExecuteSWOB(CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);


		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(workingDir + "SWOB_XML\\", 1);
		callback.AddMessage(GetString(IDS_UPDATE_FROM));
		callback.AddMessage("dd.weatheroffice.gc.ca", 1);
		callback.AddMessage("");

		string infoFilePath = GetSWOBStationsListFilePath();
		if (!FileExists(infoFilePath))
			msg = UpdateSWOBLocations(callback);

		CLocationVector locations;
		if (msg)
		{
			msg = locations.Load(infoFilePath);
			if (msg)
			{
				string filePath = workingDir + "SWOB-ML\\MissingStations.csv";
				CLocationVector missingLoc;
				if (missingLoc.Load(filePath))
					locations.insert(locations.end(), missingLoc.begin(), missingLoc.end());

				map<string, CFileInfoVector> fileList;
				set<string> missingID;
				msg = GetSWOBList(locations, fileList, missingID, callback);

				if (msg)
				{
					if (!missingID.empty())
						msg = UpdateMissingLocation(locations, fileList, missingID, callback);


					if (msg)
						msg = DownloadSWOB(locations, fileList, callback);
				}
			}
		}

		return msg;
	}

	CTRef CUIEnvCanHourly::GetSWOBTRef(const string & fileName)
	{
		StringVector info(fileName, "-");
		int year = WBSF::as<int>(info[0]);
		size_t m = WBSF::as<size_t>(info[1]) - 1;
		size_t d = WBSF::as<size_t>(info[2]) - 1;
		size_t h = WBSF::as<size_t>(info[3].substr(0, 2));

		CTRef TRef(year, m, d, h);
		ASSERT(TRef.IsValid());

		return TRef;
	}

	string CUIEnvCanHourly::GetSWOBStationsListFilePath()const
	{
		string workingDir = GetDir(WORKING_DIR);
		return workingDir + "SWOB-ML\\swob-xml_station_list.csv";
	}

	ERMsg CUIEnvCanHourly::UpdateSWOBLocations(CCallback& callback)
	{
		ERMsg msg;

		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;

		msg = GetHttpConnection("dd.weatheroffice.gc.ca", pConnection, pSession);
		if (!msg)
			return msg;


		string infoFilePath = GetSWOBStationsListFilePath();
		WBSF::CreateMultipleDir(GetPath(infoFilePath));


		string infoFilePathTmp = infoFilePath;
		SetFileTitle(infoFilePathTmp, GetFileTitle(infoFilePath) + "_tmp");



		msg = CopyFile(pConnection, "/observations/doc/swob-xml_station_list.csv", infoFilePathTmp, INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_DONT_CACHE);
		pConnection->Close();
		pSession->Close();


		//convert to compatible location file
		ifStream file;
		if (msg)
			msg = file.open(infoFilePathTmp);

		if (msg)
		{
			enum TColumns { C_ID_IATA, C_FR_NAME, C_EN_NAME, C_PROVINCE, C_TYPE, C_LATITUDE, C_LONGITUDE, C_ELEVATION, C_ID_ICAO, C_ID_WMO, C_ID_MSC, C_ID_DST_TIMEZONE, C_ID_STD_TIMEZONE, NB_COLUMNS };
			static const char* OUTPUT_NAME[NB_COLUMNS] = { "IATA", "NameFr", "", "Province", "Type", "", "", "", "ICAO", "WMO", "", "DSTTimeZone", "STDTimeZone" };

			CLocationVector locations;
			for (CSVIterator loop(file, ",", true, true); loop != CSVIterator() && msg; ++loop)
			{
				CLocation location;
				for (size_t c = 0; c < NB_COLUMNS&&msg; c++)
				{
					switch (c)
					{
					case C_ID_MSC: location.m_ID = (*loop)[C_ID_MSC]; break;
					case C_EN_NAME:
					{

						static const string CLEAN = "CDA|RCS|Auto|BC|AB|SK|MN|ON|QC|NB|NS|PI|NL|NFLD|NF|NU|P.E.I.|N.S.|Automatic|NOVA SCOTIA|Quebec|Climat|Climate|NU.|AGDM|AGCM|ON.|(AUT)|NWT|ONT.|QUE.|CS|A|NFLD.|-NFLD|SASK.|PEI|MAN.|ONT|NL.|MAN.|N.B.|N.W.T.|ONTARIO|MAN|CCG|CR10|CR3000|MB|(CR3000)|71482|23X|CR10x_V24|AAFC|AUT|71484|AEC|SE|CR23X|MCDC|RBG";
						StringVector clean(CLEAN, "|");
						StringVector name((*loop)[C_EN_NAME], " ,/\\");
						for (StringVector::iterator it = name.begin(); it != name.end();)
						{
							if (clean.Find(*it, false) == NOT_INIT)
								it++;
							else
								it = name.erase(it);
						}

						ostringstream s;
						std::copy(name.begin(), name.end(), ostream_iterator<string>(s, " "));

						//Regina Upgrade - May 2008 
						location.m_name = WBSF::PurgeFileName(WBSF::TrimConst(WBSF::UppercaseFirstLetter(s.str())));
						break;
					}
					case C_LATITUDE: location.m_lat = WBSF::as<double>((*loop)[C_LATITUDE]); break;
					case C_LONGITUDE:location.m_lon = WBSF::as<double>((*loop)[C_LONGITUDE]); break;
					case C_ELEVATION:location.m_elev = WBSF::as<double>((*loop)[C_ELEVATION]); break;
					case C_FR_NAME: location.SetSSI(OUTPUT_NAME[c], WBSF::PurgeFileName((*loop)[c])); break;
					case C_ID_IATA:
					case C_PROVINCE:
					case C_TYPE:
					case C_ID_WMO:
					case C_ID_ICAO:
					case C_ID_DST_TIMEZONE:
					case C_ID_STD_TIMEZONE: location.SetSSI(OUTPUT_NAME[c], (*loop)[c]); break;
					default: ASSERT(false);
					};

					msg += callback.StepIt(0);
				}//for all colums

				if (location.m_ID == "9052008")
					location.SetSSI("Province", "ON");
				if (location.m_ID == "7032682")
					location.SetSSI("Province", "QC");
				if (location.m_ID == "2203913")
					location.SetSSI("Province", "NT");


				if (location.m_name.empty())
					location.m_name = location.GetSSI("NameFr");

				locations.push_back(location);
			}

			msg = locations.Save(infoFilePath);
		}


		return msg;

	}

	ERMsg CUIEnvCanHourly::GetSWOBList(const CLocationVector& locations, map<string, CFileInfoVector>& fileList, std::set<std::string>& missingID, CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);
		CProvinceSelection selection(Get(PROVINCE));


		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;


		msg = GetHttpConnection("dd.weatheroffice.gc.ca", pConnection, pSession);

		if (!msg)
			return msg;



		string lastUpdateFilePath = workingDir + "SWOB-ML\\LastUpdate.csv";
		map<string, CTRef> lastUpdate;

		ifStream ifile;
		if (FileExists(lastUpdateFilePath))
			msg = ifile.open(lastUpdateFilePath);

		if (!msg)
			return msg;


		for (CSVIterator loop(ifile, ",", true); loop != CSVIterator(); ++loop)
		{
			if (loop->size() == 2)
			{
				string ID = (*loop)[0];
				CTRef TRef;
				TRef.FromFormatedString((*loop)[1], "%Y-%m-%d-%H");
				lastUpdate[ID] = TRef;
			}
		}

		ifile.close();


		//observations/swob-ml/20170622/CACM/2017-06-22-0300-CACM-AUTO-swob.xml
		CFileInfoVector dir1;
		msg = FindDirectories(pConnection, "/observations/swob-ml/", dir1);// date

		pConnection->Close();
		pSession->Close();

		CFileInfoVector dir2;
		if (msg)
		{
			callback.PushTask("Get days from: /observations/swob-ml/ (" + to_string(dir1.size()) + " days)", dir1.size());


			size_t nbTry = 0;
			CFileInfoVector::const_iterator it1 = dir1.begin();
			while (it1 != dir1.end() && msg)
			{
				nbTry++;

				msg = GetHttpConnection("dd.weatheroffice.gc.ca", pConnection, pSession);

				if (msg)
				{
					try
					{
						while ( it1 != dir1.end() && msg )
						{
							string dirName = GetLastDirName(it1->m_filePath);
							if (dirName != "latest")
							{
								CFileInfoVector dirTmp;
								msg = FindDirectories(pConnection, it1->m_filePath, dirTmp);//stations
								dir2.insert(dir2.end(), dirTmp.begin(), dirTmp.end());
							}

							msg += callback.StepIt();
							nbTry = 0;
							it1++;
						}//for all dir
					}
					catch (CException* e)
					{
						if (nbTry < 5)
						{
							callback.AddMessage(UtilWin::SYGetMessage(*e));
							callback.PushTask("Waiting 30 seconds for server...", 600);
							for (size_t i = 0; i < 600 && msg; i++)
							{
								Sleep(50);//wait 50 milisec
								msg += callback.StepIt();
							}
							callback.PopTask();

						}
						else
						{
							msg = UtilWin::SYGetMessage(*e);
						}
					}//catch


					pConnection->Close();
					pSession->Close();
				}//if msg
			}//while nbTry

			callback.PopTask();
		}//if msg

		if (msg)
		{
			callback.PushTask("Get stations list from: /observations/swob-ml/ (" + to_string(dir2.size()) + " days)", dir2.size());

			size_t nbTry = 0;
			CFileInfoVector::const_iterator it2 = dir2.begin();
			while (it2 != dir1.end() && msg)
			{
				nbTry++;

				msg = GetHttpConnection("dd.weatheroffice.gc.ca", pConnection, pSession);

				if (msg)
				{
					try
					{
						while ( it2 != dir2.end() && msg )//for all station
						{
							string dirName = GetLastDirName(GetPath(it2->m_filePath));
							int year = WBSF::as<int>(dirName.substr(0, 4));
							size_t m = WBSF::as<size_t>(dirName.substr(4, 2)) - 1;
							size_t d = WBSF::as<size_t>(dirName.substr(6, 2)) - 1;

							CTRef TRef(year, m, d);


							string ICAOID = GetLastDirName(it2->m_filePath);
							CLocationVector::const_iterator itMissing = locations.FindBySSI("ICAO", ICAOID, false);

							string prov;
							if (itMissing != locations.end())
								prov = itMissing->GetSSI("Province");
							else
								missingID.insert(ICAOID);


							if (prov.empty() || selection.at(prov))
							{
								auto findIt = lastUpdate.find(ICAOID);
								if (findIt == lastUpdate.end() || TRef >= findIt->second.as(CTM::DAILY))
								{
									CFileInfoVector fileListTmp;
									msg = FindFiles(pConnection, it2->m_filePath + "*.xml", fileListTmp);
									for (CFileInfoVector::iterator it = fileListTmp.begin(); it != fileListTmp.end() && msg; it++)
									{
										string fileName = GetFileName(it->m_filePath);
										size_t mm = WBSF::as<size_t>(fileName.substr(13, 2));

										if (mm == 0)//take only hourly value (avoid download minute and 10 minutes files)
										{
											CTRef TRef = GetSWOBTRef(fileName);
											if (findIt == lastUpdate.end() || TRef > findIt->second)
												fileList[ICAOID].push_back(*it);
										}
										msg += callback.StepIt(0);
									}//for all files
								}


							}//if it's selected province

							msg += callback.StepIt();
							nbTry = 0;
							it2++;

						}//for all stations
					}
					catch (CException* e)
					{
						if (nbTry < 5)
						{
							callback.AddMessage(UtilWin::SYGetMessage(*e));
							callback.PushTask("Waiting 30 seconds for server...", 600);
							for (size_t i = 0; i < 600 && msg; i++)
							{
								Sleep(50);//wait 50 milisec
								msg += callback.StepIt();
							}
							callback.PopTask();

						}
						else
						{
							msg = UtilWin::SYGetMessage(*e);
						}
					}


					pConnection->Close();
					pSession->Close();
				}
			}

			callback.PopTask();
		}

		return msg;
	}

	ERMsg CUIEnvCanHourly::UpdateMissingLocation(CLocationVector& locations, const std::map<std::string, CFileInfoVector>& fileList, std::set<std::string>& missingID, CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);
		callback.PushTask("Get missing SWOB-ML location (" + ToString(missingID.size()) + " stations)", missingID.size());

		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;

		msg = GetHttpConnection("dd.weatheroffice.gc.ca", pConnection, pSession);

		if (!msg)
			return msg;


		string filePath = workingDir + "SWOB-ML\\MissingStations.csv";
		CLocationVector missingLoc;

		if (FileExists(filePath))
			msg = missingLoc.Load(filePath);

		try
		{


			callback.AddMessage("Missing stations information: ");
			for (set<string>::const_iterator it = missingID.begin(); it != missingID.end() && msg; it++)
			{
				callback.AddMessage(*it, 2);

				map<string, CFileInfoVector>::const_iterator it1 = fileList.find(*it);
				ASSERT(it1 != fileList.end());

				if (it1 != fileList.end())
				{
					string source;
					msg = GetPageText(pConnection, it1->second.front().m_filePath, source, false, INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_DONT_CACHE);


					if (msg)
					{
						WBSF::ReplaceString(source, "'", " ");

						CLocation location;
						msg = GetSWOBLocation(source, location);
						if (msg)
						{
							location.SetSSI("ICAO", *it);
							locations.push_back(location);
							missingLoc.push_back(location);
						}
					}
				}

				ASSERT(locations.FindBySSI("ICAO", *it, false) != locations.end());

				msg += callback.StepIt();
			}
		}
		catch (...)
		{

		}

		missingLoc.Save(filePath);
		callback.PopTask();


		pConnection->Close();
		pSession->Close();

		return msg;
	}

	ERMsg CUIEnvCanHourly::DownloadSWOB(const CLocationVector& locations, const std::map<std::string, CFileInfoVector>& fileList, CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);
		callback.AddMessage("Number of SWOB-ML stations to download: " + ToString(fileList.size()));
		callback.PushTask("Download of SWOB-ML (" + ToString(fileList.size()) + " stations)", fileList.size());

		map<string, CTRef> lastUpdate;
		int nbDownload = 0;

		size_t nbTry = 0;
		map<string, CFileInfoVector>::const_iterator it1 = fileList.begin();
		while (it1 != fileList.end() && msg)
		{
			nbTry++;

			CInternetSessionPtr pSession;
			CHttpConnectionPtr pConnection;

			msg = GetHttpConnection("dd.weatheroffice.gc.ca", pConnection, pSession);


			for (; it1 != fileList.end() && msg; it1++)
			{
				string ID = GetLastDirName(GetPath(it1->second.front().m_filePath));
				callback.PushTask("Download SWOB-ML for " + ID + ": (" + ToString(it1->second.size()) + " files)", it1->second.size());

				{
					TRY

						map < CTRef, SWOBData > data;


					CLocationVector::const_iterator itLoc = locations.FindBySSI("ICAO", it1->first, false);
					ASSERT(itLoc != locations.end());

					CLocation location = *itLoc;

					CTRef lastTRef;
					for (CFileInfoVector::const_iterator it = it1->second.begin(); it != it1->second.end() && msg; it++)
					{

						string source;
						msg = GetPageText(pConnection, it->m_filePath, source, false, INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_DONT_CACHE);


						if (msg)
						{
							WBSF::ReplaceString(source, "'", " ");

							string fileName = GetFileName(it->m_filePath);
							CTRef UTCTRef = GetSWOBTRef(fileName);
							//CTRef TRef = CTimeZones::UTCTRef2LocalTRef(UTCTRef, zone);
							CTRef YearMonth = UTCTRef.as(CTM::MONTHLY);

							if (data.find(YearMonth) == data.end())
							{
								//load data
								string filePath = GetOutputFilePath(N_SWOB, location.GetSSI("Province"), UTCTRef.GetYear(), UTCTRef.GetMonth(), ID);
								if (FileExists(filePath))
									msg = ReadSWOB(filePath, data[YearMonth]);
							}
							if (msg)
								msg = ParseSWOB(UTCTRef, source, data[YearMonth][UTCTRef.GetDay()][UTCTRef.GetHour()], callback);

							if (msg)
							{
								nbDownload++;
								if (!lastTRef.IsInit() || UTCTRef > lastTRef)
									lastTRef = UTCTRef;
							}
						}//if msg

						msg += callback.StepIt();
					}//for all files

					for (auto it = data.begin(); it != data.end() && msg; it++)
					{
						CTRef TRef = it->first;
						string filePath = GetOutputFilePath(N_SWOB, location.GetSSI("Province"), TRef.GetYear(), TRef.GetMonth(), ID);

						CreateMultipleDir(GetPath(filePath));
						msg = SaveSWOB(filePath, it->second);
					}

					if (msg)
					{
						lastUpdate[ID] = lastTRef;
					}



					CATCH_ALL(e)

						msg = UtilWin::SYGetMessage(*e);

					END_CATCH_ALL
				}

				callback.PopTask();

				if (!msg && !callback.GetUserCancel())
				{
					if (nbTry < 5)
					{
						callback.AddMessage(msg);
						callback.PushTask("Waiting 30 seconds for server...", 600);
						for (size_t i = 0; i < 600 && msg; i++)
						{
							Sleep(50);//wait 50 milisec
							msg += callback.StepIt();
						}
						callback.PopTask();
					}
				}

				msg += callback.StepIt();
			}//for all station

			pConnection->Close();
			pSession->Close();
		}//for all run

		callback.AddMessage("Number of SWOB-ML files downloaded: " + ToString(nbDownload));
		callback.PopTask();

		msg = UpdateLastUpdate(lastUpdate);


		return msg;

	}


	string CUIEnvCanHourly::GetProvinceFormID(const string& ID)
	{
		string prov;
		ASSERT(ID.length() == 7);

		int i = WBSF::as<int>(ID.substr(0, 2));
		switch (i)
		{
		case 10:
		case 11: prov = CProvinceSelection::GetName(CProvinceSelection::BC); break;
		case 21: prov = CProvinceSelection::GetName(CProvinceSelection::YT); break;
		case 22: prov = CProvinceSelection::GetName(CProvinceSelection::NWT); break;
		case 23: prov = CProvinceSelection::GetName(CProvinceSelection::NU); break;
		case 24: prov = CProvinceSelection::GetName(CProvinceSelection::NU); break;
		case 25: prov = CProvinceSelection::GetName(CProvinceSelection::NWT); break;
		case 30: prov = CProvinceSelection::GetName(CProvinceSelection::ALTA); break;
		case 40: prov = CProvinceSelection::GetName(CProvinceSelection::SASK); break;
		case 50: prov = CProvinceSelection::GetName(CProvinceSelection::MAN); break;
		case 60:
		case 61: prov = CProvinceSelection::GetName(CProvinceSelection::ONT); break;
		case 70:
		case 71: prov = CProvinceSelection::GetName(CProvinceSelection::QUE); break;
		case 81: prov = CProvinceSelection::GetName(CProvinceSelection::NB); break;
		case 82: prov = CProvinceSelection::GetName(CProvinceSelection::NS); break;
		case 83: prov = CProvinceSelection::GetName(CProvinceSelection::PEI); break;
		case 84:
		case 85: prov = CProvinceSelection::GetName(CProvinceSelection::NFLD); break;
		case 90: prov = CProvinceSelection::GetName(CProvinceSelection::ONT); break;
		default:ASSERT(false);
		}

		ASSERT(!prov.empty());
		return prov;
	}

	ERMsg CUIEnvCanHourly::GetSWOBLocation(const string& source, CLocation& location)
	{
		ERMsg msg;
		try
		{
			zen::XmlDoc doc = zen::parse(source);

			enum TAttributes { DATE_TIME, STATION_NAME, TC_ID, SYNOP_ID, ELEVATION, DATA_PROVIDER, CLIM_ID, MSC_ID, LATITUDE, LONGITUDE, NB_ATTRIBUTES };
			static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "date_tm", "stn_nam", "tc_id", "wmo_synop_id", "stn_elev", "data_pvdr", "clim_id", "msc_id", "lat", "long" };

			zen::XmlIn in(doc.root());
			for (zen::XmlIn child = in["om:member"]["om:Observation"]["om:metadata"]["set"]["identification-elements"]["element"]; child&&msg; child.next())
			{
				string name;
				string value;
				if (child.attribute("name", name) && child.attribute("value", value))
				{
					size_t type = NOT_INIT;
					for (size_t i = 0; i < NB_ATTRIBUTES && type == NOT_INIT; i++)
						if (name == ATTRIBUTE_NAME[i])
							type = i;

					if (type != NOT_INIT)
					{
						switch (type)
						{
						case STATION_NAME:
						{
							string name = WBSF::PurgeFileName(value);
							location.m_name = WBSF::UppercaseFirstLetter(name);
							break;
						}
						case LATITUDE:		location.m_lat = ToDouble(value); break;
						case LONGITUDE:		location.m_lon = ToDouble(value); break;
						case ELEVATION:		location.m_alt = ToDouble(value); break;
						case DATE_TIME:     break;
						case CLIM_ID:		break;
						case SYNOP_ID:		location.SetSSI("WMO", value); break;
						case TC_ID:			location.SetSSI("IATA", value); break;
						case MSC_ID:		location.m_ID = value; break;
						default:			location.SetSSI(name, value);
						}
					}
				}
			}//for all attributes


			location.SetSSI("Province", GetProvinceFormID(location.m_ID));
		}
		catch (const zen::XmlParsingError& e)
		{
			// handle error
			msg.ajoute("Error parsing XML file: col=" + ToString(e.col) + ", row=" + ToString(e.row));
		}


		return msg;
	}

	ERMsg CUIEnvCanHourly::ParseSWOB(CTRef TRef, const std::string& source, SWOBDataHour& data, CCallback& callback)
	{

		ERMsg msg;

		if (source.find("<om:ObservationCollection") != NOT_INIT)
		{
			try
			{
				static set<string> variables;
				zen::XmlDoc doc = zen::parse(source);

				//CStatistic RelH;
				zen::XmlIn in(doc.root());
				for (zen::XmlIn child = in["om:member"]["om:Observation"]["om:result"]["elements"]["element"]; child&&msg; child.next())
				{
					string name;
					string value;
					string unit;

					if (child.attribute("name", name) && child.attribute("value", value) && child.attribute("uom", unit))
					{
						unit = UTF8_ANSI(unit);

						zen::XmlIn QA = child["qualifier"];//some element like Tdew don't have qualifier
						string QAStr;
						QA.attribute("value", QAStr);

						size_t type = NOT_INIT;
						for (size_t i = 0; i < NB_SWOB_VARIABLES && type == NOT_INIT; i++)
							if (name == SWOB_VARIABLE_NAME[i])
								type = i;

						if (type != NOT_INIT)
						{
							data[0] = ToString(TRef.GetYear());
							data[1] = ToString(TRef.GetMonth() + 1);
							data[2] = ToString(TRef.GetDay() + 1);
							data[3] = ToString(TRef.GetHour());
							data[type * 2 + 4] = value;
							data[type * 2 + 1 + 4] = QAStr;

							if (name == "tot_globl_solr_radn_pst1hr")
							{

								if (unit == "W/m²")
								{
									//do nothing
								}
								else if (unit == "kJ/m²")
								{
									float val = WBSF::as<float>(value);
									val *= 1000.0f / 3600.0f;//convert KJ/m² --> W/m²
									data[type * 2 + 4] = ToString(val);
								}
								else
								{
									callback.AddMessage("Other solar unit: " + unit);
								}

							}
							else
							{
								if (unit != DEFAULT_UNIT[type])
								{
									callback.AddMessage("Other unit (" + unit + ") for variable: " + name);
								}
							}

						}
					}
				}//for all attributes

			}
			catch (const zen::XmlParsingError& e)
			{
				// handle error
				callback.AddMessage("Error parsing XML file: col=" + ToString(e.col) + ", row=" + ToString(e.row));
				callback.AddMessage(source);
			}
		}
		else
		{
			callback.AddMessage("Page not found");
			//skip and continue
		}

		return msg;

	}



	ERMsg CUIEnvCanHourly::ReadSWOBData(const std::string& filePath, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;


		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		bool bFredericton = station.GetSSI("ICAO") == "CAFC";
		//CSun sun(station.m_lat, station.m_lon);


		unique_ptr<SWOBData> pSwob(new SWOBData);
		SWOBData& swob = *pSwob;


		msg = ReadSWOB(filePath, swob);
		for (size_t i = 0; i < 31; i++)
		{
			for (size_t j = 0; j < 24; j++)
			{
				if (!swob[i][j][0].empty() && !swob[i][j][1].empty() && !swob[i][j].empty() && !swob[i][j][3].empty())
				{
					int year = WBSF::as<int>(swob[i][j][0]);
					size_t m = WBSF::as<size_t>(swob[i][j][1]) - 1;
					size_t d = WBSF::as<size_t>(swob[i][j][2]) - 1;
					size_t h = WBSF::as<size_t>(swob[i][j][3]);

					CTRef UTCTRef(year, m, d, h);
					CTRef TRef = CTimeZones::UTCTRef2LocalTRef(UTCTRef, station);

					if (TRef.GetYear() >= firstYear && TRef.GetYear() <= lastYear)
					{
						for (size_t vv = 0; vv < NB_SWOB_VARIABLES; vv++)
						{
							TVarH v = VARIABLE_TYPE[vv];
							if (v != H_SKIP)
							{
								if (!swob[d][h][vv * 2 + 4].empty() && swob[d][h][vv * 2 + 4] != "MSNG")
								{
									int QAValue = WBSF::as<int>(swob[d][h][vv * 2 + 1 + 4]);
									if (QAValue > 0 || (v == H_SRAD2 && QAValue == 0))
									{
										float value = WBSF::as<float>(swob[i][j][vv * 2 + 4]);
										if (v == H_SRAD2 && value < 0)
											value = 0;

										if (v == H_SRAD2 && bFredericton)//fredericton have data 10 *????
											value /= 10;

										station[TRef].SetStat(v, value);
									}//if valid value
								}//if not missing
							}//if it's a valid var
						}//for all variables

						if (station[TRef][H_ADD1].IsInit() && station[TRef][H_ADD2].IsInit())
						{
							CStatistic RH = station[TRef][H_ADD1] + station[TRef][H_ADD2];
							station[TRef].SetStat(H_RELH, RH[MEAN]);
						}

						station[TRef].SetStat(H_ADD1, CStatistic());
						station[TRef].SetStat(H_ADD1, CStatistic());

						if (station[TRef][H_TMIN2].IsInit() && station[TRef][H_TMAX2].IsInit() && !station[TRef][H_TAIR2].IsInit())
						{
							CStatistic Tair = station[TRef][H_TMIN2] + station[TRef][H_TMAX2];
							station[TRef].SetStat(H_TAIR2, Tair[MEAN]);
						}
					}//if in year
				}//if init
			}//for all hours
		}//for all day

		return msg;
	}

	ERMsg CUIEnvCanHourly::ReadSWOB(const std::string& filePath, SWOBData& data)
	{
		ERMsg msg;
		//load data
		ifStream ifile;
		msg = ifile.open(filePath);
		if (msg)
		{
			for (CSVIterator loop(ifile); loop != CSVIterator() && msg; ++loop)
			{
				if (loop->size() >= 4)//when data finish with empty column, the llast column is removed
				{
					if (!(*loop)[0].empty() && !(*loop)[1].empty() && !(*loop)[2].empty() && !(*loop)[3].empty())
					{
						//int year = WBSF::as<int>((*loop)[0]);
						//size_t m = WBSF::as<size_t>((*loop)[1]) - 1;
						size_t d = WBSF::as<size_t>((*loop)[2]) - 1;
						size_t h = WBSF::as<size_t>((*loop)[3]);


						ASSERT(d < 31);
						ASSERT(h < 24);

						for (size_t i = 0; i < loop->size(); i++)
							data[d][h][i] = (*loop)[i];
					}
				}
			}

			ifile.close();

		}

		return msg;
	}

	ERMsg CUIEnvCanHourly::SaveSWOB(const std::string& filePath, const SWOBData& data)
	{
		ERMsg msg;

		ofStream ofile;
		msg = ofile.open(filePath);
		if (msg)
		{
			ofile << "Year,Month,Day,Hour";
			for (size_t i = 0; i < NB_SWOB_VARIABLES; i++)
				ofile << "," << SWOB_VARIABLE_NAME[i] << ",QA";

			ofile << endl;
			for (size_t d = 0; d < data.size(); d++)
			{
				for (size_t h = 0; h < 24; h++)
				{
					//ofile << TRef.GetYear() << "," << TRef.GetMonth() + 1 << "," << TRef.GetDay() + 1 << "," << TRef.GetHour();
					if (!data[d][h][0].empty() && !data[d][h][1].empty() && !data[d][h][2].empty() && !data[d][h][3].empty())
					{
						for (size_t i = 0; i < data[d][h].size(); i++)
						{
							if (i > 0)
								ofile << ",";

							ofile << data[d][h][i];
						}

						ofile << endl;
					}
				}

			}

			ofile.close();
		}

		return msg;
	}

	ERMsg CUIEnvCanHourly::UpdateLastUpdate(const map<string, CTRef>& lastUpdate)
	{
		ERMsg msg;
		string workingDir = GetDir(WORKING_DIR);
		string lastUpdateFilePath = workingDir + "SWOB-ML\\LastUpdate.csv";

		map<string, CTRef> currentUpdate;

		ifStream ifile;
		if (FileExists(lastUpdateFilePath))
			msg = ifile.open(lastUpdateFilePath);

		if (msg)
		{
			for (CSVIterator loop(ifile, ",", true); loop != CSVIterator(); ++loop)
			{
				if (loop->size() == 2)
				{
					string ID = (*loop)[0];
					CTRef TRef;
					TRef.FromFormatedString((*loop)[1]);
					currentUpdate[ID] = TRef;
				}
			}

			ifile.close();


			for (auto it = lastUpdate.begin(); it != lastUpdate.end(); it++)
				if (currentUpdate.find(it->first) == currentUpdate.end() || it->second > currentUpdate[it->first])
					currentUpdate[it->first] = it->second;

			ofStream ofile;
			msg = ofile.open(lastUpdateFilePath);
			if (msg)
			{
				ofile << "ID,LastUpdate" << endl;
				for (auto it = currentUpdate.begin(); it != currentUpdate.end(); it++)
				{
					CTRef TRef = it->second;
					string str = TRef.GetFormatedString("%Y-%m-%d-%H");
					ofile << it->first << "," << str << endl;
				}

				ofile.close();
			}
		}
		return msg;
	}

}