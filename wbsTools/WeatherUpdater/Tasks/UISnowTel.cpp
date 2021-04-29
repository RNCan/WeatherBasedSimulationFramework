#include "StdAfx.h"
#include <boost\dynamic_bitset.hpp>
#include "UISnowTel.h"


#include "Basic/FileStamp.h"
#include "Basic/HourlyDatabase.h"
#include "Basic/CSV.h"
#include "UI/Common/SYShowMessage.h"
#include "../Resource.h"
#include "TaskFactory.h"
#include "StateSelection.h"


using namespace WBSF::HOURLY_DATA;
using namespace WBSF::WEATHER;
using namespace std;
using namespace UtilWWW;


namespace WBSF
{
	 

	
	//*********************************************************************
	static const DWORD FLAGS = INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_PRAGMA_NOCACHE;



	const char* CUISnoTel::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "DataType", "State", "FirstYear", "LastYear", "UpdateUntil", "UpdateStationList" };
	const size_t CUISnoTel::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_COMBO_INDEX, T_STRING_SELECT, T_STRING, T_STRING, T_STRING, T_BOOL };
	const UINT CUISnoTel::ATTRIBUTE_TITLE_ID = IDS_UPDATER_SNOTEL_P;
	const UINT CUISnoTel::DESCRIPTION_TITLE_ID = ID_TASK_SNOTEL;

	const char* CUISnoTel::CLASS_NAME(){ static const char* THE_CLASS_NAME = "SNOTEL";  return THE_CLASS_NAME; }
	CTaskBase::TType CUISnoTel::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUISnoTel::CLASS_NAME(), (createF)CUISnoTel::create);


	const char* CUISnoTel::SERVER_NAME = "www.wcc.nrcs.usda.gov";
	const char* CUISnoTel::TEMPORAL_TYPE_NAME[NB_TEMPORAL_TYPE] = { "Hourly", "Daily", "Monthly" };

	bool CUISnoTel::IsInclude(size_t state)
	{

		enum{ NB_SNOTEL_STATES = 13 };
		static const size_t SNOTEL_STATES[NB_SNOTEL_STATES] =
		{
			CStateSelection::$AK,
			CStateSelection::$AZ,
			CStateSelection::$CA,
			CStateSelection::$CO,
			CStateSelection::$ID,
			CStateSelection::$MT,
			CStateSelection::$NV,
			CStateSelection::$NM,
			CStateSelection::$OR,
			CStateSelection::$SD,
			CStateSelection::$UT,
			CStateSelection::$WA,
			CStateSelection::$WY,
		};

		bool bInclude = false;
		for (size_t i = 0; i < NB_SNOTEL_STATES&&!bInclude; i++)
			bInclude = state == SNOTEL_STATES[i];

		return bInclude;
	}

	std::string CUISnoTel::GetSnoTelPossibleValue()
	{
		string str;

		for (size_t i = 0; i < NB_USA_STATES; i++)
		{
			str += i != 0 ? "|" : "";
			if (IsInclude(i))
				str += string(CStateSelection::DEFAULT_LIST[i].m_abrv) + "=" + CStateSelection::DEFAULT_LIST[i].m_title;
		}

		return str;
	}



	CUISnoTel::CUISnoTel(void)
	{}

	CUISnoTel::~CUISnoTel(void)
	{}


	std::string CUISnoTel::Option(size_t i)const
	{
		string str;
		switch (i)
		{
		case DATA_TYPE:	str = GetString(IDS_SNOTEL_DATA_TYPE); break;
		case STATES:	str = GetSnoTelPossibleValue(); break;
		};
		return str;
	}

	std::string CUISnoTel::Default(size_t i)const
	{
		string str;

		ASSERT(m_pProject);

		switch (i)
		{
		case WORKING_DIR:		str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()) + "SnoTel\\"; break;
		case DATA_TYPE:			str = ToString(T_HOURLY); break;
		case FIRST_YEAR:
		case LAST_YEAR:			str = ToString(CTRef::GetCurrentTRef().GetYear()); break;
		case UPDATE_UNTIL:		str = "15"; break;
		case UPDATE_STATIONLIST:str = 1; break;
		};

		return str;
	}


	string CUISnoTel::GetOutputFilePath(int year, size_t type, const string& ID)const
	{
		string filePath;
		if (type == T_MONTHLY)
			filePath = GetDir(WORKING_DIR) + TEMPORAL_TYPE_NAME[type] + "\\" + ID + ".csv";
		else
			filePath = GetDir(WORKING_DIR) + TEMPORAL_TYPE_NAME[type] + "\\" + ToString(year) + "\\" + ID + ".csv";

		return filePath;
	}

	ERMsg CUISnoTel::UpdateStationInformation(CCallback& callback)const
	{
		ERMsg msg;

		enum TNetwork{ SCAN, SNTL, SNTLT, SNOW, MPRC, OTHER, NB_NETWORK };
		enum TColumns{ C_NTWK, C_WYEAR, C_STATE, C_SITE_NAME, C_TS, C_START_DATE, C_END_DATE, C_LATITUDE, C_LONGITUDE, C_ELEV, C_COUNTY, C_HUC, NB_COLUMNS };
		enum TReportColumn{ R_STATION_ID, R_ACTON_ID, R_LATITUDE, R_LONGITUDE, R_ELEVATION, R_IN_SERVICEDATE, R_OUT_SERVICEDATE, NB_REPORT_COLUMNS };

		static const char* NETWORK_ID[NB_NETWORK] = { "scan", "sntl", "sntlt", "snow", "mprc", "other" };

		CLocationVector stations;

		callback.PushTask(GetString(IDS_LOAD_STATION_LIST), NB_NETWORK);
		//callback.SetNbStep(NB_NETWORK);

		//open a connection on the server
		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;
		msg = GetHttpConnection(SERVER_NAME, pConnection, pSession, FLAGS);
		if (msg)
		{

			for (size_t n = 0; n < NB_NETWORK&&msg; n++)
			{
				string URL = string("nwcc/yearcount?network=") + NETWORK_ID[n] + "&counttype=listwithdiscontinued&state=";

				string source;
				msg = UtilWWW::GetPageText(pConnection, URL, source, false, FLAGS);
				if (msg)
				{
					string::size_type begin = source.find("<!--GET REPORT-->");
					source = FindString(source, "<table border=1>", "</table>", begin);

					begin = 0;
					string str = FindString(source, "<tr", "</tr>", begin);//skip header

					str = FindString(source, "<tr", "</tr>", begin);//get first station
					while (begin != string::npos&&msg)
					{
						StringVector columns;

						string::size_type pos = 0;
						while (pos != string::npos)
						{
							columns.push_back(FindString(str, "<td>", "</td>", pos));
						}

						columns.pop_back();
						ASSERT(columns.size() == NB_COLUMNS);

						//ReplaceString(columns[C_SITE_NAME], "(Rev)", "");
						string::size_type pos1 = columns[C_SITE_NAME].find_last_of('('); assert(pos1 != string::npos);
						string::size_type pos2 = columns[C_SITE_NAME].find_last_of(')'); assert(pos2 != string::npos);
						assert(pos2 - pos1 - 1 > 0);
						string name = columns[C_SITE_NAME].substr(0, pos1);
						string ID = columns[C_SITE_NAME].substr(pos1 + 1, pos2 - pos1 - 1);



						double alt = Feet2Meter(ToDouble(columns[C_ELEV]));
						CLocation station(PurgeFileName(name), TrimConst(ID), ToDouble(columns[C_LATITUDE]), ToDouble(columns[C_LONGITUDE]), alt);
						station.SetSSI("Network", TrimConst(columns[C_NTWK]));
						station.SetSSI("SubDivision", columns[C_STATE]);
						station.SetSSI("County", columns[C_COUNTY]);
						station.SetSSI("Huc", columns[C_HUC]);
						//station.SetSSI("EndDate", ConvertEndDate(columns[C_END_DATE]));

						stations.push_back(station);
						str = FindString(source, "<tr", "</tr>", begin);
						msg += callback.StepIt(0);
					}

					msg += callback.StepIt();
				}
			}
		}

		callback.PopTask();

		if (!msg)
			return msg;

		callback.AddMessage(GetString(IDS_NB_STATIONS) + ToString(stations.size()));


		callback.PushTask(GetString(IDS_LOAD_STATION_LIST), stations.size());
		//callback.SetNbStep(stations.size());



		for (size_t s = 0; s < stations.size() && msg; s++)
		{
			string URL = string("reportGenerator/view_csv/customSingleStationReport/daily/" + stations[s].m_ID + ":" + stations[s].GetSSI("SubDivision") + ":" + stations[s].GetSSI("Network") + "%7Cid=%22%22%7Cname~0/0,0/stationId,actonId,latitude,longitude,elevation,inServiceDate,outServiceDate");

			string source;
			msg = UtilWWW::GetPageText(pConnection, URL, source, false, FLAGS);
			if (msg)
			{

				std::stringstream stream(source);


				string line;
				while (getline(stream, line))
				{
					if (!line.empty() && line[0] != '#')
						break;
				}

				//get data
				getline(stream, line);
				StringVector columns = Tokenize(line, ",", false);

				assert(columns.size() == NB_REPORT_COLUMNS);
				assert(columns[R_STATION_ID] == stations[s].m_ID);
				assert(fabs(Feet2Meter(ToDouble(columns[R_ELEVATION])) - stations[s].m_alt) < 0.1);


				stations[s].m_lat = ToDouble(columns[R_LATITUDE]);
				stations[s].m_lon = ToDouble(columns[R_LONGITUDE]);
				stations[s].SetSSI("ActonId", columns[C_NTWK]);
				stations[s].SetSSI("StartDate", columns[R_IN_SERVICEDATE]);
				stations[s].SetSSI("EndDate", columns[R_OUT_SERVICEDATE]);

				msg += callback.StepIt();
			}
		}

		pConnection->Close();
		pSession->Close();

		callback.PopTask();

		msg = stations.Save(GetStationListFilePath());


		return msg;
	}


	CTPeriod CUISnoTel::GetStationPeriod(const CLocation& location)
	{
		string start = location.GetSSI("StartDate");
		string end = location.GetSSI("EndDate");

		CTRef TRef1;
		CTRef TRef2;
		if (start.empty() && end.empty())
		{
			TRef1.FromFormatedString(start, "", "-", 1);
			TRef2.FromFormatedString(end, "", "-", 1);

			assert(TRef1.IsValid());
			assert(TRef2.IsValid());
		}


		return CTPeriod(TRef1, TRef2);
	}


	ERMsg CUISnoTel::Execute(CCallback& callback)
	{
		ERMsg msg;


		bool bUpdateList = as<bool>(UPDATE_STATIONLIST);
		if (!FileExists(GetStationListFilePath()))
		{
			msg = UpdateStationInformation(callback);

			if (!msg)
				return msg;
		}
		else
		{
			//callback.SkipTask(2);
		}

		m_stations.clear();
		msg = m_stations.Load(GetStationListFilePath());
		if (!msg)
			return msg;


		string workingDir = GetDir(WORKING_DIR);

		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(workingDir, 1);
		callback.AddMessage(GetString(IDS_UPDATE_FROM));
		callback.AddMessage(SERVER_NAME, 1);
		callback.AddMessage("");


		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;
		msg += GetHttpConnection(SERVER_NAME, pConnection, pSession, FLAGS);

		if (!msg)
			return msg;


		CTRef toDay = CTRef::GetCurrentTRef();
		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = lastYear - firstYear + 1;
		size_t nbDownload = 0;
		size_t nbFilesToDownload = 0;
		size_t nbFiles = 0;
		CStateSelection states(Get(STATES));
		size_t Ttype = as<size_t>(DATA_TYPE);

		callback.PushTask(GetString(IDS_GET_STATION_LIST), m_stations.size()*nbYears);
		//callback.SetNbStep(m_stations.size()*nbYears);


		//Get station
		vector<vector<bool>> bNeedDownload(m_stations.size());

		for (size_t i = 0; i != m_stations.size() && msg; i++)
		{
			bNeedDownload[i].resize(nbYears);
			for (size_t y = 0; y < nbYears&&msg; y++)
			{
				int year = firstYear + int(y);

				string state = m_stations[i].GetSSI("SubDivision");
				if (states.none() || states.at(state))
				{
					bool bSnowStation = m_stations[i].GetSSI("Network") == "SNOW";
					if ((Ttype == T_MONTHLY && bSnowStation) || (Ttype != T_MONTHLY && !bSnowStation))
					{
						bool bManualStation = m_stations[i].GetSSI("Network") == "MPRC";
						bool bIDWithAlpha = std::find_if(m_stations[i].m_ID.begin(), m_stations[i].m_ID.end(), isalpha) != m_stations[i].m_ID.end();

						if (!bManualStation || !bIDWithAlpha)
						{
							if (GetStationPeriod(m_stations[i]).IsIntersect(CTPeriod(year, 0, 0, year, 11, 30)))
							{
								nbFiles++;
								string filePath = GetOutputFilePath(year, Ttype, m_stations[i].m_ID);
								if ((Ttype == T_MONTHLY&&bSnowStation) || !FileExists(filePath) || toDay - CTRef(year, DECEMBER, LAST_DAY) < 31)
								{
									bNeedDownload[i][y] = true;
									nbFilesToDownload++;
								}
							}
						}
					}
				}

				msg += callback.StepIt();
			}
		}


		callback.AddMessage(GetString(IDS_NB_FILES_FOUND) + ToString(nbFiles));
		callback.AddMessage(GetString(IDS_NB_FILES_CLEARED) + ToString(nbFilesToDownload));
		callback.AddMessage("");

		callback.PopTask();

		callback.PushTask(GetString(IDS_UPDATE_FILE), nbFilesToDownload);
		//callback.SetNbStep(nbFilesToDownload);

		for (size_t i = 0; i != m_stations.size() && msg; i++)
		{
			for (size_t y = 0; y < nbYears&&msg; y++)
			{
				int year = firstYear + int(y);
				if (bNeedDownload[i][y])
				{
					string URL;
					if (Ttype == T_MONTHLY)
					{
						assert(m_stations[i].GetSSI("Network") == "SNOW");

						//snow monthly record
						static const char webPageDataFormat[] =
						{
							"reportGenerator/view/customSingleStationReport,metric/monthly/"
							"%s:%s:SNOW%%7Cid=%%22%%22%%7Cname/"
							"POR_BEGIN,POR_END/"
							"SNWD::collectionDate,SNWD::value,WTEQ::value"
						};

						string ID = m_stations[i].m_ID;
						string state = m_stations[i].GetSSI("SubDivision");
						URL = FormatA(webPageDataFormat, ID.c_str(), state.c_str());

					}
					else
					{
						assert(m_stations[i].GetSSI("Network") != "SNOW");
						static const char webPageDataFormat[] =
						{
							"nwcc/view?"
							"intervalType=Historic&"
							"report=ALL&"
							"timeseries=%s&"
							"format=copy&"
							"sitenum=%s&"
							"interval=YEAR&"
							"year=%d&"
							"month=CY"
						};


						string ID = m_stations[i].m_ID;
						URL = FormatA(webPageDataFormat, TEMPORAL_TYPE_NAME[Ttype], ID.c_str(), year);
					}

					string filePath = GetOutputFilePath(year, Ttype, m_stations[i].m_ID);
					CreateMultipleDir(GetPath(filePath));
					msg = UtilWWW::CopyFile(pConnection, URL, filePath, FLAGS);

					if (msg)
					{
						nbDownload++;
						msg += callback.StepIt();
					}
				}
			}
		}

		//clean connection
		pConnection->Close();
		pSession->Close();

		callback.AddMessage(GetString(IDS_NB_FILES_DOWNLOADED) + ToString(nbDownload));
		callback.PopTask();


		return msg;
	}




	ERMsg CUISnoTel::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;

		m_stations.clear();
		msg = m_stations.Load(GetStationListFilePath());
		if (!msg)
			return msg;

		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		CStateSelection states(Get(STATES));
		size_t Ttype = as<size_t>(DATA_TYPE);


		for (CLocationVector::const_iterator it = m_stations.begin(); it != m_stations.end(); it++)
		{
			string state = it->GetSSI("SubDivision");
			if (states.none() || states.at(state))
			{

				bool bSnowStation = it->GetSSI("Network") == "SNOW";
				if ((Ttype == T_MONTHLY && bSnowStation) || (Ttype != T_MONTHLY && !bSnowStation))
				{

					const CLocation& station = *it;
					//CTPeriod p = GetStationPeriod(station);

					//if (p.IsIntersect(CTPeriod(firstYear, FIRST_MONTH, FIRST_DAY, lastYear, LAST_MONTH, LAST_DAY)))
					//{
					stationList.push_back(it->m_ID);
					//}
				}
			}
		}


		return msg;
	}


	ERMsg CUISnoTel::GetWeatherStation(const std::string& ID, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;

		CLocationVector::const_iterator it = std::find_if(m_stations.begin(), m_stations.end(), FindLocationByID(ID));
		ASSERT(it != m_stations.end());

		((CLocation&)station) = *it;
		station.m_name = PurgeFileName(station.m_name);

		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = lastYear - firstYear + 1;
		CStateSelection states(Get(STATES));
		size_t Ttype = as<size_t>(DATA_TYPE);


		CTRef currentTRef = CTRef::GetCurrentTRef();

		//now extract data 
		for (size_t y = 0; y < nbYears&&msg; y++)
		{
			int year = firstYear + int(y);

			string filePath = GetOutputFilePath(year, Ttype, station.m_ID);
			if (FileExists(filePath))
			{
				CYear& dailyData = station.CreateYear(year);
				msg = ReadData(filePath, dailyData, callback);
			}
		}

		//string network = station.GetSSI("Network");
		//string country = station.GetSSI("Country");
		//string subDivisions = station.GetSSI("SubDivision");
		//
		//station.m_siteSpeceficInformation.clear();
		//station.SetSSI("Network", "SnoTel_"+ network);
		//station.SetSSI("Country", country);
		//station.SetSSI("SubDivision", subDivisions);

		station.SetSSI("Provider", "Snotel");
		


		if (station.HaveData())
		{
			msg = station.IsValid();
			ASSERT(station.IsValid());

		}

		return msg;
	}





	//***************************************************************************************************************************

	const char* CSnoTelFile::SNOTEL_VAR_NAME[NB_SNOTEL_VARIABLES] =
	{
		"Site Id", "Date", "Time",
		"TAVG", "TMAX", "TMIN", "TOBS", "PRES", "BATT", "BATV", "BATX", "BATN", "ETIB", "COND", "DPTP", "DIAG", "SRDOX", "DISO", "DISP", "DIVD", "DIV", "HFTV", "EVAP",
		"FUEL", "FMTMP", "VOLT", "TGSV", "TGSX", "TGSN", "TGSI", "JDAY", "MXPN", "MNPN", "NTRDV", "NTRDX", "NTRDN", "NTRDC", "H2OPH", "PARV", "PART", "PREC", "PRCP", "PRCPSA",
		"ETIL", "RDC", "RHUM", "RHUMV", "RHENC", "RHUMX", "RHUMN", "REST", "RESC", "SRDOO", "RVST", "SAL", "SNWD", "SNWDV", "SNWDX", "SNWDN", "SNOW", "WTEQ", "WTEQV", "WTEQX",
		"WTEQN", "SMOV", "SMOC", "SMOX", "SMON", "SMS", "SMV", "SMX", "SMN", "STV", "STX", "STN", "STO", "SRAD", "SRADV", "SRADX", "SRADN", "SRADT", "LRAD", "LRADX",
		"LRADT", "SRMV", "SRMX", "SRMN", "SRMO", "SRVO", "SRVOX", "SRVOO", "OI", "CDD", "GDD", "HDD", "TURB", "RESA", "PVPV", "SVPV", "WLEVV", "WLEVX", "WLEVN", "WLEV",
		"WTEMP", "WTAVG", "WTMAX", "WTMIN", "WELL", "WDIRV", "WDIR", "WDIRZ", "WDMVV", "WDMVX", "WDMVN", "WDMV", "WDMVT", "WSPDV", "WSPDX", "WSPDN", "WSPD"
	};

	size_t CSnoTelFile::GetVariable(const string& strType)
	{
		size_t type = UNKNOWN_POS;
		for (size_t v = 0; v < NB_SNOTEL_VARIABLES&&type == UNKNOWN_POS; v++)
		{
			if (strType == SNOTEL_VAR_NAME[v])
				type = v;
		}

		assert(type < NB_SNOTEL_VARIABLES);

		return type;
	}


	vector<size_t> CSnoTelFile::GetMembers(StringVector header)
	{
		vector<size_t> members;

		for (size_t i = 0; i < header.size(); i++)
		{
			string str = header[i].substr();
			string::size_type pos = str.find(".");
			if (pos != string::npos)
				str = header[i].substr(0, pos);

			size_t v = GetVariable(str);
			if (v < NB_SNOTEL_VARIABLES)
				members.push_back(v);
		}


		return members;
	}


	map<size_t, size_t> CSnoTelFile::GetVariablesPos(vector<size_t> members)
	{
		map<size_t, size_t> variablesPos;

		for (size_t i = 0; i < members.size(); i++)
		{
			variablesPos.insert(make_pair(members[i], i));
		}


		return variablesPos;
	}


	ERMsg CUISnoTel::ReadData(const string& filePath, CYear& data, CCallback& callback)const
	{
		ERMsg msg;


		CSnoTelFile file;
		msg = file.open(filePath);

		if (msg)
		{
			int firstYear = as<int>(FIRST_YEAR);
			int lastYear = as<int>(LAST_YEAR);

			CTPeriod validPeriod(CTRef(firstYear, FIRST_MONTH, FIRST_DAY, FIRST_HOUR, data.GetTM()), CTRef(lastYear, LAST_MONTH, LAST_DAY, LAST_HOUR, data.GetTM()));
			CWeatherAccumulator accumulator(data.GetTM());
			bool bRemovePrcp = false;

			while (!file.end())
			{
				CTRef TRef = file.GetTRef(data.GetTM());
				if (TRef.IsInit() && validPeriod.IsInside(TRef))
				{
					if (accumulator.TRefIsChanging(TRef))
					{
						data[accumulator.GetTRef()].SetData(accumulator);
					}

					double Tmin = file[V_TMIN];
					double Tmax = file[V_TMAX];
					double Tavg = file[V_TAVG];

					if (!IsMissing(Tmin) && !IsMissing(Tmax) &&
						Tmin >= -70 && Tmin <= 70 &&
						Tmax >= -70 && Tmax <= 70 &&
						Tmin <= Tmax)
					{
						accumulator.Add(TRef, H_TMIN, Tmin);
						accumulator.Add(TRef, H_TMAX, Tmax);
					}

					if (!IsMissing(Tavg) && Tavg >= -70 && Tavg <= 70)
					{
						accumulator.Add(TRef, H_TAIR, Tavg);
					}

					double prcp = file[V_PRCP];
					if (!IsMissing(prcp) && prcp < 585)
					{
						ASSERT(prcp >= 0 && prcp < 50);//inch
						accumulator.Add(TRef, H_PRCP, prcp*25.4);// inch of in --> mm
					}

					if (file.at(V_PRCP) == "   585.4" || file.at(V_PRCP) == "   654.9")
						bRemovePrcp = true;

					double Tdew = file[V_DPTP];
					if (!IsMissing(Tdew) && Tdew >= -70 && Tdew < 70)
					{
						accumulator.Add(TRef, H_TDEW, Tdew);
					}

					double Hmin = file[V_RHUMN];
					double Hmax = file[V_RHUMX];
					double Havg = file[V_RHUMV];
					double Hobs = file[V_RHUM];
					if (!IsMissing(Hmin) && !IsMissing(Hmax) &&
						Hmin > 0 && Hmax>0 && Hmin <= 100 && Hmax <= 100)
					{
						accumulator.Add(TRef, H_RELH, Hmin);
						accumulator.Add(TRef, H_RELH, Hmax);
					}

					if (!IsMissing(Havg))
					{
						ASSERT(Havg >= 0 && Havg <= 100);
						accumulator.Add(TRef, H_RELH, Havg);
					}
					
					// && file.IsHourly()
					if (data.IsHourly() && IsMissing(Hmin) && IsMissing(Hmax) && IsMissing(Havg) &&
						!IsMissing(Hobs) && Hobs > 0 && Hobs <= 100 )//hummm????
					{
						accumulator.Add(TRef, H_RELH, Hobs);
					}
					

					double ws = file[V_WSPDV];
					if (!IsMissing(ws) && ws >= 0 && ws < 120)
					{
						accumulator.Add(TRef, H_WNDS, ws * 1.609344);//mile/h --> km/h
					}

					double wd = file[V_WDIRV];
					if (!IsMissing(wd) && wd >= 0 && wd <= 360)
					{
						accumulator.Add(TRef, H_WNDD, wd);
					}

					double R = file[V_SRADV];
					if (!IsMissing(R))
					{
						ASSERT(R >= 0 && R < 1500);//(watt/m2) 
						accumulator.Add(TRef, H_SRAD, R);
					}
					else
					{
						if (!file.IsMissing(V_SRAD) ||
							!file.IsMissing(V_SRADX) ||
							!file.IsMissing(V_SRADN) ||
							!file.IsMissing(V_SRADT) ||
							!file.IsMissing(V_LRAD) ||
							!file.IsMissing(V_LRADX) ||
							!file.IsMissing(V_LRADT))
						{
							int i;
							i = 0;
						}
					}

					double Z = file[V_PRES];
					if (!IsMissing(Z))
					{
						//ASSERT(Z*33.86 >= 930 && Z*33.86 < 1080);
						//accumulator.Add(TRef, H_PRES, Z*33.86);//convert inch of mercury to hPa
						accumulator.Add(TRef, H_PRES, Z);//mbar???
					}

					double snow = file[V_SNOW];
					if (!IsMissing(snow))
					{
						ASSERT(snow >= 0 && snow < 120);//inch
						accumulator.Add(TRef, H_SNOW, snow*2.54);//inch --> cm
					}

					double sd = file[V_SNWD];
					if (!IsMissing(sd) && sd < 150)
					{
						accumulator.Add(TRef, H_SNDH, max(0.0, sd*2.54));//inch --> cm
					}

					double swe = file[V_WTEQ];
					if (!IsMissing(swe) && swe < 750)
					{
						accumulator.Add(TRef, H_SWE, max(0.0, swe*25.4));//inch --> mm
					}

					//double Ea = file[V_PVPV];
					//if (!IsMissing(Ea))
					//{
					//	ASSERT(Ea >= 0 && Ea < 300);//Pa
					//	accumulator.Add(TRef, H_EA, Ea);
					//}

					/*double Es = file[V_SVPV];
					if (!IsMissing(Es))
					{
						ASSERT(Es >= 0 && Es < 300);
						accumulator.Add(TRef, H_ES, Es);
					}*/
				}

				++file;
			}//for all line


			if (accumulator.GetTRef().IsInit())
				data[accumulator.GetTRef()].SetData(accumulator);


			if (bRemovePrcp)
				for (CTRef TRef = data.GetEntireTPeriod().Begin(); TRef <= data.GetEntireTPeriod().End(); TRef++)
					data[TRef][H_PRCP] = CStatistic();

			file.close();
		}


		return msg;
	}



	ERMsg CSnoTelFile::open(const std::string& filePath)
	{
		ERMsg msg;

		msg = m_file.open(filePath);

		if (msg)
		{
			enum { NB_HEADER_LINE = 3 };
			for (size_t i = 0; i < NB_HEADER_LINE; i++)
				getline(m_file, string());//read comment's line

			m_loop.reset(new CSVIterator(m_file));

			m_bHaveTime = m_loop->Header().Find("Time") != NOT_INIT;
			vector<size_t> m_members = GetMembers(m_loop->Header());
			if (!m_members.empty())
			{
				ASSERT(m_members.size() >= 2);
				ASSERT(m_members[0] == V_SITE_ID);
				ASSERT(m_members[1] == V_DATE);

				m_pos = GetVariablesPos(m_members);

				//test to see if the file have data
				while ((*m_loop) != CSVIterator() && (*m_loop)->empty())
					++(*m_loop);
			}
			else
			{
				msg.ajoute("Error reading input file: " + filePath);
			}
		}

		return msg;
	}

	CTRef CSnoTelFile::GetTRef(CTM TM)const
	{
		StringVector date(at(V_DATE), "-");
		StringVector time;
		ASSERT(date.size() == 3);

		int year = ToInt(date[0]);
		size_t m = ToSizeT(date[1]) - 1;
		size_t d = ToSizeT(date[2]) - 1;
		if (m_bHaveTime && at(V_TIME).empty())
		{
			time.Tokenize(at(V_TIME), ":");
			ASSERT(time.size() == 2);
		}

		size_t h = time.size() == 2 ? ToSizeT(time[0]) : 0;
		size_t mm = time.size() == 2 ? ToSizeT(time[1]) : 0;
		ASSERT(year >= 1900 && year <= 2099);
		ASSERT(m >= 0 && m < 12);
		ASSERT(d >= 0 && d < GetNbDayPerMonth(year, m));
		ASSERT(mm >= 0 && mm <= 59);

		//minute 59 reflect the chnage of the water year. don't take theses values...
		if (mm != 0)
			return CTRef();

		CTRef TRef(year, m, d, h, TM);

		//hourly data is base on the previous hour and daily data is base on the previous day, shift of -1
		TRef--;

		return TRef;
	}

}



