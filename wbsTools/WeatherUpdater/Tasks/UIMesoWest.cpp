#include "StdAfx.h"
#include "UIMesoWest.h"

#include <boost/algorithm/string.hpp>
#include "basic/WeatherStation.h"
#include "basic/CSV.h"
#include "UI/Common/SYShowMessage.h"
#include "json\json11.hpp"

#include "TaskFactory.h"
#include "../Resource.h"
#include "WeatherBasedSimulationString.h"
#include "StateSelection.h"
#include "ProvinceSelection.h"
//#include "Geomatic/TimeZones.h"
//#include "cctz/time_zone.h"
using namespace json11;

using namespace WBSF::HOURLY_DATA;
using namespace std;
using namespace UtilWWW;

namespace WBSF
{
	//MesoWest

	//Get all network information
	//https://api.mesowest.net/v2/networks?token=635d9802c84047398d1392062e39c960
	//Get all station information
	//https://api.mesowest.net/v2/stations/metadata?complete=1&state=QC&token=635d9802c84047398d1392062e39c960
	//Get time series
	//https://api.synopticlabs.org/v2/stations/timeseries?stid=D5096&start=201701010000&end=201701012359&obtimezone=LOCAL&units=speed|kph,pres|mb&token=635d9802c84047398d1392062e39c960
	//Get varaible
	//https://api.synopticlabs.org/v2/variables?token=635d9802c84047398d1392062e39c960



	//http://gl1.chpc.utah.edu/cgi-bin/droman/stn_state.cgi?state=QC&order=status
	//http://mesowest.utah.edu/cgi-bin/droman/raws_ca_monitor.cgi?state=QC&rawsflag=3
	//http://mesowest.utah.edu/cgi-bin/droman/download_api2.cgi?stn=CWZS&year1=2017&day1=23&month1=3&hour1=1&timetype=GMT&unit=0
	//http://mesowest.utah.edu/cgi-bin/droman/meso_station.cgi

