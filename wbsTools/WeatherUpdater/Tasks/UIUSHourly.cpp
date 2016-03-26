#include "StdAfx.h"
#include ".\USHourly.h"

#include "dailyStation.h"
#include "FileManager\DailyFile.h"
#include "SYShowMessage.h"
#include "CommonRes.h"
#include "Resource.h"
#include <shlwapi.h>
#include <atlpath.h>
#include "global.h"

#include "Statistic.h"
#include "ECHPackDB.h"
#include "boost\multi_array.hpp"

using namespace CFL;
using namespace DAILY_DATA;
//*********************************************************************

const char* CUSHourly::ATTRIBUTE_NAME[NB_ATTRIBUTE]={"FIRST_MONTH", "LAST_MONTH", "STATE", "BOUNDING_BOX"};
const char* CUSHourly::CLASS_NAME = "US_HOURLY";


static const CGeoRect DEFAULT_BOUDINGBOX(-180, -90, 180, 90,  CProjection::GEO);

CUSHourly::CUSHourly(void)
{
	if( !IsRegister( GetClassID() ) )
	{
		InitClass();
	}

	Reset();
}

void CUSHourly::InitClass(const CStringArray& option)
{
	GetParamClassInfo().m_className.LoadString( IDS_SOURCENAME_US_HOURLY );

	CUIWeather::InitClass(option);

	ASSERT( GetParameters().GetSize() < I_NB_ATTRIBUTE);

	CStringArrayEx strList;
	strList.LoadString( IDS_PROPERTIES_ENVCAN_HOURLY);
	ASSERT( strList.GetSize() >= NB_ATTRIBUTE);

	
	GetParameters().Add( CParamDef(CParamDef::COMBO, CParamDef::BY_NUMBER, ATTRIBUTE_NAME[0], strList[0], CGlobal::GetMonths(), "0" ));
	GetParameters().Add( CParamDef(CParamDef::COMBO, CParamDef::BY_NUMBER, ATTRIBUTE_NAME[1], strList[1], CGlobal::GetMonths(), "11" ));
	GetParameters().Add( CParamDef(CParamDef::EDIT_BROWSE, ATTRIBUTE_NAME[2], strList[2]) );
	GetParameters().Add( CParamDef(CParamDef::COORD_RECT, ATTRIBUTE_NAME[3], strList[3]) );
//	GetParameters().Add( CParamDef(CParamDef::COMBO, CParamDef::BY_NUMBER, ATTRIBUTE_NAME[4], array[4], forceList, "4") );
//	GetParameters().Add( CParamDef(CParamDef::BOOL, ATTRIBUTE_NAME[5], array[5], "0") );
//	GetParameters().Add( CParamDef(CParamDef::BOOL, ATTRIBUTE_NAME[6], array[6], "1") );

}

CUSHourly::~CUSHourly(void)
{
}


CUSHourly::CUSHourly(const CUSHourly& in)
{
	operator=(in);
}


void CUSHourly::Reset()
{
	CUIWeather::Reset();

	m_selection.Reset();
	m_firstMonth=0;
	m_lastMonth=11;

	m_boundingBox = DEFAULT_BOUDINGBOX;
	m_type = 0;
}

CUSHourly& CUSHourly::operator =(const CUSHourly& in)
{
	if( &in != this)
	{
		CUIWeather::operator =(in);
		m_selection = in.m_selection;
		m_firstMonth=in.m_firstMonth;
		m_lastMonth=in.m_lastMonth;
		m_boundingBox=in.m_boundingBox;
		m_type = in.m_type;
	}

	return *this;
}

bool CUSHourly::operator ==(const CUSHourly& in)const
{
	bool bEqual = true;

	if( CUIWeather::operator !=(in) )bEqual = false;
	if( m_selection != in.m_selection)bEqual = false;
	if( m_firstMonth!=in.m_firstMonth)bEqual = false;
	if( m_lastMonth!=in.m_lastMonth)bEqual = false;
	if( m_boundingBox != in.m_boundingBox )bEqual = false;
	if( m_type != in.m_type)bEqual = false;
	
	return bEqual;
}

bool CUSHourly::operator !=(const CUSHourly& in)const
{
	return !operator ==(in);
}

CString CUSHourly::GetValue(short type)const
{
	ERMsg msg;
	CString str;
	
	ASSERT( NB_ATTRIBUTE == 4); 
	switch(type)
	{
	case I_FIRST_MONTH: str = ToString(m_firstMonth); break;
	case I_LAST_MONTH: str = ToString(m_lastMonth); break;
	case I_PROVINCE: str = m_selection.ToString(); break;
	case I_BOUNDINGBOX: str = m_boundingBox.ToString(m_type); break;
	default: str = CUIWeather::GetValue(type); break;
	}

	return str;
}

void CUSHourly::SetValue(short type, const CString& str)
{
	ASSERT( NB_ATTRIBUTE == 4);
	switch(type)
	{
	case I_FIRST_MONTH: m_firstMonth= ToInt(str); break;
	case I_LAST_MONTH: m_lastMonth = ToInt(str); break;
	case I_PROVINCE: m_selection.FromString(str); break;
	case I_BOUNDINGBOX: m_type = m_boundingBox.FromString(str); break;
	default: CUIWeather::SetValue(type, str); break;
	}

}


