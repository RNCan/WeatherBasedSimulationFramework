#include "StdAfx.h"
#include "ERA5.h"
#include "Basic/WeatherStation.h"
#include "Geomatic/ShapeFileBase.h"
#include "TaskFactory.h"
#include "Geomatic/TimeZones.h"
#include "Geomatic/SfcGribsDatabase.h"
#include "UI/Common/SYShowMessage.h"

#include "WeatherBasedSimulationString.h"
#include "../Resource.h"
#pragma warning(disable: 4275 4251)
#include "gdal_priv.h"

using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace UtilWWW;

namespace WBSF
{
	//*********************************************************************

	CERA5::CERA5(const std::string& workingDir) :
		m_workingDir(workingDir)
	{}

	CERA5::~CERA5(void)
	{}


	enum TERA5Var { ERA5_TMIN, ERA5_TMAX, ERA5_PRCP, ERA5_TDEW, ERA5_RELH, ERA5_WNDS, ERA5_WNDD, ERA5_SRAD, ERA5_PRES, ERA5_SNOW, ERA5_SNDH, ERA5_SWE, ERA5_GHGT, NB_ERA5_VARS };
	static const  size_t VARS_ERA5[NB_ERA5_VARS] = { H_TMIN, H_TMAX, H_PRCP, H_TDEW, H_RELH, H_WNDS, H_WNDD, H_SRAD, H_PRES, H_SNOW, H_SNDH, H_SWE, H_GHGT };
	static const char* SUB_DIR[NB_ERA5_VARS] = { "tmin","tmax", "prcptot", "tdmean", "rhmean", "wsmean", "wdmean", "ssrmean","spmean", "sftot", "sdmean", "swemean", "z"};
	//static const char* UNITS[NB_ERA5_VARS] = { "[C]", "[C]", "[mm]", "[C]","[%]","[km/h]","[�]","[W/(m^2)]","[hPa]","[mm]","[cm]","[mm]", "m"};

	const char* ERA5_META_DATA[NB_ERA5_VARS][NB_META] =
	{
		{"2[m] HTGL \"Minimum Temperature [C]\"", "Minimum Temperature [C]", "TMIN", "2-HTGL", "[C]"},
		{"2[m] HTGL \"Maximum Temperature [C]\"", "Maximum Temperature [C]", "TMAX", "2-HTGL", "[C]"},
		{ "0[-] SFC \"Total precipitation [mm]\"","Total precipitation [mm]","APCP01","0-SFC","[mm]" },
		{ "2[m] HTGL \"Dew point temperature [C]\"","Dew point temperature [C]","DPT","2-HTGL","[C]" },
		{ "2[m] HTGL \"Relative humidity [%]\"","Relative humidity [%]","RH","2-HTGL","[%]" },
		{ "10[m] HTGL \"Wind speed [km/h]\"","Wind speed [km/h]","WIND","10-HTGL","[km/h]" },
		{ "10[m] HTGL \"Wind direction (from which blowing) [deg true]\"","Wind direction (from which blowing) [deg true]","WDIR","10-HTGL","[deg true]" },
		{ "0[-] SFC \"Downward short-wave radiation flux [W/(m^2)]\"","Downward short-wave radiation flux [W/(m^2)]","DSWRF","0-SFC","[W/(m^2)]" },
		{ "0[-] SFC \"Pressure [hPa]\"","Pressure [hPa]","PRES","0-SFC","[hPa]" },
		{ "0[-] SFC \"Total snowfall [mm]\"","Total snowfall [mm]","ASNOW","0-SFC","[mm]" },
		{ "0[-] SFC \"Snow depth [cm]\"","Snow depth [cm]","SNOD","0-SFC","[cm]" },
		{ "0[-] SFC \"Water equivalent of accumulated snow depth [mm]\"","Water equivalent of accumulated snow depth [mm]","WEASD","0-SFC","[mm]" },
		{ "0[-] SFC \"Height\"","Height [m]","HGT","0-SFC","[m]" },
	};



	//****************************************************************************************************
	ERMsg CERA5::Execute(CCallback& callback)
	{
		ERMsg msg;

		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(m_workingDir, 1);
		callback.AddMessage(GetString(IDS_UPDATE_FROM));
		callback.AddMessage("");

		GDALSetCacheMax64(128 * 1024 * 1024);

		ASSERT(m_first_year <= m_last_year);

		size_t nb_years = m_last_year - m_first_year + 1; 
		callback.PushTask("Create ERA5: " + ToString(nb_years) + " year", nb_years);

		for (size_t y = 0; y < nb_years&&msg; y++)
		{
			int year = int(m_first_year+y);
			msg += CERA5::CreateDailyGeotiff("canada_0p25grid", year, callback);
			msg += callback.StepIt();
		}

		callback.PopTask();

		return msg;
	}
	
