//***********************************************************************
// program to merge Landsat image image over a period
//									 
//***********************************************************************
// version
// 1.0.0	03/12/2020	Rémi Saint-Amant	Creation


//-stats -co "compress=LZW" -overwrite  "G:\MODIS\MOD09A1\MOD09A1.A2020209.h13v04.006.2020218042556.hdf" "G:\MODIS\MOD09A1(tif)\MOD09A1.A2020209.h13v04.006.2020218042556.tif"
//"G:\MODIS\MOD09A1\h13v03\MOD09A1.A2020177.h13v03.006.2020188052931.hdf" "G:\MODIS\MOD09A1(tif)\h13v03\MOD09A1.A2020177.h13v03.006.2020188052931.tif"

#include "stdafx.h"
#include <float.h>
#include <math.h>
#include <array>
#include <utility>
#include <iostream>

#include "MODIS_HDF_2_GEOTIFF.h"
#include "Basic/OpenMP.h"
#include "Basic/UtilTime.h"
#include "Basic/UtilMath.h"
#include "Geomatic/MODISDataset.h"


//#include "Geomatic/LandsatCloudsCleaner.h"
#pragma warning(disable: 4275 4251)
#include "gdal_priv.h"

using namespace std;
using namespace WBSF::Landsat;

namespace WBSF
{
	const char* CMODIS_HDF_2_GEOTIFF::VERSION = "1.0.0";
	//const int CMODIS_HDF_2_GEOTIFF::NB_THREAD_PROCESS = 2;


	//*********************************************************************************************************************

	CMODIS_HDF_2_GEOTIFFOption::CMODIS_HDF_2_GEOTIFFOption()
	{

		m_RGBType = NATURAL;
		m_outputType = GDT_Byte;
		m_scenesSize = SCENES_SIZE;
		m_scenes = { { NOT_INIT, NOT_INIT } };
		m_dstNodata = 255;
		m_bust = { { 0, 255 } };
		m_bVirtual = false;



		m_appDescription = "This software transform MODIS HDF images into stadardized MODIS images (composed of " + to_string(SCENES_SIZE) + " bands).";

		AddOption("-RGB");
		AddOption("-Rename");
		AddOption("-mask");
		AddOption("-maskValue");

		static const COptionDef OPTIONS[] =
		{
			//{ "-Virtual", 0, "", false, "Create virtual (.vrt) output file based on input files. Combine with -NoResult, this avoid to create new files. " },
			//{ "-Bust", 2, "min max", false, "replace busting pixel (lesser than min or greather than max) by no data. 0 and 255 by default." },
			{ "srcfile", 0, "", false, "Input image file path." },
			{ "dstfile", 0, "", false, "Output image file path." }
		};

		for (int i = 0; i < sizeof(OPTIONS) / sizeof(COptionDef); i++)
			AddOption(OPTIONS[i]);

		//RemoveOption("-ot");
		RemoveOption("-CPU");//no multi thread in inner loop

		static const CIOFileInfoDef IO_FILE_INFO[] =
		{
			{ "Input Image", "srcfile", "", "ScenesSize(9)*nbScenes", "B1: Landsat band 1|B2: Landsat band 2|B3: Landsat band 3|B4: Landsat band 4|B5: Landsat band 5|B6: Landsat band 6|B7: Landsat band 7|QA: Image quality|Date: date of image(Julian day 1970 or YYYYMMDD format)|... for all scenes", "" },
			{ "Output Image", "dstfile", "nbScenes", "3 color (RGB) image", "B1:red|B2:green|B3:blue", "" },
		};

		for (int i = 0; i < sizeof(IO_FILE_INFO) / sizeof(CIOFileInfoDef); i++)
			AddIOFileInfo(IO_FILE_INFO[i]);
	}

