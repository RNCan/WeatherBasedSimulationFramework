#include "StdAfx.h"
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


	const char* CUIMiscellaneous::SERVER_NAME[NB_DATASETS] = { "cdiac.ornl.gov", "" };
	const char* CUIMiscellaneous::SERVER_PATH[NB_DATASETS] = { "pub12/russia_daily/", "" };
	//const char* CUIMiscellaneous::LOCATION_PATH[NB_DATASETS] = { "pub12/russia_daily/" };
	//const char* CUIMiscellaneous::LOCATION_NAME[NB_DATASETS] = {"Russia_518_inventory.csv"};

	//*********************************************************************
	const char* CUIMiscellaneous::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "Dataset", "FirstYear", "LastYear", "ShowProgress" };
	const size_t CUIMiscellaneous::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_COMBO_INDEX, T_STRING, T_STRING, T_BOOL };
	const UINT CUIMiscellaneous::ATTRIBUTE_TITLE_ID = IDS_UPDATER_MISCELLANEOUS_P;
	const UINT CUIMiscellaneous::DESCRIPTION_TITLE_ID = ID_TASK_MISCELLANEOUS;

	const char* CUIMiscellaneous::CLASS_NAME(){ static const char* THE_CLASS_NAME = "Miscellaneous";  return THE_CLASS_NAME; }
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

		
		
		string filePath = (bLocal ? GetDir(WORKING_DIR) : string(SERVER_PATH[dataset]));

		switch (dataset)
		{
		case CDIAC_RUSSIA: filePath += "Russia_518_inventory.csv"; break;
		case SOPFEU_2013: filePath = bLocal ? GetApplicationPath() + "Layers\\SOPFEUStnDesc171005.csv": "";
		}

		return filePath;
	}

	std::string CUIMiscellaneous::Option(size_t i)const
	{
		string str;
		switch (i)
		{
		case DATASET:	str = "Russia|SOPFEU 2013"; break;
		};
		return str;
	}

	std::string CUIMiscellaneous::Default(size_t i)const
	{
		string str;

		size_t dataset = as<size_t>(DATASET);
		if (dataset >= NB_DATASETS)
			dataset = 0;


		static const char* DEFAULT_DIR[NB_DATASETS] = { "Miscellaneous\\Russia\\", "Miscellaneous\\SOPFEU" };
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

			msg = GetFtpConnection(SERVER_NAME[dataset], pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", true);
			if (msg)
			{

				CFileInfoVector fileList;
				msg = FindFiles(pConnection, path, fileList, CCallback::DEFAULT_CALLBACK);
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
							case SOPFEU_2013:
							{

							}

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

			ERMsg msgTmp = GetFtpConnection(SERVER_NAME[dataset], pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", true);
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

						msgTmp = FindFiles(pConnection, string(SERVER_PATH[dataset]) + "Russia_518_data.txt.gz", fileList, callback);
						break;

					}
					case SOPFEU_2013:
					{

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
					callback.AddMessage("Waiting 30 seconds for server...");
					for (int i = 0; i < 60 && msg; i++)
					{
						Sleep(500);//wait 500 milisec
						msg += callback.StepIt(0);
					}
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

		callback.PushTask("FTPTransfer...", NOT_INIT);

		string command = "\"" + GetApplicationPath() + "External\\FTPTransfer.exe\" -Server \"" + server + "\" -Remote \"" + inputFilePath + "\" -Local \"" + outputFilePath + "\" -Passive -Download";

		UINT show = APP_VISIBLE && as<bool>(SHOW_PROGRESS) ? SW_SHOW : SW_HIDE;

		DWORD exitCode = 0;
		msg = WinExecWait(command, GetTempPath(), show, &exitCode);
		if (msg && exitCode != 0)
			msg.ajoute("FTPTransfer as exit with error code " + ToString(int(exitCode)));

		callback.PopTask();

		return msg;
	}
	
	ERMsg CUIMiscellaneous::Uncompress(const string& filePathZip, const string& outputPath, CCallback& callback)
	{
		ERMsg msg;

		callback.PushTask(GetString(IDS_UNZIP_FILE), NOT_INIT);

		
	
		DWORD exitCode = 0;
		string command = GetApplicationPath() + "External\\7z.exe e \"" + filePathZip + "\" -y -o\"" + outputPath + "\"";
		msg = WinExecWait(command, outputPath, SW_HIDE, &exitCode);

		if (msg && exitCode != 0)
			msg.ajoute("7z.exe as exit with error code " + ToString(int(exitCode)));


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
			case SOPFEU_2013: break;
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
		case SOPFEU_2013:   filePath = GetDir(WORKING_DIR) + "MeteoOBS2013.csv"; break;
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
		case SOPFEU_2013:   break;
		}


		return filePath;


		return GetDir(WORKING_DIR) + title.substr(title.length() - 4) + "\\" + GetFileName(info.m_filePath);
	}
	
	bool CUIMiscellaneous::GetStationInformation(const string& ID, CLocation& station)const
	{
		
		size_t pos = m_stations.FindByID(ID);
		if (pos != NOT_INIT)
		{
			station = m_stations[pos];
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
				stationList.resize(m_stations.size());
				for (size_t i = 0; i < m_stations.size(); i++)
					stationList[i] = m_stations[i].m_ID;
				break;
			}

			case SOPFEU_2013:
			{
				if (m_weatherStations.empty())
					msg = LoadSOPFEUInMemory(callback);

				for (auto it = m_weatherStations.begin(); it != m_weatherStations.end(); it++)
				{
					auto it2 = m_stations.FindByID( to_string(it->first));
					if (it2 != NOT_INIT)
						stationList.push_back(to_string(it->first));
					else
						callback.AddMessage("WARNING: no station information for ID = " + ToString(it->first));

					msg = callback.StepIt(0);
				}

				break;
			}
			}//switch
		}//ifm
		
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
				if (m_weatherStations.empty())
					msg = LoadRussiaInMemory(callback);
				
				CMiWeatherStationMap::const_iterator it = m_weatherStations.find(ToInt(ID));
				if (msg && it != m_weatherStations.end() && (*it).second.IsYearInit(year))
				{
					station[year] = (*it).second[year];
				}
				
				break;
			}
			case SOPFEU_2013:
			{
				CMiWeatherStationMap::const_iterator it = m_weatherStations.find(ToInt(ID));
				if (msg && it != m_weatherStations.end() && (*it).second.IsYearInit(year))
					station[year] = (*it).second[year];

				break;
			}
			default:
			{
				int year = firstYear + int(y);

				string filePath = GetOutputFilePath(ID, year);
				if (FileExists(filePath))
					msg = ReadData(filePath, station[year]);

				msg += callback.StepIt(0);
			}
			}
			
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
			callback.PushTask("Load Russia in memmory", file.length());

			boost::iostreams::filtering_istreambuf in;
			in.push(boost::iostreams::gzip_decompressor());
			in.push(file);
			std::istream incoming(&in);

			string line;

			//std::getline(incoming, line);
			while (std::getline(incoming, line) && msg)
			{
				ASSERT(line.length() == 52);

				int ID = ToInt(line.substr(0, 5));
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
						m_weatherStations[ID][TRef].SetStat(H_TAIR2, ToDouble(Tmean));

					if (Tmin != "-99.9" && Tmax != "-99.9")
					{
						if (ToDouble(Tmin) < ToDouble(Tmax))
						{
							m_weatherStations[ID][TRef].SetStat(H_TMIN2, ToDouble(Tmin));
							m_weatherStations[ID][TRef].SetStat(H_TMAX2, ToDouble(Tmax));
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
		

		string filePath = CUIMiscellaneous::GetOutputFilePath("", -999);

		ifStream file;
		msg = file.open(filePath, ios_base::in | ios_base::binary);
		if (msg)
		{
			callback.PushTask("Load SOPFEU in memmory", file.length());
			
			enum { C_NO_STATION, C_DATE, C_HEURE, C_PLUIE, C_PLUIE_HOR, C_CC, C_TSEC, C_THUM, C_DV, C_VV, C_VVR, C_PRO, C_HR, OTHER_FIELDS };
			TVarH VAR_TYPE[OTHER_FIELDS] = { H_SKIP, H_SKIP, H_SKIP, H_SKIP, H_PRCP, H_SKIP, H_TAIR2, H_SKIP, H_WNDD, H_WNDS, H_SKIP, H_TDEW, H_RELH};
			for (CSVIterator loop(file, ",", true); loop != CSVIterator(); ++loop)
			{
				ASSERT(loop->size() >= OTHER_FIELDS);
				int ID = WBSF::as<int>((*loop)[C_NO_STATION]);
				string dateStr = (*loop)[C_DATE];
				StringVector date(dateStr, "- :");

				
				int year = WBSF::as<int>(date[0]);
				size_t m = WBSF::as<size_t>(date[1]) - 1;
				size_t d = WBSF::as<size_t>(date[2]) - 1;
				size_t h = WBSF::as<size_t>((*loop)[C_HEURE]);

				ASSERT(m < 12);
				ASSERT(d < GetNbDayPerMonth(year, m));

				

				CTRef TRef(year, m, d, h);

				for (size_t i = 0; i < OTHER_FIELDS; i++)
				{
					if (VAR_TYPE[i] != H_SKIP)
					{
						string var = (*loop)[i];
						
						if (!var.empty())
						{
							if (!m_weatherStations[ID].IsHourly())
								m_weatherStations[ID].SetHourly(true);//create hourly data

							m_weatherStations[ID][TRef].SetStat(VAR_TYPE[i], ToDouble(var));
						}
							
					}
				}
				
				msg += callback.SetCurrentStepPos((size_t)file.tellg());
			}//while data 

			callback.PopTask();
		}//if msg

		

		return msg;
	}

	ERMsg CUIMiscellaneous::ReadData(const string& filePath, CYear& data)const
	{
		ERMsg msg;

		ifStream file;
		msg = file.open(filePath, ios_base::in | ios_base::binary);
		if (msg)
		{
			boost::iostreams::filtering_istreambuf in;
			in.push(boost::iostreams::gzip_decompressor());
			in.push(file);
			std::istream incoming(&in);

			string line;

			std::getline(incoming, line);
			while (std::getline(incoming, line) && msg)
			{
				ASSERT(line.length() == 138);
				
				int year = ToInt(line.substr(14, 4));
				int month = ToInt(line.substr(18, 2)) - 1;
				int day = ToInt(line.substr(20, 2)) - 1;


				ASSERT(month >= 0 && month < 12);
				ASSERT(day >= 0 && day < GetNbDayPerMonth(year, month));

				CTRef TRef(year, month, day);

				double Tmin = -999;
				double Tmean = -999;
				double Tmax = -999;
				double ppt = -999;
				double Tdew = -999;
				double windSpeed = -999;
				double snowDept = -999;

				if (line.substr(24, 6) != "9999.9")
					Tmean = ToDouble(line.substr(24, 6));

				if (line.substr(110, 6) != "9999.9")
					Tmin = ToDouble(line.substr(110, 6));

				if (line.substr(102, 6) != "9999.9")
					Tmax = ToDouble(line.substr(102, 6));

				// A = 1 report of 6-hour precipitation 
				//     amount.
				// B = Summation of 2 reports of 6-hour 
				//     precipitation amount.
				// C = Summation of 3 reports of 6-hour 
				//     precipitation amount.
				// D = Summation of 4 reports of 6-hour 
				//     precipitation amount.
				// E = 1 report of 12-hour precipitation
				//     amount.
				// F = Summation of 2 reports of 12-hour
				//     precipitation amount.
				// G = 1 report of 24-hour precipitation
				//     amount.
				// H = Station reported '0' as the amount
				//     for the day (eg, from 6-hour reports),
				//     but also reported at least one
				//     occurrence of precipitation in hourly
				//     observations--this could indicate a
				//     trace occurred, but should be considered
				//     as incomplete data for the day.
				// I = Station did not report any precip data
				//     for the day and did not report any
				//     occurrences of precipitation in its hourly
				//     observations--it's still possible that
				//     precip occurred but was not reported.


				char pptFlag = line[123];
				if (pptFlag != 'I' && pptFlag != 'H' &&line.substr(118, 5) != "99.99")
					ppt = ToDouble(line.substr(118, 5));

				if (line.substr(35, 6) != "9999.9")
					Tdew = ToDouble(line.substr(35, 6));

				if (line.substr(78, 5) != "999.9")
					windSpeed = ToDouble(line.substr(78, 5));

				if (line.substr(125, 5) != "999.9")
					snowDept = ToDouble(line.substr(125, 5));


				if (Tmean > -999 )
				{
					double TmeanC = ((Tmean - 32.0)*5.0 / 9.0);
					data[TRef][H_TAIR2] = TmeanC;
				}

				if (Tmin > -999 && Tmin < 999 &&
					Tmax > -999 && Tmax < 999)
				{
					assert(Tmin<Tmax);
					if (Tmin > Tmax)
						Switch(Tmin, Tmax);

					double TminC = ((Tmin - 32.0)*5.0 / 9.0);
					double TmaxC = ((Tmax - 32.0)*5.0 / 9.0);
					data[TRef][H_TMIN2] = TminC;
					data[TRef][H_TMAX2] = TmaxC;
				}

				if (ppt >= 0)
				{
					data[TRef][H_PRCP] = (ppt*25.40);
				}


				if (Tdew > -999 && Tdew < 999)
				{
					data[TRef][H_TDEW] = ((Tdew - 32.0)*5.0 / 9.0);

					//here relative humidity is compute from DewPoint and Tmean (important to take Tmean)
					if (Tmean != -999)
						data[TRef][H_RELH] = Td2Hr((Tmean - 32.0)*5.0 / 9.0, (Tdew - 32.0)*5.0 / 9.0);
				}


				if (windSpeed >= 0)
				{
					//knot
					//1 Knot = 1 Nautical Mile per hour
					//1 Nautical mile = 6076.12 ft. = 1852.184256 (m) = 1.852184256 (km)
					data[TRef][H_WNDS] = (windSpeed*1.852184256);
				}

				if (snowDept >= 0)
				{
					//data[TRef][H_SNDH] = snowDept*???;
				}
			}
		}

		return msg;
	}

}