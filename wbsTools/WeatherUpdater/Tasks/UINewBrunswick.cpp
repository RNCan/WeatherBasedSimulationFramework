#include "StdAfx.h"
#include "UINewBrunswick.h"
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



namespace WBSF
{

	const char* CUINewBrunswick::SERVER_NAME[NB_NETWORKS] = { "ftp.gnb.ca", "daafmaapextweb.gnb.ca"};


	//*********************************************************************
	const char* CUINewBrunswick::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "UsderName", "Password", "WorkingDir", "FirstYear", "LastYear", "Network", "DataType"};
	const size_t CUINewBrunswick::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_STRING, T_PASSWORD, T_PATH, T_STRING, T_STRING, T_STRING_SELECT, T_COMBO_INDEX };
	const UINT CUINewBrunswick::ATTRIBUTE_TITLE_ID = IDS_UPDATER_NEWBRUNSWICK_P;
	const UINT CUINewBrunswick::DESCRIPTION_TITLE_ID = ID_TASK_NEWBRUNSWICK;

	const char* CUINewBrunswick::CLASS_NAME(){ static const char* THE_CLASS_NAME = "NewBrunswick";  return THE_CLASS_NAME; }
	CTaskBase::TType CUINewBrunswick::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUINewBrunswick::CLASS_NAME(), (createF)CUINewBrunswick::create);


	enum{ C_STATION_NAME, C_YEAR, C_MONTH, C_DAY, C_DATE, C_TIME, C_STATION_ID, C_TEMP, C_RH, C_DIR, C_WSPD, C_MX_SPD, C_RN_1, C_RN_24, C_PG_1HR, C_PG_24, C_HFFMC, C_HISI, C_HFWI, C_RN24, C_PG_24HR, C_FFMC, C_DMC, C_DC, C_ISI, C_BUI, C_FWI, C_DSR, C_TMAX, C_TMAX24, C_TMIN, C_TMIN24, NB_COLUMNS };
	static const char* COLUMN_NAME[NB_COLUMNS] = { "StationName", "Year", "Month", "Day", "Date", "Time", "StationID", "Temp", "Rh", "Dir", "Wspd", "Mx_Spd", "Rn_1", "rn_24", "PG_1hr", "pg_24", "hFFMC", "hISI", "hFWI", "Rn24", "PG_24hr", "FFMC", "DMC", "DC", "ISI", "BUI", "FWI", "DSR", "TMax", "TMax24", "TMin", "TMin24" };
	
	size_t GetColumn(const string& header)
	{
		size_t c = NOT_INIT;
		for (size_t i = 0; i < NB_COLUMNS&&c == NOT_INIT; i++)
		{
			if (IsEqual(header, COLUMN_NAME[i]))
				c = i;
		}

		return c;
	}

	vector<size_t> GetColumns(const StringVector& header)
	{
		vector<size_t> columns(header.size());
		

		for (size_t c = 0; c < header.size(); c++)
			columns[c] = GetColumn(header[c]);
		
		return columns;
	}


	static size_t GetVariable(bool bHourly, size_t type)
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
			else if (type == C_TMAX || type == C_TMIN)//type == C_TEMP ||
				v = H_TAIR;
		}
		else
		{
			if (type == C_RN24)
				v = H_PRCP;
			else if (type == C_TMAX24 || type == C_TMIN24)
				v = H_TAIR;
		}

		return v;
	}

	static vector<size_t>  GetVariables(bool bHourly, vector<size_t> coluns)
	{
		vector<size_t> vars(coluns.size());


		for (size_t c = 0; c < coluns.size(); c++)
			vars[c] = GetVariable(bHourly, coluns[c]);

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
		case NETWORK:	str = "0=Fire|1=Agriculture"; break;//|1=Agriculture
		case DATA_TYPE: str = GetString(IDS_STR_DATA_TYPE); break;
		};
		return str;
	}

	std::string CUINewBrunswick::Default(size_t i)const
	{
		string str;

		switch (i)
		{
		case WORKING_DIR: str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()) + "NB\\"; break;
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

		if (msg)
		{

			if (n == FIRE)
			{
				//open a connection on the server
				string str;
				CInternetSessionPtr pSession;
				CFtpConnectionPtr pConnection;
				msg = GetFtpConnection(SERVER_NAME[n], pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, Get(USER_NAME), Get(PASSWORD));
				if (msg)
				{
					msg = UtilWWW::FindFiles(pConnection, "yr*.csv", fileList, callback);
					pConnection->Close();
					pSession->Close();
				}
			}
			else if (n == AGRI)
			{
				ASSERT(false);
			}
		}

		return msg;
	}

	ERMsg CUINewBrunswick::GetFileList(size_t n, StringVector& fileList, CCallback& callback)const
	{
		ERMsg msg;

		fileList.clear();

		if (msg)
		{

			if (n == FIRE)
			{
				ASSERT(false);
			}
			else if (n == AGRI)
			{
				CInternetSessionPtr pSession;
				CHttpConnectionPtr pConnection;

				msg = GetHttpConnection(SERVER_NAME[AGRI], pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS);
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
						
						//static const char* STATIONS[19] = { "47", "49", "62", "42", "45", "51", "55", "36", "37", "59", "41", "67", "68", "70", "66", "65", "53", "73", "69" };
						//for (int i = 0; i < 19; i++)
						//fileList.push_back(STATIONS[i]);
					}

					//clean connection
					pConnection->Close();
					pSession->Close();
				}
			}
		}

		return msg;
	}
	
	double GetWindDir(string compass)
	{
		static const char* COMPASS[16] = { "N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE", "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"};

		double wd = 0;
		for (size_t i = 0; i < 16; i++)
		{
			if (compass == COMPASS[i])
			{
				wd = i*22.5;
				break;
			}
		}

		return wd;
	}

	ERMsg CUINewBrunswick::SaveStation(const std::string& filePath, std::string str)
	{
		ERMsg msg;

		CWeatherYears data(true);


		enum THourlyColumns{ C_DATE_TIME, C_TOUTSIDE, C_TMAX, C_TMIN, C_HUMIDITY, C_TDEW, C_WSPD, C_DIR, C_RAIN, C_TSOIL, NB_COLUMNS };
		static const TVarH COL_POS[NB_COLUMNS] = { H_SKIP, H_TAIR, H_SKIP, H_SKIP, H_RELH, H_TDEW, H_WNDS, H_WNDD, H_PRCP, H_SKIP };

		try
		{
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
						else if (date[6] == "PM")
							hour += 12;

						CTRef TRef = CTRef(year, month, day, hour);

						for (size_t i = 0; i < NB_COLUMNS; i++)
						{
							if (COL_POS[i] != H_SKIP)
							{
								if (COL_POS[i] == H_WNDD)
									tmp[i] = ToString(GetWindDir(tmp[i]));

								double value = ToDouble(tmp[i]);
								if (value > -99)
									data.GetHour(TRef).SetStat(COL_POS[i], value);
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


	ERMsg CUINewBrunswick::Execute(CCallback& callback)
	{
		ERMsg msg;

		std::bitset<NETWORK> network;

		StringVector str(Get(NETWORK), "|");
		if (str.empty())
		{
			network.set();
		}
		else
		{

			for (size_t i = 0; i < str.size(); i++)
			{
				size_t n = ToSizeT(str[i]);
				if (n < network.size())
					network.set(n);
			}
		}

		for (size_t n = 0; n < network.size()&&msg; n++)
		{
			if (network[n])
			{
				switch (n)
				{
				case FIRE: msg += ExecuteFire(callback); break;
				case AGRI: msg += ExecuteAgriculture(callback); break;
				}
			}
		}
		

		return msg;
	}

	ERMsg DownloadStation(CHttpConnectionPtr& pConnection, const std::string& ID, int year, std::string& text)
	{

		ERMsg msg;


		CString URL = _T("010-001/archive.aspx");
		CString strHeaders = _T("Content-Type: application/x-www-form-urlencoded\r\n");//\r\nUser-Agent: HttpCall\r\n

		
		CStringA strParam = "__EVENTTARGET=&__EVENTARGUMENT=&__VIEWSTATE=%2FwEPDwUKLTk1NTAxMjAxMA9kFgJmD2QWAgIED2QWAgIFD2QWBAIHDw9kDxAWA2YCAQICFgMWAh4OUGFyYW1ldGVyVmFsdWUFAjQ3FgIfAGUWAh8AZRYDZmZmZGQCCQ9kFhACBQ8QZA8WE2YCAQICAgMCBAIFAgYCBwIIAgkCCgILAgwCDQIOAg8CEAIRAhIWExAFDUFiZXJkZWVuICg0NykFAjQ3ZxAFDEFuZG92ZXIgKDQ5KQUCNDlnEAUNQnJpZ2h0b24gKDYyKQUCNjJnEAUNRHJ1bW1vbmQgKDQyKQUCNDJnEAUNRHJ1bW1vbmQgKDQ1KQUCNDVnEAUNRHJ1bW1vbmQgKDUxKQUCNTFnEAUNRHJ1bW1vbmQgKDU1KQUCNTVnEAULR29yZG9uICgzNikFAjM2ZxAFCUtlbnQgKDM3KQUCMzdnEAUNUmljaG1vbmQgKDU5KQUCNTlnEAUQU2FpbnQtQW5kcmUgKDQxKQUCNDFnEAUQU2FpbnQtQW5kcmUgKDY3KQUCNjdnEAUSU2FpbnQtTGVvbmFyZCAoNjgpBQI2OGcQBQxTaW1vbmRzICg3MCkFAjcwZxAFDldha2VmaWVsZCAoNjYpBQI2NmcQBQxXaWNrbG93ICg2NSkFAjY1ZxAFC1dpbG1vdCAoNTMpBQI1M2cQBQtXaWxtb3QgKDczKQUCNzNnEAUOV29vZHN0b2NrICg2OSkFAjY5Z2RkAgkPEGQPFh9mAgECAgIDAgQCBQIGAgcCCAIJAgoCCwIMAg0CDgIPAhACEQISAhMCFAIVAhYCFwIYAhkCGgIbAhwCHQIeFh8QBQExBQExZxAFATIFATJnEAUBMwUBM2cQBQE0BQE0ZxAFATUFATVnEAUBNgUBNmcQBQE3BQE3ZxAFATgFAThnEAUBOQUBOWcQBQIxMAUCMTBnEAUCMTEFAjExZxAFAjEyBQIxMmcQBQIxMwUCMTNnEAUCMTQFAjE0ZxAFAjE1BQIxNWcQBQIxNgUCMTZnEAUCMTcFAjE3ZxAFAjE4BQIxOGcQBQIxOQUCMTlnEAUCMjAFAjIwZxAFAjIxBQIyMWcQBQIyMgUCMjJnEAUCMjMFAjIzZxAFAjI0BQIyNGcQBQIyNQUCMjVnEAUCMjYFAjI2ZxAFAjI3BQIyN2cQBQIyOAUCMjhnEAUCMjkFAjI5ZxAFAjMwBQIzMGcQBQIzMQUCMzFnZGQCCw8QZA8WDGYCAQICAgMCBAIFAgYCBwIIAgkCCgILFgwQBQExBQExZxAFATIFATJnEAUBMwUBM2cQBQE0BQE0ZxAFATUFATVnEAUBNgUBNmcQBQE3BQE3ZxAFATgFAThnEAUBOQUBOWcQBQIxMAUCMTBnEAUCMTEFAjExZxAFAjEyBQIxMmdkZAINDxBkDxYJZgIBAgICAwIEAgUCBgIHAggWCRAFBDIwMDcFBDIwMDdnEAUEMjAwOAUEMjAwOGcQBQQyMDA5BQQyMDA5ZxAFBDIwMTAFBDIwMTBnEAUEMjAxMQUEMjAxMWcQBQQyMDEyBQQyMDEyZxAFBDIwMTMFBDIwMTNnEAUEMjAxNAUEMjAxNGcQBQQyMDE1BQQyMDE1Z2RkAhMPEGQPFh9mAgECAgIDAgQCBQIGAgcCCAIJAgoCCwIMAg0CDgIPAhACEQISAhMCFAIVAhYCFwIYAhkCGgIbAhwCHQIeFh8QBQExBQExZxAFATIFATJnEAUBMwUBM2cQBQE0BQE0ZxAFATUFATVnEAUBNgUBNmcQBQE3BQE3ZxAFATgFAThnEAUBOQUBOWcQBQIxMAUCMTBnEAUCMTEFAjExZxAFAjEyBQIxMmcQBQIxMwUCMTNnEAUCMTQFAjE0ZxAFAjE1BQIxNWcQBQIxNgUCMTZnEAUCMTcFAjE3ZxAFAjE4BQIxOGcQBQIxOQUCMTlnEAUCMjAFAjIwZxAFAjIxBQIyMWcQBQIyMgUCMjJnEAUCMjMFAjIzZxAFAjI0BQIyNGcQBQIyNQUCMjVnEAUCMjYFAjI2ZxAFAjI3BQIyN2cQBQIyOAUCMjhnEAUCMjkFAjI5ZxAFAjMwBQIzMGcQBQIzMQUCMzFnZGQCFQ8QZA8WDGYCAQICAgMCBAIFAgYCBwIIAgkCCgILFgwQBQExBQExZxAFATIFATJnEAUBMwUBM2cQBQE0BQE0ZxAFATUFATVnEAUBNgUBNmcQBQE3BQE3ZxAFATgFAThnEAUBOQUBOWcQBQIxMAUCMTBnEAUCMTEFAjExZxAFAjEyBQIxMmdkZAIXDxBkDxYJZgIBAgICAwIEAgUCBgIHAggWCRAFBDIwMDcFBDIwMDdnEAUEMjAwOAUEMjAwOGcQBQQyMDA5BQQyMDA5ZxAFBDIwMTAFBDIwMTBnEAUEMjAxMQUEMjAxMWcQBQQyMDEyBQQyMDEyZxAFBDIwMTMFBDIwMTNnEAUEMjAxNAUEMjAxNGcQBQQyMDE1BQQyMDE1Z2RkAh8PPCsADQBkGAEFHGN0bDAwJENvbnRlbnQxJGd2QXJjaGl2ZURhdGEPZ2ToC%2Fkx8tP9Qp93CvCxoYUfKz%2B%2F6A%3D%3D&__VIEWSTATEGENERATOR=7FE89812&__EVENTVALIDATION=%2FwEWhAEC6vCn2AwCjPGn3gECkN%2Fz0AcC7Of0OwL3wr3hAgLmha0BAtCk16ACAtCkn6MCAtak%2B6ACAtCk%2B6ACAtCkz6ACAtek%2F6ACAtekz6ACAtGky6ACAtGk16ACAtekn6MCAtCk%2F6ACAtak16ACAtakk6MCAtWk86ACAtaky6ACAtakz6ACAtekx6ACAtWkx6ACAtakn6MCAuLr9sIDAuPr9sIDAuDr9sIDAuHr9sIDAubr9sIDAufr9sIDAuTr9sIDAvXr9sIDAvrr9sIDAuLrtsEDAuLrusEDAuLrvsEDAuLrgsEDAuLrhsEDAuLrisEDAuLrjsEDAuLrksEDAuLr1sIDAuLr2sIDAuPrtsEDAuPrusEDAuPrvsEDAuPrgsEDAuPrhsEDAuPrisEDAuPrjsEDAuPrksEDAuPr1sIDAuPr2sIDAuDrtsEDAuDrusEDAtXQoYkBAtTQoYkBAtfQoYkBAtbQoYkBAtHQoYkBAtDQoYkBAtPQoYkBAsLQoYkBAs3QoYkBAtXQ4YoBAtXQ7YoBAtXQ6YoBApTRytAPApTR5rkJApTR0twBAv%2Fo4MUGAv%2Fo3JgNAv%2FoyL8EAv%2FopNIMAv%2FokOkLAv%2FojIwCAvKEiO4EAvS6t0wC9bq3TAL2urdMAve6t0wC8Lq3TALxurdMAvK6t0wC47q3TALsurdMAvS6908C9Lr7TwL0uv9PAvS6w08C9LrHTwL0ustPAvS6z08C9LrTTwL0updMAvS6m0wC9br3TwL1uvtPAvW6%2F08C9brDTwL1usdPAvW6y08C9brPTwL1utNPAvW6l0wC9bqbTAL2uvdPAva6%2B08C14iy%2BgQC1oiy%2BgQC1Yiy%2BgQC1Iiy%2BgQC04iy%2BgQC0oiy%2BgQC0Yiy%2BgQCwIiy%2BgQCz4iy%2BgQC14jy%2BQQC14j%2B%2BQQC14j6%2BQQCrte4hQQCrteU7AICrtegiQoCxe6SkA0Cxe6uzQYCxe666g8Cxe7WhwcCxe7iPALF7v7ZCQL0x6SlAQKA3uC2A0LNcpbYprj4QvUiSHNkEUzsHvnf&ctl00%24hfLang=fr-CA&";
		strParam += ("ctl00%24Content1%24ddlWS=" + ID).c_str();
		strParam += "&ctl00%24Content1%24ddlFromDay=1";
		strParam += "&ctl00%24Content1%24ddlFromMonth=1";
		strParam += ("&ctl00%24Content1%24ddlFromYear=" + ToString(year)).c_str();
		strParam += "&ctl00%24Content1%24hdnFromDate=";
		strParam += "&ctl00%24Content1%24ddlToDay=31" ;
		strParam += "&ctl00%24Content1%24ddlToMonth=12";
		strParam += ("&ctl00%24Content1%24ddlToYear=" + ToString(year)).c_str();
		strParam += "&ctl00%24Content1%24hdnToDate=&ctl00%24Content1%24btnGetData=Submit";
		
		
		DWORD HttpRequestFlags = INTERNET_FLAG_EXISTING_CONNECT |INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE;
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
			while ((charRead = pURLFile->Read(&(tmp[0]), MAX_READ_SIZE))>0)
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

		callback.PushTask("Download New Brunswick agriculture data", fileList.size());

		int nbRun = 0;
		size_t curI = 0;
		while (curI < fileList.size() && msg)
		{
			nbRun++;

			CInternetSessionPtr pSession;
			CHttpConnectionPtr pConnection;

			msg = GetHttpConnection(SERVER_NAME[AGRI], pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS);
			if (msg)
			{
				TRY
				{
					for (size_t i = curI; i < fileList.size() && msg; i++)
					{
						for (size_t y = 0; y < nbYears&&msg; y++)
						{
							int year = firstYear + int(y);
							//size_t lastMonth = year == currentTRef.GetYear() ? currentTRef.GetMonth() + 1: 12;
							//for (size_t m = 0; m < lastMonth&&msg; m++)
							//{
							string str;
							msg = DownloadStation(pConnection, fileList[i], year, str);

							//split data in seperate files
							if (msg)
							{
								string::size_type pos1 = str.find("<table class=\"gridviewBorder\"");
								string::size_type pos2 = str.find("</table>", pos1);

								if (pos1 != string::npos && pos2 != string::npos)
								{
									string tmp = "<?xml version=\"1.0\" encoding=\"Windows-1252\"?>\r\n" + str.substr(pos1, pos2 - pos1 + 9);


									string filePath = GetOutputFilePath(AGRI, fileList[i], year);
									CreateMultipleDir(GetPath(filePath));
									msg += SaveStation(filePath, tmp);
								}

								curI++;
								msg += callback.StepIt();
								//bDownloaded = true;
							}

							//}//month

						}//year
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


		callback.AddMessage(GetString(IDS_NB_FILES_DOWNLOADED) + ToString(curI), 1);
		callback.PopTask();

		return msg;
	}

	ERMsg CUINewBrunswick::ExecuteFire(CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);
		msg = CreateMultipleDir(workingDir);


		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(workingDir, 1);
		callback.AddMessage(GetString(IDS_UPDATE_FROM));
		callback.AddMessage(string(SERVER_NAME[FIRE]), 1);
		callback.AddMessage("");




		CFileInfoVector fileList;
		GetFileList(FIRE, fileList, callback);

		callback.PushTask("Download New Brunswick data", fileList.size());


		size_t curI = 0;
		bool bDownloaded = false;
		int year = CTRef::GetCurrentTRef().GetYear();

		CInternetSessionPtr pSession;
		CFtpConnectionPtr pConnection;

		msg = GetFtpConnection(SERVER_NAME[FIRE], pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, Get(USER_NAME), Get(PASSWORD));
		if (msg)
		{
			TRY
			{
				for (size_t i = curI; i < fileList.size() && msg; i++)
				{
					string ID = GetFileTitle(fileList[i].m_filePath);
					string outputFilePath = GetOutputFilePath(FIRE, ID, year);
					WBSF::CreateMultipleDir(GetPath(outputFilePath));
					
					msg = UtilWWW::CopyFile(pConnection, fileList[i].m_filePath, outputFilePath);


					//split data in seperate files
					if (msg)
					{
						curI++;
						msg += callback.StepIt();
					}
				}
			}
			CATCH_ALL(e)
			{
				msg = UtilWin::SYGetMessage(*e);
			}
			END_CATCH_ALL

			//clean connection
			pConnection->Close();
			pSession->Close();
		}

		callback.AddMessage(GetString(IDS_NB_FILES_DOWNLOADED) + ToString(curI), 1);
		callback.PopTask();

		return msg;
	}


	string CUINewBrunswick::GetOutputFilePath(size_t n, const string& ID, int year)const
	{
		static const char* NETWORK_NAME[NB_NETWORKS] {"Fire", "Agriculture"};
		return GetDir(WORKING_DIR) + NETWORK_NAME[n] + "\\" + ToString(year) + "\\" + ID + ".csv";
	}

	std::string CUINewBrunswick::GetStationListFilePath()const
	{
		
		return WBSF::GetApplicationPath() + "Layers\\NBStations.csv";
	}

	ERMsg CUINewBrunswick::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;

		
		msg = m_stations.Load(GetStationListFilePath());

		if (msg)
			msg += m_stations.IsValid();

		if (msg)
		{
			for (size_t i = 0; i < m_stations.size(); i++)
				stationList.push_back(m_stations[i].m_ID);
		}

		return msg;
	}

	ERMsg CUINewBrunswick::GetWeatherStation(const std::string& ID, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;

		//Get station information
		size_t it = m_stations.FindByID(ID);
		if (it == NOT_INIT)
		{
			msg.ajoute(FormatMsg(IDS_NO_STATION_INFORMATION, ID));
			return msg;
		}
		
		((CLocation&)station) = m_stations[it];

		size_t n = FIRE;//station.GetSSI("NETWORK");
		//station.m_name = TraitFileName(station.m_name);

		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = lastYear - firstYear + 1;
		station.CreateYears(firstYear, nbYears);

		station.m_name = PurgeFileName(station.m_name);

		//now extract data 
		for (size_t y = 0; y < nbYears&&msg; y++)
		{
			int year = firstYear + int(y);

			string filePath = GetOutputFilePath(n, ID, year);
			if (FileExists(filePath))
				msg = ReadData(filePath, TM, station[year], callback);

			msg += callback.StepIt(0);
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
		ASSERT(v.size()==2);

		return ToSizeT(v[0]);
	}


	ERMsg CUINewBrunswick::ReadData(const string& filePath, CTM TM, CWeatherYear& data, CCallback& callback)const
	{
		ERMsg msg;

		//now extact data 
		ifStream file;
		msg = file.open(filePath);

		if (msg)
		{
			CWeatherAccumulator accumulator(TM);
			vector<size_t> variables;

			size_t i = 0;
			for (CSVIterator loop(file); loop != CSVIterator() && msg; ++loop, i++)
			{
				if (variables.empty())
				{
					vector<size_t> columns = GetColumns(loop.Header());
					variables = GetVariables(IsHourly(), columns);
				}
					
				ASSERT(loop->size() <= variables.size());

				int year = ToInt((*loop)[C_YEAR]);
				size_t month = ToInt((*loop)[C_MONTH]) - 1;
				size_t day = ToInt((*loop)[C_DAY]) - 1;
				size_t hour = GetHour((*loop)[C_TIME]);
				
				ASSERT(month < 12);
				ASSERT(day < GetNbDayPerMonth(year, month));
				ASSERT(hour < 24);

				CTRef TRef = IsHourly() ? CTRef(year, month, day, hour) : CTRef(year, month, day);

				if (accumulator.TRefIsChanging(TRef))
				{
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

								accumulator.Add(TRef, variables[c], value);
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

