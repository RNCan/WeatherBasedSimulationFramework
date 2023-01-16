#include "StdAfx.h"
#include "EnvCanHourlyForecast.h"
#include "Basic/WeatherStation.h"
#include "Geomatic/ShapeFileBase.h"
#include "UI/Common/UtilWWW.h"
#include "UI/Common/SYSHowMessage.h"
#include "TaskFactory.h"

#include "WeatherBasedSimulationString.h"
#include "../Resource.h"



using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace UtilWWW;

namespace WBSF
{



	CRegionArray CRegionSelection::REGION_NAME;

	CRegionSelection::CRegionSelection(const string& bits) :std::bitset <NB_REGIONS>(bits)
	{
		static const char* REGION_ABVR[NB_REGIONS] = { "atl", "ont", "pnr", "pyr", "que" };
		StringVector name(IDS_REGION_NAME, "|;");
		ASSERT(name.size() == REGION_NAME.size());
		for (size_t i = 0; i < size(); i++)
		{
			REGION_NAME[i].first = name[i];
			REGION_NAME[i].second = REGION_ABVR[i];
		}
	}

	string CRegionSelection::GetString()const
	{
		string str;
		for (size_t i = 0; i < size(); i++)
		{
			if (test(i))
			{
				if (!str.empty())
					str += ";";
				str += GetName(i, true);
			}

		}

		return str;
	}

	void CRegionSelection::SetString(std::string str)
	{
		reset();

		StringVector list(str, ";");
		for (StringVector::const_iterator it = list.begin(); it != list.end(); it++)
		{
			size_t pos = GetRegion(*it, true);
			if (pos != UNKNOWN_POS)
				set(pos);
		}

	}

	std::string CRegionSelection::GetAllPossibleValue(bool bAbvr, bool bName)
	{
		string str;

		static const char* REGION_ABVR[NB_REGIONS] = { "atl", "ont", "pnr", "pyr", "que" };
		StringVector title(IDS_REGION_NAME, "|;");
		ASSERT(title.size() == REGION_NAME.size());
		for (size_t i = 0; i < REGION_NAME.size(); i++)
		{
			str += i != 0 ? "|" : "";
			if (bAbvr)
				str += REGION_ABVR[i];
			if (bAbvr && bName)
				str += "=";
			if (bName)
				str += title[i];
		}

		return str;
	}

	const char* CEnvCanHourlyForecast::SERVER_NAME = "dd.weather.gc.ca";
	const char* CEnvCanHourlyForecast::SERVER_PATH = "/meteocode/";

	//*********************************************************************


	//autre forecast quotidienne avec plus de jour et la déviation
	//https://weather.gc.ca/ensemble/naefs/EPSgrams_e.html?station=YOW

	//forecast raw wind speed
	//http://dd.weather.gc.ca/ensemble/naefs/grib2/raw/00/000/



	CEnvCanHourlyForecast::CEnvCanHourlyForecast(void) :
		m_pShapefile(NULL),
		m_bAlwaysCreate(true)
	{}


	CEnvCanHourlyForecast::~CEnvCanHourlyForecast(void)
	{
		if (m_pShapefile)
			delete m_pShapefile;

		m_pShapefile = NULL;
	}

	std::string CEnvCanHourlyForecast::GetStationListFilePath()const
	{
		return GetApplicationPath() + "Layers\\ForecastZones.csv";
	}

	std::string CEnvCanHourlyForecast::GetShapefileFilePath()const
	{
		return GetApplicationPath() + "Layers\\ForecastZones.shp";
	}

	std::string CEnvCanHourlyForecast::GetDatabaseFilePath()const
	{
		return m_workingDir + "ForecastDB" + CHourlyDatabase::DATABASE_EXT;
	}


	//***********************************************************************************************************************
	// download section
	CTRef ConvertTime(string str)
	{
		CTRef TRef;
		if (str.length() > 13)
		{
			int year = ToInt(str.substr(0, 4));
			int month = ToInt(str.substr(5, 2)) - 1;
			int day = ToInt(str.substr(8, 2)) - 1;
			int hour = ToInt(str.substr(11, 2));
			TRef = CTRef(year, month, day, hour);
		}

		return TRef;
	}

	ERMsg CEnvCanHourlyForecast::LoadStationInformation(const string& outputFilePath, CLocation& station)
	{
		ERMsg msg;

		zen::XmlDoc doc;

		msg = load(outputFilePath, doc);
		if (msg)
		{
			zen::XmlIn in(doc.root());

			string begin;
			string end;
			in["head"]["product"]["valid-begin-time"](begin);
			in["head"]["product"]["valid-end-time"](end);

			CTRef TRef1 = ConvertTime(begin);
			CTRef TRef2 = ConvertTime(end);
			ASSERT(TRef1.IsInit());
			ASSERT(TRef2.IsInit());
			int nbHours = TRef2 - TRef1;
		}

		return msg;
	}

