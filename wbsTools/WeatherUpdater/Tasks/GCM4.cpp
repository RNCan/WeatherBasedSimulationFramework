#include "StdAfx.h"



#include "units.hpp"
#include "GCM4.h"
#include "FileManagerRes.h"
#include "NormalsDatabase.h"
#include "SYShowMessage.h"
#include "BasicRes.h"
#include "CommonRes.h"
#include "Resource.h"
#include "Statistic.h"

#include "ogr_srs_api.h"
#include "MonthlyMeanGrid.h"
#include "boost\multi_array.hpp"
#include "psychrometrics_SI.h"


#pragma warning(disable: 4275 4251)
#include "gdal_priv.h"
#include "cpl_conv.h"

using namespace std;
using namespace stdString;
using namespace HOURLY_DATA;
using namespace WEATHER;
using namespace NORMALS_DATA;
using namespace CFL;
using namespace netCDF;
using namespace GeoBasic;


//directory of normal 1961-1990
//ftp://ftp.nofc.cfs.nrcan.gc.ca/canada-10km/historical/

//*********************************************************************
const char* CGCM4_ESM2_MMGCreator::ATTRIBUTE_NAME[] = { "INPUT_PATH", "OUTPUT_FILEPATH"};
const char* CGCM4_ESM2_MMGCreator::CLASS_NAME = "GCM4_ESM2_MMG_CREATOR";
static const bool SAVE_TOPO = false;

CGCM4_ESM2_MMGCreator::CGCM4_ESM2_MMGCreator(void)
{
	if( !IsRegister( GetClassID() ) )
	{
		InitClass();
	}

	Reset();

	
}

void CGCM4_ESM2_MMGCreator::InitClass(const StringVector& option)
{
	GetParamClassInfo().m_className = GetString( IDS_SOURCENAME_GCM4_MMG);

	CToolsBase::InitClass(option);

	ASSERT( GetParameters().size() <= I_NB_ATTRIBUTE);

	StringVector RPCName("RCP 2.6|RCP 4.5|RCP 8.5", "|");
	StringVector properties(IDS_PROPERTIES_GCM4_MMG, ";|");

	
	string filter1 = GetString(IDS_STR_FILTER_NC);
	string filter2 = GetString(IDS_STR_FILTER_MMG);
	GetParameters().push_back(CParamDef(CParamDef::PATH, ATTRIBUTE_NAME[0], properties[0], filter1));
	GetParameters().push_back(CParamDef(CParamDef::FILEPATH, ATTRIBUTE_NAME[1], properties[1], filter2));
	
}

CGCM4_ESM2_MMGCreator::~CGCM4_ESM2_MMGCreator(void)
{
}

void CGCM4_ESM2_MMGCreator::Reset()
{
	m_path.clear();
	m_outputFilePath.clear();
}

CGCM4_ESM2_MMGCreator& CGCM4_ESM2_MMGCreator::operator =(const CGCM4_ESM2_MMGCreator& in)
{
	if( &in != this)
	{
		CToolsBase::operator =(in);
		m_path=in.m_path;
		m_outputFilePath = in.m_outputFilePath;
	}

	return *this;
}

bool CGCM4_ESM2_MMGCreator::operator ==(const CGCM4_ESM2_MMGCreator& in)const
{
	bool bEqual = true;

	if( CToolsBase::operator !=(in))bEqual = false;
	if(	m_path!=in.m_path)bEqual = false;
	if( m_outputFilePath != in.m_outputFilePath)bEqual = false;
	

	return bEqual;
}

bool CGCM4_ESM2_MMGCreator::operator !=(const CGCM4_ESM2_MMGCreator& in)const
{
	return !operator ==(in);
}


string CGCM4_ESM2_MMGCreator::GetValue(size_t type)const
{
	string str;
	
	ASSERT( NB_ATTRIBUTE == 2); 
	switch(type)
	{
	case I_RCM_PATH: str = m_path;break;
	case I_OUTPUT_FILEPATH: str = m_outputFilePath; break;
	default: str = CToolsBase::GetValue(type); break;
	}

	return str;
}

void CGCM4_ESM2_MMGCreator::SetValue(size_t type, const string& str)
{
	ASSERT( NB_ATTRIBUTE == 2); 
	switch(type)
	{
	case I_RCM_PATH: m_path=str;break;
	case I_OUTPUT_FILEPATH: m_outputFilePath=str; break;
	default: CToolsBase::SetValue(type, str); break;
	}

}


