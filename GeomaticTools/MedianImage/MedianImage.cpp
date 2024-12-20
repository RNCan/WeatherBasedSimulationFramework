//***********************************************************************
// program to merge Landsat image image over a period
//									 
//***********************************************************************
// version
// 1.3.0	13/09/2023	R�mi Saint-Amant	Compile with GDAL 3.7.1
// 1.2.0	20/12/2021	R�mi Saint-Amant	Compile with VS 2019 and GDAL 3.0.3
// 1.1.4	11/10/2018	R�mi Saint-Amant	add -BLOCK_THREADS
// 1.1.3	20/09/2018	R�mi Saint-Amant	bestMedian use B3, B4, B5 instead of all bands
// 1.1.2	22/05/2018	R�mi Saint-Amant	Compile with VS 2017, add bestMedian
// 1.1.1	29/11/2017	R�mi Saint-Amant	Add 3 types of Landsat 8 corrections
// 1.1.0	15/11/2017	R�mi Saint-Amant	Compile with GDAL 2.0
// 1.0.2	30/01/2015	R�mi Saint-Amant	don't modify input VRT file. Bug correction in IntersectRect
// 1.0.1    27/01/2015	R�mi Saint-Amant	Compile with GDAL 1.11.1
// 1.0.0	21/12/2012	R�mi Saint-Amant	Creation


#include "stdafx.h"
#include <float.h>
#include <math.h>
#include <array>
#include <utility>
#include <iostream>
#include <boost/dynamic_bitset.hpp>

#include "MedianImage.h"
#include "Basic/OpenMP.h"

#pragma warning(disable: 4275 4251)
#include "gdal_priv.h"


using namespace std;
using namespace WBSF::Landsat1;

namespace WBSF
{


	const char* CMedianImage::VERSION = "1.3.0";
	std::string CMedianImage::GetDescription() { return  std::string("MedianImage version ") + CMedianImage::VERSION + " (" + __DATE__ + ")"; }
	//const int CMedianImage::NB_THREAD_PROCESS = 2;


	const char* CMedianImageOption::DEBUG_NAME[NB_DEBUG_BANDS] = { "Jday", "nbImages" };
	const char*  CMedianImageOption::MEAN_NAME[NB_MEAN_TYPE] = { "no", "standard", "always2", "always3", "always4" };

	//*********************************************************************************************************************


