#include "stdafx.h"
#include "UIBC.h"

#include "basic/WeatherStation.h"
#include "basic/CSV.h"
#include "UI/Common/SYShowMessage.h"

#include "TaskFactory.h"
#include "../Resource.h"
#include "WeatherBasedSimulationString.h"

using namespace WBSF::HOURLY_DATA;
using namespace std;
using namespace UtilWWW;

namespace WBSF
{

	//*********************************************************************

	static const DWORD FLAGS = INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_PRAGMA_NOCACHE | INTERNET_FLAG_TRANSFER_BINARY;
	const char* CUIBC::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "Type", "FirstYear", "LastYear", "UpdateStationList", "IgnoreEnvCan" };
	const size_t CUIBC::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_COMBO_INDEX, T_STRING, T_STRING, T_BOOL, T_BOOL };
	const UINT CUIBC::ATTRIBUTE_TITLE_ID = IDS_UPDATER_BC_P;
	const UINT CUIBC::DESCRIPTION_TITLE_ID = ID_TASK_BC;

	const char* CUIBC::CLASS_NAME(){ static const char* THE_CLASS_NAME = "BCWeather";  return THE_CLASS_NAME; }
	CTaskBase::TType CUIBC::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUIBC::CLASS_NAME(), (createF)CUIBC::create);


	const char* CUIBC::SERVER_NAME = "tools.pacificclimate.org";

//network_id	network_name
//1				EC
//2				MoTIe
//3				MoTIm
//5				BCH
//6				RTA
//9				ENV-AQN
//10			AGRI
//11			FLNRO-FERN
//12			FLNRO-WMB
//13			MoTI
//14			ENV-ASP
//15			MVan
//16			FRBC
//17			ARDA
//19			EC_raw
	const size_t CUIBC::NETWOK_ID_TO_ENUM[20] = { NOT_INIT, EC, MoTIe, MoTIm, NOT_INIT, BCH, RTA, NOT_INIT, NOT_INIT, ENV_AQN, AGRI, FLNRO_FERN, FLNRO_WMB, NOT_INIT, ENV_ASP, NOT_INIT, FRBC, ARDA, NOT_INIT, EC_RAW };
		

	const char* CUIBC::NETWORK_NAME[NB_NETWORKS] = { "AGRI", "ARDA", "BCH", "EC", "EC_raw", "ENV-AQN", "ENV_ASP", "FLNRO-FERN", "FLNRO-WMB", "FRBC", "MoTIe", "MoTIm", "RTA" };
	const char* CUIBC::NETWORK_TILE[NB_NETWORKS] = { "BC Ministry of Agriculture", "Agricultural and Rural Development Act Network", "BC Hydro", "Environment Canada(Canadian Daily Climate Data 2007)", "Environment Canada(raw observations from Climate Data Online)", "BC Ministry of Environment - Air Quality Network", "BC Ministry of Environment - Automated Snow Pillow Network", "Forest Renewal British Columbia", "BC Ministry of Forests, Lands, and Natural Resource Operations - Forest Ecosystems Research Network", "BC Ministry of Forests, Lands, and Natural Resource Operations - Wild Fire Managment Branch", "Ministry of Transportation and Infrastructure(electronic)", "Ministry of Transportation and Infrastructure(manual)", "RioTintoAlcan" };
	const char* CUIBC::TYPE_NAME[NB_TYPES] = { "1-hourly", "daily" /*"12-hourly", "irregular"*/ };
	const bool CUIBC::AVAILABILITY[NB_NETWORKS][NB_TYPES] =
	{
		false, true,//AGR
		false, true,//ARDA
		true, true,	//BCH
		false, true,//EC
		true, true,	//EC_raw
		true, false,//ENV-AQN
		false, true,//ENV_ASP
		true, false,//FLNRO-FERN
		true, true, //FLNRO-WMB
		true, false,//FRBC
		true, false,//MoTIe
		false, true,//MoTIm 
		false, true,//RTA
	};

	size_t CUIBC::GetNetwork(const string& network_name)
	{
		size_t n=NOT_INIT;
		for (size_t nn = 0; nn < NB_NETWORKS&&n == NOT_INIT; nn++)
		{
			if (IsEqual(network_name, NETWORK_NAME[nn]))
				n = nn;
		}

		return n;
	}

	CUIBC::CUIBC(void)
	{}

	CUIBC::~CUIBC(void)
	{}

	
	

	std::string CUIBC::Option(size_t i)const
	{
		string str;
		switch (i)
		{
		case DATA_TYPE:	str = GetString(IDS_STR_DATA_TYPE); break;
		};
		return str;
	}

	std::string CUIBC::Default(size_t i)const
	{
		string str;

		switch (i)
		{
		case WORKING_DIR: str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()) + "PCIC\\"; break;
		case FIRST_YEAR:
		case LAST_YEAR:	str = ToString(CTRef::GetCurrentTRef().GetYear()); break;
		case DATA_TYPE: str = "1"; break;
		case UPDATE_STATION_LIST:	str = "0"; break;
		case IGNORE_ENV_CAN:	str = "1"; break;
		};
		return str;
	}
