#include "StdAfx.h"
#include "UIWunderground.h"

#include "DailyStation.h"
#include "DailyDatabase.h"
#include "ERMsg.h"
#include "CommonRes.h"
#include "Resource.h"
#include "SelectionDlg.h"



//to update data in csv file
//http://www.wunderground.com/weatherstation/WXDailyHistory.asp?ID=IBR880152&graphspan=custom&month=3&day=11&year=2009&monthend=5&dayend=12&yearend=2009&format=1

//pour la liste des stations actives:
//http://www.wunderground.com/weatherstation/ListStations.asp?showall=&start=20&selectedState=

using namespace CFL;
//using namespace DAILY_DATA;
using namespace UtilWWW;
using namespace std; using namespace stdString; using namespace CFL;

const string CWUCountrySelection::COUNTRAY_NAME[NB_TAG]=
{
  "Afghanistan", 
  "Albania", 
  "Alberta", 
  "Algeria", 
  "Angola", 
  "Anguilla", 
  "Argentina", 
  "Armenia", 
  "Australia", 
  "Austria", 
  "Azerbaijan", 
  "Bahamas", 
  "Bahrain", 
  "Bangladesh", 
  "Barbados", 
  "Beijing", 
  "Belarus", 
  "Belgium", 
  "Belize", 
  "Benin", 
  "Bermuda", 
  "Bolivia", 
  "Bosnia", 
  "Botswana", 
  "Bouvet Island", 
  "Brazil", 
  "British Columbia", 
  "British Indian Ocean", 
  "British Virgin Islands", 
  "Brunei", 
  "Bulgaria", 
  "Burkina Faso", 
  "Burma/Myanmar", 
  "Burundi", 
  "Cambodia", 
  "Cameroon", 
  "Canada", 
  "Canary Islands", 
  "Canton Island", 
  "Cape Verde", 
  "Capital Territory", 
  "Caroline Islands", 
  "Cayman Island", 
  "Central African Republic", 
  "Chad", 
  "Cheng-Du", 
  "Chile", 
  "China", 
  "Colombia", 
  "Comoros", 
  "Congo", 
  "Cook Islands", 
  "Costa Rica", 
  "Croatia", 
  "Cuba", 
  "Cyprus", 
  "Czech Republic", 
  "Democratic Yemen", 
  "Denmark", 
  "Djibouti", 
  "Dominica", 
  "Dominican Republic", 
  "East Timor", 
  "Ecuador", 
  "Egypt", 
  "El Salvador", 
  "Equatorial Guinea", 
  "Eritrea", 
  "Estonia", 
  "Ethiopia", 
  "Falkland Islands", 
  "Fiji", 
  "Finland", 
  "France", 
  "French Guiana", 
  "French Polynesia", 
  "Gabon", 
  "Gambia", 
  "Germany", 
  "Ghana", 
  "Gibraltar", 
  "Greece", 
  "Greenland", 
  "Grenada", 
  "Guam", 
  "Guang-Zhou", 
  "Guatemala", 
  "Guinea", 
  "Guinea-Bissau", 
  "Guyana", 
  "Haiti", 
  "Han-Kou", 
  "Hawaii", 
  "Honduras", 
  "Hong Kong", 
  "Hungary", 
  "Iceland", 
  "India", 
  "Indonesia", 
  "Iran", 
  "Iraq", 
  "Ireland", 
  "Israel", 
  "Italy", 
  "Ivory Coast", 
  "Jamaica", 
  "Japan", 
  "Jordan", 
  "Kazakhstan", 
  "Kenya", 
  "Kiribati", 
  "Kuwait", 
  "Kyrgyzstan", 
  "Lan-Zhou", 
  "Lao Peoples Republic", 
  "Latvia", 
  "Lebanon", 
  "Lesotho", 
  "Liberia", 
  "Libya", 
  "Luxembourg", 
  "Macao", 
  "Macedonia", 
  "Madagascar", 
  "Madeira Islands", 
  "Malawi", 
  "Malaysia", 
  "Maldives", 
  "Mali", 
  "Malta", 
  "Manitoba", 
  "Mariana Islands", 
  "Marshall Islands", 
  "Martinique", 
  "Maryland", 
  "Mauritania", 
  "Mauritius", 
  "Mexico", 
  "Mongolia", 
  "Montana", 
  "Morocco", 
  "Mozambique", 
  "Namibia", 
  "Nauru", 
  "Nepal", 
  "Netherlands", 
  "New Brunswick", 
  "New Caledonia", 
  "New South Wales", 
  "New Zealand", 
  "Newfoundland", 
  "Nicaragua", 
  "Niger", 
  "Nigeria", 
  "North Korea", 
  "North Pacific Islands", 
  "Norway", 
  "Nova Scotia", 
  "Oman", 
  "Pakistan", 
  "Panama", 
  "Papua New Guinea", 
  "Paraguay", 
  "Peru", 
  "Philippines", 
  "Poland", 
  "Portugal", 
  "Prince Edward Island", 
  "Puerto Rico", 
  "Qatar", 
  "Republic of Moldova", 
  "Reunion Island", 
  "Romania", 
  "Russia", 
  "Rwanda", 
  "Saudi Arabia", 
  "Senegal", 
  "Seychelles", 
  "Shang-Hai", 
  "Shen-Yang", 
  "Sierra Leone", 
  "Singapore", 
  "Slovakia", 
  "Slovenia", 
  "Solomon Islands", 
  "Somalia", 
  "South Africa", 
  "South Korea", 
  "Southern Line Islands", 
  "Spain", 
  "Sri Lanka", 
  "Sudan", 
  "Suriname", 
  "Swaziland", 
  "Sweden", 
  "Switzerland", 
  "Syria", 
  "Taiwan", 
  "Tajikistan", 
  "Tanzania", 
  "Tasmania", 
  "Thailand", 
  "Togo", 
  "Tokelau Island", 
  "Tonga", 
  "Trinidad And Tobago", 
  "Tunisia", 
  "Turkey", 
  "Turks Islands", 
  "Tuvalu", 
  "United States", 
  "Uganda", 
  "Ukraine", 
  "United Arab Emirates", 
  "United Kingdom", 
  "Uruguay", 
  "Urum-Qui", 
  "Uzbekistan", 
  "Vanuata", 
  "Venezuela", 
  "Victoria", 
  "Viet Nam", 
  "Virgin Islands", 
  "Virginia", 
  "Wake Island", 
  "Wallis And Futuna Island", 
  "Western Sahara", 
  "Western Samoa", 
  "Yemen", 
  "Yugoslavia", 
  "Yukon Territory", 
  "Zambia", 
  "Zimbabwe",
  "All"
};

