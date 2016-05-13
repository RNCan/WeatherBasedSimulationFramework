//***********************************************************************
// program to merge Landsat image image over a period
//									 
//***********************************************************************
// version
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

//--config GDAL_CACHEMAX 4096  -IOCPU 4 -co "bigtiff=yes" -co "tiled=YES" -co "BLOCKXSIZE=1024" -co "BLOCKYSIZE=1024" -overview {2,4,8,16} -multi -stats -co "compress=LZW" -overwrite -Clouds "C:\Geomatique\model\Clouds" "C:\Geomatique\Input\mosaic_subset.vrt" "C:\geomatique\1996\1996subset.vrt" "C:\geomatique\output\1996_test.tif"
//-te -1271940 7942380 -1249530 7956540 -of vrt -co "bigtiff=yes" -co "compress=LZW" -co "tiled=YES" -co "BLOCKXSIZE=1024" -co "BLOCKYSIZE=1024" -blocksize 1024 1024 -multi -Type SecondBest -stats -debug -dstnodata -32768 --config GDAL_CACHEMAX 1024 "U:\GIS1\LANDSAT_SR\LCC\2014\#57_2014_182-244.vrt" "U:\GIS\#documents\TestCodes\MergeImages\Test4\Nuage.vrt"
//-stats -Type BestPixel -te 1358100 6854400 1370300 6865500 -of VRT -ot Int16 -blockSize 1024 1024 -co "compress=LZW" -co "tiled=YES" -co "BLOCKXSIZE=1024" -co "BLOCKYSIZE=1024" --config GDAL_CACHEMAX 4096  -overview {2,4,8,16} -multi -IOCPU 3 -overwrite -Clouds "U:\GIS\#documents\TestCodes\MergeImages\Test2\Model\V4_SR_DTD1_cloudv4_skip100_200" "U:\GIS1\LANDSAT_SR\LCC\1999-2006.vrt" "U:\GIS\#documents\TestCodes\MergeImages\Test2\Output\Test.vrt"
//-stats -Type Oldest -TT OverallYears -of VRT -ot Int16 -blockSize 1024 1024 -co "compress=LZW" -co "tiled=YES" -co "BLOCKXSIZE=1024" -co "BLOCKYSIZE=1024" --config GDAL_CACHEMAX 4096  -overview {2,4,8,16} -multi -IOCPU 3 -overwrite "U:\GIS\#documents\TestCodes\BandsAnalyser\Test1\Input\Test1999-2014.vrt" "U:\GIS\#documents\TestCodes\MergeImages\Test0\output\Test.vrt"

