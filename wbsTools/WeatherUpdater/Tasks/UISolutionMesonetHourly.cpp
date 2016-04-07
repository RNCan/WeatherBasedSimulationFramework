//USER = biosim.stare1
//PASSWORD = PdwtZJLQHTpV


#include "stdafx.h"
#include "UISolutionMesonetHourly.h"

#include "basic/WeatherStation.h"
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


	const char* CUISolutionMesonetHourly::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "UserName", "Password", "WorkingDir", "FirstYear", "LastYear", "UpdateStationList" };
	const size_t CUISolutionMesonetHourly::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_STRING, T_PASSWORD, T_PATH, T_STRING, T_STRING, T_BOOL };
	const UINT CUISolutionMesonetHourly::ATTRIBUTE_TITLE_ID = IDS_UPDATER_SM_HOURLY_P;
	
	const char* CUISolutionMesonetHourly::CLASS_NAME(){ static const char* THE_CLASS_NAME = "SolutionMesonetHourly";  return THE_CLASS_NAME; }
	CTaskBase::TType CUISolutionMesonetHourly::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterClass(CUISolutionMesonetHourly::CLASS_NAME(), CUISolutionMesonetHourly::create);
	static size_t OLD_CLASS_ID = CTaskFactory::RegisterClass("MesonetQuebecHourly", CUISolutionMesonetHourly::create);

	const char* CUISolutionMesonetHourly::SERVER_NAME = "data.mesonet-quebec.org";
	const char* CUISolutionMesonetHourly::MTS_SUB_DIR = "text/meteo/quebec/realtime/mts/";



	CUISolutionMesonetHourly::CUISolutionMesonetHourly(void)
	{}

	CUISolutionMesonetHourly::~CUISolutionMesonetHourly(void)
	{}



	std::string CUISolutionMesonetHourly::Default(size_t i)const
	{
		string str;
		

		switch (i)
		{
		case FIRST_YEAR:
		case LAST_YEAR:	str = ToString(CTRef::GetCurrentTRef().GetYear()); break;
		//{
		//	CTRef TRef = CTRef(CTRef::GetCurrentTRef().GetYear(), FIRST_MONTH, FIRST_DAY);
		//	str = TRef.GetFormatedString();
		//	break;
		//}

		//{
		//	CTRef TRef = CTRef(CTRef::GetCurrentTRef().GetYear(), LAST_MONTH, LAST_DAY);
		//	str = TRef.GetFormatedString();
		//	break;
		//}
		case UPDATE_STATION_LIST:	str = "1"; break;
		};
		return str;
	}
