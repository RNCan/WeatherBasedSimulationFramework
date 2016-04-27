//usager : MeteoR
//PW: LeSoleilBrille

#include "StdAfx.h"
#include "UISOPFEUHourly.h"


#include "Basic/HourlyDatabase.h"
#include "Basic/CSV.h"
#include "Basic/FileStamp.h"
#include "UI/Common/SYShowMessage.h"
#include "TaskFactory.h"
#include "Geomatic/TimeZones.h"

#include "../Resource.h"



using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace UtilWWW;

namespace WBSF
{

	const char* CUISOPFEUHourly::SERVER_NAME = "FTP3.sopfeu.qc.ca";
	const char* CUISOPFEUHourly::SERVER_PATH = "RMCQ/";
	//const char* CUISOPFEUHourly::LIST_PATH = "pub/data/noaa/";
	const char * CUISOPFEUHourly::FIELDS_NAME[NB_FIELDS] =
	{ "TAI000H", "TAN000H", "TAX000H", "TAM000H", "PC040H", "PT040H", "PC020H", "PT041H", "HAI000H", "HAN000H", "HAX000H", "NSI000H", "VDI300H", "VVI300H", "VVXI500H", "VDM025B", "VVM025B", "TDI000H", "VB000B" };
	//{ "TAi000H", "TAn000H", "TAx000H", "TAm000H", "PC040H", "PT040H", "PC020H", "PT041H", "HAi000H", "HAn000H", "HAx000H", "NSi000H", "VDi300H", "VVi300H", "VVxi500H", "VDm025B", "VVm025B", "TDi000H", "VB000B" };
	const TVarH CUISOPFEUHourly::VARIABLE[NB_FIELDS] = { H_TAIR, H_TAIR, H_TAIR, H_TAIR, H_SKIP, H_PRCP, H_SKIP, H_SWE, H_RELH, H_RELH, H_RELH, H_SNDH, H_WNDD, H_WNDS, H_SKIP, H_SKIP, H_WND2, H_TDEW, H_SKIP };

	TVarH CUISOPFEUHourly::GetVariable(std::string str)
	{
		str = MakeUpper(str);
		TVarH v = H_SKIP;
		for (size_t i = 0; i < NB_FIELDS&&v == NOT_INIT; i++)
			if (str == FIELDS_NAME[i])
				v = VARIABLE[i];

		return v;
	}

	//*********************************************************************
	const char* CUISOPFEUHourly::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "UserName", "Password", "WorkingDir", "FirstYear", "LastYear"};
	const size_t CUISOPFEUHourly::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_STRING, T_PASSWORD, T_PATH, T_STRING, T_STRING };
	const UINT CUISOPFEUHourly::ATTRIBUTE_TITLE_ID = IDS_UPDATER_SOPFEU_P;
	const UINT CUISOPFEUHourly::DESCRIPTION_TITLE_ID = ID_TASK_SOPFEU;

	const char* CUISOPFEUHourly::CLASS_NAME(){ static const char* THE_CLASS_NAME = "SOPFEU";  return THE_CLASS_NAME; }
	CTaskBase::TType CUISOPFEUHourly::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUISOPFEUHourly::CLASS_NAME(), (createF)CUISOPFEUHourly::create);



	//static const CGeoRect DEFAULT_BOUDINGBOX(-180, -90, 180, 90, PRJ_WGS_84);

	CUISOPFEUHourly::CUISOPFEUHourly(void)
	{}

	CUISOPFEUHourly::~CUISOPFEUHourly(void)
	{}


	//std::string CUISOPFEUHourly::Option(size_t i)const
	//{
	//	string str;
	//	/*switch (i)
	//	{
	//	case COUNTRIES:	str = CCountrySelection::GetAllPossibleValue(); break;
	//	case STATES:	str = CStateSelection::GetAllPossibleValue(); break;
	//	};*/
	//	return str;
	//}

	std::string CUISOPFEUHourly::Default(size_t i)const
	{
		string str;

		switch (i)
		{
		case FIRST_YEAR:
		case LAST_YEAR:	str = ToString(CTRef::GetCurrentTRef().GetYear()); break;
		};

		return str;
	}

	//*****************************************************************************

	string CUISOPFEUHourly::GetStationListFilePath()const
	{
		return GetApplicationPath() + "Layers\\SOPFEUStations.csv";
	}

	string CUISOPFEUHourly::GetOutputFilePath(CTRef TRef)const
	{
		return WBSF::FormatA("%s%d/m%4d-%02d-%02d-%02d.csv", GetDir(WORKING_DIR).c_str(), TRef.GetYear(), TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, TRef.GetHour());
	}