bool CGCM4_ESM2_MMGCreator::Compare(const CParameterBase& in)const
{
	ASSERT(in.GetClassID() == GetClassID() );
	const CGCM4_ESM2_MMGCreator& info = dynamic_cast<const CGCM4_ESM2_MMGCreator&>(in);
	return operator==(info);
}

CParameterBase& CGCM4_ESM2_MMGCreator::Assign(const CParameterBase& in)
{
	ASSERT(in.GetClassID() == GetClassID() );
	const CGCM4_ESM2_MMGCreator& info = dynamic_cast<const CGCM4_ESM2_MMGCreator&>(in);
	return operator=(info);
}

ERMsg CGCM4_ESM2_MMGCreator::Execute(CFL::CCallback& callback)
{
	ERMsg msg;

	CPLSetConfigOption("GDAL_CACHEMAX", "1000");

	callback.SetNbTask(NB_RCP);
	GetLAF(m_LAF);
	GetDEM(m_DEM);
	

	for (int rcp = 0; rcp < NB_RCP&&msg; rcp++)
	{
		CMonthlyMeanGrid MMG;
		MMG.m_firstYear = 1850;
		MMG.m_lastYear = 2100;


		MMG.m_supportedVariables[TMIN_MN] = true;
		MMG.m_supportedVariables[TMAX_MN] = true;
		MMG.m_supportedVariables[TMIN_SD] = true;
		MMG.m_supportedVariables[TMAX_SD] = true;
		MMG.m_supportedVariables[PRCP_TT] = true;
		MMG.m_supportedVariables[SPEH_MN] = true;
		MMG.m_supportedVariables[RELH_MN] = true;
		MMG.m_supportedVariables[RELH_SD] = true;
		MMG.m_supportedVariables[WNDS_MN] = true;
		MMG.m_supportedVariables[WNDS_SD] = true;

		string filePathOut = m_outputFilePath;
		SetFileTitle(filePathOut, GetFileTitle(filePathOut) + "_" + RCP_NAME[rcp]);
		msg = MMG.Save(filePathOut);
		if (!msg)
			return msg;

		msg = CreateMMG(rcp, MMG, callback);
	}


	
	return msg;

}	
	

//*******************************************************************************************************



const char* CGCM4_ESM2_MMGCreator::VARIABLES_NAMES[NB_VARIABLES_IN] = { "tasmin", "tasmax", "pr", "huss", "sfcWind" };//, "srad" 
const char* CGCM4_ESM2_MMGCreator::RCP_NAME[NB_RCP] = { "rcp26", "rcp45", "rcp85" };

string CGCM4_ESM2_MMGCreator::GetFileName(int period, int RCP, int var)
{
	string name;
	if (period == HISTORICAL)
		name = string(VARIABLES_NAMES[var]) + "_day_CanESM2_historical_r5i1p1_18500101-20051231.nc";
	else
		name = string(VARIABLES_NAMES[var]) +  "_day_CanESM2_" + RCP_NAME[RCP] + "_r5i1p1_20060101-21001231.nc";

	return name;
}
string CGCM4_ESM2_MMGCreator::GetProjectionWKT()
{
	string proj4Str = "+proj=longlat";
	CProjectionPtr prj = CProjectionManager::GetPrj(proj4Str.c_str());

	return prj->GetWKT();

}

CGeoExtents CGCM4_ESM2_MMGCreator::GetGCM4Extents()
{
	CGeoExtents extents;
	extents.m_xMin = -180;
	extents.m_xMax = 180;
	extents.m_yMin = -89.28; //it's an aproximation because delta-latitude si not constant
	extents.m_yMax = 89.28;
	extents.m_xSize = 128;
	extents.m_ySize = 64;
	extents.m_xBlockSize = 128;
	extents.m_yBlockSize = 1;


	return extents;
}

void CGCM4_ESM2_MMGCreator::GetOptions(CBaseOptions& options)
{
	
	CGeoExtents extents = GetGCM4Extents();

	int nbYears = 2100 - 1850 + 1;
	options.m_prj = GetProjectionWKT();
	options.m_extents = extents;
	options.m_nbBands = 12 * nbYears;
	options.m_dstNodata = -999;
	options.m_outputType = GDT_Float32;
	options.m_format = "GTIFF";
	options.m_bOverwrite = true;
	options.m_bComputeStats = true;
	//options.m_createOptions.push_back(string("COMPRESS=LZW"));
	options.m_createOptions.push_back(string("BIGTIFF=YES"));
}

