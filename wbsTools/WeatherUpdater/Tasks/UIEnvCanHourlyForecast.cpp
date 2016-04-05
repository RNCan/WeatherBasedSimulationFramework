#include "StdAfx.h"
#include "UIEnvCanHourlyForecast.h"
#include "Basic/WeatherStation.h"
//#include "Basic/Registry.h"
//#include "Basic/zenXml.h"
//#include "Basic/UtilTime.h"
#include "Geomatic/ShapeFileBase.h"
#include "UI/Common/UtilWWW.h"
#include "TaskFactory.h"

#include "WeatherBasedSimulationString.h"
#include "../Resource.h"



//#include "SelectionDlg.h"


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
			if (at(i))
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
				at(pos) = true;
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

	const char* CUIEnvCanHourlyForecast::SERVER_NAME = "dd.weatheroffice.ec.gc.ca";
	const char* CUIEnvCanHourlyForecast::SERVER_PATH = "/meteocode/";

	//*********************************************************************
	const char* CUIEnvCanHourlyForecast::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "Region" };
	const size_t CUIEnvCanHourlyForecast::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_STRING_BROWSE };
	const UINT CUIEnvCanHourlyForecast::ATTRIBUTE_TITLE_ID = IDS_UPDATER_EC_FORECAST_P;
	
	const char* CUIEnvCanHourlyForecast::CLASS_NAME(){ static const char* THE_CLASS_NAME = "EnvCanHourlyForecast";  return THE_CLASS_NAME; }
	CTaskBase::TType CUIEnvCanHourlyForecast::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterClass(CUIEnvCanHourlyForecast::CLASS_NAME(), CUIEnvCanHourlyForecast::create);

	

	//autre forecast quotidienne avec plus de jour et la déviation
	//https://weather.gc.ca/ensemble/naefs/EPSgrams_e.html?station=YOW

	//forecast raw wind speed
	//http://dd.weather.gc.ca/ensemble/naefs/grib2/raw/00/000/



	CUIEnvCanHourlyForecast::CUIEnvCanHourlyForecast(void):
		m_pShapefile(NULL)
	{}
	

	CUIEnvCanHourlyForecast::~CUIEnvCanHourlyForecast(void)
	{
		if (m_pShapefile)
			delete m_pShapefile;

		m_pShapefile = NULL;
	}


	std::string CUIEnvCanHourlyForecast::Option(size_t i)const
	{
		string str;
		switch (i)
		{
		case REGION: str = CRegionSelection::GetAllPossibleValue(); break;
		};
		return str;
	}

	//std::string CUIEnvCanDaily::Default(size_t i)const
	//{
	//	std::string str;
	//	//switch (i)
	//	//{
	//	//};

	//	return str;
	//}


	std::string CUIEnvCanHourlyForecast::GetStationListFilePath()const
	{
		return GetApplicationPath() + "Layers\\ForecastZones.csv";
	}

	std::string CUIEnvCanHourlyForecast::GetShapefileFilePath()const
	{
		return GetApplicationPath() + "Layers\\ForecastZones.shp";
	}

	std::string CUIEnvCanHourlyForecast::GetDatabaseFilePath()const
	{
		return GetDir(WORKING_DIR) + "ForecastDB.HourlyStations";
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

	ERMsg CUIEnvCanHourlyForecast::LoadStationInformation(const string& outputFilePath, CLocation& station)
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

		//TRANSMIT.FPXK55.05.30.1500Z.xml
	}

	//
	//static std::string TraitFileName(std::string name)
	//{
	//	size_t begin = name.find('(');
	//	size_t end = name.find(')');
	//	if (begin != std::string::npos && end != std::string::npos)
	//		name.erase(begin, end);
	//
	//	UppercaseFirstLetter(name);
	//	Trim(name);
	//
	//	return name;
	//}

	ERMsg CUIEnvCanHourlyForecast::Execute(CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);


		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(workingDir, 1);
		callback.AddMessage(GetString(IDS_UPDATE_FROM));
		callback.AddMessage(SERVER_NAME, 1);
		callback.AddMessage("");

		callback.AddMessage(GetString(IDS_UPDATE_FILE));
		//callback.AddTask(m_selection.count());


		msg = CHourlyDatabase().DeleteDatabase(GetDatabaseFilePath(), callback);
		if (!msg)
			return msg;

		//open a connection on the server
		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;


		msg = m_stations.Load(GetStationListFilePath());

		if (msg)
			msg = GetHttpConnection(SERVER_NAME, pConnection, pSession);

		if (!msg)
			return msg;

		int nbDownload = 0;
		CWeatherStationVector stations;

		for (size_t i = 0; i < m_selection.size() && msg; i++)
		{
			if (m_selection.any() && !m_selection[i])
				continue;

			string region = m_selection.GetName((int)i, true);
			string URL = SERVER_PATH + region + "/cmml/";
			string outputPath = workingDir + region + "/";

			StringVector filesList = GetFilesList(outputPath + "TRANSMIT.*.xml");
			for (StringVector::const_iterator it = filesList.begin(); it != filesList.end(); it++)
				msg += RemoveFile(*it);

			//Load files list
			CFileInfoVector fileList;
			UtilWWW::FindFiles(pConnection, URL + "TRANSMIT.*.xml", fileList);
			ClearList(fileList);
			callback.AddMessage(region + ", " + GetString(IDS_NUMBER_FILES) + ToString(fileList.size()), 1);

			//Download files
			callback.PushTask(region, fileList.size());
			//callback.SetNbStep(fileList.size());
			CreateMultipleDir(outputPath);

			for (CFileInfoVector::iterator it = fileList.begin(); it != fileList.end() && msg; it++)
			{
				string fileName = GetFileName(it->m_filePath);
				string ID = fileName.substr(0, 8);
				string outputFilePath = outputPath + fileName;

				msg += UtilWWW::CopyFile(pConnection, it->m_filePath, outputFilePath, INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_DONT_CACHE);
				if (msg)
				{
					ASSERT(FileExists(outputFilePath));
					nbDownload++;
					msg = ReadData(outputFilePath, stations, callback);
				}

				msg += callback.StepIt();
			}
		}//for all province

		callback.AddMessage(GetString(IDS_NB_FILES_DOWNLOADED) + ToString(nbDownload), 2);

		pConnection->Close();
		pSession->Close();
		callback.PopTask();

		if (msg)
		{

			//Create only one database for all forecast
			CHourlyDatabase DB;
			msg = DB.Open(GetDatabaseFilePath(), CHourlyDatabase::modeWrite, callback);
			if (msg)
			{
				for (CWeatherStationVector::const_iterator it = stations.begin(); it != stations.end(); it++)
					DB.Add(*it);

				msg = DB.Close();
			}

		}

		return msg;
	}

	
	ERMsg CUIEnvCanHourlyForecast::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;

		msg = m_DB.Open(GetDatabaseFilePath(), CHourlyDatabase::modeRead, callback);

		if (msg && !m_pShapefile)
		{
			m_pShapefile = new CShapeFileBase;
			msg = m_pShapefile->Read(GetShapefileFilePath());
		}

		stationList.clear();

		//m_DB.size()
		for (size_t i = 0; m_DB.size(); i++)
			//for (CLocationVector::const_iterator it = m_stations.begin(); it != m_stations.end(); it++)
		{
			const CLocation& station = m_DB[i];

			string region = station.GetSSI("Region");
			size_t r = m_selection.GetRegion(region);
			ASSERT(r != UNKNOWN_POS);
			if (m_selection[r])
			{
				stationList.push_back(station.m_ID);
			}
		}

		return msg;
	}




	ERMsg CUIEnvCanHourlyForecast::GetWeatherStation(const std::string& ID, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;

		if (ID.empty())
		{
			ASSERT(!station.m_ID.empty());
			ASSERT(station.m_lat != -999);
			ASSERT(station.m_lon != -999);

			set<int> years = m_DB.GetYears();

			bool bInit = false;

			for (set<int>::const_iterator it = years.begin(); it != years.end() && !bInit; it++)
				if (station.IsYearInit(*it))
					bInit = true;

			//no forecast are added on old data
			if (bInit)
			{
				int shapeNo = -1;
				//double d = m_pShapefile->GetMinimumDistance(station, &shapeNo);
				if (m_pShapefile->IsInside(station, &shapeNo))//inside a shape
				{
					const CDBF3& DBF = m_pShapefile->GetDBF();

					int Findex = DBF.GetFieldIndex("PAGEID");
					string forecastID = DBF[shapeNo][Findex].GetElement();

					CWeatherDatabaseOptimization const& zop = m_DB.GetOptimization();
					size_t index = zop.FindByID(forecastID);
					if (index < zop.size())
					{
						CWeatherStation st(true);
						msg = m_DB.Get(st, index);
						if (msg)
						{
							CWVariablesCounter counter = station.GetVariablesCount();
							CWVariables varInfo = counter.GetVariables();
							CTPeriod p = st.GetVariablesCount().GetTPeriod();

							CWeatherAccumulator accumulator(TM);
							for (CTRef d = p.Begin(); d <= p.End(); d++)
							{
								if (accumulator.TRefIsChanging(d))
								{
									if (station[accumulator.GetTRef()].GetVariablesCount().GetSum() == 0)
									{
										station[accumulator.GetTRef()].SetData(accumulator);
									}
								}

								const CHourlyData& hourData = st[d.GetYear()][d.GetMonth()][d.GetDay()][d.GetHour()];

								for (int v = 0; v <NB_VAR_H; v++)
									if (varInfo[v])
										if (hourData[v]>-999)
											accumulator.Add(d, v, hourData[v]);

							}
						}
					}
				}
			}
		}
		else
		{
			CWeatherDatabaseOptimization const& zop = m_DB.GetOptimization();
			size_t index = zop.FindByID(ID);
			if (index < zop.size())
			{
				msg = m_DB.Get(station, index);
			}
		}

		return msg;
	}

	void ReadTemperature(const zen::XmlElement& input, CWeatherStation& station)
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

	void ReadPrecipitation(const zen::XmlElement& input, CWeatherStation& station)
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

	void ReadWind(const zen::XmlElement& input, CWeatherStation& station)
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

	void ReadDay(const zen::XmlElement& input, CWeatherStation& station)
	{
		ReadTemperature(input, station);
		ReadPrecipitation(input, station);
		//ReadRelativeHumidity(input, station);
		ReadWind(input, station);

	}

	ERMsg CUIEnvCanHourlyForecast::ReadData(const string& filePath, CWeatherStationVector& stations, CCallback& callback)const
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
				for (auto it = forecast.first; it != forecast.second; it++)
				{
					auto locations = it->getChildren("location");
					const zen::XmlElement* pData = it->getChild("parameters");
					if (locations.first != locations.second && pData)
					{
						//for all location with this data
						for (auto it2 = locations.first; it2 != locations.second; it2++)
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
									weaterStation.SetDefaultSSI(CLocation::DATA_FILE_NAME, weaterStation.m_name + " (" + weaterStation.GetSSI("Region") + ").csv");
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
						}//all locations
					}//all forecast
				}
			}
		}

		return msg;
	}

}