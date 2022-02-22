//***********************************************************************
// program to merge Landsat image image over a period
//									 
//***********************************************************************
// version
// 1.1.0	20/12/2021	Rémi Saint-Amant	Compile with VS 2019 and GDAL 3.0.3
// 1.0.2	10/10/2018 Rémi Saint-Amant		Add -iFactor
// 1.0.1	10/10/2018 Rémi Saint-Amant		Add window and weight
// 1.0.0	27/06/2018	Rémi Saint-Amant	Creation


//-scenes 5 8 -i NBR -Virtual -of VRT -co "compress=LZW" --config GDAL_CACHEMAX 4096  -overview {2,4,8,16} -multi -IOCPU 3 -overwrite  "U:\GIS\#documents\TestCodes\SlowDisturbanceAnalyser\Input\1999-2014.vrt" "U:\GIS\#documents\TestCodes\SlowDisturbanceAnalyser\Output\out.vrt"
//-scenes 15 17 -i TCB -stats -Virtual -NoResult -of VRT -co "compress=LZW" --config GDAL_CACHEMAX 4096  -overview {2,4,8,16} -multi -overwrite "D:\Travaux\CloudCleaner\Input\34 ans\Te2.vrt" "D:\Travaux\CloudCleaner\Output\34 ans\Indices.vrt"

#include "stdafx.h"
#include <float.h>
#include <math.h>
#include <array>
#include <utility>
#include <iostream>