void CGCM4_ESM2_MMGCreator::ConvertData(CDataStructIn& data)const
{
	using namespace units::values;
	
	for (size_t v = 0; v<data.size(); v++)
	{
		for (size_t i = 0; i < data[v].size(); i++)
		{
			
			if (data[v][i] > -99 /*&& LAF>50.0f*/)
			{
				if (v == I_TMIN || v == I_TMAX)
				{
					data[v][i] = (float)Celsius(K(data[v][i])).get();
				}
				else if (v == I_PRCP)
				{ 
					data[v][i] *= 24 * 60 * 60;
				}
				else if (v == I_SPEH)
				{
					double Hs = data[v][i]*1000;// g[H2o]/kg[air]
					data[v][i] = Hs;
				}
				//else if (v == V_RELH)
				//{
				//	double Tmin = data[I_TMIN][i];
				//	double Tmax = data[I_TMAX][i];

				//	double Hs = data[v][i] * 1000;// g[H2o]/kg[air]
				//	double Hr = Hs2Hr(Tmin, Tmax, Hs);
				//	data[v][i] = Hr;
				//}
				else if (v == I_WNDS)
				{
					data[v][i] = (float)kph(meters_per_second(data[v][i])).get(); //m/s --> km/h
				}
			}
			else
			{
				//remove all var
				data[0][i] = -999;
				data[1][i] = -999;
				data[2][i] = -999;
				data[3][i] = -999;
				data[4][i] = -999;
			}
		}
	}
}

string CGCM4_ESM2_MMGCreator::GetFilePath( string fileTitle)
{
	return string(m_path)+fileTitle+".nc";
}



