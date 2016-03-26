#include "StdAfx.h"
#include "UIACIS.h"
#include "DailyStation.h"
#include "DailyDatabase.h"
#include "SYShowMessage.h"
#include "CommonRes.h"
#include "Resource.h"
#include "Registry.h"
#include "FileStamp.h"
#include "CSV.h"

using namespace std;
using namespace CFL;
using namespace stdString;
using namespace HOURLY_DATA;
using namespace UtilWWW;
//*********************************************************************

//INTERNET_FLAG_RELOAD  INTERNET_FLAG_NO_COOKIES | 
static const DWORD FLAGS = INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_PRAGMA_NOCACHE;

//*********************************************************************
const char* CUIACIS::ATTRIBUTE_NAME[NB_ATTRIBUTE]={"UserName", "Password", "Type"};
const char* CUIACIS::CLASS_NAME = "ACIS";
const char* CUIACIS::SERVER_NAME = "www.agric.gov.ab.ca";
//www.
CUIACIS::CUIACIS(void)
{
	if( !IsRegister( GetClassID() ) )
	{
		InitClass();
	}

	Reset();
}

void CUIACIS::InitClass(const StringVector& option)
{
	GetParamClassInfo().m_className = GetString( IDS_SOURCENAME_ACIS );

	CUIWeather::InitClass(option);

	ASSERT( GetParameters().size() < I_NB_ATTRIBUTE);

	StringVector header(IDS_PROPERTIES_ACIS, "|;");
	ASSERT( header.size() == NB_ATTRIBUTE);
	StringVector typeList("Hourly|Daily", "|");

	GetParameters().push_back( CParamDef(CParamDef::EDIT, ATTRIBUTE_NAME[0], header[0]) );
	GetParameters().push_back( CParamDef(CParamDef::PASSWORD, ATTRIBUTE_NAME[1], header[1]) );
	GetParameters().push_back( CParamDef(CParamDef::COMBO, CParamDef::BY_NUMBER, ATTRIBUTE_NAME[2], header[2], typeList, "0" ));

}

CUIACIS::~CUIACIS(void)
{
}


CUIACIS::CUIACIS(const CUIACIS& in)
{
	operator=(in);
}

long CUIACIS::GetNbDay(const CTime& t)
{
	return GetNbDay(t.GetYear(), t.GetMonth()-1, t.GetDay()-1);
}

long CUIACIS::GetNbDay(int y, int m, int d)
{
	ASSERT( m>=0 && m<12);
	ASSERT( d>=0 && d<31);

	return long(y*365+m*30.42+d);
}

void CUIACIS::Reset()
{
	CUIWeather::Reset();

	m_type=0;
	m_password.clear();
	m_userName.clear();

	m_nbDownload=0;


	m_nbDays = GetNbDay(CTime::GetCurrentTime());
}

CUIACIS& CUIACIS::operator =(const CUIACIS& in)
{
	if( &in != this)
	{
		CUIWeather::operator =(in);
		m_password = in.m_password;
		m_userName= in.m_userName;
		m_type = in.m_type;
	}

	return *this;
}

bool CUIACIS::operator ==(const CUIACIS& in)const
{
	bool bEqual = true;

	if( CUIWeather::operator !=(in) )bEqual = false;
	if( m_password != in.m_password)bEqual = false;
	if( m_userName != in.m_userName)bEqual = false;
	if( m_type != in.m_type)bEqual = false;	
	return bEqual;
}

bool CUIACIS::operator !=(const CUIACIS& in)const
{
	return !operator ==(in);
}

string CUIACIS::GetValue(size_t type)const
{
	ERMsg msg;
	string str;
	
	ASSERT( NB_ATTRIBUTE == 3); 
	switch(type)
	{
	case I_USER_NAME: str = m_userName.c_str(); break;
	case I_PASSWORD: str = m_password.c_str(); break;
	case I_TYPE: str = ToString(m_type); break;
	default: str = CUIWeather::GetValue(type); break;
	}

	return str;
}