	CMedianImageOption::CMedianImageOption()
	{
		m_scenesSize = SCENES_SIZE;
		m_bDebug = false;
		m_corr8 = NO_CORR8;
		m_bBestMedian = false;
		m_bRemoveWorstQA = false;
		m_bBandSeparatly = false;
		//m_scenes = { {NOT_INIT, NOT_INIT } };//use -Period instead

		m_meanType = NO_MEAN;
		m_appDescription = "This software select the median pixel for each band of all scenes (composed of " + to_string(SCENES_SIZE) + " bands)";

		AddOption("-period");
		AddOption("-RGB");
		static const COptionDef OPTIONS[] =
		{
			{ "-Mean", 1, "type", false, "Compute mean of median pixels. Can be \"no\", \"standard\", \"always2\", \"always3\" or \"always4\". The \"standard\" type do the average of 2 medians values when even. The \"always2\" type average 2 medians pixel when even and the median and one neighbor select by MedianType when odd. \"no\" by default." },
			{ "-corr8", 1, "type", false, "Make a correction over the Landsat 8 images to get Landsat 7 equivalent. The type can be \"Canada\", \"Australia\" or \"USA\"." },
			{ "-BestMedian", 1, "type", false, "Select the pixel that have the best median score for bands (B3,B4,B5,QA). Take individual median by band by default." },
			{ "-worstQA", 1, "", false, "Remove the worst QA before computing median." },
			{ "-BandValidity", 1, "", false, "Verify validity on each band separately. all valid or invalid be default" },
			//{ "-Scenes", 2, "first last", false, "Select a first and the last scene (1..nbScenes) to clean cloud. All scenes are selected by default." },
			{ "-Debug", 0, "", false, "Output debug information." },
			{ "srcfile", 0, "", false, "Input image file path." },
			{ "dstfile", 0, "", false, "Output image file path." }
		};

		for (int i = 0; i < sizeof(OPTIONS) / sizeof(COptionDef); i++)
			AddOption(OPTIONS[i]);


		static const CIOFileInfoDef IO_FILE_INFO[] =
		{
			{ "Input Image", "srcfile", "", "ScenesSize(9)*nbScenes", "B1: Landsat band 1|B2: Landsat band 2|B3: Landsat band 3|B4: Landsat band 4|B5: Landsat band 5|B6: Landsat band 6|B7: Landsat band 7|QA: Image quality|JD: Julian day 1970|... for all scenes", "" },
			{ "Output Image", "dstfile", "", "ScenesSize(9)", "B1: Median Landsat band 1|B2: Median Landsat band 2|B3: Median Landsat band 3|B4: Median Landsat band 4|B5: Median Landsat band 5|B6: Median Landsat band 6|B7: Median Landsat band 7|QA: Median Image quality|JD: Median Julian day 1970", "" },
			{ "Optional Output Image", "_debug", "1", "7", "Path: path number of satellite|Row: row number of satellite|JDay: Julian day (1-366)|NbScenes: number of valid scene|scene: the selected scene|sort: the sort criterious" }
			//Year: year|Month: month (1-12)|Day: day (1-31)|
		};

		for (int i = 0; i < sizeof(IO_FILE_INFO) / sizeof(CIOFileInfoDef); i++)
			AddIOFileInfo(IO_FILE_INFO[i]);
	}

	ERMsg CMedianImageOption::ParseOption(int argc, char* argv[])
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

	ERMsg CMedianImageOption::ProcessOption(int& i, int argc, char* argv[])
	{
		ERMsg msg;

		if (IsEqual(argv[i], "-Debug"))
		{
			m_bDebug = true;
		}
		if (IsEqual(argv[i], "-Mean"))
		{
			string str = argv[++i];
			if (IsEqual(str, MEAN_NAME[NO_MEAN]))
				m_meanType = NO_MEAN;
			else if (IsEqual(str, MEAN_NAME[M_STANDARD]))
				m_meanType = M_STANDARD;
			else if (IsEqual(str, MEAN_NAME[M_ALWAYS2]))
				m_meanType = M_ALWAYS2;
			else if (IsEqual(str, MEAN_NAME[M_ALWAYS3]))
				m_meanType = M_ALWAYS3;
			else if (IsEqual(str, MEAN_NAME[M_ALWAYS4]))
				m_meanType = M_ALWAYS4;
			else
				msg.ajoute("Invalid -Mean type. Mean type can be \"no\", \"standard\" or \"always2\"");
		}
		else if (IsEqual(argv[i], "-BestMedian"))
		{
			m_bBestMedian = true;
		}
		else if (IsEqual(argv[i], "-corr8"))
		{
			m_corr8 = Landsat1::GetCorr8(argv[++i]);
			if (m_corr8 == NO_CORR8)
				msg.ajoute("Invalid -Corr8 type. Type can be \"Canada\", \"Australia\" or \"USA\"");
		}
		else if (IsEqual(argv[i], "-worstQA"))
		{
			m_bRemoveWorstQA = true;
		}
		else if (IsEqual(argv[i], "-BandValidity"))
		{
			m_bBandSeparatly = true;
		}
		else
		{
			//Look to see if it's a know base option
			msg = CBaseOptions::ProcessOption(i, argc, argv);
		}


		return msg;
	}

	void CMedianImageOption::InitFileInfo(CLandsatDataset& inputDS)
	{
		ASSERT(inputDS.GetFileInfo().size() == inputDS.GetNbScenes());
		m_info = inputDS.GetFileInfo();

	}