CWUCountrySelection::CWUCountrySelection()
{
	Reset();
}

string CWUCountrySelection::GetName(short i)
{
	ASSERT( i>= 0 && i<NB_COUNTRY);
	return COUNTRAY_NAME[i];
}

string CWUCountrySelection::ToString()const
{
	string str;
	if( IsUsedAll() )
	{
		str = COUNTRAY_NAME[ALL];
	}
	else
	{
		for(int i=0; i<NB_COUNTRY; i++)
		{
			if( IsUsed(i) )
			{
				str += COUNTRAY_NAME[i];
				str += ';';
			}
		}
	}
	return str;
}

ERMsg CWUCountrySelection::FromString(const string& in)
{
	ERMsg msg;

	Reset();

	string::size_type start=0;
	string tmp = Tokenize(in, ";", start);
	while(!tmp.empty() )
	{
		msg+=SetUsed(tmp);
		tmp = Tokenize(in, ";", start);
	}

	return msg;
}

short CWUCountrySelection::GetCountry(const string& in)//by abr
{
	short country = -1;
	string tmp(in);
	if(tmp.length() != 2)
		Trim(tmp);

	MakeUpper(tmp);
	for(int i=0; i<NB_TAG; i++)
	{
		if (IsEqualNoCase(tmp, COUNTRAY_NAME[i]))
		{
			country = i;
			break;
		}
	}

	return country;
}



//*********************************************************************

