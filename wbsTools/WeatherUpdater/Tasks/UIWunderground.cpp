#include "StdAfx.h"
#include "UIWunderground.h"

#include <boost/algorithm/string.hpp>
#include "basic/WeatherStation.h"
#include "basic/CSV.h"
#include "Basic/FileStamp.h"
#include "UI/Common/SYShowMessage.h"
//#include "UI/LocEditDlg.h"

#include "TaskFactory.h"
#include "../Resource.h"
#include "WeatherBasedSimulationString.h"
#include "CountrySelection.h"
#include "StateSelection.h"
#include "ProvinceSelection.h"
#include "json\json11.hpp"

static const bool UPDATE_STATIONS_LIST = false;

using namespace WBSF::HOURLY_DATA;
using namespace std;
using namespace UtilWWW;
using namespace json11;

namespace WBSF
{
	//to update data in csv file
	//https://www.wunderground.com/weatherstation/WXDailyHistory.asp?ID=IQUEBECS44&day=1&month=1&year=2015&dayend=31&monthend=12&yearend=2015&graphspan=custom&format=1
	//https://www.wunderground.com/weatherstation/WXDailyHistory.asp?ID=IQUEBECS44&day=1&month=1&year=2015&graphspan=day&format=1



	//pour la liste des stations actives:
	//http://www.wunderground.com/weatherstation/ListStations.asp?showall=&start=20&selectedState=
	//*********************************************************************

	static const DWORD FLAGS = INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_PRAGMA_NOCACHE;//| INTERNET_FLAG_TRANSFER_BINARY
	const char* CUIWunderground::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "FirstYear", "LastYear", "DataType", /*"Countries", "States", "Province",*/ "StationList", "UpdateMissing", /*, "GoldStar"*/ };//"UpdateStationList" 
	const size_t CUIWunderground::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_STRING, T_STRING, T_COMBO_INDEX , /*T_STRING_SELECT, T_STRING_SELECT, T_STRING_SELECT,*/ T_STRING, T_BOOL };/// 
	const UINT CUIWunderground::ATTRIBUTE_TITLE_ID = IDS_UPDATER_WU_P;
	const UINT CUIWunderground::DESCRIPTION_TITLE_ID = ID_TASK_WU;

	const char* CUIWunderground::CLASS_NAME() { static const char* THE_CLASS_NAME = "WeatherUnderground";  return THE_CLASS_NAME; }
	CTaskBase::TType CUIWunderground::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUIWunderground::CLASS_NAME(), (createF)CUIWunderground::create);


	const char* CUIWunderground::SERVER_NAME = "www.wunderground.com";

	const char* CUIWunderground::get_type_name(size_t type) { return (type == HOURLY_WEATHER) ? "hourly" : "daily"; }

	//WU-History-inc.php
	static const char pageFormat1[] = "weatherstation/ListStations.asp?showall=&start=%s&selectedState=%s";
	static const char pageFormat2[] = "weatherstation/ListStations.asp?selectedCountry=%s";

	CUIWunderground::CUIWunderground(void)
	{}


	CUIWunderground::~CUIWunderground(void)
	{}


	std::string CUIWunderground::Option(size_t i)const
	{
		string str;
		switch (i)
		{
		case DATA_TYPE:	str = GetString(IDS_STR_DATA_TYPE); break;
			//case COUNTRIES:	str = CCountrySelectionWU::GetAllPossibleValue(); break;
			//case STATES:	str = CStateSelection::GetAllPossibleValue(); break;
			//case PROVINCE:	str = CProvinceSelection::GetAllPossibleValue(); break;
		};
		return str;
	}

	std::string CUIWunderground::Default(size_t i)const
	{
		string str;

		switch (i)
		{
		case WORKING_DIR: str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()) + "WeatherUnderground\\"; break;
		case FIRST_YEAR:
		case LAST_YEAR:	str = ToString(CTRef::GetCurrentTRef().GetYear()); break;
		case DATA_TYPE: str = "0"; break;
		case UPDATE_MISSING: str = "1"; break;
			//case GOLD_STAR: str = "0"; break;
		};
		return str;
	}

	std::string CUIWunderground::GetStationListFilePath(const string& country)const
	{
		string filepath = GetApplicationPath() + "Layers\\";
		if (country == "US")
			filepath += "WUStationsListUSA.csv";
		else if (country == "CA")
			filepath += "WUStationsListCanada.csv";
		else
			filepath += "WUStationsList.csv";


		return filepath;
	}


	std::string CUIWunderground::GetOutputFilePath(const string& type, const string& country, const string& states, const std::string& ID, int year)
	{
		string workingDir = GetDir(WORKING_DIR);
		string ouputPath = workingDir + type + "\\" + to_string(year) + "\\";

		if (country == "US")
			ouputPath += "USA\\" + states + "\\";
		else if (country == "CA")
			ouputPath += "Canada\\" + states + "\\";
		else
			ouputPath += "World\\" + country + "\\";


		return ouputPath + ID + ".csv";
	}

	/*string Purge(string str)
	{
		MakeUpper(str);
		ReplaceString(str, "ST.", "ST-");
		ReplaceString(str, ",", "");
		ReplaceString(str, ";", "");
		ReplaceString(str, "\"", "");
		Trim(str);

		return str;
	}

	string PurgeProvince(string str)
	{
		enum { NB_NAMES = 32 };
		static const char* MAP_NAME[NB_NAMES][2] =
		{
			{ "AL", "AB" },
			{ "ALBERTA", "AB" },
			{ "AB, CANADA.", "AB" },
			{ "B.C.", "BC" },
			{ "BRITISCH-KOLUMBIEN", "BC" },
			{ "BRITISH COLUMBIA", "BC" },
			{ "MANITOBA", "MB" },
			{ "NEW BRUNSWICK", "NB" },
			{ "NEW-BRUNSWICK", "NB" },
			{ "NOUVEAU-BRUNSWICK", "NB" },
			{ "NEWFOUNDLAND", "NL" },
			{ "NEWFOUNDLAND AND LABRADOR", "NL" },
			{ "NORTHWEST TERRITORIES", "NT" },
			{ "NOVA SCOTIA", "NS" },
			{ "NUNAVUT", "NU" },
			{ "ONTARIO", "ON" },
			{ "ONTSRIO", "ON" },
			{ "ONTARIO, CANADA", "ON" },
			{ "PEI", "PE" },
			{ "PRINCE EDWARD ISLAND", "PE" },
			{ "PQ", "QC" },
			{ "QUÃ©BEC", "QC" },
			{ "QUEBEC", "QC" },
			{ "QUÉBEC", "QC" },
			{ "QUÏ¿½BEC", "QC" },
			{ "SASK", "SA" },
			{ "SASKATCHEWAN", "SA" },
			{ "SASKATCHEWAN, CANADA", "SA" },
			{ "SK", "SA" },
			{ "SK.", "SA" },
			{ "YUKON", "YT" },
			{ "YUKON TERRITORY", "YT" },
		};

		for (size_t i = 0; i < NB_NAMES; i++)
			if (str == MAP_NAME[i][0])
				str = MAP_NAME[i][1];


		return str;
	}
*/

//ERMsg CUIWunderground::DownloadStationList(const string& country, CLocationVector& stationList2, CCallback& callback)const
//{
//	ERMsg msg;

//	CLocationVector stationList1;

//	CInternetSessionPtr pSession;
//	CHttpConnectionPtr pConnection;

//	msg += GetHttpConnection("www.wunderground.com", pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", false, 5, callback);

