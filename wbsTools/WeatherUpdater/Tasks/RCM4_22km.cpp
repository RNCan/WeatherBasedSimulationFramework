#include "StdAfx.h"
#include <netcdf>
#include <fstream>

#include "units.hpp"
#include "RCM4_22km.h"
#include "FileManagerRes.h"
#include "NormalsDatabase.h"
#include "SYShowMessage.h"
#include "BasicRes.h"
#include "CommonRes.h"
#include "mappingRes.h"
#include "Resource.h"
#include "gridym.h"
#include "Statistic.h"

#include "ogr_srs_api.h"
#include "MonthlyMeanGrid.h"
#include "boost\multi_array.hpp"
#include "psychrometrics_SI.h"

#pragma warning(disable: 4275 4251)
#include "gdal_priv.h"
#include "cpl_conv.h"





using namespace HOURLY_DATA;
//using namespace WEATHER;
using namespace CFL;
using namespace netCDF;
using namespace stdString;
using namespace std;
using namespace NORMALS_DATA;
using namespace GeoBasic;


enum { H_SPEH = H_TDEW };//used specific humidity instead iof dew point temperature

typedef std::auto_ptr<NcFile> NcFilePtr;
typedef vector<NcFilePtr> NcFilePtrVector;

const char* CRCM4_ESM2_22km_MMGCreator::VARIABLES_NAMES[NB_VARIABLES] = { "tasmin", "tasmax", "pr", "huss", "sfcWind" };
const char* CRCM4_ESM2_22km_MMGCreator::RCP_NAME[NB_RCP] = { "RCP45", "RCP85" };

enum TPeriod { HISTORICAL1, FIRST_PERIOD = HISTORICAL1, HISTORICAL2, FUTURE, LAST_PERIOD = FUTURE, NB_PERIOD_TYPE };
static const int NB_PERIOD[NB_PERIOD_TYPE] = { 1, 11, 19 };
static const int PERIOD_BEGIN[NB_PERIOD_TYPE] = { 1950, 1951, 2006 };
static const int PERIOD_LENGTH[NB_PERIOD_TYPE] = { 1, 5, 5 };

static const char* FILE_FORMAT = "%s_%s-22_CCCma-CanESM2_%s_r1i1p1_CCCma-CanRCM4_r2_day_%4d0101-%4d1231";


string GetFileName(string varName, string region, string periodName, int firstYear, int lastYear)
{
	return FormatA(FILE_FORMAT, varName.c_str(), region.c_str(), periodName.c_str(), firstYear, lastYear);
}


//*********************************************************************
const char* CRCM4_ESM2_22km_MMGCreator::ATTRIBUTE_NAME[] = { "INPUT_PATH", "REGION", "OUTPUT_FILEPATH", };
const char* CRCM4_ESM2_22km_MMGCreator::CLASS_NAME = "RCM4_ESM2_22KM_MMG_CREATOR";



//enum TVariable { V_X, V_Y, FIRST_VARIABLE, V_ABIE_BAL = FIRST_VARIABLE, V_PICE_GLA, V_PICE_RUB, NB_VARIABLES };
//static const char* VAR_NAME[NB_VARIABLES] = { "X", "Y", "nfi_ABIE_BAL_250m_wgtd", "nfi_PICE_GLA_250m_wgtd", "nfi_PICE_RUB_250m_wgtd" };
//typedef std::map<int, array<CStatistic, NB_VARIABLES> > CStatisticMap;


CRCM4_ESM2_22km_MMGCreator::CRCM4_ESM2_22km_MMGCreator(void)
{
	if( !IsRegister( GetClassID() ) )
	{
		InitClass();
	}

	Reset();
}

