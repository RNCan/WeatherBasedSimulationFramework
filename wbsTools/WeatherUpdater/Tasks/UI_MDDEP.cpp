#include "StdAfx.h"
#include "UI_MDDEP.h"

#include "DailyStation.h"
#include "DailyDatabase.h"
#include "SYShowMessage.h"
#include "CommonRes.h"
#include "Resource.h"
#include "Registry.h"
#include "FileStamp.h"
#include "CSV.h"
#include "zenXml.h"
#include "UtilTime.h"
#include "SelectionDlg.h"
#include "EnvCanForecast.h"

using namespace std;
using namespace CFL;
using namespace stdString;
using namespace HOURLY_DATA;
using namespace UtilWWW;
using namespace GeoBasic;

static const GeoBasic::CGeoRect DEFAULT_BOUDINGBOX(-180, -90, 180, 90, GeoBasic::PRJ_WGS_84);


//*********************************************************************
const char* CUI_MDDEPDaily::ATTRIBUTE_NAME[NB_ATTRIBUTE]={"BoundingBox", "UpdateStationList", "ExtractSnow"};
const char* CUI_MDDEPDaily::CLASS_NAME = "MDDEP_Daily";
const char* CUI_MDDEPDaily::SERVER_NAME = "www.mddelcc.gouv.qc.ca";
const char* CUI_MDDEPDaily::SERVER_PATH = "climat/donnees/";


CUI_MDDEPDaily::CUI_MDDEPDaily(void)
{
	if( !IsRegister( GetClassID() ) )
	{
		InitClass();
	}

	Reset();
}

void CUI_MDDEPDaily::InitClass(const StringVector& option)
{
	GetParamClassInfo().m_className = GetString(IDS_SOURCENAME_MDDEP_DAILY);
	

	CUIWeather::InitClass(option);

	ASSERT( GetParameters().size() < I_NB_ATTRIBUTE);

	StringVector header(IDS_PROPERTIES_MDDEP_DAILY, "|;");
	ASSERT( header.size() == NB_ATTRIBUTE);
	

	//GetParameters().push_back( CParamDef(CParamDef::EDIT_BROWSE, ATTRIBUTE_NAME[0], header[0]) );
	GetParameters().push_back(CParamDef(CParamDef::COORD_RECT, ATTRIBUTE_NAME[0], header[0]));
	GetParameters().push_back( CParamDef(CParamDef::BOOL, ATTRIBUTE_NAME[1], header[1], "0") );
	GetParameters().push_back( CParamDef(CParamDef::BOOL, ATTRIBUTE_NAME[2], header[2], "0") );

}

CUI_MDDEPDaily::~CUI_MDDEPDaily(void)
{
}


CUI_MDDEPDaily::CUI_MDDEPDaily(const CUI_MDDEPDaily& in)
{
	operator=(in);
}

long CUI_MDDEPDaily::GetNbDay(const CTime& t)
{
	return GetNbDay(t.GetYear(), t.GetMonth()-1, t.GetDay()-1);
	//long(t.GetYear()*365+(t.GetMonth()-1)*30.42+(t.GetDay()-1));
}

long CUI_MDDEPDaily::GetNbDay(int y, int m, int d)
{
	ASSERT( m>=0 && m<12);
	ASSERT( d>=0 && d<31);

	return long(y*365+m*30.42+d);
}

void CUI_MDDEPDaily::Reset()
{
	CUIWeather::Reset();


	m_boundingBox = DEFAULT_BOUDINGBOX;
	m_bUpdateStationList=false;
	m_bExtractSnow=false;
}

CUI_MDDEPDaily& CUI_MDDEPDaily::operator =(const CUI_MDDEPDaily& in)
{
	if( &in != this)
	{
		CUIWeather::operator =(in);
		m_boundingBox = in.m_boundingBox;
		m_bUpdateStationList = in.m_bUpdateStationList;
		m_bExtractSnow= in.m_bExtractSnow;
	}

	return *this;
}

