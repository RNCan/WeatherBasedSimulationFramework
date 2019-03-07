//***********************************************************************
// program to detect insect perturbation from Landsat image over a period
//									 
//***********************************************************************
// version
// 1.0.2	22/02/2019	Rémi Saint-Amant	bug correction in last segment
// 1.0.1	01/02/2019	Rémi Saint-Amant	Add export by segment
// 1.0.0	27/01/2019	Rémi Saint-Amant	Creation


//-scenes 5 8 -i NBR -Virtual -of VRT -co "compress=LZW" --config GDAL_CACHEMAX 4096  -overview {2,4,8,16} -multi -IOCPU 3 -overwrite  "U:\GIS\#documents\TestCodes\SlowDisturbanceAnalyser\Input\1999-2014.vrt" "U:\GIS\#documents\TestCodes\SlowDisturbanceAnalyser\Output\out.vrt"
//-scenes 15 17 -i TCB -stats -Virtual -NoResult -of VRT -co "compress=LZW" --config GDAL_CACHEMAX 4096  -overview {2,4,8,16} -multi -overwrite "D:\Travaux\CloudCleaner\Input\34 ans\Te2.vrt" "D:\Travaux\CloudCleaner\Output\34 ans\Indices.vrt"

#include "stdafx.h"
#include <float.h>
#include <math.h>
#include <array>
#include <utility>
#include <iostream>

#include "DisturbanceAnalyserII.h"
#include "..\Segmentation\SegmentationBase.h"
#include "Basic/OpenMP.h"
#include "Basic/UtilTime.h"
#include "Basic/UtilMath.h"
#include "Geomatic/LandsatCloudsCleaner.h"
#include "Geomatic/See5hooks.h"
#pragma warning(disable: 4275 4251)
#include "gdal_priv.h"

using namespace std;
using namespace WBSF::Landsat;

namespace WBSF
{
	const char* CDisturbanceAnalyserII::VERSION = "1.0.2";
	typedef CDecisionTreeBlock CSee5TreeBlock;
	typedef CDecisionTreeBaseEx CSee5Tree;
	typedef CDecisionTree CSee5TreeMT;


	///*********************************************************************************************************************

	CDisturbanceAnalyserIIOption::CDisturbanceAnalyserIIOption()
	{
		m_outputType = GDT_Int16;
		m_indice = Landsat::I_NBR;
		m_scenesSize = SCENES_SIZE;
		m_scenes = { { NOT_INIT, NOT_INIT } };
		m_rings = 0;
		m_RMSEThreshold = 50;
		m_maxBreaks = 12;
		m_firstYear = 0;
		m_dNBRThreshold = 125;
		m_bExportBrks = false;
		m_bExportAll = false;

		m_appDescription = "This software detect disturbance (harvest, insect) over a series of Landsat images.";

		AddOption("-Period");
		AddOption("-Rename");
		AddOption("-iFactor");

		static const COptionDef OPTIONS[] =
		{
			{ "-i", 1, "indice", false, "Select indice to run model. Indice can be \"B1\"..\"JD\", \"NBR\", \"NDVI\", \"NDMI\", \"TCB\", \"TCG\", \"TCW\", \"NBR2\", \"EVI\", \"SAVI\", \"MSAVI\", \"SR\", \"CL\", \"HZ\". NBR by default"  },
			{ "-Despike", 3, "type threshold min", true, "Despike to remove outbound pixels. Type is the indice type, threshold is the despike threshold and min is the minimum between T-1 and T+1 to execute despike. Supported type are the same as indice. Usual value are NBR 0.75 0.1." },
			{ "-Window", 1, "n", false, "Compute window mean around the pixel. n is the number of rings. 0 = 1x1, 1 = 3x3, 2 = 5x5 etc. By default all rings have the same weight. Weight can be modified with option -weight. 1x1 by default." },
			{ "-Weight", 1, "{w0,w1,...,wn}", false, "Change the weight of window's rings. For example, if you enter {8,1} for a 3x3 windows, weight of the center pixel will be w0=8/16 (50%) and each pixel of the ring will be w1=1/16. So the ring will have 50%. Equal weights by default." },
			{ "-RMSEThreshold", 1, "t", false, "RMSE threshold of the indice (NBR) value to continue breaking series. 50 by default." },
			{ "-MaxBreaks", 1, "n", false, "Maximum number of breaks including beginning and end as a break. Minimum value 3, 12 by default" },
			{ "-Year", 0, "", false, "Return year instead of index. By default, return the image index (0..nbImages-1)" },
			{ "-ExportBrks", 0, "", false, "Export breaks indice (NBR) and reference image (index or year if -Year is define). False by default." },
			{ "-ExportSgts", 0, "", false, "Export perturbations by segment. False by default." },
			{ "-ExportAll", 0, "", false, "Export all perturbations by year. Export only the last by default." },
			{ "-dThreshold", 1, "n", false, "Delta (T1-T2) indice (NBR) is greater than threshold to compute See5 model. For NBR, ranging from 100 to 150. 125 by default." },
			//			{ "-OutputPeriod", 2, "Begin End", false, "Select the period (YYYY-MM-DD YYYY-MM-DD) to output disturbance. The entire period are selected by default." },
			{ "See5Model", 0, "", false, "See5 detection model file path." },
			{ "srcfile", 0, "", false, "Input Landsat image file path." },
			{ "dstfile", 0, "", false, "Output image file path." }
		};


		for (int i = 0; i < sizeof(OPTIONS) / sizeof(COptionDef); i++)
			AddOption(OPTIONS[i]);

		//RemoveOption("-BLOCK_THREADS");
		RemoveOption("-ot");
		//RemoveOption("-CPU");//no multi thread in inner loop

		static const CIOFileInfoDef IO_FILE_INFO[] =
		{
			{ "Input Model", "DTModel","","","","Decision tree model file generate by See5."},
			{ "Input Image", "srcfile", "", "ScenesSize(9)*nbScenes", "B1: Landsat band 1|B2: Landsat band 2|B3: Landsat band 3|B4: Landsat band 4|B5: Landsat band 5|B6: Landsat band 6|B7: Landsat band 7|QA: Image quality|Date: date of image(Julian day 1970 or YYYYMMDD format)|... for all scenes", "" },
			{ "Output Image", "dstfile", "nbIndices", "nbScenes", "Scenes indices", "" },
		};

		for (int i = 0; i < sizeof(IO_FILE_INFO) / sizeof(CIOFileInfoDef); i++)
			AddIOFileInfo(IO_FILE_INFO[i]);
	}