//	if (msg)
//	{
//		string URL = "/weatherstation/ListStations.asp?selectedCountry=" + country;

//		string source;
//		msg = GetPageText(pConnection, URL, source, false, NULL);
//		if (msg)
//		{

//			string::size_type b = source.find("<tbody>");
//			string::size_type e = source.find("</tbody>");
//			callback.PushTask(GetString(IDS_LOAD_STATION_LIST) + " (Canada)", e);
//			callback.SetCurrentStepPos(b);

//			string::size_type posBegin = source.find("<tr>", b);
//			while (posBegin != string::npos)
//			{
//				CLocation loc;
//				loc.m_ID = Purge(FindString(source, "?ID=", "\">", posBegin));
//				if (posBegin != string::npos)
//					loc.m_name = UppercaseFirstLetter(ANSI_2_ASCII(Purge(FindString(source, "<td>", "&nbsp;", posBegin))));

//				if (posBegin != string::npos)
//					loc.SetSSI("City", UppercaseFirstLetter(ANSI_2_ASCII(Purge(FindString(source, "<td>", "&nbsp;", posBegin)))));

//				if (posBegin != string::npos)
//					loc.SetSSI("StationType", UppercaseFirstLetter(Purge(FindString(source, "<td class=\"station-type\">", "&nbsp;", posBegin))));

//				if (loc.m_name.empty())
//					loc.m_name = loc.GetSSI("City");

//				if (loc.m_name.empty())
//					loc.m_name = loc.m_ID;

//				bool bNetatmo = WBSF::Find("Netatmo", loc.GetSSI("StationType"), false);
//				if (!bNetatmo && stationList2.FindByID(loc.m_ID) == NOT_INIT)
//					stationList1.push_back(loc);


//				posBegin = source.find("<tr>", posBegin);
//				msg += callback.SetCurrentStepPos(posBegin);
//			}

//			callback.PopTask();


//			//get station information
//			callback.PushTask(GetString(IDS_LOAD_STATION_LIST) + " (Canada) : " + ToString(stationList1.size()) + " stations", stationList1.size());
//			for (size_t i = 0; i < stationList1.size() && msg; i++)
//			{
//				CLocation station = stationList1[i];

//				URL = "/personal-weather-station/dashboard?ID=" + station.m_ID;
//				string source;
//				msg = GetPageText(pConnection, URL, source, false, NULL);
//				if (msg)
//				{
//					bool bGoldStart = source.find("pws-goldstar-27x21.png") != string::npos;
//					station.SetSSI("GoldStar", bGoldStart ? "1" : "0");

//					string::size_type posBegin = source.find("id=\"forecast-link\">");
//					if (posBegin != string::npos)
//					{
//						string str = FindString(source, "</a><span class=\"subheading\">", "</span>", posBegin);
//						StringVector v(str, "> ");
//						ASSERT(v.size() >= 3);

//						if (v.size() >= 3)
//						{
//							station.m_lat = WBSF::as<double>(v[0]);
//							station.m_lon = WBSF::as<double>(v[1]);

//							if (v.size() == 4 && v[2] != "-999")
//							{
//								if (v[3] == "m")
//									station.m_elev = WBSF::as<double>(v[2]);
//								else if (v[3] == "ft")
//									station.m_elev = WBSF::Feet2Meter(WBSF::as<double>(v[2]));
//							}

//							if (station.m_elev >= -1000)//station with strange elevation are usually bad
//								stationList2.push_back(station);
//						}//if msg
//					}//if have data

//				}//if msg

//				callback.StepIt();
//			}//for all station


//			callback.PopTask();
//		}//if msg

//		callback.AddMessage(GetString(IDS_NB_STATIONS) + ToString(stationList1.size()));

//		pConnection->Close();
//		pSession->Close();

//		//pGoogleConnection->Close();
//		//pGoogleSession->Close();
//	}//if msg

//	return msg;
//}

//ERMsg CUIWunderground::CompleteStationList(CLocationVector& stationList, CCallback& callback)const
//{
//	//ERMsg msg;

//	return CLocDlg::ExtractNominatimName(stationList, false, true, true, true, callback);

//	//CInternetSessionPtr pGoogleSession;
//	//CHttpConnectionPtr pGoogleConnection;

//	//if (msg)
//	//	msg += GetHttpConnection("maps.googleapis.com", pGoogleConnection, pGoogleSession, PRE_CONFIG_INTERNET_ACCESS, "", "", false, 5, callback);

//	//if (msg)
//	//{
//	//	//get station information
//	//	callback.PushTask(GetString(IDS_LOAD_STATION_LIST) + " (Canada) : " + ToString(stationList2.size()) + " stations", stationList2.size());
//	//	for (size_t i = 0; i < stationList2.size() && msg; i++)
//	//	{
//	//		CLocation& station = stationList2[i];

//	//		if (station.m_elev == -999)
//	//		{
//	//			string elevFormat = "/maps/api/elevation/json?locations=" + ToString(station.m_lat) + "," + ToString(station.m_lon);
//	//			string strElev;
//	//			msg = UtilWWW::GetPageText(pGoogleConnection, elevFormat, strElev);
//	//			if (msg)
//	//			{
//	//				//extract elevation from google
//	//				string error;
//	//				Json jsonElev = Json::parse(strElev, error);
//	//				ASSERT(jsonElev.is_object());

//	//				if (error.empty() && jsonElev["status"] == "OK")
//	//				{
//	//					ASSERT(jsonElev["results"].is_array());
//	//					Json::array result = jsonElev["results"].array_items();
//	//					ASSERT(result.size() == 1);

//	//					station.m_elev = result[0]["elevation"].number_value();
//	//				}
//	//			}//if msg
//	//		}



//	//		//
//	//		if (station.GetSSI("Country").empty())
//	//		{
//	//			string geoFormat = "/maps/api/geocode/json?latlng=" + ToString(station.m_lat) + "," + ToString(station.m_lon);
//	//			string strGeo;


//	//			msg = UtilWWW::GetPageText(pGoogleConnection, geoFormat, strGeo);
//	//			if (msg)
//	//			{
//	//				//extract elevation from google
//	//				string error;
//	//				Json jsonGeo = Json::parse(strGeo, error);
//	//				ASSERT(jsonGeo.is_object());

//	//				if (error.empty() && jsonGeo["status"] == "OK")
//	//				{
//	//					ASSERT(jsonGeo["results"].is_array());
//	//					Json::array result1 = jsonGeo["results"].array_items();
//	//					if (!result1.empty())
//	//					{
//	//						Json::array result2 = result1[0]["address_components"].array_items();
//	//						for (int j = 0; j < result2.size(); j++)
//	//						{
//	//							Json::array result3 = result2[j]["types"].array_items();
//	//							if (result3.size() == 2 && result3[0] == "administrative_area_level_2")
//	//							{
//	//								string str = ANSI_2_ASCII(result2[j]["short_name"].string_value());
//	//								WBSF::ReplaceString(str, ",", " ");
//	//								station.SetSSI("County", str);
//	//							}

//	//							if (result3.size() == 2 && result3[0] == "administrative_area_level_1")
//	//								station.SetSSI("State", ANSI_2_ASCII(result2[j]["short_name"].string_value()));

//	//							if (result3.size() == 2 && result3[0] == "country")
//	//								station.SetSSI("Country", ANSI_2_ASCII(result2[j]["short_name"].string_value()));

//	//						}
//	//					}
//	//				}
//	//			}//if msg
//	//		}