bool CUI_MDDEPDaily::operator ==(const CUI_MDDEPDaily& in)const
{
	bool bEqual = true;

	if( CUIWeather::operator !=(in) )bEqual = false;
	if (m_boundingBox != in.m_boundingBox)bEqual = false;
	if( m_bUpdateStationList != in.m_bUpdateStationList)bEqual = false;
	if( m_bExtractSnow != in.m_bExtractSnow)bEqual = false;
	
	return bEqual;
}

bool CUI_MDDEPDaily::operator !=(const CUI_MDDEPDaily& in)const
{
	return !operator ==(in);
}

string CUI_MDDEPDaily::GetValue(size_t type)const
{
	ERMsg msg;
	string str;
	
	ASSERT( NB_ATTRIBUTE == 3); 
	switch(type)
	{
	case I_BOUNDINGBOX: str = ToString(m_boundingBox); break;
	case I_FORCE_DOWNLOAD: str = ToString( m_bUpdateStationList); break;
	case I_EXTRACT_SNOW: str = ToString(m_bExtractSnow); break;
	default: str = CUIWeather::GetValue(type); break;
	}

	return str;
}

void CUI_MDDEPDaily::SetValue(size_t type, const string& str)
{
	ASSERT( NB_ATTRIBUTE == 3); 
	switch(type)
	{
	case I_FORCE_DOWNLOAD: m_bUpdateStationList = ToBool(str); break;
	case I_BOUNDINGBOX: m_boundingBox = stdString::ToValue<GeoBasic::CGeoRect>(str); m_boundingBox.SetPrjID(PRJ_WGS_84);  break;
	case I_EXTRACT_SNOW: m_bExtractSnow = ToBool(str); break;
	default: CUIWeather::SetValue(type, str); break;
	}

}


bool CUI_MDDEPDaily::Compare(const CParameterBase& in)const
{
	ASSERT(in.GetClassID() == GetClassID() );
	const CUI_MDDEPDaily& info = dynamic_cast<const CUI_MDDEPDaily&>(in);
	return operator==(info);
}

CParameterBase& CUI_MDDEPDaily::Assign(const CParameterBase& in)
{
	ASSERT(in.GetClassID() == GetClassID() );
	const CUI_MDDEPDaily& info = dynamic_cast<const CUI_MDDEPDaily&>(in);
	return operator=(info);
}

//************************************************************************************************************
//Load station definition list section

CTPeriod String2Period(string period)
{
	CTPeriod p;
	StringVector str(period, "-|");

	if (str.size() == 6)
	{
		int v[6] = { 0 };
		for (int i = 0; i < 6; i++)
			v[i] = ToInt(str[i]);

		p = CTPeriod(CTRef(v[0], v[1] - 1, v[2] - 1), CTRef(v[3], v[4] - 1, v[5] - 1));
	}

	return p;
}


