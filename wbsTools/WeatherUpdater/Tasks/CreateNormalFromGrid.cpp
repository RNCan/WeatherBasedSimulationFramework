#include "StdAfx.h"
#include "CreateNormalFromGrid.h"

#include "FileManagerRes.h"
#include "commonRes.h"
#include "NormalFile.h"
#include "SYShowMessage.h"
#include "Resource.h"
#include "UtilMath.h"
#include "oldUtilTime.h"
#include "Statistic.h"
#include "CSVFile.h"
//#include "UtilGDAL.h"
#include "ANN\ANN.h"
#include "..\GeomaticsTools\Source\GDALBasic.h"
#include "ogr_spatialref.h"

using namespace CFL;
using namespace WEATHER;
using namespace NORMAL_DATA;
using namespace UtilWin;

//*********************************************************************
const char* CCreateNormalFromGrid::ATTRIBUTE_NAME[] = { "NORMAL_FILE_PATH", "OUTPUT_FILEPATH", "FILTER_FILEPATH"};
const char* CCreateNormalFromGrid::CLASS_NAME = "NORMAL_FROM_GRID";

CCreateNormalFromGrid::CCreateNormalFromGrid(void)
{
	if( !IsRegister( GetClassID() ) )
	{
		InitClass();
	}

	Reset();

}

void CCreateNormalFromGrid::InitClass(const CStringArray& option)
{
	GetParamClassInfo().m_className.LoadString( IDS_SOURCENAME_NORMAL_FROM_GRID );

	CToolsBase::InitClass(option);

	ASSERT( GetParameters().GetSize() < I_NB_ATTRIBUTE);

	CStringArrayEx labels(IDS_PROPERTIES_NORMAL_FROM_GRID );
	ASSERT( labels.GetSize() == NB_ATTRIBUTE);

	
	CString filter1 = GetString( IDS_CMN_FILTER_NORMALS );
	CString filter2 = GetString( IDS_CMN_FILTER_CSV);
	CString filter3 = GetString( IDS_CMN_FILTER_LOC);
	GetParameters().Add( CParamDef(CParamDef::FILEPATH, ATTRIBUTE_NAME[0], labels[0], filter1 ) );
	GetParameters().Add( CParamDef(CParamDef::FILEPATH, ATTRIBUTE_NAME[1], labels[1], filter2 ) );
	GetParameters().Add( CParamDef(CParamDef::FILEPATH, ATTRIBUTE_NAME[2], labels[2], filter3 ) );
	//GetParameters().Add( CParamDef(CParamDef::BOOL, ATTRIBUTE_NAME[2], labels[2], "1") );
	//GetParameters().Add( CParamDef(CParamDef::EDIT, ATTRIBUTE_NAME[3], labels[3]) );
	
}

CCreateNormalFromGrid::~CCreateNormalFromGrid(void)
{
}


CCreateNormalFromGrid::CCreateNormalFromGrid(const CCreateNormalFromGrid& in)
{
	operator=(in);
}

void CCreateNormalFromGrid::Reset()
{
	
	m_inputFilePath.Empty();
	m_outputFilePath.Empty();
	m_filterFilePath.Empty();
//	m_maxDistance = 50000;
}

CCreateNormalFromGrid& CCreateNormalFromGrid::operator =(const CCreateNormalFromGrid& in)
{
	if( &in != this)
	{
		CToolsBase::operator =(in);
	
		m_inputFilePath = in.m_inputFilePath;
		m_outputFilePath = in.m_outputFilePath;
		m_filterFilePath = in.m_filterFilePath;
	}

	return *this;
}

bool CCreateNormalFromGrid::operator ==(const CCreateNormalFromGrid& in)const
{
	bool bEqual = true;

	if( CToolsBase::operator !=(in))bEqual = false;
	if( m_inputFilePath != in.m_inputFilePath)bEqual = false;
	if( m_outputFilePath != in.m_outputFilePath)bEqual = false;
	if( m_filterFilePath != in.m_filterFilePath) bEqual = false;

	return bEqual;
}

