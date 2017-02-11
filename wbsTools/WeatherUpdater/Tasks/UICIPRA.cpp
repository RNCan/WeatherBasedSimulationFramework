#include "StdAfx.h"
#include "UICIPRA.h"

#include "TaskFactory.h"
#include "Basic/HourlyDatabase.h"
#include "Basic/CSV.h"
#include "UI/Common/SYShowMessage.h"

#include "../Resource.h"
#include "Geomatic/TimeZones.h"
#include "cctz\time_zone.h"



using namespace WBSF::HOURLY_DATA;
using namespace std;
using namespace UtilWWW;

//autre réseau horaire (basée sur MADIS 2006-present)
//http://mesonet.agron.iastate.edu/sites/site.php?network=CA_QC_ASOS&station=CYBG	
//station list
//http://mesonet.agron.iastate.edu/sites/networks.php?network=_ALL_&format=csv&nohtml=on
//http://mesonet.agron.iastate.edu/cgi-bin/request/asos.py?station=CMBB&station=CMBR&station=CMCT&station=CMCW&station=CMFM&station=CMGB&station=CMHB&station=CMHN&station=CMHW&station=CMLA&station=CMLI&station=CMPD&station=CMPL&station=CMRG&station=CMRI&station=CMRU&station=CMRY&station=CMSB&station=CMSC&station=CMSX&station=CMWD&station=CMYT&station=CWAF&station=CWBA&station=CWBS&station=CWBT&station=CWBY&station=CWBZ&station=CWDM&station=CWDQ&station=CWDT&station=CWEE&station=CWEO&station=CWER&station=CWEW&station=CWFQ&station=CWGR&station=CWHM&station=CWHP&station=CWHQ&station=CWHV&station=CWHY&station=CWIA&station=CWIG&station=CWIP&station=CWIS&station=CWIT&station=CWIU&station=CWIX&station=CWIZ&station=CWJB&station=CWJO&station=CWJT&station=CWKD&station=CWLU&station=CWMJ&station=CWMN&station=CWMW&station=CWNH&station=CWNQ&station=CWOC&station=CWOD&station=CWPD&station=CWPH&station=CWPK&station=CWPQ&station=CWQG&station=CWQH&station=CWQO&station=CWQR&station=CWQV&station=CWRC&station=CWRZ&station=CWSF&station=CWSG&station=CWST&station=CWTA&station=CWTB&station=CWTG&station=CWTN&station=CWTQ&station=CWTT&station=CWTY&station=CWUK&station=CWUX&station=CWVQ&station=CWVY&station=CWVZ&station=CWXC&station=CWZS&station=CXAM&station=CXBO&station=CXHF&station=CXLT&station=CXSH&station=CXZV&station=CYAD&station=CYAH&station=CYAS&station=CYBC&station=CYBG&station=CYBX&station=CYGL&station=CYGP&station=CYGR&station=CYGV&station=CYGW&station=CYHA&station=CYHH&station=CYHU&station=CYIK&station=CYKG&station=CYKL&station=CYKO&station=CYKQ&station=CYLA&station=CYLU&station=CYML&station=CYMT&station=CYMU&station=CYMX&station=CYNA&station=CYNC&station=CYND&station=CYNM&station=CYOY&station=CYPH&station=CYPX&station=CYQB&station=CYRJ&station=CYRQ&station=CYSC&station=CYTQ&station=CYUL&station=CYUY&station=CYVO&station=CYVP&station=CYYY&station=CYZG&station=CYZV&station=CZEM&data=tmpc&data=dwpc&data=relh&data=drct&data=sknt&data=mslp&data=p01m&data=skyc1&data=skyc2&data=skyc3&data=presentwx&year1=2016&month1=1&day1=1&year2=2016&month2=11&day2=22&tz=Etc%2FUTC&format=comma&latlon=no&direct=no&report_type=1&report_type=2


namespace WBSF
{

	string removeAccented(string str)
	{
		//char *p = str;
		for (string::iterator it = str.begin(); it != str.end(); it++)
		{
			static const char*
				//   "ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖ×ØÙÚÛÜÝÞßàáâãäåæçèéêëìíîïðñòóôõö÷øùúûüýþÿ"
				tr = "AAAAAAECEEEEIIIIDNOOOOOx0UUUUYPsaaaaaaeceeeeiiiiOnooooo/0uuuuypy";
			unsigned char ch = *it;
			if (ch >= 192) 
				*it = tr[ch - 192];
			
			//++p; // http://stackoverflow.com/questions/14094621/
		}

		return str;
	}

