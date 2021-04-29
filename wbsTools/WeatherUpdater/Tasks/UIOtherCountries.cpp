//
//WARNING: the metadata in the Ireland data file is not good


#include "StdAfx.h"
#include "UIOtherCountries.h"
#include "UIManitoba.h"
#include <boost\dynamic_bitset.hpp>
#include <boost\filesystem.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filter/zlib.hpp>
//#include <boost/iostreams/filter/lzma.hpp>
#include <boost/exception/diagnostic_information.hpp>

 
#include "Basic/Zip.h"
#include "Basic/DailyDatabase.h"
#include "Basic/FileStamp.h"
#include "Basic\CSV.h"
#include "Basic\UtilZen.h"
#include "Basic\json\json11.hpp"
#include "UI/Common/SYShowMessage.h"

#include "TaskFactory.h"
#include "../Resource.h"
#include "WeatherBasedSimulationString.h"
#include "IrelandCounty.h"

#include "UI/Common/UtilWin.h"
#include "Basic/decode_html_entities_utf8.h"

using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace UtilWWW;
using namespace boost;
using namespace json11;
using namespace Partio;


namespace WBSF
{

	const char* CUIOtherCountries::SERVER_NAME[NB_NETWORKS] = { "cli.fusio.net", "www.met.ie" };


	//*********************************************************************
	const char* CUIOtherCountries::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "Network", "Region", "FirstYear", "LastYear", "DataType" };
	const size_t CUIOtherCountries::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_STRING_SELECT, T_STRING_SELECT, T_STRING, T_STRING, T_COMBO_INDEX };
	const UINT CUIOtherCountries::ATTRIBUTE_TITLE_ID = IDS_UPDATER_OTHER_COUNTRY_P;
	const UINT CUIOtherCountries::DESCRIPTION_TITLE_ID = ID_TASK_OTHER_COUNTRY;

	const char* CUIOtherCountries::CLASS_NAME() { static const char* THE_CLASS_NAME = "OtherCountries";  return THE_CLASS_NAME; }
	CTaskBase::TType CUIOtherCountries::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUIOtherCountries::CLASS_NAME(), (createF)CUIOtherCountries::create);


	const char* CUIOtherCountries::NETWORKS_ID[NB_NETWORKS]{ "IEH", "IEC" };
	const char* CUIOtherCountries::NETWORKS_NAME[NB_NETWORKS]{ "Ireland (historical)", "Ireland (current)" };

	size_t CUIOtherCountries::GetNetworkFromName(string name)
	{
		size_t n = NOT_INIT;
		for (size_t i = 0; i < NB_NETWORKS && n == NOT_INIT; i++)
		{
			if (WBSF::IsEqual(name, NETWORKS_ID[i]))
				n = i;
		}

		return n;
	}


	std::bitset<CUIOtherCountries::NB_NETWORKS> CUIOtherCountries::GetNetworks()const
	{
		std::bitset<NB_NETWORKS> networks;

		StringVector str(Get(NETWORKS), "|;,");
		if (str.empty())
		{
			networks.set();
		}
		else
		{
			for (size_t i = 0; i < str.size(); i++)
			{
				size_t n = GetNetworkFromName(str[i]);
				if (n < networks.size())
					networks.set(n);
			}
		}

		return networks;
	}



	CUIOtherCountries::CUIOtherCountries(void)
	{}

	CUIOtherCountries::~CUIOtherCountries(void)
	{}



	std::string CUIOtherCountries::Option(size_t i)const
	{
		string str;
		switch (i)
		{
		case NETWORKS:
		{
			for (size_t n = 0; n < NB_NETWORKS; n++)
				str += to_string(NETWORKS_ID[n]) + "=" + to_string(NETWORKS_NAME[n]) + "|";
			break;
		}
		case IE_COUNTIES: str = CIrelandCounty::GetAllPossibleValue(); break;
		case DATA_TYPE: str = GetString(IDS_STR_DATA_TYPE); break;

		};
		return str;
	}

	std::string CUIOtherCountries::Default(size_t i)const
	{
		string str;

		switch (i)
		{
		case WORKING_DIR: str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()) + "OtherCountries\\"; break;
		case FIRST_YEAR:
		case LAST_YEAR:	str = ToString(CTRef::GetCurrentTRef().GetYear()); break;
		};

		return str;
	}

	//****************************************************
	/*ERMsg CUIOtherCountries::UpdateStationsList(size_t n, StringVector& stationList, CCallback& callback)const
	{*/
	//if (n == IRELAND_CURRENT)
	//{
	//	CInternetSessionPtr pSession;
	//	CHttpConnectionPtr pConnection;

	//	msg = GetHttpConnection("www2.metweb.ie", pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS);
	//	if (msg)
	//	{
	//		string URL = "api/stations/coordinates";
	//		string source;
	//		msg = UtilWWW::GetPageText(pConnection, URL, source);
	//		if (msg)
	//		{
	//			string error;
	//			const Json& root = Json::parse(source, error);
	//			if (error.empty())
	//			{
	//				ASSERT(root.type() == Json::ARRAY);
	//				const std::vector<Json>& stations = root.array_items();

	//				//callback.PushTask(GetString(IDS_LOAD_STATION_LIST) + "" + NETWORKS_NAME[n], stations.size());

	//				for (Json::array::const_iterator it = stations.begin(); it != stations.end() && msg; it++)
	//				{
	//					ASSERT(it->type() == Json::OBJECT);

	//					CLocation location;

	//					location.m_name = PurgeName((*it)["observationsName"].string_value());
	//					location.m_ID = (*it)["ID"].string_value();
	//					location.m_lat = WBSF::as<double>((*it)["lat"].string_value());
	//					location.m_lon = WBSF::as<double>((*it)["lng"].string_value());
	//					//location.m_alt = WBSF::Feet2Meter(WBSF::as<double>((*it)["ELEVATION"].string_value()));

	//					stationList.push_back(location);
	//					//msg += callback.StepIt();
	//				}//for all stations
	//			}
	//			else
	//			{
	//				msg.ajoute(error);
	//				msg.ajoute("JSON error : api/stations/coordinates" );
	//			}

	//			//clean connection
	//			pConnection->Close();
	//			pSession->Close();
	//		}
	//	}//if network