//	//		msg += callback.StepIt();
//	//	}//for all station

//	//}//if open connection


//	//callback.PopTask();

//	//pGoogleConnection->Close();
//	//pGoogleSession->Close();


//	//return msg;

//}



//TVarH CUIWunderground::GetVar(const string& str)
//{
//	TVarH v = H_SKIP;

//	enum { NB_VARS = 7 };
//	static const char* VAR_NAME[NB_VARS] = { "Temp.:", "DewPoint:", "Humidity:", "WindSpeed:", "Pressure:", "Precipitation:", "SolarRadiation:" };
//	static const TVarH VAR_AVAILABLE[NB_VARS] = { H_TAIR, H_TDEW, H_RELH, H_WNDS, H_PRES, H_PRCP, H_SRAD };
//	for (size_t vv = 0; vv < NB_VARS&&v == H_SKIP; vv++)
//	{
//		if (IsEqual(str, VAR_NAME[vv]))
//			v = VAR_AVAILABLE[vv];
//	}

//	return v;
//}

//CWVariables CUIWunderground::GetVariables(string str)
//{
//	//size_t v_count = 0;
//	//size_t e_count = 0;

//	WBSF::CWVariables variables;



//	boost::erase_all(str, "\r");
//	boost::erase_all(str, "\n");
//	boost::erase_all(str, "\t");
//	boost::erase_all(str, " ");
//	string::size_type posBegin = str.find("<tr>");


//	//static const TVarH VAR_AVAILABLE[7] = {H_TAIR, H_TDEW, H_RELH, H_WNDS, H_SKIP, H_PRES, H_PRCP};

//	while (posBegin != string::npos)
//	{
//		posBegin += 4;


//		string str1 = FindString(str, "<td>", "</td>", posBegin);
//		string str2 = FindString(str, "<td>", "</td>", posBegin);
//		string str3 = FindString(str, "<td>", "</td>", posBegin);

//		if (str1 != "UVIndex:" && str1 != "WindGust:")
//		{
//			size_t v = GetVar(str1);
//			ASSERT(v != NOT_INIT);
//			if (v != NOT_INIT && ((str2 != "-" && str2 != "" && str2 != "%") || (str3 != "-" && str3 != "" && str3 != "%")))
//				variables.set(v);
//		}

//		posBegin = str.find("<tr>", posBegin);
//	}

//	ASSERT(str.find("<tr>", posBegin) == string::npos);

//	return variables;
//}


//void CUIWunderground::CleanString(string& str)
//{
//	string output;

//	ReplaceString(str, "'", "");
//	ReplaceString(str, "&deg;", "");
//	ReplaceString(str, "S ", "-");
//	ReplaceString(str, "W ", "-");
//	ReplaceString(str, "N ", "");
//	ReplaceString(str, "E ", "");

//	Trim(str);
//}

/*double CUIWunderground::GetCoordinate(string str)
{
	CleanString(str);

	float ld = 0, lm = 0, ls = 0;
	sscanf(str.c_str(), "%f %f %f", &ld, &lm, &ls);

	return ld + Signe(ld)*(lm / 60.0 + ls / 3600.0);
}*/

//ERMsg CUIWunderground::ExtractElevation(CLocationVector& stationList, CCallback& callback)
//{
//	ERMsg msg;

//	CInternetSessionPtr pSession;
//	CHttpConnectionPtr pConnection;
//	msg += GetHttpConnection("i.wund.com", pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", false, 5, callback);

//	if (!msg)
//		return msg;



//	//Get elevation
//	callback.PushTask("Extract elevation", stationList.size());

//	for (size_t i = 0; i < stationList.size() && msg; i++)
//	{
//		string URL = FormatA("weatherstation/WXDailyHistory.asp?ID=%s&month=4&day=9&year=2016", stationList[i].m_ID.c_str());

//		string source;
//		msg = GetPageText(pConnection, URL, source);
//		if (msg)
//		{
//			//http://i.wund.com/auto/iphone/weatherstation/WXDailyHistory.asp?ID=KCATUSTI15&month=4&day=9&year=2016

//			string variables;
//			string::size_type posBegin = source.find("Statistics for the rest of the month");

//			if (posBegin != string::npos)
//				variables = FindString(source, "<tbody>", "</tbody>", posBegin);

//			WBSF::CWVariables v = GetVariables(variables);
//			if (v.any())
//				stationList[i].SetSSI("Variables", v.to_string());

//			string lat = TrimConst(FindString(source, "Lat:</span>", "''"));
//			if (!lat.empty())
//				stationList[i].m_lat = GetCoordinate(lat);

//			string lon = TrimConst(FindString(source, "Lon:</span>", "''"));
//			if (!lon.empty())
//				stationList[i].m_lon = GetCoordinate(lon);

//			string alt = TrimConst(FindString(source, "Elevation:</span>", "ft"));
//			if (!alt.empty())
//				stationList[i].m_elev = ToDouble(alt) * 0.3048;
//		}

//		msg += callback.StepIt();

//		if (((i + 1) % 200) == 0)
//			stationList.Save("E:\\Travaux\\Install\\DemoBioSIM\\Update\\WeatherUnderground\\test.csv", ',', callback);
//	}

//	//now update coordinate
//	stationList.Save("E:\\Travaux\\Install\\DemoBioSIM\\Update\\WeatherUnderground\\test.csv", ',', callback);

//	callback.PopTask();

//	pConnection->Close();
//	pSession->Close();