//Liste des stations et coord
//http://www.wcc.nrcs.usda.gov/nwcc/yearcount?network=sntl&counttype=listwithdiscontinued&state=
//network: scan,sntl,sntlt,snow,mprc,other


//data quotidienne
//http://www.wcc.nrcs.usda.gov/nwcc/tabget?stationidname=49L09S
//http://www.wcc.nrcs.usda.gov/nwcc/view?intervalType=Historic&report=ALL&timeseries=Daily&format=copy&sitenum=745&interval=YEAR&year=2008&month=CY
//http://www.wcc.nrcs.usda.gov/nwcc/view?intervalType=Historic&report=ALL&timeseries=Daily&format=view&sitenum=745&interval=YEAR&year=2008&month=CY

//horaire par an
//http://www.wcc.nrcs.usda.gov/nwcc/view?intervalType=Historic&report=ALL&timeseries=Hourly&format=view&sitenum=2106&interval=YEAR&year=2014&month=CY
//http://www.wcc.nrcs.usda.gov/nwcc/view?intervalType=Historic&report=ALL&timeseries=Hourly&format=view&sitenum=2106&interval=YEAR&year=2006&month=CY
//http://www.wcc.nrcs.usda.gov/reportGenerator/view_csv/customSingleStationReport,metric/hourly/2197:CO:SCAN%7Cid=%22%22%7Cname/2015-07-01,2015-07-31/TMIN::value,TMAX::value,PRCP::value,DPTP::value,RHUM::value,WSPDV::value,WDIRV::value,SRADV::value,PRES::value,SNOW::value,SNWD::value,WTEQ::value,PVPV::value,SVPV::value
//curl "http://www.wcc.nrcs.usda.gov/nwcc/view" - H "Pragma: no-cache" - H "Origin: http://www.wcc.nrcs.usda.gov" - H "Accept-Encoding: gzip, deflate" - H "Accept-Language: fr-FR,fr;q=0.8,en-US;q=0.6,en;q=0.4" - H "Upgrade-Insecure-Requests: 1" - H "User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/46.0.2490.86 Safari/537.36" - H "Content-Type: application/x-www-form-urlencoded" - H "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8" - H "Cache-Control: no-cache" - H "Referer: http://www.wcc.nrcs.usda.gov/nwcc/site" - H "Cookie: JSESSIONID=cuEUCvMs89PiSrdmaovAf3m-; _gat=1; style=null; _ga=GA1.2.177180678.1447684933" - H "Connection: keep-alive" --data "intervalType=+View+Historic+&report=SCAN&timeseries=Hourly&format=view&sitenum=2106&interval=MONTH&year=2006&month=06&day=01&userEmail=" --compressed

