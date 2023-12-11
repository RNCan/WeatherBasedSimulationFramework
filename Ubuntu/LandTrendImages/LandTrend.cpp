//***********************************************************************
// program to merge Landsat image image over a period
//
//***********************************************************************
// version
// 1.0.4	11/12/2023	Rémi Saint-Amant	Bug correction with validity position
// 1.0.3	02/11/2023	Rémi Saint-Amant	Change -ValidityMask to -CloudsMask 
// 1.0.2	27/10/2023	Rémi Saint-Amant	Add -ValidityMask options
// 1.0.1	25/10/2023	Rémi Saint-Amant	Add -BackwardFill -ForwardFill options
// 1.0.0	29/08/2023	Rémi Saint-Amant	Creation from IDL code


//"E:\Landsat\Landsat(2000-2018)\Input\Landsat_2000-2018(2).vrt" "E:\Landsat\Landsat(2000-2018)\Output\test2.vrt" -of VRT -overwrite -co "COMPRESS=LZW"   -te 1022538.9 6663106.0 1040929.5 6676670.7 -multi -Debug -SpikeThreshold 0.75 -FitMethod 1



#include <math.h>
#include <array>
#include <utility>
#include <iostream>


#include "LandTrend.h"
#include "basic/OpenMP.h"
#include "geomatic/GDAL.h"
#include "geomatic/LandTrendUtil.h"
#include "geomatic/LandTrendCore.h"





using namespace std;
using namespace WBSF::Landsat2;
using namespace LTR;



namespace WBSF
{
	const char* CLandTrend::VERSION = "1.0.4";
	const size_t CLandTrend::NB_THREAD_PROCESS = 2;


	//*********************************************************************************************************************

