﻿#include "StdAfx.h"
#include "UIMiscellaneous.h"

#include <boost\dynamic_bitset.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include "Basic/CSV.h"


#include "Basic/DailyDatabase.h"
#include "Basic/FileStamp.h"
#include "UI/Common/SYShowMessage.h"
#include "TaskFactory.h"
#include "../Resource.h"

#include "CountrySelection.h"
#include "StateSelection.h"
#include "Geomatic/TimeZones.h"
#include "RCM4_25km.h"
//#include "cctz\time_zone.h"



using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace UtilWWW;
using namespace boost;


namespace WBSF
{

	//MesoWest station list 
	//http://mesowest.utah.edu/cgi-bin/droman/meso_station.cgi

	//MesoWest station data
	//http://mesowest.utah.edu/cgi-bin/droman/download_api2.cgi?stn=CYLA&year1=2017&day1=23&month1=3&hour1=0&timetype=LOCAL&unit=1

	//MesoWest bt state
	//http://mesowest.utah.edu/cgi-bin/droman/stn_state.cgi?state=AB
	//http://gl1.chpc.utah.edu/cgi-bin/droman/stn_state.cgi?state=QC&order=status


	//variable listing
	//http://mesowest.utah.edu/cgi-bin/droman/variable_download_select.cgi
	//

	//Ireland data
	//https://cli.fusio.net/cli/climate_data/webdatac/dly1928.zip

	//cwfis_fwi
	//https://cwfis.cfs.nrcan.gc.ca/downloads/fwi_obs/

	const char* CUIMiscellaneous::SERVER_NAME[NB_DATASETS] = { "cdiac.ornl.gov", "", "", "ftp.tor.ec.gc.ca", " ftp.cccma.ec.gc.ca", "cwfis.cfs.nrcan.gc.ca" };
	const char* CUIMiscellaneous::SERVER_PATH[NB_DATASETS] = { "pub12/russia_daily/", "", "", "/Pub/Engineering_Climate_Dataset/Canadian_Weather_Energy_Engineering_Dataset_CWEEDS_2005/ZIPPED%20FILES/ENGLISH/CWEEDS_v_2016/", "/data/cordex/output/CCCma/CanRCM4", "/downloads/fwi_obs/" };