	//canada_0p25grid
	ERMsg CERA5::CreateDailyGeotiff(const string& title, int year, CCallback& callback)const
	{
		ERMsg msg;



		//string inputFilePath = m_workingDir + "z/era5_z_canada_0p25grid.tif";
		//CGDALDatasetEx DS_z;
		//msg = DS_z.OpenInputImage(inputFilePath);

//		int year = 2021;

		array<CGDALDatasetEx, NB_ERA5_VARS> DSin;
		for (size_t v = 0; v < DSin.size(); v++)
		{
			string inputFilePath = FormatA("%s%s\\era5_%s_daily_%4d_%s.tif", m_workingDir.c_str(), SUB_DIR[v], SUB_DIR[v], year, title.c_str());
			if(v==ERA5_GHGT)
				inputFilePath = FormatA("%s%s\\era5_%s_%s.tif", m_workingDir.c_str(), SUB_DIR[v], SUB_DIR[v], title.c_str());

			msg += DSin[v].OpenInputImage(inputFilePath);
		}


		if (msg)
		{
			size_t nb_days = DSin[0].GetRasterCount();
			callback.PushTask("Create ERA5 for year " + ToString(year)+ ": " + ToString(nb_days) + " days", nb_days);

			float no_data = 9999;// GetDefaultNoData(GDT_Int16);
			CBaseOptions options;
			DSin[0].UpdateOption(options);

			
			for (size_t d = 0; d < nb_days&&msg; d++)
			{
				options.m_nbBands = NB_ERA5_VARS;
				options.m_extents.m_xBlockSize = 256;
				options.m_extents.m_yBlockSize = 256;
				options.m_outputType = GDT_Float32;
				options.m_dstNodata = no_data;
				options.m_bOverwrite = true;
				options.m_bComputeStats = true;
				options.m_createOptions.push_back("COMPRESS=LZW");
				options.m_createOptions.push_back("PREDICTOR=3");
				options.m_createOptions.push_back("TILED=YES");
				options.m_createOptions.push_back("BLOCKXSIZE=256");
				options.m_createOptions.push_back("BLOCKYSIZE=256");

				CJDayRef TRef(year, d);

				string output_file_path = FormatA("%s%d\\%02d\\ERA5_%d%02d%02d_%s.tif", m_workingDir.c_str(), TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, title.c_str());
				msg = CreateMultipleDir(GetPath(output_file_path));

				CGDALDatasetEx DSout;
				msg += DSout.CreateImage(output_file_path + "2", options);
				if (msg)
				{


					for (size_t v = 0; v < DSin.size(); v++)
					{
						ASSERT(v == ERA5_GHGT || DSin[0].GetRasterCount() == DSin[v].GetRasterCount());
						GDALRasterBand* pBandin = DSin[v].GetRasterBand(v== ERA5_GHGT ?0:d);
						GDALRasterBand* pBandout = DSout.GetRasterBand(v);


						ASSERT(DSin[v].GetRasterXSize() == DSout.GetRasterXSize());
						ASSERT(DSin[v].GetRasterYSize() == DSout.GetRasterYSize() || DSin[v].GetRasterYSize() == DSout.GetRasterYSize()-1 || DSin[v].GetRasterYSize() == DSout.GetRasterYSize() + 1);

						
						float no_data_in = DSin[v].GetNoData(0);
						vector<float> data(DSin[v].GetRasterXSize() * DSin[v].GetRasterYSize());
						pBandin->RasterIO(GF_Read, 0, 0, DSin[v].GetRasterXSize(), DSin[v].GetRasterYSize(), &(data[0]), DSin[v].GetRasterXSize(), DSin[v].GetRasterYSize(), GDT_Float32, 0, 0);

						//replace small values of snow by 0
						if (v == ERA5_SNOW)
						{
							for (size_t xy = 0; xy < data.size(); xy++)
							{
								if (data[xy] < 0.001)
									data[xy] = 0;
							}
						}
						

						if (DSin[v].GetRasterYSize() == DSout.GetRasterYSize())
						{
							pBandout->RasterIO(GF_Write, 0, 0, DSout.GetRasterXSize(), DSout.GetRasterYSize(), &(data[0]), DSout.GetRasterXSize(), DSout.GetRasterYSize(), GDT_Float32, 0, 0);
						}
						else
						{
							pBandout->RasterIO(GF_Write, 0, 0, DSout.GetRasterXSize(), DSout.GetRasterYSize(), &(data[0]), DSin[v].GetRasterXSize(), DSin[v].GetRasterYSize(), GDT_Float32, 0, 0);
						}
						
						pBandout->SetDescription(ERA5_META_DATA[v][M_DESC]);
						pBandout->SetMetadataItem("GRIB_COMMENT", ERA5_META_DATA[v][M_COMMENT]);
						pBandout->SetMetadataItem("GRIB_ELEMENT", ERA5_META_DATA[v][M_ELEMENT]);
						pBandout->SetMetadataItem("GRIB_SHORT_NAME", ERA5_META_DATA[v][M_SHORT_NAME]);
						pBandout->SetMetadataItem("GRIB_UNIT", ERA5_META_DATA[v][M_UNIT]);



						DSout->FlushCache();

					}//for all variables

					DSout.Close();


					if (msg)
					{
						//copy the file to fully use compression with GDAL_translate
						string argument = "-ot Float32 -stats -co COMPRESS=LZW -co PREDICTOR=3 -co TILED=YES -co BLOCKXSIZE=256 -co BLOCKYSIZE=256 \"" + output_file_path + "2" + "\" \"" + output_file_path + "\"";
						string command = "\"" + GetApplicationPath() + "External\\gdal_translate.exe\" " + argument;
						msg += WinExecWait(command);
						msg += RemoveFile(output_file_path + "2");
						msg += RemoveFile(output_file_path + "2.aux.xml");
					}

				}//if msg

				msg+=callback.StepIt();
			}//for all days

			callback.PopTask();
		}//if dataset open




		return msg;
	}


