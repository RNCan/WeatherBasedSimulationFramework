#include "StdAfx.h"
#include "DailyDBFromHourly.h"
#include "MonthlyMeanGrid.h"

#include "NormalsStation.h"
#include "NormalsDatabase.h"
#include "DailyDatabase.h"
#include "DailyStation.h"


#include "SYShowMessage.h"
#include "BasicRes.h"
#include "CommonRes.h"
#include "FileManagerRes.h"
#include "Task.h"
#include "Resource.h"
#include "AdvancedNormalStation.h"
#include "cpl_conv.h"


using namespace std;
using namespace stdString;
using namespace CFL;
using namespace HOURLY_DATA;
//using namespace UtilWin;
//using namespace CFL;
//*********************************************************************

const char* CDailyDBFromHourly::ATTRIBUTE_NAME[NB_ATTRIBUTE] = { "InputFilePath", "OutputFilePath", "DeleteDB", "FirstYear", "LastYear"};
const char* CDailyDBFromHourly::CLASS_NAME = "DailyDBFromHourlyDB";


CDailyDBFromHourly::CDailyDBFromHourly(void)
{
	if( !IsRegister( GetClassID() ) )
	{
		InitClass();
	}

	Reset();
}

void CDailyDBFromHourly::InitClass(const StringVector& option)
{
	GetParamClassInfo().m_className = GetString(IDS_SOURCENAME_DAILY_FROM_HOURLY);

	CToolsBase::InitClass(option);
	//init static 
	ASSERT( GetParameters().size() < I_NB_ATTRIBUTE);

	StringVector properties(IDS_PROPERTIES_DAILY_FROM_HOURLY, "|;");
	ASSERT( properties.size() == NB_ATTRIBUTE);
	
	StringVector period; 
	for(int i=0; i<12; i++)
		period.push_back(FormatA("%d-%d", 1961 + 10 * i, 1990 + 10 * i));

	string filter1 = GetString( IDS_STR_FILTER_HOURLY);
	string filter2 = GetString( IDS_STR_FILTER_DAILY);
	GetParameters().push_back( CParamDef(CParamDef::FILEPATH, ATTRIBUTE_NAME[0], properties[0], filter1) );
	GetParameters().push_back( CParamDef(CParamDef::FILEPATH, ATTRIBUTE_NAME[1], properties[1], filter2) );
	GetParameters().push_back( CParamDef(CParamDef::BOOL, ATTRIBUTE_NAME[2], properties[2]) );
	GetParameters().push_back( CParamDef(CParamDef::EDIT, ATTRIBUTE_NAME[3], properties[3]) );
	GetParameters().push_back( CParamDef(CParamDef::EDIT, ATTRIBUTE_NAME[4], properties[4]) );
}

CDailyDBFromHourly::~CDailyDBFromHourly(void)
{
}


CDailyDBFromHourly::CDailyDBFromHourly(const CDailyDBFromHourly& in)
{
	operator=(in);
}

void CDailyDBFromHourly::Reset()
{
	CToolsBase::Reset();

	m_firstYear=1961;
	m_lastYear=1990;

	m_inputFilePath.clear();
	m_outputFilePath.clear();
	m_bDeleteOldDB = true;
}

CDailyDBFromHourly& CDailyDBFromHourly::operator =(const CDailyDBFromHourly& in)
{
	if( &in != this)
	{
		CToolsBase::operator =(in);
		m_firstYear=in.m_firstYear;
		m_lastYear=in.m_lastYear;
		m_inputFilePath = in.m_inputFilePath;
		m_outputFilePath = in.m_outputFilePath;
		m_bDeleteOldDB = in.m_bDeleteOldDB;
	}

	return *this;
}

bool CDailyDBFromHourly::operator ==(const CDailyDBFromHourly& in)const
{
	bool bEqual = true;

	if( CToolsBase::operator !=(in) )bEqual = false;
	if( m_firstYear != in.m_firstYear )bEqual = false;
	if( m_lastYear !=  in.m_lastYear )bEqual = false;
	if( m_inputFilePath != in.m_inputFilePath)bEqual = false;
	if( m_outputFilePath != in.m_outputFilePath)bEqual = false;
	if( m_bDeleteOldDB != in.m_bDeleteOldDB)bEqual = false;

	return bEqual;
}

