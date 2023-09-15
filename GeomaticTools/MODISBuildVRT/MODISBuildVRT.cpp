//***********************************************************************
// program to create vrt from modis tif list 
//									 
//***********************************************************************
// version
// 1.1.0	13/09/2023	Rémi Saint-Amant	Compile with GDAL 3.7.1
// 1.0.0	03/12/2020	Rémi Saint-Amant	Creation


//-stats -co "compress=LZW" -overwrite  "G:\MODIS\MOD09A1\MOD09A1.A2020209.h13v04.006.2020218042556.hdf" "G:\MODIS\MOD09A1(tif)\MOD09A1.A2020209.h13v04.006.2020218042556.tif"
//"G:\MODIS\MOD09A1\h13v03\MOD09A1.A2020177.h13v03.006.2020188052931.hdf" "G:\MODIS\MOD09A1(tif)\h13v03\MOD09A1.A2020177.h13v03.006.2020188052931.tif"

#include "stdafx.h"
#include <float.h>
#include <math.h>
#include <array>
#include <utility>
#include <iostream>

#include "MODISBuildVRT.h"
#include "Basic/OpenMP.h"
#include "Basic/UtilTime.h"
#include "Basic/UtilMath.h"
#include "Geomatic/MODISDataset.h"


//#include "Geomatic/LandsatCloudsCleaner.h"
#pragma warning(disable: 4275 4251)
#include "gdal_priv.h"

using namespace std;
using namespace WBSF::Landsat1;

namespace WBSF
{
	const char* CMODISBuildVRT::VERSION = "1.1.0";
	//const int CMODISBuildVRT::NB_THREAD_PROCESS = 2;


	//*********************************************************************************************************************

	CMODISBuildVRTOption::CMODISBuildVRTOption()
	{

		m_RGBType = NATURAL;
		m_outputType = GDT_Byte;
		m_scenesSize = SCENES_SIZE;
		m_scenes = { { NOT_INIT, NOT_INIT } };
		m_dstNodata = 255;
		m_bust = { { 0, 255 } };
		m_bVirtual = false;



		m_appDescription = "This software create MODIS VRT images (composed of " + to_string(SCENES_SIZE) + " bands) from image list.";

		//AddOption("-RGB");
		//AddOption("-Rename");
		//AddOption("-mask");
		//AddOption("-maskValue");

		static const COptionDef OPTIONS[] =
		{
			//{ "-Virtual", 0, "", false, "Create virtual (.vrt) output file based on input files. Combine with -NoResult, this avoid to create new files. " },
			//{ "-Bust", 2, "min max", false, "replace busting pixel (lesser than min or greather than max) by no data. 0 and 255 by default." },
			{ "srcfile", 0, "", false, "Input list file path." },
			{ "dstfile", 0, "", false, "Output vrt file path." }
		};

		for (int i = 0; i < sizeof(OPTIONS) / sizeof(COptionDef); i++)
			AddOption(OPTIONS[i]);

		//RemoveOption("-ot");
		RemoveOption("-CPU");//no multi thread in inner loop

		static const CIOFileInfoDef IO_FILE_INFO[] =
		{
			{ "Input Image", "srcfile", "txt file", "", "", "" },
			{ "Output Image", "dstfile", "vrt file", "", "", "" },
		};

		for (int i = 0; i < sizeof(IO_FILE_INFO) / sizeof(CIOFileInfoDef); i++)
			AddIOFileInfo(IO_FILE_INFO[i]);
	}

	ERMsg CMODISBuildVRTOption::ParseOption(int argc, char* argv[])
	{
		ERMsg msg = CBaseOptions::ParseOption(argc, argv);

		ASSERT(NB_FILE_PATH == 2);
		if (msg && m_filesPath.size() != NB_FILE_PATH)
		{
			msg.ajoute("ERROR: Invalid argument line. 2 files are needed: the source images list and destination vrt file.");
			msg.ajoute("Argument found: ");
			for (size_t i = 0; i < m_filesPath.size(); i++)
				msg.ajoute("   " + to_string(i + 1) + "- " + m_filesPath[i]);
		}

		m_outputType = GDT_Byte;

		return msg;
	}

	ERMsg CMODISBuildVRTOption::ProcessOption(int& i, int argc, char* argv[])
	{
		ERMsg msg;

		/*		if (IsEqual(argv[i], "-RGB"))
				{
					string str = argv[++i];

					if (IsEqualNoCase(str, RGB_NAME[NATURAL]))
						m_type = NATURAL;
					else if (IsEqualNoCase(str, RGB_NAME[LANDWATER]))
						m_type = LANDWATER;
					else
						msg.ajoute("Bad RGB type format. RGB type format must be \"Natural\" or \"LandWater\"");

				}
				else if (IsEqual(argv[i], "-Scenes"))
				{
					m_scenes[0] = atoi(argv[++i]) - 1;
					m_scenes[1] = atoi(argv[++i]) - 1;
				}
				else */

		{
			//Look to see if it's a know base option
			msg = CBaseOptions::ProcessOption(i, argc, argv);
		}


		return msg;
	}



	ERMsg CMODISBuildVRT::Execute()
	{
		ERMsg msg;


		if (!m_options.m_bQuiet)
		{
			cout << "Output: " << m_options.m_filesPath[CMODISBuildVRTOption::OUTPUT_FILE_PATH] << endl;
			cout << "From:   " << m_options.m_filesPath[CMODISBuildVRTOption::INPUT_FILE_PATH] << endl;
		}

		GDALAllRegister();



		ifStream file;
		msg = file.open(m_options.m_filesPath[CMODISBuildVRTOption::INPUT_FILE_PATH]);
		if (msg)
		{
			string text = file.GetText();
			file.close();


			ReplaceString(text, "\"", "");
			ReplaceString(text, "\r", "");


			StringVector all_files(text, "\n");
			std::sort(all_files.begin(), all_files.end());




			ofStream file2;
			msg = file2.open(m_options.m_filesPath[CMODISBuildVRTOption::INPUT_FILE_PATH]);
			if (msg)
			{
				for (size_t i = 0; i < all_files.size(); i++)
				{
					for (size_t j = 0; j < MODIS::SCENES_SIZE; j++)
					{
						string file_path_out = all_files[i];
						SetFileName(file_path_out, GetFileTitle(file_path_out) + "_" + MODIS::GetBandName(j) + ".tif");
						file2 << file_path_out << endl;
					}

				}

				file2.close();


				string command = "\"C:/exe/QGIS/bin/gdalbuildvrt.exe\" -separate -overwrite -a_srs \"+proj=sinu +lon_0=0 +x_0=0 +y_0=0 +R=6371007.181 +units=m +no_defs\" -input_file_list \"" + m_options.m_filesPath[CMODISBuildVRTOption::INPUT_FILE_PATH] + "\" \"" + m_options.m_filesPath[CMODISBuildVRTOption::OUTPUT_FILE_PATH] + "\"";
				msg += WinExecWait(command);
			}
		}


		return msg;
	}

}