ERMsg CUI_MDDEPDaily::GetStationList(CLocationMap& stationList, CFL::CCallback& callback)const
{
	ERMsg msg;


	//Interface attribute index to attribute index
	//www.mddelcc.gouv.qc.ca/climat/donnees/sommaire.asp?cle=7110072&mois=10&annee=2014
//climat/surveillance/station.asp
	static const char* URL_STATION = "climat/surveillance/station.asp";
	static const char* URL_INDEX = "climat/donnees/index.asp";

	callback.SetCurrentDescription(GetString(IDS_LOAD_STATION_LIST) );

	CInternetSessionPtr pSession;
	CHttpConnectionPtr pConnection;

	msg = GetHttpConnection( SERVER_NAME, pConnection, pSession);
	if( !msg)
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
			stationList[ID] = stationInfo;

			posBegin = source.find("helpOver", posBegin);
		}
	}

	//update first and last avalilable date
	msg = GetPageText(pConnection, URL_INDEX, source);
	if (msg)
	{

		//['Station :  Umiujaq',56.5439165,-76.5433171,8,
		string::size_type posBegin = source.find("['Station :");

		while (posBegin != string::npos)
		{
			string str = FindString(source, "['Station :", "<p>", posBegin); Trim(str); std::remove(str.begin(), str.end(), '\'');
			StringVector elem(str, ",");

			string ID = FindString(source, "<strong>Station : </strong>", "<br />", posBegin); Trim(ID);
			string begin = FindString(source, "permiere_date_disponible=", "&", posBegin); Trim(begin);
			string end = FindString(source, "derniere_date_disponible=", " >", posBegin); Trim(begin);


			CLocation stationInfo(elem[0], ID, stod(elem[1]), stod(elem[2]), stod(elem[3]));
			stationInfo.SetSSI("Period", begin + "|" + end);
			stationList[ID] = stationInfo;

			posBegin = source.find("['Station : ", posBegin);
		}
	}

	pConnection->Close();
	pSession->Close();

	callback.AddMessage(GetString(IDS_NB_STATIONS) + ToString(stationList.size()));

	return msg;
}
//
//ERMsg CUI_MDDEPDaily::UpdatePeriod(CLocationVector& stationList, CFL::CCallback& callback)const
//{
//	ERMsg msg;
//
//	static const char* URL = "climat/donnees/index.asp";
//
//	callback.SetCurrentDescription(GetString(IDS_LOAD_STATION_LIST));
//
//	CInternetSessionPtr pSession;
//	CHttpConnectionPtr pConnection;
//
//	msg = GetHttpConnection(SERVER_NAME, pConnection, pSession);
//	if (!msg)
//		return msg;
//
//	string source;
//	msg = GetPageText(pConnection, URL, source);
//	if (msg)
//	{
//		string::size_type posBegin = source.find("['Station :");
//
//		while (posBegin != string::npos)
//		{
//			string str = FindString(source, "Date d\'ouverture :", "<br / >", posBegin); Trim(period);
//			
//			CLocation stationInfo(name, ID, stod(latitude), stod(longitude), stod(altitude));
//			stationInfo.SetSSI("End", period);
//			stationList.push_back(stationInfo);
//
//			string::size_type posBegin = source.find("['Station :");
//		}
//	}
//
//
//
//	pConnection->Close();
//	pSession->Close();
//
//	callback.AddMessage(GetString(IDS_NB_STATIONS) + ToString(stationList.size()));
//
//	return msg;
//}


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

