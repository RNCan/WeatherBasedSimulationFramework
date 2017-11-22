#include "StdAfx.h"
#include "NormalDBFromNormal.h"

#include "MonthlyMeanGrid.h"
#include "NormalsDatabase.h"
#include "BasicRes.h"
#include "SYShowMessage.h"
#include "CommonRes.h"
#include "FileManagerRes.h"
#include "Resource.h"
#include "cpl_conv.h"
//#include "OURANOSData.h"


using namespace HOURLY_DATA;
using namespace std; using namespace stdString; using namespace CFL;
//using namespace CFL;
//*********************************************************************

const char* CNormalDBFromNormal::ATTRIBUTE_NAME[NB_ATTRIBUTE] = { "InputFilePath", "OutputFilePath", "DeleteDB", "MMGFilepath", "NormalPeriod", "NbNeighbor", "MaxDistance", "Power" };
const char* CNormalDBFromNormal::CLASS_NAME = "NormalDBFromNormalDB";


CNormalDBFromNormal::CNormalDBFromNormal(void)
{
	if( !IsRegister( GetClassID() ) )
	{
		InitClass();
	}

	Reset();
}

int GetPeriodBegin(int i)
{
	//if( i<=2 )
	return 1961+10*i;

	//return 1960+10*i;
}

//int GetPeriodEnd(int i)
//{
//	//if( i<=2 )
//	return 1990+10*i;
//
//	//return 1989+10*i;
//}

void CNormalDBFromNormal::InitClass(const StringVector& option)
{
	GetParamClassInfo().m_className = GetString( IDS_SOURCENAME_NORMAL3 );

	CToolsBase::InitClass(option);
	//init static 
	ASSERT( GetParameters().size() < I_NB_ATTRIBUTE);

	StringVector properties(IDS_PROPERTIES_NORMAL3, "|;");
	ASSERT( properties.size() == NB_ATTRIBUTE);
	StringVector period; 
	for(int i=0; i<12; i++)
	{
		string tmp = FormatA("%d-%d", 1961+10*i, 1990+10*i);
		period.push_back(tmp);
	}


	
	string filter = GetString( IDS_STR_FILTER_NORMALS);
	GetParameters().push_back( CParamDef(CParamDef::FILEPATH, ATTRIBUTE_NAME[0], properties[0], filter) );
	GetParameters().push_back( CParamDef(CParamDef::FILEPATH, ATTRIBUTE_NAME[1], properties[1], filter) );
	GetParameters().push_back( CParamDef(CParamDef::BOOL, ATTRIBUTE_NAME[2], properties[2]) );
	GetParameters().push_back( CParamDef(CParamDef::FILEPATH, ATTRIBUTE_NAME[3], properties[3], "Montly Mean Grid(*.mmg)|*.mmg||") );
	
	
	GetParameters().push_back( CParamDef(CParamDef::COMBO, CParamDef::BY_NUMBER, ATTRIBUTE_NAME[4], properties[4], period, "3") );
	GetParameters().push_back( CParamDef(CParamDef::EDIT, ATTRIBUTE_NAME[5], properties[5]) );
	GetParameters().push_back( CParamDef(CParamDef::EDIT, ATTRIBUTE_NAME[6], properties[6], "100000") );
	GetParameters().push_back( CParamDef(CParamDef::EDIT, ATTRIBUTE_NAME[7], properties[7], "2") );

}

CNormalDBFromNormal::~CNormalDBFromNormal(void)
{
}


CNormalDBFromNormal::CNormalDBFromNormal(const CNormalDBFromNormal& in)
{
	operator=(in);
}

void CNormalDBFromNormal::Reset()
{
	CToolsBase::Reset();

	
	m_inputFilePath.clear();
	m_outputFilePath.clear();
	m_bDeleteOldDB = true;

	m_MMGFilePath.clear();
	m_futurPeriod=11;
	m_nbNeighbor = 3;
	m_maxDistance = 150000;
	m_power=2;
}

CNormalDBFromNormal& CNormalDBFromNormal::operator =(const CNormalDBFromNormal& in)
{
	if( &in != this)
	{
		CToolsBase::operator =(in);
		
		m_inputFilePath = in.m_inputFilePath;
		m_outputFilePath = in.m_outputFilePath;
		m_bDeleteOldDB = in.m_bDeleteOldDB;
		m_MMGFilePath=in.m_MMGFilePath;
		m_futurPeriod=in.m_futurPeriod;
		m_nbNeighbor = in.m_nbNeighbor;
		m_maxDistance = in.m_maxDistance;
		m_power=in.m_power;
	}

	return *this;
}