//	return msg;
//}

	ERMsg GetAPIKey(string& apiKey)
	{
		ERMsg msg;

		//https://
		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;
		if (GetHttpConnection("www.wunderground.com", pConnection, pSession))
		{

			//string URL = FormatA(URL_FORMAT, stationList[i].m_ID.c_str(), d + 1, m + 1, year);

			string source;
			msg = GetPageText(pConnection, "weather/CYQB", source, false, FLAGS);

			if (!source.empty())
			{
				apiKey = FindString(source, "apiKey=", "&");
				ASSERT(apiKey.length() == 32);
			}

			pConnection->Close();
			pSession->Close();

		}

		return msg;
	}

	//******************************************************
	ERMsg CUIWunderground::Execute(CCallback& callback)
	{
		ERMsg msg;


		string workingDir = GetDir(WORKING_DIR);
		CreateMultipleDir(workingDir);

		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(workingDir, 1);
		callback.AddMessage(GetString(IDS_UPDATE_FROM));
		callback.AddMessage(SERVER_NAME, 1);
		callback.AddMessage("");


		//CCountrySelection countries(Get(COUNTRIES));
		//CStateSelection states(Get(STATES));
		//CProvinceSelection provinces(Get(PROVINCE));
		size_t type = as<size_t>(DATA_TYPE);
		//bool bUseGoldStarOnly = as<bool>(GOLD_STAR);

		string apiKey;
		msg = GetAPIKey(apiKey);
		if (!msg)
			return msg;


		//if (as<bool>(UPDATE_STATION_LIST))
		//if (UPDATE_STATIONS_LIST)
		//{

			/*
			CCountrySelection countries(Get(COUNTRIES));
			callback.PushTask("Update stations list (" + ToString(countries.count()) + " countries)", countries.count());

			for (size_t i = 0; i < countries.size() && msg; i++)
			{
				if (countries[i])
				{
					string ID = CCountrySelection::GetName(i);

					CLocationVector stationList;
					if (FileExists(GetStationListFilePath(ID)))
						msg = stationList.Load(GetStationListFilePath(ID));

					msg = DownloadStationList(ID, stationList, callback);

					if (msg)
						msg = CompleteStationList(stationList, callback);

					if (msg)
						msg = stationList.Save(GetStationListFilePath(ID), ',', callback);

					msg += callback.StepIt();
				}
			}

			callback.PopTask();

			if (!msg)
				return msg;
				*/


				//}



		CLocationVector stationList;
		msg = LoadStationList(apiKey, stationList, callback);


		//Get station
		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = lastYear - firstYear + 1;
		int currentYear = GetCurrentYear();
		CTRef today = CTRef::GetCurrentTRef();

		string tstr = get_type_name(type);

		callback.AddMessage("Download " + tstr + " WeatherUnderground stations data (" + ToString(stationList.size()) + ")");
		callback.PushTask("Download " + tstr + " WeatherUnderground stations data (" + ToString(stationList.size()) + ")", stationList.size()*nbYears);

		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;
		msg += GetHttpConnection("api.weather.com", pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS);

		if (!msg)
			return msg;


		size_t nbDownload = 0;
		for (size_t i = 0; i < stationList.size(); i++)
		{
			for (size_t y = 0; y < nbYears&&msg; y++)
			{
				int year = firstYear + int(y);
				double inc = 1;

				string ouputFilePath = GetOutputFilePath(tstr, stationList[i].GetSSI("Country"), stationList[i].GetSSI("State"), stationList[i].m_ID, year);
				CreateMultipleDir(GetPath(ouputFilePath));

				if (NeedDownload(year, ouputFilePath))
				{
					CWeatherYears data(type == HOURLY_WEATHER);
					data.LoadData(ouputFilePath);

					CTRef TRef = CWeatherYears::GetLastTref(ouputFilePath);
					inc = 1.0 / (year == currentYear ? (today.GetMonth() - TRef.GetMonth() + 1) : 12);

					size_t fisrt_m = TRef.IsInit() ? TRef.GetMonth() : FIRST_MONTH;
					size_t last_m = year == currentYear ? today.GetMonth() : LAST_MONTH;
					for (size_t m = fisrt_m; m <= last_m && msg; m++)
					{
						const char* str_type = get_type_name(type);
						static const char* URL_FORMAT = "v2/pws/history/%s?stationId=%s&format=json&units=m&startDate=%d%02d%02d&endDate=%d%02d%02d&numericPrecision=decimal&apiKey=%s";
						string URL = FormatA(URL_FORMAT, str_type, stationList[i].m_ID.c_str(), year, m + 1, TRef.GetDay() + 1, year, m + 1, GetNbDayPerMonth(year, m), apiKey.c_str());

						try
						{
							string source;
							msg = GetPageText(pConnection, URL, source, false, FLAGS);

							if (!source.empty())
							{
								msg += ParseJson(type, source, data, callback);
								nbDownload++;
								msg += callback.StepIt(inc);
							}

						}
						catch (CException* e)
						{
							//if an error occur: try again
							msg = UtilWin::SYGetMessage(*e);
						}
					}//for all months


					if (type == DAILY_WEATHER)
					{
						//remove today observation (incomplete)
						if (year == today.GetYear())
							data[today].Reset();
					}

					msg = data.SaveData(ouputFilePath);

				}//need download
			}//for all years
		}//for all stations

		pConnection->Close();
		pSession->Close();


		callback.AddMessage(GetString(IDS_NB_FILES_DOWNLOADED) + ToString(nbDownload), 2);
		callback.PopTask();




		return msg;
	}


	bool CUIWunderground::NeedDownload(int year, const string& ouputFilePath)
	{

		bool bDownload = true;

		CFileStamp fileStamp(ouputFilePath);
		CTime lu = fileStamp.m_time;
		if (lu.GetTime() > 0)
		{
			int nbDays = CTRef(lu.GetYear(), lu.GetMonth() - 1, lu.GetDay() - 1) - CTRef(year, LAST_MONTH, LAST_DAY);

			if (nbDays > 7)
				bDownload = false;
		}

		return bDownload;
	}


	ERMsg CUIWunderground::LoadStationList(const string& apiKey, CLocationVector& stationList, CCallback& callback)const
	{
		ERMsg msg;

		stationList.clear();

		StringVector stationListID(TrimConst(Get(STATION_LIST)), " ;,|");

		//CLocationVector stationList1;

		//CCountrySelection countries(Get(COUNTRIES));
		//CStateSelection states(Get(STATES));
		//CProvinceSelection provinces(Get(PROVINCE));
		//bool bUseGoldStarOnly = as<bool>(GOLD_STAR);


		//CLocationVector tmp;
		//size_t n = 0;
		//if (/*countries.at("US") ||*/ !stationListID.empty())
		//{
		//msg = tmp.Load(GetStationListFilePath("US"), ",", callback);
		//stationList1.insert(stationList1.end(), tmp.begin(), tmp.end());
		//	n++;
		//}

		//if (/*countries.at("CA") ||*/ !stationListID.empty())
		//{
		//msg = tmp.Load(GetStationListFilePath("CA"), ",", callback);
		//stationList1.insert(stationList1.end(), tmp.begin(), tmp.end());
		//	n++;
		//}

		//if (/*countries.count() > n ||*/ !stationListID.empty())//other country than US and CA
		//{

		CLocationVector locations;
		if (FileExists(GetStationListFilePath("")))
			msg += locations.Load(GetStationListFilePath(""), ",", callback);
		// (tmp.Load(GetStationListFilePath(""), ",", callback))
			//stationList1.insert(stationList1.end(), tmp.begin(), tmp.end());

		//	n++;
		//}

		StringVector toUpdated;
		for (auto it = stationListID.begin(); it != stationListID.end() && msg; it++)
		{
			size_t pos = locations.FindByID(*it);
			if (pos == NOT_INIT)
				toUpdated.push_back(*it);
		}

		bool bUpdateMissing = as<bool>(UPDATE_MISSING);
		if (!toUpdated.empty() && !apiKey.empty())
		{
			if (bUpdateMissing)
			{
				callback.PushTask("Update WeatherUnderground unknowns metadata: " + to_string(toUpdated.size()) + " stations", toUpdated.size());
				callback.AddMessage("Update WeatherUnderground unknowns metadata: " + to_string(toUpdated.size()) + " stations");

				//CLocationVector locations;
				//if (FileExists(GetStationListFilePath("")))
					//msg += locations.Load(GetStationListFilePath(""), ",", callback);

				size_t nbUpdated = 0;
				for (auto it = toUpdated.begin(); it != toUpdated.end() && msg; it++)
				{
					//https://api.weather.com/v2/pwsidentity?apiKey=6532d6454b8aa370768e63d6ba5a832e&stationId=ISAINTAL79&format=json&units=m

					CInternetSessionPtr pSession;
					CHttpConnectionPtr pConnection;
					msg += GetHttpConnection("api.weather.com", pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS);

					if (msg)
					{

						static const char* URL_FORMAT = "v2/pwsidentity?apiKey=%s&stationId=%s&format=json&units=m";
						string URL = FormatA(URL_FORMAT, apiKey.c_str(), it->c_str());

						try
						{
							string source;
							msg = GetPageText(pConnection, URL, source, false, FLAGS);

							if (!source.empty())
							{
								CLocation location;
								msg += ParseJsonMetadata(source, location, callback);
								
								//msg += callback.StepIt(inc);

								if (msg)
								{
									if (location.m_name.empty() || location.GetSSI("County").empty() || location.GetSSI("State").empty() || location.GetSSI("Country").empty())
									{
										ExtractNominatimName(location, callback);
									}

									nbUpdated++;
									locations.push_back(location);
								}
							}
						}
						catch (CException* e)
						{
							//if an error occur: try again
							msg = UtilWin::SYGetMessage(*e);
						}


						pConnection->Close();
						pSession->Close();



					}//if msg
				}


				msg += locations.Save(GetStationListFilePath(""), ',', callback);

				callback.AddMessage("Unknowns WeatherUnderground metadata updated: " + to_string(nbUpdated) + " stations");
				callback.PopTask();
			}
			else
			{
				callback.AddMessage("Some stations metadata is missing. Activate \"Update missing metadata\" to update metadata");
			}

		}//update metadata


		stationList.reserve(stationListID.size());

		for (auto it = stationListID.begin(); it != stationListID.end() && msg; it++)
		{
			size_t pos = locations.FindByID(*it);
			if (pos != NOT_INIT)
			{
				stationList.insert(stationList.end(), locations[pos]);
			}
			else
			{
				callback.AddMessage("Warning: Stations with ID " + *it + " is not in coordinate station's list");
			}

		}
		//}
		//}
		//else
		//{
		//	callback.PushTask("Clean list (" + ToString(stationList1.size()) + ")", stationList1.size());
		//	stationList.reserve(stationList1.size());
		//	for (auto it = stationList1.begin(); it != stationList1.end() && msg; it++)
		//	{
		//		bool bDownload = false;
		//		//bool bGoldStart = WBSF::as<bool>(it->GetSSI("GoldStar"));

		//		//if (!bUseGoldStarOnly || bGoldStart)
		//		{
		//			string country = it->GetSSI("Country");


		//			if (countries.at(country))
		//			{
		//				string state = it->GetSSI("State");
		//				if (country == "US")
		//				{
		//					if (states.at(state))
		//						bDownload = true;
		//				}
		//				else if (country == "CA")
		//				{
		//					if (provinces.at(state))
		//						bDownload = true;
		//				}
		//				else
		//				{
		//					bDownload = true;
		//				}

		//			}//if country selection

		//		}

		//		if (bDownload)
		//			stationList.insert(stationList.end(), *it);

		//		msg += callback.StepIt();
		//	}


		//	callback.PopTask();
		//}




		return msg;
	}

	ERMsg CUIWunderground::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;


		msg = LoadStationList("", m_stations, callback);

		if (msg)
		{
			for (CLocationVector::const_iterator it = m_stations.begin(); it != m_stations.end(); it++)
			{
				stationList.push_back(it->m_ID);
			}
		}

		return msg;
	}


	ERMsg CUIWunderground::GetWeatherStation(const string& ID, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;

		size_t type = as<size_t>(DATA_TYPE);
		if (TM.IsHourly() && type == DAILY_WEATHER)
		{
			msg.ajoute("Cannot create hourly database on daily data");
			return msg;
		}


		size_t pos = m_stations.FindByID(ID);
		if (pos == NOT_INIT)
		{
			msg.ajoute(FormatMsg(IDS_NO_STATION_INFORMATION, ID));
			return msg;
		}

		((CLocation&)station) = m_stations[pos];
		if (station.m_name.empty())
			station.m_name = station.GetSSI("City");

		station.m_name = WBSF::PurgeFileName(station.m_name);

		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = size_t(lastYear - firstYear + 1);
		station.CreateYears(firstYear, nbYears);



		station.SetHourly(type == HOURLY_WEATHER);

		//now extract data 
		for (size_t y = 0; y < nbYears&&msg; y++)
		{
			int year = firstYear + int(y);
			string filePath = GetOutputFilePath(get_type_name(type), station.GetSSI("Country"), station.GetSSI("State"), ID, year);
			if (FileExists(filePath))
			{
				msg += station.LoadData(filePath, -999, false);
				//if (type == HOURLY_WEATHER)
					//msg = ReadHourlyData(filePath, station, callback);
				//else
					//msg = ReadDailyData(filePath, station, callback);

				msg += callback.StepIt(0);
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

	ERMsg CUIWunderground::ReadDailyData(const string& filePath, CWeatherStation& data, CCallback& callback)const
	{
		ERMsg msg;

		//string path = GetPath(filePath);

		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		bool bNetatmo = WBSF::Find("Netatmo", data.GetSSI("StationType"), false);
		bool bEmptyST = data.GetSSI("StationType").empty();

		//now extact data 
		ifStream file;

		msg = file.open(filePath);

		if (msg)
		{
			enum { C_DATE, C_TMAX, C_TAIR, C_TMIN, C_DMAX, C_TDEW, C_DMIN, C_HMAX, C_RELH, C_HMIN, C_PMAX, C_PMIN, C_WMAX, C_WNDS, C_GUST, C_PRCP, NB_COLUMNS };
			//Date,TemperatureHighC,TemperatureAvgC,TemperatureLowC,DewpointHighC,DewpointAvgC,DewpointLowC,HumidityHigh,HumidityAvg,HumidityLow,PressureMaxhPa,PressureMinhPa,WindSpeedMaxKMH,WindSpeedAvgKMH,GustSpeedMaxKMH,PrecipitationSumCM
			//Date,TemperatureHighC,TemperatureAvgC,TemperatureLowC,DewpointHighC,DewpointAvgC,DewpointLowC,HumidityHigh,HumidityAvg,HumidityLow,PressureMaxhPa,PressureMinhPa,WindSpeedMaxKMH,WindSpeedAvgKMH,GustSpeedMaxKMH,PrecipitationSumCM
			for (CSVIterator loop(file); loop != CSVIterator() && msg; ++loop)
			{
				if (loop->size() == NB_COLUMNS)
				{
					string dateTimeStr = (*loop)[C_DATE];
					StringVector dateTime(dateTimeStr, " -:");

					int year = atoi(dateTime[0].c_str());
					size_t month = atoi(dateTime[1].c_str()) - 1;
					size_t day = atoi(dateTime[2].c_str()) - 1;

					ASSERT(year >= firstYear - 1 && year <= lastYear);
					ASSERT(month >= 0 && month < 12);
					ASSERT(day >= 0 && day < GetNbDayPerMonth(year, month));

					CTRef TRef = CTRef(year, month, day);
					if (TRef.IsValid())
					{
						double Tmin = -DBL_MAX;
						double Tair = -DBL_MAX;
						double Tmax = -DBL_MAX;
						double Pmin = -DBL_MAX;
						double Pmax = -DBL_MAX;
						for (size_t c = 1; c < loop->size(); c++)
						{
							if (c == C_TMAX || c == C_TMIN || c == C_TDEW || c == C_RELH || c == C_PMAX || c == C_PMIN || c == C_WNDS || c == C_PRCP)
							{
								string str = TrimConst((*loop)[c]);
								if (!str.empty() && str != "inf")
								{
									//double value = stod(str.c_str());
									double value = atof(str.c_str());

									switch (c)
									{
									case C_TMAX: Tmax = value; break;
									case C_TAIR: Tair = value;  break;
									case C_TMIN: Tmin = value; break;
									case C_DMAX: break;
									case C_TDEW: if (value > -60 && value < 50)data[TRef].SetStat(H_TDEW, value);
									case C_DMIN: break;
									case C_HMAX: break;
									case C_RELH: if (value > 0 && value <= 100)data[TRef].SetStat(H_RELH, value);
									case C_HMIN: break;
									case C_PMAX: Pmax = value; break;
									case C_PMIN: Pmin = value; break;
									case C_WMAX: break;
									case C_WNDS: if (!bNetatmo) data[TRef].SetStat(H_WNDS, value); break;
									case C_GUST: break;
									case C_PRCP: if (!bNetatmo && !bEmptyST) data[TRef].SetStat(H_PRCP, value); break;
									}
								}
							}
						}


						if (Tair != -DBL_MAX && Tair > -60 && Tair < 50)
						{
							ASSERT(Tair > -60 && Tair < 50);

							data[TRef].SetStat(H_TAIR, Tair);
						}

						if (Tmin != -DBL_MAX && Tmax != -DBL_MAX && Tmin > -60 && Tmin < 50 && Tmax > -60 && Tmax < 50)
						{
							ASSERT(Tmin > -60 && Tmin < 50);
							ASSERT(Tmax > -60 && Tmax < 50);


							if (Tmin > Tmax)
								Switch(Tmin, Tmax);

							data[TRef].SetStat(H_TMIN, Tmin);
							data[TRef].SetStat(H_TMAX, Tmax);
						}

						if (Pmin != -DBL_MAX && Pmax != -DBL_MAX && Pmin > 800 && Pmin < 1100 && Pmax > 800 && Pmax < 1100)
						{
							ASSERT(Pmin > 800 && Pmin < 1100);
							ASSERT(Pmax > 800 && Pmax < 1100);
							data[TRef].SetStat(H_PRES, (Pmin + Pmax) / 2);
						}
					}
				}

				msg += callback.StepIt(0);
			}//for all line

			file.close();
		}//if open

		return msg;
	}

	ERMsg CUIWunderground::ReadHourlyData(const string& filePath, CWeatherStation& data, CCallback& callback)const
	{
		ERMsg msg;

		//string path = GetPath(filePath);

		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		bool bNetatmo = WBSF::Find("Netatmo", data.GetSSI("StationType"), false);
		bool bEmptyST = data.GetSSI("StationType").empty();

		//now extract data 
		ifStream file;

		msg = file.open(filePath);

		if (msg)
		{
			CWeatherAccumulator accumulator(CTM::HOURLY);

			//Time,TemperatureC,DewpointC,PressurehPa,WindDirection,WindDirectionDegrees,WindSpeedKMH,WindSpeedGustKMH,Humidity,HourlyPrecipMM,Conditions,Clouds,dailyrainMM,SolarRadiationWatts/m^2,SoftwareType,DateUTC
			enum { C_TIME, C_TAIR, C_TDEW, C_PRES, C_WDIR, C_WNDD, C_WNDS, C_GUST, C_RELH, C_HOURLYPRECIPMM, C_CONDITIONS, C_CLOUDS, C_PRCP, C_SRAD, NB_COLUMNS };
			static const char* COL_NAME[NB_COLUMNS] = { "Time","TemperatureC","DewpointC","PressurehPa","WindDirection","WindDirectionDegrees","WindSpeedKMH","WindSpeedGustKMH","Humidity","HourlyPrecipMM","Conditions","Clouds","dailyrainMM","SolarRadiationWatts/m^2" };

			double last_prcp = 0;
			CTRef  last_prcp_TRef;

			std::vector<size_t> col_pos;
			for (CSVIterator loop(file); loop != CSVIterator() && msg; ++loop)
			{
				if (col_pos.empty())
				{

					col_pos.resize(loop.Header().size(), NOT_INIT);
					for (size_t c = 0; c < loop.Header().size(); c++)
					{
						auto it = std::find(std::begin(COL_NAME), std::end(COL_NAME), loop.Header()[c]);
						if (it != std::end(COL_NAME))
						{
							//size_t v1 = it - COL_NAME;
							size_t v = std::distance(std::begin(COL_NAME), it);
							ASSERT(v < NB_COLUMNS);
							col_pos[c] = v;
						}
					}
				}

				if (col_pos[C_TIME] != NOT_INIT && loop->size() >= col_pos.size())
				{
					string dateTimeStr = (*loop)[col_pos[C_TIME]];
					StringVector dateTime(dateTimeStr, " -:");

					int year = atoi(dateTime[0].c_str());
					size_t m = atoi(dateTime[1].c_str()) - 1;
					size_t d = atoi(dateTime[2].c_str()) - 1;
					size_t h = atoi(dateTime[3].c_str());
					size_t mm = atoi(dateTime[4].c_str());
					size_t ss = atoi(dateTime[5].c_str());

					ASSERT(year >= firstYear - 1 && year <= lastYear);
					ASSERT(m >= 0 && m < 12);
					ASSERT(d >= 0 && d < GetNbDayPerMonth(year, m));
					ASSERT(h >= 0 && h < 24);
					ASSERT(mm >= 0 && mm < 60);
					ASSERT(ss >= 0 && ss < 60);

					CTRef TRef = CTRef(year, m, d, h);
					if (TRef.IsValid())
					{
						if (accumulator.TRefIsChanging(TRef))
						{
							data[accumulator.GetTRef()].SetData(accumulator);
						}

						//if (TRef.as(CTM::DAILY) != last_prcp_TRef)
						//if (TRef.as(CTM::DAILY) != last_prcp_TRef)
						//{//reset each day
						//	last_prcp_TRef = TRef.as(CTM::DAILY);
						//	last_prcp = 0;
						//}


						for (size_t cc = 0; cc < loop->size(); cc++)
						{
							size_t c = col_pos[cc];
							if (c != NOT_INIT)
							{
								string str = TrimConst((*loop)[c]);
								if (!str.empty() && str != "inf")
								{
									double value = atof(str.c_str());

									if (c == C_PRCP && value < last_prcp)
									{
										//sometime the reset is not done at midnight
										//last_prcp_TRef = TRef.as(CTM::DAILY);
										last_prcp = 0;
									}

									switch (c)
									{
									case C_TAIR: if (value > -60 && value < 50)accumulator.Add(TRef, H_TAIR, value);  break;
									case C_TDEW: if (value > -60 && value < 50)accumulator.Add(TRef, H_TDEW, value); break;
									case C_RELH: if (value > 0 && value <= 100)accumulator.Add(TRef, H_RELH, value); break;
									case C_WNDS: if (!bNetatmo) accumulator.Add(TRef, H_WNDS, value); break;
									case C_WNDD: if (!bNetatmo) accumulator.Add(TRef, H_WNDD, value); break;
									case C_PRCP: if (!bNetatmo && !bEmptyST) accumulator.Add(TRef, H_PRCP, value - last_prcp); last_prcp = value;  break;
									case C_SRAD: accumulator.Add(TRef, H_SRAD, value); break;
									}
								}
							}
						}
					}
				}

				msg += callback.StepIt(0);
			}//for all line

			data[accumulator.GetTRef()].SetData(accumulator);

			file.close();
		}//if open
		return msg;
	}


	//CTPeriod CUIWunderground::GetHourlyPeriod(const string& filePath, double& last_hms)
	//{
	//	CTPeriod p;
	//	last_hms = 0;

	//	ifStream file;
	//	if (file.open(filePath, ifStream::in | ifStream::binary))
	//	{
	//		string firstLine;

	//		std::getline(file, firstLine);//read header
	//		std::getline(file, firstLine);//first line

	//		StringVector columns1(firstLine, ",");
	//		if (!columns1.empty())
	//		{
	//			StringVector dateTime1(columns1[0], " -:");
	//			if (dateTime1.size() == 6)
	//			{
	//				int year = atoi(dateTime1[0].c_str());
	//				size_t m = atoi(dateTime1[1].c_str()) - 1;
	//				size_t d = atoi(dateTime1[2].c_str()) - 1;
	//				size_t h = atoi(dateTime1[3].c_str());
	//				size_t mm = atoi(dateTime1[4].c_str());
	//				size_t ss = atoi(dateTime1[5].c_str());

	//				ASSERT(year >= 2000 && year <= 2100);
	//				ASSERT(m >= 0 && m < 12);
	//				ASSERT(d >= 0 && d < GetNbDayPerMonth(year, m));
	//				ASSERT(h >= 0 && h < 24);
	//				ASSERT(mm >= 0 && mm < 60);
	//				ASSERT(ss >= 0 && ss < 60);

	//				CTRef TRef1 = CTRef(year, m, d, h);
	//				if (TRef1.IsValid())
	//				{
	//					file.seekg(-1, ios_base::end);                // go to one spot before the EOF

	//					bool keepLooping = true;
	//					bool bSkipFirst = true;
	//					while (keepLooping)
	//					{
	//						char ch;
	//						file.get(ch);                            // Get current byte's data

	//						if ((int)file.tellg() <= 1) {             // If the data was at or before the 0th byte
	//							file.seekg(0);                       // The first line is the last line
	//							keepLooping = false;                // So stop there
	//						}
	//						else if (ch == '\n') {                   // If the data was a newline
	//							if (bSkipFirst)
	//							{
	//								bSkipFirst = false;
	//								file.seekg(-2, ios_base::cur);        // Move to the front of that data, then to the front of the data before it
	//							}
	//							else
	//							{
	//								keepLooping = false;                // Stop at the current position.
	//							}
	//						}
	//						else {                                  // If the data was neither a newline nor at the 0 byte
	//							file.seekg(-2, ios_base::cur);        // Move to the front of that data, then to the front of the data before it
	//						}
	//					}

	//					string lastLine;
	//					getline(file, lastLine);                      // Read the current line

	//					StringVector columns2(lastLine, ",");
	//					if (!columns2.empty())
	//					{
	//						StringVector dateTime2(columns2[0], " -:");
	//						if (dateTime2.size() == 6)
	//						{
	//							int year = atoi(dateTime2[0].c_str());
	//							size_t m = atoi(dateTime2[1].c_str()) - 1;
	//							size_t d = atoi(dateTime2[2].c_str()) - 1;
	//							size_t h = atoi(dateTime2[3].c_str());
	//							size_t mm = atoi(dateTime2[4].c_str());
	//							size_t ss = atoi(dateTime2[5].c_str());

	//							ASSERT(year >= 2000 && year <= 2100);
	//							ASSERT(m >= 0 && m < 12);
	//							ASSERT(d >= 0 && d < GetNbDayPerMonth(year, m));
	//							ASSERT(h >= 0 && h < 24);
	//							ASSERT(mm >= 0 && mm < 60);
	//							ASSERT(ss >= 0 && ss < 60);

	//							CTRef TRef2 = CTRef(year, m, d, h);
	//							if (TRef2.IsValid())
	//							{
	//								p = CTPeriod(TRef1, TRef2);
	//								last_hms = h + mm / 60.0 + ss / 3600.0;
	//							}
	//						}
	//					}
	//				}
	//			}
	//		}

	//		file.close();
	//	}

	//	return p;
	//}

	//void CUIWunderground::clean_source(double last_hms, std::string& source)
	//{
	//	if (last_hms < 24)
	//	{
	//		//remove all line until hms
	//		size_t pos1 = source.find('\n');
	//		//size_t pos2 = (pos1 != string::npos)?source.find('\n', pos1 + 1): string::npos;
	//		while (pos1 != string::npos)
	//		{
	//			size_t pos2 = source.find('\n', pos1 + 1);
	//			if (pos2 != string::npos)
	//			{
	//				string line = source.substr(pos1 + 1, pos2 - pos1 - 1);
	//				StringVector columns(line, ",");
	//				if (!columns.empty())
	//				{
	//					StringVector dateTime(columns[0], " -:");
	//					if (dateTime.size() == 6)
	//					{
	//						int year = atoi(dateTime[0].c_str());
	//						size_t m = atoi(dateTime[1].c_str()) - 1;
	//						size_t d = atoi(dateTime[2].c_str()) - 1;
	//						size_t h = atoi(dateTime[3].c_str());
	//						size_t mm = atoi(dateTime[4].c_str());
	//						size_t ss = atoi(dateTime[5].c_str());

	//						ASSERT(year >= 2000 && year <= 2100);
	//						ASSERT(m >= 0 && m < 12);
	//						ASSERT(d >= 0 && d < GetNbDayPerMonth(year, m));
	//						ASSERT(h >= 0 && h < 24);
	//						ASSERT(mm >= 0 && mm < 60);
	//						ASSERT(ss >= 0 && ss < 60);

	//						//CTRef TRef = CTRef(year, m, d);
	//						//if ( TRef == lastDay)
	//						//{
	//						double hms = h + mm / 60.0 + ss / 3600.0;

	//						if (hms > last_hms)
	//						{
	//							source = source.substr(pos1 + 1);
	//							pos2 = string::npos;//stop here
	//						}
	//						//}
	//					}
	//				}
	//			}
	//			else
	//			{
	//				//no update
	//				source.clear();
	//			}

	//			pos1 = pos2;
	//		}//while pos1
	//	}
	//	else
	//	{
	//		size_t pos1 = source.find('\n');
	//		//size_t pos2 = (pos1 != string::npos)?source.find('\n', pos1 + 1): string::npos;
	//		if (pos1 != string::npos)//only header, clear source
	//			source = source.substr(pos1 + 1);
	//		//else
	//			//source.clear();

	//	}
	//}

	TVarH GetVariable2(const string& header)
	{
		static const char* VAR_NAME[] = { "temp", "precip", "dewpt", "humidity", "windspeed", "winddir", "solarRadiation", "pressure" };
		static const char* STAT_NAME[] = { "Low", "Avg", "High", "Total", "Min", "Max" };
		static const TVarH VAR_TYPE[] = { H_TAIR, H_PRCP, H_TDEW, H_RELH, H_WNDS, H_WNDD, H_SRAD, H_PRES };

		TVarH var = H_SKIP;


		auto it1 = std::find_if(begin(VAR_NAME), end(VAR_NAME), [header](const char* name) {return header.find(name) != string::npos; });
		auto it2 = std::find_if(begin(STAT_NAME), end(STAT_NAME), [header](const char* stat) {return header.find(stat) != string::npos; });

		if (it1 != end(VAR_NAME) && it2 != end(STAT_NAME))
		{
			size_t d1 = std::distance(begin(VAR_NAME), it1);
			size_t d2 = std::distance(begin(STAT_NAME), it2);
			if (d1 == 0 && d2 == 0)
				var = H_TMIN;
			if (d1 == 0 && d2 == 2)
				var = H_TMAX;
			if (d1 == 1 && d2 == 3)
				var = VAR_TYPE[d1];
			if ((d1 == 0 || d1 == 2 || d1 == 3 || d1 == 4 || d1 == 5) && d2 == 1)
				var = VAR_TYPE[d1];
			if (d1 == 6 && d2 == 2)
				var = VAR_TYPE[d1];

		}

		return var;
	}

	map<string, TVarH> GetVariables2(const Json::object& elem)
	{
		map<string, TVarH> variables;

		for (auto it = elem.begin(); it != elem.end(); it++)
		{
			if (!it->second.is_object())
			{
				TVarH var = GetVariable2(it->first);
				if (var != H_SKIP)
					variables[it->first] = var;
			}
			else
			{
				for (auto iit = it->second.object_items().begin(); iit != it->second.object_items().end(); iit++)
				{
					TVarH var = GetVariable2(iit->first);
					if (var != H_SKIP)
						variables[iit->first] = var;
				}
			}

		}

		return variables;
	}

	ERMsg CUIWunderground::ParseJson(size_t type, const string& str, CWeatherYears& data, CCallback& callback)const
	{
		ERMsg msg;


		//try to load old data, ignor error if the fiole does not exist
		//if (type == HOURLY_WEATHER)



		//stringstream stream(str);




		string error;
		Json json = Json::parse(str, error);
		ASSERT(json.is_object());

		if (error.empty() /*&& observations["status"] == "OK"*/)
		{
			map<string, TVarH > variables;

			ASSERT(json["observations"].is_array());
			const Json::array& observations = json["observations"].array_items();
			for (int i = 0; i < observations.size(); i++)
			{
				if (variables.empty())
				{
					variables = GetVariables2(observations[0].object_items());
					if (type == DAILY_WEATHER)
						variables.erase("solarRadiationHigh");// it's the daily higest and not the daily mean
				}


				StringVector date_time(observations[i]["obsTimeLocal"].string_value(), " -:");
				ASSERT(date_time.size() == 6);

				int year = ToInt(date_time[0]);
				size_t m = ToSizeT(date_time[1]) - 1;
				size_t d = ToSizeT(date_time[2]) - 1;
				size_t h = ToSizeT(date_time[3]);

				ASSERT(m >= 0 && m < 12);
				ASSERT(d >= 0 && d < GetNbDayPerMonth(year, m));

				CTRef TRef(year, m, d, h, type == HOURLY_WEATHER ? CTM::HOURLY : CTM::DAILY);
				ASSERT(TRef.IsValid());

				const Json::object& elem = observations[i].object_items();
				//for (auto it = elem.begin(); it != elem.end(); it++)
				for (auto it = variables.begin(); it != variables.end(); it++)
				{
					string name = it->first;
					if (observations[i][name].is_number())
						data[TRef].SetStat(variables[name], observations[i][name].number_value());

					if (observations[i]["metric"][name].is_number())
						data[TRef].SetStat(variables[name], observations[i]["metric"][name].number_value());
				}

				if (observations[i]["metric"]["pressureMin"].is_number() && observations[i]["metric"]["pressureMax"].is_number())
				{
					double pres = (observations[i]["metric"]["pressureMin"].number_value() + observations[i]["metric"]["pressureMax"].number_value()) / 2.0;
					data[TRef].SetStat(H_PRES, pres);
				}

				//if (variables.find(name) != variables.end() && variables[name] != H_SKIP)
				//{
				//	double value = it->second.number_value();
				//	data[TRef].SetStat(variables[name], value);
				//}
				//if (name == "metric")
				//{
				//	ASSERT(observations[i]["metric"].is_object());
				//	const Json::object& metric = it->second.object_items();
				//	

				//	for (auto iit = metric.begin(); iit != metric.end(); iit++)
				//	{
				//		string name = iit->first;
				//		if (variables.find(name) != variables.end() && variables[name] != H_SKIP)
				//		{
				//			double value = iit->second.number_value();
				//			data[TRef].SetStat(variables[name], value);
				//		}
				//	}//if good vars
				//}//if metric
			//}//for all element
			}//for all observation
		}//no error
		else
		{
			msg.ajoute(error);
		}

		return msg;
	}


	ERMsg CUIWunderground::ParseJsonMetadata(const string& str, CLocation& location, CCallback& callback)const
	{
		ERMsg msg;

		string error;
		Json json = Json::parse(str, error);
		ASSERT(json.is_object());

		if (error.empty())
		{
			const Json::object& elem = json.object_items();
			location.m_ID = json["ID"].string_value();
			location.m_name = json["name"].string_value();
			location.m_lat = json["latitude"].number_value();
			location.m_lon = json["longitude"].number_value();
			location.m_elev = json["elevation"].number_value();
			location.SetSSI("city", json["city"].string_value());
			location.SetSSI("State", json["state"].string_value());
			location.SetSSI("Country", json["country"].string_value());
			location.SetSSI("StationType", json["stationType"].string_value());
			location.SetSSI("tzName", json["tzName"].string_value());
			location.SetSSI("GoldStar", to_string(json["goldStar"].bool_value()));
		}//no error
		else
		{
			msg.ajoute(error);
		}

		return msg;
	}

	ERMsg CUIWunderground::ExtractNominatimName(CLocation& location, CCallback& callback)
	{
		ERMsg msg;

		size_t miss = 0;
		bool bMissName = location.m_name.empty();
		bool bMissCounty = location.GetSSI("State").empty();
		bool bMissState = location.GetSSI("State").empty();
		bool bMissCountry = location.GetSSI("Country").empty();


		if (bMissName || bMissCounty || bMissState || bMissCountry)
		{
			CHttpConnectionPtr pConnection;
			CInternetSessionPtr pSession;
			msg += GetHttpConnection("nominatim.openstreetmap.org", pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", true);


			if (msg)
			{
				string URL = "/reverse?zoom=18&format=geojson&lat=" + ToString(location.m_lat) + "&lon=" + ToString(location.m_lon);

				string metadata;
				msg = UtilWWW::GetPageText(pConnection, URL, metadata, false, INTERNET_FLAG_SECURE | INTERNET_FLAG_IGNORE_CERT_CN_INVALID | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID);
				if (msg)
				{
					//extract elevation from google
					string error;
					Json geojson = Json::parse(metadata, error);

					if (error.empty())
					{
						ASSERT(geojson.is_object());
						ASSERT(geojson["features"].is_array());
						Json::array features = geojson["features"].array_items();
						if (features.size() == 1)
						{
							ASSERT(features[0].is_object());

							Json::object feature0 = features[0].object_items();
							Json::object properties = feature0["properties"].object_items();
							Json::object address = properties["address"].object_items();

							string village = ANSI_2_ASCII(address["village"].string_value());
							string town = ANSI_2_ASCII(address["town"].string_value());
							string suburb = ANSI_2_ASCII(address["suburb"].string_value());
							string city = ANSI_2_ASCII(address["city"].string_value());
							string county = ANSI_2_ASCII(address["county"].string_value());
							string region = ANSI_2_ASCII(address["region"].string_value());
							string state = ANSI_2_ASCII(address["state"].string_value());
							string country = ANSI_2_ASCII(address["country"].string_value());


							string name;
							if (!village.empty())
								name = village;
							else if (!town.empty())
								name = town;
							else if (!suburb.empty())
								name = suburb;
							else if (!city.empty())
								name = city;
							else if (!county.empty())
								name = county;
							else if (!region.empty())
								name = region;


							if (bMissName && !name.empty())
								location.m_name = name;

							if (bMissCounty && !county.empty())
								location.SetSSI("County", county);

							if (bMissState && !state.empty())
								location.SetSSI("State", state);

							if (bMissCountry && !country.empty())
								location.SetSSI("Country", country);
						}
						else
						{
							miss++;
						}
					}
					else
					{
						if (error.empty())
							error = geojson["error"]["message"].string_value();

						msg.ajoute(error);
					}

					pConnection->Close();
					pSession->Close();

				}//if msg
			}//if msg
		}//if missing info

		return msg;
	}


}