//liste de toutes les stations
//http://www.wcc.nrcs.usda.gov/nwcc/sitelist.jsp
//http://www.wcc.nrcs.usda.gov/nwcc/inventory

//contact
//Maggie Dunklee
// Office: (503) 414-3049
// E-mail: maggie.dunklee@por.nrcs.usda.gov


//***********************************************************************************************************************

// monthly values
//ifStream file;
//msg = file.open(filePath);

//if (msg)
//{
//	CWeatherAccumulator stat(data.GetTM());
//	map<size_t, size_t> pos;
//	bool bHourlyData = true;


//	string line;
//	for(size_t i=0; i<2; i++)
//	{
//		getline(file, line);//read comment's line
//	}


//	for (CSVIterator loop(file); loop != CSVIterator(); ++loop)
//	{
//		if (pos.empty())
//		{
//			bHourlyData = loop.Header().Find("Time");
//			vector<size_t> members = members = GetMembers(loop.Header());
//			ASSERT(members.size() >= 2);
//			ASSERT(members[0] == V_SITE_ID);
//			ASSERT(members[1] == V_DATE);

//			pos = GetVariablesPos(members);
//		}

//		StringVector date((*loop)[C_DATE], "-"); ASSERT(date.size() == 3);
//		
//		int year = ToInt(date[0]);
//		size_t m = ToInt(date[1]) - 1;
//		size_t d = ToInt(date[2]) - 1;
//		size_t h = bHourlyData ? ToInt((*loop)[C_TIME].substr(0,2)) : 0;
//		ASSERT(year >= 1900 && year <= 2099);
//		ASSERT(m >= 0 && m<12);
//		ASSERT(d >= 0 && d<GetNbDayPerMonth(year, m));
//		CTRef TRef = bHourlyData ? CTRef(year, m, d, h) : CTRef(year, m, d);