bool CNormalDBFromNormal::operator ==(const CNormalDBFromNormal& in)const
{
	bool bEqual = true;

	if( CToolsBase::operator !=(in) )bEqual = false;
	
	if( m_inputFilePath != in.m_inputFilePath)bEqual = false;
	if( m_outputFilePath != in.m_outputFilePath)bEqual = false;
	if( m_bDeleteOldDB != in.m_bDeleteOldDB)bEqual = false;
	if( m_MMGFilePath!=in.m_MMGFilePath)bEqual = false;
	if( m_futurPeriod!=in.m_futurPeriod)bEqual = false;
	if( m_nbNeighbor != in.m_nbNeighbor)bEqual = false;
	if( m_maxDistance != in.m_maxDistance)bEqual = false;
	if( m_power!=in.m_power)bEqual = false;
	
	
	return bEqual;
}

bool CNormalDBFromNormal::operator !=(const CNormalDBFromNormal& in)const
{
	return !operator ==(in);
}


string CNormalDBFromNormal::GetValue(size_t type)const
{
	string str;
	
	ASSERT( NB_ATTRIBUTE == 8); 
	switch(type)
	{
	case I_INPUT: str = m_inputFilePath; break;
	case I_OUTPUT: str = m_outputFilePath; break;
	case I_DELETE_OLD_DB: str = m_bDeleteOldDB?"1":"0"; break;
	case I_MMG_FILEPATH:str = m_MMGFilePath; break;
	case I_NORMAL_PERIOD:str = ToString(m_futurPeriod); break;
	case I_NB_NEIGHBOR: str = ToString(m_nbNeighbor); break;
	case I_MAX_DISTANCE: str = ToString(m_maxDistance); break;
	case I_POWER: str = ToString(m_power); break;
	default: str = CToolsBase::GetValue(type); break;
	}
 
	return str;
}

void CNormalDBFromNormal::SetValue(size_t type, const string& str)
{
	ASSERT( NB_ATTRIBUTE == 8); 
	switch(type)
	{
	case I_INPUT: m_inputFilePath= str; break;
	case I_OUTPUT: m_outputFilePath=str; break;
	case I_DELETE_OLD_DB: m_bDeleteOldDB=ToBool(str); break; 
	case I_MMG_FILEPATH:m_MMGFilePath=str; break;
	case I_NORMAL_PERIOD:m_futurPeriod = ToInt(str); break;
	case I_NB_NEIGHBOR: m_nbNeighbor=ToInt(str); break;
	case I_MAX_DISTANCE: m_maxDistance=ToInt(str); break;
	case I_POWER: m_power = ToDouble(str); break;
	default: CToolsBase::SetValue(type, str ); break;

	}

}