//**********************************************************************************************************************************
	ERMsg CUIBC::Execute(CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);

		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(workingDir, 1);
		callback.AddMessage("");
		

		CTRef today = CTRef::GetCurrentTRef();

		//open a connection on the server
		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;

		msg = GetHttpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", false, 5, callback);
		if (!msg)
			return msg;

		if (as<bool>(UPDATE_STATION_LIST))
		{
			msg = UpdateStationList(pConnection, callback);
			if (!msg)
				return msg;
		}

		callback.AddMessage(GetString(IDS_UPDATE_FILE));
		int nbDownload = 0;

		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = lastYear - firstYear + 1;

		callback.PushTask("Download data for all years", nbYears);
		

		for (size_t y = 0; y < nbYears&&msg; y++)
		{
			int year = firstYear + int(y);

			size_t type = as<size_t>(DATA_TYPE); ASSERT(type < NB_TYPES);
			callback.PushTask(FormatA("%04d", year), type == HOURLY_WEATHER ? 7 : 9);
			for (size_t n = 0; n < NB_NETWORKS && msg; n++)
			{
				bool bIgnoreEnvCan = as<bool>(IGNORE_ENV_CAN);
				bool bEnvCan = n == EC || n == EC_RAW;
				if (!(bIgnoreEnvCan&&bEnvCan))
				{
					static const char* URL_FORMAT =
						"dataportal/data/pcds/agg/?"
						"from-date=%4d%%2F01%%2F01&"
						"to-date=%4d%%2F12%%2F31&"
						"input-polygon=&"
						"input-var=&"
						"network-name=%s&"
						"input-freq=%s&"
						"data-format=ascii&"
						"cliptodate=cliptodate&"
						"download-timeseries=Timeseries";

					//string URL = "dataportal/data/pcds/agg/?from-date=2015%2F01%2F01&to-date=2015%2F12%2F31&input-polygon=&input-var=&network-name=FLNRO-WMB&input-freq=1-hourly&data-format=ascii&cliptodate=cliptodate&download-timeseries=Timeseries";	
					//string URL = "dataportal/data/pcds/agg/?from-date=2016%2F01%2F01&to-date=2016%2F12%2F31&input-polygon=&input-var=&network-name=BCH&input-freq=daily&data-format=ascii&cliptodate=cliptodate&download-timeseries=Timeseries
					string URL = FormatA(URL_FORMAT, year, year, NETWORK_NAME[n], TYPE_NAME[type]);

					string ouputPath = workingDir + TYPE_NAME[type] + "\\" + ToString(year) + "\\";// +NETWORK_NAME[n] + "\\";
					CreateMultipleDir(ouputPath);

					
					string  filePathZip = ouputPath + NETWORK_NAME[n] + ".zip";

					
					callback.PushTask("Downloading " + string(NETWORK_NAME[n]) + " " + ToString(year) + "...", NOT_INIT);
					msg += UtilWWW::CopyFile(pConnection, URL, filePathZip, FLAGS, "", "", false, callback);
					callback.PopTask();
					if (msg)
					{
						if (FileExists(filePathZip) )
						{
							ifStream tmp;
							tmp.open(filePathZip);
							size_t length = tmp.length();
							tmp.close();
							if (length == 2)
							{
								RemoveFile(filePathZip);
							}
							else
							{
								msg = sevenZ(filePathZip, ouputPath, callback);

								if (msg)
								{
									RemoveFile(filePathZip);
									nbDownload++;
								}
							}
						}

						msg += callback.StepIt();
					}
					else
					{
					
						pConnection->Close();
						pSession->Close();

						//wait 5 seconds 
						callback.PushTask("Waiting 5 seconds...", 50);
						for (size_t s = 0; s < 50 && msg; s++)
						{
							Sleep(100);
							msg += callback.StepIt();
						}
						callback.PopTask();

						msg = GetHttpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", false, 5, callback);
						
					}
				}//if available
			}//for all networks

			callback.PopTask();
		}//for all years


		pConnection->Close();
		pSession->Close();

		callback.AddMessage(GetString(IDS_NB_FILES_DOWNLOADED) + ToString(nbDownload));
		callback.PopTask();

		return msg;
	}


	ERMsg CUIBC::sevenZ(const string& filePathZip, const string& workingDir, CCallback& callback)
	{
		ERMsg msg;

		callback.PushTask(GetString(IDS_UNZIP_FILE), NOT_INIT);

		string command = GetApplicationPath() + "External\\7z.exe x \"" + filePathZip + "\" -y";

		DWORD exitCode = 0;
		msg = WinExecWait(command, workingDir, SW_HIDE, &exitCode);
		if (msg && exitCode != 0)
			msg.ajoute("7z.exe as exit with error code " + ToString(int(exitCode)));


		callback.PopTask();


		return msg;
	}

	std::string CUIBC::GetStationListFilePath()const
	{
		return GetDir(WORKING_DIR) + "stations.csv";
	}



	ERMsg CUIBC::UpdateStationList(CHttpConnectionPtr& pConnection, CCallback& callback)const
	{
		ERMsg msg;

		static const char* URL = "geoserver/CRMP/ows?service=WFS&version=1.1.0&request=GetFeature&typeName=CRMP%3Acrmp_network_geoserver&outputFormat=csv&srsname=epsg%3A4326&filter=%3Cogc%3AFilter+xmlns%3Aogc%3D%22http%3A%2F%2Fwww.opengis.net%2Fogc%22%3E%3Cogc%3AAnd%2F%3E%3C%2Fogc%3AFilter%3E";
		
		string localFilePath = GetStationListFilePath();
		CreateMultipleDir(GetPath(localFilePath));

		callback.PushTask("Downloading station list", NOT_INIT);
		msg = UtilWWW::CopyFile(pConnection, URL, localFilePath.c_str(), FLAGS, "", "", false, callback);
		callback.PopTask();


		return msg;

	}


	ERMsg CUIBC::LoadStationList(CCallback& callback)
	{
		ERMsg msg;

		m_stations.clear();
	/*	m_stations.Load(GetStationListFilePath(),",",callback);

		*/

		ifStream file;
		msg = file.open(GetStationListFilePath());

		if (msg)
		{
			enum { FID, NETWORK_NAME, NATIVE_ID, STATION_NAME, LON, LAT, ELEV, MIN_OBS_TIME, MAX_OBS_TIME, FREQ, PROVINCE, STATION_ID };
			for (CSVIterator loop(file, ",", true, true); loop != CSVIterator(); ++loop)
			{
				string ID = TrimConst((*loop)[NATIVE_ID]); 
				MakeUpper(ID);
				string strID = TrimConst((*loop)[NETWORK_NAME]) + ID;
				string name = (*loop)[STATION_NAME];
				CLocation loc(WBSF::UppercaseFirstLetter(name), strID, ToDouble((*loop)[LAT]), ToDouble((*loop)[LON]), ToDouble((*loop)[ELEV]));

				 
				loc.SetSSI("network_name", (*loop)[NETWORK_NAME]);
				loc.SetSSI("native_id", (*loop)[NATIVE_ID]);
				loc.SetSSI("min_obs_time", (*loop)[MIN_OBS_TIME]);
				loc.SetSSI("max_obs_time", (*loop)[MAX_OBS_TIME]);
				loc.SetSSI("freq", (*loop)[FREQ]);
				loc.SetSSI("province", (*loop)[PROVINCE]);
				loc.SetSSI("station_id", (*loop)[STATION_ID]);
				
				m_stations.push_back(loc);
				
			}
		}

		return msg;
	}

	ERMsg CUIBC::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;

		
		msg = LoadStationList(callback);
		if (!msg)
			return msg;


		set<string> fileList;

		string workingDir = GetDir(WORKING_DIR);
		CTRef today = CTRef::GetCurrentTRef();

		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = lastYear - firstYear + 1;
		size_t type = as<size_t>(DATA_TYPE);
		bool bIgnoreEnvCan = as<bool>(IGNORE_ENV_CAN);

		callback.PushTask(GetString(IDS_LOAD_STATION_LIST), nbYears);

		//find all station available that meet criterious
		for (size_t y = 0; y < nbYears&&msg; y++)
		{
			int year = firstYear + int(y);
			string command = workingDir + TYPE_NAME[type] + "\\" + ToString(year) + "\\*.ascii";
			StringVector fileListTmp = GetFilesList(command, 2, true);

			for (size_t i = 0; i < fileListTmp.size(); i++)
			{
				string network = WBSF::GetLastDirName(GetPath(fileListTmp[i]));
				bool bEnvCan = network == "EC" || network == "EC_raw";
				if (!(bIgnoreEnvCan&&bEnvCan))
				{
					string fileTitle = GetFileTitle(fileListTmp[i]);
					MakeUpper(fileTitle);
					string strID = network + fileTitle;
					
					fileList.insert(strID);//ID is composed of the name of the network and the station ID
				}
			}

			msg += callback.StepIt();
		}

		stationList.insert(stationList.begin(), fileList.begin(), fileList.end());
		callback.PopTask();

		return msg;
	}

	ERMsg CUIBC::GetWeatherStation(const string& stationID, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;

		size_t pos = m_stations.FindByID(stationID);
		if (pos == NOT_INIT)
		{
			msg.ajoute(FormatMsg(IDS_NO_STATION_INFORMATION, stationID));
			return msg;
		}

		((CLocation&)station) = m_stations[pos];
		if (station.m_name.substr(0, 3) == "Zz ")//remove Zz at the begginning of the name
			station.m_name = station.m_name.substr(3);
		if (!station.m_name.empty())
			station.m_name += " (BC)";

		string workingDir = GetDir(WORKING_DIR);
		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = lastYear - firstYear + 1;
		size_t type = as<size_t>(DATA_TYPE);

		string network_name = station.GetSSI("network_name");
		size_t n = GetNetwork(network_name);
		
		ASSERT(n != NOT_INIT);
		if (n != NOT_INIT)
		{
			string fileName = station.GetSSI("native_id") + ".ascii";

			StringVector fileList;
			for (size_t y = 0; y < nbYears; y++)
			{
				int year = firstYear + int(y);

				string filepath = workingDir + TYPE_NAME[type] + "\\" + ToString(year) + "\\" + network_name + "\\" + fileName;
				if (FileExists(filepath))
					fileList.push_back(filepath);
			}

			bool bStepIt = fileList.size() > 4;
			//now extract data 
			if (bStepIt)
				callback.PushTask(station.m_name, fileList.size());

			for (size_t i = 0; i < fileList.size() && msg; i++)
			{
				msg = ReadData(fileList[i], TM, station, callback);
				msg += callback.StepIt(bStepIt ? 1 : 0);
			}

			if (n == MoTIe)
			{
				//precipitation is not valide for the MoTIe network
				CWAllVariables no_prcp;
				no_prcp.reset(H_PRCP);
				station.CleanUnusedVariable(no_prcp);
			}

			if (msg)
			{
				//verify station is valid
				if (station.HaveData())
				{
					msg = station.IsValid();
				}
			}

			if (bStepIt)
				callback.PopTask();
		}

		return msg;
	}

	class CVarInfo
	{
	public :

		CVarInfo(TVarH var=H_SKIP, string s="", string u="")
		{
			v = var;
			stat=s;
			units=u;
		}

		TVarH v;
		string stat;
		string units;
	};

	class CVarInfoMap : public map<string, CVarInfo>
	{
	public:

		TVarH GetVariable(string var_name)const
		{
			TVarH v = H_SKIP;
			MakeLower(var_name);

			if (var_name == "temperature" || var_name == "air_temperature" || var_name == "current_air_temperature1" || var_name == "current_air_temperature2")
				v = H_TAIR;
				
			else if (var_name == "minimum_air_temperature" || var_name == "min_temp")
				v = H_TMIN;
			else if (var_name == "maximum_air_temperature" || var_name == "max_temp")
				v = H_TMAX;
			else if (var_name == "precipitation" || var_name == "hourly_precipitation" || var_name == "precipitation_new" || var_name == "one_day_precipitation" || var_name == "one_day_rain")
				v = H_PRCP;
			else if (var_name == "dew_point")
				v = H_TDEW;
			else if (var_name == "relative_humidity" || var_name == "relative_humidity1")
				v = H_RELH;
			else if (var_name == "wind_speed" || var_name == "actual_wind_speed" || var_name == "measured_wind_speed1")
				v = H_WNDS;
			else if (var_name == "wind_direction" || var_name == "wind_direction" || var_name == "actual_wind_direction" || var_name == "measured_wind_direction1")
				v = H_WNDD;
			//else if (var_name == "snow_amount" || var_name == "standard_snow" || var_name == "one_day_snow")
				//v = H_SNOW;//???
			else if (var_name == "height_of_snow" || var_name == "snow_on_the_ground")
				v = H_SNDH;//???
			//			else if (var_name == "total_cloud_cover") estcew qu'on peut transfromer en Watt/s
			else if (var_name == "mean_sea_level" || var_name == "atmospheric_pressure")
				v = H_PRES;


			//var_name == "precipitation_gauge_total"
			//var_name == "total_precipitation"		
			//var_name == "total_rain"		

			return v;

		}

		ERMsg load(const string& fielPath)
		{
			ERMsg msg;

			clear();

			enum TColumns { C_VARIABLE, C_STANDARD_NAME, C_CELL_METHOD, C_UNIT, NB_COLUMNS };

			ifStream file;
			msg = file.open(fielPath);

			if (msg)
			{
				CVarInfoMap& me = *this;

				string title;
				getline(file, title);
				ASSERT(title == "variables");

				for (CSVIterator loop(file); loop != CSVIterator(); ++loop)
				{
					ASSERT((*loop).size() == NB_COLUMNS);
					StringVector method((*loop)[C_CELL_METHOD], ":");
					ASSERT(method.size() == 2);

					string var_name = TrimConst((*loop)[C_VARIABLE]);
					TVarH v = GetVariable(var_name);
					if (v != H_SKIP)
						me[var_name] = CVarInfo(v, TrimConst(method[1]), TrimConst((*loop)[C_UNIT]));
				}
			}

			return msg;
		}
	};

	static double Normalize(double value, const string& name, const string& units, double h)
	{
		//variable						cell_method	 unit
		//temperature					time: point		celsius
		//air_temperature				time: point		celsius
		//current_air_temperature1		time: point		celsius
		//current_air_temperature2		time: point		celsius
		//minimum_air_temperature		time: minimum	celsius
		//min_temp						time: minimum	celsius
		//maximum_air_temperature		time: maximum	celsius
		//max_temp						time: maximum	celsius

		//precipitation					time: sum		mm
		//precipitation_gauge_total		time: sum		mm
		//precipitation_new				time: sum		mm
		//hourly_precipitation			time: sum		mm
		//total_precipitation			time: sum		mm
		//total_rain					time: sum		mm
		//one_day_precipitation			time: sum		mm
		//one_day_rain					time: sum		mm

		//dew_point						time: point		celsius
		//dew_point						time: mean		celsius
		//relative_humidity				time: point		%
		//relative_humidity				time: mean		%
		//relative_humidity1			time: mean		%

		//snow_amount					time: sum		cm
		//standard_snow					time: sum		cm
		//height_of_snow				time: point		cm
		//snow_on_the_ground			time : point	cm
		//one_day_snow					time : sum		cm


		//wind_direction				time: mean		degree
		//wind_direction				time: mean		degree
		//wind_speed					time: point		km/h
		//wind_speed					time: mean		m s-1
		//actual_wind_speed				time: point		none
		//measured_wind_direction1		time: mean		degrees
		//measured_wind_speed1			time: mean		km h-1
		//actual_wind_direction			time: point		degrees

		//total_cloud_cover				time: point		%

		//mean_sea_level				time: point		kPa
		//atmospheric_pressure			time: point		millibar

		if (name == "mean_sea_level")
		{
			ASSERT(units == "kPa");
			value = WBSF::msl2atp(value, h)*10;//kPa --> hPa
			ASSERT(value >= 850 && value <= 1150);
		}
		else if (units == "m s-1" || units == "m/s")
		{
			value *= 3600 / 1000;//m/s --> km/h
		}
		else if (units == "none")
		{

			int i;
			i = 0;
			ASSERT(false);

		}

		return value;
	}

	ERMsg CUIBC::ReadData(const string& filePath, CTM TM, CWeatherStation& data, CCallback& callback)const
	{
		ERMsg msg;

		string path = GetPath(filePath);
		
		CVarInfoMap var_map;
		msg += var_map.load(path + "variables.csv");
	
		if (!msg)
			return msg;
		


		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);

		//now extact data 
		ifStream file;

		msg = file.open(filePath);

		if (msg)
		{
			CWeatherAccumulator stat(TM);

			string title;
			getline(file, title);
			ASSERT(title == "station_observations");
			
			size_t timePos = NOT_INIT;
			//array<size_t, NB_VAR_H> col_pos;
			vector<CVarInfoMap::const_iterator> col_pos;

			for (CSVIterator loop(file); loop != CSVIterator(); ++loop)
			{
				if (timePos == NOT_INIT)
				{
					col_pos.insert(col_pos.begin(), loop.Header().size(), var_map.end());
					for (size_t c = 0; c < loop.Header().size(); c++)
					{
						string header = TrimConst(loop.Header().at(c));

						if (header == "time")
							timePos = c;
						else
							col_pos[c] = var_map.find(header);
					}
				}

				ASSERT(timePos != NOT_INIT);
				string dateTimeStr = (*loop)[timePos];
				StringVector dateTime(dateTimeStr, " -:");

				int year = stoi(dateTime[0]);
				size_t month = stoi(dateTime[1])-1;
				size_t day = stoi(dateTime[2])-1;
				int hour = stoi(dateTime[3]);

				ASSERT(year >= firstYear - 1 && year <= lastYear);
				ASSERT(month >= 0 && month < 12);
				ASSERT(day >= 0 && day < GetNbDayPerMonth(year, month));
				ASSERT(hour >= 0 && hour < 24);


				CTRef TRef = CTRef(year, month, day, hour, TM);

				if (stat.TRefIsChanging(TRef) )
					data[stat.GetTRef()].SetData(stat);

				for (size_t c = 0; c<loop->size(); c++)
				{
					if (col_pos[c] != var_map.end())
					{
						string str = TrimConst((*loop)[c]);
						if (str != "None" )
						{
							double value = stod(str);
							value = Normalize(value, col_pos[c]->first, col_pos[c]->second.units, data.m_z);

							
							if (col_pos[c]->second.v == H_RELH)//limit to 100
								value = min(100.0, value);
							
							if (col_pos[c]->second.v == H_SNOW || col_pos[c]->second.v == H_SNDH || col_pos[c]->second.v == H_SWE)//limit to 100
								value = max(0.0, value);
							

							if( col_pos[c]->second.v < H_ADD1)
								stat.Add(TRef, col_pos[c]->second.v, value);
						}
					}
				}

				msg += callback.StepIt(0);
			}//for all line


			if (stat.GetTRef().IsInit() )
				data[stat.GetTRef()].SetData(stat);

		}//if load 

		return msg;
	}