/*

	ERMsg CUISOPFEUHourly::UpdateStationHistory()
	{
		ERMsg msg;

		CInternetSessionPtr pSession;
		CFtpConnectionPtr pConnection;

		msg = GetFtpConnection(SERVER_NAME, pConnection, pSession);
		if (msg)
		{
			string path = GetHistoryFilePath(false);

			CFileInfoVector fileList;
			msg = FindFiles(pConnection, path, fileList, CCallback::DEFAULT_CALLBACK);
			if (msg)
			{
				ASSERT(fileList.size() == 1);

				string outputFilePath = GetHistoryFilePath(true);
				if (!IsFileUpToDate(fileList[0], outputFilePath))
					msg = CopyFile(pConnection, fileList[0].m_filePath, outputFilePath);
			}

			pConnection->Close();
			pSession->Close();
		}

		return msg;
	}
*/
	ERMsg CUISOPFEUHourly::GetFileList(CFileInfoVector& fileList, CCallback& callback)const
	{
		ERMsg msg;


		fileList.clear();
		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = lastYear - firstYear + 1;

		callback.PushTask(GetString(IDS_LOAD_FILE_LIST), 1);
		//callback.SetNbStep(nbYears);
		bool bLoaded = false;
		int nbRun = 0;

		CFileInfoVector dirList;
		while (!bLoaded && nbRun < 20 && msg)
		{
			nbRun++;

			//open a connection on the server
			CInternetSessionPtr pSession;
			CFtpConnectionPtr pConnection;

			ERMsg msgTmp = GetFtpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, Get(USER_NAME), Get(PASSWORD));
			if (msgTmp)
			{
				pSession->SetOption(INTERNET_OPTION_RECEIVE_TIMEOUT, 15000);

			
				msgTmp = FindFiles(pConnection, "RMCQ/*.17r", fileList, callback);
				if (msgTmp)
				{
					msg += callback.StepIt();
					bLoaded = true;
					nbRun = 0;
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
					callback.PushTask("Waiting 30 seconds for server...", 600);
					for (int i = 0; i < 600 && msg; i++)
					{
						Sleep(50);//wait 50 milisec
						msg += callback.StepIt();
					}
					callback.PopTask();
				}
			}
		}


		callback.PopTask();

		//remove unwanted file
		if (msg)
		{
			if (!bLoaded)
				callback.AddMessage(GetString(IDS_SERVER_BUSY));

			callback.AddMessage(GetString(IDS_NB_FILES_FOUND) + ToString(fileList.size()));
			msg = CleanList(fileList, callback);
		}

		return msg;
	}

	/*
	ERMsg CUISOPFEUHourly::CleanList(StringVector& fileList, CCallback& callback)const
	{
		ERMsg msg;

		callback.PushTask(GetString(IDS_CLEAN_LIST), fileList.size());

		for (StringVector::const_iterator it = fileList.begin(); it != fileList.end() && msg;)
		{
			string fileTitle = GetFileTitle(*it);

			if (IsFileInclude(fileTitle))
				it++;
			else
				it = fileList.erase(it);


			msg += callback.StepIt();
		}

		callback.PopTask();
		return msg;
	}*/

	CTRef GetTRef(string& fileTitle)
	{
		int year = 2000 + WBSF::as<int>(fileTitle.substr(1, 2));
		size_t jDay = WBSF::as<size_t>(fileTitle.substr(3, 3))-1;
		size_t h = WBSF::as<size_t>(fileTitle.substr(6, 2));

		CJDayRef JDay(year, jDay);
		return CTRef(JDay.GetYear(), JDay.GetMonth(), JDay.GetDay(), h);
	}

	ERMsg CUISOPFEUHourly::CleanList(CFileInfoVector& fileList, CCallback& callback)const
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);

		callback.PushTask(GetString(IDS_CLEAN_LIST), fileList.size());

		for (CFileInfoVector::const_iterator it = fileList.begin(); it != fileList.end() && msg;)
		{
			string fileTitle = GetFileTitle(it->m_filePath);
			string stationID = fileTitle.substr(0, 12);
			CTRef TRef = GetTRef(fileTitle);
			string outputFilePath = GetOutputFilePath(TRef);

			if (IsFileUpToDate(*it, outputFilePath, false))
				it = fileList.erase(it);
			else
				it++;


			msg += callback.StepIt();
		}

		callback.AddMessage(GetString(IDS_NB_FILES_CLEARED) + ToString(fileList.size()));
		callback.AddMessage("");

		callback.PopTask();

		return msg;
	}

	ERMsg CUISOPFEUHourly::Execute(CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);
		msg = CreateMultipleDir(workingDir);


		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(workingDir, 1);
		callback.AddMessage(GetString(IDS_UPDATE_FROM));
		callback.AddMessage(string(SERVER_NAME) + "/" + SERVER_PATH , 1);
		callback.AddMessage("");

		//load station list
		CFileInfoVector fileList;
		//msg = UpdateStationHistory();

		if (msg)
			msg = GetFileList(fileList, callback);

		if (msg)
		{
			CreateMultipleDir(GetDir(WORKING_DIR));

			callback.PushTask(GetString(IDS_UPDATE_FILE), fileList.size());

			int nbRun = 0;
			size_t curI = 0;

			while (curI < fileList.size() && nbRun < 20 && msg)
			{
				nbRun++;

				CInternetSessionPtr pSession;
				CFtpConnectionPtr pConnection;

				ERMsg msgTmp = GetFtpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, Get(USER_NAME), Get(PASSWORD));
				if (msgTmp)
				{
					TRY
					{
						for (size_t i = curI; i < fileList.size() && msgTmp && msg; i++)
						{
							string fileTitle = GetFileTitle(fileList[i].m_filePath);

							//string stationID = fileTitle.substr(0, 12);
							CTRef TRef = GetTRef(fileTitle);

							string outputFilePath = GetOutputFilePath(TRef);
							CreateMultipleDir(GetPath(outputFilePath));

							msgTmp += CopyFile(pConnection, fileList[i].m_filePath, outputFilePath, INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_DONT_CACHE);
							
							if (msgTmp)
							{
								msg += callback.StepIt();
								curI++;
								nbRun = 0;
							}
						}
					}
					CATCH_ALL(e)
					{
						msgTmp = UtilWin::SYGetMessage(*e);
					}
					END_CATCH_ALL

					//clean connection
					pConnection->Close();
					pSession->Close();

					if (!msgTmp)
						callback.AddMessage(FormatMsg(IDS_UPDATE_END, ToString(curI), ToString(fileList.size())));
				}
				else
				{
					if (nbRun > 1 && nbRun < 20)
					{
						
						callback.PushTask("Waiting 30 seconds for server...", 600);
						for (size_t i = 0; i < 600 && msg; i++)
						{
							Sleep(50);//wait 50 milisec
							msg += callback.StepIt();
						}
						callback.PopTask();
					}
				}
			}

			callback.AddMessage(GetString(IDS_NB_FILES_DOWNLOADED) + ToString(curI));
			callback.PopTask();
		}

		return msg;
	}


	ERMsg CUISOPFEUHourly::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;
		
		//send all available station
		msg = m_stationsList.Load(GetStationListFilePath());
		if (msg)
		{
			msg = LoadWeatherInMemory(callback);
			if (msg)
			{
				stationList.reserve(m_stations.size());
				for (CWeatherStationMap::const_iterator it = m_stations.begin(); it != m_stations.end(); it++)
					stationList.push_back(it->first);
			}
		}

		

		return msg;

		
	}

	ERMsg CUISOPFEUHourly::LoadWeatherInMemory(CCallback& callback)
	{
		ERMsg msg;

		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = lastYear - firstYear + 1;

		for (size_t y = 0; y < nbYears&& msg; y++)
		{
			int year = firstYear + int(y);

			//on pourrait optimiser en loadant une seul foisa tout les fichiers
			StringVector fileList = GetFilesList(GetDir(WORKING_DIR) + to_string(year) + "\\" + "*.csv");
			callback.PushTask("Load data for year = " + to_string(year), fileList.size());

			for (size_t i = 0; i < fileList.size() && msg; i++)
			{
				msg += ReadData(fileList[i], m_stations, callback);
				msg += callback.StepIt();
			}

			callback.PopTask();

		}


		return msg;
	}

	ERMsg CUISOPFEUHourly::GetWeatherStation(const string& ID, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;

		/*size_t pos= m_stationsList.FindByID(ID);
		if (pos == NOT_INIT)
		{
			msg.ajoute(FormatMsg(IDS_NO_STATION_INFORMATION, ID));
			return msg;
		}

		((CLocation&)station) = m_stationsList[pos];

		if (m_stations.find(ID) != m_stations.end())*/
		station = m_stations[ID];

		//station.m_name = TraitFileName(station.m_name);
		//station.m_name = PurgeFileName(station.m_name);


		//int timeZone = (int)Round(station.m_lon / 15);
		//int firstYear = as<int>(FIRST_YEAR);
	//	int lastYear = as<int>(LAST_YEAR);
		//size_t nbYears = lastYear - firstYear + 1;
		//station.CreateYears(firstYear, nbYears);

		//now extact data
		//CWeatherAccumulator accumulator(TM);
		//for (size_t y = 0; y < nbYears&& msg; y++)
		//{
		//	int year = firstYear + int(y);
		//	
		//	//on pourrait optimiser en loadant une seul foisa tout les fichiers
		//	StringVector fileList = GetFilesList(GetDir(WORKING_DIR) + to_string(year) + "\\" + "*.csv");
		//	callback.PushTask("Load data for year = " + to_string(year), fileList.size());
		//	for (size_t i = 0; i < fileList.size(); i++)
		//		msg += ReadData(fileList[i], timeZone, station, callback);

		//	callback.PopTask();
		//	
		//}

		//if (accumulator.GetTRef().IsInit())
		//{
		//	CTPeriod period(CTRef(firstYear, 0, 0, 0, TM), CTRef(lastYear, LAST_MONTH, LAST_DAY, LAST_HOUR, TM));
		//	if (period.IsInside(accumulator.GetTRef()))
		//		station[accumulator.GetTRef()].SetData(accumulator);
		//}

	//	ASSERT(station.GetEntireTPeriod().GetFirstYear() >= firstYear && station.GetEntireTPeriod().GetLastYear() <= lastYear);


		if (msg && station.HaveData())
			msg = station.IsValid();

		return msg;
	}

	

	ERMsg CUISOPFEUHourly::ReadData(const string& filePath, CWeatherStationMap& stations, CCallback& callback)const
	{
		ERMsg msg;

		StringVector str(GetFileTitle(filePath), "-");
		ASSERT(str.size() == 4);
		ASSERT(str[0][0] == 'm');
		ASSERT(str[0].size() == 5);
		str[0].erase(str[0].begin());//remove m

		CTRef UTCRef(ToInt(str[0]), ToSizeT(str[1])-1, ToSizeT(str[2])-1, ToSizeT(str[3]));

		ifStream  file;
		msg = file.open(filePath);
		if (msg)
		{
			enum {PROV, STA_ID, DATE, F_TIME, FIRST_FILED};
			for(CSVIterator loop(file, ";", false); loop!=CSVIterator(); ++loop)
			{
				ASSERT(loop->size() >= FIRST_FILED);
				string ID = (*loop)[STA_ID];
				MakeLower(ID);
				//StringVector time((*loop)[F_TIME], "|;");
				//ASSERT(time.size()==2);
				
				size_t pos = m_stationsList.FindByID(ID);
				if (pos != NOT_INIT)
				{
					CTRef TRef = CTimeZones::UTCTRef2LocalTRef(UTCRef, m_stationsList[pos]);
					for (size_t c = FIRST_FILED; c < loop->size() - 2; c += 3)
					{
						TVarH v = GetVariable((*loop)[c]);
						string flag = (*loop)[c + 1];
						string val = (*loop)[c + 2];
						ASSERT(!val.empty());

						if (v != H_SKIP && flag != "R" && !val.empty())
						{
							float value = ToFloat(val);
							if (stations.find(ID) == stations.end())
							{
								((CLocation&)stations[ID]) = m_stationsList[pos];
								stations[ID].SetHourly(true);
							}

							stations[ID][TRef].SetStat(v,value);
						}
					}
				}
				else
				{
					//callback.AddMessage("Undefine ID " + ID);
				}
			}//for all line
		}//if open


		return msg;
	}


}