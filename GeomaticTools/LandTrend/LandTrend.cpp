//***********************************************************************
// program to merge Landsat image image over a period
//									 
//***********************************************************************
// version
// 1.0.0	29/08/2023	Rémi Saint-Amant	Creation from IDL code


//"E:\Landsat\Landsat(2000-2018)\Input\Landsat_2000-2018(2).vrt" "E:\Landsat\Landsat(2000-2018)\Output\test2.vrt" -of VRT -overwrite -co "COMPRESS=LZW"   -te 1022538.9 6663106.0 1040929.5 6676670.7 -multi -Debug -SpikeThreshold 0.75 -FitMethod 1


#include "LandTrend.h"
#include <math.h>
#include <array>
#include <utility>
#include <iostream>


#include "LandTrendUtil.h"
#include "Basic/OpenMP.h"
#include "Basic/UtilTime.h"
#include "Basic/UtilMath.h"

#pragma warning(disable: 4275 4251)
#include "gdal_priv.h"


#include "LandTrendCore.h"


using namespace std;
using namespace WBSF::Landsat2;

namespace WBSF
{
	const char* CLandTrend::VERSION = "1.0.0";
	const size_t CLandTrend::NB_THREAD_PROCESS = 2;


	//*********************************************************************************************************************

	CLandTrendOption::CLandTrendOption()
	{

		m_pval =  0.05;
		m_recovery_threshold = 0.25;
		m_distweightfactor = 2;
		m_vertexcountovershoot = 3;
		m_bestmodelproportion = 0.75;
		m_minneeded = 6;
		m_max_segments = 6;
		m_desawtooth_val = 0.9;
		m_fit_method = FIT_EARLY_TO_LATE;
		m_modifier = -1;

		m_scenesSize = SCENES_SIZE;
		m_indice = I_NBR;
		m_rings = 0.5;

		m_firstYear = 0;
		m_bDebug = false;
		m_appDescription = "This software standardize Landsat images  (composed of " + to_string(SCENES_SIZE) + " bands) based on LandTrendR analysis.";


		AddOption("-RGB");
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
			{ "-Modifier", 1, "modifier", false, "Modifier to assure that disturbance is always positive. Can be 1 or -1. -1 by default for NBR."},
			{ "-Indice", 1, "indice", false, "Select indice to run model. Indice can be NBR, NDVI, NDMI, NDWI, TCB, TCG, TCW, NBR2, EVI, EVI2, SAVI, MSAVI, SR, CL, HZ, LSWI, VIgreen. NBR by default"  },
			{ "-Windows", 1, "ring", false, "Compute window mean around the pixel. n is the number of rings. 0 = 1x1, 1 = 3x3, 2 = 5x5 etc. But can also be a float to get the mean between 2 rings. For example 0.25 will be compute as follow: 0.75*(1x1) + 0.25*(3x3). 0.5 by default." },
			{ "-ValidityMask", 1, "name", false, "Mask of valid data. Number of validity bands must be the same as the number of scenes (years)." },
			{ "-FirstYear", 1, "year", false, "Specify year of the first image. Return year instead of index. By default, return the image index (0..nbImages-1)" },
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
			{ "Input Image", "srcfile", "", "nbYears", "B1: Landsat band 1|B2: Landsat band 2|B3: Landsat band 3|B4: Landsat band 4|B5: Landsat band 5|B7: Landsat band 7|... for all scenes", "" },
			{ "Output Image", "dstfile", "", "nbYears", "Same as input", "" },
			{ "Optional Output Image", "dstfile_debug","1","NbOutputLayers=(MaxSegments+1)*2+1","Nb vertices: number of vertices found|vert1: vertice1. Year if FirstYear is define|fit1: fit of vertice1|... for all vertices"}
		};

