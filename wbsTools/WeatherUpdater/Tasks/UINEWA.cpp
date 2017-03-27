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
#include "basic/units.hpp"


//http://www.ncdc.noaa.gov/qclcd/QCLCD?prior=N

using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace UtilWWW;
using namespace units::values;

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
			CStateSelection::$MA,
			CStateSelection::$MD,
			CStateSelection::$MI,
			CStateSelection::$MN,
			CStateSelection::$MO,
			CStateSelection::$NE,
			CStateSelection::$NH,
			CStateSelection::$NJ,
			CStateSelection::$NY,
			CStateSelection::$NC,
			CStateSelection::$OH,
			CStateSelection::$PA,
			CStateSelection::$SC,
			CStateSelection::$SD,
			CStateSelection::$VA,
			CStateSelection::$VT,
			CStateSelection::$WV,
			CStateSelection::$WI,
		};

 
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
					pos = source.find("<table", pos+8);
				if (pos != string::npos)
					pos = source.find("<table", pos+6); 

				
				if (pos != string::npos)
				{
					size_t pos_begin = pos + 6;
					size_t pos_end = source.find("</table>", pos_begin);
					callback.PushTask("Download stations list", pos_end - pos_begin);
					


					while (pos < pos_end && msg)
					{
						string line = FindString(source, "<td>", "</td>", pos);
						if (!line.empty())
							line = FindString(line, "<a ", "</a>");

						string ID;
						if (!line.empty()) 
							ID = TrimConst(FindString(line, "&WeatherStation=", "\">"));
						if (!line.empty() && !ID.empty())
						{
							size_t pos1 = line.find(">");
							ASSERT(pos1 != string::npos);

							string URL = FindString(line, "\"", "\"");
							string name = PurgeFileName(line.substr(pos1 + 1, line.length() - pos1 - 5));
							string state = TrimConst(line.substr(line.length() -2));
							if (state == "RI")//replace Rhode Island by pensylvania
								state = "PA";
							if (state == "DC")//District of Columbia by Maryland
								state = "MD";

							size_t s = CStateSelection::GetState(state);
							ASSERT(IsInclude(s));
							
							
							string stationPage;
							msg = GetPageText(pConnection, URL, stationPage);
							if (msg)
							{
								//Lat/Lon: 41.81/-74.25<br/>             Elevation: 386 ft.
								string lat_lon = FindString(stationPage, "Lat/Lon:", "<br");
								string elev = FindString(stationPage, "Elevation:", "ft.");

								StringVector LatLonV(lat_lon, "/");
								ASSERT(LatLonV.size() == 2);

								
								double lat = ToDouble(LatLonV[0]);
								double lon = ToDouble(LatLonV[1]);
								double alt = ToDouble(elev);
								if (alt > -999)
									alt = WBSF::Feet2Meter(alt);
								else
									alt = -999;

								CLocation location(name, ID, lat, lon, alt);
								location.SetSSI("State", state);
								stationList.push_back(location);
							}//if msg
						}//if line not empty

						msg += callback.SetCurrentStepPos(pos - pos_begin);
					}//for all lines
				}//if pos not null
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
		callback.PopTask();

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
		CStateSelection states(Get(STATES));

		
		vector<vector<array<bool, 12>>> bNeedDownload(m_stations.size());
		for (size_t i = 0; i < m_stations.size() && msg; i++)
		{
			string state = m_stations[i].GetSSI("State");
			
			if (states.at(state))
			{
				bNeedDownload[i].resize(nbYears);
				for (size_t y = 0; y < nbYears&&msg; y++)
				{
					int year = firstYear + int(y);

					size_t nbm = year == today.GetYear() ? today.GetMonth() + 1 : 12;
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
		}

		callback.PopTask();

		

		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;
		msg = GetHttpConnection(SERVER_NAME2, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS);

		if (!msg)
			return msg;

		//pSession->SetOption(INTERNET_OPTION_RECEIVE_TIMEOUT, 15000);


		int nbDownload = 0;
		int currentNbDownload = 0;

		callback.PushTask("Download NEWA data (" + ToString(nbFilesToDownload) + " files)", nbFilesToDownload);

		for (size_t i = 0; i < bNeedDownload.size() && msg; i++)
		{
			for (size_t y = 0; y < bNeedDownload[i].size()&& msg; y++)
			{
				int year = firstYear + int(y);
				for (size_t m = 0; m < bNeedDownload[i][y].size() && msg; m++)
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


	static CTRef GetTRef(string str_date, CTM TM)
	{
		CTRef TRef;

		StringVector tmp(str_date, "/ :");
		
		if (tmp.size() >= 3)
		{
			size_t m = ToSizeT(tmp[0]) - 1;
			size_t d = ToSizeT(tmp[1]) - 1;
			int year = ToInt(tmp[2]);
			size_t h = 0;

			if (tmp.size() >= 4)
				h = ToSizeT(tmp[3]);

			TRef = CTRef(year, m, d, h, TM);
		}


		return TRef;
	}

	static TVarH GetVar(const string& var_name)
	{

		static const char* VAR_NAME[] = { "temp", "avg", "max", "min", "lw", "rain", "dewpoint", "rh", "rhhrs", "avg wind", "wind spd", "wind dir", "solar rad", "Est LW" };
		static const TVarH VARIABLES[] = { H_TAIR2, H_TAIR2, H_TMAX2, H_TMIN2, H_ADD1, H_PRCP, H_TDEW, H_RELH, H_SKIP, H_WNDS, H_WNDS, H_WNDD, H_SRAD2, H_ADD1 };

		TVarH var = H_SKIP;
		for (size_t v = 0; v < sizeof(VAR_NAME) / sizeof(char*) && var == H_SKIP; v++)
		{
			if (var_name == VAR_NAME[v])
				var = VARIABLES[v];
		}

		return var;
	}

	static double Convert(TVarH var, double value)
	{
		switch (var)
		{
		case H_TMIN2: 
		case H_TAIR2:
		case H_TMAX2:
		case H_TDEW: value = (float)Celsius(Fahrenheit(value)).get(); break; //F --> °C; 
		case H_PRCP: value = (float)mm(inch(value)).get(); break; //inch --> mm
		case H_WNDS: value = kph(mph(value)).get(); break; //miles/h --> km/h
		case H_SRAD2: value *= 697.3/60.0; break;// Langley --> W/m²
		case H_RELH:
		case H_WNDD:
		case H_ADD1: break;//leaf wetness (minutes)
		default:ASSERT(false);
		}


		return value;
	}

	ERMsg CUINEWA::DownloadMonth(CHttpConnectionPtr& pConnection, int year, size_t m, const string& ID, const string& filePath, CCallback& callback)
	{
		ERMsg msg;

		size_t type = as <size_t>(DATA_TYPE);
		CTRef TRef = CTRef::GetCurrentTRef();
		CTM TM(type == HOURLY_WEATHER ? CTM::HOURLY : CTM::DAILY);


		CWeatherStation weather_data(TM.IsHourly());


		static const char PageDataFormat[] = "newaLister/%s/%s/%d/%d";

		string pageURL = FormatA(PageDataFormat, type == HOURLY_WEATHER ? "hly" : "dly", ID.c_str(), year, m + 1);

		string source;
		msg = GetPageText(pConnection, pageURL, source, false, FLAGS);

		size_t begin = source.find("<table");
		size_t end = source.find("</table>");
		if (!source.empty() && begin != string::npos && end != string::npos)
		{
			try
			{
				string tmp = source.substr(begin, end - begin + 8);
				//convert html to xml
				ReplaceString(tmp, "Total<br>Rain", "Rain<br>");
				ReplaceString(tmp, "RH<br>Hrs", "RHHrs<br>");
				
				ReplaceString(tmp, "<br>", "|");
				ReplaceString(tmp, "\t", "");
				ReplaceString(tmp, "\r", "");
				ReplaceString(tmp, "\n", "");



				static const char* TO_REMOVE[] = { " class=", " colspan=", " height=" };
				for (size_t i = 0; i < sizeof(TO_REMOVE) / sizeof(char*); i++)
				{
					size_t pos1 = tmp.find(TO_REMOVE[i]);
					while (pos1 != string::npos)
					{
						size_t pos2 = tmp.find(">", pos1 + sizeof(TO_REMOVE[i]));
						if (pos2 != string::npos)
							tmp.erase(pos1, pos2 - pos1);

						pos1 = tmp.find(TO_REMOVE[i]);
					}
				}


				if (type == HOURLY_WEATHER)
				{
					//ok seem to have a problem with the header </tr> tag
					size_t make_cor = tmp.find("<tbody>");
					if (make_cor != string::npos)
						make_cor = tmp.find("</th><tr>");
					if (make_cor != string::npos)
						tmp.insert(tmp.begin() + make_cor + 6, '/');
				}


				MakeLower(tmp);
				zen::XmlDoc doc = zen::parse(tmp);


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
					}
				}
				ASSERT(!var_names.empty());

				zen::XmlIn data = xml_in["tbody"];
				for (zen::XmlIn child1 = data["tr"]; child1&&msg; child1.next())
				{

					StringVector var_values;
					for (zen::XmlIn child2 = child1["td"]; child2&&msg; child2.next())
					{
						string var_str;
						if (child2.get()->getValue(var_str))
							var_values.push_back(var_str);
					}//for all col

					if (var_names.size() == var_values.size())
					{
						ASSERT(var_names[0] == "date" || var_names[0] == "date/time");

						CTRef TRef = GetTRef(var_values[0], TM);
						if (TRef.IsInit())
						{
							for (size_t v = 1; v < var_names.size(); v++)
							{
								TVarH var = GetVar(var_names[v]);
								if (var != H_SKIP)
								{
									double value = ToDouble(var_values[v]);
									weather_data[TRef].SetStat(var, Convert(var, value));
								}
							}
						}
					}
				}//for all line



			}//try
			catch (const zen::XmlParsingError& e)
			{
				// handle error
				msg.ajoute("Error parsing XML file: col=" + ToString(e.col) + ", row=" + ToString(e.row));
			}
		}//if is valid

		//msg += callback.StepIt();

		if (msg)//always save the file to avoid to download it again
		{
			weather_data.SaveData(filePath);
		}//if have data

		//callback.PopTask();

		return msg;
	}
	
	std::string CUINEWA::GetStationListFilePath()const
	{
		return (std::string)GetDir(WORKING_DIR) + "StationsList.csv";
	}


	string CUINEWA::GetOutputFilePath(int year, size_t m, const string& ID)const
	{
		size_t type = as<size_t>(DATA_TYPE);
		return GetDir(WORKING_DIR) + (type == HOURLY_WEATHER ? "Hourly" : "Daily") + "\\" + ToString(year) + "\\" + FormatA("%02d", m + 1) + "\\" + ID + ".csv";
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
			{
				if (m_stations[i].m_elev != -999)
					stationList.push_back(m_stations[i].m_ID);
				else
					callback.AddMessage("Station " + m_stations[i].m_name + " (" + m_stations[i].m_ID + ") have unknown elevation");
			}
		}

		return msg;
	}
	
	
	ERMsg CUINEWA::GetWeatherStation(const std::string& ID, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;

		size_t type = as <size_t>(DATA_TYPE);
		if ( TM.Type() == CTM::DAILY && type != DAILY_WEATHER)
		{
			msg.ajoute("Daily database not supported on the hourly data for the moment..."); 
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


		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = size_t(lastYear - firstYear + 1);
		station.CreateYears(firstYear, nbYears);
		station.SetHourly(TM.Type()==CTM::HOURLY);

		//now extract data 
		for (size_t y = 0; y < nbYears&&msg; y++)
		{
			int year = firstYear + int(y);

			for (size_t m = 0; m < 12 && msg; m++)
			{
				string filePath = GetOutputFilePath(year, m, ID);
				if (FileExists(filePath))
				{
					msg = station.LoadData(filePath, -999, false);
					msg += callback.StepIt(0);
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

		return msg;
	}


}