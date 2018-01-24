#include "StdAfx.h"
#include "UIWeatherFarm.h"
#include "UIManitoba.h"
#include <boost\dynamic_bitset.hpp>
#include <boost\filesystem.hpp>

#include "UI/Common/UtilWin.h"
#include "Basic/DailyDatabase.h"
#include "Basic/FileStamp.h"
#include "UI/Common/SYShowMessage.h"
#include "Basic\CSV.h"
#include "json\json11.hpp"
#include "StateSelection.h"
#include "ProvinceSelection.h"


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


//plus de donnÉes sur:
//ftp://mawpvs.dyndns.org/Partners/AI/
//ftp://mawpvs.dyndns.org/DailySummary/


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

	const char* CUIWeatherFarm::SERVER_NAME = { "weatherfarm.com", };
	const char* CUIWeatherFarm::SERVER_PATH = { "historical-data/" };



	//*********************************************************************
	const char* CUIWeatherFarm::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "MemberName", "Password", "WorkingDir", "Province", "DateRangeType", "ForceUpdateStationsList" };
	const size_t CUIWeatherFarm::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_STRING_SELECT, T_PASSWORD, T_PATH, T_STRING_SELECT, T_COMBO_INDEX, T_BOOL };
	const UINT CUIWeatherFarm::ATTRIBUTE_TITLE_ID = IDS_UPDATER_WEATHER_FARM_P;
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
		switch (i)
		{
		//case STATES:	str = CStateSelection::GetAllPossibleValue(); break;
		case PROVINCE:	str = CProvinceSelection::GetAllPossibleValue(); break;
		case DATE_RANGE_TYPE: str = "Today|Yesterday|Last 2 days|Last 5 days"; break;
		};
		return str;
	}

	std::string CUIWeatherFarm::Default(size_t i)const
	{
		string str;

		switch (i)
		{
		case WORKING_DIR: str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()) + "WeatherFarm\\"; break;
		//case STATES: str = "----"; break;
		//case ADD_OTHER: str = "0"; break;
		case PROVINCE:	str = "BC|AB|SK|MB"; break;
		case DATE_RANGE_TYPE: str = ToString(DR_YESTERFAY); break;
		case FORCE_UPDATE_STATIONS_LIST: str = "0"; break;

		};

		return str;
	}

	//****************************************************

	
	std::string CUIWeatherFarm::GetStationsListFilePath()const
	{
		string filePath = Get(WORKING_DIR) + "WeatherFarmStationList.csv";

		return filePath;
	}

	string CUIWeatherFarm::GetOutputFilePath(const string& title, int year)const
	{
		return GetDir(WORKING_DIR) + ToString(year) + "\\" + title + ".csv";
	}

	
	ERMsg CUIWeatherFarm::Execute(CCallback& callback)
	{
		ERMsg msg;

		

		string workingDir = GetDir(WORKING_DIR);
		msg = CreateMultipleDir(workingDir);

		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(workingDir, 1);
		callback.AddMessage(GetString(IDS_UPDATE_FROM));
		callback.AddMessage(string(SERVER_NAME) + "/" + SERVER_PATH, 1);
		callback.AddMessage("");

		size_t dateRange = as<size_t>(DATE_RANGE_TYPE);

		//http://weatherfarm.com/feeds/historical-data/?date-range=today&station-id=P1098&from-date=03/29/2017&to-date=03/29/2017&report-type=hourly
		//http://weatherfarm.com/feeds/historical-data/?date-range=yesterday&station-id=P1098&from-date=03/28/2017&to-date=03/28/2017&report-type=hourly
		//http://weatherfarm.com/feeds/historical-data/?date-range=last2days&station-id=P1098&from-date=03/28/2017&to-date=03/29/2017&report-type=hourly
		//http://weatherfarm.com/feeds/historical-data/?date-range=last5days&station-id=P0108&from-date=04/01/2017&to-date=04/05/2017&report-type=hourly
		//
		//http://weatherfarm.com/feeds/set-current-station?station_id=P0590
		//http://weatherfarm.com/feeds/set-current-station?station_id=P1098
		//http://weatherfarm.com/feeds/set-current-station?station_id=P1098
		//http://weatherfarm.com/feeds/set-current-station?station_id=P0108

		int RANGE_SHIFT[NB_DATE_RANGES][2] = { { 0, 0 }, { 1, 1 }, { 0, 2 }, { 0, 5 } };

		CTRef today = CTRef::GetCurrentTRef();
		
		CTRef endTRef = today - RANGE_SHIFT[dateRange][0];
		CTRef startTRef = today - RANGE_SHIFT[dateRange][1];
		
		
		CProvinceSelection provinces(Get(PROVINCE));
		CLocationVector stationListTmp;

		if (!FileExists(GetStationsListFilePath()) || as<bool>(FORCE_UPDATE_STATIONS_LIST))
		{

			msg = DownloadStationsList(stationListTmp, callback);
			if (msg)
				msg = stationListTmp.Save(GetStationsListFilePath(), ',', callback);
		}
		else
		{
			msg = stationListTmp.Load(GetStationsListFilePath(), ",", callback);
		}

		if (!msg)
			return msg;

		CLocationVector stationList;
		stationList.reserve(stationListTmp.size());
		for (auto it = stationListTmp.begin(); it < stationListTmp.end() && msg; it++)
		{
			string state = it->GetSSI("Province");

			if ( provinces.at(state) )
				stationList.push_back(*it);

		}
	
		int nbDownload = 0;
		callback.PushTask("Download WeatherFarm stations data (" + ToString(stationList.size()) + " stations)", stationList.size());

		
		enum TColumns { C_DATE_TIME, C_TEMP, C_RAIN, C_RAIN_ACC, C_RH, C_WIND_SPEED, C_WIND_DIR, NB_COLUMNS };
		static const TVarH VARIABLES[NB_COLUMNS] = {H_SKIP, H_TAIR2, H_PRCP, H_SKIP, H_RELH, H_WNDS, H_WNDD };
		
		

		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;

		msg = GetHttpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, Get(MEMBER_NAME), Get(PASSWORD), true);
		if (msg)
		{
			//https://api.loginradius.com/raas/client/auth/login?apikey=75141eeb-8c9b-4a8e-8a29-653c80621116&emailverificationurl=https://weatherfarm.com/login-2/&template=undefined&password=Stamant74&emailid=tigroux74%40hotmail.com&callback=Loginradius584694468545443000
			//DWORD HttpRequestFlags = INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_SECURE;
			//CHttpFile* pURLFile = pConnection->OpenRequest(CHttpConnection::HTTP_VERB_POST, _T("/restricted-page"), NULL, 1, NULL, NULL, HttpRequestFlags);
			//
			//string text;
			//if (pURLFile != NULL)
			//{
			//	
			//	CStringA strParam = "lrloginid=Tigroux74%40hotmail.com&lrloginpw=Stamant74";

			//	CString strContentL;
			//	strContentL.Format(_T("Content-Length: %d\r\n"), strParam.GetLength());

			//	pURLFile->AddRequestHeaders(strContentL);
			//	pURLFile->AddRequestHeaders(CString("Content-Type: application/x-www-form-urlencoded; charset=UTF-8\r\n"));

			//	// send request
			//	bool bRep = pURLFile->SendRequest(0, 0, (void*)(const char*)strParam, strParam.GetLength()) != 0;
			//	if (bRep)
			//	{
			//		const short MAX_READ_SIZE = 4096;
			//		pURLFile->SetReadBufferSize(MAX_READ_SIZE);

			//		
			//		std::string tmp;
			//		tmp.resize(MAX_READ_SIZE);
			//		UINT charRead = 0;
			//		while ((charRead = pURLFile->Read(&(tmp[0]), MAX_READ_SIZE)) > 0)
			//			text.append(tmp.c_str(), charRead);


			//	}

			//	pURLFile->Close();
			//	delete pURLFile;
			//}

			//
			for (size_t i = 0; i < stationList.size() && msg; i++)
			{
				string str;
				msg = UtilWWW::GetPageText(pConnection, "feeds/set-current-station?station_id=" + stationList[i].m_ID, str, false, INTERNET_FLAG_SECURE | INTERNET_FLAG_EXISTING_CONNECT);
				if (msg)
				{
					string error;
					Json jsonMetadata = Json::parse(str, error);

					if (error.empty())
					{
						bool bRollover = jsonMetadata["reporting_from_rollover_station"].bool_value();
						bool bHaveData = jsonMetadata["data_available"].bool_value();

						if (!bRollover && bHaveData)
						{
							static const char* DATE_RANGE_NAME[NB_DATE_RANGES] = { "today", "yesterday", "last2days", "last5days" };

							string start = FormatA("%02d/%02d/%4d", startTRef.GetMonth() + 1, startTRef.GetDay() + 1, startTRef.GetYear());
							string end = FormatA("%02d/%02d/%4d", endTRef.GetMonth() + 1, endTRef.GetDay() + 1, endTRef.GetYear());
							string URL = string("feeds/historical-data/?date-range=") + DATE_RANGE_NAME[dateRange] + "&station-id=" + stationList[i].m_ID + "&from-date=" + start + "&to-date=" + end + "&report-type=hourly";
							
							string str;
							msg = UtilWWW::GetPageText(pConnection, URL, str);
							if (msg)
							{
								string error;
								Json json = Json::parse(str, error);

								if (error.empty() )
								{
									Json::array records = json["records"].array_items();
									if (!records.empty())
									{
										nbDownload++;
										CWeatherYears data(true);

										for (Json::array::const_iterator it = records.begin(); it != records.end() && msg; it++)
										{

											Json::array values = it->array_items();

											if (values.size() == NB_COLUMNS)
											{
												CTRef TRef = GetTRef(values[C_DATE_TIME].string_value());
												if (TRef.IsInit())
												{
													//try to load old data before changing it...
													if (!data.IsYearInit(TRef.GetYear()))
													{
														string filePath = GetOutputFilePath(stationList[i].m_ID, TRef.GetYear());
														data.LoadData(filePath, -999, false);//don't erase other years when multiple years
													}
													
													size_t c = 0;
													for (Json::array::const_iterator it2 = values.begin(); it2 != values.end() && msg; it2++, c++)
													{
														if (VARIABLES[c] != H_SKIP)
														{
															string v1 = it2->string_value();
															if (!v1.empty() && v1 != "--")
															{
																double v2 = ToDouble(v1);
																if (VARIABLES[c] == H_WNDD)
																{
																	v2 = CUIManitoba::GetWindDir(v1);
																	ASSERT(v2 != -999);
																}
																	
																if (IsValid(VARIABLES[c], v2))
																{
																	data[TRef].SetStat(VARIABLES[c], v2);

																	//compute Tdew
																	if (VARIABLES[c] == H_RELH && data[TRef][H_TAIR2].IsInit())
																		data[TRef].SetStat(H_TDEW, WBSF::Hr2Td(data[TRef][H_TAIR2][MEAN], v2));
																}
															}
														}
													}//for all values

													
												}//if TRef is init
											}//if it's the same number of columns
										}//for all records

										

										//save all years 
										for (auto it = data.begin(); it != data.end(); it++)
										{
											string filePath = GetOutputFilePath(stationList[i].m_ID, it->first);
											string outputPath = GetPath(filePath);
											CreateMultipleDir(outputPath);
											it->second->SaveData(filePath);
										}

									} //record not empty
								}
								else
								{
									msg.ajoute(error);
								}
							}//if msg
						}//if have data
					}
					else
					{
						msg.ajoute(error);
					}
				}//if get text

				msg += callback.StepIt();
			}//for all station


			pConnection->Close();
			pSession->Close();

		}//if connection

		//		string::size_type pos1 = str.find("<table");
		//		string::size_type pos2 = NOT_INIT;
		//		if (pos1 < str.size())
		//		{
		//			pos1 = str.find("<select", pos1);
		//			pos2 = str.find("</select>", pos1);
		//		}

		//		if (pos1 != string::npos && pos2 != string::npos)
		//		{
		//			try
		//			{
		//				string xml_str = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n" + str.substr(pos1, pos2 - pos1 + 9);
		//				zen::XmlDoc doc = zen::parse(xml_str);

		//				zen::XmlIn in(doc.root());
		//				for (zen::XmlIn it = in["option"]; it; it.next())
		//				{
		//					string value;
		//					it.get()->getAttribute("value", value);
		//					fileList.push_back(value);
		//				}//for all station
		//			}
		//			catch (const zen::XmlParsingError& e)
		//			{
		//				// handle error
		//				msg.ajoute("Error parsing XML file: col=" + WBSF::ToString(e.col) + ", row=" + WBSF::ToString(e.row));
		//			}
		//		}
		//	}


			

		//}

		callback.AddMessage(GetString(IDS_NB_FILES_DOWNLOADED) + ToString(nbDownload), 1);
		callback.PopTask();


		return msg;
	}


	ERMsg CUIWeatherFarm::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;
		
		CProvinceSelection provinces(Get(PROVINCE));
		msg = m_stations.Load(GetStationsListFilePath());

		if (msg)
			msg += m_stations.IsValid();

		for (size_t i = 0; i < m_stations.size(); i++)
		{
			string state = m_stations[i].GetSSI("Province");

			if (provinces.at(state))
				stationList.push_back(m_stations[i].m_ID);
		}
			

		return msg;
	}

	ERMsg CUIWeatherFarm::GetWeatherStation(const std::string& ID, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;
		
		//Get station information
		size_t it = m_stations.FindByID(ID);
		ASSERT(it != NOT_INIT);

		((CLocation&)station) = m_stations[it];


		int firstYear = WBSF::as<int>(Get("FirstYear"));
		int lastYear = WBSF::as<int>(Get("LastYear"));
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
	ERMsg CUIWeatherFarm::DownloadStationsList(CLocationVector& locations, CCallback& callback)
	{
		ERMsg msg;

		callback.PushTask("Update WeatherFarm stations list (" + ToString(17) + " pages)", 17);

		size_t nbDownload = 0;

		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;
		
		CInternetSessionPtr pGoogleSession;
		CHttpConnectionPtr pGoogleConnection;

		msg = GetHttpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, Get(MEMBER_NAME), Get(PASSWORD));
		if (msg)
			msg += GetHttpConnection("maps.googleapis.com", pGoogleConnection, pGoogleSession, PRE_CONFIG_INTERNET_ACCESS);

		if (msg)
		{
			
			//http://weatherfarm.com/feeds/get-current-weather-by-geo/?latMin=30&latMax=60&longMin=-110&longMax=-105&callback=station_data
			for (size_t x = 0; x < 17&&msg; x++)
			{
				static const char* FORMAT = "feeds/get-current-weather-by-geo/?latMin=%d&latMax=%d&longMin=%d&longMax=%d&callback=station_data";
				string format = FormatA(FORMAT, 30, 60, -135 + 5 * x, -135 + 5 * (x + 1));

				string str;
				msg = UtilWWW::GetPageText(pConnection, format, str);
				if (msg)
				{
					str = str.substr(13, str.size() - 14);

					string error;
					Json jsonStation = Json::parse(str, error);

					if (error.empty())
					{
						Json::object stations = jsonStation["locations"].object_items();

						for (Json::object::const_iterator it = stations.begin(); it != stations.end() && msg; it++)
						{
							Json::object metadata = it->second.object_items();

							CLocation location;
							if (it->first.front() == 'P')
							{
								location.m_ID = metadata["id"].string_value();
								location.m_name = WBSF::PurgeFileName(metadata["name"].string_value());
								location.m_lat = ToDouble(metadata["latitude"].string_value());
								location.m_lon = ToDouble(metadata["longitude"].string_value());
								location.SetSSI("Network", "WeatherFarm");
								location.SetSSI("City", metadata["city"].string_value());
								location.SetSSI("Province", metadata["province"].string_value());

								string elevFormat = "/maps/api/elevation/json?locations=" + metadata["latitude"].string_value() + "," + metadata["longitude"].string_value();
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
										ASSERT( jsonElev["results"].is_array() );
										Json::array result = jsonElev["results"].array_items();
										ASSERT(result.size() == 1);

										location.m_elev = result[0]["elevation"].number_value();
									}
								}//if msg

								locations.push_back(location);
							}//if it's a WeatherFarm station

							msg += callback.StepIt(0);
						}//for all stations


						msg += callback.StepIt();
						nbDownload++;
					}
					else
					{
						msg.ajoute(error);
					}
				}//if msg
			}//for all blocks
		}//if msg

		pConnection->Close();
		pSession->Close();


		callback.AddMessage(GetString(IDS_NB_FILES_DOWNLOADED) + ToString(nbDownload), 2);
		callback.PopTask();


		return msg;
	}

	CTRef CUIWeatherFarm::GetTRef(string str)
	{
		CTRef TRef;
		
		StringVector vec(str, "\\/ :");
		if (vec.size() == 5)
		{
			int year = WBSF::as<int>(vec[2]);
			size_t month = WBSF::as<size_t>(vec[0]) - 1;
			size_t day = WBSF::as<size_t>(vec[1]) - 1;
			size_t hour = WBSF::as<size_t>(vec[3]);
			size_t mm = WBSF::as<size_t>(vec[4]);

			ASSERT(month >= 0 && month < 12);
			ASSERT(day >= 0 && day < GetNbDayPerMonth(year, month));
			ASSERT(hour >= 0 && hour < 24);


			TRef = CTRef(year, month, day, hour);
			if (mm != 0)
			{
				if (mm >= 30)
					TRef++;
			}
		}


		return TRef;

	}

	bool CUIWeatherFarm::IsValid(TVarH v, double value)
	{
		bool bValid = true;
		switch (v)
		{
		case H_TMIN2:
		case H_TAIR2:
		case H_TMAX2:
		case H_TDEW: bValid = value >= -50 && value <= 50; break;
		case H_PRCP: bValid = value >= 0 && value < 300; break;
		case H_RELH: bValid = value > 0 && value <= 100; break;//ignore zero values
		case H_WNDS: bValid = value >= 0 && value <= 110; break;
		case H_WNDD: bValid = value >= 0 && value <= 360; break;
		}

		return bValid;
	}
}