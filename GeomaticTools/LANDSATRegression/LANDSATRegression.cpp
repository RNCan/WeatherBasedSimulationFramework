//***********************************************************************
// program to analyze bands and report some information on changing
//									 
//***********************************************************************
// version 
// 1.2.0	13/09/2023	Rémi Saint-Amant	Compile with GDAL 3.7.1
// 1.1.0	20/12/2021	Rémi Saint-Amant	Compile with VS 2019 and GDAL 3.0.3
// 1.0.3	22/05/2018	Rémi Saint-Amant	Compile with VS 2017
// 1.0.2	04/07/2015	Rémi Saint-Amant	Bug correction in blocks
// 1.0.1	03/07/2015	Rémi Saint-Amant	Bug correction in blocks
// 1.0.0	02/07/2015	Rémi Saint-Amant	Creation
//-ExportBands -period "U:\GIS\#documents\TestCodes\SlowDisturbanceAnalyser\Input\1999-2014_begin.tif" "U:\GIS\#documents\TestCodes\SlowDisturbanceAnalyser\Input\1999-2014_end.tif" -of VRT -multi -IOCPU 4 --config GDAL_CACHEMAX 512 -ot Int16 -co "compress=LZW" -overwrite "U:\GIS\#documents\TestCodes\SlowDisturbanceAnalyser\Model\V5_T1T2_V4add_nocloud_insect" "U:\GIS\#documents\TestCodes\SlowDisturbanceAnalyser\Input\1999-2014.tif" "U:\GIS\#documents\TestCodes\SlowDisturbanceAnalyser\Output\TBE2.vrt"




//"U:\GIS1\LANDSAT_SR\mos\20141219_mergeiexe\step3_cloudfree_v2\mystack9914.vrt"
//-mask  "U:\GIS\#projets\LAQ\DATA\MASK_theme\LCVsel_crop_ice_mywat_han00_noUSA.tif" -maskValue 1


//%BSOPE% %tCN% %mask% %VRT_LST% %outpath%Test.vrt
//-mask  "U:\GIS\#projets\LAQ\DATA\MASK_theme\LCVsel_crop_ice_mywat_han00_noUSA.tif" -maskValue 1
//-of VRT -co "tiled=YES" -co "BLOCKXSIZE=1024" -co "BLOCKYSIZE=1024" -blocksize 5120 5120 -ot Int16 -multi -dstnodata -32768 -IOCPU 2 -co "compress=LZW" -co "BIGTIFF=YES" --config GDAL_CACHEMAX 648 -te 1744410 6978480 1798800 7020540 -Period "U:\GIS\#projets\LAQ\ANALYSE_CA\20150623_SR_run9\TREND_Remi\t2_bande9_mask_dtchg.tif" "U:\GIS\#projets\LAQ\ANALYSE_CA\20150623_SR_run9\TREND_Remi\t6_bande9_mask_dtchg.tif" "U:\GIS1\LANDSAT_SR\mos\20141219_mergeiexe\step3_cloudfree_v2\mystack9914.vrt" "U:\GIS\#documents\TestCodes\LandsatRegression\Output\test1.vrt"


#include "stdafx.h"
#include <float.h>
#include <math.h>
#include <array>
#include <utility>
#include <iostream>
#include <numeric>
#include <algorithm>

#include "LANDSATRegression.h"
#include "Basic/UtilTime.h"

#pragma warning(disable: 4275 4251)
#include "gdal_priv.h"


using namespace std;
using namespace WBSF::Landsat1;

namespace WBSF
{


	static char* REG_OUTPUT_NAME[NB_OUTPUT_REGRESSION] = { "5years", "begin_end", "entire" };
	static int REG_STATS[NB_REGRESSION_STATS] = { NB_VALUE, SLOPE, RMSE, STAT_R² };


	static const char* version = "1.2.0";
	string CLANDSATRegression::GetDescription() { return  string("LANDSATRegression version ") + version + " (" + _T(__DATE__) + ")\n"; }
	static const int NB_THREAD_PROCESS = 2;


