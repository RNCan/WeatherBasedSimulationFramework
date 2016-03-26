#include "StdAfx.h"
#include "UIBCSnowPillows.h"

#include "DailyStation.h"
#include "DailyDatabase.h"
#include "ERMsg.h"
#include "CommonRes.h"
#include "Resource.h"
#include <boost/multi_array.hpp>
#include "CSV.h"
//#include "selectionDlg.h"
#include "UtilWWW.h"
#include "SYShowMessage.h"


using namespace CFL;
using namespace std;
using namespace HOURLY_DATA;
using namespace stdString;
using namespace UtilWWW;


static const DWORD FLAGS = INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_PRAGMA_NOCACHE;
//***********************************************************************
const char* CUIBCSnowPillow::ATTRIBUTE_NAME[NB_ATTRIBUTE]={"Province"};
const char* CUIBCSnowPillow::CLASS_NAME = "BCSnowPillow";
const char* CUIBCSnowPillow::SERVER_NAME = "bcrfc.env.gov.bc.ca";


//http://www.weather.uwaterloo.ca/data.html#archive
//http://bcrfc.env.gov.bc.ca/data/asp/archive.htm


CUIBCSnowPillow::CUIBCSnowPillow(void)
{
	if( !IsRegister( GetClassID() ) )
	{
		InitClass();
	}

	Reset();
}

void CUIBCSnowPillow::InitClass(const StringVector& option)
{
	GetParamClassInfo().m_className = GetString( IDS_SOURCENAME_BC_SNOWPILLOW );

	CUIWeather::InitClass(option);

	ASSERT( GetParameters().size() < I_NB_ATTRIBUTE);

	//StringVector strArray(IDS_PROPERTIES_BC_SNOWPILLOW, "|;");
	//ASSERT( strArray.size() >= NB_ATTRIBUTE);

	//GetParameters().push_back( CParamDef(CParamDef::EDIT_BROWSE, ATTRIBUTE_NAME[0], strArray[0].c_str()) );

}

CUIBCSnowPillow::~CUIBCSnowPillow(void)
{
}


CUIBCSnowPillow::CUIBCSnowPillow(const CUIBCSnowPillow& in)
{
	operator=(in);
}


void CUIBCSnowPillow::Reset()
{
	CUIWeather::Reset();

	//m_selection.Reset();
//	m_forceDownload=NEVER;
//	m_bUseForecast=true;

//	m_nbDownload=0;


//	m_nbDays = GetNbDay(CTime::GetCurrentTime());
}

CUIBCSnowPillow& CUIBCSnowPillow::operator =(const CUIBCSnowPillow& in)
{
	if( &in != this)
	{
		CUIWeather::operator =(in);
		//m_selection = in.m_selection;
//		m_forceDownload = in.m_forceDownload;
//		m_bUseForecast= in.m_bUseForecast;
	}

	return *this;
}

bool CUIBCSnowPillow::operator ==(const CUIBCSnowPillow& in)const
{
	bool bEqual = true;

	if( CUIWeather::operator !=(in) )bEqual = false;
	//if( m_selection != in.m_selection)bEqual = false;
//	if( m_forceDownload != in.m_forceDownload)bEqual = false;
//	if( m_bUseForecast != in.m_bUseForecast)bEqual = false;
	
	
	return bEqual;
}

bool CUIBCSnowPillow::operator !=(const CUIBCSnowPillow& in)const
{
	return !operator ==(in);
}

string CUIBCSnowPillow::GetValue(size_t type)const
{
	ERMsg msg;
	string value;
	
	ASSERT( NB_ATTRIBUTE == 1); 
	//switch(type)
	//{
	//case I_PROVINCE: value = m_selection.ToString(); break;
//	case I_FORCE_DOWNLOAD: value.Format( "%d", m_forceDownload); break;
//	case I_FORECAST: value.Format("%d", m_bUseForecast); break;
	//default: value = CUIWeather::GetValue(type); break;
	//}
	
	value = CUIWeather::GetValue(type);
	return value;
}

void CUIBCSnowPillow::SetValue(size_t type, const string& value)
{
	ASSERT( NB_ATTRIBUTE == 1); 
	//switch(type)
	//{
	//case I_PROVINCE: m_selection.FromString(value); break;
//	case I_FORCE_DOWNLOAD: m_forceDownload = atoi(value); break;
//	case I_FORECAST: m_bUseForecast= atoi(value); break;
	//default: CUIWeather::SetValue(type, value); break;
	//}

}


bool CUIBCSnowPillow::Compare(const CParameterBase& in)const
{
	ASSERT(in.GetClassID() == GetClassID() );
	const CUIBCSnowPillow& info = dynamic_cast<const CUIBCSnowPillow&>(in);
	return operator==(info);
}