	static const DWORD FLAGS = INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_PRAGMA_NOCACHE;
	const char* CUIMesoWest::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "FirstYear", "LastYear", "States", "Provinces", "AddOthers", "IgnoreCommonStations", "ForceUpdateStationsList" };
	const size_t CUIMesoWest::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_STRING, T_STRING, T_STRING_SELECT, T_STRING_SELECT, T_BOOL, T_BOOL, T_BOOL };
	const UINT CUIMesoWest::ATTRIBUTE_TITLE_ID = IDS_UPDATER_MESOWEST_P;
	const UINT CUIMesoWest::DESCRIPTION_TITLE_ID = ID_TASK_MESOWEST;

	const char* CUIMesoWest::CLASS_NAME(){ static const char* THE_CLASS_NAME = "MesoWest";  return THE_CLASS_NAME; }
	CTaskBase::TType CUIMesoWest::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUIMesoWest::CLASS_NAME(), (createF)CUIMesoWest::create);


	const char* CUIMesoWest::SERVER_NAME = "api.mesowest.net";

	CUIMesoWest::CUIMesoWest(void)
	{}

	CUIMesoWest::~CUIMesoWest(void)
	{}


	std::string CUIMesoWest::Option(size_t i)const
	{
		string str;
		switch (i)
		{
		case STATES:	str = CStateSelection::GetAllPossibleValue(); break;
		case PROVINCES:	str = CProvinceSelection::GetAllPossibleValue(); break;
		};
		return str;
	}

	std::string CUIMesoWest::Default(size_t i)const
	{
		string str;

		switch (i)
		{
		case WORKING_DIR: str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()) + "MesoWest\\"; break;
		case FIRST_YEAR:
		case LAST_YEAR:	str = ToString(CTRef::GetCurrentTRef().GetYear()); break;
		case STATES: str = "----"; break;
		case ADD_OTHER: str = "0"; break;
		case IGNORE_COMMON_STATIONS: str = "1"; break;
		case FORCE_UPDATE_STATIONS_LIST: str = "0"; break;
		};

		return str;
	}

	std::string CUIMesoWest::GetStationListFilePath()const
	{
		string workingDir = GetDir(WORKING_DIR);
		return workingDir + "StationsList.csv";
	}


	std::string CUIMesoWest::GetOutputFilePath(const std::string& country, const std::string& states, const std::string& ID, int year, size_t m)
	{
		string workingDir = GetDir(WORKING_DIR);
		string ouputPath = workingDir + country + "\\" + states + "\\" + ToString(year) + "\\" + FormatA("%02d", m + 1) + "\\" + ID + ".Json";

		return ouputPath;
	}

	//******************************************************
	ERMsg CUIMesoWest::Execute(CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);
		CreateMultipleDir(workingDir);

		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(workingDir, 1);
		callback.AddMessage(GetString(IDS_UPDATE_FROM));
		callback.AddMessage(SERVER_NAME, 1);
		callback.AddMessage("");


		//Get station
		bool bAddOter = as<bool>(ADD_OTHER);
		bool bIgnoreCommonStations = as <bool>(IGNORE_COMMON_STATIONS);
		bool bForceUpdateList = as<bool>(FORCE_UPDATE_STATIONS_LIST);

		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = lastYear - firstYear + 1;
		CTRef current = CTRef::GetCurrentTRef();


		CStateSelection states(Get(STATES));
		CProvinceSelection provinces(Get(PROVINCES));


		CLocationVector stationListTmp;

		if (!FileExists(GetStationListFilePath()) || bForceUpdateList)
		{

			msg = DownloadStationList(stationListTmp, callback);
			if (msg)
				msg = stationListTmp.Save(GetStationListFilePath(), ',', callback);
		}
		else
		{
			msg = stationListTmp.Load(GetStationListFilePath(), ",", callback);
		}

		if (!msg)
			return msg;




		//clean common stations

		CLocationVector stationList;
		stationList.reserve(stationListTmp.size());
		for (auto it = stationListTmp.begin(); it < stationListTmp.end() && msg; it++)
		{

			if (it->m_ID.find('?') == string::npos)//don't take station with '?'
			{
				string country = it->GetSSI("Country");
				string state = it->GetSSI("State");
				string network = it->GetSSI("Network");


				if ((country == "US" && states.at(state)) ||
					(country == "CA" && provinces.at(state) ||
					(country == "--" && bAddOter)))
				{
					bool bAdd = true;
					if (bIgnoreCommonStations && IsCommonStation(network))
						bAdd = false;

					if (bAdd)
						stationList.push_back(*it);
				}
			}

			msg += callback.StepIt(0);
		}//for all stations

		size_t nbM = (lastYear < current.GetYear()) ? nbYears * 12 : (nbYears - 1) * 12 + current.GetMonth() + 1;
		callback.PushTask("Download stations data (" + ToString(stationList.size()) + " stations)", stationList.size()*nbM);

		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;
		msg += GetHttpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS);

		if (!msg)
			return msg;


		int nbDownload = 0;
		for (size_t i = 0; i < stationList.size() && msg; i++)
		{
			for (size_t y = 0; y < nbYears&&msg; y++)
			{
				int year = firstYear + int(y);

				size_t nbMonths = (year < current.GetYear()) ? 12 : current.GetMonth() + 1;
				for (size_t m = 0; m < nbMonths&&msg; m++)
				{
					static const char* URL_FORMAT = "v2/stations/timeseries?stids=%s&start=%4d%02d010000&end=%4d%02d%02d2359&obtimezone=LOCAL&units=speed|kph,pres|mb&token=635d9802c84047398d1392062e39c960";
					string URL = FormatA(URL_FORMAT, stationList[i].m_ID.c_str(), year, m + 1, year, m + 1, GetNbDayPerMonth(year, m));
					string ouputFilePath = GetOutputFilePath(stationList[i].GetSSI("Country"), stationList[i].GetSSI("State"), stationList[i].m_ID, year, m);
					CreateMultipleDir(GetPath(ouputFilePath));



					CTimeRef TRef1(GetFileInfo(ouputFilePath).m_time);

					if (!FileExists(ouputFilePath) || TRef1 - CTRef(year, m, LAST_DAY) < 5)
					{
						msg += CopyFile(pConnection, URL, ouputFilePath);
						if (msg && WBSF::GetFileInfo(ouputFilePath).m_size > 1000)
							nbDownload++;
					}

					msg += callback.StepIt();
				}
			}
		}

		pConnection->Close();
		pSession->Close();


		callback.AddMessage(GetString(IDS_NB_FILES_DOWNLOADED) + ToString(nbDownload), 2);
		callback.PopTask();


		return msg;
	}




	ERMsg CUIMesoWest::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;


		msg = m_stations.Load(GetStationListFilePath(), ",", callback);

		if (msg)
		{
			bool bIgnoreCommonStations = as <bool>(IGNORE_COMMON_STATIONS);
			bool bAddOter = as<bool>(ADD_OTHER);
			CStateSelection states(Get(STATES));
			CProvinceSelection provinces(Get(PROVINCES));

			for (CLocationVector::const_iterator it = m_stations.begin(); it != m_stations.end(); it++)
			{
				if (it->m_ID.find('?') == string::npos)//don't take station with '?'
				{

					string country = it->GetSSI("Country");
					string state = it->GetSSI("State");
					string network = it->GetSSI("Network");

					if ((country == "US" && states.at(state)) ||
						(country == "CA" && provinces.at(state) ||
						(country == "--" && bAddOter)))
					{
						bool bAdd = true;
						if (bIgnoreCommonStations && IsCommonStation(network))
							bAdd = false;

						if (bAdd)
							stationList.push_back(it->m_ID);
					}

				}
			}
		}

		return msg;
	}


	ERMsg CUIMesoWest::GetWeatherStation(const string& ID, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;

		size_t pos = m_stations.FindByID(ID);
		ASSERT(pos != NOT_INIT);

		((CLocation&)station) = m_stations[pos];

		station.m_name = WBSF::PurgeFileName(station.m_name);



		CTRef current = CTRef::GetCurrentTRef();
		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = size_t(lastYear - firstYear + 1);

		station.CreateYears(firstYear, nbYears);


		//now extract data 
		for (size_t y = 0; y < nbYears&&msg; y++)
		{
			int year = firstYear + int(y);

			size_t nbMonths = (year < current.GetYear()) ? 12 : current.GetMonth() + 1;
			for (size_t m = 0; m < nbMonths&&msg; m++)
			{
				string filePath = GetOutputFilePath(station.GetSSI("Country"), station.GetSSI("State"), ID, year, m);
				if (FileExists(filePath))
				{
					msg = ReadData(filePath, TM, year, station, callback);
					msg += callback.StepIt(0);
				}
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


	//*************************************************************************************************************************************
	bool CUIMesoWest::IsCommonStation(const std::string& network)
	{
		return IsEqual( network, "Canada");
	}

	
	CTRef CUIMesoWest::GetTRef(const string& str)
	{
		StringVector e(str,"-T:+");
		ASSERT(e.size() == 7);
		ASSERT(e[6] != "Z");

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

	ERMsg CUIMesoWest::ReadData(const string& filePath, CTM TM, int year, CWeatherStation& data, CCallback& callback)const
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

					CWeatherAccumulator accumulator(TM);

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
						if (TRefs[i].GetYear() == year)
						{
							for (size_t v = 0; v < header.size()&&msg; v++)
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


							if (accumulator.GetTRef().IsInit() && (i + 1 == TRefs.size() || accumulator.TRefIsChanging(TRefs[i + 1])))
								data[accumulator.GetTRef()].SetData(accumulator);
						}//if good year
					}//for all TRef
				}//if have one station
			}//if no error
			else
			{
				msg.ajoute(error);
			}
		}//if msg
		//	const Json& item4 = item2[j]["geometry"];
		//	//:{"x":-96.101388888889, "y" : 56.086388888889}
		//	string test = item3["station_latitude"].string_value();


		//	string name = item3["station_name"].string_value();
		//	string ID = item3["station_no"].string_value();
		//	double lat = item4["y"].number_value();
		//	double lon = item4["x"].number_value();
		//	CLocation location(name, ID, lat, lon, -999);
		//	location.SetSSI("Network", NETWORK_NAME[HYDRO]);
		//	location.SetSSI("StationNo", item3["station_id"].string_value());
		//	location.SetSSI("RiverName", item3["river_name"].string_value());
		//	location.SetSSI("InstallDate", item3["gen.shelterinstall"].string_value());
		//	locations.push_back(location);

		//	msg += callback.StepIt();


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

		return str;
	}

	/*CTPeriod  GetPeriod(string str)
	{

	CTPeriod period = GetPeriod((*it)["PERIOD_OF_RECORD"].string_value());
	string period = "" : {
	"start": "1970-01-01T00:00:00Z",
	"end" : "2017-04-05T14:00:00Z"
	}*/

	ERMsg CUIMesoWest::DownloadStationList(CLocationVector& stationList, CCallback& callback)const
	{
		ERMsg msg;

		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;

		//a faire avec JSON
		///v2/stations/metadata?token=635d9802c84047398d1392062e39c960

		msg += GetHttpConnection("api.mesowest.net", pConnection, pSession);
		if (msg)
		{
			//v2/stations/metadata?complete=1&state=QC&token=635d9802c84047398d1392062e39c960
			std::map<string, string> networks;
			{


				string URL = "v2/networks?token=635d9802c84047398d1392062e39c960";

				string source;
				msg = GetPageText(pConnection, URL, source);
				if (msg)
				{
					string error;
					const Json& root = Json::parse(source, error);
					if (error.empty())
					{
						ASSERT(root["MNET"].type() == Json::ARRAY);
						const std::vector<Json>& nets = root["MNET"].array_items();

						for (Json::array::const_iterator it = nets.begin(); it != nets.end() && msg; it++)
							networks[(*it)["ID"].string_value()] = (*it)["SHORTNAME"].string_value();
					}
				}
			}


			string URL = "v2/stations/metadata?token=635d9802c84047398d1392062e39c960";

			string source;
			msg = GetPageText(pConnection, URL, source);
			if (msg)
			{
				string error;
				const Json& root = Json::parse(source, error);
				if (error.empty())
				{

					ASSERT(root["STATION"].type() == Json::ARRAY);
					const std::vector<Json>& stations = root["STATION"].array_items();


					callback.PushTask(GetString(IDS_LOAD_STATION_LIST) + " (MesoWest)", stations.size());
					for (Json::array::const_iterator it = stations.begin(); it != stations.end() && msg; it++)
					{
						CLocation location;



						location.m_name = PurgeName((*it)["NAME"].string_value());
						location.m_ID = (*it)["STID"].string_value();
						location.m_lat = WBSF::as<double>((*it)["LATITUDE"].string_value());
						location.m_lon = WBSF::as<double>((*it)["LONGITUDE"].string_value());
						location.m_alt = WBSF::as<double>((*it)["ELEVATION"].string_value());


						string country = (*it)["COUNTRY"].string_value();
						string state = (*it)["STATE"].string_value();
						string time_zone = (*it)["TIMEZONE"].string_value();
						string id = (*it)["ID"].string_value();
						string status = (*it)["STATUS"].string_value();
						string network = (*it)["SHORTNAME"].string_value();
						string networkID = (*it)["MNET_ID"].string_value();
						string start = (*it)["PERIOD_OF_RECORD"]["start"].string_value();

						if (state == "NF")
							state = "NL";

						if (country.empty())
						{
							if (CProvinceSelection::GetProvince(state) != UNKNOWN_POS)
								country = "CA";
							else if (CStateSelection::GetState(state) != UNKNOWN_POS)
								country = "US";
							else
								country = "--";
						}

						if (network.empty() && !networkID.empty())
							network = networks[networkID];

						if (location.m_ID == "A1139")//alien station
						{
							country = "US";
							state = "WY";
						}

						location.SetSSI("Status", status);
						location.SetSSI("Country", country);
						location.SetSSI("State", state);
						location.SetSSI("Network", network);
						location.SetSSI("NetworkID", networkID);
						location.SetSSI("TimeZone", time_zone);
						location.SetSSI("Start", start);


						stationList.push_back(location);

						msg += callback.StepIt();
					}//for all stations

					callback.PopTask();
				}//if error
				else
				{
					msg.ajoute(error);
				}
			}//if msg
		}//if msg

		callback.AddMessage(GetString(IDS_NB_STATIONS) + ToString(stationList.size()));

		pConnection->Close();
		pSession->Close();


		return msg;
	}



	//Variable	Description	English Units	Metric Units
	//T10M	Air Temperature at 10 meters	° F	° C
	//TMPF	Temperature	° F	° C
	//T2M	Air Temperature at 2 meters	° F	° C
	//MTMP	Temperature	° F	° C
	//HI24	24 Hr High Temperature	° F	° C
	//LO24	24 Hr Low Temperature	° F	° C

	//P01I	Precipitation 1hr	 in	 cm
	//P01M	Precipitation 1hr manual	 in	 cm
	//P24I	Precipitation 24hr	 in	 cm
	//P24M	Precipitation 24hr manual	 in	 cm
	//PREC	Precipitation accumulated	 in	 cm
	//PREM	Precipitation manual	 in	 cm

	//DWPF	Dewpoint	° F	° C
	//MDWP	Dew Point	° F	° C
	//MRH	Relative Humidity	%	%
	//RELH	Relative Humidity	%	%

	//MSKT	Wind Speed	 mph	 m/s
	//SKNT	Wind Speed	 mph	 m/s
	//DRCT	Wind Direction	°	°
	//MDIR	Wind Direction	°	°
	//PDIR	Peak Wind Direction	°	°
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
	//MPRS	Pressure	 in ° F	 mb
	//PMSL	Sea level pressure	 in	 mb
	//PRES	Pressure	 in	 mb

	//SNOM	Snow manual	 in	 cm
	//SNOW	Snow depth	 in	 cm
	//SSTM	Snowfall	 in	 cm
	//WEQS	Snow water equivalent	 in	 cm

	//MSO2	Soil Moisture 2	%	%
	//MSOI	Soil Moisture	%	%
	//VSBY	Visibility	 miles	 km


	TVarH CUIMesoWest::GetVar(const string& str)
	{
		TVarH v = H_SKIP;

		string::size_type pos = str.find("_set_");
		if (pos != string::npos)
		{
			string tmp = str.substr(0, pos);
			enum { LO24, T2M, HI24, PMID, MDWP, RELH, MSKT, MDIR, SOLR, PRES, SNOW, WEQS, NB_VARS };

			static const char* VAR_NAME[NB_VARS] = { "air_temp_high_24_hour", "air_temp", "air_temp_low_24_hour", "precip_accum_since_local_midnight", "dew_point_temperature", "relative_humidity", "wind_speed", "wind_direction", "solar_radiation", "pressure", "snow_depth", "snow_water_equivalent" };
			static const TVarH VAR_TYPE[NB_VARS] = { H_TMIN2, H_TAIR2, H_TMAX2, H_PRCP, H_TDEW, H_RELH, H_WNDS, H_WNDD, H_SRAD2, H_PRES, H_SNDH, H_SWE };

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


//
//Air Temperature at 10 meter		°C	T10M
//Temperature						°C	TMPF
//Air Temperature at 2 meters		°C	T2M
//Temperature						°C	MTMP
//24 Hr High Temperature				HI24
//24 Hr Low Temperature				LO24
//
//Precipitation 1hr	 			mm	P01I
//Precipitation 1hr manual			P01M
//Precipitation 24hr	 			mm	P24I
//Precipitation 24hr manual			P24M
//Precipitation accumulated			PREC
//Precipitation manual	 			PREM
//
//Dewpoint						°C	DWPF
//Dew Point						°C	MDWP
//Relative Humidity				%	MRH
//Relative Humidity				%	RELH
//Wind Speed	 					m/s	MSKT
//Wind Speed	 					m/s	SKNT
//Wind Direction					°	DRCT
//Wind Direction					°	MDIR
//Peak Wind Direction				°	PDIR
//Peak Wind Speed	 				m/s PEAK
//
//Clear Sky Solar Radiation			CSLR
//Incoming Longwave Radiation			INLW
//Net Longwave Radiation			W/m² NETL
//Net Radiation					W/m² NETR
//Net Shortwave Radiation			W / m² NETS
//Outgoing Longwave Radiation			OUTL
//Outgoing Shortwave Radiatio			OUTS
//Solar Radiation					W/m² SOLR
//
//Altimeter	 	 mb					ALTI
//Pressure	 	 mb				MPRS
//Sea level pressure	 	 mb			PMSL
//Pressure	 	 mb					PRES
//
//Snow manual	 	 cm					SNOM
	
//Snowfall	 	 cm					SSTM

//Snow depth	 	 cm				SNOW
//Snow water equivalent	 			WEQS
//
//Soil Moisture  %					MSO2
//Soil Moisture	%				MSOI
//Visibility	 km				VSBY

}





