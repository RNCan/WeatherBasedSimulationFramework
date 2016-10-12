#include "StdAfx.h"
#include "UINEWA.h"


#include "Basic/HourlyDatabase.h"
#include "Basic/CSV.h"
#include "Basic/FileStamp.h"
#include "UI/Common/SYShowMessage.h"
#include "TaskFactory.h"
#include "StateSelection.h"

#include "../Resource.h"
#include "WeatherBasedSimulationString.h"

//http://www.ncdc.noaa.gov/qclcd/QCLCD?prior=N

using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace UtilWWW;

namespace WBSF
{



	//*********************************************************************
	static const DWORD FLAGS = INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_PRAGMA_NOCACHE;

	//*********************************************************************
	const char* CUINEWA::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "DataType", "FirstYear", "LastYear", "states", "UpdateUntil", "UpdateStationList" };
	const size_t CUINEWA::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_COMBO_INDEX, T_STRING, T_STRING, T_STRING_SELECT, T_STRING, T_BOOL };
	const UINT CUINEWA::ATTRIBUTE_TITLE_ID = IDS_UPDATER_NEWA_P;
	const UINT CUINEWA::DESCRIPTION_TITLE_ID = ID_TASK_NEWA;

	const char* CUINEWA::CLASS_NAME(){ static const char* THE_CLASS_NAME = "NEWA";  return THE_CLASS_NAME; }
	CTaskBase::TType CUINEWA::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUINEWA::CLASS_NAME(), (createF)CUINEWA::create);

	const char* CUINEWA::SERVER_NAME1 = "newa.cornell.edu";
	const char* CUINEWA::SERVER_NAME2 = "newa.nrcc.cornell.edu";
	

	
	CUINEWA::CUINEWA(void)
	{}

	CUINEWA::~CUINEWA(void)
	{}

	bool CUINEWA::IsInclude(size_t state)
	{
		static const size_t NEWA_STATES[NB_NEWA_STATES] =
		{
			CStateSelection::$AL,
			CStateSelection::$CT,
			CStateSelection::$DE,
			CStateSelection::$IA,
			CStateSelection::$IL,
			CStateSelection::$MD,
			CStateSelection::$MA,
			CStateSelection::$MN,
			CStateSelection::$MO,
			CStateSelection::$NE,
			CStateSelection::$NH,
			CStateSelection::$NJ,			CStateSelection::$NY,			CStateSelection::$NC,			CStateSelection::$PA,			CStateSelection::$SC,			CStateSelection::$SD,			CStateSelection::$VA,			CStateSelection::$VT,			CStateSelection::$WV,			CStateSelection::$WI,		};

 
		bool bInclude = false;
		for (size_t i = 0; i < NB_NEWA_STATES&&!bInclude; i++)
			bInclude = state == NEWA_STATES[i];

		return bInclude;
	}

	std::string CUINEWA::GetStatesPossibleValue()
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

	std::string CUINEWA::Option(size_t i)const
	{
		string str;
		switch (i)
		{
		case DATA_TYPE:	str = GetString(IDS_STR_DATA_TYPE); break;
		case STATES:	str = GetStatesPossibleValue(); break;
		};
		return str;
	}



	std::string CUINEWA::Default(size_t i)const
	{
		string str;

		switch (i) 
		{
		case WORKING_DIR: str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()) + "NEWA\\"; break;
		case DATA_TYPE: str = "0"; break;
		case FIRST_YEAR:
		case LAST_YEAR:	str = ToString(CTRef::GetCurrentTRef().GetYear()); break;
		case UPDATE_UNTIL: str = "2"; break;
		};

		return str;
	}

	

	ERMsg CUINEWA::DownloadStationList(CLocationVector& stationList, CCallback& callback)const
	{
		ERMsg msg; 


		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;
		
		msg = GetHttpConnection(SERVER_NAME1, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS);
		if (!msg)
			return msg;
		
		string source;
		msg = GetPageText(pConnection, "index.php?page=station-pages", source);
		if (msg)
		{
			try
			{
				size_t pos = source.find(".evenRow"); 
				if (pos != string::npos)
					pos = source.find("<table", pos);
				if (pos != string::npos)
					pos = source.find("<table", pos); 

				if (pos != string::npos)
				{
					while (pos != string::npos)
					{
						string tmp = FindString(source, "<td>", "</td>", pos);
						if (!tmp.empty())
						{
							string line = FindString(tmp, "<a href=\"", "</a>", pos);
							string ID = TrimConst(FindString(line, "&WeatherStation=", "\">"));
							size_t pos1 = line.find(">");
							size_t pos2 = line.find(",");

							
							ASSERT(pos1 != string::npos);
							ASSERT(pos2 != string::npos);
							

							string URL = TrimConst(line.substr(0, pos1));
							string name = TrimConst(line.substr(pos1 + 1, pos2 - pos1));
							string state = TrimConst(line.substr(pos2+1));

							size_t s = CStateSelection::GetState(state);
							ASSERT(IsInclude(s));
							
							
							string stationPage;
							msg = GetPageText(pConnection, URL, stationPage);
							if (msg)
							{
								//Lat/Lon: 41.81/-74.25<br/>             Elevation: 386 ft.
								string lat_lon = FindString(tmp, "Lat/Lon:", "<br");
								string elev = FindString(tmp, "Elevation:", "ft.");

								StringVector LatLonV(lat_lon, "/");
								ASSERT(LatLonV.size() == 2);

								
								CLocation location(name, ID, ToDouble(LatLonV[0]), ToDouble(LatLonV[1]), WBSF::Feet2Meter(ToDouble(elev)));
								location.SetSSI("State", state);
								stationList.push_back(location);

							}
							

						}
						
						
					}//for all stations
				}
			}
			catch (const zen::XmlParsingError& e)
			{
				// handle error
				msg.ajoute("Error parsing XML file: col=" + ToString(e.col) + ", row=" + ToString(e.row));
			}

		}//if msg

		pConnection->Close();
		pSession->Close();

		callback.AddMessage(GetString(IDS_NB_STATIONS) + ToString(stationList.size()));


		return msg;
	}

	ERMsg CUINEWA::Execute(CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);

		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(workingDir, 1);
		callback.AddMessage(GetString(IDS_UPDATE_FROM));
		callback.AddMessage(SERVER_NAME2, 1);
		callback.AddMessage("");
		

		bool bUpdate = as<bool>(UNPDATE_STATION_LIST);

		if (FileExists(GetStationListFilePath()) && !bUpdate)
		{
			msg = m_stations.Load(GetStationListFilePath());
		}
		else
		{
			CreateMultipleDir(GetPath(GetStationListFilePath()));
			msg = DownloadStationList(m_stations, callback);

			if (msg)
				msg = m_stations.Save(GetStationListFilePath());
		}

		if (!msg)
			return msg;


		
		msg = DownloadStation(callback);
		
		return msg;
	}



	ERMsg CUINEWA::DownloadStation(CCallback& callback)
	{
		ERMsg msg;

		

		CTRef today = CTRef::GetCurrentTRef();
		string workingDir = GetDir(WORKING_DIR);
		int nbFilesToDownload = 0;
		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = lastYear - firstYear + 1;
		callback.PushTask("Clear stations list...", m_stations.size()*nbYears * 12);
		int nbDays = as<int>(UPDATE_UNTIL);
		
		vector<vector<array<bool, 12>>> bNeedDownload(m_stations.size());
		for (size_t i = 0; i < m_stations.size() && msg; i++)
		{
			bNeedDownload[i].resize(nbYears);
			for (size_t y = 0; y < nbYears&&msg; y++)
			{
				int year = firstYear + int(y);

				size_t nbm = year == today.GetYear() ? today.GetMonth()+1 : 12;
				for (size_t m = 0; m < nbm && msg; m++)
				{
					string filePath = GetOutputFilePath(year, m, m_stations[i].m_ID);
					CTimeRef TRef1(GetFileStamp(filePath));
					CTRef TRef2(year, m, LAST_DAY);

					bNeedDownload[i][y][m] = !TRef1.IsInit() || TRef1 - TRef2 < nbDays; //let nbDays to update the data if it's not the current month
					nbFilesToDownload += bNeedDownload[i][y][m] ? 1 : 0;

					msg += callback.StepIt();
				}
			}
		}

		callback.PopTask();

		

		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;
		msg = GetHttpConnection(SERVER_NAME2, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS);

		if (!msg)
			return msg;

		pSession->SetOption(INTERNET_OPTION_RECEIVE_TIMEOUT, 15000);


		int nbDownload = 0;
		int currentNbDownload = 0;

		callback.PushTask("Download NEWA data (" + ToString(nbFilesToDownload) + " files)", nbFilesToDownload);
		//msg += DownloadMonth(pConnection, 2016, AUGUST, "2154", "c:/tmp/text.csv", callback);
		//return msg;

		for (size_t i = 0; i < m_stations.size() && msg; i++)
		{
			for (size_t y = 0; y < nbYears&&msg; y++)
			{
				int year = firstYear + int(y);
				for (size_t m = 0; m < 12 && msg; m++)
				{
					if (bNeedDownload[i][y][m])
					{
						string filePath = GetOutputFilePath(year, m, m_stations[i].m_ID);
						CreateMultipleDir(GetPath(filePath));

						msg += DownloadMonth(pConnection, year, m, m_stations[i].m_ID, filePath, callback);
						if (msg || callback.GetUserCancel())
						{
							nbDownload++;
							currentNbDownload++;
							msg += callback.StepIt();
						}
						else
						{
							pConnection->Close();
							pSession->Close();

							//wait 5 seconds 
							callback.PushTask("Waiting 5 seconds...", 50);
							for (size_t s = 0; s < 50 && msg; s++)
							{
								Sleep(100);
								msg += callback.StepIt();
							}
							callback.PopTask();

							msg = GetHttpConnection(SERVER_NAME2, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS);
							currentNbDownload = 0;
						}//if msg
					}//if need download
				}//for all months
			}//for all years
		}//for all station


		//clean connection
		pConnection->Close();
		pConnection.release();
		pSession->Close();
		pSession.release();


		callback.AddMessage(GetString(IDS_NB_FILES_DOWNLOADED) + ToString(nbDownload));
		callback.PopTask();

		return msg;
	}


	/*size_t GetHour(const string& time)
	{
		size_t h = 0;

		StringVector v(time, ", :");
		ASSERT(v.size() == 8);
		if (v.size() == 8)
			h = WBSF::as<size_t>(v[4]);
	
		return h;
	}

	CTRef GetTRef(string str_date, CTM TM)
	{
		CTRef TRef;

		StringVector tmp(str_date, ", :");
		ASSERT(tmp.size() == 8);
		size_t d = ToSizeT(tmp[1]) - 1;
		size_t m = WBSF::GetMonthIndex(tmp[2].c_str());
		int year = ToInt(tmp[3]);
		size_t h = ToSizeT(tmp[4]);

		return CTRef(year, m, d, h, TM);
	}
*/
	ERMsg CUINEWA::DownloadMonth(CHttpConnectionPtr& pConnection, int year, size_t m, const string& ID, const string& filePath, CCallback& callback)
	{
		ERMsg msg;

		size_t type = as <size_t>(DATA_TYPE);
		CTRef TRef = CTRef::GetCurrentTRef();
		
		//string output_text;
		//std::stringstream stream;
		//if (type == HOURLY_WEATHER)
			//stream << "Year,Month,Day,Hour,Var,Value\r\n";
		//else
			//stream << "Year,Month,Day,Var,Value\r\n";
		
	
		bool bFind = false; 
		callback.PushTask("Update " + filePath, GetNbDayPerMonth(year, m));
		size_t nbDays = (TRef.GetYear() == year&&TRef.GetMonth() == m) ? TRef.GetDay() + 1 : GetNbDayPerMonth(year, m);
		for (size_t d = 0; d < nbDays && msg; d++)
		{
			//Interface attribute index to attribute index
			//kged
			static const char PageDataFormat[] = "newaLister/%s/%s/%d/%d";

			string pageURL = FormatA(PageDataFormat, type == HOURLY_WEATHER ? "hly" : "dly", ID.c_str(), year, m + 1);

			string source;
			msg = GetPageText(pConnection, pageURL, source, false, FLAGS);

			if (!source.empty() && source.find("Unexpected error") == string::npos )
			{
				try
				{
					size_t begin = source.find("<table");
					size_t end = source.find("</table>");
					string tmp = source.substr(begin, end - begin);
					ReplaceString(tmp, "<br>", "|");

					//WBSF::ReplaceString(source, "'", " ");
					zen::XmlDoc doc = zen::parse(tmp);

					//string xml_name = (type == HOURLY_WEATHER) ? "element_value" : "aggregation_value";
					zen::XmlIn xml_in(doc.root());
					zen::XmlIn header = xml_in["thead"]["tr"];
					
					StringVector var_names;
					for (zen::XmlIn child = header["th"]; child&&msg; child.next())
					{
						string tmp;
						if (child.get()->getValue(tmp))
						{
							
							StringVector elem(tmp, "|");
							var_names.push_back(elem[0]);
							//ASSERT(elem.size());
						}
					}

					StringVector var_values;
					zen::XmlIn data = xml_in["tbody"];
					for (zen::XmlIn child = data["td"]; child&&msg; child.next())
					{
						string var_str;
						if (child.get()->getValue(var_str))
						{
							var_values.push_back(var_str);
						}//for all record of the day
					}


					ASSERT(var_names.size() == var_values.size());
				}//try
				catch (const zen::XmlParsingError& e)
				{
					// handle error
					msg.ajoute("Error parsing XML file: col=" + ToString(e.col) + ", row=" + ToString(e.row));
				}
			}//if is valid

			msg += callback.StepIt();
		}//for all day

		
		if (msg /*&& !output_text.empty()*/)//always save the file to avoid to download it
		{
			ofStream  file;
			msg = file.open(filePath, ios_base::out | ios_base::binary);
			if (msg)
			{
				file.close();
			}//if msg
			
		}//if have data

		callback.PopTask();

		return msg;
	}
	
	std::string CUINEWA::GetStationListFilePath()const
	{
		return (std::string)GetDir(WORKING_DIR) + "StationsList.csv";
	}


	string CUINEWA::GetOutputFilePath(int year, size_t m, const string& ID)const
	{
		size_t type = as<size_t>(DATA_TYPE);
		return GetDir(WORKING_DIR) + (type == HOURLY_WEATHER ? "Hourly" : "Daily") + "\\" + ToString(year) + "\\" + FormatA("%02d", m + 1) + "\\" + ID + ".csv.gz";
	}


	ERMsg CUINEWA::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;

		if (m_stations.empty())
			msg = m_stations.Load(GetStationListFilePath());

		if (msg)
			msg += m_stations.IsValid();

		if (msg)
		{
			for (size_t i = 0; i < m_stations.size(); i++)
				stationList.push_back(m_stations[i].m_ID);
		}

		return msg;
	}
	
	
	ERMsg CUINEWA::GetWeatherStation(const std::string& ID, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;

		size_t type = as <size_t>(DATA_TYPE);
		if ( TM.Type() == CTM::DAILY && type != DAILY_WEATHER)
		{
			msg.ajoute("Daily"); 
			return msg;
		}

		size_t pos = m_stations.FindByID(ID);
		if (pos == NOT_INIT)
		{
			msg.ajoute(FormatMsg(IDS_NO_STATION_INFORMATION, ID));
			return msg;
		}


		((CLocation&)station) = m_stations[pos];

		station.m_name = WBSF::PurgeFileName(station.m_name);
		//station.m_ID;// += "H";//add a "H" for hourly data

		for (SiteSpeceficInformationMap::iterator it = station.m_siteSpeceficInformation.begin(); it != station.m_siteSpeceficInformation.end(); it++)
		{
			WBSF::ReplaceString(it->second.first, ",", " ");
			WBSF::ReplaceString(it->second.first, ";", " ");
			WBSF::ReplaceString(it->second.first, "|", " ");
			WBSF::ReplaceString(it->second.first, "\t", " ");
			WBSF::ReplaceString(it->second.first, "\"", "'");
		}
			


		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = size_t(lastYear - firstYear + 1);
		station.CreateYears(firstYear, nbYears);
		station.SetHourly(TM.Type()==CTM::HOURLY);

		//now extact data
		CWeatherAccumulator accumulator(TM);


		//now extract data 
		for (size_t y = 0; y < nbYears&&msg; y++)
		{
			int year = firstYear + int(y);

			for (size_t m = 0; m < 12 && msg; m++)
			{
				string filePath = GetOutputFilePath(year, m, ID);
				if (FileExists(filePath))
				{
					msg = ReadDataFile(filePath, station, accumulator);
					msg += callback.StepIt(0);
				}
			}
		}

		/*if (accumulator.GetTRef().IsInit())
		{
			CTPeriod period(CTRef(firstYear, 0, 0, 0, TM), CTRef(lastYear, LAST_MONTH, LAST_DAY, LAST_HOUR, TM));
			if (period.IsInside(accumulator.GetTRef()))
				station[accumulator.GetTRef()].SetData(accumulator);
		}*/

		station.CleanUnusedVariable("TN T TX P TD H WS WD W2 R SD");

		if (msg)
		{
			//verify station is valid
			if (station.HaveData())
			{
				msg = station.IsValid();
			}
		}

		return msg;
	}

	

	ERMsg CUINEWA::ReadDataFile(const string& filePath, CWeatherStation& station, CWeatherAccumulator& accumulator)
	{
		ERMsg msg;
		size_t type = as <size_t>(DATA_TYPE);

		ifStream  file;

		msg = file.open(filePath, ios_base::in | ios_base::binary);
		if (msg)
		{
			string line;
			std::getline(file, line);
			while (std::getline(file, line) && msg)
			{
				line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
				StringVector columns(line, ",");

				size_t nbColumns = (type == HOURLY_WEATHER) ? 7 : 13;
				ASSERT(columns.size() == nbColumns);
				if (columns.size() == nbColumns)
				{
					size_t c = 0;
					int year = ToInt(columns[c++]);
					size_t m = ToInt(columns[c++]) - 1;
					size_t d = ToInt(columns[c++]) - 1;
					size_t h = 0;
					if (type == HOURLY_WEATHER)
						h = ToInt(columns[c++]);

					ASSERT(m >= 0 && m < 12);
					ASSERT(d >= 0 && d < GetNbDayPerMonth(year, m));

					CTRef TRef(year, m, d, h, type == HOURLY_WEATHER ? CTM::HOURLY : CTM::DAILY);
					ASSERT(TRef.IsValid());
					if (TRef.IsValid())//some have invalid TRef
					{
						string var_str = columns[c++];

						TVarH var = H_SKIP;
						if (var_str == "AT" || var_str == "ATA")
							var = H_TAIR2;
						else if (var_str == "TX")
							var = H_TMAX2;
						else if (var_str == "TN")
							var = H_TMIN2;
						else if (var_str == "PR")
							var = H_PRCP;
						else if (var_str == "HU" || var_str == "HUA")
							var = H_RELH;
						else if (var_str == "WS" || var_str == "WSA")
							var = H_WNDS;
						else if (var_str == "WD" || var_str == "WDA")
							var = H_WNDD;
						else if (var_str == "US")
							var = H_WND2;
						else if (var_str == "IR")
							var = H_SRAD2;
						/*else if (var_str == "PC")
							var = H_ADD1;
							else if (var_str == "P1")
							var = H_ADD2;*/


						if (var != H_SKIP)
						{
							string str = columns[c++];
							float value = value = WBSF::as<float>(str);
							if (var == H_SNDH)
								value /= 10;

							if (type == HOURLY_WEATHER)
							{
								//if (accumulator.TRefIsChanging(TRef))
								//station[accumulator.GetTRef()].SetData(accumulator);

								//accumulator.Add(TRef, var, value);

								station[TRef].SetStat(var, value);
								if (type == HOURLY_WEATHER && var == H_RELH && station[TRef][H_TAIR2])
									station[TRef].SetStat(H_TDEW, Hr2Td(station[TRef][H_TAIR2], value));

							}
							else
							{
								if (var == H_SRAD2 && type == DAILY_WEATHER)
									value *= 1000000.0f / (3600 * 24);//convert MJ/m² --> W/m²

								station[TRef].SetStat(var, value);

								string str_min = columns[c++];
								string str_max = columns[c++];

								if (var == H_TAIR2)
								{
									float Tmin = WBSF::as<float>(str_min);
									float Tmax = WBSF::as<float>(str_max);
									if (Tmin > -999 && Tmax > -999)
									{
										ASSERT(Tmin >= -70 && Tmin <= 70);
										ASSERT(Tmax >= -70 && Tmax <= 70);
										ASSERT(Tmin <= Tmax);

										station[TRef].SetStat(H_TMIN2, Tmin);
										station[TRef].SetStat(H_TMAX2, Tmax);

									}
								}
							}//if daily
						}//if good vars
					}//TRef is valid
				}//good number of column
			}//for all lines
		}//if msg


		return msg;
	}






}