//***********************************************************************
// program to merge Landsat image image over a period
//									 
//***********************************************************************
// version
// 1.0.2	29/11/2017  Rémi Saint-Amant	Load optimisation
// 1.0.1	23/11/2017  Rémi Saint-Amant	Avoid warning when breaks pixel is no data
// 1.0.0	17/11/2017	Rémi Saint-Amant	Creation

//-mask "U:\gis\#projets\LAQ\ANALYSE_CA\20160909_SR_run12\AAFC_Canada_2013_v2_masque120_181_v3_maskedOcean.tif" -maskvalue 1
//-te 1958730 7211040 1964040 7215660 -FirstYear 1984 -stats -multi -RGB Natural -of VRT -overwrite -co COMPRESS=LZW U:\GIS\#projets\LAQ\LAI\ANALYSE\20170815_Map_demo\test_code_remi_v1\BKP_9616_050_S7\BKP_out_brk01.tif U:\GIS1\LANDSAT_SR\mos\20160909_MergeImages\VRT_L578_8417_local.vrt U:\GIS\#documents\TestCodes\BreaksImage\Output\BPT1_test2.vrt
//
//-te 1557450 6778320 2268960 7365630 -FirstYear 1984 -stats -multi -IOCPU 3 -RGB Natural -of VRT -overwrite -co COMPRESS=LZW -co "tiled=YES" -co "BLOCKXSIZE=1024" -co "BLOCKYSIZE=1024" --config GDAL_CACHEMAX 4096  -overview {2,4,8,16}  U:\GIS\#projets\LAQ\LAI\ANALYSE\20170815_Map_demo\test_code_remi_v1\BKP_9616_050_S7\BKP_out.vrt U:\GIS1\LANDSAT_SR\mos\20160909_MergeImages\VRT_L578_8417_local.vrt U:\GIS\#documents\TestCodes\BreaksImage\Output\test3.vrt


#include "stdafx.h"
#include <float.h>
#include <math.h>
#include <array>
#include <utility>
#include <iostream>
#include <boost/dynamic_bitset.hpp>

#include "BreaksImage.h"
#include "Basic/OpenMP.h"
#pragma warning(disable: 4275 4251)
#include "gdal_priv.h"


using namespace std;
using namespace WBSF::Landsat;

namespace WBSF
{


	const char* CBreaksImage::VERSION = "1.0.2";
	std::string CBreaksImage::GetDescription(){ return  std::string("BreaksImage version ") + CBreaksImage::VERSION + " (" + __DATE__ + ")"; }
	const int CBreaksImage::NB_THREAD_PROCESS = 2;


	//const char* CBreaksImageOption::DEBUG_NAME[NB_DEBUG_BANDS] = { "Jday", "nbImages" };

	//*********************************************************************************************************************


	CBreaksImageOption::CBreaksImageOption()
	{

		m_scenesSize = SCENES_SIZE;
		m_bDebug = false;
		m_firstYear = 0;

		m_appDescription = "This software export landsat scene (composed of " + to_string(SCENES_SIZE) + " bands) from Breaks files";

		AddOption("-RGB");
		static const COptionDef OPTIONS[] =
		{
			{ "-FirstYear", 1, "n", false, "Specify year of the first scene when breaks file is composed of years instead of index. By default, use image index (0..nbImages-1)" },
//			{ "-Debug", 0, "", false, "Ouptu debug information." },
			{ "brkfile", 0, "", false, "Breaks image file path." },
			{ "srcfile", 0, "", false, "Input image file path." },
			{ "dstfile", 0, "", false, "Output image file path." }
		};

		for (int i = 0; i < sizeof(OPTIONS) / sizeof(COptionDef); i++)
			AddOption(OPTIONS[i]);


		static const CIOFileInfoDef IO_FILE_INFO[] =
		{
			{ "Breaks Image", "brkfile", "", "nbBreaks", "Break1: scene index (0..nbScene-1) or year|Break2: scene index (0..nbScene-1) or year|... for all breaks", "" },
			{ "Input Image", "srcfile", "", "ScenesSize(9)*nbScenes", "B1: Landsat band 1|B2: Landsat band 2|B3: Landsat band 3|B4: Landsat band 4|B5: Landsat band 5|B6: Landsat band 6|B7: Landsat band 7|QA: Image quality|JD: Julian day 1970|... for all scenes", "" },
			{ "Output Image", "dstfile", "", "nbBreaks*ScenesSize(9)", "B1: Landsat band 1|B2: Landsat band 2|B3: Landsat band 3|B4: Landsat band 4|B5: Landsat band 5|B6: Landsat band 6|B7: Landsat band 7|QA: Image quality|JD: Julian day 1970", "... for all scenes" },
//			{ "Optional Output Image", "_debug", "1", "7", "Path: path number of satellite|Row: row number of satellite|JDay: Julian day (1-366)|NbScenes: number of valid scene|scene: the selected scene|sort: the sort criterious" }
			//Year: year|Month: month (1-12)|Day: day (1-31)|
		};

		for (int i = 0; i < sizeof(IO_FILE_INFO) / sizeof(CIOFileInfoDef); i++)
			AddIOFileInfo(IO_FILE_INFO[i]);
	}