	//*********************************************************************
	const char* CUIMiscellaneous::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "Dataset", "FirstYear", "LastYear", "ShowProgress" };
	const size_t CUIMiscellaneous::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_COMBO_INDEX, T_STRING, T_STRING, T_BOOL };
	const UINT CUIMiscellaneous::ATTRIBUTE_TITLE_ID = IDS_UPDATER_MISCELLANEOUS_P;
	const UINT CUIMiscellaneous::DESCRIPTION_TITLE_ID = ID_TASK_MISCELLANEOUS;

	const char* CUIMiscellaneous::CLASS_NAME() { static const char* THE_CLASS_NAME = "Miscellaneous";  return THE_CLASS_NAME; }
	CTaskBase::TType CUIMiscellaneous::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUIMiscellaneous::CLASS_NAME(), (createF)CUIMiscellaneous::create);



	CUIMiscellaneous::CUIMiscellaneous(void)
	{}

	CUIMiscellaneous::~CUIMiscellaneous(void)
	{}

	std::string CUIMiscellaneous::GetLocationsFilePath(size_t dataset, bool bLocal)const
	{
		if (dataset >= NB_DATASETS)
			dataset = 0;



		string filePath;

		switch (dataset)
		{
		case CDIAC_RUSSIA:	filePath = (bLocal ? GetDir(WORKING_DIR) : string(SERVER_PATH[dataset])) + "Russia_518_inventory.csv"; break;
		case SOPFEU_HISTORICAL:	filePath = bLocal ? GetDir(WORKING_DIR) + "Historique Positionement Stations météo.csv" : ""; break;
		case QUEBEC_HOURLY: filePath = bLocal ? GetApplicationPath() + "Layers\\QuebecStations.csv" : ""; break;
		case CWEEDS:		filePath = GetDir(WORKING_DIR) + "CWEEDS_2016_Location_List.csv"; break;
		case RCM4_22:		filePath = GetDir(WORKING_DIR) + "orog_4nearest_24stations.csv"; break;
		case CWFIS:			filePath = (bLocal ? GetDir(WORKING_DIR) : string(SERVER_PATH[dataset])) + "cwfis_allstn.csv"; break;
		}

		return filePath;
	}

	std::string CUIMiscellaneous::Option(size_t i)const
	{
		string str;
		switch (i)
		{
		case DATASET:	str = "Russia|SOPFEU historical|Quebec Hourly|CWEEDS|CCCma RCM4_22|CWFIS"; break;
		};
		return str;
	}

	std::string CUIMiscellaneous::Default(size_t i)const
	{
		string str;

		size_t dataset = as<size_t>(DATASET);
		if (dataset >= NB_DATASETS)
			dataset = 0;


		static const char* DEFAULT_DIR[NB_DATASETS] = { "Miscellaneous\\Russia\\", "Miscellaneous\\SOPFEU", "Miscellaneous\\Quebec", "Miscellaneous\\CWEEDS\\", "Miscellaneous\\RCM4_22\\", "Miscellaneous\\CWFIS\\" };
		switch (i)
		{
		case WORKING_DIR: str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()) + DEFAULT_DIR[dataset]; break;
		case FIRST_YEAR: str = "1800"; break;
		case LAST_YEAR:	str = "2100"; break;
		case DATASET: str = ToString(dataset); break;
		};

		return str;
	}

	//****************************************************


	ERMsg CUIMiscellaneous::UpdateStationLocations()
	{
		ERMsg msg;

		size_t dataset = as<size_t>(DATASET);
		if (dataset >= NB_DATASETS)
		{
			msg.ajoute("Invalid dataset");
			return msg;
		}

		string path = GetLocationsFilePath(as<size_t>(DATASET), false);
		if (!path.empty())
		{

			CInternetSessionPtr pSession;
			CFtpConnectionPtr pConnection;

			msg = GetFtpConnection(SERVER_NAME[dataset], pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", true, 1);
			if (msg)
			{

				CFileInfoVector fileList;
				msg = FindFiles(pConnection, path, fileList, false, CCallback::DEFAULT_CALLBACK);
				if (msg)
				{
					ASSERT(fileList.size() == 1);

					string workingDir = GetDir(WORKING_DIR);
					string outputFilePath = workingDir + GetFileName(fileList[0].m_filePath);

					if (!IsFileUpToDate(fileList[0], outputFilePath))
					{
						msg = CopyFile(pConnection, fileList[0].m_filePath, outputFilePath);

						if (msg)
						{
							switch (dataset)
							{
							case CDIAC_RUSSIA:
							{
								fStream file;
								msg = file.open(outputFilePath);

								if (msg)
								{
									std::stringstream fileData;

									fileData << "ID,Name,Latitude,Longitude,Elevation,FirstYear\n";
									fileData << file.rdbuf();
									file.close();

									file.open(outputFilePath, std::fstream::out | std::fstream::trunc);
									file << fileData.rdbuf();
									file.close();
								}

								break;
							}
							case SOPFEU_HISTORICAL: break;
							case QUEBEC_HOURLY:break;
							case CWEEDS: break;
							case RCM4_22: break;
							case CWFIS: break;
							}
						}
					}

				}

				pConnection->Close();
				pSession->Close();
			}//if msg
		}//if not empty

		return msg;
	}

	ERMsg CUIMiscellaneous::LoadStationsLocations(CCallback& callback)
	{
		size_t dataset = as<size_t>(DATASET);
		ASSERT(dataset < NB_DATASETS);

		return m_stations.Load(GetLocationsFilePath(dataset, true), ",", callback);
	}

	ERMsg CUIMiscellaneous::GetFileList(CFileInfoVector& fileList, CCallback& callback)const
	{
		ERMsg msg;

		size_t dataset = as<size_t>(DATASET);
		if (dataset >= NB_DATASETS)
		{
			msg.ajoute("Invalid dataset");
			return msg;
		}


		fileList.clear();

		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = lastYear - firstYear + 1;

		callback.PushTask(GetString(IDS_LOAD_FILE_LIST), nbYears);


		dynamic_bitset<size_t> toDo(1);
		toDo.set();

		size_t nbRun = 0;

		CFileInfoVector dirList;
		while (nbRun < 20 && toDo[0] && msg)
		{
			nbRun++;

			//open a connection on the server
			CInternetSessionPtr pSession;
			CFtpConnectionPtr pConnection;

			ERMsg msgTmp = GetFtpConnection(SERVER_NAME[dataset], pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", true, 5, callback);
			if (msgTmp)
			{
				pSession->SetOption(INTERNET_OPTION_RECEIVE_TIMEOUT, 55000);
				pSession->SetOption(INTERNET_OPTION_DATA_RECEIVE_TIMEOUT, 55000);

				if (toDo[0])
				{
					switch (dataset)
					{

					case CDIAC_RUSSIA:
					{

						msgTmp = FindFiles(pConnection, string(SERVER_PATH[dataset]) + "Russia_518_data.txt.gz", fileList, false, callback);
						break;

					}
					case SOPFEU_HISTORICAL:break;
					case QUEBEC_HOURLY:break;
					case CWEEDS: break;
					case RCM4_22:
					{
						msgTmp = FindFiles(pConnection, string(SERVER_PATH[dataset]) + "*.tif", fileList, false, callback);
						break;
					}
					case CWFIS:
					{
						msgTmp = FindFiles(pConnection, string(SERVER_PATH[dataset]) + "cwfis_fwi*.bz2", fileList, false, callback);
						break;

					}
					default:;

						//	callback.PushTask(GetString(IDS_LOAD_FILE_LIST), NOT_INIT);
						//msgTmp = FindDirectories(pConnection, SERVER_PATH[dataset], dirList);
						//callback.PopTask();
					}

					if (msgTmp)
						toDo[0] = false;


					//if (msgTmp)
					//{
					//	for (size_t y = 0; y < dirList.size() && msg&&msgTmp; y++)
					//	{
					//		msg += callback.StepIt(0);

					//		const CFileInfo& info = dirList[y];
					//		string path = info.m_filePath;
					//		int year = ToInt(path.substr(14, 4));
					//		if (year >= firstYear && year <= lastYear)
					//		{
					//			int index = year - firstYear + 1;
					//			if (toDo[index])
					//			{
					//				//msgTmp = FindFiles(pConnection, string(info.m_filePath) + "*.op.gz", fileList, callback);
					//				msgTmp = FindFiles(pConnection, string(info.m_filePath) + "gsod_" + ToString(year) + ".tar", fileList, callback);
					//				if (msgTmp)
					//				{
					//					toDo[index] = false;
					//					msg += callback.StepIt();

					//					nbRun = 0;
					//				}
					//			}
					//		}
					//	}
					//}
				}
				pConnection->Close();
				pSession->Close();

				if (!msgTmp)
					callback.AddMessage(msgTmp);
			}
			else
			{
				if (nbRun > 1 && nbRun < 20)
				{
					msg += WaitServer(10, callback);
				}
			}
		}

		callback.PopTask();

		//remove unwanted file
		if (msg)
		{
			//if (toDo[nbYears])
			//callback.AddMessage(GetString(IDS_SERVER_BUSY));

			callback.AddMessage(GetString(IDS_NB_FILES_FOUND) + ToString(fileList.size()), 1);
		}

		return msg;
	}


	ERMsg CUIMiscellaneous::FTPDownload(const string& server, const string& inputFilePath, const string& outputFilePath, CCallback& callback)
	{
		ERMsg msg;

		/*callback.PushTask("FTPTransfer...", NOT_INIT);

		string command = "\"" + GetApplicationPath() + "External\\FTPTransfer.exe\" -Server \"" + server + "\" -Remote \"" + inputFilePath + "\" -Local \"" + outputFilePath + "\" -Passive -Download";

		UINT show = APP_VISIBLE && as<bool>(SHOW_PROGRESS) ? SW_SHOW : SW_HIDE;

		DWORD exitCode = 0;
		msg = WinExecWait(command, GetTempPath(), show, &exitCode);
		if (msg && exitCode != 0)
			msg.ajoute("FTPTransfer as exit with error code " + ToString(int(exitCode)));

		callback.PopTask();*/

		callback.PushTask("Russia FTP Transfer", NOT_INIT);

		string workingDir = GetPath(outputFilePath);
		string scriptFilePath = workingDir + m_name + "_script.txt";
		WBSF::RemoveFile(scriptFilePath + ".log");


		ofStream stript;
		msg = stript.open(scriptFilePath);
		if (msg)
		{
			stript << "open ftp:anonymous:public@" << server << endl;

			stript << "cd \"" << GetPath(inputFilePath) << "\"" << endl;
			stript << "lcd \"" << GetPath(outputFilePath) << "\"" << endl;
			stript << "get" << " \"" << GetFileName(outputFilePath) << "\"" << endl;
			stript << "exit" << endl;
			stript.close();

			UINT show = APP_VISIBLE && as<bool>(SHOW_PROGRESS) ? SW_SHOW : SW_HIDE;
			bool bShow = as<bool>(SHOW_PROGRESS);
			string command = "\"" + GetApplicationPath() + "External\\WinSCP.exe\" " + string(bShow ? "/console " : "") + " /passive=on" + " /log=\"" + scriptFilePath + ".log\" /ini=nul /script=\"" + scriptFilePath + "\"";

			DWORD exitCode = 0;
			msg = WinExecWait(command.c_str(), GetApplicationPath().c_str(), show, &exitCode);
			if (msg && exitCode != 0)
			{
				msg.ajoute("WinSCP as exit with error code " + ToString((int)exitCode));
				msg.ajoute("See log file: " + scriptFilePath + ".log");
			}

		}

		callback.PopTask();

		return msg;
	}

	ERMsg CUIMiscellaneous::Uncompress(const string& filePathZip, const string& outputPath, CCallback& callback)
	{
		ERMsg msg;

		callback.PushTask(GetString(IDS_UNZIP_FILE), NOT_INIT);



		DWORD exitCode = 0;
		string command = GetApplicationPath() + "External\\7za.exe e \"" + filePathZip + "\" -y -o\"" + outputPath + "\"";
		msg = WinExecWait(command, outputPath, SW_HIDE, &exitCode);

		if (msg && exitCode != 0)
			msg.ajoute("7za.exe as exit with error code " + ToString(int(exitCode)));


		callback.PopTask();


		return msg;
	}

	ERMsg CUIMiscellaneous::Execute(CCallback& callback)
	{
		ERMsg msg;

		size_t dataset = as<size_t>(DATASET);
		if (dataset >= NB_DATASETS)
		{
			msg.ajoute("Invalid dataset. Select a dataset.");
			return msg;
		}

		string workingDir = GetDir(WORKING_DIR);
		msg = CreateMultipleDir(workingDir);


		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(workingDir, 1);
		callback.AddMessage(GetString(IDS_UPDATE_FROM));
		callback.AddMessage(string(SERVER_NAME[dataset]) + "/" + SERVER_PATH[dataset], 1);
		callback.AddMessage("");


		//update station list
		CFileInfoVector fileList;
		msg = UpdateStationLocations();

		if (msg)
			msg = LoadStationsLocations(callback);

		if (msg)
			msg = GetFileList(fileList, callback);

		if (!msg)
			return msg;


		callback.PushTask(GetString(IDS_UPDATE_FILE), fileList.size());

		size_t curI = 0;

		for (size_t i = 0; i < fileList.size() && msg; i++)
		{
			msg += callback.StepIt(0);

			string outputFilePath = GetOutputFilePath(fileList[i]);
			string outputPath = GetPath(outputFilePath);
			CreateMultipleDir(outputPath);

			switch (dataset)
			{
			case CDIAC_RUSSIA: msg = FTPDownload(SERVER_NAME[dataset], fileList[i].m_filePath, outputFilePath, callback); break;
			case SOPFEU_HISTORICAL: break;
			case QUEBEC_HOURLY:break;
			case CWEEDS: break;
			case RCM4_22:break;
			case CWFIS: break;
			}

			//unzip 
			if (msg)
			{
				curI++;
				switch (dataset)
				{
				case -1:
					ASSERT(FileExists(outputFilePath));
					msg = Uncompress(outputFilePath, outputPath, callback);
					msg += callback.StepIt();

				default:;//do nothing by default

				}
			}
		}

		callback.AddMessage(GetString(IDS_NB_FILES_DOWNLOADED) + ToString(curI), 1);
		callback.PopTask();

		return msg;
	}


	string CUIMiscellaneous::GetOutputFilePath(const string& stationName, int year)const
	{
		size_t dataset = as<size_t>(DATASET);
		ASSERT(dataset < NB_DATASETS);

		string filePath;
		switch (dataset)
		{
		case CDIAC_RUSSIA:  filePath = GetDir(WORKING_DIR) + "Russia_518_data.txt.gz"; break;
		case SOPFEU_HISTORICAL:   filePath = GetDir(WORKING_DIR) + "BioSIM_SOPFEU.csv"; break;
		case QUEBEC_HOURLY: filePath = GetDir(WORKING_DIR) + stationName.substr(4, 2) + "/" + stationName + ".WY3"; break;
		case CWEEDS: break;
		case RCM4_22:break;
		case CWFIS: break;
		}


		return filePath;
	}

	string CUIMiscellaneous::GetOutputFilePath(const CFileInfo& info)const
	{
		string title = GetFileTitle(info.m_filePath);
		size_t dataset = as<size_t>(DATASET);
		ASSERT(dataset < NB_DATASETS);



		string filePath;
		switch (dataset)
		{
		case CDIAC_RUSSIA: filePath = GetDir(WORKING_DIR) + "Russia_518_data.txt.gz"; break;
		case SOPFEU_HISTORICAL:   break;
		case QUEBEC_HOURLY:break;
		case CWEEDS: break;
		case RCM4_22:break;
		case CWFIS: break;
		}


		return filePath;


		return GetDir(WORKING_DIR) + title.substr(title.length() - 4) + "\\" + GetFileName(info.m_filePath);
	}

	bool CUIMiscellaneous::GetStationInformation(const string& ID, CLocation& station)const
	{
		size_t pos = NOT_INIT;

		size_t dataset = as<size_t>(DATASET);
		ASSERT(dataset < NB_DATASETS);
		if (dataset == SOPFEU_HISTORICAL)
		{
			/*StringVector tmp(ID, "_");
			string ID = tmp[0];
			CTRef date;
			date.FromFormatedString(tmp[1]);*/

			//for (auto it2 = m_stations.begin(); it2 != m_stations.end() && pos == NOT_INIT; it2++)
			//{
			//	if (it2->m_ID == ID)
			//	{

			//		CTRef begin;
			//		begin.FromFormatedString(it2->GetSSI("Ouverture"));
			//		//CTRef end = CTRef::GetCurrentTRef();
			//		//if (it2->GetSSI("Fermeture") != "NULL")
			//			//end.FromFormatedString(it2->GetSSI("Fermeture"));

			//		if (date == begin)
			//		{
			//			station = *it2;
			//			pos = std::distance(m_stations.begin(), it2);
			//		}
			//	}
			//}
			for (auto it2 = m_stations.begin(); it2 != m_stations.end() && pos == NOT_INIT; it2++)
			{
				string id = it2->m_ID + "_" + it2->GetSSI("Ouverture");
				if (id == ID)
				{
					station = *it2;
					station.m_ID = ID;
					pos = std::distance(m_stations.begin(), it2);
				}
				
				//station.SetSSI("Provider", "SOPFEU");
				station.SetSSI("Network", it2->GetSSI("Provenance"));
				station.SetSSI("Country", "Canada");
				station.SetSSI("SubDivision", it2->GetSSI("Province"));
			}
		}
		else
		{
			pos = m_stations.FindPosByID(ID);
			if (pos != NOT_INIT)
			{
				station = m_stations[pos];
			}
		}
		//else
		//{
		//	auto it2 = m_stations.FindBySSI("NumID",ID, false);
		//	if (it2 != m_stations.end())
		//		station = *it2;
		//}


		return pos != NOT_INIT;
	}


	ERMsg CUIMiscellaneous::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;

		size_t dataset = as<size_t>(DATASET);
		if (dataset >= NB_DATASETS)
		{
			msg.ajoute("Invalid source dataset. Select a dataset.");
			return msg;
		}

		msg = m_stations.Load(GetLocationsFilePath(dataset, true), ",", callback);




		if (msg)
		{
			switch (dataset)
			{

			case CDIAC_RUSSIA:
			{
				if (m_weatherStations.empty())
					msg = LoadRussiaInMemory(callback);

				stationList.resize(m_stations.size());
				for (size_t i = 0; i < m_stations.size(); i++)
					stationList[i] = m_stations[i].m_ID;

				break;
			}

			case SOPFEU_HISTORICAL:
			{
				if (SOPFEU_stations.empty())
				{
					for (auto it1 = m_stations.begin(); it1 != m_stations.end(); it1++)
					{

						CTRef begin;
						begin.FromFormatedString(it1->GetSSI("Ouverture"));
						CTRef end = CTRef::GetCurrentTRef();
						if (it1->GetSSI("Fermeture") != "NULL")
							end.FromFormatedString(it1->GetSSI("Fermeture"));

						CTPeriod p1(begin, end);
						if (begin > end)
							callback.AddMessage("inversion: " + it1->m_ID);
						if (p1.GetNbDay() == 1)
							callback.AddMessage("Periode de 1 jour: " + it1->m_ID);


						if (begin <= CTRef(1994, DECEMBER, DAY_31))
						{
							ASSERT(end <= CTRef(1994, DECEMBER, DAY_31));
						}
						if (p1.GetNbDay() > 1 && begin.GetYear() >= 1995)
						{
							for (auto it2 = SOPFEU_stations[it1->m_ID].begin(); it2 != SOPFEU_stations[it1->m_ID].end(); it2++)
							{
								CTRef begin;
								begin.FromFormatedString(it2->GetSSI("Ouverture"));
								CTRef end = CTRef::GetCurrentTRef();
								if (it2->GetSSI("Fermeture") != "NULL")
									end.FromFormatedString(it2->GetSSI("Fermeture"));

								CTPeriod p2(begin, end);
								if (p1.IsIntersect(p2) && p1.Intersect(p2).GetNbDay() > 1)
								{
									if (it1->GetDistance(*it2, false, false) > 5000 || fabs(it1->m_elev - it2->m_elev) > 50)
										callback.AddMessage("Conflict: " + it1->m_ID);
									else
										callback.AddMessage("Intersect: " + it1->m_ID);

								}

							}

							SOPFEU_stations[it1->m_ID].push_back(*it1);
						}

					}
				}

				msg = LoadSOPFEUInMemory(callback);
				for (auto it = m_weatherStations.begin(); it != m_weatherStations.end(); it++)
				{
					bool bFind = false;
					for (auto it2 = m_stations.begin(); it2 != m_stations.end(); it2++)
					{
						string id = it2->m_ID + "_" + it2->GetSSI("Ouverture");
						if (id == it->first)
						{
							if (bFind)
								callback.AddMessage("Many station for " + it->first);
							bFind = true;
						}
					}


					if (bFind)
						stationList.push_back(it->first);
					else
						callback.AddMessage("WARNING: no station information for ID = " + it->first);

					msg = callback.StepIt(0);
				}


				break;
			}
			case QUEBEC_HOURLY:
			case CWFIS:
			{
				if (m_weatherStations.empty())
				{
					if (dataset == QUEBEC_HOURLY)
						msg = LoadQuebecInMemory(callback);
					else if (dataset == CWFIS)
						msg = LoadCWFISInMemory(callback);
				}


				for (auto it = m_weatherStations.begin(); it != m_weatherStations.end(); it++)
				{
					auto it2 = m_stations.FindPosByID(it->first);
					if (it2 != NOT_INIT)
						stationList.push_back(it->first);
					else
						callback.AddMessage("WARNING: no station information for ID = " + it->first);

					msg = callback.StepIt(0);
				}

				break;
			}
			case CWEEDS:
			{
				StringVector files = WBSF::GetFilesList(GetDir(WORKING_DIR) + "*.WY3", FILE_PATH, true);

				stationList.resize(files.size());
				for (size_t i = 0; i < files.size(); i++)
					stationList[i] = files[i];

				break;
			}

			case RCM4_22:
			{

				if (msg)
				{
					stationList.resize(m_stations.size());
					for (size_t i = 0; i < m_stations.size(); i++)
						stationList[i] = m_stations[i].m_ID;
				}
			}


			}//switch
		}//if

		return msg;
	}

	//class FindLocationByID2
	//{
	//public:

	//	FindLocationByID2(const std::string& ID) :m_ID(ID)
	//	{}

	//	bool operator ()(const std::pair<int, CLocation >& in)const{ return in.second.m_ID == m_ID; }

	//protected:
	//	std::string m_ID;
	//};


	ERMsg CUIMiscellaneous::GetWeatherStation(const std::string& ID, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;

		size_t dataset = as<size_t>(DATASET);
		ASSERT(dataset < NB_DATASETS);



		//Get station information
		GetStationInformation(ID, station);
		//station.m_name = PurgeFileName(station.m_name);



		vector<int> stationIDList;
		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYear = lastYear - firstYear + 1;
		station.CreateYears(firstYear, nbYear);

		//now extract data 
		for (size_t y = 0; y < nbYear&&msg; y++)
		{
			int year = firstYear + int(y);

			switch (dataset)
			{
			case CDIAC_RUSSIA:
			{
				ASSERT(TM.IsDaily());
				CMiWeatherStationMap::const_iterator it = m_weatherStations.find(ID);
				if (msg && it != m_weatherStations.end() && (*it).second.IsYearInit(year))
				{
					station[year] = (*it).second[year];
				}

				break;
			}
			case SOPFEU_HISTORICAL:
			case QUEBEC_HOURLY:
			{
				station.SetHourly(true);
				CMiWeatherStationMap::const_iterator it = m_weatherStations.find(ID);
				if (msg && it != m_weatherStations.end() && (*it).second.IsYearInit(year))
				{
					station[year] = (*it).second[year];
				}


				break;
			}
			case CWEEDS:
			{
				station.SetHourly(true);

				string filePath = GetOutputFilePath(ID, -999);
				ASSERT(FileExists(ID));
				msg = ReadCWEEDSData(ID, station);

			}
			case RCM4_22: break;
			case CWFIS:
			{
				station.SetHourly(true);
				CMiWeatherStationMap::const_iterator it = m_weatherStations.find(ID);
				if (msg && it != m_weatherStations.end() && (*it).second.IsYearInit(year))
				{
					station[year] = (*it).second[year];
				}


				break;
			}
			}

		}

		if (dataset == RCM4_22)
		{
			station.SetHourly(false);

			CRCM4_ESM2_NAM_25km RCM4_25km;
			RCM4_25km.m_path = GetDir(WORKING_DIR);

			msg += RCM4_25km.ExportPoint(station, CRCM4_ESM2_NAM_25km::RCP45, station.GetLocation(), callback);
		}

		//verify station is valid
		if (msg && station.HaveData())
		{
			//verify station is valid
			msg = station.IsValid();
		}

		return msg;
	}

	ERMsg CUIMiscellaneous::LoadRussiaInMemory(CCallback& callback)
	{
		ERMsg msg;



		string filePath = CUIMiscellaneous::GetOutputFilePath("", -999);

		ifStream file;
		msg = file.open(filePath, ios_base::in | ios_base::binary);
		if (msg)
		{
			callback.PushTask("Load Russia in memory", file.length());

			boost::iostreams::filtering_istreambuf in;
			in.push(boost::iostreams::gzip_decompressor());
			in.push(file);
			std::istream incoming(&in);

			string line;

			while (std::getline(incoming, line) && msg)
			{
				ASSERT(line.length() == 52);

				//int ID = ToInt(line.substr(0, 5));
				string ID = line.substr(0, 5);
				int year = ToInt(line.substr(6, 4));
				int month = ToInt(line.substr(11, 2)) - 1;
				int day = ToInt(line.substr(14, 2)) - 1;

				ASSERT(month >= 0 && month < 12);
				if (day >= 0 && day < GetNbDayPerMonth(year, month))//1900 is not leap year, but have data????
				{

					CTRef TRef(year, month, day);

					string Tmin = line.substr(19, 5);
					string Tmean = line.substr(27, 5);
					string Tmax = line.substr(35, 5);
					string ppt = line.substr(43, 5);

					if (Tmean != "-99.9")
						m_weatherStations[ID][TRef].SetStat(H_TAIR, ToDouble(Tmean));

					if (Tmin != "-99.9" && Tmax != "-99.9")
					{
						if (ToDouble(Tmin) < ToDouble(Tmax))
						{
							m_weatherStations[ID][TRef].SetStat(H_TMIN, ToDouble(Tmin));
							m_weatherStations[ID][TRef].SetStat(H_TMAX, ToDouble(Tmax));
						}
					}

					if (ppt != "-99.9")
					{
						m_weatherStations[ID][TRef].SetStat(H_PRCP, ToDouble(ppt));
					}

					msg += callback.SetCurrentStepPos((size_t)file.tellg());
				}
			}//while data 

			callback.PopTask();
		}//if msg

		return msg;
	}


	ERMsg CUIMiscellaneous::LoadSOPFEUInMemory(CCallback& callback)
	{
		ERMsg msg;

		m_weatherStations.clear();

		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);


		string filePath = CUIMiscellaneous::GetOutputFilePath("", -999);

		ifStream file;
		msg = file.open(filePath, ios_base::in | ios_base::binary);
		if (msg)
		{
			
			callback.PushTask("Load SOPFEU in memory", file.length());
			enum TColums { C_DATE, C_HEURE, C_NO_STATION, C_STATION_ID, C_STATION_NAME, C_TMIN, C_TAIR, C_TMAX, C_PRCP, C_TDEW, C_RELH, C_WNDS, C_WNDD, NB_SOPFEU_COLUMS };
			TVarH VAR_TYPE[NB_SOPFEU_COLUMS] = { H_SKIP, H_SKIP, H_SKIP, H_SKIP,  H_SKIP, H_TMIN, H_TAIR, H_TMAX, H_PRCP, H_TDEW, H_RELH, H_WNDS, H_WNDD };
			int line = 2;
			for (CSVIterator loop(file, ";", true); loop != CSVIterator() && msg; ++loop)
			{
				ASSERT(loop->size() == NB_SOPFEU_COLUMS);
				if (loop->size() != NB_SOPFEU_COLUMS)
				{
					callback.AddMessage("Invalid line " + to_string(line));
					callback.AddMessage(loop->GetLastLine());
				}

				/*if ((*loop)[C_NO_STATION].length() != 3)
				{
					callback.AddMessage("Invalid station ID: " + (*loop)[C_NO_STATION] + " at line " + to_string(line));
					callback.AddMessage(loop->GetLastLine());
				}*/

				string dateStr = (*loop)[C_DATE];
				StringVector date(dateStr, "- :");

				int year = WBSF::as<int>(date[0]);
				size_t m = WBSF::as<size_t>(date[1]) - 1;
				size_t d = WBSF::as<size_t>(date[2]) - 1;
				size_t h = WBSF::as<size_t>((*loop)[C_HEURE]);

				ASSERT(m < 12);
				ASSERT(d < GetNbDayPerMonth(year, m));
				
				//if ((*loop)[C_NO_STATION] != "113" || year != 1997)
					//continue;

				if (year >= firstYear && year <= lastYear && (*loop)[C_NO_STATION].length() == 3)
				{
					CTRef TRef(year, m, d, h);

					
					CLocation location;


					auto it1 = SOPFEU_stations.find((*loop)[C_NO_STATION]);
					if (it1 != SOPFEU_stations.end())
					{
						for (auto it2 = it1->second.begin(); it2 != it1->second.end(); it2++)
						{
							CTRef begin;
							begin.FromFormatedString(it2->GetSSI("Ouverture"));
							CTRef end = CTRef::GetCurrentTRef();
							if (it2->GetSSI("Fermeture") != "NULL")
								end.FromFormatedString(it2->GetSSI("Fermeture"));
							CTPeriod p(begin, end);
							if (p.IsInside(TRef.as(CTM::DAILY)))
							{
								location = *it2;
							}
						}

						if (!location.IsInit())
						{
							map<size_t, CLocation> nearest;
							for (auto it2 = it1->second.begin(); it2 != it1->second.end(); it2++)
							{
								CTRef begin;
								begin.FromFormatedString(it2->GetSSI("Ouverture"));

								CTRef end = CTRef::GetCurrentTRef();
								if (it2->GetSSI("Fermeture") != "NULL")
									end.FromFormatedString(it2->GetSSI("Fermeture"));

								CTPeriod p(begin, end);

								size_t nb_days = min(abs(TRef.as(CTM::DAILY) - begin), abs(TRef.as(CTM::DAILY) - end));
								nearest[nb_days] = *it2;
							}

							if (!nearest.empty() && nearest.begin()->first < 365*2)
								location = nearest.begin()->second;
						}
					}
					

					string ID = (*loop)[C_NO_STATION] + "_" + to_string(year) + "_" + to_string(m+1);
					if (location.IsInit())
						ID = location.m_ID + "_" + location.GetSSI("Ouverture");

					for (size_t i = 0; i < NB_SOPFEU_COLUMS; i++)
					{
						if (VAR_TYPE[i] != H_SKIP)
						{
							string var = (*loop)[i];

							if (!var.empty() && var != "NULL")
							{
								if (!m_weatherStations[ID].IsHourly())
									m_weatherStations[ID].SetHourly(true);//create hourly data

								double  value = ToDouble(var);
								if (VAR_TYPE[i] == H_TMIN || VAR_TYPE[i] == H_TAIR || VAR_TYPE[i] == H_TMAX || VAR_TYPE[i] == H_TDEW)
								{
									if (value < -50 || value > 50)
										value = -999;
								}
								if (VAR_TYPE[i] == H_PRCP)
								{
									if (value < 0 || value > 50)//max of 50 mm/h
										value = -999;
								}
								if (VAR_TYPE[i] == H_WNDS)
								{
									if (value < 0 || value > 100)
										value = -999;
								}
								if (VAR_TYPE[i] == H_WNDD)
								{
									if (value < 0 || value > 360)
										value = -999;
								}

								if(value!=-999)
									m_weatherStations[ID][TRef].SetStat(VAR_TYPE[i], value);
							}

						}
					}
				}

				msg += callback.SetCurrentStepPos((size_t)file.tellg());
				line++;
			}//while data 

			callback.PopTask();
		}//if msg



		return msg;
	}

	ERMsg CUIMiscellaneous::LoadQuebecInMemory(CCallback& callback)
	{
		ERMsg msg;



		m_weatherStations.clear();


		string workingDir = GetDir(WORKING_DIR);

		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = lastYear - firstYear + 1;
		set<string> missing_info;
		CLocationVector missing;

		callback.PushTask("Find Quebec Hourly weather files", nbYears * 12);
		vector<vector<vector<StringVector>>> fileList(nbYears);

		//	m_weatherStations["0"].SetHourly(true);
			//msg = ReadMTSData("G:\\Solution-Mesonet\\Hourly\\2003\\10\\02\\20031002cyad.mts", m_weatherStations["0"], callback);

		size_t nbFiles = 0;
		for (size_t y = 0; y < nbYears&&msg; y++)
		{
			fileList[y].resize(12);
			int year = firstYear + int(y);
			for (size_t m = 0; m < 12 && msg; m++)
			{
				fileList[y][m].resize(GetNbDayPerMonth(year, m));
				for (size_t d = 0; d < GetNbDayPerMonth(year, m) && msg; d++)
				{
					string filePath = workingDir + FormatA("%4d\\%02d\\%02d\\*.mts", year, m + 1, d + 1);
					fileList[y][m][d] = WBSF::GetFilesList(filePath);

					nbFiles += fileList[y][m][d].size();

					//detect missing metadata
					for (size_t f = 0; f < fileList[y][m][d].size() && msg; f++)
					{
						string title = GetFileTitle(fileList[y][m][d][f]);
						string ID = title.substr(8, 4);

						int pos = (int)m_stations.FindPosByID(ID);
						if (pos >= m_stations.size())
						{
							if (missing_info.find(ID) == missing_info.end())
							{

								ifStream file;
								msg = file.open(fileList[y][m][d][f]);
								if (msg)
								{
									string line;
									getline(file, line);//copyright
									getline(file, line);//dateTime
									getline(file, line);//header
									getline(file, line);//first line
									StringVector elem = Tokenize(line, " ", true);
									if (elem.size() >= 6)
									{
										CLocation loc("", elem[0], ToDouble(elem[3]), ToDouble(elem[4]), ToDouble(elem[5]));
										loc.SetSSI("NumID", elem[1]);
										missing.push_back(loc);
									}
									missing_info.insert(ID);
								}//if open successfully
							}//if not already detected
						}//if missing
					}//for all files
				}

				msg += callback.StepIt();
			}
		}

		//missing.Save("D:/missing.loc");


		callback.PopTask();
		callback.PushTask("Load Quebec Hourly weather files (" + to_string(nbFiles) + " files)", nbFiles);


		for (size_t y = 0; y < fileList.size() && msg; y++)
		{
			int year = firstYear + int(y);
			for (size_t m = 0; m < fileList[y].size() && msg; m++)
			{
				for (size_t d = 0; d < fileList[y][m].size() && msg; d++)
				{
					for (size_t f = 0; f < fileList[y][m][d].size() && msg; f++)
					{
						string title = GetFileTitle(fileList[y][m][d][f]);
						string ID = title.substr(8, 4);


						if (!m_weatherStations[ID].IsHourly())
							m_weatherStations[ID].SetHourly(true);

						msg = ReadMTSData(fileList[y][m][d][f], m_weatherStations[ID], callback);


						msg += callback.StepIt();
					}
				}
			}
		}



		callback.PopTask();

		return msg;
	}

	ERMsg CUIMiscellaneous::LoadCWFISInMemory(CCallback& callback)
	{
		ERMsg msg;

		m_weatherStations.clear();


		string workingDir = GetDir(WORKING_DIR);

		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = lastYear - firstYear + 1;


		//		static const char* FILE_NAME[6] = { "cwfis_fwi2000sv3.0opEC_2015.csv","cwfis_fwi2010sopEC_2015.csv","cwfis_fwi2016.csv","cwfis_fwi2017.csv","cwfis_fwi2018.csv","cwfis_fwi2019.csv" };

		string filePath = workingDir + "cwfis_fwi*.csv";
		StringVector fileList = WBSF::GetFilesList(filePath);

		callback.PushTask("Load CWFIS hourly weather files (" + to_string(fileList.size()) + " files)", fileList.size());

		for (size_t i = 0; i < fileList.size() && msg; i++)
		{
			//CTPeriod p;
			//int year = firstYear + int(y);
//			string title = GetFileTitle(fileList[i]);

			//
			//aes","wmo","rep_date, temp, td, rh, ws, wg, wdir, pres, vis, precip, rndays, sog, ffmc, dmc, dc, bui, isi, fwi, dsr, opts, calcstatus
			enum { C_AES, C_WMO, C_REP_DATE, C_TEMP, C_TD, C_RH, C_WS, C_WG, C_WDIR, C_PRES, C_VIS, C_PRECIP, C_RNDAYS, C_SOG, C_FFMC, C_DMC, C_DC, C_BUI, C_ISI, C_FWI, C_DSR, C_OPTS, C_CALCSTATUS, NB_INPUT_HOURLY_COLUMN };
			const TVarH COL_VAR[NB_INPUT_HOURLY_COLUMN] = { H_SKIP, H_SKIP, H_SKIP, H_TAIR, H_TDEW, H_RELH, H_WNDS, H_SKIP, H_WNDD, H_PRES, H_SKIP, H_PRCP,  H_SKIP, H_SNDH, H_SKIP, H_SKIP, H_SKIP, H_SKIP, H_SKIP, H_SKIP, H_SKIP, H_SKIP, H_SKIP };
			static const char* COL_NAME[NB_INPUT_HOURLY_COLUMN] = { "aes","wmo","rep_date","temp","td","rh","ws","wg","wdir","pres","vis","precip","rndays","sog","ffmc","dmc","dc","bui","isi","fwi","dsr","opts","calcstatus" };


			//now extract data 
			ifStream file;
			msg = file.open(fileList[i]);

			if (msg)
			{
				callback.PushTask("Load CWFIS " + GetFileName(fileList[i]), (size_t)file.length());
				for (CSVIterator loop(file, ",", true); loop != CSVIterator() && msg; ++loop)
				{
					ASSERT(loop->size() == NB_INPUT_HOURLY_COLUMN);

					string ID = (*loop)[C_AES];
					StringVector dateTime((*loop)[C_REP_DATE], "- :");
					ASSERT(dateTime.size() == 6);

					//if (dateTime.size() == 6)

					int year = stoi(dateTime[0]);
					int month = stoi(dateTime[1]) - 1;
					int day = stoi(dateTime[2]) - 1;
					int hour = stoi(dateTime[3]);

					//bool bInvalidLeap = !IsLeap(year) && (day == DAY_29) && (month == FEBRUARY);
					ASSERT(month >= 0 && month < 12);
					ASSERT(day >= 0 && day < GetNbDayPerMonth(year, month));
					ASSERT(hour >= 0 && hour < 24);


					if (year >= firstYear && year <= lastYear)
					{
						CTRef TRef(year, month, day, hour);

						//string header;
						//getline(file, header);
						//StringVector headers = Tokenize(header, " ", true);
						//vector<TVarH> variables(headers.size(), H_SKIP);
						//for (size_t c = 0; c < headers.size(); c++)
						//{
						//	auto it = std::find(std::begin(COL_NAME), std::end(COL_NAME), headers[c]);
						//	if (it != std::end(COL_NAME))
						//	{
						//		size_t v1 = it - COL_NAME;
						//		size_t v = std::distance(std::begin(COL_NAME), it);
						//		ASSERT(v < NB_INPUT_HOURLY_COLUMN);
						//		variables[c] = COL_POS[v];
						//	}
						//}
						for (size_t c = 0; c < NB_INPUT_HOURLY_COLUMN; c++)
						{
							if (COL_VAR[c] != H_SKIP && !(*loop)[c].empty())
							{
								if (!m_weatherStations[ID].IsHourly())
									m_weatherStations[ID].SetHourly(true);


								double value = stod((*loop)[c]);
								//value = ConvertMTSData(variables[c], value);

								//if (IsMTSValid(variables[c], value))
								m_weatherStations[ID].GetHour(TRef)[COL_VAR[c]] = value;
							}
						}
					}//valid year

					msg += callback.StepIt(double(loop->GetLastLine().length() + 1));
				}//for all lines

				callback.PopTask();
			}//if file open

			msg += callback.StepIt();
		}//for all file

		callback.PopTask();

		return msg;
	}


	double CUIMiscellaneous::ConvertMTSData(size_t v, double value)
	{
		if (value > -996)
		{
			switch (v)
			{
			case H_WNDS: value *= 3600.0 / 1000.0; break;	//m/s -> km/h
			case H_SNDH: value = max(0.0, value); break;	//éliminate value under zéro???
			case H_PRES: value = 10 * value; break;			//kPa -> hPa
			default:;//do nothing
			}
		}

		return value;
	}

	bool CUIMiscellaneous::IsMTSValid(size_t v, double value)
	{
		bool bValid = false;
		if (value > -996)
		{
			if (value >= GetLimitH(v, 0) && value <= GetLimitH(v, 1))
				bValid = true;
		}

		return bValid;
	}

	ERMsg CUIMiscellaneous::ReadMTSData(const string& filePath, CWeatherYears& data, CCallback& callback)const
	{
		ERMsg msg;

		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);


		enum { C_STID, C_STNM, C_TIME, C_LAT, C_LON, C_ELEV, C_TAIR, C_RELH, C_TDEW, C_WDIR, C_WSPD, C_WMAX, C_TAIR3HR, C_TDEW3HR, C_RAIN1HR, C_RAIN3HR, C_RAIN, C_PRES, C_PMSL, C_PMSL3HR, C_SNOW, C_SRAD, C_TAIRMIN, C_TAIRMAX, C_PT020H, C_PT040H, C_PT050H, NB_INPUT_HOURLY_COLUMN };
		const TVarH COL_POS[NB_INPUT_HOURLY_COLUMN] = { H_SKIP, H_SKIP, H_SKIP, H_SKIP, H_SKIP, H_SKIP, H_TAIR, H_RELH, H_TDEW, H_WNDD, H_WNDS, H_SKIP, H_SKIP, H_SKIP, H_PRCP, H_SKIP, H_SKIP, H_PRES, H_SKIP, H_SKIP, H_SNOW, H_SRAD, H_TMIN, H_TMAX, H_SKIP, H_SKIP, H_SKIP };
		static const char* COL_NAME[NB_INPUT_HOURLY_COLUMN] = { "STID", "STNM", "TIME", "LAT", "LON", "ELEV", "TAIR", "RELH", "TDEW", "WDIR", "WSPD", "WMAX", "TAIR3HR", "TDEW3HR", "RAIN1HR", "RAIN3HR", "RAIN", "PRES", "PMSL", "PMSL3HR", "SNOW", "SRAD", "TAIRMIN", "TAIRMAX", "PT020H", "PT040H", "PT050H" };
		//const int COL_POS[NB_VAR_H] = { C_TAIRMIN, C_TAIR, C_TAIRMAX, C_RAIN1HR, C_TDEW, C_RELH, C_WSPD, C_WDIR, C_SRAD, C_PRES, -1, C_SNOW, -1, -1, -1, -1 };

		//now extact data 
		ifStream file;

		msg = file.open(filePath);

		if (msg)
		{

			//			CWeatherAccumulator stat(TM);

			string copyright;
			getline(file, copyright);
			string dateTimeStr;
			getline(file, dateTimeStr);
			StringVector dateTime = Tokenize(dateTimeStr, " ", true);

			if (dateTime.size() == 7)
			{
				int year = stoi(dateTime[1]);
				int month = stoi(dateTime[2]);
				int day = stoi(dateTime[3]);
				int hour = stoi(dateTime[4]);

				//cctz::time_zone zone;
				//CTimeZones::GetZone(station, zone);

				_tzset();
				tm _tm = { 0, 0, hour, day, month - 1, year - 1900, -1, -1, -1 };
				__time64_t ltime = _mkgmtime64(&_tm);

				_localtime64_s(&_tm, &ltime);
				year = _tm.tm_year + 1900;
				month = _tm.tm_mon + 1 - 1;
				day = _tm.tm_mday - 1;
				hour = _tm.tm_hour;

				//que faire pour le NB
				ASSERT(year >= firstYear - 1 && year <= lastYear);
				ASSERT(month >= 0 && month < 12);
				ASSERT(day >= 0 && day < GetNbDayPerMonth(year, month));
				ASSERT(hour >= 0 && hour < 24);

				string header;
				getline(file, header);
				StringVector headers = Tokenize(header, " ", true);
				vector<TVarH> variables(headers.size(), H_SKIP);
				for (size_t c = 0; c < headers.size(); c++)
				{
					auto it = std::find(std::begin(COL_NAME), std::end(COL_NAME), headers[c]);
					if (it != std::end(COL_NAME))
					{
						size_t v1 = it - COL_NAME;
						size_t v = std::distance(std::begin(COL_NAME), it);
						ASSERT(v < NB_INPUT_HOURLY_COLUMN);
						variables[c] = COL_POS[v];
					}
				}


				size_t i = 0;

				string line;
				while (getline(file, line) && msg)
				{
					Trim(line);
					ASSERT(!line.empty());


					StringVector vars = Tokenize(line, " ", true);

					if (vars.size() >= C_TIME && vars.size() == headers.size())
					{

						int deltaHour = stoi(vars[C_TIME]) / 60;

						CTRef TRef = CTRef(year, month, day, hour) + deltaHour;
						if (TRef.GetYear() >= firstYear && TRef.GetYear() <= lastYear)
						{
							for (size_t c = 0; c < variables.size(); c++)
							{
								if (variables[c] != H_SKIP && vars[c] != "nan")
								{
									double value = stod(vars[c]);
									value = ConvertMTSData(variables[c], value);

									if (IsMTSValid(variables[c], value))
										data.GetHour(TRef)[variables[c]] = value;
								}

							}
						}
					}
					msg += callback.StepIt(0);
				}//for all line
			}//if first line

		}//if load 

		return msg;
	}

	ERMsg CUIMiscellaneous::ReadCWEEDSData(const string& filePath, CWeatherStation& station)const
	{
		ERMsg msg;

		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);


		//Extraterrestrial irradiance, kJ/m²
		//Global horizontal irradiance, kJ/m²
		//Direct normal irradiance, kJ/m²
		//Diffuse horizontal irradiance, kJ/m²

		//Global horizontal illuminance, 100 lux
		//Direct normal illuminance, 100 lux
		//Diffuse horizontal illuminance, 100 lux
		//Zenith luminance, 100 Cd / m²
		//Minutes of sunshine, 0‐60 minutes
		//Ceiling height, 10 m
		//Sky condition(see below)
		//Visibility, 100 m
		//Present Weather (see below)
		//Station pressure, 10 Pa
		//Dry bulb temperature, 0.1 C
		//Dew point temperature, 0.1 C
		//Wind direction, 0‐359 degrees
		//Wind speed, 0.1 m/s
		//Total sky cover, 0‐10 in tenths
		//Opaque sky cover, 0‐10 in tenths
		//Snow cover (0 = no snow cover, 1 = snow cover)
