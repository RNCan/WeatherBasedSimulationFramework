//***********************************************************************
// program to merge Landsat image image over a period
//									 
//***********************************************************************
// version
// 1.0.0	21/12/2012	Rémi Saint-Amant	Creation

//-te -1271940 7942380 -1249530 7956540 -of vrt -co "bigtiff=yes" -co "compress=LZW" -co "tiled=YES" -co "BLOCKXSIZE=1024" -co "BLOCKYSIZE=1024" -blocksize 1024 1024 -multi -Type SecondBest -stats -debug -dstnodata -32768 --config GDAL_CACHEMAX 1024 "U:\GIS1\LANDSAT_SR\LCC\2014\#57_2014_182-244.vrt" "U:\GIS\#documents\TestCodes\LandsatWarp\Test4\Nuage.vrt"
//-stats -Type BestPixel -te 1358100 6854400 1370300 6865500 -of VRT -ot Int16 -blockSize 1024 1024 -co "compress=LZW" -co "tiled=YES" -co "BLOCKXSIZE=1024" -co "BLOCKYSIZE=1024" --config GDAL_CACHEMAX 4096  -overview {2,4,8,16} -multi -IOCPU 3 -overwrite -Clouds "U:\GIS\#documents\TestCodes\LandsatWarp\Test2\Model\V4_SR_DTD1_cloudv4_skip100_200" "U:\GIS1\LANDSAT_SR\LCC\1999-2006.vrt" "U:\GIS\#documents\TestCodes\LandsatWarp\Test2\Output\Test.vrt"
//-stats -Type Oldest -TT OverallYears -of VRT -ot Int16 -blockSize 1024 1024 -co "compress=LZW" -co "tiled=YES" -co "BLOCKXSIZE=1024" -co "BLOCKYSIZE=1024" --config GDAL_CACHEMAX 4096  -overview {2,4,8,16} -multi -IOCPU 3 -overwrite "U:\GIS\#documents\TestCodes\BandsAnalyser\Test1\Input\Test1999-2014.vrt" "U:\GIS\#documents\TestCodes\LandsatWarp\Test0\output\Test.vrt"

#include "stdafx.h"
#include <float.h>
#include <math.h>
#include <array>
#include <utility>
#include <iostream>

#include "Landsat2RGB.h"
#include "Basic/OpenMP.h"
#include "Basic/UtilTime.h"
#include "Basic/UtilMath.h"
#include "Geomatic/LandsatCloudsCleaner.h"
#pragma warning(disable: 4275 4251)
#include "gdal_priv.h"

using namespace std;
using namespace WBSF::Landsat;

namespace WBSF
{


	const char* CLandsat2RGB::VERSION = "1.0.0";
	const int CLandsat2RGB::NB_THREAD_PROCESS = 2;


	//*********************************************************************************************************************

	CLandsat2RGBOption::CLandsat2RGBOption()
	{


		m_scenesSize = SCENES_SIZE;
		m_scene = 0;
		m_dstNodata = 255;
		//m_bBust = false;
		m_bust = { { 0, 255 } };
		m_appDescription = "This software transform Landsat images (composed of " + to_string(SCENES_SIZE) + " bands) into RGB image. All empty images will be removed.";

		//AddOption("-Period");

		static const COptionDef OPTIONS[] =
		{
			{ "-SceneSize", 1, "size", false, "Number of images per scene. 9 by default." },//overide scene size defenition
			{ "-Scene", 1, "no", false, "Select a scene (1..nbScenes). The first scene is select by default." },//overide scene size defenition
			{ "-Bust", 2, "min max", false, "replace busting pixel (lesser than min or greather than max) by no data." },
			{ "srcfile", 0, "", false, "Input image file path." },
			{ "dstfile", 0, "", false, "Output image file path." }
		};

		for (int i = 0; i < sizeof(OPTIONS) / sizeof(COptionDef); i++)
			AddOption(OPTIONS[i]);
		
		RemoveOption("-ot");
		
		static const CIOFileInfoDef IO_FILE_INFO[] =
		{
			{ "Input Image", "srcfile", "", "ScenesSize(9)*nbScenes", "B1: Landsat band 1|B2: Landsat band 2|B3: Landsat band 3|B4: Landsat band 4|B5: Landsat band 5|B6: Landsat band 6|B7: Landsat band 7|QA: Image quality|Date: date of image(Julian day 1970 or YYYYMMDD format)|... for all scenes", "" },
			{ "Output Image", "dstfile", "nbScenes", "3 color (RGB) image", "B1:red|B2:green|B3:blue", "" },
		};

		for (int i = 0; i < sizeof(IO_FILE_INFO) / sizeof(CIOFileInfoDef); i++)
			AddIOFileInfo(IO_FILE_INFO[i]);
	}

