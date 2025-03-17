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
#include "Basic/CallcURL.h"
#include "UI/Common/SYShowMessage.h"
#include "UI/Common/UtilWin.h"
#include "TaskFactory.h"
#include "Geomatic/TimeZones.h"
//#include <boost/algorithm/string/join.hpp>




#include "../Resource.h"

using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace UtilWWW;

namespace WBSF
{

	//https://wateroffice.ec.gc.ca/download/report_e.html?dt=1&df=csv&ext=zip

	//catalogue de toute les stations:
	//ftp://ftp.tor.ec.gc.ca/Pub/About_the_data/Station_catalogue/station_data_catalogue.txt

	//nouveau site:
	//http://beta.weatheroffice.gc.ca/observations/swob-ml/20170622/CACM/2017-06-22-0300-CACM-AUTO-swob.xml
	//https://climate.weather.gc.ca/historical_data/search_historic_data_stations_e.html?searchType=stnProv&timeframe=1&lstProvince=QC&optLimit=yearRange&StartYear=2020&EndYear=2020&Year=2020&Month=9&Day=29&selRowPerPage=100&startRow=1&

	const char* CUIEnvCanHourly::SERVER_NAME[NB_NETWORKS] = { "climate.weather.gc.ca","dd.weather.gc.ca", "dd.weather.gc.ca"/*"hpfx.collab.science.gc.ca"*/ };
	//*********************************************************************