void CRCM4_ESM2_22km_MMGCreator::InitClass(const StringVector& option)
{
	GetParamClassInfo().m_className = GetString( IDS_SOURCENAME_RCM22_EXTRACTOR);

	CToolsBase::InitClass(option);

	ASSERT( GetParameters().size() <= I_NB_ATTRIBUTE);
	
	StringVector regionName("NAM|ARC|EUR", "|");
	StringVector properties( IDS_PROPERTIES_RCM22_EXTRACTOR, "|");
	
	string filter1 = GetString(IDS_STR_FILTER_NC);
	string filter2 = GetString(IDS_STR_FILTER_MMG);
	GetParameters().push_back(CParamDef(CParamDef::PATH, ATTRIBUTE_NAME[0], properties[0], filter1));
	GetParameters().push_back( CParamDef(CParamDef::COMBO, CParamDef::BY_STRING, ATTRIBUTE_NAME[1], properties[1], regionName, "0" ) );
	GetParameters().push_back( CParamDef(CParamDef::FILEPATH, ATTRIBUTE_NAME[2], properties[2], filter2 ) );

}

CRCM4_ESM2_22km_MMGCreator::~CRCM4_ESM2_22km_MMGCreator(void)
{
}


void CRCM4_ESM2_22km_MMGCreator::Reset()
{
	m_path.clear();
	m_regionName = "NAM";
	m_outputFilePath.clear();
}

CRCM4_ESM2_22km_MMGCreator& CRCM4_ESM2_22km_MMGCreator::operator =(const CRCM4_ESM2_22km_MMGCreator& in)
{
	if( &in != this)
	{
		CToolsBase::operator =(in);
		m_path=in.m_path;
		m_outputFilePath = in.m_outputFilePath;
		m_regionName=in.m_regionName;
		
	}

	return *this;
}

bool CRCM4_ESM2_22km_MMGCreator::operator ==(const CRCM4_ESM2_22km_MMGCreator& in)const
{
	bool bEqual = true;

	if( CToolsBase::operator !=(in))bEqual = false;
	if(	m_path!=in.m_path)bEqual = false;
	if( m_outputFilePath != in.m_outputFilePath)bEqual = false;
	if( m_regionName!=in.m_regionName)bEqual = false;
	
	

	return bEqual;
}

bool CRCM4_ESM2_22km_MMGCreator::operator !=(const CRCM4_ESM2_22km_MMGCreator& in)const
{
	return !operator ==(in);
}


string CRCM4_ESM2_22km_MMGCreator::GetValue(size_t type)const
{
	string str;
	
	ASSERT( NB_ATTRIBUTE == 3);
	switch(type)
	{
	case I_RCM_PATH: str = m_path;break;
	case I_REGION: str = m_regionName; break;
	case I_OUTPUT_FILEPATH: str = m_outputFilePath; break;
	default: str = CToolsBase::GetValue(type); break;
	}

	return str;
}

void CRCM4_ESM2_22km_MMGCreator::SetValue(size_t type, const string& str)
{
	ASSERT( NB_ATTRIBUTE == 3); 
	switch(type)
	{
	case I_RCM_PATH: m_path=str;break;
	case I_REGION: m_regionName = str; break;
	case I_OUTPUT_FILEPATH: m_outputFilePath=str; break;
	default: CToolsBase::SetValue(type, str); break;
	}

}


bool CRCM4_ESM2_22km_MMGCreator::Compare(const CParameterBase& in)const
{
	ASSERT(in.GetClassID() == GetClassID() );
	const CRCM4_ESM2_22km_MMGCreator& info = dynamic_cast<const CRCM4_ESM2_22km_MMGCreator&>(in);
	return operator==(info);
}

CParameterBase& CRCM4_ESM2_22km_MMGCreator::Assign(const CParameterBase& in)
{
	ASSERT(in.GetClassID() == GetClassID() );
	const CRCM4_ESM2_22km_MMGCreator& info = dynamic_cast<const CRCM4_ESM2_22km_MMGCreator&>(in);
	return operator=(info);
}