		for (int i = 0; i < sizeof(IO_FILE_INFO) / sizeof(CIOFileInfoDef); i++)
			AddIOFileInfo(IO_FILE_INFO[i]);
	}

	ERMsg CLandTrendOption::ParseOption(int argc, char* argv[])
	{
		ERMsg msg = CBaseOptions::ParseOption(argc, argv);

		ASSERT(NB_FILE_PATH == 2);
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
		}
		else if (IsEqual(argv[i], "-ValidityMask"))
		{
			m_ValidityMask = argv[++i];
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

		GDALAllRegister();

		CLandsatDataset inputDS;
		CGDALDatasetEx maskDS;
		CLandsatDataset outputDS;
		CGDALDatasetEx validityDS;
		CGDALDatasetEx debugDS;

		msg = OpenAll(inputDS, maskDS, validityDS, outputDS, debugDS);
		if (!msg)
			return msg;



		CBandsHolderMT bandHolder(3, m_options.m_memoryLimit, m_options.m_IOCPU, NB_THREAD_PROCESS);

		if (maskDS.IsOpen())
			bandHolder.SetMask(maskDS.GetSingleBandHolder(), m_options.m_maskDataUsed);

		msg += bandHolder.Load(inputDS, m_options.m_bQuiet, m_options.m_extents, m_options.m_period);

		if (!msg)
			return msg;

		if (!m_options.m_bQuiet && m_options.m_bCreateImage)
			cout << "Create output images (" << outputDS.GetRasterXSize() << " C x " << outputDS.GetRasterYSize() << " R x " << outputDS.GetRasterCount() << " B) with " << m_options.m_CPU << " threads..." << endl;


		//load validity mask;
		deque<boost::dynamic_bitset<>> validity(inputDS.GetNbScenes());


		for (size_t i = 0; i < validity.size(); i++)
		{
			validity[i].resize(inputDS.GetRasterXSize() * inputDS.GetRasterYSize(), true);

			if (validityDS.IsOpen())
			{
				ASSERT(validityDS.GetRasterCount() == inputDS.GetRasterCount());
				ASSERT(validityDS.GetRasterXSize() * validityDS.GetRasterYSize() == inputDS.GetRasterXSize() * inputDS.GetRasterYSize());
				vector<char> tmp(validityDS.GetRasterXSize() * validityDS.GetRasterYSize());
				GDALRasterBand* pBand = validityDS.GetRasterBand(i);
				pBand->RasterIO(GF_Read, 0, 0, validityDS.GetRasterXSize(), validityDS.GetRasterYSize(), &(tmp[0]), validityDS.GetRasterXSize(), validityDS.GetRasterYSize(), GDT_Byte, NULL, NULL);

				ASSERT(validity[i].size() == tmp.size());
				for (size_t xy = 0; xy < tmp.size(); xy++)
					validity[i].set(xy, tmp[xy] != 0);
			}
		}



		CGeoExtents extents = bandHolder.GetExtents();
		m_options.ResetBar((size_t)extents.m_xSize * extents.m_ySize);

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
			ProcessBlock(xBlock, yBlock, bandHolder[thread], validity, outputData, debugData);
			WriteBlock(xBlock, yBlock, outputDS, debugDS, outputData, debugData);
		}//for all blocks

		CloseAll(inputDS, maskDS, outputDS, debugDS);

		return msg;
	}



	ERMsg CLandTrend::OpenAll(CLandsatDataset& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& validityDS, CLandsatDataset& outputDS, CGDALDatasetEx& debugDS)
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
				CProjectionPtr pPrj = inputDS.GetPrj();
				string prjName = pPrj ? pPrj->GetName() : "Unknown";

				cout << "    Size           = " << inputDS->GetRasterXSize() << " cols x " << inputDS->GetRasterYSize() << " rows x " << inputDS.GetRasterCount() << " bands" << endl;
				cout << "    Extents        = X:{" << ToString(extents.m_xMin) << ", " << ToString(extents.m_xMax) << "}  Y:{" << ToString(extents.m_yMin) << ", " << ToString(extents.m_yMax) << "}" << endl;
				cout << "    Projection     = " << prjName << endl;
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

		if (msg && !m_options.m_ValidityMask.empty())
		{
			if (!m_options.m_bQuiet)
				cout << "Open validity image..." << endl;
			msg += validityDS.OpenInputImage(m_options.m_ValidityMask);
			if (msg)
			{
				cout << "Validity Size      = " << validityDS->GetRasterXSize() << " cols x " << validityDS->GetRasterYSize() << " rows x " << validityDS.GetRasterCount() << " bands" << endl;

				if (validityDS.GetRasterCount() != inputDS.GetNbScenes())
					msg.ajoute("Invalid validity image. Number of bands in validity (+" + to_string(validityDS.GetRasterCount()) + ") is not equal the number of scenes (" + to_string(inputDS.GetNbScenes()) + ")of the input image.");

				if (validityDS.GetRasterXSize() != inputDS.GetRasterXSize() ||
					validityDS.GetRasterYSize() != inputDS.GetRasterYSize())
					msg.ajoute("Invalid validity image. Image size must have the same size (x and y) than the input image.");
			}
		}

		if (m_options.m_bCreateImage)
		{
			CLandTrendOption options(m_options);
			//options.m_nbBands = options.max_vertices();

			if (!m_options.m_bQuiet)
			{
				cout << endl;
				cout << "Open output images..." << endl;
				cout << "    Size           = " << options.m_extents.m_xSize << " cols x " << options.m_extents.m_ySize << " rows x " << options.m_nbBands << " bands" << endl;
				cout << "    Extents        = X:{" << ToString(options.m_extents.m_xMin) << ", " << ToString(options.m_extents.m_xMax) << "}  Y:{" << ToString(options.m_extents.m_yMin) << ", " << ToString(options.m_extents.m_yMax) << "}" << endl;
			}


			string filePath = options.m_filesPath[CLandTrendOption::OUTPUT_FILE_PATH];
			
			//replace the common part by the new name
			for (size_t z = 0; z < inputDS.GetNbScenes(); z++)
			{
				for (size_t s = 0; s < inputDS.GetSceneSize(); s++)
				{
					string subName = inputDS.GetSubname(z, s);
					options.m_VRTBandsName += GetFileTitle(filePath) + "_" + subName + ".tif|";
				}
			}



			msg += outputDS.CreateImage(filePath, options);
		}

		if (m_options.m_bDebug)
		{
			CLandTrendOption options(m_options);
			options.m_nbBands = options.max_vertices() * 2 + 1;

			if (!m_options.m_bQuiet)
				cout << "Open debug images..." << endl;

			string filePath = options.m_filesPath[CLandTrendOption::OUTPUT_FILE_PATH];
			SetFileTitle(filePath, GetFileTitle(filePath) + "_debug");
			options.m_VRTBandsName = GetFileTitle(filePath) + "_nbVert.tif|";
			for (size_t s = 0; s < options.max_vertices(); s++)
			{
				options.m_VRTBandsName += GetFileTitle(filePath) + FormatA("_Vert%02d.tif|", s + 1);
				options.m_VRTBandsName += GetFileTitle(filePath) + FormatA("_Fit%02d.tif|", s + 1);
			}

			msg += debugDS.CreateImage(filePath, options);
		}

		return msg;
	}

	void CLandTrend::ReadBlock(int xBlock, int yBlock, CBandsHolder& bandHolder)
	{
#pragma omp critical(BlockIO)
		{
			m_options.m_timerRead.Start();

			CGeoExtents extents = m_options.m_extents.GetBlockExtents(xBlock, yBlock);
			bandHolder.LoadBlock(xBlock, yBlock);


			m_options.m_timerRead.Stop();
		}
	}


	bool IsB1Trigged(const std::array <Landsat2::CLandsatPixel, 3>& p, __int32 threshold=-125)
	{
		size_t c0 = p[0].IsInit() ? 0 : 1;
		size_t c2 = p[2].IsInit() ? 2 : 1;

		if (!p[1].IsInit())
			return false;

		if (!p[c0].IsInit() && !p[c2].IsInit())//&& !p[3].IsInit()
			return false;

		bool t1 = p[c0].IsInit() ? ((__int32)p[c0][Landsat2::B1] - p[1][Landsat2::B1] < threshold) : true;
		bool t2 = p[c2].IsInit() ? ((__int32)p[c2][Landsat2::B1] - p[1][Landsat2::B1] < threshold) : true;


		return (t1 && t2);
	}




	bool IsTCBTrigged(const std::array <Landsat2::CLandsatPixel, 3>& p, __int32 threshold=750)
	{
		size_t c0 = p[0].IsInit() ? 0 : 1;
		size_t c2 = p[2].IsInit() ? 2 : 1;

		if (!p[1].IsInit())
			return false;

		if (!p[c0].IsInit() && !p[c2].IsInit())//&& !p[3].IsInit()
			return false;

		bool t1 = p[c0].IsInit() ? ((__int32)p[c0][Landsat2::I_TCB] - p[1][Landsat2::I_TCB] > threshold) : true;
		bool t2 = p[c2].IsInit() ? ((__int32)p[c2][Landsat2::I_TCB] - p[1][Landsat2::I_TCB] > threshold) : true;

		return (t1 && t2);
	}

	void CLandTrend::ProcessBlock(int xBlock, int yBlock, CBandsHolder& bandHolder, const deque<boost::dynamic_bitset<>>& validity, OutputData& outputData, DebugData& debugData)
	{
		CGeoExtents extents = bandHolder.GetExtents();
		CGeoSize blockSize = extents.GetBlockSize(xBlock, yBlock);

		if (bandHolder.IsEmpty())
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
			outputData.resize(bandHolder.size());
			for (size_t s = 0; s < outputData.size(); s++)
				outputData[s].insert(outputData[s].begin(), blockSize.m_x * blockSize.m_y, m_options.m_dstNodata);
		}
		if (m_options.m_bDebug)
		{
			debugData.resize(m_options.max_vertices() * 2 + 1);
			for (size_t s = 0; s < debugData.size(); s++)
				debugData[s].insert(debugData[s].begin(), blockSize.m_x * blockSize.m_y, m_options.m_dstNodata);
		}