//		//hourly data is base on the previous hour and daily data is base on the previous day, shift of -1
//		TRef--;

//		if (stat.TRefIsChanging(TRef))
//		{
//			data[stat.GetTRef()].SetData(stat);
//		}

//		if (pos.find(V_TMIN) != pos.end() && pos.find(V_TMAX) != pos.end() && 
//			!(*loop)[pos[V_TMIN]].empty() && !(*loop)[pos[V_TMAX]].empty())
//		{
//			float Tmin = ToFloat((*loop)[pos[V_TMIN]]);
//			ASSERT(Tmin >= -70 && Tmin <= 70);
//			float Tmax = ToFloat((*loop)[pos[V_TMAX]]);
//			ASSERT(Tmax >= -70 && Tmax <= 70);
//			stat.Add(TRef, H_TAIR, Tmin);
//			stat.Add(TRef, H_TAIR, Tmax);
//		}

//		if (pos.find(V_TAVG) != pos.end() &&
//			!(*loop)[pos[V_TAVG]].empty())
//		{
//			float Tavg = ToFloat((*loop)[pos[V_TAVG]]);
//			ASSERT(Tavg >= -70 && Tavg <= 70);

//			stat.Add(TRef, H_TAIR, Tavg);
//		}

//		if (pos.find(V_PRCP) != pos.end() &&
//			!(*loop)[pos[V_PRCP]].empty())
//		{
//			float prcp = ToFloat((*loop)[pos[V_PRCP]]);
//			ASSERT(prcp >= 0 && prcp<1000);