//Get projection
//WARNING: the rorated pole projection is not supported by GeoTIFF. Than, we used temporary this projection and we tranform the resulting image
//with this command : gdalwarp -overwrite -dstnodata -999 -co "COMPRESS=LZW" -s_srs "C:/CanRCM4/test.prj" -t_srs "+proj=lcc +lat_1=30 +lat_2=60 +lat_0=47.5 +lon_0=-97 +x_0=0 +y_0=0 +ellps=sphere +units=m +no_defs" 
//where prj file contain:
//PROJCS["Plate_Carree",
//    GEOGCS["unnamed ellipse",
//        DATUM["D_unknown",
//            SPHEROID["Unknown",1,0]],
//        PRIMEM["Greenwich",0],
//        UNIT["Degree",0.017453292519943295]],
//    PROJECTION["Plate_Carree"],
//    PARAMETER["latitude_of_origin",0],
//    PARAMETER["central_meridian",-97],
//    PARAMETER["false_easting",0],
//    PARAMETER["false_northing",0],
//    EXTENSION["PROJ4","+proj=ob_tran +o_proj=eqc +lon_0=-97 +o_lat_p=42.5 +a=1 +to_meter=0.0174532925199 +wktext"],
//    UNIT["Meter",1]]

ERMsg CRCM4_ESM2_22km_MMGCreator::ExportPoint(string filePath, int rcp, CGeoPoint pt, CFL::CCallback& callback)
{
	ERMsg msg;


	ofStream file;
	msg = file.open(filePath);
	if (msg)
	{
		m_extents = GetExtents();
		GetLandWaterProfile(m_landWaterMask);
		GetDEM(m_DEM);

		callback.SetNbTask(NB_VARIABLES);
		
		

		
		pt.Reproject(::GetReProjection(pt.GetPrjID(), m_extents.GetPrjID()));
		ASSERT(m_extents.IsInside(pt));
		//X = 17.25   Y = 2.10 
		
		callback.AddMessage(string("X = ") + to_string(pt.m_x) );
		callback.AddMessage(string("Y = ") + to_string(pt.m_y));
		CGeoPointIndex xy = m_extents.CoordToXYPos(pt);
		xy.m_y = m_extents.m_ySize - xy.m_y - 1;//reverse lat

		array< vector<float>, NB_VARIABLES>  data;
		for (size_t v = 0; v < data.size()&&msg; v++)
		{
			try
			{
				vector<string> filesList = GetFileList(VARIABLES_NAMES[v], rcp);
				callback.SetCurrentDescription(string("Extract ") + VARIABLES_NAMES[v]);
				callback.SetNbStep(filesList.size());

				//open files
				//vector<string>::const_iterator it = filesList.begin();
				for (vector<string>::const_iterator it = filesList.begin(); it != filesList.end()&&msg; it++)
				{
					string filePath = GetFilePath(*it);
					NcFile ncFile(filePath, NcFile::read);
				
					NcVar& var = ncFile.getVar(VARIABLES_NAMES[v]);
					size_t curPos = data[v].size();
					data[v].resize(data[v].size() + var.getDims().at(0).getSize());
					
					vector<size_t> startp(var.getDimCount());
					startp[1] = xy.m_y;//lat
					startp[2] = xy.m_x;//lon
					vector<size_t> countp(var.getDimCount());
					countp[0] = var.getDims().at(0).getSize();
					countp[1] = 1;
					countp[2] = 1;
					
					
					var.getVar(startp, countp, &(data[v][curPos]));

					msg += callback.StepIt();
				}

				
				for (size_t i = 0; i<data[v].size(); i++)
				{

					if (data[v][i] < 1.0E20)
					{
						using namespace units::values;
						switch (v)
						{
						case V_TMIN:	data[v][i] = (float)Celsius(K(data[v][i])).get(); break; //K --> °C
						case V_TMAX:	data[v][i] = (float)Celsius(K(data[v][i])).get(); break; //K --> °C
						case V_PRCP:	data[v][i] = (float)(data[v][i] * 60 * 60 * 24); break; //kg/(m²s) --> mm/day 
						case V_SP_HUM:	data[v][i] = (float)(data[v][i] * 1000);  break;		//kg[H2O]/kg[air] --> g[H2O]/kg[air]
						case V_WIND:	data[v][i] = (float)kph(meters_per_second(data[v][i])).get(); break; //m/s --> km/h
						default: ASSERT(false);
						}
					}
					else
					{
						data[v][i] = -999;
					}
				}
			}
			catch (exceptions::NcException& e)
			{
				msg.ajoute(e.what());
				return msg;
			}

			
		}

		//CTRef TRefBase(1950, 0, 0);
		//save file
		file << "Year,Month,Day,Tmin,Tmax,Tair,Prcp,Tdew,Hr,Hs,Pv,WndS" << endl;
		for (size_t d = 0; d < data[0].size(); d++)
		{
			//CTRef TRef = TRefBase + d;
			int year = 1950 + int(d / 365);
			CJDayRef JD(0, d % 365);
			file << year << "," << JD.GetMonth() + 1 << "," << JD.GetDay() + 1 << ",";
			double Tmin = data[V_TMIN][d];
			double Tmax = data[V_TMAX][d];
			double Tair = (Tmin + Tmax) / 2;
			double prcp = data[V_PRCP][d];
			double Hs = data[V_SP_HUM][d];
			double Pv = CFL::Hs2Pv(Hs);
			double Hr = CFL::Pv2Hr(Tair, Pv);
			double Td = CFL::Hr2Td(Tair, Hr);
			double ws = data[V_WIND][d];

			file << Tmin << "," << Tmax << "," << Tair << "," << prcp << "," << Td << "," << Hr << "," << Hs << "," << Pv << "," << ws << endl;
		}

		file.close();
	}

	return msg;
}

