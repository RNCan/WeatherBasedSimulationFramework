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

namespace WBSF
{

	const char* CUINewBrunswick::SERVER_NAME[NB_NETWORKS] = { "ftp.gnb.ca", "ftp.gnb.ca", "www1.gnb.ca", "daafmaapextweb.gnb.ca" };


	//*********************************************************************
	const char* CUINewBrunswick::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "UsderName", "Password", "WorkingDir", "FirstYear", "LastYear", "Network" };
	const size_t CUINewBrunswick::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_STRING, T_PASSWORD, T_PATH, T_STRING, T_STRING, T_STRING_SELECT };
	const UINT CUINewBrunswick::ATTRIBUTE_TITLE_ID = IDS_UPDATER_NEWBRUNSWICK_P;
	const UINT CUINewBrunswick::DESCRIPTION_TITLE_ID = ID_TASK_NEWBRUNSWICK;

	const char* CUINewBrunswick::CLASS_NAME() { static const char* THE_CLASS_NAME = "NewBrunswick";  return THE_CLASS_NAME; }
	CTaskBase::TType CUINewBrunswick::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUINewBrunswick::CLASS_NAME(), (createF)CUINewBrunswick::create);

	const char* CUINewBrunswick::NETWORK_NAME[NB_NETWORKS]{ "FireHistorical", "PrivateFire", "PublicFire", "Agriculture" };

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
		case NETWORK:	str = "FireHistorical=Fire (historical)|PrivateFire=Private fire (current)|PublicFire=Public fire (current)|Agriculture=Agriculture"; break;
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


		if (n == FIRE_HISTORICAL || n == PRIVATE_FIRE)
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
				else if (n == PRIVATE_FIRE)
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
		else if (n == PUBLIC_FIRE)
		{
			ASSERT(false);
		}
		else if (n == AGRI)
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

			if (n == FIRE_HISTORICAL || n == PRIVATE_FIRE)
			{
				ASSERT(false);
			}
			else if (n == PUBLIC_FIRE)
			{
				//https://www1.gnb.ca/0079/FireWeather/FireWeatherHourly-e.asp?Stn=all
				CInternetSessionPtr pSession;
				CHttpConnectionPtr pConnection;

				msg = GetHttpConnection(SERVER_NAME[PUBLIC_FIRE], pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", false, 5, callback);
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
			else if (n == AGRI)
			{
				CInternetSessionPtr pSession;
				CHttpConnectionPtr pConnection;

				msg = GetHttpConnection(SERVER_NAME[AGRI], pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", false, 5, callback);
				if (msg)
				{
					string str;
					msg = UtilWWW::GetPageText(pConnection, "010-001/archive.aspx", str);
					if (msg)
					{
						string::size_type pos1 = str.find("<select");
						string::size_type pos2 = str.find("</select>");

						if (pos1 != string::npos && pos2 != string::npos)
						{
							string xml_str = "<?xml version=\"1.0\" encoding=\"Windows-1252\"?>\r\n" + str.substr(pos1, pos2 - pos1 + 9);
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
					}

					//clean connection
					pConnection->Close();
					pSession->Close();
				}
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

	ERMsg CUINewBrunswick::SaveStation(const std::string& filePath, std::string str)
	{
		ERMsg msg;

		CWeatherYears data(true);


		enum THourlyColumns { C_DATE_TIME, C_TOUTSIDE, C_TMAX, C_TMIN, C_HUMIDITY, C_TDEW, C_WSPD, C_DIR, C_RAIN, C_TSOIL, NB_COLUMNS };
		static const TVarH COL_POS[NB_COLUMNS] = { H_SKIP, H_TAIR, H_TMAX, H_TMIN, H_RELH, H_TDEW, H_WNDS, H_WNDD, H_PRCP, H_SKIP };

		try
		{
			int ID = ToInt(GetFileTitle(filePath));
			WBSF::ReplaceString(str, "\t", "");

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

					int year = ToInt(date[2]);
					size_t month = ToInt(date[1]) - 1;
					size_t day = ToInt(date[0]) - 1;
					size_t hour = ToInt(date[3]);
					size_t minute = ToInt(date[4]);


					if (minute == 0)
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

						for (size_t i = 0; i < NB_COLUMNS; i++)
						{
							if (COL_POS[i] != H_SKIP)
							{
								if (COL_POS[i] == H_WNDD)
									tmp[i] = ToString(CUIManitoba::GetWindDir(tmp[i]));

								double value = ToDouble(tmp[i]);
								if (value > -99)
								{
									value = Convert(ID, COL_POS[i], value);
									data.GetHour(TRef).SetStat(COL_POS[i], value);
								}

							}
						}
					}
				}
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

	ERMsg CUINewBrunswick::Execute(CCallback& callback)
	{
		ERMsg msg;


		std::bitset<NB_NETWORKS> network = GetNetWork();

		for (size_t n = 0; n < network.size() && msg; n++)
		{
			if (network[n])
			{
				switch (n)
				{
				case FIRE_HISTORICAL: msg += ExecutePrivateFire(n, callback); break;
				case PRIVATE_FIRE: 
					if (m_stations.empty())
						msg = m_stations.Load(GetStationListFilePath());
					msg += ExecutePrivateFire(n, callback); break;
				case PUBLIC_FIRE: msg += ExecutePublicFire(callback); break;
				case AGRI: msg += ExecuteAgriculture(callback); break;
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
	ERMsg CUINewBrunswick::DownloadAgriStation(CHttpConnectionPtr& pConnection, const std::string& ID, int year, std::string& text)
	{

		ERMsg msg;

		string VIEWSTATE;
		string VIEWSTATEGENERATOR;
		string EVENTVALIDATION;


		string str;
		msg = UtilWWW::GetPageText(pConnection, "010-001/archive.aspx", str);
		if (msg)
		{

			VIEWSTATE = url_encode(WBSF::FindString(str, "id=\"__VIEWSTATE\" value=\"", "\""));
			VIEWSTATEGENERATOR = url_encode(WBSF::FindString(str, "id=\"__VIEWSTATEGENERATOR\" value=\"", "\""));
			EVENTVALIDATION = url_encode(WBSF::FindString(str, "id=\"__EVENTVALIDATION\" value=\"", "\""));
		}

		//
		//
		CString URL = _T("010-001/archive.aspx");
		CString strHeaders = _T("Content-Type: application/x-www-form-urlencoded\r\n");
		CStringA strParam = "__EVENTTARGET=&__EVENTARGUMENT=&";
		strParam += "__VIEWSTATE=" + CStringA(VIEWSTATE.c_str()) + "&";
		strParam += "__VIEWSTATEGENERATOR=" + CStringA(VIEWSTATEGENERATOR.c_str()) + "&";
		strParam += "__EVENTVALIDATION=" + CStringA(EVENTVALIDATION.c_str()) + "&";
		strParam += "ctl00%hfLang=fr-CA&";
		strParam += ("ctl00%Content1%ddlWS=" + ID + "&").c_str();
		strParam += "ctl00%Content1%ddlFromDay=1&";
		strParam += "ctl00%Content1%ddlFromMonth=1&";
		strParam += ("ctl00%Content1%ddlFromYear=" + ToString(year) + "&").c_str();
		strParam += "ctl00%Content1%hdnFromDate=&";
		strParam += "ctl00%Content1%ddlToDay=31&";
		strParam += "ctl00%Content1%ddlToMonth=12&";
		strParam += ("ctl00%Content1%ddlToYear=" + ToString(year) + "&").c_str();
		strParam += "ctl00%Content1%hdnToDate=&";
		strParam += "ctl00%Content1%btnGetData=Submit";


		DWORD HttpRequestFlags = INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE;
		CHttpFile* pURLFile = pConnection->OpenRequest(CHttpConnection::HTTP_VERB_POST, URL, NULL, 1, NULL, NULL, HttpRequestFlags);


		bool bRep = false;

		if (pURLFile != NULL)
		{
			int nbTry = 0;
			while (!bRep && msg)
			{
				TRY
				{
					nbTry++;
					pURLFile->AddRequestHeaders(strHeaders);

					CString strContentL;
					strContentL.Format(_T("Content-Length: %d\r\n"), strParam.GetLength());
					pURLFile->AddRequestHeaders(strContentL);

					// send request
					bRep = pURLFile->SendRequest(0, 0, (void*)(const char*)strParam, strParam.GetLength()) != 0;
				}
					CATCH_ALL(e)
				{
					DWORD errnum = GetLastError();
					if (errnum == 12002 || errnum == 12029)
					{
						if (nbTry >= 10)
						{
							msg = UtilWin::SYGetMessage(*e);
						}
						//try again
					}
					else if (errnum == 12031 || errnum == 12111)
					{
						//throw a exception: server reset
						THROW(new CInternetException(errnum));
					}
					else if (errnum == 12003)
					{
						msg = UtilWin::SYGetMessage(*e);

						DWORD size = 255;
						TCHAR cause[256] = { 0 };
						InternetGetLastResponseInfo(&errnum, cause, &size);
						if (_tcslen(cause) > 0)
							msg.ajoute(UtilWin::ToUTF8(cause));
					}
					else
					{
						CInternetException e(errnum);
						msg += UtilWin::SYGetMessage(e);
					}
				}
				END_CATCH_ALL
			}
		}


		if (bRep)
		{
			const short MAX_READ_SIZE = 4096;
			pURLFile->SetReadBufferSize(MAX_READ_SIZE);

			std::string tmp;
			tmp.resize(MAX_READ_SIZE);
			UINT charRead = 0;
			while ((charRead = pURLFile->Read(&(tmp[0]), MAX_READ_SIZE)) > 0)
				text.append(tmp.c_str(), charRead);

			pURLFile->Close();
		}
		else
		{
			CString tmp;
			tmp.FormatMessage(IDS_CMN_UNABLE_LOAD_PAGE, URL);
			msg.ajoute(UtilWin::ToUTF8(tmp));
		}

		delete pURLFile;
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
		callback.AddMessage(string(SERVER_NAME[PUBLIC_FIRE]), 1);
		callback.AddMessage("");

		//		int firstYear = as<int>(FIRST_YEAR);
			//	int lastYear = as<int>(LAST_YEAR);
				//size_t nbYears = lastYear - firstYear + 1;
				//CTRef currentTRef = CTRef::GetCurrentTRef();


		StringVector fileList;
		GetFileList(PUBLIC_FIRE, fileList, callback);

		callback.PushTask("Download New-Brunswick public fire data (" + ToString(fileList.size()) + " stations)", fileList.size());

		size_t nbFiles = 0;
		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;

		msg = GetHttpConnection(SERVER_NAME[PUBLIC_FIRE], pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", false, 5, callback);
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
							ASSERT(values.size() == NB_COLUMS);


							if (values.size() == NB_COLUMS)
							{
								CTRef TRef;
								TRef.FromFormatedString(values[C_DATE] + "-" + values[C_TIME].substr(0, 2));
								ASSERT(TRef.IsValid());

								if (TRef.IsValid())
								{
									if (!data.IsYearInit(TRef.GetYear()))
									{
										//try to load old data before changing it...
										string filePath = GetOutputFilePath(PUBLIC_FIRE, title, TRef.GetYear());
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
						string outputPath = GetOutputFilePath(PUBLIC_FIRE, title, it->first);
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

	ERMsg CUINewBrunswick::ExecuteAgriculture(CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);
		msg = CreateMultipleDir(workingDir);

		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(workingDir, 1);
		callback.AddMessage(GetString(IDS_UPDATE_FROM));
		callback.AddMessage(string(SERVER_NAME[AGRI]), 1);
		callback.AddMessage("");

		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = lastYear - firstYear + 1;
		CTRef currentTRef = CTRef::GetCurrentTRef();


		StringVector fileList;
		GetFileList(AGRI, fileList, callback);

		callback.PushTask("Download New-Brunswick agriculture data (" + ToString(fileList.size()*nbYears) + " files)", fileList.size()*nbYears);

		int nbFiles = 0;
		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;

		msg = GetHttpConnection(SERVER_NAME[AGRI], pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", false, 5, callback);
		if (msg)
		{
			for (size_t i = 0; i < fileList.size() && msg; i++)
			{
				for (size_t y = 0; y < nbYears&&msg; y++)
				{
					int year = firstYear + int(y);

					string filePath = GetOutputFilePath(AGRI, fileList[i], year);
					CreateMultipleDir(GetPath(filePath));
					if (year < currentTRef.GetYear() && !FileExists(filePath))//current hear is not in historical weather
					{
						string str;
						msg = DownloadAgriStation(pConnection, fileList[i], year, str);

						//split data in separate files
						if (msg)
						{
							string::size_type pos1 = str.find("<table class=\"gridviewBorder\"");
							string::size_type pos2 = str.find("</table>", pos1);

							if (pos1 != string::npos && pos2 != string::npos)
							{
								string tmp = "<?xml version=\"1.0\" encoding=\"Windows-1252\"?>\r\n" + str.substr(pos1, pos2 - pos1 + 9);
								msg += SaveStation(filePath, tmp);
								nbFiles++;
							}
						}

						msg += callback.StepIt();
					}//update only this years
				}//for all years
			}//for all files

			//clean connection
			pConnection->Close();
			pSession->Close();
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
				else if (n == PRIVATE_FIRE)
				{
					string title = GetFileTitle(fileList[i].m_filePath);
					ReplaceString(title, "Yr ", "");
					
					string tmpFilePath = GetOutputFilePath(PRIVATE_FIRE, title, year) + ".tmp";

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
		else if (n == PRIVATE_FIRE)
		{
			m_output_path = GetDir(WORKING_DIR) + "Fire\\Private\\" + ToString(year) + "\\" + name + ".csv";
		}
		else if (n == PUBLIC_FIRE)
		{
			m_output_path = GetDir(WORKING_DIR) + "Fire\\" + ToString(year) + "\\" + name + ".csv";
		}
		else if (n == AGRI)
		{
			m_output_path = GetDir(WORKING_DIR) + NETWORK_NAME[n] + "\\" + ToString(year) + "\\" + name + ".csv";
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
							msg = ReadDataHistorical(filesInfo[i].m_filePath, TM, station, callback);
					}
					else
					{
						pStr = fileTitle.substr(fileTitle.length() - 4);
						int p1 = ToInt(period[0]);
						if (p1 != 0)
						{
							if (firstYear <= p1 && lastYear >= p1)
								msg = ReadDataHistorical(filesInfo[i].m_filePath, TM, station, callback);
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

				string filePath = GetOutputFilePath(n, station.m_name, year);
				
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
							string filePath = GetOutputFilePath(PRIVATE_FIRE, ID, TRef.GetYear());
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
				string outputPath = GetOutputFilePath(PRIVATE_FIRE, ID, it->first);
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


	ERMsg CUINewBrunswick::ReadDataHistorical(const string& filePath, CTM TM, CWeatherYears& data, CCallback& callback)const
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