	ERMsg CDisturbanceAnalyserIIOption::ParseOption(int argc, char* argv[])
	{
		ERMsg msg = CBaseOptions::ParseOption(argc, argv);

		ASSERT(NB_FILE_PATH == 3);
		if (msg && m_filesPath.size() != NB_FILE_PATH)
		{
			msg.ajoute("ERROR: Invalid argument line. 3 files are needed: the See5 model, the source and destination image.");
			msg.ajoute("Argument found: ");
			for (size_t i = 0; i < m_filesPath.size(); i++)
				msg.ajoute("   " + to_string(i + 1) + "- " + m_filesPath[i]);
		}


		if (m_weight_str.empty())
		{
			m_weight.resize(m_rings + 1, 1.0 / Square(2 * m_rings + 1));
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



		Landsat::INDICES_FACTOR(m_iFactor);

		return msg;
	}

	ERMsg CDisturbanceAnalyserIIOption::ProcessOption(int& i, int argc, char* argv[])
	{
		ERMsg msg;

		if (IsEqual(argv[i], "-i"))
		{
			string str = argv[++i];
			m_indice = GetIndiceType(str);
			if (m_indice >= NB_INDICES)
			{
				msg.ajoute(str + " is not a valid indice. See help.");
			}
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
		else if (IsEqual(argv[i], "-RMSEThreshold"))
		{
			m_RMSEThreshold = atof(argv[++i]);
		}
		else if (IsEqual(argv[i], "-MaxBreaks"))
		{
			m_maxBreaks = atoi(argv[++i]);//to add begin and end
			if (m_maxBreaks < 3)
				msg.ajoute("Invalid -MaxBreaks option. -MaxBreaks must be greater or equal to 3");
		}
		else if (IsEqual(argv[i], "-Year"))
		{
			m_bYear = true;
		}
		else if (IsEqual(argv[i], "-dThreshold"))
		{
			m_dNBRThreshold = atoi(argv[++i]);
		}

		else if (IsEqual(argv[i], "-ExportSgts"))
		{
			m_bExportSgts = true;
		}
		else if (IsEqual(argv[i], "-ExportBrks"))
		{
			m_bExportBrks = true;
		}
		else if (IsEqual(argv[i], "-ExportAll"))
		{
			m_bExportAll = true;
		}
		//else if (IsEqual(argv[i], "-OutputPeriod"))
		//{
		//	m_outputPeriod.Begin().FromFormatedString(argv[++i], "", "-", 1);//in 1 base
		//	m_outputPeriod.End().FromFormatedString(argv[++i], "", "-", 1);
		//}
		//else if (IsEqual(argv[i], "-Debug"))
		//{
		//	m_bDebug = true;
		//}
		else
		{
			//Look to see if it's a know base option
			msg = CBaseOptions::ProcessOption(i, argc, argv);
		}


		return msg;
	}



	ERMsg CDisturbanceAnalyserII::LoadModel(string filePath, CSee5TreeMT& DT)
	{
		ERMsg msg;
		if (!m_options.m_bQuiet)
		{
			cout << "Read rules..." << endl;
		}

		CTimer timer(true);

		msg += DT.Load(filePath, m_options.m_CPU, m_options.m_IOCPU);
		timer.Stop();

		if (!m_options.m_bQuiet)
			cout << "Read rules time = " << SecondToDHMS(timer.Elapsed()).c_str() << endl << endl;


		return msg;
	}

	//	static size_t test1[16] = { 0 };
	//static size_t test2[16] = { 0 };

	ERMsg CDisturbanceAnalyserII::Execute()
	{
		ERMsg msg;

		if (!m_options.m_bQuiet)
		{
			cout << "Output:  " << m_options.m_filesPath[CDisturbanceAnalyserIIOption::OUTPUT_FILE_PATH] << endl;
			cout << "From:    " << m_options.m_filesPath[CDisturbanceAnalyserIIOption::INPUT_FILE_PATH] << endl;
			cout << "Model:   " << m_options.m_filesPath[CDisturbanceAnalyserIIOption::MODEL_FILE_PATH] << endl;
			cout << "Indices: " << Landsat::GetIndiceName(m_options.m_indice) << endl;

			if (!m_options.m_maskName.empty())
				cout << "Mask:   " << m_options.m_maskName << endl;
		}

		GDALAllRegister();

		CSee5TreeMT DT[NB_MODELS];
		msg += LoadModel(m_options.m_filesPath[CDisturbanceAnalyserIIOption::MODEL_FILE_PATH] + "_T123", DT[MODEL_3BRK]);
		msg += LoadModel(m_options.m_filesPath[CDisturbanceAnalyserIIOption::MODEL_FILE_PATH] + "_T12", DT[MODEL_2BRK]);

		if (!msg)
			return msg;


		CLandsatDataset inputDS;
		CGDALDatasetEx maskDS;
		CGDALDatasetEx outputDS;
		CGDALDatasetEx segmentsDS;
		CGDALDatasetEx breaksDS;

		msg = OpenAll(inputDS, maskDS, outputDS, segmentsDS, breaksDS);


		CBandsHolderMT bandHolder(int(m_options.m_rings * 2 + 1), m_options.m_memoryLimit, m_options.m_IOCPU, m_options.m_CPU);
		if (msg && maskDS.IsOpen())
			bandHolder.SetMask(maskDS.GetSingleBandHolder(), m_options.m_maskDataUsed);

		if (msg)
			msg += bandHolder.Load(inputDS, m_options.m_bQuiet, m_options.GetExtents(), m_options.m_period);

		if (!msg)
			return msg;

		CGeoExtents extents = bandHolder.GetExtents();
		size_t nbScenedProcess = m_options.m_scenes[1] - m_options.m_scenes[0] + 1;

		m_options.ResetBar(extents.m_xSize*extents.m_ySize);
		vector<pair<int, int>> XYindex = extents.GetBlockList();

		if (!m_options.m_bQuiet)
		{
			if (outputDS.IsOpen())
				cout << "Create output images (" << outputDS.GetRasterXSize() << " C x " << outputDS.GetRasterYSize() << " R x " << outputDS.GetRasterCount() << " B)" << endl;
			if (segmentsDS.IsOpen())
				cout << "Create segment images (" << segmentsDS.GetRasterXSize() << " C x " << segmentsDS.GetRasterYSize() << " R x " << segmentsDS.GetRasterCount() << " B)" << endl;
			if (breaksDS.IsOpen())
				cout << "Create breaks images (" << breaksDS.GetRasterXSize() << " C x " << breaksDS.GetRasterYSize() << " R x " << breaksDS.GetRasterCount() << " B)" << endl;
		}

		omp_set_nested(1);//for IOCPU
#pragma omp parallel for schedule(static, 1) num_threads( m_options.m_BLOCK_THREADS ) if (m_options.m_bMulti) 
//#pragma omp parallel for schedule(static, 1) num_threads( m_options.m_CPU ) if (m_options.m_bMulti) 
		for (__int64 b = 0; b < (__int64)XYindex.size(); b++)
		{
			int xBlock = XYindex[b].first;
			int yBlock = XYindex[b].second;

			int thread = ::omp_get_thread_num();
			//#pragma omp atomic
				//			test1[thread]++;

			OutputData outputData;
			BreakData breaksData;
			SegmentData segmentsData;

			ReadBlock(xBlock, yBlock, bandHolder[thread]);
			ProcessBlock(xBlock, yBlock, bandHolder[thread], DT[MODEL_3BRK], DT[MODEL_2BRK], outputData, segmentsData, breaksData);
			WriteBlock(xBlock, yBlock, outputDS, segmentsDS, breaksDS, outputData, segmentsData, breaksData);
		}//for all blocks


		CloseAll(inputDS, maskDS, outputDS, segmentsDS, breaksDS);

		//cout << "block thread: " << m_options.m_BLOCK_THREADS << endl;
		//cout << "block CPU: " << m_options.BLOCK_CPU() << endl;
		//for (__int64 b = 0; b < 16; b++)
		//	cout << "test " << b << ": " << test1[b] << "       " << test2[b] << endl;

		return msg;
	}



	ERMsg CDisturbanceAnalyserII::OpenAll(CLandsatDataset& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS, CGDALDatasetEx& segmentsDS, CGDALDatasetEx& breaksDS)
	{
		ERMsg msg;

		if (!m_options.m_bQuiet)
		{
			cout << endl << "Open input image..." << endl;
		}

		msg = inputDS.OpenInputImage(m_options.m_filesPath[CDisturbanceAnalyserIIOption::INPUT_FILE_PATH], m_options);
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
						//msg.ajoute("Scenes of the input landsat images (" + GetFileName(m_options.m_filesPath[CDisturbanceAnalyserIIOption::INPUT_FILE_PATH]) + ") are not chronologically ordered.");

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


			const std::vector<CTPeriod>& p = inputDS.GetScenePeriod();

			//set the period to the period in the scene selection
			size_t nbSceneLoaded = m_options.m_scenes[1] - m_options.m_scenes[0] + 1;
			CTPeriod period;
			for (size_t z = 0; z < nbSceneLoaded; z++)
				period.Inflate(p[m_options.m_scenes[0] + z]);

			if (m_options.m_period.IsInit())
				cout << "    User's input working period = " << m_options.m_period.GetFormatedString() << endl;



			//	if (!m_options.m_outputPeriod.IsInit())
					//m_options.m_outputPeriod = m_options.m_period;

			cout << "    Size           = " << inputDS.GetRasterXSize() << " cols x " << inputDS.GetRasterYSize() << " rows x " << inputDS.GetRasterCount() << " bands" << endl;
			cout << "    Extents        = X:{" << ToString(extents.m_xMin) << ", " << ToString(extents.m_xMax) << "}  Y:{" << ToString(extents.m_yMin) << ", " << ToString(extents.m_yMax) << "}" << endl;
			cout << "    Projection     = " << prjName << endl;
			cout << "    NbBands        = " << inputDS.GetRasterCount() << endl;
			cout << "    Scene size     = " << inputDS.GetSceneSize() << endl;
			cout << "    Entire period  = " << inputDS.GetPeriod().GetFormatedString() << " (nb scenes = " << inputDS.GetNbScenes() << ")" << endl;
			cout << "    Working period = " << period.GetFormatedString() << " (nb scenes = " << nbSceneLoaded << ")" << endl;
			//cout << "    Output period  = " << m_options.m_outputPeriod.GetFormatedString() << endl;


			m_options.m_period = period;

			if (m_options.m_bYear)
				m_options.m_firstYear = m_options.m_period.Begin().GetYear();

		}

		if (msg && !m_options.m_maskName.empty())
		{
			if (!m_options.m_bQuiet)
				cout << "Open mask..." << endl;

			msg += maskDS.OpenInputImage(m_options.m_maskName);
		}


		if (msg && m_options.m_bCreateImage)
		{
			if (!m_options.m_bQuiet)
				cout << "Open output images..." << endl;

			size_t nbScenedProcess = m_options.m_scenes[1] - m_options.m_scenes[0] + 1;

			string filePath = m_options.m_filesPath[CDisturbanceAnalyserIIOption::OUTPUT_FILE_PATH];
			CDisturbanceAnalyserIIOption options = m_options;
			size_t nb_outputs = options.m_bExportAll ? nbScenedProcess - 1 : 1;//-1 because the first year have never perturbation
			options.m_nbBands = nb_outputs * NB_OUTPUTS;


			//replace the common part by the new name
			set<string> subnames;
			for (size_t zz = 0; zz < nb_outputs; zz++)
			{
				size_t z = m_options.m_scenes[1];
				if (options.m_bExportAll)
					z = m_options.m_scenes[0] + zz + 1;//+1 skip the first

				string subName = inputDS.GetSubname(z, m_options.m_rename);
				string uniqueSubName = subName;
				size_t i = 1;
				while (subnames.find(uniqueSubName) != subnames.end())
					uniqueSubName = subName + "_" + to_string(++i);

				subnames.insert(uniqueSubName);

				options.m_VRTBandsName += GetFileTitle(filePath) + "_" + uniqueSubName + "_DTcode.tif|";
				options.m_VRTBandsName += GetFileTitle(filePath) + "_" + uniqueSubName + "_dNBR.tif|";
				options.m_VRTBandsName += GetFileTitle(filePath) + "_" + uniqueSubName + "_T1.tif|";
			}

			msg += outputDS.CreateImage(filePath, options);
		}//create output

		if (msg && m_options.m_bExportSgts)
		{
			if (!m_options.m_bQuiet)
				cout << "Open segments images..." << endl;

			size_t nbScenedProcess = m_options.m_scenes[1] - m_options.m_scenes[0] + 1;

			string filePath = m_options.m_filesPath[CDisturbanceAnalyserIIOption::OUTPUT_FILE_PATH];
			SetFileTitle(filePath, GetFileTitle(filePath) + "_sgt");
			CDisturbanceAnalyserIIOption options = m_options;
			options.m_nbBands = (options.m_maxBreaks - 1) * NB_SEGMENTS_OUTPUTS + 1;//+1 for nbSeg

			options.m_VRTBandsName += GetFileTitle(filePath) + "_NbSeg.tif|";
			//replace the common part by the new name
			set<string> subnames;
			for (size_t z = 0; z < options.m_maxBreaks - 1; z++)
			{
				string subName = inputDS.GetSubname(z, m_options.m_rename);
				string uniqueSubName = subName;
				size_t i = 1;
				while (subnames.find(uniqueSubName) != subnames.end())
					uniqueSubName = subName + "_" + to_string(++i);

				subnames.insert(uniqueSubName);

				options.m_VRTBandsName += GetFileTitle(filePath) + "_" + FormatA("%02d_DTcode.tif|", z + 1);
				options.m_VRTBandsName += GetFileTitle(filePath) + "_" + FormatA("%02d_dNBR.tif|", z + 1);
				options.m_VRTBandsName += GetFileTitle(filePath) + "_" + FormatA("%02d_T1.tif|", z + 1);
				options.m_VRTBandsName += GetFileTitle(filePath) + "_" + FormatA("%02d_T2.tif|", z + 1);
			}

			msg += segmentsDS.CreateImage(filePath, options);
		}//create output

		if (msg && m_options.m_bExportBrks)
		{
			if (!m_options.m_bQuiet)
				cout << "Open breaks images..." << endl;

			string filePath = m_options.m_filesPath[CDisturbanceAnalyserIIOption::OUTPUT_FILE_PATH];
			SetFileTitle(filePath, GetFileTitle(filePath) + "_brk");
			CDisturbanceAnalyserIIOption options = m_options;
			options.m_outputType = GDT_Int16;
			options.m_nbBands = NB_BREAKS_OUTPUTS * options.m_maxBreaks;

			for (size_t s = 0; s < options.m_maxBreaks; s++)
			{
				options.m_VRTBandsName += GetFileTitle(filePath) + FormatA("%02d_NBR.tif|", s + 1);
				options.m_VRTBandsName += GetFileTitle(filePath) + FormatA("%02d_T.tif|", s + 1);
			}

			msg += breaksDS.CreateImage(filePath, options);
		}


		return msg;
	}


	void CDisturbanceAnalyserII::ReadBlock(int xBlock, int yBlock, CBandsHolder& bandHolder)
	{
#pragma omp critical(BlockIORead)
		{
			m_options.m_timerRead.Start();

			CGeoExtents extents = m_options.m_extents.GetBlockExtents(xBlock, yBlock);
			bandHolder.LoadBlock(extents);

			m_options.m_timerRead.Stop();
		}
	}




	void CDisturbanceAnalyserII::ProcessBlock(int xBlock, int yBlock, CBandsHolder& bandHolder, CSee5TreeMT& DT123, CSee5TreeMT& DT12, OutputData& outputData, SegmentData& segmentsData, BreakData& breaksData)
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



		if (m_options.m_bCreateImage)
		{
			__int16 dstNodata = (__int16)m_options.m_dstNodata;
			size_t nbOutputs = m_options.m_bExportAll ? nbScenesProcess - 1 : 1;
			outputData.resize(nbOutputs);
			for (size_t i = 0; i < outputData.size(); i++)
			{
				for (size_t j = 0; j < outputData[i].size(); j++)
					outputData[i][j].resize(blockSize.m_x*blockSize.m_y, dstNodata);
			}

		}

		if (m_options.m_bExportSgts)
		{
			__int16 dstNodata = (__int16)GetDefaultNoData(GDT_Int16);
			segmentsData.m_nbSegments.resize(blockSize.m_x*blockSize.m_y, dstNodata);
			segmentsData.m_segments.resize(m_options.m_maxBreaks - 1);
			for (size_t i = 0; i < segmentsData.m_segments.size(); i++)
			{
				for (size_t j = 0; j < segmentsData.m_segments[i].size(); j++)
					segmentsData.m_segments[i][j].resize(blockSize.m_x*blockSize.m_y, dstNodata);
			}
		}

		if (m_options.m_bExportBrks)
		{
			__int16 dstNodata = (__int16)GetDefaultNoData(GDT_Int16);
			breaksData.resize(m_options.m_maxBreaks);
			for (size_t i = 0; i < breaksData.size(); i++)
			{
				for (size_t j = 0; j < breaksData[i].size(); j++)
					breaksData[i][j].resize(blockSize.m_x*blockSize.m_y, dstNodata);
			}
		}

#pragma omp critical(Process)
		{
			m_options.m_timerProcess.Start();
			//	int thread1 = ::omp_get_thread_num()*m_options.BLOCK_CPU();

				//multi thread here is very not efficient.
#pragma omp parallel for num_threads( m_options.m_CPU - m_options.m_BLOCK_THREADS) if (m_options.m_bMulti) 
			for (int y = 0; y < blockSize.m_y; y++)
			{
				for (int x = 0; x < blockSize.m_x; x++)
				{
					int thread = ::omp_get_thread_num();
					//#pragma omp atomic
						//			test2[thread]++;
					int xy = y * blockSize.m_x + x;
					vector<bool> bSpiking(nbScenesProcess, false);

					if (!m_options.m_despike.empty())//if despike
					{
						vector< pair<CLandsatPixel, size_t>> data;

						ASSERT(window.GetNbScenes() >= nbScenesProcess);
						data.reserve(nbScenesProcess);

						for (size_t zz = 0; zz < nbScenesProcess; zz++)
						{
							size_t z = m_options.m_scenes[0] + zz;
							CLandsatPixel pixel = ((CLandsatWindow&)window).GetPixelMean(z, x, y, (int)m_options.m_rings, m_options.m_weight);

							if (pixel.IsValid())
								data.push_back(make_pair(pixel, zz));
						}

						for (size_t i = 1; (i + 1) < data.size(); i++)
						{
							bSpiking[data[i].second] = m_options.m_despike.IsSpiking(data[i - 1].first, data[i].first, data[i + 1].first);
						}

					}

					//Get pixels
					vector<double> dataNBR1(nbScenesProcess);
					NBRVector dataNBR2;
					dataNBR2.reserve(nbScenesProcess);

					for (size_t zz = 0; zz < nbScenesProcess; zz++)
					{
						size_t z = m_options.m_scenes[0] + zz;
						CLandsatPixel pixel = window.GetPixelMean(z, x, y, (int)m_options.m_rings, m_options.m_weight);

						if (pixel.IsValid() && !bSpiking[zz])
						{
							size_t xy = (size_t)y*blockSize.m_x + x;
							double nbr = pixel[m_options.m_indice];
							dataNBR1[zz] = nbr;
							dataNBR2.push_back(make_pair(nbr, zz));
						}
					}

					//if there are more than 2 points
					if (dataNBR2.size() >= 2)
					{
						//compute segmentation
						std::vector<size_t> breaks = Segmentation(dataNBR2, m_options.m_maxBreaks, m_options.m_RMSEThreshold);

						ASSERT(breaks.size() >= 2);

						if (!outputData.empty() || !segmentsData.m_nbSegments.empty())
						{
							__int16 nbSegment = 0;
							for (size_t i = 1; i < breaks.size(); i++)
							{
								//compute DT for all segment that NBR drop more than a threshold
								//compute segmentation for this time series
								size_t t1 = breaks[i - 1];
								size_t t2 = breaks[i];
								__int16 deltaNBR = __int16(dataNBR1[t1] - dataNBR1[t2]);
								if (deltaNBR > m_options.m_dNBRThreshold)
								{
									nbSegment++;
								}
							}

							size_t seg = 0;
							for (size_t i = 1; i < breaks.size(); i++)
							{
								//compute DT for all segment that NBR drop more than a threshold
								//compute segmentation for this time series
								size_t t1 = breaks[i - 1];
								size_t t2 = breaks[i];
								__int16 deltaNBR = __int16(dataNBR1[t1] - dataNBR1[t2]);
								if (deltaNBR > m_options.m_dNBRThreshold)
								{
									seg++;
									size_t f = (i < breaks.size() - 1) ? 0 : 1;
									CSee5Tree& DT = (f == MODEL_3BRK) ? DT123[thread] : DT12[thread];

									if ((seg == nbSegment) || m_options.m_bExportAll)
									{
										CSee5TreeBlock block(DT.MaxAtt + 1);

										//fill the data structure for decision tree
										size_t c = 0;
										DVal(block, c++) = DT.MaxClass + 1;
										DVal(block, c++) = Continuous(DT, 1) ? DT_UNKNOWN : 0;

										size_t nb_z = (f == MODEL_3BRK) ? 3 : 2;
										for (size_t zz = 0; zz < nb_z; zz++)
										{
											size_t z = breaks[i + zz - 1];
											CLandsatPixel pixel = window.GetPixel(z, x, y);

											CVal(block, c++) = (ContValue)(pixel[I_B3]);
											CVal(block, c++) = (ContValue)(pixel[I_B4]);
											CVal(block, c++) = (ContValue)(pixel[I_B5]);
											CVal(block, c++) = (ContValue)(pixel[I_B7]);
											CVal(block, c++) = (ContValue)(pixel.GetTRef().GetYear());
										}


										ASSERT(c == (nb_z * 5 + 2));

										//fill virtual bands 
										//assuming virtual band never return no data
										for (; c <= DT.MaxAtt; c++)
										{
											ASSERT(DT.AttDef[c]);
											block[c] = DT.EvaluateDef(DT.AttDef[c], block.data());
										}

										ASSERT(!block.empty());
										int predict = (int)DT.Classify(block.data());

										ASSERT(predict >= 1 && predict <= DT.MaxClass);
										int DTCode = atoi(DT.ClassName[predict]);

										if (!outputData.empty())
										{
											size_t z = 0;
											if (m_options.m_bExportAll)
												z = t2 - 1;//-1 because the first year is removed

											outputData[z][O_DT_CODE][xy] = (__int16)DTCode;
											outputData[z][O_DELTA_NBR][xy] = deltaNBR;
											outputData[z][O_T1][xy] = m_options.m_firstYear + (__int16)t1;
										}

										if (!segmentsData.m_nbSegments.empty())
										{
											ASSERT(i < breaks.size());
											//size_t z = breaks.size() - i - 1;//reverse order
											if (segmentsData.m_nbSegments[xy] < 0)
												segmentsData.m_nbSegments[xy] = 1;
											else 
												segmentsData.m_nbSegments[xy]++;

											size_t z = nbSegment - segmentsData.m_nbSegments[xy];
											segmentsData.m_segments[z][O_DT_CODE][xy] = (__int16)DTCode;
											segmentsData.m_segments[z][O_DELTA_NBR][xy] = deltaNBR;
											segmentsData.m_segments[z][O_T1][xy] = m_options.m_firstYear + (__int16)t1;
											segmentsData.m_segments[z][O_T2][xy] = m_options.m_firstYear + (__int16)t2;
										}
									}
									
								}//if greater than dThreshold
							}//for all breaks

							
						}//if output data

						if (!breaksData.empty())
						{
							ASSERT(breaks.size() <= breaksData.size());
							for (size_t i = 0; i < breaks.size(); i++)
							{
								size_t z = breaks[breaks.size() - i - 1];//reverse order
								breaksData[i][O_NBR][xy] = (__int16)dataNBR1[z];
								breaksData[i][O_T][xy] = m_options.m_firstYear + (__int16)z;
							}
						}
					}
				}
			}

#pragma omp atomic 
			m_options.m_xx += blockSize.m_x*blockSize.m_y;

			m_options.UpdateBar();
			m_options.m_timerProcess.Stop();

			//bandHolder.FlushCache();
		}

	}

