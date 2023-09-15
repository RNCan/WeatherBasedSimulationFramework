//***********************************************************************
// program to merge Landsat image image over a period
//									 
//***********************************************************************
// version
// 1.2.0	13/09/2023	Rémi Saint-Amant	Compile with GDAL 3.7.1
// 1.1.0	20/12/2021	Rémi Saint-Amant	Compile with VS 2019 and GDAL 3.0.3
// 1.0.0	13/07/2018	Rémi Saint-Amant	Creation

//-of VRT -stats -overview {2,4,8,16} -te 2058840 2790270 2397465 3074715 -period "2018-07-16-00" "2018-07-16-05" -var WNDS -var WNDD  -overwrite --config GDAL_CACHEMAX 1024 -co "compress=LZW" "D:\Travaux\Dispersal2018\Weather\Test.Gribs" "D:\Travaux\Dispersal2018\Weather\output2.vrt"
//-of XYZ -te 2058840 2790270 2397465 3074715 -period "2018-07-16-00" "2018-07-16-05" -var WNDS -var WNDD --config GDAL_CACHEMAX 1024 -co "compress=LZW" "D:\Travaux\Dispersal2018\Weather\Test.Gribs" "D:\Travaux\Dispersal2018\Weather\output2.csv"
//-period "2018-07-25-02" "2018-07-25-02" -var WNDS -var WNDD -Levels "0" -loc "D:\Travaux\YanBoulanger\LacMalcolm.csv" "D:\Travaux\YanBoulanger\RAP2018.Gribs" "D:\Travaux\YanBoulanger\test.csv"




#include "stdafx.h"
#include <math.h>
#include <array>
#include <utility>
#include <iostream>

#include "ConvertNetCDF2GeoTIFF.h"
#include "Basic/OpenMP.h"
#include "Basic/UtilTime.h"
#include "Basic/UtilMath.h"
#include "Basic/CSV.h"


#pragma warning(disable: 4275 4251)
#include "gdal_priv.h"

using namespace std;

namespace WBSF
{


	const char* CConvertNetCDF2GeoTIFF::VERSION = "1.2.0";
	const int CConvertNetCDF2GeoTIFF::NB_THREAD_PROCESS = 2;
	enum TGEVar { GR_TAIR, GR_PRCP, GR_WNDU, GR_WNDV, GR_WNDS, GR_WNDD, NB_GE_VARIABLES };
	static const char* VAR_NAME[NB_GE_VARIABLES] = { "TAIR", "PRCP", "WNDU", "WNDV", "WNDS", "WNDD" };
	size_t GetVariable(const string& str)
	{
		size_t type = NOT_INIT;
		for (size_t i = 0; i < NB_GE_VARIABLES && type == NOT_INIT; i++)
			if (IsEqualNoCase(str, VAR_NAME[i]))
				type = i;

		return type;
	}


	//*********************************************************************************************************************

	CConvertNetCDF2GeoTIFFOption::CConvertNetCDF2GeoTIFFOption()
	{
		m_appDescription = "This software extract weather from gribs file.";
		m_outputType = GDT_Float32;

		static const COptionDef OPTIONS[] =
		{
			//{ "-Levels", 1, "{level1,level2,...}", false, "Altitude (in meters) over ground to extract weather. Surface (0 - 10) by default." },
			//{ "-Var", 1, "var", true, "Select variable to extract. Variable available are: TAIR, PRCP, WNDU, WNDV, WNDS, WNDD" },
			//{ "-Loc", 1, "filePath", false, "File path for locations list to extract point instead of images. " },
			//{ "-SubHourly", 1, "seconds", false, "Output frequency. 3600 s (hourly) by default." },
			//{ "-Period", 2, "begin end", false, "Period (in UTC) to extract. Format of date must be \"yyyy-mm-dd-hh\"."},
			//{ "-Tonight", 2, "first last", false, "first and last hour (in UTC) to extract." },
			//{ "-units", 2, "var unit", false, "Period (in UTC) to extract. Format of date must be \"yyyy-mm-dd-hh\"." },
			//{ "-WS", 0, "", false, "Extract wind speed from u and v component." },
			//{ "-WD", 0, "", false, "Extract wind direction from u and v component." },
			{ "gribsfile", 0, "", false, "Input image directory." },
			{ "dstfile", 0, "", false, "Output batch name." }
		};

		for (int i = 0; i < sizeof(OPTIONS) / sizeof(COptionDef); i++)
			AddOption(OPTIONS[i]);

		static const CIOFileInfoDef IO_FILE_INFO[] =
		{
			{ "Input Gribs", "gribsfile", "", "any number of weather layers", "", "" },
			{ "Output Image", "dstfile", "", "nbVariable*nbLevels", "", "" },
		};

		for (int i = 0; i < sizeof(IO_FILE_INFO) / sizeof(CIOFileInfoDef); i++)
			AddIOFileInfo(IO_FILE_INFO[i]);
	}

