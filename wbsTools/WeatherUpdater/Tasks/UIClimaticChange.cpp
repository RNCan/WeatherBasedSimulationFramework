#include "StdAfx.h"
#include ".\UIClimaticChange.h"

#include "dailyStation.h"
#include "FileManager\DailyFile.h"
#include "NormalStation.h"
//#include "FileManager\DailyDirectoryManager.h"
#include "SYShowMessage.h"
#include "CommonRes.h"
#include "Resource.h"
#include <shlwapi.h>
#include <atlpath.h>



using namespace CFL;
using namespace DAILY_DATA;
using namespace UtilWin;

//*********************************************************************

//const char* CUIClimChange::ATTRIBUTE_NAME[NB_ATTRIBUTE]={};
const char* CUIClimChange::CLASS_NAME = "CLIM_CHANGE";
//const char* CUIClimChange::SERVER_NAME = "";


CUIClimChange::CUIClimChange(void)
{
	if( !IsRegister( GetClassID() ) )
	{
		InitClass();
	}

	Reset();
}

void CUIClimChange::InitClass(const CStringArray& option)
{
	GetParamClassInfo().m_className.LoadString( IDS_SOURCENAME_CLIMCHANGE );

	CUIWeather::InitClass(option);

	//ASSERT( GetParameters().GetSize() < I_NB_ATTRIBUTE);

	//CStringArray array;
	//LoadStringArray( array, IDS_PROPERTIES_ENVCAN);
	//ASSERT( array.GetSize() == 1);

	//GetParameters().Add( CParamDef(CParamDef::EDIT_BROWSE, ATTRIBUTE_NAME[0], array[0]) );

}

CUIClimChange::~CUIClimChange(void)
{
}


CUIClimChange::CUIClimChange(const CUIClimChange& in)
{
	operator=(in);
}

void CUIClimChange::Reset()
{
	CUIWeather::Reset();

//	m_selection.Reset();
}

CUIClimChange& CUIClimChange::operator =(const CUIClimChange& in)
{
	if( &in != this)
	{
		CUIWeather::operator =(in);
//		m_selection = in.m_selection;
	}

	return *this;
}

bool CUIClimChange::operator ==(const CUIClimChange& in)const
{
	bool bEqual = true;

	if( CUIWeather::operator !=(in) )bEqual = false;
//	if( m_selection != in.m_selection)bEqual = false;
	
	return bEqual;
}

bool CUIClimChange::operator !=(const CUIClimChange& in)const
{
	return !operator ==(in);
}

CString CUIClimChange::GetValue(short type)const
{
	ERMsg msg;
	CString value;
	
	ASSERT( NB_ATTRIBUTE == 0); 
	switch(type)
	{
//	case I_PROVINCE: value = m_selection.ToString(); break;
	case -1:break;
	default: value = CUIWeather::GetValue(type); break;
	}

	return value;
}

void CUIClimChange::SetValue(short type, const CString& value)
{
	ASSERT( NB_ATTRIBUTE == 0); 
	switch(type)
	{
	//case I_PROVINCE: m_selection.FromString(value); break;
	case -1:break;
	default: CUIWeather::SetValue(type, value); break;
	}

}

/*
void CUIClimChange::GetSelection(short param, CSelectionDlgItemArray& items)const
{	
	ASSERT( param == I_PROVINCE);

	CProvinceSelection sel = m_selection;
	
	
	items.SetSize(CProvinceSelection::NB_PROVINCE);

	for(int i=0; i<CProvinceSelection::NB_PROVINCE; i++)
	{
		items[i].m_index = i;
		items[i].m_name = sel.GetName(i, CProvinceSelection::NAME);
		items[i].m_bSelected = sel.IsUsed(i);
	}

}

void CUIClimChange::SetSelection(short param, const CSelectionDlgItemArray& items)
{	
	ASSERT( param == I_PROVINCE);

	CProvinceSelection sel;

	ASSERT( items.GetSize() == CProvinceSelection::NB_PROVINCE);
	for(int i=0; i<items.GetSize();i++)
	{
		sel.SetUsed(items[i].m_index, items[i].m_bSelected);
	}
	
	m_selection = sel;
}
*/

bool CUIClimChange::Compare(const CParameterBase& in)const
{
	ASSERT(in.GetClassID() == GetClassID() );
	const CUIClimChange& info = dynamic_cast<const CUIClimChange&>(in);
	return operator==(info);
}

