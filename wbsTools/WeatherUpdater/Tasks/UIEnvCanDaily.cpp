#include "StdAfx.h"
#include "UIEnvCanDaily.h"
#include "Basic/FileStamp.h"
#include "Basic/CSV.h"
#include "UI/Common/SYShowMessage.h"
#include "ProvinceSelection.h"
#include "TaskFactory.h"
#include "WeatherBasedSimulationString.h"
#include "../Resource.h"

using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace UtilWWW;

namespace WBSF
{

	//a faire, la liste des station st ici

	//ftp://client_climate@ftp.tor.ec.gc.ca/Pub/Get_More_Data_Plus_de_donnees/ 

	const char* CUIEnvCanDaily::SERVER_NAME = "climate.weather.gc.ca";

	//*********************************************************************
	const char* CUIEnvCanDaily::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "FirstYear", "LastYear", "Province"};
	const size_t CUIEnvCanDaily::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_STRING, T_STRING, T_STRING_SELECT};
	const UINT CUIEnvCanDaily::ATTRIBUTE_TITLE_ID = IDS_UPDATER_EC_DAILY_P;
	const UINT CUIEnvCanDaily::DESCRIPTION_TITLE_ID = ID_TASK_EC_DAILY;

	const char* CUIEnvCanDaily::CLASS_NAME(){ static const char* THE_CLASS_NAME = "EnvCanDaily";  return THE_CLASS_NAME; }
	CTaskBase::TType CUIEnvCanDaily::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUIEnvCanDaily::CLASS_NAME(), (createF)CUIEnvCanDaily::create);


	CUIEnvCanDaily::CUIEnvCanDaily(void)
	{}

	CUIEnvCanDaily::~CUIEnvCanDaily(void)
	{}


	std::string CUIEnvCanDaily::Option(size_t i)const
	{
		string str;
		switch (i)
		{
		case PROVINCE:		str = CProvinceSelection::GetAllPossibleValue(); break;
		};
		return str;
	}

	std::string CUIEnvCanDaily::Default(size_t i)const
	{
		std::string str;
		switch (i)
		{
		case WORKING_DIR: str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()) + "EnvCan\\Daily\\"; break;
		case FIRST_YEAR:	
		case LAST_YEAR:		str = ToString(CTRef::GetCurrentTRef().GetYear()); break;
		};

		return str;
	}

	long CUIEnvCanDaily::GetNbDay(const CTime& t){return GetNbDay(t.GetYear(), t.GetMonth() - 1, t.GetDay() - 1);}

	long CUIEnvCanDaily::GetNbDay(int y, size_t m, size_t d)
	{
		ASSERT(m >= 0 && m < 12);
		ASSERT(d >= 0 && d < 31);

		return long(y * 365 + m*30.42 + d);
	}

	//void CUIEnvCanDaily::Reset()
	//{
	//	CTaskBase::Reset();

	//	m_selection.Reset();
	//	m_boundingBox = DEFAULT_BOUDINGBOX;
	//	m_bForceDownload = false;
	//	m_bExtractSnow = false;

	//	m_nbDownload = 0;


	//	//	m_nbDays = GetNbDay(CTime::GetCurrentTime());
	//}


	//************************************************************************************************************
	//Load station definition list section


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


	ERMsg CUIEnvCanDaily::DownloadStationList(CLocationVector& stationList, CCallback& callback)
	{
		ERMsg msg;


		//Interface attribute index to attribute index
		//sample for alberta:
		//http://climate.weatheroffice.ec.gc.ca/advanceSearch/searchHistoricDataStations_f.html?timeframe=1&Prov=XX&StationID=99999&Year=2007&Month=10&Day=2&selRowPerPage=ALL&optlimit=yearRange&searchType=stnProv&startYear=2007&endYear=2007&lstProvince=ALTA&startRow=1
		//                                     /advanceSearch/searchHistoricDataStations_e.html?timeframe=1&lstProvince=PE&optLimit=yearRange&StartYear=1993&EndYear=2014&Year=2014&Month=8&Day=7&selRowPerPage=100&cmdProvSubmit=Search
		static const char pageFormat[] =
			"historical_data/search_historic_data_stations_e.html?"
			"searchType=stnProv&"
			"timeframe=2&"
			"lstProvince=%s&"
			"optLimit=yearRange&"
			"StartYear=%d&"
			"EndYear=%d&"
			"Year=%d&"
			"Month=%d&"
			"Day=%d&"
			"selRowPerPage=%d&"
			"startRow=%d&";
			//"advanceSearch/searchHistoricDataStations_e.html?"
			//"searchType=stnProv&"
			//"timeframe=2&"
			//"lstProvince=%s&"
			//"optLimit=yearRange&"
			//"StartYear=%d&"
			//"EndYear=%d&"
			//"Year=%d&"
			//"Month=%d&"
			//"Day=%d&"
			//"selRowPerPage=%d&"
			//"startRow=%d&"
			//"cmdProvSubmit=Search";


		static const short SEL_ROW_PER_PAGE = 100;

		CProvinceSelection selection;
		selection.FromString(Get(PROVINCE));
		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);

		callback.PushTask(GetString(IDS_LOAD_STATION_LIST), selection.any() ? selection.count() : CProvinceSelection::NB_PROVINCES);

		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;

		msg = GetHttpConnection(SERVER_NAME, pConnection, pSession);
		if (!msg)
			return msg;


		//loop on province
		for (size_t i = 0; i < CProvinceSelection::NB_PROVINCES&&msg; i++)
		{
			if (selection.any() && !selection[i])
				continue;

			//first call
			CTime today = CTime::GetCurrentTime();

			string URL = FormatA(pageFormat, selection.GetName(i, CProvinceSelection::ABVR).c_str(), firstYear, lastYear, today.GetYear(), today.GetMonth(), today.GetDay(), SEL_ROW_PER_PAGE, 1);
			URL.resize(strlen(URL.c_str()));
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


	int CUIEnvCanDaily::GetNbStation(CHttpConnectionPtr& pConnection, const string& URL)const
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

	ERMsg CUIEnvCanDaily::GetStationListPage(CHttpConnectionPtr& pConnection, const string& page, CLocationVector& stationList)const
	{
		ERMsg msg;
		string source;
		msg = GetPageText(pConnection, page, source);
		if (msg)
		{
			if (source.find("locations match") != string::npos)
			{
				msg = ParseStationListPage(source, stationList);
			}
		}

		return msg;
	}

	CTPeriod CUIEnvCanDaily::String2Period(string period)
	{
		ASSERT(!period.empty() && period != "N/A" && period != "|");
		//ReplaceString(period, "-999", "00");

		StringVector str(period, "-|");

		int v[6] = { 1100, 1, 1, 2099, 12, 31 };
		ASSERT(str.size() == 6);

		if (str.size() == 6)
		{
			for (int i = 0; i < 6; i++)
				v[i] = ToInt(str[i]);
		}

		return CTPeriod(CTRef(v[0], v[1] - 1, v[2] - 1), CTRef(v[3], v[4] - 1, v[5] - 1));
	}

	static string PurgeQuote(string str)
	{
		WBSF::ReplaceString( str, "\"", "");
		return str;
	}

	ERMsg CUIEnvCanDaily::ParseStationListPage(const string& source, CLocationVector& stationList)const
	{
		ERMsg msg;
		string::size_type posBegin = 0;
		string::size_type posEnd = 0;

		FindString(source, "form action=", "method=\"post\"", posBegin, posEnd);

		while (posBegin != string::npos)
		{
			CLocation stationInfo;

			string period = PurgeQuote(FindString(source, "name=\"dlyRange\" value=", "/>", posBegin, posEnd));
			string internalID = PurgeQuote(FindString(source, "name=\"StationID\" value=", "/>", posBegin, posEnd));
			string prov = PurgeQuote(FindString(source, "name=\"Prov\" value=", "/>", posBegin, posEnd));
			string name = PurgeQuote(FindString(source, "<div class=\"col-lg-3 col-md-3 col-sm-3 col-xs-3\">", "</div>", posBegin, posEnd));

			//when the station don't have dayly value, the period is "|"
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


	ERMsg CUIEnvCanDaily::UpdateStationList(CLocationVector& stationList, CEnvCanStationMap& stationMap, CCallback& callback)
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
			if (it2 == stationMap.end() || it2->second.m_lat==-999)
			{
				stationMap[ID] = *it;
				msg += UpdateCoordinate(pConnection, ID, period.End().GetYear(), period.End().GetMonth(), stationMap[ID]);
			}
			else
			{
				stationMap[ID].SetSSI("Period", it->GetSSI("Period"));
			}

			//now: update coordinate for station that are not init
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
	ERMsg CUIEnvCanDaily::UpdateCoordinate(CHttpConnectionPtr& pConnection, __int64 id, int year, size_t month, CLocation& station)
	{
		static const char webPageDataFormat[] =
		{
			"climate_data/daily_data_e.html?"
			"timeframe=2&"
			"StationID=%ld&"
			"Year=%d&"
			"Month=%d&"
			"Day=1"
		};

		//	"Prov=CA&"

		ERMsg msg;

		string URL;


		URL = FormatA(webPageDataFormat, id, year, month + 1);

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

	//************************************************************************************************************
	//data section

	ERMsg CUIEnvCanDaily::CopyStationDataPage(CHttpConnectionPtr& pConnection, __int64 ID, int year, const string& filePath)
	{
		ERMsg msg;

	//climate_data/bulk_data_e.html?
		//hlyRange=%7C&amp;dlyRange=2011-11-25%7C2016-04-29&amp;mlyRange=%7C&amp;
		//StationID=49771&amp;
		//Prov=PE&amp;
		//urlExtension=_e.html&amp;
		//searchType=stnProv&amp;
		//optLimit=yearRange&amp;
		//StartYear=2016&amp;
		//EndYear=2016&amp;
		//selRowPerPage=100&amp;
		//Line=4&amp;
		//Month=4&amp;
		//Day=24&amp;
		//lstProvince=PE&amp;
		//timeframe=2&amp;
		//Year=2016
		static const char pageDataFormat[] =
		{
			"climate_data/bulk_data_e.html?"
			"format=csv&"
			"stationID=%d&"
			"Year=%d&"
			"Month=1&"
			"Day=1&"
			"timeframe=2&"
			"submit=Download+Data"
		};

		string URL = FormatA(pageDataFormat, ID, year);
		//msg = UtilWWW::CopyFile(pConnection, URL, filePath);
		string source;
		msg = GetPageText(pConnection, URL, source);
		if (msg)
		{
			ofStream file;

			msg = file.open(filePath);

			if (msg)
			{
				string::size_type posBegin = source.find("\"Date/Time\"", 0);
				ASSERT(posBegin != string::npos);

				if (posBegin != string::npos)
				{
					file << source.substr(posBegin);
					file.close();
				}
				else
				{
					msg.ajoute("error loading page ID = " + ToString(ID) + ", year = " + ToString(year));
				}
			}
		}


		return msg;
	}

	ERMsg CUIEnvCanDaily::DownloadStation(CHttpConnectionPtr& pConnection, const CLocation& info, CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);

		int nbFilesToDownload = 0;
		//CProvinceSelection selection;
		//selection.FromString(Get(PROVINCE));
		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = lastYear - firstYear + 1;

		CArray<bool> bNeedDownload;
		bNeedDownload.SetSize(nbYears);

		for (size_t y = 0; y < nbYears&&msg; y++)
		{
			int year = firstYear + int(y);

			CTPeriod period = String2Period(info.GetSSI("Period"));
			if (period.IsInside(CTRef(year, JANUARY, FIRST_DAY)) ||
				period.IsInside(CTRef(year, DECEMBER, LAST_DAY)))
			{
				string internalID = info.GetSSI("InternalID");
				string outputPath = GetOutputFilePath(info.GetSSI("Province"), year, internalID);
				bNeedDownload[y] = NeedDownload(outputPath, info, year);
				nbFilesToDownload += bNeedDownload[y] ? 1 : 0;
			}

			msg += callback.StepIt(0);
		}

		if (nbFilesToDownload>10)
			callback.PushTask(info.m_name + " (" + ToString(nbFilesToDownload )+ ")", nbFilesToDownload);
		


		for (size_t y = 0; y < nbYears&&msg; y++)
		{
			int year = firstYear + int(y);

			if (bNeedDownload[y])
			{
				string internalID = info.GetSSI("InternalID");
				string filePath = GetOutputFilePath(info.GetSSI("Province"), year, internalID);
				CreateMultipleDir(GetPath(filePath));

				msg += CopyStationDataPage(pConnection, ToLong(internalID), year, filePath);

				//if (msg)
					//AddToStat(year);

				if (nbFilesToDownload>10)
					msg += callback.StepIt();
			}
		}

		if (nbFilesToDownload>10)
			callback.PopTask();

		return msg;
	}

	//void CUIEnvCanDaily::InitStat()
	//{
	//	m_nbDownload = 0;
	//	int nbYear = m_lastYear - m_firstYear + 1;
	//	m_stat.clear();
	//	m_stat.insert(m_stat.begin(), nbYear, 0);
	//}

	//void CUIEnvCanDaily::AddToStat(short year)
	//{
	//	m_nbDownload++;
	//	m_stat[year - m_firstYear]++;
	//}

	//void CUIEnvCanDaily::ReportStat(CCallback& callback)
	//{
	//	for (int y = 0; y < m_stat.size(); y++)
	//	{
	//		//for(int m=0; m<m_stat.GetCols(); m++)
	//		{
	//			if (m_stat[y] > 0)
	//			{
	//				string tmp = FormatMsg(IDS_UPDATE_END, ToString(m_stat[y]), ToString(m_stat[y]));
	//				string tmp2 = ToString(m_firstYear + y) + tmp;

	//				callback.AddMessage(tmp2);
	//			}
	//		}
	//	}

	//	callback.AddMessage(FormatMsg(IDS_UPDATE_END, ToString(m_nbDownload), ToString(m_nbDownload)));

	//}

	bool CUIEnvCanDaily::NeedDownload(const string& filePath, const CLocation& info, int year)const
	{
		bool bDownload = true;

		//if (!m_bForceDownload)
		//{
		CFileStamp fileStamp(filePath);
		CTime lastUpdate = fileStamp.m_time;
		if (lastUpdate.GetTime() > 0)
		{
			int nbDays = GetNbDay(lastUpdate) - GetNbDay(year, 0, 0);

			if (nbDays > (365 + 182))//six month after the end of the year
				bDownload = false;
		}
		//}

		return bDownload;
	}



	ERMsg CUIEnvCanDaily::CleanStationList(CLocationVector& stationList, CCallback& callback)const
	{
		ERMsg msg;

		//CGeoRect boundingBox;
		

		//if (!boundingBox.IsRectEmpty() && boundingBox != DEFAULT_BOUDINGBOX)
		//{
		//	callback.SetCurrentDescription(GetString(IDS_CLEAN_LIST));
		//	callback.SetNbStep(stationList.size());

		//	for (CLocationVector::iterator it = stationList.begin(); it != stationList.end() && msg;)
		//	{
		//	//	if (boundingBox.PtInRect(*it))
		//		{
		//			it++;
		//		}
		//		//else
		//		//{
		//		//	it = stationList.erase(it);
		//		//}

		//		msg += callback.StepIt();
		//	}
		//}

		return msg;
	}
	//*************************************************************************************************

	ERMsg CUIEnvCanDaily::Execute(CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);
		CreateMultipleDir(workingDir);

		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(workingDir, 1);
		callback.AddMessage(GetString(IDS_UPDATE_FROM));
		callback.AddMessage(SERVER_NAME, 1);
		callback.AddMessage("");

		CLocationVector stationList;
		//Getlocal station list
		if (FileExists(GetStationListFilePath()))
			msg = m_stations.Load(GetStationListFilePath());

		//Get remote station list
		if (msg)
			msg = DownloadStationList(stationList, callback);

		if (msg)
			msg = UpdateStationList(stationList, m_stations, callback);

		//save event if append an error
		msg = m_stations.Save(GetStationListFilePath());

		if (msg)
			msg = CleanStationList(stationList, callback);

		if (!msg)
			return msg;



		callback.PushTask("Download data (" + ToString(stationList.size()) + " stations)", stationList.size());
		callback.AddMessage("Download data (" + ToString(stationList.size()) + " stations)");

		int nbRun = 0;
		int curI = 0;

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
					for (int i = curI; i < stationList.size() && msg; i++)
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

	//****************************************************************************************************

	string CUIEnvCanDaily::GetOutputFilePath(const string& prov, int year, const string& stationName)const
	{
		return GetDir(WORKING_DIR) + prov + "\\" + ToString(year) + "\\" + stationName + ".csv";
	}


	ERMsg CUIEnvCanDaily::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;

		msg = m_stations.Load(GetStationListFilePath());

		if (!msg)
			return msg;


		CProvinceSelection selection(Get(PROVINCE));
		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		


		for (CEnvCanStationMap::const_iterator it = m_stations.begin(); it != m_stations.end(); it++)
		{
			//const CEnvCanStation& station = (const CEnvCanStation&)it->second;
			const CLocation& station = it->second;
			CTPeriod period = String2Period(station.GetSSI("Period"));
			if (period.Begin().GetYear() <= lastYear && period.End().GetYear() >= firstYear)
			{
				string prov = station.GetSSI("Province");
				size_t p = selection.GetProvince(prov);
				ASSERT(p != -1);

				if (selection.none() || selection[p])
				{
					string stationStr = station.GetSSI("InternalID");
					stationList.push_back(stationStr);
				}
			}
		}

		return msg;
	}


	void CUIEnvCanDaily::GetStationInformation(__int64 ID, CLocation& station)const
	{

		CEnvCanStationMap::const_iterator it = m_stations.find(ID);
		if (it != m_stations.end())
			station = it->second;
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

	ERMsg CUIEnvCanDaily::GetWeatherStation(const string& stationID, CTM TM, CWeatherStation& station, CCallback& callback)
	//ERMsg CUIEnvCanDaily::GetStation(const string& stationID, CDailyStation& station, CCallback& callback)
	{
		ERMsg msg;

		if (TM.Type() == CTM::HOURLY)
		{
			msg.ajoute("Unable to extract hourly data from daily updater");
			return msg;
		}

		__int64 ID = ToValue<__int64>(stationID);
		GetStationInformation(ID, station);

		string prov = station.GetSSI("Province");
		station.m_name = TraitFileName(station.m_name) + " (" + prov + ")";
		station.m_ID += "D";//add a "D" for daily data
		station.SetSSI("Period", "");

		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = lastYear - firstYear + 1;

		//now extract data 
		for (size_t y = 0; y < nbYears&&msg; y++)
		{
			int year = firstYear + int(y);
			string filePath = GetOutputFilePath(prov, year, stationID);
			if (FileExists(filePath))
				msg = ReadData(filePath, station[year]);
		}


		//verify station is valid
		if (msg && station.HaveData())
			msg = station.IsValid();

		return msg;
	}


	//"Legend"
	//"[Empty]","No Data Available"
	//"M","Missing"
	//"E","Estimated"
	//"A","Accumulated"
	//"C","Precipitation Occurred; Amount Uncertain"
	//"L","Precipitation May or May Not Have Occurred"
	//"F","Accumulated and Estimated"
	//"N","Temperature Missing but Known to be > 0"
	//"Y","Temperature Missing but Known to be < 0"
	//"S","More Than One Occurrence"
	//"T","Trace"
	//"*","Data for this day has undergone only preliminary quality checking"
	ERMsg CUIEnvCanDaily::ReadData(const string& filePath, CYear& dailyData)const
	{
		ERMsg msg;

		enum{ DATE_TIME, YEAR, MONTH, DAY, DATA_QUALITY, MAX_TEMP, MAX_TEMP_FLAG, MIN_TEMP, MIN_TEMP_FLAG, MEAN_TEMP, MEAN_TEMP_FLAG, HEAT_DEG_DAYS, HEAT_DEG_DAYS_FLAG, COOL_DEG_DAYS, COOL_DEG_DAYS_FLAG, TOTAL_RAIN, TOTAL_RAIN_FLAG, TOTAL_SNOW, TOTAL_SNOW_FLAG, TOTAL_PRECIP, TOTAL_PRECIP_FLAG, SNOW_ON_GRND, SNOW_ON_GRND_FLAG, DIR_OF_MAX_GUST, DIR_OF_MAX_GUST_FLAG, SPD_OF_MAX_GUST, SPD_OF_MAX_GUST_FLAG, NB_DAILY_COLUMN };

		//open file
		ifStream file;
		msg = file.open(filePath);

		if (msg)
		{
			bool bHaveSnow = false;
			size_t i = 0;
			for (CSVIterator loop(file, ",", true, true); loop != CSVIterator(); ++loop, i++)
			{
				ENSURE(loop.Header().size() == NB_DAILY_COLUMN);


				int year = ToInt((*loop)[YEAR]);
				int month = ToInt((*loop)[MONTH]) - 1;
				int day = ToInt((*loop)[DAY]) - 1;
				
				ASSERT(month >= 0 && month < 12);
				ASSERT(day >= 0 && day < GetNbDayPerMonth(year, month));
				CTRef Tref(year, month, day);

				if (((*loop)[MEAN_TEMP_FLAG].empty() || (*loop)[MEAN_TEMP_FLAG] == "E" || (*loop)[MEAN_TEMP_FLAG] == "T") && !(*loop)[MEAN_TEMP].empty())
				{
					float Tair = ToFloat((*loop)[MEAN_TEMP]);
					ASSERT(Tair >= -70 && Tair <= 70);

					dailyData[Tref][H_TAIR2] = Tair;
				}

				if (((*loop)[MIN_TEMP_FLAG].empty() || (*loop)[MIN_TEMP_FLAG] == "E") && !(*loop)[MIN_TEMP].empty() &&
					((*loop)[MAX_TEMP_FLAG].empty() || (*loop)[MAX_TEMP_FLAG] == "E") && !(*loop)[MAX_TEMP].empty())
				{
					float Tmin = ToFloat((*loop)[MIN_TEMP]);
					ASSERT(Tmin >= -70 && Tmin <= 70);
					float Tmax = ToFloat((*loop)[MAX_TEMP]);
					ASSERT(Tmax >= -70 && Tmax <= 70);

					//que faire quand Tmin est plus grand que Tmax???
					if (Tmin > Tmax)
						Switch(Tmin, Tmax);

					dailyData[Tref][H_TMIN2] = Tmin;
					dailyData[Tref][H_TMAX2] = Tmax;
				}

				if (((*loop)[TOTAL_PRECIP_FLAG].empty() || (*loop)[TOTAL_PRECIP_FLAG] == "E" || (*loop)[TOTAL_PRECIP_FLAG] == "T") && !(*loop)[TOTAL_PRECIP].empty())
				{
					float prcp = ToFloat((*loop)[TOTAL_PRECIP]);
					ASSERT(prcp >= 0 && prcp < 1000);
					dailyData[Tref][H_PRCP] = prcp;
				}

				
				if (((*loop)[TOTAL_SNOW_FLAG].empty() || (*loop)[TOTAL_SNOW_FLAG] == "E" || (*loop)[TOTAL_SNOW_FLAG] == "T") && !(*loop)[TOTAL_SNOW].empty())
				{
					float snow = ToFloat((*loop)[TOTAL_SNOW]);
					ASSERT(snow >= 0 && snow < 1000);
					dailyData[Tref][H_SNOW] = snow;
				}

				if (((*loop)[SNOW_ON_GRND_FLAG].empty() || (*loop)[SNOW_ON_GRND_FLAG] == "E" || (*loop)[SNOW_ON_GRND_FLAG] == "T") && !(*loop)[SNOW_ON_GRND].empty())
				{
					float sndh = ToFloat((*loop)[SNOW_ON_GRND]);
					ASSERT(sndh >= 0 && sndh < 1000);
					dailyData[Tref][H_SNDH] = sndh;
					if (sndh>0)
						bHaveSnow = true;
				}
				else if ((*loop)[SNOW_ON_GRND_FLAG].empty() && (*loop)[SNOW_ON_GRND].empty() && !(*loop)[TOTAL_PRECIP].empty())
				{
					//when both is empty and total precip is init --> zero
					if (bHaveSnow )//if previous day is init
						dailyData[Tref][H_SNDH] = 0;
				}

				if (bHaveSnow && Tref.GetJDay() == 182)
					bHaveSnow = false;

				//problème aussi avec le DewPoint et le minimum horaire
			}//for all line
		}

		return msg;
	}

}