	ERMsg CConvertNetCDF2GeoTIFFOption::ParseOption(int argc, char* argv[])
	{
		ERMsg msg = CBaseOptions::ParseOption(argc, argv);

		ASSERT(NB_FILE_PATH == 2);


		if (msg && m_filesPath.size() != NB_FILE_PATH)
		{
			msg.ajoute("ERROR: Invalid argument line. 2 files are needed: the gribs source and destination image.");
			msg.ajoute("Argument found: ");
			for (size_t i = 0; i < m_filesPath.size(); i++)
				msg.ajoute("   " + to_string(i + 1) + "- " + m_filesPath[i]);
		}

		return msg;
	}

	ERMsg CConvertNetCDF2GeoTIFFOption::ProcessOption(int& i, int argc, char* argv[])
	{
		ERMsg msg;

		//if (IsEqual(argv[i], "-Levels"))
		//{
		//	string tmp = argv[++i];
		//	StringVector levels(tmp, "{,}");

		//	m_levels.clear();
		//	for (StringVector::const_iterator it = levels.begin(); it != levels.end(); it++)
		//	{
		//		if (!it->empty())
		//			m_levels.push_back(ToDouble(*it));
		//	}
		//}
		//else if (IsEqual(argv[i], "-Var"))
		//{
		//	string tmp = argv[++i];
		//	size_t type = GetVariable(tmp);
		//	if (type != NOT_INIT)
		//		m_variables.push_back(type);
		//	else
		//		msg.ajoute(string(tmp) + " is not a valid variable");

		//	/*StringVector vars(tmp, "{,}");

		//m_variables.clear();
		//for (StringVector::iterator it = vars.begin(); it != vars.end(); it++)
		//{
		//	Trim(*it);
		//	if (!it->empty())
		//		m_variables.push_back(*it);
		//}*/
		//}
		//else if (IsEqual(argv[i], "-Loc"))
		//{
		//	vector<double> m_elevation;

		//	m_locations_file_path = argv[++i];
		//}
		//else if (IsEqual(argv[i], "-SubHourly"))
		//{
		//	m_time_step = std::atoi(argv[++i]);
		//}
		//else if (IsEqual(argv[i], "-Tonight"))
		//{
		//	int f = std::atoi(argv[++i]);
		//	int l = std::atoi(argv[++i]);
		//	
		//	
		//	//fl += 24;
		//	
		//	

		//	CTRef now = CTRef::GetCurrentTRef(CTM::HOURLY, true);
		//	if ( (now.GetHour() > l && now.GetHour() < f) || now.GetHour() >= f)
		//	{
		//		f = f - (int)now.GetHour();
		//		l = l - (int)now.GetHour() + 24;
		//		m_period = CTPeriod(now + f, now + l);
		//	}
		//	else 
		//	{
		//		f = f - (int)now.GetHour() - 24;
		//		l = l - (int)now.GetHour();
		//		m_period = CTPeriod(now + f, now + l);
		//	}
		//}
		//else
		//{
			//Look to see if it's a know base option
			msg = CBaseOptions::ProcessOption(i, argc, argv);
		//}

		return msg;
	}