CParameterBase& CUIBCSnowPillow::Assign(const CParameterBase& in)
{
	ASSERT(in.GetClassID() == GetClassID() );
	const CUIBCSnowPillow& info = dynamic_cast<const CUIBCSnowPillow&>(in);
	return operator=(info);
}


ERMsg CUIBCSnowPillow::UpdateStationInformation(CFL::CCallback& callback)const
{
	ERMsg msg;
	
	enum TColumns{ C_NO, C_BASIN, C_STATION_NAME, C_ELEV, C_LATITUDE, C_LONGITUDE, C_RECORD_LENGTH, C_STATUS, NB_COLUMNS };

	CLocationVector stations;

	

	//open a connection on the server
	CInternetSessionPtr pSession;
	CHttpConnectionPtr pConnection;
	msg = GetHttpConnection(SERVER_NAME, pConnection, pSession, FLAGS);
	if (msg)
	{
		string URL = "data/asp/archive.htm";

		string source;
		msg = UtilWWW::GetPageText(pConnection, URL, source, false, FLAGS);
		if (msg)
		{
			string::size_type begin = source.find("Automated Snow Pillow Data Archive");
			source = stdString::FindString(source, "<table border>", "</table>", begin);
			begin = 0;
			string str = stdString::FindString(source, "<tr", "</tr>", begin);//skip header



			callback.SetCurrentDescription(GetString(IDS_LOAD_STATION_LIST));
			callback.SetNbStep(source.length());

			str = stdString::FindString(source, "<tr", "</tr>", begin);//get first station
			while (begin != string::npos&&msg)
			{
				StringVector columns;

				string::size_type pos = 0;
				while (pos != string::npos)
				{
					columns.push_back(stdString::FindString(str, "<td", "</td>", pos));
				}

				columns.pop_back();
				ASSERT(columns.size() == NB_COLUMNS);

				
				CLocation station(PurgeFileName(columns[C_STATION_NAME]), columns[C_NO], ToDouble(columns[C_LATITUDE]), ToDouble(columns[C_LONGITUDE]), ToDouble(columns[C_ELEV]));
				station.SetSSI("Basin", TrimConst(columns[C_BASIN]));
				station.SetSSI("Record Length", columns[C_RECORD_LENGTH]);
				station.SetSSI("Status", columns[C_STATUS]);

				stations.push_back(station);
				str = stdString::FindString(source, "<tr", "</tr>", begin);
				msg += callback.StepIt(0);
			}

			callback.SetCurrentStepPos(begin);
			msg += callback.StepIt(0);
		}
		
	}


	pConnection->Close();
	pSession->Close();

	msg = stations.Save(GetStationListFilePath());


	return msg;
}

string CUIBCSnowPillow::GetOutputFilePath(const std::string& ID, const std::string ext)const
{
	return GetWorkingDir() + "archive/" + ID + ext;
}


