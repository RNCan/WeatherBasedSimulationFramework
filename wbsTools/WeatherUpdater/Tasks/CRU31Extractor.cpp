#include "StdAfx.h"

#include <netcdf>

#include "CRU31Extractor.h"

//#include "dailyStation.h"
//#include "FileManager\DailyFile.h"
//#include "AdvancedNormalStation.h"
#include "FileManagerRes.h"
#include "NormalFile.h"
#include "SYShowMessage.h"
#include "CommonRes.h"
#include "mappingRes.h"
//#include "ShapefileBase.h"
#include "Resource.h"
#include "gridym.h"
#include "Statistic.h"
#include "GeomaticsTools\Source\GDALBasic.h"
#include "ogr_srs_api.h"

//#include "NETCDF\cxx\netcdfcpp.h"


using namespace WEATHER;
using namespace CFL;
using namespace netCDF;
using namespace UtilWin;
using namespace std;

const char* CCRU31Extractor::OBS_FILE_NAME[NB_VAR] = {"cru_ts_3_10.1901.2009.tmn.dat.nc", "cru_ts_3_10.1901.2009.tmx.dat.nc", "cru_ts_3_10.1901.2009.pre.dat.nc"};

static ERMsg CovertToTIFF(CString filePath, CString filePathOut, CFL::CCallback& callback)
{
	ERMsg msg;


	CGridYM test;
	msg += test.Open(filePath, CFile::modeRead);
	if( !msg )
		return msg;

	CStdStringVector options;
	options.push_back( CStdString("COMPRESS=LZW") );
	options.push_back( CStdString("BIGTIFF=IF_NEEDED") );
	//options.push_back( CStdString("INTERLEAVE=BAND") );

	CGeoExtents extents;
	extents.m_xMin = test.GetBoundingBox().m_xMin;
	extents.m_xMax = test.GetBoundingBox().m_xMax;
	extents.m_yMin = test.GetBoundingBox().m_yMin;
	extents.m_yMax = test.GetBoundingBox().m_yMax;
	extents.m_xSize = test.GetSizeX();
	extents.m_ySize =  test.GetSizeY();
	ASSERT( extents.XRes() == 0.5);
	


	CGDALDatasetEx grid;
	msg += grid.Create((LPCTSTR)filePathOut,"GTIFF",GDT_Float32,1,test.GetNoData(),options,true,extents,SRS_WKT_WGS84);
	
	if( !msg)
		return msg;

	CBoxYM box;
	test.ReadCell(CGeoPoint(1.31666, 52.6333, CProjection::GEO), box);
	test.ReadCell(CPoint(362, 285), box);
	float v1 = test.ReadCell(CGeoPoint(1.31666, 52.6333, CProjection::GEO), 0, 6);
	float v2 = test.ReadCell(CPoint(362, 285), 0, 6);

	//
	callback.SetNbStep(test.GetSizeX()*test.GetSizeY());
	GDALRasterBand* pBand = grid->GetRasterBand(1);

	std::vector<float> data(test.GetSizeX());
	for(int y=0; y<test.GetSizeY()&&msg; y++)
	{
		for(int x=0; x<test.GetSizeX()&&msg; x++)	
		{
			data[x] = test.ReadCell(CPoint(x,test.GetSizeY()-1-y), 0, 0);
		}

		pBand->RasterIO(GF_Write,0,y,data.size(),1, &(data[0]),data.size(), 1, GDT_Float32, 0, 0 );
		msg += callback.StepIt();
	}

	test.Close();
	grid.Close();

	return msg;
}

CString CCRU31Extractor::GetFilePath( short v)
{
	ASSERT( v>=0 && v<NB_VAR);

	CString fileName;
	fileName.Format("%s%s", m_CRUPath, OBS_FILE_NAME[v]);

	return fileName;
}


//*********************************************************************
const char* CCRU31Extractor::ATTRIBUTE_NAME[] = { "CRU_PATH", "INPUT_FILEPATH", "OUTPUT_FILEPATH", "FIRST+_YEAR", "LAST_YEAR", "NB_POINT_SEARCH", "MAX_DISTANCE"};
const char* CCRU31Extractor::CLASS_NAME = "CRU31_EXTRACTOR";