ERMsg CUI_MDDEPDaily::CopyStationDataPage(CHttpConnectionPtr& pConnection, const string& ID, int year, int m, const string& filePath)
{
	ERMsg msg;


	static const char pageDataFormat[] =
	{
		"climat/donnees/sommaire.asp?"
		"cle=%s&"
		"mois=%02d&"
		"annee=%4d"
	};

	string URL = FormatA(pageDataFormat, ID.c_str(), m + 1, year);
	
	string source;
	msg = GetPageText( pConnection, URL, source);
	if (msg)
	{
		ofStream file;

		msg = file.open(filePath);

		if (msg)
		{
			file << "Year,Month,Day,Tmax,Tmin,Rain,Snow,Prcp,SnwD" << endl;
			string::size_type posBegin = source.find("<tbody><tr class=", 0);
			//if(posBegin != string::npos)

			
			while (posBegin != string::npos)
			{
				file << year << "," << m+1;
				string str = FindString(source, "<td", "</td>", posBegin);
				file << "," << CleanStr(str);//add day

				for (int i = 0; i < 6; i++)
				{
					string str1 = CleanStr(FindString(source, "<td", "</td>", posBegin));
					string str2 = CleanStr(FindString(source, "<td", "</td>", posBegin));
					
					//C Cumulé	E Estimé	I Incomplet	Q Quantité Inconnue
					//D Douteux	F Forcé	K Estimé(krigeage)	T Trace

					if (str1 == "-" || !str2.empty())
						str1 = "-999.0";

					file << "," << str1;

					if (i == 1)//skip one extra val
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

ERMsg CUI_MDDEPDaily::DownloadStation( CHttpConnectionPtr& pConnection, const CLocation& info, CFL::CCallback& callback)
{
	ERMsg msg;

	string workingDir = GetWorkingDir();
	
	int nbFilesToDownload=0;
	int nbYear = m_lastYear-m_firstYear+1;
	CTRef today = CTRef::GetCurrentTRef();

	vector<array<bool, 12>> bNeedDownload(nbYear);

	CTPeriod period = String2Period(info.GetSSI("Period"));
	if (!period.IsInit())
		return msg;
	
	//#pragma omp parallel for num_threads(3)
	for(int y=0; y<nbYear&&msg; y++)
	{
		int year = m_firstYear + y;
		//int begin = stoi(info.GetSSI("Begin"));
		//if (year >= begin)
		//{
		int monthMax = year < today.m_year ? 12 : today.m_month+1;
		for (int m = 0; m < monthMax&&msg; m++)
		{
			if (period.IsInside(CTRef(year, m, FIRST_DAY)) || period.IsInside(CTRef(year, m, LAST_DAY)))
			{
				string filePath = GetOutputFilePath(year, m, info.m_ID);
				bNeedDownload[y][m] = NeedDownload(filePath, info, year, m);
				nbFilesToDownload += bNeedDownload[y][m] ? 1 : 0;
			}
		}
	}
	

	callback.SetCurrentDescription(info.m_name);
	callback.SetNbStep(nbFilesToDownload);
	

	for(int y=0; y<nbYear&&msg; y++)
	{
		int year = m_firstYear + y;
		
		int monthMax = year < today.m_year ? 12 : today.m_month + 1;
		for (int m = 0; m < monthMax&&msg; m++)
		{
			if( bNeedDownload[y][m] )
			{
				string filePath = GetOutputFilePath(year, m, info.m_ID);;
				CreateMultipleDir(GetPath(filePath));

				msg += CopyStationDataPage(pConnection, info.m_ID, year, m, filePath);
				msg += callback.StepIt();
			}
		}
	}

	

	return msg;
}

bool CUI_MDDEPDaily::NeedDownload(const string& filePath, const CLocation& info, int year, int m)const
{
	bool bDownload = true;

	if( !m_bUpdateStationList)
	{
		CFileStamp fileStamp(filePath);
		CTime lastUpdate = fileStamp.m_time;
		if( lastUpdate.GetTime() > 0 )
		{
			int nbDays = GetNbDay(lastUpdate) - GetNbDay(year, m, 0);
				
			if(nbDays>(365+182))//six month after the end of the year
				bDownload = false;
		}
	}

	return bDownload;
}



ERMsg CUI_MDDEPDaily::CleanStationList(CLocationVector& stationList, CFL::CCallback& callback)const
{
	ERMsg msg;

	if (!m_boundingBox.IsRectEmpty() && m_boundingBox != DEFAULT_BOUDINGBOX)
	{
		callback.SetCurrentDescription(GetString(IDS_CLEAN_LIST));
		callback.SetNbStep(stationList.size());

		for (CLocationVector::iterator it = stationList.begin(); it != stationList.end() && msg;)
		{
			if (m_boundingBox.PtInRect(*it))
			{
				it++;
			}
			else
			{
				it = stationList.erase(it);
			}

			msg += callback.StepIt();
		}
	}

	return msg;
}
//*************************************************************************************************

ERMsg CUI_MDDEPDaily::Execute(CFL::CCallback& callback)
{
	ERMsg msg;

	string workingDir = GetWorkingDir();
	CreateMultipleDir(workingDir);

	callback.AddMessage(GetString(IDS_UPDATE_DIR));
	callback.AddMessage(workingDir, 1);
	callback.AddMessage(GetString(IDS_UPDATE_FROM));
	callback.AddMessage(SERVER_NAME, 1);
	callback.AddMessage("");


	//Getlocal station list
	if (FileExists(GetStationListFilePath()))
		msg = m_stations.Load(GetStationListFilePath());
		
	//Get remote station list
	if (msg && m_bUpdateStationList)
	{
		msg = GetStationList(m_stations, callback);

		//save event if append an error
		msg = m_stations.Save(GetStationListFilePath());
	}

	CLocationVector stationList; 
	stationList.reserve(m_stations.size());
	for (CLocationMap::const_iterator it = m_stations.begin(); it != m_stations.end(); it++)
		stationList.push_back(it->second);

	if (msg)
		msg = CleanStationList(stationList, callback);

	if (!msg)
		return msg;



	callback.AddTask(stationList.size());



	int nbRun = 0;
	size_t curI = 0;

	while (curI<stationList.size() && msg)
	{
		nbRun++;

		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;

		msg = GetHttpConnection(SERVER_NAME, pConnection, pSession);

		if (msg)
		{
			TRY
			{
				for (size_t i = curI; i<stationList.size() && msg; i++)
				{
					msg = DownloadStation(pConnection, stationList[i], callback);
					if (msg)
					{
						curI++;
						nbRun = 0;
						msg += callback.StepIt(0);
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
					Sleep(1000);//wait 1 sec
				}
				}

			//clean connection
			pConnection->Close();
			pSession->Close();
		}
	}



	return msg;
}

//****************************************************************************************************
				
ERMsg CUI_MDDEPDaily::PreProcess(CFL::CCallback& callback)
{
	ERMsg msg = m_stations.Load(GetStationListFilePath());
	return msg;
}

string CUI_MDDEPDaily::GetOutputFilePath(int year, int month, const string& stationName)const
{
	return GetWorkingDir() + ToString(year) + "\\" + FormatA("%02d", month+1) + "\\" + stationName + ".csv";
}


ERMsg CUI_MDDEPDaily::GetStationList(StringVector& stationList, CFL::CCallback& callback)
{
	ERMsg msg;

	for(CLocationMap::const_iterator it=m_stations.begin(); it!=m_stations.end(); it++) 
	{
		const CLocation& station = (const CLocation& )it->second;
		CTPeriod period = String2Period(station.GetSSI("Period"));
		if (period.Begin().GetYear() <= m_lastYear && period.End().GetYear() >= m_firstYear)
		{
			if (m_boundingBox.IsRectEmpty() ||
				m_boundingBox == DEFAULT_BOUDINGBOX ||
				m_boundingBox.PtInRect(station))
			{
				stationList.push_back(it->second.m_ID);
			}
		}
	}

	return msg;
}

std::string CUI_MDDEPDaily::GetStationListFilePath()const
{ 
	return GetWorkingDir() + "DailyStationsList.xml"; 
}
	
void CUI_MDDEPDaily::GetStationInformation(const string& ID, CLocation& station)const
{

	CLocationMap::const_iterator it = m_stations.find(ID);
	if( it!=m_stations.end())
		station = it->second;
}

static std::string TraitFileName(std::string name)
{
	size_t begin = name.find('(');
	size_t end = name.find(')');
	if( begin!=std::string::npos && end!=std::string::npos)
		name.erase(begin, end);

	std::replace(name.begin(), name.end(), ',', '-');
	std::replace(name.begin(), name.end(), ';', '-');
	stdString::UppercaseFirstLetter(name);
	Trim(name);

	return name;
}

ERMsg CUI_MDDEPDaily::GetStation(const string& ID, CDailyStation& station, CFL::CCallback& callback)
{
	ERMsg msg;
	
	GetStationInformation(ID, station );
	station.m_name = TraitFileName(station.m_name);

	int nbYears = m_lastYear-m_firstYear+1;
	station.CreateYears(m_firstYear, nbYears);

	CTRef today = CTRef::GetCurrentTRef();

	//now extract data 
	for(int y=0; y<nbYears&&msg; y++)
	{
		int year = m_firstYear + y;
		int nbMonth = year < today.m_year ? 12 : today.m_month + 1;
		for (int m = 0; m < nbMonth&&msg; m++)
		{
			string filePath = GetOutputFilePath(year, m, ID);
			if (CFL::FileExists(filePath))
			{
				msg = ReadData(filePath, station[year]);
			}
		}
	}

	
	if( msg )
	{
		//verify station is valid
		if( station.HaveData() )
		{
			msg = station.IsValid();
		}
		else
		{
			//msg.ajoute( stdString::GetString(IDS_NO_WEATHER) + station.m_name + " [" + station.m_ID + "]" );
		}
	}

	return msg;
}


//"Legend"
//"[Empty]","No Data Available"
//"M","Missing"
//"E","Estimated"
//"A","Accumulated"
//"C","Precipitation Occurred; Amount Uncertain"
//"L","Precipitation May or May Not Have Occurred"
//"F","Accumulated and Estimated"
//"N","Temperature Missing but Known to be > 0"
//"Y","Temperature Missing but Known to be < 0"
//"S","More Than One Occurrence"
//"T","Trace"
//"*","Data for this day has undergone only preliminary quality checking"
ERMsg CUI_MDDEPDaily::ReadData(const string& filePath, CYear& dailyData)const
{
	ERMsg msg;

	enum{ YEAR,MONTH,DAY,MAX_TEMP,MIN_TEMP,TOTAL_RAIN,TOTAL_SNOW,TOTAL_PRECIP,SNOW_ON_GRND,NB_DAILY_COLUMN};

	//open file
	ifStream file;
	msg = file.open(filePath);

	if( msg )
	{
		size_t i=0;
		for(CSVIterator loop(file, ",", true, true); loop!=CSVIterator(); ++loop, i++)
		{
			//CTM TM(CTM::DAILY);
			//CWeatherAccumulator stat(TM);
			ENSURE( loop.Header().size() == NB_DAILY_COLUMN );

			
			int year = stdString::ToInt((*loop)[YEAR]);
			int month = stdString::ToInt((*loop)[MONTH])-1;
			int day = stdString::ToInt((*loop)[DAY])-1;
			ASSERT( year>=m_firstYear && year<=m_lastYear);
			ASSERT( month>=0 && month<12);
			ASSERT( day>=0 && day<CFL::GetNbDayPerMonth(year, month) );
			CTRef Tref(year,month,day);
	

			float Tmin = ToFloat((*loop)[MIN_TEMP]);
			float Tmax = ToFloat((*loop)[MAX_TEMP]);
				

			if (Tmin>-999 && Tmax>-999)
			{
				ASSERT(Tmin >= -70 && Tmin <= 70);
				ASSERT(Tmax >= -70 && Tmax <= 70);
				dailyData[Tref][H_TAIR] = (Tmin+Tmax)/2;
				dailyData[Tref][H_TRNG] = Tmax - Tmin;
				//stat.Add(Tref, H_TAIR, Tmin);
				//stat.Add(Tref, H_TAIR, Tmax);
			}
			
			float prcp = ToFloat((*loop)[TOTAL_PRECIP]);
			if (prcp>-999)
			{
				ASSERT(prcp>=0&&prcp<1000);
				//stat.Add(Tref, H_PRCP, prcp);
				dailyData[Tref][H_PRCP] = prcp;
			}
				
			if (m_bExtractSnow)
			{
				float snow = ToFloat((*loop)[TOTAL_SNOW]);
				//if (((*loop)[TOTAL_SNOW_FLAG].empty() || (*loop)[TOTAL_SNOW_FLAG] == "E" || (*loop)[TOTAL_SNOW_FLAG] == "T") && !(*loop)[TOTAL_SNOW].empty())
				if (snow>-999)
				{
					ASSERT(snow >= 0 && snow < 1000);
					//stat.Add(Tref, H_SNOW, snow);
					dailyData[Tref][H_SNOW] = snow;
				}

				float sndh = ToFloat((*loop)[SNOW_ON_GRND]);
				//if ( (*loop)[SNOW_ON_GRND_FLAG].empty() )
				if (sndh>-999)
				{
					ASSERT(sndh >= 0 && sndh < 1000);
					//stat.Add(Tref, H_SNDH, sndh);
					dailyData[Tref][H_SNDH] = sndh;
				}
			}


			//dailyData[Tref].SetData(stat);
		}//for all line
    }
    
	return msg;
}