	//****************************************************************************************************

	//string CERA5::GetOutputFilePath(const string& filePath)const
	//{
	//	CTRef TRef = GetRemoteTRef(filePath);
	//	string title = GetFileTitle(filePath);
	//	return FormatA("%s%d\\%02d\\%02d\\%s.grib2", m_workingDir.c_str(), TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, title.c_str());
	//}


	//ERMsg CERA5::CreateDailyGeotiff(StringVector& filesList, const std::string& file_path_out, CCallback& callback)const
	//{
	//	ERMsg msg;

	//	GribVariables var_in;
	//	vector<CSfcDatasetCached> DSin(filesList.size());
	//	map<size_t, const char*> description;
	//	map<size_t, char**> meta_data;


	//	for (size_t i = 0; i < filesList.size(); i++)
	//	{
	//		msg += DSin[i].open(filesList[i], true);
	//		if (msg)
	//		{
	//			var_in = var_in | DSin[i].get_variables();
	//			for (size_t v = 0; v < DSin[i].get_variables().size() && msg; v++)
	//			{
	//				if (var_in.test(v))
	//				{
	//					size_t b = DSin[i].get_band(v);
	//					ASSERT(b != NOT_INIT); //in HRRR all bands are available

	//					GDALRasterBand* poBand = DSin[i]->GetRasterBand(int(b) + 1);//one base in direct load
	//					if (poBand->GetDescription())
	//						description[v] = poBand->GetDescription();

	//					if (poBand->GetMetadata())
	//						meta_data[v] = poBand->GetMetadata();

	//				}
	//			}
	//		}
	//	}

	//	if (msg)
	//	{
	//		GribVariables var_out = var_in;

	//		if (var_out[H_TAIR])
	//		{
	//			//if temperature present, add min and max
	//			var_out.set(H_TMIN);
	//			var_out.set(H_TMAX);
	//		}



	//		float no_data = 9999;

	//		CBaseOptions options;
	//		DSin[0].UpdateOption(options);
	//		options.m_extents.m_xBlockSize = 256;
	//		options.m_extents.m_yBlockSize = 256;
	//		options.m_nbBands = var_out.count();
	//		options.m_outputType = GDT_Float32;
	//		options.m_dstNodata = no_data;
	//		options.m_bOverwrite = true;
	//		options.m_bComputeStats = true;
	//		options.m_createOptions.push_back("COMPRESS=LZW");
	//		options.m_createOptions.push_back("PREDICTOR=3");
	//		options.m_createOptions.push_back("TILED=YES");
	//		options.m_createOptions.push_back("BLOCKXSIZE=256");
	//		options.m_createOptions.push_back("BLOCKYSIZE=256");