	ERMsg CConvertNetCDF2GeoTIFF::Execute()
	{
		ERMsg msg;


		string in_path_filter = m_options.m_filesPath[CConvertNetCDF2GeoTIFFOption::INPUT_PATH];
		string out_file_path1 = m_options.m_filesPath[CConvertNetCDF2GeoTIFFOption::OUTPUT_PATH];
		SetFileTitle(out_file_path1, GetFileTitle(out_file_path1)+"_convert");
		string out_file_path2 = m_options.m_filesPath[CConvertNetCDF2GeoTIFFOption::OUTPUT_PATH];
		SetFileTitle(out_file_path2, GetFileTitle(out_file_path2) + "_build");
		string out_file_path3 = m_options.m_filesPath[CConvertNetCDF2GeoTIFFOption::OUTPUT_PATH];
		SetFileTitle(out_file_path3, GetFileTitle(out_file_path3) + "_inv");
		


		string out_path = GetPath(m_options.m_filesPath[CConvertNetCDF2GeoTIFFOption::OUTPUT_PATH]);
		string out_tmp_path = out_path +"tmp\\";
		string out_tif_path = out_path + "GeoTiff\\";
		string template_file_path = out_path + "template.inv";

		CreateMultipleDir(out_tmp_path);
		CreateMultipleDir(out_tif_path);


		if (!m_options.m_bQuiet)
		{
			cout << "Output: " << out_path << endl;
			cout << "From:   " << in_path_filter << endl;
		}

		ofStream out1;
		ofStream out2;
		ofStream out3;
		ofStream template_file;

		

		
		

		msg += out1.open(out_file_path1);
		msg += out2.open(out_file_path2);
		msg += out3.open(out_file_path3);
		msg += template_file.open(template_file_path);
		if (msg)
		{


			static const char* VAR_NAME[14] = { "HGT", "geopotential_height", "T2", "temperature", "u10_e", "ue_unstaggered", "v10_e", "ve_unstaggered", "", "w_unstaggered", "PSFC", "pressure", "precipitation", ""};
			static const size_t NB_LAYERS[14] = { 1, 11, 1, 11, 1, 11, 1, 11, 0, 11, 1, 11, 1, 0 };

			StringVector list = WBSF::GetFilesList(in_path_filter, 2, true);
			for (size_t i = 0; i < list.size()&&msg; i++)
			{

				string out_file_path_list = out_tmp_path + GetFileTitle(list[i]) + "_list.txt";
				string out_file_path_vrt = out_tmp_path + GetFileTitle(list[i]) + ".vrt";
				string out_file_tiff = out_tif_path + GetFileTitle(list[i]) + ".tif";
				string out_file_inv = out_tif_path + GetFileTitle(list[i]) + ".inv";
				
				ofStream out_list;
				msg += out_list.open(out_file_path_list);
				if (msg)
				{

					//gdal_translate -co compress=LZW -b 1 -ot Float32 -a_ullr  -698700 551200 1483200 -648700 -a_srs "+proj=lcc +lat_1=30 +lat_2=60 +lat_0=48.13746 +lon_0=-71.4 +x_0=0 +y_0=0 +datum=WGS84" NETCDF:"D:\Travaux\WRF2018\WRF_NARR_2018071009-1112\wrfout_subset_d03_2018-07-10_17_00_00.nc":HGT ./tmp/wrfout_subset_d04_2013-07-15_18_00_00.geopotential_height.0.tif
					static const char* FORMAT = "gdal_translate -co compress=LZW -b %02d -ot Float32 -a_ullr  -698700 551200 1483200 -648700 -a_srs \"+proj=lcc +lat_1=30 +lat_2=60 +lat_0=48.13746 +lon_0=-71.4 +x_0=0 +y_0=0 +datum=WGS84\" NETCDF:\"%s\":%s \"%s%s\"";

					for (size_t v = 0; v < 14; v++)
					{
						size_t nb_b = NB_LAYERS[v];
						for (size_t b = 0; b < nb_b; b++)
						{
							string tiff_file_name = FormatA("%s_%s_%02d.tif", GetFileTitle(list[i]).c_str(), VAR_NAME[v], b+1);
							string str = FormatA(FORMAT, b + 1, list[i].c_str(), VAR_NAME[v], out_tmp_path.c_str(), tiff_file_name.c_str());
							out1 << str << endl;

							out_list << "./tmp/" << tiff_file_name << endl;
						}
					}

					out_list.close();

					static const char* FORMAT1 = "gdalBuildVRT -separate -input_file_list %s %s";
					static const char* FORMAT2 = "gdal_translate -co compress=LZW -ot Float32 -co TILED=YES -co BLOCKXSIZE=128 -co BLOCKYSIZE=128 %s %s";

					string str1 = FormatA(FORMAT1, out_file_path_list.c_str(), out_file_path_vrt.c_str());
					string str2 = FormatA(FORMAT2, out_file_path_vrt.c_str(), out_file_tiff.c_str());
					out2 << str1 << endl;
					out2 << str2 << endl;

					//.\tmp\wrfout_subset_d04_2013-07-15_18_00_00.vrt .\geoTIFF\wrfout_subset_d04_2013-07-15_18_00_00.tif
					//copy template.inv /Y .\GeoTiff\wrfout_subset_d03_2018-07-10_18_00_00.inv
					static const char* FORMAT3 = "copy template.inv /Y .\\GeoTiff\\%s";
					string str3 = FormatA(FORMAT3, out_file_inv.c_str());
					out3 << str3 << endl;
				}
			}

			out1.close();
			out2.close();
			out3.close();

			//create template file
			static const char* INV_VAR_NAME[14] = { "HGT", "HGT", "TMP","TMP", "UGRD", "UGRD", "VGRD", "VGRD", "", "WGRD", "PRES", "PRES", "APCP", "" };
			static const char* INV_LAYER_0[14] = { "surface", "%02d", "2 m above ground", "%02d", "10 m above ground", "%02d", "10 m above ground", "%02d", "", "%02d", "surface", "%02d", "surface", ""};

			
			for (size_t v = 0,bb = 0, b=0; v < 14; v++)
			{
				size_t nb_b = NB_LAYERS[v];
				for (size_t b = 0; b < nb_b; b++,bb++)
				{
					static const char* FORMAT = "%03d:0:d=0:%s:%s:anl";

					string layer = FormatA(INV_LAYER_0[v], b + 1);
					string str = FormatA(FORMAT, bb+1, INV_VAR_NAME[v], layer.c_str());
					template_file << str << endl;

				}
			}

			template_file.close();

		}

		return msg;
	}


}