//***********************************************************************
// program to merge Landsat image image over a period
//									 
//***********************************************************************
// version
// 3.0.0	31/10/2017	Rémi Saint-Amant	Compile with GDAL 2.0 and. Remove all cloud remover
// 2.1.3	27/05/2016	Rémi Saint-Amant	use JD -1 as no data
// 2.1.2	27/05/2016	Rémi Saint-Amant	Add option Mosaic
// 2.1.1	22/05/2016	Rémi Saint-Amant	Add option MaxSkip
// 2.1.0    09/05/2016  Rémi Saint-Amant	Compile with GDAL 1.11.3 adn WBSF with a new clouds New tree
// 2.0.1    16/03/2015  Rémi Saint-Amant	Don't flush cache
// 2.0.0	09/03/2015	Rémi Saint-Amant	Add clouds detection
// 1.7.0	05/03/2015	Rémi Saint-Amant	Add Landsat version in the debug layer
// 1.6.8	05/02/2015	Rémi Saint-Amant	Bug correction in UnionExtents and extents
// 1.6.7	30/01/2015	Rémi Saint-Amant	don't modify input VRT file. Bug correction in IntersectRect
// 1.6.6    27/01/2015	Rémi Saint-Amant	Compile with GDAL 1.11.1
// 1.6.5	23/12/2014	Rémi Saint-Amant	Add SecondBest type
// 1.6.4    25/10/2014	Rémi Saint-Amant	bug correction in help
// 1.6.3	11/09/2014	Rémi Saint-Amant	bugs correction with unprocess blocks
// 1.6.2	24/07/2014	Rémi Saint-Amant	Bugs corrections
// 1.6.1	09/07/2014	Rémi Saint-Amant	Add Row and Path in debug layer. Merge GDALDatasetEx and GDALTDataset
// 1.6.0	26/06/2014	Rémi Saint-Amant	Compilation with GDAL 1.11 and UNICODE and VC 2013
//											Remove used of file .scenes
// 1.4.5    19/11/2013	Rémi Saint-Amant	New compilation
// 1.4.4    05/08/2013	Rémi Saint-Amant	New compilation
// 1.4.3	29/05/2013	Rémi Saint-Amant	Somme corrections
// 1.4.2    25/05/2013  Rémi Saint-Amant	Add statistics.
// 1.4.1    15/05/2013  Rémi Saint-Amant	Output only in one file and save .scenes file
//											Add time transformation and build overview
// 1.4.0    15/05/2013  Rémi Saint-Amant	debug as Int32. add stats option.
// 1.3.8    01/05/2013  Rémi Saint-Amant	Add -YYYYMMDD option
// 1.3.7    01/05/2013  Rémi Saint-Amant	bug correction in BestPixel, Add NbLayer.
// 1.3.6    29/04/2013  Rémi Saint-Amant	bug correction, new algo for byYear
// 1.3.5    19/04/2013  Rémi Saint-Amant	template multiThread, new progress bar
// 1.3.4    18/04/2013  Rémi Saint-Amant	correction of bug in mask and parallel
// 1.3.3    16/04/2013	Rémi Saint-Amant	change in parallel
// 1.3.2    16/04/2013	Rémi Saint-Amant	change best pixel highest for lowest
// 1.3.1    12/04/2013  Rémi Saint-Amant	bug correction in write time
// 1.3.0    09/04/2013  Rémi Saint-Amant	New .vrt format with 9 layers by scene in input and 10 layer by scene as output. 
// 1.2.0	26/01/2013	Rémi Saint-Amant	Operation by block instead of by row.
// 1.1.1    24/01/2013	Rémi Saint-Amant	Optimization in time and extents of basic dataset
// 1.1.0    21/01/2013	Rémi Saint-Amant	Add maxNDVI and Debug
// 1.0.0	21/12/2012	Rémi Saint-Amant	Creation

//-1201530 8294610 -1199530 8297610 

//-debug -ExportStats -of VRT -co "compress=LZW" -co "tiled=YES" -co "BLOCKXSIZE=1024" -co "BLOCKYSIZE=1024" --config GDAL_CACHEMAX 4096  -overview {2,4,8,16} -stats -multi -IOCPU 3 -overwrite "U:\GIS\#documents\TestCodes\MergeImages\TestLandsat8\input\_578_2017.vrt" "U:\GIS\#documents\TestCodes\MergeImages\TestLandsat8\output\test_2017(new).vrt"
//-te 1644300 6506700 1645200 6507600 -of VRT -co "compress=LZW" -overview {2,4,8,16} -multi -IOCPU 2 -overwrite "U:\GIS\#documents\TestCodes\MergeImages\TestLandsat8\input\_578_2017.vrt" "U:\GIS\#documents\TestCodes\MergeImages\TestLandsat8\output\test_2017(new).vrt"


