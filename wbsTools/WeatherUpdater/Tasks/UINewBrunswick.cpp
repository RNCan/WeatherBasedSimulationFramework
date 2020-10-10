#include "StdAfx.h"
#include "UINewBrunswick.h"
#include "UIManitoba.h"
#include <boost\dynamic_bitset.hpp>
#include <boost\filesystem.hpp>

#include "Basic/DailyDatabase.h"
#include "Basic/FileStamp.h"
#include "UI/Common/SYShowMessage.h"
#include "Basic\CSV.h"
#include "Basic\UtilZen.h"
#include "Basic/CallcURL.h"

#include "TaskFactory.h"
#include "../Resource.h"
#include "WeatherBasedSimulationString.h"


#include "UI/Common/UtilWin.h"
#include "Basic/decode_html_entities_utf8.h"

using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace UtilWWW;
using namespace boost;

//public fire available here:
//https://www1.gnb.ca/0079/FireWeather/FireWeatherHourly-e.asp?stn=Alward

//public agriculture data available here:
//https://agri.gnb.ca/010-001/WebServiceData.aspx



namespace WBSF
{

	const char* CUINewBrunswick::SERVER_NAME[NB_NETWORKS] = { "ftp.gnb.ca", "ftp.gnb.ca", "www1.gnb.ca", "agri.gnb.ca", "agri.gnb.ca" };