const char* CUIWunderground::ATTRIBUTE_NAME[NB_ATTRIBUTE]={"Province"};
const char* CUIWunderground::CLASS_NAME = "WeatherUnderground";
const char* CUIWunderground::SERVER_NAME = "www.wunderground.com";


static const char pageFormat1[] = "weatherstation/ListStations.asp?showall=&start=%s&selectedState=%s";
static const char pageFormat2[] = "weatherstation/ListStations.asp?selectedCountry=%s";
static const short SEL_ROW_PER_PAGE	= 40;

static const char pageDataFormat[] = 
{
"weatherstation/WXDailyHistory.asp?"
"ID=%s&"
"graphspan=custom&"
"month=%d&"
"Day=%d"
"Year=%d&"
"monthEnd=%d&"
"DayEnd=%d"
"YearEnd=%d&"
"format=1"
};





CUIWunderground::CUIWunderground(void)
{
	if( !IsRegister( GetClassID() ) )
	{
		InitClass();
	}

	Reset();
}

void CUIWunderground::InitClass(const StringVector& option)
{
	GetParamClassInfo().m_className = GetString( IDS_SOURCENAME_WUNDERGROUND);

	CUIWeather::InitClass(option);

	ASSERT( GetParameters().size() < I_NB_ATTRIBUTE);

	StringVector title(IDS_PROPERTIES_WUNDERGROUND, "|;");
	ASSERT( title.size() == NB_ATTRIBUTE);
	//StringVector forceList(IDS_PROPERTIES_ENVCAN_FORCELIST);

	GetParameters().push_back( CParamDef(CParamDef::EDIT, ATTRIBUTE_NAME[0], title[0]) );
	//GetParameters().push_back( CParamDef(CParamDef::COMBO, CParamDef::BY_NUMBER, ATTRIBUTE_NAME[1], title[1], forceList, "0") );
	//GetParameters().push_back( CParamDef(CParamDef::BOOL, ATTRIBUTE_NAME[2], title[2], "0") );

}

CUIWunderground::~CUIWunderground(void)
{
}


CUIWunderground::CUIWunderground(const CUIWunderground& in)
{
	operator=(in);
}

long CUIWunderground::GetNbDay(const CTime& t)
{
	return GetNbDay(t.GetYear(), t.GetMonth()-1, t.GetDay()-1);
	//long(t.GetYear()*365+(t.GetMonth()-1)*30.42+(t.GetDay()-1));
}

long CUIWunderground::GetNbDay(int y, int m, int d)
{
	ASSERT( m>=0 && m<12);
	ASSERT( d>=0 && d<31);

	return long(y*365+m*30.42+d);
}

void CUIWunderground::Reset()
{
	CUIWeather::Reset();

	m_selection.Reset();
}

CUIWunderground& CUIWunderground::operator =(const CUIWunderground& in)
{
	if( &in != this)
	{
		CUIWeather::operator =(in);
		m_selection = in.m_selection;
		//m_forceDownload = in.m_forceDownload;
		//m_bUseForecast= in.m_bUseForecast;
	}

	return *this;
}

bool CUIWunderground::operator ==(const CUIWunderground& in)const
{
	bool bEqual = true;

	if( CUIWeather::operator !=(in) )bEqual = false;
	if( m_selection != in.m_selection)bEqual = false;
	//if( m_forceDownload != in.m_forceDownload)bEqual = false;
	//if( m_bUseForecast != in.m_bUseForecast)bEqual = false;
	
	
	return bEqual;
}

bool CUIWunderground::operator !=(const CUIWunderground& in)const
{
	return !operator ==(in);
}

string CUIWunderground::GetValue(size_t type)const
{
	ERMsg msg;
	string value;
	
	ASSERT( NB_ATTRIBUTE == 1); 
	switch(type)
	{
	case I_PROVINCE: value = m_selection.ToString(); break;
	//case I_FORCE_DOWNLOAD: break;
	//case I_FORECAST: break;
	default: value = CUIWeather::GetValue(type); break;
	}

	return value;
}

