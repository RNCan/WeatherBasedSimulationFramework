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
	const char* CMergeImagesOption::MERGE_TYPE_NAME[NB_MERGE_TYPE] = { "Oldest", "Newest", "MaxNDVI", "Best", "SecondBest", "BestNew" };
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
		m_bust = { { 0, 255 } };
		m_clean = { { 0, 0 } };
		m_ring = 0;
		m_scenesSize = SCENES_SIZE;
		m_TM = CTM::ANNUAL;
		m_nbPixelDT = 0;
		m_nbPixel = 0;
		m_QA = 0;
		m_QAmiss = 100;
		m_meanDmax=-1;

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
			{ "-Bust", 2, "min max", false, "replace busting pixel (lesser than min or greather than max) by no data." },
			{ "-QA", 2, "nbPix miss", false, "add nbPix buffer arround the pixel to compute QA and missing QA wille be replace by miss." },
			{ "-Mean", 1, "Dmax", false, "Compute mean of the 2 first pixels if the normalized visual distance (RGB) is lesser than Dmax" },
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
		if (!cloudsCleaner.empty() && m_options.m_clean[0] > 0)
			nbPass++;
		if (!cloudsCleaner.empty() && m_options.m_ring > 0)
			nbPass += m_options.m_ring;

		m_options.ResetBar(extents.m_xSize*extents.m_ySize*nbPass);

		std::array< OutputData, NB_THREAD_PROCESS> outputData;
		std::array< DebugData, NB_THREAD_PROCESS> debugData;
		std::array< OutputData, NB_THREAD_PROCESS> statsData;
		for (size_t i = 0; i < NB_THREAD_PROCESS; i++)
			InitMemory(inputBH.GetSceneSize(), extents.GetBlockSize(), outputData[i], debugData[i], statsData[i]);




		vector<pair<int, int>> XYindex = extents.GetBlockList(3, 3);

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
			//CTPeriod p = inputDS.GetPeriod();
			//CTPeriod period(CTRef(p.Begin().GetYear() - 1, 0, 0), CTRef(p.End().GetYear() + 1, LAST_MONTH, LAST_DAY));
			//options.m_period = period;

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
		//mosaicBH.LoadBlock(extents, period);
		mosaicBH.LoadBlock(extents);

		m_options.m_timerRead.Stop();
	}
	}

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
			CMatrix<size_t> secondBest(blockSize.m_y, blockSize.m_x);
			CMatrix<int> DTCode(blockSize.m_y, blockSize.m_x);
			int nbTriggerUsed = 0;
			CLandsatWindow mosaicWindow;


			if (!mosaicBH.IsEmpty())
				mosaicWindow = static_cast<CLandsatWindow&>(mosaicBH.GetWindow());


			if (!cloudsCleaner.empty() && (m_options.m_clean[0] > 0 || m_options.m_ring > 0))
			{
				firstChoice.resize(blockSize.m_y, blockSize.m_x);
				selectedChoice.resize(blockSize.m_y, blockSize.m_x);
			//	secondBest.resize(blockSize.m_y, blockSize.m_x);
			}
			