ERMsg CGCM4_ESM2_MMGCreator::CreateMMG(int rcp, CMonthlyMeanGrid& MMG, CFL::CCallback& callback)
{
	ERMsg msg;

	//Open input
	NcFilePtrArray ncFile1;
	NcFilePtrArray ncFile2;

	try
	{
		for (int v = 0; v<NB_VARIABLES_IN&&msg; v++)
		{
			string filePath1 = m_path + GetFileName(HISTORICAL, rcp, v);
			string filePath2 = m_path + GetFileName(FUTURE, rcp, v);

			ncFile1[v] = NcFilePtr(new NcFile(filePath1, NcFile::read));
			ncFile2[v] = NcFilePtr(new NcFile(filePath2, NcFile::read));
			ASSERT(!ncFile1[v]->isNull() && !ncFile2[v]->isNull());
		};
	}
	catch (...)
	{
		msg.ajoute("Unable to open input NetCDF file");
		return msg;
	}




	NcVar& var1 = ncFile1[0]->getVar(VARIABLES_NAMES[0]);
	NcVar& var2 = ncFile2[0]->getVar(VARIABLES_NAMES[0]);

	
	size_t d1[NB_DIMS] = { 0 };
	size_t d2[NB_DIMS] = { 0 };
	for (int i = 0; i<NB_DIMS; i++)
	{
		d1[i] = var1.getDim(i).getSize();
		d2[i] = var2.getDim(i).getSize();
	}

	ASSERT(d1[DIM_LON] == d2[DIM_LON]);
	ASSERT(d1[DIM_LAT] == d2[DIM_LAT]);


	//open output
	CBaseOptions options;
	GetOptions(options);

	ASSERT(d1[DIM_LON] == options.m_extents.m_xSize);
	ASSERT(d1[DIM_LAT] == options.m_extents.m_ySize);

	CGDALDatasetEx grid[NB_FIELDS];
	for(int v=0; v<NB_FIELDS; v++)
	{
		string filePathOut = MMG.GetFilePath(v); 
		if( !filePathOut.empty() )
			msg += grid[v].CreateImage(filePathOut, options);
	}

	if( !msg)
		return msg;

	
	callback.SetCurrentDescription(RCP_NAME[rcp]);
	callback.SetNbStep((d1[DIM_TIME] + d2[DIM_TIME] )* NB_VARIABLES_IN);

	//********************************************
	//CMonthlyVariable output;
	//for (int v = 0; v < NB_FIELDS; v++)
	//{
	//	if (grid[v].IsOpen())
	//		output[v].resize(d1[DIM_LAT] * d1[DIM_LON]);
	//}
	

	ASSERT(options.m_nbBands * 365 == d1[DIM_TIME] + d2[DIM_TIME]);
	for (size_t b = 0, d = 0; b < options.m_nbBands; b++)
	//int b = 3001, d = 0;
	{
		int m = b % 12;
		
		array< vector<CStatistic>, NB_VARIABLES_OUT> output;
		for (size_t v = 0; v < output.size(); v++)
			output[v].resize(d1[DIM_LAT] * d1[DIM_LON]);
		
		for (size_t dd = 0; dd < CFL::GetNbDayPerMonth(m); dd++, d++)
		{
			CDataStructIn data;
			for (size_t v = 0; v < data.size(); v++)
				data[v].resize(d1[DIM_LAT] * d1[DIM_LON]);

			size_t base = (d < d1[DIM_TIME] ? 0 : d1[DIM_TIME]);
			vector<size_t> startp(NB_DIMS);
			vector<size_t> countp(NB_DIMS);

			for (size_t j = 0; j < NB_DIMS; j++)
			{
				startp[j] = (j == DIM_TIME ? d - base : 0);
				countp[j] = (j == DIM_TIME ? 1 : d1[j]);//j==0 : TIME; only one day at a time
			}

			//read all data for this month
			for (size_t v = 0; v < data.size(); v++)
			{
				NcVar& var1 = ncFile1[v]->getVar(VARIABLES_NAMES[v]);
				NcVar& var2 = ncFile2[v]->getVar(VARIABLES_NAMES[v]);
				NcVar& var = (d < d1[DIM_TIME] ? var1 : var2);

				var.getVar(startp, countp, &(data[v][0]));

				msg += callback.StepIt();
			}

			//convert wind speed and compute relative humidity
			ConvertData(data);

			
			//fill daily struct; put the data from North to South 
			for (size_t i = 0; i < data[0].size(); i++)
			{

				size_t x = (i + m_LAF.size_x() / 2) % m_LAF.size_x();
				size_t y = m_LAF.size_y() - size_t(i / m_LAF.size_x()) - 1;
				size_t ii = y*m_LAF.size_x() + x;

				x = ii%m_LAF.size_x();
				y = size_t(ii / m_LAF.size_x() );
				float LAF = m_LAF[y][x];
				if (data[I_TMIN][i]>-999 && data[I_TMAX][i]>-999 &&
					data[I_PRCP][i]>-999 && data[I_SPEH][i]>-999 &&
					data[I_WNDS][i]>-999 && m_LAF[y][x]>50)
				{
					output[O_TMIN_MN][ii] += data[I_TMIN][i];
					output[O_TMIN_SD][ii] += data[I_TMIN][i];
					output[O_TMAX_MN][ii] += data[I_TMAX][i];
					output[O_TMAX_SD][ii] += data[I_TMAX][i];
					output[O_PRCP_TT][ii] += data[I_PRCP][i];
					output[O_SPEH_MN][ii] += data[I_SPEH][i];//out specific humidity in Tdew
					output[O_RELH_MN][ii] += Hs2Hr(data[I_TMIN][i], data[I_TMAX][i], data[I_SPEH][i]);
					output[O_RELH_SD][ii] += Hs2Hr(data[I_TMIN][i], data[I_TMAX][i], data[I_SPEH][i]);
					output[O_WNDS_MN][ii] += data[I_WNDS][i];
					output[O_WNDS_SD][ii] += data[I_WNDS][i];
				}
			}
		}//for all day of the month

		//save data of the month
		CStatistic::SetVMiss(-999);
		for (int v = 0, vv=0; v<NB_FIELDS; v++)
		{
			if (grid[v].IsOpen())
			{
				vector<float> data( output[vv].size() );
				for (size_t i = 0; i < output[vv].size(); i++)
				{
					switch (vv)
					{
					case O_TMIN_MN:
					case O_TMAX_MN:
					case O_SPEH_MN:
					case O_RELH_MN:
					case O_WNDS_MN: data[i] = float(output[vv][i][MEAN]); break;
					case O_TMIN_SD:
					case O_TMAX_SD:
					case O_RELH_SD:
					case O_WNDS_SD: data[i] = float(output[vv][i][STD_DEV]); break;
					case O_PRCP_TT: data[i] = float(output[vv][i][SUM]); break;
					};
				}
					

				GDALRasterBand* pBand = grid[v]->GetRasterBand(int(b + 1));
				//GDALRasterBand* pBand = grid[v]->GetRasterBand(int(1));
				pBand->RasterIO(GF_Write, 0, 0, options.m_extents.m_xSize, options.m_extents.m_ySize, &(data[0]), options.m_extents.m_xSize, options.m_extents.m_ySize, GDT_Float32, 0, 0);
				vv++;
			}
		}
	}

	for (int v = 0; v < NB_FIELDS; v++)
	{
		if (grid[v].IsOpen())
		{
			callback.AddMessage("Close " + grid[v].GetFilePath() + " ...");
			grid[v].ComputeStats(true);
			grid[v].Close();
		}
	}

	

	return msg;
}