	ERMsg CMODIS_HDF_2_GEOTIFFOption::ParseOption(int argc, char* argv[])
	{
		ERMsg msg = CBaseOptions::ParseOption(argc, argv);

		ASSERT(NB_FILE_PATH == 2);
		if (msg && m_filesPath.size() != NB_FILE_PATH)
		{
			msg.ajoute("ERROR: Invalid argument line. 2 files are needed: the source and destination image.");
			msg.ajoute("Argument found: ");
			for (size_t i = 0; i < m_filesPath.size(); i++)
				msg.ajoute("   " + to_string(i + 1) + "- " + m_filesPath[i]);
		}

		m_outputType = GDT_Byte;

		return msg;
	}

	ERMsg CMODIS_HDF_2_GEOTIFFOption::ProcessOption(int& i, int argc, char* argv[])
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
		if (IsEqual(argv[i], "-Bust"))
		{
			m_bust[0] = ToInt(argv[++i]);
			m_bust[1] = ToInt(argv[++i]);
		}
		else if (IsEqual(argv[i], "-Virtual"))
		{
			m_bVirtual = true;
		}
		else
		{
			//Look to see if it's a know base option
			msg = CBaseOptions::ProcessOption(i, argc, argv);
		}


		return msg;
	}




	/*
	Bit     Description
31      Adjacency correction performed;
		key: yes(1) or no (0)
30      Atmospheric correction performed;
		key: yes (1) or no (0)
26-29   Band 7 data quality, four bit range;
		  key: 0000  (0): highest quality
				   0111  (7): noisy detector
				   1000  (8): dead detector; data copied from adjacent
							  detector
				   1001  (9): solar zenith >= 86 degrees
				   1010 (10): solar zenith >= 85 and < 86 degrees
				   1011 (11): missing input
			   1100 (12): internal constant used in place of
				  climatological data for at least one
			  atmospheric constant
			   1101 (13): quality so low that it is not useful
			   1110 (14): L1B data faulty
			   1111 (15): not useful for any other reason/not
						  processed (e.g. clouds, ocean)
22-25   Band 6 data quality, four bit range;
		key: (same as "Band 7 data quality")
18-21   Band 5 data quality, four bit range;
		key: (same as "Band 7 data quality")
14-17   Band 4 data quality, four bit range;
		key: (same as "Band 7 data quality")
10-13   Band 3 data quality, four bit range;
		key: (same as "Band 7 data quality")
6-9     Band 2 data quality, four bit range;
		key: (same as "Band 7 data quality")
2-5     Band 1 data quality, four bit range;
		key: (same as "Band 7 data quality")
0-1	MODLAND QA bits;
		key: 00 (0): corrected product produced at ideal
					 quality -- all bands
			 01 (1): corrected product produced, less than
					 ideal quality -- some or all bands
			 10 (2): corrected product not produced due to
					 cloud effects -- all bands
			 11 (3): corrected product not produced for
					 other reasons -- some or all bands,
					 may be fill value
		[Note that a value of (11) overrides a value of (01)]
*/

	struct MODIS_QC_BITS
	{
		UINT32 MODLAND_QA : 2;//0 - 1
		//00 (0) : corrected product produced at ideal quality -- all bands
		//01 (1) : corrected product produced, less than ideal quality -- some or all bands
		//10 (2) : corrected product not produced due to cloud effects -- all bands
		//11 (3) : corrected product not produced for other reasons -- some or all bands, may be fill value 
		//[Note that a value of(11) overrides a value of(01)]

		UINT32 Band1_QC : 4;//2 - 5     Band 1 data quality, four bit range;
		UINT32 Band2_QC : 4;//6 - 9     Band 2 data quality, four bit range;
		UINT32 Band3_QC : 4;//10 - 13   Band 3 data quality, four bit range;
		UINT32 Band4_QC : 4;//14 - 17   Band 4 data quality, four bit range;
		UINT32 Band5_QC : 4;//18 - 21   Band 5 data quality, four bit range;
		UINT32 Band6_QC : 4;//22 - 25   Band 6 data quality, four bit range;
		UINT32 Band7_QC : 4;//26 - 29   Band 7 data quality, four bit range;
		//0000  (0) : highest quality
		//0111  (7) : noisy detector
		//1000  (8) : dead detector; data copied from adjacent detector
		//1001  (9) : solar zenith >= 86 degrees
		//1010 (10) : solar zenith >= 85 and < 86 degrees
		//1011 (11) : missing input
		//1100 (12) : internal constant used in place of climatological data for at least one atmospheric constant
		//1101 (13) : quality so low that it is not useful
		//1110 (14) : L1B data faulty
		//1111 (15) : not useful for any other reason / not processed(e.g.clouds, ocean)

		UINT32 AtmosphericCorrectionPerformed : 1;//30    key: yes(1) or no(0)
		UINT32 AdjacencyCorrectionPerformed : 1;//31      key: yes(1) or no(0)
	};

	__int16 ConvertQC(UINT32 QC)
	{
		MODIS_QC_BITS bits = *(MODIS_QC_BITS*)(&QC);

		//if (bits.Band1_QC >= 8 ||
		//	bits.Band2_QC >= 8 ||
		//	bits.Band3_QC >= 8 ||
		//	bits.Band4_QC >= 8 ||
		//	bits.Band5_QC >= 8 ||
		//	bits.Band6_QC >= 8 ||
		//	bits.Band7_QC)
		//	return 9999;


		__int16 QC_out = (__int16)bits.Band1_QC + bits.Band2_QC + bits.Band3_QC + bits.Band4_QC + bits.Band5_QC + bits.Band6_QC + bits.Band7_QC;

		return QC_out;
	}

	/*
	sur_refl_state_500m	UINT16	(see grid structure)

	Description:
	MODIS Land Surface Reflectance 500m State flags.

	Bit    Description
	15     PGE11 internal snow mask;
		   key: snow (1) or no snow (0)
	14     BRDF correction performed;
		   key: yes (1) or no (0)
	13     Pixel is adjacent to cloud;
		   key: yes (1) or no (0)
	12     Snow/ice flag;
		   key: yes (1) or no (0)
	11     PGE11 internal fire mask;
		   key: fire (1) or no fire (0)
	10     PGE11 internal cloud mask;
		   key: cloudy (1) or clear (0)
	8-9    Cirrus detected;
		   key: 00 (0): none
				01 (1): small
			10 (2): average
			11 (3): high
	6-7    Aerosol quantity;
		   key: 00 (0): climatology
				01 (1): low
			10 (2): average
			11 (3): high
	3-5    Land/water flag;


	3-5    land/water flag; class definitions:
		   000 -- shallow ocean
		   001 -- land
		   010 -- ocean coastlines and land shorelines
		   011 -- shallow inland water
		   100 -- ephemeral water
		   101 -- deep inland water
		   110 -- continental/moderate ocean
		   111 -- deep ocean



	2      Cloud shadow;
		   key: yes (1) or no (0)
	0-1    Cloud state;
		   key: 00 (0): clear
				01 (1): cloudy
				10 (2): mixed
				11 (3): not set, assumed clear

	*/


	//MODIS Land Surface Reflectance 500m State flags.
	struct MODIS_State_BITS
	{
		UINT16 CloudState : 2;//0 - 1    
		//00 (0) : clear
		//01 (1) : cloudy
		//10 (2) : mixed
		//11 (3) : not set, assumed clear
		UINT16 CloudShadow : 1;//2 key: yes(1) or no(0)

		UINT16 LandWaterFlag : 3;//3 - 5
		//  000 (0) : shallow ocean
		//	001 (1) : land
		//	010 (2) : ocean coastlines and land shorelines
		//	011 (3) : shallow inland water
		//	100 (4) : ephemeral water
		//	101 (5) : deep inland water
		//	110 (6) : continental/moderate ocean
		//	111 (7) : deep ocean

		UINT16 AerosolQuantity : 2;//6 - 7    
		// 00 (0) : climatology
		// 01 (1) : low
		// 10 (2) : average
		// 11 (3) : high
		UINT16 CirrusDetected : 2;//8 - 9    
		// 00 (0) : none
		// 01 (1) : small
		// 10 (2) : average
		// 11 (3) : high
		UINT16 InternalCloudMask : 1;//10 	 key: cloudy(1) or clear(0)
		UINT16 InternalFireMask : 1;//11     key: fire(1) or no fire(0)
		UINT16 SnowIceFlag : 1;		//12     	   key: yes(1) or no(0)
		UINT16 PixelIsAdjacentToCloud : 1;	//13     key: yes(1) or no(0)
		UINT16 BRDFCorrectionPerformed : 1;	//14     	 key: yes(1) or no(0)
		UINT16 InternalSnowMask : 1;		//15     internal snow mask;	key: snow(1) or no snow(0)

	};

	UINT16 ConvertState(UINT16 state)
	{
		MODIS_State_BITS bits = *(MODIS_State_BITS*)(&state);
		//bits.CloudState = 0;
		//bits.CloudShadow = 0;
		//bits.LandWaterFlag = 0;
		//bits.AerosolQuantity = 0;
		//bits.CirrusDetected = 0;
		//bits.InternalCloudMask = 0;
		//bits.InternalFireMask = 0;
		//bits.SnowIceFlag = 0;
		//bits.PixelIsAdjacentToCloud = 0;
		//bits.BRDFCorrectionPerformed = 0;
		//bits.InternalSnowMask = 0;
		//UINT16 state_out = *(UINT16*)(&bits);

		UINT16 state_out = 0;

		if (bits.CloudState == 3)
			bits.CloudState = 0;


		//if (bits.InternalSnowMask || bits.PixelIsAdjacentToCloud || bits.SnowIceFlag || bits.InternalCloudMask || bits.CloudShadow || bits.CloudState)
			//state_out = 1 + ((bits.CloudState == 2) * 1) + ((bits.CloudState == 1 || bits.SnowIceFlag || bits.InternalSnowMask) * 2);

		state_out = bits.LandWaterFlag;


		return state_out;
	}


	ERMsg CMODIS_HDF_2_GEOTIFF::Execute()
	{
		ERMsg msg;

		assert(sizeof(MODIS_QC_BITS) == 4);
		assert(sizeof(MODIS_State_BITS) == 2);


		if (!m_options.m_bQuiet)
		{
			cout << "Output: " << m_options.m_filesPath[CMODIS_HDF_2_GEOTIFFOption::OUTPUT_FILE_PATH] << endl;
			cout << "From:   " << m_options.m_filesPath[CMODIS_HDF_2_GEOTIFFOption::INPUT_FILE_PATH] << endl;

			if (!m_options.m_maskName.empty())
				cout << "Mask:   " << m_options.m_maskName << endl;
		}

		GDALAllRegister();





		string tmp_path = "G:/MODIS/tmp/";
		CreateMultipleDir(tmp_path);
		CreateMultipleDir(GetPath(m_options.m_filesPath[CMODIS_HDF_2_GEOTIFFOption::OUTPUT_FILE_PATH]));

		string file_path_in = m_options.m_filesPath[CMODIS_HDF_2_GEOTIFFOption::INPUT_FILE_PATH];
		string title = GetFileTitle(file_path_in);
		string file_path_out = tmp_path + title + ".tif";
		string file_path_list = tmp_path + title + ".txt";
		//string file_path_vrt = m_options.m_filesPath[CMODIS_HDF_2_GEOTIFFOption::OUTPUT_FILE_PATH];
		string file_path_JD_out = tmp_path + title + "_JD.tif";
		string file_path_QA_out = tmp_path + title + "_QA.tif";

		//convert HDF into seperate .tif file:
		string command = "\"C:/exe/QGIS/bin/gdal_translate.exe\" -a_srs \"+proj=sinu +lon_0=0 +x_0=0 +y_0=0 +R=6371007.181 +units=m +no_defs\" -sds \"" + file_path_in + "\" \"" + file_path_out + "\"";
		msg += WinExecWait(command);
		if (!msg)
			return msg;


		enum TProduct { P_UNKNOWNS = -1, P_MOD09A1, P_MYD09A1, P_MOD09GA, P_MYD09GA, P_MCD43A4, P_MCD12Q1, NB_PRODUCT };
		static const char* PRODUCT_NAME[NB_PRODUCT] = { "MOD09A1", "MYD09A1", "MOD09GA", "MYD09GA", "MCD43A4", "MCD12Q1" };
		StringVector tmp(title, ".");
		ASSERT(tmp.size() == 5);
		size_t product = std::distance(std::begin(PRODUCT_NAME), std::find(std::begin(PRODUCT_NAME), std::end(PRODUCT_NAME), tmp[0]));
		ASSERT(product != P_UNKNOWNS);



		string year = tmp[1].substr(1, 4);

		if (product == P_MOD09A1 || product == P_MYD09A1)
		{
			//convert DOY int Julian day 1970
			CJDayRef TRef(ToInt(year), 0);
			int base = m_options.GetTRefIndex(TRef) - 1;


			string file_path_JD_in = tmp_path + title + "_13.tif";

			command = "\"E:/Project/bin/Tools/ImageCalculator.exe\" -multi -overwrite -co COMPRESS=LZW -e \"JD70=i1b1+" + to_string(base) + "\" \"" + file_path_JD_in + "\" \"" + file_path_JD_out + "\"";
			msg += WinExecWait(command);
			if (!msg)
				return msg;



			//convert 16 bit state into flag
			string file_path_state_in = tmp_path + title + "_12.tif";

			CGDALDatasetEx state1;
			msg = state1.OpenInputImage(file_path_state_in);
			if (msg)
			{
				CBaseOptions options;
				state1.UpdateOption(options);

				GDALRasterBand * pBand = state1->GetRasterBand(1);

				vector<UINT16> data(state1.GetRasterXSize()*state1.GetRasterYSize());
				pBand->RasterIO(GF_Read, 0, 0, state1.GetRasterXSize(), state1.GetRasterYSize(), &(data[0]), state1.GetRasterXSize(), state1.GetRasterYSize(), GDT_UInt16, 0, 0);

				state1.Close();

				CGDALDatasetEx state2;
				//create new images
				options.m_outputType = GDT_Int16;
				options.m_bOverwrite = true;
				options.m_dstNodata = -32768;
				msg = state2.CreateImage(file_path_QA_out, options);
				if (msg)
				{
					//CStatistic stat;
					//for (size_t i = 0; i < data1.size(); i++)
					//{
					//	data[i] = ConvertState(data[i]);
					//}

					GDALRasterBand * pBand = state2->GetRasterBand(1);

					pBand->RasterIO(GF_Write, 0, 0, state2.GetRasterXSize(), state2.GetRasterYSize(), &(data[0]), state2.GetRasterXSize(), state2.GetRasterYSize(), GDT_Int16, 0, 0);
					state2.Close();
				}
				//UINT16 ConvertState(UINT16 state)
			}
		}
		else if (product == P_MOD09GA || product == P_MYD09GA)
		{
			//convert 16 bit state into flag
			string file_path_state_in = tmp_path + title + "_02.tif";

			CGDALDatasetEx state1;
			msg = state1.OpenInputImage(file_path_state_in);
			if (msg)
			{
				CBaseOptions options;
				state1.UpdateOption(options);

				GDALRasterBand * pBand = state1->GetRasterBand(1);

				vector<UINT16> data(state1.GetRasterXSize()*state1.GetRasterYSize());
				pBand->RasterIO(GF_Read, 0, 0, state1.GetRasterXSize(), state1.GetRasterYSize(), &(data[0]), state1.GetRasterXSize(), state1.GetRasterYSize(), GDT_UInt16, 0, 0);

				state1.Close();

				CGDALDatasetEx QC;
				//create new images
				options.m_outputType = GDT_Int16;
				options.m_bOverwrite = true;
				options.m_dstNodata = -32768;
				msg = QC.CreateImage(file_path_QA_out, options);
				if (msg)
				{
					//for (size_t i = 0; i < data.size(); i++)
					//{
					//	data[i] = ConvertState(data[i]);
					//}

					GDALRasterBand * pBand = QC->GetRasterBand(1);

					pBand->RasterIO(GF_Write, 0, 0, QC.GetRasterXSize(), QC.GetRasterYSize(), &(data[0]), QC.GetRasterXSize(), QC.GetRasterYSize(), GDT_Int16, 0, 0);
					QC.Close();
				}

				CGDALDatasetEx JD;
				msg = JD.CreateImage(file_path_JD_out, options);
				if (msg)
				{
					//convert DOY int Julian day 1970
					string DOY_str = tmp[1].substr(5, 3);
					__int16 DOY = ToInt(DOY_str);

					CJDayRef TRef(ToInt(year), DOY - 1);
					__int16 DOY1970 = m_options.GetTRefIndex(TRef);

					vector<__int16> data(JD.GetRasterXSize()*JD.GetRasterYSize(), DOY1970);

					GDALRasterBand * pBand = JD->GetRasterBand(1);
					pBand->RasterIO(GF_Write, 0, 0, JD.GetRasterXSize(), JD.GetRasterYSize(), &(data[0]), JD.GetRasterXSize(), JD.GetRasterYSize(), GDT_Int16, 0, 0);
					JD.Close();
				}
			}


		}
		else if (product == P_MCD43A4)
		{
			CGDALDatasetEx DS1;
			string file_path_JD_in = tmp_path + title + "_07.tif";
			msg = DS1.OpenInputImage(file_path_JD_in);
			if (msg)
			{
				CBaseOptions options;
				DS1.UpdateOption(options);
				DS1.Close();


				//CGDALDatasetEx DS2;
				////create new images
				//options.m_outputType = GDT_Int16;
				//options.m_bOverwrite = true;
				//options.m_dstNodata = -32768;
				//msg = DS2.CreateImage(file_path_JD_out, options);
				//if (msg)
				//{
				//	//convert DOY int Julian day 1970
				string DOY_str = tmp[1].substr(5, 3);
				__int16 DOY = ToInt(DOY_str);

				CJDayRef TRef(ToInt(year), DOY - 1);
				__int16 DOY1970 = m_options.GetTRefIndex(TRef);


				//	vector<__int16> data(DS2.GetRasterXSize()*DS2.GetRasterYSize(), DOY1970);

				//	GDALRasterBand * pBand = DS2->GetRasterBand(1);
				//	pBand->RasterIO(GF_Write, 0, 0, DS2.GetRasterXSize(), DS2.GetRasterYSize(), &(data[0]), DS2.GetRasterXSize(), DS2.GetRasterYSize(), GDT_Int16, 0, 0);
				//	DS2.Close();
				//}

				//msg = DS2.CreateImage(file_path_QA_out, options);
				//if (msg)
				//{
				//	//convert DOY int Julian day 1970
				//	vector<__int16> data(DS2.GetRasterXSize()*DS2.GetRasterYSize(), 0);

				//	GDALRasterBand * pBand = DS2->GetRasterBand(1);
				//	pBand->RasterIO(GF_Write, 0, 0, DS2.GetRasterXSize(), DS2.GetRasterYSize(), &(data[0]), DS2.GetRasterXSize(), DS2.GetRasterYSize(), GDT_Int16, 0, 0);
				//	DS2.Close();
				//}
				
				string options_str = "-stats -ot INT16 -overwrite -dstnodata -32768 -overwrite -co COMPRESS=LZW";
				command = "\"E:/Project/bin/Tools/ImageCalculator.exe\" " + options_str + " -e \"JD=if(i1b1>=-100," + to_string(DOY1970) + ",-32768)\" \"" + file_path_JD_in + "\" \"" + file_path_JD_out + "\"";
				msg += WinExecWait(command);



				string file_paths;
				for (size_t i = 0; i < 7; i++)
				{
					if (!file_paths.empty())
						file_paths += " ";

					file_paths += "\"" + tmp_path + title + "_" + FormatA("%02d", i + 1) + ".tif\"";
				}
				
				
				command = "\"E:/Project/bin/Tools/ImageCalculator.exe\" " + options_str + " -e \"QA=i1b1+i2b1+i3b1+i4b1+i5b1+i6b1+i7b1\" " + file_paths + " \"" + file_path_QA_out + "\"";
				msg += WinExecWait(command);


			}
		}
		else if (product == P_MCD12Q1)
		{

		}


		if (!msg)
			return msg;


		StringVector fileId;
		if (product == P_MOD09A1 || product == P_MYD09A1)
		{
			fileId.Tokenize("01|02|03|04|05|06|07|QA|JD", "|");
		}
		else if (product == P_MOD09GA || product == P_MYD09GA)
		{
			fileId.Tokenize("12|13|14|15|16|17|18|QA|JD", "|");
		}
		else if (product == P_MCD43A4)
		{
			fileId.Tokenize("08|09|10|11|12|13|14|QA|JD", "|");
		}
		else if (product == P_MCD12Q1)
		{
			fileId.Tokenize("01", "|");
		}

		if (msg)
		{
			ofStream file;
			msg = file.open(file_path_list);
			if (msg)
			{
				for (size_t i = 0; i < fileId.size(); i++)
				{
					string file_path_in = tmp_path + title + "_" + fileId[i] + ".tif";
					string file_path_out = m_options.m_filesPath[CMODIS_HDF_2_GEOTIFFOption::OUTPUT_FILE_PATH];
					SetFileName(file_path_out, GetFileTitle(file_path_out) + "_" + MODIS::GetBandName(i) + ".tif" );
					file << file_path_out << endl;

					//convert into geotiff
					//command = "\"C:/exe/QGIS/bin/gdalwarp.exe\" -overwrite -co COMPRESS=LZW -dstnodata -32768 -ot INT16 -s_srs \"+proj=sinu +lon_0=0 +x_0=0 +y_0=0 +R=6371007.181 +units=m +no_defs\" \"" + file_path_in + "\" \"" + file_path_out + "\"";
					//command = "\"C:/exe/QGIS/bin/gdal_translate.exe\" -overwrite -co COMPRESS=LZW -dstnodata -32768 -ot INT16 -s_srs \"+proj=sinu +lon_0=0 +x_0=0 +y_0=0 +R=6371007.181 +units=m +no_defs\" \"" + file_path_in + "\" \"" + file_path_out + "\"";
					string options_str = "-stats -ot INT16 -overwrite -dstnodata -32768 -co COMPRESS=LZW -co \"tiled=YES\" -co \"BLOCKXSIZE=1024\" -co \"BLOCKYSIZE=1024\" ";
					
					if(fileId.size()==9)
						options_str += " -mask \"G:\\MODIS\\water_mask_500m.tif\" -maskvalue 0";

					command = "\"E:/Project/bin/Tools/ImageCalculator.exe\" " + options_str + " -e i1b1 \"" + file_path_in + "\" \"" + file_path_out + "\"";
					msg += WinExecWait(command);
				}
				
				file.close();
			}

			command = "\"C:/exe/QGIS/bin/gdalbuildvrt.exe\" -separate -overwrite -a_srs \"+proj=sinu +lon_0=0 +x_0=0 +y_0=0 +R=6371007.181 +units=m +no_defs\" -input_file_list \"" + file_path_list + "\" \"" + m_options.m_filesPath[CMODIS_HDF_2_GEOTIFFOption::OUTPUT_FILE_PATH] + "\"";
			msg += WinExecWait(command);

			//ofStream file;
			//msg = file.open(file_path_list);
			//if (msg)
			//{
			//	for (size_t i = 0; i < fileId.size(); i++)
			//		file << tmp_path << title << "_" << fileId[i] << ".tif" << endl;

			//	file.close();
			//}

			//command = "\"C:/exe/QGIS/bin/gdalbuildvrt.exe\" -separate -overwrite -input_file_list \"" + file_path_list + "\" \"" + file_path_vrt + "\"";
			//msg += WinExecWait(command);
			//if (msg)
			//{
			//	//convert into geotiff
			//	//command = "\"C:/exe/QGIS/bin/gdalwarp.exe\" -overwrite -co COMPRESS=LZW -dstnodata -32768 -ot INT16 -s_srs \"+proj=sinu +lon_0=0 +x_0=0 +y_0=0 +R=6371007.181 +units=m +no_defs\" -t_srs \"+proj=lcc +lat_0=0 +lon_0=-95 +lat_1=49 +lat_2=77 +x_0=0 +y_0=-8000000 +datum=WGS84 +units=m +no_defs\" \"" + file_path_vrt + "\" \"" + m_options.m_filesPath[CMODIS_HDF_2_GEOTIFFOption::OUTPUT_FILE_PATH] + "\"";
			//	//command = "\"C:/exe/QGIS/bin/gdalwarp.exe\" -overwrite -co COMPRESS=LZW -dstnodata -32768 -ot INT16 -s_srs \"+proj=sinu +lon_0=0 +x_0=0 +y_0=0 +R=6371007.181 +units=m +no_defs\" \"" + file_path_vrt + "\" \"" + m_options.m_filesPath[CMODIS_HDF_2_GEOTIFFOption::OUTPUT_FILE_PATH] + "\"";
			//	command = "\"C:/exe/QGIS/bin/gdalwarp.exe\" -overwrite -co COMPRESS=LZW -dstnodata -32768 -ot INT16 -s_srs \"+proj=sinu +lon_0=0 +x_0=0 +y_0=0 +R=6371007.181 +units=m +no_defs\" \"" + file_path_vrt + "\" \"" + m_options.m_filesPath[CMODIS_HDF_2_GEOTIFFOption::OUTPUT_FILE_PATH] + "\"";
			//	msg += WinExecWait(command);

			//	for(size_t i=0;i<7; i++)
			//	command = "\"C:/exe/QGIS/bin/gdalwarp.exe\" -overwrite -co COMPRESS=LZW -dstnodata -32768 -ot INT16 -s_srs \"+proj=sinu +lon_0=0 +x_0=0 +y_0=0 +R=6371007.181 +units=m +no_defs\" \"" + file_path_vrt + "\" \"" + m_options.m_filesPath[CMODIS_HDF_2_GEOTIFFOption::OUTPUT_FILE_PATH] + "\"";
			//}
		}

		if (!msg)
			return msg;


		//clean file
		for (size_t i = 0; i < 30; i++)
		{
			string file_path = tmp_path + title + "_" + FormatA("%02d", i + 1) + ".tif";
			if (FileExists(file_path))
				msg += WBSF::RemoveFile(file_path);
		}

		if (FileExists(file_path_list))
			msg += WBSF::RemoveFile(file_path_list);
		//if (FileExists(file_path_vrt))
			//msg += WBSF::RemoveFile(file_path_vrt);
		if (FileExists(file_path_JD_out))
			msg += WBSF::RemoveFile(file_path_JD_out);
		if (FileExists(file_path_QA_out))
			msg += WBSF::RemoveFile(file_path_QA_out);



		return msg;
	}

}