void CUIACIS::SetValue(size_t type, const string& str)
{
	ASSERT( NB_ATTRIBUTE == 3); 
	switch(type)
	{
	case I_USER_NAME: m_userName= str; break;
	case I_PASSWORD: m_password = str; break;
	case I_TYPE: m_type = ToInt(str); break;
	default: CUIWeather::SetValue(type, str); break;
	}

}


bool CUIACIS::Compare(const CParameterBase& in)const
{
	ASSERT(in.GetClassID() == GetClassID() );
	const CUIACIS& info = dynamic_cast<const CUIACIS&>(in);
	return operator==(info);
}

CParameterBase& CUIACIS::Assign(const CParameterBase& in)
{
	ASSERT(in.GetClassID() == GetClassID() );
	const CUIACIS& info = dynamic_cast<const CUIACIS&>(in);
	return operator=(info);
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

	
ERMsg CUIACIS::GetStationList(CLocationVector& stationList, CFL::CCallback& callback)const
{
	ERMsg msg;

	CInternetSessionPtr pSession;
	CHttpConnectionPtr pConnection;

	msg = GetHttpConnection( SERVER_NAME, pConnection, pSession);
	if( !msg)
		return msg;

	callback.SetCurrentDescription(GetString(IDS_LOAD_STATION_LIST) );

	//Get cookie and page list
	string source;
	msg = GetPageText( pConnection, "acis/alberta-weather-data-viewer.jsp", source);
	if(msg)
	{

		vector<string> listID;
		string::size_type posBegin = source.find( "Select Station(s):");
		//int posEnd = posBegin;
		while(msg&&posBegin!=string::npos)
		{
			string ID = FindString(source, "<option value=", ">", posBegin);
			//string Name = FindString(source, "</option>", posBegin);

			if( ID != "-1" )
			{
				listID.push_back(ID);

				posBegin = source.find( "<option value=", posBegin);
				msg+=callback.StepIt(0);
			}
		}

		callback.SetNbStep(listID.size());
		for(size_t i=0; i<listID.size()&&msg; i++)
		{
			string metadata;
			msg = GetPageText( pConnection, ("acis/station/metadata?resource=complete&station="+listID[i]).c_str(), metadata);
			string::size_type metaPosBegin = metadata.find("<name>");
			if(msg && metaPosBegin!=string::npos)
			{
				//CLocation stationInfo;
				CLocation stationInfo;
				stationInfo.m_name = TrimConst(FindString(metadata, "<name>", "</name>", metaPosBegin));
				stationInfo.m_ID = TrimConst(listID[i]);
				stationInfo.m_lat = ToDouble(FindString(metadata, "<latitude>", "</latitude>", metaPosBegin));
				stationInfo.m_lon = ToDouble(FindString(metadata, "<longitude>", "</longitude>", metaPosBegin)); 
				stationInfo.m_alt = ToDouble(FindString(metadata, "<elevation>", "</elevation>", metaPosBegin));
				string ECID = FindString(metadata, "<EC>", "</EC>", metaPosBegin);
				if( !ECID.empty() )
					stationInfo.SetSSI( "ClimateID", ECID);
					
				stationList.push_back(stationInfo);
				msg+=callback.StepIt();
			}
		}
	}

	pConnection->Close();
	pSession->Close();
	


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

string GetSessiosnID(CHttpConnectionPtr& pConnection)
{
	string sessionID;
	string page;
	
	if (GetPageText( pConnection, "acis/alberta-weather-data-viewer.jsp", page))
	{
			
		string::size_type posBegin = page.find("getSessionId()");
		if(posBegin>=0)
			sessionID = FindString( page, "return \"", "\";", posBegin);
	}

	return sessionID;
}

ERMsg CUIACIS::Execute(CFL::CCallback& callback)
{
	ERMsg msg;

	string workingDir = GetWorkingDir();

	callback.AddMessage(GetString(IDS_UPDATE_DIR ));
	callback.AddMessage(workingDir, 1);
	callback.AddMessage(GetString(IDS_UPDATE_FROM ) );
	callback.AddMessage(SERVER_NAME, 1 );
	callback.AddMessage("");


	if( FileExists(GetStationListFilePath()) )
	{
		msg = m_stations.Load(GetStationListFilePath());
	}
	else
	{
		CreateMultipleDir(GetPath(GetStationListFilePath()));
		msg = GetStationList(m_stations, callback);
	
		if(msg)
			msg = m_stations.Save(GetStationListFilePath());
	}

	if( !msg )
		return msg;


	if (m_type == HOURLY_WEATHER)
		msg = DownloadStationHourly(callback);
	else
		msg = DownloadStationDaily(callback);
	
	//get a sessionID
	//CInternetSessionPtr pSession;
	//CHttpConnectionPtr pConnection;
	//msg = GetHttpConnection(SERVER_NAME, pConnection, pSession);
	//if(msg)
	//{
	//	string sessionID = GetSessiosnID(pConnection);
	//	for (size_t i = 0; i < m_stations.size() && msg; i++)
	//	{
	//		if (m_type == HOURLY_WEATHER)
	//			msg = DownloadStationHourly(pConnection, sessionID, m_stations[i], callback);
	//		else
	//			msg = DownloadStationDaily(pConnection, sessionID, m_stations[i], callback);
	//	}
	//	
	//	//clean connection
	//	pConnection->Close();
	//	pSession->Close();
	//}

	return msg;
}

int ClearIECache()
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


CTRef GetTRef(__time64_t t, CTM TM=CTM(CTM::DAILY))
{
	CTRef TRef;
	
	if (t > 0)
	{
		struct tm *theTime = _localtime64(&t);
		TRef = CTRef(1900 + theTime->tm_year, theTime->tm_mon, theTime->tm_mday - 1, theTime->tm_hour, TM);
	}

	return TRef;
}

ERMsg CUIACIS::DownloadStationHourly( CFL::CCallback& callback)
{
	ERMsg msg;
	

	CTRef today = CTRef::GetCurrentTRef();
	string workingDir = GetWorkingDir();
	int nbFilesToDownload = 0;
	size_t nbYears = m_lastYear - m_firstYear + 1;


	callback.SetCurrentDescription("Clear stations list...");
	callback.SetNbStep(m_stations.size()*nbYears*12);

	vector<vector<array<bool, 12>>> bNeedDownload(m_stations.size());
	for (size_t i = 0; i < m_stations.size() && msg; i++)
	{
		bNeedDownload[i].resize(nbYears);
		for (size_t y = 0; y < nbYears&&msg; y++)
		{
			int year = m_firstYear + int(y);

			size_t nbm = year == today.GetYear() ? today.GetMonth()+1: 12;
			for (size_t m = 0; m < nbm && msg; m++)
			{
				string filePath = GetOutputFilePath(year, m, m_stations[i].m_ID);
				CTRef TRef = GetTRef( CFL::GetFileStamp(filePath) );
				
				bNeedDownload[i][y][m] = !TRef.IsInit() || today - TRef < 2; //let 2 days to update the data
				nbFilesToDownload += bNeedDownload[i][y][m] ? 1 : 0;
				msg += callback.StepIt();
			}
		}
	}

	CInternetSessionPtr pSession;
	CHttpConnectionPtr pConnection;
	msg = GetHttpConnection(SERVER_NAME, pConnection, pSession, FLAGS);
	if (!msg)
		return msg;
	
	//une protection empêche de charger plus de fichier
	string sessionID = GetSessiosnID(pConnection);
	int nbDownload = 0;
	int currentNbDownload = 0;

	callback.SetCurrentDescription("Download data");
	callback.SetNbStep(nbFilesToDownload);
	for (size_t i = 0; i < m_stations.size() && msg; i++)
	{
		if (m_stations[i].GetSSI("ClimateID").substr(0, 3) == "999")
		{
			for (size_t y = 0; y < nbYears&&msg; y++)
			{
				int year = m_firstYear + int(y);
				for (size_t m = 0; m < 12 && msg; m++)
				{
					if (bNeedDownload[i][y][m])
					{

						string filePath = GetOutputFilePath(year, m, m_stations[i].m_ID);
						CreateMultipleDir(GetPath(filePath));

						string pageURL = FormatA(PageDataFormatH, m_stations[i].m_ID.c_str(), year, m + 1, 1, year, m + 1, CFL::GetNbDayPerMonth(year, m), sessionID.c_str());

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

								callback.SetCurrentDescription("Waiting");
								callback.SetNbStep(600);
								for (size_t s = 0; s < 600 && msg; s++)
								{
									Sleep(100);
									msg += callback.StepIt();
								}


								nbDownload++;
								currentNbDownload++;
								
								//if (nbDownload == 6)
								//{
								//	//clean connection
								//	pConnection->Close();
								//	pSession->Close();

								//	msg = GetHttpConnection(SERVER_NAME, pConnection, pSession, FLAGS);
								//	if (!msg)
								//		return msg;

								//	sessionID = GetSessiosnID(pConnection);
								//}
							}
							else
							{
								msg.ajoute(source);
							}

							if (msg && currentNbDownload==8)
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
								callback.SetCurrentDescription("Waiting...");
								callback.SetNbStep(5*600);
								for (size_t s = 0; s < 5*600 && msg; s++)
								{
									Sleep(100);
									msg += callback.StepIt();
								}
								
								msg = GetHttpConnection(SERVER_NAME, pConnection, pSession, FLAGS);
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
	

	return msg;
}

ERMsg CUIACIS::DownloadStationDaily(/*CHttpConnectionPtr& pConnection, string sessionID, const CLocation& location,*/ CFL::CCallback& callback)
{
	ERMsg msg;

	//string workingDir = GetWorkingDir();

	//int nbFilesToDownload = 0;
	//int nbYear = m_lastYear - m_firstYear + 1;

	//vector<bool> bNeedDownload(nbYear);

	//for (size_t y = 0; y<nbYear&&msg; y++)
	//{
	//	int year = m_firstYear + int(y);

	//	string filePath = GetOutputFilePath(year, m_stations[i].m_ID);
	//	bNeedDownload[y] = !CFL::FileExists(filePath);
	//	nbFilesToDownload += bNeedDownload[y] ? 1 : 0;
	//}

	//callback.SetCurrentDescription("Download data");
	//callback.SetNbStep(nbFilesToDownload);

	//for (size_t y = 0; y<nbYear&&msg; y++)
	//{
	//	int year = m_firstYear + int(y);
	//	if (bNeedDownload[y])
	//	{
	//		string filePath = GetOutputFilePath(year, m_stations[i].m_ID);
	//		CreateMultipleDir(GetPath(filePath));

	//		string pageURL = FormatA(PageDataFormatD, m_stations[i].m_ID.c_str(), year, 1, 1, year, 12, 31, sessionID.c_str());
	//		string source;
	//		msg = GetPageText(pConnection, pageURL, source);
	//		if (msg)
	//		{
	//			if (source.find("An Error Has Occurred") == string::npos && source.find("Page Not Found") == string::npos)
	//			{
	//				ofStream file;
	//				msg = file.open(filePath);
	//				if (msg)
	//				{
	//					file << source;
	//					file.close();
	//				}
	//			}
	//		}

	//			//http://www.agriculture.alberta.ca/acis/weather-data/timeseries?stations=859927&elements=PRA,PRCIP,ATAM,ATX,ATN,HUAM,WSAM,WDAM&startdate=20140225&enddate=20140311&interval=daily&format=csv&session=gi0Q54-ytZwK1deOsn4Hauu&precipunit=mm&comment=true

	//			//if( msg)
	//			//AddToStat(year);

	//		msg += callback.StepIt();
	//		
	//	}
	//}

	return msg;
}

bool CUIACIS::IsValid(const CLocation& info, short y, short m, const string& filePath )const
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


	
ERMsg CUIACIS::PreProcess(CFL::CCallback& callback)
{

	ERMsg msg = m_stations.Load(GetStationListFilePath());
	msg += m_stations.IsValid();

	return msg;
}

string CUIACIS::GetOutputFilePath( int year, const string& ID)const
{
	return GetWorkingDir() + ToString(year) + "\\" + ID + ".csv";
}

string CUIACIS::GetOutputFilePath(int year, size_t m, const string& ID)const
{
	return GetWorkingDir() + ToString(year) + "\\" + FormatA("%02d", m + 1) + "\\" + ID + ".csv";
}

/*
string CUIACIS::GetForecastFilePath( const string& stationID)const
{
	string tmp;
	tmp.Format( "%sForecast\\%s.csv", GetWorkingDir(), stationID);
	return tmp;
}
*/
bool CUIACIS::IsFileInclude(const string& filePath)const
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

ERMsg CUIACIS::CleanList(StringVector& fileList, CFL::CCallback& callback)const
{
	ERMsg msg;

	callback.SetCurrentDescription(GetString(IDS_CLEAN_LIST));
	callback.SetNbStep(fileList.size());
	

	for (StringVector::iterator it = fileList.begin(); it != fileList.end() && msg;)
	{
		if (!IsFileInclude(*it))
			it = fileList.erase(it);
		else
			it++;

		msg += callback.StepIt();
	}

	return msg;
}


ERMsg CUIACIS::GetStationList(StringVector& stationList, CFL::CCallback& callback)
{
	ERMsg msg; 

	for(int i=0; i<m_stations.size(); i++)
	{
		stationList.push_back(m_stations[i].m_ID);
	}

	return msg;
}


class CFindID
{
public:

	CFindID(long ID)
	{
		m_ID=ID;
	}

	bool operator ()( const CLocation& location ) const { return m_ID==ToLong(location.m_ID); }

protected:

	long m_ID;
};

void CUIACIS::GetStationInformation(const string& stationID, CLocation& station)const
{
	CLocationVector::const_iterator it = std::find_if( m_stations.begin(), m_stations.end(), CFindID(ToLong(stationID)) );
	if( it!=m_stations.end() )
		station = *it;
}

ERMsg CUIACIS::GetStation(const string& str, CDailyStation& station, CFL::CCallback& callback)
{
	return GetWeatherStation(str, CTM(CTM::DAILY), station, callback);
}

ERMsg CUIACIS::GetWeatherStation(const std::string& ID, CTM TM, CWeatherStation& station, CFL::CCallback& callback)
{
	ERMsg msg;


	string::size_type pos = 0;
	//string prov = Tokenize(stationName, "\\", pos);
	//string stationID = Tokenize(stationName, "\\", pos);
	//__int64 ID = ToValue<__int64>(stationID);

	GetStationInformation(ID, station);
	station.m_name = station.m_name;
	station.m_ID;// += "H";//add a "H" for hourly data

	size_t nbYears = size_t(m_lastYear - m_firstYear + 1);
	station.CreateYears(m_firstYear, nbYears);


	//now extract data 
	for (size_t y = 0; y<nbYears&&msg; y++)
	{
		int year = m_firstYear + int(y);

		for (size_t m = 0; m < 12&&msg; m++)
		{
		
			string filePath = GetOutputFilePath(year, m, ID);
			if (CFL::FileExists(filePath))
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

ERMsg CUIACIS::ReadData(const string& filePath, CYear& dailyData, CFL::CCallback& callback)const
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
	//		ASSERT( month>=0 && month<12);
	//		ASSERT( day>=0 && day<CFL::GetNbDayPerMonth(year, month) );
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

