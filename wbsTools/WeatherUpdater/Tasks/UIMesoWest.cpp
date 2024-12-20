#include "StdAfx.h"
#include "StdAfx.h"
#include "UIMesoWest.h"

#include <boost/algorithm/string.hpp>
#include <filesystem>
#include "basic/WeatherStation.h"
#include "basic/CSV.h"
#include "Basic/ExtractLocationInfo.h"
#include "Basic\json\json11.hpp"
#include "Basic/ExtractLocationInfo.h"
#include "Basic/Registry.h"
#include "Basic/CallcURL.h"
#include "Geomatic/TimeZones.h"
#include "Geomatic/ShapeFileBase.h"
#include "UI/Common/SYShowMessage.h"

#include "TaskFactory.h"
#include "../Resource.h"
#include "WeatherBasedSimulationString.h"
#include "StateSelection.h"
#include "ProvinceSelection.h"
#include "CountrySelection.h"

using namespace json11;

using namespace WBSF::HOURLY_DATA;
using namespace std;
using namespace UtilWWW;

namespace WBSF
{
	//MesoWest
	//newapi
	// https://api.synopticdata.com/v2/networks?&token={Your API Token}



	//Mesowest from NOAA
	//wrh.noaa.gov/mesowest/timeseries.php?sid=D5096&num=48
	//https://www.wrh.noaa.gov/mesowest/timeseries.php?sid=D5096&num=72&banner=NONE&units=METRIC
	//https://www.wrh.noaa.gov/mesowest/timeseries.php?sid=D5096&num=72&banner=NONE&units=METRIC


	//Get all network information
	//https://api.mesowest.net/v2/networks?token=635d9802c84047398d1392062e39c960
	//Get all station information
	//https://api.mesowest.net/v2/stations/metadata?complete=1&state=QC&token=635d9802c84047398d1392062e39c960
	//https://api.synopticdata.com/v2/stations/metadata?&token=635d9802c84047398d1392062e39c960&complete=1&sensorvars=1&output=json&status=active&country=ca
	//Get time series
	//https://api.synopticlabs.org/v2/stations/timeseries?stid=D5096&start=201701010000&end=201701012359&obtimezone=LOCAL&units=speed|kph,pres|mb&token=635d9802c84047398d1392062e39c960
	//get latest time series
	//https://api.mesowest.net/v2/stations/timeseries?stid=KASX&recent=4320&obtimezone=local&complete=1&hfmetars=0&units=speed|kph,pres|mb&token=d8c6aee36a994f90857925cea26934be
	//Get variable
	//https://api.synopticlabs.org/v2/variables?token=635d9802c84047398d1392062e39c960

	///get latest droman data
	//https://mesowest.utah.edu/cgi-bin/droman/meso_table_mesodyn.cgi?stn=3B19&unit=1&time=LOCAL&year1=&month1=&day1=0&hour1=00&hours=24&past=0&order=2


	//http://gl1.chpc.utah.edu/cgi-bin/droman/stn_state.cgi?state=QC&order=status
	//http://mesowest.utah.edu/cgi-bin/droman/raws_ca_monitor.cgi?state=QC&rawsflag=3
	//http://mesowest.utah.edu/cgi-bin/droman/download_api2.cgi?output=csv&product=&stn=CWZS&timetype=GMT&unit=1&yearcal=2018&monthcal=04
	//https://mesowest.utah.edu/cgi-bin/droman/download_api2_handler.cgi?output=csv&product=&stn=CWZS&unit=1&day1=23&month1=03&year1=2017&time=GMT&hour1=1&hours=1&daycalendar=0&yearcal=2018&monthcal=04
	//https://mesowest.utah.edu/cgi-bin/droman/download_api2_handler.cgi?output=csv&product=&stn=D5096&unit=1&day1=23&month1=03&year1=2017&time=GMT&hour1=1&hours=1&daycalendar=0&yearcal=2018&monthcal=04

	//http://mesowest.utah.edu/cgi-bin/droman/meso_station.cgi

	//https://www.wrh.noaa.gov/mesowest/getobextXml.php?sid=MTHIL&num=168&banner=NONE&units=METRIC
	//https://www.wrh.noaa.gov/mesowest/timeseries.php?sid=D5096&num=72&banner=NONE&units=METRIC

	//https://api.weather.gov/stations/D5096/observations
	//https://www.ncdc.noaa.gov/cdo-web/webservices/v2#stations
	//https://www.ncdc.noaa.gov/cdo-web/api/v2/data?datasetid=PRECIP_15&stationid=COOP:010008&units=metric&startdate=2010-05-01&enddate=2010-05-31

	//https://www.wrh.noaa.gov/mesowest/timeseries.php?sid=D5096&num=72&banner=NONE&units=METRIC
	//https://www.wrh.noaa.gov/mesowest/getobextXml.php?sid=AP244&num=168&&units=METRIC