CCRU31Extractor::CCRU31Extractor(void)
{
	if( !IsRegister( GetClassID() ) )
	{
		InitClass();
	}

	Reset();

	
}

void CCRU31Extractor::InitClass(const CStringArray& option)
{
	GetParamClassInfo().m_className.LoadString( IDS_SOURCENAME_CRU_EXTRACTOR);

	CToolsBase::InitClass(option);

	ASSERT( GetParameters().GetSize() <= I_NB_ATTRIBUTE);


	CStringArrayEx properties( IDS_PROPERTIES_CRU_EXTRACTOR);
	ASSERT( properties.GetSize() == NB_ATTRIBUTE);

	
	CString filter1 = GetString( IDS_CMN_FILTER_NORMALS);
	GetParameters().Add( CParamDef(CParamDef::PATH, ATTRIBUTE_NAME[0], properties[0], "NetCDF files(.nc)|*.nc||" ) );
	GetParameters().Add( CParamDef(CParamDef::FILEPATH, ATTRIBUTE_NAME[1], properties[1], filter1 ) );
	GetParameters().Add( CParamDef(CParamDef::FILEPATH, ATTRIBUTE_NAME[2], properties[2], filter1 ) );
	GetParameters().Add( CParamDef(CParamDef::EDIT, ATTRIBUTE_NAME[3], properties[3], "1901") );
	GetParameters().Add( CParamDef(CParamDef::EDIT, ATTRIBUTE_NAME[4], properties[4], "2009") );
	GetParameters().Add( CParamDef(CParamDef::EDIT, ATTRIBUTE_NAME[5], properties[5], "3") );
	GetParameters().Add( CParamDef(CParamDef::EDIT, ATTRIBUTE_NAME[6], properties[6], "100000") );
	
}

CCRU31Extractor::~CCRU31Extractor(void)
{
}


//CCRU31Extractor::CCRU31Extractor(const CCRU31Extractor& in)
//{
//	operator=(in);
//}

void CCRU31Extractor::Reset()
{
	m_CRUPath.Empty();
	m_inputFilePath.Empty();
	m_outputFilePath.Empty();
	m_firstYear=1901;
	m_lastYear=2009;
	m_nbPointSearch=3;
	m_maxDistance = 150000;
}

CCRU31Extractor& CCRU31Extractor::operator =(const CCRU31Extractor& in)
{
	if( &in != this)
	{
		CToolsBase::operator =(in);
		m_CRUPath=in.m_CRUPath;
		m_inputFilePath = in.m_inputFilePath;
		m_outputFilePath = in.m_outputFilePath;
		m_firstYear=in.m_firstYear;
		m_lastYear=in.m_lastYear;
		m_nbPointSearch=in.m_nbPointSearch;
		m_maxDistance = in.m_maxDistance;
	}

	return *this;
}

bool CCRU31Extractor::operator ==(const CCRU31Extractor& in)const
{
	bool bEqual = true;

	if( CToolsBase::operator !=(in))bEqual = false;
	if(	m_CRUPath!=in.m_CRUPath)bEqual = false;
	if( m_inputFilePath != in.m_inputFilePath)bEqual = false;
	if( m_outputFilePath != in.m_outputFilePath)bEqual = false;
	if( m_firstYear!=in.m_firstYear)bEqual = false;
	if( m_lastYear!=in.m_lastYear)bEqual = false;
	if( m_nbPointSearch!=in.m_nbPointSearch)bEqual = false;
	if( m_maxDistance != in.m_maxDistance)bEqual = false;

	return bEqual;
}

bool CCRU31Extractor::operator !=(const CCRU31Extractor& in)const
{
	return !operator ==(in);
}


CString CCRU31Extractor::GetValue(short type)const
{
	CString str;
	
	ASSERT( NB_ATTRIBUTE == 7); 
	switch(type)
	{
	case I_CRU_PATH: str = m_CRUPath;break;
	case I_INPUT_FILEPATH: str = m_inputFilePath;break;
	case I_OUTPUT_FILEPATH: str = m_outputFilePath; break;
	case I_FIRST_YEAR: str = ToString(m_firstYear); break;
	case I_LAST_YEAR: str = ToString(m_lastYear); break;
	case I_NB_POINT_SEARCH: str = ToString(m_nbPointSearch); break;
	case I_MAX_DISTANCE: str = ToString(m_maxDistance); break;
	default: str = CToolsBase::GetValue(type); break;
	}

	return str;
}

