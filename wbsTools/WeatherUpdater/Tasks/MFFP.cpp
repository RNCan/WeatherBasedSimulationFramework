//usager : MeteoR
//PW: LeSoleilBrille

#include "StdAfx.h"
#include "MFFP.h"


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

	const char* CMFFP::SERVER_NAME = "horus.mesonet-quebec.org";
	const char* CMFFP::SERVER_PATH = "/";

	const char * CMFFP::FIELDS_NAME[NB_FIELDS] =
	{ "TAI000H", "TAN000H", "TAX000H", "TAM000H", "PC040H", "PT040H", "PC020H", "PT020H", "PT041H", "HAI000H", "HAN000H", "HAX000H", "NSI000H", "VDM400H", "VVM400H", "VDXI500H", "VVXI500H", "VDXI500HT", "VDM425H", "VVM425H", "VDXI525H", "VVXI525H", "VDXI525HT", "TDI000H", "TDN000H", "TDX000H", "VB000B" };
	//TAm000H
	//TAi000H
	//TAn000H
	//TAx000H
	//HAi000H
	//HAn000H
	//HAx000H
	//TDi000H
	//TDn000H
	//TDx000H
	//VDm400H	:wind direction 
	//VVm400H	:wind speed 
	//VDxi500H	:wind direction 
	//VVxi500H	:wind speed
	//VDxi500HT	:wind direction 
	//VDm425H	:wind direction 
	//VVm425H	:wind speed
	//VDxi525H	:wind direction 
	//VVxi525H	:wind speed
	//VDxi525HT	:wind direction 
	//PC020H
	//PC040H
	//PT020H
	//PT040H
	//PT041H
	//NSi000H
	
	
	const TVarH CMFFP::VARIABLE[NB_FIELDS] = { H_TAIR2, H_TMIN2, H_TMAX2, H_TAIR2, H_SKIP, H_PRCP, H_SKIP, H_SKIP, H_SWE, H_RELH, H_RELH, H_RELH, H_SNDH, H_WNDD, H_WNDS, H_SKIP, H_SKIP, H_SKIP, H_SKIP, H_WND2, H_SKIP, H_SKIP, H_SKIP, H_TDEW, H_SKIP, H_SKIP, H_SKIP };

	TVarH CMFFP::GetVariable(std::string str)
	{
		str = MakeUpper(str);
		TVarH v = H_SKIP;
		bool find = false;
		for (size_t i = 0; i < NB_FIELDS&&v == NOT_INIT; i++)
		{
			if (str == FIELDS_NAME[i])
			{
				find = true;
				v = VARIABLE[i];
				break;
			}
		}

		ASSERT(find);

		return v;
	}

	//*********************************************************************

	CMFFP::CMFFP(void)
	{
		m_firstYear=0;
		m_lastYear=0;
	}

	CMFFP::~CMFFP(void)
	{}

	//*****************************************************************************

	string CMFFP::GetStationListFilePath()const
	{
		return GetApplicationPath() + "Layers\\QuebecStations.csv";
	}

	string CMFFP::GetOutputFilePath(CTRef TRef)const
	{
		return WBSF::FormatA("%s%d/m%4d-%02d-%02d-%02d.csv", m_workingDir.c_str(), TRef.GetYear(), TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, TRef.GetHour());
	}

	ERMsg CMFFP::GetFileList(CFileInfoVector& fileList, CCallback& callback)const
	{
		ERMsg msg;


		fileList.clear();
		//size_t nbYears = m_lastYear - m_firstYear + 1;

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

			
			ERMsg msgTmp = GetFtpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, m_userName, m_password);
			if (msgTmp)
			{
				pSession->SetOption(INTERNET_OPTION_RECEIVE_TIMEOUT, 35000);

			
				msgTmp = FindFiles(pConnection, string(SERVER_PATH) + "*.*", fileList, callback);
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
				callback.AddMessage(msgTmp);
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
	

	static CTRef GetTRef(string& fileTitle)
	{
		int year = 2000 + WBSF::as<int>(fileTitle.substr(1, 2));
		size_t jDay = WBSF::as<size_t>(fileTitle.substr(3, 3))-1;
		size_t h = WBSF::as<size_t>(fileTitle.substr(6, 2));

		CJDayRef JDay(year, jDay);
		return CTRef(JDay.GetYear(), JDay.GetMonth(), JDay.GetDay(), h);
	}

	ERMsg CMFFP::CleanList(CFileInfoVector& fileList, CCallback& callback)const
	{
		ERMsg msg;

		string workingDir = m_workingDir;

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

	ERMsg CMFFP::Execute(CCallback& callback)
	{
		ERMsg msg;
		
		/*{

			StringVector fileList = WBSF::GetFilesList("G:\\Quebec\\MFFP\\2017(old)\\*.*");
			for (size_t i = 0; i < fileList.size(); i++)
			{
				string fileTitle = GetFileTitle(fileList[i]);
				CTRef TRef = GetTRef(fileTitle);

				string outputFilePath = GetOutputFilePath(TRef);
				CreateMultipleDir(GetPath(outputFilePath));

				WBSF::CopyOneFile(fileList[i], outputFilePath);
			}

		}*/


		string workingDir = m_workingDir;
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
			CreateMultipleDir(m_workingDir);

			callback.PushTask(GetString(IDS_UPDATE_FILE), fileList.size());

			int nbRun = 0;
			size_t curI = 0;

			while (curI < fileList.size() && nbRun < 20 && msg)
			{
				nbRun++;

				CInternetSessionPtr pSession;
				CFtpConnectionPtr pConnection;

				ERMsg msgTmp = GetFtpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, m_userName, m_password);
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


	ERMsg CMFFP::GetStationList(StringVector& stationList, CCallback& callback)
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

	ERMsg CMFFP::LoadWeatherInMemory(CCallback& callback)
	{
		ERMsg msg;

		size_t nbYears = m_lastYear - m_firstYear + 1;

		for (size_t y = 0; y < nbYears&& msg; y++)
		{
			int year = m_firstYear + int(y);

			//on pourrait optimiser en loadant une seul fois tout les fichiers
			StringVector fileList = GetFilesList(m_workingDir + to_string(year) + "\\" + "*.csv");
			callback.PushTask("Load MFFP data for year = " + to_string(year), fileList.size());

			for (size_t i = 0; i < fileList.size() && msg; i++)
			{
				msg += ReadData(fileList[i], m_stations, callback);
				msg += callback.StepIt();
			}

			callback.PopTask();

		}


		return msg;
	}

	ERMsg CMFFP::GetWeatherStation(const string& ID, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;
	
		station = m_stations[ID];

		if (msg && station.HaveData())
			msg = station.IsValid();

		return msg;
	}

	bool CMFFP::IsValid(TVarH v, double value)
	{
		bool bValid = true;
		switch (v)
		{
		case H_TMIN2: 
		case H_TAIR2:
		case H_TMAX2: 
		case H_TDEW: bValid = value >= -45 && value <= 40; break;
		case H_PRCP:
		case H_SWE:
		case H_SNDH: bValid = value >= 0; break;
		case H_RELH: bValid = value >= 1 && value <= 100; break;
		case H_WNDD: bValid = value >= 0 && value <= 360; break;
		case H_WNDS:
		case H_WND2: bValid = value >= 0 && value <= 120; break;
		}

		return bValid;
	}

	ERMsg CMFFP::ReadData(const string& filePath, CWeatherStationMap& stations, CCallback& callback)const
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
							
							if (IsValid(v, value ))
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