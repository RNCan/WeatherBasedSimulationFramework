//***********************************************************************
// program to merge Landsat image image over a period
//									 
//***********************************************************************
// version
// 2.2.0	30/10/2017	Rémi Saint-Amant	Compile with GDAL 2.0 and add 
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

//--config GDAL_CACHEMAX 4096  -IOCPU 4 -co "bigtiff=yes" -co "tiled=YES" -co "BLOCKXSIZE=1024" -co "BLOCKYSIZE=1024" -overview {2,4,8,16} -multi -stats -co "compress=LZW" -overwrite -Clouds "C:\Geomatique\model\Clouds" "C:\Geomatique\Input\mosaic_subset.vrt" "C:\geomatique\1996\1996subset.vrt" "C:\geomatique\output\1996_test.tif"
//-te -1271940 7942380 -1249530 7956540 -of vrt -co "bigtiff=yes" -co "compress=LZW" -co "tiled=YES" -co "BLOCKXSIZE=1024" -co "BLOCKYSIZE=1024" -blocksize 1024 1024 -multi -Type SecondBest -stats -debug -dstnodata -32768 --config GDAL_CACHEMAX 1024 "U:\GIS1\LANDSAT_SR\LCC\2014\#57_2014_182-244.vrt" "U:\GIS\#documents\TestCodes\MergeImages\Test4\Nuage.vrt"
//-stats -Type BestPixel -te 1358100 6854400 1370300 6865500 -of VRT -ot Int16 -blockSize 1024 1024 -co "compress=LZW" -co "tiled=YES" -co "BLOCKXSIZE=1024" -co "BLOCKYSIZE=1024" --config GDAL_CACHEMAX 4096  -overview {2,4,8,16} -multi -IOCPU 3 -overwrite -Clouds "U:\GIS\#documents\TestCodes\MergeImages\Test2\Model\V4_SR_DTD1_cloudv4_skip100_200" "U:\GIS1\LANDSAT_SR\LCC\1999-2006.vrt" "U:\GIS\#documents\TestCodes\MergeImages\Test2\Output\Test.vrt"
//-stats -Type Oldest -TT OverallYears -of VRT -ot Int16 -blockSize 1024 1024 -co "compress=LZW" -co "tiled=YES" -co "BLOCKXSIZE=1024" -co "BLOCKYSIZE=1024" --config GDAL_CACHEMAX 4096  -overview {2,4,8,16} -multi -IOCPU 3 -overwrite "U:\GIS\#documents\TestCodes\BandsAnalyser\Test1\Input\Test1999-2014.vrt" "U:\GIS\#documents\TestCodes\MergeImages\Test0\output\Test.vrt"

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
	const char* CMergeImages::VERSION = "2.2.0";
	const size_t CMergeImages::NB_THREAD_PROCESS = 2;
	static const int NB_TOTAL_STATS = CMergeImagesOption::NB_STATS*SCENES_SIZE;



	//*********************************************************************************************************************


	const short CMergeImagesOption::BANDS_STATS[NB_STATS] = { LOWEST, MEAN, STD_DEV, HIGHEST };
	const char* CMergeImagesOption::MERGE_TYPE_NAME[NB_MERGE_TYPE] = { "Oldest", "Newest", "MaxNDVI", "Best", "SecondBest", "MedianNDVI", "MedianNBR", "MedianNDMI"};
	const char* CMergeImagesOption::DEBUG_NAME[NB_DEBUG_BANDS] = { "captor", "path", "row", "year", "month", "day", "Jday", "nbImages", "Scene", "sort", "Isolated", "Buffer", "NbTriggers", "nbSkips" };
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

		m_mergeType = BEST_PIXEL;
		m_bDebug = false;
		m_bExportStats = false;
		m_bust = { { 0, 255 } };
		m_clear = { { 0, 0 } };
		m_buffer = 0;
		m_scenesSize = SCENES_SIZE;
		m_TM = CTM::ANNUAL;
		m_nbPixelDT = 0;
		m_nbPixel = 0;
		m_QA = 0;
		m_QAmiss = 100;
		m_meanDmax=-1;
		m_maxSkip = NOT_INIT;

		m_appDescription = "This software merge all Landsat scenes (composed of " + to_string(SCENES_SIZE) + " bands) of an input images by selecting desired pixels.";

		static const COptionDef OPTIONS[] =
		{
			//{ "-TT", 1, "t", false, "The temporal transformation allow user to merge images in different time period segment. The available types are: OverallYears, ByYears, ByMonths and None. None can be use to subset part of the input image. ByYears and ByMonths merge the images by years or by months. ByYear by default." },
			{ "-Type", 1, "t", false, "Merge type criteria: Oldest, Newest, MaxNDVI, Best, SecondBest, MedianNDVI, MedianNBR or MedianNDMI. Best by default." },
			{ "-Clouds", 1, "file", false, "Decision tree model file path to remove clouds." },
			{ "-MaxSkip", 1, "nb", false, "Maximum number of skip image when removing clouds." },
			//{ "-NoDefautTrigger", 0, "", false, "Without this option, \"B1 -125\" and \"TCB 750\" is added to trigger to be used with the \"-Clouds\" options." },
			//{ "-Trigger", 3, "m tt th", true, "Add optimization trigger to execute decision tree when comparing T1 with T2. m is the merge method (can be \"OR\" or \"AND\"), tt is the trigger type and th is the trigger threshold. Supported type are \"B1\"..\"B9\",\"NBR\",\"EUCLIDEAN\", \"NDVI\", \"NDMI\", \"TCB\" (Tasseled Cap Brightness), \"TCG\" (Tasseled Cap Greenness) or \"TCW\" (Tasseled Cap Wetness)." },
			{ "-Mosaic", 1, "file", false, "Mosaic image file path to trigger DT when remove clouds." },
			{ "-Pre", 1, "file", false, "first mosaic image file path to trigger DT when remove clouds." },
			{ "-Pos", 1, "file", false, "second mosaic image file path to trigger DT when remove clouds." },
			{ "-Clear", 2, "NbPix  maxCount", false, "Don't replace isolated cloud pixel if the count of cloud pixel around (nbPix level) the pixel is equal or lesser than maxCount." },
			{ "-Buffer", 1, "NbPix ", false, "Grow decision tree cloud by nbPix." },
			{ "-Bust", 2, "min max", false, "replace busting pixel (lesser than min or greather than max) by no data." },
			{ "-QA", 2, "nbPix miss", false, "add nbPix buffer arround the pixel to compute QA and missing QA will be replace by miss." },
//			{ "-Mean", 1, "Dmax", false, "Compute mean of the 2 first pixels if the normalized visual distance (RGB) is lesser than Dmax" },
			//{ "-DistanceMin", 1, "d", false, "Minimum spectral distance [T1,T2] to call clouds decision tree. 250 is used by default." },
			{ "-Debug", 0, "", false, "Export, for each output layer, the input temporal information." },
			{ "-ExportStats", 0, "", false, "Output exportStats (lowest, mean, SD, highest) of all bands" },
			{ "-SceneSize", 1, "size", false, "Number of images associate per scene. 9 by default." },//overide scene size defenition
			{ "srcfile", 0, "", false, "Input image file path." },
			{ "dstfile", 0, "", false, "Output image file path." }
		};

		for (int i = 0; i < sizeof(OPTIONS) / sizeof(COptionDef); i++)
			AddOption(OPTIONS[i]);


		//Pour les trigger Bande 1 c’est - 125 quand on fait  ex.b1 1994 – b1 1995 ou b1 1996 – b1 1995.
		//Pour le tassel Cap brightness c’est + 750  ex.tcb1994 – tcb 1995 ou tcb 1996 – tcb 1995


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
			m_outputType = m_TTF == YYYYMMDD ? GDT_Int32 : GDT_Int16;


		//if (!m_bNoDefaultTrigger)
		//{
		//	m_triggerB1.push_back(CIndices(I_B1, -125.0, M_AND));
		//	m_triggerTCB.push_back(CIndices(I_TCB, 750.0, M_AND));
		//}


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
		else if (IsEqual(argv[i], "-MaxSkip"))
		{
			m_maxSkip = atoi(argv[++i]);
		}
		//else if (IsEqual(argv[i], "-Trigger"))
		//{
		//	string str = argv[++i];
		//	TIndices type = GetIndicesType(str);
		//	double threshold = atof(argv[++i]);
		//	if (type < NB_INDICES)
		//		m_trigger.push_back(CIndices(type, threshold, Landsat::M_AND));
		//	else
		//		msg.ajoute(str + " is an invalid trigger for -trigger option");
		//}
		else if (IsEqual(argv[i], "-Clouds"))
		{
			m_cloudsCleanerModel = argv[++i];
		}
		else if (IsEqual(argv[i], "-Pre"))
		{
			m_mosaicFilePath[0] = argv[++i];
		}
		else if (IsEqual(argv[i], "-Pos"))
		{
			m_mosaicFilePath[1] = argv[++i];
		}
		else if (IsEqual(argv[i], "-Mosaic"))
		{
			m_mosaicFilePath[2] = argv[++i];
		}
		else if (IsEqual(argv[i], "-Clear"))
		{
			m_clear[0] = ToInt(argv[++i]);
			m_clear[1] = ToInt(argv[++i]);
		}
		else if (IsEqual(argv[i], "-Buffer"))
		{
			m_buffer= ToInt(argv[++i]);
		}
		else if (IsEqual(argv[i], "-Bust"))
		{
			m_bust[0] = ToInt(argv[++i]);
			m_bust[1] = ToInt(argv[++i]);
		}
		else if (IsEqual(argv[i], "-QA"))
		{
			m_QA = ToInt(argv[++i]);
			m_QAmiss = ToInt(argv[++i]);
		}
		else if (IsEqual(argv[i], "-Mean"))
		{
			m_meanDmax = ToDouble(argv[++i]);
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

			if (!m_options.m_maskName.empty())
				cout << "Mask:   " << m_options.m_maskName << endl;

			if (!m_options.m_cloudsCleanerModel.empty())
				cout << "Clouds: " << m_options.m_cloudsCleanerModel << endl;

			if (!m_options.m_mosaicFilePath[0].empty())
				cout << "pre: " << m_options.m_mosaicFilePath[0] << endl;
			if (!m_options.m_mosaicFilePath[1].empty())
				cout << "pos: " << m_options.m_mosaicFilePath[1] << endl;
			if (!m_options.m_mosaicFilePath[2].empty())
				cout << "mosaic: " << m_options.m_mosaicFilePath[2] << endl;
		}

		GDALAllRegister();

		CLandsatDataset inputDS;
		CGDALDatasetEx maskDS;
		array<CLandsatDataset,3> mosaicDS;
		CLandsatCloudCleaner cloudsCleaner;//Decision Tree
		CBandsHolderMT inputBH(1, m_options.m_memoryLimit, m_options.m_IOCPU, NB_THREAD_PROCESS);
		array<CBandsHolderMT, 3> mosaicBH = { { { 1, 0, m_options.m_IOCPU, NB_THREAD_PROCESS }, { 1, 0, m_options.m_IOCPU, NB_THREAD_PROCESS }, { 1, 0, m_options.m_IOCPU, NB_THREAD_PROCESS } } };
		
		//(1, 0, m_options.m_IOCPU, NB_THREAD_PROCESS);
		//CBandsHolderMT mosaicBH2(1, 0, m_options.m_IOCPU, NB_THREAD_PROCESS);
	

		CGDALDatasetEx outputDS;
		CGDALDatasetEx debugDS;
		CGDALDatasetEx statsDS;

		msg = OpenInput(inputDS, maskDS, mosaicDS);
		if (msg && maskDS.IsOpen())
			inputBH.SetMask(maskDS.GetSingleBandHolder(), m_options.m_maskDataUsed);


		if (msg)
			msg = OpenOutput(outputDS, debugDS, statsDS);

		if (msg)
			msg = inputBH.Load(inputDS, m_options.m_bQuiet, m_options.GetExtents(), m_options.m_period);

		for (size_t i = 0; i < mosaicDS.size(); i++)
			if (msg && mosaicDS[i].IsOpen())
				msg = mosaicBH[i].Load(mosaicDS[i], true, m_options.GetExtents());
		
		//if (msg && mosaicDS2.IsOpen())
			//msg = mosaicBH2.Load(mosaicDS2, true, m_options.GetExtents());

		if (msg && !m_options.m_cloudsCleanerModel.empty())
			cloudsCleaner.Load(m_options.m_cloudsCleanerModel, m_options.m_CPU, m_options.m_IOCPU);

		if (!msg)
			return msg;

		if (!m_options.m_bQuiet && m_options.m_bCreateImage)
		{
			cout << "Create output images (" << outputDS.GetRasterXSize() << " C x " << outputDS.GetRasterYSize() << " R x " << outputDS.GetRasterCount() << " B) with " << m_options.m_CPU << " threads..." << endl;
		}

		CGeoExtents extents = inputBH.GetExtents();


		omp_set_nested(1);//for IOCPU

		int nbPass = 1;
		if (!cloudsCleaner.empty() && m_options.m_clear[0] > 0)
			nbPass++;
		if (!cloudsCleaner.empty() && m_options.m_buffer > 0)
			nbPass += m_options.m_buffer;

		m_options.ResetBar(extents.m_xSize*extents.m_ySize*nbPass);

		std::array< OutputData, NB_THREAD_PROCESS> outputData;
		std::array< DebugData, NB_THREAD_PROCESS> debugData;
		std::array< OutputData, NB_THREAD_PROCESS> statsData;
		for (size_t i = 0; i < NB_THREAD_PROCESS; i++)
			InitMemory(inputBH.GetSceneSize(), extents.GetBlockSize(), outputData[i], debugData[i], statsData[i]);

		vector<pair<int, int>> XYindex = extents.GetBlockList(10, 10);