void CUIWunderground::SetValue(size_t type, const string& value)
{
	ASSERT( NB_ATTRIBUTE == 1); 
	switch(type)
	{
	case I_PROVINCE: m_selection.FromString(value); break;
//	case I_FORCE_DOWNLOAD:  break;
	//case I_FORECAST:  break;
	default: CUIWeather::SetValue(type, value); break;
	}

}


void CUIWunderground::GetSelection(short param, CSelectionDlgItemVector& items)const
{	
	//ASSERT( param == I_PROVINCE);

	CWUCountrySelection sel = m_selection;
	//
	//
	items.resize(CWUCountrySelection::NB_COUNTRY);

	for(int i=0; i<CWUCountrySelection::NB_COUNTRY; i++)
	{
		items[i].m_index = i;
		items[i].m_name = sel.GetName(i);
		items[i].m_bSelected = sel.IsUsed(i);
	}

}

void CUIWunderground::SetSelection(short param, const CSelectionDlgItemVector& items)
{	
	ASSERT( param == I_PROVINCE);

	CWUCountrySelection sel;

	ASSERT( items.size() == CWUCountrySelection::NB_COUNTRY);
	for (size_t i = 0; i<items.size(); i++)
	{
		sel.SetUsed(items[i].m_index, items[i].m_bSelected);
	}
	
	m_selection = sel;
}

bool CUIWunderground::Compare(const CParameterBase& in)const
{
	ASSERT(in.GetClassID() == GetClassID() );
	const CUIWunderground& info = dynamic_cast<const CUIWunderground&>(in);
	return operator==(info);
}

CParameterBase& CUIWunderground::Assign(const CParameterBase& in)
{
	ASSERT(in.GetClassID() == GetClassID() );
	const CUIWunderground& info = dynamic_cast<const CUIWunderground&>(in);
	return operator=(info);
}

		
ERMsg CUIWunderground::GetStationList(CLocationVector& stationList, CFL::CCallback& callback)const
{
	ERMsg msg;

	CInternetSessionPtr pSession;
	CHttpConnectionPtr pConnection;

	msg = GetHttpConnection( SERVER_NAME, pConnection, pSession);
	if( !msg)
		return msg;

	callback.SetCurrentDescription(GetString(IDS_LOAD_STATION_LIST) );
	callback.SetNbStep(m_selection.GetNbSelection());
		
//loop on province
	for(int i=0; i<CWUCountrySelection::NB_COUNTRY&&msg; i++)
	{
		if( !m_selection.IsUsed(i) )
			continue;
	
		string countryName= m_selection.GetName(i);
		replace(countryName.begin(), countryName.end(), ' ', '+');
		string pageURL = FormatA(pageFormat2, countryName.c_str());

		msg = GetStationListPage(pConnection, pageURL, stationList);
		msg+=callback.StepIt();
	}

	pConnection->Close();
	pSession->Close();
	
	callback.AddMessage(GetString(IDS_NB_STATIONS) + ToString(stationList.size()));

	return msg;
}	


ERMsg CUIWunderground::GetStationListPage(CHttpConnectionPtr& pConnection, const string& page, CLocationVector& stationList)const
{
	ERMsg msg;
	string source;
	msg = GetPageText( pConnection, page, source);

	return msg;
}

