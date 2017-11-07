//***********************************************************************
// program to merge Landsat image image over a period
//									 
//***********************************************************************
// version
// 1.0.0	01/11/2017	Rémi Saint-Amant	Creation



//-FirstYear 1984 -RMSEThreshold 75 -MaxBreaks 5 -of VRT "U:\GIS\#documents\TestCodes\Segmentation\Input\1984-2016.vrt" "U:\GIS\#documents\TestCodes\Segmentation\output\1984-2016.vrt"

#include "stdafx.h"
#include <math.h>
#include <array>
#include <utility>
#include <iostream>
#include <boost\dynamic_bitset.hpp>

#include "Segmentation.h"
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
	const char* CSegmentation::VERSION = "1.0.0";
	const size_t CSegmentation::NB_THREAD_PROCESS = 2;
	//static const int NB_TOTAL_STATS = CSegmentationOption::NB_STATS*SCENES_SIZE;


	//*********************************************************************************************************************

	CSegmentationOption::CSegmentationOption()
	{

		m_scenesSize = 1;
		m_RMSEThreshold = 0;
		m_maxLayers = 5;
		m_firstYear = 0;
		m_bDebug = false;

		m_appDescription = "This software find breaks in NBR value of Landsat scenes (composed of " + to_string(SCENES_SIZE) + " bands).";

		static const COptionDef OPTIONS[] =
		{
			{ "-RMSEThreshold", 1, "t", false, "RMSE threshold of the NBR value to continue breaking series. 0 by default." },
			{ "-MaxBreaks", 1, "n", false, "Maximum number of breaks. 3 by default for a total of 5 output layers" },
			{ "-FirstYear", 1, "n", false, "Specify year of the first image. Return year instead of index. By default, return the image index (0..nbImages-1)" },
			//{ "-Standardized", 0, "", false, "Standardize input." },
			{ "-Debug",  0,"",false,"Output debug information."},
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
			{ "Output Image", "dstfile", "", "NbOutputLayers=maxBreaks+2", "break's index", "" },
			{ "Optional Output Image", "dstfile_debug","1","NbOutputLayers+1","Nb breaks: number of breaks found|NBR1: NBR of breaks1|... for all breaks"}
		};

		for (int i = 0; i < sizeof(IO_FILE_INFO) / sizeof(CIOFileInfoDef); i++)
			AddIOFileInfo(IO_FILE_INFO[i]);
	}

	ERMsg CSegmentationOption::ParseOption(int argc, char* argv[])
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

	ERMsg CSegmentationOption::ProcessOption(int& i, int argc, char* argv[])
	{
		ERMsg msg;

		
		if (IsEqual(argv[i], "-RMSEThreshold"))
		{
			m_RMSEThreshold = atof(argv[++i]);
		}
		else if (IsEqual(argv[i], "-MaxBreaks"))
		{
			m_maxLayers = atoi(argv[++i])+2;//to add begin and end
		}
		else if (IsEqual(argv[i], "-FirstYear"))
		{
			m_firstYear = atoi(argv[++i]);
		}
		else if (IsEqual(argv[i], "-Debug"))
		{
			m_bDebug = true;
		}
		else
		{
			//Look to see if it's a know base option
			msg = CBaseOptions::ProcessOption(i, argc, argv);
		}

		return msg;
	}

	
	ERMsg CSegmentation::Execute()
	{
		ERMsg msg;

		if (!m_options.m_bQuiet)
		{
			cout << "Output: " << m_options.m_filesPath[CSegmentationOption::OUTPUT_FILE_PATH] << endl;
			cout << "From:   " << m_options.m_filesPath[CSegmentationOption::INPUT_FILE_PATH] << endl;
			cout << "MaxBreaks:   " << m_options.m_maxLayers-2 << endl;
			cout << "Threshold:   " << m_options.m_RMSEThreshold << endl;

			if (!m_options.m_maskName.empty())
				cout << "Mask:   " << m_options.m_maskName << endl;

		}

		GDALAllRegister();

		CGDALDatasetEx inputDS;
		CGDALDatasetEx maskDS;
		CGDALDatasetEx outputDS;
		CGDALDatasetEx debugDS;
		
		msg = OpenAll(inputDS, maskDS, outputDS, debugDS);
		if (!msg)
			return msg;


		
		CBandsHolderMT bandHolder(1, m_options.m_memoryLimit, m_options.m_IOCPU, NB_THREAD_PROCESS);

		if (maskDS.IsOpen())
			bandHolder.SetMask(maskDS.GetSingleBandHolder(), m_options.m_maskDataUsed);

		msg += bandHolder.Load(inputDS, m_options.m_bQuiet, m_options.m_extents, m_options.m_period);

		if (!msg)
			return msg;

		if (!m_options.m_bQuiet && m_options.m_bCreateImage)
			cout << "Create output images (" << outputDS.GetRasterXSize() << " C x " << outputDS.GetRasterYSize() << " R x " << outputDS.GetRasterCount() << " B) with " << m_options.m_CPU << " threads..." << endl;


		CGeoExtents extents = bandHolder.GetExtents();
		m_options.ResetBar(extents.m_xSize*extents.m_ySize);

		vector<pair<int, int>> XYindex = extents.GetBlockList();

		omp_set_nested(1);//for IOCPU
#pragma omp parallel for schedule(static, 1) num_threads( NB_THREAD_PROCESS ) if (m_options.m_bMulti) 
		for (int b = 0; b < (int)XYindex.size(); b++)
		{
			int thread = omp_get_thread_num();
			int xBlock = XYindex[b].first;
			int yBlock = XYindex[b].second;

			OutputData outputData;
			DebugData debugData;
			ReadBlock(xBlock, yBlock, bandHolder[thread]);
			ProcessBlock(xBlock, yBlock, bandHolder[thread], outputData, debugData);
			WriteBlock(xBlock, yBlock, outputDS, debugDS, outputData, debugData);
		}//for all blocks

		CloseAll(inputDS, maskDS, outputDS, debugDS);

		return msg;
	}



	ERMsg CSegmentation::OpenAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS, CGDALDatasetEx& debugDS)
	{
		ERMsg msg;

		if (!m_options.m_bQuiet)
			cout << endl << "Open input image..." << endl;

		msg = inputDS.OpenInputImage(m_options.m_filesPath[CSegmentationOption::INPUT_FILE_PATH], m_options);

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
				//cout << "    Scene size     = " << inputDS.GetSceneSize() << endl;
				//cout << "    Nb. Scenes     = " << inputDS.GetNbScenes() << endl;
				//cout << "    First image    = " << inputDS.GetPeriod().Begin().GetFormatedString() << endl;
				//cout << "    Last image     = " << inputDS.GetPeriod().End().GetFormatedString() << endl;
				//cout << "    Input period   = " << m_options.m_period.GetFormatedString() << endl;

				if (inputDS.GetRasterCount() < 2)
					msg.ajoute("Segmentation need at leat 2 bands");
			}
		}


		if (msg && !m_options.m_maskName.empty())
		{
			if (!m_options.m_bQuiet)
				cout << "Open mask image..." << endl;
			msg += maskDS.OpenInputImage(m_options.m_maskName);
		}
	
		if (m_options.m_bCreateImage)
		{
			//CTPeriod period = m_options.GetTTPeriod();

			CSegmentationOption options(m_options);
			options.m_nbBands = options.m_maxLayers;
			//options.m_nbBands = 1;// options.m_maxLayers;
			//options.m_createOptions.push_back("NBITS=2");
			//options.m_dstNodata = 3;

			if (!m_options.m_bQuiet)
			{
				cout << endl;
				cout << "Open output images..." << endl;
				cout << "    Size           = " << options.m_extents.m_xSize << " cols x " << options.m_extents.m_ySize << " rows x " << options.m_nbBands << " bands" << endl;
				cout << "    Extents        = X:{" << ToString(options.m_extents.m_xMin) << ", " << ToString(options.m_extents.m_xMax) << "}  Y:{" << ToString(options.m_extents.m_yMin) << ", " << ToString(options.m_extents.m_yMax) << "}" << endl;
			}

			//string filePath = options.m_filesPath[CSegmentationOption::OUTPUT_FILE_PATH];
			//for (size_t b = 0; b < SCENES_SIZE; b++)
				//options.m_VRTBandsName += GetBandFileTitle(filePath, b) + ".tif|";

			string filePath = options.m_filesPath[CSegmentationOption::OUTPUT_FILE_PATH];
			for (size_t s = 0; s < options.m_maxLayers; s++)
				options.m_VRTBandsName += GetFileTitle(filePath) + FormatA("_brk%02d.tif|", s+1);

			msg += outputDS.CreateImage(filePath, options);
		}

		if (m_options.m_bDebug)
		{
			CSegmentationOption options(m_options);
			options.m_nbBands = options.m_maxLayers+1;

			if (!m_options.m_bQuiet)
				cout << "Open debug images..." << endl;

			string filePath = options.m_filesPath[CSegmentationOption::OUTPUT_FILE_PATH] ;
			SetFileTitle(filePath, GetFileTitle(filePath) + "_debug");
			options.m_VRTBandsName = GetFileTitle(filePath) + "_nbBrk.tif|";
			for (size_t s = 0; s < options.m_maxLayers; s++)
				options.m_VRTBandsName += GetFileTitle(filePath) + FormatA("_brk%02d.tif|", s+1);

			msg += debugDS.CreateImage(filePath, options);
		}

		return msg;
	}

	void CSegmentation::ReadBlock(int xBlock, int yBlock, CBandsHolder& bandHolder)
	{
#pragma omp critical(BlockIO)
	{
		m_options.m_timerRead.Start();

//		CGeoExtents extents = m_options.m_extents.GetBlockExtents(xBlock, yBlock);
		bandHolder.LoadBlock(xBlock, yBlock);
		

		m_options.m_timerRead.Stop();
	}
	}

	void CSegmentation::ProcessBlock(int xBlock, int yBlock, CBandsHolder& bandHolder, OutputData& outputData, DebugData& debugData)
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

		

		//init memory
		if (m_options.m_bCreateImage)
		{
			outputData.resize(m_options.m_maxLayers);
			for (size_t s = 0; s < outputData.size(); s++)
				outputData[s].insert(outputData[s].begin(), blockSize.m_x*blockSize.m_y, m_options.m_dstNodata);
		}
		if (m_options.m_bDebug)
		{
			debugData.resize(m_options.m_maxLayers+1);
			for (size_t s = 0; s < debugData.size(); s++)
				debugData[s].insert(debugData[s].begin(), blockSize.m_x*blockSize.m_y, m_options.m_dstNodata);
		}

