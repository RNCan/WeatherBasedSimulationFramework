#include "StdAfx.h"
#include "UIBC_PAWS.h"

#include "DailyStation.h"
#include "DailyDatabase.h"
#include "SYShowMessage.h"
#include "CommonRes.h"
#include "Resource.h"
#include "Registry.h"
#include "FileStamp.h"
#include "CSV.h"
#include "HttpCall.h"
#include "SelectionDlg.h"


//Avalanche and Weather Programs

static const DWORD FLAGS = INTERNET_FLAG_RELOAD | INTERNET_FLAG_SECURE | INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_TRANSFER_ASCII;

using namespace std;
namespace WBSF
{
	//*********************************************************************
	//const char* CUIBCPAWS::ATTRIBUTE_NAME[0]={};
	const char* CUIBCPAWS::CLASS_NAME = "BC_PAWS";
	const char* CUIBCPAWS::SERVER_NAME = "prdoas3.pub-apps.th.gov.bc.ca";



	CUIBCPAWS::CUIBCPAWS(void)
	{
		if (!IsRegister(GetClassID()))
		{
			InitClass();
		}

		Reset();
	}

	void CUIBCPAWS::InitClass(const StringVector& option)
	{
		GetParamClassInfo().m_className = GetString(IDS_SOURCENAME_BC_PAWS);

		CUIWeather::InitClass(option);

		//ASSERT( GetParameters().size() < I_NB_ATTRIBUTE);

		//StringVector header(IDS_PROPERTIES_BC_PAWS, "|;");
		//ASSERT( header.size() == NB_ATTRIBUTE);

		//StringVector forceList(IDS_PROPERTIES_ENV_CAN_OPTIONS);
		//GetParameters().push_back( CParamDef(CParamDef::EDIT, ATTRIBUTE_NAME[0], header[0]) );
		//GetParameters().push_back( CParamDef(CParamDef::COMBO, CParamDef::BY_NUMBER, ATTRIBUTE_NAME[1], header[1], forceList, "0") );
		//GetParameters().push_back( CParamDef(CParamDef::BOOL, ATTRIBUTE_NAME[2], header[2], "0") );

	}

	CUIBCPAWS::~CUIBCPAWS(void)
	{
	}


	CUIBCPAWS::CUIBCPAWS(const CUIBCPAWS& in)
	{
		operator=(in);
	}

	void CUIBCPAWS::Reset()
	{
		CUIWeather::Reset();
	}

	CUIBCPAWS& CUIBCPAWS::operator =(const CUIBCPAWS& in)
	{
		if (&in != this)
		{
			CUIWeather::operator =(in);
		}

		return *this;
	}

	bool CUIBCPAWS::operator ==(const CUIBCPAWS& in)const
	{
		bool bEqual = true;

		if (CUIWeather::operator !=(in))bEqual = false;

		return bEqual;
	}

	bool CUIBCPAWS::operator !=(const CUIBCPAWS& in)const
	{
		return !operator ==(in);
	}

	string CUIBCPAWS::GetValue(size_t type)const
	{
		ERMsg msg;
		string value;

		ASSERT(NB_ATTRIBUTE == 0);
		//switch(type)
		//{
		//case I_PROVINCE: value = m_selection.ToString(); break;
		//default: value = CUIWeather::GetValue(type); break;
		//}

		return CUIWeather::GetValue(type);
	}

	void CUIBCPAWS::SetValue(size_t type, const string& value)
	{
		ASSERT(NB_ATTRIBUTE == 0);
		//switch(type)
		//{
		//case I_PROVINCE: m_selection.FromString(value); break;
		//default: CUIWeather::SetValue(type, value); break;
		//}

		CUIWeather::SetValue(type, value);
	}



	bool CUIBCPAWS::Compare(const CParameterBase& in)const
	{
		ASSERT(in.GetClassID() == GetClassID());
		const CUIBCPAWS& info = dynamic_cast<const CUIBCPAWS&>(in);
		return operator==(info);
	}