	//*********************************************************************

	const char* CUICIPRA::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "UserName", "Password", "WorkingDir", "Network", "FirstYear", "LastYear", "Forecast"  };
	const size_t CUICIPRA::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_STRING, T_PASSWORD, T_PATH, T_STRING_SELECT, T_STRING, T_STRING, T_BOOL };
	const UINT CUICIPRA::ATTRIBUTE_TITLE_ID = IDS_UPDATER_SM_CIPRA_HOURLY_P;
	const UINT CUICIPRA::DESCRIPTION_TITLE_ID = ID_TASK_SM_CIPRA_HOURLY;

	const char* CUICIPRA::CLASS_NAME(){ static const char* THE_CLASS_NAME = "CIPRA";  return THE_CLASS_NAME; }
	CTaskBase::TType CUICIPRA::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUICIPRA::CLASS_NAME(), (createF)CUICIPRA::create);
	

	const char* CUICIPRA::SERVER_NAME = "horus.mesonet-quebec.org";
	const char* CUICIPRA::SUB_DIR = "/";

	const char* CUICIPRA::NETWORK_NAME[NB_NETWORKS] = { "atantic", "ontario", "pommes", "quebec" };
	const char* CUICIPRA::NETWORK_TIMEZONE_NAME[NB_NETWORKS] = { "AST", "EST", "HNE", "HNE" };

	size_t CUICIPRA::GetNetworkIndex(const std::string& network_name)
	{
		size_t n = NOT_INIT;
		for (size_t nn = 0; nn < NB_NETWORKS&&n == NOT_INIT; nn++)
		{
			if (IsEqual(network_name, NETWORK_NAME[nn]))
				n = nn;
		}

		assert(n < NB_NETWORKS);

		return n;
	}

	CUICIPRA::CUICIPRA(void)
	{}

	
	CUICIPRA::~CUICIPRA(void)
	{}


	std::string CUICIPRA::Option(size_t i)const
	{
		string str;
		switch (i)
		{
		case NETWORKS:	str = "Atantic|Ontario|Pommes|Quebec"; break;
		};
		return str;
	}


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

	string CUICIPRA::GetOutputFilePath(string filePath)const
	{
		assert(!filePath.empty());
			
			
		filePath.erase(filePath.begin());
		for (size_t n = 0; n < NB_NETWORKS; n++)
			ReplaceString(filePath, string("/")+NETWORK_TIMEZONE_NAME[n], "");

		string workingDir = GetDir(WORKING_DIR);
		return workingDir + filePath;
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
		//msg = UpdateStationsFile(callback);
		//if (!msg)
			//return msg;

		//CTime today = CTime::GetCurrentTime();
		
		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		int nbYears = lastYear - firstYear + 1;
		int current_year = CTRef::GetCurrentTRef().GetYear();
		bool bForecast = as<bool>(FORECAST) && lastYear == current_year;

		callback.AddMessage(GetString(IDS_UPDATE_FILE));
		int nbDownload = 0;

		StringVector networks = as<StringVector>(NETWORKS);
		if (networks.empty())
			networks = StringVector("Atantic|Ontario|Pommes|Quebec", "|");


		//callback.PushTask("Netwoks (" + ToString(networks.size()) + ")", networks.size());

		//open a connection on the server
		CInternetSessionPtr pSession;
		CFtpConnectionPtr pConnection;

		msg = GetFtpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, Get(USER_NAME), Get(PASSWORD));

		if (msg)
		{
			
			CFileInfoVector cleanFileList;

			callback.PushTask(GetString(IDS_LOAD_FILE_LIST) + "(" + ToString((nbYears + (bForecast ? 1 : 0))*networks.size()) + ")", (nbYears + (bForecast ? 1 : 0))*networks.size());

			size_t nbFileFound = 0;
			for (size_t n = 0; n < networks.size(); n++)
			{
				size_t nn = GetNetworkIndex(networks[n]);
				if (nn > NB_NETWORKS)
					continue;

				for (size_t y = 0; y < nbYears + (bForecast ? 1 : 0) && msg; y++)
				{
					int year = firstYear + int(y);


					string subDir = string("/") + NETWORK_NAME[nn] + "/" + (year <= current_year ? (NETWORK_TIMEZONE_NAME[nn] + string("/") + ToString(year) + "/*.BRU") : "pec/*.pec");
					string path = string(NETWORK_NAME[nn]) + "/" + (year <= current_year ? ToString(year) : "pec") + "/";
					string ouputPath = workingDir + path;

					
					//Load files list
					CFileInfoVector fileList;
					msg += UtilWWW::FindFiles(pConnection, subDir, fileList, callback);
					nbFileFound += fileList.size();

					for (CFileInfoVector::const_iterator it = fileList.begin(); it != fileList.end() && msg; it++)
					{
						string filePath = ouputPath + GetFileName(it->m_filePath);
						if (!UtilWWW::IsFileUpToDate(*it, filePath, false))
							cleanFileList.push_back(*it);
					}

					msg += callback.StepIt();
				}

				
			}
		
			pConnection->Close();
			pSession->Close();

			callback.PopTask();
		

			if (msg)
			{
				
				callback.AddMessage(GetString(IDS_NB_FILES_FOUND) + ToString(nbFileFound));
				callback.AddMessage(GetString(IDS_NB_FILES_CLEARED) + ToString(cleanFileList.size()));
						

						//download files
				callback.PushTask("Download (" + ToString(cleanFileList.size()) + " files)", cleanFileList.size());

				

				int nbRun = 0;
				int curI = 0;

				while (curI < cleanFileList.size() && msg)
				{
					nbRun++;
					//open a connection on the server
					msg = GetFtpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, Get(USER_NAME), Get(PASSWORD));


					for (size_t k = curI; k < cleanFileList.size() && msg; k++)
					{
						string ouputFilePath = GetOutputFilePath(cleanFileList[k].m_filePath);
						CreateMultipleDir(GetPath(ouputFilePath));

						//string filePath = ouputPath + GetFileName(fileList[k].m_filePath);
						msg += CopyFile(pConnection, cleanFileList[k].m_filePath, ouputFilePath);
						if (msg)
						{
							curI++;
							nbDownload++;
						}

						msg += callback.StepIt();

					}//for all files 


					//if an error occur: try again
					if (!msg && !callback.GetUserCancel())
					{
						if (nbRun < 5)
						{
							callback.PushTask("Waiting 2 seconds for server...", 100);
							for (size_t i = 0; i < 40 && msg; i++)
							{
								Sleep(50);//wait 50 milisec
								msg += callback.StepIt();
							}
							callback.PopTask();
						}
					}

					pConnection->Close();
					pSession->Close();
				}

			}//for all files

			callback.PopTask();
		}//if msg
	

	

		callback.AddMessage("");
		callback.AddMessage(GetString(IDS_NB_FILES_DOWNLOADED) + ToString(nbDownload));

		return msg;
	}

	string CUICIPRA::GetStationsFilePath()
	{
		string srcFilePath = GetApplicationPath() + "Layers\\CIPRAStations.csv";
		
		return srcFilePath;
		//return GetDir(WORKING_DIR) + "Stations.csv";
		
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

	string CUICIPRA::GetOutputFilePath(const string& network, int year, const string& name)
	{
		return GetDir(WORKING_DIR) + network + "\\" + ToString(year) + "\\" + name;
	}

	ERMsg CUICIPRA::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;

		
		msg = LoadStations(callback);
		if (!msg)
			return msg;

		string workingDir = GetDir(WORKING_DIR);


		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		int nbYear = lastYear - firstYear + 1;
		StringVector networks = as<StringVector>(NETWORKS);


		//find all station in the directories
		
		for (size_t n = 0; n < networks.size(); n++)
		{
			size_t nn = GetNetworkIndex(networks[n]);


			for (size_t y = 0; y < nbYear&&msg; y++)
			{
				int year = firstYear + int(y);

				string path = workingDir + string(NETWORK_NAME[nn]) + "/" + ToString(year) + "/";

				StringVector fileList = GetFilesList(path + "*.BRu");
				
				for (size_t i = 0; i < fileList.size() && msg; i++)
				{
					string stationName = GetFileTitle(fileList[i]);


					if (WBSF::FindStringExact(stationList, stationName, false) == stationList.end())
						stationList.push_back(stationName);
				}

				msg += callback.StepIt(0);
			}
		}
		

		return msg;
	}

	class SpecialFindLocationByName
	{
	public:

		SpecialFindLocationByName(const std::string& name) :m_name(name)
		{}

		bool operator ()(const CLocation& in)const
		{ 
			string name = in.m_name;
			std::replace(name.begin(), name.end(), ' ', '_');
			name = removeAccented(name);

			return name == m_name;
		}


	protected:

		std::string m_name;
	};

	ERMsg CUICIPRA::GetWeatherStation(const string& stationName, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;



		CWeatherDatabaseOptimization::const_iterator it = std::find_if(m_stations.begin(), m_stations.end(), SpecialFindLocationByName(stationName));
		if (it == m_stations.end())
		{
			msg.ajoute(FormatMsg(IDS_NO_STATION_INFORMATION, stationName));
			return msg;
		}


		((CLocation&)station) = *it;
		
		string network = station.GetSSI("Network");
		string fileName = station.m_name;
		std::replace(fileName.begin(), fileName.end(), ' ', '_');
		fileName = removeAccented(fileName);

		station.m_name = PurgeFileName(station.m_name);
		

		cctz::time_zone zone;
		CTimeZones::GetZone(station, zone);
		//zone.lookup();

		string workingDir = GetDir(WORKING_DIR);
		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = lastYear - firstYear + 1;
		int current_year = CTRef::GetCurrentTRef().GetYear();
		bool bForecast = as<bool>(FORECAST) && lastYear == current_year;



		station.CreateYears(firstYear, nbYears);

		//size_t filesize = 0;
		//for (size_t y = 0; y < nbYears&&msg; y++)
		//{
		//	int year = firstYear + int(y);
		//	string filePath = GetOutputFilePath(network, year, fileName+".BRU");

		//	if (FileExists(filePath))
		//	{
		//		CFileInfo filesInfo;
		//		GetFileInfo(filePath, filesInfo);
		//		filesize += filesInfo.m_size;
		//	}//if file exists
		//}//for all years (files)

		if (nbYears>8)
			callback.PushTask(station.m_name, nbYears);

		//now extact data
		for (size_t y = 0; y < nbYears&&msg; y++)
		{
			int year = firstYear + int(y);

			string filePath = GetOutputFilePath(network, year, fileName + ".BRU");

			if (FileExists(filePath))
				msg += ReadDataFile(filePath, zone, TM, station, callback);
		}//for all years

		if (bForecast)
		{
			string filePath = GetDir(WORKING_DIR) + network + "\\" + fileName + ".pre";

			if (FileExists(filePath))
				msg += ReadForecastDataFile(filePath, zone, TM, station, callback);

			msg += callback.StepIt(nbYears>8 ? 1 : 0);
		}

		
		//verify station is valid
		if (msg && station.HaveData())
			msg = station.IsValid();

		if (nbYears>8)
			callback.PopTask();


		return msg;
	}


	ERMsg CUICIPRA::ReadDataFile(const string& filePath, const cctz::time_zone& zone, CTM TM, CWeatherYears& data, CCallback& callback)const
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
		const int COL_POS[NB_VAR_H] = { C_TMIN, C_TAIR, C_TMAX, C_PRCP, -1, C_RELH, -1, C_WNDD, C_SRAD, C_PRES, -1, -1, -1, C_WNDS, -1, -1 };
		const double FACTOR[NB_VAR_H] = { 1, 1, 1, 1, 0, 1, 0, 1, 1000.0 / 3600.0, 10, 0, 0, 0, 1, 0, 0 };

		//now extract data 
		ifStream file;

		msg = file.open(filePath);

		if (msg)
		{
			CWeatherAccumulator stat(TM);

			int DLS = 0;
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


				cctz::civil_second cs(year, int(month) + 1, int(day) + 1, int(hour), 0, 0);
				auto testDLS = zone.lookup(cs);
				
				int DLSh = (testDLS.pre.time_since_epoch().count() -testDLS.post.time_since_epoch().count())/3600;
				if (DLSh == 1)
				{
					if (stat.TRefIsChanging(TRef+1))
						data[stat.GetTRef()].SetData(stat);
				}
					

				DLS += DLSh;
				TRef+=DLS;

				if (stat.TRefIsChanging(TRef))
				{
					data[stat.GetTRef()].SetData(stat);
				}

				for (size_t v = 0; v < NB_VAR_H; v++)
				{
					if (COL_POS[v] >= 0)
					{
						double value = ToDouble((*loop)[COL_POS[v]]);
						if (value > -991 )
						{
							stat.Add(TRef, v, value*FACTOR[v]);
							if (COL_POS[v] == C_RELH)
							{
								double T = ToDouble((*loop)[C_TAIR]);
								if (T > -991 && value <= 100)
								{
									stat.Add(TRef, H_TDEW, Hr2Td(T, value));
								}
							}
						}
					}
				}

				//msg += callback.StepIt(loop->GetLastLine().length() + 2);
				msg += callback.StepIt(0);
			}//for all line (

			if (stat.GetTRef().IsInit())
				data[stat.GetTRef()].SetData(stat);

		}//if load 

		return msg;
	}
	
	
	ERMsg CUICIPRA::ReadForecastDataFile(const string& filePath, const cctz::time_zone& zone, CTM TM, CWeatherYears& data, CCallback& callback)const
	{
		ERMsg msg;

		//LEGENDE = AAAA,MM,JJ,HZ,TMP,OP,POP(0.2),ACC(EQUIV. D EAU),HR,VT
		

		enum TColumns { C_YEAR, C_MONTH, C_DAY, C_HOUR, C_TAIR, C_UNUSED1, C_UNUSED2, C_PRCP, C_RELH, C_VT, NB_COLUMNS };

		//wind speed at 2 meters
		const int COL_POS[NB_VAR_H] = { -1, C_TAIR, -1, C_PRCP, -1, C_RELH, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
		//const double FACTOR[NB_VAR_H] = { 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

		//now extract data 
		ifStream file;

		msg = file.open(filePath);

		if (msg)
		{
			CWeatherAccumulator stat(TM);
			CSVIterator loop(file, ",", false);
			
			for (size_t i = 0; i < 7 && loop != CSVIterator(); ++loop);//skip 7 line
				
			//int DLS = 0;
			for (; loop != CSVIterator(); ++loop)
			{
				int year = ToInt((*loop)[C_YEAR]);
				size_t month = ToInt((*loop)[C_MONTH]) - 1;
				size_t day = ToInt((*loop)[C_DAY]) - 1;
				size_t hour = ToInt((*loop)[C_HOUR]);

				//ASSERT(year >= firstYear && year <= lastYear);
				ASSERT(month >= 0 && month < 12);
				ASSERT(day >= 0 && day < GetNbDayPerMonth(year, month));
				ASSERT(hour >= 0 && hour < 24);

				CTRef TRef(year, month, day, hour);

				//est-ce l'heure local????
				/*cctz::civil_second cs(year, int(month) + 1, int(day) + 1, int(hour), 0, 0);
				auto testDLS = zone.lookup(cs);

				int DLSh = (testDLS.pre.time_since_epoch().count() - testDLS.post.time_since_epoch().count()) / 3600;
				if (DLSh == 1)
				{
					if (stat.TRefIsChanging(TRef + 1))
						data[stat.GetTRef()].SetData(stat);
				}

				DLS += DLSh;
				TRef += DLS;*/

				if (stat.TRefIsChanging(TRef))
				{
					data[stat.GetTRef()].SetData(stat);
				}

				for (size_t v = 0; v < NB_VAR_H; v++)
				{
					if (COL_POS[v] >= 0)
					{
						double value = ToDouble((*loop)[COL_POS[v]]);
						if (value > -991 )
						{
							stat.Add(TRef, v, value);
							if (COL_POS[v] == C_RELH)
							{
								double T = ToDouble((*loop)[C_TAIR]);
								if (T > -991 && value <= 100)
								{
									stat.Add(TRef, H_TDEW, Hr2Td(T, value));
								}
							}
						}
					}
				}

				
				msg += callback.StepIt(0);
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


		//if (!FileExists(dstFilePath))
		//{
			//string srcFilePath = GetApplicationPath() + "Layers\\CIPRAStations.csv";

			//CString src(CStringA(srcFilePath.c_str()));
			//CString dst(CStringA(dstFilePath.c_str()));
			//CopyFile(src, dst, TRUE);
		//}


		return msg;

	}
}