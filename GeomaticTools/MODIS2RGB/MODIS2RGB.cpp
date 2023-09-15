//***********************************************************************
// program to merge Landsat image image over a period
//									 
//***********************************************************************
// version
// 1.1.0	13/09/2023	Rémi Saint-Amant	Compile with GDAL 3.7.1
// 1.0.0	03/12/2020	Rémi Saint-Amant	Creation


//-stats -co "compress=LZW" -multi -overwrite  "G:\MODIS\MOD09A1(tif)\MOD09A1.A2020209.h13v04.006.2020218042556.tif" "G:\MODIS\MOD09A1(tif)\MOD09A1.A2020209.h13v04.006.2020218042556_RGB.tif"

#include "stdafx.h"
#include <float.h>
#include <math.h>
#include <array>
#include <utility>
#include <iostream>

#include "MODIS2RGB.h"
#include "Basic/OpenMP.h"
#include "Basic/UtilTime.h"
#include "Basic/UtilMath.h"
//#include "Geomatic/LandsatCloudsCleaner.h"
#pragma warning(disable: 4275 4251)
#include "gdal_priv.h"

using namespace std;
using namespace WBSF::Landsat1;

namespace WBSF
{
	const char* CMODIS2RGB::VERSION = "1.1.0";
	//const int CMODIS2RGB::NB_THREAD_PROCESS = 2;

	static const double P_MIN = 0.0;
	static const double P_MAX = 99.9;
	//*********************************************************************************************************************

	CMODIS2RGBOption::CMODIS2RGBOption()
	{

		m_RGBType = NATURAL;
		m_outputType = GDT_Byte;
		m_scenesSize = SCENES_SIZE;
		m_scenes = { { NOT_INIT, NOT_INIT } };
		m_dstNodata = 255;
		m_bust = { { 0, 255 } };
		m_bVirtual = false;



		m_appDescription = "This software transform MODIS images (composed of " + to_string(SCENES_SIZE) + " bands) into RGB image.";

		AddOption("-RGB");
		AddOption("-Rename");

		static const COptionDef OPTIONS[] =
		{
			{ "-Virtual", 0, "", false, "Create virtual (.vrt) output file based on input files. Combine with -NoResult, this avoid to create new files. " },
			{ "-Bust", 2, "min max", false, "replace busting pixel (lesser than min or greather than max) by no data. 0 and 255 by default." },
			{ "srcfile", 0, "", false, "Input image file path." },
			{ "dstfile", 0, "", false, "Output image file path." }
		};

		for (int i = 0; i < sizeof(OPTIONS) / sizeof(COptionDef); i++)
			AddOption(OPTIONS[i]);

		RemoveOption("-ot");
		RemoveOption("-CPU");//no multi thread in inner loop

		static const CIOFileInfoDef IO_FILE_INFO[] =
		{
			{ "Input Image", "srcfile", "", "ScenesSize(9)*nbScenes", "B1: Landsat band 1|B2: Landsat band 2|B3: Landsat band 3|B4: Landsat band 4|B5: Landsat band 5|B6: Landsat band 6|B7: Landsat band 7|QA: Image quality|Date: date of image(Julian day 1970 or YYYYMMDD format)|... for all scenes", "" },
			{ "Output Image", "dstfile", "nbScenes", "3 color (RGB) image", "B1:red|B2:green|B3:blue", "" },
		};

		for (int i = 0; i < sizeof(IO_FILE_INFO) / sizeof(CIOFileInfoDef); i++)
			AddIOFileInfo(IO_FILE_INFO[i]);
	}

	ERMsg CMODIS2RGBOption::ParseOption(int argc, char* argv[])
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

		m_outputType = GDT_Byte;