	CParameterBase& CUIBCPAWS::Assign(const CParameterBase& in)
	{
		ASSERT(in.GetClassID() == GetClassID());
		const CUIBCPAWS& info = dynamic_cast<const CUIBCPAWS&>(in);
		return operator=(info);
	}


	//Interface attribute index to attribute index
	//sample for BC:
	//saw-paws/weatherdata?loc=ENDGOAL&ins=2001-10-25&dis=&locId=850&page=init&type=RAWS&code=51126
	static const char pageFormat[] =
		"saw-paws/weatherdata?"
		"code=38121&"
		"ins=1986-01-02&"
		"dis=&"
		"loc=3+VALLEY+REMOTE&"
		"page=init&"
		"type=RAWS&"
		"locId=796";

	static const short SEL_ROW_PER_PAGE = 25;

	static string CleanString(string str)
	{
		string output;

		//str = UtilWWW::FindString(str, "<td>", "</td>");
		//str.Replace("<abbr title=\"degrees\">","");
		//str.Replace("<abbr title=\"minute\">","");
		//str.Replace("<abbr title=\"second\">","");
		//str.Replace("<abbr title=\"North\">","");
		//str.Replace("<abbr title=\"West\">","");
		//str.Replace("<abbr title=\"meter\">","");
		//str.Replace("</abbr>","");
		//

		//str.Replace("m","");
		////str.Replace( ",", "" );//remove thousand ,
		//str.Replace( "&deg;", " " );
		//str.Replace( "&quot;", " " );
		//str.Replace( "°", " " );
		//str.Replace( "'", " " );
		//str.Replace( "\"", " " );
		//str.Replace( "N", " " );
		//str.Replace( "W", " " );
		//str.Trim();


		return str;
	}

	ERMsg CUIBCPAWS::UpdateStationList(CFL::CCallback& callback)const
	{
		ERMsg msg;

		CLocationVector stations;
		CreateMultipleDir(GetPath(GetStationListFilePath()));


		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;

		msg = GetHttpConnection(SERVER_NAME, pConnection, pSession, FLAGS);
		if (!msg)
			return msg;

		string source;
		UtilWWW::GetPageText(pConnection, "saw-paws/weatherstation?page=stationList&format=csv", source, true, FLAGS);

		ReplaceString(source, ", ", ".");
		ReplaceString(source, "\" ", "\"");
		ReplaceString(source, "\"", "");
		ReplaceString(source, "CODE", "KeyID");
		ReplaceString(source, "LAT.", "Latitude");
		ReplaceString(source, "LONG.", "Longitude");
		ReplaceString(source, "ELEVATION", "Elevation");
		ofStream file;
		msg = file.open(GetStationListFilePath());
		if (msg)
		{
			file.write(source);
			file.close();

			msg = stations.Load(GetStationListFilePath());
			if (msg)
			{
				//now add internal ID
				callback.SetCurrentDescription(GetString(IDS_LOAD_STATION_LIST));
				callback.SetNbStep(stations.size());


				string source;
				UtilWWW::GetPageText(pConnection, "saw-paws/weatherstation", source, false, FLAGS);
				//loop on province
				string::size_type begin = 0;
				string str = stdString::FindString(source, "code:\"<a href=weatherdata?", " style", begin);

				while (begin != string::npos&&msg)
				{
					enum TElem{ CODE, INSTTATION_DATE, DISCONTINUATION_DATE, LOCATION_NAME, PAGE, TYPE, LOC_ID, NB_ELEMENTS };
					StringVector elem(str, "&");
					ASSERT(elem.size() == NB_ELEMENTS);

					StringVector ID(elem[CODE], "="); ASSERT(ID.size() == 2);
					StringVector locID(elem[LOC_ID], "=");	ASSERT(locID.size() == 2);

					size_t pos = stations.FindByID(ID[1]);
					assert(pos != UNKNOWN_POS);

					stations[pos].m_name = stdString::UppercaseFirstLetter(stations[pos].m_name);
					stations[pos].SetSSI("InternalID", locID[1]);

					str = stdString::FindString(source, "code:\"<a href=weatherdata?", " style", begin);
					msg += callback.StepIt();
				}

				//save with update
				msg = stations.Save(GetStationListFilePath());
			}
		}

		pConnection->Close();
		pSession->Close();

		callback.AddMessage(GetString(IDS_NB_STATIONS) + ToString(stations.size()));

		return msg;
	}