ERMsg CRCM4_ESM2_22km_MMGCreator::Execute(CFL::CCallback& callback)
{
	ERMsg msg;

//	return ExportPoint("D:\\CanRCM4\\Test\\Quebec daily 1950-2100 RCP85.csv", RCP85, CGeoPoint(-71.38, 46.75, PRJ_WGS_84), callback);


	CPLSetConfigOption("GDAL_CACHEMAX", "4000");

	int firstYear = PERIOD_BEGIN[FIRST_PERIOD];
	int lastYear = PERIOD_BEGIN[LAST_PERIOD] + NB_PERIOD[LAST_PERIOD] * PERIOD_LENGTH[LAST_PERIOD] - 1;
	int nbYears = lastYear - firstYear + 1;
	
	string prj = GetPrjStr();

	//Get extents
	m_extents = GetExtents();

	//Load land/water profile
	GetLandWaterProfile(m_landWaterMask);
	GetDEM(m_DEM);


	CBaseOptions options;
	options.m_format = "GTIFF";
	options.m_outputType = GDT_Float32;
	options.m_nbBands = nbYears * 12;
	options.m_dstNodata = -999;
	options.m_bOverwrite = true;
	options.m_extents = m_extents;
	options.m_prj = prj;
	options.m_createOptions.push_back(string("BIGTIFF=YES"));

	CMonthlyMeanGrid MMG;
	MMG.m_firstYear = firstYear;
	MMG.m_lastYear = lastYear;

	//for (int i = TMIN_MN; i < NB_FIELDS; i++)
		//MMG.m_supportedVariables[i] = true;

	//MMG.m_supportedVariables[SPEH_MN] = false;
	//MMG.m_supportedVariables[PRCP_SD] = false;

	MMG.m_supportedVariables[TMIN_MN] = true;
	MMG.m_supportedVariables[TMAX_MN] = true;
	MMG.m_supportedVariables[DEL_STD] = true;
	MMG.m_supportedVariables[EPS_STD] = true;
	MMG.m_supportedVariables[PRCP_TT] = true;
	MMG.m_supportedVariables[SPEH_MN] = true;
	MMG.m_supportedVariables[RELH_MN] = true;
	MMG.m_supportedVariables[RELH_SD] = true;
	MMG.m_supportedVariables[WNDS_MN] = true;
	MMG.m_supportedVariables[WNDS_SD] = true;


	callback.SetNbTask(nbYears * NB_RCP * NB_PERIOD_TYPE + 1);

	for (int rcp = 0; rcp < NB_RCP&&msg; rcp++)
	{
		string filePathOut = m_outputFilePath;
		SetFileTitle(filePathOut, GetFileTitle(filePathOut) + "_" + RCP_NAME[rcp]);
		msg = MMG.Save(filePathOut);
		if (!msg)
			return msg;

		callback.SetCurrentDescription(string("Open output images for ") + RCP_NAME[rcp] );

		CGDALDatasetEx grid[NB_FIELDS];
		for (int v = 0; v < NB_FIELDS; v++)
		{
			string filePathOut = MMG.GetFilePath(v);
			if (!filePathOut.empty())
				msg += grid[v].CreateImage(filePathOut, options);
		}

		if (!msg)
			return msg;



		for (int t = 0; t < NB_PERIOD_TYPE&&msg; t++)
		{
			for (int p = 0; p < NB_PERIOD[t] && msg; p++)
			{
				int periodFirstYear = PERIOD_BEGIN[t] + p*PERIOD_LENGTH[t];
				callback.AddMessage(ToString(periodFirstYear));

				CMonthlyVariableVector data;
				msg += GetMMGForPeriod(t, rcp, p, data, callback);

				for (size_t m = 0; m < data.size() && msg; m++)
				{
					if ((m % 12) == 0)
					{
						callback.SetCurrentDescription("Save data for year = " + ToString(periodFirstYear + int(m / 12)));
						callback.SetNbStep(12 * data[m].size());
					}

					for (size_t v = 0; v < data[m].size() && msg; v++)
					{
						if (grid[v].IsOpen())
						{
							GDALRasterBand* pBand = grid[v]->GetRasterBand(int((periodFirstYear - firstYear) * 12 + m + 1));//+1 = base 1
							pBand->RasterIO(GF_Write, 0, 0, m_extents.m_xSize, m_extents.m_ySize, &(data[m][v][0]), m_extents.m_xSize, m_extents.m_ySize, GDT_Float32, 0, 0);
							msg += callback.StepIt();
						}
					}
				}
			}
		}

		callback.SetCurrentDescription("Close MMG");
		callback.SetNbStep(NB_FIELDS);

		for (int v = 0; v < NB_FIELDS&&msg; v++)
		{
			grid[v].Close();
			msg += callback.StepIt();
		}
	}

	return msg;

}	
	

