//***********************************************************************
// program to merge Landsat image image over a period
//									 
//***********************************************************************
// version
// 1.3.0	13/09/2023	R�mi Saint-Amant	Compile with GDAL 3.7.1
// 1.2.0	20/12/2021	R�mi Saint-Amant	Compile with VS 2019 and GDAL 3.0.3
// 1.1.3	22/02/2019	R�mi Saint-Amant	Add -corr8 options
// 1.1.2	29/09/2018	R�mi Saint-Amant	Add option -RemoveEmpty and -Rename 
// 1.1.1	22/05/2018	R�mi Saint-Amant	Compile with VS 2017
// 1.1.0	16/11/2017	R�mi Saint-Amant	Compile with GDAL 2.2
// 1.0.0	21/12/2015	R�mi Saint-Amant	Creation

//-te -1271940 7942380 -1249530 7956540 -of vrt -co "bigtiff=yes" -co "compress=LZW" -co "tiled=YES" -co "BLOCKXSIZE=1024" -co "BLOCKYSIZE=1024" -blocksize 1024 1024 -multi -Type SecondBest -stats -debug -dstnodata -32768 --config GDAL_CACHEMAX 1024 "U:\GIS1\LANDSAT_SR\LCC\2014\#57_2014_182-244.vrt" "U:\GIS\#documents\TestCodes\LandsatWarp\Test4\Nuage.vrt"
//-stats -Type BestPixel -te 1358100 6854400 1370300 6865500 -of VRT -ot Int16 -blockSize 1024 1024 -co "compress=LZW" -co "tiled=YES" -co "BLOCKXSIZE=1024" -co "BLOCKYSIZE=1024" --config GDAL_CACHEMAX 4096  -overview {2,4,8,16} -multi -IOCPU 3 -overwrite -Clouds "U:\GIS\#documents\TestCodes\LandsatWarp\Test2\Model\V4_SR_DTD1_cloudv4_skip100_200" "U:\GIS1\LANDSAT_SR\LCC\1999-2006.vrt" "U:\GIS\#documents\TestCodes\LandsatWarp\Test2\Output\Test.vrt"
//-stats -Type Oldest -TT OverallYears -of VRT -ot Int16 -blockSize 1024 1024 -co "compress=LZW" -co "tiled=YES" -co "BLOCKXSIZE=1024" -co "BLOCKYSIZE=1024" --config GDAL_CACHEMAX 4096  -overview {2,4,8,16} -multi -IOCPU 3 -overwrite "U:\GIS\#documents\TestCodes\BandsAnalyser\Test1\Input\Test1999-2014.vrt" "U:\GIS\#documents\TestCodes\LandsatWarp\Test0\output\Test.vrt"
//-RGB Natural -stats --config GDAL_CACHEMAX 4096  -overview {2,4,8,16} -multi -overwrite -of VRT -co "compress=LZW" -te 1718910 6620910 1751910 6652920 "D:\Travaux\MergeImage\input\2015-2016-2017.vrt" "D:\Travaux\MergeImage\input\subset\2015-2016-2017.vrt"


#include "stdafx.h"
#include <math.h>
#include <array>
#include <utility>
#include <iostream>

#include "LandsatWarp.h"
#include "Basic/OpenMP.h"
#include "Basic/UtilTime.h"
#include "Basic/UtilMath.h"

#pragma warning(disable: 4275 4251)
#include "gdal_priv.h"

using namespace std;
using namespace WBSF::Landsat1;

namespace WBSF
{


	const char* CLandsatWarp::VERSION = "1.3.0";
	const int CLandsatWarp::NB_THREAD_PROCESS = 2;


	//*********************************************************************************************************************

