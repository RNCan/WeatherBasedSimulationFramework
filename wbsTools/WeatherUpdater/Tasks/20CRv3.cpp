#include "StdAfx.h"
#include "20CRv3.h"
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

	C20CRv3::C20CRv3(const std::string& workingDir) :
		m_workingDir(workingDir),
		m_show_download(false)
	{}

	C20CRv3::~C20CRv3(void)
	{}


	enum T20CRv3Var { CR_TMIN, CR_TMAX, CR_PRCP, CR_TDEW, CR_RELH, CR_WNDS, CR_WNDD, CR_GHGT, NB_20CRV3_VARS };

	static const  size_t VARS_20CRV3[NB_20CRV3_VARS] = { H_TMIN, H_TMAX, H_PRCP, H_TDEW, H_RELH, H_WNDS, H_WNDD, H_GHGT };

	const char* META_DATA_20CRV3[NB_20CRV3_VARS][NB_META] =
	{
		{"2[m] HTGL \"Minimum Temperature [C]\"", "Minimum Temperature [C]", "TMIN", "2-HTGL", "[C]"},
		{"2[m] HTGL \"Maximum Temperature [C]\"", "Maximum Temperature [C]", "TMAX", "2-HTGL", "[C]"},
		{ "0[-] SFC \"Total precipitation [mm]\"","Total precipitation [mm]","APCP01","0-SFC","[mm]" },
		{ "2[m] HTGL \"Dew point temperature [C]\"","Dew point temperature [C]","DPT","2-HTGL","[C]" },
		{ "2[m] HTGL \"Relative humidity [%]\"","Relative humidity [%]","RH","2-HTGL","[%]" },
		{ "10[m] HTGL \"Wind speed [km/h]\"","Wind speed [km/h]","WIND","10-HTGL","[km/h]" },
		{ "10[m] HTGL \"Wind direction (from which blowing) [deg true]\"","Wind direction (from which blowing) [deg true]","WDIR","10-HTGL","[deg true]" },
		{ "0[-] SFC \"Height\"","Height [m]","HGT","0-SFC","[m]" },
	};



	//****************************************************************************************************
	ERMsg C20CRv3::Execute(CCallback& callback)
	{
		ERMsg msg;

		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(m_workingDir, 1);
		callback.AddMessage(GetString(IDS_UPDATE_FROM));
		callback.AddMessage("");

		GDALSetCacheMax64(128 * 1024 * 1024);

		ASSERT(m_first_year <= m_last_year);





		size_t nb_years = m_last_year - m_first_year + 1;
		callback.PushTask("Create 20CRv3: " + ToString(nb_years) + " year", nb_years);

		for (size_t y = 0; y < nb_years && msg; y++)
		{
			int year = int(m_first_year + y);


			/*for (size_t m = 0; m < 12 && msg; m++)
			{
				for (size_t d = 0; d < GetNbDayPerMonth(year, m) && msg; d++)
				{
					CTRef TRef(year, m, d);
					string output_file_path = FormatA("%s%d\\%02d\\ERA5_%d%02d%02d.tif", m_workingDir.c_str(), TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1);
					if (!FileExists(output_file_path))
					{
						msg += Download(TRef, callback);
						if (msg)
							msg += CreateDailyGeotiff(TRef, callback);
					}


					msg += callback.StepIt();

				}
			}*/

			msg += CreateDailyGeotiff(year, callback);


			msg += callback.StepIt();
		}

		callback.PopTask();

		return msg;
	}


	enum T20CRv3VarIn { D_20CRV3_TMIN, D_20CRV3_TMAX, D_20CRV3_PRCP, D_20CRV3_RELH, D_20CRV3_WNDU, D_20CRV3_WNDV, D_20CRV3_GEOPOTENTIAL, NB_20CRV3_VARS_DAILY };
	static const array<string, NB_20CRV3_VARS_DAILY> NAME_20CRV3_D =
	{
		"tmin.2m","tmax.2m", "apcp", "rhum.2m", "uwnd.10m", "vwnd.10m", "hgt.sfc"
	};

	static const array<string, NB_20CRV3_VARS_DAILY> LAYER_NAME =
	{
		"tmin","tmax", "apcp", "rhum", "uwnd", "vwnd", ""
	};

	ERMsg C20CRv3::Download(int year, CCallback& callback)const
	{
		ERMsg msg;

		//bool already_downloaded = true;
		//for (size_t v = 0; v < NB_20CRV3_VARS_DAILY && already_downloaded; v++)
		//{
		//	string inputFilePath = FormatA("%s%s.%d.nc", m_workingDir.c_str(), NAME_20CRV3_H[v].c_str(), TRef.GetYear());

		//	if (!FileExists(inputFilePath))
		//		already_downloaded = false;
		//}

		//if (already_downloaded)
		//	return msg;


		//string all_variables;
		//for (auto it = NAME_20CRV3_H.begin(); it != NAME_20CRV3_H.end(); it++)
		//{
		//	all_variables += all_variables.empty() ? "" : " ";
		//	all_variables += *it;
		//}


		//callback.PushTask("Download 20CRv3 for year " + TRef.GetFormatedString(), NOT_INIT);

		//string output_filepath_tmp = FormatA("%s%d\\%02d\\%02d\\ERA5", m_workingDir.c_str(), TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1);
		//msg = CreateMultipleDir(GetPath(output_filepath_tmp));

		//if (msg)
		//{
		//	string box;
		//	if (m_bounding_box.m_xMax != -90 || m_bounding_box.m_yMin != -180 || m_bounding_box.m_xMin != 90 || m_bounding_box.m_yMax != 180)
		//		box = WBSF::FormatA(" --area %lf %lf %lf %lf", m_bounding_box.m_yMax, m_bounding_box.m_xMin, m_bounding_box.m_yMin, m_bounding_box.m_xMax);

		//	string date = " --startyear " + to_string(TRef.GetYear()) + " --months " + to_string(TRef.GetMonth() + 1) + " --day " + to_string(TRef.GetDay() + 1);
		//	if (TRef.GetYear() < 1979)
		//		date += " --prelimbe";

		//	string argument = "hourly --variables " + all_variables + date + box + " --levels surface --threads 6 --format grib --outputprefix \"" + output_filepath_tmp + "\"";
		//	string command = "era5cli.exe " + argument;
		//	msg += WinExecWait(command, GetPath(output_filepath_tmp), m_show_download ? SW_SHOW : SW_HIDE);
		//}



		//if (msg)
		//{
		//	string box;
		//	if (m_bounding_box.m_xMax != -90 || m_bounding_box.m_yMin != -180 || m_bounding_box.m_xMin != 90 || m_bounding_box.m_yMax != 180)
		//	{
		//		char a1 = m_bounding_box.m_xMin < 0 ? 'W' : 'E';
		//		char a2 = m_bounding_box.m_xMax < 0 ? 'W' : 'E';
		//		char a3 = m_bounding_box.m_yMin < 0 ? 'S' : 'N';
		//		char a4 = m_bounding_box.m_yMax < 0 ? 'S' : 'N';
		//		box = WBSF::FormatA("_%.0lf%c-%.0lf%c_%.0lf%c-%.0lf%c", abs(m_bounding_box.m_xMin), a1, abs(m_bounding_box.m_xMax), a2, abs(m_bounding_box.m_yMin), a3, abs(m_bounding_box.m_yMax), a4);
		//	}

		//	for (size_t v = 0; v < NB_20CRV3_VARS_DAILY && msg; v++)
		//	{
		//		string output_filepath1 = output_filepath_tmp + "_" + NAME_20CRV3_H[v] + "_" + to_string(TRef.GetYear()) + "_hourly" + box + ".grb";
		//		if (!FileExists(output_filepath1))
		//			msg.ajoute("File doesn't exist: " + output_filepath1);

		//		if (msg)
		//		{
		//			string output_filepath2 = FormatA("%s%d\\%02d\\%02d\\ERA5_%d%02d%02d_%s.grd", m_workingDir.c_str(), TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, NAME_20CRV3_H[v].c_str());
		//			//msg = CreateMultipleDir(GetPath(output_filepath2));
		//			if (FileExists(output_filepath2))
		//				msg += RemoveFile(output_filepath2);

		//			if (msg)
		//				msg += WBSF::RenameFile(output_filepath1, output_filepath2);
		//		}
		//	}

		//}


		//callback.PopTask();

		return msg;
	}

	size_t xy_out(size_t xy)
	{
		size_t x = xy % 360;
		size_t y = xy / 360;
		x = x < 180 ? x + 180 : x - 180;
		size_t xy_out = y * 360 + x;
		return xy_out;
	}

	ERMsg C20CRv3::CreateDailyGeotiff(int year, CCallback& callback)const
	{
		ERMsg msg;


		string sflx_file_path = FormatA("%sNetCDF\\land.sflx.nc", m_workingDir.c_str());
		CGDALDatasetEx DSsflx;
		msg += DSsflx.OpenInputImage(sflx_file_path);
		if (!msg)
			return msg;

		GDALRasterBand* pBandin = DSsflx.GetRasterBand(0);
		//int nb_bands = DSsflx.GetRasterCount();


		float no_data_in = DSsflx.GetNoData(0);
		vector<float> sflx(DSsflx.GetRasterXSize() * DSsflx.GetRasterYSize(), no_data_in);
		pBandin->RasterIO(GF_Read, 0, 0, DSsflx.GetRasterXSize(), DSsflx.GetRasterYSize(), &(sflx[0]), DSsflx.GetRasterXSize(), DSsflx.GetRasterYSize(), GDT_Float32, 0, 0);


		callback.PushTask("Create 20CRv3 for year " + to_string(year) + ": " + ToString(GetNbDaysPerYear(year)) + " days", GetNbDaysPerYear(year));


		CTPeriod p(year, JANUARY, DAY_01,year, DECEMBER, DAY_31);
		for (CTRef TRef = p.Begin(); TRef <= p.End() && msg; TRef++)
		{

			array<CGDALDatasetEx, NB_20CRV3_VARS_DAILY> DSin;
			for (size_t v = 0; v < NB_20CRV3_VARS_DAILY; v++)
			{
				string inputFilePath = FormatA("NETCDF:%sNetCDF\\%s\\%s.%d.nc:%s", m_workingDir.c_str(), NAME_20CRV3_D[v].c_str(), NAME_20CRV3_D[v].c_str(), TRef.GetYear(), LAYER_NAME[v].c_str());
				if (v == D_20CRV3_GEOPOTENTIAL)
					inputFilePath = FormatA("%sNetCDF\\%s.nc", m_workingDir.c_str(), NAME_20CRV3_D[v].c_str());

				msg += DSin[v].OpenInputImage(inputFilePath);
			}

			if (msg)
			{
				string output_file_path = FormatA("%s%d\\%02d\\20CRv3_%d%02d%02d.tif", m_workingDir.c_str(), TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1);
				//size_t nb_days = DSin[0].GetRasterCount();
				//callback.PushTask("Create 20CRv3 for year " + TRef.GetFormatedString(), NB_20CRV3_VARS_DAILY);

				float no_data = 9999;
				CBaseOptions options;
				DSin[D_20CRV3_GEOPOTENTIAL].UpdateOption(options);

				options.m_nbBands = NB_20CRV3_VARS;
				options.m_extents.m_xMin = -180.5;
				options.m_extents.m_xMax = 179.5;
				options.m_extents.m_xBlockSize = 64;
				options.m_extents.m_yBlockSize = 64;
				options.m_outputType = GDT_Float32;
				options.m_dstNodata = no_data;
				options.m_bOverwrite = true;
				options.m_bComputeStats = true;
				options.m_createOptions.clear();
				options.m_createOptions.push_back("COMPRESS=LZW");
				//options.m_createOptions.push_back("PREDICTOR=3");
				options.m_createOptions.push_back("TILED=YES");
				options.m_prj = "WGS84";
				//options.m_createOptions.push_back("BLOCKXSIZE=256");
				//options.m_createOptions.push_back("BLOCKYSIZE=256");


				msg = CreateMultipleDir(GetPath(output_file_path));

				CGDALDatasetEx DSout;
				msg += DSout.CreateImage(output_file_path + "2", options);
				if (msg)
				{
					//compute variable: Tmin, Tmax, wind speed and direction and snow depth 
					size_t size_xy = DSin[0].GetRasterXSize() * DSin[0].GetRasterYSize();
					array<vector<float>, NB_20CRV3_VARS> data;

					for (size_t v = 0; v < NB_20CRV3_VARS; v++)
					{
						data[v].resize(size_xy, no_data);
					}


					for (size_t v = 0; v < NB_20CRV3_VARS_DAILY && msg; v++)
					{
						if (DSin[v].IsOpen())
						{
							ASSERT(DSin[v].GetRasterCount() == 1 || DSin[v].GetRasterCount() == 365 || DSin[v].GetRasterCount() == 366);
							ASSERT(TRef.GetJDay() < DSin[v].GetRasterCount());

							//for (size_t h = 0; h < DSin[v].GetRasterCount() && msg; h++)
							//{
							GDALRasterBand* pBandin = DSin[v].GetRasterBand(v == D_20CRV3_GEOPOTENTIAL?0:TRef.GetJDay());
							ASSERT(DSin[v].GetRasterXSize() * DSin[v].GetRasterYSize() == size_xy);

							float no_data_in = DSin[v].GetNoData(0);
							vector<float> data_d(size_xy);
							pBandin->RasterIO(GF_Read, 0, 0, DSin[v].GetRasterXSize(), DSin[v].GetRasterYSize(), &(data_d[0]), DSin[v].GetRasterXSize(), DSin[v].GetRasterYSize(), GDT_Float32, 0, 0);

							switch (v)
							{
							case D_20CRV3_TMIN:
							{
								ASSERT(data[CR_TMIN].size() == data_d.size());
								for (size_t xy = 0; xy < data_d.size(); xy++)
								{
									data[CR_TMIN][xy_out(xy)] = data_d[xy] - 273.15;//K --> C
								}
								break;
							}
							case D_20CRV3_TMAX:
							{
								ASSERT(data[CR_TMAX].size() == data_d.size());
								for (size_t xy = 0; xy < data_d.size(); xy++)
								{
									if (sflx[xy] == 1)
										data[CR_TMAX][xy_out(xy)] = data_d[xy] - 273.15;//K --> C
								}
								break;
							}
							case D_20CRV3_PRCP:
							{
								ASSERT(data[CR_PRCP].size() == data_d.size());
								for (size_t xy = 0; xy < data_d.size(); xy++)
								{
									if (sflx[xy] == 1)
										data[CR_PRCP][xy_out(xy)] = data_d[xy]*8;//3 hours prcp --> daily (mm)
								}

								break;
							}
							case D_20CRV3_RELH:
							{
								ASSERT(data[CR_RELH].size() == data_d.size());
								//vector<float> Tair_h(DSin[v].GetRasterXSize() * DSin[v].GetRasterYSize());

								//GDALRasterBand* pBandinTmin = DSin[H_20CRV3_TMIN].GetRasterBand(h);
								//pBandinTair->RasterIO(GF_Read, 0, 0, DSin[H_ERA5_TAIR].GetRasterXSize(), DSin[H_ERA5_TAIR].GetRasterYSize(), &(Tair_h[0]), DSin[H_ERA5_TAIR].GetRasterXSize(), DSin[H_ERA5_TAIR].GetRasterYSize(), GDT_Float32, 0, 0);

								for (size_t xy = 0; xy < data_d.size(); xy++)
								{
									if (sflx[xy] == 1 && data_d[xy]>=0 && data_d[xy]<=100)
									{
										data[CR_RELH][xy_out(xy)] = data_d[xy];
										double Pv = WBSF::Hr2Pv(data[CR_TMIN][xy_out(xy)], data[CR_TMAX][xy_out(xy)], data[CR_RELH][xy_out(xy)]);
										data[CR_TDEW][xy_out(xy)] = WBSF::Pv2Td(Pv/1000.0);
									}
								}


								break;
							}
							case D_20CRV3_WNDU:
							{
								ASSERT(data[CR_WNDS].size() == data_d.size());

								for (size_t xy = 0; xy < data_d.size(); xy++)
								{
									if (sflx[xy] == 1)
									{
										data[CR_WNDS][xy_out(xy)] = data_d[xy] * 3600 / 1000;//m/s --> km/h;
									}
								}


								break;
							}
							case D_20CRV3_WNDV:
							{
								ASSERT(data[CR_WNDD].size() == data_d.size());
								for (size_t xy = 0; xy < data_d.size(); xy++)
								{
									if (sflx[xy] == 1)
									{
										data[CR_WNDD][xy_out(xy)] = data_d[xy] * 3600 / 1000;//m/s --> km/h;
										double u = data[CR_WNDS][xy_out(xy)];
										double v = data[CR_WNDD][xy_out(xy)];
										data[CR_WNDS][xy_out(xy)] = sqrt(Square(u) + Square(v));//approximation of wind speed
										data[CR_WNDD][xy_out(xy)] = GetWindDirection(u, v);//approximation of wind direction
									}
								}

								break;
							}

							case D_20CRV3_GEOPOTENTIAL:
							{
								ASSERT(data[CR_GHGT].size() == data_d.size());
								for (size_t xy = 0; xy < data_d.size(); xy++)
								{
									if (sflx[xy] == 1)
										data[CR_GHGT][xy_out(xy)] = (data_d[xy]);
								}

								break;
							}
							default: ASSERT(false);

							}

							msg += callback.StepIt(0);

						}//is open
					}//for all variable

					//callback.PopTask();


					//for (size_t xy = 0; xy < data_u.size(); xy++)
					//{
					//	//do not take directional wind speed but radial wind speed.
					//	ASSERT(data_u[xy].size() == data_v[xy].size());
					//	for (size_t h = 0; h < data_u[xy].size(); h++)
					//	{
					//		data[CR_WNDS][xy] += sqrt(Square(data_u[xy](h)) + Square(data_v[xy](h))) / data_u[xy].size();
					//		data[CR_WNDD][xy] += GetWindDirection(data_u[xy](h), data_v[xy](h)) / data_u[xy].size();//approximation of wind direction
					//	}

					//	//double u = data_u[xy][MEAN];
					//	//double v = data_v[xy][MEAN];
					//	//data[ERA5_WNDS][xy] = sqrt(Square(u) + Square(v));

					//	//data[ERA5_WNDS][xy] = sqrt(Square(u) + Square(v));
					//	//data[ERA5_WNDD][xy] = GetWindDirection(u, v);//approximation of wind direction

					//	data[CR_TMIN][xy] = data_T[xy][LOWEST];
					//	data[CR_TMAX][xy] = data_T[xy][HIGHEST];
					//}

					for (size_t v = 0; v < NB_20CRV3_VARS; v++)
					{

						GDALRasterBand* pBandout = DSout.GetRasterBand(v);

						pBandout->RasterIO(GF_Write, 0, 0, DSout.GetRasterXSize(), DSout.GetRasterYSize(), &(data[v][0]), DSout.GetRasterXSize(), DSout.GetRasterYSize(), GDT_Float32, 0, 0);

						pBandout->SetDescription(META_DATA_20CRV3[v][M_DESC]);
						pBandout->SetMetadataItem("GRIB_COMMENT", META_DATA_20CRV3[v][M_COMMENT]);
						pBandout->SetMetadataItem("GRIB_ELEMENT", META_DATA_20CRV3[v][M_ELEMENT]);
						pBandout->SetMetadataItem("GRIB_SHORT_NAME", META_DATA_20CRV3[v][M_SHORT_NAME]);
						pBandout->SetMetadataItem("GRIB_UNIT", META_DATA_20CRV3[v][M_UNIT]);
						DSout->FlushCache();
					}


					DSout.Close();
				}//if open output

				if (msg)
				{
					//copy the file to fully use compression with GDAL_translate
					string gdal_data_path = GetApplicationPath() + "External\\gdal-data";
					string projlib_path = GetApplicationPath() + "External\\projlib";
					string plugin_path = GetApplicationPath() + "External\\gdalplugins";

					string option = "--config GDAL_DATA \"" + gdal_data_path + "\" --config PROJ_LIB \"" + projlib_path + "\" --config GDAL_DRIVER_PATH \"" + plugin_path +"\"";
					string argument = "-ot Float32 -stats -co COMPRESS=LZW -co TILED=YES -co BLOCKXSIZE=64 -co BLOCKYSIZE=64";//-co PREDICTOR=3 
					string command = "\"" + GetApplicationPath() + "External\\gdal_translate.exe\" " + option + " " + argument + " \"" + output_file_path + "2" + "\" \"" + output_file_path + "\"";
					msg += WinExecWait(command);
					msg += RemoveFile(output_file_path + "2");
					RemoveFile(output_file_path + "2.aux.xml");
				}

			}//if msg

			msg += callback.StepIt();




		}//for all days

		callback.PopTask();

		return msg;
	}
	//canada_0p25grid
	//ERMsg C20CRv3::CreateDailyGeotiff(const string& title, int year, CCallback& callback)const
	//{
	//	ERMsg msg;

	//	size_t nb_bands = 0;
	//	array<CGDALDatasetEx, NB_20CRV3_VARS> DSin;
	//	for (size_t v = 0; v < DSin.size(); v++)
	//	{
	//		string inputFilePath = FormatA("%sNetCDF\\%s.%4d.tif", m_workingDir.c_str(),  year, title.c_str());
	//		if (v == CR_GHGT)
	//			inputFilePath = FormatA("%s%s.tif", m_workingDir.c_str(), title.c_str());

	//		if (FileExists(inputFilePath))
	//		{
	//			msg += DSin[v].OpenInputImage(inputFilePath);
	//			nb_bands++;
	//		}
	//	}


	//	if (msg)
	//	{
	//		size_t nb_days = DSin[0].GetRasterCount();
	//		callback.PushTask("Create ERA5 for year " + ToString(year) + ": " + ToString(nb_days) + " days", nb_days);

	//		float no_data = 9999;// GetDefaultNoData(GDT_Int16);
	//		CBaseOptions options;
	//		DSin[0].UpdateOption(options);


	//		for (size_t d = 0; d < nb_days && msg; d++)
	//		{
	//			options.m_nbBands = nb_bands;
	//			options.m_extents.m_xBlockSize = 256;
	//			options.m_extents.m_yBlockSize = 256;
	//			options.m_outputType = GDT_Float32;
	//			options.m_dstNodata = no_data;
	//			options.m_bOverwrite = true;
	//			options.m_bComputeStats = true;
	//			options.m_createOptions.push_back("COMPRESS=LZW");
	//			options.m_createOptions.push_back("PREDICTOR=3");
	//			options.m_createOptions.push_back("TILED=YES");
	//			options.m_createOptions.push_back("BLOCKXSIZE=256");
	//			options.m_createOptions.push_back("BLOCKYSIZE=256");

	//			CJDayRef TRef(year, d);

	//			string output_file_path = FormatA("%s%d\\%02d\\ERA5_%d%02d%02d_%s.tif", m_workingDir.c_str(), TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, title.c_str());
	//			msg = CreateMultipleDir(GetPath(output_file_path));

	//			CGDALDatasetEx DSout;
	//			msg += DSout.CreateImage(output_file_path + "2", options);
	//			if (msg)
	//			{


	//				for (size_t v = 0, vv = 0; v < DSin.size(); v++)
	//				{
	//					GDALRasterBand* pBandout = DSout.GetRasterBand(vv);

	//					if (DSin[v].IsOpen())
	//					{
	//						ASSERT(v == CR_GHGT || DSin[0].GetRasterCount() == DSin[v].GetRasterCount());
	//						GDALRasterBand* pBandin = DSin[v].GetRasterBand(v == CR_GHGT ? 0 : d);

	//						ASSERT(DSin[v].GetRasterXSize() == DSout.GetRasterXSize());
	//						ASSERT(DSin[v].GetRasterYSize() == DSout.GetRasterYSize() || DSin[v].GetRasterYSize() == DSout.GetRasterYSize() - 1 || DSin[v].GetRasterYSize() == DSout.GetRasterYSize() + 1);


	//						float no_data_in = DSin[v].GetNoData(0);
	//						vector<float> data(DSin[v].GetRasterXSize() * DSin[v].GetRasterYSize());
	//						pBandin->RasterIO(GF_Read, 0, 0, DSin[v].GetRasterXSize(), DSin[v].GetRasterYSize(), &(data[0]), DSin[v].GetRasterXSize(), DSin[v].GetRasterYSize(), GDT_Float32, 0, 0);


	//						if (DSin[v].GetRasterYSize() == DSout.GetRasterYSize())
	//						{
	//							pBandout->RasterIO(GF_Write, 0, 0, DSout.GetRasterXSize(), DSout.GetRasterYSize(), &(data[0]), DSout.GetRasterXSize(), DSout.GetRasterYSize(), GDT_Float32, 0, 0);
	//						}
	//						else
	//						{
	//							pBandout->RasterIO(GF_Write, 0, 0, DSout.GetRasterXSize(), DSout.GetRasterYSize(), &(data[0]), DSin[v].GetRasterXSize(), DSin[v].GetRasterYSize(), GDT_Float32, 0, 0);
	//						}


	//						pBandout->SetDescription(META_DATA_20CRV3[v][M_DESC]);
	//						pBandout->SetMetadataItem("GRIB_COMMENT", META_DATA_20CRV3[v][M_COMMENT]);
	//						pBandout->SetMetadataItem("GRIB_ELEMENT", META_DATA_20CRV3[v][M_ELEMENT]);
	//						pBandout->SetMetadataItem("GRIB_SHORT_NAME", META_DATA_20CRV3[v][M_SHORT_NAME]);
	//						pBandout->SetMetadataItem("GRIB_UNIT", META_DATA_20CRV3[v][M_UNIT]);
	//						DSout->FlushCache();


	//						vv++;
	//					}
	//					/*else
	//					{
	//						float no_data = -999;
	//						pBandout->RasterIO(GF_Write, 0, 0, DSout.GetRasterXSize(), DSout.GetRasterYSize(), &no_data, 1, 1, GDT_Float32, 0, 0);
	//					}*/






	//				}//for all variables

	//				DSout.Close();


	//				if (msg)
	//				{
	//					//copy the file to fully use compression with GDAL_translate
	//					string gdal_data_path = GetApplicationPath() + "External\\gdal-data";
	//					string projlib_path = GetApplicationPath() + "External\\projlib";

	//					//-stats : do not include stat to avoid the creation of the xml file
	//					string option = "--config GDAL_DATA \"" + gdal_data_path + "\" --config PROJ_LIB \"" + projlib_path + "\"";
	//					string argument = "-unscale -ot Float32 -stats -co COMPRESS=LZW -co PREDICTOR=3 -co TILED=YES -co BLOCKXSIZE=256 -co BLOCKYSIZE=256";
	//					string command = "\"" + GetApplicationPath() + "External\\gdal_translate.exe\" " + option + " " + argument + " \"" + output_file_path + "2" + "\" \"" + output_file_path + "\"";
	//					msg += WinExecWait(command);
	//					msg += RemoveFile(output_file_path + "2");
	//					msg += RemoveFile(output_file_path + "2.aux.xml");
	//				}

	//			}//if msg

	//			msg += callback.StepIt();
	//		}//for all days

	//		callback.PopTask();
	//	}//if dataset open




	//	return msg;
	//}


	//****************************************************************************************************

	//string C20CRv3::GetOutputFilePath(const string& filePath)const
	//{
	//	CTRef TRef = GetRemoteTRef(filePath);
	//	string title = GetFileTitle(filePath);
	//	return FormatA("%s%d\\%02d\\%02d\\%s.grib2", m_workingDir.c_str(), TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, title.c_str());
	//}


	//ERMsg C20CRv3::CreateDailyGeotiff(StringVector& filesList, const std::string& file_path_out, CCallback& callback)const
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


	ERMsg C20CRv3::GetGribsList(CTPeriod p, CGribsMap& gribsList, CCallback& callback)
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

	CTRef C20CRv3::GetLocalTRef(string filePath)
	{
		CTRef TRef;

		string title = GetFileTitle(filePath);
		int year = WBSF::as<int>(title.substr(7, 4));
		size_t m = WBSF::as<int>(title.substr(11, 2)) - 1;
		size_t d = WBSF::as<int>(title.substr(13, 2)) - 1;
		TRef = CTRef(year, m, d);

		return TRef;
	}



	ERMsg C20CRv3::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;


		////get all file in the directory 
		//StringVector fileList;
		//int firstYear = as<int>(FIRST_YEAR);
		//int lastYear = as<int>(LAST_YEAR);
		//size_t nbYears = lastYear - firstYear + 1;
		//string workingDir = GetDir(WORKING_DIR);
		//CGeoRect rect = as<CGeoRect>(BOUNDING_BOX);
		//rect.SetPrjID(PRJ_WGS_84);

		//string strSearch = workingDir + "Daily\\Reanalysis_1dd_v1.2_p1d.*";
		//StringVector fileListTmp = GetFilesList(strSearch);
		//sort(fileListTmp.begin(), fileListTmp.end());


		//for (size_t i = 0; i < fileListTmp.size() && msg; i++)
		//{
		//	string ext = fileListTmp[i].substr(fileListTmp[i].size() - 6);
		//	ASSERT(ext.length() == 6);
		//	int year = ToInt(ext.substr(0, 4));
		//	size_t m = ToSizeT(ext.substr(4, 2)) - 1;
		//	if (year >= firstYear && year <= lastYear)
		//	{
		//		fileList.push_back(fileListTmp[i]);
		//		if (!m_firstTRef.IsInit())
		//			m_firstTRef = CTRef(year, m);

		//		ASSERT((m_firstTRef + fileList.size() - 1).GetYear() == year && (m_firstTRef + fileList.size() - 1).GetMonth() == m);
		//	}
		//}

		//if (msg && !fileList.empty())
		//{
		//	_setmaxstdio(1024);
		//	m_files.resize(fileList.size());

		//	//remove file 
		//	for (size_t i = 0; i < fileList.size() && msg; i++)
		//		msg += m_files[i].open(fileList[i], ios::in | ios::binary);

		//	stationList.reserve(180 * 360);
		//	for (size_t i = 0; i < 180; i++)
		//	{
		//		for (size_t j = 0; j < 360; j++)
		//		{
		//			double lat = 89.5 - i;
		//			double lon = 0.5 + j;
		//			if (lon > 180)
		//				lon -= 360;

		//			CGeoPoint pt(lon, lat, PRJ_WGS_84);

		//			if (rect.IsEmpty() || rect.IsInside(pt))
		//			{
		//				string title = FormatA("%03d_%03d", i + 1, j + 1);
		//				stationList.push_back(title);
		//			}

		//			msg += callback.StepIt(0);
		//		}
		//	}
		//}



		return msg;
	}




	ERMsg C20CRv3::GetWeatherStation(const string& ID, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ASSERT(ID.length() == 7);
		ERMsg msg;

		//size_t i = WBSF::as<size_t>(ID.substr(0, 3)) - 1;
		//size_t j = WBSF::as<size_t>(ID.substr(4, 3)) - 1;
		//double lat = 89.5 - i;
		//double lon = 0.5 + j;
		//if (lon > 180)
		//	lon -= 360;

		//((CLocation&)station) = CLocation(ID, ID, lat, lon, 0);


		//int firstYear = m_firstTRef.GetYear();
		//int lastYear = (m_firstTRef + m_files.size()).GetYear();
		//size_t nbYears = lastYear - firstYear + 1;
		//station.CreateYears(firstYear, nbYears);

		//for (size_t mm = 0; mm < m_files.size() && msg; mm++)
		//{
		//	CTRef TRef = m_firstTRef + mm;
		//	TRef.Transform(CTM(CTM::DAILY));
		//	size_t nbDays = TRef.GetNbDayPerMonth();
		//	for (size_t d = 0; d < nbDays && msg; d++, TRef++)
		//	{
		//		m_files[mm].seekg(1440 + (d * 180 * 360 + (i * 360 + j)) * 4);

		//		float prcp = 0;
		//		m_files[mm].read((char*)(&prcp), sizeof(prcp));
		//		//prcp = ReverseFloat(prcp);

		//		if (isnan(prcp) || !isfinite(prcp))
		//			prcp = -999;

		//		if (prcp > 0 && prcp < 0.00001f)
		//			prcp = 0;

		//		if (prcp < -999)
		//			prcp = -999;

		//		ASSERT(prcp == -999 || prcp >= 0);

		//		station[TRef].SetStat(H_PRCP, prcp);
		//	}
		//}


		//if (msg && station.HaveData())
		//	msg = station.IsValid();

		return msg;
	}



}