	ERMsg CMedianImage::Execute()
	{
		ERMsg msg;

		if (!m_options.m_bQuiet)
		{
			cout << "Output: " << m_options.m_filesPath[CMedianImageOption::OUTPUT_FILE_PATH] << endl;
			cout << "From:   " << m_options.m_filesPath[CMedianImageOption::INPUT_FILE_PATH] << endl;

			if (!m_options.m_maskName.empty())
				cout << "Mask:   " << m_options.m_maskName << endl;
		}

		GDALAllRegister();

		CLandsatDataset inputDS;
		CGDALDatasetEx maskDS;
		CLandsatDataset outputDS;
		CLandsatDataset debugDS;

		msg = OpenAll(inputDS, maskDS, outputDS, debugDS);


		if (msg)
		{
			CBandsHolderMT bandHolder(1, m_options.m_memoryLimit, m_options.m_IOCPU, m_options.m_BLOCK_THREADS);

			if (maskDS.IsOpen())
				bandHolder.SetMask(maskDS.GetSingleBandHolder(), m_options.m_maskDataUsed);

			msg += bandHolder.Load(inputDS, m_options.m_bQuiet, m_options.GetExtents(), m_options.m_period);

			if (!msg)
				return msg;

			if (!m_options.m_bQuiet && m_options.m_bCreateImage)
			{
				cout << "Create output images (" << outputDS.GetRasterXSize() << " C x " << outputDS.GetRasterYSize() << " R x " << outputDS.GetRasterCount() << " B) with " << m_options.m_CPU << " threads..." << endl;
			}

			CGeoExtents extents = bandHolder.GetExtents();
			m_options.ResetBar((size_t)extents.m_xSize*extents.m_ySize);

			vector<pair<int, int>> XYindex = extents.GetBlockList();

			omp_set_nested(1);//for IOCPU
#pragma omp parallel for schedule(static, 1) num_threads( m_options.m_BLOCK_THREADS ) if (m_options.m_bMulti) 
			for (int b = 0; b < (int)XYindex.size(); b++)
			{
				int xBlock = XYindex[b].first;
				int yBlock = XYindex[b].second;

				int thread = ::omp_get_thread_num();

				OutputData outputData;
				DebugData debugData;

				ReadBlock(xBlock, yBlock, bandHolder[thread]);
				ProcessBlock(xBlock, yBlock, bandHolder[thread], outputData, debugData);
				WriteBlock(xBlock, yBlock, bandHolder[thread], outputDS, debugDS, outputData, debugData);
			}//for all blocks

			CloseAll(inputDS, maskDS, outputDS, debugDS);
		}//if msg

		return msg;
	}

	void CMedianImage::AllocateMemory(size_t sceneSize, CGeoSize blockSize, OutputData& outputData)
	{

		if (m_options.m_bCreateImage)
		{
			OutputDataType dstNodata = (OutputDataType)m_options.m_dstNodata;
			outputData.resize(SCENES_SIZE);
			for (size_t z = 0; z < outputData.size(); z++)
			{
				outputData[z].resize(blockSize.m_y*blockSize.m_x, dstNodata);
				//for (size_t j = 0; j < outputData[z].size(); j++)
				//outputData[z][j].resize(blockSize.m_x, dstNodata);
			}
		}
	}