	ERMsg CLandsat2RGBOption::ParseOption(int argc, char* argv[])
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

		if (m_outputType == GDT_Unknown)
			m_outputType = m_TTF == YYYYMMDD ? GDT_Int32 : GDT_Int16;

		return msg;
	}

	ERMsg CLandsat2RGBOption::ProcessOption(int& i, int argc, char* argv[])
	{
		ERMsg msg;

		if (IsEqual(argv[i], "-Scene"))
		{
			m_scene = ToInt(argv[++i])-1;
		}
		else if (IsEqual(argv[i], "-Bust"))
		{
			//m_bBust = true;
			m_bust[0] = ToInt(argv[++i]);
			m_bust[1] = ToInt(argv[++i]);

		}
		else
		{
			//Look to see if it's a know base option
			msg = CBaseOptions::ProcessOption(i, argc, argv);
		}
		

		return msg;
	}


	ERMsg CLandsat2RGB::Execute()
	{
		ERMsg msg;

		if (!m_options.m_bQuiet)
		{
			cout << "Output: " << m_options.m_filesPath[CLandsat2RGBOption::OUTPUT_FILE_PATH] << endl;
			cout << "From:   " << m_options.m_filesPath[CLandsat2RGBOption::INPUT_FILE_PATH] << endl;

			if (!m_options.m_maskName.empty())
				cout << "Mask:   " << m_options.m_maskName << endl;
		}

		GDALAllRegister();

		CLandsatDataset inputDS;
		CLandsatCloudCleaner cloudsCleaner;
		CBandsHolderMT bandHolder(1, m_options.m_memoryLimit, m_options.m_IOCPU, NB_THREAD_PROCESS);
		CGDALDatasetEx maskDS;

		msg = OpenInput(inputDS, maskDS);

		if (msg && maskDS.IsOpen())
			bandHolder.SetMask(maskDS.GetSingleBandHolder(), m_options.m_maskDataUsed);

		if (msg)
			msg += bandHolder.Load(inputDS, m_options.m_bQuiet, m_options.GetExtents(), m_options.m_period);

		if (!msg)
			return msg;


		CGeoExtents extents = bandHolder.GetExtents();
		m_options.ResetBar(extents.m_xSize*extents.m_ySize);
		vector<pair<int, int>> XYindex = extents.GetBlockList(5,5);

		if (!m_options.m_bQuiet)
		{
			cout << "Preprocess input images (" << inputDS.GetRasterXSize() << " C x " << inputDS.GetRasterYSize() << " R x " << inputDS.GetRasterCount() << " B) with " << m_options.m_CPU << " threads..." << endl;
		}

		omp_set_nested(1);//for IOCPU
		
		CLandsatDataset outputDS;
		msg = OpenOutput(outputDS);

		if (!msg)
			return msg;


		if (!m_options.m_bQuiet && m_options.m_bCreateImage)
		{
			cout << "Create output images (" << outputDS.GetRasterXSize() << " C x " << outputDS.GetRasterYSize() << " R x " << outputDS.GetRasterCount() << " B) with " << m_options.m_CPU << " threads..." << endl;
		}


#pragma omp parallel for schedule(static, 1) num_threads( NB_THREAD_PROCESS ) if (m_options.m_bMulti) 
		for (int b = 0; b < (int)XYindex.size(); b++)
		{
			int xBlock = XYindex[b].first;
			int yBlock = XYindex[b].second;

			int blockThreadNo = ::omp_get_thread_num();

			OutputData outputData;

			ReadBlock(xBlock, yBlock, bandHolder[blockThreadNo]);
			ProcessBlock(xBlock, yBlock, bandHolder[blockThreadNo], outputData);
			WriteBlock(xBlock, yBlock, bandHolder[blockThreadNo], outputDS, outputData);
			
		}//for all blocks

		//outputData.clear(); outputData.shrink_to_fit();

		CloseAll(inputDS, maskDS, outputDS);

		return msg;
	}



	ERMsg CLandsat2RGB::OpenInput(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS)
	{
		ERMsg msg;

		if (!m_options.m_bQuiet)
			cout << endl << "Open input image..." << endl;

		msg = inputDS.OpenInputImage(m_options.m_filesPath[CLandsat2RGBOption::INPUT_FILE_PATH], m_options);
		if (msg)
			inputDS.UpdateOption(m_options);


		if (msg && !m_options.m_bQuiet)
		{
			CGeoExtents extents = inputDS.GetExtents();
			CProjectionPtr pPrj = inputDS.GetPrj();
			string prjName = pPrj ? pPrj->GetName() : "Unknown";

			cout << "    Size           = " << inputDS.GetRasterXSize() << " cols x " << inputDS.GetRasterYSize() << " rows x " << inputDS.GetRasterCount() << " bands" << endl;
			cout << "    Extents        = X:{" << ToString(extents.m_xMin) << ", " << ToString(extents.m_xMax) << "}  Y:{" << ToString(extents.m_yMin) << ", " << ToString(extents.m_yMax) << "}" << endl;
			cout << "    Projection     = " << prjName << endl;
			cout << "    NbBands        = " << inputDS.GetRasterCount() << endl;
			cout << "    Scene size     = " << inputDS.GetSceneSize() << endl;
			cout << "    Nb. scenes     = " << inputDS.GetNbScenes() << endl;
			cout << "    First image    = " << inputDS.GetPeriod().Begin().GetFormatedString() << endl;
			cout << "    Last image     = " << inputDS.GetPeriod().End().GetFormatedString() << endl;
			cout << "    Input period   = " << m_options.m_period.GetFormatedString() << endl;

			if (inputDS.GetSceneSize() != SCENES_SIZE)
				cout << FormatMsg("WARNING: the number of bands per scene (%1) is different than the inspected number (%2)", to_string(inputDS.GetSceneSize()), to_string(SCENES_SIZE)) << endl;

			if (m_options.m_scene >= inputDS.GetNbScenes())
				msg.ajoute("Scene " + ToString(m_options.m_scene+1) + "must be smaller thant the number of scenes of the input image");
		}

		if (msg && !m_options.m_maskName.empty())
		{
			if (!m_options.m_bQuiet)
				cout << "Open mask..." << endl;

			msg += maskDS.OpenInputImage(m_options.m_maskName);
		}

		return msg;
	}

	ERMsg CLandsat2RGB::OpenOutput(CGDALDatasetEx& outputDS)
	{
		ERMsg msg;

		if (m_options.m_bCreateImage)
		{
			CLandsat2RGBOption options(m_options);
			
			options.m_nbBands = 3;
			options.m_outputType = GDT_Byte;


			if (!m_options.m_bQuiet)
			{
				cout << endl;
				cout << "Open output images..." << endl;
				cout << "    Size           = " << options.m_extents.m_xSize << " cols x " << options.m_extents.m_ySize << " rows x " << options.m_nbBands << " bands" << endl;
				cout << "    Extents        = X:{" << ToString(options.m_extents.m_xMin) << ", " << ToString(options.m_extents.m_xMax) << "}  Y:{" << ToString(options.m_extents.m_yMin) << ", " << ToString(options.m_extents.m_yMax) << "}" << endl;
			}

			string filePath = options.m_filesPath[CLandsat2RGBOption::OUTPUT_FILE_PATH];
			msg += outputDS.CreateImage(filePath, options);
		}


		return msg;
	}

	void CLandsat2RGB::ReadBlock(int xBlock, int yBlock, CBandsHolder& bandHolder)
	{
#pragma omp critical(BlockIO)
	{
		m_options.m_timerRead.Start();

		CGeoExtents extents = m_options.m_extents.GetBlockExtents(xBlock, yBlock);
		bandHolder.LoadBlock(extents, m_options.m_period);

		m_options.m_timerRead.Stop();
	}
	}

	void CLandsat2RGB::ProcessBlock(int xBlock, int yBlock, CBandsHolder& bandHolder, OutputData& outputData)
	{

		CGeoExtents extents = bandHolder.GetExtents();
		CGeoSize blockSize = extents.GetBlockSize(xBlock, yBlock);

		if (bandHolder.IsEmpty())
		{
//#pragma omp critical(ProcessBlock)
	//	{
			int nbCells = blockSize.m_x*blockSize.m_y;

#pragma omp atomic
			m_options.m_xx += nbCells;

			m_options.UpdateBar();
	//	}

		return;
		}

		CLandsatWindow window = static_cast<CLandsatWindow&>(bandHolder.GetWindow());

		if (m_options.m_bCreateImage)
		{
			Color8 dstNodata = (Color8)m_options.m_dstNodata;
			outputData.resize(3);
			for (size_t j = 0; j < outputData.size(); j++)
				outputData[j].resize(blockSize.m_x*blockSize.m_y, dstNodata);
		}

#pragma omp critical(ProcessBlock)
	{
		m_options.m_timerProcess.Start();

		//CTPeriod period = m_options.GetTTPeriod();
		//int nbSegment = period.GetNbRef();

#pragma omp parallel for num_threads( m_options.m_CPU ) if (m_options.m_bMulti) 
		for (int y = 0; y < blockSize.m_y; y++)
		{
			for (int x = 0; x < blockSize.m_x; x++)
			{
				//process all bands for the moment
				//for (size_t iz = 0; iz < window.GetNbScenes(); iz++)
				//{
					//Get pixel

				ASSERT(m_options.m_scene < bandHolder.GetNbScenes());
				CLandsatPixel pixel = window.GetPixel(m_options.m_scene, x, y);
				if (window.IsValid(m_options.m_scene, pixel))
				{
					bool bIsBlack = (pixel[B4] == 0 && pixel[B5] == 0 && pixel[B3] == 0);
					//bool bIsBust = (pixel[B4] < -150 || pixel[B4] > 6000 || pixel[B5] < -190 || pixel[B5] > 5000 || pixel[B3] < -200 || pixel[B3] > 2500);
					

					if (!bIsBlack )
					{
						Color8 R = Color8(max(0.0, min(254.0, ((pixel[B4] + 150.0) / 6150.0) * 254.0)));
						Color8 G = Color8(max(0.0, min(254.0, ((pixel[B5] + 190.0) / 5190.0) * 254.0)));
						Color8 B = Color8(max(0.0, min(254.0, ((pixel[B3] + 200.0) / 2700.0) * 254.0)));

						bool bIsBust = m_options.IsBusting(R, G, B);

						if (!bIsBust)
						{
							outputData[0][y*blockSize.m_x + x] = R;
							outputData[1][y*blockSize.m_x + x] = G;
							outputData[2][y*blockSize.m_x + x] = B;
						}
					}
					//"if (([1][1] < -190 ),0, if (([1][1] > 5000 ),254,(( ([1][1]/5192) * 253) + 9.307)))" %inpath%mrg57_%myy%_182 - 244_%myyear%_b5.tif %outpath%b5.tif
					//"if (( [1][1] < -200 ),0, if (([1][1] > 2500 ),254,(( ([1][1]/2702) * 253) + 18.821)))" %inpath%mrg57_%myy%_182 - 244_%myyear%_b3.tif %outpath%b3.tif
				}
				//}

#pragma omp atomic 
				m_options.m_xx++;

			}
		}

		m_options.UpdateBar();
		m_options.m_timerProcess.Stop();

		bandHolder.ReleaseBlocks();
	}



	}

	void CLandsat2RGB::WriteBlock(int xBlock, int yBlock, CBandsHolder& bandHolder, CGDALDatasetEx& outputDS, OutputData& outputData)
	{
#pragma omp critical(BlockIO)
	{
		m_options.m_timerWrite.Start();


		CGeoExtents extents = bandHolder.GetExtents();
		CGeoRectIndex outputRect = extents.GetBlockRect(xBlock, yBlock);
		CTPeriod period = m_options.GetTTPeriod();
		int nbSegment = period.GetNbRef();

		if (outputDS.IsOpen())
		{
			ASSERT(outputRect.m_x >= 0 && outputRect.m_x < outputDS.GetRasterXSize());
			ASSERT(outputRect.m_y >= 0 && outputRect.m_y < outputDS.GetRasterYSize());
			ASSERT(outputRect.m_xSize > 0 && outputRect.m_xSize <= outputDS.GetRasterXSize());
			ASSERT(outputRect.m_ySize > 0 && outputRect.m_ySize <= outputDS.GetRasterYSize());

			for (size_t z = 0; z < outputData.size(); z++)
			{
				GDALRasterBand *pBand = outputDS.GetRasterBand(z);
				if (!outputData[z].empty())
				{
					pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(outputData[z][0]), outputRect.Width(), outputRect.Height(), GDT_Byte, 0, 0);
				}
				else
				{
					Color8 noData = (Color8)outputDS.GetNoData(z);
					pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &noData, 1, 1, GDT_Byte, 0, 0);
				}
			}
		}


		m_options.m_timerWrite.Stop();
	}
	}

	void CLandsat2RGB::CloseAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS)
	{
		inputDS.Close();
		maskDS.Close();


		m_options.m_timerWrite.Start();

		if (m_options.m_bComputeStats)
			outputDS.ComputeStats(m_options.m_bQuiet);

		if (!m_options.m_overviewLevels.empty())
			outputDS.BuildOverviews(m_options.m_overviewLevels, m_options.m_bQuiet);

		outputDS.Close();

		m_options.m_timerWrite.Stop();
		m_options.PrintTime();
	}


}