//http://tgs.gov.mb.ca/climate/HourlyReport.aspx
	//http://tgs.gov.mb.ca/climate/DailyReport.aspx?%2FwEPDwUJMjE5Njc0MjAyD2QWAmYPZBYCAgMPZBYCAgEPZBYIAgEPZBYGAgEPZBYCAgEPZBYCZg8QDxYGHg1EYXRhVGV4dEZpZWxkBQtTdGF0aW9uTmFtZR4ORGF0YVZhbHVlRmllbGQFBVN0bklEHgtfIURhdGFCb3VuZGdkEBU%2BBkFsdG9uYQZBcmJvcmcGQmlydGxlCkJvaXNzZXZhaW4HQnJhbmRvbghDYXJiZXJyeQZDYXJtYW4NQ3lwcmVzcyBSaXZlcgdEYXVwaGluCURlbG9yYWluZQZEdWdhbGQJRWxtIENyZWVrC0VtZXJzb24gQXV0CUVyaWtzZGFsZQlFdGhlbGJlcnQNRmlzaGVyIEJyYW5jaAdGb3JyZXN0BUdpbWxpCUdsYWRzdG9uZQhHbGVuYm9ybwlHcmFuZHZpZXcGR3JldG5hB0hhbWlvdGEJS2lsbGFybmV5CUxldGVsbGllcgdNYW5pdG91CE1jQ3JlYXJ5Bk1lbGl0YQxNZWxpdGEgU291dGgJTWlubmVkb3NhCU1vb3NlaG9ybgpNb3JkZW4gQ0RBBk1vcnJpcwdQaWVyc29uC1BpbG90IE1vdW5kClBpbmF3YSBBdXQHUG9ydGFnZQxQb3J0YWdlIEVhc3QGUmVzdG9uClJvYmxpbiBBVVQHUnVzc2VsbAdTZWxraXJrClNob2FsIExha2UIU29tZXJzZXQGU291cmlzC1NwcmFndWUgQVVUCVN0IFBpZXJyZQtTdC4gQWRvbHBoZQhTdGFyYnVjawlTdGUuIFJvc2UJU3RlaW5iYWNoClN3YW4gUml2ZXILU3dhbiBWYWxsZXkGVGV1bG9uB1RoZSBQYXMIVHJlaGVybmUGVmlyZGVuCldhc2FnYW1pbmcIV2F3YW5lc2ENV2lua2xlciBDTUNEQxBXaW5uaXBlZyBBaXJwb3J0CVdvb2RsYW5kcxU%2BAzI0NAMyMDYDMjE2AzIwOQEyATQBNQE4ATkDMjQxAzIxNwMyMzcCMTEDMjE4AzIxMwI1NgMyMzMCMTQDMjA0AzIzOQMyMTkCMTcDMjE0AzIyMAMyMzgDMjQyAjI1AjI2Azc0MAMyMjEDMjA1AjI5AzIyMgMyMzICMzQCMzUCMzcDMjM1AzI0NQIzOAMyMTUDMjEwAjQwAzI0NgMyMDgCNDEDMjAzAzI0MwMyMDIDMjExAzIyMwI0NAMyMzEDMjA3AjQ1AzIwMQMyMjQCNTEDMjQwAzIzMAI1MgMyMjUUKwM%2BZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dkZAICD2QWAgIBD2QWAgICDw9kFgweC29uTW91c2VPdmVyBT9DaGFuZ2VNb3VzZSgnY3RsMDBfRGVmYXVsdENvbnRlbnRfaW1nUGxhbnREYXRlJywgJ29uTW91c2VPdmVyJykeCm9uTW91c2VPdXQFPkNoYW5nZU1vdXNlKCdjdGwwMF9EZWZhdWx0Q29udGVudF9pbWdQbGFudERhdGUnLCAnb25Nb3VzZU91dCcpHgtvbk1vdXNlRG93bgVAQ2hhbmdlQm9yZGVyKCdjdGwwMF9EZWZhdWx0Q29udGVudF9pbWdQbGFudERhdGUnLCAnb25Nb3VzZURvd24nKR4Jb25Nb3VzZVVwBT5DaGFuZ2VCb3JkZXIoJ2N0bDAwX0RlZmF1bHRDb250ZW50X2ltZ1BsYW50RGF0ZScsICdvbk1vdXNlVXAnKR4Hb25DbGljawUxU2hvd0NhbGVuZGFyKCdjdGwwMF9EZWZhdWx0Q29udGVudF90eHRQbGFudERhdGUnKR4Kb25LZXlQcmVzcwUxU2hvd0NhbGVuZGFyKCdjdGwwMF9EZWZhdWx0Q29udGVudF90eHRQbGFudERhdGUnKWQCAw9kFgICAQ9kFgICAg8PZBYMHwMFPUNoYW5nZU1vdXNlKCdjdGwwMF9EZWZhdWx0Q29udGVudF9pbWdFbmREYXRlJywgJ29uTW91c2VPdmVyJykfBAU8Q2hhbmdlTW91c2UoJ2N0bDAwX0RlZmF1bHRDb250ZW50X2ltZ0VuZERhdGUnLCAnb25Nb3VzZU91dCcpHwUFPkNoYW5nZUJvcmRlcignY3RsMDBfRGVmYXVsdENvbnRlbnRfaW1nRW5kRGF0ZScsICdvbk1vdXNlRG93bicpHwYFPENoYW5nZUJvcmRlcignY3RsMDBfRGVmYXVsdENvbnRlbnRfaW1nRW5kRGF0ZScsICdvbk1vdXNlVXAnKR8HBS9TaG93Q2FsZW5kYXIoJ2N0bDAwX0RlZmF1bHRDb250ZW50X3R4dEVuZERhdGUnKR8IBS9TaG93Q2FsZW5kYXIoJ2N0bDAwX0RlZmF1bHRDb250ZW50X3R4dEVuZERhdGUnKWQCAw9kFgRmD2QWAmYPZBYCZg8PFgIeBFRleHQFJzxiPkRhaWx5IFdlYXRoZXIgU3VtbWFyeSBmb3IgQWx0b25hPC9iPmRkAgEPZBYCZg9kFgJmDw8WAh8JBTU8Yj5Gcm9tOiBKYW51YXJ5IDAxLCAyMDE1ICAgVG86IERlY2VtYmVyIDMxLCAyMDEyPC9iPmRkAgUPPCsADQIADxYEHwJnHgtfIUl0ZW1Db3VudGZkDBQrAAcWCB4ETmFtZQUBIB4KSXNSZWFkT25seWgeBFR5cGUZKwIeCURhdGFGaWVsZAUBIBYIHwsFBFRNYXgfDGgfDRkpW1N5c3RlbS5EZWNpbWFsLCBtc2NvcmxpYiwgVmVyc2lvbj0yLjAuMC4wLCBDdWx0dXJlPW5ldXRyYWwsIFB1YmxpY0tleVRva2VuPWI3N2E1YzU2MTkzNGUwODkfDgUEVE1heBYIHwsFBFRNaW4fDGgfDRkrBB8OBQRUTWluFggfCwUEVEF2Zx8MaB8NGSsEHw4FBFRBdmcWCB8LBQNQUFQfDGgfDRkrBB8OBQNQUFQWCB8LBQNHREQfDGgfDRkrBB8OBQNHREQWCB8LBQNDSFUfDGgfDRkrBB8OBQNDSFVkAgcPPCsADQIADxYEHwJnHwpmZAwUKwAHFggfCwUERGF0ZR8MaB8NGSsCHw4FBERhdGUWCB8LBQRUTWF4HwxoHw0ZKwQfDgUEVE1heBYIHwsFBFRNaW4fDGgfDRkrBB8OBQRUTWluFggfCwUEVEF2Zx8MaB8NGSsEHw4FBFRBdmcWCB8LBQNQUFQfDGgfDRkrBB8OBQNQUFQWCB8LBQNHREQfDGgfDRkrBB8OBQNHREQWCB8LBQNDSFUfDGgfDRkrBB8OBQNDSFVkGAIFKWN0bDAwJERlZmF1bHRDb250ZW50JGdkRGFpbHlSZXBvcnRSZXN1bHRzDzwrAAoBCGZkBSpjdGwwMCREZWZhdWx0Q29udGVudCRnZERhaWx5U3VtbWFyeVJlc3VsdHMPPCsACgEIZmTVSc4twRdxMVqAcyvhfogEoVAoXQ%3D%3D&__EVENTVALIDATION=%2FwEWQgKW3rD4BALx5%2BaKCAKvkdK9BAKvka69BAKO%2Ba6hCALr27hqAunbuGoC7tu4agL927hqAvLbuGoCpqOJywkCgLrIyg4CgLrAyg4C6tv0aQK10IiUBgKc3fD9BQLu28BpApzdyP0FAurbyGkC8eeWiwgCjvmioQgCjvmqoQgC6tvcaQLx55KLCALNmpe%2BBwK10ICUBgK7tKvgAwLr28RpAuvbwGkCxprvvQcCpqOxywkCyoiwoAIC69uUagK7tNPgAwK7tK%2FgAwLo28hpAujbxGkC6NvcaQLKiISgAgLKiICgAgLo25hqAsqIjKACAs2am74HAunb%2BGkCr5GivQQCtdCMlAYC6dv0aQKc3fT9BQKc3cT9BQK7tNvgAwKmo7XLCQKc3cz9BQLp28hpAqajjcsJAoC6zMoOAunbxGkCpqO5ywkC8efuiggC7tv0aQLNmu%2B9BwLNmpO%2BBwLu2%2FBpAsqIiKACApiE9P8MAs%2F%2FtpIDAtryq8ECOENE9x0TNYLoEkxglpTY36S14I0%3D&ctl00%24DefaultContent%24cboStationNames=244&ctl00%24DefaultContent%24txtPlantDate=2015-01-01&ctl00%24DefaultContent%24txtEndDate=2015-12-31&ctl00%24DefaultContent%24btn_DRSearch=Submit
}