//******************************************************
ERMsg CUIWunderground::Execute(CFL::CCallback& callback)
{
	ERMsg msg;

	
	string workingDir = GetWorkingDir();

	callback.AddMessage(GetString(IDS_UPDATE_DIR ));
	callback.AddMessage(workingDir, 1);
	callback.AddMessage(GetString(IDS_UPDATE_FROM ) );
	callback.AddMessage(SERVER_NAME, 1 );
	callback.AddMessage("");
	
	//Get station
	int nbYear = m_lastYear - m_firstYear+1;

	callback.SetNbStep(1);
	//Get the stations list
	CLocationVector stationList;
	msg = GetStationList(stationList, callback);

	//create LOC from station list
	CLocationVector loc;
	for (size_t i = 0; i<stationList.size(); i++)
	{
		CLocation st;
		st.m_name = stationList[i].m_name;
		st.m_ID = stationList[i].m_ID;

		//double lat=0;
		//double lon=0;
		//sscanf( stationList[i].m_extra, "%lf,%lf", &lat, &lon);
		//ASSERT( lat>=-90 && lat<=90);
		//ASSERT( lon>=-180 && lon<=180);
		//st.SetLat( lat );
		//st.SetLon( lon );

		//loc.push_back(st);
	}

	msg += loc.Save("d:\\travail\\WUnderground\\allStation.csv");
	


/*	callback.SetNbStep(stationList.size());

	InitStat();

		int nbRun = 0;
		int curI=0;

		while(curI<stationList.size() && msg)//nbRun<5 && 
		{
			nbRun++;

			CInternetSession ISession;
			CHttpConnectionPtr pConnection;
			//CInternetSession sessionForecast;
			//CHttpConnectionPtr pConnectionForecast;

			msg = GetHttpConnection(SERVER_NAME, pConnection, ISession);
			if( msg && bUseForecast)
				msg = m_forecast.InitConnection(); 

			if (msg)
			{
				TRY
				{
					for(int i=curI; i<stationList.size()&&msg; i++)
					{
						msg = DonwloadStation( pConnection, stationList[i], callback);
						if( msg )
						{
							curI++;
							nbRun=0;
						}
					}
				}
				CATCH_ALL(e)
				{
					msg = SYGetMessage(*e);
				}
				END_CATCH_ALL

				//if an error occur: try again
				if( !msg && !callback.GetUserCancel() )
				{
					callback.AddStep(1);//one step more

					if( nbRun < 5) 
					{
						callback.AddMessage( msg );
						msg.asgType( ERMsg::OK );
						Sleep(1000);//wait 1 sec
					}
				}

				//clean connection
				pConnection->Close();
				ISession.Close();
				
				m_forecast.CloseConnection();
			}
		}


		ReportStat(callback);
	}
*/
	return msg;
}


/*ERMsg CUIWunderground::DonwloadStation( CHttpConnectionPtr& pConnection, const CLocation& info, CFL::CCallback& callback)
{
	ERMsg msg;

	string workingDir = GetWorkingDir();
	
	string outputPath;
	outputPath.Format( "%s%s\\", workingDir, info.m_prov);
	CreateMultipleDir(outputPath);

	
	CECPackDB packDB;
	msg = packDB.Open(outputPath+info.m_id);
	if( !msg)
		return msg;

	int nbFilesToDownload=0;
	int nbYear = m_lastYear-m_firstYear+1;
	boost::multi_array<bool, 2> bNeedDownload(boost::extents[nbYear][12]);

	
	
	ASSERT( bNeedDownload.shape()[0] == nbYear);
	ASSERT( bNeedDownload.shape()[1] == 12);
	

	for(int y=0; y<nbYear&&msg; y++)
	{
		int year = m_firstYear + y;
		for(int m=0; m<12&&msg; m++)
		{
			bNeedDownload[y][m] = NeedDownload(packDB, info, year, m);
			nbFilesToDownload+=bNeedDownload[y][m]?1:0;
		}
	}
	
	callback.SetCurrentDescription(info.m_name);
	callback.SetNbStep(0,nbFilesToDownload, 1);
	

	for(int y=0; y<nbYear&&msg; y++)
	{
		int year = m_firstYear + y;
		for(int m=0; m<12&&msg; m++)
		{
			//if( IsValidNew(packDB, info, year, m) )
			if( bNeedDownload[y][m] )
			{
				string pageURL;
				pageURL.Format(pageDataFormat, info.m_id, year, m+1, 1);

				msg += CopyStationDataPage(pConnection, pageURL, year, m, packDB);


				if( msg)
					AddToStat(year, m);

				msg += callback.StepIt();
			}
		}
	}

	if(m_forecast.IsInit())
	{
		CTRef TRef = CTRef::GetCurrentTRef();
		string outputText;

		msg += m_forecast.GetForecastText(info.m_id, outputText, callback);
		if( msg && !outputText.IsEmpty() )
			msg = packDB.SetForecast(TRef, outputText);
	}



	msg += packDB.Close();

	return msg;
}

void CUIWunderground::InitStat()
{
	m_nbDownload=0;
	int nbYear=m_lastYear-m_firstYear+1;
	m_stat.SetSize(nbYear, 12);
	for(int y=0; y<nbYear; y++)
		for(int m=0; m<12; m++)
			m_stat[y][m] = 0;
}

void CUIWunderground::AddToStat(short year, short m)
{
	m_nbDownload++;
	m_stat[year-m_firstYear][m]++;
}

void CUIWunderground::ReportStat(CFL::CCallback& callback)
{
	for(int y=0; y<m_stat.GetRows(); y++)
	{
		for(int m=0; m<m_stat.GetCols(); m++)
		{
			if( m_stat[y][m] > 0)
			{
				string tmp;
				tmp.FormatMessage( IDS_UPDATE_END, ToString( m_stat[y][m] ), ToString( m_stat[y][m] ) );
				string tmp2;
				tmp2.Format("%d\\%d%s", m_firstYear+y, m+1, tmp);
				callback.AddMessage( tmp2 );
			}
		}
	}


	string tmp;
	tmp.FormatMessage( IDS_UPDATE_END, ToString( m_nbDownload ), ToString( m_nbDownload ) );
	callback.AddMessage( tmp );

	

}
*/