bool CCreateNormalFromGrid::operator !=(const CCreateNormalFromGrid& in)const
{
	return !operator ==(in);
}


CString CCreateNormalFromGrid::GetValue(short type)const
{
	CString str;
	

	ASSERT( NB_ATTRIBUTE == 3);
	switch(type)
	{
	case I_INPUT_FILEPATH: str = m_inputFilePath;break;
	case I_OUTPUT_FILEPATH: str = m_outputFilePath; break;
	case I_FILTER_FILEPATH: str = m_filterFilePath; break;
		
	//case I_MAX_DISTANCE: str = ToString(m_maxDistance); break;
	default: str = CToolsBase::GetValue(type); break;
	}

	return str;
}

void CCreateNormalFromGrid::SetValue(short type, const CString& str)
{
	ASSERT( NB_ATTRIBUTE == 3); 
	switch(type)
	{
	//case I_INPUT_PATH: m_inputPath = str;break;
	case I_INPUT_FILEPATH: m_inputFilePath = str;break;
	case I_OUTPUT_FILEPATH: m_outputFilePath=str; break;
	case I_FILTER_FILEPATH: m_filterFilePath=str; break;
	//case I_MAX_DISTANCE: m_maxDistance=ToInt(str); break;
	default: CToolsBase::SetValue(type, str); break;
	}

}

bool CCreateNormalFromGrid::Compare(const CParameterBase& in)const
{
	ASSERT(in.GetClassID() == GetClassID() );
	const CCreateNormalFromGrid& info = dynamic_cast<const CCreateNormalFromGrid&>(in);
	return operator==(info);
}

CParameterBase& CCreateNormalFromGrid::Assign(const CParameterBase& in)
{
	ASSERT(in.GetClassID() == GetClassID() );
	const CCreateNormalFromGrid& info = dynamic_cast<const CCreateNormalFromGrid&>(in);
	return operator=(info);
}




//create normal database
ERMsg CCreateNormalFromGrid::Execute(CFL::CCallback& callback)
{
	ERMsg msg;

	GDALAllRegister();

	callback.SetNbStep(26);
	

	m_inputFilePath.Trim(" ");
	m_outputFilePath.Trim(" ");
	m_filterFilePath.Trim(" ");

	CString inputFilePath(GetAbsoluteFilePath(m_inputFilePath));
	CNormalsDatabase inputDB;
	msg += inputDB.Open( inputFilePath);

	
	//if( outputFilePath.CompareNoCase( inputFilePath )==0 )
	//{
	//	msg.asgType( ERMsg::ERREUR);
	//	msg.ajoute("Le nom en entré est le même qu'en sortie. Entré un nouveau nom");
	//	return msg;
	//}

	const char* CC_TYPE[3] = {"a1b", "a2", "b1"};
	const char* PERIOD[3] = {"2020", "2050", "2080"};
	int periodBegin[3] = { 2010, 2040, 2070 };
	int periodEnd[3] = { 2039, 2069, 2099 };

	for(int cc=0; cc<3; cc++)
	{
		for(int p=0; p<3; p++)
		{
	
			CString outputFilePath = GetPath(GetAbsoluteFilePath(m_outputFilePath)) + CC_TYPE[cc] + PERIOD[p]+".normals";
			
			
			msg += CNormalsDatabase::DeleteDatabase(outputFilePath, callback);

			if( !msg)
				return msg;


			CNormalsDatabase outputDB;
			outputDB.SetBeginYear(periodBegin[p]);
			outputDB.SetEndYear(periodEnd[p]);
			msg += outputDB.Open( outputFilePath, CNormalsDatabase::modeEdit);
			
			if( msg)
				msg += CreateDatabase(cc, p, inputDB, outputDB, callback);

			outputDB.Close();
			msg = outputDB.Open(outputFilePath);

			if(msg)
				outputDB.Close();

		}
	}

	inputDB.Close();


	return msg;
}

//*****************************