		return msg;
	}

	ERMsg CMODIS2RGBOption::ProcessOption(int& i, int argc, char* argv[])
	{
		ERMsg msg;

		/*		if (IsEqual(argv[i], "-RGB"))
				{
					string str = argv[++i];

					if (IsEqualNoCase(str, RGB_NAME[NATURAL]))
						m_type = NATURAL;
					else if (IsEqualNoCase(str, RGB_NAME[LANDWATER]))
						m_type = LANDWATER;
					else
						msg.ajoute("Bad RGB type format. RGB type format must be \"Natural\" or \"LandWater\"");

				}
				else if (IsEqual(argv[i], "-Scenes"))
				{
					m_scenes[0] = atoi(argv[++i]) - 1;
					m_scenes[1] = atoi(argv[++i]) - 1;
				}
				else */
		if (IsEqual(argv[i], "-Bust"))
		{
			m_bust[0] = ToInt(argv[++i]);
			m_bust[1] = ToInt(argv[++i]);
		}
		else if (IsEqual(argv[i], "-Virtual"))
		{
			m_bVirtual = true;
		}
		else
		{
			//Look to see if it's a know base option
			msg = CBaseOptions::ProcessOption(i, argc, argv);
		}


		return msg;
	}


	ERMsg CMODIS2RGB::Execute()
	{
		ERMsg msg;

		if (!m_options.m_bQuiet)
		{
			cout << "Output: " << m_options.m_filesPath[CMODIS2RGBOption::OUTPUT_FILE_PATH] << endl;
			cout << "From:   " << m_options.m_filesPath[CMODIS2RGBOption::INPUT_FILE_PATH] << endl;

			if (!m_options.m_maskName.empty())
				cout << "Mask:   " << m_options.m_maskName << endl;
		}

		GDALAllRegister();

		m_options.m_RGBType = CBaseOptions::TRUE_COLOR;




		CLandsatDataset inputDS;
		//CLandsatCloudCleaner cloudsCleaner;
		CBandsHolderMT bandHolder(1, m_options.m_memoryLimit, m_options.m_IOCPU, m_options.m_BLOCK_THREADS);
		CGDALDatasetEx maskDS;
		vector<CGDALDatasetEx> outputDS;
		msg = OpenAll(inputDS, maskDS, outputDS);
		if (!msg)
			return msg;

		if (m_options.m_bCreateImage)
		{
			if (msg && maskDS.IsOpen())
				bandHolder.SetMask(maskDS.GetSingleBandHolder(), m_options.m_maskDataUsed);

			if (msg)
				msg += bandHolder.Load(inputDS, m_options.m_bQuiet, m_options.GetExtents(), m_options.m_period);

			if (!msg)
				return msg;


			size_t nbScenedProcess = m_options.m_scenes[1] - m_options.m_scenes[0] + 1;
			CGeoExtents extents = bandHolder.GetExtents();
			m_options.ResetBar(nbScenedProcess*extents.m_xSize*extents.m_ySize);
			vector<pair<int, int>> XYindex = extents.GetBlockList(5, 5);

			//m_options.m_stats.resize(nbScenedProcess);
			//if (m_options.m_RGBType == CBaseOptions::TRUE_COLOR)
			//{
			//	//Get statistic of the image
			//	for (size_t zz = 0; zz < nbScenedProcess; zz++)
			//	{

			//		for (size_t b = B1; b <= B4; b++)
			//		{
			//			size_t z = (m_options.m_scenes[0] + zz)*SCENES_SIZE + b;
			//			GDALRasterBand * pBand = inputDS.GetRasterBand(z);
			//			pBand->GetStatistics(false, true, &m_options.m_stats[zz][b].m_min, &m_options.m_stats[zz][b].m_max, &m_options.m_stats[zz][b].m_mean, &m_options.m_stats[zz][b].m_sd);
			//		}
			//	}

			//}

			m_options.m_statsEx.resize(nbScenedProcess);


			//for (int b = 0; b < (int)XYindex.size(); b++)
			//{
			//	int xBlock = XYindex[b].first;
			//	int yBlock = XYindex[b].second;

			//	//int thread = 0;// ::omp_get_thread_num();

			//	ReadBlock(xBlock, yBlock, bandHolder);
			//	for (size_t zz = 0; zz < nbScenedProcess; zz++)
			//	{
			//		size_t z = m_options.m_scenes[0] + zz;
			//		CGeoExtents extents = bandHolder.GetExtents();
			//		CGeoSize blockSize = extents.GetBlockSize(xBlock, yBlock);

			//		if (!bandHolder.IsEmpty())
			//		{

			//			CLandsatWindow window = static_cast<CLandsatWindow&>(bandHolder.GetWindow());

			//			for (size_t y = 0; y < blockSize.m_y; y++)
			//			{
			//				for (size_t x = 0; x < blockSize.m_x; x++)
			//				{
			//					ASSERT(z < bandHolder.GetNbScenes());
			//					CLandsatPixel pixel = window.GetPixel(z, (int)x, (int)y);

			//					if (pixel.IsValid() && pixel[QA] == 0)
			//					{
			//						for (size_t b = B1; b <= B4; b++)
			//						{
			//							m_options.m_statsEx[zz][b] += pixel[b];
			//						}
			//					}
			//				}
			//			}
			//		}
			//	}
			//}//for all blocks



			//for (size_t zz = 0; zz < nbScenedProcess; zz++)
			//{
			//	/*double minR = min(0.0, m_options.m_statsEx[zz][B1].percentil(P_MIN));
			//	double maxR = max(1000.0, m_options.m_statsEx[zz][B1].percentil(P_MAX));
			//	double minG = min(0.0, m_options.m_statsEx[zz][B4].percentil(P_MIN));
			//	double maxG = max(1000.0, m_options.m_statsEx[zz][B4].percentil(P_MAX));
			//	double minB = min(0.0, m_options.m_statsEx[zz][B3].percentil(P_MIN));
			//	double maxB = max(1000.0, m_options.m_statsEx[zz][B3].percentil(P_MAX));*/

			//	double minR = m_options.m_statsEx[zz][B1].percentil(P_MIN);
			//	double maxR = m_options.m_statsEx[zz][B1].percentil(P_MAX);
			//	double minG = m_options.m_statsEx[zz][B4].percentil(P_MIN);
			//	double maxG = m_options.m_statsEx[zz][B4].percentil(P_MAX);
			//	double minB = m_options.m_statsEx[zz][B3].percentil(P_MIN);
			//	double maxB = m_options.m_statsEx[zz][B3].percentil(P_MAX);

			//	cout << "stats " << to_string(m_options.m_statsEx[zz][B1][NB_VALUE]) << endl;
			//	cout << "R " << to_string(minR) << " : " << to_string(maxR) << endl;
			//	cout << "G " << to_string(minG) << " : " << to_string(maxG) << endl;
			//	cout << "B " << to_string(minB) << " : " << to_string(maxB) << endl;
			//}

			omp_set_nested(1);//for IOCPU
#pragma omp parallel for schedule(static, 1) num_threads( m_options.m_BLOCK_THREADS ) if (m_options.m_bMulti) 
			for (int b = 0; b < (int)XYindex.size(); b++)
			{
				int xBlock = XYindex[b].first;
				int yBlock = XYindex[b].second;

				int thread = ::omp_get_thread_num();

				ReadBlock(xBlock, yBlock, bandHolder[thread]);
				for (size_t zz = 0; zz < nbScenedProcess; zz++)
				{
					size_t z = m_options.m_scenes[0] + zz;

					OutputData outputData;
					ProcessBlock(xBlock, yBlock, bandHolder[thread], z, outputData);
					WriteBlock(xBlock, yBlock, bandHolder[thread], outputDS[zz], outputData);
				}
			}//for all blocks
		}

		if (msg && m_options.m_bVirtual)
			CreateVirtual(inputDS);


		CloseAll(inputDS, maskDS, outputDS);

		return msg;
	}



	ERMsg CMODIS2RGB::OpenAll(CLandsatDataset& inputDS, CGDALDatasetEx& maskDS, vector<CGDALDatasetEx>& outputDS)
	{
		ERMsg msg;

		if (!m_options.m_bQuiet)
			cout << endl << "Open input image..." << endl;

		msg = inputDS.OpenInputImage(m_options.m_filesPath[CMODIS2RGBOption::INPUT_FILE_PATH], m_options);
		if (!msg)
			return msg;

		inputDS.UpdateOption(m_options);


		if (msg && !m_options.m_bQuiet)
		{
			if (m_options.m_period.IsInit())
			{
				const std::vector<CTPeriod>& scenesPeriod = inputDS.GetScenePeriod();
				CTPeriod p = m_options.m_period;

				set<size_t> selected;

				for (size_t s = 0; s < scenesPeriod.size(); s++)
				{
					if (p.IsIntersect(scenesPeriod[s]))
						selected.insert(s);
				}

				if (!selected.empty())
				{
					m_options.m_scenes[0] = *selected.begin();
					m_options.m_scenes[1] = *selected.rbegin();
				}
				else
				{
					msg.ajoute("No input scenes intersect -PeriodTrait (" + m_options.m_period.GetFormatedString("%1 %2", "%F") + ").");
				}
			}


			CGeoExtents extents = inputDS.GetExtents();
			CProjectionPtr pPrj = inputDS.GetPrj();
			string prjName = pPrj ? pPrj->GetName() : "Unknown";

			if (m_options.m_scenes[0] == NOT_INIT)
				m_options.m_scenes[0] = 0;

			if (m_options.m_scenes[1] == NOT_INIT)
				m_options.m_scenes[1] = inputDS.GetNbScenes() - 1;

			if (m_options.m_scenes[0] >= inputDS.GetNbScenes() || m_options.m_scenes[1] >= inputDS.GetNbScenes())
				msg.ajoute("Scenes {" + to_string(m_options.m_scenes[0] + 1) + ", " + to_string(m_options.m_scenes[1] + 1) + "} must be in range {1, " + to_string(inputDS.GetNbScenes()) + "}");

			if (m_options.m_scenes[0] > m_options.m_scenes[1])
				msg.ajoute("First scene (" + to_string(m_options.m_scenes[0] + 1) + ") must be smaller or equal to the last scene (" + to_string(m_options.m_scenes[1] + 1) + ")");

			const std::vector<CTPeriod>& p = inputDS.GetScenePeriod();

			//set the period to the period in the scene selection
			size_t nbSceneLoaded = m_options.m_scenes[1] - m_options.m_scenes[0] + 1;
			CTPeriod period;
			for (size_t z = 0; z < nbSceneLoaded; z++)
				period.Inflate(p[m_options.m_scenes[0] + z]);

			if (m_options.m_period.IsInit())
				cout << "    User's Input loading period  = " << m_options.m_period.GetFormatedString() << endl;

			cout << "    Size           = " << inputDS.GetRasterXSize() << " cols x " << inputDS.GetRasterYSize() << " rows x " << inputDS.GetRasterCount() << " bands" << endl;
			cout << "    Extents        = X:{" << ToString(extents.m_xMin) << ", " << ToString(extents.m_xMax) << "}  Y:{" << ToString(extents.m_yMin) << ", " << ToString(extents.m_yMax) << "}" << endl;
			cout << "    Projection     = " << prjName << endl;
			cout << "    NbBands        = " << inputDS.GetRasterCount() << endl;
			cout << "    Scene size     = " << inputDS.GetSceneSize() << endl;
			cout << "    Entire period  = " << inputDS.GetPeriod().GetFormatedString() << " (nb scenes = " << inputDS.GetNbScenes() << ")" << endl;
			cout << "    Loaded period  = " << period.GetFormatedString() << " (nb scenes = " << nbSceneLoaded << ")" << endl;


			m_options.m_period = period;

			if (inputDS.GetSceneSize() != SCENES_SIZE)
				cout << FormatMsg("WARNING: the number of bands per scene (%1) is different than the inspected number (%2)", to_string(inputDS.GetSceneSize()), to_string(SCENES_SIZE)) << endl;

			//if (m_options.m_scene >= inputDS.GetNbScenes())
				//msg.ajoute("Scene " + ToString(m_options.m_scene+1) + "must be smaller thant the number of scenes of the input image");
		}

		if (msg && !m_options.m_maskName.empty())
		{
			if (!m_options.m_bQuiet)
				cout << "Open mask..." << endl;

			msg += maskDS.OpenInputImage(m_options.m_maskName);
		}


		if (msg && m_options.m_bCreateImage)
		{
			size_t nbScenedProcess = m_options.m_scenes[1] - m_options.m_scenes[0] + 1;

			outputDS.resize(nbScenedProcess);
			//replace the common part by the new name
			set<string> subnames;
			for (size_t zz = 0; zz < nbScenedProcess; zz++)
			{
				size_t z = m_options.m_scenes[0] + zz;

				CMODIS2RGBOption options(m_options);
				string filePath = options.m_filesPath[CMODIS2RGBOption::OUTPUT_FILE_PATH];
				string path = GetPath(filePath);

				options.m_nbBands = 3;
				options.m_outputType = GDT_Byte;
				options.m_format = "GTiff";

				//string subName = WBSF::TrimConst(inputDS.GetCommonImageName(z), "_");
				//if (subName.empty())
					//subName = FormatA("%02d", z+1);
				string subName = inputDS.GetSubname(z, m_options.m_rename);

				string uniqueSubName = subName;
				size_t i = 1;
				while (subnames.find(uniqueSubName) != subnames.end())
					uniqueSubName = subName + "_" + to_string(++i);

				subnames.insert(uniqueSubName);


				filePath = path + GetFileTitle(filePath) + "_" + uniqueSubName + "_RGB.tif";

				msg += outputDS[zz].CreateImage(filePath, options);
			}


			if (!m_options.m_bQuiet)
			{
				cout << endl;
				cout << "Open output images..." << endl;
				cout << "    Nb images      = " << nbScenedProcess << endl;
				cout << "    Size           = " << m_options.m_extents.m_xSize << " cols x " << m_options.m_extents.m_ySize << " rows x  3 bands" << endl;
				cout << "    Extents        = X:{" << ToString(m_options.m_extents.m_xMin) << ", " << ToString(m_options.m_extents.m_xMax) << "}  Y:{" << ToString(m_options.m_extents.m_yMin) << ", " << ToString(m_options.m_extents.m_yMax) << "}" << endl;
			}


		}


		return msg;
	}

	void CMODIS2RGB::ReadBlock(int xBlock, int yBlock, CBandsHolder& bandHolder)
	{
#pragma omp critical(BlockIO)
		{
			m_options.m_timerRead.Start();

			CGeoExtents extents = m_options.m_extents.GetBlockExtents(xBlock, yBlock);
			bandHolder.LoadBlock(extents);

			m_options.m_timerRead.Stop();
		}
	}

	void CMODIS2RGB::ProcessBlock(int xBlock, int yBlock, CBandsHolder& bandHolder, size_t z, OutputData& outputData)
	{
		CGeoExtents extents = bandHolder.GetExtents();
		CGeoSize blockSize = extents.GetBlockSize(xBlock, yBlock);
		size_t zz = z - m_options.m_scenes[0];

		if (bandHolder.IsEmpty())
		{
#pragma omp atomic
			m_options.m_xx += blockSize.m_x*blockSize.m_y;

			m_options.UpdateBar();
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

		m_options.m_timerProcess.Start();

		const std::array<CStatisticEx, 9>& stat = m_options.m_statsEx[zz];

		/*double minR = min(0.0, m_options.m_statsEx[zz][B1].percentil(P_MIN));
		double maxR = max(2000.0, m_options.m_statsEx[zz][B1].percentil(P_MAX));
		double minG = min(0.0, m_options.m_statsEx[zz][B4].percentil(P_MIN));
		double maxG = max(2000.0, m_options.m_statsEx[zz][B4].percentil(P_MAX));
		double minB = min(0.0, m_options.m_statsEx[zz][B3].percentil(P_MIN));
		double maxB = max(2000.0, m_options.m_statsEx[zz][B3].percentil(P_MAX));
*/
		double minR = 0.0;
		double maxR = 1500;
		double minG = 0.0;
		double maxG = 1400;
		double minB = 0.0;
		double maxB = 1300;


		for (size_t y = 0; y < blockSize.m_y; y++)
		{
			for (size_t x = 0; x < blockSize.m_x; x++)
			{
				ASSERT(z < bandHolder.GetNbScenes());
				CLandsatPixel pixel = window.GetPixel(z, (int)x, (int)y);
				//if (pixel.IsValid())
				if (pixel[B1] >= -100 && pixel[B1] <= 16000 &&
					pixel[B4] >= -100 && pixel[B4] <= 16000 &&
					pixel[B3] >= -100 && pixel[B3] <= 16000)
				{
					//bool bIsBlack = pixel.IsBlack();

					//if (!bIsBlack)
					{

						//Color8 R = pixel.R(m_options.m_RGBType, m_options.m_stats[zz]);
						//Color8 G = pixel.G(m_options.m_RGBType, m_options.m_stats[zz]);
						//Color8 B = pixel.B(m_options.m_RGBType, m_options.m_stats[zz]);

						//Color8 R = Color8(max(0.0, min(254.0, ((pixel[B1] ) / 10000.0) * 128)));
						//Color8 G = Color8(max(0.0, min(254.0, ((pixel[B4] ) / 10000.0) * 228)));
						//Color8 B = Color8(max(0.0, min(254.0, ((pixel[B3] ) / 10000.0) * 243)));
						//Color8 R = Color8(max(0.0, min(254.0, (((double)pixel[B1] - 0.0) / (1500.0 - 0.0)) * 254.0)));
						//Color8 G = Color8(max(0.0, min(254.0, (((double)pixel[B4] - 0.0) / (1500.0 - 0.0)) * 254.0)));
						//Color8 B = Color8(max(0.0, min(254.0, (((double)pixel[B3] - 0.0) / (1500.0 - 0.0)) * 254.0)));

						//Color8 R = Color8(max(0.0, min(254.0, (((double)pixel[B1] + 150.0) / 6150.0) * 254.0)));
						//Color8 G = Color8(max(0.0, min(254.0, (((double)pixel[B4] + 190.0) / 5190.0) * 254.0)));
						//Color8 B = Color8(max(0.0, min(254.0, (((double)pixel[B3] + 200.0) / 2700.0) * 254.0)));

						//const CBandStats& stats = m_options.m_stats[zz];



						Color8 R = Color8(max(0.0, min(254.0, ((pixel[B1] - minR) / (maxR - minR)) * 254)));
						Color8 G = Color8(max(0.0, min(254.0, ((pixel[B4] - minG) / (maxG - minG)) * 254)));
						Color8 B = Color8(max(0.0, min(254.0, ((pixel[B3] - minB) / (maxB - minB)) * 254)));






						bool bIsBust = m_options.IsBusting(R, G, B);

						if (!bIsBust)
						{
							outputData[0][y*blockSize.m_x + x] = R;
							outputData[1][y*blockSize.m_x + x] = G;
							outputData[2][y*blockSize.m_x + x] = B;
						}
					}
				}

#pragma omp atomic 
				m_options.m_xx++;

			}
		}

		m_options.UpdateBar();
		m_options.m_timerProcess.Stop();


	}

	void CMODIS2RGB::WriteBlock(int xBlock, int yBlock, CBandsHolder& bandHolder, CGDALDatasetEx& outputDS, OutputData& outputData)
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


	ERMsg CMODIS2RGB::CreateVirtual(CLandsatDataset& inputDS)
	{
		ERMsg msg;

		size_t nbScenedProcess = m_options.m_scenes[1] - m_options.m_scenes[0] + 1;
		for (size_t zz = 0; zz < nbScenedProcess; zz++)
		{
			size_t z = m_options.m_scenes[0] + zz;

			string subName = WBSF::TrimConst(inputDS.GetCommonImageName(z), "_");
			std::string filePath = m_options.m_filesPath[CMODIS2RGBOption::OUTPUT_FILE_PATH];
			filePath = GetPath(filePath) + GetFileTitle(filePath) + "_" + subName + "_RGB.vrt";
			msg += inputDS.CreateRGB(z, filePath, (CBaseOptions::TRGBTye)m_options.m_RGBType);
		}

		return msg;
	}

	void CMODIS2RGB::CloseAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, std::vector<CGDALDatasetEx>& outputDS)
	{
		inputDS.Close();
		maskDS.Close();

		m_options.m_timerWrite.Start();

		for (size_t i = 0; i < outputDS.size(); i++)
			outputDS[i].Close(m_options);

		m_options.m_timerWrite.Stop();
		m_options.PrintTime();
	}


}