	CLandsatWarpOption::CLandsatWarpOption()
	{

		m_scenesSize = SCENES_SIZE;
		m_appDescription = "This software warp a series of Landsat images (composed of " + to_string(SCENES_SIZE) + " bands). All empty images will be removed.";

		AddOption("-Period");
		AddOption("-RGB");
		AddOption("-RemoveEmpty");
		

		static const COptionDef OPTIONS[] =
		{
			{ "-Rename", 1, "format", false, "Add at the end of output file, the mean image date. See strftime for option. %F for YYYY-MM-DD. Use %J for julian day since 1970 and %P for path/row." },//overide scene size defenition
			{ "-Corr8", 1, "type", false, "Make a correction over the landsat 8 images to get landsat 7 equivalent. The type can be \"Canada\", \"Australia\" or \"USA\"." },
			{ "srcfile", 0, "", false, "Input image file path." },
			{ "dstfile", 0, "", false, "Output image file path." }
		};

		for (int i = 0; i < sizeof(OPTIONS) / sizeof(COptionDef); i++)
			AddOption(OPTIONS[i]);


		RemoveOption("-ot");

		static const CIOFileInfoDef IO_FILE_INFO[] =
		{
			{ "Input Image", "srcfile", "", "ScenesSize(9)*nbScenes", "B1: Landsat band 1|B2: Landsat band 2|B3: Landsat band 3|B4: Landsat band 4|B5: Landsat band 5|B6: Landsat band 6|B7: Landsat band 7|QA: Image quality|JD: date of image(Julian day 1970 or YYYYMMDD format)|... for all scenes", "" },
			{ "Output Image", "dstfile", "", "ScenesSize(9)*nbScenes", "B1: Landsat band 1|B2: Landsat band 2|B3: Landsat band 3|B4: Landsat band 4|B5: Landsat band 5|B6: Landsat band 6|B7: Landsat band 7|QA: Image quality|JD: date of image(Julian day 1970 or YYYYMMDD format)|... for all scenes", "" },
		};

		for (int i = 0; i < sizeof(IO_FILE_INFO) / sizeof(CIOFileInfoDef); i++)
			AddIOFileInfo(IO_FILE_INFO[i]);
	}

	ERMsg CLandsatWarpOption::ParseOption(int argc, char* argv[])
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
			m_outputType = GDT_Int16;