void CUSHourly::GetSelection(short param, CSelectionDlgItemArray& items)const
{	
	ASSERT( param == I_PROVINCE);

	CStateSelection sel = m_selection;
	
	items.SetSize(CStateSelection::NB_STATE);

	for(int i=0; i<CStateSelection::NB_STATE; i++)
	{
		items[i].m_index = i;
		items[i].m_name = sel.GetName(i, CStateSelection::NAME);
		items[i].m_bSelected = sel.IsUsed(i);
	}

}

void CUSHourly::SetSelection(short param, const CSelectionDlgItemArray& items)
{	
	ASSERT( param == I_PROVINCE);

	CStateSelection sel;

	ASSERT( items.GetSize() == CStateSelection::NB_STATE);
	for(int i=0; i<items.GetSize();i++)
	{
		sel.SetUsed(items[i].m_index, items[i].m_bSelected);
	}
	
	m_selection = sel;
}

bool CUSHourly::Compare(const CParameterBase& in)const
{
	ASSERT(in.GetClassID() == GetClassID() );
	const CUSHourly& info = dynamic_cast<const CUSHourly&>(in);
	return operator==(info);
}

CParameterBase& CUSHourly::Assign(const CParameterBase& in)
{
	ASSERT(in.GetClassID() == GetClassID() );
	const CUSHourly& info = dynamic_cast<const CUSHourly&>(in);
	return operator=(info);
}

//*****************************************************************************

ERMsg CUSHourly::Execute(CSCCallBack& callback)
{
	return ERMsg();
}


ERMsg CUSHourly::PreProcess(CSCCallBack& callback)
{
	return ERMsg();
}

ERMsg CUSHourly::LoadStationList()
{
	ERMsg msg;

	CString workingDir = GetWorkingDir(); 


	CStdioFileEx file;
	msg = file.Open(workingDir + "STNS.TXT", CFile::modeRead);

	if(msg)
	{
		CString line;
		file.ReadString(line);

		while( file.ReadString(line) )
		{
			line.Trim();
			if( !line.IsEmpty() )
			{
				CString state = line.Mid(29,2);
				if( m_selection.IsUsed(state) )
				{
					CStation station;
					
					station.SetID(line.Mid(0,5) );
					station.SetName(line.Mid(6,22));
					int latNeg = line.Mid(37,1).CompareNoCase("N")==0?1:-1;
					station.SetLat(latNeg*(ToInt(line.Mid(38,2))+ToInt(line.Mid(41,2))/60.0) );

					int lonNeg = line.Mid(44,1).CompareNoCase("E")==0?1:-1;
					station.SetLon(lonNeg*(ToInt(line.Mid(45,3))+ToInt(line.Mid(49,2))/60.0) );
					station.SetElev(ToInt(line.Mid(52,5)) );

					ASSERT( station.IsValid() );
					m_stationList.Add(station);
				}
			}
		}
	}

	return msg;
}

ERMsg CUSHourly::GetStationList(CStringArray& stationList, CSCCallBack& callback)
{
	ERMsg msg;

	msg = LoadStationList();

	if( msg && m_lastYear>=1990 && m_firstYear<=1995)
	{
		for(int i=0; i<m_stationList.GetSize(); i++)
		{
			if( m_stationList[i].GetID() != "14751" &&
				m_stationList[i].GetID() != "26415" &&
				m_stationList[i].GetID() != "24284" &&
				m_stationList[i].GetID() != "24230" &&
				m_stationList[i].GetID() != "21504" &&
				m_stationList[i].GetID() != "22516" &&
				m_stationList[i].GetID() != "22521" &&
				m_stationList[i].GetID() != "22536" &&
				m_stationList[i].GetID() != "41415" &&
				m_stationList[i].GetID() != "11641" )
			{
				stationList.Add( m_stationList[i].GetID() );
			}
		}
	}

	return msg;
}

void CUSHourly::GetStationHeader( const CString& stationName, CDailyStation& station)
{
	for(int i=0; i<m_stationList.GetSize(); i++)
	{
		if( m_stationList[i].GetID() == stationName )
		{
			((CStation&)station) = m_stationList[i];
			break;
		}
	}
}

