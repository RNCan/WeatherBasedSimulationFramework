#include "StdAfx.h"
#include "UIWunderground.h"

#include <boost/algorithm/string.hpp>
#include "basic/WeatherStation.h"
#include "basic/CSV.h"
#include "UI/Common/SYShowMessage.h"

#include "TaskFactory.h"
#include "../Resource.h"
#include "WeatherBasedSimulationString.h"
#include "CountrySelection.h"
#include "StateSelection.h"
#include "ProvinceSelection.h"

static const bool UPDATE_STATIONS_LIST = false;
static const bool UPDATE_STATIONS_INFO = false; 

using namespace WBSF::HOURLY_DATA;
using namespace std;
using namespace UtilWWW;

namespace WBSF
{
	//to update data in csv file
	//https://www.wunderground.com/weatherstation/WXDailyHistory.asp?ID=IQUEBECS44&day=1&month=1&year=2015&dayend=31&monthend=12&yearend=2015&graphspan=custom&format=1
	
	

	//pour la liste des stations actives:
	//http://www.wunderground.com/weatherstation/ListStations.asp?showall=&start=20&selectedState=
	//*********************************************************************

	static const DWORD FLAGS = INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_PRAGMA_NOCACHE;//| INTERNET_FLAG_TRANSFER_BINARY
	const char* CUIWunderground::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "FirstYear", "LastYear", "Countries", "States", "Province" };//"UpdateStationList" 
	const size_t CUIWunderground::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_STRING, T_STRING, T_STRING_SELECT, T_STRING_SELECT, T_STRING_SELECT};///, T_BOOL 
	const UINT CUIWunderground::ATTRIBUTE_TITLE_ID = IDS_UPDATER_WU_P;
	const UINT CUIWunderground::DESCRIPTION_TITLE_ID = ID_TASK_WU;

	const char* CUIWunderground::CLASS_NAME(){ static const char* THE_CLASS_NAME = "WeatherUnderground";  return THE_CLASS_NAME; }
	CTaskBase::TType CUIWunderground::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUIWunderground::CLASS_NAME(), (createF)CUIWunderground::create);


	const char* CUIWunderground::SERVER_NAME = "www.wunderground.com";


	static const char pageFormat1[] = "weatherstation/ListStations.asp?showall=&start=%s&selectedState=%s";
	static const char pageFormat2[] = "weatherstation/ListStations.asp?selectedCountry=%s";
	static const short SEL_ROW_PER_PAGE = 40;

	static const char pageDataFormat[] =
	{
		"weatherstation/WXDailyHistory.asp?"
		"ID=%s&"
		"graphspan=custom&"
		"month=%d&"
		"Day=%d"
		"Year=%d&"
		"monthEnd=%d&"
		"DayEnd=%d"
		"YearEnd=%d&"
		"format=1"
	};





	CUIWunderground::CUIWunderground(void)
	{}


	CUIWunderground::~CUIWunderground(void)
	{}


	std::string CUIWunderground::Option(size_t i)const
	{
		string str;
		switch (i)
		{
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
		//case UPDATE_STATION_LIST:	str = "0"; break;
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


	std::string CUIWunderground::GetOutputFilePath(const string& country, const string& states, const std::string& ID, int year)
	{
		string workingDir = GetDir(WORKING_DIR);
		string ouputPath = workingDir + ToString(year) + "\\";


		if (country == "US")
			ouputPath += "USA\\" + states +"\\";
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
		enum{ NB_NAMES = 32 };
		static const char* MAP_NAME[NB_NAMES][2] =
		{
			{"AL", "AB"},
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

	
	ERMsg CUIWunderground::DownloadStationList(const string& country, CLocationVector& stationList, CCallback& callback)const
	{
		ERMsg msg;

		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;

		//msg = stationList.Load(GetStationListFilePath());
		msg += GetHttpConnection("i.wund.com", pConnection, pSession);


		if (country == "US")
		{
			//Download USA
			//http://i.wund.com/weatherstation/ListStations.asp?selectedState=CO&selectedCountry=United+States

			callback.PushTask(GetString(IDS_LOAD_STATION_LIST) + " (USA)", NB_USA_STATES);
			string lastName = "Unknown1";

			for (size_t i = 0; i < NB_USA_STATES&&msg; i++)
			{
				string state = CStateSelection::GetName(i, CStateSelection::BY_ABVR).c_str();
				string URL = FormatA("weatherstation/ListStations.asp?selectedState=%s&selectedCountry=United+States", state.c_str());

				string source;
				msg = GetPageText(pConnection, URL, source);
				if (msg)
				{

					string::size_type posBegin = source.find("Search Results");

					if (posBegin != string::npos)
						posBegin = source.find("<tbody>");

					if (posBegin != string::npos)
						posBegin = source.find("<tr>", posBegin);


					while (posBegin != string::npos)
					{
						CLocation loc;
						loc.m_ID = Purge(FindString(source, "?ID=", "\">", posBegin));
						loc.m_name = UppercaseFirstLetter(Purge(FindString(source, "<td class=\"sortC\">", "&nbsp;", posBegin)));
						loc.SetSSI("City", UppercaseFirstLetter(Purge(FindString(source, "<td class=\"sortC\">", "&nbsp;", posBegin))));
						loc.SetSSI("State", Purge(FindString(source, "<td class=\"sortC\">", "&nbsp;", posBegin)));
						loc.SetSSI("Country", Purge(FindString(source, "<td class=\"sortC\">", "&nbsp;", posBegin)));
						StringVector latLonStr(FindString(source, "\">", "&nbsp;", posBegin), ",");
						loc.SetSSI("StationType", Purge(FindString(source, "<td class=\"sortC\">", "&nbsp;", posBegin)));
						ASSERT(latLonStr.size() == 2);

						if (latLonStr.size() == 2)
						{
							if (loc.m_name.empty())
								loc.m_name = loc.GetSSI("City");

							if (loc.m_name.empty())
							{
								loc.m_name = WBSF::GenerateNewName(lastName);
								lastName = loc.m_name;
							}

							loc.m_lat = ToDouble(latLonStr[0]);
							loc.m_lon = ToDouble(latLonStr[1]);
							stationList.push_back(loc);
						}

						posBegin = source.find("<tr>", posBegin);
					}
				}//if msg


				msg += callback.StepIt();
			}//for all states

			callback.PopTask();
		}
		else if (country == "CA")
		{

			//Download Canada
			//de start=11680 à start=15800, il faut laisser tomber les US
			
			string lastName = "Unknown1";

			static const char* CA_NAME[2] = { "CA", "CANADA" };
			for (size_t i = 0; i < 2 && msg; i++)
			{
				string URL = FormatA("weatherstation/ListStations.asp?selectedCountry=%s", CA_NAME[i]);

				string source;
				msg = GetPageText(pConnection, URL, source);
				if (msg)
				{
					
					string::size_type b = source.find("<tbody>");
					string::size_type e = source.find("</tbody>");
					callback.PushTask(GetString(IDS_LOAD_STATION_LIST) + " (Canada)", e);
					callback.SetCurrentStepPos(b);

					string::size_type posBegin = source.find("Search Results");

					if (posBegin != string::npos)
						posBegin = source.find("<tbody>", posBegin);

					if (posBegin != string::npos)
						posBegin = source.find("<tr>", posBegin);


					while (posBegin != string::npos)
					{
						CLocation loc;
						loc.m_ID = Purge(FindString(source, "?ID=", "\">", posBegin));
						if (posBegin != string::npos)
							loc.m_name = UppercaseFirstLetter(Purge(FindString(source, "<td class=\"sortC\">", "&nbsp;", posBegin)));
						if (posBegin != string::npos)
							loc.SetSSI("City", UppercaseFirstLetter(Purge(FindString(source, "<td class=\"sortC\">", "&nbsp;", posBegin))));
						if (posBegin != string::npos)
						{
							string tmp = FindString(source, "<td class=\"sortC\">", "&nbsp;", posBegin);
							MakeUpper(tmp);

							tmp = PurgeProvince(tmp);
							loc.SetSSI("State", tmp);
						}

						if (posBegin != string::npos)
							loc.SetSSI("Country", Purge(FindString(source, "<td class=\"sortC\">", "&nbsp;", posBegin)));

						StringVector latLonStr;
						if (posBegin != string::npos)
							latLonStr.Tokenize(FindString(source, "\">", "&nbsp;", posBegin), ",");

						if (posBegin != string::npos)
							loc.SetSSI("StationType", Purge(FindString(source, "<td class=\"sortC\">", "&nbsp;", posBegin)));
						ASSERT(latLonStr.size() == 2);

						//if (loc.GetSSI("Country") == "US")
							//break;//US station is also return fior this kind of request... hummm

						//if (loc.GetSSI("Country") == "CANADA")
							//loc.SetSSI("Country", "CA");


						if (latLonStr.size() == 2 && loc.GetSSI("Country") == "CA")
						{
							if (loc.m_name.empty())
								loc.m_name = loc.GetSSI("City");

							if (loc.m_name.empty())
							{
								loc.m_name = WBSF::GenerateNewName(lastName);
								lastName = loc.m_name;
							}


							loc.m_lat = ToDouble(latLonStr[0]);
							loc.m_lon = ToDouble(latLonStr[1]);
							stationList.push_back(loc);
						}

						posBegin = source.find("<tr>", posBegin);
						msg += callback.SetCurrentStepPos(posBegin);
					}

					callback.PopTask();
				}//if msg


				
			}//for all pages

			callback.AddMessage(GetString(IDS_NB_STATIONS) + ToString(stationList.size()));
			
		}
		else
		{
			//Download Canada
			//de start=11680 à start=15800, il faut laisser tomber les US
			//callback.PushTask(GetString(IDS_LOAD_STATION_LIST) + " (Canada)", size_t(15800 - 11680), 20);
			//string lastName = "Unknown1";

			//for (int start = 15800; start <= 15800 && msg; start += 20)
			//{
			//	string URL = FormatA("weatherstation/ListStations.asp?start=%d", start);

			//	string source;
			//	msg = GetPageText(pConnection, URL, source);
			//	if (msg)
			//	{
			//		string::size_type posBegin = source.find("Search Results");

			//		if (posBegin != string::npos)
			//			posBegin = source.find("<tbody>", posBegin);

			//		if (posBegin != string::npos)
			//			posBegin = source.find("<tr>", posBegin);


			//		while (posBegin != string::npos)
			//		{
			//			CLocation loc;
			//			loc.m_ID = Purge(FindString(source, "?ID=", "\">", posBegin));
			//			if (posBegin != string::npos)
			//				loc.m_name = Purge(FindString(source, "<td class=\"sortC\">", "&nbsp;", posBegin));
			//			if (posBegin != string::npos)
			//				loc.SetSSI("City", Purge(FindString(source, "<td class=\"sortC\">", "&nbsp;", posBegin)));
			//			if (posBegin != string::npos)
			//			{
			//				string tmp = FindString(source, "<td class=\"sortC\">", "&nbsp;", posBegin);
			//				MakeUpper(tmp);

			//				tmp = PurgeProvince(tmp);
			//				loc.SetSSI("State", tmp);
			//			}

			//			if (posBegin != string::npos)
			//				loc.SetSSI("Country", Purge(MakeUpper(FindString(source, "<td class=\"sortC\">", "&nbsp;", posBegin))));

			//			StringVector latLonStr;
			//			if (posBegin != string::npos)
			//				latLonStr.Tokenize(FindString(source, "\">", "&nbsp;", posBegin), ",");

			//			if (posBegin != string::npos)
			//				loc.SetSSI("StationType", Purge(FindString(source, "<td class=\"sortC\">", "&nbsp;", posBegin)));
			//			ASSERT(latLonStr.size() == 2);

			//			if (loc.GetSSI("Country") == "US")
			//				break;//US station is also return fior this kind of request... hummm

			//			if (loc.GetSSI("Country") == "CANADA")
			//				loc.SetSSI("Country", "CA");


			//			if (latLonStr.size() == 2 && loc.GetSSI("Country") == "CA")
			//			{
			//				if (loc.m_name.empty())
			//					loc.m_name = loc.GetSSI("City");

			//				if (loc.m_name.empty())
			//				{
			//					loc.m_name = WBSF::GenerateNewName(lastName);
			//					lastName = loc.m_name;
			//				}


			//				loc.m_lat = ToDouble(latLonStr[0]);
			//				loc.m_lon = ToDouble(latLonStr[1]);
			//				stationList.push_back(loc);
			//			}

			//			posBegin = source.find("<tr>", posBegin);
			//		}
			//	}//if msg


			//	msg += callback.StepIt();
			//}//for all pages

			//callback.AddMessage(GetString(IDS_NB_STATIONS) + ToString(stationList.size()));
			//callback.PopTask();
		}

		

		pConnection->Close();
		pSession->Close();


		return msg;
	}

	TVarH CUIWunderground::GetVar(const string& str)
	{
		TVarH v=H_SKIP;
		
		enum {NB_VARS=7};
		static const char* VAR_NAME[NB_VARS] = { "Temp.:", "DewPoint:", "Humidity:", "WindSpeed:", "Pressure:", "Precipitation:", "SolarRadiation:" };
		static const TVarH VAR_AVAILABLE[NB_VARS] = { H_TAIR2, H_TDEW, H_RELH, H_WNDS, H_PRES, H_PRCP, H_SRAD2 };
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
		
		while (posBegin != string::npos )
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
		msg += GetHttpConnection("i.wund.com", pConnection, pSession);

		if (!msg)
			return msg;



		//Get elevation
		callback.PushTask("Extract elevation", stationList.size());

		for (size_t i = 0; i < stationList.size()&&msg; i++)
		{
			string URL = FormatA("weatherstation/WXDailyHistory.asp?ID=%s&month=4&day=9&year=2016", stationList[i].m_ID.c_str());

			string source;
			msg = GetPageText(pConnection, URL, source);
			if (msg)
			{
				//http://i.wund.com/auto/iphone/weatherstation/WXDailyHistory.asp?ID=KCATUSTI15&month=4&day=9&year=2016

				string variables;
				string::size_type posBegin = source.find("Statistics for the rest of the month");
				
				if(posBegin!=string::npos)
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

			if (((i+1) % 200)==0)
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
		

		CCountrySelection countries(Get(COUNTRIES));
		CStateSelection states(Get(STATES));
		CProvinceSelection provinces(Get(PROVINCE));
		

		//if (as<bool>(UPDATE_STATION_LIST))
		if (UPDATE_STATIONS_LIST)
		{
			size_t n = 0;
			if (countries.at("US"))
			{
				CLocationVector tmp;
				msg = DownloadStationList("US", tmp, callback);
				if (msg)
					msg = tmp.Save(GetStationListFilePath("US"), ',', callback);
				n++;
			}

			if (msg && countries.at("CA"))
			{
				CLocationVector tmp;
				msg = DownloadStationList("CA", tmp, callback);
				if (msg)
					msg = tmp.Save(GetStationListFilePath("CA"), ',', callback);
				n++;
			}

			if (msg && countries.count()>n)//other country than US and CA
			{
				CLocationVector tmp;
				msg = DownloadStationList("", tmp, callback);
				if (msg)
					msg = tmp.Save(GetStationListFilePath(""), ',', callback);
				n++;
			}

			if (!msg)
				return msg;
		}

		

		if (UPDATE_STATIONS_INFO)
		{
			CLocationVector stationList1;
			LoadStationList(stationList1, callback);
			return ExtractElevation(stationList1, callback);
		}
			


		CLocationVector stationList1;
		LoadStationList(stationList1, callback);

		
	

		//Get station
		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = lastYear - firstYear + 1;


		callback.PushTask("Clean list (" + ToString(stationList1.size()) + ")", stationList1.size());
		
		CLocationVector stationList;
		stationList.reserve(stationList1.size());
		for (auto it = stationList1.begin(); it < stationList1.end() && msg; it++)
		{
			string countryStr = it->GetSSI("Country");
			//size_t country = CCountrySelection::GetCountry(countryStr.c_str());

			bool bDownload = false;
			if (countries.at(countryStr))
			{
				if (countryStr == "US" || countryStr == "CA")
				{
					string stateStr = it->GetSSI("State");
					
					if (states.at(stateStr))
						bDownload = true;
				}
				else
				{
					bDownload = true;
				}//use this state if USA
			}//if country selection

			if (bDownload)
				stationList.insert(stationList.end(), *it);
				//it++;
			//else
				//it = stationList.erase(it);

			msg += callback.StepIt();
		}

		callback.PopTask();

		
		callback.PushTask("Download stations data (" + ToString(stationList.size()) + ")", stationList.size()*nbYears);

		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;
		msg += GetHttpConnection(SERVER_NAME, pConnection, pSession);

		if (!msg)
			return msg;


		int nbDownload = 0;
		for (size_t i = 0; i < stationList.size(); i++)
		{
			for (size_t y = 0; y < nbYears&&msg; y++)
			{
				int year = firstYear + int(y);

				static const char* URL_FORMAT = "weatherstation/WXDailyHistory.asp?ID=%s&day=1&month=1&year=%d&dayend=31&monthend=12&yearend=%d&graphspan=custom&format=1";
				string URL = FormatA(URL_FORMAT, stationList[i].m_ID.c_str(), year, year);
				string ouputFilePath = GetOutputFilePath(stationList[i].GetSSI("Country"), stationList[i].GetSSI("State"), stationList[i].m_ID, year);
				CreateMultipleDir(GetPath(ouputFilePath));

				string source;
				msg = GetPageText(pConnection, URL, source, false, FLAGS);

				if (!source.empty() && source.find("An error occurred while processing your request") == string::npos)
				{
					if (source[0] == '\n')
						source.erase(source.begin());

					ReplaceString(source, "\n<br>", "");
					ReplaceString(source, "<br>", "");

					ofStream file;
					msg = file.open(ouputFilePath);
					if (msg)
					{
						file << source;
						file.close();
						nbDownload++;
					}
				}

				msg += callback.StepIt();
			}
		}

		pConnection->Close();
		pSession->Close();


		callback.AddMessage(GetString(IDS_NB_FILES_DOWNLOADED) + ToString(nbDownload), 2);
		callback.PopTask();
		



		return msg;
	}


	string ANSI_2_ASCII(string str)
	{
		int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);

		std::wstring w_text;
		w_text.resize(len);
		MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &(w_text[0]), len);

		//Convert UTF16 to ASCII encoding
		static const UINT US_ASCII = 20127;
		int newLen = WideCharToMultiByte(US_ASCII, 0, w_text.c_str(), -1, NULL, 0, 0, 0);

		
		str.resize(newLen);
		WideCharToMultiByte(US_ASCII, 0, w_text.c_str(), -1, &(str[0]), newLen, 0, 0);
		str.resize(strlen(str.c_str()));
		
		return str;
	}



	ERMsg CUIWunderground::LoadStationList(CLocationVector& stationList, CCallback& callback)const
	{
		ERMsg msg;

		stationList.clear();

		CCountrySelection countries(Get(COUNTRIES));
		CStateSelection states(Get(STATES));
		CProvinceSelection provinces(Get(PROVINCE));

		CLocationVector tmp;
		size_t n = 0;
		if (countries.at("US"))
		{
			msg = tmp.Load(GetStationListFilePath("US"), ",", callback);
			stationList.insert(stationList.end(), tmp.begin(), tmp.end());
			n++;
		}

		if (countries.at("CA"))
		{
			msg = tmp.Load(GetStationListFilePath("CA"), ",", callback);
			stationList.insert(stationList.end(), tmp.begin(), tmp.end());
			n++;
		}

		if (countries.count()>n)//other country than US and CA
		{
			msg = tmp.Load(GetStationListFilePath(""), ",", callback);
			stationList.insert(stationList.end(), tmp.begin(), tmp.end());
			n++;
		}

		return msg;
	}

	ERMsg CUIWunderground::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;

		
		msg = LoadStationList(m_stations, callback);

		if (msg)
		{

			CCountrySelection countries(Get(COUNTRIES));
			CStateSelection states(Get(STATES));
			CProvinceSelection provinces(Get(PROVINCE));



			//t firstYear = as<int>(FIRST_YEAR);
			//int lastYear = as<int>(LAST_YEAR);

			for (CLocationVector::const_iterator it = m_stations.begin(); it != m_stations.end(); it++)
			{
				string country = it->GetSSI("Country");

				bool bDownload = false;
				if (countries.at(country))
				{
					if (country == "US" || country == "CA")
					{
						string state = it->GetSSI("State");
						
						if (states.at(state))
							bDownload = true;
					}
					else
					{
						bDownload = true;
					}//use this state if USA
				}//if country selection

				if (bDownload)
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

		station.m_name = WBSF::PurgeFileName(station.m_name);
		station.m_name = ANSI_2_ASCII(station.m_name);
		//station.m_ID;// += "H";//add a "H" for hourly data

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
			string filePath = GetOutputFilePath(station.GetSSI("Country"), station.GetSSI("State"), ID, year);
			if (FileExists(filePath))
			{
				msg = ReadData(filePath, TM, station, callback);
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

	ERMsg CUIWunderground::ReadData(const string& filePath, CTM TM, CWeatherStation& data, CCallback& callback)const
	{
		ERMsg msg;

		string path = GetPath(filePath);

		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		bool bNetatmo = WBSF::Find("Netatmo", data.GetSSI("StationType"), false);
		bool bEmptyST = data.GetSSI("StationType").empty();

		//now extact data 
		ifStream file;

		msg = file.open(filePath);

		if (msg)
		{
			enum{ C_DATE, C_TMAX, C_TAIR, C_TMIN, C_DMAX, C_TDEW, C_DMIN, C_HMAX, C_RELH, C_HMIN, C_PMAX, C_PMIN, C_WMAX, C_WNDS, C_GUST, C_PRCP, NB_COLUMNS};
			//Date,TemperatureHighC,TemperatureAvgC,TemperatureLowC,DewpointHighC,DewpointAvgC,DewpointLowC,HumidityHigh,HumidityAvg,HumidityLow,PressureMaxhPa,PressureMinhPa,WindSpeedMaxKMH,WindSpeedAvgKMH,GustSpeedMaxKMH,PrecipitationSumCM
			for (CSVIterator loop(file); loop != CSVIterator(); ++loop)
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

							data[TRef].SetStat(H_TAIR2, Tair);
						}

						if (Tmin != -DBL_MAX && Tmax != -DBL_MAX && Tmin > -60 && Tmin < 50 && Tmax > -60 && Tmax < 50)
						{
							ASSERT(Tmin > -60 && Tmin < 50);
							ASSERT(Tmax > -60 && Tmax < 50);


							if (Tmin > Tmax)
								Switch(Tmin, Tmax);

							data[TRef].SetStat(H_TMIN2, Tmin);
							data[TRef].SetStat(H_TMAX2, Tmax);
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
		}//if load 

		return msg;
	}




	

}