	static CTPeriod GetStationPeriod(const CLocation& location)
	{
		CTRef TRef1;
		TRef1.FromFormatedString(location.GetSSI("INSTALLED"), "", "/", 1);


		CTRef TRef2 = CTRef::GetCurrentTRef();
		string str = location.GetSSI("DISCONT.");
		if (!str.empty())
			TRef2.FromFormatedString(str, "", "/", 1);

		assert(TRef1.IsValid());
		assert(TRef2.IsValid());
		return CTPeriod(TRef1, TRef2);
	}


	ERMsg CUIBCPAWS::Execute(CFL::CCallback& callback)
	{
		ERMsg msg;


		if (!FileExists(GetStationListFilePath()))
		{
			msg = UpdateStationList(callback);

			if (!msg)
				return msg;
		}

		msg = m_stations.Load(GetStationListFilePath());
		if (!msg)
			return msg;



		string workingDir = GetWorkingDir();

		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(workingDir, 1);
		callback.AddMessage(GetString(IDS_UPDATE_FROM));
		callback.AddMessage(SERVER_NAME, 1);
		callback.AddMessage("");

		size_t nbYears = m_lastYear - m_firstYear + 1;
		callback.SetNbTask(nbYears);
		//Get station





		for (size_t y = 0; y < nbYears&&msg; y++)
		{
			int year = m_firstYear + int(y);

			callback.SetCurrentDescription("Download Data for " + ToString(year));
			callback.SetNbStep((int)m_stations.size());

			int nbRun = 0;
			size_t curI = 0;
			while (curI < m_stations.size() && msg)
			{
				nbRun++;

				CInternetSessionPtr pSession;
				CHttpConnectionPtr pConnection;

				msg = GetHttpConnection(SERVER_NAME, pConnection, pSession, FLAGS);

				if (msg)
				{
					TRY
					{
						for (size_t i = curI; i < m_stations.size() && msg; i++)
						{
							CTPeriod p = GetStationPeriod(m_stations[i]);

							if (p.IsIntersect(CTPeriod(year, 0, 0, year, 11, 30)))
							{
								string filePath = GetOutputFilePath(year, m_stations[i].m_ID);
								if (!CFL::FileExists(filePath))
								{
									static const char webPageDataFormat1[] =
									{
										"saw-paws/weatherdata?"
										"loc=%s&"
										"ins=%s&"
										"dis=%s&"
										"locId=%s&"
										"page=init&"
										"type=%s&"
										"code=%s"
									};


									string loc = m_stations[i].m_name; stdString::MakeUpper(loc); std::replace(loc.begin(), loc.end(), ' ', '+');
									string ins = m_stations[i].GetSSI("INSTALLED");
									string dis = m_stations[i].GetSSI("DISCONT.");
									string locID = m_stations[i].GetSSI("InternalID");
									string type = m_stations[i].GetSSI("TYPE");
									string code = m_stations[i].m_ID;


									string URL1 = stdString::FormatA(webPageDataFormat1, loc.c_str(), ins.c_str(), dis.c_str(), locID.c_str(), type.c_str(), code.c_str());

									string txt1;
									UtilWWW::GetPageText(pConnection, URL1, txt1, false, FLAGS);

									static const char webPageDataFormat2[] =
									{
										"saw-paws/weatherdata?"
										"page=submit&"
										"yearSelect=%d"
									};

									string URL2 = FormatA(webPageDataFormat2, year);


									CFL::CreateMultipleDir(GetPath(filePath));
									msg = UtilWWW::CopyFile(pConnection, URL2, filePath, FLAGS);
								}
							}


							if (msg)
							{
								curI++;
								nbRun = 0;
								msg += callback.StepIt();
							}
						}

					}
						CATCH_ALL(e)
					{
						msg = SYGetMessage(*e);
					}
					END_CATCH_ALL

						//if an error occur: try again
						if (!msg && !callback.GetUserCancel())
						{
							callback.AddTask(1);//one step more

							if (nbRun < 5)
							{
								callback.AddMessage(msg);
								msg.asgType(ERMsg::OK);
								Sleep(10000);//wait 10 sec
							}
						}

					//clean connection
					pConnection->Close();
					pSession->Close();
				}
			}
		}


		return msg;
	}