#include "stdafx.h"
#include <float.h>
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
	const char* CMergeImages::VERSION = "2.1.0";
	const int CMergeImages::NB_THREAD_PROCESS = 2;
	static const int NB_TOTAL_STATS = CMergeImagesOption::NB_STATS*SCENES_SIZE;



	//*********************************************************************************************************************


	const short CMergeImagesOption::BANDS_STATS[NB_STATS] = { LOWEST, MEAN, STD_DEV, HIGHEST };
	const char* CMergeImagesOption::MERGE_TYPE_NAME[NB_MERGE_TYPE] = { "Oldest", "Newest", "MaxNDVI", "Best", "SecondBest" };
	const char* CMergeImagesOption::DEBUG_NAME[NB_DEBUG_BANDS] = { "captor", "path", "row", "year", "month", "day", "Jday", "nbImages", "Scene" };
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
//		m_bNoDefaultTrigger = false;
		m_clean = { 0, 0 };
		m_ring = 0;
		m_scenesSize = SCENES_SIZE;
		m_TM = CTM::ANNUAL;
		m_nbPixelDT = 0;
		m_nbPixel = 0;

		

		m_appDescription = "This software merge all Landsat scenes (composed of " + to_string(SCENES_SIZE) + " bands) of an input images by selecting desired pixels.";


		//AddOption("-TTF");
		//AddOption("-Period");

		static const COptionDef OPTIONS[] =
		{
			//{ "-TT", 1, "t", false, "The temporal transformation allow user to merge images in different time period segment. The available types are: OverallYears, ByYears, ByMonths and None. None can be use to subset part of the input image. ByYears and ByMonths merge the images by years or by months. ByYear by default." },
			{ "-Type", 1, "t", false, "Merge type criteria: Oldest, Newest, MaxNDVI, BestPixel or SecondBest. BestPixel by default." },
			{ "-Clouds", 1, "file", false, "Decision tree model file path to remove clouds." },
			//{ "-NoDefautTrigger", 0, "", false, "Without this option, \"B1 -125\" and \"TCB 750\" is added to trigger to be used with the \"-Clouds\" options." },
			//{ "-Trigger", 3, "m tt th", true, "Add optimization trigger to execute decision tree when comparing T1 with T2. m is the merge method (can be \"OR\" or \"AND\"), tt is the trigger type and th is the trigger threshold. Supported type are \"B1\"..\"B9\",\"NBR\",\"EUCLIDEAN\", \"NDVI\", \"NDMI\", \"TCB\" (Tasseled Cap Brightness), \"TCG\" (Tasseled Cap Greenness) or \"TCW\" (Tasseled Cap Wetness)." },
			{ "-Mosaic", 1, "file", false, "Extra multi years mosaic image file path to trigger DT when remove clouds." },
			{ "-Clean", 2, "NbPix  maxCount", false, "Don't replace isolated cloud pixel if the count of cloud pixel around nbPix is less than maxCount." },
			{ "-Ring", 1, "NbPix ", false, "Grow decision tree cloud by nbPix." },
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
			{ "Optional Output Image", "_debug", "Number of output periods", "9", "Path: path number of satellite|Row: row number of satellite|Year: year|Month: month (1-12)|Day: day (1-31)|JDay: Julian day (1-366)|NbScenes: number of valid scene", "" }
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
				msg.ajoute("ERROR: Invalid -Type option: valid type are \"Oldest\", \"Newest\", \"MaxNDVI\", \"Best\" or \"SecondBest\".");
			}
		}
		//else if (IsEqual(argv[i], "-NoDefautTrigger"))
		//{
		//	m_bNoDefaultTrigger = true;
		//}
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
		else if (IsEqual(argv[i], "-Mosaic"))
		{
			m_mosaicFilePath = argv[++i];
		}
		else if (IsEqual(argv[i], "-Clean"))
		{
			m_clean[0] = ToInt(argv[++i]);
			m_clean[1] = ToInt(argv[++i]);
		}
		else if (IsEqual(argv[i], "-Ring"))
		{
			m_ring = ToInt(argv[++i]);
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
		}

		GDALAllRegister();
		
		CLandsatDataset inputDS;
		CGDALDatasetEx maskDS;
		CLandsatDataset mosaicDS;
		CLandsatCloudCleaner cloudsCleaner;//Decision Tree
		CBandsHolderMT inputBH(1, m_options.m_memoryLimit, m_options.m_IOCPU, NB_THREAD_PROCESS);
		CBandsHolderMT mosaicBH(1, m_options.m_memoryLimit, m_options.m_IOCPU, NB_THREAD_PROCESS);
		
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

		if (msg && mosaicDS.IsOpen())
			msg = mosaicBH.Load(mosaicDS, m_options.m_bQuiet, m_options.GetExtents(), mosaicDS.GetPeriod());

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
		if (!cloudsCleaner.empty() && m_options.m_clean[0]>0)
			nbPass++;
		if (!cloudsCleaner.empty() && m_options.m_ring > 0)
			nbPass++;

		m_options.ResetBar(extents.m_xSize*extents.m_ySize*nbPass);

		std::array< OutputData, NB_THREAD_PROCESS> outputData;
		std::array< DebugData, NB_THREAD_PROCESS> debugData;
		std::array< OutputData, NB_THREAD_PROCESS> statsData;
		for (size_t i = 0; i < NB_THREAD_PROCESS; i++)
			InitMemory(inputBH.GetSceneSize(), extents.GetBlockSize(), outputData[i], debugData[i], statsData[i]);




		vector<pair<int, int>> XYindex = extents.GetBlockList(3,3);

