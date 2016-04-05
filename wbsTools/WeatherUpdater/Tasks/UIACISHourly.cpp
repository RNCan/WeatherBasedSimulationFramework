#include "StdAfx.h"
#include "UIACISHourly.h"


#include "Basic/HourlyDatabase.h"
#include "Basic/CSV.h"
#include "Basic/FileStamp.h"
#include "UI/Common/SYShowMessage.h"
#include "TaskFactory.h"

#include "../Resource.h"



using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace UtilWWW;

namespace WBSF
{

	//*********************************************************************
	static const DWORD FLAGS = INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_PRAGMA_NOCACHE;

	//*********************************************************************
	const char* CUIACISHourly::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "UserName", "Password", "WorkingDir", "FirstYear", "LastYear" };
	const size_t CUIACISHourly::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_STRING, T_PASSWORD, T_PATH, T_STRING, T_STRING };
	const UINT CUIACISHourly::ATTRIBUTE_TITLE_ID = IDS_UPDATER_ACIS_HOURLY_P;

	const char* CUIACISHourly::CLASS_NAME(){ static const char* THE_CLASS_NAME = "ACISHourly";  return THE_CLASS_NAME; }
	CTaskBase::TType CUIACISHourly::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterClass(CUIACISHourly::CLASS_NAME(), CUIACISHourly::create);
	
	const char* CUIACISHourly::SERVER_NAME = "www.agric.gov.ab.ca";

	//www.
	CUIACISHourly::CUIACISHourly(void)
	{}

	CUIACISHourly::~CUIACISHourly(void)
	{}

	long CUIACISHourly::GetNbDay(const CTime& t)
	{
		return GetNbDay(t.GetYear(), t.GetMonth() - 1, t.GetDay() - 1);
	}

	long CUIACISHourly::GetNbDay(int y, int m, int d)
	{
		ASSERT(m >= 0 && m < 12);
		ASSERT(d >= 0 && d < 31);

		return long(y * 365 + m*30.42 + d);
	}

	//StringVector typeList("Hourly|Daily", "|");


	

	std::string CUIACISHourly::Default(size_t i)const
	{
		string str;

		switch (i)
		{
		case FIRST_YEAR:
		case LAST_YEAR:	str = ToString(CTRef::GetCurrentTRef().GetYear()); break;
		};

		return str;
	}


	//Interface attribute index to attribute index
	//sample for alberta:

	//http://www.agric.gov.ab.ca/acis/api/v1/legacy/weather-data/timeseries?stations=10540,15219,77444,11799,2058&elements=PRCIP,ATX,ATN,HU,HUAM,WS,WD,WSAM,WDAM&startdate=20151001&enddate=20151031&interval=HOURLY&format=csv&precipunit=mm&inclCompleteness=true&inclSource=false&inclComments=false&session=Ol-9wEJrnJseSEDtllSgoDv
	static const char PageDataFormatH[] =
		"acis/api/v1/legacy/weather-data/timeseries?"
		"stations=%s&"
		"elements=PRCIP,ATX,ATN,HU,HUAM,WS,WD,WSAM,WDAM&"
		"startdate=%4d%02d%02d&"
		"enddate=%4d%02d%02d&"
		"interval=HOURLY&"
		"format=csv&"
		"precipunit=mm&"
		"inclCompleteness=true&"
		"inclSource=false&"
		"inclComments=false&"
		"session=%s";



	static const char PageDataFormatD[] =
		"acis/api/v1/legacy/weather-data/timeseries?"
		"stations=%s&"
		"elements=PRCIP,ATX,ATN,HU,HUAM,WS,WD,WSAM,WDAM&"
		"startdate=%4d%02d%02d&"
		"enddate=%4d%02d%02d&"
		"interval=DAILY&"
		"format=csv&"
		"precipunit=mm&"
		"inclCompleteness=true&"
		"inclSource=false&"
		"inclComments=false&"
		"session=%s";


	ERMsg CUIACISHourly::GetStationList(CLocationVector& stationList, CCallback& callback)const
	{
		ERMsg msg;

		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;

		msg = GetHttpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, Get(USER_NAME), Get(PASSWORD));
		if (!msg)
			return msg;

		

		//Get cookie and page list
		string source;
		msg = GetPageText(pConnection, "acis/alberta-weather-data-viewer.jsp", source);
		if (msg)
		{

			vector<string> listID;
			string::size_type posBegin = source.find("Select Station(s):");
			//int posEnd = posBegin;
			while (msg&&posBegin != string::npos)
			{
				string ID = FindString(source, "<option value=", ">", posBegin);
				//string Name = FindString(source, "</option>", posBegin);

				if (ID != "-1")
				{
					listID.push_back(ID);

					posBegin = source.find("<option value=", posBegin);
					msg += callback.StepIt(0);
				}
			}

			callback.PushTask(GetString(IDS_LOAD_STATION_LIST), listID.size());
			//callback.SetNbStep(listID.size());
			for (size_t i = 0; i < listID.size() && msg; i++)
			{
				string metadata;
				msg = GetPageText(pConnection, ("acis/station/metadata?resource=complete&station=" + listID[i]).c_str(), metadata);
				string::size_type metaPosBegin = metadata.find("<name>");
				if (msg && metaPosBegin != string::npos)
				{
					//CLocation stationInfo;
					CLocation stationInfo;
					stationInfo.m_name = TrimConst(FindString(metadata, "<name>", "</name>", metaPosBegin));
					stationInfo.m_ID = TrimConst(listID[i]);
					stationInfo.m_lat = ToDouble(FindString(metadata, "<latitude>", "</latitude>", metaPosBegin));
					stationInfo.m_lon = ToDouble(FindString(metadata, "<longitude>", "</longitude>", metaPosBegin));
					stationInfo.m_alt = ToDouble(FindString(metadata, "<elevation>", "</elevation>", metaPosBegin));
					string ECID = FindString(metadata, "<EC>", "</EC>", metaPosBegin);
					if (!ECID.empty())
						stationInfo.SetSSI("ClimateID", ECID);

					stationList.push_back(stationInfo);
					msg += callback.StepIt();
				}
			}
		}

		pConnection->Close();
		pSession->Close();
		callback.PopTask();


		//clear all non ACIS stations
		for (CLocationVector::iterator it = stationList.begin(); it != stationList.end();)
		{
			if (it->GetSSI("ClimateID").substr(0, 3) == "999")
				it++;
			else
				it = stationList.erase(it);
		}


		callback.AddMessage(GetString(IDS_NB_STATIONS) + ToString(stationList.size()));


		return msg;
	}

	string CUIACISHourly::GetSessiosnID(CHttpConnectionPtr& pConnection)
	{
		string sessionID;
		string page;

		if (GetPageText(pConnection, "acis/alberta-weather-data-viewer.jsp", page))
		{

			string::size_type posBegin = page.find("getSessionId()");
			if (posBegin >= 0)
				sessionID = FindString(page, "return \"", "\";", posBegin);
		}

		return sessionID;
	}

	ERMsg CUIACISHourly::Execute(CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);

		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(workingDir, 1);
		callback.AddMessage(GetString(IDS_UPDATE_FROM));
		callback.AddMessage(SERVER_NAME, 1);
		callback.AddMessage("");


		if (FileExists(GetStationListFilePath()))
		{
			msg = m_stations.Load(GetStationListFilePath());
		}
		else
		{
			CreateMultipleDir(GetPath(GetStationListFilePath()));
			msg = GetStationList(m_stations, callback);

			if (msg)
				msg = m_stations.Save(GetStationListFilePath());
		}

		if (!msg)
			return msg;


		
		msg = DownloadStationHourly(callback);

		return msg;
	}

	static int ClearIECache()
	{
		// Pointer to a GROUPID variable
		GROUPID groupId = 0;

		// Local variables
		DWORD cacheEntryInfoBufferSizeInitial = 0;
		DWORD cacheEntryInfoBufferSize = 0;
		int *cacheEntryInfoBuffer = 0;
		INTERNET_CACHE_ENTRY_INFO *internetCacheEntry;
		HANDLE enumHandle = NULL;
		BOOL returnValue = false;

		// Delete the groups first.
		// Groups may not always exist on the system.
		// For more information, visit the following Microsoft Web site:
		// http://msdn2.microsoft.com/en-us/library/ms909365.aspx			
		// By default, a URL does not belong to any group. Therefore, that cache may become
		// empty even when the CacheGroup APIs are not used because the existing URL does not belong to any group.			
		enumHandle = FindFirstUrlCacheGroup(0, CACHEGROUP_SEARCH_ALL, 0, 0, &groupId, 0);

		// If there are no items in the Cache, you are finished.
		if (enumHandle != NULL && ERROR_NO_MORE_ITEMS == GetLastError())
			return 0;

		// Loop through Cache Group, and then delete entries.
		while (1)
		{
			// Delete a particular Cache Group.
			returnValue = DeleteUrlCacheGroup(groupId, CACHEGROUP_FLAG_FLUSHURL_ONDELETE, 0);

			if (returnValue)
				returnValue = FindNextUrlCacheGroup(enumHandle, &groupId, 0);

			DWORD dwError = GetLastError();
			if (!returnValue && ERROR_NO_MORE_ITEMS == dwError)
			{
				break;
			}
		}

		// Start to delete URLs that do not belong to any group.
		enumHandle = FindFirstUrlCacheEntry(NULL, 0, &cacheEntryInfoBufferSizeInitial);
		if (enumHandle == NULL && ERROR_NO_MORE_ITEMS == GetLastError())
			return 0;

		cacheEntryInfoBufferSize = cacheEntryInfoBufferSizeInitial;
		internetCacheEntry = (INTERNET_CACHE_ENTRY_INFO *)malloc(cacheEntryInfoBufferSize);
		enumHandle = FindFirstUrlCacheEntry(NULL, internetCacheEntry, &cacheEntryInfoBufferSizeInitial);
		while (1)
		{
			cacheEntryInfoBufferSizeInitial = cacheEntryInfoBufferSize;
			returnValue = DeleteUrlCacheEntry(internetCacheEntry->lpszSourceUrlName);

			if (returnValue)
				returnValue = FindNextUrlCacheEntry(enumHandle, internetCacheEntry, &cacheEntryInfoBufferSizeInitial);

			DWORD dwError = GetLastError();
			if (!returnValue && ERROR_NO_MORE_ITEMS == dwError)
			{
				break;
			}

			if (!returnValue && cacheEntryInfoBufferSizeInitial > cacheEntryInfoBufferSize)
			{
				cacheEntryInfoBufferSize = cacheEntryInfoBufferSizeInitial;
				internetCacheEntry = (INTERNET_CACHE_ENTRY_INFO *)realloc(internetCacheEntry, cacheEntryInfoBufferSize);
				returnValue = FindNextUrlCacheEntry(enumHandle, internetCacheEntry, &cacheEntryInfoBufferSizeInitial);
			}
		}

		free(internetCacheEntry);

		return 0;
	}


	static CTRef GetTRef(__time64_t t, CTM TM = CTM(CTM::DAILY))
	{
		CTRef TRef;

		if (t > 0)
		{
			struct tm *theTime = _localtime64(&t);
			TRef = CTRef(1900 + theTime->tm_year, theTime->tm_mon, theTime->tm_mday - 1, theTime->tm_hour, TM);
		}

		return TRef;
	}

	ERMsg CUIACISHourly::DownloadStationHourly(CCallback& callback)
	{
		ERMsg msg;


		CTRef today = CTRef::GetCurrentTRef();
		string workingDir = GetDir(WORKING_DIR);
		int nbFilesToDownload = 0;
		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = lastYear - firstYear + 1;


		callback.PushTask("Clear stations list...", m_stations.size()*nbYears * 12);
		//callback.SetNbStep(m_stations.size()*nbYears * 12);

		vector<vector<array<bool, 12>>> bNeedDownload(m_stations.size());
		for (size_t i = 0; i < m_stations.size() && msg; i++)
		{
			bNeedDownload[i].resize(nbYears);
			for (size_t y = 0; y < nbYears&&msg; y++)
			{
				int year = firstYear + int(y);

				size_t nbm = year == today.GetYear() ? today.GetMonth() + 1 : 12;
				for (size_t m = 0; m < nbm && msg; m++)
				{
					string filePath = GetOutputFilePath(year, m, m_stations[i].m_ID);
					CTRef TRef = GetTRef(GetFileStamp(filePath));

					bNeedDownload[i][y][m] = !TRef.IsInit() || today - TRef < 2; //let 2 days to update the data
					nbFilesToDownload += bNeedDownload[i][y][m] ? 1 : 0;
					msg += callback.StepIt();
				}
			}
		}

		callback.PopTask();


		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;
		msg = GetHttpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, Get(USER_NAME), Get(PASSWORD));
		if (!msg)
			return msg;

		//une protection empêche de charger plus de fichier
		string sessionID = GetSessiosnID(pConnection);
		int nbDownload = 0;
		int currentNbDownload = 0;

		callback.PushTask("Download data", nbFilesToDownload);
		//callback.SetNbStep(nbFilesToDownload);
		for (size_t i = 0; i < m_stations.size() && msg; i++)
		{
			if (m_stations[i].GetSSI("ClimateID").substr(0, 3) == "999")
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

							string pageURL = FormatA(PageDataFormatH, m_stations[i].m_ID.c_str(), year, m + 1, 1, year, m + 1, GetNbDayPerMonth(year, m), sessionID.c_str());

							string source;
							msg = GetPageText(pConnection, pageURL, source, false, FLAGS | INTERNET_FLAG_FORMS_SUBMIT);
							if (msg)
							{
								if (source.find("An Error Has Occurred") == string::npos &&
									source.find("Page Not Found") == string::npos &&
									source.find("Too Many Requests") == string::npos)
								{
									ofStream file;
									msg = file.open(filePath);
									if (msg)
									{
										file << source;
										file.close();
									}

									callback.PushTask("Waiting", 600);
									//callback.SetNbStep(600);
									for (size_t s = 0; s < 600 && msg; s++)
									{
										Sleep(100);
										msg += callback.StepIt();
									}

									callback.PopTask();
									nbDownload++;
									currentNbDownload++;

									//if (nbDownload == 6)
									//{
									//	//clean connection
									//	pConnection->Close();
									//	pSession->Close();

									//	msg = GetHttpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, Get(USER_NAME), Get(PASSWORD));
									//	if (!msg)
									//		return msg;

									//	sessionID = GetSessiosnID(pConnection);
									//}
								}
								else
								{
									msg.ajoute(source);
								}

								if (msg && currentNbDownload == 8)
								{
									CString Url = _T("http://www.agric.gov.ab.ca");


									DWORD bufferSize = 0;
									CString cookiesData;
									DWORD error;

									BOOL test = InternetGetCookieEx(Url, _T("JSESSIONID"), NULL, &bufferSize, INTERNET_COOKIE_HTTPONLY, NULL);
									if (test)
										InternetGetCookieEx(Url, _T("JSESSIONID"), cookiesData.GetBufferSetLength(bufferSize), &bufferSize, INTERNET_COOKIE_HTTPONLY, NULL);
									else
										error = GetLastError();

									InternetSetCookieEx(Url, _T("JSESSIONID"), _T(""), INTERNET_COOKIE_HTTPONLY, NULL);

									test = InternetGetCookieEx(Url, _T("JSESSIONID"), NULL, &bufferSize, INTERNET_COOKIE_HTTPONLY, NULL);
									if (test)
										InternetGetCookieEx(Url, _T("JSESSIONID"), cookiesData.GetBufferSetLength(bufferSize), &bufferSize, INTERNET_COOKIE_HTTPONLY, NULL);
									else
										error = GetLastError();

									pConnection->Close();
									pSession->Close();



									//wait 10 minutes
									callback.PushTask("Waiting...", 5 * 600);
									//callback.SetNbStep(5 * 600);
									for (size_t s = 0; s < 5 * 600 && msg; s++)
									{
										Sleep(100);
										msg += callback.StepIt();
									}
									callback.PopTask();

									msg = GetHttpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, Get(USER_NAME), Get(PASSWORD));
									string olID = sessionID;
									sessionID = GetSessiosnID(pConnection);

									assert(sessionID != olID);
									currentNbDownload = 0;
								}
							}

							if (msg)
								msg += callback.StepIt();
						}
					}
				}
			}
		}


		//clean connection
		//pConnection->OpenRequest(CHttpConnection::HTTP_VERB_UNLINK, _T("acis/api/v1/legacy/weather-data/timeseries?"));
		pConnection->Close();
		pConnection.release();
		pSession->Close();
		pSession.release();


		callback.AddMessage(GetString(IDS_NB_FILES_DOWNLOADED) + ToString(nbDownload), 2);
		callback.PopTask();

		return msg;
	}

	
	bool CUIACISHourly::IsValid(const CLocation& info, short y, short m, const string& filePath)const
	{
		bool bDownload = false;

		//	CTPeriod period = String2Period(info.GetSSI("Period"));
		//if( period.IsInside( CTRef(m_firstYear+y, m, LAST_DAY) ) )
		{
			/*bDownload = true;

			if( m_password!=ALWAYS)
			{
			CFileStatus status;
			if( CFile::GetStatus( filePath, status) )
			{
			try
			{
			//if the date(year, month) of the file in the directory is
			//the same than the data in the file, we download
			//else we don't download
			if(!( status.m_mtime.GetYear() == (m_firstYear+y) &&
			status.m_mtime.GetMonth() == (m+1)) )
			bDownload = false;

			if( m_password==AFTER_ONE_YEAR )
			{
			int nbDays = GetNbDay(status.m_mtime);
			if( m_nbDays - nbDays > 365)
			bDownload = true;
			}
			else if( m_password==SEPTEMBER_2007)
			{
			//force if the file is older than on month
			if( status.m_mtime.GetYear() < 2007 ||
			status.m_mtime.GetYear() == 2007 && status.m_mtime.GetMonth() < 9 )
			bDownload = true;
			}
			}
			catch(...)
			{
			}
			}
			}
			*/
		}

		return bDownload;
	}



	
	string CUIACISHourly::GetOutputFilePath(int year, const string& ID)const
	{
		return GetDir(WORKING_DIR) + ToString(year) + "\\" + ID + ".csv";
	}

	string CUIACISHourly::GetOutputFilePath(int year, size_t m, const string& ID)const
	{
		return GetDir(WORKING_DIR) + ToString(year) + "\\" + FormatA("%02d", m + 1) + "\\" + ID + ".csv";
	}

	/*
	string CUIACISHourly::GetForecastFilePath( const string& stationID)const
	{
	string tmp;
	tmp.Format( "%sForecast\\%s.csv", GetWorkingDir(), stationID);
	return tmp;
	}
	*/
	bool CUIACISHourly::IsFileInclude(const string& filePath)const
	{
		bool bRep = false;

		//long stationID = CGSODStation::GetStationID(fileTitle);

		//CStdioFile file;
		//if( file.Open(filePath, CFile::modeRead) )
		//{
		//	ULONGLONG seekEnd = Max((ULONGLONG)0, (ULONGLONG)(file.GetLength()-150) );
		//	file.Seek(seekEnd, CFile::begin);

		//	//find the last line
		//	string line;
		//	string tmp;
		//	while( file.ReadString(tmp) )
		//		if( !tmp.empty() )
		//			line = tmp;

		//}

		//if( StationExist(fileTitle) )
		//{
		//	CGSODStation station;
		//	GetStationInformation(fileTitle, station);
		//
		//	short country = CCountrySelection::GetCountry(station.m_CTRY);
		//	ASSERT( country != -1);

		//	if( m_type.IsUsed(country) )
		//	{
		//		if( m_boundingBox.IsRectEmpty() ||
		//			m_boundingBox.PtInRect( station.GetCoord() ) )
		//			bRep=true;
		//	}
		//}

		return bRep;
	}

	ERMsg CUIACISHourly::CleanList(StringVector& fileList, CCallback& callback)const
	{
		ERMsg msg;

		callback.PushTask(GetString(IDS_CLEAN_LIST), fileList.size());
		//callback.SetNbStep(fileList.size());


		for (StringVector::iterator it = fileList.begin(); it != fileList.end() && msg;)
		{
			if (!IsFileInclude(*it))
				it = fileList.erase(it);
			else
				it++;

			msg += callback.StepIt();
		}

		callback.PopTask();

		return msg;
	}


	ERMsg CUIACISHourly::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;

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


	class CFindID
	{
	public:

		CFindID(long ID)
		{
			m_ID = ID;
		}

		bool operator ()(const CLocation& location) const { return m_ID == ToLong(location.m_ID); }

	protected:

		long m_ID;
	};

	void CUIACISHourly::GetStationInformation(const string& stationID, CLocation& station)const
	{
		CLocationVector::const_iterator it = std::find_if(m_stations.begin(), m_stations.end(), CFindID(ToLong(stationID)));
		if (it != m_stations.end())
			station = *it;
	}

	
	ERMsg CUIACISHourly::GetWeatherStation(const std::string& ID, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;


		string::size_type pos = 0;
		
		GetStationInformation(ID, station);
		station.m_name = station.m_name;
		station.m_ID;// += "H";//add a "H" for hourly data

		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = size_t(lastYear - firstYear + 1);
		station.CreateYears(firstYear, nbYears);


		//now extract data 
		for (size_t y = 0; y < nbYears&&msg; y++)
		{
			int year = firstYear + int(y);

			for (size_t m = 0; m < 12 && msg; m++)
			{

				string filePath = GetOutputFilePath(year, m, ID);
				if (FileExists(filePath))
					msg = ReadData(filePath, station[year], callback);
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

	ERMsg CUIACISHourly::ReadData(const string& filePath, CYear& dailyData, CCallback& callback)const
	{
		ERMsg msg;

		//enum{ DATE_TIME,YEAR,MONTH,DAY,DATA_QUALITY,MAX_TEMP,MAX_TEMP_FLAG,MIN_TEMP,MIN_TEMP_FLAG,MEAN_TEMP,MEAN_TEMP_FLAG,HEAT_DEG_DAYS,HEAT_DEG_DAYS_FLAG,COOL_DEG_DAYS,COOL_DEG_DAYS_FLAG,TOTAL_RAIN,TOTAL_RAIN_FLAG,TOTAL_SNOW,TOTAL_SNOW_FLAG,TOTAL_PRECIP,TOTAL_PRECIP_FLAG,SNOW_ON_GRND,SNOW_ON_GRND_FLAG,DIR_OF_MAX_GUST,DIR_OF_MAX_GUST_FLAG,SPD_OF_MAX_GUST,SPD_OF_MAX_GUST_FLAG,NB_DAILY_COLUMN};


		//ifStream file;
		//msg = file.open(filePath);

		//if( msg )
		//{
		//	//vector<int> members;

		//	size_t i=0;
		//	for(CSVIterator loop(file); loop!=CSVIterator(); ++loop, i++)
		//	{
		//		CTM TM(CTM::DAILY);
		//		CWeatherAccumulator stat(TM);
		//		ENSURE( loop.Header().size() == NB_DAILY_COLUMN );

		//		
		//		int year = stdString::ToInt((*loop)[YEAR]);
		//		int month = stdString::ToInt((*loop)[MONTH])-1;
		//		int day = stdString::ToInt((*loop)[DAY])-1;
		//		ASSERT( year>=m_firstYear && year<=m_lastYear);
		//		ASSERT( month>=0 && moBnth<12);
		//		ASSERT( day>=0 && day<GetNbDayPerMonth(year, month) );
		//		CTRef Tref(year,month,day);
		//
		//		if( ((*loop)[MIN_TEMP_FLAG].empty()||(*loop)[MIN_TEMP_FLAG]=="E") && !(*loop)[MIN_TEMP].empty() &&
		//			((*loop)[MAX_TEMP_FLAG].empty()||(*loop)[MAX_TEMP_FLAG]=="E") && !(*loop)[MAX_TEMP].empty())
		//		{
		//			float Tmin = ToFloat((*loop)[MIN_TEMP]);
		//			ASSERT(Tmin>=-70&&Tmin<=70);
		//			float Tmax = ToFloat((*loop)[MAX_TEMP]);
		//			ASSERT(Tmax>=-70&&Tmax<=70);
		//			stat.Add(Tref, H_TAIR, Tmin);
		//			stat.Add(Tref, H_TAIR, Tmax);
		//			//dailyData[m][d][TMIN] = Tmin;
		//			//dailyData[m][d][TMAX] = Tmax;
		//		}
		//		
		//		if(((*loop)[TOTAL_PRECIP_FLAG].empty()||(*loop)[TOTAL_PRECIP_FLAG]=="E"||(*loop)[TOTAL_PRECIP_FLAG]=="T") && !(*loop)[TOTAL_PRECIP].empty())
		//		{
		//			float prcp = ToFloat((*loop)[TOTAL_PRECIP]);
		//			ASSERT(prcp>=0&&prcp<1000);
		//			stat.Add(Tref, H_PRCP, prcp);
		//		}
		//			

		//		dailyData[Tref].SetData(stat);
		//	}//for all line
		//   }

		return msg;
	}

}