	ERMsg CUIBCPAWS::PreProcess(CFL::CCallback& callback)
	{
		ERMsg msg = m_stations.Load(GetStationListFilePath());
		msg += m_stations.IsValid();

		return msg;
	}

	string CUIBCPAWS::GetOutputFilePath(short year, const string& stationName)const
	{
		return GetWorkingDir() + ToString(year) + "\\" + stationName + ".txt";
	}

	ERMsg CUIBCPAWS::GetStationList(StringVector& stationList, CFL::CCallback& callback)
	{
		ERMsg msg;

		for (size_t i = 0; i < m_stations.size(); i++)
		{
			CTPeriod p = GetStationPeriod(m_stations[i]);
			if (p.IsIntersect(CTPeriod(m_firstYear, 0, 0, m_lastYear, 11, 30)))
			{
				stationList.push_back(m_stations[i].m_ID);
			}
		}

		return msg;
	}



	ERMsg CUIBCPAWS::GetStation(const string& str, CDailyStation& station, CFL::CCallback& callback)
	{
		return GetWeatherStation(str, CTM(CTM::DAILY), station, callback);
	}


	ERMsg CUIBCPAWS::GetWeatherStation(const std::string& ID, CTM TM, CWeatherStation& station, CFL::CCallback& callback)
	{
		ERMsg msg;

		size_t pos = m_stations.FindByID(ID);
		((CLocation&)station) = m_stations[pos];

		int nbYears = m_lastYear - m_firstYear + 1;

		CTRef currentTRef = CTRef::GetCurrentTRef();

		//now extract data 
		for (size_t y = 0; y < nbYears&&msg; y++)
		{
			int year = m_firstYear + int(y);

			string filePath = GetOutputFilePath(year, station.m_ID);
			if (CFL::FileExists(filePath))
			{
				CYear& dailyData = station.CreateYear(year);
				msg = ReadData(filePath, dailyData);
			}
		}


		if (station.HaveData())
		{
			msg = station.IsValid();
			ASSERT(station.IsValid());

		}
		//else
		//{
		//msg.ajoute( stdString::GetString(IDS_NO_WEATHER) + station.m_name + " [" + station.m_ID + "]" );
		//}

		return msg;
	}