void CCRU31Extractor::SetValue(short type, const CString& str)
{
	ASSERT( NB_ATTRIBUTE == 7); 
	switch(type)
	{
	case I_CRU_PATH: m_CRUPath=str;break;
	case I_INPUT_FILEPATH: m_inputFilePath = str;break;
	case I_OUTPUT_FILEPATH: m_outputFilePath=str; break;
	case I_FIRST_YEAR: m_firstYear = ToInt(str); break;
	case I_LAST_YEAR: m_lastYear = ToInt(str); break;
	case I_NB_POINT_SEARCH: m_nbPointSearch = ToInt(str); break;
	case I_MAX_DISTANCE: m_maxDistance=ToInt(str); break;
	default: CToolsBase::SetValue(type, str); break;
	}

}


bool CCRU31Extractor::Compare(const CParameterBase& in)const
{
	ASSERT(in.GetClassID() == GetClassID() );
	const CCRU31Extractor& info = dynamic_cast<const CCRU31Extractor&>(in);
	return operator==(info);
}

CParameterBase& CCRU31Extractor::Assign(const CParameterBase& in)
{
	ASSERT(in.GetClassID() == GetClassID() );
	const CCRU31Extractor& info = dynamic_cast<const CCRU31Extractor&>(in);
	return operator=(info);
}