//*******************************************************************************************************

vector<string> CRCM4_ESM2_22km_MMGCreator::GetFileList(int periodType, int rcp, int periodNo)const
{

	vector<string> fileList;
	for(int i=0; i<NB_VARIABLES; i++)
	{
		string varName = VARIABLES_NAMES[i];
		int firstYear = PERIOD_BEGIN[periodType] + PERIOD_LENGTH[periodType]*periodNo;
		int lastYear = PERIOD_BEGIN[periodType] + PERIOD_LENGTH[periodType]*(periodNo+1)-1;
			
		fileList.push_back(GetFileName(varName, m_regionName, GetPeriodName(periodType, rcp), firstYear, lastYear));
	}

	return fileList;
}

vector<string> CRCM4_ESM2_22km_MMGCreator::GetFileList(string varName, int rcp)const
{
	vector<string> fileList;

	for(int i=0; i<NB_PERIOD_TYPE; i++)
	{
		for(int j=0; j<NB_PERIOD[i]; j++)
		{
			int firstYear = PERIOD_BEGIN[i] + PERIOD_LENGTH[i]*j;
			int lastYear = PERIOD_BEGIN[i] + PERIOD_LENGTH[i]*(j+1)-1;
			
			fileList.push_back( GetFileName(varName, m_regionName, GetPeriodName(i, rcp), firstYear, lastYear) );
		}
	}


	return fileList;

}