	ERMsg CUIBCPAWS::ReadData(const string& filePath, CYear& dailyData)const
	{
		ERMsg msg;

		enum{ CODE, DATETIME, OBS_TYPE, MAX_TEMP, PRES_TEMP, MIN_TEMP, RH, DEW_POINT, HRLY_PRCP, NEW_PRCP, PRCP_GAUGE, NEW_SNOW, SNOW_PACK, PRECIP_DET_RATIO, WND_SPD_MEAS, MAX_GUST_1, WND_DIR_1, STD_DEV_1, ATM_PRESSURE, NB_VARS };



		ifStream file;
		msg = file.open(filePath);

		if (msg)
		{
			//vector<int> members;

			size_t i = 0;
			for (CSVIterator loop(file, "\t"); loop != CSVIterator(); ++loop, i++)
			{
				CTM TM(CTM::DAILY);
				CWeatherAccumulator stat(TM);
				ENSURE(loop.Header().size() == NB_VARS);

				//NEW_PRCP, PRECIP_DET_RATIO, WND_SPD_MEAS, MAX_GUST_1, WND_DIR_1, STD_DEV_1, ATM_PRESSURE, NB_VARS
				string ID = (*loop)[CODE];
				StringVector dateTime((*loop)[DATETIME], "/ :"); ASSERT(dateTime.size() == 5);
				int year = stdString::ToInt(dateTime[0]);
				size_t m = stdString::ToInt(dateTime[1]) - 1;
				size_t d = stdString::ToInt(dateTime[2]) - 1;
				size_t h = stdString::ToInt(dateTime[3]);
				ASSERT(year >= m_firstYear && year <= m_lastYear);
				ASSERT(m >= 0 && m < 12);
				ASSERT(d >= 0 && d < CFL::GetNbDayPerMonth(year, m));
				CTRef Tref(year, m, d, h);

				if (!(*loop)[MAX_TEMP].empty() && !(*loop)[MIN_TEMP].empty())
				{
					float Tmin = ToFloat((*loop)[MIN_TEMP]);
					ASSERT(Tmin >= -70 && Tmin <= 70);
					float Tmax = ToFloat((*loop)[MAX_TEMP]);
					ASSERT(Tmax >= -70 && Tmax <= 70);
					stat.Add(Tref, H_TAIR, Tmin);
					stat.Add(Tref, H_TAIR, Tmax);
				}

				if (!(*loop)[HRLY_PRCP].empty())
				{
					float prcp = ToFloat((*loop)[HRLY_PRCP]);
					ASSERT(prcp >= 0 && prcp < 1000);

					stat.Add(Tref, H_PRCP, prcp);
				}

				if (!(*loop)[DEW_POINT].empty())
				{
					float Tdew = ToFloat((*loop)[DEW_POINT]);
					ASSERT(Tdew >= -70 && Tdew < 70);

					stat.Add(Tref, H_TDEW, Tdew);
				}

				if (!(*loop)[RH].empty())
				{
					float Hr = ToFloat((*loop)[RH]);
					ASSERT(Hr >= 0 && Hr < 70);

					stat.Add(Tref, H_RELH, Hr);
				}

				if (!(*loop)[SNOW_PACK].empty())
				{
					float sd = ToFloat((*loop)[SNOW_PACK]);
					ASSERT(sd >= 0 && sd < 300);

					stat.Add(Tref, H_SNDH, sd);
				}
				if (!(*loop)[NEW_SNOW].empty())
				{
					float sd = ToFloat((*loop)[SNOW_PACK]);
					ASSERT(sd >= 0 && sd < 300);

					stat.Add(Tref, H_SNOW, sd);
				}

				if (!(*loop)[WND_SPD_MEAS].empty())
				{
					float ws = ToFloat((*loop)[WND_SPD_MEAS]);
					ASSERT(ws >= 0 && ws < 120);//km/h

					stat.Add(Tref, H_WNDS, ws);
				}

				if (!(*loop)[WND_DIR_1].empty())
				{
					float wd = ToFloat((*loop)[WND_DIR_1]);
					ASSERT(wd >= 0 && wd < 360);

					stat.Add(Tref, H_WNDD, wd);
				}

				if (!(*loop)[ATM_PRESSURE].empty())
				{
					float P = ToFloat((*loop)[ATM_PRESSURE]);
					ASSERT(P >= 930 && P < 1080);

					stat.Add(Tref, H_PRES, P);
				}


				dailyData[Tref].SetData(stat);
			}//for all line
		}

		return msg;
	}


}