/*
		blank Value was observed (that is, not derived with a model and not altered).
Exception: irradiance and minutes of sunshine flags are written as blank though
they are interpolated to change the time base from local apparent to local
standard time.
A Value has been algorithmically adjusted (e.g. some values in CWEC files are
smoothed at the beginning and end of months).
E Value was missing and has been replaced by a manual estimate.
I Value was missing and has been replaced with one derived by interpolation from
neighboring observations.
M Value was missing and has been replaced with one derived with a model (model used depends on element).
Q Value is derived from other values (e.g. illuminance data which are not observed).
S Irradiance is a SUNY value.
N Value is from NARR time series.
T Value is interpolated with the specific procedure for gaps 3 hours or shorter.
9 Value is missing; data positions contain 9s as well.
*/
		enum THeader { C_PRODUCT, C_NAME, C_PROVINCE, C_CONTRY, C_ID, C_LAT, C_LON, C_TIMEZONE, C_ALT, NB_HEADER };

		//now extact data 
		ifStream file;
		msg = file.open(filePath);

		if (msg)
		{
			string header;
			getline(file, header);
			StringVector metadata = Tokenize(header, ",", false);
			ASSERT(metadata.size() == NB_HEADER);
			station.m_ID = metadata[C_ID];
			station.m_name = WBSF::UppercaseFirstLetter(metadata[C_NAME]);
			station.m_lat = WBSF::as<double>(metadata[C_LAT]);
			station.m_lon = WBSF::as<double>(metadata[C_LON]);
			station.m_alt = WBSF::as<double>(metadata[C_ALT]);
			station.SetSSI("Province", metadata[C_PROVINCE]);
			station.SetSSI("TimeZone", metadata[C_TIMEZONE]);

			string line;
			while (getline(file, line) && msg)
			{

				//Trim(line);
				if (line.length() == 120)
				{


					int year = stoi(line.substr(8, 4));
					size_t m = stoi(line.substr(12, 2)) - 1;
					size_t d = stoi(line.substr(14, 2)) - 1;
					size_t h = stoi(line.substr(16, 2)) - 1;

					//que faire pour le NB

					ASSERT(m < 12);
					ASSERT(d < GetNbDayPerMonth(year, m));
					ASSERT(h < 24);

					if (year >= firstYear && year <= lastYear)
					{
						if (line[26] != '9')
						{
							double srad = stof(line.substr(22, 4)) * 1000.0 / 3600.0;//kJ/m² -> W/m²
							station[year][m][d][h][H_SRAD] = srad;
						}

						if (line[92] != '9')
						{
							double pres = stof(line.substr(87, 5)) / 10.0;//dPa -> hPa
							station[year][m][d][h][H_PRES] = pres;
						}
						if (line[97] != '9')
						{
							double Tair = stof(line.substr(93, 4)) / 10.0;
							station[year][m][d][h][H_TAIR] = Tair;
						}
						if (line[102] != '9')
						{
							double Tdew = stof(line.substr(98, 4)) / 10.0;
							station[year][m][d][h][H_TDEW] = Tdew;
							if (!WEATHER::IsMissing(station[year][m][d][h][H_TAIR]))
								station[year][m][d][h][H_RELH] = Td2Hr(station[year][m][d][h][H_TAIR], Tdew);
						}
						if (line[106] != '9')
						{
							double wDir = stof(line.substr(103, 3));
							station[year][m][d][h][H_WNDD] = wDir;
						}
						if (line[111] != '9')
						{
							double wSpd = stof(line.substr(107, 4)) * 3600.0 / (10 * 1000.0); //[0.1 m/s] --> [km/h]
							station[year][m][d][h][H_WNDS] = wSpd;
						}
					}//if valid year
				}//line with 120 chars
			}//for all line


		}//if load 

		return msg;
	}


}