string CRCM4_ESM2_22km_MMGCreator::GetPrjStr()
{
	return "+proj=ob_tran +o_proj=eqc +lon_0=-97 +o_lat_p=42.5 +a=1 +to_meter=0.0174532925199";
}

CGeoExtents CRCM4_ESM2_22km_MMGCreator::GetExtents()
{
	static const char* GDAL_PRJ = "PROJCS[\"Plate_Carree\",GEOGCS[\"unnamed ellipse\",DATUM[\"D_unknown\",SPHEROID[\"Unknown\", 1, 0]],PRIMEM[\"Greenwich\", 0],UNIT[\"Degree\", 0.017453292519943295]],PROJECTION[\"Plate_Carree\"],PARAMETER[\"latitude_of_origin\", 0],PARAMETER[\"central_meridian\", -97],PARAMETER[\"false_easting\", 0],PARAMETER[\"false_northing\", 0],EXTENSION[\"PROJ4\", \"+proj=ob_tran +o_proj=eqc +lon_0=-97 +o_lat_p=42.5 +a=1 +to_meter=0.0174532925199 +wktext\"],UNIT[\"Meter\", 1]]";

	//CProjectionManager::CreateProjection(GDAL_PRJ);
	CProjectionPtr pPrj = GetProjection(GDAL_PRJ);
	ASSERT(pPrj);
	size_t prjID = pPrj->GetPrjID();
	
	
	string fileTitle("orog_" + m_regionName  + "-22");
	string filePath = GetFilePath(fileTitle);
	NcFile ncFile(filePath.c_str(), NcFile::read);
	
	NcVar& rlat = ncFile.getVar("rlat");
	NcVar& rlon = ncFile.getVar("rlon");
	
	size_t xsize = rlon.getDim(0).getSize();
	size_t ysize = rlat.getDim(0).getSize();

	vector<double> x(xsize);
	vector<double> y(ysize);
	rlon.getVar(&(x[0]));
	rlat.getVar(&(y[0]));
	double xCellSize = (x.back()-x.front())/(xsize-1);
	double yCellSize = (y.back()-y.front())/(ysize-1);

	CGeoExtents extents;
	extents.SetPrjID(prjID);
	extents.m_xMin = x.front()-xCellSize/2;//-34.1;
	extents.m_xMax = x.back()+xCellSize/2;//34.1;
	extents.m_yMin = y.front()-yCellSize/2;//-28.82;
	extents.m_yMax = y.back()+yCellSize/2;//28.38;
	extents.m_xSize = (int)xsize;
	extents.m_ySize = (int)ysize;
	ASSERT( fabs(extents.XRes() - 0.22)<0.0001 );
	ASSERT( fabs(extents.YRes() + 0.22)<0.0001 );

	return extents;
}

string CRCM4_ESM2_22km_MMGCreator::GetPeriodName(int t, int rcp)const
{
	string name = "historical";
	if( t==FUTURE)
		name = RCP_NAME[rcp];

	return name;
}


void CRCM4_ESM2_22km_MMGCreator::ConvertData(size_t v, vector<float>& data)const
{
	

	using namespace units::values;

	for(size_t i=0; i<data.size(); i++)
	{
		
		if( data[i] < 1.0E20 && m_landWaterMask[i])
		{
			switch(v)
			{
			case V_TMIN:	data[i] = (float)Celsius(K(data[i])).get(); break; //K --> °C
			case V_TMAX:	data[i] = (float)Celsius(K(data[i])).get(); break; //K --> °C
			case V_PRCP:	data[i] = (float)(data[i]*60*60*24); break; //kg/(m²s) --> mm/day 
			case V_SP_HUM:	data[i] = data[i] * 1000; break; //kg[H2O]/kg[air] --> g[H2O]/kg[air]
			case V_WIND:	data[i] = (float)kph(meters_per_second(data[i])).get(); break; //m/s --> km/h
			default: ASSERT(false);
			}
		}
		else
		{
			data[i] = -999;
		}
	}
}