bool CUIWunderground::IsValid(const CLocation& info, short y, short m, const string& filePath )const
{
	bool bDownload = false;

	//if( info.m_period.IsInside(m_firstYear+y, m) )
	//{
	//	bDownload = true;

	//	if( m_forceDownload!=ALWAYS)
	//	{
	//		CFileStatus status;
	//		if( CFile::GetStatus( filePath, status) )
	//		{
	//			try
	//			{
	//				//if the date(year, month) of the file in the directory is
	//				//the same than the data in the file, we download
	//				//else we don't download
	//				if(!( status.m_mtime.GetYear() == (m_firstYear+y) &&
	//					status.m_mtime.GetMonth() == (m+1)) )
	//						bDownload = false;

	//				if( m_forceDownload==AFTER_ONE_YEAR )
	//				{
	//					int nbDays = GetNbDay(status.m_mtime);
	//					if( m_nbDays - nbDays > 365)
	//						bDownload = true;
	//				}
	//				else if( m_forceDownload==SEPTEMBER_2007)
	//				{
	//					//force if the file is older than on month
	//					if( status.m_mtime.GetYear() < 2007 ||
	//						status.m_mtime.GetYear() == 2007 && status.m_mtime.GetMonth() < 9 )
	//						bDownload = true;
	//				}
	//			}
	//			catch(...)
	//			{
	//			}
	//		}
	//	}
	//}

	return bDownload;
}
/*
bool CUIWunderground::NeedDownload(const CECPackDB& packDB, const CLocation& info, short year, short m)const
{
	ASSERT(m_forceDownload>=0&&m_forceDownload<NB_FORCE);
	bool bDownload = false;

	if( info.m_period.IsInside(year, m) )
	{
		bDownload = true;

		if( m_forceDownload!=ALWAYS)
		{
			CTime lastUpdate = packDB.GetLastUpdate(year, m);
			if( lastUpdate.GetTime() > 0 )
			{
				//CTime dataTime(year, m+1, 1,0,0,0);

				int nbDays1 = m_nbDays - GetNbDay(lastUpdate);
				int nbDays2 = m_nbDays - GetNbDay(year, m, 0);
				int nbDays3 = GetNbDay(lastUpdate) - GetNbDay(year, m, 0);
					
				if ( packDB.IsBlankPage(year, m) && nbDays3>31)//if ( packDB.IsBlankPage(year, m) && nbDays > 365)
					bDownload = false;
				else if( packDB.IsComplet(year, m) )//|| nbDays2>2*365: not usefull no incomplete file after one year
					bDownload = false;

				//if( packDB.IsComplet(year, m) )
				//else if ( packDB.IsBlankPage(year, m) && nbDays > 365)
					//bDownload = false;

				if( m_forceDownload==AFTER_ONE_YEAR )
				{
					if( nbDays1 > 365)
						bDownload = true;
				}
				else if( m_forceDownload==SEPTEMBER_2007)
				{
					//force if the file is older than september 2007 (change in coordinate)
					if( lastUpdate.GetYear() < 2007 ||
						lastUpdate.GetYear() == 2007 && lastUpdate.GetMonth() < 9 )
						bDownload = true;

				}
				else if( m_forceDownload==BLANK_PAGE)
				{
					if( packDB.IsBlankPage(year, m) )
						bDownload = true;
				}
				else if( m_forceDownload==INCOMPLETE_PAGE)
				{
					if( !packDB.IsBlankPage(year, m) &&
						!packDB.IsComplet(year, m) )
						bDownload = true;
				}

			//}
			//catch(...)
			//{
			//}
			}
		}
	}

	return bDownload;
}

//[vide] = Aucune donnée disponible 
//M = Données manquantes 
//E = Valeur estimée 
//A = Valeur accumulée 
//C = Précipitation, quantité incertaine 
//L = des précipitation peuvent avoir eu lieu 
//F = Valeur accumulée et estimée 
//N = Température manquante, mais > 0 
//Y = Température manquante, mais < 0 
//S = À plus d'une reprise 
//T = Trace 
//* = La valeur affichée est basée sur des données incomplètes. 
//= Ces données journalières n'ont subit qu'un contrôle de qualité préliminaire 
*/				
ERMsg CUIWunderground::PreProcess(CFL::CCallback& callback)
{
	return ERMsg();
}

