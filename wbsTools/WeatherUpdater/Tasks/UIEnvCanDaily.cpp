#include "StdAfx.h"
#include "UIEnvCanDaily.h"
#include "Basic/FileStamp.h"
#include "Basic/CSV.h"
#include "UI/Common/SYShowMessage.h"
#include "UI/Common/UtilWin.h"
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

	//données quotidienne un an toutes les stations
	//ftp://ftp.tor.ec.gc.ca/Climate_Services/Daily/

	//données horaire par années:
	//ftp://ftp.tor.ec.gc.ca/NAS_ClimateData_FlatFiles/HLY/HLY01/

	const char* CUIEnvCanDaily::SERVER_NAME = "climate.weather.gc.ca";

	//*********************************************************************
	const char* CUIEnvCanDaily::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "FirstYear", "LastYear", "Province" };
	const size_t CUIEnvCanDaily::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_STRING, T_STRING, T_STRING_SELECT };
	const UINT CUIEnvCanDaily::ATTRIBUTE_TITLE_ID = IDS_UPDATER_EC_DAILY_P;
	const UINT CUIEnvCanDaily::DESCRIPTION_TITLE_ID = ID_TASK_EC_DAILY;

	const char* CUIEnvCanDaily::CLASS_NAME() { static const char* THE_CLASS_NAME = "EnvCanDaily";  return THE_CLASS_NAME; }
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

	long CUIEnvCanDaily::GetNbDay(const CTime& t) { return GetNbDay(t.GetYear(), t.GetMonth() - 1, t.GetDay() - 1); }

	long CUIEnvCanDaily::GetNbDay(int y, size_t m, size_t d)
	{
		ASSERT(m >= 0 && m < 12);
		ASSERT(d >= 0 && d < 31);

		return long(y * 365 + m * 30.42 + d);
	}


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
		//sample for Alberta:
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


		static const short SEL_ROW_PER_PAGE = 100;

		CProvinceSelection selection;
		selection.FromString(Get(PROVINCE));
		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);

		callback.PushTask(GetString(IDS_LOAD_STATION_LIST), selection.any() ? selection.count() : CProvinceSelection::NB_PROVINCES);

		int nbRun = 0;
		size_t curI = 0;
		while (curI < CProvinceSelection::NB_PROVINCES && msg)
		{
			nbRun++;
			CInternetSessionPtr pSession;
			CHttpConnectionPtr pConnection;

			msg = GetHttpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", false, 5, callback);
			if (msg)
			{

				try
				{
					//loop on province
					while (curI < CProvinceSelection::NB_PROVINCES&&msg)
					{
						if (selection[curI])
						{

							//first call
							CTime today = CTime::GetCurrentTime();

							string URL = FormatA(pageFormat, selection.GetName(curI, CProvinceSelection::ABVR).c_str(), firstYear, lastYear, today.GetYear(), today.GetMonth(), today.GetDay(), SEL_ROW_PER_PAGE, 1);
							URL.resize(strlen(URL.c_str()));
							int nbStation = GetNbStation(pConnection, URL);

							if (nbStation != -1)
							{
								short nbPage = (nbStation - 1) / SEL_ROW_PER_PAGE + 1;

								callback.AddMessage(FormatMsg(IDS_LOAD_PAGE, selection.GetName(curI, CProvinceSelection::NAME), ToString(nbPage)));

								for (int j = 0; j < nbPage&&msg; j++)
								{
									short startRow = j * SEL_ROW_PER_PAGE + 1;
									URL = FormatA(pageFormat, selection.GetName(curI, CProvinceSelection::ABVR).c_str(), firstYear, lastYear, today.GetYear(), today.GetMonth(), today.GetDay(), SEL_ROW_PER_PAGE, startRow);

									msg = GetStationListPage(pConnection, URL, stationList);

									msg += callback.StepIt(1.0 / nbPage);
								}
							}
							else
							{
								throw (new UtilWin::CStringException(UtilWin::GetCString(IDS_SERVER_DOWN)));
							}
						}

						curI++;
					}
				}
				catch (CException* e)
				{
					//if an error occur: try again
					if (nbRun < 5)
					{
						callback.AddMessage(UtilWin::SYGetMessage(*e));
						msg = WaitServer(10, callback);
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


		}


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
			string::size_type posBegin = source.find("stations found");

			if (posBegin != string::npos)
			{
				posBegin -= 8;//return before the requested number
				string tmp = FindString(source, ">", "stations found", posBegin);
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
			if (source.find("stations found") != string::npos)
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
		WBSF::ReplaceString(str, "\"", "");
		return str;
	}

	ERMsg CUIEnvCanDaily::ParseStationListPage(const string& source, CLocationVector& stationList)const
	{
		ERMsg msg;
		string::size_type posBegin = 0;
		string::size_type posEnd = 0;

		//string::size_type posEnd = source.find("historical-data-results hidden-lg");


		FindString(source, "form action=", "method=\"post\"", posBegin, posEnd);

		while (posBegin != string::npos)
		{
			CLocation stationInfo;

			string period = PurgeQuote(FindString(source, "name=\"dlyRange\" value=", "/>", posBegin, posEnd));
			string internalID = PurgeQuote(FindString(source, "name=\"StationID\" value=", "/>", posBegin, posEnd));
			string prov = PurgeQuote(FindString(source, "name=\"Prov\" value=", "/>", posBegin, posEnd));
			string name = PurgeQuote(FindString(source, "<div class=\"col-lg-3 col-md-3 col-sm-3 col-xs-3\">", "</div>", posBegin, posEnd));

			if (posBegin != string::npos)
			{
				//when the station don't have daily value, the period is "|"
				if (!period.empty() && period != "N/A" && period != "|")
				{
					stationInfo.m_name = WBSF::PurgeFileName(Trim(WBSF::UppercaseFirstLetter(name)));
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

	ERMsg CUIEnvCanDaily::UpdateStationList(CLocationVector& stationList, CLocationVector& stations, CCallback& callback)
	{
		ERMsg msg;

		//update coordinates
		callback.PushTask("Update coordinates", stationList.size());

		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;

		msg = GetHttpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", false, 5, callback);
		if (!msg)
			return msg;


		for (CLocationVector::iterator it = stationList.begin(); it != stationList.end(); it++)
		{
			//this station doesn't exist, we add it
			CTPeriod period = String2Period(it->GetSSI("Period"));

			string internalID = it->GetSSI("InternalID");
			CLocationVector::iterator it2 = stations.FindBySSI("InternalID", internalID, false);
			if (it2 == stations.end() || it2->m_lat == -999)
			{
				if (it2 == stations.end())
				{
					stations.push_back(*it);
					it2 = stations.FindBySSI("InternalID", internalID, false);
				}

				ASSERT(it2 != stations.end());
				__int64 internalID64 = ToInt64(internalID);

				ERMsg msgTmp = UpdateCoordinate(pConnection, internalID64, period.End().GetYear(), period.End().GetMonth(), *it2);
				if (!msgTmp)
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

		ERMsg msg;

		string URL = FormatA(webPageDataFormat, id, year, month + 1);

		try
		{
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
					ClimateID = Trim(ClimateID);

					string WMOID = FindString(source, "labelledby=\"wmoid\">", "</div>", posBegin, posEnd);
					WMOID = Trim(WMOID);

					string TCID = FindString(source, "labelledby=\"tcid\">", "</div>", posBegin, posEnd);
					TCID = Trim(TCID);

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
						msg.ajoute("EnvCan Daily bad coordinate: " + URL);
					}
				}
			}
		}
		catch (CException* e)
		{
			//if an error occur: try again
			msg = UtilWin::SYGetMessage(*e);
		}

		return msg;
	}

	//************************************************************************************************************
	//data section

	ERMsg CUIEnvCanDaily::CopyStationDataPage(CHttpConnectionPtr& pConnection, __int64 ID, int year, const string& filePath, CCallback& callback)
	{
		ERMsg msg;

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

		string source;
		msg = GetPageText(pConnection, URL, source);
		if (msg)
		{
			string::size_type posBegin = source.find("\"Date/Time\"", 0);
			ASSERT(posBegin != string::npos);

			if (posBegin != string::npos)
			{
				ofStream file;
				msg = file.open(filePath);

				if (msg)
				{
					file << source.substr(posBegin);
					file.close();
				}
			}
			else
			{
				callback.AddMessage("Unable to load data from page with ID = " + ToString(ID) + ", year = " + ToString(year));
				msg = WaitServer(10, callback);
			}
		}


		return msg;
	}

	ERMsg CUIEnvCanDaily::DownloadStation(CHttpConnectionPtr& pConnection, const CLocation& info, CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);

		int nbFilesToDownload = 0;
		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = lastYear - firstYear + 1;

		CArray<bool> bNeedDownload;
		bNeedDownload.SetSize(nbYears);

		for (size_t y = 0; y < nbYears&&msg; y++)
		{
			int year = firstYear + int(y);

			CTPeriod period1 = String2Period(info.GetSSI("Period"));
			CTPeriod period2(CTRef(year, JANUARY, FIRST_DAY), CTRef(year, DECEMBER, LAST_DAY));
			if (period1.IsIntersect(period2))
			{
				string internalID = info.GetSSI("InternalID");
				string outputPath = GetOutputFilePath(info.GetSSI("Province"), year, internalID);
				bNeedDownload[y] = NeedDownload(outputPath, info, year);
				nbFilesToDownload += bNeedDownload[y] ? 1 : 0;
			}

			msg += callback.StepIt(0);
		}

		if (nbFilesToDownload > 10)
			callback.PushTask(info.m_name + " (" + ToString(nbFilesToDownload) + ")", nbFilesToDownload);



		for (size_t y = 0; y < nbYears&&msg; y++)
		{
			int year = firstYear + int(y);

			if (bNeedDownload[y])
			{
				string internalID = info.GetSSI("InternalID");
				string filePath = GetOutputFilePath(info.GetSSI("Province"), year, internalID);
				CreateMultipleDir(GetPath(filePath));
				try
				{
					msg += CopyStationDataPage(pConnection, ToLong(internalID), year, filePath, callback);
				}
				catch (CException* e)
				{
					if (nbFilesToDownload > 10)
						callback.PopTask();

					throw e;
				}

				if (nbFilesToDownload > 10)
					msg += callback.StepIt();
			}
		}

		if (nbFilesToDownload > 10)
			callback.PopTask();

		return msg;
	}


	bool CUIEnvCanDaily::NeedDownload(const string& filePath, const CLocation& info, int year)const
	{
		bool bDownload = true;

		CFileStamp fileStamp(filePath);
		CTime lastUpdate = fileStamp.m_time;
		if (lastUpdate.GetTime() > 0)
		{
			int nbDays = GetNbDay(lastUpdate) - GetNbDay(year, 0, 0);

			if (nbDays > (365 + 182))//six month after the end of the year
				bDownload = false;
		}

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



		//Getlocal station list
		if (FileExists(GetStationListFilePath()))
		{
			msg = m_stations.Load(GetStationListFilePath());
		}


		//Get remote station list
		CLocationVector stationList;
		if (msg)
			msg = DownloadStationList(stationList, callback);

		if (msg)
			msg = UpdateStationList(stationList, m_stations, callback);


		//save event if append an error
		msg += m_stations.Save(GetStationListFilePath());

		if (msg)
			msg = CleanStationList(stationList, callback);

		if (!msg)
			return msg;



		callback.PushTask("Download EnvCan daily data (" + ToString(stationList.size()) + " stations)", stationList.size());
		callback.AddMessage("Download EnvCan daily data (" + ToString(stationList.size()) + " stations)");
		size_t nbFiles = 0;
		int nbRun = 0;
		int curI = 0;

		while (curI < stationList.size() && msg)
		{
			nbRun++;

			CInternetSessionPtr pSession;
			CHttpConnectionPtr pConnection;

			msg = GetHttpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", false, 5, callback);

			if (msg)
			{
				try
				{
					while (curI < stationList.size() && msg)
					{
						msg = DownloadStation(pConnection, stationList[curI], callback);
						if (msg)
						{
							nbFiles++;
							curI++;
							nbRun = 0;
							msg += callback.StepIt();
						}
					}
				}
				catch (CException* e)
				{

					//if an error occur: try again
					if (nbRun < 5)
					{
						callback.AddMessage(UtilWin::SYGetMessage(*e));
						msg = WaitServer(10, callback);
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

		callback.AddMessage(GetString(IDS_NB_FILES_DOWNLOADED) + ToString(nbFiles), 1);
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



		for (CLocationVector::const_iterator it = m_stations.begin(); it != m_stations.end(); it++)
		{
			const CLocation& station = *it;
			CTPeriod period1 = String2Period(station.GetSSI("Period"));
			CTPeriod period2(CTRef(firstYear, JANUARY, DAY_01), CTRef(lastYear, DECEMBER, DAY_31));

			if (period1.IsIntersect(period2))
			{
				string prov = station.GetSSI("Province");
				size_t p = selection.GetProvince(prov);
				ASSERT(p != -1);

				if (selection.none() || selection[p])
				{
					string stationStr = station.m_ID;//.GetSSI("InternalID");
					stationList.push_back(stationStr);
				}
			}
		}

		return msg;
	}


	void CUIEnvCanDaily::GetStationInformation(const std::string& ID, CLocation& station)const
	{

		size_t i = m_stations.FindByID(ID);
		if (i < m_stations.size())
			station = m_stations[i];
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

	ERMsg CUIEnvCanDaily::GetWeatherStation(const string& ID, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;

		if (TM.Type() == CTM::HOURLY)
		{
			msg.ajoute("Unable to extract hourly data from daily updater");
			return msg;
		}


		GetStationInformation(ID, station);

		string internalID = station.GetSSI("InternalID");
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
			string filePath = GetOutputFilePath(prov, year, internalID);
			if (FileExists(filePath))
				msg = ReadData(filePath, station[year]);
		}

		station.CompleteSnow();

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

		enum { DATE_TIME, YEAR, MONTH, DAY, DATA_QUALITY, MAX_TEMP, MAX_TEMP_FLAG, MIN_TEMP, MIN_TEMP_FLAG, MEAN_TEMP, MEAN_TEMP_FLAG, HEAT_DEG_DAYS, HEAT_DEG_DAYS_FLAG, COOL_DEG_DAYS, COOL_DEG_DAYS_FLAG, TOTAL_RAIN, TOTAL_RAIN_FLAG, TOTAL_SNOW, TOTAL_SNOW_FLAG, TOTAL_PRECIP, TOTAL_PRECIP_FLAG, SNOW_ON_GRND, SNOW_ON_GRND_FLAG, DIR_OF_MAX_GUST, DIR_OF_MAX_GUST_FLAG, SPD_OF_MAX_GUST, SPD_OF_MAX_GUST_FLAG, NB_DAILY_COLUMN };

		//open file
		ifStream file;
		msg = file.open(filePath);

		if (msg)
		{
			size_t i = 0;
			for (CSVIterator loop(file, ",", true, true); loop != CSVIterator(); ++loop, i++)
			{
				if (loop.Header().size() != NB_DAILY_COLUMN)
				{
					//if the Env. Can web site change...
					msg.ajoute("Number of columns in Env Can daily file" + to_string(loop.Header().size()) + "is not the number expected " + to_string(NB_DAILY_COLUMN));
					msg.ajoute(filePath);
					return msg;
				}


				if (loop->size() == NB_DAILY_COLUMN)
				{
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

						dailyData[Tref][H_TAIR] = Tair;
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

						dailyData[Tref][H_TMIN] = Tmin;
						dailyData[Tref][H_TMAX] = Tmax;
					}

					if (((*loop)[TOTAL_PRECIP_FLAG].empty() || (*loop)[TOTAL_PRECIP_FLAG] == "E" || (*loop)[TOTAL_PRECIP_FLAG] == "T") && !(*loop)[TOTAL_PRECIP].empty())
					{
						float prcp = ToFloat((*loop)[TOTAL_PRECIP]);
						ASSERT(prcp >= 0 && prcp < 1000);
						dailyData[Tref][H_PRCP] = prcp;
					}

					//By RSA 29-03-2018
					//The snow in EnvCan is in cm and not in water equivalent
					//So we have to take rain instead
					if (dailyData[Tref][H_PRCP].IsInit() &&
						((*loop)[TOTAL_RAIN_FLAG].empty() || (*loop)[TOTAL_RAIN_FLAG] == "E" || (*loop)[TOTAL_RAIN_FLAG] == "T") && !(*loop)[TOTAL_RAIN].empty())
					{
						float rain = ToFloat((*loop)[TOTAL_RAIN]);
						ASSERT(rain >= 0 && rain < 1000);
						ASSERT(dailyData[Tref][H_PRCP][SUM] - rain >= 0);
						dailyData[Tref][H_SNOW] = max(0.0, dailyData[Tref][H_PRCP][SUM] - rain);
					}

					if (((*loop)[SNOW_ON_GRND_FLAG].empty() || (*loop)[SNOW_ON_GRND_FLAG] == "E" || (*loop)[SNOW_ON_GRND_FLAG] == "T") && !(*loop)[SNOW_ON_GRND].empty())
					{
						float sndh = ToFloat((*loop)[SNOW_ON_GRND]);
						ASSERT(sndh >= 0 && sndh < 1000);
						dailyData[Tref][H_SNDH] = sndh;
					}
				}


			}//for all line
		}

		return msg;
	}

}