	CLANDSATRegressionOptions::CLANDSATRegressionOptions()
	{
		m_outputType = GDT_Float32;
		m_dstNodata = WBSF::GetDefaultNoData(GDT_Float32);
		m_scenesSize = SCENES_SIZE;
		m_RFactor = 1000;

		m_appDescription = "This software compute regression in a time series of landsat scenes (composed by 9 images by scene)";

		AddOption("-TTF");
		AddOption("-Period");

		static const COptionDef OPTIONS[] =
		{
			//{ "-Despike", 2, "dt dh", true, "Despike to remove invalid pixel. dt is the despike type and th is the despike threshold. Supported type are \"NBR\",\"B5\",\"EUCLIDEAN\", \"NDVI\", \"NDMI\", \"TCB\" (Tasseled Cap Brightness), \"TCG\" (Tasseled Cap Greenness) or \"TCW\" (Tasseled Cap Wetness)." },
			{ "-RFactor", 1, "f", false, "multiplication factor for output. 1000 by default." },
			{ "-Period", 2, "begin end", false, "file path of the begginning and ending image period in Julian day 1970." },
			{ "srcfile", 0, "", false, "Input LANDSAT image file path." },
			{ "dstfile", 0, "", false, "Output image file path." }
		};

		for (int i = 0; i < sizeof(OPTIONS) / sizeof(COptionDef); i++)
			AddOption(OPTIONS[i]);

		static const CIOFileInfoDef IO_FILE_INFO[] =
		{
			{ "Input Image", "srcfile", "", "ScenesSize(9)*nbScenes", "B1: Landsat band 1|B2: Landsat band 2|B3: Landsat band 3|B4: Landsat band 4|B5: Landsat band 5|B6: Landsat band 6|B7: Landsat band 7|QA: Image quality|B9: Date(Julian day 1970 or YYYYMMDD format)|... for each scene" },
			{ "Ouput Image", "dstfile", "", "3 periods*4 statistics", "n: number of values|slope: slope of NBR in function of years|RMSE|R²|... for 3 variables (5 years periods, begin end period, entire period" },

		};

		for (int i = 0; i < sizeof(IO_FILE_INFO) / sizeof(CIOFileInfoDef); i++)
			AddIOFileInfo(IO_FILE_INFO[i]);

	}


	ERMsg CLANDSATRegressionOptions::ParseOption(int argc, char* argv[])
	{
		ERMsg msg = CBaseOptions::ParseOption(argc, argv);

		ASSERT(CLANDSATRegression::NB_FILE_PATH == 2);
		if (msg && m_filesPath.size() != CLANDSATRegression::NB_FILE_PATH)
		{
			msg.ajoute("Invalid argument line. 2 files are needed: the LANDSAT image and output image.");
			msg.ajoute("Argument found: ");
			for (size_t i = 0; i < m_filesPath.size(); i++)
				msg.ajoute("   " + to_string(i + 1) + "- " + m_filesPath[i]);
		}

		return msg;
	}

	ERMsg CLANDSATRegressionOptions::ProcessOption(int& i, int argc, char* argv[])
	{
		ERMsg msg;

		//if (EQUAL(argv[i], "-Despike"))
		//{
		//	string str = argv[++i];
		//	TIndices type = GetIndiceType(str);
		//	//string op = argv[++i];
		//	double threshold = atof(argv[++i]);
		//	double min_trigger = atof(argv[++i]);

		//	if (type < NB_INDICES)
		//	{
		//		m_despike.push_back(CIndices(type, "<", threshold, min_trigger));
		//	}
		//	else
		//	{
		//		msg.ajoute(str + " is an invalid type for -Despike option");
		//	}
		//}
		//else
		if (EQUAL(argv[i], "-RFactor"))
		{
			m_RFactor = atof(argv[++i]);
		}
		else if (EQUAL(argv[i], "-period"))
		{
			m_beginFilePath = argv[++i];
			m_endFilePath = argv[++i];
		}
		else
		{
			//Look to see if it's a know base option
			msg = CBaseOptions::ProcessOption(i, argc, argv);
		}

		return msg;
	}

	//**********************************************************************************************************************************************
	//CLANDSATRegression