#pragma omp parallel for schedule(static, 1) num_threads( NB_THREAD_PROCESS ) if (m_options.m_bMulti) 
		for (int b = 0; b < (int)XYindex.size(); b++)
		{
			int xBlock = XYindex[b].first;
			int yBlock = XYindex[b].second;

			if ((yBlock % 10)==0 && xBlock==0)//faudrait flusher juste les image qui ne seront plus utiliser
			{
				CGeoExtents extents = m_options.m_extents.GetBlockExtents(xBlock, yBlock);
				inputDS.FlushCache(extents.m_yMax);
				inputBH.FlushCache(extents.m_yMax);
					
				for (size_t i = 0; i < mosaicDS.size(); i++)
				{
					if (mosaicDS[i].IsOpen())
					{
						mosaicDS[i].FlushCache(extents.m_yMax);
						mosaicBH[i].FlushCache(extents.m_yMax);
					}
				}
			}

		
			int thNo = ::omp_get_thread_num();
			array<CBandsHolder*, 3> BH = { { &(mosaicBH[0][thNo]), &(mosaicBH[1][thNo]), &(mosaicBH[2][thNo]) } };


			ReadBlock(xBlock, yBlock, inputBH[thNo], BH);
			ProcessBlock(xBlock, yBlock, inputBH[thNo], inputDS, BH, outputDS, outputData[thNo], debugData[thNo], statsData[thNo], cloudsCleaner);
			WriteBlock(xBlock, yBlock, inputBH[thNo], outputDS, debugDS, statsDS, outputData[thNo], debugData[thNo], statsData[thNo]);
		}//for all blocks

		CloseAll(inputDS, maskDS, mosaicDS, outputDS, debugDS, statsDS);

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


	ERMsg CMergeImages::OpenInput(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, array<CLandsatDataset,3>& mosaicDS)
	{
		ERMsg msg;

		if (!m_options.m_bQuiet)
			cout << endl << "Open input image..." << endl;

		CMergeImagesOption options(m_options);
		msg = inputDS.OpenInputImage(m_options.m_filesPath[CMergeImagesOption::INPUT_FILE_PATH], options);

		for (size_t i = 0; i < 3; i++)
		{
			if (msg && !m_options.m_mosaicFilePath[i].empty())
			{
				CMergeImagesOption options(m_options);
				msg += mosaicDS[i].OpenInputImage(m_options.m_mosaicFilePath[i], options);
				
			}
		}

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
		
		if (msg && !m_options.m_bQuiet && mosaicDS[2].IsOpen())
		{
			CGeoExtents extents = inputDS.GetExtents();
			CProjectionPtr pPrj = inputDS.GetPrj();
			string prjName = pPrj ? pPrj->GetName() : "Unknown";

			cout << "mosaic" << endl;
			cout << "    Size           = " << mosaicDS[2].GetRasterXSize() << " cols x " << mosaicDS[2].GetRasterYSize() << " rows x " << mosaicDS[2].GetRasterCount() << " bands" << endl;
			cout << "    Extents        = X:{" << ToString(extents.m_xMin) << ", " << ToString(extents.m_xMax) << "}  Y:{" << ToString(extents.m_yMin) << ", " << ToString(extents.m_yMax) << "}" << endl;
			cout << "    Projection     = " << prjName << endl;
			cout << "    NbBands        = " << mosaicDS[2].GetRasterCount() << endl;
			cout << "    Scene size     = " << mosaicDS[2].GetSceneSize() << endl;
			cout << "    Nb. scenes     = " << mosaicDS[2].GetNbScenes() << endl;
			cout << "    First image    = " << mosaicDS[2].GetPeriod().Begin().GetFormatedString() << endl;
			cout << "    Last image     = " << mosaicDS[2].GetPeriod().End().GetFormatedString() << endl;

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

	void CMergeImages::ReadBlock(int xBlock, int yBlock, CBandsHolder& inputBH, array<CBandsHolder*, 3>& mosaicBH)
	{
#pragma omp critical(BlockIO)
	{
		m_options.m_timerRead.Start();

		CGeoExtents extents = m_options.m_extents.GetBlockExtents(xBlock, yBlock);
		inputBH.LoadBlock(extents, m_options.m_period);

		//CTPeriod p = m_options.m_period;
		//CTPeriod period(CTRef(p.Begin().GetYear() - 1, FIRST_MONTH, FIRST_DAY), CTRef(p.End().GetYear() + 1, LAST_MONTH, LAST_DAY));
		
		for (size_t i = 0; i < mosaicBH.size(); i++)
			mosaicBH[i]->LoadBlock(extents);
		

		m_options.m_timerRead.Stop();
	}
	}

	void CMergeImages::ProcessBlock(int xBlock, int yBlock, CBandsHolder& bandHolder, CGDALDatasetEx& inputDS, array<CBandsHolder*,3>& mosaicBH, CGDALDatasetEx& outputDS, OutputData& outputData, DebugData& debugData, StatData& statsData, CLandsatCloudCleaner& cloudsCleaner)
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

			//lansat pixel adn DTCode
			CMatrix<size_t> firstChoice;
			CMatrix<size_t> selectedChoice;
			//CMatrix<size_t> secondBest;
			//CMatrix<int> DTCode(blockSize.m_y, blockSize.m_x);
			
			CLandsatWindow preWindow;
			CLandsatWindow posWindow;
			CLandsatWindow mosaicWindow;


			if (!mosaicBH[0]->IsEmpty())
				preWindow = static_cast<CLandsatWindow&>(mosaicBH[0]->GetWindow());
			
			if (!mosaicBH[1]->IsEmpty())
				posWindow = static_cast<CLandsatWindow&>(mosaicBH[1]->GetWindow());

			if (!mosaicBH[2]->IsEmpty())
				mosaicWindow = static_cast<CLandsatWindow&>(mosaicBH[2]->GetWindow());
				


			if (!cloudsCleaner.empty() && (m_options.m_clear[0] > 0 || m_options.m_buffer > 0))
			{
				firstChoice.resize(blockSize.m_y, blockSize.m_x);
				selectedChoice.resize(blockSize.m_y, blockSize.m_x);
			}
			
#pragma omp parallel for num_threads( m_options.m_CPU ) if (m_options.m_bMulti ) 
			for (int y = 0; y < blockSize.m_y; y++)
			{
				for (int x = 0; x < blockSize.m_x; x++)
				{
					int nbTriggerUsed = 0;
					int nbSkip = 0;
					Test1Vector imageList;
					CTRefSet Treference;
					CStatisticVector stats(window.GetSceneSize());

					//process all bands
					for (size_t s = 0; s < window.GetNbScenes(); s++)
					{
						//Get pixel
						CLandsatPixel pixel = window.GetPixel(s, x, y);
						if (window.IsValid(s, pixel) && int(pixel[JD])>=0)
						{
							bool bValid = true;

							CTRef TRef = m_options.GetTRef(int(pixel[JD]));
							CTRef TRefQA;
							if (m_options.m_mergeType == CMergeImagesOption::BEST_PIXEL ||
								m_options.m_mergeType == CMergeImagesOption::SECOND_BEST)
							{
								bool bAdd = true;
							//	if ((m_options.m_mergeType == CMergeImagesOption::SECOND_BEST || (CMergeImagesOption::BEST_PIXEL&&m_options.m_maxSkip == 1) ) && !imageList.empty())
								//{
								if (Treference.find(TRef) != Treference.end())
									bAdd = false;
								//}

								bool bIsBlack = (pixel[B4] == 0 && pixel[B5] == 0 && pixel[B3] == 0);
								if (bIsBlack)
									bAdd = false;

								if (m_options.IsBusting(pixel.R(), pixel.G(), pixel.B()))
									bAdd = false;

								//add image qualities
								if (bAdd)
								{
									Treference.insert(TRef);
									__int16 QA = CMergeImages::GetMeanQA(window, blockSize, s, x, y);
									TRefQA.SetRef(QA, CTM(CTM::ATEMPORAL));
								}
							}
							else if (m_options.m_mergeType == CMergeImagesOption::MAX_NDVI || m_options.m_mergeType == CMergeImagesOption::MEDIAN_NDVI)
							{
								long NDVI = (long)WBSF::LimitToBound(pixel.NDVI() * 1000, m_options.m_outputType);
								TRefQA.SetRef(NDVI, CTM(CTM::ATEMPORAL));
							}
							else if (m_options.m_mergeType == CMergeImagesOption::MEDIAN_NBR)
							{
								long NBR = (long)WBSF::LimitToBound(pixel.NBR() * 1000, m_options.m_outputType);
								TRefQA.SetRef(NBR, CTM(CTM::ATEMPORAL));
							}
							else if (m_options.m_mergeType == CMergeImagesOption::MEDIAN_NDMI)
							{
								long NDMI = (long)WBSF::LimitToBound(pixel.NDMI() * 1000, m_options.m_outputType);
								TRefQA.SetRef(NDMI, CTM(CTM::ATEMPORAL));
							}
							else
							{
								TRefQA = TRef;
							}

							if (TRefQA.IsInit())
								imageList.insert(make_pair(TRefQA, s));
							

#pragma omp atomic
							m_options.m_nbPixel++;

						}//if valid

						if (m_options.m_bExportStats)
						{
							for (size_t z = 0; z < pixel.size(); z++)
							{
								if (window.IsValid(s, z, pixel[z]))
									stats[z] += pixel[z];
							}
						}//if export

					}//iz

					if (!imageList.empty())
						m_options.m_nbImages += (double)imageList.size();

					//find input temporal index
					size_t iz = get_iz(imageList, m_options.m_mergeType);

					//get first choice before erase
					if (!firstChoice.empty())
						firstChoice[y][x] = iz;

					//clean clouds
					if (!cloudsCleaner.empty())
					{
						iz = GetCloudFreeIz(imageList, m_options.m_mergeType, preWindow, window, posWindow, mosaicWindow, x, y, cloudsCleaner, nbTriggerUsed, nbSkip);
					}//if remove clouds

					//get first choice before erase
					if (!selectedChoice.empty())
						selectedChoice[y][x] = iz;//clouds was remove

					//if (!secondBest.empty())
					//{
					//	secondBest[y][x] = iz²;
					//} 
					//	

					if (m_options.m_bDebug)
					{
						
						if (iz != NOT_INIT)
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
									int v = int(name[2] - '0');
									if (v == 5 || v == 7 || v == 8)
									{
										cap = v;
										col = ToInt(name.substr(3, 3));
										row = ToInt(name.substr(6, 3));
									}
								}
							}

							Test1Vector::iterator it = get_it(imageList, m_options.m_mergeType);
							debugData[CMergeImagesOption::D_CAPTOR][y][x] = cap;
							debugData[CMergeImagesOption::D_PATH][y][x] = col;
							debugData[CMergeImagesOption::D_ROW][y][x] = row;
							debugData[CMergeImagesOption::D_YEAR][y][x] = TRef.GetYear();
							debugData[CMergeImagesOption::D_MONTH][y][x] = int(TRef.GetMonth()) + 1;
							debugData[CMergeImagesOption::D_DAY][y][x] = int(TRef.GetDay()) + 1;
							debugData[CMergeImagesOption::D_JDAY][y][x] = int(m_options.GetTRefIndex(TRef));
							debugData[CMergeImagesOption::NB_IMAGES][y][x] = int(imageList.size());
							debugData[CMergeImagesOption::SCENE][y][x] = (int)iz + 1;
							debugData[CMergeImagesOption::SORT_TEST][y][x] = it->first.GetRef();
						}
						
						debugData[CMergeImagesOption::NB_TRIGGERS][y][x] = nbTriggerUsed;
						debugData[CMergeImagesOption::NB_SKIPS][y][x] = nbSkip;
					}

					if (iz != NOT_INIT && m_options.m_bCreateImage)
					{
						CLandsatPixel pixel;
						if (window.GetPixel(iz, x, y, pixel) && !m_options.IsBusting(pixel.R(), pixel.G(), pixel.B()))
						{
							for (size_t z = 0; z < SCENES_SIZE; z++)
								outputData[z][y][x] = (__int16)outputDS.PostTreatment(pixel[z], z);
						}
					}//for all landsat bands

					if (m_options.m_bExportStats)
					{
						for (size_t z = 0; z < bandHolder.GetSceneSize(); z++)
							for (size_t ss = 0; ss < CMergeImagesOption::NB_STATS; ss++)
								if (stats[z][NB_VALUE]>0)
									statsData[z*CMergeImagesOption::NB_STATS + ss][y][x] = (__int16)outputDS.PostTreatment(stats[z][CMergeImagesOption::BANDS_STATS[ss]], z);
					}//if export stats

#pragma omp atomic 
					m_options.m_xx++;

				}//for x
				m_options.UpdateBar();
			}//for y


			//now, clean isolated pixel and grow ring
			if (m_options.m_bCreateImage && !cloudsCleaner.empty())
			{
				if (m_options.m_clear[0] > 0)
				{
					CMatrix<size_t> selectedChoice2 = selectedChoice;
					//first clean pixel
					for (int y = 0; y < blockSize.m_y; y++)
					{
						for (int x = 0; x < blockSize.m_x; x++)
						{
							if (firstChoice[y][x] != NOT_INIT && selectedChoice[y][x] != firstChoice[y][x])
							{
								if (IsIsolatedPixel(x, y, firstChoice, selectedChoice))
								{
									CLandsatPixel pixel;
									if (window.GetPixel(firstChoice[y][x], x, y, pixel) && !m_options.IsBusting(pixel.R(), pixel.G(), pixel.B()))
									{
										selectedChoice2[y][x] = firstChoice[y][x];

										for (size_t z = 0; z < SCENES_SIZE; z++)
											outputData[z][y][x] = (__int16)outputDS.PostTreatment(pixel[z], z);

										if (m_options.m_bDebug)
											debugData[CMergeImagesOption::ISOLATED][y][x] = int(firstChoice[y][x]);
									}
								}
							}
							m_options.m_xx++;
						}//x
						m_options.UpdateBar();
					}//y

					selectedChoice = selectedChoice2;
				}
				//second grow pixel ring
				if (m_options.m_buffer > 0)
				{
					CMatrix<size_t> selectedChoice2 = selectedChoice;
					//CMatrix<size_t> secondBest2 = secondBest;
					for (int r = 0; r < m_options.m_buffer; r++)
					{
						for (int y = 0; y < blockSize.m_y; y++)
						{
							for (int x = 0; x < blockSize.m_x; x++)
							{
								if (firstChoice[y][x] != NOT_INIT && firstChoice[y][x] != selectedChoice[y][x])
								{
									for (int yy = y - 1; yy <= (y + 1); yy++)
									{
										if (yy >= 0 && yy < blockSize.m_y)
										{
											for (int xx = x - 1; xx <= x + 1; xx++)
											{
												if (xx >= 0 && xx < blockSize.m_x )
												{
													if (firstChoice[yy][xx] != NOT_INIT/* && selectedChoice[yy][xx] != NOT_INIT */&& selectedChoice[yy][xx] == firstChoice[yy][xx])
													{
														//if (secondBest[yy][xx] != NOT_INIT)
														//{
														//size_t iz = selectedChoice[y][x];
														//size_t iz = secondBest[yy][xx];


														//because we can't change selectedChoise, we have to chnage firstChoice instead
														//firstChoice[yy][xx] = selectedChoice[y][x];
														//find the second best pixel
														//iz = GetCloudFreeIz(imageList, SECOND_BEST, preWindow, window, posWindow, x, y, cloudsCleaner);
														//CLandsatPixel pixel;
														//GetPixel(imageList, , preWindow, window, posWindow, xx, yy, cloudsCleaner, pixel);
														
														//CLandsatPixel pixel = GetPixel(window, selectedChoice[y][x], xx, yy);
														//CLandsatPixel pixel = GetPixel(window, secondBest[y][x], secondBest[yy][xx], xx, yy);

														if (selectedChoice2[yy][xx]==NOT_INIT || selectedChoice2[yy][xx] != selectedChoice[y][x])//if this pixel already changed
														{
															CLandsatPixel pixel;
															if (selectedChoice[y][x]==NOT_INIT || window.GetPixel(selectedChoice[y][x], xx, yy, pixel) && !m_options.IsBusting(pixel.R(), pixel.G(), pixel.B()))
																//if (window.IsValid(selectedChoice[y][x], pixel))
															{
																selectedChoice2[yy][xx] = selectedChoice[y][x];
																//secondBest2[yy][xx] = secondBest[y][x];

																for (size_t z = 0; z < SCENES_SIZE; z++)
																	outputData[z][yy][xx] = (__int16)outputDS.PostTreatment(pixel[z], z);

																if (m_options.m_bDebug)
																	debugData[CMergeImagesOption::BUFFER][yy][xx] = int(selectedChoice[y][x]);
															}
														}
														
													}//for all landsat bands
												}//if xx
											}//xx
										}//if
									}//yy

								}//if clouds

								m_options.m_xx++;
							}//x
							m_options.UpdateBar();
						}//y

						selectedChoice = selectedChoice2;
						//secondBest = secondBest2;
					}//ring
				}//if ring
			}//cloud cleaner

			m_options.m_timerProcess.Stop();
			bandHolder.FlushCache();//clean memory ???? 
			
			for (size_t i = 0; i < mosaicBH.size(); i++)
				mosaicBH[i]->FlushCache();//clean memory ???? 
			
		}//critical process
	}

	
	void CMergeImages::WriteBlock(int xBlock, int yBlock, CBandsHolder& bandHolder, CGDALDatasetEx& outputDS, CGDALDatasetEx& debugDS, CGDALDatasetEx& statsDS, OutputData& outputData, DebugData& debugData, StatData& statsData)
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


		
			for (size_t z = 0; z < SCENES_SIZE; z++)
			{
				GDALRasterBand *pBand = outputDS.GetRasterBand(z);
				if (!bandHolder.IsEmpty())
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


		if (m_options.m_bDebug && !debugData.empty())
		{
			for (size_t z = 0; z < CMergeImagesOption::NB_DEBUG_BANDS; z++)
			{
				GDALRasterBand *pBand = debugDS.GetRasterBand(z);//s*CMergeImagesOption::NB_DEBUG_BANDS + 
				if (!bandHolder.IsEmpty())
				{
					for (int y = 0; y < outputRect.Height(); y++)
						pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y + y, outputRect.Width(), 1, &(debugData[z][y][0]), outputRect.Width(), 1, GDT_Int32, 0, 0);
				}
				else
				{
					__int32 noData = (__int32)debugDS.GetNoData(z);
					pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(noData), 1, 1, GDT_Int32, 0, 0);
				}

				//pBand->FlushCache();
			}

		}

		if (m_options.m_bExportStats && !statsData.empty())
		{
			for (size_t z = 0; z < NB_TOTAL_STATS; z++)
			{
				GDALRasterBand *pBand = statsDS.GetRasterBand(z);
				if (!bandHolder.IsEmpty())
				{
					vector<__int16> tmp;
					tmp.reserve(outputRect.Width()*outputRect.Height());
						
					for (int y = 0; y < outputRect.Height(); y++)
						tmp.insert(tmp.end(), statsData[z][y].begin(), statsData[z][y].begin() + outputRect.Width());

					pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(tmp[0]), outputRect.Width(), outputRect.Height(), GDT_Int16, 0, 0);
				}
				else
				{
					__int16 noData = (__int16)statsDS.GetNoData(z);
					pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &noData, 1, 1, GDT_Int16, 0, 0);
				}

				//pBand->FlushCache();
			}//for all bands in a scene
		}//stats

		m_options.m_timerWrite.Stop();
	}
	}

	void CMergeImages::CloseAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, array<CLandsatDataset,3>& mosaicDS, CGDALDatasetEx& outputDS, CGDALDatasetEx& debugDS, CGDALDatasetEx& statsDS)
	{
		inputDS.Close();
		maskDS.Close();

		for (size_t i = 0; i < mosaicDS.size(); i++)
			mosaicDS[i].Close();
		

		m_options.m_timerWrite.Start();

		if (m_options.m_bComputeStats)
			outputDS.ComputeStats(m_options.m_bQuiet);

		if (!m_options.m_overviewLevels.empty())
			outputDS.BuildOverviews(m_options.m_overviewLevels, m_options.m_bQuiet);

		if (m_options.m_bComputeStats)
			debugDS.ComputeStats(m_options.m_bQuiet);

		if (!m_options.m_overviewLevels.empty())
			debugDS.BuildOverviews(m_options.m_overviewLevels, m_options.m_bQuiet);

		if (m_options.m_bComputeStats)
			statsDS.ComputeStats(m_options.m_bQuiet);

		if (!m_options.m_overviewLevels.empty())
			statsDS.BuildOverviews(m_options.m_overviewLevels, m_options.m_bQuiet);


		outputDS.Close();
		debugDS.Close();
		statsDS.Close();


		if (!m_options.m_bQuiet)
		{
			double percent = m_options.m_nbPixel > 0 ? (double)m_options.m_nbPixelDT / m_options.m_nbPixel * 100 : 0;

			_tprintf("\n");
			_tprintf("Percentage of pixel treated by DecisionTree: %0.3lf %%\n", percent);
			_tprintf("Mean number of images read at the same time: %.3lf\n", m_options.m_nbImages[MEAN]);
			_tprintf("Maximum number of images read at the same time: %.0lf\n\n", m_options.m_nbImages[HIGHEST]);

		}


		m_options.m_timerWrite.Stop();
		m_options.PrintTime();
	}

	Test1Vector::iterator CMergeImages::get_it(Test1Vector& imageList, size_t type)
	{
		Test1Vector::iterator it = imageList.end();
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
			else if (type == CMergeImagesOption::MEDIAN_NDVI || type == CMergeImagesOption::MEDIAN_NBR || type == CMergeImagesOption::MEDIAN_NDMI)
			{
				//select median
				size_t N = imageList.size() / 2;

				it = imageList.begin();
				for (size_t n = 0; n < N; n++)
					it++;
			}
			
		}

		return it;
	}
	
	size_t CMergeImages::get_iz(Test1Vector& imageList, size_t type)
	{
		size_t iz = NOT_INIT;
		Test1Vector::iterator it = get_it(imageList, type);
		if(it != imageList.end())
			iz=it->second;

		return iz;
	}

	size_t CMergeImages::GetCloudFreeIz(Test1Vector& imageList, size_t mergeType, CLandsatWindow& preWindow, CLandsatWindow& window, CLandsatWindow& posWindow, CLandsatWindow& mosaicWindow, int x, int y, CLandsatCloudCleaner& cloudsCleaner, int& nbTriggerUsed, int& nbSkip)
	{
		size_t iz = NOT_INIT;
	
		//now looking for all other images
		Test1Vector::iterator it1 = get_it(imageList, mergeType);
		if (it1 != imageList.end())
		{
			size_t b = it1->second*window.GetSceneSize() + JD;
			CTRef TRef = m_options.GetTRef(int(window.at(b)->at(x, y)));

			//Get pixel
			CLandsatPixel pixel2;
			CLandsatPixel pixel3;

			
			//find the latest valid pixel before TRef
			if (!preWindow.empty())
			{
				preWindow.GetPixel(0, x, y, pixel2);
			}
				
			//find the first valid pixel after TRef
			if (!posWindow.empty())
			{
				posWindow.GetPixel(0, x, y, pixel3);
			}

			//find the latest valid pixel before TRef
			if (!mosaicWindow.empty())
			{
				size_t pre_iz = NOT_INIT;
				for (size_t i = mosaicWindow.GetNbScenes() - 1; i <mosaicWindow.GetNbScenes() && pre_iz == NOT_INIT; i--)
				{
					size_t b = i*mosaicWindow.GetSceneSize() + JD;
					CTRef preTRef = m_options.GetTRef(int(mosaicWindow.at(b)->at(x, y)));
					if (preTRef.IsInit() && preTRef.GetYear() < TRef.GetYear())
						pre_iz = i;
				}

				if (pre_iz != NOT_INIT)
					mosaicWindow.GetPixel(pre_iz, x, y, pixel2);

				size_t pos_iz = NOT_INIT;
				for (size_t i = 0; i < mosaicWindow.GetNbScenes() && pos_iz == NOT_INIT; i++)
				{
					size_t b = i*mosaicWindow.GetSceneSize() + JD;
					CTRef posTRef = m_options.GetTRef(int(mosaicWindow.at(b)->at(x, y)));
					if (posTRef.IsInit() && posTRef.GetYear() > TRef.GetYear())
						pos_iz = i;
				}

				if (pos_iz != NOT_INIT)
					mosaicWindow.GetPixel(pos_iz, x, y, pixel3);
			}

			//now looking for all other images
			while (it1 != imageList.end() && nbSkip < m_options.m_maxSkip)
			{
				size_t iz1 = it1->second;
				CLandsatPixel pixel1 = window.GetPixel(iz1, x, y);
				ASSERT(pixel1.IsInit());
				CTRef TRef = m_options.GetTRef(int(pixel1[JD]));

				bool bDoTrigger = true;
				bool bTrig_B1 = false;
				bool bTrig_TCB = false;


				if (pixel2.IsInit() && pixel3.IsInit())
				{
					bTrig_B1 = (pixel2[I_B1] - pixel1[I_B1] < -125) && (pixel3[I_B1] - pixel1[I_B1] < -125);
					bTrig_TCB = (pixel2.TCB() - pixel1.TCB() > 750) && (pixel3.TCB() - pixel1.TCB() > 750);
					nbTriggerUsed = 2;
				}
				else if (pixel2.IsInit())
				{
					bTrig_B1 = (pixel2[I_B1] - pixel1[I_B1] < -125);
					bTrig_TCB = (pixel2.TCB() - pixel1.TCB() > 750);
					nbTriggerUsed = 1;
				}
				else if (pixel3.IsInit())
				{
					bTrig_B1 = (pixel3[I_B1] - pixel1[I_B1] < -125);
					bTrig_TCB = (pixel3.TCB() - pixel1.TCB() > 750);
					nbTriggerUsed = 1;
				}
				else
				{
					Test1Vector::iterator it2 = get_it(imageList, CMergeImagesOption::SECOND_BEST);
					while (it2 != imageList.end() && nbSkip < m_options.m_maxSkip)
					{
						size_t iz2 = it2->second;
						//size_t iz = get_iz(imageList, m_options.m_mergeType);
						if (window.GetPixel(iz2, x, y, pixel2))
						{
							if ((abs(pixel2[I_B1] - pixel1[I_B1]) < -125) || abs((pixel2.TCB() - pixel1.TCB()) > 750))
							{
#pragma omp atomic
								m_options.m_nbPixelDT++;

								int DTCode = cloudsCleaner.GetDTCode(pixel2, pixel1);
								if (cloudsCleaner.IsFirstCloud(DTCode))
								{
									imageList.erase(it2);
									it2 = get_it(imageList, CMergeImagesOption::SECOND_BEST);
									nbSkip++;
								}
								else if (cloudsCleaner.IsSecondCloud(DTCode))
								{
									bDoTrigger = false;
									imageList.erase(it1);
									it1 = get_it(imageList, m_options.m_mergeType);
									it2 = imageList.end();
									nbSkip++;
								}
								else
								{
									bTrig_B1 = (pixel2[I_B1] - pixel1[I_B1] < -125);
									bTrig_TCB = (pixel2.TCB() - pixel1.TCB() > 750);
									nbTriggerUsed = 1;
									it2 = imageList.end();
								}
							}
							else
							{
								bTrig_B1 = (pixel2[I_B1] - pixel1[I_B1] < -125);
								bTrig_TCB = (pixel2.TCB() - pixel1.TCB() > 750);
								nbTriggerUsed = 1;
								it2 = imageList.end();
							}
						}
					}
				}

				if (bDoTrigger && nbSkip < m_options.m_maxSkip)
				{
					if (bTrig_B1 || bTrig_TCB)
					{
#pragma omp atomic
						m_options.m_nbPixelDT++;

						int DTCode = 99;
						if (pixel2.IsInit())
							DTCode = cloudsCleaner.GetDTCode(pixel2, pixel1);
						//else
						//	DTCode = cloudsCleaner.GetDTCode(pixel3, pixel1);

						if (cloudsCleaner.IsSecondCloud(DTCode))
						{
							imageList.erase(it1);
							it1 = get_it(imageList, m_options.m_mergeType);
							nbSkip++;
						}
						else
						{
							iz = iz1;
							it1 = imageList.end();
						}
					}
					else
					{
						iz = iz1;
						it1 = imageList.end();
					}
				}
			}//while not good pixel


			if (nbSkip == m_options.m_maxSkip && it1 != imageList.end())
			{
				iz = it1->second;
			}

			/*if (iz == NOT_INIT)
			{
			int g;
			g = 0;
			}*/
		}

		return iz;
	}

	bool CMergeImages::IsIsolatedPixel(int x, int y, const CMatrix<size_t>& firstChoice, const CMatrix<size_t>& selectedChoice)const
	{
		int d = m_options.m_clear[0];

		size_t count = 0;
		for (int yy = y - d; (yy <= y + d) && (count <= m_options.m_clear[1]); yy++)
		{
			if (yy >= 0 && yy < firstChoice.size_y())
			{
				for (int xx = (x - d); (xx <= x + d) && (count <= m_options.m_clear[1]); xx++)
				{
					if (xx >= 0 && xx < firstChoice.size_x())
					{
						if (firstChoice[yy][xx] != NOT_INIT && firstChoice[yy][xx] != selectedChoice[yy][xx])
							count++;
					}
				}
			}
		}

		return count <= m_options.m_clear[1];
	}


	__int16 CMergeImages::GetMeanQA(CLandsatWindow& window, CGeoSize size, size_t iz, int x, int y)
	{
		CStatistic sumQA;

		int d = m_options.m_QA;

		size_t count = 0;
		//for (int yy = y - d; (yy <= y + d); yy++)
		for (int dd = -d; (dd <= d); dd++)
		{
		//	for (int xx = (x - d); (xx <= x + d); xx++)
			{
				//select only *
				if (dd==-d||dd==0||dd==d)
				{
					int xx = x + dd;
					int yy = y + dd;
					if (xx >= 0 && xx < size.m_x && yy >= 0 && yy < size.m_y)
					{
						CLandsatPixel pixel;
						if (window.GetPixel(iz, xx, yy, pixel))
							sumQA += pixel[QA];
						else
							sumQA += m_options.m_QAmiss;//add bad value for missing pixel
					}
				}
			}
		}

		ASSERT(sumQA.IsInit());

		return __int16 (Round(sumQA[MEAN]));
	}

	bool CMergeImages::IsCloud(CLandsatCloudCleaner& cloudsCleaner, const CLandsatPixel & pixel1, const CLandsatPixel & pixel2)
	{
		bool bIsCloud = false;
		bool bFirst_B1 = pixel1[I_B1] - pixel2[I_B1] < -125;
		bool bFirst_TCB = pixel1.TCB() - pixel2.TCB() > 750;
		if (bFirst_B1 || bFirst_TCB)
		{
#pragma omp atomic
			m_options.m_nbPixelDT++;

			//the image of comparison seem to be a cloud or a shadow, we remove it
			int DTCo = cloudsCleaner.GetDTCode(pixel1, pixel2);
			bIsCloud = cloudsCleaner.IsSecondCloud(DTCo);
		}
		
		return bIsCloud;
	}

	CLandsatPixel CMergeImages::GetPixel(CLandsatWindow& window, size_t iz1, size_t iz2, int x, int y)
	{
		CLandsatPixel pixel;

		if (iz1 != NOT_INIT)
		{
			CLandsatPixel pixel1 = window.GetPixel(iz1, x, y);
			if (window.IsValid(iz1, pixel1) && !m_options.IsBusting(pixel1.R(), pixel1.G(), pixel1.B()))
			{
				pixel = pixel1;
				if (m_options.m_meanDmax > 0 && iz2 != NOT_INIT)
				{
					CLandsatPixel pixel2 = window.GetPixel(iz2, x, y);
					if (window.IsValid(iz2, pixel2) && !m_options.IsBusting(pixel2.R(), pixel2.G(), pixel2.B()) )
					{
						double f = max(0.0, min(0.25, (m_options.m_meanDmax - pixel.GetEuclideanDistance(pixel2, true)) / m_options.m_meanDmax));

						for (size_t z = 0; z < SCENES_SIZE; z++)
							pixel[z] = (1-f)*pixel1[z] + f*pixel2[z];
					}
				}
			}
		}


		return pixel;
	}
}