//			stat.Add(TRef, H_PRCP, prcp);
//		}

//		if (pos.find(V_DPTP) != pos.end() &&
//			!(*loop)[pos[V_DPTP]].empty())
//		{
//			float Tdew = ToFloat((*loop)[pos[V_DPTP]]);
//			ASSERT(Tdew >= -70 && Tdew<70);

//			stat.Add(TRef, H_TDEW, Tdew);
//		}

//		if (pos.find(V_TMIN) != pos.end() && pos.find(V_TMAX) != pos.end() && 
//			!(*loop)[pos[V_RHUMN]].empty() && !(*loop)[pos[V_RHUMX]].empty())
//		{
//			float Hmin = ToFloat((*loop)[V_RHUMN]);
//			ASSERT(Hmin >= 0 && Hmin <= 100);
//			float Hmax = ToFloat((*loop)[V_RHUMX]);
//			ASSERT(Hmax >= 0 && Hmax <= 100);

//			stat.Add(TRef, H_RELH, Hmin);
//			stat.Add(TRef, H_RELH, Hmax);
//		}
//		
//		if (pos.find(V_PRCP) != pos.end() && 
//				!(*loop)[V_RHUMV].empty())
//		{
//			float Hr = ToFloat((*loop)[V_RHUMV]);
//			ASSERT(Hr >= 0 && Hr <= 100);