CStatistic ComputeWindSpeed(int m, CWeatherStation& dailyStation)
{
	ASSERT(dailyStation.size()==1);

	CStatistic windStat;
	
	for(size_t d=0; d<CFL::GetNbDayPerMonth(m); d++)
	{
		double wnds = dailyStation.at(0)[m][d][H_WNDS][MEAN];
		if( wnds > -999 )
			windStat +=	wnds;
	}


	return windStat;
}



ERMsg CRCM4_ESM2_22km_MMGCreator::GetMMGForPeriod(int periodType, int rcp, int periodNo, CMonthlyVariableVector& dataOut, CFL::CCallback& callback)
{
	ASSERT( m_extents.IsInit() );

	ERMsg msg;
	

	int firstYear = PERIOD_BEGIN[periodType] + PERIOD_LENGTH[periodType]*periodNo;
	int lastYear = PERIOD_BEGIN[periodType] + PERIOD_LENGTH[periodType]*(periodNo+1)-1;
	int nbYears = lastYear-firstYear+1;

	vector<string> fileList;
	fileList = GetFileList(periodType, rcp, periodNo);

	NcFilePtrVector ncFiles;

	
	callback.SetCurrentDescription("Open NetCDF files" );

	try
	{
		//open files
		for (vector<string>::const_iterator it = fileList.begin(); it != fileList.end(); it++)
		{
			string filePath = GetFilePath(*it);
			ncFiles.push_back(NcFilePtr(new NcFile(filePath, NcFile::read)));
		}
	}
	catch (exceptions::NcException& e)
	{
		msg.ajoute(e.what());
		return msg;
	}
	
	vector<size_t> startp(3);
	vector<size_t> countp(3);
	countp[0] = 1;
	countp[1] = m_extents.m_ySize;
	countp[2] = m_extents.m_xSize;
	int dd=0;

	
	dataOut.resize(nbYears*12);
	for(size_t m=0; m<dataOut.size(); m++)
		for(int v=0; v<NB_FIELDS; v++)
			dataOut[m][v].insert(dataOut[m][v].begin(), m_extents.m_ySize*m_extents.m_xSize, -999);

	for(int y=0; y<nbYears&&msg; y++)
	{
		int year = firstYear+y;

		vector<auto_ptr<CYear>> dailyData(m_extents.m_ySize*m_extents.m_xSize);
		vector<vector<float>> data(ncFiles.size());
		for(size_t v=0; v<data.size()&&msg; v++)
			data[v].resize(m_extents.m_ySize*m_extents.m_xSize);


		int sdd = sizeof(CWeatherStation);
		int ts = sdd*m_extents.m_ySize*m_extents.m_xSize;
		
		

		callback.SetCurrentDescription("Load data for year = " + ToString(year));
		callback.SetNbStep(ncFiles.size()*365);

		for(size_t m=0; m<12&&msg; m++)
		{
			for (size_t d = 0; d<CFL::GetNbDayPerMonth(m) && msg; d++, dd++)//no leap year
			{
				for(size_t v=0; v<data.size()&&msg; v++)
				{
					startp[0] = dd;
					
					NcVar& var = ncFiles[v]->getVar(VARIABLES_NAMES[v]);
					var.getVar(startp, countp, &(data[v][0]));

					ConvertData(v, data[v]);

					msg += callback.StepIt();
				}

				//extract humidity and fill daily data struck
				for(size_t i=0; i<dailyData.size()&&msg; i++)
				{
					int x = i%m_extents.m_xSize;
					int y = m_extents.m_ySize - int(i/m_extents.m_xSize) - 1;

					double Tmin = data[V_TMIN][i];
					double Tmax = data[V_TMAX][i];
					double Prcp = data[V_PRCP][i];
					double WindS = data[V_WIND][i];
					double Hs = data[V_SP_HUM][i];

					int ii = y*m_extents.m_xSize + x;

					if (Tmin>-999 && Tmax>-999 && Hs>-999 && WindS>-999)
					{
						if( dailyData[ii].get()==NULL)
							dailyData[ii].reset(new CYear);

						double Hr = CFL::Hs2Hr(Tmin, Tmax, Hs);
						CDay& day = (*(dailyData[ii]))[m][d];
						day[H_TAIR] += Tmin;
						day[H_TAIR] += Tmax;
						day[H_PRCP] = Prcp;
						day[H_TDEW] = Hs;//put Hs into Tdew
						day[H_RELH] = Hr;
						day[H_WNDS] = WindS;
					}
				}//if valid
			}//d
		}//m
		

		callback.SetCurrentDescription("Create normals for year = " + ToString(year) );
		callback.SetNbStep(dailyData.size());

		
		
		#pragma omp parallel for
		for(int i=0; i<(int)dailyData.size(); i++)
		{
			#pragma omp flush(msg)
			if( msg && dailyData[i].get() != NULL)
			{
				CWeatherStation dailyStation;
				dailyStation[year] = *(dailyData[i]);
	
				CAdvancedNormalStation normalsStations;
				msg+=normalsStations.FromDaily(dailyStation, 1);
			
				for(int m=0; m<12&&msg; m++)
				{
					for(int v=0; v<WNDS_MN&&msg; v++)
						dataOut[y*12+m][v][i]= normalsStations[m][v];
				
					//now compute windspeed (not log of wind speed)
					CStatistic windStat = ComputeWindSpeed(m, dailyStation);
					ASSERT( windStat[NB_VALUE]>0);

					dataOut[y*12+m][WNDS_MN][i] = (float)windStat[MEAN];
					dataOut[y*12+m][WNDS_SD][i] = (float)windStat[STD_DEV];
				}
			}
			#pragma omp flush(msg)
			if (msg)
				msg+=callback.StepIt();
			#pragma omp flush(msg)		
		}
	}
	

	return msg;
}