		return msg;
	}

	ERMsg CLandsatWarpOption::ProcessOption(int& i, int argc, char* argv[])
	{
		ERMsg msg;


		if (IsEqual(argv[i], "-rename"))
		{
			m_rename = argv[++i];
		}
		else if (IsEqual(argv[i], "-corr8"))
		{
			m_corr8 = Landsat1::GetCorr8(argv[++i]);
			if (m_corr8 == NO_CORR8)
				msg.ajoute("Invalid -Corr8 type. Type can be \"Canada\", \"Australia\" or \"USA\"");
		}
		else
		{
			//Look to see if it's a know base option
			msg = CBaseOptions::ProcessOption(i, argc, argv);
		}

		return msg;
	}


	ERMsg CLandsatWarp::Execute()
	{
		ERMsg msg;

		if (!m_options.m_bQuiet)
		{
			cout << "Output: " << m_options.m_filesPath[CLandsatWarpOption::OUTPUT_FILE_PATH] << endl;
			cout << "From:   " << m_options.m_filesPath[CLandsatWarpOption::INPUT_FILE_PATH] << endl;

			if (!m_options.m_maskName.empty())
				cout << "Mask:   " << m_options.m_maskName << endl;
		}

		GDALAllRegister();

		CLandsatDataset inputDS;
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
		m_options.ResetBar((size_t)extents.m_xSize*extents.m_ySize);
		vector<pair<int, int>> XYindex = extents.GetBlockList(5, 5);

		vector < set<size_t>> imagesList(XYindex.size());


		if (!m_options.m_bQuiet)
		{
			cout << "Preprocess input images (" << inputDS.GetRasterXSize() << " C x " << inputDS.GetRasterYSize() << " R x " << inputDS.GetRasterCount() << " B) with " << m_options.m_CPU << " threads..." << endl;
		}

		omp_set_nested(1);//for IOCPU

		set<size_t> selected;

#pragma omp parallel for schedule(static, 1) num_threads( NB_THREAD_PROCESS ) if (m_options.m_bMulti) 
		for (int b = 0; b < (int)XYindex.size(); b++)
		{
			int xBlock = XYindex[b].first;
			int yBlock = XYindex[b].second;

			int thread = ::omp_get_thread_num();
			imagesList[b] = GetImageList(xBlock, yBlock, bandHolder[thread], inputDS);
			if (!imagesList[b].empty())
			{
#pragma omp critical(PreProcessBlock)
				selected.insert(imagesList[b].begin(), imagesList[b].end());
			}
		}//for all blocks



		CLandsatDataset outputDS;
		msg = OpenOutput(inputDS, outputDS, selected);


		if (!msg)
			return msg;



		if (!m_options.m_bQuiet && m_options.m_bCreateImage)
		{
			cout << "Create output images (" << outputDS.GetRasterXSize() << " C x " << outputDS.GetRasterYSize() << " R x " << outputDS.GetRasterCount() << " B) with " << m_options.m_CPU << " threads..." << endl;
		}

		set<size_t> nonEmpty;

		m_options.ResetBar((size_t)extents.m_xSize*extents.m_ySize);
#pragma omp parallel for schedule(static, 1) num_threads( NB_THREAD_PROCESS ) if (m_options.m_bMulti) 
		for (int b = 0; b < (int)XYindex.size(); b++)
		{
			int xBlock = XYindex[b].first;
			int yBlock = XYindex[b].second;

			int blockThreadNo = ::omp_get_thread_num();

			OutputData outputData;
			ReadBlock(xBlock, yBlock, bandHolder[blockThreadNo]);
			ProcessBlock(xBlock, yBlock, imagesList[b], bandHolder[blockThreadNo], outputData);
			WriteBlock(xBlock, yBlock, bandHolder[blockThreadNo], outputDS, outputData);
			if (!outputData.empty())
#pragma omp critical(PreProcessBlock)
				nonEmpty.insert(imagesList[b].begin(), imagesList[b].end());
		}//for all blocks


		CloseAll(inputDS, maskDS, outputDS);

		return msg;
	}



	ERMsg CLandsatWarp::OpenInput(CLandsatDataset& inputDS, CGDALDatasetEx& maskDS)
	{
		ERMsg msg;

		if (!m_options.m_bQuiet)
		{
			cout << endl << "Open input image..." << endl;
			if (m_options.m_period.IsInit())
				cout << "    User require loaded period    = " << m_options.m_period.GetFormatedString() << endl;
		}

		msg = inputDS.OpenInputImage(m_options.m_filesPath[CLandsatWarpOption::INPUT_FILE_PATH], m_options);
		if (msg)
			inputDS.UpdateOption(m_options);


		if (msg && !m_options.m_bQuiet)
		{
			CTPeriod loadedPeriod;
			size_t nbSceneLoaded = 0;
			const std::vector<CTPeriod>& p = inputDS.GetScenePeriod();

			for (size_t i = 0; i < p.size(); i++)
			{
				if (m_options.m_period.IsIntersect(p[i]))
				{
					loadedPeriod += p[i];
					nbSceneLoaded++;
				}
			}


			CGeoExtents extents = inputDS.GetExtents();
			CProjectionPtr pPrj = inputDS.GetPrj();
			string prjName = pPrj ? pPrj->GetName() : "Unknown";

			cout << "    Size           = " << inputDS.GetRasterXSize() << " cols x " << inputDS.GetRasterYSize() << " rows x " << inputDS.GetRasterCount() << " bands" << endl;
			cout << "    Extents        = X:{" << ToString(extents.m_xMin) << ", " << ToString(extents.m_xMax) << "}  Y:{" << ToString(extents.m_yMin) << ", " << ToString(extents.m_yMax) << "}" << endl;
			cout << "    Projection     = " << prjName << endl;
			cout << "    NbBands        = " << inputDS.GetRasterCount() << endl;
			cout << "    Scene size     = " << inputDS.GetSceneSize() << endl;
			//cout << "    Nb. scenes     = " << inputDS.GetNbScenes() << endl;
			//cout << "    First image    = " << inputDS.GetPeriod().Begin().GetFormatedString() << endl;
			//cout << "    Last image     = " << inputDS.GetPeriod().End().GetFormatedString() << endl;
			//cout << "    Input period   = " << inputDS.GetPeriod().GetFormatedString() << endl;
			cout << "    Entire period  = " << inputDS.GetPeriod().GetFormatedString() << " (nb scenes = " << inputDS.GetNbScenes() << ")" << endl;
			cout << "    Loaded period  = " << loadedPeriod.GetFormatedString() << " (nb scenes = " << nbSceneLoaded << ")" << endl;


			if (inputDS.GetSceneSize() != SCENES_SIZE)
				cout << FormatMsg("WARNING: the number of bands per scene (%1) is different than the inspected number (%2)", to_string(inputDS.GetSceneSize()), to_string(SCENES_SIZE)) << endl;
		}

		if (msg && !m_options.m_maskName.empty())
		{
			if (!m_options.m_bQuiet)
				cout << "Open mask..." << endl;

			msg += maskDS.OpenInputImage(m_options.m_maskName);
		}

		return msg;
	}

	ERMsg CLandsatWarp::OpenOutput(CLandsatDataset& inputDS, CLandsatDataset& outputDS, const set<size_t>& selected)
	{
		ERMsg msg;

		m_options.m_nbBands = selected.size()*SCENES_SIZE;
		if (m_options.m_bCreateImage)
		{
			CLandsatWarpOption options(m_options);
			options.m_outputType = GDT_Int16;

			if (!m_options.m_bQuiet)
			{
				cout << endl;
				cout << "Open output images..." << endl;
				cout << "    Size           = " << options.m_extents.m_xSize << " cols x " << options.m_extents.m_ySize << " rows x " << options.m_nbBands << " bands" << endl;
				cout << "    Extents        = X:{" << ToString(options.m_extents.m_xMin) << ", " << ToString(options.m_extents.m_xMax) << "}  Y:{" << ToString(options.m_extents.m_yMin) << ", " << ToString(options.m_extents.m_yMax) << "}" << endl;
				cout << "    Output period  = " << m_options.m_period.GetFormatedString() << endl;
			}

			string filePath = options.m_filesPath[CLandsatWarpOption::OUTPUT_FILE_PATH];

			bool bHaveDuplicate = false;
			set<string> subnames;
			for (auto it = selected.begin(); it != selected.end()&&!bHaveDuplicate; it++)
			{
				size_t i = *it;
				string subName = inputDS.GetSubname(i, m_options.m_rename);
				bHaveDuplicate = subnames.find(subName) != subnames.end();
				subnames.insert(subName);
			}
			subnames.clear();

			//replace the common part by the new name
			for (auto it = selected.begin(); it != selected.end(); it++)
			{
				size_t i = *it;
				
				string subName = inputDS.GetSubname(i, m_options.m_rename);
				
				size_t j = 1;
				string uniqueSubName = subName + (bHaveDuplicate ? ("-0" + to_string(j)) : "");
				while (subnames.find(uniqueSubName) != subnames.end())
					uniqueSubName = subName + (bHaveDuplicate?("-0" + to_string(++j)):"");

				subnames.insert(uniqueSubName);
				for (size_t b = 0; b < SCENES_SIZE; b++)
				{
					options.m_VRTBandsName += GetFileTitle(filePath) + "_" + uniqueSubName + "_" + Landsat1::GetBandName(b)+".tif|";
				}
			}

			msg += outputDS.CreateImage(filePath, options);

			//if (msg && !options.m_bQuiet)
			//{
				//CTPeriod period = options.GetTTPeriod();
				//cout << "    Output period:  " << period.GetFormatedString() << endl;
				//cout << endl;
			//}
		}


		return msg;
	}

	void CLandsatWarp::ReadBlock(int xBlock, int yBlock, CBandsHolder& bandHolder)
	{
#pragma omp critical(BlockIO)
		{
			m_options.m_timerRead.Start();

			CGeoExtents extents = m_options.m_extents.GetBlockExtents(xBlock, yBlock);
			bandHolder.LoadBlock(extents, m_options.m_period);

			m_options.m_timerRead.Stop();
		}
	}

	set<size_t> CLandsatWarp::GetImageList(int xBlock, int yBlock, CBandsHolder& bandHolder, CLandsatDataset& input)
	{
		set<size_t> scenes;

		const std::vector<CTPeriod>& scenesPeriod = input.GetScenePeriod();
		CTPeriod p = m_options.m_period;
		CGeoExtents extents = m_options.m_extents;

		size_t nbScenes = scenesPeriod.size();
		size_t scenesSize = m_options.m_scenesSize;

		for (size_t s = 0; s < nbScenes; s++)
		{
			bool bSkipScene = false;
			for (size_t z = 0; z < scenesSize; z++)
			{
				CGeoExtents ext = input.GetInternalExtents(s*scenesSize + z);
				bool include1 = extents.IsIntersect(ext);
				bool include2 = p.IsIntersect(scenesPeriod[s]);

				if (include1&&include2)//if one band of the scene is include we include the this image
				{
					scenes.insert(s);
					z = scenesSize;
				}
			}
		}

		return scenes;
	}

	void CLandsatWarp::ProcessBlock(int xBlock, int yBlock, set<size_t>& imagesList, CBandsHolder& bandHolder, OutputData& outputData)
	{

		CGeoExtents extents = bandHolder.GetExtents();
		CGeoSize blockSize = extents.GetBlockSize(xBlock, yBlock);

		if (bandHolder.IsEmpty())
		{
#pragma omp critical(ProcessBlock)
			{
				int nbCells = blockSize.m_x*blockSize.m_y;

#pragma omp atomic
				m_options.m_xx += nbCells;

				m_options.UpdateBar();
			}

			return;
		}

		CLandsatWindow window = static_cast<CLandsatWindow&>(bandHolder.GetWindow());
		window.m_corr8 = m_options.m_corr8;

		__int16 dstNodata = (__int16)m_options.m_dstNodata;
		outputData.resize(imagesList.size());
		for (size_t i = 0; i < outputData.size(); i++)
		{
			outputData[i].resize(SCENES_SIZE);
			for (size_t z = 0; z < outputData[i].size(); z++)
				outputData[i][z].resize(blockSize.m_x*blockSize.m_y, dstNodata);
		}



#pragma omp critical(ProcessBlock)
		{
			m_options.m_timerProcess.Start();

			for (int y = 0; y < blockSize.m_y; y++)
			{
				for (int x = 0; x < blockSize.m_x; x++)
				{
					for (auto it = imagesList.begin(); it != imagesList.end(); it++)
					{
						size_t i = std::distance(imagesList.begin(), it);
						CLandsatPixel pixel = window.GetPixel(*it, x, y);
						if (pixel[JD] == -1)
							pixel.Reset();//pixel[JD] = (__int16)(dstNodata);

						for (size_t z = 0; z < SCENES_SIZE; z++)
							outputData[i][z][y*blockSize.m_x + x] = pixel[z];
					}

#pragma omp atomic 
					m_options.m_xx++;
				}


				m_options.UpdateBar();
			}
			m_options.m_timerProcess.Stop();
		}
	}

	void CLandsatWarp::WriteBlock(int xBlock, int yBlock, CBandsHolder& bandHolder, CGDALDatasetEx& outputDS, OutputData& outputData)
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

				__int16 noData = (__int16)m_options.m_dstNodata;
				for (size_t s = 0; s < outputDS.GetRasterCount() / SCENES_SIZE; s++)
				{
					for (size_t z = 0; z < SCENES_SIZE; z++)
					{
						GDALRasterBand *pBand = outputDS.GetRasterBand(s*SCENES_SIZE + z);
						if (!outputData.empty())
							pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(outputData[s][z][0]), outputRect.Width(), outputRect.Height(), GDT_Int16, 0, 0);
						else
							pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &noData, 1, 1, GDT_Int16, 0, 0);
					}
				}
			}


			m_options.m_timerWrite.Stop();
		}
	}

	void CLandsatWarp::CloseAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS)
	{
		inputDS.Close();
		maskDS.Close();

		m_options.m_timerWrite.Start();

		outputDS.Close(m_options);

		m_options.m_timerWrite.Stop();
		m_options.PrintTime();
	}


}