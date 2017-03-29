#include "StdAfx.h"
#include "UIWeatherFarm.h"
#include <boost\dynamic_bitset.hpp>
#include <boost\filesystem.hpp>

#include "UI/Common/UtilWin.h"
#include "Basic/DailyDatabase.h"
#include "Basic/FileStamp.h"
#include "UI/Common/SYShowMessage.h"
#include "Basic\CSV.h"
#include "json\json11.hpp"

#include "TaskFactory.h"
#include "../Resource.h"
#include "WeatherBasedSimulationString.h"


using namespace std; 
using namespace WBSF::HOURLY_DATA;
using namespace UtilWWW;
using namespace boost;
using namespace json11;

//Forecast fro USA
//https://darksky.net/dev/docs/forecast
//https://api.darksky.net/forecast/[key]/[latitude],[longitude]


//plus de donn…es sur:
//ftp://mawpvs.dyndns.org/Partners/AI/
//ftp://mawpvs.dyndns.org/DailySummary/

//POST /historical-data/ HTTP/1.1
//Host: weatherfarm.com
//Connection: keep-alive
//Content-Length: 37
//Cache-Control: max-age=0
//Origin: http://weatherfarm.com
//Upgrade-Insecure-Requests: 1
//User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/56.0.2924.87 Safari/537.36
//Content-Type: application/x-www-form-urlencoded
//Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8
//Referer: http://weatherfarm.com/historical-data/
//Accept-Encoding: gzip, deflate
//Accept-Language: fr-FR,fr;q=0.8,en-US;q=0.6,en;q=0.4
//Cookie: __cfduid=dcf42fb516b31804ab50ce5fbc06f3f1b1490393015; bm_monthly_unique=true; __gads=ID=d6d70cfa67089697:T=1490393017:S=ALNI_MaJv1iDg69zeamIVN_P371IR1CcOw; PHPSESSID=bjc2liqaorkaju0jlo3916qg26; __uvt=; __insp_wid=1405423566; __insp_slim=1490393023499; __insp_nv=true; __insp_targlpu=http%3A%2F%2Fweatherfarm.com%2F; __insp_targlpt=WeatherFarm%20%7C%20Canada%E2%80%99s%20Largest%20Weather%20Network%20%7C%20Serving%20Farmers%20Weather%20Needs; __insp_norec_sess=true; wf_fbc_zoom_level=6; default_station=P1180; bm_daily_unique=true; bm_sample_frequency=100; _gat=1; _dc_gtm_UA-56476360-1=1; _dc_gtm_UA-56526625-1=1; _gali=frm-historical-data; bm_last_load_status=NOT_BLOCKING; _ga=GA1.2.89097397.1490393023; fsk_ut_146=A8JFDMYkAbHv3OBSTfk8YCJOIPhFdj; fsk_uts=deaf6cf3-02a3-4dba-80cd-f83d0a083d03; uvts=5oTr7PIlvdZgNWWV


//http://weatherfarm.com/feeds/historical-data/?station-id=P0484&from-date=03/24/2017&to-date=03/24/2017&report-type=hourly&date-range=yesterday
//http://weatherfarm.com/feeds/historical-data/?station-id=P0484&from-date=03/24/2017&to-date=03/25/2017&report-type=hourly&date-range=last2days
//http://weatherfarm.com/feeds/historical-data/?station-id=P0484&from-date=03/21/2017&to-date=03/25/2017&report-type=hourly&date-range=last5days
//http://weatherfarm.com/feeds/get-current-weather-by-geo/?latMin=48&latMax=52&longMin=-117&longMax=-115&callback=station_data

//coordonner en sakatchewan
//http://environment.gov.sk.ca/saskspills/spills_srch.asp



//Norquay, SK Swan Plain station-id=P0656
//http://weatherfarm.com/feeds/historical-data/?date-range=last5days&station-id=P0484&from-date=03/21/2017&to-date=03/25/2017&report-type=hourly


//APAS - RM of Sask Landing  Leinan, SK P1098
namespace WBSF
{