	void CDisturbanceAnalyserII::WriteBlock(int xBlock, int yBlock, CGDALDatasetEx& outputDS, CGDALDatasetEx& segmentsDS, CGDALDatasetEx& breaksDS, OutputData& outputData, SegmentData& segmentsData, BreakData& breaksData)
	{
#pragma omp critical(BlockIOWrite)
		{
			m_options.m_timerWrite.Start();

			if (outputDS.IsOpen())
			{
				CGeoExtents extents = outputDS.GetExtents();
				CGeoSize blockSize = extents.GetBlockSize(xBlock, yBlock);
				CGeoRectIndex outputRect = extents.GetBlockRect(xBlock, yBlock);
				__int16 noData = (__int16)m_options.m_dstNodata;

				ASSERT(outputRect.m_x >= 0 && outputRect.m_x < outputDS.GetRasterXSize());
				ASSERT(outputRect.m_y >= 0 && outputRect.m_y < outputDS.GetRasterYSize());
				ASSERT(outputRect.m_xSize > 0 && outputRect.m_xSize <= outputDS.GetRasterXSize());
				ASSERT(outputRect.m_ySize > 0 && outputRect.m_ySize <= outputDS.GetRasterYSize());

				size_t nbScenesProcess = m_options.m_scenes[1] - m_options.m_scenes[0] + 1;
				size_t nbOutputs = m_options.m_bExportAll ? nbScenesProcess - 1 : 1;//-1 because first year is remove


#pragma omp parallel for num_threads( m_options.m_IOCPU ) if (m_options.m_bMulti&&outputDS.IsVRT()) 
				for (int z = 0; z < nbOutputs; z++)
				{
					for (size_t i = 0; i < NB_OUTPUTS; i++)
					{
						size_t b = z * NB_OUTPUTS + i;
						GDALRasterBand *pBand = outputDS.GetRasterBand(b);
						if (!outputData.empty())
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

			if (segmentsDS.IsOpen())
			{
				CGeoExtents extents = segmentsDS.GetExtents();
				CGeoSize blockSize = extents.GetBlockSize(xBlock, yBlock);
				CGeoRectIndex outputRect = extents.GetBlockRect(xBlock, yBlock);
				__int16 noData = (__int16)m_options.m_dstNodata;

				ASSERT(outputRect.m_x >= 0 && outputRect.m_x < segmentsDS.GetRasterXSize());
				ASSERT(outputRect.m_y >= 0 && outputRect.m_y < segmentsDS.GetRasterYSize());
				ASSERT(outputRect.m_xSize > 0 && outputRect.m_xSize <= segmentsDS.GetRasterXSize());
				ASSERT(outputRect.m_ySize > 0 && outputRect.m_ySize <= segmentsDS.GetRasterYSize());

				GDALRasterBand *pBand = segmentsDS.GetRasterBand(0);//first bans is the number of segment
				if (!segmentsData.m_nbSegments.empty())
				{
					pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(segmentsData.m_nbSegments[0]), outputRect.Width(), outputRect.Height(), GDT_Int16, 0, 0);
				}
				else
				{
					pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &noData, 1, 1, GDT_Int16, 0, 0);
				}

#pragma omp parallel for num_threads( m_options.m_IOCPU ) if (m_options.m_bMulti&&segmentsDS.IsVRT()) 
				for (int z = 0; z < m_options.m_maxBreaks - 1; z++)
				{
					for (size_t i = 0; i < NB_SEGMENTS_OUTPUTS; i++)
					{
						size_t b = z * NB_SEGMENTS_OUTPUTS + i + 1;//skip first band
						GDALRasterBand *pBand = segmentsDS.GetRasterBand(b);

						if (!segmentsData.m_nbSegments.empty())
						{
							pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(segmentsData.m_segments[z][i][0]), outputRect.Width(), outputRect.Height(), GDT_Int16, 0, 0);
						}
						else
						{
							pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &noData, 1, 1, GDT_Int16, 0, 0);
						}
					}
				}
			}


			if (breaksDS.IsOpen())
			{
				CGeoExtents extents = breaksDS.GetExtents();
				CGeoSize blockSize = extents.GetBlockSize(xBlock, yBlock);
				CGeoRectIndex outputRect = extents.GetBlockRect(xBlock, yBlock);
				__int16 noData = (__int16)m_options.m_dstNodata;

				ASSERT(outputRect.m_x >= 0 && outputRect.m_x < breaksDS.GetRasterXSize());
				ASSERT(outputRect.m_y >= 0 && outputRect.m_y < breaksDS.GetRasterYSize());
				ASSERT(outputRect.m_xSize > 0 && outputRect.m_xSize <= breaksDS.GetRasterXSize());
				ASSERT(outputRect.m_ySize > 0 && outputRect.m_ySize <= breaksDS.GetRasterYSize());

#pragma omp parallel for num_threads( m_options.m_IOCPU ) if (m_options.m_bMulti&&breaksDS.IsVRT()) 
				for (int z = 0; z < m_options.m_maxBreaks; z++)
				{
					for (size_t i = 0; i < NB_BREAKS_OUTPUTS; i++)
					{
						size_t b = z * NB_BREAKS_OUTPUTS + i;
						GDALRasterBand *pBand = breaksDS.GetRasterBand(b);
						if (!breaksData.empty())
						{
							pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(breaksData[z][i][0]), outputRect.Width(), outputRect.Height(), GDT_Int16, 0, 0);
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

	void CDisturbanceAnalyserII::CloseAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS, CGDALDatasetEx& segmentsDS, CGDALDatasetEx& breaksDS)
	{
		inputDS.Close();
		maskDS.Close();


		m_options.m_timerWrite.Start();
		outputDS.Close(m_options);
		segmentsDS.Close(m_options);
		breaksDS.Close(m_options);


		m_options.m_timerWrite.Stop();
		m_options.PrintTime();
	}

	//ERMsg CDisturbanceAnalyserII::CreateVirtual(CLandsatDataset& inputDS)
	//{
	//	ERMsg msg;

	//	size_t nbScenedProcess = m_options.m_scenes[1] - m_options.m_scenes[0] + 1;
	//	for (size_t zz = 0; zz < nbScenedProcess; zz++)
	//	{
	//		size_t z = m_options.m_scenes[0] + zz;
	//		for (size_t i = 0; i < m_options.m_indices.size(); i++)
	//		{
	//			if (m_options.m_indices.test(i))
	//			{
	//				string subName = WBSF::TrimConst(inputDS.GetCommonImageName(z), "_");
	//				std::string filePath = m_options.m_filesPath[CDisturbanceAnalyserIIOption::OUTPUT_FILE_PATH];
	//				filePath = GetPath(filePath) + GetFileTitle(filePath) + "_" + subName + "_" + Landsat::GetIndiceName(i) + ".vrt";
	//				msg += inputDS.CreateIndices(z, filePath, (Landsat::TIndices)i);

	//				//Input : le vrt de nos 30 ans Landsat + le dd5, DEM, slope.Le Seuil du despike(ex. 0.75) et le seuil de tolérance(ex. 0.1)
	//				//En output : une couche de dNBR par années avec du No Data supprimé par le despike. .
	//			}
	//		}
	//	}

	//	return msg;
	//}
}

