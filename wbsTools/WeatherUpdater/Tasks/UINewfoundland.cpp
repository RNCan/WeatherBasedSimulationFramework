#include "StdAfx.h"
#include "UINewfoundland.h"
#include <boost\dynamic_bitset.hpp>
#include <boost\filesystem.hpp>

#include "UI/Common/UtilWin.h"
#include "Basic/DailyDatabase.h"
#include "Basic/FileStamp.h"
#include "UI/Common/SYShowMessage.h"
#include "Basic\CSV.h"


#include "TaskFactory.h"
#include "../Resource.h"
#include "WeatherBasedSimulationString.h"


using namespace std; 
using namespace WBSF::HOURLY_DATA;
using namespace UtilWWW;
using namespace boost;


namespace WBSF
{
	const char* CUINewfoundland::SERVER_NAME = "Ftpque.nrcan.gc.ca";
	const char* CUINewfoundland::SERVER_PATH = "/";

	//*********************************************************************
	const char* CUINewfoundland::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "UsderName", "Password", "WorkingDir", "FirstYear", "LastYear", "ShowProgress" };
	const size_t CUINewfoundland::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_STRING, T_PASSWORD, T_PATH, T_STRING, T_STRING, T_BOOL };
	const UINT CUINewfoundland::ATTRIBUTE_TITLE_ID = IDS_UPDATER_NEWFOUNDLAND_P;
	const UINT CUINewfoundland::DESCRIPTION_TITLE_ID = ID_TASK_NEWFOUNDLAND;

	const char* CUINewfoundland::CLASS_NAME(){ static const char* THE_CLASS_NAME = "Newfoundland";  return THE_CLASS_NAME; }
	CTaskBase::TType CUINewfoundland::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUINewfoundland::CLASS_NAME(), (createF)CUINewfoundland::create);


	CUINewfoundland::CUINewfoundland(void)
	{}

	CUINewfoundland::~CUINewfoundland(void)
	{}


	std::string CUINewfoundland::Option(size_t i)const
	{
		string str;
		//switch (i)
		//{
		//};
		return str;
	}

	std::string CUINewfoundland::Default(size_t i)const
	{
		string str;

		switch (i)
		{
		case WORKING_DIR: str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()) + "Newfoundland\\"; break;
		case FIRST_YEAR:
		case LAST_YEAR:	str = ToString(CTRef::GetCurrentTRef().GetYear()); break;
		};

		return str;
	}

	//****************************************************


	std::string CUINewfoundland::GetStationsListFilePath()const
	{
		return GetDir(WORKING_DIR) + "Station_Metadata.csv";
	}

	string CUINewfoundland::GetOutputFilePath(const string& fileTitle, int year)const
	{
		return GetDir(WORKING_DIR) + ToString(year) + "\\" + fileTitle +".csv";
	}

	string CUINewfoundland::GetOutputFilePath(int year)const
	{
		return GetDir(WORKING_DIR) + ToString(year) + "\\" + ToString(year) + ".zip";
	}


	ERMsg CUINewfoundland::Execute(CCallback& callback)
	{
		ERMsg msg;
		
		string workingDir = GetDir(WORKING_DIR);
		msg = CreateMultipleDir(workingDir);


		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(workingDir, 1);
		callback.AddMessage(GetString(IDS_UPDATE_FROM));
		callback.AddMessage(string(SERVER_NAME) + "/", 1);
		callback.AddMessage("");
		

		StringVector fileList;

		msg += UpdateStationList(callback);

		if (!msg)
			return msg;


		size_t nbDownloads = 0;

		int firstYear = WBSF::as<int>(Get(FIRST_YEAR));
		int lastYear = WBSF::as<int>(Get(LAST_YEAR));
		size_t nbYears = lastYear - firstYear + 1;

		callback.AddMessage(GetString(IDS_NUMBER_FILES) + ToString(nbYears), 1);
		callback.AddMessage("");

		callback.PushTask("Update Newfoundland weather data (" + ToString(nbYears) + " files)", nbYears);

		//open a connection on the server
		CInternetSessionPtr pSession;
		CFtpConnectionPtr pConnection;

		msg = GetFtpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, Get(USER_NAME), Get(PASSWORD), true);
		if (msg)
		{
			pSession->SetOption(INTERNET_OPTION_RECEIVE_TIMEOUT, 45000);

			//add station list
			for (size_t y = 0; y < nbYears; y++)
			{
				int year = firstYear + int(y);

				string filter = "/hydromanitoba/" + ToString(year) + "/" + ToString(year) + ".zip";

				CFileInfoVector fileList;
				msg = FindFiles(pConnection, filter, fileList, callback);
				if (fileList.size() == 1)
				{
					string fileName = GetFileName(fileList.front().m_filePath);
					int year = WBSF::as<int>(fileName);

					string outputFilePath = GetOutputFilePath(year);
					if (!IsFileUpToDate(fileList.front(), outputFilePath))
					{
						CreateMultipleDir(GetPath(outputFilePath));
						callback.AddMessage("Download Newfoundland file: " + fileName + " ...");
						msg = CopyFile(pConnection, fileList.front().m_filePath, outputFilePath, INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_DONT_CACHE);

						if (msg)
						{
							msg = sevenZ(outputFilePath, GetPath(outputFilePath), callback);
							if (msg)
							{
								nbDownloads++;
								msg += callback.StepIt();
							}
						}
					}
				}
			}


			pConnection->Close();
			pSession->Close();
		}

		

		callback.AddMessage("Number of file downloaded:" + ToString(nbDownloads) );
		callback.PopTask();

		return msg;
	}


	ERMsg CUINewfoundland::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;

		msg = m_stations.Load(GetStationsListFilePath());

		if (msg)
			msg += m_stations.IsValid();

		//Update network
		//for (size_t i = 0; i < locations.size(); i++)
			//locations[i].SetSSI("Network", "Newfoundland");

		//m_stations.insert(m_stations.end(), locations.begin(), locations.end());
				
		for (size_t i = 0; i < m_stations.size(); i++)
			stationList.push_back(m_stations[i].m_ID);
	

		return msg;
	}

	ERMsg CUINewfoundland::GetWeatherStation(const std::string& ID, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;

		//NL_New_Harbour_15_orig.csv
//		StationName, DateTime, Temp, Rh, Wspd, Dir, Rn24
	//	New Harbour 15, 2017 / 01 / 01 00:00 : 00, -1.2, 99, 8.6, 276, 0


		//Get station information
		size_t it = m_stations.FindByID(ID);
		ASSERT(it != NOT_INIT);

		((CLocation&)station) = m_stations[it];
		int firstYear = WBSF::as<int>(Get(FIRST_YEAR));
		int lastYear = WBSF::as<int>(Get(LAST_YEAR));
		size_t nbYears = lastYear - firstYear + 1;
		
		station.CreateYears(firstYear, nbYears);
		station.m_name = PurgeFileName(station.m_name);

		//now extract data 
		for (size_t y = 0; y < nbYears&&msg; y++)
		{
			int year = firstYear + int(y);

			string filePath = GetOutputFilePath(station.GetSSI("FileTitle"), year);
			if (FileExists(filePath))
				msg = ReadDataFile(filePath, TM, station, callback);

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

	ERMsg CUINewfoundland::UpdateStationList(CCallback& callback)
	{
		ERMsg msg;

		CInternetSessionPtr pSession;
		CFtpConnectionPtr pConnection;

		msg = GetFtpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, Get(USER_NAME), Get(PASSWORD), true);
		
		if (msg)
		{
			string path = "/hydromanitoba/Station_Metadata.csv";

			CFileInfoVector fileList;
			msg = FindFiles(pConnection, path, fileList, callback);

			pConnection->Close();
			pSession->Close();

			if (msg)
			{
				ASSERT(fileList.size() == 1);

				string outputFilePath = GetStationsListFilePath();
				if (!IsFileUpToDate(fileList.front(), outputFilePath))
					msg = CopyFile(pConnection, fileList.front().m_filePath, outputFilePath, INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_DONT_CACHE);
			}
		}

		return msg;
	}

	ERMsg CUINewfoundland::sevenZ(const string& filePathZip, const string& workingDir, CCallback& callback)
	{
		ERMsg msg;

		callback.PushTask(GetString(IDS_UNZIP_FILE), NOT_INIT);

		string command = GetApplicationPath() + "External\\7z.exe x \"" + filePathZip + "\" -y";
		//UINT show = as<bool>(SHOW_APP) ? SW_SHOW : SW_HIDE;

		DWORD exitCode = 0;
		msg = WinExecWait(command, workingDir, SW_HIDE, &exitCode);
		if (msg && exitCode != 0)
			msg.ajoute("7z.exe as exit with error code " + ToString(int(exitCode)));


		callback.PopTask();


		return msg;
	}


	CTRef CUINewfoundland::GetTRef(string str)//const vector<size_t>& vars, const CSVIterator& loop)
	{
		CTRef TRef;
		
		//2017/01/01 00:00:00
		StringVector vec(str, " :/");
		if (vec.size() == 6)
		{
			int year = WBSF::as<int>(vec[0]);
			size_t month = WBSF::as<size_t>(vec[1]) - 1;
			size_t day = WBSF::as<size_t>(vec[2]) - 1;
			size_t hour = WBSF::as<size_t>(vec[3]);

			ASSERT(month >= 0 && month < 12);
			ASSERT(day >= 0 && day < GetNbDayPerMonth(year, month));
			ASSERT(hour >= 0 && hour < 24);


			TRef = CTRef(year, month, day, hour);
		}


		return TRef;

	}

	ERMsg CUINewfoundland::ReadDataFile(const string& filePath, CTM TM, CWeatherYears& data, CCallback& callback)const
	{
		ERMsg msg;

		enum TColumns { C_NAME, C_DATE_TIME, C_TAIR, C_RELH, C_WNDS, C_WNDD, C_RAIN24, NB_COLUMNS };
		static const TVarH COL_VAR[NB_COLUMNS] = { H_SKIP, H_SKIP, H_TAIR2, H_RELH, H_WNDS, H_WNDD, H_PRCP};
		
		//now extract data 
		ifStream file;

		msg = file.open(filePath);

		if (msg)
		{
			CWeatherAccumulator stat(TM);
			double lastPrcp = 0;

			for (CSVIterator loop(file); loop != CSVIterator(); ++loop)
			{
				CTRef TRef = GetTRef((*loop)[C_DATE_TIME]);
				if (TRef.IsInit())
				{
					if (stat.TRefIsChanging(TRef))
						data[stat.GetTRef()].SetData(stat);

					for (size_t c = 0; c < loop->size(); c++)
					{
						if (COL_VAR[c] != H_SKIP && !(*loop)[c].empty())
						{
							double value = ToDouble((*loop)[c]);

							if (c == C_RAIN24)
							{
								if (value < lastPrcp)
									lastPrcp = 0;

								value -= lastPrcp;
								lastPrcp = ToDouble((*loop)[c]);
							}

							stat.Add(TRef, COL_VAR[c], value);
							if (c == C_RELH)
							{
								double T = ToDouble((*loop)[C_TAIR]);
								stat.Add(TRef, H_TDEW, Hr2Td(T, value));
							}
						}//if valid value
					}//for all columns
				}//TRef is init

				msg += callback.StepIt(0);
			}//for all line (

			if (stat.GetTRef().IsInit())
				data[stat.GetTRef()].SetData(stat);

		}//if load 

		return msg;
	}

	
}