bool CDailyDBFromHourly::operator !=(const CDailyDBFromHourly& in)const
{
	return !operator ==(in);
}


string CDailyDBFromHourly::GetValue(size_t type)const
{
	string str;
	
	ASSERT( NB_ATTRIBUTE == 5); 
	switch(type)
	{
	case I_INPUT:			str = m_inputFilePath; break;
	case I_OUTPUT:			str = m_outputFilePath; break;
	case I_DELETE_OLD_DB:	str = m_bDeleteOldDB?"1":"0"; break;
	case I_FIRST_YEAR:		str = ToString(m_firstYear); break;
	case I_LAST_YEAR:		str = ToString(m_lastYear); break;
	default: str = CToolsBase::GetValue(type); break;
	}
 
	return str;
}

void CDailyDBFromHourly::SetValue(size_t type, const string& str)
{
	ASSERT( NB_ATTRIBUTE == 5); 
	switch(type)
	{
	case I_INPUT: m_inputFilePath= str; break;
	case I_OUTPUT: m_outputFilePath=str; break;
	case I_DELETE_OLD_DB: m_bDeleteOldDB=ToBool(str); break;
	case I_FIRST_YEAR: m_firstYear=ToInt(str); break;
	case I_LAST_YEAR: m_lastYear = ToInt(str); break;
	default: CToolsBase::SetValue(type, str ); break;

	}

}



bool CDailyDBFromHourly::Compare(const CParameterBase& in)const
{
	ASSERT(in.GetClassID() == GetClassID() );
	const CDailyDBFromHourly& info = dynamic_cast<const CDailyDBFromHourly&>(in);
	return operator==(info);
}

CParameterBase& CDailyDBFromHourly::Assign(const CParameterBase& in)
{
	ASSERT(in.GetClassID() == GetClassID() );
	const CDailyDBFromHourly& info = dynamic_cast<const CDailyDBFromHourly&>(in);
	return operator=(info);
}



