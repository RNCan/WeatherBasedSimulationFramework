#include "StdAfx.h"
#include "UICIPRA.h"

#include "TaskFactory.h"
#include "Basic/HourlyDatabase.h"
#include "Basic/CSV.h"
#include "UI/Common/SYShowMessage.h"

#include "../Resource.h"

using namespace WBSF::HOURLY_DATA;
using namespace std;
using namespace UtilWWW;


namespace WBSF
{


	
	//*********************************************************************

	const char* CUICIPRA::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "UserName", "Password", "WorkingDir", "FirstYear", "LastYear" };
	const size_t CUICIPRA::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_STRING, T_PASSWORD, T_PATH, T_STRING, T_STRING };
	const UINT CUICIPRA::ATTRIBUTE_TITLE_ID = IDS_UPDATER_SM_CIPRA_HOURLY_P;
	const UINT CUICIPRA::DESCRIPTION_TITLE_ID = ID_TASK_SM_CIPRA_HOURLY;

	const char* CUICIPRA::CLASS_NAME(){ static const char* THE_CLASS_NAME = "CIPRA";  return THE_CLASS_NAME; }
	CTaskBase::TType CUICIPRA::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUICIPRA::CLASS_NAME(), (createF)CUICIPRA::create);
	

	const char* CUICIPRA::SERVER_NAME = "horus.mesonet-quebec.org";
	const char* CUICIPRA::SUB_DIR = "/";

	enum TSource { S_ATLANTIQUE, S_ONTARIO, S_POMME, S_QUEBEC, NB_SOURCE_TYPE };
	static const char* SOURCE_TYPE_NAME[NB_SOURCE_TYPE] = { "atantique", "ontario", "pommes", "quebec" };


	CUICIPRA::CUICIPRA(void)
	{}

	
	CUICIPRA::~CUICIPRA(void)
	{}

	std::string CUICIPRA::Default(size_t i)const
	{
		string str;

		switch (i)
		{
		case WORKING_DIR: str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()) + "CIPRA"; break;
		case FIRST_YEAR:
		case LAST_YEAR:	str = ToString(CTRef::GetCurrentTRef().GetYear()); break;
		};

		return str;
	}

	
	//*********************************************************************************
	ERMsg CUICIPRA::Execute(CCallback& callback)
	{
		ERMsg msg;


		string workingDir = GetDir(WORKING_DIR);

		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(workingDir, 1);
		callback.AddMessage(GetString(IDS_UPDATE_FROM));
		callback.AddMessage(SERVER_NAME, 1);
		callback.AddMessage("");


		//Pour le moment il n'y a pas de site ou les stations sont listés
		msg = UpdateStationsFile(callback);
		if (!msg)
			return msg;

		CTime today = CTime::GetCurrentTime();
		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		int nbYears = lastYear - firstYear + 1;

		callback.AddMessage(GetString(IDS_UPDATE_FILE));
		int nbDownload = 0;

		callback.PushTask("Sources", NB_SOURCE_TYPE);

		//for (size_t t = 0; t < NB_SOURCE_TYPE; t++)
		//{
		//	//open a connection on the server
		//	CInternetSessionPtr pSession;
		//	CFtpConnectionPtr pConnection;

		//	msg = GetFtpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, Get(USER_NAME), Get(PASSWORD));

		//	if (msg)
		//	{
		//		//get houe reference
		//		CFileInfoVector dirList;
		//		UtilWWW::FindDirectories(pConnection, SUB_DIR, dirList);
		//		if (dirList.size()>1)
		//			callback.PushTask(string("Fuseau horaire "), dirList.size());
		//		

		//		for (size_t f = 0; f < dirList.size() && msg; f++)
		//		{
		//			string fh = GetLastDirName(dirList[f].m_filePath);

		//			if (!IsEqual(fh, "pec"))
		//			{
		//				callback.PushTask(string("Années ") + SOURCE_TYPE_NAME[t], nbYears);
		//				for (size_t y = 0; y < nbYears&&msg; y++)
		//				{
		//					int nbDownloadPerYear = 0;

		//					int year = firstYear + int(y);
		//					//string path = string(SOURCE_TYPE_NAME[t]) + "/" + ToString(year) + "/";
		//					string path = dirList[f].m_filePath + "/" + ToString(year) + "/";
		//					
		//					string ouputPath = workingDir + path;

		//					//Load files list
		//					callback.AddMessage(path);
		//					callback.AddMessage(GetString(IDS_LOAD_FILE_LIST),1);
		//					CFileInfoVector fileList;
		//					msg += UtilWWW::FindFiles(pConnection, "/" + path + "*.BRU", fileList, callback);

		//					pConnection->Close();
		//					pSession->Close();


		//					if (msg)
		//					{
		//						callback.AddMessage(GetString(IDS_NB_FILES_FOUND) + ToString(fileList.size()));

		//						//clean files list
		//						for (size_t k = 0; k < fileList.size() && msg; k++)
		//						{
		//							string filePath = ouputPath + GetFileName(fileList[k].m_filePath);
		//							if (UtilWWW::IsFileUpToDate(fileList[k], filePath, false))
		//								fileList.erase(fileList.begin() + k);

		//							msg += callback.StepIt(0);
		//						}

		//						callback.AddMessage(GetString(IDS_NB_FILES_CLEARED) + ToString(fileList.size()));

		//						//download files
		//						callback.PushTask(path, fileList.size());

		//						CreateMultipleDir(ouputPath);

		//						int nbRun = 0;
		//						int curI = 0;

		//						while (curI < fileList.size() && msg)
		//						{
		//							nbRun++;
		//							//open a connection on the server
		//							msg = GetFtpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, Get(USER_NAME), Get(PASSWORD));


		//							for (size_t k = curI; k < fileList.size() && msg; k++)
		//							{
		//								string filePath = ouputPath + GetFileName(fileList[k].m_filePath);
		//								msg += CopyFile(pConnection, fileList[k].m_filePath, filePath);
		//								if (msg)
		//								{
		//									curI++;
		//									nbDownloadPerYear++;
		//								}

		//								msg += callback.StepIt();

		//							}//for all files 


		//							//if an error occur: try again
		//							if (!msg && !callback.GetUserCancel())
		//							{
		//								if (nbRun < 5)
		//								{
		//									callback.PushTask("Waiting 2 seconds for server...", 100);
		//									for (size_t i = 0; i < 40 && msg; i++)
		//									{
		//										Sleep(50);//wait 50 milisec
		//										msg += callback.StepIt();
		//									}
		//									callback.PopTask();
		//								}
		//							}

		//							pConnection->Close();
		//							pSession->Close();
		//						}

		//						callback.AddMessage(GetString(IDS_NB_FILES_DOWNLOADED) + ToString(nbDownloadPerYear), 2);

		//						nbDownload += nbDownloadPerYear;
		//					}//for all files

		//					callback.PopTask();
		//				}//for all years

		//				callback.PopTask();
		//			}
		//		}//if not pec

		//		callback.StepIt();

		//		if (dirList.size()>1)
		//			callback.PopTask();
		//	}//if msg
		//	else
		//	{

		//	}

		//	callback.StepIt();
		//}//type: pomme, agro

		callback.PopTask();

		callback.AddMessage("");
		callback.AddMessage(GetString(IDS_NB_FILES_DOWNLOADED) + ToString(nbDownload), 2);

		return msg;
	}

	string CUICIPRA::GetStationsFilePath()
	{
		return GetDir(WORKING_DIR) + "Stations.csv";
	}

	string CUICIPRA::GetMissingFilePath()
	{
		return GetDir(WORKING_DIR) + "MissingStations.csv";
	}


	ERMsg CUICIPRA::LoadStations(CCallback& callback)
	{
		ERMsg msg;

		msg = UpdateStationsFile(callback);
		if (msg)
		{
			msg = m_stations.Load(GetStationsFilePath());

			CLocationVector missing;
			if (missing.Load(GetMissingFilePath()))
				m_stations.insert(m_stations.begin(), missing.begin(), missing.end());

		}
		return msg;
	}

	string CUICIPRA::GetStationName(const string& ID)const
	{
		string name;

		size_t pos = m_stations.FindByID(ID);
		if (pos < m_stations.size())
			name = m_stations[pos].m_name.c_str();

		return name;
	}

	int CUICIPRA::GetStationIndex(const StringVector& allFilePath, const string& stationName)
	{
		int index = -1;

		for (int i = 0; i < allFilePath.size(); i++)
		{
			if (allFilePath[i].find(stationName, 6) != string::npos)
			{
				index = i;
				break;
			}
		}

		return index;
	}

	string CUICIPRA::GetOutputFilePath(size_t t, int year, const string& name)
	{
		return GetDir(WORKING_DIR) + SOURCE_TYPE_NAME[t] + "\\" + ToString(year) + "\\" + name;
	}

	ERMsg CUICIPRA::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;

		
		msg = LoadStations(callback);
		if (!msg)
			return msg;

		string workingDir = GetDir(WORKING_DIR);

		//find all station in the directories
		for (size_t i = 0; i < m_stations.size() && msg; i++)
		{
			for (int t = 0; t < NB_SOURCE_TYPE; t++)
			{

				bool bAdd = false;
				int firstYear = as<int>(FIRST_YEAR);
				int lastYear = as<int>(LAST_YEAR);
				int nbYear = lastYear - firstYear + 1;
				for (size_t y = 0; y < nbYear&&!bAdd; y++)
				{
					int year = firstYear + int(y);

					string filePath = GetOutputFilePath(t, year, m_stations[i].GetDataFileName());

					if (FileExists(filePath))
						bAdd = true;
				}

				if (bAdd)
					stationList.push_back(ToString(t) + "/" + ToString(i));

				msg += callback.StepIt(0);
			}
		}

		return msg;
	}

	ERMsg CUICIPRA::GetWeatherStation(const string& stationName, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;


		string::size_type pos = 0;
		int t = ToInt(Tokenize(stationName, "/", pos));
		int index = ToInt(Tokenize(stationName, "/", pos));
		((CLocation&)station) = m_stations[index];
		station.m_name = PurgeFileName(station.m_name);

		string workingDir = GetDir(WORKING_DIR);
		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = lastYear - firstYear + 1;
		station.CreateYears(firstYear, nbYears);

		size_t filesize = 0;
		for (size_t y = 0; y < nbYears&&msg; y++)
		{
			int year = firstYear + int(y);
			string filePath = GetOutputFilePath(t, year, station.GetDataFileName());

			if (FileExists(filePath))
			{
				CFileInfo filesInfo;
				GetFileInfo(filePath, filesInfo);
				filesize += filesInfo.m_size;
			}//if file exists
		}//for all years (files)

		callback.PushTask(station.m_name, filesize);

		//now extact data
		for (size_t y = 0; y < nbYears&&msg; y++)
		{
			int year = firstYear + int(y);

			string filePath = GetOutputFilePath(t, year, station.GetDataFileName());

			if (FileExists(filePath))
				msg += ReadDataFile(filePath, TM, station, callback);

		}//for all years


		station.SetDataFileName("");//use default file name insted of .BRU
		//verify station is valid
		if (msg && station.HaveData())
			msg = station.IsValid();

		callback.PopTask();


		return msg;
	}


	ERMsg CUICIPRA::ReadDataFile(const string& filePath, CTM TM, CWeatherYears& data, CCallback& callback)const
	{
		ERMsg msg;

		//1.  Année au complet (ex. 2010)
		//2.  Jour de l'année - de 1 à 365 ou 366 (ex. 156)
		//3.  Heure normale de l'Est - de 0 (minuit) à 2300 (11 heures du soir) (ex. 1500)
		//4.  Numéro de la station (ex. 3874)
		//5.  Température maximum de l'heure - °C (ex. 15.6)
		//6.  Température minimum de l'heure - °C (ex. 15.2)
		//7.  Température moyenne de l'heure - °C (ex. 15.4)
		//8.  Humidité relative - % (ex. 75.4)
		//9.  Précipitations - mm (ex. 2.6)
		//10. Heure du début de la pluie dans l'heure (ex. 1423)
		//11. Radiation solaire - kJ/m2/h (ex. 1235)
		//12. Température de l'air à 5 cm du sol - °C (ex. 12.4)
		//13. Température moyenne du sol à 5 cm - °C (ex. 13.4)
		//14. Température moyenne du sol à 10 cm - °C (ex. 12.5)
		//15. Température moyenne du sol à 20 cm - °C (ex. 12.0)
		//16. Température moyenne du sol à 50 cm - °C (ex. 11.5)
		//17. Vitesse du vent - km/h (ex. 10.7)
		//18. Direction du vent - degrés de 0 (Nord) à 360 (ex. 245)
		//19. Pression barométrique - kPa (ex. 95.7)
		//20. Mouillure du feuillage - Voltage relatif (ex. 0.74)
		//21. Mouillure du feuillage - nombre de minutes au-dessus seuil (ex. 43)
		//22. Champ réservé pour usage futur (ex. -991)
		//23. Champ réservé pour usage futur (ex. -991)


		enum TColumns { C_YEAR, C_JDAY, C_HOUR, C_STAION_NO, C_TMAX, C_TMIN, C_TAIR, C_RELH, C_PRCP, C_PRCP_BEGIN, C_SRAD, C_TGROUND, CT_SOIL_5, CT_SOIL_10, C_T_SOIL_20, C_T_SOIL_50, C_WNDS, C_WNDD, C_PRES, C_SWEB, C_SWEB_DURATION, C_UNUSED1, C_UNUSED2, NB_COLUMNS };

		//wind speed at 2 meters
		const int COL_POS[NB_VAR_H] = { C_TAIR, -1, C_PRCP, -1, C_RELH, -1, C_WNDD, C_SRAD, C_PRES, -1, -1, -1, -1, -1, -1, C_WNDS, -1, -1 };
		const double FACTOR[NB_VAR_H] = { 1, 0, 1, 0, 1, 0, 1, 0.001, 10, 0, 0, 0, 0, 0, 0, 1, 0, 0 };

		//now extract data 
		ifStream file;

		msg = file.open(filePath);

		if (msg)
		{
			CWeatherAccumulator stat(TM);

			for (CSVIterator loop(file, ",", false); loop != CSVIterator(); ++loop)
			{
				int year = ToInt((*loop)[C_YEAR]);
				SIZE_T jDay = ToInt((*loop)[C_JDAY]) - 1;
				size_t month = GetMonthIndex(year, jDay);
				size_t day = GetDayOfTheMonth(year, jDay);
				size_t hour = ToInt((*loop)[C_HOUR]) / 100;

				//ASSERT(year >= firstYear && year <= lastYear);
				ASSERT(month >= 0 && month < 12);
				ASSERT(day >= 0 && day < GetNbDayPerMonth(year, month));
				ASSERT(hour >= 0 && hour < 24);

				CTRef TRef(year, month, day, hour);

				if (stat.TRefIsChanging(TRef))
				{
					data[stat.GetTRef()].SetData(stat);
				}

				for (size_t v = 0; v < NB_VAR_H; v++)
				{
					if (COL_POS[v] >= 0)
					{
						double value = ToDouble((*loop)[COL_POS[v]]);
						if (value > -991)
						{
							stat.Add(TRef, v, value*FACTOR[v]);
							if (COL_POS[v] == C_RELH)
							{
								double T = ToDouble((*loop)[C_TAIR]);
								if (T > -991)
								{
									stat.Add(TRef, H_TDEW, Hr2Td(T, value));
								}
							}
						}
					}
				}

				msg += callback.StepIt(loop->GetLastLine().length() + 2);
			}//for all line (

			if (stat.GetTRef().IsInit())
				data[stat.GetTRef()].SetData(stat);

		}//if load 

		return msg;
	}
	
	ERMsg CUICIPRA::UpdateStationsFile(CCallback& callback)
	{
		ERMsg msg;

		string dstFilePath = GetStationsFilePath();


		if (!FileExists(dstFilePath))
		{
			string srcFilePath = GetApplicationPath() + "Layers\\CIPRAStations.csv";

			CString src(CStringA(srcFilePath.c_str()));
			CString dst(CStringA(dstFilePath.c_str()));
			CopyFile(src, dst, TRUE);
		}


		return msg;

	}
}