	const char* CUIEnvCanHourly::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "FirstYear", "LastYear", "Province", "Network", "PartnerNetwork", "MaxSwobDays", "HourlyPrcpMax" };
	const size_t CUIEnvCanHourly::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_STRING, T_STRING, T_STRING_SELECT, T_STRING_SELECT, T_STRING_SELECT, T_STRING, T_STRING };
	const UINT CUIEnvCanHourly::ATTRIBUTE_TITLE_ID = IDS_UPDATER_EC_HOURLY_P;
	const UINT CUIEnvCanHourly::DESCRIPTION_TITLE_ID = ID_TASK_EC_HOURLY;

	const char* CUIEnvCanHourly::CLASS_NAME() { static const char* THE_CLASS_NAME = "EnvCanHourly";  return THE_CLASS_NAME; }
	CTaskBase::TType CUIEnvCanHourly::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUIEnvCanHourly::CLASS_NAME(), (createF)CUIEnvCanHourly::create);
	const char* NETWORK_NAME[] = { "Historical", "SWOB-ML", "SWOB-Partners" };


	//SWOB partners network
	static const size_t NB_PARTNER_NETWORK = 23;
	static const char* PARTNERS_NETWORK_NAME[NB_PARTNER_NETWORK] = { "bc-RioTinto","bc-crd", "bc-env-aq","bc-env-snow","bc-forestry","bc-tran","dfo-ccg-lighthouse","nb-firewx","nl-water","ns-firewx", "nt-forestry","nt-water",
		"on-firewx","on-grca","on-mto","on-trca","on_water","pc-firewx","qc-pom","sk-forestry","yt-avalanche","yt-firewx","yt-water" };



	static const char* PARTNERS_NETWORK_ID[NB_PARTNER_NETWORK] = { "RIOTINTO","BC-CRD","BC-ENV-AQ","BC-ENV-ASW","BC-WMB","BC-TRAN","DFO","NB-DNRED","NL-DECCM-WRMD","NS-DLF", "NWT-ENR","NWT-ENR",
		"ON-MNRF-AFFES","ON-GRCA","ON-MTO","ON-TRCA","ON-MNRF-EC-WSC","PC-NRMB","POM","SK-SPSA-WMB","YAA","YT-DCS-WFM","YT-DE-WRB" };



	string GetAllPartnersNetworkString()
	{
		std::ostringstream oss;
		std::copy(begin(PARTNERS_NETWORK_NAME), end(PARTNERS_NETWORK_NAME), std::ostream_iterator<std::string>(oss, "|"));
		return oss.str();
	}

	string GetNetwork(string  filepath)
	{
		string network;
		if (filepath.find("partners") != string::npos)
		{
			StringVector tmp(filepath, "/");
			assert(tmp.size() >= 7 && tmp.size() <= 9);

			network = tmp[5];
		}

		return network;
	}

	string IATA2ID(const CLocationVector& locations, string IATA_ID)
	{
		string ID = IATA_ID;
		auto it = locations.FindBySSI("IATA", IATA_ID, false);
		if (it != locations.end())
			ID = it->m_ID;

		return ID;
	}


	string GetID(string filepath)
	{
		string ID;
		if (filepath.find("partners") != string::npos)
		{
			StringVector tmp(filepath, "/");
			assert(tmp.size() >= 7 && tmp.size() <= 9);

			string p_network = GetNetwork(filepath);
			auto it_network = find_if(begin(PARTNERS_NETWORK_NAME), end(PARTNERS_NETWORK_NAME), [p_network](const char* name) {return IsEqual(p_network, name); });
			ASSERT(it_network != end(PARTNERS_NETWORK_NAME));
			size_t network_i = std::distance(begin(PARTNERS_NETWORK_NAME), it_network);
			ASSERT(network_i < NB_PARTNER_NETWORK);


			string network = PARTNERS_NETWORK_ID[network_i];

			if (p_network == "yt-water")
			{
				StringVector tmp(GetFileTitle(filepath), "-");
				ASSERT(tmp.size() == 13);

				string stID = tmp[7] + "-" + tmp[8];

				ID = network + "-" + stID;
			}
			else if (p_network == "nl-water")
			{
				StringVector tmp(GetFileTitle(filepath), "-");
				ASSERT(tmp.size() == 11);

				string stID = tmp[7];

				ID = network + "-" + stID;
			}
			else if (p_network == "qc-pom" || p_network == "yt-firewx" ||
				p_network == "yt-avalanche")
			{
				ID = tmp[7];
			}
			else if (p_network == "on_water")
			{
				if (tmp[7].find("wiski") != string::npos)
					ID = "ON-MNR-" + tmp[7];
				else
					ID = "EC-" + tmp[7];
			}
			else if (p_network == "on-mto")
			{
				//remove RON and add -
				string stID = tmp[7];
				ASSERT(stID.length() >= 7);
				stID = stID.substr(3, 2) + "-" + stID.substr(5);

				ID = network + "-" + stID;
			}
			else
			{
				string stID = tmp[7];

				ID = network + "-" + stID;
			}
		}
		else
		{
			StringVector tmp(filepath, "/");
			assert(tmp.size() == 6 || tmp.size() == 7);
			ID = tmp[5];
		}

		WBSF::ReplaceString(ID, "_", "-");
		return MakeUpper(ID);
	}


	enum TSWOBStationColumns { C_ID_MSC, C_NAME, C_LATITUDE, C_LONGITUDE, C_ELEVATION, C_PROVINCE, C_DATA_PROVIDER, C_DATASET_NETWORK, C_AUTO_MAN, C_ID_IATA, C_ID_WMO, NB_SWOB_COLUMNS, C_UNUSED = NB_SWOB_COLUMNS };


	array<size_t, NB_SWOB_COLUMNS> GetSwobColumns(StringVector headers)
	{
		static const char* SWOB_COLUMNS_NAME1[NB_SWOB_COLUMNS] = { "MSC_ID",  "Name", "Latitude", "Longitude", "Elevation(m)", "Province/Territory", "Data_Provider", "Dataset/Network", "AUTO/MAN","IATA_ID", "WMO_ID" };
		static const char* SWOB_COLUMNS_NAME2[NB_SWOB_COLUMNS] = { "# MSC ID",  "EN name", "Latitude", "Longitude", "Elevation", "Province", "Data Provider", "Dataset/Network", "AUTO/MAN","#IATA", "# WMO ID" };


		array<size_t, NB_SWOB_COLUMNS> columns;
		for (size_t c = 0; c < NB_SWOB_COLUMNS; c++)
		{
			auto it = find_if(headers.begin(), headers.end(), [c](const string& name) {return IsEqual(name, SWOB_COLUMNS_NAME1[c]); });
			if (it == headers.end())
				it = find_if(headers.begin(), headers.end(), [c](const string& name) {return IsEqual(name, SWOB_COLUMNS_NAME2[c]); });

			columns[c] = std::distance(headers.begin(), it);
		}

		ASSERT(columns[C_ID_MSC] < NB_SWOB_COLUMNS);
		ASSERT(columns[C_NAME] < NB_SWOB_COLUMNS);
		ASSERT(columns[C_LATITUDE] < NB_SWOB_COLUMNS);
		ASSERT(columns[C_LONGITUDE] < NB_SWOB_COLUMNS);
		ASSERT(columns[C_ELEVATION] < NB_SWOB_COLUMNS);
		ASSERT(columns[C_PROVINCE] < NB_SWOB_COLUMNS);
		ASSERT(columns[C_ID_WMO] < NB_SWOB_COLUMNS);


		return columns;

	}

	string GetCleanSwobName(string name_in)
	{
		static const string CLEAN = "CDA|RCS|Auto|BC|AB|SK|MN|ON|QC|NB|NS|PI|NL|NFLD|NF|NU|P.E.I.|N.S.|Automatic|NOVA SCOTIA|Quebec|Climat|Climate|NU.|AGDM|AGCM|ON.|(AUT)|NWT|ONT.|QUE.|CS|A|NFLD.|-NFLD|SASK.|PEI|MAN.|ONT|NL.|MAN.|N.B.|N.W.T.|ONTARIO|MAN|CCG|CR10|CR3000|MB|(CR3000)|71482|23X|CR10x_V24|AAFC|AUT|71484|AEC|SE|CR23X|MCDC|RBG";
		StringVector clean(CLEAN, "|");

		StringVector name(name_in, " ,/\\");
		for (StringVector::iterator it = name.begin(); it != name.end();)
		{
			if (clean.Find(*it, false) == NOT_INIT)
				it++;
			else
				it = name.erase(it);
		}

		"TRIAL IS", "CAPE MUDGE", "PINE ISLAND", "CAPE BEALE", "CAPE SCOTT", "BOAT BLUFF", "EGG ISLAND";

		ostringstream s;
		std::copy(name.begin(), name.end(), ostream_iterator<string>(s, " "));

		//Regina Upgrade - May 2008 
		return WBSF::PurgeFileName(WBSF::TrimConst(WBSF::UppercaseFirstLetter(UTF8_ANSI(s.str()))));
	}

	string GetOwnerName(const string& data_provider)
	{
		string owner = "EnvCan";

		static const char* PROVIDER_LONG_NAME[NB_PARTNER_NETWORK] = {
		"RioTinto",
		"Capital Regional District (CRD)",
		"Government of British Columbia: Ministry of Environment (BC-ENV)"
		"Government of British Columbia: Ministry of Environment",
		"Government of British Columbia: Ministry of Forests Lands and Natural Resource Operations and Rural Development BC Wildfire Service",
		"Government of British Columbia: Ministry of Transportation and Infrastructure",
		"Government of Canada: Fisheries and Oceans Canada Canadian Coast Guard",
		"Department of Fisheries and Ocean Canada",
		"The Government of New Brunswick: Department of Natural Resources and Energy Development",
		"Government of Newfoundland and Labrador: Department of Environment Climate Change and Municipalities Water Resources Management Division",
		"Government of Northwest Territories: Department of Environment and Natural Resources Forest Management Division",
		"Government of Northwest Territories: Department of Environment and Natural Resources Water Resources Division",
		"Grand River Conservation Authority",
		"Government of Ontario: Ministry of Transportation",
		"Toronto and Region Conservation Authority",
		"ON Ministry of Natural Resources and Forestry Aviation Forest Fire and Emergency Services",
		"Port of Montreal",
		"Parks Canada Agency: Natural Resource Management Branch",
		"Government of Saskatchewan: Public Safety Agency",
		"Avalanche Canada",
		"Government of Yukon: Department of Community Services Wildland Fire Management (YT-DCS-WFM).",
		"Government of Yukon: Department of Environment Water Resources Branch",
		"Government of Yukon",
		"Government of Yukon on behalf of The Yukon Avalanche Association"

		};


		static const char* PROVIDER_SHORT_NAME[NB_PARTNER_NETWORK] = {
		"BC RioTinto",
		"BC CRD"
		"BC Environment",
		"BC Environment",
		"BC Forests",
		"BC Transportation",
		"Canadian Coast Guard",
		"Fisheries and Ocean Canada",
		"New Brunswick Government",
		"Newfoundland Resources",
		"Northwest Territories Forest",
		"Northwest Territories Water",
		"Ontario Grand River Conservation Authority",
		"Ontario Transportation",
		"Toronto and Region Conservation Authority",
		"Ontario Resources",
		"Port of Montreal",
		"Parks Canada Agency",
		"Saskatchewan Public Safety Agency",
		"Avalanche Canada",
		"Yukon Wildfire",
		"Yukon Water",
		"Yukon Government",
		"Yukon Avalanche Association",

		};


		auto it = find_if(begin(PROVIDER_LONG_NAME), end(PROVIDER_LONG_NAME), [data_provider](const char* name) {return IsEqual(data_provider, name); });
		if (it != end(PROVIDER_LONG_NAME))
			owner = PROVIDER_SHORT_NAME[std::distance(begin(PROVIDER_LONG_NAME), it)];

		return owner;
	}


	string GetSwobProvince(string prov_name)
	{


		if (IsEqual(prov_name, "Newfoundland and Labrador"))
			prov_name = CProvinceSelection::GetName(CProvinceSelection::NFLD, CProvinceSelection::NAME);
		else if (IsEqual(prov_name, "New Brunswick"))
			prov_name = CProvinceSelection::GetName(CProvinceSelection::NB, CProvinceSelection::NAME);
		else if (IsEqual(prov_name, "British Columbia"))
			prov_name = CProvinceSelection::GetName(CProvinceSelection::BC, CProvinceSelection::NAME);

		size_t p = CProvinceSelection::GetProvince(prov_name, CProvinceSelection::NAME);
		if (p == NOT_INIT)
			p = CProvinceSelection::GetProvince(prov_name, CProvinceSelection::ABVR);

		return (p != NOT_INIT) ? CProvinceSelection::GetName(p, CProvinceSelection::ABVR) : "";


	}

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
		case NETWORK:	str = "Hist=Historical|SWOB=SWOB|SWOB_PARTNERS=SWOB(Partners)"; break;
		case PARTNERS_NETWORK:str = GetAllPartnersNetworkString();	break;
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
		case MAX_SWOB_DAYS: str = "0"; break;
		case HOURLY_PRCP_MAX: str = "100"; break;
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



	static string CleanString(string str)
	{
		string output;

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

		return ld + Signe(ld) * (lm / 60.0 + ls / 3600.0);
	}


	ERMsg CUIEnvCanHourly::DownloadStationList(CLocationVector& stationList, CCallback& callback)const
	{
		ERMsg msg;


		//Interface attribute index to attribute index
		//sample for alberta:
		//http://climate.weatheroffice.ec.gc.ca/advanceSearch/searchHistoricDataStations_f.html?timeframe=1&Prov=XX&StationID=99999&Year=2007&Month=10&Day=2&selRowPerPage=ALL&optlimit=yearRange&searchType=stnProv&startYear=2007&endYear=2007&lstProvince=ALTA&startRow=1

		static const char pageFormat[] =
			"https://climate.weather.gc.ca/historical_data/search_historic_data_stations_e.html?"
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
			"startRow=%d";
		//"cmdProvSubmit=Search";

		static const short SEL_ROW_PER_PAGE = 100;

		//CProvinceSelection selection;
		//selection.FromString(Get(PROVINCE));
		CProvinceSelection selection(Get(PROVINCE));
		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		callback.PushTask(GetString(IDS_LOAD_STATION_LIST), selection.any() ? selection.count() : CProvinceSelection::NB_PROVINCES);

		//loop on province
		size_t nbRun = 0;
		size_t curI = 0;
		while (curI < CProvinceSelection::NB_PROVINCES && msg)
		{
			nbRun++;

			if (selection[curI])
			{

				//first call
				CTime today = CTime::GetCurrentTime();
				string URL = FormatA(pageFormat, selection.GetName(curI, CProvinceSelection::ABVR).c_str(), firstYear, lastYear, today.GetYear(), today.GetMonth(), today.GetDay(), SEL_ROW_PER_PAGE, 1);


				size_t nbStation = NOT_INIT;
				msg = GetNbStation(URL, nbStation);

				if (msg)
				{
					ASSERT(nbStation != NOT_INIT && nbStation > 0);
					size_t nbPage = (nbStation - 1) / SEL_ROW_PER_PAGE + 1;

					callback.AddMessage(FormatMsg(IDS_LOAD_PAGE, selection.GetName(curI, CProvinceSelection::NAME), ToString(nbPage)));

					for (size_t j = 0; j < nbPage && msg; j++)
					{
						size_t startRow = j * SEL_ROW_PER_PAGE + 1;
						URL = FormatA(pageFormat, selection.GetName(curI, CProvinceSelection::ABVR).c_str(), firstYear, lastYear, today.GetYear(), today.GetMonth(), today.GetDay(), SEL_ROW_PER_PAGE, startRow);
						msg = GetStationListPage(URL, stationList);

						msg += callback.StepIt(1.0 / nbPage);
					}

					if (msg)
					{
						curI++;
						nbRun = 0;
					}

				}

				if (!msg && !callback.GetUserCancel() && nbRun < 5)
				{
					callback.AddMessage(msg);
					msg = WaitServer(10, callback);//remove error
				}
			}
			else
			{
				curI++;
			}
		}//for all provinces

		callback.AddMessage(GetString(IDS_NB_STATIONS) + ToString(stationList.size()));
		callback.PopTask();

		return msg;
	}


	ERMsg CUIEnvCanHourly::GetNbStation(const string& URL, size_t& nbStation)const
	{
		ERMsg msg;

		string argument = "-s -k \"" + URL + "\"";
		string exe = GetApplicationPath() + "External\\curl.exe";
		CCallcURL cURL(exe);

		string source;
		msg = cURL.get_text(argument, source);

		if (msg)
		{
			string::size_type posBegin = source.find("stations found");

			if (posBegin != string::npos)
			{
				posBegin -= 8;//return before the requested number
				string tmp = FindString(source, ">", "stations found", posBegin);
				nbStation = ToInt(tmp);
			}
			else
			{
				msg.ajoute("Unable to find number of pages for:");
				msg.ajoute(URL);
			}

		}

		return msg;
	}

	ERMsg CUIEnvCanHourly::GetStationListPage(const string& URL, CLocationVector& stationList)const
	{
		ERMsg msg;


		string argument = "-s -k \"" + URL + "\"";
		string exe = GetApplicationPath() + "External\\curl.exe";
		CCallcURL cURL(exe);

		string source;
		msg = cURL.get_text(argument, source);

		//		msg = GetPageText(pConnection, page, source);
		if (msg)
		{
			if (Find(source, "stations found"))
			{
				msg = ParseStationListPage(source, stationList);
			}
			else
			{
				msg.ajoute("Unable get station list for:");
				msg.ajoute(URL);
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
				//when the station don't have hourly value, the period is "|"
				if (!period.empty() && period != "N/A" && period != "|")
				{
					//stationInfo.m_name = Trim(name);
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

	ERMsg CUIEnvCanHourly::UpdateStationList(CLocationVector& stationList, CLocationVector& stations, CCallback& callback)const
	{
		ERMsg msg;

		//update coordinates
		callback.PushTask("Update coordinates", stationList.size());

		for (CLocationVector::iterator it = stationList.begin(); it != stationList.end(); it++)
		{
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

				ERMsg msgTmp = UpdateCoordinate(internalID64, period.End().GetYear(), period.End().GetMonth(), period.End().GetDay(), *it2);
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

		callback.PopTask();

		return msg;
	}


	//because station coordinate in the csv file is lesser accurate than in the web page
	//we have to update coordinate from web page
	ERMsg CUIEnvCanHourly::UpdateCoordinate(__int64 ID, int year, size_t m, size_t d, CLocation& station)const
	{
		static const char webPageDataFormat[] =
		{
			"https://climate.weather.gc.ca/climate_data/hourly_data_e.html?"
			"timeframe=1&"
			"StationID=%ld&"
			"Year=%d&"
			"Month=%d&"
			"Day=%d"
		};

		ERMsg msg;

		string URL = FormatA(webPageDataFormat, ID, year, m + 1, d + 1);


		string argument = "-s -k \"" + URL + "\"";
		string exe = GetApplicationPath() + "External\\curl.exe";
		CCallcURL cURL(exe);

		string source;
		msg = cURL.get_text(argument, source);


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
					msg.ajoute("EnvCan Hourly bad coordinate: " + URL);
				}
			}
		}

		return msg;
	}

	//******************************************************


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
			StringVector net("HIST|SWOB|SWOB_PARTNERS", "|");
			for (size_t i = 0; i < str.size(); i++)
			{
				size_t n = net.Find(str[i], false);
				if (n < network.size())
					network.set(n);
			}
		}

		return network;
	}


	ERMsg CUIEnvCanHourly::Execute(CCallback& callback)
	{
		ERMsg msg;

		std::bitset<NB_NETWORKS> network = GetNetWork();

		for (size_t n = 0; n < network.size(); n++)
		{
			if (network.test(n))
			{
				switch (n)
				{
				case N_HISTORICAL:	msg = ExecuteHistorical(callback); break;
				case N_SWOB:		msg = ExecuteSWOB(N_SWOB, callback); break;
				case N_SWOB_PARTNERS:msg = ExecuteSWOB(N_SWOB_PARTNERS, callback); break;
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
		callback.AddMessage(SERVER_NAME[N_HISTORICAL], 1);
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

			msg = DownloadStation(stationList[curI], callback);
			if (msg)
			{
				curI++;
				nbRun = 0;
				nbFiles++;
				msg += callback.StepIt();
			}
			else if (!callback.GetUserCancel() && nbRun < 5)
			{
				callback.AddMessage(msg);
				msg = WaitServer(10, callback);//remove error
			}
		}

		callback.AddMessage("Number of stations updated: " + ToString(nbFiles));
		callback.PopTask();

		return msg;
	}


	ERMsg CUIEnvCanHourly::DownloadStation(const CLocation& station, CCallback& callback)
	{
		ERMsg msg;

		static const char pageDataFormat[] =
		{
			"https://climate.weather.gc.ca/climate_data/bulk_data_e.html?"
			"format=csv&"
			"stationID=%s&"
			"Year=%d&"
			"Month=%d&"
			"Day=1&"
			"timeframe=1&"
			"submit=Download+Data"
		};



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

		for (size_t y = 0; y < nbYear && msg; y++)
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

			try
			{
				for (size_t y = 0; y < nbYear && msg; y++)
				{
					int year = firstYear + int(y);

					for (size_t m = 0; m < 12 && msg; m++)
					{
						if (bNeedDownload[y][m])
						{
							string internalID = station.GetSSI("InternalID");
							string filePath = GetOutputFilePath(N_HISTORICAL, station.GetSSI("Province"), year, m, internalID);
							CreateMultipleDir(GetPath(filePath));


							string URL = FormatA(pageDataFormat, internalID.c_str(), year, m + 1);

							string exe = "\"" + GetApplicationPath() + "External\\curl.exe\"";
							string argument = "-s -k \"" + URL + "\" --output \"" + filePath + "\"";
							string command = exe + " " + argument;

							DWORD exit_code;
							msg = WinExecWait(command, "", SW_HIDE, &exit_code);
							if (exit_code == 0 && FileExists(filePath))
							{
								ifStream file;
								msg += file.open(filePath);
								if (msg)
								{
									string line;
									if (std::getline(file, line))
									{
										if (!WBSF::Find(line, "Climate ID"))
										{
											msg.ajoute("Error :" + line);
											msg.ajoute(filePath);
										}
									}
									else
									{
										msg.ajoute("Empty file: " + filePath);
									}

									file.close();
								}

								if (!msg)
								{
									msg = WBSF::RemoveFile(filePath);
								}
							}
							else
							{
								//msg.ajoute("Error in WinCSV");
								callback.AddMessage("Error in curl.exe");
							}

							msg += callback.StepIt(0);
							msg += callback.StepIt(nbFilesToDownload > 60 ? 1 : 0);
						}
					}
				}
			}
			catch (CException*)
			{
				if (nbFilesToDownload > 60)
					callback.PopTask();

				throw;
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

	string CUIEnvCanHourly::GetOutputFilePath(size_t n, const string& prov, int year, size_t m, const string& ID)const
	{
		ASSERT(prov.length() < 4);
		std::stringstream month;
		month << std::setfill('0') << std::setw(2) << (m + 1);

		string filePath;
		if (n == N_HISTORICAL)
			filePath = GetDir(WORKING_DIR) + prov + "\\" + ToString(year) + "\\" + month.str() + "\\" + ID + ".csv";
		else if (n == N_SWOB)
			filePath = GetDir(WORKING_DIR) + NETWORK_NAME[n] + "\\" + ToString(year) + "\\" + month.str() + "\\" + ID + ".csv";
		else if (n == N_SWOB_PARTNERS)
			filePath = GetDir(WORKING_DIR) + NETWORK_NAME[n] + "\\" + ToString(year) + "\\" + month.str() + "\\" + ID + ".csv";

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
				string network = NETWORK_NAME[n];
				switch (n)
				{
				case N_HISTORICAL:
				{
					msg += m_stations.Load(GetStationListFilePath());
					for (CLocationVector::const_iterator it = m_stations.begin(); it != m_stations.end() && msg; it++)
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
								tmpList.insert(station.m_ID);
							}
						}

						msg += callback.StepIt(0);
					}

					break;
				}

				case N_SWOB:
				{
					msg += m_SWOBstations.Load(GetSWOBStationsListFilePath(n));
					string filePath = workingDir + NETWORK_NAME[N_SWOB] + "\\MissingStations.csv";
					CLocationVector missingLoc;
					if (missingLoc.Load(filePath))
						m_SWOBstations.insert(m_SWOBstations.end(), missingLoc.begin(), missingLoc.end());


					for (CLocationVector::const_iterator it = m_SWOBstations.begin(); it != m_SWOBstations.end() && msg; it++)
					{
						string prov = it->GetSSI("Province");
						if (selection.at(prov))
						{
							tmpList.insert(it->m_ID);
						}
					}
					break;
				}

				case N_SWOB_PARTNERS:
				{

					StringVector partners_network(Get(PARTNERS_NETWORK), "|;,");
					msg += m_SWOB_partners_stations.Load(GetSWOBStationsListFilePath(n));
					string filePath = workingDir + NETWORK_NAME[N_SWOB_PARTNERS] + "\\MissingStations.csv";
					CLocationVector missingLoc;
					if (missingLoc.Load(filePath))
						m_SWOB_partners_stations.insert(m_SWOB_partners_stations.end(), missingLoc.begin(), missingLoc.end());


					for (CLocationVector::const_iterator it = m_SWOB_partners_stations.begin(); it != m_SWOB_partners_stations.end() && msg; it++)
					{
						string prov = it->GetSSI("Province");
						string p_network = it->GetSSI("Network");
						//if (!p_network.empty())
						{
							size_t it_p_network = partners_network.Find(p_network);

							//if ((partners_network.empty() || it_p_network != UNKNOWN_POS))
							{
								tmpList.insert(network + "\\" + it->m_ID);
							}
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


	CLocation CUIEnvCanHourly::GetStationInformation(string network, const string& ID)const
	{
		CLocation station;
		if (network.empty())
		{
			size_t i = m_stations.FindPosByID(ID);
			if (i < m_stations.size())
				station = m_stations[i];

			size_t pos = m_SWOBstations.FindPosByID(ID);
			if (pos != NOT_INIT)
			{
				if (station.IsInit())
					station.SetSSI("IATA", m_SWOBstations[pos].GetSSI("IATA"));
				else
					station = m_SWOBstations[pos];
			}
		}
		else //if (network == NETWORK_NAME[N_SWOB_PARTNERS])
		{
			size_t pos = m_SWOB_partners_stations.FindPosByID(ID);
			ASSERT(pos != NOT_INIT);
			//if (pos != NOT_INIT)
			//{
			station = m_SWOB_partners_stations[pos];
			//}
		}

		return station;
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


	ERMsg CUIEnvCanHourly::ReadSwobData(size_t network, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;
		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = size_t(lastYear - firstYear + 1);


		string ID = station.m_ID;

		for (size_t y = 0; y < nbYears && msg; y++)
		{
			int year = firstYear + int(y);

			for (size_t m = 0; m < 12 && msg; m++)
			{
				size_t size1 = sizeof(string);
				size_t size2 = sizeof(SWOBDataHour);
				size_t size3 = sizeof(SWOBData);
				string filePath = GetOutputFilePath(network, station.GetSSI("Province"), year, m, ID);
				if (FileExists(filePath))
					msg = ReadSWOBData(filePath, TM, station, callback);

				//read also files with IATA name
				if (network == N_SWOB)
				{
					string filePath = GetOutputFilePath(network, station.GetSSI("Province"), year, m, station.GetSSI("IATA"));
					if (FileExists(filePath))
						msg = ReadSWOBData(filePath, TM, station, callback);
				}


				msg += callback.StepIt(0);
			}
		}

		return msg;
	}


	ERMsg CUIEnvCanHourly::GetWeatherStation(const string& Network_ID, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;

		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = size_t(lastYear - firstYear + 1);

		StringVector tmp(Network_ID, "\\");
		ASSERT(tmp.size() == 1 || tmp.size() == 2);

		string network = (tmp.size() == 2) ? tmp[0] : "";
		string ID = (tmp.size() == 2) ? tmp[1] : tmp[0];

		((CLocation&)station) = GetStationInformation(network, ID);
		string prov = station.GetSSI("Province");
		station.m_name = TraitFileName(station.m_name) + " (" + prov + ")";


		station.SetSSI("FirstYear", "");
		station.SetSSI("LastYear", "");

		station.SetHourly(true);
		station.CreateYears(firstYear, nbYears);

		if (network.empty())
		{
			string internalID = station.GetSSI("InternalID");
			//now extract data 
			for (size_t y = 0; y < nbYears && msg; y++)
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


			if (msg && m_SWOBstations.FindPosByID(ID) != UNKNOWN_POS)
			{
				msg += ReadSwobData(N_SWOB, TM, station, callback);
			}

			station.m_ID += "H";//add a "H" for hourly data
			//AdministrativeDivisions
			station.SetSSI("Provider", "EnvCan");
			station.SetSSI("Network", "EnvCan");
			station.SetSSI("Country", "CAN");
			station.SetSSI("SubDivision", station.GetSSI("Province"));

		}
		else //N_SWOB_PARTNERS
		{
			msg += ReadSwobData(N_SWOB_PARTNERS, TM, station, callback);

			//AdministrativeDivisions
			station.SetSSI("Provider", "EnvCan(SwobPartners)");
			station.SetSSI("Network", station.GetSSI("Owner"));
			station.SetSSI("Country", "CAN");
			station.SetSSI("SubDivision", station.GetSSI("Province"));

		}

		if (msg)
		{
			//remove all precipitation greater than HOURLY_PRCP_MAX
			float max_prcp = as<float>(HOURLY_PRCP_MAX);
			CTPeriod p = station.GetEntireTPeriod();
			CTPeriod invalid_p;
			CStatistic invalid_prcp;
			for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
			{
				CHourlyData& hData = station.GetHour(TRef);
				if (max_prcp <= 0.01)
				{
					hData[H_PRCP] = -999;//remove precipitation
				}
				else
				{
					if (hData[H_PRCP] >= max_prcp)
					{
						//callback.AddMessage("Invalid precipitation: "+ToString(hData[H_PRCP],1)+ ", "+station.m_ID + ", " + station.m_name + "," + TRef.GetFormatedString());
						invalid_prcp += hData[H_PRCP];
						hData[H_PRCP] = -999;
						if (!invalid_p.IsInit())
							invalid_p.Begin() = TRef;
						invalid_p.End() = TRef;
					}
					else
					{
						if (invalid_p.IsInit())
						{
							callback.AddMessage("Invalid precipitation: " + ToString(invalid_prcp[MEAN], 1) + "," + station.m_ID + "," + station.m_name + "," + invalid_p.GetFormatedString("%1 to %2") + "," + to_string(invalid_p.size()));
							invalid_p.clear();
						}
					}
				}
			}

			if (invalid_p.IsInit())
			{
				callback.AddMessage("Invalid precipitation: " + ToString(invalid_prcp[MEAN], 1) + "," + station.m_ID + "," + station.m_name + "," + invalid_p.GetFormatedString("%1 to %2") + "," + to_string(invalid_p.size()));
				invalid_p.clear();
			}

			CWAllVariables vars;
			vars.reset(H_ADD1);
			vars.reset(H_ADD2);
			station.CleanUnusedVariable(vars);

			//verify station is valid
			if (station.HaveData())
			{
				msg = station.IsValid();
			}
		}


		if (station.m_ID == "9052008H")
		{
			station.m_name = "Sioux Falls (SD)";
			station.SetSSI("Country", "USA");
			station.SetSSI("SubDivision", "SD");
		}


		return msg;
	}



	int GetHour(const string& line)
	{
		string::size_type pos = 0;
		return ToInt(Tokenize(line, ":", pos));
	}

	enum { LONGITUDE_X, LATITUDE_Y, STATION_NAME, CLIMATE_ID, DATE_TIME, H_YEAR, H_MONTH, H_DAY, TIMEVAL, TEMPERATURE, TEMPERATURE_FLAG, DEWPOINT, DEWPOINT_FLAG, RELHUM, RELHUM_FLAG, PRECIP, PRECIP_FLAG, WIND_DIR, WIND_DIR_FLAG, WIND_SPEED, WIND_SPEED_FLAG, VISIBILITY, VISIBILITY_FLAG, PRESSURE, PRESSURE_FLAG, HMDX, HMDX_FLAG, WIND_CHILL, WIND_CHILL_FLAG, WEATHER_INFO, NB_INPUT_HOURLY_COLUMNS };
	static const char* COLUMNS_NAME[NB_INPUT_HOURLY_COLUMNS] = { "Longitude (x)", "Latitude (y)", "Station Name", "Climate ID", "Date/Time (LST)", "Year", "Month", "Day", "Time (LST)", u8"Temp (°C)", "Temp Flag", u8"Dew Point Temp (°C)", "Dew Point Temp Flag", "Rel Hum (%)", "Rel Hum Flag", "Precip. Amount (mm)", "Precip. Amount Flag", "Wind Dir (10s deg)", "Wind Dir Flag", "Wind Spd (km/h)", "Wind Spd Flag", "Visibility (km)", "Visibility Flag", "Stn Press (kPa)", "Stn Press Flag", "Hmdx", "Hmdx Flag", "Wind Chill", "Wind Chill Flag", "Weather" };
	const TVarH COL_VAR[NB_INPUT_HOURLY_COLUMNS] = { H_SKIP, H_SKIP,H_SKIP,H_SKIP,H_SKIP,H_SKIP,H_SKIP,H_SKIP,H_SKIP,
			H_TAIR, H_SKIP, H_TDEW, H_SKIP, H_RELH, H_SKIP, H_PRCP, H_SKIP, H_WNDD, H_SKIP, H_WNDS, H_SKIP, H_SKIP, H_SKIP, H_PRES, H_SKIP,
			H_SKIP, H_SKIP, H_SKIP, H_SKIP, H_SKIP };
	const double FACTOR[NB_VAR_H] = { 0, 1, 0, 1, 1, 1, 1, 10, 0, 10, 0, 0, 0, 0, 0 };

	static TVarH GetVariable(string header)
	{
		auto it = find_if(begin(COLUMNS_NAME), end(COLUMNS_NAME), [header](const char* name) {return IsEqual(header, name); });

		if (it == end(COLUMNS_NAME))
		{
			throw ERMsg(ERMsg::ERREUR, FormatA("Unknown column header in EnvCan hourly data: %s", header.c_str()));
		}

		return COL_VAR[std::distance(begin(COLUMNS_NAME), it)];
	}

	static std::map<TVarH, size_t> GetVariables(StringVector headers)
	{
		if (!headers.empty() && headers[0].length() >= 3)
		{
			char c0 = headers[0][0];
			char c1 = headers[0][1];
			char c2 = headers[0][2];
			//remove BOM if any
			if (c0 == '\xef' && c1 == '\xbb' && c2 == '\xbf')
				headers[0] = headers[0].substr(3);
		}


		std::map<TVarH, size_t> variables;
		for (size_t i = 0; i < headers.size(); i++)
		{
			TVarH v = GetVariable(headers[i]);
			if (v != H_SKIP)
				variables[v] = i;
		}

		return variables;
	}

	bool IsValid(TVarH v, const StringVector& row)
	{
		bool bValid = false;
		switch (v)
		{
		case H_TAIR: bValid = row.size() >= TEMPERATURE_FLAG && (row[TEMPERATURE_FLAG].empty() || row[TEMPERATURE_FLAG] == "E") && !row[TEMPERATURE].empty(); break;
		case H_PRCP: bValid = row.size() >= PRECIP_FLAG && row[PRECIP_FLAG].empty() && !row[PRECIP].empty(); break;
		case H_PRES: bValid = row.size() >= PRESSURE_FLAG && row[PRESSURE_FLAG].empty() && !row[PRESSURE].empty(); break;
		case H_TDEW: bValid = row.size() >= DEWPOINT_FLAG && (row[DEWPOINT_FLAG].empty() || row[DEWPOINT_FLAG] != "M") && !row[DEWPOINT].empty(); break;
		case H_RELH: bValid = row.size() >= RELHUM_FLAG && (row[RELHUM_FLAG].empty() || row[RELHUM_FLAG] != "M") && !row[RELHUM].empty(); break;
		case H_WNDS: bValid = row.size() >= WIND_SPEED_FLAG && (row[WIND_SPEED_FLAG].empty() || row[WIND_SPEED_FLAG] != "E") && !row[WIND_SPEED].empty(); break;
		case H_WNDD: bValid = row.size() >= WIND_DIR_FLAG && (row[WIND_DIR_FLAG].empty() || row[WIND_DIR_FLAG] == "E") && !row[WIND_DIR].empty(); break;
		default: ASSERT(false);
		}

		return bValid;
	}



	ERMsg CUIEnvCanHourly::ReadData(const string& filePath, CTM TM, CYear& data, CCallback& callback)const
	{
		ERMsg msg;


		map<TVarH, size_t> variables;
		size_t TairColPos = NOT_INIT;
		size_t TdewColPos = NOT_INIT;
		size_t RelHColPos = NOT_INIT;

		//now extract data 
		ifStream file;
		msg = file.open(filePath);

		if (msg)
		{
			for (CSVIterator loop(file, ",", true, true); loop != CSVIterator() && msg; ++loop)
			{
				if (variables.empty())
				{
					variables = GetVariables(loop.Header());
					if (variables.empty())
					{
						callback.AddMessage("Empty Env Can Hourly file: " + filePath);
						return msg;
					}

					size_t TairColPos = variables[H_TAIR];
					size_t TdewColPos = variables[H_TDEW];
					size_t RelHColPos = variables[H_RELH];

					ASSERT(TairColPos < NB_INPUT_HOURLY_COLUMNS && TdewColPos < NB_INPUT_HOURLY_COLUMNS && RelHColPos < NB_INPUT_HOURLY_COLUMNS);
				}

				if ((*loop).size() >= TEMPERATURE)
				{
					int year = ToInt((*loop)[H_YEAR]);
					int month = ToInt((*loop)[H_MONTH]) - 1;
					int day = ToInt((*loop)[H_DAY]) - 1;
					int hour = GetHour((*loop)[TIMEVAL]);

					ASSERT(month >= 0 && month < 12);
					ASSERT(day >= 0 && day < GetNbDayPerMonth(year, month));
					ASSERT(hour >= 0 && hour < 24);

					CTRef TRef(year, month, day, hour);

					bool bValid[NB_VAR_H] = { false };

					for (auto it = variables.begin(); it != variables.end(); it++)
					{
						TVarH v = it->first;
						bValid[v] = IsValid(v, *loop);

						if (bValid[v])
						{
							size_t c = it->second;
							ASSERT(c < (*loop).size());
							double value = ToDouble((*loop)[c]) * FACTOR[v];
							data[TRef].SetStat(v, value);
						}
					}

					if (bValid[H_TAIR] && (!bValid[H_TDEW] || !bValid[H_RELH]))
					{
						double Tair = ToDouble((*loop)[TairColPos]) * FACTOR[H_TAIR];
						double Tdew = ToDouble((*loop)[TdewColPos]) * FACTOR[H_TDEW];
						double Hr = ToDouble((*loop)[RelHColPos]) * FACTOR[H_RELH];
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
		//max_wnd_spd_10m_pst1hr_tm replace by snw_dpth_wtr_equiv RSA 2022-08-04
	{ "stn_pres", "air_temp", "rel_hum", "max_wnd_spd_10m_pst1hr", /*"max_wnd_spd_10m_pst1hr_tm"*/"snw_dpth_wtr_equiv",
	"wnd_dir_10m_pst1hr_max_spd", "avg_wnd_spd_10m_pst1hr", "avg_wnd_dir_10m_pst1hr", "avg_air_temp_pst1hr",
	"max_air_temp_pst1hr", "max_rel_hum_pst1hr", "min_air_temp_pst1hr", "min_rel_hum_pst1hr", "pcpn_amt_pst1hr",
	"snw_dpth", "rnfl_amt_pst1hr", "max_vis_pst1hr", "dwpt_temp", "tot_globl_solr_radn_pst1hr",
	"min_air_temp_pst24hrs", "max_air_temp_pst24hrs", "pcpn_amt_pst24hrs", "min_rel_hum_pst24hrs","max_rel_hum_pst24hrs",
	"avg_wnd_spd_pst2mts",	"avg_wnd_dir_pst2mts", "rnfl_amt_pst24hrs"
	};




	const char* CUIEnvCanHourly::DEFAULT_UNIT[NB_SWOB_VARIABLES] =
	{
		"hPa", "°C", "%", "km/h", "mm",
		"°", "km/h", "°", "°C",
		"°C", "%", "°C", "%", "mm",
		"cm", "mm", "km", "°C", "W/m²",
		"°C", "°C", "mm", "%", "%",
		"km/h", "°", "mm"
	};

	const TVarH CUIEnvCanHourly::VARIABLE_TYPE[NB_SWOB_VARIABLES] =
	{
		H_PRES, H_SKIP, H_SKIP, H_SKIP, H_SKIP,
		H_SKIP, H_WNDS, H_WNDD, H_TAIR,
		H_TMAX, H_ADD2, H_TMIN, H_ADD1, H_PRCP,
		H_SNDH, H_SKIP, H_SWE, H_TDEW, H_SRAD,
		H_SKIP, H_SKIP, H_SKIP, H_SKIP, H_SKIP,
		H_WNDS, H_WNDD, H_SKIP
	};


	ERMsg CUIEnvCanHourly::ExecuteSWOB(size_t network, CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);



		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(workingDir + NETWORK_NAME[network] + "\\", 1);
		callback.AddMessage(GetString(IDS_UPDATE_FROM));
		callback.AddMessage(SERVER_NAME[network], 1);
		callback.AddMessage("");

		string infoFilePath = GetSWOBStationsListFilePath(network);


		ERMsg msgLoc = UpdateSWOBLocations(network, callback);
		if (!msgLoc)
		{
			callback.AddMessage("Warning:");
			callback.AddMessage(msgLoc);
		}


		CLocationVector locations;
		msg = locations.Load(infoFilePath);

		if (msg)
		{


			string filePath = workingDir + NETWORK_NAME[network] + "\\MissingStations.csv";
			CLocationVector missingLoc;
			if (missingLoc.Load(filePath))
				locations.insert(locations.end(), missingLoc.begin(), missingLoc.end());


			map<string, CFileInfoVector> fileList;
			//set<string> missingID;
			msg = GetSWOBList(network, locations, fileList, callback);

			if (msg)
			{
				//					if (!missingID.empty())
					//					msg = UpdateMissingLocation(network, locations, fileList, missingID, callback);


				if (msg)
					msg = DownloadSWOB(network, locations, fileList, callback);
			}

		}

		return msg;
	}

	CTRef CUIEnvCanHourly::GetSWOBTRef(const string& fileName, bool bLighthouse)
	{
		CTRef TRef;
		if (!bLighthouse)
		{
			StringVector info(fileName, "-");

			int year = WBSF::as<int>(info[0]);
			size_t m = WBSF::as<size_t>(info[1]) - 1;
			size_t d = WBSF::as<size_t>(info[2]) - 1;
			size_t h = WBSF::as<size_t>(info[3].substr(0, 2));

			TRef = CTRef(year, m, d, h);
			ASSERT(TRef.IsValid());
		}
		else
		{
			//some file have a YYYMMDD format
			int year = WBSF::as<int>(fileName.substr(0, 4));
			size_t m = WBSF::as<size_t>(fileName.substr(4, 2)) - 1;
			size_t d = WBSF::as<size_t>(fileName.substr(6, 2)) - 1;
			size_t h = WBSF::as<size_t>(fileName.substr(9, 2));//+1 for T

			TRef = CTRef(year, m, d, h);
			ASSERT(TRef.IsValid());
		}


		return TRef;
	}

	string CUIEnvCanHourly::GetSWOBStationsListFilePath(size_t network)const
	{
		ASSERT(network == N_SWOB || network == N_SWOB_PARTNERS);

		string workingDir = GetDir(WORKING_DIR) + NETWORK_NAME[network] + "\\";
		string name = network == N_SWOB ? "swob-xml_station_list.csv" : "swob-xml_partners_station_list.csv";

		return workingDir + name;
	}


	ERMsg CUIEnvCanHourly::UpdateSWOBLocations(size_t network, CCallback& callback)
	{
		ASSERT(network == N_SWOB || network == N_SWOB_PARTNERS);

		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);

		string station_partner_network_filepath = workingDir + NETWORK_NAME[network] + "\\StationPartnersNetwork.csv";

		CParnerNetwork station_partners_network;
		if (FileExists(station_partner_network_filepath))
			msg += station_partners_network.load(station_partner_network_filepath);



		string infoFilePath = GetSWOBStationsListFilePath(network);
		WBSF::CreateMultipleDir(GetPath(infoFilePath));


		string infoFilePathTmp = infoFilePath;
		SetFileTitle(infoFilePathTmp, GetFileTitle(infoFilePath) + "_tmp");


		string URL = string("https://") + SERVER_NAME[network] + string("/observations/doc/") + (network == N_SWOB ? "swob-xml_station_list.csv" : "swob-xml_partner_station_list.csv");
		msg = CopyFileCurl(URL, infoFilePathTmp);

		//convert to compatible location file
		ifStream file;
		if (msg)
			msg = file.open(infoFilePathTmp);

		if (msg)
		{
			static const char* SSI_NAME[NB_SWOB_COLUMNS] = { "", "", "", "", "", "Province", "Data_Provider", "Network", "AUTO/MAN", "IATA", "WMO" };

			array<size_t, NB_SWOB_COLUMNS> columns;


			//le fichie est corrompu en date du 20 décembre 2022
			CLocationVector locations;
			for (CSVIterator loop(file, ",", true, true); loop != CSVIterator() && msg; ++loop)
				//for (CSVIterator loop(file, ";", true, true); loop != CSVIterator() && msg; ++loop)
			{
				if (locations.empty())
				{
					columns = GetSwobColumns(loop.Header());
				}

				CLocation location;

				location.m_ID = (*loop)[columns[C_ID_MSC]]; ASSERT(!location.m_ID.empty());
				//if(location.m_ID.length() > 3 && location.m_ID[2] == '_')
					//location.m_ID[2] = '-';
				//else if (location.m_ID.length() > 4 && location.m_ID[3] == '_')
					//location.m_ID[3] = '-';
				//else if (location.m_ID.find("RioTinto_") != string::npos )
					//location.m_ID[8] = '-';
				//replace all _ by -
				WBSF::ReplaceString(location.m_ID,"_","-");
				WBSF::ReplaceString(location.m_ID, "TRCA", "ON-TRCA");


				location.m_name = GetCleanSwobName((*loop)[columns[C_NAME]]);
				location.m_lat = WBSF::as<double>((*loop)[columns[C_LATITUDE]]);
				location.m_lon = WBSF::as<double>((*loop)[columns[C_LONGITUDE]]);
				location.m_elev = WBSF::as<double>((*loop)[columns[C_ELEVATION]]);

				if (columns[C_PROVINCE] < loop->size())
				{
					string prov_name = GetSwobProvince((*loop)[columns[C_PROVINCE]]);

					if (prov_name.empty())
					{
						if (location.m_ID.find("BC-ENV") != string::npos)
							prov_name = "BC";
					}

					location.SetSSI(SSI_NAME[C_PROVINCE], prov_name);
				}

				for (size_t c = C_DATA_PROVIDER; c < NB_SWOB_COLUMNS; c++)
				{
					if (columns[c] < loop->size())
					{
						//remove ,;
						string value = (*loop)[columns[c]];
						ReplaceString(value, ",", "");
						ReplaceString(value, ";", "");

						location.SetSSI(SSI_NAME[c], value);
					}
				}

				location.SetSSI("Owner", GetOwnerName(location.GetSSI("Data_Provider")));


				ASSERT(columns[C_DATA_PROVIDER] < loop->size());


				if (WBSF::Find(location.GetSSI(SSI_NAME[C_DATA_PROVIDER]), "Canadian Coast Guard"))
				{
					string IATA = location.GetSSI("IATA");
					MakeLower(IATA);
					ReplaceString(IATA, " ", "-");

					string ID = location.m_ID;
					location.m_ID = "DFO-" + MakeUpper(IATA);

					location.SetSSI("IATA", ID);

				}


				if (location.m_ID == "9052008")
					location.SetSSI("Province", "ON");
				if (location.m_ID == "7032682")
					location.SetSSI("Province", "QC");
				if (location.m_ID == "2203913")
					location.SetSSI("Province", "NT");
				if (location.m_ID == "ON-MNRF-AFFES-PNF")//Petawawa, ON
					location.m_lon = -77.4385;
				if (location.m_ID == "NB-DNRED-DUNG")//Dungarvon, NB
					location.m_lon = -66.3067;


				ASSERT(location.m_lon >= -180 && location.m_lon <= 180);

				auto it_p_network = station_partners_network.find(location.m_ID);

				if (it_p_network != station_partners_network.end())
					location.SetSSI(SSI_NAME[C_DATASET_NETWORK], station_partners_network[location.m_ID]);

				ASSERT(!location.m_name.empty());
				ASSERT(!location.m_ID.empty());

				locations.push_back(location);
			}

			msg = locations.Save(infoFilePath);
		}


		return msg;

	}

	ERMsg CUIEnvCanHourly::GetSWOBList(size_t network, CLocationVector& locations, map<string, CFileInfoVector>& fileList, CCallback& callback)
	{
		ASSERT(network == N_SWOB || network == N_SWOB_PARTNERS);

		//std::set<std::string>& missingID,

		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);
		CProvinceSelection selection(Get(PROVINCE));
		size_t maxDays = as<size_t>(MAX_SWOB_DAYS);
		CTRef now = CTRef::GetCurrentTRef();
		map<string, CTRef> lastUpdate;


		string lastUpdateFilePath = workingDir + NETWORK_NAME[network] + "\\LastUpdate.csv";


		ifStream ifile;
		if (FileExists(lastUpdateFilePath))
		{
			msg = ifile.open(lastUpdateFilePath);
			if (!msg)
				return msg;
		}


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


		set<string> dates;
		set<string> stationsID;

		CFileInfoVector dir2;
		if (msg)
		{

			vector<string> networks_URL;
			string URL = string("https://") + SERVER_NAME[network] + "/observations/swob-ml/";

			//select networks
			if (network == N_SWOB)
			{
				networks_URL.push_back(URL);
			}
			else if (network == N_SWOB_PARTNERS)
			{
				URL += "partners/";

				//select all partners network
				CFileInfoVector dir_network;
				if (msg)
					msg = FindDirectoriesCurl(URL, dir_network, callback);// date

				if (msg)
				{
					StringVector partners_network(Get(PARTNERS_NETWORK), "|;,");
					for (auto it = dir_network.begin(); it != dir_network.end(); it++)
					{
						string p_network = GetLastDirName(it->m_filePath);
						size_t n = partners_network.Find(p_network);

						if (partners_network.empty() || n != UNKNOWN_POS)
						{
							networks_URL.push_back(it->m_filePath);
						}

						//warning if new network
						auto it_network = find_if(begin(PARTNERS_NETWORK_NAME), end(PARTNERS_NETWORK_NAME), [p_network](const char* name) {return IsEqual(p_network, name); });
						if (it_network == end(PARTNERS_NETWORK_NAME) && p_network != "dfo-moored-buoys")
						{
							callback.AddMessage("Warning: new network was added: " + p_network);
						}
					}
				}
			}

			//for each networks, select dates to update
			CFileInfoVector dir1;
			if (msg)
			{
				for (size_t i = 0; i < networks_URL.size() && msg; i++)
				{
					CFileInfoVector dates_URL;
					msg = FindDirectoriesCurl(networks_URL[i], dates_URL, callback);// date
					if (msg)
					{
						for (CFileInfoVector::const_iterator it1 = dates_URL.begin(); it1 != dates_URL.end() && msg; it1++)
						{
							string YYYYMMDD = GetLastDirName(it1->m_filePath);
							bool bOnlyDigit = std::all_of(YYYYMMDD.begin(), YYYYMMDD.end(), [](unsigned char c) { return std::isdigit(c); });
							if (YYYYMMDD.size() == 8 && bOnlyDigit)
							{
								string p_network = GetNetwork(it1->m_filePath);

								int year = WBSF::as<int>(YYYYMMDD.substr(0, 4));
								size_t m = WBSF::as<size_t>(YYYYMMDD.substr(4, 2)) - 1;
								size_t d = WBSF::as<size_t>(YYYYMMDD.substr(6, 2)) - 1;
								CTRef TRef(year, m, d);

								size_t last_update_days = maxDays;
								if (maxDays == 0)
								{
									last_update_days = 100;//all days
									if (p_network.empty())
									{
										size_t last_update_days_prov = 0;
										//select the oldest update for a selected province
										for (size_t p = 0; p < NB_PROVINCES; p++)
										{
											if (selection[p])
											{
												auto findIt = lastUpdate.find("SWOB-" + CProvinceSelection::GetName(p));
												if (findIt != lastUpdate.end())
													last_update_days_prov = max(last_update_days_prov, size_t(max(0, now - findIt->second.as(CTM::DAILY) + 1)));
												else
													last_update_days_prov = 100;
											}
										}

										if (last_update_days_prov != 0)
											last_update_days = last_update_days_prov;
									}
									else
									{
										auto findIt = lastUpdate.find(p_network);
										if (findIt != lastUpdate.end())
											last_update_days = max(0, now - findIt->second.as(CTM::DAILY) + 1);
									}
								}

								if (max(0, now - TRef) <= last_update_days)
								{
									dir1.push_back(*it1);
								}
							}//valid date
						}//for all dates
					}//if msg
				}//for all networks
			}//if message


			if (msg)
			{
				string station_partner_network_filepath = workingDir + NETWORK_NAME[network] + "\\StationPartnersNetwork.csv";

				CParnerNetwork station_partners_network;
				if (FileExists(station_partner_network_filepath))
					msg += station_partners_network.load(station_partner_network_filepath);


				CLocationVector missingLoc;
				string missing_filepath = workingDir + NETWORK_NAME[network] + "\\MissingStations.csv";
				if (FileExists(missing_filepath))
					msg += missingLoc.Load(missing_filepath);


				callback.PushTask("Get days stations to update from: " + URL + " (" + to_string(dir1.size()) + " days)", dir1.size());



				for (CFileInfoVector::const_iterator it1 = dir1.begin(); it1 != dir1.end() && msg; it1++)
				{
					string p_network = GetNetwork(it1->m_filePath);

					if (p_network == "dfo-moored-buoys")
						continue;

					string YYYYMMDD = GetLastDirName(it1->m_filePath);
					int year = WBSF::as<int>(YYYYMMDD.substr(0, 4));
					size_t m = WBSF::as<size_t>(YYYYMMDD.substr(4, 2)) - 1;
					size_t d = WBSF::as<size_t>(YYYYMMDD.substr(6, 2)) - 1;
					CTRef TRef(year, m, d);



					if (p_network != "yt-water" && p_network != "nl-water")
					{

						CFileInfoVector dirTmp;
						msg = FindDirectoriesCurl(it1->m_filePath, dirTmp, callback);//stations

						for (CFileInfoVector::const_iterator it2 = dirTmp.begin(); it2 != dirTmp.end(); it2++)
						{
							string ID = GetID(it2->m_filePath);
							if (network == N_SWOB)
								ID = IATA2ID(locations, ID);

							CLocationVector::iterator it_location = locations.FindByID(ID, false); ;


							if (it_location == locations.end())
							{
								callback.AddMessage("Add missing station: " + ID);

								CLocation location = GetMissingLocation(it2->m_filePath);
								if (location.IsInit())
								{
									locations.push_back(location);
									if (network == N_SWOB)
										ID = IATA2ID(locations, ID);

									it_location = locations.FindByID(ID, false);
									ASSERT(it_location != locations.end());
									missingLoc.push_back(location);
								}
							}//unknown locations

							if (it_location != locations.end())
							{
								string prov = it_location->GetSSI("Province");
								string ID = it_location->m_ID;

								if (network == N_SWOB_PARTNERS)
								{
									string p_network = GetNetwork(it2->m_filePath);
									station_partners_network[ID] = p_network;

									//update network
									it_location->SetSSI("Network", p_network);
								}

								if (network == N_SWOB_PARTNERS || prov.empty() || selection.at(prov))
								{

									CTRef last_update_TRef;
									if (maxDays == 0)
									{
										auto findIt = lastUpdate.find(ID);
										//if (findIt == lastUpdate.end())//to update with old code
											//auto findIt = lastUpdate.find(IATA_ID);

										if (findIt != lastUpdate.end())
											last_update_TRef = findIt->second.as(CTM::DAILY);
									}


									if (!last_update_TRef.IsInit() || TRef >= last_update_TRef)
									{
										dir2.push_back(*it2);

										dates.insert(YYYYMMDD);
										stationsID.insert(ID);

									}
								}
							}
							else
							{
								callback.AddMessage("Unable to get location info for: " + ID);
								//missingID.insert(IATA_ID);
							}
						}
					}
					else//if (p_network != "yt-water" && p_network != "nl-water")
					{
						dir2.push_back(*it1);
						dates.insert(YYYYMMDD);

						//all station in the same directory
						CFileInfoVector files;
						msg = FindFilesCurl(it1->m_filePath + "*-AUTO-swob.xml", files);//stations
						for (CFileInfoVector::const_iterator it2 = files.begin(); it2 != files.end(); it2++)
						{
							string ID = GetID(it2->m_filePath);
							if (network == N_SWOB)
								ID = IATA2ID(locations, ID);


							CLocationVector::iterator it_location = locations.FindByID(ID, false);

							if (it_location == locations.end())
							{
								callback.AddMessage("Add missing station: " + ID);

								CLocation location = GetMissingLocation(it2->m_filePath);
								ASSERT(location.IsInit());

								locations.push_back(location);
								it_location = locations.FindByID(ID, false);
								ASSERT(it_location != locations.end());
								missingLoc.push_back(location);
							}//unknown station

							if (it_location != locations.end())
							{
								//string prov = it_location->GetSSI("Province");
								string ID = it_location->m_ID;

								string p_network = GetNetwork(it2->m_filePath);
								station_partners_network[ID] = p_network;

								//update network
								it_location->SetSSI("Network", p_network);

								stationsID.insert(ID);
							}
							else
							{
								callback.AddMessage("Unable to get location info for: " + ID);
								//missingID.insert(IATA_ID);
							}

						}
					}


					
					msg += callback.StepIt();
				}//if msg

				callback.PopTask();

				if (msg)
				{
					msg += missingLoc.Save(missing_filepath);
					msg += station_partners_network.save(station_partner_network_filepath);
				}
			}//if msg



		}

		if (msg)
		{
			callback.PushTask("Get hours to update for all days (" + to_string(dates.size()) + ") and stations (" + to_string(stationsID.size()) + "): " + to_string(dir2.size()) + " days stations", dir2.size());
			callback.AddMessage("Get hours to update for all days (" + to_string(dates.size()) + ") and stations (" + to_string(stationsID.size()) + ") " + to_string(dir2.size()) + " days stations");


			for (CFileInfoVector::const_iterator it2 = dir2.begin(); it2 != dir2.end() && msg; it2++)
			{

				//string p_network = GetNetwork(it2->m_filePath);


				CFileInfoVector fileListTmp;
				bool bLighthouse = Find(it2->m_filePath, "dfo-ccg-lighthouse");
				string filter = bLighthouse ? "*.xml" : "*-AUTO-swob.xml";
				msg = FindFilesCurl(it2->m_filePath + filter, fileListTmp);

				map<string, array<size_t, 24>> hour_count;



				for (CFileInfoVector::iterator it = fileListTmp.begin(); it != fileListTmp.end() && msg; it++)
				{
					string ID = GetID(it->m_filePath);
					if (network == N_SWOB)
						ID = IATA2ID(locations, ID);

					CTRef last_update_TRef;
					if (maxDays == 0)
					{
						CLocationVector::iterator it_location = locations.FindByID(ID, false);

						//locations.FindBySSI("IATA", IATA_ID, false);
						//if (it_location == locations.end())
							//it_location = locations.FindByID(IATA_ID, false);

						//ASSERT(it_location != locations.end());
						if (it_location != locations.end())
						{
							//string prov = it_location->GetSSI("Province");
							string ID = it_location->m_ID;

							auto findIt = lastUpdate.find(ID);
							//if (findIt == lastUpdate.end())//to update with old code
							//	auto findIt = lastUpdate.find(IATA_ID);

							if (findIt != lastUpdate.end())
								last_update_TRef = findIt->second;
						}
						else
						{
							//ASSERT(missingID.find(IATA_ID) != missingID.end());
							//	callback.AddMessage("Missing location ID: " + IATA_ID);
							//	callback.AddMessage(it->m_filePath);
						}
					}

					string fileName = GetFileName(it->m_filePath);
					CTRef TRef = GetSWOBTRef(fileName, bLighthouse);
					if (!last_update_TRef.IsInit() || TRef >= last_update_TRef)
					{

						if (hour_count[ID][TRef.GetHour()] == 0)//support one file by hour per station
						{
							fileList[ID].push_back(*it);
							hour_count[ID][TRef.GetHour()]++;
						}
					}

					msg += callback.StepIt(0);
				}//for all files

				msg += callback.StepIt();
			}



			callback.PopTask();
		}//if msg



		return msg;
	}

	/*ERMsg CUIEnvCanHourly::UpdateMissingLocation(size_t network, CLocationVector& locations, const std::map<std::string, CFileInfoVector>& fileList, std::set<std::string>& missingID, CCallback& callback)
	{
		ERMsg msg;


		string workingDir = GetDir(WORKING_DIR);
		callback.PushTask(string("Get missing ") + NETWORK_NAME[network] + " location (" + ToString(missingID.size()) + " stations)", missingID.size());
		callback.AddMessage(string("Get missing ") + NETWORK_NAME[network] + " location (" + ToString(missingID.size()) + " stations)");

		string filePath = workingDir + NETWORK_NAME[network] + "\\MissingStations.csv";
		CLocationVector missingLoc;

		if (FileExists(filePath))
			msg = missingLoc.Load(filePath);



		callback.AddMessage("Update missing stations information: ");
		for (set<string>::const_iterator it = missingID.begin(); it != missingID.end() && msg; it++)
		{
			callback.AddMessage(*it, 2);

			map<string, CFileInfoVector>::const_iterator it1 = fileList.find(*it);
			ASSERT(it1 != fileList.end());

			if (it1 != fileList.end())
			{
				string source;
				string URL = it1->second.front().m_filePath;
				msg = GetPageTextCurl("-s -k \"" + URL + "\"", source);

				if (msg)
				{
					WBSF::ReplaceString(source, "'", " ");

					CLocation location;
					msg = GetSWOBLocation(URL, source, location);
					if (msg)
					{
						location.SetSSI("IATA", *it);
						locations.push_back(location);
						missingLoc.push_back(location);
					}
				}


				ASSERT(locations.FindBySSI("IATA", *it, false) != locations.end());
			}

			msg += callback.StepIt();
		}


		missingLoc.Save(filePath);
		callback.PopTask();

		return msg;
	}*/

	CLocation CUIEnvCanHourly::GetMissingLocation(std::string filepath)
	{
		CLocation location;



		string URL = filepath;

		string p_network = GetNetwork(filepath);
		if (p_network != "yt-water" && p_network != "nl-water")
		{
			URL.clear();

			bool bLighthouse = Find(filepath, "dfo-ccg-lighthouse");
			string filter = bLighthouse ? "*.xml" : "*-AUTO-swob.xml";

			CFileInfoVector fileListTmp;
			if (FindFilesCurl(filepath + filter, fileListTmp) && !fileListTmp.empty())
			{
				URL = fileListTmp.front().m_filePath;
			}
		}


		if (!URL.empty())
		{

			string source;
			if (GetPageTextCurl("-s -k \"" + URL + "\"", source))
			{
				WBSF::ReplaceString(source, "'", " ");


				if (GetSWOBLocation(source, location))
				{
					if (p_network.empty())
					{
						//for SWOB, IATA ID it not exactly the same as in the metadata
						string IATA_ID = GetID(filepath);
						ASSERT(!IATA_ID.empty());
						location.SetSSI("IATA", IATA_ID);
					}
					else//SWOB Partners
					{
						ASSERT(location.m_ID == GetID(filepath));
					}
				}
			}//msg
		}

		return location;
	}

	ERMsg CUIEnvCanHourly::DownloadSWOB(size_t network, const CLocationVector& locations, const std::map<std::string, CFileInfoVector>& fileList, CCallback& callback)
	{
		ASSERT(network == N_SWOB || network == N_SWOB_PARTNERS);


		ERMsg msg;

		size_t nbDayStation = 0;
		for (map<string, CFileInfoVector>::const_iterator it1 = fileList.begin(); it1 != fileList.end(); it1++)
			nbDayStation += it1->second.size();


		string workingDir = GetDir(WORKING_DIR);
		string network_name = NETWORK_NAME[network];


		callback.PushTask("Download of " + network_name + " (" + ToString(fileList.size()) + " stations)", fileList.size());
		callback.AddMessage("Number of " + network_name + " stations to download: " + ToString(fileList.size()) + " (nb hours stations = " + ToString(nbDayStation) + ")");

		map<string, CTRef> lastUpdate;
		size_t nbDownload = 0;
		size_t nb_stations_updated = 0;
		for (map<string, CFileInfoVector>::const_iterator it1 = fileList.begin(); it1 != fileList.end() && msg; it1++)
		{
			string  IATA_ID = it1->first;


			auto it_location = locations.FindBySSI("IATA", IATA_ID);
			if (it_location == locations.end())
				it_location = locations.FindByID(IATA_ID);

			ASSERT(it_location != locations.end());
			string ID = it_location != locations.end() ? it_location->m_ID : IATA_ID;

			callback.PushTask("Download " + network_name + " for " + IATA_ID + ": (" + ToString(it1->second.size()) + " hours)", it1->second.size());

			map < CTRef, SWOBData > data;
			CTRef lastTRef;
			for (CFileInfoVector::const_iterator it2 = it1->second.begin(); it2 != it1->second.end() && msg; it2++)
			{
				string source;
				string URL = it2->m_filePath;
				msg = GetPageTextCurl("-s -k \"" + URL + "\"", source);


				if (msg)
				{
					WBSF::ReplaceString(source, "'", " ");

					bool bLighthouse = Find(it2->m_filePath, "dfo-ccg-lighthouse");
					string fileName = GetFileName(it2->m_filePath);
					CTRef UTCTRef = GetSWOBTRef(fileName, bLighthouse);
					CTRef YearMonth = UTCTRef.as(CTM::MONTHLY);

					if (data.find(YearMonth) == data.end())
					{
						//load data
						string filePath = GetOutputFilePath(network, "", UTCTRef.GetYear(), UTCTRef.GetMonth(), ID);
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
			}//for all hours

			ERMsg msgSaved;
			// save the job done event if they are not finished (error)
			for (auto it = data.begin(); it != data.end(); it++)
			{
				CTRef TRef = it->first;
				string filePath = GetOutputFilePath(network, "", TRef.GetYear(), TRef.GetMonth(), ID);

				CreateMultipleDir(GetPath(filePath));
				msgSaved += SaveSWOB(filePath, it->second);
			}

			if (msgSaved)
			{
				lastUpdate[ID] = lastTRef;
				nb_stations_updated++;

				//save last update at each 15 stations
				if ((nb_stations_updated % 15) == 0)
					msg += UpdateLastUpdate(network, lastUpdate);

			}
			else
			{

				callback.AddMessage(msgSaved);
			}

			callback.PopTask();
			msg += callback.StepIt();

		}//for all station

		callback.AddMessage("Number of " + network_name + " files downloaded: " + ToString(nbDownload));
		callback.PopTask();


		if (msg)
		{
			CTRef now = CTRef::GetCurrentTRef(CTM::HOURLY);
			if (network == N_SWOB)
			{
				CProvinceSelection selection(Get(PROVINCE));

				//select the oldest update for a selected province
				for (size_t p = 0; p < NB_PROVINCES; p++)
				{
					if (selection[p])
					{
						string p_network = "SWOB-" + CProvinceSelection::GetName(p);
						lastUpdate[p_network] = now;
					}
				}

			}
			else
			{
				string partners_network = Get(PARTNERS_NETWORK);
				if (partners_network.empty())
					partners_network = GetAllPartnersNetworkString();

				StringVector p_networks(partners_network, "|;,");
				for (size_t n = 0; n < p_networks.size(); n++)
				{
					lastUpdate[p_networks[n]] = now;
				}
			}
		}

		//save at the end including network update.
		msg += UpdateLastUpdate(network, lastUpdate);
		return msg;
	}


	string CUIEnvCanHourly::GetProvinceFormID(const string& ID)
	{
		string prov;
		if (ID.length() == 7)
		{

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
		}
		else
		{
			//Parter
			prov = ID.substr(0, 2);
			if (prov == "EC")
				prov = "ON";

			ASSERT(CProvinceSelection::GetProvince(prov) != NOT_INIT);
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
			for (zen::XmlIn child = in["om:member"]["om:Observation"]["om:metadata"]["set"]["identification-elements"]["element"]; child && msg; child.next())
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
							string name = UTF8_ANSI(WBSF::PurgeFileName(value));
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
				SWOBDataHour data_equivalent;


				static set<string> variables;
				zen::XmlDoc doc = zen::parse(source);


				zen::XmlIn in(doc.root());
				for (zen::XmlIn child = in["om:member"]["om:Observation"]["om:result"]["elements"]["element"]; child && msg; child.next())
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
						ReplaceString(QAStr, ",", ".");
						ASSERT(QAStr.find(',') == string::npos);//some qualifier have 2 elements separate by coma
						ASSERT(QAStr.find(';') == string::npos);

						size_t type = NOT_INIT;
						size_t type_equivalent = NOT_INIT;

						for (size_t i = 0; i < NB_SWOB_VARIABLES && type == NOT_INIT; i++)
							if (name == SWOB_VARIABLE_NAME[i])
								type = i;

						//if default name is missing, use other near equivalent variable


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
									callback.AddMessage("Other unit (" + unit + ") for variable: " + name + ". file: ");
								}
							}

						}
						else
						{
							if (name == "avg_air_temp_pst2mts")
								type_equivalent = SWOB_AIR_TEMP;
							else if (name == "avg_rel_hum_pst2mts")
								type_equivalent = SWOB_REL_HUM;
							else if (name == "avg_wnd_spd_10m_pst10mts")
								type_equivalent = SWOB_AVG_WND_SPD_10M_PST1HR;//does 10 minutes speed equivalent to one hour speed
							else if (name == "avg_wnd_dir_pst1hr" || name == "avg_wnd_dir_10m_pst10mts")
								type_equivalent = SWOB_AVG_WND_DIR_10M_PST1HR;
							else if (name == "avg_snw_dpth_pst5mts")
								type_equivalent = SWOB_SNW_DPTH;
							else if (name == "snw_dpth_wtr_equiv_1" || name == "snw_dpth_wtr_equiv_2")
								type_equivalent = SWOB_SNW_DPTH_WTR_EQUI;

							if (type_equivalent != NOT_INIT)
							{
								data_equivalent[0] = ToString(TRef.GetYear());
								data_equivalent[1] = ToString(TRef.GetMonth() + 1);
								data_equivalent[2] = ToString(TRef.GetDay() + 1);
								data_equivalent[3] = ToString(TRef.GetHour());
								data_equivalent[type_equivalent * 2 + 4] = value;
								data_equivalent[type_equivalent * 2 + 1 + 4] = QAStr;
							}
						}
					}
				}//for all attributes

				//replace element by equivalent if missing
				size_t eq_type[6] = { SWOB_AIR_TEMP, SWOB_REL_HUM, SWOB_AVG_WND_SPD_10M_PST1HR, SWOB_AVG_WND_DIR_10M_PST1HR, SWOB_SNW_DPTH, SWOB_SNW_DPTH_WTR_EQUI };

				for (size_t i = 0; i < 6; i++)
				{
					if (data[eq_type[i] * 2 + 4].empty() && !data_equivalent[eq_type[i] * 2 + 4].empty())
					{
						data[eq_type[i] * 2 + 4] = data_equivalent[eq_type[i] * 2 + 4];
						data[eq_type[i] * 2 + 1 + 4] = data_equivalent[eq_type[i] * 2 + 1 + 4];
						if (data[0].empty())
						{
							//just in case there is only equivalent values. Probably never append
							ASSERT(data[1].empty() && data[2].empty() && data[3].empty());
							ASSERT(!data_equivalent[0].empty() && !data_equivalent[1].empty() && !data_equivalent[2].empty() && !data_equivalent[3].empty());
							data[0] = data_equivalent[0];
							data[1] = data_equivalent[1];
							data[2] = data_equivalent[2];
							data[3] = data_equivalent[3];

						}

					}
				}

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
		bool bFredericton = station.GetSSI("IATA") == "CAFC";
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
					CSun sun(station.m_lat, station.m_lon);

					if (TRef.GetYear() >= firstYear && TRef.GetYear() <= lastYear)
					{
						for (size_t vv = 0; vv < NB_SWOB_VARIABLES; vv++)
						{
							TVarH v = VARIABLE_TYPE[vv];
							if (v != H_SKIP)
							{
								string strValue = swob[d][h][vv * 2 + 4];
								string strQA = swob[d][h][vv * 2 + 1 + 4];

								if (vv == SWOB_PCPN_AMT_PST1HR)//many partner station seem to have pcpn at 0 but have rnfl non zero
								{
									string str_rnfl = swob[d][h][SWOB_RNFL_AMT_PST1HR * 2 + 4];
									string str_str_rnfl_QA = swob[d][h][SWOB_RNFL_AMT_PST1HR * 2 + 1 + 4];

									//use max of pcpn and rnfl
									if (strValue.empty() || strValue == "MSNG" || strQA.empty())
									{
										if (!str_rnfl.empty() && str_rnfl != "MSNG" && str_str_rnfl_QA == "100")
										{
											//float rnfl = WBSF::as<float>(str_rnfl);
											//int rnfl_QA = WBSF::as<int>(str_str_rnfl_QA);

											//if (QAValue == 100 && rnfl_QA == 100 && value == 0 && rnfl > 0)
											//{
												//replace precipitation by rainfall
												//value = rnfl;
											//}
											strValue = str_rnfl;
											strQA = str_str_rnfl_QA;
										}
									}
									else if (strValue == "0.0" && strQA == "100" && str_str_rnfl_QA == "100")
									{
										//some network like bc-forest have precipitation at zero but have rainfall
										strValue = str_rnfl;
										strQA = str_str_rnfl_QA;
									}
								}

								if (vv == AVG_WND_SPD_PST2MTS && station[year][m][d][h][v] != WEATHER::MISSING)
								{
									//if 1 hour wind speed is already define, don't use 2 minutes
									strValue.clear();
								}
								if (vv == AVG_WND_DIR_PST2MTS && station[year][m][d][h][v] != WEATHER::MISSING)
								{
									//if 1 hour wind speed is already define, don't use 2 minutes
									strValue.clear();
								}


								if (!strValue.empty() && strValue != "MSNG" && !strQA.empty())
								{
									float value = WBSF::as<float>(strValue);
									int QAValue = WBSF::as<int>(strQA);
									if (v == H_SRAD && bFredericton && WBSF::as<float>(strValue) > 1000)//Fredericton some data 10 *????
										QAValue = -1;


									if (v == H_SRAD && strQA == "4")//Remove radiation when QA == 4, srad always equal zero
										QAValue = -1;

									bool bDaylight = TRef.GetHour() >= floor(sun.GetSunrise(TRef)) && TRef.GetHour() <= ceil(sun.GetSunset(TRef));
									if (v == H_SRAD && strQA == "0" && bDaylight)//Remove radiation when QA == 0 during daylight
										QAValue = -1;



									if (QAValue > 0)
									{

										if (v == H_SRAD && value < 0)
											value = 0;

										station[TRef].SetStat(v, value);
									}//if valid value
								}

							}//if it's a valid var
						}//for all variables

						if (station[TRef][H_ADD1].IsInit() && station[TRef][H_ADD2].IsInit())
						{
							CStatistic RH = station[TRef][H_ADD1] + station[TRef][H_ADD2];
							station[TRef].SetStat(H_RELH, RH[MEAN]);
						}

						station[TRef].SetStat(H_ADD1, CStatistic());
						station[TRef].SetStat(H_ADD1, CStatistic());

						if (station[TRef][H_TMIN].IsInit() && station[TRef][H_TMAX].IsInit() && !station[TRef][H_TAIR].IsInit())
						{
							CStatistic Tair = station[TRef][H_TMIN] + station[TRef][H_TMAX];
							station[TRef].SetStat(H_TAIR, Tair[MEAN]);
						}

						//when min/mean/max air temperature is not define, used the air_temp variable
						if (!station[TRef][H_TAIR].IsInit())
						{
							size_t vv = 1;
							//get the air_temp variable
							string strValue = swob[d][h][vv * 2 + 4];
							if (!strValue.empty() && strValue != "MSNG")
							{
								string strQA = swob[d][h][vv * 2 + 1 + 4];
								if (!strQA.empty())//is it valid or not?
								{
									int QAValue = WBSF::as<int>(strQA);

									if (QAValue == 100)
									{
										float value = WBSF::as<float>(strValue);
										station[TRef].SetStat(H_TMIN, value);
										station[TRef].SetStat(H_TAIR, value);
										station[TRef].SetStat(H_TMAX, value);
									}
								}
							}
						}

						//when relative humidity is missing, , used the rel_hum variable
						if (!station[TRef][H_RELH].IsInit())
						{
							size_t vv = 2;
							//get the air_temp variable
							string strValue = swob[d][h][vv * 2 + 4];
							if (!strValue.empty() && strValue != "MSNG")
							{
								string strQA = swob[d][h][vv * 2 + 1 + 4];
								if (!strQA.empty())//is it valid or not?
								{
									int QAValue = WBSF::as<int>(strQA);

									if (QAValue == 100)
									{
										float value = WBSF::as<float>(strValue);
										station[TRef].SetStat(H_RELH, value);
									}
								}
							}
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
				if (loop->size() >= 4)//when data finish with empty column, the last column is removed
				{
					if (!(*loop)[0].empty() && !(*loop)[1].empty() && !(*loop)[2].empty() && !(*loop)[3].empty())
					{
						size_t d = WBSF::as<size_t>((*loop)[2]) - 1;
						size_t h = WBSF::as<size_t>((*loop)[3]);


						ASSERT(d < 31);
						ASSERT(h < 24);
						//file must be remove
						//ASSERT(loop->size() == (NB_SWOB_VARIABLES * 2 + 4)|| loop->size() == (NB_SWOB_VARIABLES * 2 + 4-1));

						size_t NEW_SWE_COL = 4 * 2 + 4;

						for (size_t i = 0; i < loop->size() && i < (NB_SWOB_VARIABLES * 2 + 4); i++)
						{
							//variable replace 2022-08-04 to avoid changing the number of columns
							//set this variable only of the column in the file contain the good variable
							if (i != NEW_SWE_COL || loop.Header()[NEW_SWE_COL] == SWOB_VARIABLE_NAME[4])//Replace wind unused var by SWE
								data[d][h][i] = (*loop)[i];
						}

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
					if (!data[d][h][0].empty() && !data[d][h][1].empty() && !data[d][h][2].empty() && !data[d][h][3].empty())
					{
						for (size_t i = 0; i < data[d][h].size(); i++)
						{
							if (i > 0)
								ofile << ",";

							ASSERT(data[d][h][i].find(',') == string::npos);
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

	ERMsg CUIEnvCanHourly::UpdateLastUpdate(size_t network, const map<string, CTRef>& lastUpdate)
	{
		ERMsg msg;
		string workingDir = GetDir(WORKING_DIR);
		string network_name = NETWORK_NAME[network];
		string lastUpdateFilePath = workingDir + network_name + "\\LastUpdate.csv";



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
			for (size_t t = 0; t < 5 && !msg; t++)
			{
				//wait 10 second and retry
				WaitServer(10);//remove error
				msg = ofile.open(lastUpdateFilePath);
			}

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