	ERMsg CMedianImage::OpenAll(CLandsatDataset& inputDS, CGDALDatasetEx& maskDS, CLandsatDataset& outputDS, CGDALDatasetEx& debugDS)
	{
		ERMsg msg;


		if (!m_options.m_bQuiet)
			cout << endl << "Open input image..." << endl;

		msg = inputDS.OpenInputImage(m_options.m_filesPath[CMedianImageOption::INPUT_FILE_PATH], m_options);

		if (!msg)
			return msg;

		if (msg)
		{
			inputDS.UpdateOption(m_options);
			m_options.InitFileInfo(inputDS);
		}


		if (!m_options.m_bQuiet)
		{
			CGeoExtents extents = inputDS.GetExtents();
			CProjectionPtr pPrj = inputDS.GetPrj();
			string prjName = pPrj ? pPrj->GetName() : "Unknown";

			size_t nbSceneLoaded = 0;
			const std::vector<CTPeriod>& p = inputDS.GetScenePeriod();
			CTPeriod period;
			for (size_t z = 0; z < p.size(); z++)
			{
				if (m_options.m_period.IsIntersect(p[z]))
				{
					nbSceneLoaded++;
					period.Inflate(p[z]);
				}

			}

			
			cout << "    Size           = " << inputDS->GetRasterXSize() << " cols x " << inputDS->GetRasterYSize() << " rows x " << inputDS.GetRasterCount() << " bands" << endl;
			cout << "    Extents        = X:{" << ToString(extents.m_xMin) << ", " << ToString(extents.m_xMax) << "}  Y:{" << ToString(extents.m_yMin) << ", " << ToString(extents.m_yMax) << "}" << endl;
			cout << "    Projection     = " << prjName << endl;
			cout << "    Scene size     = " << inputDS.GetSceneSize() << endl;
			cout << "    Entire period  = " << inputDS.GetPeriod().GetFormatedString() << " (nb scenes = " << inputDS.GetNbScenes() << ")" << endl;
			cout << "    Input period   = " << m_options.m_period.GetFormatedString() << endl;
			cout << "    Loaded period  = " << period.GetFormatedString() << " (nb scenes = " << nbSceneLoaded << ")" << endl;


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
			CMedianImageOption option(m_options);
			option.m_nbBands = m_options.m_scenesSize;

			if (!m_options.m_bQuiet)
			{
				cout << endl;
				cout << "Open output images..." << endl;
				cout << "    Size           = " << m_options.m_extents.m_xSize << " cols x " << m_options.m_extents.m_ySize << " rows x " << option.m_nbBands << " bands" << endl;
				cout << "    Extents        = X:{" << ToString(m_options.m_extents.m_xMin) << ", " << ToString(m_options.m_extents.m_xMax) << "}  Y:{" << ToString(m_options.m_extents.m_yMin) << ", " << ToString(m_options.m_extents.m_yMax) << "}" << endl;

			}

			string filePath = option.m_filesPath[CMedianImageOption::OUTPUT_FILE_PATH];
			msg += outputDS.CreateImage(filePath, option);
		}

		if (msg && m_options.m_bDebug)
		{
			if (!m_options.m_bQuiet)
				cout << "Create debug images..." << endl;

			CMedianImageOption options(m_options);
			options.m_outputType = GDT_Int16;
			options.m_nbBands = CMedianImageOption::NB_DEBUG_BANDS;
			options.m_dstNodata = WBSF::GetDefaultNoData(GDT_Int16);


			string filePath = options.m_filesPath[CMedianImageOption::OUTPUT_FILE_PATH];
			string title = GetFileTitle(filePath) + "_debug";
			SetFileTitle(filePath, title);

			for (size_t b = 0; b < CMedianImageOption::NB_DEBUG_BANDS; b++)
				options.m_VRTBandsName += title + "_" + CMedianImageOption::DEBUG_NAME[b] + ".tif|";

			msg += debugDS.CreateImage(filePath, options);
		}


		return msg;
	}

	void CMedianImage::ReadBlock(int xBlock, int yBlock, CBandsHolder& bandHolder)
	{
#pragma omp critical(BlockIORead)
		{
			m_options.m_timerRead.Start();

			CGeoExtents extents = m_options.m_extents.GetBlockExtents(xBlock, yBlock);
			bandHolder.LoadBlock(extents, m_options.m_period);

			m_options.m_timerRead.Stop();
		}
	}

	void CMedianImage::ProcessBlock(int xBlock, int yBlock, CBandsHolder& bandHolder, OutputData& outputData, DebugData& debugData)
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

		if (m_options.m_bCreateImage)
			AllocateMemory(SCENES_SIZE, blockSize, outputData);