#pragma omp critical(ProcessBlock)
		{
			//CLandsatWindow window = static_cast<CLandsatWindow&>(bandHolder.GetWindow());
			CLandsatWindow window = static_cast<CLandsatWindow&>(bandHolder.GetWindow());

			m_options.m_timerProcess.Start();

#pragma omp parallel for num_threads( m_options.m_CPU ) if (m_options.m_bMulti ) 
			for (int y = 0; y < blockSize.m_y; y++)
			{
				for (int x = 0; x < blockSize.m_x; x++)
				{
					int xy = y * blockSize.m_x + x;

					//Get pixel
					CRealArray years = ::convert(allpos(window.size()));
					CRealArray data(window.size());
					std::valarray<bool> goods(window.size());

					ASSERT(validity.size() == window.size());

					for (size_t z = 0; z < window.size(); z++)
					{
						CLandsatPixel pixel = window.GetPixel(z, x, y);

						//if (pixel.IsValid())
						//{
						//	
						//	
						//	
						//	if (z>0 && z < window.size() - 1)//if not at the begin or the end
						//	{
						//		
						//		size_t previous_pos = window.GetPrevious(z, x, y);
						//		size_t next_pos = window.GetNext(z, x, y);

						//		CLandsatPixel previous;
						//		CLandsatPixel next;

						//		if (previous_pos != -1)
						//			previous = window.GetPixel(previous_pos, x, y);
						//		if (next_pos != -1)
						//			next = window.GetPixel(next_pos, x, y);

						//		std::array <Landsat2::CLandsatPixel, 3> p = { previous, pixel, next };
						//		bool bCloud = IsB1Trigged(p, -175);
						//		bool bShadow = IsTCBTrigged(p, 600);

						//		if (bCloud || bShadow)
						//			pixel.Reset();
						//	}
						//	 
						//	

						//	
						//}
						 
						goods[z] = pixel.IsValid() && validity[z][xy];
						if(goods[z])
							data[z] = window.GetPixelIndice(z, m_options.m_indice, x, y, m_options.m_rings);
					}

					
					//compute LandTrend for this time series indice
					CBestModelInfo result = fit_trajectory_v2(years, data, goods,
						m_options.m_minneeded, m_options.m_srcNodata, m_options.m_modifier, m_options.m_desawtooth_val, m_options.m_pval,
						m_options.m_max_segments, m_options.m_recovery_threshold, 2,
						m_options.m_vertexcountovershoot, m_options.m_bestmodelproportion, TFitMethod(m_options.m_fit_method));



					//if need output
					if (!outputData.empty() && result.ok)
					{
						//create output image doing a regression for each band by segment
						for (size_t s = 0; s < SCENES_SIZE; s++)//foir all bands
						{
							
							CVectices V = result.vertices;
							CRealArray X(window.size());
							CRealArray Y(window.size());
							for (size_t z = 0; z < window.size(); z++)
							{
								X[z] = z;
								Y[z] = window.GetPixel(z, x, y)[s];
							}

							CRealArray yfit(Y.size());
							for (size_t i = 0; i < V.size() - 1; i++)//for all segement
							{
								//we need to remove bad data from vertices
								std::valarray<bool> G = subset(goods, V[i], V[i + 1]);

								CRealArray xx = subset(X, V[i], V[i + 1])[G];
								CRealArray yy = subset(Y, V[i], V[i + 1])[G];
								ASSERT(xx.size() == yy.size());
								ASSERT(xx.size() > 0);

								if(xx.size()>=2)
								{
									//if we've done desawtooth, it's possible that all of the
									//  values in a segment have same value, in which case regress
									//  would choke, so deal with that.

									RegressP P = Regress(xx, yy);

									yfit[get_slice(V[i], V[i + 1])] = FitRegress(subset(X, V[i], V[i + 1]), P);
								}
								else
								{
									//if only one point, take this yfit value
									yfit[get_slice(V[i], V[i + 1])] = yy[0];
								}
								
							}

							for (size_t z = 0; z < window.size(); z++)
							{
								outputData[z * SCENES_SIZE + s][xy] = yfit[z];
							}
						}

					//if output debug
						if (!debugData.empty())
						{
							debugData[0][xy] = (__int16)result.vertvals.size();
							for (size_t s = result.vertvals.size()-1; s < result.vertvals.size(); s--)
							{
								debugData[s * 2 + 1][xy] = (__int16)(m_options.m_firstYear + result.vertices[s]);
								debugData[s * 2 + 2][xy] = (__int16)result.vertvals[s];
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


	void CLandTrend::WriteBlock(int xBlock, int yBlock, CGDALDatasetEx& outputDS, CGDALDatasetEx& debugDS, OutputData& outputData, DebugData& debugData)
	{
#pragma omp critical(BlockIO)
		{
			m_options.m_timerWrite.Start();

			if (outputDS.IsOpen())
			{
				CGeoExtents extents = outputDS.GetExtents();
				CGeoRectIndex outputRect = extents.GetBlockRect(xBlock, yBlock);

				ASSERT(outputRect.m_x >= 0 && outputRect.m_x < outputDS.GetRasterXSize());
				ASSERT(outputRect.m_y >= 0 && outputRect.m_y < outputDS.GetRasterYSize());
				ASSERT(outputRect.m_xSize > 0 && outputRect.m_xSize <= outputDS.GetRasterXSize());
				ASSERT(outputRect.m_ySize > 0 && outputRect.m_ySize <= outputDS.GetRasterYSize());

				for (size_t z = 0; z < outputData.size(); z++)
				{
					GDALRasterBand* pBand = outputDS.GetRasterBand(z);
					if (!outputData.empty())
					{
						ASSERT(outputData.size() == outputDS.GetRasterCount());
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
					GDALRasterBand* pBand = debugDS.GetRasterBand(z);
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

	void CLandTrend::CloseAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS, CGDALDatasetEx& debugDS)
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