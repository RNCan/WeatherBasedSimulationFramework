#include "StdAfx.h"
#include "UISaskatchewan.h"
#include <boost\dynamic_bitset.hpp>
#include <boost\filesystem.hpp>

#include "UI/Common/UtilWin.h"
#include "Basic/DailyDatabase.h"
#include "Basic/FileStamp.h"
#include "UI/Common/SYShowMessage.h"
//#include "Basic\CSV.h"
#include "json\json11.hpp"

#include "TaskFactory.h"
#include "../Resource.h"
#include "WeatherBasedSimulationString.h"


using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace UtilWWW;
using namespace boost;
using namespace json11;

//stations coord
//http://wfm.gov.sk.ca/wfm/map_data?first
//hourly/daily data
//http://wfm.gov.sk.ca/wfm/table?stn=URNMC&typ=extended&date=2019-05-15&submit=
//http://wfm.gov.sk.ca/wfm/table/stn/data?stn=SNISB&date=2019-06-06&typ=today&dtype=hourly&tqx=reqId%3A0


namespace WBSF
{

	const char* UISaskatchewan::SUBDIR_NAME[NB_NETWORKS] = { "Fire" };
	const char* UISaskatchewan::NETWORK_NAME[NB_NETWORKS] = { "Saskatchewan Fire" };
	const char* UISaskatchewan::SERVER_NAME[NB_NETWORKS] = { "wfm.gov.sk.ca" };
	const char* UISaskatchewan::SERVER_PATH[NB_NETWORKS] = { "/" };