	ERMsg CBreaksImageOption::ParseOption(int argc, char* argv[])
	{
		ERMsg msg = CBaseOptions::ParseOption(argc, argv);

		ASSERT(NB_FILE_PATH == 3);
		if (msg && m_filesPath.size() != NB_FILE_PATH)
		{
			msg.ajoute("ERROR: Invalid argument line. 3 files are needed: the breaks, the source and destination image.");
			msg.ajoute("Argument found: ");
			for (size_t i = 0; i < m_filesPath.size(); i++)
				msg.ajoute("   " + to_string(i + 1) + "- " + m_filesPath[i]);
		}


		if (m_outputType == GDT_Unknown)
			m_outputType = GDT_Int16;

		return msg;
	}

	ERMsg CBreaksImageOption::ProcessOption(int& i, int argc, char* argv[])
	{
		ERMsg msg;

		if (IsEqual(argv[i], "-FirstYear"))
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

	void CBreaksImageOption::InitFileInfo(CLandsatDataset& inputDS)
	{
		ASSERT(inputDS.GetFileInfo().size() == inputDS.GetNbScenes());
		m_info = inputDS.GetFileInfo();

	}



	ERMsg CBreaksImage::Execute()
	{
		ERMsg msg;

		if (!m_options.m_bQuiet)
		{
			cout << "Output: " << m_options.m_filesPath[CBreaksImageOption::OUTPUT_FILE_PATH] << endl;
			cout << "From:   " << m_options.m_filesPath[CBreaksImageOption::INPUT_FILE_PATH] << endl;
			cout << "Using:  " << m_options.m_filesPath[CBreaksImageOption::BREAKS_FILE_PATH] << endl;

			if (!m_options.m_maskName.empty())
				cout << "Mask:   " << m_options.m_maskName << endl;
		}

		GDALAllRegister();

		CGDALDatasetEx breaksDS;
		CLandsatDataset inputDS;
		CGDALDatasetEx maskDS;
		CLandsatDataset outputDS;
		//CLandsatDataset debugDS;

		msg = OpenAll(breaksDS, inputDS, maskDS, outputDS);


		if (msg)
		{
			CBandsHolderMT bandHolder1(1, m_options.m_memoryLimit, m_options.m_IOCPU, NB_THREAD_PROCESS);
			CBandsHolderMT bandHolder2(1, m_options.m_memoryLimit, m_options.m_IOCPU, NB_THREAD_PROCESS);

			if (maskDS.IsOpen())
				bandHolder1.SetMask(maskDS.GetSingleBandHolder(), m_options.m_maskDataUsed);
				

			msg += bandHolder1.Load(breaksDS, m_options.m_bQuiet, m_options.GetExtents(), m_options.m_period);
			msg += bandHolder2.Load(inputDS, m_options.m_bQuiet, m_options.GetExtents(), m_options.m_period);

			if (!msg)
				return msg;

			if (!m_options.m_bQuiet && m_options.m_bCreateImage)
			{
				cout << "Create output images (" << outputDS.GetRasterXSize() << " C x " << outputDS.GetRasterYSize() << " R x " << outputDS.GetRasterCount() << " B) with " << m_options.m_CPU << " threads..." << endl;
			}

			CGeoExtents extents = bandHolder1.GetExtents();
			m_options.ResetBar(extents.m_xSize*extents.m_ySize);

			vector<pair<int, int>> XYindex = extents.GetBlockList();

			omp_set_nested(1);//for IOCPU
#pragma omp parallel for schedule(static, 1) num_threads( NB_THREAD_PROCESS ) if (m_options.m_bMulti) 
			for (int b = 0; b < (int)XYindex.size(); b++)
			{
				int xBlock = XYindex[b].first;
				int yBlock = XYindex[b].second;

				int thread = ::omp_get_thread_num();

				OutputData outputData;
				//DebugData debugData;

				ReadBlock(xBlock, yBlock, bandHolder1[thread], bandHolder2[thread]);
				ProcessBlock(xBlock, yBlock, bandHolder1[thread], bandHolder2[thread], outputData);
				WriteBlock(xBlock, yBlock, bandHolder2[thread], outputDS, outputData);
			}//for all blocks

			CloseAll(breaksDS, inputDS, maskDS, outputDS);
		}//if msg

		return msg;
	}

	

	ERMsg CBreaksImage::OpenAll(CGDALDatasetEx& breaksDS, CLandsatDataset& inputDS, CGDALDatasetEx& maskDS, CLandsatDataset& outputDS)
	{
		ERMsg msg;


		
		
		if (!m_options.m_bQuiet)
			cout << endl << "Open input image..." << endl;
			
		msg = inputDS.OpenInputImage(m_options.m_filesPath[CBreaksImageOption::INPUT_FILE_PATH], m_options);

		if (msg)
		{

			if (!m_options.m_bQuiet)
				cout << endl << "Open breaks image..." << endl;

			msg = breaksDS.OpenInputImage(m_options.m_filesPath[CBreaksImageOption::BREAKS_FILE_PATH], m_options);
		}

		if (!msg)
			return msg;

		
		inputDS.UpdateOption(m_options);
		m_options.InitFileInfo(inputDS);
		//CGeoExtents extents = m_options.GetExtents();
		//extents.IntersectRect( breaksDS.GetExtents() );
		m_options.m_extents.IntersectExtents(breaksDS.GetExtents());
		//m_options.m_extents = extents;

		if (!m_options.m_bQuiet)
		{
			CGeoExtents extents = inputDS.GetExtents();
			CProjectionPtr pPrj = inputDS.GetPrj();
			string prjName = pPrj ? pPrj->GetName() : "Unknown";

			cout << "    Size           = " << inputDS->GetRasterXSize() << " cols x " << inputDS->GetRasterYSize() << " rows x " << inputDS.GetRasterCount() << " bands" << endl;
			cout << "    Extents        = X:{" << ToString(extents.m_xMin) << ", " << ToString(extents.m_xMax) << "}  Y:{" << ToString(extents.m_yMin) << ", " << ToString(extents.m_yMax) << "}" << endl;
			cout << "    Projection     = " << prjName << endl;
			cout << "    Scene size     = " << inputDS.GetSceneSize() << endl;
			cout << "    Nb. scenes     = " << inputDS.GetNbScenes() << endl;
			cout << "    First image    = " << inputDS.GetPeriod().Begin().GetFormatedString() << endl;
			cout << "    Last image     = " << inputDS.GetPeriod().End().GetFormatedString() << endl;

			if (inputDS.GetSceneSize() != SCENES_SIZE)
				cout << FormatMsg("WARNING: the number of bands per scene (%1) is different than the inspected number (%2)", to_string(inputDS.GetSceneSize()), to_string(SCENES_SIZE)) << endl;
		}

		if (!m_options.m_maskName.empty())
		{
			if (!m_options.m_bQuiet)
				cout << "Open mask..." << endl;

			msg += maskDS.OpenInputImage(m_options.m_maskName);
		}

		if (msg && m_options.m_bCreateImage)
		{
			CBreaksImageOption options(m_options);
			options.m_nbBands = breaksDS.GetRasterCount()*inputDS.GetSceneSize();

			if (!m_options.m_bQuiet)
			{
				cout << endl;
				cout << "Open output images..." << endl;
				cout << "    Size           = " << m_options.m_extents.m_xSize << " cols x " << m_options.m_extents.m_ySize << " rows x " << options.m_nbBands << " bands" << endl;
				cout << "    Extents        = X:{" << ToString(m_options.m_extents.m_xMin) << ", " << ToString(m_options.m_extents.m_xMax) << "}  Y:{" << ToString(m_options.m_extents.m_yMin) << ", " << ToString(m_options.m_extents.m_yMax) << "}" << endl;
			}

			string filePath = options.m_filesPath[CBreaksImageOption::OUTPUT_FILE_PATH];
			string title = GetFileTitle(filePath);
			for (size_t i = 0; i < breaksDS.GetRasterCount(); i++)
			{
				for (size_t z = 0; z < inputDS.GetSceneSize(); z++)
				{
					options.m_VRTBandsName += title + "_brk" + FormatA("%02d", i+1) + "_" + Landsat::GetSceneName(z) + ".tif|";
				}
			}

			msg += outputDS.CreateImage(filePath, options);
		}

		/*if (msg && m_options.m_bDebug)
		{
			if (!m_options.m_bQuiet)
				cout << "Create debug images..." << endl;

			CBreaksImageOption options(m_options);
			options.m_outputType = GDT_Int16;
			options.m_nbBands = CBreaksImageOption::NB_DEBUG_BANDS;
			options.m_dstNodata = WBSF::GetDefaultNoData(GDT_Int16);


			string filePath = options.m_filesPath[CBreaksImageOption::OUTPUT_FILE_PATH];
			string title = GetFileTitle(filePath) + "_debug";
			SetFileTitle(filePath, title);

			for (size_t b = 0; b < CBreaksImageOption::NB_DEBUG_BANDS; b++)
				options.m_VRTBandsName += title + "_" + CBreaksImageOption::DEBUG_NAME[b] + ".tif|";

			msg += debugDS.CreateImage(filePath, options);
		}

*/
		return msg;
	}

	void CBreaksImage::ReadBlock(int xBlock, int yBlock, CBandsHolder& bandHolder1, CBandsHolder& bandHolder2)
	{
#pragma omp critical(BlockIO)
	{
		m_options.m_timerRead.Start();

		CGeoExtents extents = m_options.m_extents.GetBlockExtents(xBlock, yBlock);
		bandHolder1.LoadBlock(extents);

		boost::dynamic_bitset<size_t> selected_scene(bandHolder2.GetNbScenes());

		// sum the elements of v
		
#pragma omp parallel for num_threads( m_options.m_CPU ) if (m_options.m_bMulti) 
		for (__int64 i = 0; i < (__int64)bandHolder1.size(); i++)//for all breaks
		{
			//for (int j = 0; j < bandHolder1[i]->GetData()->size(); j++)
			const DataVector* pData = bandHolder1[i]->GetData();
			ASSERT(pData);
			for (auto it = pData->begin(); it != pData->end(); it++)
			{
				if (*it > -32768)
				{
					size_t iz = (size_t)(*it - m_options.m_firstYear);
					if (iz < selected_scene.size())
					{
#pragma omp critical
						selected_scene.set(iz);
						//#pragma omp atomic 
					}
				}
			}
		}

		ASSERT(bandHolder2.size() == selected_scene.size()*SCENES_SIZE);
		boost::dynamic_bitset<size_t> selected_bands(selected_scene.size()*SCENES_SIZE);
		for (size_t i = 0; i != selected_scene.size(); i++)
		{
			if (selected_scene.test(i))
			{
				for (size_t j = 0; j != SCENES_SIZE; j++)
					selected_bands.set(i*SCENES_SIZE + j);
			}
		}
			

		bandHolder2.LoadBlock(extents, selected_bands);

		m_options.m_timerRead.Stop();
	}
	}

	void CBreaksImage::ProcessBlock(int xBlock, int yBlock, CBandsHolder& bandHolder1, CBandsHolder& bandHolder2, OutputData& outputData)
	{
		CGeoExtents extents = bandHolder1.GetExtents();
		CGeoSize blockSize = extents.GetBlockSize(xBlock, yBlock);

		if (bandHolder1.IsEmpty())
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

		size_t bInvalid = 0;

		
		CRasterWindow window1 = static_cast<CLandsatWindow&>(bandHolder1.GetWindow());
		CLandsatWindow window2 = static_cast<CLandsatWindow&>(bandHolder2.GetWindow());

		if (m_options.m_bCreateImage)
		{
			outputData.resize(window1.size()*window2.GetSceneSize());

			for (size_t z = 0; z < outputData.size(); z++)
				outputData[z].resize(blockSize.m_x*blockSize.m_y, (__int16)m_options.m_dstNodata);
		}

		//if (m_options.m_bDebug)
		//{
		//	__int16 dstNodata = (__int16)WBSF::GetDefaultNoData(GDT_Int16);

		//	debugData.resize(CBreaksImageOption::NB_DEBUG_BANDS);

		//	for (size_t z = 0; z < debugData.size(); z++)
		//		debugData[z].resize(blockSize.m_x*blockSize.m_y, dstNodata);
		//}

		//vector<boost::dynamic_bitset<size_t>> clouds;


#pragma omp critical(ProcessBlock)
		{
			m_options.m_timerProcess.Start();

#pragma omp parallel for num_threads( m_options.m_CPU ) if (m_options.m_bMulti) 
			for (int y = 0; y < blockSize.m_y; y++)
			{
				for (int x = 0; x < blockSize.m_x; x++)
				{
					size_t xy = y*blockSize.m_x + x;

					for (size_t i = 0; i < window1.size(); i++)//for all breaks
					{
						if (window1[i]->IsValid(x, y))
						{
							size_t iz = (size_t)(window1[i]->at(x, y) - m_options.m_firstYear);

							if (iz < window2.GetNbScenes())
							{
								CLandsatPixel pixel = window2.GetPixel(iz, x, y);

								if (!outputData.empty())
								{
									for (size_t z = 0; z < pixel.size(); z++)
										outputData[i *window2.GetSceneSize() + z][xy] = pixel[z];
								}
							}
							else
							{
#pragma omp atomic
								bInvalid++;

							}
						}
					}
#pragma omp atomic 
					m_options.m_xx++;
				}//for x

				m_options.UpdateBar();
			}//for y

			m_options.m_timerProcess.Stop();
		}//critical

		if (bInvalid>0)
			cout << "WARNING: invalid breaks value (" << to_string(bInvalid) << "). Breaks must range from 0 to number of scenes - 1 or from the fist year to the last year" << endl;

	}

	void CBreaksImage::WriteBlock(int xBlock, int yBlock, CBandsHolder& bandHolder, CGDALDatasetEx& outputDS, OutputData& outputData)
	{
#pragma omp critical(BlockIO)
	{
		m_options.m_timerWrite.Start();


		CGeoExtents extents = bandHolder.GetExtents();
		CGeoRectIndex outputRect = extents.GetBlockRect(xBlock, yBlock);


		if (outputDS.IsOpen())
		{
			ASSERT(outputRect.m_x >= 0 && outputRect.m_x < outputDS.GetRasterXSize());
			ASSERT(outputRect.m_y >= 0 && outputRect.m_y < outputDS.GetRasterYSize());
			ASSERT(outputRect.m_xSize > 0 && outputRect.m_xSize <= outputDS.GetRasterXSize());
			ASSERT(outputRect.m_ySize > 0 && outputRect.m_ySize <= outputDS.GetRasterYSize());
			ASSERT(outputData.empty() || outputData.size() == outputDS.GetRasterCount());

			for (size_t z = 0; z < outputDS.GetRasterCount(); z++)
			{
				GDALRasterBand *pBand = outputDS.GetRasterBand(z);
				if (!outputData.empty())
				{
					pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(outputData[z][0]), outputRect.Width(), outputRect.Height(), GDT_Int16, 0, 0);
				}
				else
				{
					__int16 noData = (__int16)outputDS.GetNoData(z);
					pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &noData, 1, 1, GDT_Int16, 0, 0);
				}
			}
		}

		//if (debugDS.IsOpen())
		//{
		//	for (size_t z = 0; z < CBreaksImageOption::NB_DEBUG_BANDS; z++)
		//	{
		//		GDALRasterBand *pBand = debugDS.GetRasterBand(z);//s*CBreaksImageOption::NB_DEBUG_BANDS + 
		//		if (!debugData.empty())
		//		{
		//			pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(debugData[z][0]), outputRect.Width(), outputRect.Height(), GDT_Int16, 0, 0);
		//		}
		//		else
		//		{
		//			__int16 noData = (__int16)debugDS.GetNoData(z);
		//			pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(noData), 1, 1, GDT_Int16, 0, 0);
		//		}
		//	}
		//}

		m_options.m_timerWrite.Stop();
	}
	}

	void CBreaksImage::CloseAll(CGDALDatasetEx& breaksDS, CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS)
	{
		breaksDS.Close();
		inputDS.Close();
		maskDS.Close();


		m_options.m_timerWrite.Start();

		outputDS.Close(m_options);
		//debugDS.Close(m_options);

		m_options.m_timerWrite.Stop();
		m_options.PrintTime();
	}
}