CParameterBase& CUIClimChange::Assign(const CParameterBase& in)
{
	ASSERT(in.GetClassID() == GetClassID() );
	const CUIClimChange& info = dynamic_cast<const CUIClimChange&>(in);
	return operator=(info);
}

#include "madis.h"
ERMsg CUIClimChange::Execute(CSCCallBack& callback)
{
	ERMsg msg;

	CString workingDir = CString(GetWorkingDir()) + "*.nc";

	callback.AddMessage(GetString(IDS_UPDATE_DIR ));
	callback.AddMessage(workingDir, 1);
	callback.AddMessage(GetString(IDS_UPDATE_FROM ) );
	callback.AddMessage(workingDir, 1 );
	callback.AddMessage("");


	CStringArray fileList;
	UtilWin::GetFileList(fileList, workingDir, true, true);

	callback.SetNbStep( fileList.GetSize() );
	
	CMADIS madis;
	//for(int i=0; i<fileList.GetSize(); i++)
	//{
	//	madis.ExtractLOC(fileList[i], callback);
	//}

	madis.ExtractLOC("D:\\MADIS\\Point\\SAO\\20090528_1600.nc", callback);
	madis.ExtractLOC("D:\\MADIS\\Point\\Profiler\\20090528_1800.nc", callback);
	return msg;
}



ERMsg CUIClimChange::PreProcess(CSCCallBack& callback)
{
	return ERMsg();
}

ERMsg CUIClimChange::GetStationList(CStringArray& stationList, CSCCallBack& callback)
{
	ERMsg msg;

	CPath workingDir = GetWorkingDir(); 

	//find all station in the directories
	callback.SetCurrentDescription(UtilWin::GetString( IDS_GET_STATION_LIST ));
	callback.SetCurrentStepRange( 0, 1, 1); 
	callback.SetStartNewStep();

	CString filePath = GetWorkingDir()+"*.txt";
	//CStringArray fileNameArray;
	
	UtilWin::GetFileList( stationList, filePath);
	
	//find all station available that meet criterious
	//for(int y=0; y<nbRow; y++)
	//{
	//	for(int x=0; x<nbCol; x++)
	//	{
	//		CString fileTitle;
	//		fileTitle.Format( "acu1961a1990_%3d", x+1);
	//		stationList.Add(fileTitle);
	//	}
	//}

	return msg;
}

ERMsg CUIClimChange::GetStation(const CString& stationName, CDailyStation& station, CSCCallBack& callback)
{
	ERMsg msg;

	CString workingDir = GetWorkingDir(); 
	CString filePath = workingDir + stationName + ".txt";

	CStdioFile file;
	msg = UtilWin::OpenFile( file, filePath, CFile::modeRead);
	if( !msg)
		return msg;

	CString line;
	for(int i=0; i<6; i++)
		file.ReadString(line);

	CYearPackage pakage;

	for(int y=0; y<30; y++)
	{
		CDailyData oneYearData;
		for(int jd=0; jd<365; jd++)
		{
			file.ReadString(line);

			int pos = 0;

			
			if( y==0 && jd==0)
			{
				for(int i=0; i<4; i++)
					line.Tokenize(" \t", pos);

				station.SetName(stationName);
				station.SetLat(atof(line.Tokenize(" \t", pos)));
				station.SetLon(atof(line.Tokenize(" \t", pos)));
				station.SetElev((int)atof(line.Tokenize(" \t", pos)));
			}
			else
			{
				for(int i=0; i<7; i++)
					line.Tokenize(" \t", pos);
			}
			
			
			oneYearData[jd][TMIN] = (float)atof(line.Tokenize(" \t", pos));
			oneYearData[jd][TMAX] = (float)atof(line.Tokenize(" \t", pos));
			oneYearData[jd][PRCP] = (float)atof(line.Tokenize(" \t", pos));

			//eliminate trace
			if( oneYearData[jd][PRCP] < 0.1 )
				oneYearData[jd][PRCP] = 0;
		}

		int year=atoi(line.Mid(9,4));

		CDailyYear yearData;
		yearData.SetYear(year );
		yearData.SetData(oneYearData);
		pakage.SetYear(yearData);
	}

	if( msg )
	{
		pakage.SetFileTitle(station.GetName());
		station.AddPakage(pakage);
	}


	return msg;
}