ERMsg CUIWunderground::GetStationList(StringVector& stationList, CFL::CCallback& callback)
{
	ERMsg msg;

//	CSortedArray<string, const string&> stationList;
	string workingDir = GetWorkingDir(); 

	//find all station in the directories
	int nbYear = m_lastYear-m_firstYear+1;


	//callback.SetCurrentDescription(UtilWin::GetString( IDS_GET_STATION_LIST ));
	//callback.SetNbStep( 0, m_selection.GetNbSelection(), 1); 
	//

	//for(int x=0; x<m_selection.GetNbSelection()&&msg; x++)
	//{
	//	string prov = m_selection.GetNameBySelectionIndex(x);

	//	string path;
	//	path.Format( "%s%s\\", workingDir, prov);
	//	StringVector tmpFilePath;
	//	CECPackDB::GetFileList(tmpFilePath, path);
	//
	//	for(int i=0; i<tmpFilePath.size()&&msg; i++)
	//	{
	//		string fileTitle = UtilWin::GetFileTitle(tmpFilePath[i]);
	//		ASSERT( UtilWin::FindInArray(stationList, fileTitle)==-1);

	//		CECPackDB packDB;
	//		msg = packDB.Open(tmpFilePath[i]);
	//		if(msg)
	//		{
	//			//look for year
	//			short firstYear = packDB.GetFirstYear();
	//			short lastYear = packDB.GetLastYear();

	//			//if period overlapse
	//			if( m_firstYear <= lastYear && m_lastYear >= firstYear )
	//			{
	//				//look for coord
	//				//CStation station;
	//				//VERIFY(packDB.GetStationCoord(station));

	//				//if( m_boundingBox.IsRectEmpty() || 
	//				//	m_boundingBox.PtInRect(station.GetCoord()) )
	//				//{
	//					stationList.push_back(prov+"\\"+fileTitle);
	//				//}
	//			}
	//		}

	//		msg+=callback.StepIt(1.0/tmpFilePath.size());
	//	}//all files
	//}//all province


	return msg;
}

ERMsg CUIWunderground::GetStation(const string& stationName, CDailyStation& station, CFL::CCallback& callback)
{
	ERMsg msg;

	//string workingDir = GetWorkingDir(); 

	//string filePath = (string)workingDir+stationName;
	//
	//CECPackDB packDB;

	//msg = packDB.Open(filePath);
	//if(msg)
	//{
	//	msg = packDB.GetDailyStation(m_firstYear, m_lastYear, m_bUseForecast, station);
	//}

	//packDB.Close();

	return msg;
}