	//******************************************************
	class MostRecentFirst
	{
	public:

		inline bool operator() (const CFileInfo& info1, const CFileInfo& info2)
		{
			return (info1.m_time > info2.m_time);
		}

	};

	void ClearList(CFileInfoVector& fileList)
	{

		set<string> IDs;

		std::sort(fileList.begin(), fileList.end(), MostRecentFirst());

		for (CFileInfoVector::const_iterator it = fileList.begin(); it != fileList.end();)
		{
			string fileName = GetFileName(it->m_filePath); ASSERT(fileName.length() >= 26);
			string ID = fileName.substr(9, 6);
			if (IDs.find(ID) == IDs.end())
			{
				IDs.insert(ID);
				it++;
			}
			else
			{
				it = fileList.erase(it);
			}
		}
	}



	ERMsg CEnvCanHourlyForecast::Execute(CCallback& callback)
	{
		ERMsg msg;

		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(m_workingDir, 1);
		callback.AddMessage(GetString(IDS_UPDATE_FROM));
		callback.AddMessage(SERVER_NAME, 1);
		callback.AddMessage("");

		callback.AddMessage(GetString(IDS_UPDATE_FILE));
		//callback.AddTask(m_regions.count());


		std::time_t now = std::time(0);

		CHourlyDatabase().DeleteDatabase(GetDatabaseFilePath(), callback);

		msg = m_stations.Load(GetStationListFilePath());

		if (!msg)
			return msg;

		callback.PushTask("Download MeteoCode (" + to_string(m_regions.size()) + " regions)", m_regions.size());

		int nbDownload = 0;


		for (size_t i = 0; i < m_regions.size() && msg; i++)
		{
			if (m_regions.any() && !m_regions[i])
				continue;

			string region = m_regions.GetName((int)i, true);
			string URL = SERVER_PATH + region + "/cmml/";
			string outputPath = m_workingDir + region + "/";

			StringVector filesList = GetFilesList(outputPath + "TRANSMIT.*.xml");
			for (StringVector::const_iterator it = filesList.begin(); it != filesList.end(); it++)
			{
				CFileInfo info = WBSF::GetFileInfo(*it);
				//keep forecast to fill gap between observation and forecast
				__int64 hours = (now - info.m_time) / 3600;
				if (hours > 7 * 24)//keep file one week
					msg += RemoveFile(*it);
			}



			if (msg)
			{
				CFileInfoVector fileList;

				//open a connection on the server
				//CInternetSessionPtr pSession;
				//CHttpConnectionPtr pConnection;
				//msg = GetHttpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", false, 5, callback);

				if (msg)
				{
					//pSession->SetOption(INTERNET_OPTION_RECEIVE_TIMEOUT, 15000);

					//Load files list
					//size_t nbTry = 0;
					//while (fileList.empty() && msg)
					//{
						//nbTry++;

						//try
						//{
					string remotePath = string("https://") + SERVER_NAME + "/" + URL + "TRANSMIT.*.xml";
					msg += UtilWWW::FindFilesCurl(remotePath, fileList);

					//}
					//catch (CException* e)
					//{
						//if (nbTry < 5)
						//{
						//	callback.AddMessage(UtilWin::SYGetMessage(*e));
						//	msg = WaitServer(10, callback);
						//}
						//else
						//{
						//	msg = UtilWin::SYGetMessage(*e);
						//}
					//}
				//}

					ClearList(fileList);
					CreateMultipleDir(outputPath);
					//pConnection->Close();
					//pSession->Close();
				}


				if (msg)
				{
					callback.PushTask(region + " (" + to_string(fileList.size()) + " files)", fileList.size());
					callback.AddMessage(region + ", " + GetString(IDS_NUMBER_FILES) + ToString(fileList.size()), 1);

					//Download files
					CFileInfoVector::iterator it = fileList.begin();

					size_t nbTry = 0;
					while (it != fileList.end() && msg)
					{

						nbTry++;

						//CInternetSessionPtr pSession;
						//CHttpConnectionPtr pConnection;
						//msg = GetHttpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", false, 5, callback);

						if (msg)
						{
							//pSession->SetOption(INTERNET_OPTION_RECEIVE_TIMEOUT, 15000);

							//try
							//{
							while (it != fileList.end() && msg)
							{
								string fileName = GetFileName(it->m_filePath);
								string ID = fileName.substr(0, 8);
								string outputFilePath = outputPath + fileName;

								msg += UtilWWW::CopyFileCurl(it->m_filePath, outputFilePath);
								if (msg)
								{
									ASSERT(FileExists(outputFilePath));
									it++;
									nbDownload++;
									nbTry = 0;
									msg += callback.StepIt();
								}
							}
							//}
							//catch (CException* e)
							//{
							//	if (nbTry < 5)
							//	{
							//		callback.AddMessage(UtilWin::SYGetMessage(*e));
							//		msg = WaitServer(10, callback);
							//	}
							//	else
							//	{
							//		msg = UtilWin::SYGetMessage(*e);
							//	}
							//}

							//pConnection->Close();
							//pSession->Close();

						}
					}
					callback.PopTask();
					msg += callback.StepIt();



				}//if msg
			}//if msg

		}//for all province

		callback.AddMessage(GetString(IDS_NB_FILES_DOWNLOADED) + ToString(nbDownload), 2);
		callback.PopTask();


		if (msg || m_bAlwaysCreate)
		{
			if (!msg)
			{
				callback.AddMessage(msg);
				msg = ERMsg();
			}

			callback.PushTask("Create forecast database (" + to_string(m_regions.size()) + " regions)", m_regions.size());

			//Create only one database for all forecast
			CHourlyDatabase DB;
			msg = DB.Open(GetDatabaseFilePath(), CHourlyDatabase::modeWrite, callback);
			if (msg)
			{
				for (size_t i = 0; i < m_regions.size() && msg; i++)
				{
					if (m_regions.any() && !m_regions[i])
						continue;

					CWeatherStationVector stations;
					string region = m_regions.GetName((int)i, true);
					string outputPath = m_workingDir + region + "/";


					CFileInfoVector filesInfo;
					GetFilesInfo(outputPath + "TRANSMIT.*.xml", false, filesInfo);
					sort(filesInfo.begin(), filesInfo.end(), [](const CFileInfo& lhs, const CFileInfo& rhs) {return lhs.m_time < rhs.m_time; });

					//sorting file by name will sort by time
					for (CFileInfoVector::const_iterator it = filesInfo.begin(); it != filesInfo.end() && msg; it++)
					{
						msg = ReadData(it->m_filePath, stations, callback);
						msg += callback.StepIt(0);
					}

					for (CWeatherStationVector::const_iterator it = stations.begin(); it != stations.end() && msg; it++)
					{
						DB.Add(*it);
						msg += callback.StepIt(0);
					}


					msg += callback.StepIt();
				}


				msg = DB.Close();
			}

			callback.PopTask();
		}



		return msg;
	}