	const char* CUIWeatherFarm::SUBDIR_NAME[NB_NETWORKS] = { "Agriculture" };
	const char* CUIWeatherFarm::NETWORK_NAME[NB_NETWORKS] = { "Agriculture", };
	const char* CUIWeatherFarm::SERVER_NAME[NB_NETWORKS] = { "weatherfarm.com", };
	const char* CUIWeatherFarm::SERVER_PATH[NB_NETWORKS] = { "historical-data/" };

	size_t CUIWeatherFarm::GetNetwork(const string& network)
	{
		size_t n = NOT_INIT;

		for (size_t i = 0; i < NB_NETWORKS && n == NOT_INIT; i++)
		{
			if (IsEqualNoCase(network, NETWORK_NAME[i]))
				n = i;
		}

		return n;
	}

	//*********************************************************************
	const char* CUIWeatherFarm::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "FirstYear", "LastYear", };
	const size_t CUIWeatherFarm::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_STRING, T_STRING, };
	const UINT CUIWeatherFarm::ATTRIBUTE_TITLE_ID = IDS_UPDATER_MANITOBA_P;
	const UINT CUIWeatherFarm::DESCRIPTION_TITLE_ID = ID_TASK_WEATHER_FARM;

	const char* CUIWeatherFarm::CLASS_NAME(){ static const char* THE_CLASS_NAME = "WeatherFarm";  return THE_CLASS_NAME; }
	CTaskBase::TType CUIWeatherFarm::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUIWeatherFarm::CLASS_NAME(), (createF)CUIWeatherFarm::create);



	CUIWeatherFarm::CUIWeatherFarm(void)
	{}

	CUIWeatherFarm::~CUIWeatherFarm(void)
	{}



	std::string CUIWeatherFarm::Option(size_t i)const
	{
		string str;
		//switch (i)
		//{
		//};
		return str;
	}

	std::string CUIWeatherFarm::Default(size_t i)const
	{
		string str;

		switch (i)
		{
		case WORKING_DIR: str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()) + "WeatherFarm\\"; break;
		case FIRST_YEAR:
		case LAST_YEAR:	str = ToString(CTRef::GetCurrentTRef().GetYear()); break;
		};

		return str;
	}

	//****************************************************


	std::string CUIWeatherFarm::GetStationsListFilePath()const
	{
		static const char* FILE_NAME[NB_NETWORKS] = { "WeatherFarm.csv" };

		string filePath = WBSF::GetApplicationPath() + "Layers\\" + FILE_NAME[0];
		return filePath;
	}

	string CUIWeatherFarm::GetOutputFilePath(const string& title, int year)const
	{
		return GetDir(WORKING_DIR) + ToString(year) + "\\" + title + ".csv";
	}


	ERMsg CUIWeatherFarm::Execute(CCallback& callback)
	{
		ERMsg msg;

		callback.PushTask("DownloadWeatehrFarm Data", 1);

		string workingDir = GetDir(WORKING_DIR);
		msg = CreateMultipleDir(workingDir);

		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(workingDir, 1);
		callback.AddMessage(GetString(IDS_UPDATE_FROM));
		callback.AddMessage(string(SERVER_NAME[0]) + "/" + SERVER_PATH[0], 1);
		callback.AddMessage("");

	//	msg = ExecuteAgriculture(callback);
		msg += callback.StepIt();

		callback.PopTask();


		return msg;
	}


	ERMsg CUIWeatherFarm::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;

		m_stations.clear();
		CLocationVector locations;
		msg = locations.Load(GetStationsListFilePath());

		if (msg)
			msg += locations.IsValid();

		//Update network
		for (size_t i = 0; i < locations.size(); i++)
			locations[i].SetSSI("Network", NETWORK_NAME[0]);

		m_stations.insert(m_stations.end(), locations.begin(), locations.end());

		for (size_t i = 0; i < locations.size(); i++)
			stationList.push_back(locations[i].m_ID);

		return msg;
	}

	ERMsg CUIWeatherFarm::GetWeatherStation(const std::string& NID, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;

		size_t n = ToSizeT(NID.substr(0, 1));
		string ID = NID.substr(2);

		//Get station information
		size_t it = m_stations.FindByID(ID);
		ASSERT(it != NOT_INIT);

		((CLocation&)station) = m_stations[it];


		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = lastYear - firstYear + 1;
		station.CreateYears(firstYear, nbYears);

		station.m_name = PurgeFileName(station.m_name);

		//now extract data 
		for (size_t y = 0; y < nbYears&&msg; y++)
		{
			int year = firstYear + int(y);

			string filePath = GetOutputFilePath(ID, year);
			if (FileExists(filePath))
				msg = station.LoadData(filePath, -999, false);

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


	//**********************************************************************************************************

	ERMsg CUIWeatherFarm::DownloadStationData(CHttpConnectionPtr& pConnection, const std::string& ID, CTRef TRef, std::string& text)
	{
		ERMsg msg;


		CString URL;
		//CString strHeaders = _T("Content-Type: application/x-www-form-urlencoded\r\n");
		CStringA strParam;


		URL = _T("climate/HourlyReport.aspx");
		string strDate = FormatA("%4d-%02d-%02d", TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1);
		strParam += "__VIEWSTATE=%2FwEPDwULLTEwOTU4OTMxNzYPZBYCZg9kFgICAw9kFgICAQ9kFgYCAQ9kFgQCAQ9kFgICAQ9kFgJmDxAPFgYeDURhdGFUZXh0RmllbGQFC1N0YXRpb25OYW1lHg5EYXRhVmFsdWVGaWVsZAUFU3RuSUQeC18hRGF0YUJvdW5kZ2QQFSgGQWx0b25hBkFyYm9yZwZCaXJ0bGUKQm9pc3NldmFpbglEZWxvcmFpbmUGRHVnYWxkCUVsbSBDcmVlawlFcmlrc2RhbGUJRXRoZWxiZXJ0B0ZvcnJlc3QJR2xhZHN0b25lCEdsZW5ib3JvCUdyYW5kdmlldwdIYW1pb3RhCUtpbGxhcm5leQlMZXRlbGxpZXIHTWFuaXRvdQxNZWxpdGEgU291dGgJTWlubmVkb3NhCU1vb3NlaG9ybgZNb3JyaXMHUGllcnNvbgxQb3J0YWdlIEVhc3QGUmVzdG9uB1J1c3NlbGwHU2Vsa2lyawhTb21lcnNldAZTb3VyaXMJU3QgUGllcnJlC1N0LiBBZG9scGhlCFN0YXJidWNrCVN0ZS4gUm9zZQlTdGVpbmJhY2gLU3dhbiBWYWxsZXkGVGV1bG9uCFRyZWhlcm5lBlZpcmRlbghXYXdhbmVzYQ1XaW5rbGVyIENNQ0RDCVdvb2RsYW5kcxUoAzI0NAMyMDYDMjE2AzIwOQMyNDEDMjE3AzIzNwMyMTgDMjEzAzIzMwMyMDQDMjM5AzIxOQMyMTQDMjIwAzIzOAMyNDIDNzQwAzIyMQMyMDUDMjIyAzIzMgMyMzUDMjQ1AzIxNQMyMTADMjQ2AzIwOAMyMDMDMjQzAzIwMgMyMTEDMjIzAzIzMQMyMDcDMjAxAzIyNAMyNDADMjMwAzIyNRQrAyhnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZGQCAg9kFgICAQ9kFgICAg8PZBYMHgtvbk1vdXNlT3ZlcgU8Q2hhbmdlTW91c2UoJ2N0bDAwX0RlZmF1bHRDb250ZW50X2ltZ0hSRGF0ZScsICdvbk1vdXNlT3ZlcicpHgpvbk1vdXNlT3V0BTtDaGFuZ2VNb3VzZSgnY3RsMDBfRGVmYXVsdENvbnRlbnRfaW1nSFJEYXRlJywgJ29uTW91c2VPdXQnKR4Lb25Nb3VzZURvd24FPUNoYW5nZUJvcmRlcignY3RsMDBfRGVmYXVsdENvbnRlbnRfaW1nSFJEYXRlJywgJ29uTW91c2VEb3duJykeCW9uTW91c2VVcAU7Q2hhbmdlQm9yZGVyKCdjdGwwMF9EZWZhdWx0Q29udGVudF9pbWdIUkRhdGUnLCAnb25Nb3VzZVVwJykeB29uQ2xpY2sFLlNob3dDYWxlbmRhcignY3RsMDBfRGVmYXVsdENvbnRlbnRfdHh0SFJEYXRlJykeCm9uS2V5UHJlc3MFLlNob3dDYWxlbmRhcignY3RsMDBfRGVmYXVsdENvbnRlbnRfdHh0SFJEYXRlJylkAgMPZBYEZg9kFgJmD2QWAmYPDxYCHgRUZXh0BSE8Yj5Ib3VybHkgUmF3IERhdGEgZm9yIEFsdG9uYTwvYj5kZAIBD2QWAmYPZBYCZg8PFgIfCQUYPGI%2BRmVicnVhcnkgMDEsIDIwMTc8L2I%2BZGQCBQ88KwANAGQYAQUjY3RsMDAkRGVmYXVsdENvbnRlbnQkZ2RIb3VybHlSZXBvcnQPZ2RqOfHRBZakZ7g%2FfO653G9kKVd%2Fqg%3D%3D&";
		strParam += "__EVENTVALIDATION=%2FwEWKwLgns6JDALx5%2BaKCAKvkdK9BAKvka69BAKO%2Ba6hCAKmo4nLCQKAusjKDgKAusDKDgK10IiUBgKc3fD9BQKc3cj9BQLx55aLCAKO%2BaKhCAKO%2BaqhCALx55KLCALNmpe%2BBwK10ICUBgK7tKvgAwLGmu%2B9BwKmo7HLCQLKiLCgAgK7tNPgAwK7tK%2FgAwLKiISgAgLKiICgAgLKiIygAgLNmpu%2BBwKvkaK9BAK10IyUBgKc3fT9BQKc3cT9BQK7tNvgAwKmo7XLCQKc3cz9BQKmo43LCQKAuszKDgKmo7nLCQLx5%2B6KCALNmu%2B9BwLNmpO%2BBwLKiIigAgKho5PqDALa8rvJA0HoHMqTYMMp8eVkb8x3GI60%2FxwX&";
		strParam += ("ctl00%24DefaultContent%24cboStationNames=" + ID + "&").c_str();
		strParam += ("ctl00%24DefaultContent%24txtHRDate=" + strDate + "&").c_str();
		strParam += "ctl00%24DefaultContent%24btn_HRSearch=Submit";

		DWORD HttpRequestFlags = INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE;
		CHttpFile* pURLFile = pConnection->OpenRequest(CHttpConnection::HTTP_VERB_POST, URL, NULL, 1, NULL, NULL, HttpRequestFlags);


		bool bRep = false;

		if (pURLFile != NULL)
		{
			{
				TRY
				{
					CString strContentL;
					strContentL.Format(_T("Content-Length: %d\r\n"), strParam.GetLength());

					pURLFile->AddRequestHeaders(strContentL);
					pURLFile->AddRequestHeaders(CString("Content-Type: application/x-www-form-urlencoded\r\n"));

					// send request
					bRep = pURLFile->SendRequest(0, 0, (void*)(const char*)strParam, strParam.GetLength()) != 0;
				}
					CATCH_ALL(e)

				{
					DWORD errnum = GetLastError();

					if (errnum == 12002 || errnum == 12029)
					{
						msg = UtilWin::SYGetMessage(*e);
					}
					else if (errnum == 12031 || errnum == 12111)
					{
						//throw a exception: server reset
						msg = UtilWin::SYGetMessage(*e);
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

			if (msg)
			{
				if (bRep)
				{
					const short MAX_READ_SIZE = 4096;
					pURLFile->SetReadBufferSize(MAX_READ_SIZE);

					std::string tmp;
					tmp.resize(MAX_READ_SIZE);
					UINT charRead = 0;
					while ((charRead = pURLFile->Read(&(tmp[0]), MAX_READ_SIZE)) > 0)
						text.append(tmp.c_str(), charRead);
				}
				else
				{
					CString tmp;
					tmp.FormatMessage(IDS_CMN_UNABLE_LOAD_PAGE, URL);
					msg.ajoute(UtilWin::ToUTF8(tmp));
				}
			}

			pURLFile->Close();
			delete pURLFile;
		}

		return msg;
	}

	ERMsg CUIWeatherFarm::GetHistoricalStationList(size_t dataType, StringVector& fileList, CCallback& callback)
	{
		ERMsg msg;

		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;

		msg = GetHttpConnection(SERVER_NAME[0], pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS);
		if (msg)
		{
			string str;
			msg = UtilWWW::GetPageText(pConnection, SERVER_PATH[0], str);
			if (msg)
			{
				string::size_type pos1 = str.find("<table");
				string::size_type pos2 = NOT_INIT;
				if (pos1 < str.size())
				{
					pos1 = str.find("<select", pos1);
					pos2 = str.find("</select>", pos1);
				}

				if (pos1 != string::npos && pos2 != string::npos)
				{
					try
					{
						string xml_str = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n" + str.substr(pos1, pos2 - pos1 + 9);
						zen::XmlDoc doc = zen::parse(xml_str);

						zen::XmlIn in(doc.root());
						for (zen::XmlIn it = in["option"]; it; it.next())
						{
							string value;
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


			pConnection->Close();
			pSession->Close();

		}

		return msg;
	}


	ERMsg CUIWeatherFarm::SaveAgricultureHourlyStation(const std::string& filePath, std::string str)
	{
		ERMsg msg;

		CWeatherYears data(true);

		enum THourlyColumns{ C_HOUR, C_TEMP, C_RH, C_RAIN, C_WIND_SPEED, C_WIND_DIR, C_PEAK_WIND, C_SOIL_TEMP, NB_COLUMNS };
		static const TVarH HOURLY_VARS[NB_COLUMNS] = { H_SKIP, H_TAIR2, H_RELH, H_PRCP, H_WNDS, H_WNDD, H_SKIP, H_ADD1 };

		try
		{
			int ID = ToInt(GetFileTitle(filePath));
			WBSF::ReplaceString(str, "\t", "");

			zen::XmlDoc doc = zen::parse(str);

			zen::XmlIn in(doc.root());
			for (zen::XmlIn itDay = in["table"]; itDay; itDay.next())
			{
				string date;
				itDay.get()->getAttribute("date", date);
				CTRef TRef;
				TRef.FromFormatedString(date);
				TRef.Transform(CTM(CTM::HOURLY));

				for (zen::XmlIn itTR = itDay["tr"]; itTR; itTR.next())
				{
					StringVector tmp;
					for (zen::XmlIn itTD = itTR["td"]; itTD; itTD.next())
					{
						string value;
						itTD["font"](value);
						Trim(value);
						tmp.push_back(value);
					}//for all columns


					if (tmp.size() == NB_COLUMNS)
					{
						string strHour = tmp[C_HOUR];
						size_t hour = ToSizeT(strHour.substr(0, 2));

						if (strHour == "12AM")
							hour = 0;
						else if (strHour.substr(2) == "PM" && strHour != "12PM")
							hour += 12;

						TRef.m_hour = hour;
						ASSERT(TRef.IsValid());

						for (size_t i = 0; i < NB_COLUMNS; i++)
						{
						}
					}
				}
			}

			if (msg && data.size() == 1)
			{
				//save annual data
				const CWeatherYear& d = data[size_t(0)];
				msg = d.SaveData(filePath);
			}//if msg
		}
		catch (const zen::XmlParsingError& e)
		{
			// handle error
			msg.ajoute("Error parsing XML file: col=" + WBSF::ToString(e.col) + ", row=" + WBSF::ToString(e.row));
		}


		return msg;
	}


}