//**********************************************************************************************************************************
	ERMsg CUISolutionMesonetHourly::Execute(CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);

		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(workingDir, 1);
		callback.AddMessage("");

		if (false)
		{
			//open a connection on the server
			CInternetSessionPtr pSession;
			CFtpConnectionPtr pConnection;

			//string password = Decrypt(Get(PASSWORD));
			msg = GetFtpConnection("horus.mesonet-quebec.org", pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, Get(USER_NAME), Get(PASSWORD));
			if (msg && as<bool>(UPDATE_STATION_LIST))
				msg = UpdateStationList(pConnection, callback);

			if (!msg)
				return msg;
		}

		

		CTRef today = CTRef::GetCurrentTRef();

		//open a connection on the server
		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;

		msg = GetHttpConnection(SERVER_NAME, pConnection, pSession);
		if (!msg)
			return msg;

		callback.AddMessage(GetString(IDS_UPDATE_FILE));
		int nbDownload = 0;

		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = lastYear - firstYear + 1;

		for (size_t y = 0; y < nbYears&&msg; y++)
		{
			int year = firstYear + int(y);

			callback.PushTask(FormatA("%04d", year), GetNbDaysPerYear(year));

			for (size_t m = 0; m < 12 && msg; m++)
			{
				for (size_t d = 0; d < GetNbDayPerMonth(year, m) && msg; d++)
				{
					CTRef TRef(year, m, d);
					if (TRef < today)
					{
						string URL = MTS_SUB_DIR + FormatA("%04d/%02d/%02d/", year, m + 1, d + 1) + "*.mts";

						CFileInfoVector fileList;
						msg += FindFiles(pConnection, URL, fileList);

						string ouputPath = workingDir + FormatA("%04d\\%02d\\%02d\\", year, m + 1, d + 1);
						CreateMultipleDir(ouputPath);

						for (size_t f = 0; f < fileList.size() && msg; f++)
						{
							string filePath = ouputPath + GetFileName(fileList[f].m_filePath);

							if (!IsFileUpToDate(fileList[f], filePath, false))
							{
								msg += CopyFile(pConnection, fileList[f].m_filePath, filePath);
								if (msg)
									nbDownload++;
							}

							if (msg)
								msg += callback.StepIt(0);
						}// for all files
					}//if before today

					if (msg)
						msg += callback.StepIt();

				}//for all days
			}//for all months

			callback.PopTask();
		}//for all years

		callback.AddMessage(GetString(IDS_NB_FILES_DOWNLOADED) + ToString(nbDownload), 2);


		pConnection->Close();
		pSession->Close();


		return msg;
	}


	std::string CUISolutionMesonetHourly::GetStationListFilePath()const
	{
		return GetDir(WORKING_DIR) + "stations.xml";
	}


	std::string CUISolutionMesonetHourly::GetMissingFilePath()const
	{
		return GetDir(WORKING_DIR) + "missingStations.csv";
	}

	ERMsg CUISolutionMesonetHourly::UpdateStationList(CFtpConnectionPtr& pConnection, CCallback& callback)const
	{
		ERMsg msg;

		string FTPFilePath = "/stations.xml";
		string localFilePath = GetStationListFilePath();

		if (UtilWWW::IsFileUpToDate(pConnection, FTPFilePath.c_str(), localFilePath.c_str()))
			return msg;

		CreateMultipleDir(GetPath(localFilePath));
		return UtilWWW::CopyFile(pConnection, FTPFilePath.c_str(), localFilePath.c_str());

	}


	ERMsg CUISolutionMesonetHourly::LoadStationList(CCallback& callback)
	{
		ERMsg msg;

		m_stations.clear();

		ifStream file;
		file.imbue(std::locale(std::locale::classic(), new std::codecvt_utf8<char>()));
		msg = file.open(GetStationListFilePath());
		if (msg)
		{
			try
			{
				string str = file.GetText();
				std::replace(str.begin(), str.end(), '\'', 'Ã');

				zen::XmlDoc doc = zen::parse(str);

				zen::XmlIn in(doc.root());
				for (zen::XmlIn child = in["station"]; child; child.next())
				{
					CLocation tmp;
					child.attribute("fullname", tmp.m_name);
					std::replace(tmp.m_name.begin(), tmp.m_name.end(), 'Ã', '\'');
					child.attribute("name", tmp.m_ID);
					child.attribute("lat", tmp.m_lat);
					child.attribute("lon", tmp.m_lon);
					child.attribute("elev", tmp.m_elev);
					m_stations[tmp.m_ID] = tmp;
				}

			}
			catch (const zen::XmlFileError& e)
			{
				// handle error
				msg.ajoute(GetErrorDescription(e.lastError));
			}
			catch (const zen::XmlParsingError& e)
			{
				// handle error
				msg.ajoute("Error parsing XML file: col=" + ToString(e.col) + ", row=" + ToString(e.row));
			}
		}

		CLocationVector missing;
		if (missing.Load(GetMissingFilePath()))
		{
			for (auto it = missing.begin(); it != missing.end(); it++)
				if (m_stations.find(it->m_ID) == m_stations.end())
					m_stations[it->m_ID] = *it;
		}



		return msg;
	}

	ERMsg CUISolutionMesonetHourly::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;

		msg = LoadStationList(callback);
		if (!msg)
			return msg;


		set<string> fileList;

		string workingDir = GetDir(WORKING_DIR);
		CTRef today = CTRef::GetCurrentTRef();

		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = lastYear - firstYear + 1;

		callback.PushTask(GetString(IDS_LOAD_STATION_LIST), nbYears * 12);

		//find all station available that meet criterious
		for (size_t y = 0; y < nbYears&&msg; y++)
		{
			int year = firstYear + int(y);
			for (size_t m = 0; m < 12 && msg; m++)
			{
				size_t d = 0;//take only the first day of each month
				CTRef TRef(year, m, d);
				if (TRef < today)
				{
					string inputString = workingDir + FormatA("%4d\\%02d\\%02d\\*.mts", year, m + 1, d + 1);
					StringVector fileListTmp = GetFilesList(inputString, 2, true);

					for (size_t i = 0; i < fileListTmp.size(); i++)
					{
						string fileTitle = Right(GetFileTitle(fileListTmp[i]), 4);
						MakeLower(fileTitle);

						fileList.insert(fileTitle);
					}
				}

				msg += callback.StepIt();
			}
		}

		stationList.insert(stationList.begin(), fileList.begin(), fileList.end());
		callback.PopTask();

		return msg;
	}

	ERMsg CUISolutionMesonetHourly::GetWeatherStation(const string& stationID, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;

		CLocationMap::const_iterator it = m_stations.find(stationID);
		if (it != m_stations.end())
		{
			((CLocation&)station) = it->second;
		}
		else
		{
			msg.ajoute(FormatMsg(IDS_NO_STATION_INFORMATION, stationID));
			return msg;
		}

		string workingDir = GetDir(WORKING_DIR);
		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = lastYear - firstYear + 1;


		size_t nbFiles = 0;
		for (size_t y = 0; y < nbYears; y++)
		{
			int year = firstYear + int(y);
			
			for (size_t m = 0; m < 12 && msg; m++)
			{
				for (size_t d = 0; d < GetNbDayPerMonth(year, m) && msg; d++)
				{
					string filePath = workingDir + FormatA("%4d\\%02d\\%02d\\%4d%02d%02d%s.mts", year, m + 1, d + 1, year, m + 1, d + 1, stationID.c_str());
					if (FileExists(filePath))
						nbFiles++;
				}
			}
		}

		//now extract data 
		callback.PushTask(station.m_name, nbFiles);
		for (size_t y = 0; y < nbYears&&msg; y++)
		{
			int year = firstYear + int(y);

			for (size_t m = 0; m < 12 && msg; m++)
			{
				for (size_t d = 0; d < GetNbDayPerMonth(year, m) && msg; d++)
				{
					string filePath = workingDir + FormatA("%4d\\%02d\\%02d\\%4d%02d%02d%s.mts", year, m + 1, d + 1, year, m + 1, d + 1, stationID.c_str());
					if (FileExists(filePath))
					{
						msg = ReadData(filePath, TM, station[year], callback);
						if (msg)
							msg += callback.StepIt();
					}
				}
			}
		}

		if (msg)
		{
			//verify station is valid
			if (station.HaveData())
			{
				msg = station.IsValid();
			}
		}

		callback.PopTask();

		return msg;
	}

	double Convert(size_t v, double value)
	{

		switch (v)
		{
		case H_WNDS: value *= 3600.0 / 1000.0; break;	//m/s -> km/h
		case H_SNDH: value = max(0.0, value); break;	//éliminate value under zéro???
		case H_PRES: value = 10 * value; break;			//kPa -> hPa
		default:;//do nothing
		}

		return value;
	}

	ERMsg CUISolutionMesonetHourly::ReadData(const string& filePath, CTM TM, CYear& data, CCallback& callback)const
	{
		ERMsg msg;

//		CTRef startDate = FromFormatedString(Get(START_DATE));
	//	CTRef endDate = FromFormatedString(Get(END_DATE));
		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);