#include "stdafx.h"
#include <math.h>
#include <array>
#include <utility>
#include <iostream>
#include <boost\dynamic_bitset.hpp>

#include "MergeImages.h"
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
	const char* CMergeImages::VERSION = "3.0.0";
	const size_t CMergeImages::NB_THREAD_PROCESS = 2;
	static const int NB_TOTAL_STATS = CMergeImagesOption::NB_STATS*SCENES_SIZE;

	//*********************************************************************************************************************


	const short CMergeImagesOption::BANDS_STATS[NB_STATS] = { LOWEST, MEAN, STD_DEV, HIGHEST };
	const char* CMergeImagesOption::MERGE_TYPE_NAME[NB_MERGE_TYPE] = { "Oldest", "Newest", "MaxNDVI", "Best", "SecondBest", "MedianNDVI", "MedianNBR", "MedianNDMI"};
	const char* CMergeImagesOption::DEBUG_NAME[NB_DEBUG_BANDS] = { "captor", "path", "row", "year", "month", "day", "Jday", "nbImages", "Scene", "sort"};
	const char* CMergeImagesOption::STAT_NAME[NB_STATS] = { "lo", "mn", "sd", "hi" };

	short CMergeImagesOption::GetMergeType(const char* str)
	{
		short type = UNKNOWN;

		string tmp(str);
		for (int i = 0; i < NB_MERGE_TYPE; i++)
		{
			if (IsEqualNoCase(tmp, MERGE_TYPE_NAME[i]))
			{
				type = i;
				break;
			}
		}

		return type;
	}



	CMergeImagesOption::CMergeImagesOption()
	{

		m_mergeType = MEDIAN_NDVI;
		m_medianType = BEST_PIXEL;
		m_bDebug = false;
		m_bExportStats = false;
		m_scenesSize = SCENES_SIZE;
		m_TM = CTM::ANNUAL;

		m_appDescription = "This software merge all Landsat scenes (composed of " + to_string(SCENES_SIZE) + " bands) of an input images by selecting desired pixels.";

		static const COptionDef OPTIONS[] =
		{
			//{ "-TT", 1, "t", false, "The temporal transformation allow user to merge images in different time period segment. The available types are: OverallYears, ByYears, ByMonths and None. None can be use to subset part of the input image. ByYears and ByMonths merge the images by years or by months. ByYear by default." },
			{ "-Type", 1, "t", false, "Merge type criteria: Oldest, Newest, MaxNDVI, Best, SecondBest, MedianNDVI, MedianNBR or MedianNDMI. MedianNDVI by default." },
			{ "-MedianType", 1, "t", false, "Median merge type to select the right median image when the number of image is even. Can be: Oldest, Newest, MaxNDVI, Best, SecondBest. Best by default." },
			{ "-Debug", 0, "", false, "Export, for each output layer, the input temporal information." },
			{ "-ExportStats", 0, "", false, "Output exportStats (lowest, mean, SD, highest) of all bands" },
//			{ "-SceneSize", 1, "size", false, "Number of images associate per scene. 9 by default." },//overide scene size defenition
			{ "srcfile", 0, "", false, "Input image file path." },
			{ "dstfile", 0, "", false, "Output image file path." }
		};

		for (int i = 0; i < sizeof(OPTIONS) / sizeof(COptionDef); i++)
			AddOption(OPTIONS[i]);

		static const CIOFileInfoDef IO_FILE_INFO[] =
		{
			{ "Input Image", "srcfile", "", "ScenesSize(9)*nbScenes", "B1: Landsat band 1|B2: Landsat band 2|B3: Landsat band 3|B4: Landsat band 4|B5: Landsat band 5|B6: Landsat band 6|B7: Landsat band 7|QA: Image quality|Date: date of image(Julian day 1970 or YYYYMMDD format)|... for all scenes", "" },
			{ "Output Image", "dstfile", "Number of output periods", "ScenesSize(9)", "B1: Landsat band 1|B2: Landsat band 2|B3: Landsat band 3|B4: Landsat band 4|B5: Landsat band 5|B6: Landsat band 6|B7: Landsat band 7|QA: Image quality|Date: Date of the selected image(Julian day 1970 or YYYYMMDD format)|... for all scenes", "" },
			{ "Optional Output Image", "_stats", "Number of output periods", "SceneSize(9) x NbStats(4)", "B1Lowest: lowest value of the input image B1|B1Mean: mean of the input image B1|B1SD: standard deviation of input image B1|B1Highest: highest value of the input image B1|... for each bands of the scene", "" },
			{ "Optional Output Image", "_debug", "Number of output periods", "14", "Path: path number of satellite|Row: row number of satellite|Year: year|Month: month (1-12)|Day: day (1-31)|JDay: Julian day (1-366)|NbScenes: number of valid scene|sort:|Isolated:|Buffer:|NbTriggers:|nbSkips:" }
		};

		for (int i = 0; i < sizeof(IO_FILE_INFO) / sizeof(CIOFileInfoDef); i++)
			AddIOFileInfo(IO_FILE_INFO[i]);
	}

	ERMsg CMergeImagesOption::ParseOption(int argc, char* argv[])
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

	ERMsg CMergeImagesOption::ProcessOption(int& i, int argc, char* argv[])
	{
		ERMsg msg;


		if (IsEqual(argv[i], "-Type") && i < argc - 1)
		{
			m_mergeType = GetMergeType(argv[++i]);
			if (m_mergeType == UNKNOWN)
			{
				msg.ajoute("ERROR: Invalid -Type option: valid type are \"Oldest\", \"Newest\", \"MaxNDVI\", \"Best\", \"SecondBest\", \"MedianNDVI\", \"MedianNBR\", \"MedianNDMI\".");
			}
		}
		else if (IsEqual(argv[i], "-MedianType") && i < argc - 1)
		{
			m_medianType = GetMergeType(argv[++i]);
			if (m_mergeType > SECOND_BEST)
			{
				msg.ajoute("ERROR: Invalid -MedianType option: valid type are \"Oldest\", \"Newest\", \"MaxNDVI\", \"Best\", \"SecondBest\".");
			}
		}
		else if (IsEqual(argv[i], "-Debug"))
		{
			m_bDebug = true;
		}
		else if (IsEqual(argv[i], "-ExportStats"))
		{
			m_bExportStats = true;
		}
		else
		{
			//Look to see if it's a know base option
			msg = CBaseOptions::ProcessOption(i, argc, argv);
		}

		return msg;
	}




	string CMergeImages::GetBandFileTitle(const string& filePath, size_t b)
	{
		string str = GetFileTitle(filePath);

		/*if (m_options.m_TM.Mode() != CTM::OVERALL_YEARS)
		{
		CTPeriod p = m_options.GetTTSegment(s);
		p.Transform(m_options.m_TM);
		str += "_" + p.Begin().ToString().substr(2);
		}*/


		if (b < SCENES_SIZE)
			str += string("_") + CLandsatDataset::SCENE_NAME[b];

		return str;
	}

	ERMsg CMergeImages::Execute()
	{
		ERMsg msg;

		if (!m_options.m_bQuiet)
		{
			cout << "Output: " << m_options.m_filesPath[CMergeImagesOption::OUTPUT_FILE_PATH] << endl;
			cout << "From:   " << m_options.m_filesPath[CMergeImagesOption::INPUT_FILE_PATH] << endl;
			cout << "Type:   " << CMergeImagesOption::MERGE_TYPE_NAME[m_options.m_mergeType] << endl;
			if (m_options.m_mergeType>CMergeImagesOption::SECOND_BEST)
				cout << "MedianType:   " << CMergeImagesOption::MERGE_TYPE_NAME[m_options.m_medianType] << endl;

			if (!m_options.m_maskName.empty())
				cout << "Mask:   " << m_options.m_maskName << endl;
		}

		GDALAllRegister();

		CLandsatDataset inputDS;
		CGDALDatasetEx maskDS;
		CGDALDatasetEx outputDS;
		CGDALDatasetEx debugDS;
		CGDALDatasetEx statsDS;
		

		msg = OpenInput(inputDS, maskDS);

		if (msg)
			msg = OpenOutput(outputDS, debugDS, statsDS);

		
		CBandsHolderMT bandsHolder(1, m_options.m_memoryLimit, m_options.m_IOCPU, NB_THREAD_PROCESS);

		if (msg)
			msg = bandsHolder.Load(inputDS, m_options.m_bQuiet, m_options.GetExtents(), m_options.m_period);

		if (msg && maskDS.IsOpen())
			bandsHolder.SetMask(maskDS.GetSingleBandHolder(), m_options.m_maskDataUsed);

		
		if (!msg)
			return msg;

		
		if (!m_options.m_bQuiet && m_options.m_bCreateImage)
		{
			cout << "Create output images (" << outputDS.GetRasterXSize() << " C x " << outputDS.GetRasterYSize() << " R x " << outputDS.GetRasterCount() << " B) with " << m_options.m_CPU << " threads..." << endl;
		}

		CGeoExtents extents = bandsHolder.GetExtents();
		m_options.ResetBar(extents.m_xSize*extents.m_ySize);

		vector<pair<int, int>> XYindex = extents.GetBlockList();

		omp_set_nested(1);//for IOCPU
#pragma omp parallel for schedule(static, 1) num_threads( NB_THREAD_PROCESS ) if (m_options.m_bMulti) 
		for (int b = 0; b < (int)XYindex.size(); b++)
		{
			int thread = ::omp_get_thread_num();
			int xBlock = XYindex[b].first;
			int yBlock = XYindex[b].second;

			OutputData outputData;
			DebugData debugData;
			OutputData statsData;

			ReadBlock(xBlock, yBlock, bandsHolder[thread]);
			ProcessBlock(xBlock, yBlock, bandsHolder[thread], outputData, debugData, statsData, inputDS);
			WriteBlock(xBlock, yBlock, outputDS, debugDS, statsDS, outputData, debugData, statsData);
		}//for all blocks

		CloseAll(inputDS, maskDS, outputDS, debugDS, statsDS);

		return msg;
	}


	void CMergeImages::InitMemory(size_t sceneSize, CGeoSize blockSize, OutputData& outputData, DebugData& debugData, OutputData& statsData)
	{
		if (m_options.m_bCreateImage)
		{
			__int16 dstNodata = (__int16)m_options.m_dstNodata;

			if (outputData.empty())
				outputData.resize(SCENES_SIZE);

			for (size_t z = 0; z < outputData.size(); z++)
			{
				if (outputData[z].empty())
					outputData[z].resize(blockSize.m_y);

				for (size_t j = 0; j < outputData[z].size(); j++)
				{
					if (outputData[z][j].empty())
						outputData[z][j].resize(blockSize.m_x, dstNodata);
					else
						std::fill(outputData[z][j].begin(), outputData[z][j].end(), dstNodata);
				}
			}
		}

		if (m_options.m_bDebug)
		{
			__int32 dstNodata = (__int32)WBSF::GetDefaultNoData(GDT_Int32);

			if (debugData.empty())
				debugData.resize(CMergeImagesOption::NB_DEBUG_BANDS);

			for (size_t z = 0; z < debugData.size(); z++)
			{
				if (debugData[z].empty())
					debugData[z].resize(blockSize.m_y);

				for (size_t j = 0; j < debugData[z].size(); j++)
				{
					if (debugData[z][j].empty())
						debugData[z][j].resize(blockSize.m_x, dstNodata);
					else
						std::fill(debugData[z][j].begin(), debugData[z][j].end(), dstNodata);
				}
			}
		}

		if (m_options.m_bExportStats)
		{
			//we used no data of int32 instead
			__int16 dstNodata = (__int16)WBSF::GetDefaultNoData(GDT_Int16);

			if (statsData.empty())
				statsData.resize(sceneSize*CMergeImagesOption::NB_STATS);

			for (size_t z = 0; z < statsData.size(); z++)
			{
				if (statsData[z].empty())
					statsData[z].resize(blockSize.m_y);

				for (size_t j = 0; j < statsData[z].size(); j++)
				{
					if (statsData[z][j].empty())
						statsData[z][j].resize(blockSize.m_x, dstNodata);
					else
						std::fill(statsData[z][j].begin(), statsData[z][j].end(), dstNodata);
				}
			}
		}
	}


	ERMsg CMergeImages::OpenInput(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS)
	{
		ERMsg msg;

		if (!m_options.m_bQuiet)
			cout << endl << "Open input image..." << endl;

		CMergeImagesOption options(m_options);
		msg = inputDS.OpenInputImage(m_options.m_filesPath[CMergeImagesOption::INPUT_FILE_PATH], options);

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
		}
		
		if (msg && !m_options.m_maskName.empty())
		{
			if (!m_options.m_bQuiet)
				cout << "Open mask..." << endl;

			msg += maskDS.OpenInputImage(m_options.m_maskName);
		}



		return msg;
	}

	ERMsg CMergeImages::OpenOutput(CGDALDatasetEx& outputDS, CGDALDatasetEx& debugDS, CGDALDatasetEx& statsDS)
	{

		ERMsg  msg;

		if (m_options.m_bCreateImage)
		{
			//CTPeriod period = m_options.GetTTPeriod();

			CMergeImagesOption options(m_options);
			options.m_nbBands = SCENES_SIZE;

			if (!m_options.m_bQuiet)
			{
				cout << endl;
				cout << "Open output images..." << endl;
				cout << "    Size           = " << options.m_extents.m_xSize << " cols x " << options.m_extents.m_ySize << " rows x " << options.m_nbBands << " bands" << endl;
				cout << "    Extents        = X:{" << ToString(options.m_extents.m_xMin) << ", " << ToString(options.m_extents.m_xMax) << "}  Y:{" << ToString(options.m_extents.m_yMin) << ", " << ToString(options.m_extents.m_yMax) << "}" << endl;
			}

			string filePath = options.m_filesPath[CMergeImagesOption::OUTPUT_FILE_PATH];
			for (size_t b = 0; b < SCENES_SIZE; b++)
				options.m_VRTBandsName += GetBandFileTitle(filePath, b) + ".tif|";

			msg += outputDS.CreateImage(filePath, options);
		}


		if (msg && m_options.m_bDebug)
		{
			if (!m_options.m_bQuiet)
				cout << "Create debug images..." << endl;

			CMergeImagesOption options(m_options);
			options.m_outputType = GDT_Int32;
			options.m_nbBands = CMergeImagesOption::NB_DEBUG_BANDS;
			options.m_dstNodata = WBSF::GetDefaultNoData(GDT_Int32);


			string filePath = options.m_filesPath[CMergeImagesOption::OUTPUT_FILE_PATH];
			SetFileTitle(filePath, GetFileTitle(filePath) + "_debug");

			for (size_t b = 0; b < CMergeImagesOption::NB_DEBUG_BANDS; b++)
				options.m_VRTBandsName += GetBandFileTitle(filePath, -1) + "_" + CMergeImagesOption::DEBUG_NAME[b] + ".tif|";

			msg += debugDS.CreateImage(filePath, options);
		}

		if (msg && m_options.m_bExportStats)
		{
			if (!m_options.m_bQuiet)
				cout << "Create stats images..." << endl;

			CMergeImagesOption options = m_options;

			options.m_nbBands = NB_TOTAL_STATS;
			options.m_outputType = GDT_Int16;
			options.m_dstNodata = WBSF::GetDefaultNoData(GDT_Int16);

			string filePath = options.m_filesPath[CMergeImagesOption::OUTPUT_FILE_PATH];
			SetFileTitle(filePath, GetFileTitle(filePath) + "_stats");

			for (size_t b = 0; b < SCENES_SIZE; b++)
			{
				for (size_t ss = 0; ss < CMergeImagesOption::NB_STATS; ss++)
				{
					options.m_VRTBandsName += GetBandFileTitle(filePath, b) + "_" + CMergeImagesOption::STAT_NAME[ss] + ".tif|";
				}
			}

			msg += statsDS.CreateImage(filePath, options);
		}

		return msg;
	}

	void CMergeImages::ReadBlock(int xBlock, int yBlock, CBandsHolder& bandHolder)
	{
#pragma omp critical(BlockIO)
	{
		m_options.m_timerRead.Start();

		CGeoExtents extents = m_options.m_extents.GetBlockExtents(xBlock, yBlock);
		bandHolder.LoadBlock(extents, m_options.m_period);

		m_options.m_timerRead.Stop();
	}
	}

	void CMergeImages::ProcessBlock(int xBlock, int yBlock, CBandsHolder& bandHolder, OutputData& outputData, DebugData& debugData, StatData& statsData, CGDALDatasetEx& inputDS)
	{
		CGeoExtents extents = bandHolder.GetExtents();
		CGeoSize blockSize = extents.GetBlockSize(xBlock, yBlock);

		if (bandHolder.IsEmpty())
		{
			int nbCells = blockSize.m_x*blockSize.m_y;

#pragma omp atomic
			m_options.m_xx += nbCells;
			m_options.UpdateBar();

			return;
		}


#pragma omp critical(ProcessBlock)
		{

			CLandsatWindow window = static_cast<CLandsatWindow&>(bandHolder.GetWindow());

			InitMemory(bandHolder.GetSceneSize(), blockSize, outputData, debugData, statsData);
			m_options.m_timerProcess.Start();

			
#pragma omp parallel for num_threads( m_options.m_CPU ) if (m_options.m_bMulti ) 
			for (int y = 0; y < blockSize.m_y; y++)
			{
				for (int x = 0; x < blockSize.m_x; x++)
				{
					Test1Vector imageList1;
					Test1Vector imageList2;
					CStatisticVector stats(window.GetSceneSize());

					//process all bands
					for (size_t s = 0; s < window.GetNbScenes(); s++)
					{
						//Get pixel
						CLandsatPixel pixel = window.GetPixel(s, x, y);
						CTRef criterion1 = GetCriterion(pixel, m_options.m_mergeType);
						CTRef criterion2 = GetCriterion(pixel, m_options.m_medianType);

						if (criterion1.IsInit())
						{
							auto it = imageList1.find(criterion1);
							if (it == imageList1.end())
							{
								imageList1.insert(make_pair(criterion1, s));
								imageList2.insert(make_pair(criterion2, s));
							}
						}
	
						if (m_options.m_bExportStats)
						{
							for (size_t z = 0; z < pixel.size(); z++)
							{
								if (window.IsValid(s, z, pixel[z]))
									stats[z] += pixel[z];
							}
						}//if export

					}//iz

					//find selected image index
					size_t iz = get_iz(imageList1, m_options.m_mergeType, imageList2, m_options.m_medianType);
					if (iz != NOT_INIT)
					{
						if (m_options.m_bCreateImage)
						{
							CLandsatPixel pixel;
							if (window.GetPixel(iz, x, y, pixel))
							{
								for (size_t z = 0; z < SCENES_SIZE; z++)
									outputData[z][y][x] = (__int16)WBSF::LimitToBound(pixel[z], GDT_Int16);
							}
						}


						if (m_options.m_bDebug)
						{
							CLandsatPixel pixel = window.GetPixel(iz, x, y);
							CTRef TRef = m_options.GetTRef(int(pixel[JD]));

							int cap = -1;
							int col = -1;
							int row = -1;
							if (inputDS.IsVRT())
							{
								size_t iiz = iz*bandHolder.GetSceneSize() + JD;
								std::string name = GetFileTitle(inputDS.GetInternalName(iiz));
								if (name.size() >= 16)
								{
									int v = int(name[3] - '0');
									if (v == 5 || v == 7 || v == 8)
									{
										cap = v;
										col = ToInt(name.substr(10, 3));
										row = ToInt(name.substr(13, 3));
									}
								}
							}

							Test1Vector::const_iterator it = get_it(imageList1, m_options.m_mergeType, imageList2, m_options.m_medianType);
							debugData[CMergeImagesOption::D_CAPTOR][y][x] = cap;
							debugData[CMergeImagesOption::D_PATH][y][x] = col;
							debugData[CMergeImagesOption::D_ROW][y][x] = row;
							debugData[CMergeImagesOption::D_YEAR][y][x] = TRef.GetYear();
							debugData[CMergeImagesOption::D_MONTH][y][x] = int(TRef.GetMonth()) + 1;
							debugData[CMergeImagesOption::D_DAY][y][x] = int(TRef.GetDay()) + 1;
							debugData[CMergeImagesOption::D_JDAY][y][x] = int(m_options.GetTRefIndex(TRef));
							debugData[CMergeImagesOption::NB_IMAGES][y][x] = int(imageList1.size());
							debugData[CMergeImagesOption::SCENE][y][x] = (int)iz + 1;
							debugData[CMergeImagesOption::SORT_TEST][y][x] = it->first.GetRef();
						}

						if (m_options.m_bExportStats)
						{
							for (size_t z = 0; z < bandHolder.GetSceneSize(); z++)
								for (size_t ss = 0; ss < CMergeImagesOption::NB_STATS; ss++)
									if (stats[z][NB_VALUE]>0)
										statsData[z*CMergeImagesOption::NB_STATS + ss][y][x] = (__int16)WBSF::LimitToBound(stats[z][CMergeImagesOption::BANDS_STATS[ss]], GDT_Int16);
						}//if export stats
					}
#pragma omp atomic 
					m_options.m_xx++;

				}//for x
				m_options.UpdateBar();
			}//for y
			
			m_options.m_timerProcess.Stop();
			
		}//critical process
	}

	
	void CMergeImages::WriteBlock(int xBlock, int yBlock, CGDALDatasetEx& outputDS, CGDALDatasetEx& debugDS, CGDALDatasetEx& statsDS, OutputData& outputData, DebugData& debugData, StatData& statsData)
	{
#pragma omp critical(BlockIO)
	{
		m_options.m_timerWrite.Start();

		CGeoExtents extents = outputDS.GetExtents();
		CGeoRectIndex outputRect = extents.GetBlockRect(xBlock, yBlock);
		CTPeriod period = m_options.GetTTPeriod();
		int nbSegment = period.GetNbRef();


		if (outputDS.IsOpen())
		{
			ASSERT(outputRect.m_x >= 0 && outputRect.m_x < outputDS.GetRasterXSize());
			ASSERT(outputRect.m_y >= 0 && outputRect.m_y < outputDS.GetRasterYSize());
			ASSERT(outputRect.m_xSize > 0 && outputRect.m_xSize <= outputDS.GetRasterXSize());
			ASSERT(outputRect.m_ySize > 0 && outputRect.m_ySize <= outputDS.GetRasterYSize());
		
			for (size_t z = 0; z < SCENES_SIZE; z++)
			{
				GDALRasterBand *pBand = outputDS.GetRasterBand(z);
				if (!outputData.empty())
				{
					ASSERT(outputData.size() == SCENES_SIZE);
						
					for (int y = 0; y < outputRect.Height(); y++)
						pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y + y, outputRect.Width(), 1, &(outputData[z][y][0]), outputRect.Width(), 1, GDT_Int16, 0, 0);
				}
				else
				{
					__int16 noData = (__int16)outputDS.GetNoData(z);
					pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &noData, 1, 1, GDT_Int16, 0, 0);
				}
			}
		}


		if (debugDS.IsOpen())
		{
			for (size_t z = 0; z < CMergeImagesOption::NB_DEBUG_BANDS; z++)
			{
				GDALRasterBand *pBand = debugDS.GetRasterBand(z);//s*CMergeImagesOption::NB_DEBUG_BANDS + 
				if (!debugData.empty())
				{
					for (int y = 0; y < outputRect.Height(); y++)
						pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y + y, outputRect.Width(), 1, &(debugData[z][y][0]), outputRect.Width(), 1, GDT_Int32, 0, 0);
				}
				else
				{
					__int32 noData = (__int32)debugDS.GetNoData(z);
					pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(noData), 1, 1, GDT_Int32, 0, 0);
				}
			}
		}

		if (statsDS.IsOpen())
		{
			for (size_t z = 0; z < NB_TOTAL_STATS; z++)
			{
				GDALRasterBand *pBand = statsDS.GetRasterBand(z);
				if (!statsData.empty())
				{
					vector<__int16> tmp;
					tmp.reserve(outputRect.Width()*outputRect.Height());
						
					//for (int y = 0; y < outputRect.Height(); y++)
						//tmp.insert(tmp.end(), statsData[z][y].begin(), statsData[z][y].begin() + outputRect.Width());
					
					for (int y = 0; y < statsData[z].size(); y++)
						tmp.insert(tmp.end(), statsData[z][y].begin(), statsData[z][y].end() );

					pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(tmp[0]), outputRect.Width(), outputRect.Height(), GDT_Int16, 0, 0);
				}
				else
				{
					__int16 noData = (__int16)statsDS.GetNoData(z);
					pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &noData, 1, 1, GDT_Int16, 0, 0);
				}
			}//for all bands in a scene
		}//stats

		m_options.m_timerWrite.Stop();
	}
	}

	void CMergeImages::CloseAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS, CGDALDatasetEx& debugDS, CGDALDatasetEx& statsDS)
	{
		inputDS.Close();
		maskDS.Close();

		m_options.m_timerWrite.Start();

		if (m_options.m_bComputeStats)
			outputDS.ComputeStats(m_options.m_bQuiet);

		if (!m_options.m_overviewLevels.empty())
			outputDS.BuildOverviews(m_options.m_overviewLevels, m_options.m_bQuiet);

		outputDS.Close();

		if (m_options.m_bComputeStats)
			debugDS.ComputeStats(m_options.m_bQuiet);

		if (!m_options.m_overviewLevels.empty())
			debugDS.BuildOverviews(m_options.m_overviewLevels, m_options.m_bQuiet);

		debugDS.Close();

		if (m_options.m_bComputeStats)
			statsDS.ComputeStats(m_options.m_bQuiet);

		if (!m_options.m_overviewLevels.empty())
			statsDS.BuildOverviews(m_options.m_overviewLevels, m_options.m_bQuiet);

		statsDS.Close();

		m_options.m_timerWrite.Stop();
		m_options.PrintTime();
	}

	CTRef CMergeImages::GetCriterion(CLandsatPixel& pixel, size_t type)
	{
		CTRef criterion;

		if (pixel.IsValid() && int(pixel[JD]) >= 0)
		{
			CTRef TRef = CBaseOptions::GetTRef(CBaseOptions::JDAY1970, int(pixel[JD]));

			if (type == CMergeImagesOption::BEST_PIXEL ||
				type == CMergeImagesOption::SECOND_BEST)
			{
				bool bAdd = true;

				bool bIsBlack = (pixel[B4] == 0 && pixel[B5] == 0 && pixel[B3] == 0);
				if (bIsBlack)
					bAdd = false;

				//if (m_options.IsBusting(pixel.R(), pixel.G(), pixel.B()))
					//bAdd = false;

				//add image qualities
				if (bAdd)
				{
					//__int16 QA = CMergeImages::GetMeanQA(window, blockSize, s, x, y);
					__int16 Qa = pixel[QA];
					criterion.SetRef(Qa, CTM(CTM::ATEMPORAL));
				}
			}
			else if (type == CMergeImagesOption::MAX_NDVI || type == CMergeImagesOption::MEDIAN_NDVI)
			{
				long NDVI = (long)WBSF::LimitToBound(pixel.NDVI() * 1000, GDT_Int16);
				criterion.SetRef(NDVI, CTM(CTM::ATEMPORAL));
			}
			else if (type == CMergeImagesOption::MEDIAN_NBR)
			{
				long NBR = (long)WBSF::LimitToBound(pixel.NBR() * 1000, GDT_Int16);
				criterion.SetRef(NBR, CTM(CTM::ATEMPORAL));
			}
			else if (type == CMergeImagesOption::MEDIAN_NDMI)
			{
				long NDMI = (long)WBSF::LimitToBound(pixel.NDMI() * 1000, GDT_Int16);
				criterion.SetRef(NDMI, CTM(CTM::ATEMPORAL));
			}
			else
			{
				criterion = TRef;
			}
		}

		return criterion;
	}

	Test1Vector::const_iterator CMergeImages::get_it(const Test1Vector& imageList, size_t type)
	{
		Test1Vector::const_iterator it = imageList.end();
		if (!imageList.empty())
		{
			if (type == CMergeImagesOption::OLDEST)
			{
				it = imageList.begin();
			}
			else if (type == CMergeImagesOption::NEWEST)
			{
				it = imageList.end()--;
			}
			else if (type == CMergeImagesOption::MAX_NDVI)
			{
				it = imageList.end()--;
			}
			else if (type == CMergeImagesOption::BEST_PIXEL)
			{
				//take the pixel with the lowest score.
				it = imageList.begin();
			}
			else if (type == CMergeImagesOption::SECOND_BEST)
			{
				//take the pixel with the second lowest score.
				if (imageList.size() >= 2)
				{
					it = imageList.begin(); 
					it++;
				}
					
			}
			else
			{
				//select median
				size_t N = (imageList.size()+1) / 2 - 1;
				it = imageList.begin();
				for (size_t n = 0; n < N; n++)
					it++;
			}
		}

		return it;
	}
	
	Test1Vector::const_iterator CMergeImages::get_it(const Test1Vector& imageList1, size_t type1, const Test1Vector& imageList2, size_t type2)
	{
		ASSERT(imageList1.size() == imageList2.size());

		Test1Vector::const_iterator it = imageList1.end();
		if (!imageList1.empty())
		{
			it = get_it(imageList1, type1);

			if (type1 > CMergeImagesOption::SECOND_BEST &&
				imageList1.size() % 2 == 0)
			{
				Test1Vector::const_iterator it2 = it;

				Test1Vector imageList3;
				
				Test1Vector::const_iterator it3 = std::find_if(imageList2.begin(), imageList2.end(), [it2](const pair<CTRef, size_t >& a) {return a.second == it2->second;	});
				ASSERT(it3 != imageList2.end());

				imageList3.insert(make_pair(it3->first, 0));
				it2++;
				ASSERT(it2 != imageList1.end());
				it3 = std::find_if(imageList2.begin(), imageList2.end(), [it2](const pair<CTRef, size_t >& a) {return a.second == it2->second;	});
				ASSERT(it3 != imageList2.end());
				imageList3.insert(make_pair(it3->first, 1));
				
				it3 = get_it(imageList3, type2);
				ASSERT(it3 != imageList3.end());
				if (it3->second==1)
					it = it2;
			
			}
		}

		return it;
	}

	size_t CMergeImages::get_iz(const Test1Vector& imageList1, size_t type1, const Test1Vector& imageList2, size_t type2)
	{
		size_t iz = NOT_INIT;
		Test1Vector::const_iterator it = get_it(imageList1, type1, imageList2, type2);
		if(it != imageList1.end())
			iz=it->second;

		return iz;
	}


}