ERMsg CCreateNormalFromGrid::CreateDatabase(int cc, int p, CNormalsDatabase& inputDB, CNormalsDatabase& outputDB, CFL::CCallback& callback)
{
	ERMsg msg;

	CGDALDatasetEx gridE;
	msg += gridE.Open("D:\\Travail\\KatherineManess\\1km Climate and 100m DEM\\cc\\Subset\\bcab_dem_1km.tif",GA_ReadOnly);


	const char* CC_TYPE[3] = {"a1b", "a2", "b1"};
	const char* W_TYPE[3] = {"tmin", "tmax", "ppt" };
	const char* PERIOD[3] = {"2020", "2050", "2080"};

	//for(int cc=0; cc<3; cc++)
	{
		//for(int p=0; p<3; p++)
		{
			CGDALDatasetEx grid[12][3];

			for(int m=0; m<12; m++)
			{
				for(int w=0; w<3; w++)
				{
					CString gridFilePath;
					gridFilePath.Format( "D:\\Travail\\KatherineManess\\1km Climate and 100m DEM\\cc\\Subset\\%s%s%s%02d.tif", CC_TYPE[cc], W_TYPE[w], PERIOD[p], m+1);
					msg += grid[m][w].Open((LPCTSTR)gridFilePath, GA_ReadOnly);
				}
			}

			callback.SetCurrentDescription("Generate database" );
			callback.SetNbStep(0, gridE->GetRasterXSize()*gridE->GetRasterYSize(), 1);
			


			CGeoExtents geoExtents = gridE.GetExtents();

			int nbPointSearch = 8;
	
			for(int y=0; y<gridE->GetRasterYSize()&&msg; y++)
			{
				for(int x=0; x<gridE->GetRasterXSize()&&msg; x++)
				{
					int elev=-9999;
					gridE->GetRasterBand(1)->RasterIO(GF_Read,x,y,1,1,&elev,1,1,GDT_Int32,0,0);

					if( elev > -9999)
					{
				
						CGeoPoint2 xyTest = geoExtents.XYPosToCoord(CPoint2(0,0));

						CGeoPoint2 xy = geoExtents.XYPosToCoord(CPoint2(x,y));
						xy.ReprojectToGeographic( );

						CNormalsStation station;
				

						CString name;
						name.Format("%03d_%03d", y+1, x+1);
						station.SetName(name);
				
						station.SetID( ToString(outputDB.GetSize()+1) );
						station.SetLat(xy.m_y);
						station.SetLon(xy.m_x);
						station.SetElev(elev);
			
						CNormalsData data;

						//find nearest temperature, precipitation, humidity and wind speed
						CWCategory catTmp = "T P H WS";
						for(int c=0; c<catTmp.GetNbCat(); c++)
						{
							CSearchResultVector results;
							short cat = catTmp.GetCat(c);
				
							VERIFY( inputDB.Match(station,nbPointSearch,results,cat) );
					
				
							CNormalStationArray stationArray; 
							inputDB.GetStations(results, stationArray);

							stationArray.GetInverseDistanceMean(station, cat, data);
						}

						//replace Tmin, Tmax and prcp by the grid value
						ASSERT( grid[0][0]->GetRasterCount() == 1);
						for(int m=0; m<12; m++)
						{
							float v=0;
							grid[m][0]->GetRasterBand(1)->RasterIO(GF_Read,x,y,1,1,&v,1,1,GDT_Float32,0,0);
							data[m][TMIN_MN] = v;
							grid[m][1]->GetRasterBand(1)->RasterIO(GF_Read,x,y,1,1,&v,1,1,GDT_Float32,0,0);
							data[m][TMAX_MN] = v;
							grid[m][2]->GetRasterBand(1)->RasterIO(GF_Read,x,y,1,1,&v,1,1,GDT_Float32,0,0);
							data[m][PRCP_TT] = v;
						}				
				
						station.SetData(data);
						outputDB.Add(station);
					}//if 

					msg += callback.StepIt();
				}//x
			}//y
	
	
			for(int m=0; m<12; m++)
			{
				for(int w=0; w<3; w++)
					grid[m][w].Close();
			}
		}
	}
	
	gridE.Close();


	return msg;
}