	static const DWORD FLAGS = INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_PRAGMA_NOCACHE;
	const char* CUIMesoWest::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "Token", "APIType", "FirstYear", "LastYear", "Network", "States", "Provinces", "OtherCountries", "Subset_IDS", "ForceUpdateStationsList", "WithTemp", "UsePrcp" };
	const size_t CUIMesoWest::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_STRING, T_COMBO_INDEX, T_STRING, T_STRING, T_STRING_SELECT, T_STRING_SELECT, T_STRING_SELECT, T_STRING_SELECT, T_STRING, T_BOOL, T_BOOL, T_BOOL };
	const UINT CUIMesoWest::ATTRIBUTE_TITLE_ID = IDS_UPDATER_MESOWEST_P;
	const UINT CUIMesoWest::DESCRIPTION_TITLE_ID = ID_TASK_MESOWEST;

	const char* CUIMesoWest::CLASS_NAME() { static const char* THE_CLASS_NAME = "MesoWest";  return THE_CLASS_NAME; }
	CTaskBase::TType CUIMesoWest::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUIMesoWest::CLASS_NAME(), (createF)CUIMesoWest::create);
	//const char* CUIMesoWest::API_NAME[NB_API] = { "Current", "Historical" };


	const char* CUIMesoWest::SERVER_NAME = "api.mesowest.net";
	static const char* DEFAULT_VARS = "air_temp,precip_accum_since_local_midnight,dew_point_temperature,relative_humidity,wind_speed,wind_direction,pressure,snow_depth,solar_radiation,snow_accum,snow_depth,snow_water_equiv";


	CUIMesoWest::CUIMesoWest(void)
	{}

	CUIMesoWest::~CUIMesoWest(void)
	{}



	ERMsg CUIMesoWest::LoadNetwork(string file_path, NetworkMap& networks)
	{
		//open file
		ifStream file;
		ERMsg msg = file.open(file_path);
		ASSERT(msg);


		if (msg)
		{
			for (CSVIterator loop(file, ",", true); loop != CSVIterator(); ++loop)
			{
				array<string, NB_NETWORK_COLUMN> network;
				ASSERT(loop->size() == NB_NETWORK_COLUMN);
				for (size_t i = 0; i < loop->size() && i < NB_NETWORK_COLUMN; i++)
					network[i] = (*loop)[i];

				networks[network[NETWORK_SHORT_NAME]] = network;
			}

			file.close();
		}

		return msg;
	}

	std::string CUIMesoWest::Option(size_t i)const
	{
		if (m_networks.empty())
		{
			CUIMesoWest& me = const_cast<CUIMesoWest&>(*this);
			string file_path = GetApplicationPath() + "Layers\\MesoWest-Networks.csv";
			LoadNetwork(file_path, me.m_networks);
		}

		std::string all_networks;
		for (auto it = m_networks.begin(); it != m_networks.end(); it++)
		{
			const array<string, NB_NETWORK_COLUMN>& columns = it->second;
			all_networks += columns[NETWORK_SHORT_NAME] + "=" + columns[NETWORK_SHORT_NAME] + " (" + columns[NETWORK_NAME] + ")|";
		}

		string str;
		switch (i)
		{
		case API_TYPE: str = GetString(IDS_MESOWEST_API_TYPE); break;
		case NETWORKS: str = all_networks; break;
		case STATES:	str = CStateSelection::GetAllPossibleValue(); break;
		case PROVINCES:	str = CProvinceSelection::GetAllPossibleValue(); break;
		case OTHER_COUNTRIES:str = CCountrySelectionGADM::GetAllPossibleValue(true, true, "CAN|USA"); break;
		};
		return str;
	}

	std::string CUIMesoWest::Default(size_t i)const
	{
		string str;

		switch (i)
		{
		case WORKING_DIR: str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()) + "MesoWest\\"; break;
		case API_TYPE: str = "0"; break;
		case FIRST_YEAR:
		case LAST_YEAR:	str = ToString(CTRef::GetCurrentTRef().GetYear()); break;
		case NETWORKS: str = "AIRNOW|HADS|IFLOWS|RAWS|UCOOP|WEATHERSTEM";  break;
		case STATES: str = "AL|AZ|AR|CA|CO|CT|DE|FL|GA|ID|IL|IN|IA|KS|KY|LA|ME|MD|MA|MI|MN|MS|MO|MT|NE|NV|NH|NJ|NM|NY|NC|ND|OH|OK|OR|PA|RI|SC|SD|TN|TX|UT|VT|VA|WA|WV|WI|WY|AK"; break;
		case PROVINCES: str = "----"; break;
		case OTHER_COUNTRIES: str = "----"; break;
		case FORCE_UPDATE_STATIONS_LIST: str = "0"; break;
		case WITH_TEMP: str = "1"; break;
		case USE_PRCP: str = "1"; break;
		};

		return str;
	}


	std::string CUIMesoWest::GetStationListFilePath()const
	{
		string workingDir = GetDir(WORKING_DIR);
		return workingDir + "\\MesoWest-stations.csv";
	}



	std::string CUIMesoWest::GetOutputFilePath(std::string country, std::string state, const std::string& ID, int year)
	{

		string workingDir = GetDir(WORKING_DIR);
		if (country != "USA" && country != "CAN")
		{
			state = country;
			country = "--";
		}

		string ouputPath = workingDir + "\\" + country + "\\" + state + "\\" + ToString(year) + "\\" + ID + ".csv";


		return ouputPath;
	}

	//******************************************************
	ERMsg CUIMesoWest::Execute(CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);


		CreateMultipleDir(workingDir);
		size_t APItype = as<size_t>(API_TYPE);

		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(workingDir, 1);
		callback.AddMessage(GetString(IDS_UPDATE_FROM));
		callback.AddMessage(SERVER_NAME, 1);
		callback.AddMessage("");


		//load/update stations list
		bool bForceUpdateList = as<bool>(FORCE_UPDATE_STATIONS_LIST);
		if (!FileExists(GetStationListFilePath()) || bForceUpdateList)
		{
			msg += UpdateStationList(callback);
		}

		if (msg)
			msg = m_stations.Load(GetStationListFilePath(), ",", callback);

		if (!msg)
			return msg;


		if (APItype == LATEST)
			msg = ExecuteLatest(callback);
		else if (APItype == HISTORICAL)
			msg = ExecuteHistorical(callback);

		return msg;
	}


	static inline bool file_exists(const std::string& name) {
		struct stat buffer;
		return (stat(name.c_str(), &buffer) == 0);
	}

	StringVector CUIMesoWest::GetSubsetIds()
	{
		string subset_str = Get(SUBSET_ID);

		if (file_exists(subset_str))
		{
			CLocationVector locations;
			locations.Load(subset_str);

			subset_str.clear();
			for (auto it = locations.begin(); it != locations.end(); it++)
				subset_str += it->m_ID + "|";
		}

		return StringVector(subset_str, ",;| ");
	}

	template<class A, class B>
	ERMsg SaveMap(string file_path, const std::map<A, B>& map, const string& header, char sep = ':')
	{
		ERMsg msg;

		ofStream tfStream;
		msg = tfStream.open(file_path);
		if (msg)
		{
			tfStream << header << std::endl;
			for (std::map<A, B>::const_iterator it = map.begin(); it != map.end(); it++)
				tfStream << it->first << sep << it->second << std::endl;
			tfStream.close();
		}

		return msg;
	}

	template<class A, class B>
	ERMsg LoadMap(string file_path, std::map<A, B>& map, char sep = ':')
	{
		ERMsg msg;

		ifStream tfStream;
		msg = tfStream.open(file_path);
		if (msg)
		{
			string header;
			std::string key;
			std::string value;

			std::getline(tfStream, header);
			while (std::getline(tfStream, key, sep) && std::getline(tfStream, value))
			{
				Trim(key);
				if (!key.empty())
					map[key] = value;
			}

			tfStream.close();
		}

		return msg;
	}

	ERMsg CUIMesoWest::ExecuteLatest(CCallback& callback)
	{
		ERMsg msg;

		string networks_str = Get(NETWORKS);
		string token = Get(API_TOKEN);
		StringVector networks_select(networks_str, "|");
		CStateSelection states(Get(STATES));
		CProvinceSelection provinces(Get(PROVINCES));
		CCountrySelectionGADM countries(Get(OTHER_COUNTRIES));
		StringVector subsetIDS = GetSubsetIds();
		CTRef now = CTRef::GetCurrentTRef(CTM::HOURLY);

		if (m_networks.empty())
		{
			string file_path = GetApplicationPath() + "Layers\\MesoWest-Networks.csv";
			LoadNetwork(file_path, m_networks);
		}

		string networks_no;
		for (size_t n = 0; n < networks_select.size(); n++)
		{
			auto it = m_networks.find(networks_select[n]);

			if (it != m_networks.end())
			{
				networks_no += !networks_no.empty() ? "," : "";
				string networkID = it->second[NETWORK_ID];
				networks_no += networkID;
			}
			else
			{
				msg.ajoute("Invalid network ID" + networks_select[n]);
			}
		}

		if (!msg)
			return msg;

		vector<pair<string, string>> request;
		map<string, string> latest_execute;

		string latest_file_path = GetDir(WORKING_DIR) + "latest_execute.txt";
		LoadMap(latest_file_path, latest_execute);



		if (subsetIDS.empty())
		{
			if (provinces.any())
			{
				if (provinces.all())
				{
					request.push_back(make_pair("CA", ""));
				}
				else
				{
					for (size_t i = 0; i < provinces.size(); i++)
					{
						if (provinces[i])
						{
							request.push_back(make_pair("CA", provinces.GetName(i, CProvinceSelection::ABVR)));
							if (i == CProvinceSelection::NFLD)
								request.push_back(make_pair("CA", "NF"));//add also NF
							if (i == CProvinceSelection::YT)
								request.push_back(make_pair("CA", "YK"));//add also YK
						}
					}
				}
			}

			if (states.any())
			{
				if (states.all())
				{
					request.push_back(make_pair("US", ""));
				}
				else
				{
					for (size_t i = 0; i < states.size(); i++)
					{
						if (states[i])
							request.push_back(make_pair("US", states.GetName(i, CStateSelection::BY_ABVR)));
					}
				}
			}

			if (countries.any())
			{
				for (size_t i = 0; i < countries.size(); i++)
				{
					if (countries[i])
						request.push_back(make_pair(countries.GetName(i, CCountrySelectionGADM::BY_ID3), ""));
				}
			}
		}
		else
		{
			string IDs = to_string(subsetIDS, ",");
			request.push_back(make_pair("stid", IDs));
		}


		size_t nbDownload = 0;
		size_t SUs = 0;

		callback.PushTask("Download MesoWest data (" + ToString(request.size()) + " request)", request.size());
		callback.AddMessage("Number of MesoWest request: " + ToString(request.size()));


		size_t nbTry = 0;
		size_t cur_r = 0;

		while (cur_r < request.size() && msg)
		{
			nbTry++;
			CInternetSessionPtr pSession;
			CHttpConnectionPtr pConnection;
			msg += GetHttpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", false, 5, callback);

			if (msg)
			{
				try
				{

					pSession->SetOption(INTERNET_OPTION_RECEIVE_TIMEOUT, 60000);
					pSession->SetOption(INTERNET_OPTION_CONNECT_TIMEOUT, 60000);
					pSession->SetOption(INTERNET_OPTION_DATA_RECEIVE_TIMEOUT, 60000);


					static const char* URL_FORMAT =
						"v2/stations/timeseries?token=%s&precip=1&obtimezone=UTC&timeformat=%%25Y-%%25m-%%25dT%%25H:%%25M:%%25S%%20%%25Z&units=speed|kph,pres|mb,height|m&output=json&status=ACTIVE&fields=stid";
					string URL = FormatA(URL_FORMAT, token.c_str());



					if (request[cur_r].first == "stid")
					{
						URL += "&stid=" + request[cur_r].second;
					}
					else
					{
						string country = request[cur_r].first;
						string state = request[cur_r].second;

						URL += "&country=" + country;
						if (!state.empty())
							URL += "&state=" + state;

						if (!networks_no.empty())
							URL += "&network=" + networks_no;
					}
					URL += "&vars=" + string(DEFAULT_VARS);


					size_t nb_hour_needed = 24;//24 by default the first call
					string key = networks_str + "_" + request[cur_r].first + "_" + request[cur_r].second;
					if (latest_execute.find(key) != latest_execute.end())
					{

						CTRef last_update;
						last_update.FromString(latest_execute[key]);
						if (last_update.IsValid())
						{
							nb_hour_needed = max(0, now - last_update + 3);
							if (nb_hour_needed > 24 * 30)
							{
								callback.AddMessage("Warning: the number of hours needed (" + to_string(nb_hour_needed) + ") to update data exceed the maximum 720 (30*24) hours");
								nb_hour_needed = min(size_t(30 * 24), nb_hour_needed);
							}
						}
					}


					if (nb_hour_needed > 5)
					{
						URL += "&recent=" + to_string(nb_hour_needed * 60);


						static const DWORD FLAGS = INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_PRAGMA_NOCACHE;//| INTERNET_FLAG_TRANSFER_BINARY

						string source;
						msg = GetPageText(pConnection, URL, source, false, FLAGS);
						ASSERT(!source.empty());

						if (msg && !source.empty() && source[0] == '{')
						{
							msg += MergeJsonData(key, source, nbDownload, SUs, callback);
							msg += callback.StepIt();

							//save state after each request
							latest_execute[key] = CTRef::GetCurrentTRef(CTM::HOURLY).ToString();
							SaveMap(latest_file_path, latest_execute, "Request,Time");

							cur_r++;
							nbTry = 0;
						}
						else
						{
							if (!msg)
								msg.ajoute("Server return: " + source.substr(0, 100) + "...");

							if (nbTry < 5)
							{
								callback.AddMessage(msg);
								msg = WaitServer(120, callback);//wait 2 minutes before resending a request
							}
						}
					}
					else
					{
						callback.AddMessage(key + " is up to date. knipped");
						cur_r++;
					}




				}//try
				catch (CException* e)
				{
					if (nbTry < 5)
					{
						callback.AddMessage(UtilWin::SYGetMessage(*e));
						msg += WaitServer(120, callback);
					}
					else
					{
						msg = UtilWin::SYGetMessage(*e);
					}
				}

				pConnection->Close();
				pSession->Close();
			}//if msg
		}//for all request

		callback.AddMessage("Nb stations update: " + ToString(nbDownload));
		callback.AddMessage("Service Units: " + to_string(SUs) + " (" + to_string(SUs*0.00000015) + "$)");

		callback.PopTask();

		return msg;

	}


	ERMsg CUIMesoWest::ExecuteHistorical(CCallback& callback)
	{
		ERMsg msg;

		//Get stations list
		CLocationVector stationList = GetStationList(callback);

		string token = Get(API_TOKEN);
		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = lastYear - firstYear + 1;
		CTRef now = CTRef::GetCurrentTRef(CTM::HOURLY);

		size_t nbDownload = 0;
		set<size_t> nbStations;
		callback.PushTask("Download MesoWest stations data (" + ToString(stationList.size()) + " stations years)", stationList.size()*nbYears);
		callback.AddMessage("Number of MesoWest files to download: " + ToString(stationList.size()*nbYears));
		size_t SUs = 0;

		size_t nbTry = 0;
		size_t cur_i = 0;

		while (cur_i < stationList.size() && msg)
		{
			nbTry++;

			CInternetSessionPtr pSession;
			CHttpConnectionPtr pConnection;
			msg += GetHttpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", false, 5, callback);

			if (msg)
			{
				pSession->SetOption(INTERNET_OPTION_RECEIVE_TIMEOUT, 45000);

				try
				{
					while (cur_i < stationList.size() && msg)
					{
						for (size_t y = 0; y < nbYears&&msg; y++)
						{
							int year = firstYear + int(y);

							string country = stationList[cur_i].GetSSI("Country");
							string subDivisions = stationList[cur_i].GetSSI("SubDivision");
							string ID = stationList[cur_i].m_ID;
							string ouputFilePath = GetOutputFilePath(country, subDivisions, ID, year);
							CreateMultipleDir(GetPath(ouputFilePath));

							CTPeriod p = GetActualState(ouputFilePath);

							if (year < now.GetYear())
							{
								if (p.IsInit())
								{
									p = CTPeriod(p.End(), CTRef(year, LAST_MONTH, DAY_31, LAST_HOUR));
								}
								else
								{
									p = CTPeriod(CTRef(year, JANUARY, FIRST_DAY, FIRST_HOUR), CTRef(year, DECEMBER, DAY_31, LAST_HOUR));
								}
							}
							else
							{
								if (p.IsInit())
								{
									p = CTPeriod(p.End(), now - 1);
								}
								else
								{
									p = CTPeriod(CTRef(year, JANUARY, FIRST_DAY, FIRST_HOUR), now - 1);
								}
							}


							if (p.size() > 2)//call only if more than one hour
							{
								string time_zone = stationList[cur_i].GetSSI("TimeZone");
								ASSERT(time_zone.length() == 5);
								int time_shift = ToInt(time_zone.substr(0, 3));

								//convert local to UTC (aproximiative)
								p.Begin() -= time_shift + 1;
								p.End() -= time_shift - 1;


								//static const char* URL_FORMAT = "v2/stations/timeseries?token=%s&stid=%s&precip=1&start=%4d%02d%02d%02d00&end=%4d%02d%02d%02d59&obtimezone=UTC&timeformat=%%25Y-%%25m-%%25dT%%25H:%%25M:%%25S%%20%%25Z&units=speed|kph,pres|mb,height|m&fields=stid&output=csv&vars=%s";
								static const char* URL_FORMAT = "v2/stations/timeseries?token=%s&stid=%s&precip=1&start=%4d%02d%02d%02d00&end=%4d%02d%02d%02d59&obtimezone=UTC&timeformat=%%25Y-%%25m-%%25dT%%25H:%%25M:%%25S%%20%%25Z&units=speed|kph,pres|mb,height|m&fields=stid&output=json&vars=%s";
								string URL = FormatA(URL_FORMAT, token.c_str(), stationList[cur_i].m_ID.c_str(), p.Begin().GetYear(), p.Begin().GetMonth() + 1, p.Begin().GetDay() + 1, p.Begin().GetHour(), p.End().GetYear(), p.End().GetMonth() + 1, p.End().GetDay() + 1, p.End().GetHour(), DEFAULT_VARS);

								if (p.size() < 7 * 24)
									URL += "&recent=" + to_string(p.size() * 60);//recent in minutes


								static const DWORD FLAGS = INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_PRAGMA_NOCACHE;//| INTERNET_FLAG_TRANSFER_BINARY

								string source;
								msg = GetPageText(pConnection, URL, source, false, FLAGS);
								ASSERT(!source.empty());

								if (msg && !source.empty())
								{
									if (msg && !source.empty() && source[0] == '{')
									{
										nbStations.insert(cur_i);
										msg += MergeJsonData("", source, nbDownload, SUs, callback);
										if (msg)
										{
											msg += callback.StepIt();
											nbTry = 0;
											nbDownload++;
										}
									}
									else
									{
										string error = "Server return: " + source.substr(0, 100) + "...";
										if (nbTry < 5)
										{
											callback.AddMessage(error);
											msg = WaitServer(120, callback);//wait 2 minutes before resending a request
										}
										else
										{
											msg.ajoute(error);
										}

									}
								}

							}

						}//for all years

						cur_i++;
					}//for all stations
				}//try
				catch (CException* e)
				{
					if (nbTry < 5)
					{
						callback.AddMessage(UtilWin::SYGetMessage(*e));
						msg += WaitServer(120, callback);
					}
					else
					{
						msg = UtilWin::SYGetMessage(*e);
					}
				}

				pConnection->Close();
				pSession->Close();

			}//if msg
		}//while

		callback.AddMessage("Nb stations update: " + ToString(nbStations.size()));
		callback.AddMessage(GetString(IDS_NB_FILES_DOWNLOADED) + ToString(nbDownload));
		callback.AddMessage("Service Units: " + to_string(SUs) + " (" + to_string(SUs*0.00000015) + "$)");
		callback.PopTask();


		return msg;
	}


	CLocationVector CUIMesoWest::GetStationList(CCallback& callback)
	{
		ERMsg msg;

		CLocationVector stationList;

		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		CTRef now = CTRef::GetCurrentTRef(CTM::HOURLY);
		CTPeriod downloadPeriod(CTRef(firstYear, JANUARY, DAY_01), CTRef(lastYear, DECEMBER, DAY_31));

		StringVector networks_select(Get(NETWORKS), "|");
		CStateSelection states(Get(STATES));
		CProvinceSelection provinces(Get(PROVINCES));
		CCountrySelectionGADM countries(Get(OTHER_COUNTRIES));
		StringVector subsetIDS = GetSubsetIds();
		bool bWithTemp = as<bool>(WITH_TEMP);


		stationList.reserve(m_stations.size());
		for (auto it = m_stations.begin(); it < m_stations.end() && msg; it++)
		{
			if (it->m_ID.find('?') == string::npos)//don't take station with '?'
			{
				string country = it->GetSSI("Country");
				string subDivisions = it->GetSSI("SubDivision");
				string network = it->GetSSI("Network");
				string start = it->GetSSI("Start");
				string end = it->GetSSI("End");
				string vars = it->GetSSI("Variables");
				string exclude = it->GetSSI("Excluded");

				if (!start.empty() && exclude != "1")
				{
					//empty start period seem to don't have data 
					CTRef startTRef = GetTRef(start);
					CTRef endTRef = !end.empty() ? GetTRef(end) : now;

					CTPeriod activePeriod(startTRef, endTRef);
					activePeriod.Transform(CTM::DAILY);

					bool IsIncludeI = networks_select.empty() || networks_select.Find(network, false, true) != string::npos;
					bool IsIncludeII = !bWithTemp || vars.find("air_temp") != string::npos;
					bool IsIncludeIII = (it->GetSSI("Status") == "ACTIVE") || (downloadPeriod.Begin().GetYear() < now.GetYear() - 1);//if active or historical data
					bool IsIncludeIV = false;

					if (subsetIDS.empty())
					{
						if (country == "USA")
							IsIncludeIV = states.at(subDivisions);
						else if (country == "CAN")
							IsIncludeIV = provinces.at(subDivisions);
						else
							IsIncludeIV = countries.at(country);
					}
					else
					{
						if (subsetIDS.Find(it->m_ID, false, true) != NOT_INIT)
							IsIncludeIV = true;
					}

					if (activePeriod.IsIntersect(downloadPeriod) &&
						IsIncludeI && IsIncludeII && IsIncludeIII && IsIncludeIV)
					{
						stationList.push_back(*it);
					}
				}
			}

			msg += callback.StepIt(0);
		}//for all stations



		return stationList;
	}

	ERMsg CUIMesoWest::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;

		msg = m_stations.Load(GetStationListFilePath(), ",", callback);

		if (msg)
		{
			CLocationVector stations = GetStationList(callback);
			for (CLocationVector::const_iterator it = stations.begin(); it != stations.end(); it++)
				stationList.push_back(it->m_ID);
		}//if msg

		return msg;
	}


	ERMsg CUIMesoWest::GetWeatherStation(const string& ID, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;

		size_t pos = m_stations.FindPosByID(ID);
		ASSERT(pos != NOT_INIT);

		((CLocation&)station) = m_stations[pos];

		station.m_name = WBSF::UppercaseFirstLetter(WBSF::PurgeFileName(station.m_name));

		bool bUsePrcp = as<bool>(USE_PRCP);
		CTRef current = CTRef::GetCurrentTRef();
		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = size_t(lastYear - firstYear + 1);

		//now extract data 
		for (size_t y = 0; y < nbYears&&msg; y++)
		{
			int year = firstYear + int(y);

			string filePath = GetOutputFilePath(station.GetSSI("Country"), station.GetSSI("SubDivision"), ID, year);
			if (FileExists(filePath))
			{
				station.LoadData(filePath, -999, false);

			}
		}


		if (!bUsePrcp)
		{
			CWVariables variables = CWAllVariables();
			variables.reset(H_PRCP);
			station.CleanUnusedVariable(variables);
		}

		//string network = station.GetSSI("Network");
		//string country = station.GetSSI("Country");
		//string subDivisions = station.GetSSI("SubDivision");

		//station.m_siteSpeceficInformation.clear();
		//station.SetSSI("Network", network);
		//station.SetSSI("Country", country);
		//station.SetSSI("SubDivision", subDivisions);
		station.SetSSI("Provider", "MesoWest");

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


	//*************************************************************************************************************************************


	CTRef CUIMesoWest::GetTRef(const string& str)
	{
		StringVector e(str, "-T:+Z");
		ASSERT(e.size() == 6 || e.size() == 7);


		int year = WBSF::as<int>(e[0]);
		size_t m = WBSF::as<size_t>(e[1]) - 1;
		size_t d = WBSF::as<size_t>(e[2]) - 1;
		size_t hh = WBSF::as<size_t>(e[3]);
		size_t mm = WBSF::as<size_t>(e[4]);
		size_t ss = WBSF::as<size_t>(e[5]);

		ASSERT(m >= 0 && m < 12);
		ASSERT(d >= 0 && d < GetNbDayPerMonth(year, m));
		ASSERT(hh >= 0 && hh < 24);
		ASSERT(mm >= 0 && mm < 60);


		return CTRef(year, m, d, hh);

	}

	bool CUIMesoWest::IsValid(TVarH v, double value)
	{
		bool bValid = true;
		switch (v)
		{
		case H_TMIN:
		case H_TAIR:
		case H_TMAX:
		case H_TDEW: bValid = value >= -70 && value <= 50; break;
		case H_PRCP: bValid = value >= 0 && value < 300; break;
		case H_RELH: bValid = value > 0 && value <= 100; break;//ignore zero values
		case H_WNDS: bValid = value >= 0 && value <= 110; break;
		case H_WNDD: bValid = value >= 0 && value <= 360; break;
		}

		return bValid;
	}


	ERMsg CUIMesoWest::ReadJSONData(const string& filePath, CTM TM, int year, CWeatherAccumulator& accumulator, CWeatherStation& data, CCallback& callback)const
	{
		ERMsg msg;

		//now extact data 
		ifStream file;

		msg = file.open(filePath);

		if (msg)
		{
			string error;
			std::vector<Json>& items = Json::parse_multi(file.GetText(), error);

			if (error.empty())
			{
				ASSERT(items.size() == 1);//JSON root object

				const Json& root = items.front();
				ASSERT(root["STATION"].type() == Json::ARRAY);

				const Json::array& stations = root["STATION"].array_items();

				if (stations.size() == 1)
				{
					double last_prcp = 0;
					CTRef last_prcp_ref;



					const Json& obs = stations[0]["OBSERVATIONS"];
					ASSERT(obs.type() == Json::OBJECT);
					ASSERT(obs["date_time"].type() == Json::ARRAY);

					const Json::array& date_time = obs["date_time"].array_items();
					vector<CTRef> TRefs(date_time.size());
					for (size_t i = 0; i < date_time.size(); i++)
					{
						string dateTime = date_time[i].string_value();
						TRefs[i] = GetTRef(dateTime);
					}

					const Json::object& values = obs.object_items();

					StringVector header;
					for (Json::object::const_iterator it = values.begin(); it != values.end(); it++)
						header.push_back(it->first);

					std::vector<HOURLY_DATA::TVarH > variables = GetVariables(header);

					for (size_t i = 0; i < TRefs.size() && msg; i++)
					{

						if (accumulator.GetTRef().IsInit() && accumulator.TRefIsChanging(TRefs[i]))
							data[accumulator.GetTRef()].SetData(accumulator);


						if (TRefs[i].GetYear() == year)
						{
							for (size_t v = 0; v < header.size() && msg; v++)
							{
								if (variables[v] != H_SKIP)
								{
									ASSERT(obs[header[v]].type() == Json::ARRAY);

									const Json::array& varValues = obs[header[v]].array_items();
									ASSERT(TRefs.size() == varValues.size());

									if (!varValues[i].is_null())
									{
										double value = varValues[i].number_value();
										if (IsValid(variables[v], value))
										{
											if (variables[v] == H_PRCP)
											{
												//accumulated precipitation since local midnight
												//need to remove last observation
												if (last_prcp_ref.as(CTM::DAILY) != TRefs[i].as(CTM::DAILY))
													last_prcp = 0;//reset on a new day

												double diff = value - last_prcp;
												ASSERT(diff >= 0);

												accumulator.Add(TRefs[i], variables[v], diff);

												last_prcp_ref = TRefs[i];
												last_prcp = value;
											}
											else
											{
												accumulator.Add(TRefs[i], variables[v], value);
											}
										}//if valid
									}
								}//if its a varialbe

								msg += callback.StepIt(0);
							}//for all object
						}//if good year
					}//for all TRef
				}//if have one station
			}//if no error
			else
			{
				msg.ajoute(error);
				msg.ajoute("JSON error : " + filePath);
			}
		}//if msg

		return msg;
	}



	static string PurgeName(string str)
	{
		string::size_type pos = str.find_last_of(",");

		str = str.substr(0, pos - 1);
		ReplaceString(str, ",", "");
		ReplaceString(str, ";", "");
		ReplaceString(str, "\"", "");
		ReplaceString(str, ".", "");
		ReplaceString(str, "?", "");

		Trim(str);

		return WBSF::UppercaseFirstLetter(WBSF::PurgeFileName(str));
	}

	ERMsg CUIMesoWest::UpdateNetwork(NetworkMap& networks, CCallback& callback)const
	{
		ERMsg msg;

		string token = Get(API_TOKEN);
		string URL = "https://api.mesowest.net/v2/networks?token=" + token;
		string argument = "-s -k \"" + URL + "\"";
		string exe = GetApplicationPath() + "External\\curl.exe";
		CCallcURL cURL(exe);

		string source;
		msg = cURL.get_text(argument, source);

		if (msg)
		{


			string command = exe + " " + argument;

			string error;
			const Json& root = Json::parse(source, error);
			if (error.empty())
			{
				ASSERT(root["MNET"].type() == Json::ARRAY);
				const std::vector<Json>& nets = root["MNET"].array_items();

				static const char* CATEGORY_NAME[13] = { "Agricultural", "Air Quality", "Offshore", "Federal and state networks", "Hydrological", "State and Local", "NWS/FAA", "CWOP", "Fire weather", "Road and rail weather", "Public Utility", "Research and Education", "Commercial" };

				for (Json::array::const_iterator it = nets.begin(); it != nets.end() && msg; it++)
				{
					const json11::Json& item = *it;
					string ID = TrimConst(item["ID"].string_value());
					if (!ID.empty())
					{

						array<string, NB_NETWORK_COLUMN> network;
						network[NETWORK_ID] = ID;
						network[NETWORK_NAME] = TrimConst(item["LONGNAME"].string_value());
						network[NETWORK_SHORT_NAME] = TrimConst(item["SHORTNAME"].string_value());
						size_t cat = atoi(item["CATEGORY"].string_value().c_str()) - 1;
						network[NETWORK_TYPE] = cat < 13 ? CATEGORY_NAME[cat] : "Unknowns";
						network[NETWORK_ACTIVE_STATIONS] = TrimConst(item["ACTIVE_STATIONS"].dump());

						//remove coma to avoid problem in csv file
						for (size_t i = 0; i < network.size(); i++)
							ReplaceString(network[i], ",", "");

						networks[ID] = network;
					}
				}
			}




			string file_path = GetDir(WORKING_DIR) + "MesoWest-Networks.csv";
			//save network to current directory
			ofStream file;
			msg = file.open(file_path);
			if (msg)
			{
				file << "NetworkID,Name,Short Name,Type,Stations Reporting" << endl;

				for (auto it = networks.begin(); it != networks.end(); it++)
				{
					string line;
					for (size_t i = 0; i < it->second.size(); i++)
					{
						line += !line.empty() ? "," : "";
						line += it->second[i];
					}
					file << line << endl;
				}


				file.close();
			}
		}

		return msg;
	}

	ERMsg CUIMesoWest::UpdateStationList(CCallback& callback)const
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);


		string token = Get(API_TOKEN);
		CTRef current = CTRef::GetCurrentTRef();

		callback.PushTask("Download stations list for MesoWest", -1);
		string URL = "https://api.mesowest.net/v2/stations/metadata?complete=1&sensorvars=1&vars=" + string(DEFAULT_VARS) + "&token=" + token;
		string json_filepath = workingDir + "MesoWest-stations.json";

		string argument = "-s -k \"" + URL + "\" --output \"" + json_filepath + "\"";
		string exe = GetApplicationPath() + "External\\curl.exe";
		string command = exe + " " + argument;

		DWORD exit_code;
		msg = WinExecWait(command, "", SW_HIDE, &exit_code);
		callback.PopTask();



		if (exit_code == 0 && FileExists(json_filepath))
		{
			string csv_filepath = workingDir + "MesoWest-stations.csv";
			msg = convert_json_to_csv(json_filepath, csv_filepath, callback);
		}

		return msg;
	}

	ERMsg CUIMesoWest::convert_json_to_csv(const std::string& jsonFilePath, const std::string& csvFilePath, CCallback& callback)const
	{
		ERMsg msg;

		NetworkMap networks;
		msg = UpdateNetwork(networks, callback);
		if (!msg)
			return msg;


		string workingDir = GetDir(WORKING_DIR);


		string path = GetPath(jsonFilePath);
		std::string correction_file_path = path + "MesoWest-corrections.csv";
		CLocationVector correction;
		correction.Load(correction_file_path);


		CLocationVector old_locations;
		old_locations.Load(csvFilePath);

		std::string invalid_coord_file_path = path + "MesoWest-invalid-stations.csv";
		CLocationVector old_invalid_Stations;
		old_invalid_Stations.Load(invalid_coord_file_path);

		ofStream invalid;
		msg += invalid.open(invalid_coord_file_path);

		CRegistry registry;
		std::string GADM_file_path = registry.GetString(CRegistry::GetGeoRegistryKey(L_WORLD_GADM));

		CShapeFileBase shapefile;
		bool bGADM = shapefile.Read(GADM_file_path);
		if (!bGADM)
			callback.AddMessage("WARNING: unable to load correctly GADM file: " + GADM_file_path);


		std::locale utf8_locale = std::locale(std::locale::classic(), new std::codecvt_utf8<char>());

		ifStream file;
		file.imbue(utf8_locale);
		msg += file.open(jsonFilePath);
		if (msg)
		{
			string source = file.GetText();
			file.close();

			//parse station list
			string error;
			const Json& root = Json::parse(source, error);
			if (error.empty())
			{
				CLocationVector locations;
				locations.reserve(70000);

				ASSERT(root["STATION"].type() == Json::ARRAY);
				const std::vector<Json>& stations = root["STATION"].array_items();

				callback.PushTask("Extract country and administrative sub-division (MesoWest)", stations.size());
				invalid << "ID,Name,Latitude,Longitude,Elevation,Country1,SubDivision1,Country2,SubDivision2,Distance(km),Comment" << endl;


				for (Json::array::const_iterator it = stations.begin(); it != stations.end() && msg; it++)
				{
					CLocation location = LocationFromJson(it);
					//make a manual correction if any
					size_t corr_pos = correction.FindPosByID(location.m_ID);
					bool bManualCorrection = corr_pos != NOT_INIT;
					if (bManualCorrection)
					{
						location = correction[corr_pos];
					}

					bool bExclude = location.GetSSI("Excluded") == "1";
					if (!bExclude)
					{
						if (location.IsValid(true))
						{
							size_t invalid_pos = old_invalid_Stations.FindPosByID(location.m_ID);
							bool bInvalid = invalid_pos != NOT_INIT;
							bool bNeedExtration = true;


							//find the old station information if any
							size_t oldPos = old_locations.FindPosByID(location.m_ID);
							if (!bInvalid && !bManualCorrection && oldPos != NOT_INIT)
							{
								bool bDiffLat = fabs(location.m_lat - old_locations[oldPos].m_lat) > 0.001;
								bool bDiffLon = fabs(location.m_lon - old_locations[oldPos].m_lon) > 0.001;
								bool bDiffAlt = old_locations[oldPos].m_alt == -999 || (location.m_alt != -999 && fabs(location.m_alt - old_locations[oldPos].m_alt) > 1);
								bNeedExtration = bDiffLat || bDiffLon || bDiffAlt;

								if (!bNeedExtration)
									location = old_locations[oldPos];
							}


							if (bNeedExtration && bGADM)
							{
								string country = location.GetSSI("Country");
								string subDivision = location.GetSSI("SubDivision");
								string countryII = "--";
								string subDivisionII = "--";
								double d = GetCountrySubDivision(shapefile, location.m_lat, location.m_lon, country, subDivision, countryII, subDivisionII);

								if (countryII == "--")
								{
									invalid << location.m_ID << "," << location.m_name << "," << ToString(location.m_lat, 4) << "," << ToString(location.m_lon, 4) << "," << to_string(location.m_alt) << "," << country << "," << subDivision << "," << countryII << "," << subDivisionII << "," << to_string(Round(d / 1000, 1)) << "," << "UnknownCountry" << endl;
									country = "UNK";//Unknown
								}
								else
								{
									bool bBuoy = location.m_name.find("Buoy") != string::npos || location.m_name.find("Platform") != string::npos;

									if (!country.empty() && country != countryII && d > 20000 && !bBuoy)
										invalid << location.m_ID << "," << location.m_name << "," << ToString(location.m_lat, 4) << "," << ToString(location.m_lon, 4) << "," << to_string(location.m_alt) << "," << country << "," << subDivision << "," << countryII << "," << subDivisionII << "," << to_string(Round(d / 1000, 1)) << "," << "MissmatchCountry" << endl;
									
									if (!country.empty() && country == countryII && d > 20000 && !bBuoy)
										invalid << location.m_ID << "," << location.m_name << "," << ToString(location.m_lat, 4) << "," << ToString(location.m_lon, 4) << "," << to_string(location.m_alt) << "," << country << "," << subDivision << "," << countryII << "," << subDivisionII << "," << to_string(Round(d / 1000, 1)) << "," << "BadCountryID" << endl;


									country = countryII;

									if (!subDivision.empty() && subDivisionII != "--" && subDivisionII != subDivision && d > 20000)
										invalid << location.m_ID << "," << location.m_name << "," << ToString(location.m_lat, 4) << "," << ToString(location.m_lon, 4) << "," << to_string(location.m_alt) << "," << country << "," << subDivision << "," << countryII << "," << subDivisionII << "," << to_string(Round(d / 1000, 1)) << "," << "MissmatchSubDivision" << endl;

									subDivision = subDivisionII;
								}


								if (location.GetSSI("Network").empty() && networks.find(location.GetSSI("networkID")) != networks.end())
									location.SetSSI("Network", networks.at(location.GetSSI("networkID"))[NETWORK_SHORT_NAME]);


								location.SetSSI("Country", country);
								location.SetSSI("SubDivision", subDivision);
								location.SetSSI("d", to_string(Round(d / 1000, 1)));
								location.SetSSI("ElevType", (location.m_alt == -999) ? "SRTM" : "MESO");
							}

							locations.push_back(location);

						}
						else//invalid lat/lon
						{
							string country = location.GetSSI("Country");
							string subDivision = location.GetSSI("SubDivision");
							invalid << location.m_ID << "," << location.m_name << "," << ToString(location.m_lat, 4) << "," << ToString(location.m_lon, 4) << "," << to_string(location.m_alt) + "," << country << "," << subDivision << "," << "," << "," << "," << "BadLocation" << endl;
						}
					}

					msg += callback.StepIt();
				}//for all stations

				if (bGADM)
					shapefile.Close();

				invalid.close();
				file.close();


				callback.AddMessage("Number of stations: " + to_string(locations.size()));
				callback.PopTask();

				if (msg)
				{
					//extraxt missing elevations
					ASSERT(locations.IsValid(true));
					//if missing elevation, extract elevation at 30 meters: not support < -60� and > 60�
					if (!locations.IsValid(false))
						msg = locations.ExtractOpenTopoDataElevation(false, COpenTopoDataElevation::NASA_SRTM30M, COpenTopoDataElevation::I_BILINEAR, callback);

					//if still missing elevation, extract elevation at 30 meters ASTER
					if (msg && !locations.IsValid(false))
						msg = locations.ExtractOpenTopoDataElevation(false, COpenTopoDataElevation::NASA_ASTER30M, COpenTopoDataElevation::I_BILINEAR, callback);

					if (/*msg && */!locations.IsValid(false))
					{
						for (CLocationVector::iterator it = locations.begin(); it != locations.end(); )
						{
							if (!it->IsValid(false))
							{
								callback.AddMessage("WARNING: invalid coordinate :" + it->m_name + "(" + it->m_ID + "), " + "lat=" + to_string(it->m_lat) + ", lon=" + to_string(it->m_lon) + ", elev=" + to_string(it->m_alt), 2);
								it = locations.erase(it);
							}
							else
							{
								it++;
							}
						}
					}

					//save event if extract to fail

					msg += locations.Save(csvFilePath);

					set<string> unknow_country;
					for (CLocationVector::iterator it = locations.begin(); it != locations.end(); it++)
					{
						size_t country = CCountrySelectionGADM::GetCountry(it->GetSSI("Country"));

						if (country == -1)
							unknow_country.insert(it->GetSSI("Country"));
					}

					for (auto it = unknow_country.begin(); it != unknow_country.end(); it++)
					{
						size_t c = CCountrySelectionGADM::GetCountry(*it);
						string name = c != NOT_INIT ? CCountrySelectionGADM::GetName(c, 1) : "";
						callback.AddMessage("WARNING: unknown country: " + *it + "," + name);
					}
				}
			}//if error
			else
			{
				msg.ajoute(error);
			}
		}//if msg

		return msg;
	}

	CLocation CUIMesoWest::LocationFromJson(const Json::array::const_iterator& it)const
	{

		CLocation location;

		location.m_name = PurgeName((*it)["NAME"].string_value());
		location.m_ID = (*it)["STID"].string_value();
		if (!(*it)["LATITUDE"].is_null())
			location.m_lat = WBSF::as<double>((*it)["LATITUDE"].string_value());
		if (!(*it)["LONGITUDE"].is_null())
			location.m_lon = WBSF::as<double>((*it)["LONGITUDE"].string_value());
		if (!(*it)["ELEVATION"].is_null())
			location.m_alt = WBSF::Feet2Meter(WBSF::as<double>((*it)["ELEVATION"].string_value()));

		if (location.m_lat == 0 && location.m_lon == 0)
			location.m_lat = location.m_lon = -999;


		//remove ID from the name
		string ID = location.m_ID;
		ID.insert(1, 1, 'w');
		ReplaceString(location.m_name, ID, "");
		Trim(location.m_name);

		string country = (*it)["COUNTRY"].string_value();
		string subDivisions = (*it)["STATE"].string_value();
		string time_zone = (*it)["TIMEZONE"].string_value();
		string id = (*it)["ID"].string_value();
		string status = (*it)["STATUS"].string_value();
		string network = (*it)["SHORTNAME"].string_value();
		string networkID = (*it)["MNET_ID"].string_value();
		string start = (*it)["PERIOD_OF_RECORD"]["start"].string_value();
		string end = (*it)["PERIOD_OF_RECORD"]["end"].string_value();
		CTRef TRef = !end.empty() ? GetTRef(end) : CTRef();
		string str = (*it)["SENSOR_VARIABLES"].string_value();

		country = CCountrySelectionGADM::NOAA_to_GADM(country);
		size_t c_ID = CCountrySelectionGADM::GetCountry(country);//by ID
		if (c_ID != NOT_INIT)
			country = CCountrySelectionGADM::m_default_list[c_ID][0];


		if (country.empty())
		{
			if (CProvinceSelection::GetProvince(subDivisions) != UNKNOWN_POS)
				country = "CAN";
			else if (CStateSelection::GetState(subDivisions) != UNKNOWN_POS)
				country = "USA";
			//else
				//country = "UNK";//Unknown
		}

		if (country == "CAN")
		{
			if (subDivisions == "NF")
				subDivisions = "NL";
			else if (subDivisions == "YK")
				subDivisions = "YT";
			else if (subDivisions == "BC`")
				subDivisions = "BC";
			else if (subDivisions == "PQ")
				subDivisions = "QC";
		}

		string vars_str;
		if ((*it)["SENSOR_VARIABLES"].is_object())
		{
			auto vars = (*it)["SENSOR_VARIABLES"].object_items();

			for (auto it_vars = vars.begin(); it_vars != vars.end(); it_vars++)
			{
				if (!vars_str.empty())
					vars_str += "|";

				vars_str += it_vars->first;
			}
		}
		//if the station is still active or seem to be still active, we don't set end date 
		if (status == "ACTIVE" || TRef.GetYear() >= CTRef::GetCurrentTRef().GetYear() - 1)
			end.clear();


		//if (!DEM_alt_str.empty())
		//{
		//	if (DEM_alt != -999 && fabs(location.m_alt - DEM_alt) > 350)
		//	{
		//		location.m_name += " (suspicious elev)";
		//		//location.m_alt = DEM_alt;
		//	}
		//}


		string time_zone_Z;
		__int64 time_zone_shift = CTimeZones::GetTimeZone(location);
		if (time_zone_shift == NOT_INIT)
		{
			time_zone_shift = __int64(location.m_lon / 15 * 3600);
		}

		double tmp = time_zone_shift / 3600.0f;
		int HH = int(tmp);
		int MM = int((tmp - HH) * 60);
		time_zone_Z = FormatA("%+03d%02d", HH, MM);



		//location.SetSSI("ELEV_DEM", DEM_alt_str);
		location.SetSSI("Status", status);
		location.SetSSI("Country", country);
		location.SetSSI("SubDivision", subDivisions);
		location.SetSSI("Network", network);
		location.SetSSI("NetworkID", networkID);
		location.SetSSI("TimeZoneName", time_zone);
		location.SetSSI("TimeZone", time_zone_Z);
		location.SetSSI("Variables", vars_str);
		//location.SetSSI("Excluded", exclude);
		location.SetSSI("Start", start);
		location.SetSSI("End", end);
		//location.SetSSI("ElevType", (location.m_alt == -999) ? "SRTM" : "MESO");

		return location;
	}

	double CUIMesoWest::GetCountrySubDivision(CShapeFileBase& shapefile, double lat, double lon, std::string countryI, std::string subDivisionI, std::string& countryII, std::string& subDivisionII)
	{
		double d = -1;
		countryII = "--";
		subDivisionII.clear();


		const CDBF3& DBF = shapefile.GetDBF();

		int FindexH = DBF.GetFieldIndex("HASC_1");
		ASSERT(FindexH >= 0 && FindexH < DBF.GetNbField());
		int FindexGID = DBF.GetFieldIndex("GID_0");
		ASSERT(FindexGID >= 0 && FindexGID < DBF.GetNbField());

		CGeoPoint pt(lon, lat, PRJ_WGS_84);
		int shapeNo = -1;

		bool bInShape = shapefile.IsInside(pt, &shapeNo);
		if (bInShape)//inside a shape
		{
			countryII = TrimConst(DBF[shapeNo][FindexGID].GetElement());
			StringVector tmp(DBF[shapeNo][FindexH].GetElement(), ".");

			if (tmp.size() >= 2)
				subDivisionII = TrimConst(tmp[1]);

			d = 0;
		}
		else
		{
			d = shapefile.GetMinimumDistance(pt, &shapeNo);
			ASSERT(shapeNo >= 0 && shapeNo < DBF.GetNbRecord());

			countryII = TrimConst(DBF[shapeNo][FindexGID].GetElement());

			StringVector tmp(DBF[shapeNo][FindexH].GetElement(), ".");

			if (tmp.size() >= 2)
				subDivisionII = TrimConst(tmp[1]);

		}


		if (countryI.length() == 3)
		{
			if (countryII != countryI ||
				(!subDivisionI.empty() && subDivisionII != subDivisionI))
			{

				double min_d = 1e20;
				for (int i = 0; i < DBF.GetNbRecord(); i++)
				{

					StringVector tmp(DBF[i][FindexH].GetElement(), ".");
					string country = TrimConst(DBF[i][FindexGID].GetElement());

					string subDivision;
					if (tmp.size() >= 2)
						subDivision = TrimConst(tmp[1]);

					if (countryI == country &&
						(subDivisionI.empty() || subDivisionI == subDivision))
					{
						double dd = shapefile[i].GetShape().GetMinimumDistance(pt);
						if (dd < min_d)
						{
							d = dd;
							min_d = dd;
							countryII = country;
							subDivisionII = subDivision;
							/*if (min_d < 20000)
							{
								countryII = countryI;
								subDivisionII = subDivisionI;
							}*/
						}
					}
				}
			}
		}

		if (countryII.length() < 2)
			countryII = "--";

		if (subDivisionII.length() != 2)
			subDivisionII.clear();


		return d;
	}

	//Variable	Description	English Units	Metric Units
	//T10M	Air Temperature at 10 meters	� F	� C
	//TMPF	Temperature	� F	� C
	//T2M	Air Temperature at 2 meters	� F	� C
	//MTMP	Temperature	� F	� C
	//HI24	24 Hr High Temperature	� F	� C
	//LO24	24 Hr Low Temperature	� F	� C

	//P01I	Precipitation 1hr	 in	 cm
	//P01M	Precipitation 1hr manual	 in	 cm
	//P24I	Precipitation 24hr	 in	 cm
	//P24M	Precipitation 24hr manual	 in	 cm
	//PREC	Precipitation accumulated	 in	 cm
	//PREM	Precipitation manual	 in	 cm

	//DWPF	Dewpoint	� F	� C
	//MDWP	Dew Point	� F	� C
	//MRH	Relative Humidity	%	%
	//RELH	Relative Humidity	%	%

	//MSKT	Wind Speed	 mph	 m/s
	//SKNT	Wind Speed	 mph	 m/s
	//DRCT	Wind Direction	�	�
	//MDIR	Wind Direction	�	�
	//PDIR	Peak Wind Direction	�	�
	//PEAK	Peak Wind Speed	 mph	 m/s

	//CSLR	Clear Sky Solar Radiation	 W/m*m	 W/m*m
	//INLW	Incoming Longwave Radiation	 W/m*m	 W/m*m
	//NETL	Net Longwave Radiation	 W/m*m	 W/m*m
	//NETR	Net Radiation	 W/m*m	 W/m*m
	//NETS	Net Shortwave Radiation	 W/m*m	 W/m*m
	//OUTL	Outgoing Longwave Radiation	 W/m*m	 W/m*m
	//OUTS	Outgoing Shortwave Radiation	 W/m*m	 W/m*m
	//SOLR	Solar Radiation	 W/m*m	 W/m*m

	//ALTI	Altimeter	 in	 mb
	//MPRS	Pressure	 in � F	 mb
	//PMSL	Sea level pressure	 in	 mb
	//PRES	Pressure	 in	 mb

	//SNOM	Snow manual	 in	 cm
	//SNOW	Snow depth	 in	 cm
	//SSTM	Snowfall	 in	 cm
	//WEQS	Snow water equivalent	 in	 cm

	//MSO2	Soil Moisture 2	%	%
	//MSOI	Soil Moisture	%	%
	//VSBY	Visibility	 miles	 km

	//enum Tcols { NB_COLS = 13 };
	//static const char* COL_NAME[NB_COLS] = { "", "air_temp","", "precip_accum", "dew_point_temperature","relative_humidity","wind_speed","wind_direction","solar_radiation","pressure","snow_accum", "snow_depth", "snow_water_equiv" };
	//enum TPrcp { PRECIP_ACCUM_SINCE_LOCAL_MIDNIGHT, PRECIP_ACCUM_ONE_HOUR, NB_PRCP_TYPE };
	//static const char* PRCP_TYPE_NAME[NB_PRCP_TYPE] = { "precip_accum_since_local_midnight", "precip_accum_one_hour" };


	TVarH CUIMesoWest::GetVar(const string& str)
	{
		TVarH v = H_SKIP;

		string::size_type pos = str.find("_set_");
		if (pos != string::npos)
		{
			string tmp = str.substr(0, pos);
			//enum { LO24, T2M, HI24, PMID, P1H, MDWP, RELH, MSKT, MDIR, SOLR, PRES, SNOW, SNDP, WEQS, NB_VARS };
			enum { LO24, T2M, HI24, PMID, MDWP, RELH, MSKT, MDIR, SOLR, PRES, SNOW, SNDP, WEQS, NB_VARS };

			static const char* VAR_NAME[NB_VARS] = { "air_temp_high_24_hour", "air_temp", "air_temp_low_24_hour", "precip_intervals",/*"precip_accum_since_local_midnight","precip_accum_one_hour",*/ "dew_point_temperature", "relative_humidity", "wind_speed", "wind_direction", "solar_radiation", "pressure","snow_accum", "snow_depth", "snow_water_equivalent" };
			static const TVarH VAR_TYPE[NB_VARS] = { H_TMIN, H_TAIR, H_TMAX, H_PRCP, H_TDEW, H_RELH, H_WNDS, H_WNDD, H_SRAD, H_PRES, H_SNOW, H_SNDH, H_SWE };

			for (size_t vv = 0; vv < NB_VARS&&v == H_SKIP; vv++)
			{
				if (IsEqual(tmp, VAR_NAME[vv]))
					v = VAR_TYPE[vv];
			}

		}


		return v;
	}

	bool CUIMesoWest::IsUnknownVar(const string& str)
	{
		bool bUnknown = false;

		string::size_type pos = str.find("_set_");
		if (pos != string::npos)
		{
			bUnknown = true;

			string tmp = str.substr(pos);

			static const char* VAR_NAME_UNUSED[] = { "altimeter", "pressure_tendency", "pressure_change_code", "wind_gust", "peak_wind_speed", "peak_wind_direction", "wind_cardinal_direction", "visibility", "heat_index", "weather_condition", "metar_remark", "metar", "wind_chill" };
			static const size_t NB_UNUSEDS = sizeof(VAR_NAME_UNUSED) / sizeof(char*);

			for (size_t v = 0; v < NB_UNUSEDS&&bUnknown; v++)
			{
				if (IsEqual(tmp, VAR_NAME_UNUSED[v]))
					bUnknown = false;
			}
		}

		return bUnknown;
	}


	std::vector<HOURLY_DATA::TVarH > CUIMesoWest::GetVariables(const StringVector& header)
	{
		std::vector<HOURLY_DATA::TVarH > variables(header.size());

		for (size_t v = 0; v < header.size(); v++)
			variables[v] = GetVar(header[v]);


		ASSERT(variables.size() == header.size());

		return variables;
	}



	CTPeriod CUIMesoWest::GetActualState(const string& filePath/*, double& last_hms*/)
	{
		CTPeriod p;
		//	last_hms = 0;

		ifStream file;
		if (file.open(filePath, ifStream::in | ifStream::binary))
		{
			string firstLine;

			std::getline(file, firstLine);//read header
			std::getline(file, firstLine);//first line

			StringVector columns1(firstLine, ",");
			if (columns1.size() >= 4)
			{
				int year = atoi(columns1[0].c_str());
				size_t m = atoi(columns1[1].c_str()) - 1;
				size_t d = atoi(columns1[2].c_str()) - 1;
				size_t h = atoi(columns1[3].c_str());

				ASSERT(year >= 2000 && year <= 2100);
				ASSERT(m >= 0 && m < 12);
				ASSERT(d >= 0 && d < GetNbDayPerMonth(year, m));
				ASSERT(h >= 0 && h < 24);
				//ASSERT(mm >= 0 && mm < 60);
				//ASSERT(ss >= 0 && ss < 60);

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
					if (columns2.size() >= 4)
					{
						int year = atoi(columns2[0].c_str());
						size_t m = atoi(columns2[1].c_str()) - 1;
						size_t d = atoi(columns2[2].c_str()) - 1;
						size_t h = atoi(columns2[3].c_str());

						ASSERT(year >= 2000 && year <= 2100);
						ASSERT(m >= 0 && m < 12);
						ASSERT(d >= 0 && d < GetNbDayPerMonth(year, m));
						ASSERT(h >= 0 && h < 24);

						CTRef TRef2 = CTRef(year, m, d, h);
						if (TRef2.IsValid())
						{
							p = CTPeriod(TRef1, TRef2);
						}//TRef2 valid
					}//column2 >=4
				}//TRef1 valid
			}//column1 >=4

			file.close();
		}

		return p;
	}

	ERMsg CUIMesoWest::MergeData(const string& country, const string& state, const string& ID, const std::string& source, size_t& SUs, CCallback& callback)
	{
		ERMsg msg;


		ASSERT(!ID.empty());
		size_t pos_station = m_stations.FindPosByID(ID);
		ASSERT(pos_station != NOT_INIT);

		string time_zone = m_stations[pos_station].GetSSI("TimeZone");
		ASSERT(time_zone.length() == 5);
		int time_shift = ToInt(time_zone.substr(0, 3));


		CWeatherYears data(true);
		CWeatherAccumulator accumulator(CTM::HOURLY);
		double last_prcp = 0;


		std::stringstream stream(source);
		//remove all lines begginning with #
		streampos pos = stream.tellg();
		string head;
		std::getline(stream, head);
		while (!head.empty() && head[0] == '#')
		{
			pos = stream.tellg();
			std::getline(stream, head);
		}
		stream.seekg(pos);




		enum Tcols { NB_COLS = 13 };
		static const char* COL_NAME[NB_COLS] = { "", "air_temp","", "precip_accum", "dew_point_temperature","relative_humidity","wind_speed","wind_direction","solar_radiation","pressure","snow_accum", "snow_depth", "snow_water_equiv" };
		enum TPrcp { PRECIP_ACCUM_SINCE_LOCAL_MIDNIGHT, PRECIP_ACCUM_ONE_HOUR, NB_PRCP_TYPE };
		static const char* PRCP_TYPE_NAME[NB_PRCP_TYPE] = { "precip_accum_since_local_midnight", "precip_accum_one_hour" };

		std::vector<TVarH> variables;
		//std::vector<size_t> prcp_type;
		for (CSVIterator loop(stream); loop != CSVIterator() && msg; ++loop)
		{
			if (variables.empty())
			{
				if (loop.Header().size() >= 2)
				{
					variables.resize(loop.Header().size(), H_SKIP);
					//prcp_type.resize(loop.Header().size(), NOT_INIT);
					ASSERT(loop.Header()[0] == "Station_ID");
					ASSERT(loop.Header()[1] == "Date_Time");
					for (size_t cc = 2; cc < loop.Header().size(); cc++)
					{
						string var_str = loop.Header()[cc];
						for (size_t c = 0; c < NB_COLS&&variables[cc] == H_SKIP; c++)
						{
							if (strlen(COL_NAME[c]) > 0 && var_str.find(COL_NAME[c]) == 0)
							{

								if ((TVarH)c == H_PRCP)
								{
									//type of precipitation

									for (size_t p = 0; p < NB_PRCP_TYPE; p++)
									{
										if (var_str.find(PRCP_TYPE_NAME[p]) == 0)
										{
											//prcp_type[cc] = p;
											variables[cc] = (TVarH)c;
										}
									}
								}
								else
								{
									variables[cc] = (TVarH)c;
								}
							}
						}
					}
				}
				else
				{
					msg.ajoute("MesoWest: invalid station data");
				}

				if (variables.empty())//no data available for this stations
					break;

				++loop;//skip units
			}





			if (msg && loop != CSVIterator() && (*loop).size() >= 2)
			{
				string dateTimeStr = (*loop)[1];
				//StringVector dateTime(dateTimeStr, " -:T");
				if (dateTimeStr.size() == 23)
				{
					//2019-09-25T20:45:00-0400
					int year = atoi(dateTimeStr.substr(0, 4).c_str());
					size_t m = atoi(dateTimeStr.substr(5, 2).c_str()) - 1;
					size_t d = atoi(dateTimeStr.substr(8, 2).c_str()) - 1;
					size_t h = atoi(dateTimeStr.substr(11, 2).c_str());
					size_t mm = atoi(dateTimeStr.substr(14, 2).c_str());
					size_t ss = atoi(dateTimeStr.substr(17, 2).c_str());
					//float zone = atoi(dateTimeStr.substr(19, 5).c_str())/100.0f;

					//some time zone are in number and not letter
					//string zone = dateTimeStr.substr(20, 3);
					//ASSERT(atoi(zone.c_str()) != 0 || (zone.length() == 3 && (zone[1] == 'S' || zone[1] == 'D')));

					//ASSERT(year >= firstYear - 1 && year <= lastYear);
					ASSERT(m >= 0 && m < 12);
					ASSERT(d >= 0 && d < GetNbDayPerMonth(year, m));
					ASSERT(h >= 0 && h < 24);
					ASSERT(mm >= 0 && mm < 60);
					ASSERT(ss >= 0 && ss < 60);


					CTRef TRef = CTRef(year, m, d, h);
					//if (zone.length() == 3 && zone[1] == 'D')//transform daylight onto standard
						//TRef--;

					if (TRef.IsValid())
					{
						//convert UTC to locale
						TRef += time_shift;

						if (accumulator.TRefIsChanging(TRef))
						{
							if (!data.IsYearInit(accumulator.GetTRef().GetYear()))
							{
								//try to load old data before changing it...
								string filePath = GetOutputFilePath(country, state, ID, accumulator.GetTRef().GetYear());
								data.LoadData(filePath, -999, false);//don't erase other years when multiple years
							}

							data[accumulator.GetTRef()].SetData(accumulator);
							last_prcp = 0;
						}

						//if (TRef.as(CTM::DAILY) != last_prcp_TRef)
						//if (TRef.as(CTM::DAILY) != last_prcp_TRef)
						//{//reset each day
						//	last_prcp_TRef = TRef.as(CTM::DAILY);
						//	last_prcp = 0;
						//}


						for (size_t cc = 2; cc < loop->size() && msg; cc++)
						{
							TVarH v = variables[cc];
							if (v != H_SKIP)
							{
								string str = TrimConst((*loop)[cc]);
								if (!str.empty() && str != "M")
								{
									double value = atof(str.c_str());
									double base = (v == H_PRCP) ? last_prcp : 0;

									if (IsValid(v, value - base))
									{
										accumulator.Add(TRef, v, value - base);
										if (v == H_PRCP)
											last_prcp = value;
									}
									//}
									//	case H_PRCP: if ((value - last_prcp) >= 0) accumulator.Add(TRef, H_PRCP, value - last_prcp); last_prcp = value;  break;
									//	case H_TDEW: if (value > -60 && value < 50)accumulator.Add(TRef, H_TDEW, value); break;
									//	case H_RELH: if (value > 0 && value <= 100)accumulator.Add(TRef, H_RELH, value); break;
									//	case H_WNDS: if (value >= 0) accumulator.Add(TRef, H_WNDS, value); break;
									//	case H_WNDD: if (value >= 0 && value <= 360) accumulator.Add(TRef, H_WNDD, value); break;
									//	case H_SRAD: accumulator.Add(TRef, H_SRAD, value); break;
									//	case H_PRES: accumulator.Add(TRef, H_PRES, value); break;
									//	case H_SNOW: accumulator.Add(TRef, H_SNOW, value); break;
									//	case H_SNDH: accumulator.Add(TRef, H_SNDH, value / 10.0); break;
									//	case H_SWE: accumulator.Add(TRef, H_SWE, value); break;
									//	}//switch variables
									//}
								}//not empty
							}//v != skip

							msg += callback.StepIt(0);
						}//all cols
					}//if valid TRef
				}//date time have 24 characters
			}//if have data
		}//for all line
		//do not save last hours (can be incomplete)
		//if (accumulator.TRefIsChanging(accumulator.GetTRef()))
			//data[accumulator.GetTRef()].SetData(accumulator);

	//save all years: save empty file to avoid download it again
		if (data.HaveData())
		{
			for (auto it = data.begin(); it != data.end(); it++)
			{
				string filePath = GetOutputFilePath(country, state, ID, it->first);
				string outputPath = GetPath(filePath);
				CreateMultipleDir(outputPath);
				it->second->SaveData(filePath);
			}
		}
		else
		{
			callback.AddMessage(ID + " don't have data");
		}

		return msg;
	}



	ERMsg CUIMesoWest::MergeJsonData(std::string request, const std::string& source, size_t& nbDownload, size_t& SUs, CCallback& callback)
	{
		ERMsg msg;

		ReplaceString(request, "|", " ");

		string error;
		std::vector<Json>& items = Json::parse_multi(source, error);

		if (error.empty())
		{
			ASSERT(items.size() == 1);//JSON root object

			const Json& root = items.front();

			ASSERT(root["SUMMARY"].type() == Json::OBJECT);
			ASSERT(root["SUMMARY"]["RESPONSE_CODE"].type() == Json::NUMBER);

			if (root["SUMMARY"]["RESPONSE_CODE"].int_value() == 1)
			{
				ASSERT(root["STATION"].type() == Json::ARRAY);

				const Json::array& stations = root["STATION"].array_items();

				if (!request.empty())
				{
					callback.PushTask("Split MesoWest data for " + request + ": " + ToString(stations.size()) + " stations", stations.size());
					callback.AddMessage("Split MesoWest data for " + request + ": " + ToString(stations.size()) + " stations");
				}


				for (size_t s = 0; s < stations.size(); s++)
				{
					ASSERT(stations[s]["STID"].is_string());
					string ID = stations[s]["STID"].string_value();
					size_t pos = m_stations.FindPosByID(ID);

					if (!ID.empty() && pos != NOT_INIT)
					{
						string country = m_stations[pos].GetSSI("Country");
						string subDivisions = m_stations[pos].GetSSI("SubDivision");
						string time_zone = m_stations[pos].GetSSI("TimeZone");
						ASSERT(time_zone.length() == 5);
						int time_shift = ToInt(time_zone.substr(0, 3));

						CWeatherYears data(true);
						CWeatherAccumulator accumulator(CTM::HOURLY);
						//double last_prcp = 0;

						const Json& obs = stations[s]["OBSERVATIONS"];
						ASSERT(obs.type() == Json::OBJECT);
						ASSERT(obs["date_time"].type() == Json::ARRAY);

						const Json::array& date_time = obs["date_time"].array_items();
						vector<CTRef> TRefs(date_time.size());
						for (size_t i = 0; i < date_time.size(); i++)
						{
							string dateTime = date_time[i].string_value();
							TRefs[i] = GetTRef(dateTime);

							//convert UTC to locale
							TRefs[i] += time_shift;
						}

						const Json::object& values = obs.object_items();

						//get variable name
						StringVector header;
						for (Json::object::const_iterator it = values.begin(); it != values.end(); it++)
							header.push_back(it->first);

						//get BioSIM variable from variable name
						std::vector<HOURLY_DATA::TVarH > variables = GetVariables(header);

						//for all times
						for (size_t i = 0; i < TRefs.size() && msg; i++)
						{
							if (accumulator.GetTRef().IsInit() &&
								accumulator.TRefIsChanging(TRefs[i]))
							{
								if (!data.IsYearInit(accumulator.GetTRef().GetYear()))
								{
									//try to load old data before changing it...
									string filePath = GetOutputFilePath(country, subDivisions, ID, accumulator.GetTRef().GetYear());
									data.LoadData(filePath, -999, false);//don't erase other years when multiple years
								}

								data[accumulator.GetTRef()].SetData(accumulator);
								//last_prcp = 0;
							}

							for (size_t v = 0; v < header.size() && msg; v++)
							{
								if (variables[v] != H_SKIP)
								{
									ASSERT(obs[header[v]].type() == Json::ARRAY);

									const Json::array& varValues = obs[header[v]].array_items();
									ASSERT(TRefs.size() == varValues.size());

									if (!varValues[i].is_null())
									{
										double value = varValues[i].number_value();
										//double base = 0;// (variables[v] == H_PRCP) ? last_prcp : 0;

										//here precipitation is tranformed into 
										if (IsValid(variables[v], value))
										{
											accumulator.Add(TRefs[i], variables[v], value);
											//if (variables[v] == H_PRCP)
												//last_prcp = value;
										}

										SUs++;
									}//if its a varialbe

									msg += callback.StepIt(0);
								}//valid var
							}//for all variables
							msg += callback.StepIt(0);
						}//for all TRef


				//do not save last hours (can be incomplete)
				//if (accumulator.TRefIsChanging(accumulator.GetTRef()))
					//data[accumulator.GetTRef()].SetData(accumulator);

			//save all years: save empty file to avoid download it again
						if (data.HaveData())
						{
							for (auto it = data.begin(); it != data.end(); it++)
							{
								string filePath = GetOutputFilePath(country, subDivisions, ID, it->first);
								string outputPath = GetPath(filePath);
								CreateMultipleDir(outputPath);
								msg = it->second->SaveData(filePath);
							}

							nbDownload++;
						}
						else
						{
							//callback.AddMessage(ID + " don't have data");
						}

						if (!request.empty())
							msg += callback.StepIt();
					}
					else
					{
						callback.AddMessage("Station metadata no found:" + ID);
					}
				}//for all stations

				if (!request.empty())
					callback.PopTask();

			}
			else
			{
				if (root["SUMMARY"]["RESPONSE_CODE"].int_value() != 2)//no data found
				{
					string error = root["SUMMARY"]["RESPONSE_MESSAGE"].string_value();
					msg.ajoute(error);
				}
			}
		}//if no error
		else
		{
			msg.ajoute(error);
		}


		return msg;



	}

}




