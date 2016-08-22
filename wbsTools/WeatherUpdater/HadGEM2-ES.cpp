#include "StdAfx.h"



#include "units.hpp"
#include "HadGEM2_10km.h"
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
const char* CHadGEM2_10km_MMGCreator::ATTRIBUTE_NAME[] = { "INPUT_PATH", "OUTPUT_FILEPATH"};
const char* CHadGEM2_10km_MMGCreator::CLASS_NAME = "HADGEM2_10KM_MMG_CREATOR";


CHadGEM2_10km_MMGCreator::CHadGEM2_10km_MMGCreator(void)
{
	if( !IsRegister( GetClassID() ) )
	{
		InitClass();
	}

	Reset();
}

void CHadGEM2_10km_MMGCreator::InitClass(const StringVector& option)
{
	GetParamClassInfo().m_className = GetString(IDS_SOURCENAME_HADGEM2_10_EXTRACTOR);

	CToolsBase::InitClass(option);

	ASSERT( GetParameters().size() <= I_NB_ATTRIBUTE);

	StringVector RPCName("RCP 2.6|RCP 4.5|RCP 8.5", "|");
	StringVector properties(IDS_PROPERTIES_HADGEM2_10_EXTRACTOR, ";|");

	
	string filter1 = GetString(IDS_STR_FILTER_NC);
	string filter2 = GetString(IDS_STR_FILTER_MMG);
	GetParameters().push_back(CParamDef(CParamDef::PATH, ATTRIBUTE_NAME[0], properties[0], filter1));
	GetParameters().push_back(CParamDef(CParamDef::FILEPATH, ATTRIBUTE_NAME[1], properties[1], filter2));
}

CHadGEM2_10km_MMGCreator::~CHadGEM2_10km_MMGCreator(void)
{
}


//CHadGEM2_10km_MMGCreator::CHadGEM2_10km_MMGCreator(const CHadGEM2_10km_MMGCreator& in)
//{
//	operator=(in);
//}

void CHadGEM2_10km_MMGCreator::Reset()
{
	m_path.clear();
	//m_inputRCP = "RCP 8.5";
	m_outputFilePath.clear();
	//m_nbPointSearch=3;
		
	
}

CHadGEM2_10km_MMGCreator& CHadGEM2_10km_MMGCreator::operator =(const CHadGEM2_10km_MMGCreator& in)
{
	if( &in != this)
	{
		CToolsBase::operator =(in);
		m_path=in.m_path;
		//m_inputRCP = in.m_inputRCP;
		m_outputFilePath = in.m_outputFilePath;
	}

	return *this;
}

bool CHadGEM2_10km_MMGCreator::operator ==(const CHadGEM2_10km_MMGCreator& in)const
{
	bool bEqual = true;

	if( CToolsBase::operator !=(in))bEqual = false;
	if(	m_path!=in.m_path)bEqual = false;
	//if( m_inputRCP != in.m_inputRCP)bEqual = false;
	if( m_outputFilePath != in.m_outputFilePath)bEqual = false;
	

	return bEqual;
}

bool CHadGEM2_10km_MMGCreator::operator !=(const CHadGEM2_10km_MMGCreator& in)const
{
	return !operator ==(in);
}


string CHadGEM2_10km_MMGCreator::GetValue(size_t type)const
{
	string str;
	
	ASSERT( NB_ATTRIBUTE == 2); 
	switch(type)
	{
	case I_RCM_PATH: str = m_path;break;
//	case I_RCP: str = m_inputRCP;break;
	case I_OUTPUT_FILEPATH: str = m_outputFilePath; break;
	default: str = CToolsBase::GetValue(type); break;
	}

	return str;
}

void CHadGEM2_10km_MMGCreator::SetValue(size_t type, const string& str)
{
	ASSERT( NB_ATTRIBUTE == 2); 
	switch(type)
	{
	case I_RCM_PATH: m_path=str;break;
	//case I_RCP: m_inputRCP = str;break;
	case I_OUTPUT_FILEPATH: m_outputFilePath=str; break;
	default: CToolsBase::SetValue(type, str); break;
	}

}


bool CHadGEM2_10km_MMGCreator::Compare(const CParameterBase& in)const
{
	ASSERT(in.GetClassID() == GetClassID() );
	const CHadGEM2_10km_MMGCreator& info = dynamic_cast<const CHadGEM2_10km_MMGCreator&>(in);
	return operator==(info);
}

CParameterBase& CHadGEM2_10km_MMGCreator::Assign(const CParameterBase& in)
{
	ASSERT(in.GetClassID() == GetClassID() );
	const CHadGEM2_10km_MMGCreator& info = dynamic_cast<const CHadGEM2_10km_MMGCreator&>(in);
	return operator=(info);
}