#pragma omp parallel for schedule(static, 1) num_threads( NB_THREAD_PROCESS ) if (m_options.m_bMulti) 
		for (int b = 0; b < (int)XYindex.size(); b++)
		{
			//if (b % 9)//faudrait flusher juste les image qui ne seront plus utiliser
			//{
			//	inputDS->FlushCache();
			//	if (mosaicDS.IsOpen())
			//		mosaicDS->FlushCache();
			//}
				


			int xBlock = XYindex[b].first;
			int yBlock = XYindex[b].second;

			int thNo = ::omp_get_thread_num();

			ReadBlock(xBlock, yBlock, inputBH[thNo], mosaicBH[thNo]);
			ProcessBlock(xBlock, yBlock, inputBH[thNo], inputDS, mosaicBH[thNo], outputDS, outputData[thNo], debugData[thNo], statsData[thNo], cloudsCleaner);
			WriteBlock(xBlock, yBlock, inputBH[thNo], outputDS, debugDS, statsDS, outputData[thNo], debugData[thNo], statsData[thNo]);


		}//for all blocks

		CloseAll(inputDS, maskDS, mosaicDS, outputDS, debugDS, statsDS);

		return msg;
	}


	void CMergeImages::InitMemory(size_t sceneSize, CGeoSize blockSize, OutputData& outputData, DebugData& debugData, OutputData& statsData)
	{
		if (m_options.m_bCreateImage)
		{
			float dstNodata = (float)m_options.m_dstNodata;

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
			float dstNodata = (float)WBSF::GetDefaultNoData(GDT_Int32);

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


	//void CMergeImages::InitMemory(size_t sceneSize, CGeoSize blockSize, OutputData& outputData, DebugData& debugData, OutputData& statsData)
	//{
	//	if (m_options.m_bCreateImage)
	//	{
	//		float dstNodata = (float)m_options.m_dstNodata;
	//		if (outputData.size != m_options.GetTTPeriod().GetNbRef())
	//			outputData.resize(m_options.GetTTPeriod().GetNbRef());

	//		for (size_t s = 0; s < outputData.size(); s++)
	//		{
	//			if (outputData[s].size != SCENES_SIZE)
	//				outputData[s].resize(SCENES_SIZE);

	//			for (size_t z = 0; z < outputData[s].size(); z++)
	//			{
	//				if (outputData[s][z].size() != blockSize.m_y)
	//					outputData[s][z].resize(blockSize.m_y);

	//				for (size_t j = 0; j < outputData[s][z].size(); j++)
	//				{
	//					if (outputData[s][z][j].size() != blockSize.m_x)
	//						outputData[s][z][j].resize(blockSize.m_x, dstNodata);
	//					else
	//						std::fill(outputData[s][z][j].begin(), outputData[s][z][j].end(), dstNodata);
	//				}
	//			}
	//		}
	//	}

	//	if (m_options.m_bDebug)
	//	{
	//		__int32 dstNodata = (__int32)WBSF::GetDefaultNoData(GDT_Int32);

	//		if (debugData.size() != m_options.GetTTPeriod().GetNbRef())
	//			debugData.resize(m_options.GetTTPeriod().GetNbRef());

	//		for (size_t s = 0; s < debugData.size(); s++)
	//		{
	//			if (debugData[s].size() != CMergeImagesOption::NB_DEBUG_BANDS)
	//				debugData[s].resize(CMergeImagesOption::NB_DEBUG_BANDS);

	//			for (size_t z = 0; z < debugData[s].size(); z++)
	//			{
	//				if (debugData[s][z].size() != blockSize.m_y)
	//					debugData[s][z].resize(blockSize.m_y);

	//				for (size_t j = 0; j < debugData[s][z].size(); j++)
	//				{
	//					if (debugData[s][z][j].size() != blockSize.m_x)
	//						debugData[s][z][j].resize(blockSize.m_x, dstNodata);
	//					else
	//						std::fill(debugData[s][z][j].begin(), debugData[s][z][j].end(), dstNodata);
	//				}
	//			}
	//		}
	//	}

	//	if (m_options.m_bExportStats)
	//	{
	//		//we used no data of int32 instead
	//		float dstNodata = (float)WBSF::GetDefaultNoData(GDT_Int32);

	//		if (statsData.size() != m_options.GetTTPeriod().GetNbRef())
	//			statsData.resize(m_options.GetTTPeriod().GetNbRef());

	//		for (size_t s = 0; s < statsData.size(); s++)
	//		{
	//			if (statsData[s].size() != sceneSize*CMergeImagesOption::NB_STATS)
	//				statsData[s].resize(sceneSize*CMergeImagesOption::NB_STATS);

	//			for (size_t z = 0; z < statsData[s].size(); z++)
	//			{
	//				if (statsData[s][z].size() != blockSize.m_y)
	//					statsData[s][z].resize(blockSize.m_y);

	//				for (size_t j = 0; j < statsData[s][z].size(); j++)
	//				{
	//					if (statsData[s][z][j].size() != blockSize.m_x)
	//						statsData[s][z][j].resize(blockSize.m_x, dstNodata);
	//					else
	//						std::fill(statsData[s][z][j].begin(), statsData[s][z][j].end(), dstNodata);
	//				}
	//			}
	//		}
	//	}
	//}

	ERMsg CMergeImages::OpenInput(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& mosaicDS)
	{
		ERMsg msg;

		if (!m_options.m_bQuiet)
			cout << endl << "Open input image..." << endl;

		CMergeImagesOption options(m_options);
		msg = inputDS.OpenInputImage(m_options.m_filesPath[CMergeImagesOption::INPUT_FILE_PATH], options);

		if (msg && !m_options.m_mosaicFilePath.empty())
		{
			if (!m_options.m_bQuiet)
				cout << "Open mosaic..." << endl;

			//load only one year before and one year after
			CMergeImagesOption options(m_options);
			CTPeriod p = inputDS.GetPeriod();
			CTPeriod period(CTRef(p.Begin().GetYear() - 1, 0, 0), CTRef(p.End().GetYear() + 1, LAST_MONTH, LAST_DAY));
			options.m_period = period;
			
			msg = mosaicDS.OpenInputImage(m_options.m_mosaicFilePath, options);
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


			if (mosaicDS.IsOpen())
			{
				cout << "*****   Mosaic images informations   *****" << endl;

				extents = mosaicDS.GetExtents();
				pPrj = mosaicDS.GetPrj();
				prjName = pPrj ? pPrj->GetName() : "Unknown";

				cout << "    Size           = " << mosaicDS.GetRasterXSize() << " cols x " << mosaicDS.GetRasterYSize() << " rows x " << mosaicDS.GetRasterCount() << " bands" << endl;
				cout << "    Extents        = X:{" << ToString(extents.m_xMin) << ", " << ToString(extents.m_xMax) << "}  Y:{" << ToString(extents.m_yMin) << ", " << ToString(extents.m_yMax) << "}" << endl;
				cout << "    Projection     = " << prjName << endl;
				cout << "    NbBands        = " << mosaicDS.GetRasterCount() << endl;
				cout << "    Scene size     = " << mosaicDS.GetSceneSize() << endl;
				cout << "    Nb. scenes     = " << mosaicDS.GetNbScenes() << endl;
				cout << "    First image    = " << mosaicDS.GetPeriod().Begin().GetFormatedString() << endl;
				cout << "    Last image     = " << mosaicDS.GetPeriod().End().GetFormatedString() << endl;
				//cout << "    Input period   = " << mosaicDS.GetPeriod().GetFormatedString() << endl;

				if (mosaicDS.GetSceneSize() != SCENES_SIZE)
					cout << FormatMsg("WARNING: the number of bands per scene (%1) is different than the inspected number (%2)", to_string(mosaicDS.GetSceneSize()), to_string(SCENES_SIZE)) << endl;
			}
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
			options.m_outputType = GDT_Float32;
			options.m_dstNodata = WBSF::GetDefaultNoData(GDT_Int32);

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

	void CMergeImages::ReadBlock(int xBlock, int yBlock, CBandsHolder& inputBH, CBandsHolder& mosaicBH)
	{
#pragma omp critical(BlockIO)
	{
		m_options.m_timerRead.Start();

		CGeoExtents extents = m_options.m_extents.GetBlockExtents(xBlock, yBlock);
		inputBH.LoadBlock(extents, m_options.m_period);


		CTPeriod p = m_options.m_period;
		CTPeriod period(CTRef(p.Begin().GetYear() - 1, FIRST_MONTH, FIRST_DAY), CTRef(p.End().GetYear() + 1, LAST_MONTH, LAST_DAY));
		mosaicBH.LoadBlock(extents, period);

		m_options.m_timerRead.Stop();
	}
	}

	/*void CMergeImages::ReleaseBlock(int xBlock, int yBlock, CBandsHolder& inputBH, CBandsHolder& mosaicBH)
	{
#pragma omp critical(BlockIO)
	{
		m_options.m_timerRead.Start();

		CGeoExtents extents = m_options.m_extents.GetBlockExtents(xBlock, yBlock);
		inputBH.LoadBlock(extents, m_options.m_period);


		CTPeriod p = m_options.m_period;
		CTPeriod period(CTRef(p.Begin().GetYear() - 1, FIRST_MONTH, FIRST_DAY), CTRef(p.End().GetYear() + 1, LAST_MONTH, LAST_DAY));
		mosaicBH.LoadBlock(extents, period);

		m_options.m_timerRead.Stop();
	}
	}*/


	
	void CMergeImages::ProcessBlock(int xBlock, int yBlock, CBandsHolder& bandHolder, CGDALDatasetEx& inputDS, CBandsHolder& mosaicBH, CGDALDatasetEx& outputDS, OutputData& outputData, DebugData& debugData, OutputData& statsData, CLandsatCloudCleaner& cloudsCleaner)
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
			//CMatrix<size_t> secondChoice;
			CMatrix<int> DTCode(blockSize.m_y, blockSize.m_x);

			if (!cloudsCleaner.empty() && (m_options.m_clean[0]>0 || m_options.m_ring > 0) )
			{
				firstChoice.resize(blockSize.m_y, blockSize.m_x);
				selectedChoice.resize(blockSize.m_y, blockSize.m_x);
				//secondChoice.resize(blockSize.m_y, blockSize.m_x);
			}
				
			
			

			//schedule(static, 1) 
#pragma omp parallel for num_threads( m_options.m_CPU ) if (m_options.m_bMulti) 
			for (int y = 0; y <blockSize.m_y; y++)
			{
				for (int x = 0; x < blockSize.m_x; x++)
				{
					//Test2Vector allImages;
					Test1Vector imageList;
					CTRefSet Treference;
					CStatisticVector stats(window.GetSceneSize());

					//process all bands for the moment
					for (size_t iz = 0; iz < window.GetNbScenes(); iz++)
					{
						//Get pixel
						CLandsatPixel pixel = window.GetPixel(iz, x, y);
						if (window.IsValid(iz, pixel))
						{
							bool bValid = true;

							CTRef TRef = m_options.GetTRef(int(pixel[JD]));
							CTRef TRefQA;
							if (m_options.m_mergeType == CMergeImagesOption::BEST_PIXEL ||
								m_options.m_mergeType == CMergeImagesOption::SECOND_BEST)
							{
								bool bAdd = true;
								if (m_options.m_mergeType == CMergeImagesOption::SECOND_BEST && !imageList.empty())
								{
									if (Treference.find(TRef) != Treference.end())
									bAdd = false;
								}

								//add image qualities
								if (bAdd)
								{
									Treference.insert(TRef);
									TRefQA.SetRef((long)pixel[QA], CTM(CTM::ATEMPORAL));
								}
							}
							else if (m_options.m_mergeType == CMergeImagesOption::MAX_NDVI)
							{
								long NDVI = (long)WBSF::LimitToBound(pixel.NDVI() * 1000, m_options.m_outputType);
								TRefQA.SetRef(NDVI, CTM(CTM::ATEMPORAL));
							}
							else
							{
								TRefQA = TRef;
							}

							if (TRefQA.IsInit())
								imageList.push_back(make_pair(TRefQA, iz));

#pragma omp atomic
							m_options.m_nbPixel++;

						}//if valid

						if (m_options.m_bExportStats)
						{
							for (size_t z = 0; z < pixel.size(); z++)
							{
								if (window.IsValid(iz, z, pixel[z]))
									stats[z] += pixel[z];
							}
						}//if export

					}//iz


					sort(imageList.begin(), imageList.end());

					//get first choice before erase
					if (!firstChoice.empty())
						firstChoice[y][x] = get_iz(imageList, m_options.m_mergeType);

					if (!imageList.empty())
						m_options.m_nbImages += (double)imageList.size();

					//clean clouds
					if (!cloudsCleaner.empty() )
					{
						int nbTriggerUsed=0;
						//now looking for all other images
						//for (Test1Vector::iterator it1 = imageList.begin(); it1 != imageList.end();)
						Test1Vector::iterator it1 = imageList.begin();
						while (it1 != imageList.end())
						{
							size_t iz = get_iz(imageList, m_options.m_mergeType);
							size_t iz1 = it1->second;
							CLandsatPixel pixel = window.GetPixel(iz1, x, y);
							CTRef TRef = m_options.GetTRef(int(pixel[JD]));

							bool bTrig_B1 = false;
							bool bTrig_TCB = false;
							bool bTrig2 = false;

							//Get pixel
							CLandsatPixel pixel_m1;
							CLandsatPixel pixel_p1;
							CLandsatPixel pixel2;

							if (!mosaicBH.IsEmpty())
							{
								CLandsatWindow mosaicWindow = static_cast<CLandsatWindow&>(mosaicBH.GetWindow());

								for (size_t mz = 0; mz < mosaicWindow.GetNbScenes(); mz++)
								{
									CLandsatPixel pixelM = mosaicWindow.GetPixel(mz, x, y);
									if (mosaicWindow.IsValid(mz, pixelM))
									{
										CTRef TRef2 = m_options.GetTRef(int(pixelM[JD]));
										if (TRef2.GetYear() == TRef.GetYear() - 1)
											pixel_m1 = pixelM;
										if (TRef2.GetYear() == TRef.GetYear() + 1)
											pixel_p1 = pixelM;
									}
								}

								if (pixel_m1.IsInit() && pixel_p1.IsInit())
								{
									bTrig_B1 = (pixel_m1[I_B1] - pixel[I_B1] < -125) && (pixel_p1[I_B1] - pixel[I_B1] < -125);
									bTrig_TCB = (pixel_m1.TCB() - pixel.TCB() > 750) && (pixel_p1.TCB() - pixel.TCB() > 750);
									nbTriggerUsed = 2;
								}
								else if (pixel_m1.IsInit())
								{
									bTrig_B1 = pixel_m1[I_B1] - pixel[I_B1] < -125;
									bTrig_TCB = pixel_m1.TCB() - pixel.TCB() > 750;
									nbTriggerUsed = 1;
								}
								else if (pixel_p1.IsInit())
								{
									bTrig_B1 = (pixel_p1[I_B1] - pixel[I_B1] < -125);
									bTrig_TCB = (pixel_p1.TCB() - pixel.TCB() > 750);
									nbTriggerUsed = 1;
								}
							}
							else
							{
								size_t iz2 = get_iz(imageList, CMergeImagesOption::SECOND_BEST);
								if (iz2 !=NOT_INIT && iz2 != iz1)
								{
									//size_t iz2 = it2->second;
									pixel2 = window.GetPixel(iz2, x, y);
									if (pixel2.IsInit())
										bTrig2 = (pixel2[I_B1] - pixel[I_B1] < -125) || pixel2.TCB() - pixel.TCB() > 750;
								}
							}

							if ( bTrig_B1 || bTrig_TCB || bTrig2)
							{
								if (pixel_m1.IsInit() )
									DTCode[y][x] = cloudsCleaner.GetDTCode(pixel_m1, pixel);
								else if (pixel_p1.IsInit())
									DTCode[y][x] = cloudsCleaner.GetDTCode(pixel_p1, pixel);
								else if (pixel2.IsInit())
									DTCode[y][x] = cloudsCleaner.GetDTCode(pixel2, pixel);

#pragma omp atomic
								m_options.m_nbPixelDT ++;

								if (cloudsCleaner.IsSecondCloud(DTCode[y][x]))
								{
									it1 = imageList.erase(it1);
								}
								else
								{
									//it1++;
									it1 = imageList.end();
								}
							}
							else
							{
								//it1++;
								it1 = imageList.end();
							}
						}//while not good pixel
					}//if remove clouds

					//find input temporal index
					size_t iz = get_iz(imageList, m_options.m_mergeType);
					
					//get first choice before erase
					if (!selectedChoice.empty())
					{
						selectedChoice[y][x] = iz;//clouds was remove
						//secondChoice[y][x] = get_iz(imageList, CMergeImagesOption::SECOND_BEST);
					}
							
					if (iz != NOT_INIT && m_options.m_bDebug)
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

						debugData[CMergeImagesOption::D_CAPTOR][y][x] = cap;
						debugData[CMergeImagesOption::D_PATH][y][x] = col;
						debugData[CMergeImagesOption::D_ROW][y][x] = row;
						debugData[CMergeImagesOption::D_YEAR][y][x] = TRef.GetYear();
						debugData[CMergeImagesOption::D_MONTH][y][x] = int(TRef.GetMonth()) + 1;
						debugData[CMergeImagesOption::D_DAY][y][x] = int(TRef.GetDay()) + 1;
						debugData[CMergeImagesOption::D_JDAY][y][x] = int(TRef.GetJDay()) + 1;
						debugData[CMergeImagesOption::NB_IMAGES][y][x] = int(imageList.size());
						debugData[CMergeImagesOption::SCENE][y][x] = (int)iz;
					}

					if (m_options.m_bCreateImage)
					{
						for (size_t z = 0; z < SCENES_SIZE; z++)
						{
							double pixel = MISSING_DATA;
							if (iz != NOT_INIT)
							{
								size_t iiz = iz*bandHolder.GetSceneSize() + z;
								pixel = window[iiz]->at(x, y);
							}

							//size_t b = s*SCENES_SIZE + z;
							outputData[z][y][x] = (float)outputDS.PostTreatment(pixel, z);
						}
					}//for all landsat bands

					if (m_options.m_bExportStats)
					{
						for (size_t z = 0; z < bandHolder.GetSceneSize(); z++)
							for (size_t ss = 0; ss < CMergeImagesOption::NB_STATS; ss++)
								if (stats[z][NB_VALUE]>0)
									statsData[z*CMergeImagesOption::NB_STATS + ss][y][x] = float(stats[z][CMergeImagesOption::BANDS_STATS[ss]]);
					}//if export stats

#pragma omp atomic 
					m_options.m_xx++;

				}//for x
				m_options.UpdateBar();
			}//for y


			//now, clean isolated pixel and grow ring
			if (m_options.m_bCreateImage && !cloudsCleaner.empty() && m_options.m_clean[0]>0)
			{
				//first clean pixel
				for (int y = 0; y < blockSize.m_y; y++)
				{
					for (int x = 0; x < blockSize.m_x; x++)
					{
						if (firstChoice[y][x] != NOT_INIT && selectedChoice[y][x] != firstChoice[y][x])
						{
							if (IsIsolatedPixel(x, y, firstChoice, selectedChoice))
							{
								selectedChoice[y][x] = firstChoice[y][x];
								for (size_t z = 0; z < SCENES_SIZE; z++)
								{
									ASSERT(firstChoice[y][x] != NOT_INIT);

									size_t iiz = firstChoice[y][x] * bandHolder.GetSceneSize() + z;
									double pixel = window[iiz]->at(x, y);
									outputData[z][y][x] = (float)outputDS.PostTreatment(pixel, z);
								}//for all landsat bands
							}
						}
						m_options.m_xx++;
					}//x
					m_options.UpdateBar();
				}//y
			}
				//second grow pixel ring
			if (!cloudsCleaner.empty() && m_options.m_ring>0)
			{
				for (int y = 0; y < blockSize.m_y; y++)
				{
					for (int x = 0; x < blockSize.m_x; x++)
					{
						if (firstChoice[y][x] != NOT_INIT && selectedChoice[y][x] != NOT_INIT && firstChoice[y][x] != selectedChoice[y][x])
						{
							for (int yy = y - m_options.m_ring; yy <= y + m_options.m_ring; yy++)
							{
								if (yy >= 0 && yy < blockSize.m_y)
								{
									for (int xx = x - m_options.m_ring; xx <= x + m_options.m_ring; xx++)
									{
										if (xx >= 0 && xx < blockSize.m_x && xx != yy)
										{
											if (firstChoice[yy][xx] != NOT_INIT && selectedChoice[yy][xx] != NOT_INIT && selectedChoice[yy][xx] == firstChoice[yy][xx])
											{
												//if (secondChoice[yy][xx] != NOT_INIT)
												//{
													for (size_t z = 0; z < SCENES_SIZE; z++)
													{
														//size_t iiz = secondChoice[yy][xx] * bandHolder.GetSceneSize() + z;
														//take the same layer than the cloud pixel
														size_t iiz = selectedChoice[y][x] * bandHolder.GetSceneSize() + z;

														double pixel = window[iiz]->at(xx, yy);
														outputData[z][yy][xx] = (float)outputDS.PostTreatment(pixel, z);
													}//for all landsat bands
												//}
											}//for all landsat bands
										}//if xx
									}//xx
								}//if
							}//yy

						}//if clouds
#pragma omp atomic 
						m_options.m_xx++;
					}//x
					m_options.UpdateBar();
				}//y
			}

			m_options.m_timerProcess.Stop();
			bandHolder.ReleaseBlocks();//clean memory ???? 
			mosaicBH.ReleaseBlocks();//clean memory ???? 
		}//critical process
	}

	void CMergeImages::WriteBlock(int xBlock, int yBlock, CBandsHolder& bandHolder, CGDALDatasetEx& outputDS, CGDALDatasetEx& debugDS, CGDALDatasetEx& statsDS, OutputData& outputData, DebugData& debugData, OutputData& statsData)
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


			//for (size_t s = 0; s < nbSegment; s++)
			//{
				for (size_t z = 0; z < SCENES_SIZE; z++)
				{
					GDALRasterBand *pBand = outputDS.GetRasterBand(z);
					if (!bandHolder.IsEmpty())
					{
						ASSERT(outputData.size() == SCENES_SIZE);
						
						for (int y = 0; y < outputRect.Height(); y++)
							pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y + y, outputRect.Width(), 1, &(outputData[z][y][0]), outputRect.Width(), 1, GDT_Float32, 0, 0);
					}
					else
					{
						float noData = (float)outputDS.GetNoData(z);
						pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &noData, 1, 1, GDT_Float32, 0, 0);
					}

					//pBand->FlushCache();
				}
			//}
		}


		if (m_options.m_bDebug && !debugData.empty())
		{
			//for (size_t s = 0; s < nbSegment; s++)
			//{
				for (size_t z = 0; z < CMergeImagesOption::NB_DEBUG_BANDS; z++)
				{
					GDALRasterBand *pBand = debugDS.GetRasterBand(z);//s*CMergeImagesOption::NB_DEBUG_BANDS + 
					if (!bandHolder.IsEmpty())
					{
						//for (int y = 0; y < (int)debugData[s][z].size(); y++)
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
			//}
		}

		if (m_options.m_bExportStats && !statsData.empty())
		{
//			for (size_t s = 0; s < nbSegment; s++)
	//		{
				for (size_t z = 0; z < NB_TOTAL_STATS; z++)
				{
					GDALRasterBand *pBand = statsDS.GetRasterBand(z);
					if (!bandHolder.IsEmpty())
					{
						vector<float> tmp;
						tmp.reserve(outputRect.Width()*outputRect.Height());
						
						for (int y = 0; y < outputRect.Height(); y++)
							tmp.insert(tmp.end(), statsData[z][y].begin(), statsData[z][y].begin() + outputRect.Width());

						pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(tmp[0]), outputRect.Width(), outputRect.Height(), GDT_Float32, 0, 0);
					}
					else
					{
						float noData = (float)statsDS.GetNoData(z);
						pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &noData, 1, 1, GDT_Float32, 0, 0);
					}

					//pBand->FlushCache();
				}//for all bands in a scene
		//	}
		}//stats

		m_options.m_timerWrite.Stop();
	}
	}

	void CMergeImages::CloseAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& mosaicDS, CGDALDatasetEx& outputDS, CGDALDatasetEx& debugDS, CGDALDatasetEx& statsDS)
	{
		inputDS.Close();
		maskDS.Close();
		mosaicDS.Close();

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

	size_t CMergeImages::get_iz(const Test1Vector& imageList, size_t type)
	{
		size_t iz = NOT_INIT;
		if (!imageList.empty())
		{
			if (type == CMergeImagesOption::OLDEST)
			{
				iz = imageList.begin()->second;
			}
			else if (type == CMergeImagesOption::NEWEST)
			{
				iz = imageList.rbegin()->second;
			}
			else if (type == CMergeImagesOption::MAX_NDVI)
			{
				iz = imageList.rbegin()->second;
			}
			else if (type == CMergeImagesOption::BEST_PIXEL)
			{
				//take the pixel with the lowest score.
				iz = imageList.begin()->second;
			}
			else if (type == CMergeImagesOption::SECOND_BEST)
			{
				//take the pixel with the lowest score.
				if (imageList.size() >= 2)
					iz = (imageList.begin() + 1)->second;
			}
		}
		return iz;
	}


//	bool CMergeImages::IsIsolatedPixel(int x, int y, CLandsatCloudCleaner& cloudsCleaner, const CMatrix<int>& DTCode)const
	bool CMergeImages::IsIsolatedPixel(int x, int y, const CMatrix<size_t>& firstChoice, const CMatrix<size_t>& selectedChoice)const
	{
		int d = m_options.m_clean[0];

		size_t count = 0;
		for (int yy = y - d; (yy <= y + d) && (count <= m_options.m_clean[1]); yy++)
		{
			if (yy >= 0 && yy < firstChoice.size_y())
			{
				for (int xx = (x - d); (xx <= x + d) && (count <= m_options.m_clean[1]); xx++)
				{
					if (xx >= 0 && xx < firstChoice.size_x())
					{
						//if (cloudsCleaner.IsCloud(DTCode[yy][xx]))
						if (firstChoice[yy][xx] != NOT_INIT && firstChoice[yy][xx] != selectedChoice[yy][xx])
							count++;
					}
				}
			}
		}

		return count <= m_options.m_clean[1];
	}

	//bool CMergeImages::IsIsolatedPixel(int x, int y, CLandsatCloudCleaner& cloudsCleaner, const CMatrix<int>& DTCode)const
	//{
	//	int d = 2*m_options.m_clean +1;

	//	boost::dynamic_bitset<int> line();
	//	//vector<boost::dynamic_bitset<int>> scan(line, 2 *d);

	//	size_t count = 0;
	//	for (int yy = y - d; (yy <= y + d) && (count <= m_options.m_clean); yy++)
	//	{
	//		if (yy >= 0 && yy < DTCode.size_y())
	//		{
	//			for (int xx = (x - d); (xx <= x + d) && (count <= m_options.m_clean); xx++)
	//			{
	//				if (xx >= 0 && xx < DTCode.size_x())
	//				{
	//					if (cloudsCleaner.IsCloud(DTCode[yy][xx]))
	//						count++;
	//				}
	//			}
	//		}
	//	}

	//	return count <= m_options.m_clean;
	//}

}