//		size_t nbYears = startDate.GetYear() - endDate.GetYear() + 1;

	//	int nbYear = m_lastYear - m_firstYear + 1;
		enum { C_STID, C_STNM, C_MINUTES, C_LAT, C_LON, C_ELEV, C_TAIR, C_RELH, C_TDEW, C_WDIR, C_WSPD, C_WMAX, C_TAIR3HR, C_TDEW3HR, C_RAIN1HR, C_RAIN3HR, C_RAIN, C_PRES, C_PMSL, C_PMSL3HR, C_SNOW, C_SRAD, C_TAIRMIN, C_TAIRMAX, C_PT020H, C_PT040H, C_PT050H, NB_INPUT_HOURLY_COLUMN };
		const int COL_POS[NB_VAR_H] = { C_TAIR, -1, C_RAIN1HR, C_TDEW, C_RELH, C_WSPD, C_WDIR, C_SRAD, /*C_PRES*/C_PMSL, -1, C_SNOW, -1, -1, -1, -1, -1, -1, -1 };

		//now extact data 
		ifStream file;

		msg = file.open(filePath);

		if (msg)
		{
			CWeatherAccumulator stat(TM);

			string copyright;
			getline(file, copyright);
			string dateTimeStr;
			getline(file, dateTimeStr);
			StringVector dateTime = Tokenize(dateTimeStr, " ", true);
			string header;
			getline(file, header);

			int year = stoi(dateTime[1]);
			int month = stoi(dateTime[2]);
			int day = stoi(dateTime[3]);
			int hour = stoi(dateTime[4]);

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


			size_t i = 0;

			string line;
			while (getline(file, line) && msg)
			{
				Trim(line);
				ASSERT(!line.empty());

				StringVector vars = Tokenize(line, " ", true);
				ASSERT(vars.size() >= NB_INPUT_HOURLY_COLUMN);

				int deltaHour = stoi(vars[C_MINUTES]) / 60;

				CTRef TRef = CTRef(year, month, day, hour) + deltaHour;

				if (stat.TRefIsChanging(TRef) && data.GetEntireTPeriod(CTM(CTM::HOURLY)).IsInside(stat.GetTRef()))
					data[stat.GetTRef()].SetData(stat);

				bool bValid[NB_VAR_H] = { 0 };

				for (size_t v = 0; v<NB_VAR_H; v++)
				{
					if (COL_POS[v] >= 0)
					{
						double value = stod(vars[COL_POS[v]]);

						if (value>-996)
							stat.Add(TRef, v, Convert(v, value));
					}
				}

				msg += callback.StepIt(0);
			}//for all line


			if (stat.GetTRef().IsInit() && data.GetEntireTPeriod(CTM(CTM::HOURLY)).IsInside(stat.GetTRef()))
				data[stat.GetTRef()].SetData(stat);

		}//if load 

		return msg;
	}


}