ERMsg CHadGEM2_10km_MMGCreator::Execute(CFL::CCallback& callback)
{
	ERMsg msg;

	CPLSetConfigOption("GDAL_CACHEMAX", "8000");

	
	
	/*
	CGDALDatasetEx grid1;
	CGDALDatasetEx grid2;

	CBaseOptions options;
	grid1.OpenInputImage("D:\\CanRCM4\\RCM4_10km(Lambert)\\GCM4_ESM2_10km_1951-2100_rcp85_RelH.tif");
	grid1.UpdateOption(options);
	
	callback.SetNbStep(grid1.GetRasterCount());

	options.m_nbBands -= 12;

	grid2.CreateImage("D:\\GCM10km\\MMG\\GCM4_ESM2_10km_1951-2100_RCP85_speH.tif", options);
	for (int b = 12; b < grid1.GetRasterCount(); b++)
	{
		vector<float> data(grid1.GetRasterXSize()*grid1.GetRasterYSize());
		grid1.GetRasterBand(b + 1)->RasterIO(GF_Read, 0, 0, options.m_extents.m_xSize, options.m_extents.m_ySize, &(data[0]), options.m_extents.m_xSize, options.m_extents.m_ySize, GDT_Float32, 0, 0);
		grid2.GetRasterBand(b - 12 + 1)->RasterIO(GF_Write, 0, 0, options.m_extents.m_xSize, options.m_extents.m_ySize, &(data[0]), options.m_extents.m_xSize, options.m_extents.m_ySize, GDT_Float32, 0, 0);
		msg += callback.StepIt();
	}

	grid1.Close();
	grid2.Close();

	return msg;*/

	callback.SetNbTask(NB_RCP);
	GetDEM(m_DEM);

	for (int rcp = 0; rcp < NB_RCP&&msg; rcp++)
		//int rcp = RCP85;
	{
		CMonthlyMeanGrid MMG;
		MMG.m_firstYear = 1951;
		MMG.m_lastYear = 2100;


		MMG.m_supportedVariables[TMIN_MN] = true;
		MMG.m_supportedVariables[TMAX_MN] = true;
		MMG.m_supportedVariables[PRCP_TT] = true;
		MMG.m_supportedVariables[SPEH_MN] = true;
		MMG.m_supportedVariables[WNDS_MN] = true;

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



const char* CHadGEM2_10km_MMGCreator::VARIABLES_NAMES[NB_VARIABLES] = { "tmin", "tmax", "prec", "vapr", "swind" };//, "srad" 
const char* CHadGEM2_10km_MMGCreator::RCP_NAME[NB_RCP] = { "rcp26", "rcp45", "rcp85" };



string CHadGEM2_10km_MMGCreator::GetFileName(int period, int RCP, int var)
{
	string name;
	if (period == HISTORICAL)
		name = string("hadgem2_es_hist_") + VARIABLES_NAMES[var] + "_abs_1951-2005.nc";
	else
		name = string("hadgem2_es_") + RCP_NAME[RCP] + "_" + VARIABLES_NAMES[var] + "_abs_2006-2100.nc";

	return name;
}
string CHadGEM2_10km_MMGCreator::GetProjectionWKT()
{
	string proj4Str = "+proj=lcc +lat_1=49 +lat_2=77 +lat_0=0 +lon_0=-95 +x_0=0 +y_0=0 +ellps=WGS84 +units=m +no_defs";
	CProjectionPtr prj = CProjectionManager::GetPrj(proj4Str.c_str());

	return prj->GetWKT();

}

CGeoExtents CHadGEM2_10km_MMGCreator::GetGCM10Extents()
{
	CGeoExtents extents;
	extents.m_xMin = -2600000;
	extents.m_xMax = 3100000;
	extents.m_yMin = 5700000;
	extents.m_yMax = 10500000;
	extents.m_xSize = 570;
	extents.m_ySize = 480;
	extents.m_xBlockSize = 570;
	extents.m_yBlockSize = 1;
	ASSERT(extents.XRes() == 10000);
	
	return extents;
}

void CHadGEM2_10km_MMGCreator::GetOptions(CBaseOptions& options)
{
	
	CGeoExtents extents = GetGCM10Extents();

	int nbYears = 2100 - 1951 + 1;
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

void CHadGEM2_10km_MMGCreator::ConvertData(CMonthlyVariable& data)const
{
	using namespace units::values;
	
	for (size_t v = 0; v<data.size(); v++)
	{
		for (size_t i = 0; i < data[v].size(); i++)
		{
			if (data[v][i] > -99)
			{
				if (v == SPEH_MN)
				{
					double Pv = data[v][i]*1000;//Pa
					double Hs = Pv2Hs(Pv);// g[H²O]/kg[air]

					data[v][i] = Hs;//
				}

				if (v == WNDS_MN)
				{
					data[v][i] = (float)kph(meters_per_second(data[v][i])).get(); //m/s --> km/h
				}
				
			}
			else
			{
				//remove all var
				data[TMIN_MN][i] = -999;
				data[TMAX_MN][i] = -999;
				data[PRCP_TT][i] = -999;
				data[SPEH_MN][i] = -999;
				data[WNDS_MN][i] = -999;
			}
		}
	}
}

string CHadGEM2_10km_MMGCreator::GetFilePath( string fileTitle)
{
	return string(m_path)+fileTitle+".nc";
}



ERMsg CHadGEM2_10km_MMGCreator::CreateMMG(int rcp, CMonthlyMeanGrid& MMG, CFL::CCallback& callback)
{
	ERMsg msg;

	//Open input
	NcFilePtrArray ncFile1;
	NcFilePtrArray ncFile2;

	try
	{
		for (int v = 0; v<NB_VARIABLES&&msg; v++)
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
	callback.SetNbStep(options.m_nbBands*5);
	

	//********************************************
	CMonthlyVariable output;
	for (size_t v = 0; v < NB_FIELDS; v++)
	{
		if (grid[v].IsOpen())
			output[v].resize(d1[DIM_LAT] * d1[DIM_LON]);
	}


	for (size_t b = 0; b < options.m_nbBands; b++)
	//int b = (2100-1951+1)*12 - 1;
	{
		size_t* d = (b < d1[DIM_TIME] ? d1 : d2);
		size_t base = (b < d1[DIM_TIME] ? 0 : d1[DIM_TIME]);
		vector<size_t> startp(NB_DIMS);
		vector<size_t> countp(NB_DIMS);

		for (size_t j = 0; j < NB_DIMS; j++)
		{
			startp[j] = (j == DIM_TIME ? b - base : 0);
			countp[j] = (j == DIM_TIME ? 1 : d[j]);//j==0 : TIME; only one month at a time
		}
		
		

		//read all data for this month
		for (size_t v = 0, vv = 0; v<NB_FIELDS; v++)
		{
			if (!output[v].empty())
			{
				
				NcVar& var1 = ncFile1[vv]->getVar(VARIABLES_NAMES[vv]);
				NcVar& var2 = ncFile2[vv]->getVar(VARIABLES_NAMES[vv]);
				NcVar& var = (b < d1[DIM_TIME] ? var1 : var2);

				var.getVar(startp, countp, &(output[v][0]));
				vv++;

				msg += callback.StepIt();
			}
		}

		//convert wind speed and comput dew point temperature
		ConvertData(output);

		//save data
		for (int v = 0; v<NB_FIELDS; v++)
		{
			if (!output[v].empty())
			{
				GDALRasterBand* pBand = grid[v]->GetRasterBand(int(b + 1));
				//GDALRasterBand* pBand = grid[v]->GetRasterBand(1);
				pBand->RasterIO(GF_Write, 0, 0, options.m_extents.m_xSize, options.m_extents.m_ySize, &(output[v][0]), options.m_extents.m_xSize, options.m_extents.m_ySize, GDT_Float32, 0, 0);
			}
		}

		
	}

	for (int v = 0; v < NB_FIELDS; v++)
	{
		if (grid[v].IsOpen())
		{
			
			callback.AddMessage("Close " + grid[v].GetFilePath() + " ...");
			//grid[v].ComputeStats(true);
			grid[v].Close();
		}
	}

	

	return msg;
}


void CHadGEM2_10km_MMGCreator::GetDEM(CFL::CMatrix<float>& DEM)
{
	string filePath = GetFilePath("topo");
	NcFile ncFile(filePath, NcFile::read);

	CGeoExtents extents = GetGCM10Extents();

	vector<float> data(extents.m_xSize*extents.m_ySize);
	vector<size_t> startp(4);
	vector<size_t> countp(4);
	countp[0] = 1;
	countp[1] = 1;
	countp[2] = extents.m_ySize;
	countp[3] = extents.m_xSize;


	NcVar& var = ncFile.getVar("topo");
	var.getVar(startp, countp, &(data[0]));


	DEM.resize(extents.m_ySize, extents.m_xSize);

	for (size_t i = 0; i<DEM.size_y(); i++)
		for (size_t j = 0; j<DEM.size_x(); j++)
			DEM[i][j] = data[i*DEM.size_x() + j];
}