	//		CGDALDatasetEx DSout;
	//		msg += DSout.CreateImage(file_path_out + "2", options);

	//		if (msg)
	//		{
	//			CGeoExtents extents = DSout.GetExtents();
	//			callback.PushTask("Create daily HRRR Geotiff for " + GetFileTitle(file_path_out) + ": " + ToString(filesList.size()) + " hours", var_in.count() * extents.YNbBlocks() * extents.XNbBlocks());

	//			for (size_t by = 0; by < extents.YNbBlocks(); by++)
	//			{
	//				for (size_t bx = 0; bx < extents.XNbBlocks(); bx++)
	//				{
	//					size_t b_out = 0;
	//					for (size_t v = 0; v < var_out.size() && msg; v++)
	//					{
	//						if (var_in.test(v) && var_out.test(v))
	//						{
	//							CGeoExtents block_ext = extents.GetBlockExtents(int(bx), int(by));
	//							vector<CStatistic> stat;
	//							//(block_ext.GetNbPixels());

	//							for (size_t i = 0; i < DSin.size() && msg; i++)
	//							{
	//								size_t b = DSin[i].get_band(v);
	//								ASSERT(b != NOT_INIT); //in HRRR all bands are available

	//								if (b != NOT_INIT)
	//								{
	//									//copy part of the date
	//									//CGeoExtents ext = DSin[i].GetExtents().GetBlockExtents(int(i), int(j));

	//									GDALRasterBand* poBand = DSin[i]->GetRasterBand(int(b) + 1);//one base in direct load
	//									int nXBlockSize, nYBlockSize;
	//									poBand->GetBlockSize(&nXBlockSize, &nYBlockSize);
	//									if (stat.empty())
	//										stat.resize(nXBlockSize * nYBlockSize);

	//									GDALDataType type = poBand->GetRasterDataType();

	//									vector<float> data;

	//									if (nXBlockSize == options.m_extents.m_xBlockSize &&
	//										nYBlockSize == options.m_extents.m_yBlockSize &&
	//										type == GDT_Float32)
	//									{
	//										data.resize(nXBlockSize * nYBlockSize, no_data);
	//										poBand->ReadBlock(int(bx), int(by), &(data[0]));
	//										poBand->FlushBlock(int(bx), int(by));
	//										poBand->FlushCache();
	//									}//
	//									else//if not the same block size or type
	//									{
	//										CGeoExtents blockExtent = extents.GetBlockExtents(int(bx), int(by));
	//										data.resize(DSout.GetRasterXSize() * DSout.GetRasterYSize());

	//										poBand->RasterIO(GF_Read, blockExtent.m_xMin, blockExtent.m_yMin, blockExtent.m_xBlockSize, blockExtent.m_yBlockSize, &(data[0]), blockExtent.m_xBlockSize, blockExtent.m_yBlockSize, GDT_Float32, 0, 0);
	//										ASSERT(data.size() == stat.size());
	//									}

	//									for (size_t xy = 0; xy < data.size(); xy++)
	//									{
	//										if (fabs(data[xy] - no_data) > 0.1)
	//											stat[xy] += data[xy];
	//									}


	//								}//if valid band
	//							}//for all input band


	//							if (v == H_TAIR)
	//							{
	//								GDALRasterBand* pBandout = DSout.GetRasterBand(b_out);

	//								int nXBlockSize, nYBlockSize;
	//								pBandout->GetBlockSize(&nXBlockSize, &nYBlockSize);

	//								vector<float> data(nXBlockSize * nYBlockSize, no_data);
	//								ASSERT(data.size() == stat.size());

	//								//add min and max
	//								for (size_t xy = 0; xy < data.size(); xy++)
	//								{
	//									if (stat[xy].IsInit())
	//										data[xy] = stat[xy][LOWEST];
	//								}




	//								pBandout->WriteBlock(int(bx), int(by), &(data[0]));


	//								//pBandout->RasterIO(GF_Write, 0, 0, DSout.GetRasterXSize(), DSout.GetRasterYSize(), &(data[0]), DSout.GetRasterXSize(), DSout.GetRasterYSize(), GDT_Float32, 0, 0);
	//								pBandout->SetDescription("2[m] HTGL=\"Specified height level above ground\"");
	//								pBandout->SetMetadataItem("GRIB_COMMENT", "Minimum Temperature [C]");
	//								pBandout->SetMetadataItem("GRIB_ELEMENT", "TMIN");
	//								pBandout->SetMetadataItem("GRIB_SHORT_NAME", "2-HTGL");
	//								pBandout->SetMetadataItem("GRIB_UNIT", "[C]");