void CGCM4_ESM2_MMGCreator::GetDEM(CFL::CMatrix<float>& DEM)
{
	string filePath = GetFilePath("orograw_CanESM2");
	NcFile ncFile(filePath, NcFile::read);

	CGeoExtents extents = GetGCM4Extents();

	vector<float> data(extents.m_xSize*extents.m_ySize);
	vector<size_t> startp(2);
	vector<size_t> countp(2);
	countp[0] = extents.m_ySize;
	countp[1] = extents.m_xSize;


	NcVar& var = ncFile.getVar("orograw");
	var.getVar(startp, countp, &(data[0]));


	DEM.resize(extents.m_ySize, extents.m_xSize);

	for (size_t i = 0; i<DEM.size_y(); i++)
	{
		for (size_t j = 0; j<DEM.size_x(); j++)
		{
			if (m_LAF[m_LAF.size_y() - i - 1][(j + m_LAF.size_x() / 2) % m_LAF.size_x()]>50)
				DEM[DEM.size_y() - i - 1][(j + DEM.size_x() / 2) % DEM.size_x()] = data[i*DEM.size_x() + j];
			else
				DEM[DEM.size_y() - i - 1][(j + DEM.size_x() / 2) % DEM.size_x()] = -999;
		}
	}



	if (SAVE_TOPO)
	{
		CBaseOptions options;
		GetOptions(options);
		options.m_nbBands = 1;
		
		string filePathOut = filePath;
		SetFileExtension( filePathOut, ".tif");
		
		CGDALDatasetEx grid;
		grid.CreateImage(filePathOut, options);

		GDALRasterBand* pBand = grid->GetRasterBand(1);
		pBand->RasterIO(GF_Write, 0, 0, options.m_extents.m_xSize, options.m_extents.m_ySize, &(DEM[0][0]), options.m_extents.m_xSize, options.m_extents.m_ySize, GDT_Float32, 0, 0);
		
		grid.Close();
	}
}

void CGCM4_ESM2_MMGCreator::GetLAF(CFL::CMatrix<float>& LAF)
{
	string filePath = GetFilePath("sftlf_CanESM2");
	NcFile ncFile(filePath, NcFile::read);

	CGeoExtents extents = GetGCM4Extents();

	vector<float> data(extents.m_xSize*extents.m_ySize);
	vector<size_t> startp(2);
	vector<size_t> countp(2);
	countp[0] = extents.m_ySize;
	countp[1] = extents.m_xSize;


	NcVar& var = ncFile.getVar("sftlf");
	var.getVar(startp, countp, &(data[0]));


	LAF.resize(extents.m_ySize, extents.m_xSize);

	for (size_t i = 0; i<LAF.size_y(); i++)
		for (size_t j = 0; j<LAF.size_x(); j++)
			LAF[LAF.size_y() - i - 1][(j + LAF.size_x() / 2) % LAF.size_x()] = data[i*LAF.size_x() + j];


	if (SAVE_TOPO)
	{
		CBaseOptions options;
		GetOptions(options);
		options.m_nbBands = 1;

		string filePathOut = filePath;
		SetFileExtension(filePathOut, ".tif");

		CGDALDatasetEx grid;
		grid.CreateImage(filePathOut, options);

		GDALRasterBand* pBand = grid->GetRasterBand(1);
		pBand->RasterIO(GF_Write, 0, 0, options.m_extents.m_xSize, options.m_extents.m_ySize, &(LAF[0][0]), options.m_extents.m_xSize, options.m_extents.m_ySize, GDT_Float32, 0, 0);

		grid.Close();
	}
}