	CLandTrendOption::CLandTrendOption()
	{

		m_pval = 0.05;
		m_recovery_threshold = 0.25;
		m_distweightfactor = 2;
		m_vertexcountovershoot = 3;
		m_bestmodelproportion = 0.75;
		m_minneeded = 6;
		m_max_segments = 6;
		m_desawtooth_val = 0.9;
		m_fit_method = FIT_EARLY_TO_LATE;
		m_modifier = -1;

		m_scenes_def = { { B1,B2,B3,B4,B5,B7 } };
		//m_scenesSize = SCENES_SIZE;
		m_indice = I_NBR;
		m_rings = 0;

		m_firstYear = 0;
		m_bBreaks = false;
		m_bBackwardFill = false;
		m_bForwardFill = false;

		m_appDescription = "This software standardize Landsat images  (composed of " + to_string(SCENES_SIZE) + " bands) based on LandTrendR analysis.";


		//AddOption("-RGB");
		static const COptionDef OPTIONS[] =
		{
			{ "-MaxSegments", 1, "s", false, "Maximum number of segments to be fitted on the time series. 6 by default."},
			{ "-SpikeThreshold", 1, "Thres", false, "Threshold for dampening the spikes (1.0 means no dampening) .0.9 by default."},
			{ "-VertexCountOvershoot", 1, "n", false, "The initial model can overshoot the maxSegments + 1 vertices by this amount.Later, it will be pruned down to maxSegments + 1. 3 by default."},
			{ "-RecoveryThreshold", 1, "Thres", false, "If a segment has a recovery rate faster than 1 / recoveryThreshold(in years), then the segment is disallowed. 0.25 by default"},
			{ "-pValThreshold", 1, "pVal", false, "If the p - value of the fitted model exceeds this threshold, then the current model is discarded and another one is fitted using the Levenberg - Marquardt optimizer. 0.1 by default."},
			{ "-BestModelProportion", 1, "f", false, "Allows models with more vertices to be chosen if their p - value is no more than(2 - bestModelProportion) times the p - value of the best model. 0.75 by default."},
			{ "-MinObservationsNeeded", 1, "min", false, "Min observations needed to perform output fitting. 6 by default."},
			{ "-FitMethod", 1, "method", false, "Select between 0=early-to-late regression and 1=MPFit. 0 by default."},
			//{ "-Modifier", 1, "modifier", false, "Modifier to assure that disturbance is always positive. Can be 1 or -1. -1 by default for NBR."},
			{ "-Indice", 1, "indice", false, "Select indice to run model. Indice can be NBR, NDVI, NDMI, NDWI, TCB, TCG, TCW, NBR2, EVI, EVI2, SAVI, MSAVI, SR, CL, HZ, LSWI, VIgreen. NBR by default"  },
			{ "-Window", 1, "radius", false, "Compute window mean around the pixel where the radius is the number of pixels around the pixel: 1 = 1x1, 2 = 3x3, 3 = 5x5 etc. But can also be a float to get the average between 2 rings. For example 1.25 will be compute as follow: 0.75*(1x1) + 0.25*(3x3). 1 by default." },
			{ "-BackwardFill", 0, "", false, "Fill all missing values at the beginning of the series with the first valid value."},
			{ "-ForwardFill", 0, "", false, "Fill all missing values at the end of the series with the last valid value."},
			{ "-CloudsMask", 1, "name", false, "Mask of clouds data. Zero = no clouds, others values are invalid. Number of clouds bands must be the same as the number of scenes (years)." },
			{ "-FirstYear", 1, "year", false, "Specify year of the first image. Return year instead of index. By default, return the image index (0..nbImages-1)" },
			{ "-Breaks",  0,"",false,"Output breaks information (number of segment, segment index/year, segment fit value. "},
			{ "srcfile", 0, "", false, "Input image file path." },
			{ "dstfile", 0, "", false, "Output image file path." }
		};

		AddOption("-ty");
		for (size_t i = 0; i < sizeof(OPTIONS) / sizeof(COptionDef); i++)
			AddOption(OPTIONS[i]);


		//Pour les trigger Bande 1 c’est - 125 quand on fait  ex.b1 1994 – b1 1995 ou b1 1996 – b1 1995.
		//Pour le tassel Cap brightness c’est + 750  ex.tcb1994 – tcb 1995 ou tcb 1996 – tcb 1995


		static const CIOFileInfoDef IO_FILE_INFO[] =
		{
			{ "Input Image", "srcfile", "", "nbYears", "B1: Landsat band 1|B2: Landsat band 2|B3: Landsat band 3|B4: Landsat band 4|B5: Landsat band 5|B7: Landsat band 7|... for all scenes", "" },
			{ "Output Image", "dstfile", "", "nbYears", "Same as input", "" },
			{ "Optional Output Image", "dstfile_breaks","1","NbOutputLayers=(MaxSegments+1)*2+1","Nb vertices: number of vertices found|vert1: vertice1. Year if FirstYear is define|fit1: fit of vertice1|... for all vertices"}
		};

		for (size_t i = 0; i < sizeof(IO_FILE_INFO) / sizeof(CIOFileInfoDef); i++)
			AddIOFileInfo(IO_FILE_INFO[i]);
	}

	ERMsg CLandTrendOption::ParseOption(int argc, char* argv[])
	{
		ERMsg msg = CBaseOptions::ParseOption(argc, argv);

		assert(NB_FILE_PATH == 2);
		if (msg && m_filesPath.size() != NB_FILE_PATH)
		{
			msg.ajoute("ERROR: Invalid argument line. 2 files are needed: the source && destination image.");
			msg.ajoute("Argument found: ");
			for (size_t i = 0; i < m_filesPath.size(); i++)
				msg.ajoute("   " + to_string(i + 1) + "- " + m_filesPath[i]);
		}

		if (m_outputType == GDT_Unknown)
			m_outputType = GDT_Int16;


		return msg;
	}

