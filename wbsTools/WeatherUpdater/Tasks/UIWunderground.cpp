#include "StdAfx.h"
#include "UIWunderground.h"

#include <boost/algorithm/string.hpp>
#include "basic/WeatherStation.h"
#include "basic/CSV.h"
#include "Basic/FileStamp.h"
#include "UI/Common/SYShowMessage.h"

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
	const char* CUIWunderground::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "FirstYear", "LastYear", "DataType", "Countries", "States", "Province", "StationList", "GoldStar" };//"UpdateStationList" 
	const size_t CUIWunderground::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_STRING, T_STRING, T_COMBO_INDEX , T_STRING_SELECT, T_STRING_SELECT, T_STRING_SELECT, T_STRING, T_BOOL };/// 
	const UINT CUIWunderground::ATTRIBUTE_TITLE_ID = IDS_UPDATER_WU_P;
	const UINT CUIWunderground::DESCRIPTION_TITLE_ID = ID_TASK_WU;

	const char* CUIWunderground::CLASS_NAME() { static const char* THE_CLASS_NAME = "WeatherUnderground";  return THE_CLASS_NAME; }
	CTaskBase::TType CUIWunderground::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUIWunderground::CLASS_NAME(), (createF)CUIWunderground::create);


	const char* CUIWunderground::SERVER_NAME = "www.wunderground.com";


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
		case COUNTRIES:	str = CCountrySelectionWU::GetAllPossibleValue(); break;
		case STATES:	str = CStateSelection::GetAllPossibleValue(); break;
		case PROVINCE:	str = CProvinceSelection::GetAllPossibleValue(); break;
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
		case GOLD_STAR: str = "0"; break;
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
			filepath += "WUStationsListWorld.csv";


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

	string Purge(string str)
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


	ERMsg CUIWunderground::DownloadStationList(const string& country, CLocationVector& stationList2, CCallback& callback)const
	{
		ERMsg msg;

		CLocationVector stationList1;

		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;

		msg += GetHttpConnection("www.wunderground.com", pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", false, 5, callback);

		if (msg)
		{
			string URL = "/weatherstation/ListStations.asp?selectedCountry=" + country;

			string source;
			msg = GetPageText(pConnection, URL, source, false, NULL);
			if (msg)
			{

				string::size_type b = source.find("<tbody>");
				string::size_type e = source.find("</tbody>");
				callback.PushTask(GetString(IDS_LOAD_STATION_LIST) + " (Canada)", e);
				callback.SetCurrentStepPos(b);

				string::size_type posBegin = source.find("<tr>", b);
				while (posBegin != string::npos)
				{
					CLocation loc;
					loc.m_ID = Purge(FindString(source, "?ID=", "\">", posBegin));
					if (posBegin != string::npos)
						loc.m_name = UppercaseFirstLetter(ANSI_2_ASCII(Purge(FindString(source, "<td>", "&nbsp;", posBegin))));

					if (posBegin != string::npos)
						loc.SetSSI("City", UppercaseFirstLetter(ANSI_2_ASCII(Purge(FindString(source, "<td>", "&nbsp;", posBegin)))));

					if (posBegin != string::npos)
						loc.SetSSI("StationType", UppercaseFirstLetter(Purge(FindString(source, "<td class=\"station-type\">", "&nbsp;", posBegin))));

					if (loc.m_name.empty())
						loc.m_name = loc.GetSSI("City");

					if (loc.m_name.empty())
						loc.m_name = loc.m_ID;

					bool bNetatmo = WBSF::Find("Netatmo", loc.GetSSI("StationType"), false);
					if (!bNetatmo && stationList2.FindByID(loc.m_ID) == NOT_INIT)
						stationList1.push_back(loc);


					posBegin = source.find("<tr>", posBegin);
					msg += callback.SetCurrentStepPos(posBegin);
				}

				callback.PopTask();


				//get station information
				callback.PushTask(GetString(IDS_LOAD_STATION_LIST) + " (Canada) : " + ToString(stationList1.size()) + " stations", stationList1.size());
				for (size_t i = 0; i < stationList1.size() && msg; i++)
				{
					CLocation station = stationList1[i];

					URL = "/personal-weather-station/dashboard?ID=" + station.m_ID;
					string source;
					msg = GetPageText(pConnection, URL, source, false, NULL);
					if (msg)
					{
						bool bGoldStart = source.find("pws-goldstar-27x21.png") != string::npos;
						station.SetSSI("GoldStar", bGoldStart ? "1" : "0");

						string::size_type posBegin = source.find("id=\"forecast-link\">");
						if (posBegin != string::npos)
						{
							string str = FindString(source, "</a><span class=\"subheading\">", "</span>", posBegin);
							StringVector v(str, "> ");
							ASSERT(v.size() >= 3);

							if (v.size() >= 3)
							{
								station.m_lat = WBSF::as<double>(v[0]);
								station.m_lon = WBSF::as<double>(v[1]);

								if (v.size() == 4 && v[2] != "-999")
								{
									if (v[3] == "m")
										station.m_elev = WBSF::as<double>(v[2]);
									else if (v[3] == "ft")
										station.m_elev = WBSF::Feet2Meter(WBSF::as<double>(v[2]));
								}

								if (station.m_elev >= -1000)//station with strange elevation are usually bad
									stationList2.push_back(station);
							}//if msg
						}//if have data

					}//if msg

					callback.StepIt();
				}//for all station


				callback.PopTask();
			}//if msg

			callback.AddMessage(GetString(IDS_NB_STATIONS) + ToString(stationList1.size()));

			pConnection->Close();
			pSession->Close();

			//pGoogleConnection->Close();
			//pGoogleSession->Close();
		}//if msg

		return msg;
	}

	ERMsg CUIWunderground::CompleteStationList(CLocationVector& stationList2, CCallback& callback)const
	{
		ERMsg msg;


		CInternetSessionPtr pGoogleSession;
		CHttpConnectionPtr pGoogleConnection;

		if (msg)
			msg += GetHttpConnection("maps.googleapis.com", pGoogleConnection, pGoogleSession, PRE_CONFIG_INTERNET_ACCESS, "", "", false, 5, callback);

		if (msg)
		{
			//get station information
			callback.PushTask(GetString(IDS_LOAD_STATION_LIST) + " (Canada) : " + ToString(stationList2.size()) + " stations", stationList2.size());
			for (size_t i = 0; i < stationList2.size() && msg; i++)
			{
				CLocation& station = stationList2[i];

				if (station.m_elev == -999)
				{
					string elevFormat = "/maps/api/elevation/json?locations=" + ToString(station.m_lat) + "," + ToString(station.m_lon);
					string strElev;
					msg = UtilWWW::GetPageText(pGoogleConnection, elevFormat, strElev);
					if (msg)
					{
						//extract elevation from google
						string error;
						Json jsonElev = Json::parse(strElev, error);
						ASSERT(jsonElev.is_object());

						if (error.empty() && jsonElev["status"] == "OK")
						{
							ASSERT(jsonElev["results"].is_array());
							Json::array result = jsonElev["results"].array_items();
							ASSERT(result.size() == 1);

							station.m_elev = result[0]["elevation"].number_value();
						}
					}//if msg
				}



				//
				if (station.GetSSI("Country").empty())
				{
					string geoFormat = "/maps/api/geocode/json?latlng=" + ToString(station.m_lat) + "," + ToString(station.m_lon);
					string strGeo;


					msg = UtilWWW::GetPageText(pGoogleConnection, geoFormat, strGeo);
					if (msg)
					{
						//extract elevation from google
						string error;
						Json jsonGeo = Json::parse(strGeo, error);
						ASSERT(jsonGeo.is_object());

						if (error.empty() && jsonGeo["status"] == "OK")
						{
							ASSERT(jsonGeo["results"].is_array());
							Json::array result1 = jsonGeo["results"].array_items();
							if (!result1.empty())
							{
								Json::array result2 = result1[0]["address_components"].array_items();
								for (int j = 0; j < result2.size(); j++)
								{
									Json::array result3 = result2[j]["types"].array_items();
									if (result3.size() == 2 && result3[0] == "administrative_area_level_2")
									{
										string str = ANSI_2_ASCII(result2[j]["short_name"].string_value());
										WBSF::ReplaceString(str, ",", " ");
										station.SetSSI("County", str);
									}

									if (result3.size() == 2 && result3[0] == "administrative_area_level_1")
										station.SetSSI("State", ANSI_2_ASCII(result2[j]["short_name"].string_value()));

									if (result3.size() == 2 && result3[0] == "country")
										station.SetSSI("Country", ANSI_2_ASCII(result2[j]["short_name"].string_value()));

								}
							}
						}
					}//if msg
				}

				msg += callback.StepIt();
			}//for all station

		}//if open connection


		callback.PopTask();

		pGoogleConnection->Close();
		pGoogleSession->Close();


		return msg;

	}



	TVarH CUIWunderground::GetVar(const string& str)
	{
		TVarH v = H_SKIP;

		enum { NB_VARS = 7 };
		static const char* VAR_NAME[NB_VARS] = { "Temp.:", "DewPoint:", "Humidity:", "WindSpeed:", "Pressure:", "Precipitation:", "SolarRadiation:" };
		static const TVarH VAR_AVAILABLE[NB_VARS] = { H_TAIR, H_TDEW, H_RELH, H_WNDS, H_PRES, H_PRCP, H_SRAD };
		for (size_t vv = 0; vv < NB_VARS&&v == H_SKIP; vv++)
		{
			if (IsEqual(str, VAR_NAME[vv]))
				v = VAR_AVAILABLE[vv];
		}

		return v;
	}

	CWVariables CUIWunderground::GetVariables(string str)
	{
		//size_t v_count = 0;
		//size_t e_count = 0;

		WBSF::CWVariables variables;



		boost::erase_all(str, "\r");
		boost::erase_all(str, "\n");
		boost::erase_all(str, "\t");
		boost::erase_all(str, " ");
		string::size_type posBegin = str.find("<tr>");


		//static const TVarH VAR_AVAILABLE[7] = {H_TAIR, H_TDEW, H_RELH, H_WNDS, H_SKIP, H_PRES, H_PRCP};

		while (posBegin != string::npos)
		{
			posBegin += 4;


			string str1 = FindString(str, "<td>", "</td>", posBegin);
			string str2 = FindString(str, "<td>", "</td>", posBegin);
			string str3 = FindString(str, "<td>", "</td>", posBegin);

			if (str1 != "UVIndex:" && str1 != "WindGust:")
			{
				size_t v = GetVar(str1);
				ASSERT(v != NOT_INIT);
				if (v != NOT_INIT && ((str2 != "-" && str2 != "" && str2 != "%") || (str3 != "-" && str3 != "" && str3 != "%")))
					variables.set(v);
			}

			posBegin = str.find("<tr>", posBegin);
		}

		ASSERT(str.find("<tr>", posBegin) == string::npos);

		return variables;
	}


	void CUIWunderground::CleanString(string& str)
	{
		string output;

		ReplaceString(str, "'", "");
		ReplaceString(str, "&deg;", "");
		ReplaceString(str, "S ", "-");
		ReplaceString(str, "W ", "-");
		ReplaceString(str, "N ", "");
		ReplaceString(str, "E ", "");

		Trim(str);
	}

	double CUIWunderground::GetCoordinate(string str)
	{
		CleanString(str);

		float ld = 0, lm = 0, ls = 0;
		sscanf(str.c_str(), "%f %f %f", &ld, &lm, &ls);

		return ld + Signe(ld)*(lm / 60.0 + ls / 3600.0);
	}

	ERMsg CUIWunderground::ExtractElevation(CLocationVector& stationList, CCallback& callback)
	{
		ERMsg msg;

		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;
		msg += GetHttpConnection("i.wund.com", pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", false, 5, callback);

		if (!msg)
			return msg;



		//Get elevation
		callback.PushTask("Extract elevation", stationList.size());

		for (size_t i = 0; i < stationList.size() && msg; i++)
		{
			string URL = FormatA("weatherstation/WXDailyHistory.asp?ID=%s&month=4&day=9&year=2016", stationList[i].m_ID.c_str());

			string source;
			msg = GetPageText(pConnection, URL, source);
			if (msg)
			{
				//http://i.wund.com/auto/iphone/weatherstation/WXDailyHistory.asp?ID=KCATUSTI15&month=4&day=9&year=2016

				string variables;
				string::size_type posBegin = source.find("Statistics for the rest of the month");

				if (posBegin != string::npos)
					variables = FindString(source, "<tbody>", "</tbody>", posBegin);

				WBSF::CWVariables v = GetVariables(variables);
				if (v.any())
					stationList[i].SetSSI("Variables", v.to_string());

				string lat = TrimConst(FindString(source, "Lat:</span>", "''"));
				if (!lat.empty())
					stationList[i].m_lat = GetCoordinate(lat);

				string lon = TrimConst(FindString(source, "Lon:</span>", "''"));
				if (!lon.empty())
					stationList[i].m_lon = GetCoordinate(lon);

				string alt = TrimConst(FindString(source, "Elevation:</span>", "ft"));
				if (!alt.empty())
					stationList[i].m_elev = ToDouble(alt) * 0.3048;
			}

			msg += callback.StepIt();

			if (((i + 1) % 200) == 0)
				stationList.Save("E:\\Travaux\\Install\\DemoBioSIM\\Update\\WeatherUnderground\\test.csv", ',', callback);
		}

		//now update coordinate
		stationList.Save("E:\\Travaux\\Install\\DemoBioSIM\\Update\\WeatherUnderground\\test.csv", ',', callback);

		callback.PopTask();

		pConnection->Close();
		pSession->Close();

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

		//if (as<bool>(UPDATE_STATION_LIST))
		if (UPDATE_STATIONS_LIST)
		{
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
		}

		CLocationVector stationList;
		msg = LoadStationList(stationList, callback);


		//Get station
		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = lastYear - firstYear + 1;
		int currentYear = GetCurrentYear();
		CTRef today = CTRef::GetCurrentTRef();

		string tstr = type == HOURLY_WEATHER ? "hourly" : "daily";

		callback.AddMessage("Download " + tstr + " WeatherUnderground stations data (" + ToString(stationList.size()) + ")");
		callback.PushTask("Download " + tstr + " WeatherUnderground stations data (" + ToString(stationList.size()) + ")", stationList.size()*nbYears);

		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;
		msg += GetHttpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", false, 5, callback);

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

					if (type == HOURLY_WEATHER)
					{
						//https://www.wunderground.com/weatherstation/WXDailyHistory.asp?ID=IQUEBECS44&day=1&month=1&year=2015&graphspan=day&format=1


							//CWeatherYear data;
							//ReadHourlyData(ouputFilePath, data, callback);
						double last_hms = 0;
						CTPeriod p = GetHourlyPeriod(ouputFilePath, last_hms).as(CTM::DAILY);
						inc = 1.0 / (year == currentYear ? today.GetJDay():GetNbDaysPerYear(year));

						size_t last_m = year == currentYear ? today.GetMonth() : LAST_MONTH;
						for (size_t m = 0; m <= last_m && msg; m++)
						{
							size_t last_d = (year == currentYear && m == today.GetMonth()) ? today.GetDay() : WBSF::GetNbDayPerMonth(year, m) - 1;
							for (size_t d = 0; d <= last_d && msg; d++)
							{
								CTRef TRef(year, m, d);
								if (!p.End().IsInit() || TRef >= p.End())
								{
									static const char* URL_FORMAT = "weatherstation/WXDailyHistory.asp?ID=%s&day=%02d&month=%02d&year=%d&graphspan=day&format=1";
									string URL = FormatA(URL_FORMAT, stationList[i].m_ID.c_str(), d + 1, m + 1, year);

									string source;
									msg = GetPageText(pConnection, URL, source, false, FLAGS);
									
									if (!source.empty())
									{
										if(source[0] == '\n')
											source.erase(source.begin());

										ReplaceString(source, "\n<br>", "");
										ReplaceString(source, "<br>", "");
									}
										

									//Time,TemperatureC,DewpointC,PressurehPa,WindDirection,WindDirectionDegrees,WindSpeedKMH,WindSpeedGustKMH,Humidity,HourlyPrecipMM,Conditions,Clouds,dailyrainMM,SoftwareType,DateUTC
									if (source.substr(0,5) ==  "Time,")
									{
										if (FileExists(ouputFilePath))//don't remove header if file does not exist
										{
											clean_source((TRef == p.End()) ? last_hms : 24, source);
										}
										else
										{
											size_t pos1 = source.find('\n');
											size_t pos2 = (pos1 != string::npos)?source.find('\n', pos1 + 1): string::npos;
											if (pos2 == string::npos)//only header, clear source
												source.clear();
										}

										if (!source.empty() )
										{
											ofStream file;
											msg = file.open(ouputFilePath, std::fstream::out | std::fstream::app);
											if (msg)
											{
												file << source;
												file.close();
												nbDownload++;
											}
										}
									}
								}

								msg += callback.StepIt(inc);
							}//for d
						}//for m


						inc = 0;
					}
					else
					{
						static const char* URL_FORMAT = "weatherstation/WXDailyHistory.asp?ID=%s&day=1&month=1&year=%d&dayend=31&monthend=12&yearend=%d&graphspan=custom&format=1";
						string URL = FormatA(URL_FORMAT, stationList[i].m_ID.c_str(), year, year);
						//string ouputFilePath = GetOutputFilePath(tstr, stationList[i].GetSSI("Country"), stationList[i].GetSSI("State"), stationList[i].m_ID, year);
						//CreateMultipleDir(GetPath(ouputFilePath));

						////if (year == currentYear || !FileExists(ouputFilePath))
						//if (NeedDownload(year, ouputFilePath))
						//{
						string source;
						msg = GetPageText(pConnection, URL, source, false, FLAGS);
						
						if (!source.empty())
						{
							if (source[0] == '\n')
								source.erase(source.begin());

							ReplaceString(source, "\n<br>", "");
							ReplaceString(source, "<br>", "");
						}

						if ( source.substr(0, 5) == "Date,")
						{
							

							ofStream file;
							msg = file.open(ouputFilePath);
							if (msg)
							{
								file << source;
								file.close();
								nbDownload++;
							}
						}
					}
				}//need download

				msg += callback.StepIt(inc);
			}
		}

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


	ERMsg CUIWunderground::LoadStationList(CLocationVector& stationList, CCallback& callback)const
	{
		ERMsg msg;

		stationList.clear();

		StringVector stationListID(TrimConst(Get(STATION_LIST)), " ;,|");

		CLocationVector stationList1;

		CCountrySelection countries(Get(COUNTRIES));
		CStateSelection states(Get(STATES));
		CProvinceSelection provinces(Get(PROVINCE));
		bool bUseGoldStarOnly = as<bool>(GOLD_STAR);


		CLocationVector tmp;
		size_t n = 0;
		if (countries.at("US") || !stationListID.empty())
		{
			msg = tmp.Load(GetStationListFilePath("US"), ",", callback);
			stationList1.insert(stationList1.end(), tmp.begin(), tmp.end());
			n++;
		}

		if (countries.at("CA") || !stationListID.empty())
		{
			msg = tmp.Load(GetStationListFilePath("CA"), ",", callback);
			stationList1.insert(stationList1.end(), tmp.begin(), tmp.end());
			n++;
		}

		if (countries.count() > n || !stationListID.empty())//other country than US and CA
		{
			if (tmp.Load(GetStationListFilePath(""), ",", callback))
				stationList1.insert(stationList1.end(), tmp.begin(), tmp.end());

			n++;
		}

		
		if (!stationListID.empty())
		{
			stationList.reserve(stationListID.size());

			for (auto it = stationListID.begin(); it != stationListID.end() && msg; it++)
			{
				size_t pos = stationList1.FindByID(*it);
				if (pos != NOT_INIT)
					stationList.insert(stationList.end(), stationList1[pos]);
				else
					callback.AddMessage("Warning: Stations with ID " + *it + " is not in coordinate station's list. This station must to be added to the stations list.");
			}
		}
		else
		{
			callback.PushTask("Clean list (" + ToString(stationList1.size()) + ")", stationList1.size());
			stationList.reserve(stationList1.size());
			for (auto it = stationList1.begin(); it != stationList1.end() && msg; it++)
			{
				bool bDownload = false;
				bool bGoldStart = WBSF::as<bool>(it->GetSSI("GoldStar"));

				if (!bUseGoldStarOnly || bGoldStart)
				{
					string country = it->GetSSI("Country");


					if (countries.at(country))
					{
						string state = it->GetSSI("State");
						if (country == "US")
						{
							if (states.at(state))
								bDownload = true;
						}
						else if (country == "CA")
						{
							if (provinces.at(state))
								bDownload = true;
						}
						else
						{
							bDownload = true;
						}

					}//if country selection

				}

				if (bDownload)
					stationList.insert(stationList.end(), *it);

				msg += callback.StepIt();
			}


			callback.PopTask();
		}




		return msg;
	}

	ERMsg CUIWunderground::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;


		msg = LoadStationList(m_stations, callback);

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
		size_t type = as<size_t>(DATA_TYPE);
		string tstr = type == HOURLY_WEATHER ? "hourly" : "daily";

		station.SetHourly(type == HOURLY_WEATHER);

		//now extract data 
		for (size_t y = 0; y < nbYears&&msg; y++)
		{
			int year = firstYear + int(y);
			string filePath = GetOutputFilePath(tstr, station.GetSSI("Country"), station.GetSSI("State"), ID, year);
			if (FileExists(filePath))
			{
				if (type == HOURLY_WEATHER)
					msg = ReadHourlyData(filePath, station, callback);
				else
					msg = ReadDailyData(filePath, station, callback);

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

									if (c == C_PRCP && value< last_prcp)
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


	CTPeriod CUIWunderground::GetHourlyPeriod(const string& filePath, double& last_hms)
	{
		CTPeriod p;
		last_hms = 0;

		ifStream file;
		if (file.open(filePath, ifStream::in | ifStream::binary))
		{
			string firstLine;

			std::getline(file, firstLine);//read header
			std::getline(file, firstLine);//first line

			StringVector columns1(firstLine, ",");
			if (!columns1.empty())
			{
				StringVector dateTime1(columns1[0], " -:");
				if (dateTime1.size() == 6)
				{
					int year = atoi(dateTime1[0].c_str());
					size_t m = atoi(dateTime1[1].c_str()) - 1;
					size_t d = atoi(dateTime1[2].c_str()) - 1;
					size_t h = atoi(dateTime1[3].c_str());
					size_t mm = atoi(dateTime1[4].c_str());
					size_t ss = atoi(dateTime1[5].c_str());

					ASSERT(year >= 2000 && year <= 2100);
					ASSERT(m >= 0 && m < 12);
					ASSERT(d >= 0 && d < GetNbDayPerMonth(year, m));
					ASSERT(h >= 0 && h < 24);
					ASSERT(mm >= 0 && mm < 60);
					ASSERT(ss >= 0 && ss < 60);

					CTRef TRef1 = CTRef(year, m, d, h);
					if (TRef1.IsValid())
					{
						file.seekg(-1, ios_base::end);                // go to one spot before the EOF

						bool keepLooping = true;
						bool bSkipFirst = true;
						while (keepLooping)
						{
							char ch;
							file.get(ch);                            // Get current byte's data

							if ((int)file.tellg() <= 1) {             // If the data was at or before the 0th byte
								file.seekg(0);                       // The first line is the last line
								keepLooping = false;                // So stop there
							}
							else if (ch == '\n') {                   // If the data was a newline
								if (bSkipFirst)
								{
									bSkipFirst = false;
									file.seekg(-2, ios_base::cur);        // Move to the front of that data, then to the front of the data before it
								}
								else
								{
									keepLooping = false;                // Stop at the current position.
								}
							}
							else {                                  // If the data was neither a newline nor at the 0 byte
								file.seekg(-2, ios_base::cur);        // Move to the front of that data, then to the front of the data before it
							}
						}

						string lastLine;
						getline(file, lastLine);                      // Read the current line

						StringVector columns2(lastLine, ",");
						if (!columns2.empty())
						{
							StringVector dateTime2(columns2[0], " -:");
							if (dateTime2.size() == 6)
							{
								int year = atoi(dateTime2[0].c_str());
								size_t m = atoi(dateTime2[1].c_str()) - 1;
								size_t d = atoi(dateTime2[2].c_str()) - 1;
								size_t h = atoi(dateTime2[3].c_str());
								size_t mm = atoi(dateTime2[4].c_str());
								size_t ss = atoi(dateTime2[5].c_str());

								ASSERT(year >= 2000 && year <= 2100);
								ASSERT(m >= 0 && m < 12);
								ASSERT(d >= 0 && d < GetNbDayPerMonth(year, m));
								ASSERT(h >= 0 && h < 24);
								ASSERT(mm >= 0 && mm < 60);
								ASSERT(ss >= 0 && ss < 60);

								CTRef TRef2 = CTRef(year, m, d, h);
								if (TRef2.IsValid())
								{
									p = CTPeriod(TRef1, TRef2);
									last_hms = h + mm / 60.0 + ss / 3600.0;
								}
							}
						}
					}
				}
			}

			file.close();
		}

		return p;
	}

	void CUIWunderground::clean_source(double last_hms, std::string& source)
	{
		if (last_hms < 24)
		{
			//remove all line until hms
			size_t pos1 = source.find('\n');
			//size_t pos2 = (pos1 != string::npos)?source.find('\n', pos1 + 1): string::npos;
			while (pos1 != string::npos)
			{
				size_t pos2 = source.find('\n', pos1 + 1);
				if (pos2 != string::npos)
				{
					string line = source.substr(pos1 + 1, pos2 - pos1 - 1);
					StringVector columns(line, ",");
					if (!columns.empty())
					{
						StringVector dateTime(columns[0], " -:");
						if (dateTime.size() == 6)
						{
							int year = atoi(dateTime[0].c_str());
							size_t m = atoi(dateTime[1].c_str()) - 1;
							size_t d = atoi(dateTime[2].c_str()) - 1;
							size_t h = atoi(dateTime[3].c_str());
							size_t mm = atoi(dateTime[4].c_str());
							size_t ss = atoi(dateTime[5].c_str());

							ASSERT(year >= 2000 && year <= 2100);
							ASSERT(m >= 0 && m < 12);
							ASSERT(d >= 0 && d < GetNbDayPerMonth(year, m));
							ASSERT(h >= 0 && h < 24);
							ASSERT(mm >= 0 && mm < 60);
							ASSERT(ss >= 0 && ss < 60);

							//CTRef TRef = CTRef(year, m, d);
							//if ( TRef == lastDay)
							//{
							double hms = h + mm / 60.0 + ss / 3600.0;

							if (hms > last_hms)
							{
								source = source.substr(pos1 + 1);
								pos2 = string::npos;//stop here
							}
							//}
						}
					}
				}
				else
				{
					//no update
					source.clear();
				}

				pos1 = pos2;
			}//while pos1
		}
		else
		{
			size_t pos1 = source.find('\n');
			//size_t pos2 = (pos1 != string::npos)?source.find('\n', pos1 + 1): string::npos;
			if (pos1 != string::npos)//only header, clear source
				source = source.substr(pos1 + 1);
			//else
				//source.clear();

		}


	}
}