	ERMsg CLANDSATRegression::OpenAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& beginDS, CGDALDatasetEx& endDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS)
	{
		ERMsg msg;


		if (!m_options.m_bQuiet)
			cout << endl << "Open input image..." << endl;

		msg = inputDS.OpenInputImage(m_options.m_filesPath[INPUT_FILE_PATH], m_options);

		if (msg)
		{

			inputDS.UpdateOption(m_options);

			if (!m_options.m_bQuiet)
			{
				CGeoExtents extents = inputDS.GetExtents();
				CProjectionPtr pPrj = inputDS.GetPrj();
				string prjName = pPrj ? pPrj->GetName() : "Unknown";

				cout << "    Size           = " << inputDS->GetRasterXSize() << " cols x " << inputDS->GetRasterYSize() << " rows x " << inputDS.GetRasterCount() << " bands" << endl;
				cout << "    Extents        = X:{" << ToString(extents.m_xMin) << ", " << ToString(extents.m_xMax) << "}  Y:{" << ToString(extents.m_yMin) << ", " << ToString(extents.m_yMax) << "}" << endl;
				cout << "    Projection     = " << prjName << endl;
				cout << "    NbBands        = " << inputDS.GetRasterCount() << endl;
				cout << "    Scene size     = " << inputDS.GetSceneSize() << endl;
				cout << "    Nb. Scenes     = " << inputDS.GetNbScenes() << endl;
				cout << "    First image    = " << inputDS.GetPeriod().Begin().GetFormatedString() << endl;
				cout << "    Last image     = " << inputDS.GetPeriod().End().GetFormatedString() << endl;
				cout << "    Input period   = " << m_options.m_period.GetFormatedString() << endl;
			}
		}


		if (msg && !m_options.m_maskName.empty())
		{
			if (!m_options.m_bQuiet)
				cout << "Open mask image..." << endl;
			msg += maskDS.OpenInputImage(m_options.m_maskName);
		}

		if (!m_options.m_beginFilePath.empty())
			msg += beginDS.OpenInputImage(m_options.m_beginFilePath);

		if (!m_options.m_endFilePath.empty())
			msg += endDS.OpenInputImage(m_options.m_endFilePath);


		if (msg && m_options.m_bCreateImage)
		{
			if (!m_options.m_bQuiet)
				cout << endl << "Create output image..." << endl;

			string filePath = m_options.m_filesPath[OUTPUT_FILE_PATH];
			CLANDSATRegressionOptions options = m_options;
			options.m_nbBands = NB_OUTPUT_REGRESSION*NB_REGRESSION_STATS;
			//options.m_outputType = GDT_Float32;
			//options.m_dstNodata = ::GetDefaultNoData(GDT_Float32);


			for (size_t i = 0; i < NB_OUTPUT_REGRESSION; i++)
				for (size_t s = 0; s < NB_REGRESSION_STATS; s++)
					options.m_VRTBandsName += string(GetFileTitle(filePath)) + "_" + REG_OUTPUT_NAME[i] + "_" + CStatistic::GetName(REG_STATS[s]) + ".tif|";

			msg += outputDS.CreateImage(filePath, options);
		}


		return msg;
	}


	ERMsg CLANDSATRegression::Execute()
	{
		ERMsg msg;

		if (!m_options.m_bQuiet)
		{
			cout << "Output: " << m_options.m_filesPath[OUTPUT_FILE_PATH] << endl;
			cout << "From:   " << m_options.m_filesPath[INPUT_FILE_PATH] << endl;

			if (!m_options.m_maskName.empty())
				cout << "Mask:   " << m_options.m_maskName << endl;
		}

		GDALAllRegister();



		CLandsatDataset inputDS;
		CGDALDatasetEx beginDS;
		CGDALDatasetEx endDS;
		CGDALDatasetEx maskDS;
		CGDALDatasetEx outputDS;


		msg = OpenAll(inputDS, beginDS, endDS, maskDS, outputDS);

		if (msg)
		{
			size_t nbScenes = inputDS.GetNbScenes();
			size_t sceneSize = inputDS.GetSceneSize();
			CBandsHolderMT bandHolder(1, m_options.m_memoryLimit, m_options.m_IOCPU, NB_THREAD_PROCESS);

			if (maskDS.IsOpen())
				bandHolder.SetMask(maskDS.GetSingleBandHolder(), m_options.m_maskDataUsed);

			msg += bandHolder.Load(inputDS, m_options.m_bQuiet, m_options.m_extents, m_options.m_period);

			if (!msg)
				return msg;


			if (!m_options.m_bQuiet && outputDS.IsOpen())
				cout << "Create output images x(" << outputDS.GetRasterXSize() << " C x " << outputDS.GetRasterYSize() << " R x " << outputDS.GetRasterCount() << " B) with " << m_options.m_CPU << " threads..." << endl;

			//CBandsHolder bandHolderBegin(1, m_options.m_memoryLimit, m_options.m_IOCPU);
			//bandHolderBegin.Load(beginDS)


			CGeoExtents extents = bandHolder.GetExtents();
			m_options.ResetBar((size_t)extents.m_xSize*extents.m_ySize);

			vector<pair<int, int>> XYindex = extents.GetBlockList();

			omp_set_nested(1);
#pragma omp parallel for schedule(static, 1) num_threads(NB_THREAD_PROCESS) if (m_options.m_bMulti)
			for (int b = 0; b < (int)XYindex.size(); b++)
			{
				int threadBlockNo = ::omp_get_thread_num();
				int xBlock = XYindex[b].first;
				int yBlock = XYindex[b].second;

				//data
				RegressionVector data;

				ReadBlock(xBlock, yBlock, bandHolder[threadBlockNo]);
				ProcessBlock(xBlock, yBlock, bandHolder[threadBlockNo], beginDS, endDS, data);
				WriteBlock(xBlock, yBlock, bandHolder[threadBlockNo], data, outputDS);

			}//for all blocks

			//close inputs
			CloseAll(inputDS, beginDS, endDS, maskDS, outputDS);
		}

		return msg;
	}

	void CLANDSATRegression::ReadBlock(int xBlock, int yBlock, CBandsHolder& bandHolder)
	{
#pragma omp critical(BlockIO)
	{

		m_options.m_timerRead.Start();
		bandHolder.LoadBlock(xBlock, yBlock, m_options.m_period);
		m_options.m_timerRead.Stop();
	}
	}

	//Get input image reference
	void CLANDSATRegression::ProcessBlock(int xBlock, int yBlock, const CBandsHolder& bandHolder, CGDALDatasetEx& beginDS, CGDALDatasetEx& endDS, RegressionVector& data)
	{
		size_t nbScenes = bandHolder.GetNbScenes();
		size_t sceneSize = bandHolder.GetSceneSize();
		CGeoExtents entireExtents = bandHolder.GetExtents();
		CGeoExtents extents = entireExtents.GetBlockExtents(xBlock, yBlock);
		CGeoSize blockSize = extents.GetSize();

		__int16 noData = (__int16)GetDefaultNoData(GDT_Int16);
		if (beginDS.IsOpen() && endDS.IsOpen())
			(__int16)beginDS.GetNoData(0);

		if (bandHolder.IsEmpty())
		{
			size_t nbCells = (size_t)extents.m_xSize*extents.m_ySize;
#pragma omp atomic		
			m_options.m_xx += (min(nbCells, (size_t)blockSize.m_x*blockSize.m_y));

			m_options.UpdateBar();
			return;
		}

#pragma omp critical(ProcessBlock)
	{
		m_options.m_timerProcess.Start();



		for (size_t i = 0; i < NB_OUTPUT_REGRESSION; i++)
			for (size_t s = 0; s < NB_REGRESSION_STATS; s++)
				data[i][s].insert(data[i][s].begin(), blockSize.m_x*blockSize.m_y, (float)m_options.m_dstNodata);

		CLandsatWindow window = static_cast<CLandsatWindow&>(bandHolder.GetWindow());

		//process all x and y 
#pragma omp parallel for schedule(static, 1) num_threads( m_options.m_CPU ) if (m_options.m_bMulti)  
		for (int y = 0; y < blockSize.m_y; y++)
		{
			for (int x = 0; x < blockSize.m_x; x++)
			{
				int threadNo = ::omp_get_thread_num();

				CTPeriod validPeriod[NB_OUTPUT_REGRESSION];
				if (beginDS.IsOpen() && endDS.IsOpen())
				{
					CGeoPoint pt = extents.XYPosToCoord(CGeoPointIndex(x, y));
#pragma omp critical(BlockIO)
					{
						CGeoPointIndex xy;
						xy = beginDS.GetExtents().CoordToXYPos(pt);
						int b = (int)beginDS.ReadPixel(0, xy);
						xy = endDS.GetExtents().CoordToXYPos(pt);
						int e = (int)endDS.ReadPixel(0, xy);

						if (b != noData && e != noData)
						{
							validPeriod[O_FIVE].Begin() = m_options.GetTRef(b);
							validPeriod[O_FIVE].End() = CTRef(validPeriod[O_FIVE].Begin().GetYear() + 4, LAST_MONTH, LAST_DAY);

							validPeriod[O_BEGIN_END].Begin() = m_options.GetTRef(b);
							validPeriod[O_BEGIN_END].End() = m_options.GetTRef(e);
						}

						validPeriod[O_ALL] = m_options.m_period;
					}
				}

				CStatisticXY statXY[NB_OUTPUT_REGRESSION];

				for (size_t s = 0; s < window.GetNbScenes() - 1; s++)
				{
					CLandsatPixel pixel = window.GetPixel(s, x, y);

					if (pixel.IsValid())
					{
						double date = pixel.GetTRef().GetYear() + (double)pixel.GetTRef().GetJDay() / pixel.GetTRef().GetNbDaysPerYear();

						for (size_t i = 0; i < NB_OUTPUT_REGRESSION; i++)
							if (validPeriod[i].IsInit() && validPeriod[i].IsInside(pixel.GetTRef()))
								statXY[i].Add(date, pixel.NBR()*m_options.m_RFactor);

					}//pixel 1 is valid
				}//for all years

				for (size_t i = 0; i < NB_OUTPUT_REGRESSION; i++)
				{
					if (statXY[i][NB_VALUE] >= 3)
					{
						for (size_t s = 0; s < NB_REGRESSION_STATS; s++)
							data[i][s][y*blockSize.m_x + x] = (float)statXY[i][REG_STATS[s]];
					}
				}

#pragma omp atomic	
				m_options.m_xx++;
			}//for x


			m_options.UpdateBar();
		}//for y

		m_options.m_timerProcess.Stop();
	}
	}

	void CLANDSATRegression::WriteBlock(int xBlock, int yBlock, const CBandsHolder& bandHolder, RegressionVector& data, CGDALDatasetEx& outputDS)
	{
#pragma omp critical(BlockIO)
	{
		m_options.m_timerWrite.Start();

		CGeoExtents extents = bandHolder.GetExtents();
		CGeoSize blockSize = extents.GetBlockSize(xBlock, yBlock);
		CGeoRectIndex outputRect = extents.GetBlockRect(xBlock, yBlock);

		ASSERT(outputRect.Width() == blockSize.m_x);
		ASSERT(outputRect.Height() == blockSize.m_y);

		if (m_options.m_bCreateImage)
		{
			float noData = (float)m_options.m_dstNodata;
			ASSERT(outputDS.GetRasterCount() == NB_OUTPUT_REGRESSION*NB_REGRESSION_STATS);

			for (size_t i = 0; i < NB_OUTPUT_REGRESSION; i++)
			{
				for (size_t s = 0; s < NB_REGRESSION_STATS; s++)
				{
					GDALRasterBand *pBand = outputDS.GetRasterBand(i*NB_REGRESSION_STATS + s);

					if (!data[i][s].empty())
					{
						ASSERT(data[i][s].size() == blockSize.m_x*blockSize.m_y);
						pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(data[i][s][0]), outputRect.Width(), outputRect.Height(), GDT_Float32, 0, 0);
					}//for all bands in a scene
					else
					{
						pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(noData), 1, 1, GDT_Float32, 0, 0);
					}

					pBand->FlushCache();
				}
			}
		}//regression
		m_options.m_timerWrite.Stop();
	}
	}

	void CLANDSATRegression::CloseAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& beginDS, CGDALDatasetEx& endDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS)
	{
		if (!m_options.m_bQuiet)
			cout << endl << "Close all files..." << endl;

		//close input
		inputDS.Close();
		beginDS.Close();
		endDS.Close();
		maskDS.Close();


		//close output
		m_options.m_timerWrite.Start();
		//if (m_options.m_bComputeStats)
			//outputDS.ComputeStats(m_options.m_bQuiet);

		//if (!m_options.m_overviewLevels.empty())
			//outputDS.BuildOverviews(m_options.m_overviewLevels, m_options.m_bQuiet);

		outputDS.Close(m_options);

		m_options.m_timerWrite.Stop();
		m_options.PrintTime();
	}


	int _tmain(int argc, _TCHAR* argv[])
	{
		CTimer timer(true);

		CLANDSATRegression trendsAnalyser;
		ERMsg msg = trendsAnalyser.m_options.ParseOption(argc, argv);

		if (!msg || !trendsAnalyser.m_options.m_bQuiet)
			cout << trendsAnalyser.GetDescription() << endl;


		if (msg)
			msg = trendsAnalyser.Execute();

		if (!msg)
		{
			PrintMessage(msg);
			return -1;
		}

		timer.Stop();

		if (!trendsAnalyser.m_options.m_bQuiet)
			cout << endl << "Total time = " << SecondToDHMS(timer.Elapsed()) << endl;

		return 0;

	}




}