#pragma omp critical(ProcessBlock)
		{
			//CLandsatWindow window = static_cast<CLandsatWindow&>(bandHolder.GetWindow());
			CRasterWindow window = bandHolder.GetWindow();
			
			m_options.m_timerProcess.Start();
			
#pragma omp parallel for num_threads( m_options.m_CPU ) if (m_options.m_bMulti ) 
			for (int y = 0; y < blockSize.m_y; y++)
			{
				for (int x = 0; x < blockSize.m_x; x++)
				{
					int xy = y*blockSize.m_x + x;

					//Get pixel
					NBRVector data;
					data.reserve(window.size());
					for (size_t z = 0; z < window.size(); z++)
					{
						if (window[z]->IsValid(x, y))
							data.push_back(make_pair(window[z]->at(x, y), z));
					}

					//if there are more than 2 points
					if (data.size()>=2)
					{
						//compute segmentation for this time series
						std::vector<size_t> segment = Segmentation(data, m_options.m_maxLayers, m_options.m_RMSEThreshold);
						ASSERT(segment.size() <= outputData.size());
						
						//if need ouput
						if (!outputData.empty())
						{
							for (size_t s = 0; s < segment.size(); s++)
							{
								size_t z = segment[segment.size() - s - 1];//reverse order
								outputData[s][xy] = m_options.m_firstYear + (__int16)z;
							}
						}

						//if output debug
						if (!debugData.empty())
						{
							debugData[0][xy] = (__int16)segment.size();
							for (size_t s = 0; s < segment.size(); s++)
							{
								size_t z = segment[segment.size() - s - 1];//reverse order
								debugData[s + 1][xy] = (__int16)window[z]->at(x, y);
							}
						}
						
					}
						
#pragma omp atomic 
					m_options.m_xx++;

				}//for x
				m_options.UpdateBar();
			}//for y

			m_options.m_timerProcess.Stop();
			
		}//critical process
	}

	
	void CSegmentation::WriteBlock(int xBlock, int yBlock, CGDALDatasetEx& outputDS, CGDALDatasetEx& debugDS, OutputData& outputData, DebugData& debugData)
	{
#pragma omp critical(BlockIO)
	{
		m_options.m_timerWrite.Start();

		if (outputDS.IsOpen() )
		{
			CGeoExtents extents = outputDS.GetExtents();
			CGeoRectIndex outputRect = extents.GetBlockRect(xBlock, yBlock);

			ASSERT(outputRect.m_x >= 0 && outputRect.m_x < outputDS.GetRasterXSize());
			ASSERT(outputRect.m_y >= 0 && outputRect.m_y < outputDS.GetRasterYSize());
			ASSERT(outputRect.m_xSize > 0 && outputRect.m_xSize <= outputDS.GetRasterXSize());
			ASSERT(outputRect.m_ySize > 0 && outputRect.m_ySize <= outputDS.GetRasterYSize());
		
			for (size_t z = 0; z < outputData.size(); z++)
			{
				GDALRasterBand *pBand = outputDS.GetRasterBand(z);
				if (!outputData.empty())
				{
					ASSERT(outputData.size() == outputDS.GetRasterCount());
						
					for (int y = 0; y < outputRect.Height(); y++)
						pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(outputData[z][0]), outputRect.Width(), outputRect.Height(), GDT_Int16, 0, 0);
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
			CGeoExtents extents = debugDS.GetExtents();
			CGeoRectIndex debugRect = extents.GetBlockRect(xBlock, yBlock);

			ASSERT(debugRect.m_x >= 0 && debugRect.m_x < debugDS.GetRasterXSize());
			ASSERT(debugRect.m_y >= 0 && debugRect.m_y < debugDS.GetRasterYSize());
			ASSERT(debugRect.m_xSize > 0 && debugRect.m_xSize <= debugDS.GetRasterXSize());
			ASSERT(debugRect.m_ySize > 0 && debugRect.m_ySize <= debugDS.GetRasterYSize());

			for (size_t z = 0; z < debugData.size(); z++)
			{
				GDALRasterBand *pBand = debugDS.GetRasterBand(z);
				if (!debugData.empty())
				{
					ASSERT(outputData.size() == outputDS.GetRasterCount());

					for (int y = 0; y < debugRect.Height(); y++)
						pBand->RasterIO(GF_Write, debugRect.m_x, debugRect.m_y, debugRect.Width(), debugRect.Height(), &(debugData[z][0]), debugRect.Width(), debugRect.Height(), GDT_Int16, 0, 0);
				}
				else
				{
					__int16 noData = (__int16)outputDS.GetNoData(z);
					pBand->RasterIO(GF_Write, debugRect.m_x, debugRect.m_y, debugRect.Width(), debugRect.Height(), &noData, 1, 1, GDT_Int16, 0, 0);
				}
			}
		}

		
		m_options.m_timerWrite.Stop();
	}
	}

	void CSegmentation::CloseAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS, CGDALDatasetEx& debugDS)
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

		
		m_options.m_timerWrite.Stop();
		m_options.PrintTime();
	}

	pair<double, size_t> CSegmentation::ComputeRMSE(const NBRVector& data, size_t i)
	{
		ASSERT(i< data.size());

		if ((i - 1) >= data.size() || (i + 1) >= data.size())
			return make_pair(32767, data[i].second);

		CStatisticXY stat;
		for (int ii = -1; ii <= 1; ii++)
			stat.Add(data[i + ii].second, data[i + ii].first);

		double a0 = stat[INTERCEPT];
		double a1 = stat[SLOPE];
		
		CStatistic rsq;
		for (int ii = -1; ii <= 1; ii++)
			rsq += Square(a0 + a1*data[i + ii].second - data[i + ii].first);
	
		return make_pair(sqrt(rsq[MEAN]), data[i].second);
	}

	
	vector<size_t> CSegmentation::Segmentation(const NBRVector& dataIn, size_t maxLayers, double max_error)
	{
		ASSERT(!dataIn.empty());

		//compute mean and variance 
		//CStatistic stat;
		//for (size_t z = 0; z < dataIn.size(); z++)
			//stat += dataIn[z].first;

		//copy vector
		NBRVector data(dataIn.size());

		//normalize data
		
		for (size_t i = 0; i < dataIn.size(); i++)
			data[i] = dataIn[i];
			//data[i] = make_pair((dataIn[i].first - stat[MEAN]) / stat[STD_DEV], dataIn[i].second);

		//now compute RMSE and eliminate lo RMSE
		NBRVector dataRMSE(data.size());

		//1-first scan make pair and calculate the RMSE 
		for (size_t i = 0; i < data.size(); i++)
			dataRMSE[i] = ComputeRMSE(data, i);

		//RMSE array have the same size
		ASSERT(dataRMSE.size() == data.size());

		NBRVector::iterator minIt = std::min_element(dataRMSE.begin(), dataRMSE.end());
		while (dataRMSE.size() > 2 &&
			((minIt->first <= max_error) || (dataRMSE.size() > maxLayers)))
		{
			//erase data item
			NBRVector::iterator it = std::find_if(data.begin(), data.end(), [minIt](const NBRPair& p) { return p.second == minIt->second; });
			ASSERT(it != data.end());
			data.erase(it);

			//A-Detect the point - Remove it - and recompute all the sats
			//1- GET the min value in the RMSE array and REMOVE it 
			//remove RMSE item and update RMSE for points that have change
			size_t pos = std::distance(dataRMSE.begin(), minIt);
			dataRMSE.erase(minIt);

			if (pos - 1 < dataRMSE.size())
				dataRMSE[pos - 1] = ComputeRMSE(data, pos - 1);
			if (pos + 1 < dataRMSE.size())
				dataRMSE[pos] = ComputeRMSE(data, pos);


			ASSERT(dataRMSE.size() == data.size());

			minIt = std::min_element(dataRMSE.begin(), dataRMSE.end());
		}

		//add breaking point images index
		vector<size_t> breaks;

		for (size_t i = 0; i < dataRMSE.size(); i++)
			breaks.push_back(dataRMSE[i].second);

		return breaks;
	}

}