//}

	ERMsg CUIOtherCountries::GetStationList(size_t n, size_t type, StringVector& stationList, CCallback& callback)const
	{
		ASSERT(!m_stations[n].empty());


		ERMsg msg;

		stationList.clear();

		//size_t type = as<int>(DATA_TYPE);
		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		CIrelandCounty counties(Get(IE_COUNTIES));
		ASSERT(firstYear <= lastYear);

		size_t nbYears = lastYear - firstYear + 1;
		CTRef currentTRef = CTRef::GetCurrentTRef();

		if (n == IRELAND_HISTORICAL)
		{
			for (size_t i = 0; i < m_stations[n].size(); i++)
			{
				int open_year = ToInt(m_stations[n][i].GetSSI("open_year"));
				int close_year = ToInt(m_stations[n][i].GetSSI("close_year"));
				
				if (close_year == -999)
					close_year = currentTRef.GetYear();

				ASSERT(open_year <= close_year);

				if (!((lastYear < open_year) ||
					(firstYear > close_year)))
				{
					size_t county = WBSF::as<size_t>(m_stations[n][i].GetSSI("countyno")) - 1;
					if (counties[county])
					{
						StringVector data_type(m_stations[n][i].GetSSI("data_types"), "|");
						if (type == DAILY_WEATHER || data_type.size() == 2)
							stationList.push_back(m_stations[n][i].m_ID);
					}
				}
				
			}
		}
		else if (n == IRELAND_CURRENT)
		{

			if (type == HOURLY_WEATHER)
			{
				for (size_t i = 0; i < m_stations[n].size(); i++)
				{
					string yesterday_name = TrimConst(m_stations[n][i].GetSSI("slug"));
					if (!yesterday_name.empty())
					{
						stationList.push_back(m_stations[n][i].m_ID);
					}
				}
			}
			else
			{
				stationList.push_back("all_stations");
			}
		}


		return msg;
	}


	ERMsg CUIOtherCountries::Execute(CCallback& callback)
	{
		ERMsg msg;

		size_t type = as<int>(DATA_TYPE);
		std::bitset<NB_NETWORKS> networks = GetNetworks();

		for (size_t n = 0; n < networks.size() && msg; n++)
		{
			if (networks[n])
			{
				switch (n)
				{
				case IRELAND_HISTORICAL:
				case IRELAND_CURRENT: msg += ExecuteIreland(n, type, callback); break;
				default: ASSERT(false);
				}
			}
		}


		return msg;
	}

	ERMsg CUIOtherCountries::ExecuteIreland(size_t n, size_t type, CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);
		msg = CreateMultipleDir(workingDir);

		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(workingDir, 1);
		callback.AddMessage(GetString(IDS_UPDATE_FROM));
		callback.AddMessage(string(SERVER_NAME[n]), 1);
		callback.AddMessage("");

		

		if (m_stations[n].empty())
		{
			msg = m_stations[n].Load(GetStationListFilePath(n));
			//update lat, lon and elevation
			//for (size_t i = 0; i < m_stations[n].size(); i++)
			//{
			//	string filePath = GetOutputFilePath(n, m_stations[n][i].m_ID, -999, DAILY_WEATHER);
			//	if (!FileExists(filePath))
			//		filePath = GetOutputFilePath(n, m_stations[n][i].m_ID, -999, HOURLY_WEATHER);

			//	if (FileExists(filePath))
			//	{
			//		ifStream file;
			//		msg = file.open(filePath, ios_base::in | ios_base::binary);
			//		if (msg)
			//		{
			//			boost::iostreams::filtering_istreambuf in;
			//			in.push(boost::iostreams::gzip_decompressor());
			//			in.push(file);
			//			std::istream incoming(&in);
			//			
			//			string name, coord, alt;
			//			std::getline(incoming, name);
			//			std::getline(incoming, alt);
			//			std::getline(incoming, coord);
			//			StringVector tmp(coord, ":, ");
			//			StringVector tmp2(WBSF::TrimConst(WBSF::FindString(alt, ":", "M")), " ).");
			//			StringVector tmp3(name, ":");


			//			if(tmp.size() == 4 && tmp2.size()==1 && tmp2[0][0] != 'S'&& tmp3.size()==2)
			//			{
			//				m_stations[n][i].SetSSI("BD_Name", tmp3[1]);
			//				m_stations[n][i].SetSSI("BD_Lat", tmp[1]);
			//				m_stations[n][i].SetSSI("BD_Lon",tmp[3]);
			//				m_stations[n][i].SetSSI("BD_Elev", tmp2[0]);
			//			}
			//			
			//		}
			//	}
			//}//for all stations
			
			//return m_stations[n].Save( GetStationListFilePath(n) + "2");

		}

		StringVector stationList;
		if(msg)
			msg = GetStationList(n, type, stationList, callback);

		if (!msg)
			return msg;


		stationList = CleanIrelandList(n, type, stationList);
		

		callback.PushTask("Download Ireland data (" + ToString(stationList.size()) + " files)", stationList.size());

		int nbFiles = 0;
		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;

		msg = GetHttpConnection(SERVER_NAME[n], pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", false, 5, callback);
		if (msg)
		{
			for (size_t i = 0; i < stationList.size() && msg; i++)
			{
				string str;
				msg = DownloadIrelandStation(pConnection, n, type, stationList[i], callback);
				if (msg)
				{
					nbFiles++;
					msg += callback.StepIt();
				}
			}//for all files

			//clean connection
			pConnection->Close();
			pSession->Close();
		}//if msg



		callback.AddMessage(GetString(IDS_NB_FILES_DOWNLOADED) + ToString(nbFiles), 1);
		callback.PopTask();

		return msg;
	}

	ERMsg CUIOtherCountries::DownloadIrelandStation(UtilWWW::CHttpConnectionPtr& pConnection, size_t n, size_t type, const std::string& ID, CCallback& callback)
	{
		ERMsg msg;


		if (n == IRELAND_HISTORICAL)
		{
			size_t pos = m_stations[n].FindByID(ID);
			ASSERT(pos != NOT_INIT);

			int close_year = ToInt(m_stations[n][pos].GetSSI("close_year"));
			string URL = string("cli/climate_data/webdata") + (close_year == -999 ? "/" : "c/") + ((type == HOURLY_WEATHER) ? "hly" : "dly") + ID + ".zip";

			string outputFilePath = GetOutputFilePath(n, type, ID);
			string outputPath = GetPath(outputFilePath);
			CreateMultipleDir(outputPath);
			string fileTitle = GetFileTitle(outputFilePath);
			string filePathZip = outputPath + fileTitle + ".zip";
			string filePathData = outputPath + fileTitle + ".csv";

			msg = UtilWWW::CopyFile(pConnection, URL, filePathZip, INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_TRANSFER_BINARY);
			if (msg)
			{
				//unzip only .csv file because they are smaller than the zip file
				DWORD exitCode = 0;
				string command = GetApplicationPath() + "External\\7za.exe e \"" + filePathZip + "\" -y -o\"" + outputPath + "\"" + " \"" + GetFileName(filePathData) + "\"";
				msg = WinExecWait(command, outputPath, SW_HIDE, &exitCode);

				if (msg && exitCode != 0)
					callback.AddMessage("7za.exe was unable to unzip station with ID" + ID);

				msg += WBSF::RemoveFile(filePathZip);
				if (msg)
				{
					//now rezip the file
					command = GetApplicationPath() + "External\\7za.exe a -tgzip \"" + outputFilePath + "\" \"" + filePathData + "\"";
					msg = WinExecWait(command, outputPath, SW_HIDE, &exitCode);

					if (msg && exitCode != 0)
						callback.AddMessage("7za.exe was unable to zip file " + filePathData);

					msg += WBSF::RemoveFile(filePathData);
				}
			}
		}
		else if (n == IRELAND_CURRENT)
		{
			//Latest Weather Reports on 06-May-2018 FOR 15:00</h2>
			string src;
			msg = UtilWWW::GetPageText(pConnection, "latest-reports/observations", src);
			string strDate = FindString(src, "Latest Weather Reports on", "FOR");
			StringVector date(strDate, "-");
			if (date.size() == 3)
			{
				size_t day = ToSizeT(date[0]) - 1;
				size_t month = WBSF::GetMonthIndex(date[1].c_str());
				int year = ToInt(date[2]);

				CTRef TRef = CTRef(year, month, day);

				string tmpFilePath = GetDir(WORKING_DIR) + NETWORKS_NAME[n] + "\\tmp.csv";
				CreateMultipleDir(GetPath(tmpFilePath));

				if (type == HOURLY_WEATHER)
				{
					size_t pos = m_stations[n].FindByID(ID);
					ASSERT(pos != NOT_INIT);
					string name = TrimConst(m_stations[n][pos].GetSSI("slug"));
					ASSERT(!name.empty());

					//download yesterday
					string URL = "latest-reports/observations/download/" + name + "/yesterday";
					msg = UtilWWW::CopyFile(pConnection, URL, tmpFilePath, INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_TRANSFER_ASCII);
					if (msg)
					{
						string filePath = GetOutputFilePath(n, type, ID);
						CreateMultipleDir(GetPath(filePath));

						msg = MergeCurrentIrelandHourly(TRef - 1, m_stations[n][pos].m_elev, tmpFilePath, filePath, callback);
						if (msg)
						{

							//downlaod today
							string URL = "latest-reports/observations/download/" + name;
							msg = UtilWWW::CopyFile(pConnection, URL, tmpFilePath, INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_TRANSFER_ASCII);
							if (msg)
							{
								string filePath = GetOutputFilePath(n, type, ID);
								CreateMultipleDir(GetPath(filePath));

								msg = MergeCurrentIrelandHourly(TRef, m_stations[n][pos].m_elev, tmpFilePath, filePath, callback);
							}
						}
						WBSF::RemoveFile(tmpFilePath);
					}
				}
				else if (type == DAILY_WEATHER)
				{
					//download yesterday
					string URL = "latest-reports/observations/yesterday/download";
					msg = UtilWWW::CopyFile(pConnection, URL, tmpFilePath, INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_TRANSFER_ASCII);
					if (msg)
					{
						msg = SplitCurrentIrelandDaily (TRef - 1,tmpFilePath, callback);
						WBSF::RemoveFile(tmpFilePath);
					}
				}
			}//if valid date
		}



		return msg;
	}


	ERMsg CUIOtherCountries::MergeCurrentIrelandHourly(CTRef TRef, double elev, const string& inputFilePath, const string& outputFilePath, CCallback& callback)
	{
		ERMsg msg;

		TRef.Transform(CTM(CTM::HOURLY));

		ifStream file;
		msg = file.open(inputFilePath);
		if (msg)
		{
			CWeatherYears data(true);
			data.LoadData(outputFilePath);


			enum THourlyColumns { C_TIME, C_REPORT, C_TEMPERATURE, C_WIND_SPEED, C_WIND_GUST, C_WIND_DIRECTION, C_HUMIDITY, C_RAINFALL, C_PRESSURE, NB_COLUMNS };
			static const WBSF::HOURLY_DATA::TVarH COL_POS_H[NB_COLUMNS] = { H_SKIP, H_SKIP, H_TAIR,  H_WNDS, H_SKIP, H_WNDD, H_RELH, H_PRCP, H_PRES };

			for (CSVIterator loop(file, ",", true, true); loop != CSVIterator() && msg; ++loop)
			{
				if (loop->size() == NB_COLUMNS)
				{
					StringVector time((*loop)[C_TIME], "-: ");
					ASSERT(time.size() == 2);

					TRef.m_hour = ToInt(time[0]);
					ASSERT(TRef.m_hour < 24);

					for (size_t v = 0; v < loop->size(); v++)
					{
						TVarH variable = COL_POS_H[v];
						if (variable != H_SKIP)
						{
							if ((*loop)[v] != "X" && (*loop)[v] != "n/a")
							{
								double value = ToDouble((*loop)[v]);
								if (variable == H_PRES)//wind speed here is in km/h
									value = Convert(variable, value, elev, true);

								data[TRef].SetStat(variable, value);

								if (variable == H_RELH)
								{
									double T = data[TRef][H_TAIR][MEAN];
									if (T != -999)
										data[TRef].SetStat(H_TDEW, WBSF::Hr2Td(T, value));
								}
							}
						}
					}
				}//empty

				msg += callback.StepIt(0);
			}//for all line (

			if (msg)
			{
				//save data
				CreateMultipleDir(GetPath(outputFilePath));
				msg = data.SaveData(outputFilePath);
			}//if msg
		}//if msg

		return msg;
	}

	ERMsg CUIOtherCountries::SplitCurrentIrelandDaily(CTRef TRef, const string& inputFilePath, CCallback& callback)
	{
		ERMsg msg;


		ifStream file;
		msg = file.open(inputFilePath);
		if (msg)
		{

			//Station,"Rain (mm)","Max Temperature (ºC)","Min Temperature (ºC)","Sun (hr)","Wind Gust (Kts)","Wind Speed (Kts)","Soil (ºC)","Global (J/cm^2)","Gmin (ºC)"
			
			enum THourlyColumns { STATION, RAIN, MAX_TEMPERATURE, MIN_TEMPERATURE, SUN, WIND_GUST, WIND_SPEED, SOIL, GLOBAL, GMIN, NB_COLUMNS };
			static const WBSF::HOURLY_DATA::TVarH COL_POS_H[NB_COLUMNS] = { H_SKIP, H_PRCP, H_TMAX,  H_TMIN,  H_SKIP, H_SKIP, H_WNDS, H_SKIP, H_SRAD, H_SKIP };

			for (CSVIterator loop(file, ",", true, true); loop != CSVIterator() && msg; ++loop)
			{
				if (loop->size() == NB_COLUMNS)
				{
					string name = (*loop)[STATION];

					CLocationVector::const_iterator it = std::find_if(m_stations[IRELAND_CURRENT].begin(), m_stations[IRELAND_CURRENT].end(), FindLocationByName(name));
					if (it!= m_stations[IRELAND_CURRENT].end())
					{
						
						string outputFilePath = GetOutputFilePath(IRELAND_CURRENT, DAILY_WEATHER, it->m_ID);

						CWeatherYears data(false);
						data.LoadData(outputFilePath);

						for (size_t v = 0; v < loop->size(); v++)
						{
							TVarH variable = COL_POS_H[v];
							if (variable != H_SKIP)
							{
								if ((*loop)[v] != "X" && (*loop)[v] != "n/a")
								{
									double value = ToDouble((*loop)[v]);
									value = Convert(variable, value, -999, false);
									data[TRef].SetStat(variable, value);
								}
							}
						}

						
						//save data
						CreateMultipleDir(GetPath(outputFilePath));
						msg = data.SaveData(outputFilePath);
						
					}//know station
					else
					{
						callback.AddMessage("unknown station " + name);
					}
				}//good columns size
				msg += callback.StepIt(0);
			}//for all line (

			
		}//if msg

		return msg;
	}
	string CUIOtherCountries::GetOutputFilePath(size_t n, size_t type, const string& ID, int year)const
	{
		string fileName;

		if (n == IRELAND_HISTORICAL)
			fileName = GetDir(WORKING_DIR) + NETWORKS_NAME[n] + "\\" + ((type == HOURLY_WEATHER) ? "hourly\\hly" : "daily\\dly") + ID + ".gz";
		if (n == IRELAND_CURRENT)
			fileName = GetDir(WORKING_DIR) + NETWORKS_NAME[n] + "\\" + ((type == HOURLY_WEATHER) ? "hourly\\hly" : "daily\\dly") + ID + ".csv";

		return fileName;
	}

	std::string CUIOtherCountries::GetStationListFilePath(size_t n)const
	{
		ASSERT(n < NB_NETWORKS);

		static const char* FILE_NAME[NB_NETWORKS] = { "IrelandStations.csv", "IrelandStations.csv" };
		//if (n == FIRE_HISTORICAL)
			//return GetDir(WORKING_DIR) + NETWORK_NAME[FIRE] + "\\" + FILE_NAME[n];

		return WBSF::GetApplicationPath() + "Layers\\" + FILE_NAME[n];
	}

	ERMsg CUIOtherCountries::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;

		
		size_t type = as<int>(DATA_TYPE);
		std::bitset<NB_NETWORKS> networks = GetNetworks();

		for (size_t n = 0; n < networks.size() && msg; n++)
		{
			if (!m_stations[n].empty())
				m_stations[n].clear();

			if (networks[n])
			{
				msg = m_stations[n].Load(GetStationListFilePath(n));

				if (msg)
				{
					StringVector stationListTmp;
					GetStationList(n, type, stationListTmp, callback);

					for (size_t i = 0; i < stationListTmp.size(); i++)
						stationList.push_back(to_string(n) + "/" + stationListTmp[i]);
				}
			}
		}
		 
		//if (msg)
			//msg += m_stations.IsValid();

		return msg;
	}

	StringVector CUIOtherCountries::CleanIrelandList(size_t n, size_t type, const StringVector& stationList)
	{
		StringVector out;

		
		if (!(n == IRELAND_CURRENT && type == DAILY_WEATHER) )
		{
			out.reserve(stationList.size());
			for (size_t ii = 0; ii < stationList.size(); ii++)
			{

				size_t pos = m_stations[n].FindByID(stationList[ii]);
				ASSERT(pos != NOT_INIT);

				int close_year = ToInt(m_stations[n][pos].GetSSI("close_year"));
				string filePath = GetOutputFilePath(n, type, m_stations[n][pos].m_ID, -999);
				if (close_year == -999 || !FileExists(filePath))
					out.push_back(stationList[ii]);
			}

			
		}
		else
		{
			out = stationList;
		}
		
		return out;
	}

	ERMsg CUIOtherCountries::GetWeatherStation(const std::string& IDin, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;

		StringVector tmp(IDin, "/");
		ASSERT(tmp.size() == 2);
		
		size_t n = WBSF::as<size_t>(tmp[0]);
		string ID = tmp[1];

		
		//Get station information
		size_t it = m_stations[n].FindByID(ID);
		ASSERT(it != NOT_INIT);


		if (it == NOT_INIT)
		{
			msg.ajoute(FormatMsg(IDS_NO_STATION_INFORMATION, ID));
			return msg;
		}

		size_t type = as<int>(DATA_TYPE);


		((CLocation&)station) = m_stations[n][it];
		station.m_name = PurgeFileName(station.m_name);
		station.SetHourly(type == HOURLY_WEATHER);


		if (n == IRELAND_HISTORICAL)
		{
			string filePath = GetOutputFilePath(n, type, ID);
			if (FileExists(filePath))
				msg = ReadIrelandData(filePath, station, callback);
		}
		else if (n == IRELAND_CURRENT)
		{
			//now extract data 
			string filePath = GetOutputFilePath(n, type, ID);
			if (FileExists(filePath))
			{
				station.LoadData(filePath);
			}
		}

		//verify station is valid
		if (msg && station.HaveData())
		{
			//verify station is valid
			msg = station.IsValid();
		}

		station.SetSSI("Provider", "Irland");
		station.SetSSI("Network", "Irland");
		

		return msg;
	}



	//date, ind, rain
	//Time,Report,Temperature,"Wind Speed","Wind Gust","Wind Direction",Humidity,Rainfall,Pressure

	/*date:   -00 to 00 utc
	rain : -Precipitation Amount(mm)
	maxtp : -Maximum Air Temperature(C)
	mintp : -Minimum  Air Temperature(C)
	gmin : -09utc Grass Minimum Temperature(C)
	soil:   -Mean 10cm Soil Temperature(C)
	wdsp : -Mean Wind Speed(knot)
	hm : -Highest ten minute mean wind speed(knot)
	ddhm : -Wind Direction at max 10 min.mean(deg)
	hg : -Highest Gust(knot)
	cbl : -Mean CBL Pressure(hpa)
	sun : -Sunshine duration(hours)
	g_rad : -Global Radiation(j / cm sq.)
	pe : -Potential Evapotranspiration(mm)
	evap : -Evaporation(mm)
	smd_wd : -Soil Moisture Deficits(mm) well drained
	smd_md : -Soil Moisture Deficits(mm) moderately drained
	smd_pd : -Soil Moisture Deficits(mm) poorly drained
	ind : -Indicator(i)

	rain:  -  Precipitation Amount (mm)
	temp:  -  Air Temperature (C)
	wetb:  -  Wet Bulb Temperature (C)
	dewpt: -  Dew Point Temperature (C)
	rhum:  -  Relative Humidity (%)
	vappr: -  Vapour Pressure (hPa)
	msl:   -  Mean Sea Level Pressure (hPa)
	wdsp:  -  Mean Wind Speed (knot)
	wddir: -  Predominant Wind Direction (degree)
	ww:    -  Synop code for Present Weather
	w:     -  Synop code for Past Weather
	sun:   -  Sunshine duration (hours)
	vis:   -  Visibility (m)
	clht:  -  Cloud height (100's of ft) - 999 if none
	clamt: -  Cloud amount
	ind:   -  Indicator

*/

	enum TIrelandColumns { C_DATE, C_RAIN, C_MAXT, C_MAXTP, C_MINT, C_MINTP, C_IGMIN, C_GMIN, C_SOIL, C_WDSP, C_HM, C_DDHM, C_HG, C_CBL, C_SUN, C_G_RAD, C_PE, C_EVAP, C_SMD_WD, C_SMD_MD, C_SMD_PD, C_IND, C_TEMP, C_WETB, C_DEWPT, C_RHUM, C_VAPPR, C_MSL, C_WDDIR, C_WW, C_W, C_VIS, C_CLHT, C_CLAMT, C_GLORAD, C_DOS, NB_COLUMNS };
	static const char* COLUMN_NAME[NB_COLUMNS] = { "Date", "rain","maxt","maxtp","mint","mintp","igmin","gmin","soil","wdsp","hm","ddhm","hg","cbl","sun","g_rad","pe","evap","smd_wd","smd_md","smd_pd","ind", "temp","wetb","dewpt","rhum","vappr","msl","wddir","ww","w","vis","clht","clamt", "glorad", "dos",  };
	static HOURLY_DATA::TVarH COLUMN_VARIABLE[NB_COLUMNS] = { H_SKIP, H_PRCP, H_TMAX, H_TMAX, H_TMIN, H_TMIN, H_SKIP, H_SKIP,H_SKIP,H_WNDS,H_SKIP,H_SKIP,H_SKIP,H_SKIP,H_SKIP,H_SRAD,H_SKIP,H_SKIP,H_SKIP,H_SKIP,H_SKIP,H_SKIP,H_TAIR, H_SKIP, H_TDEW, H_RELH, H_SKIP, H_PRES, H_WNDD, H_SKIP, H_SKIP, H_SKIP, H_SKIP, H_SKIP, H_SRAD, H_SKIP };

	size_t CUIOtherCountries::GetIrelandColumn(const string& header)
	{
		size_t c = NOT_INIT;
		for (size_t i = 0; i < NB_COLUMNS&&c == NOT_INIT; i++)
		{
			if (IsEqual(header, COLUMN_NAME[i]))
				c = i;
		}

		ASSERT(c != NOT_INIT);

		return c;
	}

	vector<size_t> CUIOtherCountries::GetIrelandColumns(const StringVector& header)
	{
		vector<size_t> columns(header.size());

		for (size_t c = 0; c < header.size(); c++)
			columns[c] = GetIrelandColumn(header[c]);

		return columns;
	}

	double CUIOtherCountries::Convert(HOURLY_DATA::TVarH v, double value, double elev, bool bHourly)
	{
		switch (v)
		{
			//knot
			//1 Knot = 1 Nautical Mile per hour
			//1 Nautical mile = 6076.12 ft. = 1852.184256 (m) = 1.852184256 (km)
		case H_WNDS: value = value * 1.852184256; break;//knok --> km/h
			//joule/cm² = watt·s/cm² = 10000·w·s/m² = 10000/3600·w·h/m²
			//1 m² = 10000 cm²
		case H_SRAD: value = value * 10000 / ((bHourly ? 1 : 24) * 60 * 60); break;	//j/cm² --> w·h/m²
		case H_PRES: value = WBSF::msl2atp(value, elev); break;	//MSL --> station pressure
		default:;//do nothing
		}


		return value;
	}

	ERMsg CUIOtherCountries::ReadIrelandData(const string& filePath, CWeatherStation& station, CCallback& callback)const
	{
		ASSERT(filePath.find("hly") != string::npos || filePath.find("dly") != string::npos);

		ERMsg msg;

		bool bHourly = filePath.find("hly") != string::npos;
		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);

		//now extact data 