ERMsg CNormalDBFromNormal::Execute(CFL::CCallback& callback)
{
	ERMsg msg;

	//COuranosDatabase ouranos("D:\\ouranos_data\\");
	//msg += ouranos.CreateMMGNew("adj", callback);
	//msg += ouranos.CreateMMGNew("adl", callback);

	

	//msg += data.ConvertData("adj", callback);
	//msg += data.ConvertData("adl", callback);
	//return msg;

	//msg += data.CreateNormalDatabase(1961,1962, "d:\\weather\\CC(new)\\test.Normals", callback);
	//return msg;


	//msg += data.CreateNormalDatabase(1961,1990, "d:\\weather\\CC(new)\\OuranosGrid 1961-1990.Normals", callback);
	//msg += data.CreateNormalDatabase(1971,2000, "d:\\weather\\CC(new)\\OuranosGrid 1971-2000.Normals", callback);
	//msg += data.CreateNormalDatabase(1981,2010, "d:\\weather\\CC(new)\\OuranosGrid 1981-2010.Normals", callback);
	//msg += data.CreateNormalDatabase(1991,2020, "d:\\weather\\CC(new)\\OuranosGrid 1991-2020.Normals", callback);
	//msg += ouranos.CreateNormalDatabase(2001,2030, "d:\\weather\\OuranosGrid 2001-2030.Normals", callback);
	//msg += ouranos.CreateNormalDatabase(2011,2040, "d:\\weather\\OuranosGrid 2011-2040.Normals", callback);
	//msg += ouranos.CreateNormalDatabase(2021,2050, "d:\\weather\\OuranosGrid 2021-2050.Normals", callback);
	//msg += ouranos.CreateNormalDatabase(2031,2060, "d:\\weather\\CC(new)\\OuranosGrid 2031-2060.Normals", callback);
	//msg += ouranos.CreateNormalDatabase(2041,2070, "d:\\weather\\CC(new)\\OuranosGrid 2041-2070.Normals", callback);
	//msg += ouranos.CreateNormalDatabase(2051,2080, "d:\\weather\\CC(new)\\OuranosGrid 2051-2080.Normals", callback);
	//msg += ouranos.CreateNormalDatabase(2061,2090, "d:\\weather\\CC(new)\\OuranosGrid 2061-2090.Normals", callback);
	//msg += ouranos.CreateNormalDatabase(2071,2100, "d:\\weather\\CC(new)\\OuranosGrid 2071-2100.Normals", callback);
	//
	//return msg;

	//COuranosDatabase data("D:\\ouranos_data\\");//("U:\\BioSIM_Weather\\ClimaticChange\\ouranos_data\\");
	//return data.CreateDailyDatabase("D:\\ouranos_data\\OURANOS 1961-2100.DailyStations", callback);//("U:\\BioSIM_Weather\\ClimaticChange\\ouranos_data\\OURANOS 1961-2100.DailyStations", callback);
	//int t = sizeof( CNormalsStation );
	//return data.CreateMMGNew(callback);

	//projPJ src = pj_init_plus("+proj=latlong +datum=NAD83");
	//projPJ dst = pj_init_plus("+proj=stere +lat_0=90 +lat_ts=60 +lon_0=-115 +k=1 +x_0=2700000 +y_0=8023500 +a=6370997 +b=6370997 +units=m +no_defs");
	//
	//int i1 = pj_is_latlong(src);
	//int i2 = pj_is_geocent(src);
	//int i3 = pj_is_latlong(dst);
	//int i4 = pj_is_geocent(dst);

	//double x = CFL::Deg2Rad(-110);
	//double y = CFL::Deg2Rad(58);
	//pj_transform( src, dst, 1, 1,&x, &y, NULL );


	callback.AddMessage( GetString(IDS_CREATE_DATABASE) );
	callback.AddMessage(GetAbsoluteFilePath(m_outputFilePath), 1);

	string outputFilePath( GetAbsoluteFilePath(m_outputFilePath) );
	SetFileExtension(outputFilePath, CNormalsDatabase::DATABASE_EXT);

	
	if( m_bDeleteOldDB )
		msg = CNormalsDatabase().DeleteDatabase(outputFilePath, callback);


	return CreateNormalDatabase(outputFilePath, callback);
}

ERMsg CNormalDBFromNormal::CreateNormalDatabase(const string& outputFilePath, CFL::CCallback& callback)
{
	ERMsg msg;

	CPLSetConfigOption( "GDAL_CACHEMAX","1024");


	CNormalsDatabase normalDBIn;
	msg += normalDBIn.Open(GetAbsoluteFilePath(m_inputFilePath));
	
	CNormalsDatabase normalDBOut;
	int firstYear = 1961 + 10 * m_futurPeriod;
//	normalDBOut.SetBeginYear(1961+10*m_futurPeriod);
	//normalDBOut.SetEndYear(1990+10*m_futurPeriod);
	msg += normalDBOut.Open(outputFilePath, CNormalsDatabase::modeEdit);

	CMonthlyMeanGrid mmg;
	msg += mmg.Open(GetAbsoluteFilePath(m_MMGFilePath));

	if( !msg)
		return msg;

	callback.SetCurrentDescription("Create " + GetFileTitle(outputFilePath));
	callback.AddMessage("Create " + GetFileTitle(outputFilePath));
	callback.AddMessage("first ref year: " + ToString(firstYear));
	callback.SetNbStep(normalDBIn.size());
	

	for (size_t i = 0; i<normalDBIn.size() && msg; i++)
	{
		CNormalsStation station;
		normalDBIn.Get(station, i);
	
		if (mmg.UpdateData(firstYear, GetPeriodBegin(m_futurPeriod), m_nbNeighbor, m_maxDistance, m_power, station))
			normalDBOut.Add(station);

		msg+=callback.StepIt();
	}

	mmg.Close();
	normalDBOut.Close();

	if(msg)
	{
		msg = normalDBOut.Open(outputFilePath);
	}

	return msg;
}


bool CNormalDBFromNormal::Compare(const CParameterBase& in)const
{
	ASSERT(in.GetClassID() == GetClassID() );
	const CNormalDBFromNormal& info = dynamic_cast<const CNormalDBFromNormal&>(in);
	return operator==(info);
}

CParameterBase& CNormalDBFromNormal::Assign(const CParameterBase& in)
{
	ASSERT(in.GetClassID() == GetClassID() );
	const CNormalDBFromNormal& info = dynamic_cast<const CNormalDBFromNormal&>(in);
	return operator=(info);
}