ERMsg CCRU31Extractor::Execute(CFL::CCallback& callback)
{
	ERMsg msg;



	callback.SetCurrentDescription("Extraction");
	callback.SetNbStep(31143);
	
	m_inputFilePath.Trim(" ");
	m_outputFilePath.Trim(" ");

	CString inputFilePath(GetAbsoluteFilePath(m_inputFilePath));
	CString outputFilePath(GetAbsoluteFilePath(m_outputFilePath));
	UtilWin::SetFileExtension( outputFilePath, ".Normals");

	msg += CNormalsDatabase::DeleteDatabase(outputFilePath, callback);
	
	callback.AddMessage( GetString(IDS_CREATE_DATABASE) );
	callback.AddMessage(outputFilePath, 1);
	callback.AddMessage("");
	
	//NcError::set_err( NcError::silent_nonfatal );
  
	
	NcFile filePre(LPCTSTR(m_CRUPath + OBS_FILE_NAME[PRCP31]), NcFile::read);
	NcFile fileTmn(LPCTSTR(m_CRUPath + OBS_FILE_NAME[TMIN31]), NcFile::read);	
	NcFile fileTmx(LPCTSTR(m_CRUPath + OBS_FILE_NAME[TMAX31]), NcFile::read);	

	ASSERT( !filePre.isNull() && !fileTmn.isNull() & !fileTmx.isNull());
	

	NcVar& varPre = filePre.getVar("pre");
	NcVar& varTmn = fileTmn.getVar("tmn");
	NcVar& varTmx = fileTmx.getVar("tmx");
	
	ENSURE( varPre.getDimCount() == 3 && varTmn.getDimCount() == 3&& varTmx.getDimCount() == 3);

	size_t d1 = varPre.getDim(0).getSize();
	size_t d2 = varPre.getDim(1).getSize();
	size_t d3 = varPre.getDim(2).getSize();

	CGridYM TYN_elev;
	

	CString filePath = m_CRUPath + "halfdeg.elv";
	msg += TYN_elev.Open(GetAbsoluteFilePath(filePath), CFile::modeRead);

	CNormalsDatabase inputDB;
	msg += inputDB.Open( GetAbsoluteFilePath(m_inputFilePath));

	CNormalsDatabase outputDB;
	msg += outputDB.Open( outputFilePath, CNormalsDatabase::modeEdit);
	if( !msg)
		return msg;
	
	
	callback.SetNbStep(TYN_elev.GetSizeX()*TYN_elev.GetSizeY());


	int nbYears = m_lastYear-m_firstYear+1;
	size_t firstMonth = (m_firstYear-1901)*12;
	size_t nbMonths = nbYears*12;
	vector<size_t> startp(3);
	vector<size_t> countp(3);
	
	ASSERT( firstMonth>=0 && firstMonth<d1);
	ASSERT( firstMonth+nbMonths<=d1);

	
	const CGeoPoint& refP = TYN_elev.GetBoundingBox().LowerLeft();
	for(int y=0; y<TYN_elev.GetSizeY()&&msg; y++)
	{
		for(int x=0; x<TYN_elev.GetSizeX()&&msg; x++)	
		{
			int elev = (int)TYN_elev.ReadCell(CPoint(x,y), 0, 0);

			if(elev > TYN_elev.GetNoData() )
			{
				CNormalsStation station;
				CString name;
				name.Format( "%03d-%03d", TYN_elev.GetSizeY()-y, x+1);
				CString ID;
				ID.Format( "CRU 3.1 %03d-%03d", TYN_elev.GetSizeY()-y, x+1);
				station.SetName(name);
				station.SetID(ID);
				station.SetLat( refP.m_lat() + ((y+0.5)*TYN_elev.GetCellSizeY()));
				station.SetLon( refP.m_lon() + ((x+0.5)*TYN_elev.GetCellSizeX()));
				station.SetElev( elev );

				CNormalsData data;
				//find nearest temperature, precipitation, humidity and wind speed
				CWCategory catTmp = "T P H WS";
				for(int c=0; c<catTmp.GetNbCat(); c++)
				{
					CSearchResultVector results;
					short cat = catTmp.GetCat(c);
				
					VERIFY( inputDB.Match(station,m_nbPointSearch,results,cat) );
					
				
					CNormalStationArray stationArray; 
					inputDB.GetStations(results, stationArray);

					if( results[0].m_distance < m_maxDistance )
						stationArray.GetInverseDistanceMean(station, cat, data);
				}

				vector<float> pre(nbMonths);
				vector<float> tmn(nbMonths);
				vector<float> tmx(nbMonths);

				startp[0] = firstMonth;
				startp[1] = y;
				startp[2] = x;
				countp[0] = nbMonths;
				countp[1] = 1;
				countp[2] = 1;

				if( data.HaveCat(TEMPERATURE) )
				{
					varTmn.getVar(startp, countp, &(tmn[0]));
					varTmx.getVar(startp, countp, &(tmx[0]));
				}

				//if( data.HaveCat(PRECIPITATION) )
				varPre.getVar(startp, countp, &(pre[0]));
				
				
				for(size_t m=0; m<12; m++)
				{
					CFL::CStatistic statPre;
					CFL::CStatistic statTmn;
					CFL::CStatistic statTmx;

					for(int yy=0; yy<nbYears; yy++)
					{
						statPre+=pre[yy*12+m];
						statTmn+=tmn[yy*12+m];
						statTmx+=tmx[yy*12+m];
					}

					if( data.HaveCat(TEMPERATURE) )
					{
						data[m][NORMAL_DATA::TMIN_MN] = (float)statTmn[CFL::MEAN];
						data[m][NORMAL_DATA::TMAX_MN] = (float)statTmx[CFL::MEAN];
					}
					
					if( data.HaveCat(PRECIPITATION) || statPre[CFL::COEF_VAR]>0)
					{
						data[m][NORMAL_DATA::PRCP_TT] = (float)statPre[CFL::MEAN];
					
						if( statPre[CFL::COEF_VAR] > 0 )//some station have 0 as SD???
							data[m][NORMAL_DATA::PRCP_SD] = (float)statPre[CFL::COEF_VAR];
					}
				}
			
				if( data.HaveData() )
				{
					station.SetData(data);
					if( station.IsValid() )
						msg = outputDB.Add( station);
				}
			}

			msg += callback.StepIt();
		}//x
	}//y
	
	inputDB.Close();
	outputDB.Close();
	if( msg)
	{
		msg = outputDB.Open(outputFilePath);
	}
	
	
	return msg;

}