	size_t UISaskatchewan::GetNetwork(const string& network)
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
	const char* UISaskatchewan::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "DownloadArchive" };
	const size_t UISaskatchewan::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_BOOL };
	const UINT UISaskatchewan::ATTRIBUTE_TITLE_ID = IDS_UPDATER_SASKATCHEWAN_P;
	const UINT UISaskatchewan::DESCRIPTION_TITLE_ID = ID_TASK_SASKATCHEWAN;

	const char* UISaskatchewan::CLASS_NAME() { static const char* THE_CLASS_NAME = "Saskatchewan";  return THE_CLASS_NAME; }
	CTaskBase::TType UISaskatchewan::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(UISaskatchewan::CLASS_NAME(), (createF)UISaskatchewan::create);



	UISaskatchewan::UISaskatchewan(void)
	{}

	UISaskatchewan::~UISaskatchewan(void)
	{}



	std::string UISaskatchewan::Option(size_t i)const
	{
		string str;
		//switch (i)
		//{
		//};
		return str;
	}

	std::string UISaskatchewan::Default(size_t i)const
	{
		string str;

		switch (i)
		{
		case WORKING_DIR: str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()) + "Saskatchewan\\"; break;
		case DOWNLOAD_ARCHIVE:	str = "1"; break;
		};

		return str;
	}

	//****************************************************


	std::string UISaskatchewan::GetStationsListFilePath(size_t network)const
	{
		static const char* FILE_NAME[NB_NETWORKS] = { "SaskatchewanFireStations.csv" };

		string filePath = WBSF::GetApplicationPath() + "Layers\\" + FILE_NAME[network];
		return filePath;
	}

	string UISaskatchewan::GetOutputFilePath(size_t network, const string& title, int year, size_t m)const
	{
		return GetDir(WORKING_DIR) + SUBDIR_NAME[network] + "\\" + ToString(year) + "\\" + (m != NOT_INIT ? FormatA("%02d\\", m + 1).c_str() : "") + title + ".csv";
	}


	ERMsg UISaskatchewan::Execute(CCallback& callback)
	{
		ERMsg msg;


		string workingDir = GetDir(WORKING_DIR) + SUBDIR_NAME[FIRE] + "\\";
		msg = CreateMultipleDir(workingDir);


		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(workingDir, 1);
		callback.AddMessage(GetString(IDS_UPDATE_FROM));
		callback.AddMessage(string(SERVER_NAME[FIRE]) + "/" + SERVER_PATH[FIRE], 1);
		callback.AddMessage("");

		msg = ExecuteFire(callback);

		return msg;
	}


	ERMsg UISaskatchewan::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;


		m_stations.clear();

		CLocationVector locations;
		msg = locations.Load(GetStationsListFilePath(FIRE));

		if (msg)
			msg += locations.IsValid();

		//Update network
		for (size_t i = 0; i < locations.size(); i++)
			locations[i].SetSSI("Network", NETWORK_NAME[FIRE]);

		m_stations.insert(m_stations.end(), locations.begin(), locations.end());

		for (size_t i = 0; i < locations.size(); i++)
			stationList.push_back(ToString(FIRE) + "/" + locations[i].m_ID);


		return msg;
	}

	ERMsg UISaskatchewan::GetWeatherStation(const std::string& NID, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;

		size_t n = ToSizeT(NID.substr(0, 1));
		string ID = NID.substr(2);

		//Get station information
		size_t it = m_stations.FindByID(ID);
		ASSERT(it != NOT_INIT);

		((CLocation&)station) = m_stations[it];

		//size_t type = as<size_t>(DATA_TYPE);
		int firstYear = WBSF::as<int>(Get("FirstYear"));
		int lastYear = WBSF::as<int>(Get("LastYear"));
		size_t nbYears = lastYear - firstYear + 1;
		station.CreateYears(firstYear, nbYears);

		station.m_name = PurgeFileName(station.m_name);

		//now extract data 
		for (size_t y = 0; y < nbYears&&msg; y++)
		{
			int year = firstYear + int(y);

			string filePath = GetOutputFilePath(n, ID, year);
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


	//******************************************************************************************************

	ERMsg UISaskatchewan::ExecuteFire(CCallback& callback)
	{
		ERMsg msg;

		CLocationVector locations;
		msg = locations.Load(GetStationsListFilePath(FIRE));

		if (msg)
			msg += locations.IsValid();

		if (msg)
		{

			bool bArchive = as<bool>(DOWNLOAD_ARCHIVE);

			callback.PushTask("Update Saskatchewan fire weather data (" + ToString(locations.size()) + " stations)", locations.size() * (bArchive ? 2 : 1));
			size_t curI = 0;
			int nbRun = 0;
			CTRef today = CTRef::GetCurrentTRef();

			while (curI < locations.size() && nbRun < 5 && msg)
			{
				nbRun++;

				CInternetSessionPtr pSession;
				CHttpConnectionPtr pConnection;

				ERMsg msgTmp = GetHttpConnection(SERVER_NAME[FIRE], pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", false, 5, callback);
				if (msgTmp)
				{
					try
					{
						for (size_t i = curI; i < locations.size() && msg; i++)
						{
							string ID = locations[i].m_ID;

							if (bArchive)
							{
								//download the last year
								for (CTRef TRef = today - 151; TRef <= today && msg; TRef += 3)
								{
									string URL = FormatA("wfm/table/stn/data?stn=%s&date=%04d-%02d-%02d&typ=today&dtype=hourly&tqx=reqId%%3A0", ID.c_str(), TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1);

									string source;
									msgTmp += UtilWWW::GetPageText(pConnection, URL, source, false, INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_PRAGMA_NOCACHE);

									//split data in separate files
									if (msgTmp)
									{
										msg += MergeFireData(locations[i].m_ID, source, callback);
										if (msg)
										{
											msg += callback.StepIt(0);
										}
									}
								}

								curI++;
								msg += callback.StepIt();
							}
							else
							{
								string URL = FormatA("wfm/table/stn/data?stn=%s&date=%04d-%02d-%02d&typ=today&dtype=hourly&tqx=reqId%%3A0", ID.c_str(), today.GetYear(), today.GetMonth() + 1, today.GetDay() + 1);

								string source;
								msgTmp += UtilWWW::GetPageText(pConnection, URL, source, false, INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_PRAGMA_NOCACHE);

								//split data in separate files
								if (msgTmp)
								{
									msg += MergeFireData(locations[i].m_ID, source, callback);
									if (msg)
									{
										curI++;
										msg += callback.StepIt();
									}
								}
							}
						}//for all station
					}
					catch (CException* e)
					{
						msgTmp = UtilWin::SYGetMessage(*e);
					}


					//clean connection
					pConnection->Close();
					pSession->Close();
				}
				else
				{
					if (nbRun > 1 && nbRun < 5)
					{
						msg += WaitServer(10, callback);
					}
				}
			}


			callback.AddMessage(GetString(IDS_NB_STATIONS) + ToString(curI), 1);
			callback.PopTask();
		}

		return msg;
	}


	ERMsg UISaskatchewan::MergeFireData(const string& ID, std::string source, CCallback& callback)
	{
		ERMsg msg;


		CTM TM(CTM::HOURLY);

		CWeatherYears data(true);
		vector<size_t> vars;

		WBSF::ReplaceString(source, "google.visualization.Query.setResponse(", "");
		WBSF::ReplaceString(source, ");", "");

		string error;
		Json json_features = Json::parse(source, error);

		if (error.empty())
		{
			const string& table_str = json_features["table"].string_value();
			Json json_rows = Json::parse(table_str, error);
			Json::array rows = json_rows["rows"].array_items();

			for (Json::array::const_iterator it = rows.begin(); it != rows.end() && msg; it++)
			{
				Json::object json_c = it->object_items();
				ASSERT(json_c.size() == 1);
				Json::array cols = json_c.begin()->second.array_items();
				ASSERT(cols.size() == 13);
				if (cols.size() == 13)
				{

					Json::object json_date = cols.at(0).object_items();
					ASSERT(json_date.size() == 2);
					string str_date = json_date["f"].string_value();
					StringVector date(str_date, "- :");
					ASSERT(date.size() == 6);
					CTRef TRef(ToInt(date[0]), ToSizeT(date[1]) - 1, ToSizeT(date[2]) - 1, ToSizeT(date[3]));
					ASSERT(TRef.IsValid());

					if (!data.IsYearInit(TRef.GetYear()))
					{
						//try to load old data before changing it...
						string filePath = GetOutputFilePath(FIRE, ID, TRef.GetYear());
						if (!data.LoadData(filePath, -999, false))//don't erase other years when multiple years
							data.CreateYear(TRef.GetYear());
					}

					for (size_t v = 1; v < 9 && msg; v++)
					{
						Json::object json_value = cols.at(v).object_items();
						if (json_value.size() == 1)
						{
							if (!json_value["v"].is_null())
							{
								double value = json_value["v"].number_value();

								switch (v)
								{
								case 1: data[TRef].SetStat(H_TAIR, value); break;//Temp	
								case 2: data[TRef].SetStat(H_RELH, value); break;//RH	
								case 3: data[TRef].SetStat(H_WNDS, value); break;//Wind  speed	
								case 4: break;//Wind Gusts	
								case 5: break;//Dir	Wind
								case 6: data[TRef].SetStat(H_WNDD, value); break;//Wind Az
								case 7: data[TRef].SetStat(H_PRCP, value); break;//Rain (1 hr)	
								case 8: data[TRef].SetStat(H_PRES, value); break;//Press
								};
							}
						}
					}

					if (data[TRef][H_TAIR].IsInit() && data[TRef][H_RELH].IsInit())
						data[TRef].SetStat(H_TDEW, Hr2Td(data[TRef][H_TAIR], data[TRef][H_RELH]));
				}
			}

			//save all years 
			for (auto it = data.begin(); it != data.end(); it++)
			{
				if (it->second->HaveData())
				{
					string filePath = GetOutputFilePath(FIRE, ID, it->first);
					string outputPath = GetPath(filePath);
					CreateMultipleDir(outputPath);
					it->second->SaveData(filePath, TM);
				}
			}
		}

		return msg;
	}


}