string CRCM4_ESM2_22km_MMGCreator::GetFilePath( string fileTitle)
{

	//string filePath;
	//filePath.Format("%s%s", m_path, fileName.c_str() );

	return m_path+m_regionName+"\\"+fileTitle+".nc";
}






void CRCM4_ESM2_22km_MMGCreator::GetLandWaterProfile(CLandWaterBitset& landWaterMask)
{
	string fileTitle( "sftlf_" + m_regionName+"-22");
	string filePath = GetFilePath( fileTitle);
	NcFile ncFile(filePath, NcFile::read);
	
	vector<float> data(m_extents.m_xSize*m_extents.m_ySize);
	vector<size_t> startp(3);
	vector<size_t> countp(3);
	countp[0] = 1;
	countp[1] = m_extents.m_ySize;
	countp[2] = m_extents.m_xSize;
	
					
	NcVar& var = ncFile.getVar("sftlf");
	var.getVar(startp, countp, &(data[0]));


	landWaterMask.resize(m_extents.m_xSize*m_extents.m_ySize);
	
	for(size_t i=0; i<landWaterMask.size(); i++)
		landWaterMask[i] = data[i] > 0;
}



void CRCM4_ESM2_22km_MMGCreator::GetDEM(CFL::CMatrix<float>& DEM)
{
	string fileTitle("orog_" + m_regionName + "-22");
	string filePath = GetFilePath(fileTitle);
	NcFile ncFile(filePath, NcFile::read);

	vector<float> data(m_extents.m_xSize*m_extents.m_ySize);
	vector<size_t> startp(3);
	vector<size_t> countp(3);
	countp[0] = 1;
	countp[1] = m_extents.m_ySize;
	countp[2] = m_extents.m_xSize;


	NcVar& var = ncFile.getVar("orog");
	var.getVar(startp, countp, &(data[0]));


	DEM.resize(m_extents.m_ySize, m_extents.m_xSize);

	for (size_t i = 0; i<DEM.size_y(); i++)
		for (size_t j = 0; j<DEM.size_x(); j++)
			DEM[i][j] = data[i*DEM.size_x() + j];
}