	ERMsg CLandTrendOption::ProcessOption(int& i, int argc, char* argv[])
	{
		ERMsg msg;

		if (IsEqual(argv[i], "-MaxSegments"))
		{
			m_max_segments = atoi(argv[++i]);
		}
		else if (IsEqual(argv[i], "-SpikeThreshold"))
		{
			m_desawtooth_val = atof(argv[++i]);
		}
		else if (IsEqual(argv[i], "-VertexCountOvershoot"))
		{
			m_vertexcountovershoot = atoi(argv[++i]);
		}
		else if (IsEqual(argv[i], "-RecoveryThreshold"))
		{
			m_recovery_threshold = atof(argv[++i]);
		}
		else if (IsEqual(argv[i], "-pValThreshold"))
		{
			m_pval = atof(argv[++i]);
		}
		else if (IsEqual(argv[i], "-BestModelProportion"))
		{
			m_bestmodelproportion = atof(argv[++i]);
		}
		else if (IsEqual(argv[i], "-MinObservationsNeeded"))
		{
			m_minneeded = atoi(argv[++i]);
		}
		else if (IsEqual(argv[i], "-FitMethod"))
		{
			m_fit_method = atoi(argv[++i]);
			if (m_fit_method > NB_FIT_METHODS)
				msg.ajoute(to_string(m_fit_method) + " is an invalid FitMethod.");
		}
		else if (IsEqual(argv[i], "-Modifier"))
		{
			m_modifier = atoi(argv[++i]);
			if (m_modifier != -1 || m_modifier != 1)
				msg.ajoute(to_string(m_modifier) + " is an invalid Modifier.");
		}
		else if (IsEqual(argv[i], "-FirstYear"))
		{
			m_firstYear = atoi(argv[++i]);
		}
		else if (IsEqual(argv[i], "-Indice"))
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
			m_rings = atof(argv[++i]);
			if (m_rings < 1)
			{
				msg.ajoute(to_string(m_rings) + " is not a valid window radius. Radius must be >= 1.");
			}

			m_rings -= 1;//convert radius to rings

		}
		else if (IsEqual(argv[i], "-CloudsMask"))
		{
			m_CloudsMask = argv[++i];
		}
		else if (IsEqual(argv[i], "-Breaks"))
		{
			m_bBreaks = true;
		}
		else if (IsEqual(argv[i], "-BackwardFill"))
		{
			m_bBackwardFill = true;
		}
		else if (IsEqual(argv[i], "-ForwardFill"))
		{
			m_bForwardFill = true;
		}
		else
		{
			//Look to see if it's a know base option
			msg = CBaseOptions::ProcessOption(i, argc, argv);
		}