//			stat.Add(TRef, H_RELH, Hr);
//		}
//		
//		if (pos.find(V_RHUM) != pos.end() &&
//			!(*loop)[pos[V_RHUM]].empty())
//		{
//			float Hr = ToFloat((*loop)[pos[V_RHUM]]);
//			ASSERT(Hr >= 0 && Hr <= 100);

//			stat.Add(TRef, H_RELH, Hr);
//		}

//		if (pos.find(V_PRCP) != pos.end() && 
//			!(*loop)[V_WSPDV].empty())
//		{
//			float ws = ToFloat((*loop)[V_WSPDV]);
//			ASSERT(ws >= 0 && ws<120);//km/h

//			stat.Add(TRef, H_WNDS, ws);
//		}
//		

//		if (pos.find(V_PRCP) != pos.end() && 
//			!(*loop)[V_WDIRV].empty())
//		{
//			float wd = ToFloat((*loop)[V_WDIRV]);
//			ASSERT(wd >= 0 && wd<360);

//			stat.Add(TRef, H_WNDD, wd);
//		}

//		if (pos.find(V_PRCP) != pos.end() && 
//			!(*loop)[V_SRADV].empty())
//		{
//			float R = ToFloat((*loop)[V_SRADV]);//(watt/m2) --> (MJ/m2)
//			ASSERT(R >= 0 && R<1500);

//			if (m_type == T_HOURLY)
//				R *= 60 * 60 / 1000000;
//			else 
//				R *= 60 * 60 * 24/1000000;

//			stat.Add(TRef, H_SRAD, R);
//		}
//		else if (!(*loop)[V_SRAD].empty() || !(*loop)[V_SRADX].empty() || !(*loop)[V_SRADN].empty() || !(*loop)[V_SRADT].empty() || !(*loop)[V_LRAD].empty() || !(*loop)[V_LRADX].empty() || !(*loop)[V_LRADT].empty())
//		{
//			int i;
//			i = 0;
//		}

//		if (pos.find(V_PRCP) != pos.end() && 
//			!(*loop)[V_PRES].empty())
//		{
//			float P = ToFloat((*loop)[V_PRES]);
//			ASSERT(P >= 930 && P<1080);

