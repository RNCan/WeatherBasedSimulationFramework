#include "StdAfx.h"
#include "UIMDDELCC.h"

#include "basic/WeatherStation.h"
#include "basic/CSV.h"
#include "UI/Common/SYShowMessage.h"

#include "TaskFactory.h"
#include "../Resource.h"
#include "WeatherBasedSimulationString.h"

using namespace WBSF::HOURLY_DATA;
using namespace std;
using namespace UtilWWW;

namespace WBSF
{

	//*********************************************************************

	static const DWORD FLAGS = INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_PRAGMA_NOCACHE | INTERNET_FLAG_TRANSFER_BINARY;
	const char* CUIMDDELCC::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "FirstYear", "LastYear", "UpdateUntil", "UpdateStationsList" };
	const size_t CUIMDDELCC::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_STRING, T_STRING, T_STRING, T_BOOL};
	const UINT CUIMDDELCC::ATTRIBUTE_TITLE_ID = IDS_UPDATER_MDDELCC_DAILY_P;
	const UINT CUIMDDELCC::DESCRIPTION_TITLE_ID = ID_TASK_MDDELCC_DAILY;

	const char* CUIMDDELCC::CLASS_NAME(){ static const char* THE_CLASS_NAME = "MDDELCC_Daily";  return THE_CLASS_NAME; }
	CTaskBase::TType CUIMDDELCC::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUIMDDELCC::CLASS_NAME(), (createF)CUIMDDELCC::create);


	const char* CUIMDDELCC::SERVER_NAME = "www.mddelcc.gouv.qc.ca";
	


	CUIMDDELCC::CUIMDDELCC(void)
	{}

	CUIMDDELCC::~CUIMDDELCC(void)
	{}


	std::string CUIMDDELCC::Option(size_t i)const
	{
		string str;
		return str;
	}

	std::string CUIMDDELCC::Default(size_t i)const
	{
		string str;

		switch (i)
		{
		case WORKING_DIR: str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()) + "MDDELCC\\"; break;
		case FIRST_YEAR:
		case LAST_YEAR:	str = ToString(CTRef::GetCurrentTRef().GetYear()); break;
		case UPDATE_UNTIL: str = "15"; break;
		case UPDATE_STATIONS_LIST: str = "0"; break;
		};
		return str;
	}

	//************************************************************************************************************
	//Load station definition list section


	ERMsg CUIMDDELCC::DownloadStationList(CLocationVector& stationList, CCallback& callback)const
	{
		ERMsg msg;

		//Interface attribute index to attribute index
		//www.mddelcc.gouv.qc.ca/climat/donnees/sommaire.asp?cle=7110072&mois=10&annee=2014
		//climat/surveillance/station.asp
		static const char* URL_STATION = "climat/surveillance/station.asp";
		static const char* URL_INDEX = "climat/donnees/OQCarte.asp";

		callback.PushTask(GetString(IDS_LOAD_STATION_LIST), NOT_INIT);

		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;

		msg = GetHttpConnection(SERVER_NAME, pConnection, pSession);
		if (!msg)
			return msg;

		string source;
		msg = GetPageText(pConnection, URL_STATION, source);
		if (msg)
		{

			string::size_type posBegin = source.find("helpOver");

			while (posBegin != string::npos)
			{

				string name = FindString(source, "<b>", "</b>", posBegin); Trim(name);
				string period = FindString(source, "ouverture :", "<br />", posBegin); Trim(period);
				string latitude = FindString(source, "Latitude :", "°", posBegin); Trim(latitude); std::replace(latitude.begin(), latitude.end(), ',', '.');
				string longitude = FindString(source, "Longitude :", "°", posBegin); Trim(longitude); std::replace(longitude.begin(), longitude.end(), ',', '.');
				string altitude = FindString(source, "Altitude :", "m", posBegin); Trim(altitude); std::replace(altitude.begin(), altitude.end(), ',', '.');
				string ID = FindString(source, ">", "<", posBegin); Trim(ID);
				if (altitude.empty())
					altitude = "-999";

				CLocation stationInfo(name, ID, stod(latitude), stod(longitude), stod(altitude));
				stationInfo.SetSSI("Begin", period);
				stationList.push_back( stationInfo);

				posBegin = source.find("helpOver", posBegin);
			}
		}


		pConnection->Close();
		pSession->Close();

		callback.AddMessage(GetString(IDS_NB_STATIONS) + ToString(stationList.size()));


		callback.PopTask();

		return msg;
	}
	

	//************************************************************************************************************
	//data section
	string CleanStr(const string& str)
	{
		string::size_type lenght = str.find_last_of('>') + 1;
		string val = str.substr(lenght);
		ReplaceString(val, "&nbsp;", "");
		ReplaceString(val, ",", ".");

		return val;
	}

	ERMsg CUIMDDELCC::CopyStationDataPage(CHttpConnectionPtr& pConnection, const string& ID, int year, size_t m, const string& filePath)
	{
		ERMsg msg;


		static const char pageDataFormat[] =
		{
			"climat/donnees/sommaire.asp?"
			"cle=%s&"
			"date_selection=%4d-%02d-%02d"
		};

		
		//string URL = "climat/donnees/sommaire.asp?cle=7030170&date_selection=2016-03-01";
		string URL = FormatA(pageDataFormat, ID.c_str(), year, m + 1, 1);

		string source;
		msg = GetPageText(pConnection, URL, source);
		if (msg)
		{
			ofStream file;

			msg = file.open(filePath);

			if (msg && source.find("Le mois que vous demandez") == string::npos)
			{
				file << "Year,Month,Day,Tmax,Tmean,Tmin,Rain,Snow,Prcp,SnwD" << endl;
				string::size_type posBegin = source.find("<tbody><tr class=", 0);


				while (posBegin != string::npos)
				{
					file << year << "," << FormatA("%02d", m + 1);
					string str = FindString(source, "<td", "</td>", posBegin);
					file << "," << CleanStr(str);//add day

					for (int i = 0; i < 7; i++)
					{
						string str1 = CleanStr(FindString(source, "<td", "</td>", posBegin));
						string str2 = CleanStr(FindString(source, "<td", "</td>", posBegin));

						//C Cumulé	E Estimé	I Incomplet	Q Quantité Inconnue
						//D Douteux	F Forcé	K Estimé(krigeage)	T Trace

						if (str1.empty())
							str1 = "-999.0";

						if (str1 == "-" )
							str1 = "-999.0";
						if (!str2.empty())
						{
							switch (str2[0])
							{
							case 'I'://Incomplet
							case 'Q':// Quantité Inconnue
							case 'D':// Douteux	
							case 'K': // Estimé(krigeage)	
								str1 = "-999.0";
								break;
							case 'C'://Cumulé
							case 'E'://Estimé	
							case 'F':// Forcé
							case 'T':// Trace
								break;//keep the value
							default: ASSERT(false);
							}
						}

						file << "," << str1;

						if (i == 2)//skip one extra val
							FindString(source, "<td", "</td>", posBegin);
					}

					file << endl;
					posBegin = source.find("<tbody><tr class=", posBegin);
				}

				file.close();
			}
		}


		return msg;
	}

	ERMsg CUIMDDELCC::DownloadStation(CHttpConnectionPtr& pConnection, const CLocation& station, CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);

		
		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = lastYear - firstYear + 1;

		CTRef today = CTRef::GetCurrentTRef();

		vector<array<bool, 12>> bNeedDownload(nbYears);
		size_t nbFilesToDownload = 0;

		for (size_t y = 0; y < nbYears&&msg; y++)
		{
			int year = firstYear + int(y);
		
			size_t monthMax = year < today.m_year ? 12 : today.m_month + 1;
			for (size_t m = 0; m < monthMax&&msg; m++)
			{
				string filePath = GetOutputFilePath(year, m, station.m_ID);
				if (FileExists(filePath))
				{
					if (year == today.m_year && m == today.m_month)
					{
						bNeedDownload[y][m] = true;
					}
					else
					{
						CFileInfo info = GetFileInfo(filePath);
						if (info.m_size == 0)
						{
							bNeedDownload[y][m] = false;
						}
						else
						{
							CTimeRef TRef1(info.m_time);
							CTRef TRef2(year, m, LAST_DAY);

							bNeedDownload[y][m] = !TRef1.IsInit() || TRef1 - TRef2 < as<int>(UPDATE_UNTIL); //let UPDATE_UNTIL days
							nbFilesToDownload += bNeedDownload[y][m] ? 1 : 0;
						}
					}
				}
				else
				{
					bNeedDownload[y][m] = true;
					nbFilesToDownload++;
				}
			}
		}

		if (nbFilesToDownload>5)
			callback.PushTask(station.m_name, nbFilesToDownload);


		for (int y = 0; y < nbYears&&msg; y++)
		{
			int year = firstYear + int(y);

			size_t monthMax = year < today.m_year ? 12 : today.m_month + 1;
			for (size_t m = 0; m < monthMax&&msg; m++)
			{
				if (bNeedDownload[y][m])
				{
					string filePath = GetOutputFilePath(year, m, station.m_ID);;
					CreateMultipleDir(GetPath(filePath));

					msg += CopyStationDataPage(pConnection, station.m_ID, year, m, filePath);
					
					if (nbFilesToDownload>5)
						msg += callback.StepIt();
				}
			}
		}

		if (nbFilesToDownload>5)
			callback.PopTask();


		return msg;
	}

	

	//*************************************************************************************************

	ERMsg CUIMDDELCC::Execute(CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);
		CreateMultipleDir(workingDir);

		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(workingDir, 1);
		callback.AddMessage(GetString(IDS_UPDATE_FROM));
		callback.AddMessage(SERVER_NAME, 1);
		callback.AddMessage("");

		bool bForceUpdateList = as<bool>(UPDATE_STATIONS_LIST);

		//Getlocal station list
		if (!FileExists(GetStationListFilePath()) || bForceUpdateList)
		{
			msg = DownloadStationList(m_stations, callback);
			if (msg)
				msg = m_stations.Save(GetStationListFilePath());
		}
		else
		{
			msg = m_stations.Load(GetStationListFilePath());
		}

		if (!msg)
			return msg;

		//remove all station begginning with 0
		CLocationVector stationList;
		stationList.reserve(m_stations.size());
		for (CLocationVector::const_iterator it = m_stations.begin(); it != m_stations.end(); it++)
			if (!it->m_ID.empty() && it->m_ID[0] != '0')
				stationList.push_back(*it);


		callback.PushTask("Download MDDELCC (" + ToString(stationList.size()) + " stations)", stationList.size());


		int nbRun = 0;
		size_t curI = 0;

		while (curI < stationList.size() && msg)
		{
			nbRun++;

			CInternetSessionPtr pSession;
			CHttpConnectionPtr pConnection;

			msg = GetHttpConnection(SERVER_NAME, pConnection, pSession);

			if (msg)
			{
				TRY
				{
					for (size_t i = curI; i < stationList.size() && msg; i++)
					{
						ERMsg msgTmp = DownloadStation(pConnection, stationList[i], callback);
						//if (msgTmp)
						{
							curI++;
							nbRun = 0;
							msg += callback.StepIt();
						}

						if (!msgTmp)
							callback.AddMessage(msgTmp);
					}
				}
				CATCH_ALL(e)
				{
					msg = UtilWin::SYGetMessage(*e);
				}
				END_CATCH_ALL

					//if an error occur: try again
					if (!msg && !callback.GetUserCancel())
					{
						if (nbRun < 5)
						{
							callback.AddMessage(msg);
							msg.asgType(ERMsg::OK);
							Sleep(1000);//wait 1 sec
						}
					}

				//clean connection
				pConnection->Close();
				pSession->Close();
			}
		}


		callback.AddMessage(GetString(IDS_NB_FILES_DOWNLOADED) + ToString(curI));
		callback.PopTask();



		
		return msg;
	}

	//****************************************************************************************************

	string CUIMDDELCC::GetOutputFilePath(int year, size_t m, const string& stationName)const
	{
		return GetDir(WORKING_DIR) + ToString(year) + "\\" + FormatA("%02d", m + 1) + "\\" + stationName + ".csv";
	}


	ERMsg CUIMDDELCC::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;

		msg = m_stations.Load(GetStationListFilePath());

		if (msg)
		{
			int firstYear = as<int>(FIRST_YEAR);
			int lastYear = as<int>(LAST_YEAR);

			for (CLocationVector::const_iterator it = m_stations.begin(); it != m_stations.end(); it++)
			{
				if (!it->m_ID.empty() && it->m_ID[0] != '0')
					stationList.push_back(it->m_ID);
			}
		}


		return msg;
	}

	std::string CUIMDDELCC::GetStationListFilePath()const
	{
		return GetDir(WORKING_DIR) + "DailyStationsList.csv";
	}

	
	static std::string TraitFileName(std::string name)
	{
		size_t begin = name.find('(');
		size_t end = name.find(')');
		if (begin != std::string::npos && end != std::string::npos)
			name.erase(begin, end);

		std::replace(name.begin(), name.end(), ',', '-');
		std::replace(name.begin(), name.end(), ';', '-');
		UppercaseFirstLetter(name);
		Trim(name);

		return name;
	}

	ERMsg CUIMDDELCC::GetWeatherStation(const string& ID, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;

		size_t it = m_stations.FindByID(ID);
		if (it == NOT_INIT)
		{
			msg.ajoute(FormatMsg(IDS_NO_STATION_INFORMATION, ID));
			return msg;
		}

		((CLocation&)station) = m_stations[it];
		station.m_name = TraitFileName(station.m_name);

		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		int nbYears = lastYear - firstYear + 1;
		station.CreateYears(firstYear, nbYears);

		CTRef today = CTRef::GetCurrentTRef();

		//now extract data 
		for (size_t y = 0; y < nbYears&&msg; y++)
		{
			int year = firstYear + int(y);
			size_t nbMonth = year < today.m_year ? 12 : today.m_month + 1;
			for (size_t m = 0; m < nbMonth&&msg; m++)
			{
				string filePath = GetOutputFilePath(year, m, ID);
				CFileInfo info = GetFileInfo(filePath);
				if (info.m_size>0)
					msg = ReadData(filePath, station[year]);
			}
		}


		if (msg && station.HaveData())
			msg = station.IsValid();

		return msg;
	}
	
	ERMsg CUIMDDELCC::ReadData(const string& filePath, CWeatherYear& dailyData)const
	{
		ERMsg msg;

		enum{ YEAR, MONTH, DAY, MAX_TEMP, MEAN_TEMP, MIN_TEMP, TOTAL_RAIN, TOTAL_SNOW, TOTAL_PRECIP, SNOW_ON_GRND, NB_DAILY_COLUMN };

		//open file
		ifStream file;
		msg = file.open(filePath);

		if (msg)
		{
			size_t i = 0;
			for (CSVIterator loop(file, ",", true, true); loop != CSVIterator(); ++loop, i++)
			{
				ENSURE(loop.Header().size() == NB_DAILY_COLUMN);

				int year = ToInt((*loop)[YEAR]);
				size_t month =ToInt((*loop)[MONTH]) - 1;
				size_t day = ToInt((*loop)[DAY]) - 1;

				ASSERT(month<12);
				ASSERT(day < GetNbDayPerMonth(year, month));
				CTRef Tref(year, month, day);


				float Tmin = ToFloat((*loop)[MIN_TEMP]);
				float Tair = ToFloat((*loop)[MEAN_TEMP]);
				float Tmax = ToFloat((*loop)[MAX_TEMP]);


				if (Tmin > -999 && Tmax>-999)
				{
					ASSERT(Tmin >= -70 && Tmin <= 70);
					ASSERT(Tmax >= -70 && Tmax <= 70);
					dailyData[Tref][H_TMIN2] = Tmin;
					dailyData[Tref][H_TMAX2] = Tmax;
				}

				if (Tair>-999)
				{ 
					ASSERT(Tair >= -70 && Tair <= 70);
					dailyData[Tref][H_TAIR2] = Tair; 
				}

				float prcp = ToFloat((*loop)[TOTAL_PRECIP]);
				if (prcp > -999)
				{
					ASSERT(prcp >= 0 && prcp < 1000);
					dailyData[Tref][H_PRCP] = prcp;
				}

				float snow = ToFloat((*loop)[TOTAL_SNOW]);
				if (snow > -999)
				{
					ASSERT(snow >= 0 && snow < 1000);
					dailyData[Tref][H_SNOW] = snow;
				}

				float sndh = ToFloat((*loop)[SNOW_ON_GRND]);
				if (sndh > -999)
				{
					ASSERT(sndh >= 0 && sndh < 1000);
					dailyData[Tref][H_SNDH] = sndh;
				}
			}//for all line
		}

		return msg;
	}

}