		return msg;
	}


	ERMsg CLandTrend::Execute()
	{
		ERMsg msg;




		if (!m_options.m_bQuiet)
		{
			cout << "Output: " << m_options.m_filesPath[CLandTrendOption::OUTPUT_FILE_PATH] << endl;
			cout << "From:   " << m_options.m_filesPath[CLandTrendOption::INPUT_FILE_PATH] << endl;
			cout << "max_segments:   " << m_options.m_max_segments << endl;

			if (!m_options.m_maskName.empty())
				cout << "Mask:   " << m_options.m_maskName << endl;

		}

		//MODIFIER[] = {}
		m_options.m_modifier = -1;
		if (m_options.m_indice == Landsat2::I_B4)
			m_options.m_modifier = 1;


		GDALAllRegister();

		CLandsatDataset inputDS;
		CGDALDatasetEx maskDS;
		CLandsatDataset outputDS;
		CGDALDatasetEx cloudsDS;
		CGDALDatasetEx breaksDS;

		msg = OpenAll(inputDS, maskDS, cloudsDS, outputDS, breaksDS);
		if (!msg)
			return msg;

		if (!msg)
			return msg;

		if (!m_options.m_bQuiet && m_options.m_bCreateImage)
			cout << "Create output images (" << outputDS.GetRasterXSize() << " C x " << outputDS.GetRasterYSize() << " R x " << outputDS.GetRasterCount() << " B) with " << m_options.m_CPU << " threads..." << endl;

		CGeoExtents extents = m_options.m_extents;
		m_options.ResetBar((size_t)extents.m_xSize * extents.m_ySize);

		vector<pair<int, int>> XYindex = extents.GetBlockList();
		map<int, bool> treadNo;

		omp_set_nested(1);//for IOCPU
#pragma omp parallel for schedule(static, 1) num_threads( NB_THREAD_PROCESS ) if (m_options.m_bMulti)
		for (int b = 0; b < (int)XYindex.size(); b++)
		{
			// int thread = omp_get_thread_num();
	//#pragma omp atomic treadNo[thread] = true;


			int xBlock = XYindex[b].first;
			int yBlock = XYindex[b].second;

			Landsat2::CLandsatWindow inputData;
			OutputData outputData;
			BreaksData breaksData;
			ReadBlock(inputDS, cloudsDS, xBlock, yBlock, inputData);
			ProcessBlock(xBlock, yBlock, inputData, outputData, breaksData);
			WriteBlock(xBlock, yBlock, outputDS, breaksDS, outputData, breaksData);
		}//for all blocks

		CloseAll(inputDS, maskDS, outputDS, breaksDS);

		return msg;
	}



	ERMsg CLandTrend::OpenAll(CLandsatDataset& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& cloudsDS, CLandsatDataset& outputDS, CGDALDatasetEx& breaksDS)
	{
		ERMsg msg;

		if (!m_options.m_bQuiet)
			cout << endl << "Open input image..." << endl;

		msg = inputDS.OpenInputImage(m_options.m_filesPath[CLandTrendOption::INPUT_FILE_PATH], m_options);

		if (msg)
		{
			inputDS.UpdateOption(m_options);

			if (!m_options.m_bQuiet)
			{
				CGeoExtents extents = inputDS.GetExtents();
				//            CProjectionPtr pPrj = inputDS.GetPrj();
				//            string prjName = pPrj ? pPrj->GetName() : "Unknown";

				cout << "    Size           = " << inputDS->GetRasterXSize() << " cols x " << inputDS->GetRasterYSize() << " rows x " << inputDS.GetRasterCount() << " bands" << endl;
				cout << "    Extents        = X:{" << ToString(extents.m_xMin) << ", " << ToString(extents.m_xMax) << "}  Y:{" << ToString(extents.m_yMin) << ", " << ToString(extents.m_yMax) << "}" << endl;
				//          cout << "    Projection     = " << prjName << endl;
				cout << "    NbBands        = " << inputDS.GetRasterCount() << endl;
				cout << "    Scene size     = " << inputDS.GetSceneSize() << endl;
				cout << "    Nb. Scenes     = " << inputDS.GetNbScenes() << endl;

				if (inputDS.GetRasterCount() < 2)
					msg.ajoute("LandTrend need at least 2 bands");
			}
		}


		if (msg && !m_options.m_maskName.empty())
		{
			if (!m_options.m_bQuiet)
				cout << "Open mask image..." << endl;
			msg += maskDS.OpenInputImage(m_options.m_maskName);
		}

		if (msg && !m_options.m_CloudsMask.empty())
		{
			if (!m_options.m_bQuiet)
				cout << "Open clouds image..." << endl;
			msg += cloudsDS.OpenInputImage(m_options.m_CloudsMask);
			if (msg)
			{
				cout << "clouds Image Size      = " << cloudsDS->GetRasterXSize() << " cols x " << cloudsDS->GetRasterYSize() << " rows x " << cloudsDS.GetRasterCount() << " bands" << endl;

				if (cloudsDS.GetRasterCount() != inputDS.GetNbScenes())
					msg.ajoute("Invalid clouds image. Number of bands in clouds image (+" + to_string(cloudsDS.GetRasterCount()) + ") is not equal the number of scenes (" + to_string(inputDS.GetNbScenes()) + ") of the input image.");

				if (cloudsDS.GetRasterXSize() != inputDS.GetRasterXSize() ||
					cloudsDS.GetRasterYSize() != inputDS.GetRasterYSize())
					msg.ajoute("Invalid clouds image. Image size must have the same size (x and y) than the input image.");
			}
		}

		if (msg && m_options.m_bCreateImage)
		{
			size_t nb_scene = m_options.m_scene_extents[1] - m_options.m_scene_extents[0] + 1;
			CLandTrendOption options(m_options);
			options.m_nbBands = nb_scene * options.GetSceneSize();

			if (!m_options.m_bQuiet)
			{
				cout << endl;
				cout << "Open output images..." << endl;
				cout << "    Size           = " << options.m_extents.m_xSize << " cols x " << options.m_extents.m_ySize << " rows x " << options.m_nbBands << " bands" << endl;
				cout << "    Extents        = X:{" << ToString(options.m_extents.m_xMin) << ", " << ToString(options.m_extents.m_xMax) << "}  Y:{" << ToString(options.m_extents.m_yMin) << ", " << ToString(options.m_extents.m_yMax) << "}" << endl;
				cout << "    NbBands        = " << options.m_nbBands << endl;
				cout << "    Scene size     = " << options.GetSceneSize() << endl;
				cout << "    Nb. Scenes     = " << nb_scene << endl;

			}


			string filePath = options.m_filesPath[CLandTrendOption::OUTPUT_FILE_PATH];

			//replace the common part by the new name
			for (size_t zz = 0; zz < nb_scene; zz++)
			{
				size_t z = m_options.m_scene_extents[0] + zz;
				for (size_t s = 0; s < inputDS.GetSceneSize(); s++)
				{
					string subName = inputDS.GetSubname(z, s);
					options.m_VRTBandsName += GetFileTitle(filePath) + "_" + subName + ".tif|";
				}
			}



			msg += outputDS.CreateImage(filePath, options);
		}

		if (msg && m_options.m_bBreaks)
		{
			CLandTrendOption options(m_options);
			options.m_nbBands = options.max_vertices() * 2 + 1;

			if (!m_options.m_bQuiet)
			{
				cout << "Open breaks images..." << endl;
				cout << "    NbBands        = " << options.m_nbBands << endl;
			}

			string filePath = options.m_filesPath[CLandTrendOption::OUTPUT_FILE_PATH];
			SetFileTitle(filePath, GetFileTitle(filePath) + "_Breaks");
			options.m_VRTBandsName = GetFileTitle(filePath) + "_nbVert.tif|";
			for (size_t s = 0; s < options.max_vertices(); s++)
			{
				options.m_VRTBandsName += GetFileTitle(filePath) + FormatA("_Vert%02d.tif|", s + 1);
				options.m_VRTBandsName += GetFileTitle(filePath) + FormatA("_Fit%02d.tif|", s + 1);
			}

			msg += breaksDS.CreateImage(filePath, options);
		}

		return msg;
	}

	void CLandTrend::ReadBlock(Landsat2::CLandsatDataset& inputDS, CGDALDatasetEx& cloudsDS, int xBlock, int yBlock, Landsat2::CLandsatWindow& block_data)
	{
#pragma omp critical(BlockIO)
		{
			m_options.m_timerRead.start();

			CGeoExtents extents = m_options.m_extents.GetBlockExtents(xBlock, yBlock);
			inputDS.ReadBlock(extents, block_data, int(ceil(m_options.m_rings)), m_options.m_IOCPU, m_options.m_scene_extents[0], m_options.m_scene_extents[1]);

			if (cloudsDS.IsOpen())
			{
				assert(cloudsDS.GetRasterCount() == inputDS.GetNbScenes());
				assert(cloudsDS.GetRasterXSize() * cloudsDS.GetRasterYSize() == inputDS.GetRasterXSize() * inputDS.GetRasterYSize());


				CRasterWindow clouds_block;
				cloudsDS.ReadBlock(extents, clouds_block, int(ceil(m_options.m_rings)), m_options.m_IOCPU, m_options.m_scene_extents[0], m_options.m_scene_extents[1]);
				assert(block_data.size() == clouds_block.size());
				DataType noData = (DataType)cloudsDS.GetNoData(0);
				
				for (size_t i = 0; i < clouds_block.size(); i++)
				{
					assert(block_data[i].data().size() == clouds_block[i].data().size());

					boost::dynamic_bitset<> validity(clouds_block[i].data().size(), true);
					
					for (size_t xy = 0; xy < clouds_block[i].data().size(); xy++)
						validity.set(xy, clouds_block[i].data()[xy] == 0 || clouds_block[i].data()[xy] == noData);

					//set this validity for all scene bands
					assert(validity.size() == block_data[i].data().size());
					for (size_t ii = 0; ii < block_data.GetSceneSize(); ii++)
						block_data.at(i* block_data.GetSceneSize()+ii).SetValidity(validity);
				}
			}

			m_options.m_timerRead.stop();
		}
	}


	bool IsB1Trigged(const std::array <Landsat2::CLandsatPixel, 3>& p, int32_t threshold = -125)
	{
		size_t c0 = p[0].IsInit() ? 0 : 1;
		size_t c2 = p[2].IsInit() ? 2 : 1;

		if (!p[1].IsInit())
			return false;

		if (!p[c0].IsInit() && !p[c2].IsInit())//&& !p[3].IsInit()
			return false;

		bool t1 = p[c0].IsInit() ? ((int32_t)p[c0][Landsat2::B1] - p[1][Landsat2::B1] < threshold) : true;
		bool t2 = p[c2].IsInit() ? ((int32_t)p[c2][Landsat2::B1] - p[1][Landsat2::B1] < threshold) : true;


		return (t1 && t2);
	}




	bool IsTCBTrigged(const std::array <Landsat2::CLandsatPixel, 3>& p, int32_t threshold = 750)
	{
		size_t c0 = p[0].IsInit() ? 0 : 1;
		size_t c2 = p[2].IsInit() ? 2 : 1;

		if (!p[1].IsInit())
			return false;

		if (!p[c0].IsInit() && !p[c2].IsInit())//&& !p[3].IsInit()
			return false;

		bool t1 = p[c0].IsInit() ? ((int32_t)p[c0][Landsat2::I_TCB] - p[1][Landsat2::I_TCB] > threshold) : true;
		bool t2 = p[c2].IsInit() ? ((int32_t)p[c2][Landsat2::I_TCB] - p[1][Landsat2::I_TCB] > threshold) : true;

		return (t1 && t2);
	}

	void CLandTrend::ProcessBlock(int xBlock, int yBlock, const Landsat2::CLandsatWindow& window, OutputData& outputData, BreaksData& breaksData)
	{
		CGeoExtents extents = m_options.GetExtents();
		CGeoSize blockSize = extents.GetBlockSize(xBlock, yBlock);

		if (window.empty())
		{
			int nbCells = blockSize.m_x * blockSize.m_y;

#pragma omp atomic
			m_options.m_xx += nbCells;
			m_options.UpdateBar();

			return;
		}



		//init memory
		if (m_options.m_bCreateImage)
		{
			outputData.resize(window.GetNbBands());
			for (size_t s = 0; s < outputData.size(); s++)
				outputData[s].insert(outputData[s].begin(), blockSize.m_x * blockSize.m_y, DataType(m_options.m_dstNodata));
		}
		if (m_options.m_bBreaks)
		{
			breaksData.resize(m_options.max_vertices() * 2 + 1);
			for (size_t s = 0; s < breaksData.size(); s++)
				breaksData[s].insert(breaksData[s].begin(), blockSize.m_x * blockSize.m_y, DataType(m_options.m_dstNodata));
		}

#pragma omp critical(ProcessBlock)
		{
			m_options.m_timerProcess.start();

			int16_t no_data = int16_t(GetDefaultNoData(GDT_Int16));

#pragma omp parallel for num_threads( m_options.m_CPU ) if (m_options.m_bMulti )
			for (int y = 0; y < blockSize.m_y; y++)
			{
				for (int x = 0; x < blockSize.m_x; x++)
				{
					int xy = y * blockSize.m_x + x;

					//Get pixel
					CRealArray years = ::convert(allpos(window.size()));
					CRealArray data(window.size());
					CBoolArray goods(window.size());

					bool bBackwardFill = m_options.m_bBackwardFill;
					bool bForwardFill = m_options.m_bForwardFill;
					size_t m_first_valid = NOT_INIT;
					size_t m_last_valid = NOT_INIT;
					if (m_options.m_bBackwardFill || m_options.m_bForwardFill)
					{
						for (size_t z = 0; z < window.size(); z++)
						{
							bool bValid = window.IsValid(z, x, y);
							if (m_options.m_bBackwardFill && bValid && m_first_valid == NOT_INIT)
								m_first_valid = z;

							if (m_options.m_bForwardFill && bValid)
								m_last_valid = z;
						}
					}

					//assert(validity.size() == window.size());
					for (size_t z = 0; z < window.size(); z++)
					{
						size_t zz = z;
						
						if (m_first_valid != NOT_INIT && zz < m_first_valid)
							zz = m_first_valid;
						if (m_last_valid != NOT_INIT && zz > m_last_valid)
							zz = m_last_valid;
						

						CLandsatPixel pixel = window.GetPixel(zz, x, y);
						goods[z] = pixel.IsValid();//&& validity[zz][xy]

						if (goods[z])
						{

							data[z] = window.GetPixelIndice(zz, m_options.m_indice, x, y, m_options.m_rings);
							//goods[z] = data[z] != no_data;
						}

					}


					if (goods.max())//at least one valid pixel
					{
						//compute LandTrend for this time series indice
						CBestModelInfo result = fit_trajectory_v2(years, data, goods,
							m_options.m_minneeded, int(m_options.m_srcNodata), m_options.m_modifier, m_options.m_desawtooth_val, m_options.m_pval,
							m_options.m_max_segments, m_options.m_recovery_threshold, 2,
							m_options.m_vertexcountovershoot, m_options.m_bestmodelproportion, TFitMethod(m_options.m_fit_method));



						//if need output
						if (result.ok)
						{
							if (!outputData.empty())
							{
								//create output image doing a regression for each band by segment
								for (size_t s = 0; s < SCENES_SIZE; s++)//for all bands
								{

									CVectices V = result.vertices;
									CRealArray X(window.size());
									CRealArray Y(window.size());
									for (size_t z = 0; z < window.size(); z++)
									{
										X[z] = REAL_TYPE(z);
										Y[z] = window.GetPixel(z, x, y)[s];
									}

									CRealArray yfit(Y.size());
									for (size_t i = 0; i < V.size() - 1; i++)//for all segmentx
									{
										//we need to remove bad data from vertices
										CBoolArray G = subset(goods, V[i], V[i + 1]);

										CRealArray xx = subset(X, V[i], V[i + 1])[G];
										CRealArray yy = subset(Y, V[i], V[i + 1])[G];
										assert(xx.size() == yy.size());
										assert(xx.size() > 0);
										xx = CRealArray(xx[yy != no_data]);
										yy = CRealArray(yy[yy != no_data]);
										if (xx.size() >= 2)
										{
											//if we've done desawtooth, it's possible that all of the
											//  values in a segment have same value, in which case regress
											//  would choke, so deal with that.

											RegressP P = Regress(xx, yy);

											yfit[get_slice(V[i], V[i + 1])] = FitRegress(subset(X, V[i], V[i + 1]), P);
										}
										else if (xx.size() == 1)
										{
											//if only one point, take this yfit value
											yfit[get_slice(V[i], V[i + 1])] = yy[0];
										}

									}

									for (size_t z = 0; z < window.size(); z++)
									{
										outputData[z * SCENES_SIZE + s][xy] = DataType(yfit[z]);
									}
								}
							}

							//if output breaks
							if (!breaksData.empty())
							{
								breaksData[0][xy] = (LandsatDataType)result.vertvals.size();
								for (size_t s = result.vertvals.size() - 1; s < result.vertvals.size(); s--)
								{
									breaksData[s * 2 + 1][xy] = (LandsatDataType)(m_options.m_firstYear + result.vertices[s]);
									breaksData[s * 2 + 2][xy] = (LandsatDataType)result.vertvals[s];
								}
							}
						}
					}//if al leat one valid pixel

#pragma omp atomic
					m_options.m_xx++;

				}//for x
				m_options.UpdateBar();
			}//for y

			m_options.m_timerProcess.stop();

		}//critical process
	}


	void CLandTrend::WriteBlock(int xBlock, int yBlock, CGDALDatasetEx& outputDS, CGDALDatasetEx& breaksDS, OutputData& outputData, BreaksData& breaksData)
	{
#pragma omp critical(BlockIO)
		{
			m_options.m_timerWrite.start();

			if (outputDS.IsOpen())
			{
				CGeoExtents extents = outputDS.GetExtents();
				CGeoRectIndex outputRect = extents.GetBlockRect(xBlock, yBlock);

				assert(outputRect.m_x >= 0 && outputRect.m_x < outputDS.GetRasterXSize());
				assert(outputRect.m_y >= 0 && outputRect.m_y < outputDS.GetRasterYSize());
				assert(outputRect.m_xSize > 0 && outputRect.m_xSize <= outputDS.GetRasterXSize());
				assert(outputRect.m_ySize > 0 && outputRect.m_ySize <= outputDS.GetRasterYSize());

				for (size_t z = 0; z < outputData.size(); z++)
				{
					GDALRasterBand* pBand = outputDS.GetRasterBand(z);
					if (!outputData.empty())
					{
						assert(outputData.size() == outputDS.GetRasterCount());
						pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(outputData[z][0]), outputRect.Width(), outputRect.Height(), GDT_Int16, 0, 0);
					}
					else
					{
						LandsatDataType noData = (LandsatDataType)outputDS.GetNoData(z);
						pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &noData, 1, 1, GDT_Int16, 0, 0);
					}
				}
			}

			if (breaksDS.IsOpen())
			{
				CGeoExtents extents = breaksDS.GetExtents();
				CGeoRectIndex breaksRect = extents.GetBlockRect(xBlock, yBlock);

				assert(breaksRect.m_x >= 0 && breaksRect.m_x < breaksDS.GetRasterXSize());
				assert(breaksRect.m_y >= 0 && breaksRect.m_y < breaksDS.GetRasterYSize());
				assert(breaksRect.m_xSize > 0 && breaksRect.m_xSize <= breaksDS.GetRasterXSize());
				assert(breaksRect.m_ySize > 0 && breaksRect.m_ySize <= breaksDS.GetRasterYSize());

				for (size_t z = 0; z < breaksData.size(); z++)
				{
					GDALRasterBand* pBand = breaksDS.GetRasterBand(z);
					if (!breaksData.empty())
					{
						assert(breaksData.size() == breaksDS.GetRasterCount());

						for (int y = 0; y < breaksRect.Height(); y++)
							pBand->RasterIO(GF_Write, breaksRect.m_x, breaksRect.m_y, breaksRect.Width(), breaksRect.Height(), &(breaksData[z][0]), breaksRect.Width(), breaksRect.Height(), GDT_Int16, 0, 0);
					}
					else
					{
						LandsatDataType noData = (LandsatDataType)breaksDS.GetNoData(z);
						pBand->RasterIO(GF_Write, breaksRect.m_x, breaksRect.m_y, breaksRect.Width(), breaksRect.Height(), &noData, 1, 1, GDT_Int16, 0, 0);
					}
				}
			}


			m_options.m_timerWrite.stop();
		}
	}

	void CLandTrend::CloseAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS, CGDALDatasetEx& breaksDS)
	{
		inputDS.Close();
		maskDS.Close();

		m_options.m_timerWrite.start();

		outputDS.Close(m_options);
		breaksDS.Close(m_options);


		m_options.m_timerWrite.stop();

	}

}