#include "LandsatIndices.h"
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
	const char* CLandsatIndices::VERSION = "1.1.0";
	

	///*********************************************************************************************************************

	CLandsatIndicesOption::CLandsatIndicesOption()
	{
		m_outputType = GDT_Byte;
		m_scenesSize = SCENES_SIZE;
		m_scenes = { { NOT_INIT, NOT_INIT } };
		m_bVirtual = false;
		m_rings = 0;

		m_appDescription = "This software computes indices (NBR, HDMI, ...) from Landsat images (composed of " + to_string(SCENES_SIZE) + " bands).";

		AddOption("-Period");
		AddOption("-Rename");
		AddOption("-iFactor");

		static const COptionDef OPTIONS[] =
		{
			{ "-i", 1, "indice", true, "Select indices to output. Indice can be \"B1\"..\"JD\", \"NBR\", \"NDVI\", \"NDMI\", \"TCB\", \"TCG\", \"TCW\", \"NBR2\", \"EVI\", \"SAVI\", \"MSAVI\", \"SR\", \"CL\", \"HZ\"."  },
			{ "-Despike", 3, "type threshold min", true, "Despike to remove outbound pixels. Type is the indice type, threshold is the despike threshold and min is the minimum between T-1 and T+1 to execute despike. Supported type are the same as indice. Usual value are TCB 0.75 0.1." },
			{ "-Virtual", 0, "", false, "Create virtual (.vrt) output file that used input file. Combine with -NoResult, this avoid to copy files. " },
			{ "-Window", 1, "n", false, "Compute window mean around the pixel. n is the number of rings. 0 = 1x1, 1 = 3x3, 2 = 5x5 etc. By default all rings have the same weight. Weight can be modified with option -weight. 1x1 by default." },
			{ "-Weight", 1, "{w0,w1,...,wn}", false, "Change the weight of window's rings. For example, if you enter {8,1} for a 3x3 windows, weight of the center pixel will be w0=8/16 (50%) and each pixel of the ring will be w1=1/16. So the ring will have 50%. Equal weights by default." },
			{ "srcfile", 0, "", false, "Input image file path." },
			{ "dstfile", 0, "", false, "Output image file path." }
		};


		for (int i = 0; i < sizeof(OPTIONS) / sizeof(COptionDef); i++)
			AddOption(OPTIONS[i]);

		RemoveOption("-ot");
		//RemoveOption("-CPU");//no multi thread in inner loop

		static const CIOFileInfoDef IO_FILE_INFO[] =
		{
			{ "Input Image", "srcfile", "", "ScenesSize(9)*nbScenes", "B1: Landsat band 1|B2: Landsat band 2|B3: Landsat band 3|B4: Landsat band 4|B5: Landsat band 5|B6: Landsat band 6|B7: Landsat band 7|QA: Image quality|Date: date of image(Julian day 1970 or YYYYMMDD format)|... for all scenes", "" },
			{ "Output Image", "dstfile", "nbIndices", "nbScenes", "Scenes indices", "" },
		};

		for (int i = 0; i < sizeof(IO_FILE_INFO) / sizeof(CIOFileInfoDef); i++)
			AddIOFileInfo(IO_FILE_INFO[i]);
	}

	ERMsg CLandsatIndicesOption::ParseOption(int argc, char* argv[])
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

		if (m_indices.count() == 0)
		{
			msg.ajoute("No indice selected. Indices must be add with option -i.");
		}

		if (m_bVirtual && m_rings > 0)
		{
			msg.ajoute("option -Virtual cannot be used with option -Window)");
		}

		if (m_weight_str.empty())
		{
			m_weight.resize(m_rings + 1, 1.0 / (m_rings + 1));
		}
		else
		{
			StringVector tmp(m_weight_str, "{ ,;}");
			if (tmp.size() == m_rings + 1)
			{
				for (size_t i = 0; i < tmp.size(); i++)
					m_weight.push_back(as<double>(tmp[i]));
			}
			else
			{
				msg.ajoute("Invalid weight. The number of weight (" + to_string(tmp.size()) + ") is no equal to the number of rings + 1 (" + to_string(m_rings + 1) + ").");
			}
		}



		if (!m_despike.empty() && m_bVirtual)
		{
			msg.ajoute("Option -Virtual can't be used with option -Despike.");
		}

		m_outputType = GDT_Int16;

		Landsat::INDICES_FACTOR(m_iFactor);

		return msg;
	}

	ERMsg CLandsatIndicesOption::ProcessOption(int& i, int argc, char* argv[])
	{
		ERMsg msg;

		if (IsEqual(argv[i], "-i"))
		{
			string str = argv[++i];
			TIndices type = GetIndiceType(str);
			if (type < NB_INDICES)
			{
				m_indices.set(type);
			}
			else
			{
				msg.ajoute(str + " is not a valid indices. See help.");
			}
		}
		else if (IsEqual(argv[i], "-Virtual"))
		{
			m_bVirtual = true;
		}
		else if (IsEqual(argv[i], "-Window"))
		{
			m_rings = atoi(argv[++i]);
		}
		else if (IsEqual(argv[i], "-Weight"))
		{
			m_weight_str = argv[++i];
		}
		else if (IsEqual(argv[i], "-Despike"))
		{
			string str = argv[++i];
			TIndices type = GetIndiceType(str);
			double threshold = atof(argv[++i]);
			double min_trigger = atof(argv[++i]);

			if (type != I_INVALID)
			{
				m_despike.push_back(CIndices(type, "<", threshold, min_trigger));
			}
			else
			{
				msg.ajoute(str + " is an invalid type for -Despike option");
			}
		}
		else
		{
			//Look to see if it's a know base option
			msg = CBaseOptions::ProcessOption(i, argc, argv);
		}


		return msg;
	}



	ERMsg CLandsatIndices::Execute()
	{
		ERMsg msg;

		if (!m_options.m_bQuiet)
		{
			string indices;
			for (size_t i = 0; i < m_options.m_indices.size(); i++)
			{
				if (m_options.m_indices.test(i))
				{
					if (!indices.empty())
						indices += ", ";
					indices += Landsat::GetIndiceName(i);
				}
			}

			cout << "Output:  " << m_options.m_filesPath[CLandsatIndicesOption::OUTPUT_FILE_PATH] << endl;
			cout << "From:    " << m_options.m_filesPath[CLandsatIndicesOption::INPUT_FILE_PATH] << endl;
			cout << "Indices: " << indices << endl;

			if (!m_options.m_maskName.empty())
				cout << "Mask:   " << m_options.m_maskName << endl;
		}

		GDALAllRegister();

		CLandsatDataset inputDS;
		CBandsHolderMT bandHolder(int(m_options.m_rings * 2 + 1), m_options.m_memoryLimit, m_options.m_IOCPU, m_options.m_BLOCK_THREADS);
		CGDALDatasetEx maskDS;
		CGDALDatasetEx outputDS;
		msg = OpenAll(inputDS, maskDS, outputDS);


		if (msg && maskDS.IsOpen())
			bandHolder.SetMask(maskDS.GetSingleBandHolder(), m_options.m_maskDataUsed);

		if (msg)
			msg += bandHolder.Load(inputDS, m_options.m_bQuiet, m_options.GetExtents(), m_options.m_period);

		if (!msg)
			return msg;

		if (m_options.m_bCreateImage)
		{
			CGeoExtents extents = bandHolder.GetExtents();
			size_t nbScenedProcess = m_options.m_scenes[1] - m_options.m_scenes[0] + 1;



			m_options.ResetBar(extents.m_xSize*extents.m_ySize);
			vector<pair<int, int>> XYindex = extents.GetBlockList();

			if (!m_options.m_bQuiet && m_options.m_bCreateImage)
			{
				cout << "Create output images (" << outputDS.GetRasterXSize() << " C x " << outputDS.GetRasterYSize() << " R x " << outputDS.GetRasterCount() << " B)" << endl;
			}

			omp_set_nested(1);//for IOCPU
#pragma omp parallel for schedule(static, 1) num_threads( m_options.m_BLOCK_THREADS ) if (m_options.m_bMulti) 
			for (__int64 b = 0; b < (__int64)XYindex.size(); b++)
			{
				int xBlock = XYindex[b].first;
				int yBlock = XYindex[b].second;

				int thread = ::omp_get_thread_num();

				OutputData outputData;

				ReadBlock(xBlock, yBlock, bandHolder[thread]);
				ProcessBlock(xBlock, yBlock, bandHolder[thread], outputData);
				WriteBlock(xBlock, yBlock, bandHolder[thread], outputDS, outputData);
			}//for all blocks
		}

		if (m_options.m_bVirtual)
			CreateVirtual(inputDS);

		CloseAll(inputDS, maskDS, outputDS);

		return msg;
	}



	ERMsg CLandsatIndices::OpenAll(CLandsatDataset& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS)
	{
		ERMsg msg;

		if (!m_options.m_bQuiet)
		{
			cout << endl << "Open input image..." << endl;
		}

		msg = inputDS.OpenInputImage(m_options.m_filesPath[CLandsatIndicesOption::INPUT_FILE_PATH], m_options);
		if (msg)
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
					//if (s > 1 && (scenesPeriod[s].End() < scenesPeriod[s - 1].Begin()))
						//msg.ajoute("Scenes of the input landsat images (" + GetFileName(m_options.m_filesPath[CLandsatIndicesOption::INPUT_FILE_PATH]) + ") are not chronologically ordered.");

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
					msg.ajoute("No input scenes intersect -Period (" + m_options.m_period.GetFormatedString("%1 %2", "%F") + ").");
				}
			}

			CGeoExtents extents = inputDS.GetExtents();
			CProjectionPtr pPrj = inputDS.GetPrj();
			string prjName = pPrj ? pPrj->GetName() : "Unknown";

			if (inputDS.GetSceneSize() != SCENES_SIZE)
				cout << FormatMsg("WARNING: the number of bands per scene (%1) is different than the inspected number (%2)", to_string(inputDS.GetSceneSize()), to_string(SCENES_SIZE)) << endl;

			//if (m_options.m_scenes[0] == NOT_INIT)
				//m_options.m_scenes[0] = 0;

			//if (m_options.m_scenes[1] == NOT_INIT)
