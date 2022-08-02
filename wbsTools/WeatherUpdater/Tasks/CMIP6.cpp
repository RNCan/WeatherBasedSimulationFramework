#include "StdAfx.h"



#include "Basic/units.hpp"
#include "Basic/Statistic.h"
#include "Basic/NormalsDatabase.h"

#include "CMIP6.h"
//#include "UI/Common/SYShowMessage.h"
#include "ogr_srs_api.h"
#include "Simulation/MonthlyMeanGrid.h"
#include "boost\multi_array.hpp"
#include "Basic/psychrometrics_SI.h"


#pragma warning(disable: 4275 4251)
#include "gdal_priv.h"
#include "cpl_conv.h"

using namespace std;
using namespace netCDF;

namespace WBSF
{

	using namespace HOURLY_DATA;
	using namespace WEATHER;
	using namespace NORMALS_DATA;



	//*********************************************************************

	CCMIP6_MMGCreator::CCMIP6_MMGCreator(void)
	{
	}

	CCMIP6_MMGCreator::~CCMIP6_MMGCreator(void)
	{
	}

	void CCMIP6_MMGCreator::Reset()
	{
		m_path.clear();
		m_output_filepath.clear();
	}

	ERMsg CCMIP6_MMGCreator::Execute(CCallback& callback)
	{
		ERMsg msg;

		CPLSetConfigOption("GDAL_CACHEMAX", "1000");



		msg = GetLAF("G:\\Travaux\\CMIP6\\MIROC-ES2L\\sftlf_fx_MIROC-ES2L_historical_r1i1p1f2_gn_v20190823.nc", m_LAF);
		if (msg)
			msg = GetDEM("G:\\Travaux\\CMIP6\\MIROC-ES2L\\orog_fx_MIROC-ES2L_historical_r1i1p1f2_gn_v20190823.nc", m_DEM);

		if (msg)
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

			string filePathOut = m_output_filepath;
			SetFileTitle(filePathOut, GetFileTitle(filePathOut) + "_" + m_ssp_name);
			msg = MMG.Save(filePathOut);
			if (!msg)
				return msg;


			
			//msg = CreateMMG(m_model_name, m_ssp_name, MMG, callback);

		}