//			stat.Add(TRef, H_PRES, P);
//		}

//		if (pos.find(V_PRCP) != pos.end() && 
//			!(*loop)[V_SNOW].empty())
//		{
//			float P = ToFloat((*loop)[V_SNOW]);
//			ASSERT(P >= 0 && P<120);

//			stat.Add(TRef, H_SNOW, P);
//		}

//		if (pos.find(V_PRCP) != pos.end() && 
//			!(*loop)[V_SNWD].empty())
//		{
//			float sd = ToFloat((*loop)[V_SNWD]);
//			ASSERT(sd >= 0 && sd<300);

//			stat.Add(TRef, H_SNDH, sd);
//		}

//		if (pos.find(V_PRCP) != pos.end() && 
//			!(*loop)[V_WTEQ].empty())
//		{
//			float sd = ToFloat((*loop)[V_WTEQ]);
//			ASSERT(sd >= 0 && sd<3000);

//			stat.Add(TRef, H_SWE, sd);
//		}

//		if (pos.find(V_PRCP) != pos.end() && 
//			!(*loop)[V_PVPV].empty())
//		{
//			float Ea = ToFloat((*loop)[V_PVPV]) * 1000;
//			ASSERT(Ea >= 0 && Ea<300);

//			stat.Add(TRef, H_EA, Ea);
//		}

//		if (pos.find(V_PRCP) != pos.end() && 
//			!(*loop)[V_SVPV].empty())
//		{
//			float Es = ToFloat((*loop)[V_SVPV])*1000;
//			ASSERT(Es >= 0 && Es<300);

//			stat.Add(TRef, H_ES, Es);
//		}
//	}//for all line


//	if (stat.GetTRef().IsInit())
//		data[stat.GetTRef()].SetData(stat);
//	
//}


//
//void CUISnoTel::QualityControl(CYear& data)
//{
//	int nbZeroMin = 0;
//	float lastTmin = -999;
//	int nbZeroMax = 0;
//	float lastTmax = -999;
//	
//	//int nbZeroMinMax = 0;
//	//float lastTminmax = -999;
//	CTPeriod p = data.GetEntireTPeriod(CTM(CTM::DAILY));
//
//	for(CTRef d=p.Begin(); d<=p.End(); d++)
//	{
//		
//		const CStatistic& stat = data[d.GetMonth()][d.GetDay()].GetStat(H_TAIR);
//
//		if( !stat.empty() &&
//			fabs( stat[LOWEST] - lastTmin) < 0.15)
//		{
//			nbZeroMin++;
//		}
//		else nbZeroMin = 0;
//		lastTmin = stat[LOWEST];
//
//		if( !stat.empty() &&
//			fabs( stat[HIGHEST] - lastTmax) < 0.15)
//		{
//			nbZeroMax++;
//		}
//		else nbZeroMax = 0;
//		lastTmax = stat[HIGHEST];
//
//		bool bMin = nbZeroMin == 2;
//		bool bMax = nbZeroMax == 2;
//		if(  bMin||bMax)
//		{
//			//remove these data
//			for(CTRef dd=d-2; dd<=p.End(); dd++)
//			{
//				const CStatistic& stat = data[dd.GetMonth()][dd.GetDay()].GetStat(H_TAIR);
//				float T = bMin?(stat[LOWEST]-lastTmin):(stat[HIGHEST]-lastTmax);
//				if( fabs( T ) < 0.15 )
//				{
//					data[dd.GetMonth()][dd.GetDay()].SetStat(H_TAIR, CStatistic());
//				}
//				else break;
//			}
//		}
//	}
//
//	{
//		int nbZeroMin = 0;
//		float lastTmin = -999;
//		int nbZeroMax = 0;
//		float lastTmax = -999;
//	
//		for(CTRef d=p.Begin(); d<=p.End(); d++)
//		//for(size_t m=0; m<data.size(); m++)
//			//for(size_t d=0; d<data[m].size(); d++)
//		{
//		
//			const CStatistic& stat = data[d.GetMonth()][d.GetDay()].GetStat(H_TAIR);
//
//			if( !stat.empty() &&
//				fabs( stat[LOWEST] - lastTmin) < 1.0)
//			{
//				nbZeroMin++;
//			}
//			else nbZeroMin = 0;
//			lastTmin = stat[LOWEST];
//
//			if( !stat.empty() &&
//				fabs( stat[HIGHEST] - lastTmax) < 1.0)
//			{
//				nbZeroMax++;
//			}
//			else nbZeroMax = 0;
//			lastTmax = stat[HIGHEST];
//
//			bool bMin = nbZeroMin == 8;
//			bool bMax = nbZeroMax == 8;
//			if(  bMin||bMax)
//			{
//				//remove these data
//				for(CTRef dd=d-8; dd<=p.End(); dd++)
//				{
//					const CStatistic& stat = data[dd.GetMonth()][dd.GetDay()].GetStat(H_TAIR);
//					float T = bMin?(stat[LOWEST]-lastTmin):(stat[HIGHEST]-lastTmax);
//					if( fabs( T ) < 1 )
//					{
//						data[dd.GetMonth()][dd.GetDay()].SetStat(H_TAIR, CStatistic());
//					}
//					else break;
//				}
//			}
//		}
//	}
//}
//
//
//size_t CUISnoTel::GetDataType(const string& strType)
//{
//	const char* DATA_TYPE[NB_TYPE] = {"date", "pill", "prec", "tmax", "tmin", "tavg", "srad", "radn", "prcp", "rhum", "wdir", "fuel"};
//
//	size_t type=UNKNOWN_POS;
//	for(size_t i=0; i<NB_TYPE; i++)
//	{
//		if( strType == DATA_TYPE[i])
//		{
//			type = i;
//			break;
//		}
//	}
//
//	ASSERT(type < NB_TYPE);
//	return type;
//}
//
//array<size_t, CUISnoTel::NB_TYPE> CUISnoTel::GetMembers(StringVector header)
//{
//	array<size_t, NB_TYPE> members;
//	
//	for (size_t i = 0; i < header.size(); i++)
//	{
//		size_t pos = GetDataType(header[i]);
//		members[pos] = i;
//	}
//
//	ASSERT(members[C_DATE] != UNKNOWN_POS);
//
//	return members;
//}
//
//ERMsg CUISnoTel::ReadDataFile(const string& filePath, int currentYear, CYear& data)const
//{
//	ERMsg msg;
//
//	ifStream file;
//	msg = file.open(filePath);
//	if(msg )
//	{
//		string line;
//
//		CTRef today = CTRef::GetCurrentTRef(CTRef::DAILY);
//	//	//int curYear = GetCurrentYear();
//	//	//int curDay = GetCurrentJulianDay()-1;
//	
//		//vector<int> typeArray;
//	//	int pos = line.Find("date");
//	//	if( pos == -1)
//	//		return msg;
//
//	//	string elem = line.Tokenize( " \t", pos);ASSERT(elem == "date");
//	//	while( pos >= 0)
//		
//		array<size_t, NB_TYPE> members;
//		size_t i = 0;
//		for (CSVIterator loop(file, " \t"); loop != CSVIterator(); ++loop, i++)
//		{
//			if (members.empty())
//			{
//				members = GetMembers((StringVector&)loop.Header());
//				if (members[C_DATE] == UNKNOWN_POS)
//					return msg;
//
//				ASSERT(members[C_DATE] == 0);
//			}
//
//
//			int day = ToInt((*loop)[members[C_DATE]].substr(2, 2)) - 1;
//			ASSERT(day>=0 && day<31);
//			int month = ToInt((*loop)[members[C_DATE]].substr(0, 2)) - 1;
//			ASSERT( month>=0 && month<12);
//			int year = ToInt((*loop)[members[C_DATE]].substr(4, 2));
//			year = year<20?2000+year:1900+year;
//			ASSERT( year >= 1900 && year <=2100);
//
//	//		//int d = GetJDay(day, month, year)-1;
//			CTRef Tref(year, month, day);
//
//	//		//don't take data if it's the current day because, data is incomplet
//			if( Tref == today)
//				continue;
//
//			if( year != currentYear)
//				continue;
//	
//			CTM TM(CTM::DAILY);
//			CWeatherAccumulator stat(TM);
//			
//			if (members[C_TMIN] != UNKNOWN_POS && !(*loop)[members[C_TMIN]].empty() &&
//				members[C_TMAX] != UNKNOWN_POS && !(*loop)[members[C_TMAX]].empty())
//			{
//				double Tmin = ToDouble((*loop)[members[C_TMIN]]);
//				double Tmax = ToDouble((*loop)[members[C_TMAX]]);
//
//				if( !(Tmin==0 && Tmax==0) && (Tmin < Tmax) )
//				{
//					int limitTmin1 = (month==1 || month==2)?-45:-30;
//					int limitTmax1 = (month==1 || month==2)?-30:-20;
//					int limitTmin2 = (month==7 || month==8)?35:30;
//					int limitTmax2 = (month==7 || month==8)?40:35;
//				
//					if( Tmin > limitTmin1 && Tmin < limitTmin2 &&
//						Tmax > limitTmax1 && Tmax < limitTmax2)
//					{
//						//temperature in °C or in °F???
//						//in documention, it's indicate in °C
//						//but seem to be in °F
//						//data[d][DAILY_DATA::TMIN] = Tmin;//(Tmin-32)*5/9;
//						//data[d][DAILY_DATA::TMAX] = Tmax;//(Tmax-32)*5/9;
//						//stat.Add( Tref, H_TAIR, Tmin );
//						//stat.Add( Tref, H_TAIR, Tmax );
//						data[year][month][day][H_TAIR] = (Tmin+Tmax)/2;
//						data[year][month][day][H_TRNG] = Tmax - Tmin;
//					}
//				}
//			}
//
//			if (members[C_PRCP] != UNKNOWN_POS && !(*loop)[members[C_PRCP]].empty() )
//			{
//				double ppt = ToDouble((*loop)[members[C_PRCP]]);
//				if( ppt >= 0)
//				{
//					//stat.Add( Tref, H_PRCP, ppt*25.4 );
//					data[year][month][day][H_PRCP] = ppt;
//				}
//			}
//
//			
//		}
//
//		file.close();
//	}
//	
//	return msg;
//}
//
//

