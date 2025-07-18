#include "StdAfx.h"
#include "ERA5.h"
#include "Basic/WeatherStation.h"
#include "Basic/CallcURL.h"
#include "Geomatic/ShapeFileBase.h"
#include "Geomatic/TimeZones.h"
#include "Geomatic/SfcGribsDatabase.h"
#include "TaskFactory.h"
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
		m_workingDir(workingDir),
		m_show_download(false),
		m_bLand(false)
	{
	}

	CERA5::~CERA5(void)
	{
	}

	//ERA5 is available on Google Drive: 
	//https://console.cloud.google.com/storage/browser/gcp-public-data-arco-era5/raw/date-variable-single_level/1942/01/01/2m_temperature?pageState=(%22StorageObjectListTable%22:(%22f%22:%22%255B%255D%22))&hl=fr&inv=1&invt=AbincQ

	enum TERA5Var { ERA5_TMIN, ERA5_TMAX, ERA5_PRCP, ERA5_TDEW, ERA5_RELH, ERA5_WNDS, ERA5_WNDD, ERA5_SRAD, ERA5_PRES, ERA5_SNOW, ERA5_SNDH, ERA5_SWE, ERA5_GHGT, NB_ERA5_VARS };

	static const  size_t VARS_ERA5[NB_ERA5_VARS] = { H_TMIN, H_TMAX, H_PRCP, H_TDEW, H_RELH, H_WNDS, H_WNDD, H_SRAD, H_PRES, H_SNOW, H_SNDH, H_SWE, H_GHGT };
	static const char* SUB_DIR[NB_ERA5_VARS] = { "tmin","tmax", "prcptot", "tdmean", "rhmean", "wsmean", "wdmean", "ssrmean","spmean", "sftot", "sdmean", "swemean", "z" };
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


		CTRef today = CTRef::GetCurrentTRef(CTM(CTM::DAILY));


		size_t nb_years = m_last_year - m_first_year + 1;
		callback.PushTask("Create ERA5: " + ToString(nb_years) + " year", nb_years);

		for (size_t y = 0; y < nb_years && msg; y++)
		{
			int year = int(m_first_year + y);

			callback.PushTask("Create ERA5 for year " + to_string(year) + ": " + ToString(GetNbDaysPerYear(year)) + " days", GetNbDaysPerYear(year));
			for (size_t m = 0; m < 12 && msg; m++)
			{
				for (size_t d = 0; d < GetNbDayPerMonth(year, m) && msg; d++)
				{
					CTRef TRef(year, m, d);

					if (TRef < today - 8)//8 lag days 
					{
						string output_file_path = FormatA("%s%d\\%02d\\ERA5_%d%02d%02d.tif", m_workingDir.c_str(), TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1);
						if (!FileExists(output_file_path))
						{
							msg += DownloadERA5GoogleCloud(TRef, callback);
							if (msg)
								msg += CreateDailyGeotiff(TRef, callback);
						}
					}

					msg += callback.StepIt();

				}
			}

			callback.PopTask();
			msg += callback.StepIt();
		}

		callback.PopTask();

		return msg;
	}


	enum TERA5VarH { H_ERA5_TAIR, H_ERA5_PRCP, H_ERA5_TDEW, H_ERA5_WNDU, H_ERA5_WNDV, H_ERA5_SRAD, H_ERA5_PRES, H_ERA5_SNOWFALL, H_ERA5_SWE, H_ERA5_SNOW_DENSITY, H_ERA5_GEOPOTENTIAL, NB_ERA5_VARS_HOURLY };
	//enum TERA5VarH { H_ERA5_SRAD, NB_ERA5_VARS_HOURLY };

	//static const array<string, NB_ERA5_VARS_HOURLY> ERA5_NAME_H =
	//{
	//	"surface_net_solar_radiation"
	//};

	static const array<string, NB_ERA5_VARS_HOURLY> ERA5_NAME_H =
	{
		"2m_temperature", "total_precipitation", "2m_dewpoint_temperature",
		"10m_u_component_of_wind", "10m_v_component_of_wind",
		"mean_surface_downward_short_wave_radiation_flux", "surface_pressure",
		"snowfall","snow_depth", "snow_density", "geopotential"
	};

	//static const array<string, NB_ERA5_VARS_HOURLY> ERA5_NAME_H =
	//{
	//	"2m_temperature", "total_precipitation", "2m_dewpoint_temperature",
	//	"10m_u_component_of_wind", "10m_v_component_of_wind",
	//	"surface_net_solar_radiation", "surface_pressure",
	//	"snowfall","snow_depth", "snow_density", "geopotential"
	//};


	//mean_surface_downward_short_wave_radiation_flux
	//"Surface solar radiation downwards"
	//"surface_net_solar_radiation"


	ERMsg CERA5::DownloadERA5(CTRef TRef, CCallback& callback)const
	{
		ERMsg msg;

		bool already_downloaded = true;
		for (size_t v = 0; v < NB_ERA5_VARS_HOURLY && already_downloaded; v++)
		{
			string inputFilePath = FormatA("%s%d\\%02d\\%02d\\ERA5_%d%02d%02d_%s.grb", m_workingDir.c_str(), TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, ERA5_NAME_H[v].c_str());

			if (!FileExists(inputFilePath))
				already_downloaded = false;
		}

		if (already_downloaded)
			return msg;


		string all_variables;
		for (auto it = ERA5_NAME_H.begin(); it != ERA5_NAME_H.end(); it++)
		{
			all_variables += all_variables.empty() ? "" : " ";
			all_variables += *it;
		}

		string box_output("");//empty string by default for sprintf
		string box_option("");//empty string by default for sprintf
		if (round(m_bounding_box.m_yMin) != -90 || round(m_bounding_box.m_xMin) != -180 || round(m_bounding_box.m_yMax) != 90 || round(m_bounding_box.m_xMax) != 180)
		{
			box_option = WBSF::FormatA(" --area %lf %lf %lf %lf", m_bounding_box.m_yMax, m_bounding_box.m_xMin, m_bounding_box.m_yMin, m_bounding_box.m_xMax);


			char a1 = m_bounding_box.m_xMin < 0 ? 'W' : 'E';
			char a2 = m_bounding_box.m_xMax < 0 ? 'W' : 'E';
			char a3 = m_bounding_box.m_yMin < 0 ? 'S' : 'N';
			char a4 = m_bounding_box.m_yMax < 0 ? 'S' : 'N';
			box_output = WBSF::FormatA("_%.0lf%c-%.0lf%c_%.0lf%c-%.0lf%c", abs(m_bounding_box.m_xMin), a1, abs(m_bounding_box.m_xMax), a2, abs(m_bounding_box.m_yMin), a3, abs(m_bounding_box.m_yMax), a4);
		}

		callback.PushTask("Download ERA5 for day " + TRef.GetFormatedString(), NOT_INIT);

		string output_filepath_tmp = FormatA("%s%d\\%02d\\%02d\\ERA5", m_workingDir.c_str(), TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1);
		msg = CreateMultipleDir(GetPath(output_filepath_tmp));

		if (msg)
		{

			//C:\Users\Remi\AppData\Local\Packages\PythonSoftwareFoundation.Python.3.12_qbz5n2kfra8p0\LocalCache\local-packages\Python312\Scripts
			//string exe_path = "C:\\Users\\tigro\\AppData\\Local\\Programs\\Python\\Python312\\Scripts\\";


			//if (m_bounding_box.m_xMax != -90 || m_bounding_box.m_yMin != -180 || m_bounding_box.m_xMin != 90 || m_bounding_box.m_yMax != 180)
				//box_option = WBSF::FormatA(" --area %lf %lf %lf %lf", m_bounding_box.m_yMax, m_bounding_box.m_xMin, m_bounding_box.m_yMin, m_bounding_box.m_xMax);

			string date = " --startyear " + to_string(TRef.GetYear()) + " --months " + to_string(TRef.GetMonth() + 1) + " --day " + to_string(TRef.GetDay() + 1);
			//if (TRef.GetYear() < 1979)
				//date += " --prelimbe";

			string land_option;
			if (m_bLand)
				land_option = " --land";

			//Note here that the path of the python script where era5cli.exe is installed must be in the user path
			string argument = "hourly --overwrite" + land_option + " --variables " + all_variables + date + box_option + " --levels surface --threads 1 --format grib --outputprefix \"" + output_filepath_tmp + "\"";
			string command = "era5cli.exe " + argument;

			//char Buffer[MAX_PATH * 10] = { 0 };
			//DWORD rep = GetEnvironmentVariableA("path", Buffer, MAX_PATH * 10);


			msg += WinExecWait(command, GetPath(output_filepath_tmp), m_show_download ? SW_SHOW : SW_HIDE);

			//if (msg)
			//{
			//	//verify if all files are downloaded
			//	bool not_downloaded = false;
			//	for (size_t v = 0; v < NB_ERA5_VARS_HOURLY; v++)
			//	{
			//		string output_filepath1 = output_filepath_tmp + FormatA("_%s_%04d-%02d_hourly", ERA5_NAME_H[v].c_str(), TRef.GetYear(), TRef.GetMonth() + 1) + box_output + ".grb";
			//			//FormatA("%s%d\\%02d\\%02d\\ERA5_%s_%04d-%02d_hourly%s.grb", m_workingDir.c_str(), TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, ERA5_NAME_H[v].c_str(), TRef.GetYear(), TRef.GetMonth() + 1, box_output.c_str());

			//		if (!FileExists(output_filepath1))
			//			not_downloaded = true;
			//	}

			//	if (not_downloaded)
			//	{
			//		callback.AddMessage("Server problem... wait 10 mintues");
			//		//wait 10 minutes to let the server recovery
			//		msg = WaitServer(10 * 60, callback);
			//		if (msg)//re-try after 10 minutes
			//			msg = WinExecWait(command, GetPath(output_filepath_tmp), m_show_download ? SW_SHOW : SW_HIDE);
			//	}
			//}

		}



		if (msg)
		{
			/*string box;
			if (m_bounding_box.m_xMax != -90 || m_bounding_box.m_yMin != -180 || m_bounding_box.m_xMin != 90 || m_bounding_box.m_yMax != 180)
			{
				char a1 = m_bounding_box.m_xMin < 0 ? 'W' : 'E';
				char a2 = m_bounding_box.m_xMax < 0 ? 'W' : 'E';
				char a3 = m_bounding_box.m_yMin < 0 ? 'S' : 'N';
				char a4 = m_bounding_box.m_yMax < 0 ? 'S' : 'N';
				box = WBSF::FormatA("%.0lf%c-%.0lf%c_%.0lf%c-%.0lf%c", abs(m_bounding_box.m_xMin), a1, abs(m_bounding_box.m_xMax), a2, abs(m_bounding_box.m_yMin), a3, abs(m_bounding_box.m_yMax), a4);
			}*/

			for (size_t v = 0; v < NB_ERA5_VARS_HOURLY && msg; v++)
			{
				//string output_filepath1 = output_filepath_tmp + "_" + ERA5_NAME_H[v] + "_" + to_string(TRef.GetYear()) + "_hourly" + box + ".grb";
				//string output_filepath1 = FormatA("%s%d\\%02d\\%02d\\ERA5_%s_%04d-%02d_hourly%s.grb", m_workingDir.c_str(), TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, ERA5_NAME_H[v].c_str(), TRef.GetYear(), TRef.GetMonth() + 1, box_output.c_str());
				string output_filepath1 = output_filepath_tmp + FormatA("_%s_%04d-%02d_hourly", ERA5_NAME_H[v].c_str(), TRef.GetYear(), TRef.GetMonth() + 1) + box_output + ".grb";

				if (!FileExists(output_filepath1))
					msg.ajoute("File does't exist: " + output_filepath1);

				if (msg)
				{
					string output_filepath2 = FormatA("%s%d\\%02d\\%02d\\ERA5_%d%02d%02d_%s.grb", m_workingDir.c_str(), TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, ERA5_NAME_H[v].c_str());
					//msg = CreateMultipleDir(GetPath(output_filepath2));
					if (FileExists(output_filepath2))
						msg += RemoveFile(output_filepath2);

					if (msg)
						msg += WBSF::RenameFile(output_filepath1, output_filepath2);
				}
			}

		}


		callback.PopTask();

		return msg;
	}

	ERMsg CERA5::DownloadERA5GoogleCloud(CTRef TRef, CCallback& callback)const
	{
		ERMsg msg;

		string output_path = FormatA("%s%d\\%02d\\%02d\\", m_workingDir.c_str(), TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1);
		msg = CreateMultipleDir(output_path);


		size_t to_download = 0;
		for (size_t v = 0; v < ERA5_NAME_H.size(); v++)
		{
			string filepath = FormatA("%sERA5_%d%02d%02d_%s.nc", output_path.c_str(), TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, ERA5_NAME_H[v].c_str());
			if (!FileExists(filepath))
				to_download++;
		}

		if (to_download == 0)
			return msg;



		callback.PushTask("Download ERA5 for day " + TRef.GetFormatedString(), to_download);




		if (msg)
		{
			CCallcURL cURL;

			for (size_t v = 0; v < ERA5_NAME_H.size(); v++)
			{
				string URL = FormatA("https://storage.googleapis.com/gcp-public-data-arco-era5/raw/date-variable-single_level/%d/%02d/%02d/%s/surface.nc", TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, ERA5_NAME_H[v].c_str());
				string filepath = FormatA("%sERA5_%04d%02d%02d_%s.nc", output_path.c_str(), TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, ERA5_NAME_H[v].c_str());
				if (!FileExists(filepath))
				{
					msg += cURL.copy_file(URL, filepath);
					msg += callback.StepIt();
				}
			}
		}

		callback.PopTask();

		return msg;
	}


	ERMsg CERA5::CreateDailyGeotiff(CTRef TRef, CCallback& callback)const
	{
		ERMsg msg;

		string output_file_path = FormatA("%s%d\\%02d\\ERA5_%d%02d%02d.tif", m_workingDir.c_str(), TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1);


		//bool convert_xy = false;
		array<CGDALDatasetEx, NB_ERA5_VARS_HOURLY> DSin;
		for (size_t v = 0; v < NB_ERA5_VARS_HOURLY; v++)
		{
			//try with .grb
			/*string inputFilePath = FormatA("%s%d\\%02d\\%02d\\ERA5_%d%02d%02d_%s.grb", m_workingDir.c_str(), TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, ERA5_NAME_H[v].c_str());

			if (FileExists(inputFilePath))
			{
				msg += DSin[v].OpenInputImage(inputFilePath);
			}
			else
			{*/

			//try with .nc
			string input_filepath = FormatA("%s%d\\%02d\\%02d\\ERA5_%d%02d%02d_%s.nc", m_workingDir.c_str(), TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, ERA5_NAME_H[v].c_str());

			if (FileExists(input_filepath))
			{
				//	convert_xy = true;//assuming all .grb or .nc
				msg += DSin[v].OpenInputImage(input_filepath);
				if (DSin[v].IsOpen() && DSin[v].GetRasterCount() != 24)
				{
					msg.ajoute("File don't have 24 hours:" + input_filepath);
				}

			}
			//	}
		}


		if (msg)
		{
			callback.PushTask("Create ERA5 for day " + TRef.GetFormatedString(), NB_ERA5_VARS_HOURLY * 24);

			CBaseOptions options;
			DSin[0].UpdateOption(options);

			options.m_nbBands = NB_ERA5_VARS;
			options.m_extents.m_xMin -= 180.0;//convert  [0,360] to [-180,180]
			options.m_extents.m_xMax -= 180.0;//convert  [0,360] to [-180,180]
			options.m_extents.m_xBlockSize = 256;
			options.m_extents.m_yBlockSize = 256;
			options.m_outputType = GDT_Float32;
			options.m_dstNodata = (float)GetDefaultNoData(GDT_Float32);
			options.m_bOverwrite = true;
			options.m_bComputeStats = true;
			options.m_createOptions.push_back("COMPRESS=LZW");
			options.m_createOptions.push_back("PREDICTOR=3");
			options.m_createOptions.push_back("TILED=YES");
			options.m_createOptions.push_back("BLOCKXSIZE=256");
			options.m_createOptions.push_back("BLOCKYSIZE=256");


			msg = CreateMultipleDir(GetPath(output_file_path));

			CGDALDatasetEx DSout;
			msg += DSout.CreateImage(output_file_path + "2", options);
			if (msg)
			{
				//compute variable: Tmin, Tmax, wind speed and direction and snow depth 
				size_t size_xy = DSin[0].GetRasterXSize() * DSin[0].GetRasterYSize();
				array<vector<float>, NB_ERA5_VARS> data;
				vector<CStatistic> data_T(size_xy);
				vector<CStatisticEx> data_u(size_xy);
				vector<CStatisticEx> data_v(size_xy);
				for (size_t v = 0; v < NB_ERA5_VARS; v++)
				{
					data[v].resize(size_xy);
				}


				for (size_t v = 0; v < NB_ERA5_VARS_HOURLY && msg; v++)
				{
					if (DSin[v].IsOpen())
					{
						ASSERT(DSin[v].GetRasterCount() == 24);


						for (size_t h = 0; h < DSin[v].GetRasterCount() && msg; h++)
						{
							GDALRasterBand* pBandin = DSin[v].GetRasterBand(h);
							ASSERT(DSin[v].GetRasterXSize() * DSin[v].GetRasterYSize() == size_xy);

							//float no_data_in = DSin[v].GetNoData(0);
							__int32 no_data = (__int32)DSin[v].GetNoData(h);
							vector<__int32> data_h(size_xy);
							pBandin->RasterIO(GF_Read, 0, 0, DSin[v].GetRasterXSize(), DSin[v].GetRasterYSize(), &(data_h[0]), DSin[v].GetRasterXSize(), DSin[v].GetRasterYSize(), GDT_Int32, 0, 0);
							double scale = pBandin->GetScale();
							double offset = pBandin->GetOffset();


							switch (v)
							{
							case H_ERA5_TAIR:
							{
								ASSERT(data_T.size() == data_h.size());
								for (size_t xy = 0; xy < data_h.size(); xy++)
								{
									//data_T[xy] += data_h[xy] - 273.15;
									if (data_h[xy] != no_data)
										data_T[xy] += (data_h[xy] * scale + offset) - 273.15;
								}
								break;
							}
							case H_ERA5_PRCP:
							{
								ASSERT(data[ERA5_PRCP].size() == data_h.size());
								for (size_t xy = 0; xy < data_h.size(); xy++)
								{
									if (data_h[xy] != no_data)
										data[ERA5_PRCP][xy] += (data_h[xy] * scale + offset) * 1000;//m --> mm
								}

								break;
							}
							case H_ERA5_TDEW:
							{
								ASSERT(data[ERA5_TDEW].size() == data_h.size());

								//Reload hourly values
								vector<__int32> Tair_h(DSin[v].GetRasterXSize() * DSin[v].GetRasterYSize());
								GDALRasterBand* pBandinTair = DSin[H_ERA5_TAIR].GetRasterBand(h);

								pBandinTair->RasterIO(GF_Read, 0, 0, DSin[H_ERA5_TAIR].GetRasterXSize(), DSin[H_ERA5_TAIR].GetRasterYSize(), &(Tair_h[0]), DSin[H_ERA5_TAIR].GetRasterXSize(), DSin[H_ERA5_TAIR].GetRasterYSize(), GDT_Int32, 0, 0);
								double Tscale = pBandinTair->GetScale();
								double Toffset = pBandinTair->GetOffset();

								for (size_t xy = 0; xy < data_h.size(); xy++)
								{
									if (Tair_h[xy] != no_data && data_h[xy] != no_data)
									{
										double Tair = (Tair_h[xy] * Tscale + Toffset) - 273.15;
										double Tdew = (data_h[xy] * scale + offset) - 273.15;
										double RH = WBSF::Td2Hr(Tair, Tdew);

										data[ERA5_TDEW][xy] += Tdew / 24;
										data[ERA5_RELH][xy] += RH / 24;
									}

									//data[ERA5_TDEW][xy] += ((data_h[xy] * scale + offset) - 273.15) / 24;
									//data[ERA5_RELH][xy] += WBSF::Td2Hr(((Tair_h[xy] * Tscale + Toffset) - 273.15), ((data_h[xy] * scale + offset) - 273.15)) / 24;
								}


								break;
							}
							case H_ERA5_WNDU:
							{
								ASSERT(data_u.size() == data_h.size());

								for (size_t xy = 0; xy < data_h.size(); xy++)
								{
									if (data_h[xy] != no_data)
										data_u[xy] += (data_h[xy] * scale + offset) * 3600 / 1000;//m/s --> km/h;
								}


								break;
							}
							case H_ERA5_WNDV:
							{
								ASSERT(data_v.size() == data_h.size());
								for (size_t xy = 0; xy < data_h.size(); xy++)
								{
									if (data_h[xy] != no_data)
										data_v[xy] += (data_h[xy] * scale + offset) * 3600 / 1000;//m/s --> km/h;
								}

								break;
							}
							case H_ERA5_SRAD:
							{
								double factor = (ERA5_NAME_H[H_ERA5_SRAD] == "surface_net_solar_radiation") ? 3600.0 : 1.0;


								ASSERT(data[ERA5_SRAD].size() == data_h.size());
								for (size_t xy = 0; xy < data_h.size(); xy++)
								{
									if (data_h[xy] != no_data)
										data[ERA5_SRAD][xy] += (data_h[xy] * scale + offset) / (factor * 24);
								}
								break;
							}
							case H_ERA5_PRES:
							{
								ASSERT(data[ERA5_PRES].size() == data_h.size());
								for (size_t xy = 0; xy < data_h.size(); xy++)
								{
									if (data_h[xy] != no_data)
										data[ERA5_PRES][xy] += ((data_h[xy] * scale + offset) / 100) / 24;//Pa --> hPa
								}
								break;
							}

							case H_ERA5_SNOWFALL:
							{
								ASSERT(data[ERA5_SNOW].size() == data_h.size());
								for (size_t xy = 0; xy < data_h.size(); xy++)
								{
									double v = (data_h[xy] * scale + offset) * 1000;//m --> mm
									ASSERT(v >= 0);
									if (data_h[xy] != no_data && v < 0.001)
										v = 0;

									if (data_h[xy] != no_data)
										data[ERA5_SNOW][xy] += v;

								}

								break;
							}
							case H_ERA5_SWE:
							{
								ASSERT(data[ERA5_SWE].size() == data_h.size());
								for (size_t xy = 0; xy < data_h.size(); xy++)
								{
									ASSERT((data_h[xy] * scale + offset) >= 0);

									if (data_h[xy] != no_data)
										data[ERA5_SWE][xy] += ((data_h[xy] * scale + offset) * 1000) / 24;//m --> mm
								}
								break;
							}
							case H_ERA5_SNOW_DENSITY:
							{
								vector<__int32> SWE_h(size_xy);

								GDALRasterBand* pBandinSWE = DSin[H_ERA5_SWE].GetRasterBand(h);
								pBandinSWE->RasterIO(GF_Read, 0, 0, DSin[H_ERA5_SWE].GetRasterXSize(), DSin[H_ERA5_SWE].GetRasterYSize(), &(SWE_h[0]), DSin[H_ERA5_SWE].GetRasterXSize(), DSin[H_ERA5_SWE].GetRasterYSize(), GDT_Int32, 0, 0);
								double SWEscale = pBandinSWE->GetScale();
								double SWEoffset = pBandinSWE->GetOffset();


								ASSERT(data[ERA5_SNDH].size() == data_h.size());
								for (size_t xy = 0; xy < data_h.size(); xy++)
								{
									ASSERT((data_h[xy] * scale + offset) >= 0);
									if (SWE_h[xy] != no_data && data_h[xy] != no_data)
									{
										double SWE = (SWE_h[xy] * SWEscale + SWEoffset) * 997;
										double SD = (data_h[xy] * scale + offset);
										data[ERA5_SNDH][xy] += ((SWE / SD) * 100) / 24;// m*kg/m�/ kg/m� --> cm
									}
								}

								break;
							}
							case H_ERA5_GEOPOTENTIAL:
							{
								ASSERT(data[ERA5_GHGT].size() == data_h.size());
								for (size_t xy = 0; xy < data_h.size(); xy++)
								{
									if (data_h[xy] != no_data)
										data[ERA5_GHGT][xy] += ((data_h[xy] * scale + offset) / 9.8) / 24;//Divide by acceleration at surface m�/s� --> m
								}

								break;
							}
							default: ASSERT(false);

							}

							msg += callback.StepIt();
						}//for all hours
					}//is open
				}//for all variable

				callback.PopTask();


				for (size_t xy = 0; xy < data_u.size(); xy++)
				{
					//do not take directional wind speed but radial wind speed.
					ASSERT(data_u[xy].size() == data_v[xy].size());
					for (size_t h = 0; h < data_u[xy].size(); h++)
					{
						data[ERA5_WNDS][xy] += sqrt(Square(data_u[xy](h)) + Square(data_v[xy](h))) / data_u[xy].size();
						data[ERA5_WNDD][xy] += GetWindDirection(data_u[xy](h), data_v[xy](h)) / data_u[xy].size();//approximation of wind direction
					}

					data[ERA5_TMIN][xy] = data_T[xy][LOWEST];
					data[ERA5_TMAX][xy] = data_T[xy][HIGHEST];
				}

				for (size_t v = 0; v < NB_ERA5_VARS; v++)
				{
					GDALRasterBand* pBandout = DSout.GetRasterBand(v);

					size_t size_x = DSout.GetRasterXSize();
					size_t size_y = DSout.GetRasterYSize();
					ASSERT(size_x* size_y== size_xy);

					//-180.1250000000000000,-90.1250000000000000 : 179.8750000000000000,90.1250000000000000
					//-0.1250000000000000,-90.1250000000000000 : 359.8750000000000000,90.1250000000000000
					//Covert [0, 360] to [-180, 180]
					vector<float> data_out(data[v].size());
					for (size_t y = 0; y < size_y; y++)
					{
						for (size_t x = 0; x < size_x; x++)
						{
							size_t xy_in = y * size_x + x;
							size_t xy_out = y * size_x + ((x+size_x/2)%size_x);
							data_out[xy_out] = data[v][xy_in];
						}
					}


					pBandout->RasterIO(GF_Write, 0, 0, DSout.GetRasterXSize(), DSout.GetRasterYSize(), &(data_out[0]), DSout.GetRasterXSize(), DSout.GetRasterYSize(), GDT_Float32, 0, 0);

					pBandout->SetDescription(ERA5_META_DATA[v][M_DESC]);
					pBandout->SetMetadataItem("GRIB_COMMENT", ERA5_META_DATA[v][M_COMMENT]);
					pBandout->SetMetadataItem("GRIB_ELEMENT", ERA5_META_DATA[v][M_ELEMENT]);
					pBandout->SetMetadataItem("GRIB_SHORT_NAME", ERA5_META_DATA[v][M_SHORT_NAME]);
					pBandout->SetMetadataItem("GRIB_UNIT", ERA5_META_DATA[v][M_UNIT]);
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

				//-stats : do not include stat to avoid the creation of the xml file
				string option = "--config GDAL_DATA \"" + gdal_data_path + "\" --config PROJ_LIB \"" + projlib_path + "\" --config GDAL_DRIVER_PATH \"" + plugin_path + "\"";
				string argument = "-unscale -ot Float32 -stats -co COMPRESS=LZW -co PREDICTOR=3 -co TILED=YES -co BLOCKXSIZE=256 -co BLOCKYSIZE=256";
				string command = "\"" + GetApplicationPath() + "External\\gdal_translate.exe\" " + option + " " + argument + " \"" + output_file_path + "2" + "\" \"" + output_file_path + "\"";
				msg += WinExecWait(command);
				msg += RemoveFile(output_file_path + "2");
				msg += RemoveFile(output_file_path + "2.aux.xml");
			}

		}//if msg

		return msg;
	}


	//canada_0p25grid
	//ERMsg CERA5::CreateDailyGeotiff(const string& title, int year, CCallback& callback)const
	//{
	//	ERMsg msg;

	//	size_t nb_bands = 0;
	//	array<CGDALDatasetEx, NB_ERA5_VARS> DSin;
	//	for (size_t v = 0; v < DSin.size(); v++)
	//	{
	//		string inputFilePath = FormatA("%s%s\\era5_%s_daily_%4d_%s.tif", m_workingDir.c_str(), SUB_DIR[v], SUB_DIR[v], year, title.c_str());
	//		if (v == ERA5_GHGT)
	//			inputFilePath = FormatA("%s%s\\era5_%s_%s.tif", m_workingDir.c_str(), SUB_DIR[v], SUB_DIR[v], title.c_str());

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
	//						ASSERT(v == ERA5_GHGT || DSin[0].GetRasterCount() == DSin[v].GetRasterCount());
	//						GDALRasterBand* pBandin = DSin[v].GetRasterBand(v == ERA5_GHGT ? 0 : d);

	//						ASSERT(DSin[v].GetRasterXSize() == DSout.GetRasterXSize());
	//						ASSERT(DSin[v].GetRasterYSize() == DSout.GetRasterYSize() || DSin[v].GetRasterYSize() == DSout.GetRasterYSize() - 1 || DSin[v].GetRasterYSize() == DSout.GetRasterYSize() + 1);


	//						float no_data_in = DSin[v].GetNoData(0);
	//						vector<float> data(DSin[v].GetRasterXSize() * DSin[v].GetRasterYSize());
	//						pBandin->RasterIO(GF_Read, 0, 0, DSin[v].GetRasterXSize(), DSin[v].GetRasterYSize(), &(data[0]), DSin[v].GetRasterXSize(), DSin[v].GetRasterYSize(), GDT_Float32, 0, 0);

	//						//replace small values of snow by 0
	//						if (v == ERA5_SNOW)
	//						{
	//							for (size_t xy = 0; xy < data.size(); xy++)
	//							{
	//								if (data[xy] < 0.001)
	//									data[xy] = 0;
	//							}
	//						}

	//						if (DSin[v].GetRasterYSize() == DSout.GetRasterYSize())
	//						{
	//							pBandout->RasterIO(GF_Write, 0, 0, DSout.GetRasterXSize(), DSout.GetRasterYSize(), &(data[0]), DSout.GetRasterXSize(), DSout.GetRasterYSize(), GDT_Float32, 0, 0);
	//						}
	//						else
	//						{
	//							pBandout->RasterIO(GF_Write, 0, 0, DSout.GetRasterXSize(), DSout.GetRasterYSize(), &(data[0]), DSin[v].GetRasterXSize(), DSin[v].GetRasterYSize(), GDT_Float32, 0, 0);
	//						}


	//						pBandout->SetDescription(ERA5_META_DATA[v][M_DESC]);
	//						pBandout->SetMetadataItem("GRIB_COMMENT", ERA5_META_DATA[v][M_COMMENT]);
	//						pBandout->SetMetadataItem("GRIB_ELEMENT", ERA5_META_DATA[v][M_ELEMENT]);
	//						pBandout->SetMetadataItem("GRIB_SHORT_NAME", ERA5_META_DATA[v][M_SHORT_NAME]);
	//						pBandout->SetMetadataItem("GRIB_UNIT", ERA5_META_DATA[v][M_UNIT]);
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
	//					string plugin_path = GetApplicationPath() + "External\\gdalplugins";

	//					string option = "--config GDAL_DATA \"" + gdal_data_path + "\" --config PROJ_LIB \"" + projlib_path + "\" --config GDAL_DRIVER_PATH \"" + plugin_path + "\"";
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
		size_t m = WBSF::as<int>(title.substr(9, 2)) - 1;
		size_t d = WBSF::as<int>(title.substr(11, 2)) - 1;
		TRef = CTRef(year, m, d);

		return TRef;
	}



	ERMsg CERA5::GetStationList(StringVector& stationList, CCallback& callback)
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




	ERMsg CERA5::GetWeatherStation(const string& ID, CTM TM, CWeatherStation& station, CCallback& callback)
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