	ERMsg CEnvCanHourlyForecast::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;

		stationList.clear();

		msg = m_DB.Open(GetDatabaseFilePath(), CHourlyDatabase::modeRead, callback);
		if (msg && !m_pShapefile)
		{
			m_pShapefile = new CShapeFileBase;
			msg = m_pShapefile->Read(GetShapefileFilePath());
		}

		for (size_t i = 0; m_DB.size(); i++)
		{
			const CLocation& station = m_DB[i];

			string region = station.GetSSI("Region");
			size_t r = m_regions.GetRegion(region);
			ASSERT(r != UNKNOWN_POS);
			if (m_regions[r])
				stationList.push_back(station.m_ID);
		}

		return msg;
	}




	ERMsg CEnvCanHourlyForecast::GetWeatherStation(const std::string& ID, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;

		if (ID.empty())
		{
			ASSERT(!station.m_ID.empty());
			ASSERT(station.m_lat != -999);
			ASSERT(station.m_lon != -999);

			if (!m_DB.IsOpen())
				msg = m_DB.Open(GetDatabaseFilePath(), CHourlyDatabase::modeRead, callback);

			if (msg && !m_pShapefile)
			{
				m_pShapefile = new CShapeFileBase;
				msg = m_pShapefile->Read(GetShapefileFilePath());
			}

			if (!msg)
				return msg;


			CTRef current = CTRef::GetCurrentTRef(TM);
			station.GetStat(H_TAIR);//force to compute stat before call GetVariablesCount
			CWVariablesCounter counter = station.GetVariablesCount();
			CTRef TRefEnd = counter.GetTPeriod().End();
			ASSERT(TRefEnd.as(CTM::DAILY) <= current.as(CTM::DAILY));

			static const int NB_MISS_DAY_TO_IGNORE_FORECAST = 7;
			//station must have data in the last week
			if (current.as(CTM::DAILY) - TRefEnd.as(CTM::DAILY) < NB_MISS_DAY_TO_IGNORE_FORECAST)
			{

				array<bool, NB_VAR_H> bAddForecast;
				for (TVarH v = H_FIRST_VAR; v < NB_VAR_H; v++)
					bAddForecast[v] = current.as(CTM::DAILY) - counter[v].second.End().as(CTM::DAILY) < NB_MISS_DAY_TO_IGNORE_FORECAST;

				int shapeNo = -1;

				if (m_pShapefile->IsInside(station, &shapeNo))//inside a shape
				{
					const CDBF3& DBF = m_pShapefile->GetDBF();

					int Findex = DBF.GetFieldIndex("PAGEID");
					string forecastID = DBF[shapeNo][Findex].GetElement();

					CWeatherDatabaseOptimization const& zop = m_DB.GetOptimization();
					size_t index = zop.FindPosByID(forecastID);
					if (index < zop.size())
					{
						CWeatherStation st(true);
						msg = m_DB.Get(st, index);
						if (msg)
						{
							CTPeriod p = st.GetVariablesCount().GetTPeriod();

							CWeatherAccumulator accumulator(TM);
							for (CTRef d = p.Begin(); d <= p.End(); d++)
							{
								if (accumulator.TRefIsChanging(d))
								{
									CTRef TRef = accumulator.GetTRef();
									for (TVarH v = H_FIRST_VAR; v < NB_VAR_H; v++)
									{
										if (bAddForecast[v] && !station[TRef][v].IsInit())
											station[TRef].SetStat(v, accumulator.GetStat(v));
									}
								}

								const CHourlyData& hourData = st[d.GetYear()][d.GetMonth()][d.GetDay()][d.GetHour()];

								for (int v = 0; v < NB_VAR_H; v++)
									if (hourData[v] > -999)
										accumulator.Add(d, v, hourData[v]);

							}//for all days

							if (accumulator.GetTRef().IsInit())
							{
								CTRef TRef = accumulator.GetTRef();
								for (TVarH v = H_FIRST_VAR; v < NB_VAR_H; v++)
									if (bAddForecast[v] && !station[TRef][v].IsInit())
										station[TRef].SetStat(v, accumulator.GetStat(v));
							}
						}
					}
				}
			}
		}
		else
		{
			CWeatherDatabaseOptimization const& zop = m_DB.GetOptimization();
			size_t index = zop.FindPosByID(ID);
			if (index < zop.size())
			{
				msg = m_DB.Get(station, index);
			}
		}

		return msg;
	}

	void CEnvCanHourlyForecast::ReadTemperature(const zen::XmlElement& input, CWeatherStation& station)
	{
		zen::XmlIn in(input);
		for (zen::XmlIn child = in["temperature-list"]; child; child.next())
		{
			string type;
			child.attribute("type", type);

			int var = -1;
			if (type == "air")
				var = H_TAIR;
			else if (type == "dew-point")
				var = H_TDEW;


			if (var == H_TAIR || var == H_TDEW)
			{
				for (zen::XmlIn child2 = child["temperature-value"]; child2; child2.next())
				{

					string start;
					string end;
					child2.attribute("start", start);
					child2.attribute("end", end);

					CTRef TRef1 = ConvertTime(start);
					CTRef TRef2 = ConvertTime(end);

					double low = 0;
					double hi = 0;
					child2["lower-limit"](low);
					child2["upper-limit"](hi);

					for (CTRef ref = TRef1; ref <= TRef2; ref++)
					{
						ASSERT(ref.GetYear() > 0);
						ASSERT(station.IsHourly());

						CHourlyData& data = station[ref.GetYear()][ref.GetMonth()][ref.GetDay()][ref.GetHour()];
						data[var] = (low + hi) / 2;

						if (data[H_TAIR] > -999 && data[H_TDEW] > -999)
							data[H_RELH] = Td2Hr(data[H_TAIR], data[H_TDEW]);
					}
				}
			}
		}
	}

	void CEnvCanHourlyForecast::ReadPrecipitation(const zen::XmlElement& input, CWeatherStation& station)
	{
		zen::XmlIn child = input.getChild("accum-list");
		if (child)
		{
			for (zen::XmlIn child2 = child["accum-amount"]; child2; child2.next())
			{
				string start;
				string end;
				child2.attribute("start", start);
				child2.attribute("end", end);

				CTRef TRef1 = ConvertTime(start);
				CTRef TRef2 = ConvertTime(end);

				double low = 0;
				double hi = 0;
				child2["lower-limit"](low);
				child2["upper-limit"](hi);
				double prcp = (low + hi) / (2 * (TRef2 - TRef1));

				for (CTRef ref = TRef1; ref < TRef2; ref++)
				{
					ASSERT(ref.GetYear() > 0);
					ASSERT(station.IsHourly());
					station[ref.GetYear()][ref.GetMonth()][ref.GetDay()][ref.GetHour()][H_PRCP] = prcp;
				}
			}
		}
	}

	void CEnvCanHourlyForecast::ReadWind(const zen::XmlElement& input, CWeatherStation& station)
	{
		zen::XmlIn child = input.getChild("wind-list");
		if (child)
		{
			for (zen::XmlIn child2 = child["wind"]; child2; child2.next())
			{
				string start;
				string end;
				child2.attribute("start", start);
				child2.attribute("end", end);
				CTRef TRef1 = ConvertTime(start);
				CTRef TRef2 = ConvertTime(end);

				zen::XmlIn in(child2["wind-speed"]);

				double low = 0;
				double hi = 0;
				in["lower-limit"](low);
				in["upper-limit"](hi);
				double windSpeed = (low + hi) / 2;

				for (CTRef ref = TRef1; ref <= TRef2; ref++)
				{
					ASSERT(ref.GetYear() > 0);
					ASSERT(station.IsHourly());
					station[ref.GetYear()][ref.GetMonth()][ref.GetDay()][ref.GetHour()][H_WNDS] = windSpeed;
				}
			}
		}
	}

	void CEnvCanHourlyForecast::ReadDay(const zen::XmlElement& input, CWeatherStation& station)
	{
		ReadTemperature(input, station);
		ReadPrecipitation(input, station);
		//ReadRelativeHumidity(input, station);
		ReadWind(input, station);

	}

	ERMsg CEnvCanHourlyForecast::ReadData(const string& filePath, CWeatherStationVector& stations, CCallback& callback)const
	{
		ERMsg msg;

		//open file
		zen::XmlDoc doc;

		msg = load(filePath, doc);
		if (msg)
		{
			const zen::XmlElement* pElem = doc.root().getChild("data");
			if (pElem)
				pElem = pElem->getChild("forecast");

			if (pElem)
			{
				auto forecast = pElem->getChildren("meteocode-forecast");
				for (auto it = forecast.first; it != forecast.second && msg; it++)
				{
					auto locations = it->getChildren("location");
					const zen::XmlElement* pData = it->getChild("parameters");
					if (locations.first != locations.second && pData)
					{
						//for all location with this data
						for (auto it2 = locations.first; it2 != locations.second && msg; it2++)
						{
							zen::XmlIn in(*it2);

							string ID;
							in["msc-zone-code"](ID);


							CLocationVector::const_iterator itFind = std::find_if(m_stations.begin(), m_stations.end(), FindLocationByID(ID));

							if (itFind == m_stations.end() && (ID.back() == 'a' || ID.back() == 'b' || ID.back() == 'c' || ID.back() == 'd'))
							{
								string ID2(ID);
								ID2.pop_back();
								itFind = std::find_if(m_stations.begin(), m_stations.end(), FindLocationByID(ID2));
								if (itFind != m_stations.end())
									ID = ID2;
							}


							if (itFind != m_stations.end())
							{
								CWeatherStationVector::iterator it = std::find_if(stations.begin(), stations.end(), FindLocationByID(ID));
								if (it == stations.end())
								{
									CWeatherStation weaterStation(true);
									((CLocation&)weaterStation) = *itFind;
									weaterStation.m_name = PurgeFileName(weaterStation.m_name);
									weaterStation.SetDefaultSSI(CLocation::DATA_FILE_NAME, RemoveAccented(weaterStation.m_name + " (" + weaterStation.GetSSI("Region") + ").csv"));
									stations.push_back(weaterStation);
									it = std::find_if(stations.begin(), stations.end(), FindLocationByID(ID));
								}


								ReadDay(*pData, *it);
							}
							else
							{
								//Seem to be normal, no station information for some forecast??? Stop writing warning
								//string locName;
								//in["msc-zone-name"](locName);
								//callback.AddMessage("No station information for ID: " + ID + " (" + locName + ")");
							}

							msg += callback.StepIt(0);
						}//all locations
					}//all forecast
				}
			}
		}

		return msg;
	}


	ERMsg CEnvCanHourlyForecast::Finalize(size_t type, CCallback& callback)
	{
		ERMsg msg;

		m_DB.Close();
		if (m_pShapefile)
		{
			delete m_pShapefile;
			m_pShapefile = NULL;
		}

		return msg;
	}

}