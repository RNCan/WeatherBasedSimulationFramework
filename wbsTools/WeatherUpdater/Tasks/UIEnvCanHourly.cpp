#include "StdAfx.h"
#include "UIEnvCanHourly.h"

#include "Basic/FileStamp.h"
#include "Basic/CSV.h"
#include "Basic/Statistic.h"
#include "Basic/UtilMath.h"
#include "Basic/Psychrometrics_SI.h"
#include "UI/Common/SYShowMessage.h"
#include "TaskFactory.h"

#include "../Resource.h"

using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace UtilWWW;

namespace WBSF
{

	const char* CUIEnvCanHourly::SERVER_NAME = "climate.weather.gc.ca";
	//*********************************************************************

	const char* CUIEnvCanHourly::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "FirstYear", "LastYear", "Province" };
	const size_t CUIEnvCanHourly::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_STRING, T_STRING, T_STRING_SELECT };
	const UINT CUIEnvCanHourly::ATTRIBUTE_TITLE_ID = IDS_UPDATER_EC_HOURLY_P;
	const UINT CUIEnvCanHourly::DESCRIPTION_TITLE_ID = ID_TASK_EC_HOURLY;

	const char* CUIEnvCanHourly::CLASS_NAME(){ static const char* THE_CLASS_NAME = "EnvCanHourly";  return THE_CLASS_NAME; }
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
		case PROVINCE:		str = CProvinceSelection::GetAllPossibleValue(); break;
		};
		return str;
	}

	std::string CUIEnvCanHourly::Default(size_t i)const
	{
		std::string str;
		switch (i)
		{
		case WORKING_DIR: str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()) + "EnvCan\\Hourly\\"; break;
		case FIRST_YEAR:
		case LAST_YEAR:		str = ToString(CTRef::GetCurrentTRef().GetYear()); break;
		};

		return str;
	}

	long CUIEnvCanHourly::GetNbDay(const CTime& t){	return GetNbDay(t.GetYear(), t.GetMonth() - 1, t.GetDay() - 1);	}
	long CUIEnvCanHourly::GetNbDay(int year, size_t m, size_t d)
	{
		ASSERT(m >= 0 && m < 12);
		ASSERT(d >= 0 && d < 31);

		return long(year * 365 + m*30.42 + d);
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

		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;

		msg = GetHttpConnection(SERVER_NAME, pConnection, pSession);
		if (!msg)
			return msg;

		callback.PushTask(GetString(IDS_LOAD_STATION_LIST), selection.any() ? selection.count() : CProvinceSelection::NB_PROVINCES);
		//callback.SetNbStep(selection.any() ? selection.count() : CProvinceSelection::NB_PROVINCES);


		//loop on province
		for (size_t i = 0; i < CProvinceSelection::NB_PROVINCES&&msg; i++)
		{
			if (selection.any() && !selection[i])
				continue;

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
					short startRow = j*SEL_ROW_PER_PAGE + 1;
					URL = FormatA(pageFormat, selection.GetName(i, CProvinceSelection::ABVR).c_str(), firstYear, lastYear, today.GetYear(), today.GetMonth(), today.GetDay(), SEL_ROW_PER_PAGE, startRow);
					msg = GetStationListPage(pConnection, URL, stationList);

					msg += callback.StepIt(1.0 / nbPage);
				}
			}
			else
			{
				msg.ajoute(GetString(IDS_SERVER_DOWN));
			}
		}

		pConnection->Close();
		pSession->Close();

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
			string::size_type posBegin = source.find("locations match");

			if (posBegin != string::npos)
			{
				posBegin -= 8;//return before hte requested number
				string tmp = FindString(source, ">", "locations match", posBegin);
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
			if (Find(source, "locations match"))
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

		return msg;
	}

	ERMsg CUIEnvCanHourly::UpdateStationList(CLocationVector& stationList, CEnvCanStationMap& stationMap, CCallback& callback)const
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
			CTPeriod period = String2Period(it->GetSSI("Period"));
			//if (period.IsInit())
			//{
			string internalID = it->GetSSI("InternalID");
			__int64 ID = ToInt64(internalID);
			CEnvCanStationMap::iterator it2 = stationMap.find(ID);
			if (it2 == m_stations.end() || it2->second.m_lat == -999)
			{
				stationMap[ID] = *it;
				msg += UpdateCoordinate(pConnection, ID, period.End().GetYear(), period.End().GetMonth(), period.End().GetDay(), stationMap[ID]);
			}
			else
			{
				stationMap[ID].SetSSI("Period", it->GetSSI("Period"));
			}

			//now: update coordinate for stationList station
			*it = stationMap[ID];

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
					msg.ajoute("Bad coordinate");
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
	ERMsg CUIEnvCanHourly::Execute(CCallback& callback)
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

		//InitStat();

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
				TRY
				{
					for (size_t i = curI; i < stationList.size() && msg; i++)
					{
						msg = DownloadStation(pConnection, stationList[i], callback);
						if (msg)
						{
							curI++;
							nbRun = 0;
							msg += callback.StepIt();
						}
					}
				}
				CATCH_ALL(e)
				{
					msg = UtilWin::SYGetMessage(*e);
				}
				END_CATCH_ALL

					//if an error occur: try again
					if (!msg && !callback.GetUserCancel())
					{
						if (nbRun < 5)
						{
							callback.AddMessage(msg);
							msg.asgType(ERMsg::OK);
							Sleep(1000);//wait 1 sec
						}
					}

				//clean connection
				pConnection->Close();
				pSession->Close();
			}
		}

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
		
		if (nbYear>5)
			callback.PushTask("Get number of files to update for " + station.m_name, nbYear * 12, 1);

		vector< array<bool, 12> > bNeedDownload;
		bNeedDownload.resize(nbYear);

		for (size_t y = 0; y < nbYear&&msg; y++)
		{
			int year = firstYear + int(y);

			for (size_t m = 0; m < 12 && msg; m++)
			{
				CTPeriod period = String2Period(station.GetSSI("Period"));
				if (period.IsInside(CTRef(year, m, FIRST_DAY)) ||
					period.IsInside(CTRef(year, m, LAST_DAY)))
				{
					string outputPath = GetOutputFilePath(station.GetSSI("Province"), year, m, station.GetSSI("InternalID"));
					bNeedDownload[y][m] = NeedDownload(outputPath, station, year, m);
					nbFilesToDownload += bNeedDownload[y][m] ? 1 : 0;
				}

				if (nbYear>5)
					msg += callback.StepIt();
			}
		}
		
		if (nbYear>5)
			callback.PopTask();


		//
		if (nbFilesToDownload > 0)
		{
			if (nbFilesToDownload>60)
				callback.PushTask("Update files for " + station.m_name + " (" + ToString(nbFilesToDownload)+")", nbFilesToDownload);

			for (size_t y = 0; y < nbYear&&msg; y++)
			{
				int year = firstYear + int(y);

				for (size_t m = 0; m < 12 && msg; m++)
				{
					if (bNeedDownload[y][m])
					{
						string internalID = station.GetSSI("InternalID");
						string filePath = GetOutputFilePath(station.GetSSI("Province"), year, m, internalID);
						CreateMultipleDir(GetPath(filePath));

						msg += CopyStationDataPage(pConnection, ToLong(internalID), year, m, filePath);
						msg += callback.StepIt(nbFilesToDownload>60?1:0);
					}
				}
			}

			if (nbFilesToDownload>60)
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

	string CUIEnvCanHourly::GetOutputFilePath(const string& prov, int year, size_t m, const string& stationName)const
	{
		ASSERT(prov.length() < 4);
		std::stringstream month;
		month << std::setfill('0') << std::setw(2) << (m + 1);

		return GetDir(WORKING_DIR) + prov + "\\" + ToString(year) + "\\" + month.str() + "\\" + stationName + ".csv";
	}


	ERMsg CUIEnvCanHourly::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;

		
		msg = m_stations.Load(GetStationListFilePath());

		if (!msg)
			return msg;


		CProvinceSelection selection(Get(PROVINCE));
		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		//CGeoRect boundingBox;

		for (CEnvCanStationMap::const_iterator it = m_stations.begin(); it != m_stations.end() && msg; it++)
		{
			const CLocation& station = it->second;
			CTPeriod p = String2Period(station.GetSSI("Period"));
			if (p.Begin().GetYear() <= lastYear && p.End().GetYear() >= firstYear) //Bug correction: By RSA 02/2012
			{
				/*	if (boundingBox.IsRectEmpty() ||
						boundingBox == DEFAULT_BOUDINGBOX ||
						boundingBox.PtInRect(station))
						{
						*/		//if (station.GetSSI("InternalID") == "50308")
				//{
				string prov = station.GetSSI("Province");
				size_t p = selection.GetProvince(prov);
				ASSERT(p != -1);


				if (selection.none() || selection[p])
				{
					string stationStr = prov + "\\" + ToString(station.GetSSI("InternalID"));
					stationList.push_back(stationStr);
				}
			}

			msg += callback.StepIt(0);
		}

		return msg;
	}

	void CUIEnvCanHourly::GetStationInformation(__int64 ID, CLocation& station)const
	{
		CEnvCanStationMap::const_iterator it = m_stations.find(ID);
		if (it != m_stations.end())
			station = it->second;
	}
	//
	//ERMsg CUIEnvCanHourly::GetStation(const string& str, CDailyStation& station, CCallback& callback)
	//{
	//	return GetWeatherStation(str, CTM(CTM::DAILY), station, callback);
	//}

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

	ERMsg CUIEnvCanHourly::GetWeatherStation(const string& stationName, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;

		//CProvinceSelection selection(Get(PROVINCE));
		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = size_t(lastYear - firstYear + 1);
		//CGeoRect boundingBox;

		string::size_type pos = 0;
		string prov = Tokenize(stationName, "\\", pos);
		string stationID = Tokenize(stationName, "\\", pos);
		__int64 ID = ToValue<__int64>(stationID);

		GetStationInformation(ID, station);
		station.m_name = TraitFileName(station.m_name) + " (" + prov + ")";
		station.m_ID += "H";//add a "H" for hourly data
		station.SetSSI("FirstYear", "");
		station.SetSSI("LastYear", "");


		station.CreateYears(firstYear, nbYears);


		//now extract data 
		for (size_t y = 0; y < nbYears&&msg; y++)
		{
			int year = firstYear + int(y);

			for (size_t m = 0; m < 12 && msg; m++)
			{
				if (msg)
				{
					string filePath = GetOutputFilePath(prov, year, m, stationID);
					if (FileExists(filePath))
						msg = ReadData(filePath, TM, station[year], callback);
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



	int GetHour(const string& line)
	{
		string::size_type pos = 0;
		return ToInt(Tokenize(line, ":", pos));
	}
	//
	//string CUIEnvCanHourly::GetStationIDFromName(const string& stationName)
	//{
	//	string::size_type pos = 0;
	//	string prov = Tokenize(stationName, "\\", pos);
	//	string stationID = Tokenize(stationName, "\\", pos);
	//	__int64 ID = ToValue<__int64>(stationID);
	//
	//	CLocation station;
	//	GetStationInformation(ID, station);
	//
	//
	//	return station.m_ID.c_str();
	//}


	ERMsg CUIEnvCanHourly::ReadData(const string& filePath, CTM TM, CYear& data, CCallback& callback)const
	{
		ERMsg msg;

		//int nbYear = m_lastYear-m_firstYear+1;

		enum{ DATE_TIME, H_YEAR, H_MONTH, H_DAY, TIMEVAL, DATA_QUALITY, TEMPERATURE, TEMPERATURE_FLAG, DEWPOINT, DEWPOINT_FLAG, RELHUM, RELHUM_FLAG, WIND_DIR, WIND_DIR_FLAG, WIND_SPEED, WIND_SPEED_FLAG, VISIBILITY, VISIBILITY_FLAG, PRESSURE, PRESSURE_FLAG, HMDX, HMDX_FLAG, WIND_CHILL, WIND_CHILL_FLAG, WEATHER_INFO, NB_INPUT_HOURLY_COLUMN };

		const int COL_POS[NB_VAR_H] = { -1, TEMPERATURE, -1, -1, DEWPOINT, RELHUM, WIND_SPEED, WIND_DIR, -1, PRESSURE, -1, -1, -1, -1, -1 };
		const double FACTOR[NB_VAR_H] = { 0, 1, 0, 0, 1, 1, 1, 10, 0, 10, 0, 0, 0, 0, 0 };



		//now extact data 
		ifStream file;

#pragma omp flush(msg)
		msg = file.open(filePath);
#pragma omp flush(msg)

		if (msg)
		{
			CWeatherAccumulator accumulator(TM);

			size_t i = 0;
			for (CSVIterator loop(file, ",", true, true); loop != CSVIterator() && msg; ++loop, i++)
			{
				ENSURE(loop.Header().size() == NB_INPUT_HOURLY_COLUMN);

				int year = ToInt((*loop)[H_YEAR]);
				int month = ToInt((*loop)[H_MONTH]) - 1;
				int day = ToInt((*loop)[H_DAY]) - 1;
				int hour = GetHour((*loop)[TIMEVAL]);
				//ASSERT( year>=m_firstYear && year<=m_lastYear);
				ASSERT(month >= 0 && month < 12);
				ASSERT(day >= 0 && day < GetNbDayPerMonth(year, month));
				ASSERT(hour >= 0 && hour < 24);

				CTRef TRef(year, month, day, hour);

				if (accumulator.TRefIsChanging(TRef))
				{
					data[accumulator.GetTRef()].SetData(accumulator);
				}

				bool bValid[NB_VAR_H] = { 0 };
				bValid[H_TAIR2] = ((*loop)[TEMPERATURE_FLAG].empty() || (*loop)[TEMPERATURE_FLAG] == "E") && !(*loop)[TEMPERATURE].empty();
				bValid[H_PRES] = (*loop)[PRESSURE_FLAG].empty() && !(*loop)[PRESSURE].empty();
				bValid[H_TDEW] = ((*loop)[DEWPOINT_FLAG].empty() || (*loop)[DEWPOINT_FLAG] != "M") && !(*loop)[DEWPOINT].empty();
				bValid[H_RELH] = ((*loop)[RELHUM_FLAG].empty() || (*loop)[RELHUM_FLAG] != "M")  && !(*loop)[RELHUM].empty();
				bValid[H_WNDS] = ((*loop)[WIND_SPEED_FLAG].empty() || (*loop)[WIND_SPEED_FLAG] != "E") && !(*loop)[WIND_SPEED].empty();
				bValid[H_WNDD] = ((*loop)[WIND_DIR_FLAG].empty() || (*loop)[WIND_DIR_FLAG] == "E") && !(*loop)[WIND_DIR].empty();
				//bValid[H_EA] = m_bExtractVaporPressure && bValid[H_TAIR] && (bValid[H_TDEW] || bValid[H_RELH]);
				//bValid[H_EA] = bValid[H_TAIR] && (bValid[H_TDEW] || bValid[H_RELH]);
				//bValid[H_ES] = m_bExtractVaporPressure && bValid[H_TAIR] && (bValid[H_TDEW] || bValid[H_RELH]);
				//bValid[H_ES] = bValid[H_TAIR] && (bValid[H_TDEW] || bValid[H_RELH]);

				for (int v = 0; v < NB_VAR_H; v++)
				{

					if (bValid[v])
					{
						/*if (v == H_ES)
						{
							double Tair = ToDouble((*loop)[COL_POS[H_TAIR]])*FACTOR[H_TAIR];
							double Es = e°(Tair) * 1000;
							accumulator.Add(TRef, H_ES, Es);
						}
						else if (v == H_EA)
						{
							double Tair = ToDouble((*loop)[COL_POS[H_TAIR]])*FACTOR[H_TAIR];
							double Tdew = ToDouble((*loop)[COL_POS[H_TDEW]])*FACTOR[H_TDEW];
							double Hr = ToDouble((*loop)[COL_POS[H_RELH]])*FACTOR[H_RELH];
							if (Hr == -999 && Tdew != -999)
								Hr = Td2Hr(Tair, Tdew);
							else if (Tdew == -999 && Hr != -999)
								Tdew = Hr2Td(Tair, Hr);

							double Ea = Hr2Pv(Tair, Hr);
							accumulator.Add(TRef, H_EA, Ea);
						}
						else
						{*/
						if (COL_POS[v] >= 0)
							accumulator.Add(TRef, v, ToDouble((*loop)[COL_POS[v]])*FACTOR[v]);
						//}
					}
				}
				

					

				if (bValid[H_TAIR2] && (!bValid[H_TDEW] || !bValid[H_RELH]))
				{
					double Tair = ToDouble((*loop)[COL_POS[H_TAIR2]])*FACTOR[H_TAIR2];
					double Tdew = ToDouble((*loop)[COL_POS[H_TDEW]])*FACTOR[H_TDEW];
					double Hr = ToDouble((*loop)[COL_POS[H_RELH]])*FACTOR[H_RELH];
					if (Hr == -999 && Tdew != -999)
						accumulator.Add(TRef, H_RELH, Td2Hr(Tair, Tdew));
					else if (Tdew == -999 && Hr != -999)
						accumulator.Add(TRef, H_TDEW, Hr2Td(Tair, Hr));
				}
			


				msg += callback.StepIt(0);
			}//for all line


			if (accumulator.GetTRef().IsInit())
				data[accumulator.GetTRef()].SetData(accumulator);

		}//if load 

		return msg;
	}
}