//				m_options.m_scenes[1] = inputDS.GetNbScenes() - 1;

			//if (m_options.m_scenes[0] >= inputDS.GetNbScenes() || m_options.m_scenes[1] >= inputDS.GetNbScenes())
			//	msg.ajoute("Scenes {" + to_string(m_options.m_scenes[0] + 1) + ", " + to_string(m_options.m_scenes[1] + 1) + "} must be in range {1, " + to_string(inputDS.GetNbScenes()) + "}");

			//if (m_options.m_scenes[0] > m_options.m_scenes[1])
			//	msg.ajoute("First scene (" + to_string(m_options.m_scenes[0] + 1) + ") must be smaller or equal to the last scene (" + to_string(m_options.m_scenes[1] + 1) + ")");

			const std::vector<CTPeriod>& p = inputDS.GetScenePeriod();

			//set the period to the period in the scene selection
			size_t nbSceneLoaded = m_options.m_scenes[1] - m_options.m_scenes[0] + 1;
			CTPeriod period;
			for (size_t z = 0; z < nbSceneLoaded; z++)
				period.Inflate(p[m_options.m_scenes[0] + z]);

			if (m_options.m_period.IsInit())
				cout << "    User's input working period = " << m_options.m_period.GetFormatedString() << endl;

			cout << "    Size           = " << inputDS.GetRasterXSize() << " cols x " << inputDS.GetRasterYSize() << " rows x " << inputDS.GetRasterCount() << " bands" << endl;
			cout << "    Extents        = X:{" << ToString(extents.m_xMin) << ", " << ToString(extents.m_xMax) << "}  Y:{" << ToString(extents.m_yMin) << ", " << ToString(extents.m_yMax) << "}" << endl;
			cout << "    Projection     = " << prjName << endl;
			cout << "    NbBands        = " << inputDS.GetRasterCount() << endl;
			cout << "    Scene size     = " << inputDS.GetSceneSize() << endl;
			cout << "    Entire period  = " << inputDS.GetPeriod().GetFormatedString() << " (nb scenes = " << inputDS.GetNbScenes() << ")" << endl;
			cout << "    Working period = " << period.GetFormatedString() << " (nb scenes = " << nbSceneLoaded << ")" << endl;


			m_options.m_period = period;
		}

		if (msg && !m_options.m_maskName.empty())
		{
			if (!m_options.m_bQuiet)
				cout << "Open mask..." << endl;

			msg += maskDS.OpenInputImage(m_options.m_maskName);
		}


		if (msg && m_options.m_bCreateImage)
		{
			if (!m_options.m_bQuiet && m_options.m_bCreateImage)
				cout << "Create output images " << " x(" << m_options.m_extents.m_xSize << " C x " << m_options.m_extents.m_ySize << " R x " << m_options.m_nbBands << " bands) with " << m_options.m_CPU << " threads..." << endl;


			size_t nbScenedProcess = m_options.m_scenes[1] - m_options.m_scenes[0] + 1;
			string filePath = m_options.m_filesPath[CLandsatIndicesOption::OUTPUT_FILE_PATH];
			CLandsatIndicesOption options = m_options;
			options.m_nbBands = m_options.m_indices.count() * nbScenedProcess;

			//replace the common part by the new name
			set<string> subnames;
			for (size_t zz = 0; zz < nbScenedProcess; zz++)
			{
				size_t z = m_options.m_scenes[0] + zz;
				string subName = inputDS.GetSubname(z, m_options.m_rename);
				string uniqueSubName = subName;
				size_t i = 1;
				while (subnames.find(uniqueSubName) != subnames.end())
					uniqueSubName = subName + "_" + to_string(++i);

				subnames.insert(uniqueSubName);

				for (size_t i = 0; i < m_options.m_indices.size(); i++)
				{
					if (m_options.m_indices.test(i))
					{
						options.m_VRTBandsName += GetFileTitle(filePath) + "_" + uniqueSubName + "_" + Landsat::GetIndiceName(i) + ".tif|";
					}
				}
			}


			msg += outputDS.CreateImage(filePath, options);
		}


		return msg;
	}


	void CLandsatIndices::ReadBlock(int xBlock, int yBlock, CBandsHolder& bandHolder)
	{
#pragma omp critical(BlockIO)
		{
			m_options.m_timerRead.Start();

			CGeoExtents extents = m_options.m_extents.GetBlockExtents(xBlock, yBlock);
			bandHolder.LoadBlock(extents);

			m_options.m_timerRead.Stop();
		}
	}




	void CLandsatIndices::ProcessBlock(int xBlock, int yBlock, CBandsHolder& bandHolder, OutputData& outputData)
	{
		ASSERT(m_options.m_weight.size() == m_options.m_rings + 1);


		CGeoExtents extents = bandHolder.GetExtents();
		CGeoSize blockSize = extents.GetBlockSize(xBlock, yBlock);
		size_t nbScenesProcess = m_options.m_scenes[1] - m_options.m_scenes[0] + 1;

		if (bandHolder.IsEmpty())
		{
#pragma omp atomic
			m_options.m_xx += blockSize.m_x*blockSize.m_y;
			m_options.UpdateBar();

			return;
		}

		CLandsatWindow window = static_cast<CLandsatWindow&>(bandHolder.GetWindow());

		ASSERT(m_options.m_bCreateImage);

		__int16 dstNodata = (__int16)m_options.m_dstNodata;
		outputData.resize(nbScenesProcess);
		for (size_t i = 0; i < outputData.size(); i++)
		{
			outputData[i].resize(m_options.m_indices.count());
			for (size_t j = 0; j < outputData[i].size(); j++)
				outputData[i][j].resize(blockSize.m_x*blockSize.m_y, dstNodata);
		}


		//#pragma omp critical(ProcessBlock)
				//{
		m_options.m_timerProcess.Start();

		//multithread here is very not efficient.
#pragma omp parallel for num_threads( m_options.BLOCK_CPU() ) if (m_options.m_bMulti) 
		for (int y = 0; y < blockSize.m_y; y++)
		{
			for (int x = 0; x < blockSize.m_x; x++)
			{
				vector<bool> bSpiking(nbScenesProcess, false);

				if (!m_options.m_despike.empty())//if despike
				{
					vector< pair<CLandsatPixel, size_t>> data;

					ASSERT(window.GetNbScenes() == nbScenesProcess);
					data.reserve(nbScenesProcess);

					for (size_t zz = 0; zz < nbScenesProcess; zz++)
					{
						size_t z = m_options.m_scenes[0] + zz;
						//CLandsatPixel pixel = ((CLandsatWindow&)window).GetPixel(z, x, y);
						CLandsatPixel pixel = ((CLandsatWindow&)window).GetPixelMean(z, x, y, (int)m_options.m_weight.size(), m_options.m_weight);

						if (pixel.IsValid())
							data.push_back(make_pair(pixel, zz));
					}

					for (size_t i = 1; (i + 1) < data.size(); i++)
					{
						bSpiking[data[i].second] = m_options.m_despike.IsSpiking(data[i - 1].first, data[i].first, data[i + 1].first);
					}

				}

				for (size_t zz = 0; zz < nbScenesProcess; zz++)
				{
					size_t z = m_options.m_scenes[0] + zz;
					CLandsatPixel pixel = m_options.m_rings == 0 ? window.GetPixel(z, x, y) : window.GetPixelMean(z, x, y, (int)m_options.m_rings, m_options.m_weight);
					if (pixel.IsValid() && !bSpiking[zz])
					{
						//bool bIsBlack = pixel.IsBlack();
						//if (!bIsBlack)
						size_t ii = 0;
						for (size_t i = 0; i < m_options.m_indices.size(); i++)
						{
							if (m_options.m_indices.test(i))
							{
								size_t xy = (size_t)y*blockSize.m_x + x;
								outputData[zz][ii][xy] = pixel[Landsat::TIndices(i)];
								ii++;
							}
						}
					}
				}
			}
		}

#pragma omp atomic 
		m_options.m_xx += blockSize.m_x*blockSize.m_y;

		m_options.UpdateBar();
		m_options.m_timerProcess.Stop();

		bandHolder.FlushCache();
		//}
	}

	void CLandsatIndices::WriteBlock(int xBlock, int yBlock, CBandsHolder& bandHolder, CGDALDatasetEx& outputDS, OutputData& outputData)
	{
#pragma omp critical(BlockIO)
		{
			m_options.m_timerWrite.Start();


			CGeoExtents extents = bandHolder.GetExtents();
			CGeoRectIndex outputRect = extents.GetBlockRect(xBlock, yBlock);
			__int16 noData = (__int16)m_options.m_dstNodata;


			if (outputDS.IsOpen())
			{
				ASSERT(outputRect.m_x >= 0 && outputRect.m_x < outputDS.GetRasterXSize());
				ASSERT(outputRect.m_y >= 0 && outputRect.m_y < outputDS.GetRasterYSize());
				ASSERT(outputRect.m_xSize > 0 && outputRect.m_xSize <= outputDS.GetRasterXSize());
				ASSERT(outputRect.m_ySize > 0 && outputRect.m_ySize <= outputDS.GetRasterYSize());

				for (size_t z = 0; z < outputData.size(); z++)
				{
					for (size_t i = 0; i < outputData[z].size(); i++)
					{
						size_t b = z * m_options.m_indices.count() + i;
						GDALRasterBand *pBand = outputDS.GetRasterBand(b);
						if (!outputData[z][i].empty())
						{
							pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(outputData[z][i][0]), outputRect.Width(), outputRect.Height(), GDT_Int16, 0, 0);
						}
						else
						{
							pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &noData, 1, 1, GDT_Int16, 0, 0);
						}
					}
				}
			}


			m_options.m_timerWrite.Stop();
		}
	}

	void CLandsatIndices::CloseAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS)
	{
		inputDS.Close();
		maskDS.Close();


		m_options.m_timerWrite.Start();
		outputDS.Close(m_options);

		m_options.m_timerWrite.Stop();
		m_options.PrintTime();
	}

	ERMsg CLandsatIndices::CreateVirtual(CLandsatDataset& inputDS)
	{
		ERMsg msg;

		size_t nbScenedProcess = m_options.m_scenes[1] - m_options.m_scenes[0] + 1;
		for (size_t zz = 0; zz < nbScenedProcess; zz++)
		{
			size_t z = m_options.m_scenes[0] + zz;
			for (size_t i = 0; i < m_options.m_indices.size(); i++)
			{
				if (m_options.m_indices.test(i))
				{
					string subName = WBSF::TrimConst(inputDS.GetCommonImageName(z), "_");
					std::string filePath = m_options.m_filesPath[CLandsatIndicesOption::OUTPUT_FILE_PATH];
					filePath = GetPath(filePath) + GetFileTitle(filePath) + "_" + subName + "_" + Landsat::GetIndiceName(i) + ".vrt";
					msg += inputDS.CreateIndices(z, filePath, (Landsat::TIndices)i);

					//Input : le vrt de nos 30 ans Landsat + le dd5, DEM, slope.Le Seuil du despike(ex. 0.75) et le seuil de tolérance(ex. 0.1)
					//En output : une couche de dNBR par années avec du No Data supprimé par le despike. .
				}
			}
		}

		return msg;
	}
}