ERMsg CDailyDBFromHourly::Execute(CFL::CCallback& callback)
{
	ERMsg msg;

	//CPLSetConfigOption( "GDAL_CACHEMAX","128");
	//for(int y=0; y<12&&msg; y++)
	//{
	//	short firstYear=1961+10*y;
	//	short lastYear=firstYear+30-1;

	//	string filePath;
	//	filePath.Format("d:\\weather\\OuranosGrid%d-%d.Normals", firstYear, lastYear);
	//	CNormalsDatabase normalFile;
	//	msg+=normalFile.Open(filePath, CNormalsDatabase::modeEdit);
	//	if( msg)
	//	{
	//		for(int i=0; i<normalFile.size(); i++)
	//		{
	//			CNormalsStation station;
	//			normalFile.GetAt(i, station);
	//			if( station.GetElev() == 0)
	//			{
	//				station.SetElev( CFL::Rand(0,5) );
	//				msg += normalFile.SetAt(i, station);
	//			}
	//		}
	//	}
	//}

	//	COuranosDatabase ouranosDatabase(m_MMGFilePath);
	//ouranosDatabase.ExtractMask("c:\\temp\\grid\\mask.flt", callback);

	/*for(int y=0; y<12&&msg; y++)
	{
	short firstYear=1961+10*y;
	short lastYear=firstYear+30-1;

	string filePath;
	filePath.Format("C:\\ouranos_data\\Normals\\OuranosGrid%d-%d.Normals", firstYear, lastYear);
	msg += ouranosDatabase.CreateNormal( firstYear, lastYear, filePath, callback);
	}

	return msg;*/


	//CGeoPointWP pt(-70.37, 46.8, CProjection(CProjection::GEO) );
	//CClimaticChange cc;
	//msg += cc.Open(m_MMGFilePath);
	//msg += cc.ExportMonthlyValue(1971, 2100, pt, "C:\\temp\\testCC.csv", callback );
	//return msg;

	//msg+=ouranosDatabase.ExtractTopo("D:\\Travail\\OURANOS\\MapInput\\Topo.flt", callback);
	//msg+=ouranosDatabase.ExtractRealTopo("D:\\Travail\\OURANOS\\MapInput\\RealTopoHight.flt", true, callback);
	//	msg += ouranosDatabase.CreateCCGridFromNormals( filePathIn, pathOut, callback );
	//	return msg;

	/*	COuranosDatabase ouranosDatabase(m_MMGFilePath);
	for(int m=0;m<12;m++)
	{
	string tmp1;
	string tmp2;
	string tmp3;
	tmp1.Format( "D:\\Travail\\OURANOS\\MapOutput\\testtmin%d.flt", m+1);
	tmp2.Format( "D:\\Travail\\OURANOS\\MapOutput\\testtmin%ddelta.flt", m+1);
	tmp3.Format( "D:\\Travail\\OURANOS\\MapOutput\\DiffTmin%d.flt", m+1);
	ouranosDatabase.GetDiffMap(tmp1, tmp2, tmp3, true, callback);

	tmp1.Format( "D:\\Travail\\OURANOS\\MapOutput\\testTmax%d.flt", m+1);
	tmp2.Format( "D:\\Travail\\OURANOS\\MapOutput\\testTmax%ddelta.flt", m+1);
	tmp3.Format( "D:\\Travail\\OURANOS\\MapOutput\\DiffTmax%d.flt", m+1);
	ouranosDatabase.GetDiffMap(tmp1, tmp2, tmp3, true, callback);

	tmp1.Format( "D:\\Travail\\OURANOS\\MapOutput\\testPrcp%d.flt", m+1);
	tmp2.Format( "D:\\Travail\\OURANOS\\MapOutput\\testPrcp%ddelta.flt", m+1);
	tmp3.Format( "D:\\Travail\\OURANOS\\MapOutput\\DiffPrcp%d.flt", m+1);
	ouranosDatabase.GetDiffMap(tmp1, tmp2, tmp3, false, callback);
	}

	return msg;
	*/


	//msg+=ouranosDatabase.GetDiffMap("C:\\temp\\Maps\\Precip2001-2030.flt","C:\\temp\\Maps\\Precip1971-2000.flt","C:\\temp\\Maps\\DiffPrecip2001-2030.flt",false,callback);
	//msg+=ouranosDatabase.GetDiffMap("C:\\ouranos_data\\MapOutput\\prcp 2001-2030.flt", "C:\\ouranos_data\\MapOutput\\prcp 1971-2000.flt", "C:\\ouranos_data\\MapOutput\\Diffprcp 2001-2030.flt",false,callback);
	//msg+=ouranosDatabase.GetDiffMap("C:\\ouranos_data\\MapOutput\\Diffprcp 2001-2030.flt", "C:\\temp\\Maps\\DiffPrecip2001-2030.flt", "C:\\temp\\MapsDiff\\DiffPrecip2001-2030.flt", true, callback);

	/*
	for(int y=1; y<4&&msg; y++)
	{
	short firstYear=1971+30*y;
	short lastYear=firstYear+30-1;

	string filePathIn;
	filePathIn.Format("d:\\weather\\OuranosGrid%d-%d.Normals",firstYear, lastYear);
	string pathOut;
	pathOut.Format("%sadj\\Maps\\%d-%d\\", m_MMGFilePath, firstYear, lastYear);
	CreateMultipleDir( pathOut );

	msg += ouranosDatabase.CreateCCGridFromNormals( filePathIn, pathOut, callback );

	for(int m=0; m<12&&msg; m++)
	{
	for(int v=0; v<NB_VAR_BASE&&msg; v++)
	{
	string filePath;
	filePath.Format("%sadj\\Maps\\%d-%d\\%s%s%d-%d.bil", m_MMGFilePath, firstYear, lastYear,CWeatherDefine::GetFormatNameD(v), CFL::GetMonthName(m, false), firstYear, lastYear);
	msg += ouranosDatabase.CreateCCGrid( v, firstYear, lastYear, m, filePath, callback);

	//string filePath2;
	//filePath2.Format("C:\\temp\\MapsMonthly\\%s%s%d-%d.csv", CWeatherDefine::GetFormatNameD(v), CFL::GetMonthName(m, false), firstYear, lastYear);
	//for(int y2=0; y2<30; y2++)
	//	ouranosDatabase.ExportData(firstYear+y2,m,CGeoPointWP(-71.3799999873, 46.8,CProjection(CProjection::GEO)), filePath2, callback);

	//filePath.Format("C:\\temp\\Maps\\%s%d-%d.bil", CWeatherDefine::GetFormatNameD(v), firstYear, lastYear);
	//msg += ouranosDatabase.CreateCCGrid( v, firstYear, lastYear, -1, filePath, callback);
	}
	}
	}

	return msg;
	*/

	//msg += ouranosDatabase.CreateCCGrid( PRCP, 1971, CFL::JULY, "C:\\temp\\prcpJuly1971-2000.bil", callback);
	//msg += ouranosDatabase.CreateCCGrid( PRCP, 1971, CFL::AUGUST, "C:\\temp\\prcpAugust1971-2000.bil", callback);
	//msg += ouranosDatabase.CreateCCGrid( PRCP, 1971, CFL::SEPTEMBER, "C:\\temp\\prcpSeptember1971-2000.bil", callback);

	//msg += ouranosDatabase.CreateCCGrid( PRCP, 2001, CFL::JUNE, "C:\\temp\\prcpJune2001-2030.bil", callback);
	//msg += ouranosDatabase.CreateCCGrid( PRCP, 2001, CFL::JULY, "C:\\temp\\prcpJuly2001-2030.bil", callback);
	//msg += ouranosDatabase.CreateCCGrid( PRCP, 2001, CFL::AUGUST, "C:\\temp\\prcpAugust2001-2030.bil", callback);
	//msg += ouranosDatabase.CreateCCGrid( PRCP, 2001, CFL::SEPTEMBER, "C:\\temp\\prcpSeptember2001-2030.bil", callback);
	//msg += ouranosDatabase.CreateCCGrid( TMIN, "C:\\temp\\adjTmin.bil", callback);
	//msg += ouranosDatabase.CreateCCGrid( TMAX, "C:\\temp\\adjTmax.bil", callback);
	//msg += ouranosDatabase.CreateCCGrid( PRCP, "C:\\temp\\adjPrcp.bil", callback);
	//msg += ouranosDatabase.CreateCCGrid( WNDS, "C:\\temp\\adjWnds.bil", callback);
	//msg += ouranosDatabase.CreateCCGrid( RELH, "C:\\temp\\adjrelh.bil", callback);


	//bool bRep = ouranosDatabase.VerifyZero(PRCP);
	//if( bRep )
	//	msg.ajoute("PRCP ok");
	//else msg.ajoute("PRCP pas ok");

	//ouranosDatabase.ExtractTopo("c:\\temp\\topo.nc");
	//return msg;
	//ouranosDatabase.CreateCCGrid(TMIN, 2071, CFL::JANUARY, "c:\\temp\\grid\\2071-2100\\tmin2071-2100jan.flt");
	//ouranosDatabase.CreateCCGrid(TMIN, 2071, CFL::JULY, "c:\\temp\\grid\\2071-2100\\tmin2071-2100jul.flt");
	//ouranosDatabase.CreateCCGrid(TMAX, 2071, CFL::JANUARY, "c:\\temp\\grid\\2071-2100\\tmax2071-2100jan.flt");
	//ouranosDatabase.CreateCCGrid(TMAX, 2071, CFL::JULY, "c:\\temp\\grid\\2071-2100\\tmax2071-2100jul.flt");
	//ouranosDatabase.CreateCCGrid(PRCP, 2071, CFL::JANUARY, "c:\\temp\\grid\\2071-2100\\prcp2071-2100jan.flt");
	//ouranosDatabase.CreateCCGrid(PRCP, 2071, CFL::JULY, "c:\\temp\\grid\\2071-2100\\prcp2071-2100jul.flt");
	//ouranosDatabase.CreateCCGrid(RELH, 2071, CFL::JANUARY, "c:\\temp\\grid\\2071-2100\\relh2071-2100jan.flt");
	//ouranosDatabase.CreateCCGrid(RELH, 2071, CFL::JULY, "c:\\temp\\grid\\2071-2100\\relh2071-2100jul.flt");
	//ouranosDatabase.CreateCCGrid(WNDS, 2071, CFL::JANUARY, "c:\\temp\\grid\\2071-2100\\wnds2071-2100jan.flt");
	//ouranosDatabase.CreateCCGrid(WNDS, 2071, CFL::JULY, "c:\\temp\\grid\\2071-2100\\wnds2071-2100jul.flt");


	//msg = CTYNClimaticChange::CreateBillFromATEAM(m_MMGFilePath, callback);
	//return msg;

	//msg = CCerfacsMMG::CreateMMG(m_MMGFilePath, m_outputFilePath, callback);
	//return msg;

	//msg=CCerfacsClimaticChange::CreateGrid(m_MMGFilePath+"Cerfacs_WHC.loc",m_MMGFilePath+"Cerfacs_WHC.flt", callback);
	//msg+=CCerfacsClimaticChange::CreateGrid(m_MMGFilePath+"Cerfacs_Altitude.loc",m_MMGFilePath+"Cerfacs_Altitude.flt", callback);

	//msg = CCerfacsMMG::CompareWHC(m_MMGFilePath+"Cerfacs_WHC.loc", m_MMGFilePath+"AWC_HWSD_30as(2m).flt",  "C:\\Travail\\IsabelChuine\\water holding capacity\\Webb\\1deg-WHC.flt","C:\\Travail\\IsabelChuine\\water holding capacity\\IGBP\\pawc.flt",m_MMGFilePath+"ComparisonWHC.csv");
	//return msg;




	//msg=CCerfacsClimaticChange::CreateDailyFile(m_MMGFilePath,1971,2000,false,callback);
	//return msg;

	//COuranosDatabase data(m_MMGFilePath);
	//int t = sizeof( CNormalsStation );
	//data.CreateMMGNew(callback);

	//return msg;


	//if( m_nbYearMin <= 1)
	//{
	//	msg.ajoute( "the number of year must be greater then 1");
	//	return msg;
	//}

	//callback.AddMessage( GetString(IDS_CREATE_DATABASE) );
	//callback.AddMessage(GetAbsoluteFilePath(m_outputFilePath), 1);

	//string outputFilePath( GetAbsoluteFilePath(m_outputFilePath) );
	//SetFileExtension( outputFilePath, ".DailyStations");
	//
	//
	//if( m_bDeleteOldDB )
	//	msg = CDailyDatabase::DeleteDatabase(outputFilePath, callback);


	//CNormalFromDaily normalFromDaily;
	//normalFromDaily.m_bApplyCC=m_bApplyCC;
	//normalFromDaily.m_bCreateAll=false;
	//normalFromDaily.m_refPeriodIndex=m_referencePeriod;
	//normalFromDaily.m_CCPeriodIndex=m_futurPeriod;
	//normalFromDaily.m_inputDBFilePath= m_inputFilePath;
	//normalFromDaily.m_outputDBFilePath=m_outputFilePath;
	//
	//
	//normalFromDaily.m_firstYear= m_firstYear;
	//normalFromDaily.m_lastYear= m_lastYear;
	//normalFromDaily.m_nbYearMin= m_nbYearMin;
	//normalFromDaily.m_inputMMGFilePath= m_MMGFilePath;
	////normalFromDaily.m_firstApplyedCCYear= m_futurPeriod;
	//normalFromDaily.m_nbNeighbor = m_nbNeighbor;
	//normalFromDaily.m_maxDistance = m_maxDistance;
	//normalFromDaily.m_power = m_power;

	return msg;//normalFromDaily.CreateNormalDatabase(callback);
}