#pragma omp parallel for num_threads( m_options.m_CPU ) if (m_options.m_bMulti ) 
			for (int y = 0; y < blockSize.m_y; y++)
			{
				for (int x = 0; x < blockSize.m_x; x++)
				{
					Test1Vector imageList;
					CTRefSet Treference;
					CStatisticVector stats(window.GetSceneSize());

					//process all bands
					for (size_t s = 0; s < window.GetNbScenes(); s++)
					{
						//Get pixel
						CLandsatPixel pixel = window.GetPixel(s, x, y);
						if (window.IsValid(s, pixel))
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


					//get first choice before erase
					if (!firstChoice.empty())
						firstChoice[y][x] = get_iz(imageList, m_options.m_mergeType);

					//clean clouds
					if (!cloudsCleaner.empty())
					{
						//now looking for all other images
						Test1Vector::iterator it1 = get_it(imageList, m_options.m_mergeType);
						while (it1 != imageList.end())
						{
							size_t iz1 = it1->second;
							CLandsatPixel pixel1 = window.GetPixel(iz1, x, y);
							CTRef TRef = m_options.GetTRef(int(pixel1[JD]));

							bool bTrig_B1 = false;
							bool bTrig_TCB = false;

							//Get pixel
							CLandsatPixel pixel2;
							CLandsatPixel pixel3;

							Test1Vector::iterator it2 = get_it(imageList, CMergeImagesOption::SECOND_BEST);
							while (it2 != imageList.end())
							{
								size_t iz2 = it2->second;
								CLandsatPixel pixel = window.GetPixel(iz2, x, y);
								ASSERT(pixel2.IsInit());

								if (IsCloud(cloudsCleaner, pixel1, pixel))
								{
									imageList.erase(it2);
									it2 = get_it(imageList, CMergeImagesOption::SECOND_BEST);
								}
								else
								{
									pixel2 = pixel;
									it2 = imageList.end();
								}
							}



							if (!mosaicBH.IsEmpty())
							{
								//for (size_t mz = 0; mz < mosaicWindow.GetNbScenes() && !pixel2.IsInit(); mz++)
								//Test1Vector::iterator it2 = get_it(imageList, CMergeImagesOption::SECOND_BEST);
								//size_t mz = 
								//while (it2 != imageList.end())
								//{
									//inputDS.GetScenePeriod()[mz].Intersect()
								//CTRef TRef2 = m_options.GetTRef(int(pixelM[JD]));
								//		if (TRef2.GetYear() == TRef.GetYear() - 1 || (TRef2.GetYear() == TRef.GetYear() + 2 && !pixel2.IsInit()))
								//pixel2 = pixelM;
								//	if (TRef2.GetYear() == TRef.GetYear() - 2 || TRef2.GetYear() == TRef.GetYear() + 1)
								//	pixel_p1 = pixelM;
								//if (!IsCloud(cloudsCleaner, pixel, pixel2))
								//we assume there arew no cloud in mosaic

								pixel3 = mosaicWindow.GetPixel(0, x, y);
								//if (mosaicWindow.IsValid(0, pixel))
									//pixel3 = pixel;
							}


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


							if (bTrig_B1 || bTrig_TCB)
							{
#pragma omp atomic
								m_options.m_nbPixelDT++;

								if (pixel2.IsInit())
									DTCode[y][x] = cloudsCleaner.GetDTCode(pixel2, pixel1);
								else
									DTCode[y][x] = cloudsCleaner.GetDTCode(pixel3, pixel1);

								if (cloudsCleaner.IsSecondCloud(DTCode[y][x]))
								{
									imageList.erase(it1);
									it1 = get_it(imageList, m_options.m_mergeType);
								}
								else
								{
									it1 = imageList.end();
								}
							}
							else
							{
								it1 = imageList.end();
							}
						}//while not good pixel
					}//if remove clouds


					//find input temporal index
					size_t iz = get_iz(imageList, m_options.m_mergeType);
					size_t iz² = get_iz(imageList, CMergeImagesOption::SECOND_BEST);

					//get first choice before erase
					if (!selectedChoice.empty())
						selectedChoice[y][x] = iz;//clouds was remove

					if (!secondBest.empty())
						secondBest[y][x] = iz²;

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

						Test1Vector::iterator it = get_it(imageList, m_options.m_mergeType);
						debugData[CMergeImagesOption::D_CAPTOR][y][x] = cap;
						debugData[CMergeImagesOption::D_PATH][y][x] = col;
						debugData[CMergeImagesOption::D_ROW][y][x] = row;
						debugData[CMergeImagesOption::D_YEAR][y][x] = TRef.GetYear();
						debugData[CMergeImagesOption::D_MONTH][y][x] = int(TRef.GetMonth()) + 1;
						debugData[CMergeImagesOption::D_DAY][y][x] = int(TRef.GetDay()) + 1;
						debugData[CMergeImagesOption::D_JDAY][y][x] = int(m_options.GetTRefIndex(TRef));
						debugData[CMergeImagesOption::NB_IMAGES][y][x] = int(imageList.size());
						debugData[CMergeImagesOption::SCENE][y][x] = (int)iz;
						debugData[CMergeImagesOption::SORT_TEST][y][x] = it->first.GetRef();
						debugData[CMergeImagesOption::NB_TRIGGERS][y][x] = nbTriggerUsed;
					}

					if (m_options.m_bCreateImage)
					{
						CLandsatPixel pixel = GetPixel(window, iz, iz², x, y);
						for (size_t z = 0; z < SCENES_SIZE; z++)
							outputData[z][y][x] = (float)outputDS.PostTreatment(pixel[z], z);
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
			if (m_options.m_bCreateImage && !cloudsCleaner.empty())
			{
				if (m_options.m_clean[0] > 0)
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
									secondBest[y][x] = selectedChoice[y][x];
									selectedChoice[y][x] = firstChoice[y][x];
									CLandsatPixel pixel = GetPixel(window, selectedChoice[y][x], secondBest[y][x], x, y);
									for (size_t z = 0; z < SCENES_SIZE; z++)
										outputData[z][y][x] = (float)outputDS.PostTreatment(pixel[z], z);
								}
							}
							m_options.m_xx++;
						}//x
						m_options.UpdateBar();
					}//y
				}
				//second grow pixel ring
				if (m_options.m_ring > 0)
				{
					CMatrix<size_t> selectedChoice2 = selectedChoice;
					CMatrix<size_t> secondBest2 = secondBest;
					for (int r = 0; r < m_options.m_ring; r++)
					{
						for (int y = 0; y < blockSize.m_y; y++)
						{
							for (int x = 0; x < blockSize.m_x; x++)
							{
								if (firstChoice[y][x] != NOT_INIT && selectedChoice[y][x] != NOT_INIT && firstChoice[y][x] != selectedChoice[y][x])
								{
									//for (int yy = y - m_options.m_ring; yy <= (y + m_options.m_ring); yy++)
									for (int yy = y - 1; yy <= (y + 1); yy++)
									{
										if (yy >= 0 && yy < blockSize.m_y)
										{
											//for (int xx = x - m_options.m_ring; xx <= (x + m_options.m_ring); xx++)
											for (int xx = x - 1; xx <= x + 1; xx++)
											{
												if (xx >= 0 && xx < blockSize.m_x && xx != yy)
												{
													//
													if (firstChoice[yy][xx] != NOT_INIT && selectedChoice[yy][xx] != NOT_INIT && selectedChoice[yy][xx] == firstChoice[yy][xx])
													{
														//if (secondBest[yy][xx] != NOT_INIT)
														//{
														//size_t iz = selectedChoice[y][x];
														//size_t iz = secondBest[yy][xx];


														//because we can't change selectedChoise, we have to chnage firstChoice instead
														//firstChoice[yy][xx] = selectedChoice[y][x];

														CLandsatPixel pixel = GetPixel(window, selectedChoice[y][x], secondBest[y][x], xx, yy);
														//CLandsatPixel pixel = GetPixel(window, secondBest[y][x], secondBest[yy][xx], xx, yy);

														if (window.IsValid(selectedChoice[y][x], pixel))
														{
															selectedChoice2[yy][xx] = selectedChoice[y][x];
															secondBest2[yy][xx] = secondBest[y][x];

															for (size_t z = 0; z < SCENES_SIZE; z++)
																outputData[z][yy][xx] = (float)outputDS.PostTreatment(pixel[z], z);
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
						secondBest = secondBest2;
					}//ring
				}//if ring
			}//cloud cleaner

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
				//take the pixel with the lowest score.
				if (imageList.size() >= 2)
				{
					it = imageList.begin(); 
					it++;
				}
					
			}
			else if (type == CMergeImagesOption::BEST_NEWEST)
			{
				//take the pixel with the lowest score.
				it = imageList.begin();
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
						if (firstChoice[yy][xx] != NOT_INIT && firstChoice[yy][xx] != selectedChoice[yy][xx])
							count++;
					}
				}
			}
		}

		return count <= m_options.m_clean[1];
	}


	__int16 CMergeImages::GetMeanQA(CLandsatWindow& window, CGeoSize size, size_t iz, int x, int y)
	{
		CStatistic sumQA;

		int d = m_options.m_QA;

		size_t count = 0;
		for (int yy = y - d; (yy <= y + d); yy++)
		{
			for (int xx = (x - d); (xx <= x + d); xx++)
			{
				//CGeoSize size = window.GetGeoSize();
				if (xx >= 0 && xx < size.m_x && yy >= 0 && yy < size.m_y)
				{
					CLandsatPixel pixel = window.GetPixel(iz, xx, yy);
					if (window.IsValid(iz, pixel))
						sumQA += pixel[QA];
					else
						sumQA += m_options.m_QAmiss;//add bad value for missing pixel
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