ERMsg CUSHourly::GetStation(const CString& stationName, CDailyStation& station)
{
	ERMsg msg;

	CString workingDir = GetWorkingDir(); 

	GetStationHeader( stationName, station );
	
	//CStdioFileEx fileOut( "d:\\travail\\hourly\\PeakHourUSA.csv", CFile::modeWrite|CFile::modeCreate|CFile::modeNoTruncate);
	
	
	//if( fileOut.GetLength() == 0)
	//	fileOut.WriteString( "#,Lat,Lon,Elev,Year,Month,Day,Hmin,Tmin,Hmax,Tmax\n");
	
//	fileOut.SeekToEnd();
	
	CYearPackage pakage;

	int nbYear = m_lastYear-m_firstYear+1;
	for(int y=0; y<nbYear; y++)
	{
		int year=m_firstYear+y;

		CString filePath;
		filePath.Format( "%sData\\%s\\%s_%d", workingDir, stationName, stationName, year-1900);
		
		CStdioFileEx file;
		msg = file.Open( filePath, CFile::modeRead );
		if( !msg )
			return msg;

		CDailyData dailyData;
		
		CString line;
		
		C24HoursData hours24;
		double minValue = 999;
		double maxValue = -999;
		int minHour = -1;
		int maxHour = -1;



		bool bContinue = true; 
		while( bContinue )
		{ 
			bContinue = file.ReadString(line);

			int year = ToInt(line.Mid(6,4));
			int m = ToInt(line.Mid(10,2))-1;
			int d = ToInt(line.Mid(12,2))-1;
			int h = ToInt(line.Mid(14,2))-1;

			if( hours24.GetYear() == -999)
				hours24.SetDate(year,m,d);

			if( d != hours24.GetDay() || !bContinue)
			{
				int jd = hours24.GetJDay();

				if( hours24.IsValid( CHourlyData::T_DEW ) )
				{
					dailyData[jd][TDEW] = (float)hours24.GetStat(CHourlyData::T_DEW)[CFL::MEAN];
				}

				if( hours24.IsValid( CHourlyData::RH ) )
				{
					dailyData[jd][RELH] = (float)hours24.GetStat(CHourlyData::RH)[CFL::MEAN];
					dailyData[jd][ADD1] = (float)hours24.GetStat(CHourlyData::RH)[CFL::LOWEST];
					dailyData[jd][ADD2] = (float)hours24.GetStat(CHourlyData::RH)[CFL::HIGHEST];
				}
				if( hours24.IsValid( CHourlyData::WIND_SPEED ) )
					dailyData[jd][WNDS] = (float)hours24.GetStat(CHourlyData::WIND_SPEED)[CFL::MEAN];

				if( hours24.IsValid( CHourlyData::T ) )
				{
					dailyData[jd][TMIN] = (float)hours24.GetTmin();
					dailyData[jd][TMAX] = (float)hours24.GetTmax();

					/*CString str;
					for(int i=1; i<CStation::NB_MEMBER; i++)
						str += station.GetString(i)+",";

					fileOut.WriteString(str);
					str.Format("%4d,%2d,%2d,%d,%.1lf,%d,%.1lf\n", hours24.GetYear(), hours24.GetMonth()+1, hours24.GetDay()+1, minHour, minValue, maxHour, maxValue);
					fileOut.WriteString(str);
					*/
				}
				
				if( hours24.IsValid( CHourlyData::HUMIDEX ) )
				{
					dailyData[jd][PRCP] = (float)hours24.GetStat(CHourlyData::HUMIDEX)[CFL::SUM];
				}

				/*if( hours24[23][CHourlyData::VISIBILITY] > -999 )
				{
					dailyData[jd][SNOW] = 0;//need a value
					dailyData[jd][SNDH] = (float)hours24[23][CHourlyData::VISIBILITY];
				}*/

				hours24.Reset();
				hours24.SetDate(year,m,d);
				minValue = 999;
				maxValue = -999;
				minHour = -1;
				maxHour = -1;

				
			}

			if( d == hours24.GetDay() && bContinue)
			{

				if( line.Mid(55,5) != "999.9" )
				{
					hours24.Set(h, CHourlyData::T, line.Mid(55,5) );

					double T = ToDouble(line.Mid(55,5));

					if( h<= 12 && T<minValue)
					{
						minValue= T;
						minHour=h;
					}
					if( h>= 12 && T>maxValue)
					{
						maxValue = T;
						maxHour = h;
					}
				}
				if( line.Mid(61,5) != "999.9" )
					hours24.Set(h, CHourlyData::T_DEW, line.Mid(61,5));
				if( line.Mid(67,3) != "999" )
					hours24.Set(h, CHourlyData::RH, line.Mid(67,3));
				if( line.Mid(80,4) != "99.9" )
				{
					double ws = ToDouble(line.Mid(80,4));
					hours24.Set(h, CHourlyData::WIND_SPEED, ToString(ws*3600/1000));//in km/h
				}
				
				if( line.Mid(128,1) == " " )
				{
					double ppt = ToDouble(line.Mid(125,3));
					hours24.Set(h, CHourlyData::HUMIDEX, ToString(ppt*0.2540));//100e of inch --> mm of water
				}

				if( line.Mid(128,1) == "A" )
				{
					int i;
					i=0;
				}

				if( line.Mid(129,3) != "999" )
				{
					double snowDepth = ToDouble(line.Mid(129,3));
					hours24.Set(h, CHourlyData::VISIBILITY, ToString(snowDepth*2.54));//in cm
				}
			}

		}
		
		
		if( dailyData.HaveData() )
		{
			CDailyYear yearData;
			yearData.SetYear(m_firstYear+y);
			yearData.SetData(dailyData);
			pakage.SetYear(yearData);
		}


	}//for all year


	if( msg )
	{
		
		pakage.SetFileTitle(station.GetName()+ "_" + station.GetID() );
		station.AddPakage(pakage);
	}

	return msg;
}