namespace WBSF
{


	//http://www.usask.ca/ip3/basins1/data/IP3DataList.htm
	void ConvertCanadianRockiesHydrologicalObservatory()
	{
		string path = "U:\\BioSIM_Weather\\Canadian Rockies Hydrological Observatory\\Data\\*.obs";
		StringVector list = GetFilesList(path);

		for (size_t i = 0; i < list.size(); i++)
		{
			ifStream file;
			if (file.open(list[i]))
			{
				string header = "YEAR,MONTH,DAY,HOUR";

				string line;
				std::getline(file, line);
				line = FindString(line, "from", "station");
				//string::size_type pos = line.find("from"); 
				StringVector names(line, ",");
				CWeatherStationVector station(names.size());



				while (std::getline(file, line) && line[0] != '#')
				{
					if (line[0] != '$')
					{
						string::size_type pos = line.find(' ');
						string var = line.substr(0, pos);
						size_t nbStation = ToSizeT(line.substr(pos + 1, 1));
						assert(station.size() == nbStation);

						if (var == "t")
							header += ",TAIR";
						else if (var == "rh")
							header += ",RELH";
						else if (var == "u")
							header += ",WND2";
						else if (var == "p")
							header += ",PRCP";
						else if (var == "Qsi")
							header += ",SRAD";
						else if (var == "ea")
							header += ",EA";
						else
						{
							int i;
							i = 0;
							header += ",SKIP";
						}
					}
				}

				CWeatherFormat format(header.c_str());
				for (size_t s = 0; s != station.size(); s++)
				{
					station[s].SetHourly(true);
					station[s].SetFormat(format);
				}



				for (CSVIterator loop(file, "\t", false); loop != CSVIterator(); ++loop)
				{
					size_t c = 0;

					double dateTime = ToDouble((*loop)[c++]);
					size_t nbHours = Round(dateTime * 24) + 7 * 24 - 1;

					CTRef TRef = CTRef(1900, FIRST_MONTH, FIRST_DAY, FIRST_HOUR) + nbHours;
					assert(TRef.GetYear() >= 1993 && TRef.GetYear() <= 2012);

					for (size_t f = 0; f != format.size(); f++)
					{
						if (format[f] != H_SKIP)
						{
							for (size_t s = 0; s != station.size(); s++)
							{
								double value = ToDouble((*loop)[c++]);
								if (format[f].m_var == H_WND2)
									value *= 3600 / 1000;

								station[s][TRef].SetStat(format[f].m_var, value);//semble ne pas y avoir de valeurs manquantes
							}
						}
					}
				}

				file.close();

				//SaveData
				for (size_t s = 0; s != station.size(); s++)
					station[s].SaveData("U:\\BioSIM_Weather\\Canadian Rockies Hydrological Observatory\\Hourly\\CanadianRockiesHydrologicalObservatory\\" + names[s] + ".csv");
			}
		}

	}
}