//		ifStream file;
	//	msg = file.open(filePath);
		try
		{
			ifStream file;
			msg = file.open(filePath, ios_base::in | ios_base::binary);
			if (msg)
			{
				boost::iostreams::filtering_istreambuf in;
				in.push(boost::iostreams::gzip_decompressor());
				in.push(file);
				std::istream incoming(&in);

				//double elev = station.m_elev;
				vector<TVarH> variables;

				string line;
				while (std::getline(incoming, line))//read header lines
				{
					/*
					WARNING: the metadata in the file is corrupted for many stations (~150}
					
					if (line.find("Station Height") != string::npos)
					{
						string tmp = WBSF::TrimConst(WBSF::FindString(line, ":", "M"));
						ASSERT(!tmp.empty());
						station.m_elev = WBSF::as<double>(tmp);
					}
					*/

					if (line.find("date,") != string::npos)
					{
						StringVector head(line, ",");
						vector<size_t> columns = GetIrelandColumns(head);
						variables.resize(columns.size(), H_SKIP);
						for (size_t i = 0; i < columns.size(); i++)
						{
							if (columns[i] != NOT_INIT)
								variables[i] = COLUMN_VARIABLE[columns[i]];
							else 
								callback.AddMessage("Unknown variable : " + head[i] + " (" + GetFileTitle(filePath) +")");
							
						}
						break;
					}

					//pos = incoming.tellg();
				}
				ASSERT(!variables.empty());

				for (CSVIterator loop(incoming, ",", false); loop != CSVIterator() && msg; ++loop)
				{
					ASSERT(loop->size() <= variables.size());
					if (!loop->empty())
					{
						StringVector date((*loop)[C_DATE], " -:");
						if (date.size() >= 3)
						{
							size_t day = ToSizeT(date[0]) - 1;
							size_t month = WBSF::GetMonthIndex(date[1].c_str());
							int year = ToInt(date[2]);
							if (year >= firstYear && year <= lastYear)
							{
								CTRef TRef = CTRef(year, month, day);

								//if it's hourly data, read alod hour
								if (date.size() == 5)
								{
									size_t hour = ToSizeT(date[3]);
									TRef = CTRef(year, month, day, hour);
								}
								ASSERT(TRef.IsValid()); 
								ASSERT(loop->size() == variables.size());
								for (size_t c = 0; c < loop->size() && msg; c++)
								{
									if (variables[c] != NOT_INIT)
									{
										string str = TrimConst((*loop)[c]);
										if (!str.empty())
										{
											double value = ToDouble(str);
											value = Convert(variables[c], value, station.m_elev, bHourly);

											station[TRef].SetStat(variables[c], value);
										}

										msg += callback.StepIt(0);
									}//if it's a supported variable
								}//for all variable
							}//valid year
						}//if valid date
					}//line not empty

					msg += callback.StepIt(0);
				}//for all lines
			}//if load 
		}
		catch (std::exception& e)
		{
			msg.ajoute(e.what());
		}
		//catch (io::lzma_error const& e) {
		//	std::cout << boost::diagnostic_information(e, true);
		//	std::cout << e.code() << ": " << e.code().message() << "\n";
		//}
		//catch (boost::exception const& e) {
		//	std::cout << boost::diagnostic_information(e, true);
		//}

		return msg;
	}

}