		if (m_options.m_bDebug)
		{
			__int16 dstNodata = (__int16)WBSF::GetDefaultNoData(GDT_Int16);

			debugData.resize(CMedianImageOption::NB_DEBUG_BANDS);
			for (size_t z = 0; z < debugData.size(); z++)
				debugData[z].resize(blockSize.m_x*blockSize.m_y, dstNodata);
		}

//#pragma omp critical(ProcessBlock)
		{
			m_options.m_timerProcess.Start();

#pragma omp parallel for num_threads( m_options.BLOCK_CPU() ) if (m_options.m_bMulti) 
			for (int y = 0; y < blockSize.m_y; y++)
			{
				for (int x = 0; x < blockSize.m_x; x++)
				{
					size_t xy = y * blockSize.m_x + x;

					array<vector<pair<__int16, __int16>>, SCENES_SIZE> median;
					array<vector<pair<__int16, __int16>>, SCENES_SIZE> median2;
					array < vector < pair<__int16, __int16>>, SCENES_SIZE>  medianQA;

					for (size_t z = 0; z < median.size(); z++)
					{
						median[z].reserve(window.GetNbScenes());
						median2[z].reserve(window.GetNbScenes());
						medianQA[z].reserve(window.GetNbScenes());
					}


					for (size_t iz = 0; iz < window.GetNbScenes(); iz++)
					{
						CLandsatPixel pixel = window.GetPixel(iz, x, y);

						//if (pixel.IsValid() && !pixel.IsBlack())
						if (!pixel.IsBlack())
						{
							if (m_options.m_bBandSeparatly || pixel.IsValid())
							{
								for (size_t z = 0; z < pixel.size(); z++)
								{
									if (!m_options.m_bBandSeparatly || pixel[z] > -32000)
									{
										median[z].push_back(make_pair(pixel[z], pixel[QA]));
										median2[z].push_back(make_pair(pixel[z], __int16(iz)));
										medianQA[z].push_back(make_pair(pixel[QA], medianQA[z].size()));
									}
								}
							}
						}//!black
					}//iz

					if (!median[0].empty())
					{
						if (m_options.m_bRemoveWorstQA )
						{
							for (size_t z = 0; z < median.size(); z++)
							{
								if (median[z].size() > 1)
								{
									sort(medianQA[z].begin(), medianQA[z].end());
									//eliminate worst QA
									__int16 worstQA = medianQA[z].back().second;

									median[z].erase(median[z].begin() + worstQA);
									median2[z].erase(median2[z].begin() + worstQA);
								}
							}
						}


						for (size_t z = 0; z < median.size(); z++)
						{
							sort(median[z].begin(), median[z].end());
							sort(median2[z].begin(), median2[z].end());
						}

						//partial_sort(median[z].begin(), median[z].end(), median[z].begin() + (median[z].size() + 1) / 2);
					//find input temporal index
						if (!outputData.empty())
						{
							size_t iz1 = NOT_INIT;
							size_t iz2 = NOT_INIT;
							if (m_options.m_bBestMedian)
							{
								vector<pair<size_t, size_t>> scenesScore(window.GetNbScenes());
								for (size_t s = 0; s < scenesScore.size(); s++)
									scenesScore[s] = make_pair(0, s);

								for (size_t z = B3; z <= B5; z++)
								{
									size_t N = (median2[z].size() + 1) / 2;
									for (size_t l = 0; l < median2[z].size(); l++)
									{
										int score = int(N - (abs((int)N - (int)l)));
										scenesScore[median2[z][l].second].first += score;
									}
								}

								//add QA
								for (size_t l = 0; l < median2[QA].size(); l++)
								{
									scenesScore[median2[QA][l].second].first += 3*(median2[QA].size() - l- 1);
								}


								std::sort(scenesScore.begin(), scenesScore.end());
								size_t iz = scenesScore.rbegin()->second;

								CLandsatPixel pixel = window.GetPixel(iz, x, y);
								for (size_t z = 0; z < pixel.size(); z++)
									outputData[z][xy] = pixel[z];
							}
							else
							{
								

								for (size_t z = 0; z < median.size(); z++)
								{
									size_t N1 = (median[z].size() + 1) / 2 - 1;
									size_t N2 = median[z].size() / 2 + 1 - 1;
									size_t N = median[z][N1].second < median[z][N2].second ? N1 : N2;
									ASSERT(N2 == N1 || N2 == N1 + 1);

									if ((m_options.m_meanType == CMedianImageOption::NO_MEAN) || median[z].size() == 1)
									{
										outputData[z][xy] = median[z][N].first;
									}
									else if (m_options.m_meanType == CMedianImageOption::M_STANDARD)
									{
										outputData[z][xy] = (OutputDataType)((median[z][N1].first + median[z][N2].first) / 2.0);
									}
									else if (m_options.m_meanType == CMedianImageOption::M_ALWAYS2 || median[z].size() == 2)
									{
										size_t N3 = N2 != N1 ? N2 : median[z][N1 - 1].second < median[z][N1 + 1].second ? N1 - 1 : N1 + 1;
										outputData[z][xy] = (OutputDataType)((median[z][N1].first + median[z][N3].first) / 2.0);
									}
									else if (m_options.m_meanType == CMedianImageOption::M_ALWAYS3 || median[z].size() == 3)
									{
										ASSERT(median[z].size() >= 3);

										size_t N3 = N2 == N1 ? N1 - 1 : median[z][N1 - 1].second < median[z][N2 + 1].second ? N1 - 1 : N1;
										size_t N4 = N2 == N1 ? N2 + 1 : median[z][N1 - 1].second < median[z][N2 + 1].second ? N2 : N2 + 1;
										//int N4 = int(N1) - 1;
										//int N5 = int(N2) + 1;
										//if()

										CStatistic stat;
										for (size_t n = N3; n <= N4; n++)
										{
											//size_t nn = n;
											if(n<median[z].size())
												stat += median[z][n].first;
										}
											
										if (stat.IsInit())
											outputData[z][xy] = (OutputDataType)stat[MEAN];
									}
									else if (m_options.m_meanType == CMedianImageOption::M_ALWAYS4 || median[z].size() == 4)
									{
										ASSERT(median[z].size() >= 4);
										size_t N3 = N2 != N1 ? N1 - 1 : median[z][N1 - 2].second < median[z][N1 + 2].second ? N1 - 2 : N1 - 1;
										size_t N4 = N2 != N1 ? N2 + 1 : median[z][N1 - 2].second < median[z][N1 + 2].second ? N1 + 1  : N1 + 2;

										CStatistic stat;
										for (size_t n = N3; n <= N4; n++)
										{
											//size_t nn = n;
											if (n<median[z].size())
												stat += median[z][n].first;
										}

										if (stat.IsInit())
											outputData[z][xy] = (OutputDataType)stat[MEAN];
									}
								}
							}

							//a enlever...
							/*CLandsatPixel testRemi = window.GetPixelMedian(x, y);
							for (size_t z = 0; z < median.size(); z++)
								outputData[z][xy] = testRemi[z];*/
							
						}//if output data

						if (m_options.m_bDebug)
						{
							size_t N1 = (median[JD].size() + 1) / 2 - 1;
							size_t N2 = median[JD].size() / 2 + 1 - 1;
							size_t N = median[JD][N1].second < median[JD][N2].second ? N1 : N2;
							ASSERT(N2 == N1 || N2 == N1 + 1);

							__int16 jd1970 = 0;
							if ((m_options.m_meanType == CMedianImageOption::NO_MEAN) || median[JD].size() == 1)
							{
								jd1970 = median[JD][N].first;
							}
							else if (m_options.m_meanType == CMedianImageOption::M_STANDARD)
							{
								jd1970 = (OutputDataType)((median[JD][N1].first + median[JD][N2].first) / 2.0);
							}
							else if (m_options.m_meanType == CMedianImageOption::M_ALWAYS2 || median[JD].size() == 2)
							{
								size_t N3 = N2 != N1 ? N2 : median[JD][N1 - 1].second < median[JD][N1 + 1].second ? N1 - 1 : N1 + 1;
								jd1970 = (OutputDataType)((median[JD][N1].first + median[JD][N3].first) / 2.0);
							}
							else if (m_options.m_meanType == CMedianImageOption::M_ALWAYS3 || median[JD].size() == 3)
							{
								size_t N3 = N2 == N1 ? N1 - 1 : median[JD][N1 - 1].second < median[JD][N2 + 1].second ? N1 - 1 : N1;
								size_t N4 = N2 == N1 ? N2 + 1 : median[JD][N1 - 1].second < median[JD][N2 + 1].second ? N2 : N2 + 1;

								CStatistic stat;
								for (size_t n = N3; n <= N4; n++)
								{
									if (n<median[JD].size())
										stat += median[JD][n].first;
								}

								if (stat.IsInit())
									jd1970 = (OutputDataType)stat[MEAN];
							}
							else if (m_options.m_meanType == CMedianImageOption::M_ALWAYS4 || median[JD].size() == 4)
							{
								size_t N3 = N2 != N1 ? N1 - 1 : median[JD][N1 - 2].second < median[JD][N1 + 2].second ? N1 - 2 : N1 - 1;
								size_t N4 = N2 != N1 ? N2 + 1 : median[JD][N1 - 2].second < median[JD][N1 + 2].second ? N1 + 1 : N1 + 2;

								CStatistic stat;
								for (size_t n = N3; n <= N4; n++)
								{
									if (n<median[JD].size())
										stat += median[JD][n].first;
								}

								if (stat.IsInit())
									jd1970 = (OutputDataType)stat[MEAN];
							}

							debugData[CMedianImageOption::D_JDAY][xy] = jd1970;
							debugData[CMedianImageOption::NB_IMAGES][xy] = int(median[0].size());
						}
					}
#pragma omp atomic 
					m_options.m_xx++;

				}//for x

				m_options.UpdateBar();

			}//for y

			m_options.m_timerProcess.Stop();
		}//critical

	}

	void CMedianImage::WriteBlock(int xBlock, int yBlock, CBandsHolder& bandHolder, CGDALDatasetEx& outputDS, CGDALDatasetEx& debugDS, OutputData& outputData, DebugData& debugData)
	{
#pragma omp critical(BlockIOWrite)
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
						//for (int y = 0; y < (int)outputData[z].size(); y++)
						pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(outputData[z][0]), outputRect.Width(), outputRect.Height(), GDT_Int16, 0, 0);
					}
					else
					{
						OutputDataType noData = (OutputDataType)outputDS.GetNoData(z);
						pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &noData, 1, 1, GDT_Int16, 0, 0);
					}
				}
			}

			if (debugDS.IsOpen())
			{
				for (size_t z = 0; z < CMedianImageOption::NB_DEBUG_BANDS; z++)
				{
					GDALRasterBand *pBand = debugDS.GetRasterBand(z);//s*CMedianImageOption::NB_DEBUG_BANDS + 
					if (!debugData.empty())
					{
						pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(debugData[z][0]), outputRect.Width(), outputRect.Height(), GDT_Int16, 0, 0);
					}
					else
					{
						__int16 noData = (__int16)debugDS.GetNoData(z);
						pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(noData), 1, 1, GDT_Int16, 0, 0);
					}
				}
			}


			m_options.m_timerWrite.Stop();
		}
	}

	void CMedianImage::CloseAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS, CGDALDatasetEx& debugDS)
	{
		inputDS.Close();
		maskDS.Close();

		m_options.m_timerWrite.Start();

		outputDS.Close(m_options);
		debugDS.Close(m_options);

		m_options.m_timerWrite.Stop();
		m_options.PrintTime();
	}
}