	//								b_out++;
	//							}

	//							{
	//								size_t stat_type = (v == H_PRCP) ? SUM : MEAN;
	//								GDALRasterBand* pBandout = DSout.GetRasterBand(b_out);

	//								int nXBlockSize, nYBlockSize;
	//								pBandout->GetBlockSize(&nXBlockSize, &nYBlockSize);

	//								vector<float> data(nXBlockSize * nYBlockSize, no_data);
	//								ASSERT(data.size() == stat.size());

	//								for (size_t xy = 0; xy < data.size(); xy++)
	//								{
	//									if (stat[xy].IsInit())
	//										data[xy] = stat[xy][stat_type];
	//								}


	//								pBandout->WriteBlock(int(bx), int(by), &(data[0]));
	//								pBandout->SetDescription(description[v]);
	//								pBandout->SetMetadata(meta_data[v]);
	//								b_out++;
	//							}

	//							if (v == H_TAIR)
	//							{
	//								GDALRasterBand* pBandout = DSout.GetRasterBand(b_out);

	//								int nXBlockSize, nYBlockSize;
	//								pBandout->GetBlockSize(&nXBlockSize, &nYBlockSize);

	//								vector<float> data(nXBlockSize * nYBlockSize, no_data);
	//								ASSERT(data.size() == stat.size());

	//								for (size_t xy = 0; xy < data.size(); xy++)
	//								{
	//									if (stat[xy].IsInit())
	//										data[xy] = stat[xy][HIGHEST];
	//								}

	//								pBandout->WriteBlock(int(bx), int(by), &(data[0]));

	//								pBandout->SetDescription("2[m] HTGL=\"Specified height level above ground\"");
	//								pBandout->SetMetadataItem("GRIB_COMMENT", "Maximum Temperature [C]");
	//								pBandout->SetMetadataItem("GRIB_ELEMENT", "TMAX");
	//								pBandout->SetMetadataItem("GRIB_SHORT_NAME", "2-HTGL");
	//								pBandout->SetMetadataItem("GRIB_UNIT", "[C]");


	//								b_out++;
	//							}

	//							msg += callback.StepIt();
	//						}//if var used
	//					}//for all variable
	//				}//for all block x
	//			}//for all block y

	//			DSout.Close(options);

	//			callback.PopTask();
	//		}//out open

	//		for (size_t i = 0; i < filesList.size(); i++)
	//			DSin[i].close();

	//		if (msg)
	//		{
	//			//convert with gdal_translate to optimize size
	//			string argument = "-ot Float32 -a_nodata 9999 -stats -co COMPRESS=LZW -co PREDICTOR=3 -co TILED=YES -co BLOCKXSIZE=256 -co BLOCKYSIZE=256 \"" + file_path_out + "2" + "\" \"" + file_path_out + "\"";
	//			string command = "\"" + GetApplicationPath() + "External\\gdal_translate.exe\" " + argument;
	//			msg += WinExecWait(command);
	//			msg += RemoveFile(file_path_out + "2");
	//		}


	//	}//if msg


	//	return msg;
	//}


	ERMsg CERA5::GetGribsList(CTPeriod p, CGribsMap& gribsList, CCallback& callback)
	{
		ERMsg msg;

		if (p.GetTM() == CTM::HOURLY)
			return msg;

		//p = p.as(CTM::DAILY);
		int firstYear = p.Begin().GetYear();
		int lastYear = p.End().GetYear();
		size_t nbYears = lastYear - firstYear + 1;

		for (size_t y = 0; y < nbYears; y++)
		{
			int year = firstYear + int(y);

			StringVector list1;
			string filter = "\\*.tif";
			list1 = GetFilesList(m_workingDir + ToString(year) + filter, FILE_PATH, true);

			for (size_t i = 0; i < list1.size(); i++)
			{
				CTRef TRef = GetLocalTRef(list1[i]);
				if (p.IsInside(TRef))
					gribsList[TRef] = list1[i];
			}

		}

		return msg;
	}

	CTRef CERA5::GetLocalTRef(string filePath)
	{
		CTRef TRef;

		string title = GetFileTitle(filePath);
		int year = WBSF::as<int>(title.substr(5, 4));
		size_t m = WBSF::as<int>(title.substr(9, 2))-1;
		size_t d = WBSF::as<int>(title.substr(11, 2))-1;
		TRef = CTRef(year, m, d);

		return TRef;
	}



}