ERMsg CUIBCSnowPillow::Execute(CFL::CCallback& callback)
{
	ERMsg msg;
	
	size_t nbYears = m_lastYear - m_firstYear + 1;
	callback.SetNbTask(nbYears + 2);


	if (!FileExists(GetStationListFilePath()) )
	{
		msg = UpdateStationInformation(callback);

		if (!msg)
			return msg;
	}
	else
	{
		callback.SkipTask(2);
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

	CInternetSessionPtr pSession;
	CHttpConnectionPtr pConnection;
	msg += GetHttpConnection(SERVER_NAME, pConnection, pSession, FLAGS);

	if (!msg)
		return msg;

	//Get station
	int nbDownload = 0;
	int nbFilesToDownload = (int)m_stations.size();
	
	callback.SetCurrentDescription("Download data" );
	callback.AddMessage("Number of files to download: " + ToString(nbFilesToDownload));
	callback.SetNbStep(nbFilesToDownload);

	TRY
	{
		for (CLocationVector::const_iterator it = m_stations.begin(); it != m_stations.end() && msg; it++)
		{
			const CLocation& station = *it;
			string filePathZip = GetOutputFilePath(station.m_ID, ".zip");
			string outputFilePath = GetOutputFilePath(station.m_ID, ".csv");

			static const char webPageDataFormat[] =	{"data/asp/archive/%s.zip"	};

			string ID = station.m_ID;
			string URL = stdString::FormatA(webPageDataFormat, ID.c_str());

			CFL::CreateMultipleDir(GetPath(filePathZip));
			msg = UtilWWW::CopyFile(pConnection, URL, filePathZip, FLAGS);
			string command = "7z.exe e \"" + filePathZip + "\" -y -o\"" + outputFilePath + "\"";
			msg += CFL::WinExecWait(command.c_str());
			msg += CFL::RemoveFile(filePathZip);

			if (msg)
			{
				nbDownload++;
				msg += callback.StepIt();
			}
		}
	}
	CATCH_ALL(e)
	{
		msg = SYGetMessage(*e);
	}
	END_CATCH_ALL

	callback.AddMessage(FormatMsg(IDS_UPDATE_END, ToString(nbDownload), ToString(nbFilesToDownload)));

	//}

	//clean connection
	pConnection->Close();
	pSession->Close();

	return msg;
}

ERMsg CUIBCSnowPillow::PreProcess(CFL::CCallback& callback)
{
	
	return m_stations.Load(GetStationListFilePath());
}


ERMsg CUIBCSnowPillow::GetStationList(StringVector& stationList, CFL::CCallback& callback)
{
	ERMsg msg;
	
	for(size_t i=0; i<m_stations.size(); i++)
		stationList.push_back( m_stations[i].m_ID );
	
	return msg;
}


ERMsg CUIBCSnowPillow::GetStation(const string& fileName, CDailyStation& station, CFL::CCallback& callback)
{
	ERMsg msg;


	/*if( fileName == "4A29P" )
	{
		msg.ajoute( "la station " + fileName + "est trop bizar");
		return msg;
	}
*/
	
	string filePath = GetWorkingDir()+fileName+".csv";

//	 —up to 3 hourly measurements missing,,,,,,,,,,,,,
//U  —more than 3 hourly measurements missing,,,,,,,,,,,,,
//E  —estimated,,,,,,,,,,,,,
//R  —original data revised,,,,,,,,,,,,,
//L  —8 AM measurement not available,,,,,,,,,,,,,
//A  —8 AM measurement available after L code,,,,,,,,,,,,,
//M  —missing,,,,,,,,,,,,,

	enum TElem{PILLOW_ID, P_YEAR, P_MONTH, P_DAY, P_T_MAX, P_T_MAX_QC, P_T_MIN, P_T_MIN_QC, P_PRECIPITATION, P_PRECIPITATION_QC, P_TOTAL_PRECIPITATION, P_TOTAL_PRECIPITATION_QC, P_SNOW_WATER_EQUIVALENT, P_SNOW_WATER_EQUIVALENT_QC, P_SNOW_DEPTH, P_SNOW_DEPTH_QC};


	ifStream file;
	msg = file.open(filePath);

	if( msg )
	{
		size_t i=0;
		for(CSVIterator loop(file); loop!=CSVIterator(); ++loop, i++)
		{
			if( !(*loop)[PILLOW_ID].empty() && !(*loop)[P_YEAR].empty() && 
				!(*loop)[P_MONTH].empty() && !(*loop)[P_DAY].empty() )
			{
				//CTM TM(CTM::DAILY); 
				//CWeatherAccumulator stat(TM);

				int year = stdString::ToInt((*loop)[P_YEAR]);
				size_t m = stdString::ToInt((*loop)[P_MONTH]) - 1;
				size_t d = stdString::ToInt((*loop)[P_DAY]) - 1;
				if (d > 30)//sometime year and day are inversed
				{
					assert(false);
					int day = (int)d;
					CFL::Switch(year, day);
					d = day;
				}
					

				ASSERT( year>=m_firstYear && year<=m_lastYear);
				ASSERT( m<12);
				ASSERT(d<CFL::GetNbDayPerMonth(year, m) );

				CTRef Tref(year,m,d);
		
				//for(int i=0; i<9; i++)
					//++loop;
				
				//ASSERT( (*loop)[PILLOW_ID]=="Pillow I.D." );
				//loop.ReadHeader();
				
				station.CreateYear(year);
				if( !(*loop)[P_T_MIN].empty() && !(*loop)[P_T_MAX].empty() && 
					(*loop)[P_T_MIN_QC] != "M" && (*loop)[P_T_MAX_QC] != "M")
				{
					float Tmin = ToFloat((*loop)[P_T_MIN]);
					float Tmax = ToFloat((*loop)[P_T_MAX]);

					if( Tmin>-50 && Tmin<50 && Tmax>-50 && Tmax<50 )
					{
						station[Tref][H_TAIR] = (Tmin+Tmax)/2;
						station[Tref][H_TRNG] = Tmax - Tmin;
					}
				}

				if( !(*loop)[P_PRECIPITATION].empty() && (*loop)[P_PRECIPITATION_QC] != "M" )
					station[Tref][H_PRCP] = ToDouble((*loop)[P_PRECIPITATION]);

				if( !(*loop)[P_SNOW_WATER_EQUIVALENT].empty() && (*loop)[P_SNOW_WATER_EQUIVALENT_QC] != "M" )
					station[Tref][H_SWE] = ToDouble((*loop)[P_SNOW_WATER_EQUIVALENT]);
				
				if( !(*loop)[P_SNOW_DEPTH].empty() && (*loop)[P_SNOW_DEPTH_QC] != "M" )
					station[Tref][H_SNDH] = ToDouble((*loop)[P_SNOW_DEPTH]);
			}
		}
			
		file.close();
	}
			
			
	return msg;
}