	//*********************************************************************
	const char* CUINewBrunswick::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "Network", "FirstYear", "LastYear", "UserName", "Password",  "ShowCURL" };
	const size_t CUINewBrunswick::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_STRING_SELECT, T_STRING, T_STRING, T_STRING, T_PASSWORD, T_BOOL };
	const UINT CUINewBrunswick::ATTRIBUTE_TITLE_ID = IDS_UPDATER_NEWBRUNSWICK_P;
	const UINT CUINewBrunswick::DESCRIPTION_TITLE_ID = ID_TASK_NEWBRUNSWICK;

	const char* CUINewBrunswick::CLASS_NAME() { static const char* THE_CLASS_NAME = "NewBrunswick";  return THE_CLASS_NAME; }
	CTaskBase::TType CUINewBrunswick::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUINewBrunswick::CLASS_NAME(), (createF)CUINewBrunswick::create);

	const char* CUINewBrunswick::NETWORK_NAME[NB_NETWORKS]{ "FireHistorical", "FirePrivate", "FirePublic", "AgriD", "AgriH" };

	size_t CUINewBrunswick::GetNetworkFromName(string name)
	{
		size_t n = NOT_INIT;
		for (size_t i = 0; i < NB_NETWORKS && n == NOT_INIT; i++)
			if (WBSF::IsEqual(name, NETWORK_NAME[i]))
				n = i;

		return n;
	}




	enum TFireColumns { C_STATION_NAME, C_YEAR, C_MONTH, C_DAY, C_DATE, C_TIME, C_STATION_ID, C_TEMP, C_RH, C_DIR, C_WSPD, C_MX_SPD, C_RN_1, C_RN_24, C_PG_1HR, C_PG_24, C_HFFMC, C_HISI, C_HFWI, C_RN24, C_PG_24HR, C_FFMC, C_DMC, C_DC, C_ISI, C_BUI, C_FWI, C_DSR, C_TMAX, C_TMAX24, C_TMIN, C_TMIN24, NB_COLUMNS };
	static const char* COLUMN_NAME[NB_COLUMNS] = { "StationName", "Year", "Month", "Day", "Date", "Time", "StationID", "Temp", "Rh", "Dir", "Wspd", "Mx_Spd", "Rn_1", "rn_24", "PG_1hr", "pg_24", "hFFMC", "hISI", "hFWI", "Rn24", "PG_24hr", "FFMC", "DMC", "DC", "ISI", "BUI", "FWI", "DSR", "TMax", "TMax24", "TMin", "TMin24" };



	static size_t GetNBColumn(const string& header)
	{
		size_t c = NOT_INIT;
		for (size_t i = 0; i < NB_COLUMNS&&c == NOT_INIT; i++)
		{
			if (IsEqual(header, COLUMN_NAME[i]))
				c = i;
		}

		return c;
	}

	static vector<size_t> GetNBColumns(const StringVector& header)
	{
		vector<size_t> columns(header.size());


		for (size_t c = 0; c < header.size(); c++)
			columns[c] = GetNBColumn(header[c]);

		return columns;
	}


	static size_t GetNBVariable(bool bHourly, size_t type)
	{
		size_t v = NOT_INIT;

		if (bHourly)
		{
			if (type == C_RH)
				v = H_RELH;
			else if (type == C_DIR)
				v = H_WNDD;
			else if (type == C_WSPD)
				v = H_WNDS;
			else if (type == C_RN_1)
				v = H_PRCP;
			else if (type == C_TMIN)
				v = H_TMIN;
			else if (type == C_TEMP)
				v = H_TAIR;
			else if (type == C_TMAX)
				v = H_TMAX;
		}
		else
		{
			if (type == C_RN24)
				v = H_PRCP;
			else if (type == C_TMIN24)
				v = H_TMIN;
			else if (type == C_TMAX24)
				v = H_TMAX;
		}

		return v;
	}

	static vector<size_t>  GetNBVariables(bool bHourly, const vector<size_t>& columns)
	{
		vector<size_t> vars(columns.size());


		for (size_t c = 0; c < columns.size(); c++)
			vars[c] = GetNBVariable(bHourly, columns[c]);

		return vars;
	}

	CUINewBrunswick::CUINewBrunswick(void)
	{}

	CUINewBrunswick::~CUINewBrunswick(void)
	{}



	std::string CUINewBrunswick::Option(size_t i)const
	{
		string str;
		switch (i)
		{
		case NETWORK:	str = "FireHistorical=Fire (historical)|FirePrivate=fire (current, need user and password)|FirePublic=fire (current)|AgriD=Agriculture(daily)|AgriH=Agriculture(hourly)"; break;
		};
		return str;
	}

	std::string CUINewBrunswick::Default(size_t i)const
	{
		string str;

		switch (i)
		{
		case WORKING_DIR: str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()) + "New-Brunswick\\"; break;
		case FIRST_YEAR:
		case LAST_YEAR:	str = ToString(CTRef::GetCurrentTRef().GetYear()); break;
		case SHOW_CURL: str = "0"; break;
		};

		return str;
	}

	//****************************************************
	ERMsg CUINewBrunswick::GetFileList(size_t n, CFileInfoVector& fileList, CCallback& callback)const
	{
		ERMsg msg;

		fileList.clear();

		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = lastYear - firstYear + 1;
		CTRef currentTRef = CTRef::GetCurrentTRef();


		if (n == FIRE_HISTORICAL || n == FIRE_PRIVATE)
		{
			//open a connection on the server
			string str;
			CInternetSessionPtr pSession;
			CFtpConnectionPtr pConnection;
			msg = GetFtpConnection(SERVER_NAME[n], pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, Get(USER_NAME), Get(PASSWORD), true, 5, callback);
			if (msg)
			{

				//historical data...
				if (n == FIRE_HISTORICAL)
				{
					CFileInfoVector dir;
					msg = UtilWWW::FindDirectories(pConnection, "/", dir);
					if (msg)
					{
						callback.PushTask("Find files from directories (nb directories = " + ToString(dir.size()) + ")", dir.size());
						//download all file *.txt
						for (size_t d = 0; d < dir.size() && msg; d++)
						{
							CFileInfoVector tmp;
							msg = UtilWWW::FindFiles(pConnection, dir[d].m_filePath + "*.txt", tmp);

							for (size_t i = 0; i < tmp.size(); i++)
							{
								string filePath = GetOutputFilePath(n, tmp[i].m_filePath, -1);
								if (!FileExists(filePath))
									fileList.push_back(tmp[i]);
							}



							msg += callback.StepIt();
						}

						callback.PopTask();
					}
				}
				else if (n == FIRE_PRIVATE)
				{

					//current data
					msg = UtilWWW::FindFiles(pConnection, "*.csv", fileList, false, callback);

					//CLean List
					for (CFileInfoVector::iterator it = fileList.begin(); it != fileList.end(); )
					{

						string title = GetFileTitle(it->m_filePath);
						ReplaceString(title, "Yr ", "");

						if (find_if(m_stations.begin(), m_stations.end(), FindLocationByName(title)) != m_stations.end())
							it++;
						else
							it = fileList.erase(it);


					}
				}


				pConnection->Close();
				pSession->Close();
			}
		}
		else
		{
			ASSERT(false);
		}


		return msg;
	}

	ERMsg CUINewBrunswick::GetFileList(size_t n, StringVector& fileList, CCallback& callback)const
	{
		ERMsg msg;

		fileList.clear();

		if (msg)
		{

			if (n == FIRE_HISTORICAL || n == FIRE_PRIVATE)
			{
				ASSERT(false);
			}
			else if (n == FIRE_PUBLIC)
			{
				//https://www1.gnb.ca/0079/FireWeather/FireWeatherHourly-e.asp?Stn=all
				CInternetSessionPtr pSession;
				CHttpConnectionPtr pConnection;

				msg = GetHttpConnection(SERVER_NAME[FIRE_PUBLIC], pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", false, 5, callback);
				if (msg)
				{
					string str;
					msg = UtilWWW::GetPageText(pConnection, "0079/FireWeather/FireWeatherHourly-e.asp?Stn=all", str);
					if (msg)
					{
						string::size_type posBegin = str.find("</THEAD>");
						string::size_type posEnd = str.find("</TABLE>");

						while (posBegin != string::npos && posEnd != string::npos && posBegin < posEnd)
						{
							string str1 = FindString(str, "<TD>", "</TD>", posBegin);
							string str2 = FindString(str1, "stn=", "'");
							fileList.push_back(str2);

							posBegin = str.find("<TD>", posBegin);
						}
					}

					//clean connection
					pConnection->Close();
					pSession->Close();
				}
			}
			else if (n == AGRI_HOURLY)
			{
				string argument = "-s \"https://agri.gnb.ca/010-001/archive.aspx\"";
				string exe = GetApplicationPath() + "External\\curl.exe";
				CCallcURL cURL(exe);

				string source;
				msg = cURL.get_text(argument, source);

				if (msg)
				{


					//CInternetSessionPtr pSession;
					//CHttpConnectionPtr pConnection;

					//msg = GetHttpConnection(SERVER_NAME[AGRI], pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", false, 5, callback);
					//if (msg)
					//{
						//string str;
						//msg = UtilWWW::GetPageText(pConnection, "010-001/archive.aspx", str);
						//if (msg)
						//{
					string::size_type pos = source.find("id=\"ctl00_Content1_lblParish\"");
					if (pos != string::npos)
					{
						string::size_type pos1 = source.find("<select ", pos);
						string::size_type pos2 = source.find("</select>", pos);

						if (pos1 != string::npos && pos2 != string::npos)
						{
							string str = source.substr(pos1, pos2 - pos1 + 9);
							ReplaceString(str, "<br />", "");
							string xml_str = "<?xml version=\"1.0\" encoding=\"Windows-1252\"?>\r\n" + str;

							try
							{
								zen::XmlDoc doc = zen::parse(xml_str);

								zen::XmlIn in(doc.root());
								for (zen::XmlIn it = in["option"]; it; it.next())
								{
									string value;
									//it(value);
									it.get()->getAttribute("value", value);
									fileList.push_back(value);
								}//for all station
							}
							catch (const zen::XmlParsingError& e)
							{
								// handle error
								msg.ajoute("Error parsing XML file: col=" + WBSF::ToString(e.col) + ", row=" + WBSF::ToString(e.row));
							}
						}
					}
				}

				//clean connection
				//pConnection->Close();
				//pSession->Close();
				//}
			}
		}

		return msg;
	}

	double Convert(int ID, TVarH v, double value)
	{
		//some station are in metric (°C, mm, km/h) and some station are in imperial (°F, po, miles/hour)
		std::array<int, 11> IMPERIAL_ID = { { 36,47,51,53,55,59,62,65,68,69,70} };


		if (find(IMPERIAL_ID.begin(), IMPERIAL_ID.end(), ID) != IMPERIAL_ID.end())
		{
			if (v == H_TAIR || v == H_TMAX || v == H_TMIN || v == H_TDEW)
				value = ((value - 32.0)*5.0 / 9.0); //°F --> °C
			else if (v == H_WNDS)
				value = value * 1.60934;//miles/hour --> km/hour
			else if (v == H_PRCP)
				value = value * 25.4;//in --> mm
		}


		return value;
	}

	//a conserver pour les données historique horaire agri
	//https://agri.gnb.ca/010-001/archive.aspx
	ERMsg CUINewBrunswick::SaveAgriStationHourly(const std::string& filePath, std::string str)
	{
		ERMsg msg;

		CWeatherYears data(true);

		string strID = GetFileTitle(filePath);

		enum THourlyColumns { C_DATE_TIME, C_TOUTSIDE, C_TMAX, C_TMIN, C_HUMIDITY, C_TDEW, C_WSPD, C_DIR, C_RAIN, C_TSOIL, NB_COLUMNS };
		static const TVarH COL_POS[NB_COLUMNS] = { H_SKIP, H_TAIR, H_TMAX, H_TMIN, H_RELH, H_TDEW, H_WNDS, H_WNDD, H_PRCP, H_SKIP };

		try
		{
			CWeatherAccumulator accumulator(CTM(CTM::HOURLY, CTM::FOR_EACH_YEAR));

			int ID = ToInt(strID);
			zen::XmlDoc doc = zen::parse(str);

			zen::XmlIn in(doc.root());
			for (zen::XmlIn itTR = in["tr"]; itTR; itTR.next())
			{
				StringVector tmp;
				for (zen::XmlIn itTD = itTR["td"]; itTD; itTD.next())
				{
					string value;
					zen::XmlIn itSpan = itTD["font"]["span"];
					itSpan(value);
					if (!value.empty())
						tmp.push_back(value);
				}//for all columns


				if (tmp.size() == NB_COLUMNS)
				{
					StringVector date(tmp[C_DATE_TIME], " /-:");
					ASSERT(date.size() == 7);

					int year = ToInt(date[0]);
					size_t month = ToInt(date[1]) - 1;
					size_t day = ToInt(date[2]) - 1;
					size_t hour = ToInt(date[3]);
					size_t minute = ToInt(date[4]);
					

					//if (minute == 0)
					{
						ASSERT(month >= 0 && month < 12);
						ASSERT(day >= 0 && day < GetNbDayPerMonth(year, month));
						ASSERT(hour >= 0 && hour < 24);

						if (date[6] == "AM" && hour == 12)
							hour = 0;
						else if (date[6] == "PM" && hour == 12)
							hour = 12;
						else if (date[6] == "PM")
							hour += 12;

						CTRef TRef = CTRef(year, month, day, hour);


						if (accumulator.TRefIsChanging(TRef))
						{
							data[accumulator.GetTRef()].SetData(accumulator);
						}

						for (size_t i = 0; i < NB_COLUMNS; i++)
						{
							if (COL_POS[i] != H_SKIP)
							{
								if (COL_POS[i] == H_WNDD)
									tmp[i] = ToString(CUIManitoba::GetWindDir(tmp[i]));

								double value = ToDouble(tmp[i]);
								if (COL_POS[i] == H_PRCP )
								{
									//no precipitation before may abd after november
									if(month<MAY || month>OCTOBER)
										value = -99;
									else if(strID =="45"|| strID =="76")//invalid precipitation for this 2 stations
										value = -99;
								}

								if (value > -99)
								{
									value = Convert(ID, COL_POS[i], value);
									accumulator.Add(TRef, COL_POS[i], value);
								}
							}
						}
					}
				}//is valid
			}//for all line

			if (accumulator.GetTRef().IsInit())
			{
				data[accumulator.GetTRef()].SetData(accumulator);
			}

			if (msg)
			{
				ASSERT(data.size() == 1);
				//save data
				for (auto it1 = data.begin(); it1 != data.end(); it1++)
				{
					msg += it1->second->SaveData(filePath);
				}
			}//if msg
		}
		catch (const zen::XmlParsingError& e)
		{
			// handle error
			msg.ajoute("Error parsing XML file: col=" + WBSF::ToString(e.col) + ", row=" + WBSF::ToString(e.row));
		}


		return msg;
	}


	static TVarH GetVar(const string& header)
	{
		TVarH var = H_SKIP;
		static const char* VAR_NAME[] = { "CHU", "GDD", "LBSV", "Rain", "PDays", "Tmax", "Tmin", "SoilT", "FHB" };
		StringVector tmp(header, "_");
		ASSERT(tmp.size() == 2);

		if (tmp[1] == VAR_NAME[3])
			var = H_PRCP;
		else if (tmp[1] == VAR_NAME[5])
			var = H_TMAX;
		else if (tmp[1] == VAR_NAME[6])
			var = H_TMIN;

		return var;
	}

	static size_t GetDayIndex(const string& header)
	{
		size_t i = NOT_INIT;
		StringVector tmp(header, "_");
		ASSERT(tmp.size() == 2);

		i = ToSizeT(tmp[0].substr(1)) - 1;
		ASSERT(i < 366);

		return i;
	}


	ERMsg CUINewBrunswick::SplitAgriStation(const std::string& filePath)
	{
		ERMsg msg;

		

		enum THourlyColumns { C_STATIONID, C_LAT, C_LONG, C_FROMDATE, C_TODATE, C_FIRST_DATA, NB_COLUMNS };
		//static const TVarH COL_POS[NB_COLUMNS] = { H_SKIP, H_TAIR, H_TMAX, H_TMIN, H_RELH, H_TDEW, H_WNDS, H_WNDD, H_PRCP, H_SKIP };


		ifStream file;
		msg = file.open(filePath);

		if (msg)
		{
			for (CSVIterator loop(file); loop != CSVIterator() && msg; ++loop)
			{
				CWeatherYears data(false);
				double last_prcp = 0;


				string ID = (*loop)[C_STATIONID];
				StringVector time((*loop)[C_FROMDATE], "/");
				ASSERT(time.size() == 3);

				int year = ToInt(time[2]);
				size_t month = ToInt(time[1]) - 1;
				size_t day = ToInt(time[0]) - 1;

				ASSERT(month >= 0 && month < 12);
				ASSERT(day >= 0 && day < GetNbDayPerMonth(year, month));

				CTRef TRef = CTRef(year, month, day);
				//ASSERT(TRef.GetJDay() == 0);

				for (size_t c = C_FIRST_DATA; c < loop->size(); c++)
				{
					TVarH var = GetVar(loop.Header()[c]);
					if (var != H_SKIP)
					{
						size_t d = GetDayIndex(loop.Header()[c]);
						ASSERT((TRef + d).GetYear() == year);

						string str = TrimConst((*loop)[c]);
						if (!str.empty())
						{
							double value = ToDouble(str);
							if (var == H_PRCP)
							{
								double tmp = value;
								value = max(0.0, value -last_prcp);
								ASSERT(value >= 0);
								last_prcp= tmp;
							}

							data.GetDay(TRef + d).SetStat(var, value);
						}
					}
				}

				string data_filepath = GetOutputFilePath(AGRI_DAILY, ID, year);
				msg += data.SaveData(data_filepath, CTM(CTM::DAILY) );
			
			}//for all line 
		}//if open file


		return msg;
	}

	std::bitset<CUINewBrunswick::NB_NETWORKS> CUINewBrunswick::GetNetWork()const
	{
		std::bitset<NB_NETWORKS> network;

		StringVector str(Get(NETWORK), "|;,");
		if (str.empty())
		{
			network.set();
		}
		else
		{

			for (size_t i = 0; i < str.size(); i++)
			{
				size_t n = GetNetworkFromName(str[i]);
				ASSERT(n < NB_NETWORKS);
				network.set(n);
			}
		}

		return network;
	}

	ERMsg  CUINewBrunswick::UpdateOldFile(CCallback& callback)
	{
		ERMsg msg;
		StringVector fileList = GetFilesList("G:\\NewBrunswick\\Fire\\old\\*.csv", 2, true);
		callback.PushTask("update", fileList.size());
		for (size_t i = 0; i < fileList.size() && msg; i++)
		{
			string title = GetFileTitle(fileList[i]);
			ReplaceString(title, "Yr ", "");
			ReplaceString(title, "yr ", "");
			ReplaceString(title, "St.", "Saint ");
			ReplaceString(title, "St", "Saint");
			ReplaceString(title, " 2016", "");
			ReplaceString(title, " 2017", "");
			ReplaceString(title, " 2018", "");
			ReplaceString(title, " 2019", "");
			msg += MergeData(title, fileList[i], callback);
			msg += callback.StepIt();
		}

		callback.PopTask();
		return msg;
	}

	ERMsg CUINewBrunswick::Execute(CCallback& callback)
	{
		//return UpdateOldFile(callback);


		ERMsg msg;

		std::bitset<NB_NETWORKS> network = GetNetWork();

		for (size_t n = 0; n < network.size() && msg; n++)
		{
			if (network[n])
			{
				switch (n)
				{
				case FIRE_HISTORICAL: msg += ExecutePrivateFire(n, callback); break;
				case FIRE_PRIVATE:
					if (m_stations.empty())
						msg = m_stations.Load(GetStationListFilePath());
					msg += ExecutePrivateFire(n, callback); break;
				case FIRE_PUBLIC: msg += ExecutePublicFire(callback); break;
				case AGRI_DAILY: msg += ExecuteAgricultureDaily(callback); break;
				case AGRI_HOURLY: msg += ExecuteAgricultureHourly(callback); break;
				}
			}
		}


		return msg;
	}

	string url_encode(const string &value)
	{
		ostringstream escaped;
		escaped.fill('0');
		escaped << hex;

		for (string::const_iterator i = value.begin(), n = value.end(); i != n; ++i) {
			string::value_type c = (*i);

			// Keep alphanumeric and other accepted characters intact
			if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
				escaped << c;
				continue;
			}

			// Any other characters are percent-encoded
			escaped << uppercase;
			escaped << '%' << setw(2) << int((unsigned char)c);
			escaped << nouppercase;
		}

		return escaped.str();
	}

	ERMsg CUINewBrunswick::DownloadAgriStationDaily(const std::string& file_path, int year)
	{

		ERMsg msg;

		bool bShowCurl = as<bool>(SHOW_CURL);

		string URL = "https://agri.gnb.ca/010-001/WebServiceData.aspx";
		string strHeaders = "-H \"Content-Type: application/x-www-form-urlencoded\"";

		string argument = "-s \""+ URL +"\""; //-H \"Connection: keep-alive\" -H \"Upgrade-Insecure-Requests: 1\" -H \"User-Agent: Mozilla/5.0 (Windows NT 6.1; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/85.0.4183.121 Safari/537.36\" -H \"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9\" -H \"Sec-Fetch-Site: none\" -H \"Sec-Fetch-Mode: navigate\" -H \"Sec-Fetch-Dest: document\" -H \"Accept-Language: fr-FR,fr;q=0.9,en-US;q=0.8,en;q=0.7\"";
		string exe = GetApplicationPath() + "External\\curl.exe";
		CCallcURL cURL(exe);

		string source;
		msg = cURL.get_text(argument, source);

		if (msg)
		{
			//activate date
			string VIEWSTATE = url_encode(WBSF::FindString(source, "id=\"__VIEWSTATE\" value=\"", "\""));
			string VIEWSTATEGENERATOR = url_encode(WBSF::FindString(source, "id=\"__VIEWSTATEGENERATOR\" value=\"", "\""));
			string EVENTVALIDATION = url_encode(WBSF::FindString(source, "id=\"__EVENTVALIDATION\" value=\"", "\""));

			string strParam = "__EVENTTARGET=ctl00%24Content1%24cbNarrowSearch&__EVENTARGUMENT=&__LASTFOCUS=&";
			strParam += "__VIEWSTATE=" + VIEWSTATE + "&";
			strParam += "__VIEWSTATEGENERATOR=" + VIEWSTATEGENERATOR + "&";
			strParam += "__EVENTVALIDATION=" + EVENTVALIDATION + "&";
			strParam += "ctl00%24hfLang=fr-CA&";
			strParam += "ctl00%24Content1%24cbNarrowSearch=on&";
			strParam += "ctl00%24Content1%24ddlCalendarYear=" + ToString(year) + "&";
			strParam += "ctl00%24Content1%24tbFromDateMonth=01&";
			strParam += "ctl00%24Content1%24tbFromDateDay=01&";
			strParam += "ctl00%24Content1%24tbToDateMonth=12&";
			strParam += "ctl00%24Content1%24tbToDateDay=31&";

			string argument = "-s \"" + URL + "\" " + strHeaders + " --data-binary \"" + strParam + "\"";
			msg = cURL.get_text(argument, source);

			//request data 
			VIEWSTATE = url_encode(WBSF::FindString(source, "id=\"__VIEWSTATE\" value=\"", "\""));
			VIEWSTATEGENERATOR = url_encode(WBSF::FindString(source, "id=\"__VIEWSTATEGENERATOR\" value=\"", "\""));
			EVENTVALIDATION = url_encode(WBSF::FindString(source, "id=\"__EVENTVALIDATION\" value=\"", "\""));

			
			
			strParam = "__EVENTTARGET=&__EVENTARGUMENT=&__LASTFOCUS=&";
			strParam += "__VIEWSTATE=" + VIEWSTATE + "&";
			strParam += "__VIEWSTATEGENERATOR=" + VIEWSTATEGENERATOR + "&";
			strParam += "__EVENTVALIDATION=" + EVENTVALIDATION + "&";
			strParam += "ctl00%24hfLang=fr-CA&";
			strParam += "ctl00%24Content1%24cbNarrowSearch=on&";
			strParam += "ctl00%24Content1%24ddlCalendarYear=" + ToString(year) + "&";
			strParam += "ctl00%24Content1%24tbFromDateMonth=01&";
			strParam += "ctl00%24Content1%24tbFromDateDay=01&";
			strParam += "ctl00%24Content1%24tbToDateMonth=12&";
			strParam += "ctl00%24Content1%24tbToDateDay=31&";
			strParam += "ctl00%24Content1%24SelectAllParishes=on&ctl00%24Content1%24SelectAllParishes=on&ctl00%24Content1%24SelectAllStations=on&ctl00%24Content1%24chklstParishesWithWeatherStations%240=on&ctl00%24Content1%24chklstParishesWithWeatherStations%241=on&ctl00%24Content1%24chklstParishesWithWeatherStations%242=on&ctl00%24Content1%24chklstParishesWithWeatherStations%243=on&ctl00%24Content1%24chklstParishesWithWeatherStations%244=on&ctl00%24Content1%24chklstParishesWithWeatherStations%245=on&ctl00%24Content1%24chklstParishesWithWeatherStations%246=on&ctl00%24Content1%24chklstParishesWithWeatherStations%247=on&ctl00%24Content1%24chklstParishesWithWeatherStations%248=on&ctl00%24Content1%24chklstParishesWithWeatherStations%249=on&ctl00%24Content1%24chklstParishesWithWeatherStations%2410=on&ctl00%24Content1%24chklstParishesWithWeatherStations%2411=on&ctl00%24Content1%24chklstParishesWithWeatherStations%2412=on&ctl00%24Content1%24chklstParishesWithWeatherStations%2413=on&ctl00%24Content1%24chklstParishesWithWeatherStations%2414=on&ctl00%24Content1%24chklstParishesWithWeatherStations%2415=on&ctl00%24Content1%24chklstParishesWithWeatherStations%2416=on&ctl00%24Content1%24chklstParishesWithWeatherStations%2417=on&ctl00%24Content1%24chklstParishesWithWeatherStations%2418=on&ctl00%24Content1%24chklstParishesWithWeatherStations%2419=on&ctl00%24Content1%24chklstParishesWithWeatherStations%2420=on&ctl00%24Content1%24chklstParishesWithWeatherStations%2421=on&ctl00%24Content1%24chklstParishesWithWeatherStations%2422=on&ctl00%24Content1%24chklstParishesWithWeatherStations%2423=on&ctl00%24Content1%24chklstWeatherStationsToInclude%240=on&ctl00%24Content1%24chklstWeatherStationsToInclude%241=on&ctl00%24Content1%24chklstWeatherStationsToInclude%242=on&ctl00%24Content1%24chklstWeatherStationsToInclude%243=on&ctl00%24Content1%24chklstWeatherStationsToInclude%244=on&ctl00%24Content1%24chklstWeatherStationsToInclude%245=on&ctl00%24Content1%24chklstWeatherStationsToInclude%246=on&ctl00%24Content1%24chklstWeatherStationsToInclude%247=on&ctl00%24Content1%24chklstWeatherStationsToInclude%248=on&ctl00%24Content1%24chklstWeatherStationsToInclude%249=on&ctl00%24Content1%24chklstWeatherStationsToInclude%2410=on&ctl00%24Content1%24chklstWeatherStationsToInclude%2411=on&ctl00%24Content1%24chklstWeatherStationsToInclude%2412=on&ctl00%24Content1%24chklstWeatherStationsToInclude%2413=on&ctl00%24Content1%24chklstWeatherStationsToInclude%2414=on&ctl00%24Content1%24chklstWeatherStationsToInclude%2415=on&ctl00%24Content1%24chklstWeatherStationsToInclude%2416=on&ctl00%24Content1%24chklstWeatherStationsToInclude%2417=on&ctl00%24Content1%24chklstWeatherStationsToInclude%2418=on&ctl00%24Content1%24chklstWeatherStationsToInclude%2419=on&ctl00%24Content1%24chklstWeatherStationsToInclude%2420=on&ctl00%24Content1%24chklstWeatherStationsToInclude%2421=on&ctl00%24Content1%24chklstWeatherStationsToInclude%2422=on&ctl00%24Content1%24chklstWeatherStationsToInclude%2423=on&ctl00%24Content1%24chklstWeatherStationsToInclude%2424=on&ctl00%24Content1%24chklstWeatherStationsToInclude%2425=on&ctl00%24Content1%24chklstWeatherStationsToInclude%2426=on&ctl00%24Content1%24chklstWeatherStationsToInclude%2427=on&ctl00%24Content1%24chklstWeatherStationsToInclude%2428=on&ctl00%24Content1%24chklstWeatherStationsToInclude%2429=on&ctl00%24Content1%24chklstWeatherStationsToInclude%2430=on&ctl00%24Content1%24chklstWeatherStationsToInclude%2431=on&ctl00%24Content1%24chklstWeatherStationsToInclude%2432=on&ctl00%24Content1%24chklstWeatherStationsToInclude%2433=on&ctl00%24Content1%24chklstWeatherStationsToInclude%2434=on&ctl00%24Content1%24chklstWeatherStationsToInclude%2435=on&ctl00%24Content1%24chklstWeatherStationsToInclude%2436=on&ctl00%24Content1%24chklstWeatherStationsToInclude%2437=on&ctl00%24Content1%24chklstWeatherStationsToInclude%2438=on&";
			strParam += "ctl00%24Content1%24btnSubmit=Generate";

			
			argument = "-v \"" + URL + "\" "+ strHeaders +" --data-binary \"" + strParam + "\" --output \"" + file_path + "\"";
			string command = exe + " " + argument;

			DWORD exit_code;
			msg = WinExecWait(command, "", bShowCurl?SW_SHOW: SW_HIDE, &exit_code);
			if (exit_code == 0 && FileExists(file_path))
			{

				ifStream file;
				msg = file.open(file_path);

				if (msg)
				{
					string head(34, ' ');
					file.read(&head[0], head.size());
					file.close();

					if (!IsEqual(head,  "StationId,Lat,Long,Fromdate,Todate"))
					{
						msg.ajoute("NB agri data file do not begin with: StationId,Lat,Long,Fromdate,Todate");
						msg.ajoute(file_path);
					}
				}
			}
		}

		return msg;
	}


	ERMsg CUINewBrunswick::DownloadAgriStationHourly(const std::string& ID, int year, std::string& str)
	{

		ERMsg msg;


		string argument = "-s \"https://agri.gnb.ca/010-001/archive.aspx\" -H \"Connection: keep-alive\" -H \"Upgrade-Insecure-Requests: 1\" -H \"User-Agent: Mozilla/5.0 (Windows NT 6.1; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/85.0.4183.121 Safari/537.36\" -H \"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9\" -H \"Sec-Fetch-Site: none\" -H \"Sec-Fetch-Mode: navigate\" -H \"Sec-Fetch-Dest: document\" -H \"Accept-Language: fr-FR,fr;q=0.9,en-US;q=0.8,en;q=0.7\"";
		string exe = GetApplicationPath() + "External\\curl.exe";
		CCallcURL cURL(exe);

		string source;
		msg = cURL.get_text(argument, source);

		if (msg)
		{
			string VIEWSTATE = url_encode(WBSF::FindString(source, "id=\"__VIEWSTATE\" value=\"", "\""));
			string VIEWSTATEGENERATOR = url_encode(WBSF::FindString(source, "id=\"__VIEWSTATEGENERATOR\" value=\"", "\""));
			string EVENTVALIDATION = url_encode(WBSF::FindString(source, "id=\"__EVENTVALIDATION\" value=\"", "\""));

			string URL = "https://agri.gnb.ca/010-001/archive.aspx";
			string strHeaders = "-H \"Content-Type: application/x-www-form-urlencoded\"";
			string strParam = "__EVENTTARGET=&__EVENTARGUMENT=&";
			strParam += "__VIEWSTATE=" + VIEWSTATE + "&";
			strParam += "__VIEWSTATEGENERATOR=" + VIEWSTATEGENERATOR + "&";
			strParam += "__EVENTVALIDATION=" + EVENTVALIDATION + "&";
			strParam += "ctl00%24hfLang=fr-CA&";
			strParam += "ctl00%24Content1%24ddlWS=" + ID + "&";
			strParam += "ctl00%24Content1%24ddlFromDay=1&";
			strParam += "ctl00%24Content1%24ddlFromMonth=1&";
			strParam += "ctl00%24Content1%24ddlFromYear=" + ToString(year) + "&";
			strParam += "ctl00%24Content1%24hdnFromDate=&";
			strParam += "ctl00%24Content1%24ddlToDay=31&";
			strParam += "ctl00%24Content1%24ddlToMonth=12&";
			strParam += "ctl00%24Content1%24ddlToYear=" + ToString(year) + "&";
			strParam += "ctl00%24Content1%24hdnToDate=&";
			strParam += "ctl00%24Content1%24btnGetData=Submit";


			string argument = "-s \"" + URL + "\" "+ strHeaders +" --data-binary \"" + strParam + "\"";
			msg = cURL.get_text(argument, str);


			if (msg)
			{
				if (str.find("No data available") == string::npos)
				{
					string::size_type pos1 = str.find("<table class=\"gridviewBorder\"");
						string::size_type pos2 = str.find("</table>", pos1);

						if (pos1 != string::npos && pos2 != string::npos)
						{
							str = "<?xml version=\"1.0\" encoding=\"Windows-1252\"?>\r\n" + str.substr(pos1, pos2 - pos1 + 9);
								WBSF::ReplaceString(str, "\t", "");
						}
						else
						{
							msg.ajoute("Invalid data for station: " + ID);
						}
				}
				else
				{
					str.clear();
				}
			}
		}

		return msg;
	}



	ERMsg CUINewBrunswick::ExecutePublicFire(CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);
		msg = CreateMultipleDir(workingDir);

		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(workingDir, 1);
		callback.AddMessage(GetString(IDS_UPDATE_FROM));
		callback.AddMessage(string(SERVER_NAME[FIRE_PUBLIC]), 1);
		callback.AddMessage("");

		//		int firstYear = as<int>(FIRST_YEAR);
			//	int lastYear = as<int>(LAST_YEAR);
				//size_t nbYears = lastYear - firstYear + 1;
				//CTRef currentTRef = CTRef::GetCurrentTRef();


		StringVector fileList;
		GetFileList(FIRE_PUBLIC, fileList, callback);

		callback.PushTask("Download New-Brunswick public fire data (" + ToString(fileList.size()) + " stations)", fileList.size());
		callback.AddMessage("Download New-Brunswick public fire data (" + ToString(fileList.size()) + " stations)");

		size_t nbFiles = 0;
		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;

		msg = GetHttpConnection(SERVER_NAME[FIRE_PUBLIC], pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", false, 5, callback);
		if (msg)
		{
			for (size_t i = 0; i < fileList.size() && msg; i++)
			{
				string title = GetFileTitle(fileList[i]);
				ReplaceString(title, "+", " ");

				CWeatherYears data(true);

				string str;
				msg = UtilWWW::GetPageText(pConnection, "0079/FireWeather/FireWeatherHourly-e.asp?Stn=" + fileList[i], str);
				if (msg)
				{
					string::size_type pos1 = str.find("</THEAD></TR>");
					string::size_type pos2 = str.find("</TABLE>", pos1);

					if (pos1 != string::npos && pos2 != string::npos)
					{
						pos1 += 13;
						string xml_str = "<?xml version=\"1.0\" encoding=\"Windows-1252\"?><data>" + str.substr(pos1, pos2 - pos1) + "</data>";
						//clean HTML string
						ReplaceString(xml_str, "\t", "");
						ReplaceString(xml_str, "\r", "");
						ReplaceString(xml_str, "\n", "");
						ReplaceString(xml_str, " nowrap", "");
						ReplaceString(xml_str, " align=center", "");
						ReplaceString(xml_str, " align=right", "");

						//load as XML
						zen::XmlDoc doc = zen::parse(xml_str);
						zen::XmlIn in(doc.root());
						zen::XmlIn test = in["TR"];

						for (zen::XmlIn itTR = in["TR"]; itTR; itTR.next())
						{
							StringVector values;
							string value;
							for (zen::XmlIn itTD = itTR["TD"]; itTD; itTD.next())
							{
								string value;
								itTD.get()->getValue(value);
								values.push_back(value);
							}

							enum TColums { C_DATE, C_TIME, C_TEMP, C_RH, C_DIR, C_WSPD, C_WIND_GUST, C_RAIN, C_RAIN24H, NB_COLUMS };
							TVarH VAR[NB_COLUMS] = { H_SKIP, H_SKIP, H_TAIR, H_RELH, H_WNDD, H_WNDS, H_SKIP, H_PRCP, H_SKIP };
							ASSERT(values.size() >= NB_COLUMS);

							if (values.size() >= NB_COLUMS)
							{
								CTRef TRef;
								TRef.FromFormatedString(values[C_DATE] + "-" + values[C_TIME].substr(0, 2));
								ASSERT(TRef.IsValid());

								if (TRef.IsValid())
								{
									if (!data.IsYearInit(TRef.GetYear()))
									{
										//try to load old data before changing it...
										string filePath = GetOutputFilePath(FIRE_PUBLIC, title, TRef.GetYear());
										data.LoadData(filePath, -999, false);//don't erase other years when multiple years
									}

									for (size_t c = 0; c < NB_COLUMS; c++)
									{
										if (VAR[c] != H_SKIP)
										{
											data.GetHour(TRef).SetStat(VAR[c], ToFloat(values[c]));
										}
									}
								}
							}
						}//for all station

						nbFiles++;
					}

				}//if msg


				if (data.HaveData())
				{

					for (auto it = data.begin(); it != data.end(); it++)
					{
						string outputPath = GetOutputFilePath(FIRE_PUBLIC, title, it->first);
						CreateMultipleDir(GetPath(outputPath));
						it->second->SaveData(outputPath);
					}
				}
				else
				{
					callback.AddMessage(title + " don't have data");
				}


				msg += callback.StepIt();
			}//for all files



			//clean connection
			pConnection->Close();
			pSession->Close();
		}//if msg



		callback.AddMessage(GetString(IDS_NB_FILES_DOWNLOADED) + ToString(nbFiles), 1);
		callback.PopTask();

		return msg;
	}

	ERMsg CUINewBrunswick::ExecuteAgricultureDaily(CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);
		msg = CreateMultipleDir(workingDir);

		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(workingDir, 1);
		callback.AddMessage(GetString(IDS_UPDATE_FROM));
		callback.AddMessage(string(SERVER_NAME[AGRI_DAILY]), 1);
		callback.AddMessage("");

		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = lastYear - firstYear + 1;
		CTRef currentTRef = CTRef::GetCurrentTRef();



		callback.PushTask("Download New-Brunswick agriculture data (" + ToString(nbYears) + " files)", nbYears);

		int nbFiles = 0;
		if (msg)
		{
			for (size_t y = 0; y < nbYears&&msg; y++)
			{
				int year = firstYear + int(y);

				string filePath = GetOutputFilePath(AGRI_DAILY, "daily_all", year);
				CreateMultipleDir(GetPath(filePath));


				msg = DownloadAgriStationDaily(filePath, year);

				//split data in separate files
				if (msg)
				{
					msg += SplitAgriStation(filePath);
					if(msg)
						nbFiles++;

					WBSF::RemoveFile(filePath);
				}

				msg += callback.StepIt();
				//}//update only this years
			}//for all years
		}//if msg



		callback.AddMessage(GetString(IDS_NB_FILES_DOWNLOADED) + ToString(nbFiles), 1);
		callback.PopTask();

		return msg;
	}

	ERMsg CUINewBrunswick::ExecuteAgricultureHourly(CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);
		msg = CreateMultipleDir(workingDir);

		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(workingDir, 1);
		callback.AddMessage(GetString(IDS_UPDATE_FROM));
		callback.AddMessage(string(SERVER_NAME[AGRI_HOURLY]), 1);
		callback.AddMessage("");

		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = lastYear - firstYear + 1;
		CTRef currentTRef = CTRef::GetCurrentTRef();


		StringVector fileList;
		msg = GetFileList(AGRI_HOURLY, fileList, callback);


		int nbFiles = 0;


		if (msg)
		{
			callback.PushTask("Download New-Brunswick agriculture hourly data (" + ToString(fileList.size()*nbYears) + " files)", fileList.size()*nbYears);
			callback.AddMessage("Download New-Brunswick agriculture hourly data (" + ToString(fileList.size()*nbYears) + " files)");

			for (size_t i = 0; i < fileList.size() && msg; i++)
			{
				for (size_t y = 0; y < nbYears&&msg; y++)
				{
					int year = firstYear + int(y);

					string filePath = GetOutputFilePath(AGRI_HOURLY, fileList[i], year);

					CreateMultipleDir(GetPath(filePath));
					if (year < currentTRef.GetYear() && !FileExists(filePath))//current hear is not in historical weather
					{
						string str;
						msg = DownloadAgriStationHourly(fileList[i], year, str);
						

						//split data in separate files
						if (msg && !str.empty())
						{
							msg += SaveAgriStationHourly(filePath, str);
							if (msg)
								nbFiles++;
							
						}
					}//update only this years

					msg += callback.StepIt();
				}//for all years
			}//for all files

		}//if msg



		callback.AddMessage(GetString(IDS_NB_FILES_DOWNLOADED) + ToString(nbFiles), 1);
		callback.PopTask();

		return msg;
	}

	ERMsg CUINewBrunswick::ExecutePrivateFire(size_t n, CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);
		msg = CreateMultipleDir(workingDir);


		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(workingDir, 1);
		callback.AddMessage(GetString(IDS_UPDATE_FROM));
		callback.AddMessage(string(SERVER_NAME[n]), 1);
		callback.AddMessage("");


		CFileInfoVector fileList;
		GetFileList(n, fileList, callback);

		//clean list

		callback.PushTask("Download New-Brunswick private fire data (" + ToString(fileList.size()) + " files)", fileList.size());
		callback.AddMessage("Download New-Brunswick private fire data (" + ToString(fileList.size()) + " files)");


		size_t nbFiles = 0;
		bool bDownloaded = false;
		int year = CTRef::GetCurrentTRef().GetYear();




		CInternetSessionPtr pSession;
		CFtpConnectionPtr pConnection;

		msg = GetFtpConnection(SERVER_NAME[n], pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, Get(USER_NAME), Get(PASSWORD), true, 5, callback);
		if (msg)
		{
			for (size_t i = 0; i < fileList.size() && msg; i++)
			{


				if (n == FIRE_HISTORICAL)
				{
					string outputFilePath = GetOutputFilePath(FIRE_HISTORICAL, fileList[i].m_filePath, -1);
					WBSF::CreateMultipleDir(GetPath(outputFilePath));
					msg = UtilWWW::CopyFile(pConnection, fileList[i].m_filePath, outputFilePath);
				}
				else if (n == FIRE_PRIVATE)
				{
					string title = GetFileTitle(fileList[i].m_filePath);
					ReplaceString(title, "Yr ", "");

					string tmpFilePath = GetOutputFilePath(FIRE_PRIVATE, title, year) + ".tmp";

					WBSF::CreateMultipleDir(GetPath(tmpFilePath));
					msg = UtilWWW::CopyFile(pConnection, fileList[i].m_filePath, tmpFilePath);
					msg = MergeData(title, tmpFilePath, callback);
					WBSF::RemoveFile(tmpFilePath);
				}



				if (msg)
				{
					nbFiles++;
					msg += callback.StepIt();
				}

			}

			//clean connection
			pConnection->Close();
			pSession->Close();
		}

		callback.AddMessage(GetString(IDS_NB_FILES_DOWNLOADED) + ToString(nbFiles), 1);


		//if (n == FIRE_HISTORICAL)
		//{
		//	CLocationVector locations;
		//	//load all station coordinates

		//	string path = GetDir(WORKING_DIR) + NETWORK_NAME[FIRE_HISTORICAL] + "\\Historical\\*.txt";
		//	CFileInfoVector filesInfo;
		//	WBSF::GetFilesInfo(path, false, filesInfo);
		//	for (size_t i = 0; i < filesInfo.size() && msg; i++)
		//	{
		//		if (filesInfo[i].m_size < 100)
		//		{
		//			ifStream file;
		//			msg += file.open(filesInfo[i].m_filePath);
		//			if (msg)
		//			{
		//				string txt1;
		//				std::getline(file, txt1);
		//				Trim(txt1);
		//				string::size_type pos = txt1.find_last_of("(");
		//				if (pos != string::npos)
		//					txt1 = txt1.substr(0, pos - 1);

		//				string ID = TrimConst(txt1.substr(txt1.length() - 5));
		//				string name = TrimConst(txt1.substr(0, txt1.length() - 5));
		//				name = PurgeFileName(name);

		//				string txt2;
		//				std::getline(file, txt2);
		//				StringVector info2(txt2, ",; m");

		//				if (!ID.empty() && !name.empty() && (info2.size() == 8 || info2.size() == 6))
		//				{
		//					if (info2.size() == 8)
		//					{
		//						double lat = ToDouble(info2[0]) + ToDouble(info2[1]) / 60.0 + ToDouble(info2[2]) / 3600.0;
		//						double lon = -(-ToDouble(info2[3]) + ToDouble(info2[4]) / 60.0 + ToDouble(info2[5]) / 3600.0);
		//						double alt = ToDouble(info2[6]);

		//						CLocation loc(name, ID, lat, lon, alt);
		//						loc.SetSSI("Network", NETWORK_NAME[FIRE_HISTORICAL]);
		//						locations.push_back(loc);
		//					}
		//					else if (info2.size() == 6)
		//					{
		//						double lat = ToDouble(info2[0]) + ToDouble(info2[1]) / 60.0;
		//						double lon = -(-ToDouble(info2[2]) + ToDouble(info2[3]) / 60.0);
		//						double alt = ToDouble(info2[4]);

		//						CLocation loc(name, ID, lat, lon, alt);
		//						loc.SetSSI("Network", NETWORK_NAME[FIRE_HISTORICAL]);
		//						locations.push_back(loc);
		//					}

		//				}
		//				else
		//				{
		//					callback.AddMessage("WARNING : invalid station info : " + txt1 + txt2);
		//				}
		//			}//if msg
		//		}//if it's a header file
		//	}//for all file

		//	string filePaht = GetStationListFilePath(FIRE_HISTORICAL);
		//	msg += locations.Save(filePaht);
		//}




		callback.PopTask();

		return msg;
	}


	string CUINewBrunswick::GetOutputFilePath(size_t n, const string& name, int year)const
	{
		string m_output_path;
		if (n == FIRE_HISTORICAL)
		{
			string dir = GetLastDirName(GetPath(name));
			string ID = dir.substr(0, 5);
			string title = TrimConst(GetFileTitle(name));
			ReplaceString(title, " to", "-");

			int firstYear = -999;
			int lastYear = -999;
			string stationName = title;


			if (title.length() > 5)
			{
				StringVector period(title.substr(title.length() - 5), "-");
				if (period.size() == 2)
				{
					firstYear = ToInt(period[0]);
					lastYear = ToInt(period[1]);
					firstYear += (firstYear > 50) ? 1900 : 2000;
					lastYear += (lastYear > 50) ? 1900 : 2000;

					ASSERT(firstYear > 1950 && firstYear <= 2050);
					ASSERT(lastYear > 1950 && lastYear <= 2050);

					stationName = TrimConst(title.substr(0, title.length() - 5));
				}
				else
				{
					string test = title.substr(title.length() - 4);

					if (isdigit(test[0]) && isdigit(test[1]) && isdigit(test[2]) && isdigit(test[3]))
					{
						int year = ToInt(test);

						firstYear = year;
						lastYear = year;
						stationName = TrimConst(title.substr(0, title.length() - 4));
					}
					else
					{
						string test = title.substr(title.length() - 2);

						if (isdigit(test[0]) && isdigit(test[1]))
						{
							int year = ToInt(test);
							year += (year > 50) ? 1900 : 2000;

							firstYear = year;
							lastYear = year;
							stationName = TrimConst(title.substr(0, title.length() - 2));
						}
					}
				}

			}

			bool bData = firstYear != -999 && lastYear != -999;


			stationName = PurgeFileName(stationName);
			string p = bData ? " " + ToString(firstYear) + "-" + ToString(lastYear) : "";

			m_output_path = GetDir(WORKING_DIR) + "Fire\\Historical\\" + ID + " " + stationName + p + ".txt";
		}
		else if (n == FIRE_PRIVATE)
		{
			m_output_path = GetDir(WORKING_DIR) + "Fire\\Private\\" + ToString(year) + "\\" + name + ".csv";
		}
		else if (n == FIRE_PUBLIC)
		{
			m_output_path = GetDir(WORKING_DIR) + "Fire\\" + ToString(year) + "\\" + name + ".csv";
		}
		else if (n == AGRI_DAILY)
		{
			m_output_path = GetDir(WORKING_DIR) + "Agriculture\\Daily\\" + ToString(year) + "\\" + name + ".csv";
		}
		else if (n == AGRI_HOURLY)
		{
			m_output_path = GetDir(WORKING_DIR) + "Agriculture\\Hourly\\" + ToString(year) + "\\" + name + ".csv";
		}

		return m_output_path;
	}

	std::string CUINewBrunswick::GetStationListFilePath()const
	{
		//static const char* FILE_NAME[NETWORK] = { "HistoricalStations.csv",  "NBFireStations.csv", "NBFireStations.csv", "NBAgStations.csv" };
		//if (n == FIRE_HISTORICAL)
			//return GetDir(WORKING_DIR) + NETWORK_NAME[FIRE_HISTORICAL] + "\\" + FILE_NAME[n];

		return WBSF::GetApplicationPath() + "Layers\\NBStations.csv";
	}

	ERMsg CUINewBrunswick::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;

		if (!m_stations.empty())
			m_stations.clear();


		std::bitset<NB_NETWORKS> network = GetNetWork();

		msg = m_stations.Load(GetStationListFilePath());

		if (msg)
			msg += m_stations.IsValid();


		if (msg)
		{
			for (size_t i = 0; i < m_stations.size(); i++)
			{
				size_t n = GetNetworkFromName(m_stations[i].GetSSI("Network"));
				ASSERT(n < network.size());

				if (network[n])
				{
					//if (stationList.Find(m_stations[i].m_ID) == -1)
					stationList.push_back(to_string(n) + "/" + m_stations[i].m_ID);
				}
			}
		}

		return msg;
	}

	ERMsg CUINewBrunswick::GetWeatherStation(const std::string& IDin, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;

		//Get station information
		StringVector tmp(IDin, "/");
		ASSERT(tmp.size() == 2);
		size_t n = ToSizeT(tmp[0]);
		string ID = tmp[1];
		size_t it = m_stations.FindByID(ID);
		if (it == NOT_INIT)
		{
			msg.ajoute(FormatMsg(IDS_NO_STATION_INFORMATION, ID));
			return msg;
		}
		
		if (n == AGRI_DAILY && TM.Type() == CTM::HOURLY)
			return msg;


		((CLocation&)station) = m_stations[it];
		station.SetHourly(TM.Type() == CTM::HOURLY);

		//size_t n = GetNetworkFromName(station.GetSSI("Network"));
		//station.m_name = TraitFileName(station.m_name);

		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = lastYear - firstYear + 1;
		station.CreateYears(firstYear, nbYears);
		//station.m_name = PurgeFileName(station.m_name);


		if (n == FIRE_HISTORICAL)
		{
			string path = GetDir(WORKING_DIR) + "Fire\\Historical\\" + ID + "*.txt";
			CFileInfoVector filesInfo;
			WBSF::GetFilesInfo(path, false, filesInfo);
			for (size_t i = 0; i < filesInfo.size() && msg; i++)
			{
				if (filesInfo[i].m_size > 500)
				{
					string fileTitle = GetFileTitle(filesInfo[i].m_filePath);
					string pStr = fileTitle.substr(fileTitle.length() - 9);
					StringVector period(pStr, "-");

					if (period.size() == 2)
					{
						int p1 = ToInt(period[0]);
						int p2 = ToInt(period[1]);

						if (firstYear <= p2 && lastYear >= p1)
							msg = ReadFireHistorical(filesInfo[i].m_filePath, TM, station, callback);
					}
					else
					{
						pStr = fileTitle.substr(fileTitle.length() - 4);
						int p1 = ToInt(period[0]);
						if (p1 != 0)
						{
							if (firstYear <= p1 && lastYear >= p1)
								msg = ReadFireHistorical(filesInfo[i].m_filePath, TM, station, callback);
						}

					}
				}


				msg += callback.StepIt(0);
			}
		}
		else
		{
			//now extract data 
			for (size_t y = 0; y < nbYears&&msg; y++)
			{
				int year = firstYear + int(y);
				string name = (n == AGRI_HOURLY || n == AGRI_DAILY) ? station.m_ID : station.m_name;
				string filePath = GetOutputFilePath(n, name, year);

				if (FileExists(filePath))
				{
					msg = station.LoadData(filePath, -999, false);
				}

				msg += callback.StepIt(0);
			}
		}

		//verify station is valid
		if (msg && station.HaveData())
		{
			//verify station is valid
			msg = station.IsValid();
		}

		return msg;
	}

	static size_t GetHour(const string& time)
	{
		size_t h = 0;
		StringVector v(time, ":");
		ASSERT(v.size() == 2);

		return ToSizeT(v[0]);
	}


	ERMsg CUINewBrunswick::MergeData(const string& ID, const string& filePath, CCallback& callback)const
	{
		ERMsg msg;



		//now extract data 
		ifStream file;
		msg = file.open(filePath);

		if (msg)
		{
			CWeatherAccumulator accumulator(CTM::HOURLY);
			vector<size_t> variables;
			bool bHourly = true;
			CWeatherYears data(bHourly);


			for (CSVIterator loop(file); loop != CSVIterator() && msg; ++loop)
			{
				if (variables.empty())
				{
					StringVector head = loop.Header();
					vector<size_t> columns = GetNBColumns(head);

					variables = GetNBVariables(bHourly, columns);
				}

				ASSERT(loop->size() <= variables.size());
				if (loop->size() > C_TIME)
				{
					int year = ToInt((*loop)[C_YEAR]);
					size_t month = ToInt((*loop)[C_MONTH]) - 1;
					size_t day = ToInt((*loop)[C_DAY]) - 1;
					size_t hour = GetHour((*loop)[C_TIME]);

					ASSERT(month < 12);
					ASSERT(day < GetNbDayPerMonth(year, month));
					ASSERT(hour < 24);

					CTRef TRef = bHourly ? CTRef(year, month, day, hour) : CTRef(year, month, day);

					if (accumulator.TRefIsChanging(TRef))
					{
						if (!data.IsYearInit(TRef.GetYear()))
						{
							//try to load old data before changing it...
							string filePath = GetOutputFilePath(FIRE_PRIVATE, ID, TRef.GetYear());
							data.LoadData(filePath, -999, false);//don't erase other years when multiple years
						}

						data[accumulator.GetTRef()].SetData(accumulator);
					}

					for (size_t c = 0; c < loop->size(); c++)
					{

						if (variables[c] != NOT_INIT)
						{
							string str = (*loop)[c];
							if (!str.empty())
							{
								double value = ToDouble(str);
								if (value > -999 && value < 999)
								{
									if (variables[c] == H_RELH && value > 100)
										value = 100;

									accumulator.Add(TRef, variables[c], value);
								}



								if (variables[c] == H_RELH)
								{
									vector<size_t>::const_iterator it = find(variables.begin(), variables.end(), H_TAIR);
									if (it != variables.end())
									{
										string TairStr = (*loop)[std::distance(variables.cbegin(), it)];

										if (!TairStr.empty())
										{
											double Tair = ToDouble(TairStr);
											if (Tair > -999 && Tair < 999)
											{
												double Tdew = Hr2Td(Tair, value);
												accumulator.Add(TRef, H_TDEW, Tdew);
											}
										}
									}
								}
							}
						}
					}

					msg += callback.StepIt(0);
				}
			}//for all line


			if (accumulator.GetTRef().IsInit())
				data[accumulator.GetTRef()].SetData(accumulator);

			for (auto it = data.begin(); it != data.end(); it++)
			{
				CWeatherYear station;
				string outputPath = GetOutputFilePath(FIRE_PRIVATE, ID, it->first);
				//load old data
				CreateMultipleDir(GetPath(outputPath));
				it->second->SaveData(outputPath);
			}
		}//if load 





		return msg;
	}

	enum TFireHistorical { CH_NAME, CH_FILENAME, CH_DATE, CH_JULIAN, CH_TIME, CH_TEMP, CH_RH, CH_DIR, CH_WSPD, CH_MX_SPD, CH_RN_1, CH_RN24, CH_FFMC, CH_DMC, CH_DC, CH_ISI, CH_BUI, CH_FWI, CH_DSR, CH_NB_COLUMNS };
	static const char* COLUMN_NAME_FH[CH_NB_COLUMNS] = { "Name", "Filename", "Date", "Julian", "Time", "Temp", "RH", "Dir", "Wspd", "Mx_Spd", "Rn_1", "Rn24", "FFMC", "DMC", "DC", "ISI", "BUI", "FWI", "DSR" };
	static const TVarH VARS[CH_NB_COLUMNS] = { H_SKIP, H_SKIP, H_SKIP, H_SKIP, H_SKIP, H_TAIR, H_RELH, H_WNDD, H_WND2, H_SKIP, H_PRCP, H_SKIP, H_SKIP, H_SKIP, H_SKIP, H_SKIP, H_SKIP, H_SKIP, H_SKIP };

	static size_t GetColumnFH(const string& header)
	{
		size_t c = NOT_INIT;
		for (size_t i = 0; i < CH_NB_COLUMNS&&c == NOT_INIT; i++)
		{
			if (IsEqual(header, COLUMN_NAME_FH[i]))
				c = i;
		}

		return c;
	}

	static vector<size_t> GetColumnsFH(const StringVector& header)
	{
		vector<size_t> columns(header.size());

		for (size_t c = 0; c < header.size(); c++)
			columns[c] = GetColumnFH(header[c]);

		return columns;
	}

	static vector<size_t>  GetVariablesFH(const vector<size_t>& columns)
	{
		vector<size_t> vars(columns.size());

		for (size_t c = 0; c < columns.size(); c++)
			vars[c] = columns[c] < CH_NB_COLUMNS ? VARS[columns[c]] : H_SKIP;

		return vars;
	}


	ERMsg CUINewBrunswick::ReadFireHistorical(const string& filePath, CTM TM, CWeatherYears& data, CCallback& callback)const
	{
		ERMsg msg;



		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		//size_t nbYears = lastYear - firstYear + 1;

		//now extract data 
		ifStream file;
		msg = file.open(filePath);

		if (msg)
		{
			CWeatherAccumulator accumulator(TM);
			vector<size_t> columns;
			vector<size_t> variables;

			bool bTower = filePath.find("Tower") != string::npos;

			for (CSVIterator loop(file, ",\t", true); loop != CSVIterator() && msg; ++loop)
			{
				if (variables.empty())
				{
					StringVector head = loop.Header();
					columns = GetColumnsFH(head);
					variables = GetVariablesFH(columns);
					//skip units
					++loop;
				}

				vector<size_t>::const_iterator itDate = find(columns.begin(), columns.end(), CH_DATE); ASSERT(itDate != columns.end());
				vector<size_t>::const_iterator itTime = find(columns.begin(), columns.end(), CH_TIME); ASSERT(itTime != columns.end());
				string dateStr = (*loop)[std::distance(columns.cbegin(), itDate)];
				string timeStr = (*loop)[std::distance(columns.cbegin(), itTime)];

				StringVector date(dateStr, "/-");
				ASSERT(date.size() == 3);
				StringVector time(timeStr, " ");
				ASSERT(time.size() == 2);
				size_t yi = date[0].length() == 4 ? 0 : 2;
				size_t di = yi == 0 ? 2 : 0;

				int year = ToInt(date[yi]);
				if (year >= firstYear && year <= lastYear)
				{
					size_t month = ToInt(date[1]) - 1;
					size_t day = ToInt(date[di]) - 1;
					size_t hour = ToInt(time[0]);



					ASSERT(month < 12);
					ASSERT(day < GetNbDayPerMonth(year, month));
					ASSERT(hour <= 24);

					CTRef TRef = CTRef(year, month, day, hour);


					if (accumulator.TRefIsChanging(TRef))
					{
						data[accumulator.GetTRef()].SetData(accumulator);
					}

					ASSERT(loop->size() == variables.size());
					for (size_t c = 0; c < loop->size(); c++)
					{
						if (variables[c] != H_SKIP)
						{
							string str = (*loop)[c];
							if (!str.empty() && str != "-")
							{
								double value = ToDouble(str);
								accumulator.Add(TRef, variables[c], value);

								if (variables[c] == H_RELH)
								{
									vector<size_t>::const_iterator it = find(variables.begin(), variables.end(), H_TAIR);
									if (it != variables.end())
									{
										string TairStr = (*loop)[std::distance(variables.cbegin(), it)];
										if (!TairStr.empty() && TairStr != "-")
										{
											double Tair = ToDouble(TairStr);
											double Tdew = Hr2Td(Tair, value);
											accumulator.Add(TRef, H_TDEW, Tdew);
										}
									}
								}
							}
						}
					}
				}

				msg += callback.StepIt(0);

			}//for all line


			if (accumulator.GetTRef().IsInit())
				data[accumulator.GetTRef()].SetData(accumulator);

		}//if load 

		return msg;
	}
}