		return msg;

	}


	//*******************************************************************************************************



	const char* CCMIP6_MMGCreator::VARIABLES_NAMES[NB_VARIABLES_IN] = { "tasmin", "tasmax", "pr", "huss", "sfcWind" };//, "srad" 

	string CCMIP6_MMGCreator::GetFileName(int p, int RCP, int var)
	{
		string name;


		//sfcWind_day_MIROC-ES2L_historical_r1i1p1f2_gn_18560101-18561231_v20191129
		string SSP_name = (p == HISTORICAL) ? "historical" : m_ssp_name.c_str();
		string period = "18560101-18561231";
		string version = "v20191129";
		string replicate = "r1i1p1f2";

		name = string(VARIABLES_NAMES[var]) + "_day_" + m_model_name + "_" + SSP_name + "_" + replicate + "_" + "gn" + "_" + period + "_" + version + ".nc";


		return name;
	}

	//string CCMIP6_MMGCreator::GetProjectionWKT()
	//{
	//	string proj4Str = "+proj=longlat";
	//	CProjectionPtr prj = CProjectionManager::GetPrj(proj4Str.c_str());

	//	return prj->GetWKT();

	//}

	//CGeoExtents CCMIP6_MMGCreator::GetGCM4Extents()
	//{
	//	CGeoExtents extents;
	//	extents.m_xMin = -180;
	//	extents.m_xMax = 180;
	//	extents.m_yMin = -89.28; //it's an approximation because delta-latitude is not constant
	//	extents.m_yMax = 89.28;
	//	extents.m_xSize = 128;
	//	extents.m_ySize = 64;
	//	extents.m_xBlockSize = 128;
	//	extents.m_yBlockSize = 1;


	//	return extents;
	//}

	void CCMIP6_MMGCreator::GetOptions(CBaseOptions& options)
	{

		CGeoExtents extents;// = GetGCM4Extents();

		int nbYears = 2100 - 1850 + 1;
		//options.m_prj = GetProjectionWKT();
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

	void CCMIP6_MMGCreator::ConvertData(CDataStructIn& data)const
	{
		using namespace units::values;

		for (size_t v = 0; v < data.size(); v++)
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
						double Hs = data[v][i] * 1000;// g[H2o]/kg[air]
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

	string CCMIP6_MMGCreator::GetFilePath(string fileTitle)
	{
		return string(m_path) + fileTitle + ".nc";
	}



	ERMsg CCMIP6_MMGCreator::CreateMMG(int rcp, CMonthlyMeanGrid& MMG, CCallback& callback)
	{
		ERMsg msg;

		//Open input
		NcFilePtrArray ncFile1;
		NcFilePtrArray ncFile2;

		try
		{
			for (int v = 0; v < NB_VARIABLES_IN && msg; v++)
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
		for (int i = 0; i < NB_DIMS; i++)
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
		for (int v = 0; v < NB_FIELDS; v++)
		{
			string filePathOut = MMG.GetFilePath(v);
			if (!filePathOut.empty())
				msg += grid[v].CreateImage(filePathOut, options);
		}

		if (!msg)
			return msg;


		//callback.SetCurrentDescription(RCP_NAME[rcp]);
		//callback.SetNbStep((d1[DIM_TIME] + d2[DIM_TIME]) * NB_VARIABLES_IN);

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

			for (size_t dd = 0; dd < GetNbDayPerMonth(m); dd++, d++)
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
					size_t ii = y * m_LAF.size_x() + x;

					x = ii % m_LAF.size_x();
					y = size_t(ii / m_LAF.size_x());
					float LAF = m_LAF[y][x];
					if (data[I_TMIN][i] > -999 && data[I_TMAX][i] > -999 &&
						data[I_PRCP][i] > -999 && data[I_SPEH][i] > -999 &&
						data[I_WNDS][i] > -999 && m_LAF[y][x] > 50)
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
			for (int v = 0, vv = 0; v < NB_FIELDS; v++)
			{
				if (grid[v].IsOpen())
				{
					vector<float> data(output[vv].size());
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


	ERMsg CCMIP6_MMGCreator::GetDEM(const string& file_path, CMatrix<float>& DEM)
	{
		ERMsg msg;

		string tif_file_path = file_path + ".tif";
		//convert nc into GeoTIFF
		string argument = "-a_srs \"+proj=longlat +datum=WGS84 +no_defs\" -ot Float32 -stats -co COMPRESS=LZW -co PREDICTOR=3 -co TILED=YES -co BLOCKXSIZE=64 -co BLOCKYSIZE=64 \"" + file_path + "\" \"" + tif_file_path + "\"";
		string command = "\"" + GetApplicationPath() + "External\\gdal_translate.exe\" " + argument;
		msg += WinExecWait(command);


		CGDALDatasetEx DS;
		msg = DS.OpenInputImage(tif_file_path);
		if (msg)
		{
			GDALRasterBand* pBand = DS.GetRasterBand(0);

			vector<float> data(DS.GetRasterXSize() * DS.GetRasterYSize());
			pBand->RasterIO(GF_Read, 0, 0, DS.GetRasterXSize(), DS.GetRasterYSize(), &(data[0]), DS.GetRasterXSize(), DS.GetRasterYSize(), GDT_Float32, 0, 0);


			CGeoExtents extents = DS.GetExtents();
			DEM.resize(extents.m_ySize, extents.m_xSize);

			ASSERT(DEM.size_x() == m_LAF.size_x());
			ASSERT(DEM.size_y() == m_LAF.size_y());


			for (size_t i = 0; i < DEM.size_y(); i++)
			{
				for (size_t j = 0; j < DEM.size_x(); j++)
				{
					if (m_LAF[m_LAF.size_y() - i - 1][(j + m_LAF.size_x() / 2) % m_LAF.size_x()] > 50)
						DEM[DEM.size_y() - i - 1][(j + DEM.size_x() / 2) % DEM.size_x()] = data[i * DEM.size_x() + j];
					else
						DEM[DEM.size_y() - i - 1][(j + DEM.size_x() / 2) % DEM.size_x()] = -999;
				}
			}

			
		}




		//convert nc into GeoTIFF
		//string argument = "-ot Float32 -stats -co COMPRESS=LZW -co PREDICTOR=3 -co TILED=YES -co BLOCKXSIZE=256 -co BLOCKYSIZE=256 \"" + file_path + "\" \"" + file_path + ".tif\"";
		//string command = "\"" + GetApplicationPath() + "External\\gdal_translate.exe\" " + argument;
		//msg += WinExecWait(command);
		//msg += RemoveFile(output_file_path + "2");
		//msg += RemoveFile(output_file_path + "2.aux.xml");



		////string filePath = GetFilePath("orograw_CanESM2");
		//NcFile ncFile(filePath, NcFile::read);

		////CGeoExtents extents = GetGCM4Extents();

		//vector<float> data(extents.m_xSize * extents.m_ySize);
		//vector<size_t> startp(2);
		//vector<size_t> countp(2);
		//countp[0] = extents.m_ySize;
		//countp[1] = extents.m_xSize;


		//NcVar& var = ncFile.getVar("orograw");
		//var.getVar(startp, countp, &(data[0]));


		//DEM.resize(extents.m_ySize, extents.m_xSize);

		//for (size_t i = 0; i < DEM.size_y(); i++)
		//{
		//	for (size_t j = 0; j < DEM.size_x(); j++)
		//	{
		//		if (m_LAF[m_LAF.size_y() - i - 1][(j + m_LAF.size_x() / 2) % m_LAF.size_x()] > 50)
		//			DEM[DEM.size_y() - i - 1][(j + DEM.size_x() / 2) % DEM.size_x()] = data[i * DEM.size_x() + j];
		//		else
		//			DEM[DEM.size_y() - i - 1][(j + DEM.size_x() / 2) % DEM.size_x()] = -999;
		//	}
		//}

		return msg;
	}

	ERMsg CCMIP6_MMGCreator::GetLAF(const string& file_path, CMatrix<float>& LAF)
	{
		ERMsg msg;

		string tif_file_path = file_path + ".tif";
		//convert nc into GeoTIFF
		string argument = "-a_srs \"+proj=longlat +datum=WGS84 +no_defs\" -ot Float32 -stats -co COMPRESS=LZW -co PREDICTOR=3 -co TILED=YES -co BLOCKXSIZE=64 -co BLOCKYSIZE=64 \"" + file_path + "\" \"" + tif_file_path + "\"";
		string command = "\"" + GetApplicationPath() + "External\\gdal_translate.exe\" " + argument;
		msg += WinExecWait(command);


		CGDALDatasetEx DS;
		msg = DS.OpenInputImage(tif_file_path);
		if (msg)
		{
			GDALRasterBand* pBand = DS.GetRasterBand(0);

			vector<float> data(DS.GetRasterXSize() * DS.GetRasterYSize());
			pBand->RasterIO(GF_Read, 0, 0, DS.GetRasterXSize(), DS.GetRasterYSize(), &(data[0]), DS.GetRasterXSize(), DS.GetRasterYSize(), GDT_Float32, 0, 0);


			CGeoExtents extents = DS.GetExtents();
			LAF.resize(extents.m_ySize, extents.m_xSize);

			for (size_t i = 0; i < LAF.size_y(); i++)
			{
				for (size_t j = 0; j < LAF.size_x(); j++)
				{
					LAF[LAF.size_y() - i - 1][(j + LAF.size_x() / 2) % LAF.size_x()] = data[i * LAF.size_x() + j];
				}
			}

		}

		//msg += RemoveFile(output_file_path + "2");
		//msg += RemoveFile(output_file_path + "2.aux.xml");


		return msg;

		/*string filePath = GetFilePath("sftlf_CanESM2");
		NcFile ncFile(filePath, NcFile::read);

		CGeoExtents extents = GetGCM4Extents();

		vector<float> data(extents.m_xSize * extents.m_ySize);
		vector<size_t> startp(2);
		vector<size_t> countp(2);
		countp[0] = extents.m_ySize;
		countp[1] = extents.m_xSize;


		NcVar& var = ncFile.getVar("sftlf");
		var.getVar(startp, countp, &(data[0]));


		LAF.resize(extents.m_ySize, extents.m_xSize);

		for (size_t i = 0; i < LAF.size_y(); i++)
			for (size_t j = 0; j < LAF.size_x(); j++)
				LAF[LAF.size_y() - i - 1][(j + LAF.size_x() / 2) % LAF.size_x()] = data[i * LAF.size_x() + j];*/


				/*if (SAVE_TOPO)
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
				}*/

		return msg;
	}

}