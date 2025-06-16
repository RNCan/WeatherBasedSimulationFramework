//***********************************************************************
// program to merge Landsat image image over a period
//
//***********************************************************************
// version
// 1.0.0	10/06/2025	Rémi Saint-Amant	Creation from LandtrendImage code


//"D:\Travaux\Landsat\Landsat(2000-2018)\Input\Landsat_2000-2018(2).vrt" "D:\Travaux\Landsat\Landsat(2000-2018)\Output\test3.vrt" -of VRT -overwrite -co "COMPRESS=LZW"   -te 1022538.9 6663106.0 1040929.5 6676670.7 -multi -SpikeThreshold 0.75 



#include <math.h>
#include <array>
#include <utility>
#include <iostream>


#include "Desawtooth.h"
#include "basic/OpenMP.h"
#include "geomatic/GDAL.h"
#include "geomatic/LandTrendUtil.h"
#include "geomatic/LandTrendCore.h"





using namespace std;
using namespace WBSF::Landsat2;
using namespace LTR;



namespace WBSF
{
	const char* CDesawtooth::VERSION = "1.0.0";
	const size_t CDesawtooth::NB_THREAD_PROCESS = 2;


	//*********************************************************************************************************************

	CDesawtoothOption::CDesawtoothOption()
	{
		m_scenes_def = { { B1,B2,B3,B4,B5,B7 } };
		m_desawtooth_val = 0.9;
		//m_modifier = -1;
		m_indice = I_NBR;
		m_rings = 0;
		m_firstYear = 0;
		m_minneeded = 3;

		m_bBackwardFill = false;
		m_bForwardFill = false;

		m_appDescription = "This software export desawtooth of Landsat images  (composed of " + to_string(SCENES_SIZE) + " bands) indice.";
		std::string indicesName = Landsat2::GetIndiceNames();

		//AddOption("-RGB");
		static const COptionDef OPTIONS[] =
		{
			{ "-SpikeThreshold", 1, "Thres", false, "Threshold for dampening the spikes (1.0 means no dampening). 0.9 by default."},
			{ "-MinObservationsNeeded", 1, "min", false, "Min observations needed to perform output fitting. 6 by default."},
			{ "-Indice", 1, "indice", false, ("Select indice to run desawtooth. Indice can be: " + indicesName + ". NBR by default").c_str()  },
			{ "-Window", 1, "radius", false, "Compute window mean around the pixel where the radius is the number of pixels around the pixel: 1 = 1x1, 2 = 3x3, 3 = 5x5 etc. But can also be a float to get the average between 2 rings. For example 1.25 will be compute as follow: 0.75*(1x1) + 0.25*(3x3). 1 by default." },
			//{ "-Modifier", 1, "m", false, "-1 or 1 to invert indices value. -1 by default." },
			{ "-BackwardFill", 0, "", false, "Fill all missing values at the beginning of the series with the first valid value."},
			{ "-ForwardFill", 0, "", false, "Fill all missing values at the end of the series with the last valid value."},
			{ "-CloudsMask", 1, "name", false, "Mask of clouds data. Zero = no clouds, others values are invalid. Number of clouds bands must be the same as the number of scenes (years)." },
			{ "-FirstYear", 1, "year", false, "Specify year of the first image. Return year instead of index. By default, return the image index (0..nbImages-1)" },
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

	ERMsg CDesawtoothOption::ParseOption(int argc, char* argv[])
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
			m_outputType = GDT_Float32;

		if (m_dstNodata == MISSING_NO_DATA)
			m_dstNodata = WBSF::GetDefaultNoData(GDT_Int16);//use Int16 missing value



		return msg;
	}

	ERMsg CDesawtoothOption::ProcessOption(int& i, int argc, char* argv[])
	{
		ERMsg msg;

		if (IsEqual(argv[i], "-SpikeThreshold"))
		{
			m_desawtooth_val = atof(argv[++i]);
		}

		else if (IsEqual(argv[i], "-MinObservationsNeeded"))
		{
			m_minneeded = atoi(argv[++i]);
		}
		/*else if (IsEqual(argv[i], "-Modifier"))
		{
			m_modifier = atoi(argv[++i]);
			if (m_modifier != -1 || m_modifier != 1)
				msg.ajoute(to_string(m_modifier) + " is an invalid Modifier. Modifier must be 1 or -1.");
		}*/
		else if (IsEqual(argv[i], "-FirstYear"))
		{
			m_firstYear = atoi(argv[++i]);
		}
		else if (IsEqual(argv[i], "-Indice"))
		{
			string str = argv[++i];
			m_indice = GetIndiceType(str);
			if (m_indice == I_INVALID)
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


	ERMsg CDesawtooth::Execute()
	{
		ERMsg msg;




		if (!m_options.m_bQuiet)
		{
			cout << "Output: " << m_options.m_filesPath[CDesawtoothOption::OUTPUT_FILE_PATH] << endl;
			cout << "From:   " << m_options.m_filesPath[CDesawtoothOption::INPUT_FILE_PATH] << endl;

			if (!m_options.m_maskName.empty())
				cout << "Mask:   " << m_options.m_maskName << endl;

		}

		
		/*m_options.m_modifier = -1;
		if (m_options.m_indice == Landsat2::I_B4)
			m_options.m_modifier = 1;*/


		GDALAllRegister();

		CLandsatDataset inputDS;
		CGDALDatasetEx maskDS;
		CLandsatDataset outputDS;
		CGDALDatasetEx cloudsDS;

		msg = OpenAll(inputDS, maskDS, cloudsDS, outputDS);
		if (!msg)
			return msg;


		if (!m_options.m_bQuiet && m_options.m_bCreateImage)
			cout << "Create output images (" << outputDS.GetRasterXSize() << " C x " << outputDS.GetRasterYSize() << " R x " << outputDS.GetRasterCount() << " B) with " << m_options.m_CPU << " threads..." << endl;

		CGeoExtents extents = m_options.m_extents;
		m_options.ResetBar((size_t)extents.m_xSize * extents.m_ySize);

		vector<pair<int, int>> XYindex = extents.GetBlockList();
		map<int, bool> treadNo;

		omp_set_nested(1);//for IOCPU
//#pragma omp parallel for schedule(static, 1) num_threads( NB_THREAD_PROCESS ) if (m_options.m_bMulti)
		for (int b = 0; b < (int)XYindex.size(); b++)
		{
			int xBlock = XYindex[b].first;
			int yBlock = XYindex[b].second;

			Landsat2::CLandsatWindow inputData;
			OutputData outputData;

			ReadBlock(inputDS, cloudsDS, xBlock, yBlock, inputData);
			ProcessBlock(xBlock, yBlock, inputData, outputData);
			WriteBlock(xBlock, yBlock, outputDS, outputData);
		}//for all blocks

		CloseAll(inputDS, maskDS, outputDS);

		return msg;
	}



	ERMsg CDesawtooth::OpenAll(CLandsatDataset& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& cloudsDS, CLandsatDataset& outputDS)
	{
		ERMsg msg;

		if (!m_options.m_bQuiet)
			cout << endl << "Open input image..." << endl;

		msg = inputDS.OpenInputImage(m_options.m_filesPath[CDesawtoothOption::INPUT_FILE_PATH], m_options);

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
					msg.ajoute("Desawtooth need at least 2 bands");
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
			size_t nb_scenes = m_options.m_scene_extents[1] - m_options.m_scene_extents[0] + 1;
			CDesawtoothOption options(m_options);
			options.m_scenes_def.clear();
			options.m_nbBands = nb_scenes;

			if (!m_options.m_bQuiet)
			{
				cout << endl;
				cout << "Open output images..." << endl;
				cout << "    Size           = " << options.m_extents.m_xSize << " cols x " << options.m_extents.m_ySize << " rows x " << options.m_nbBands << " bands" << endl;
				cout << "    Extents        = X:{" << ToString(options.m_extents.m_xMin) << ", " << ToString(options.m_extents.m_xMax) << "}  Y:{" << ToString(options.m_extents.m_yMin) << ", " << ToString(options.m_extents.m_yMax) << "}" << endl;
				//cout << "    NbBands        = " << options.m_nbBands << endl;
				cout << "    Nb. Scenes     = " << nb_scenes << endl;
			}

			std::string indices_name = Landsat2::GetIndiceName(m_options.m_indice);
			string filePath = options.m_filesPath[CDesawtoothOption::OUTPUT_FILE_PATH];

			//replace the common part by the new name
			for (size_t zz = 0; zz < nb_scenes; zz++)
			{
				size_t z = m_options.m_scene_extents[0] + zz;
				string subName = inputDS.GetSubname(z) + "_" + indices_name;
				options.m_VRTBandsName += GetFileTitle(filePath) + "_" + subName + ".tif|";

			}

			msg += outputDS.CreateImage(filePath, options);
		}

		return msg;
	}

	void CDesawtooth::ReadBlock(Landsat2::CLandsatDataset& inputDS, CGDALDatasetEx& cloudsDS, int xBlock, int yBlock, Landsat2::CLandsatWindow& block_data)
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
				DataType cloudNoData = (DataType)cloudsDS.GetNoData(0);

				for (size_t i = 0; i < clouds_block.size(); i++)
				{
					assert(block_data[i].data().size() == clouds_block[i].data().size());

					boost::dynamic_bitset<> validity(clouds_block[i].data().size(), true);

					for (size_t xy = 0; xy < clouds_block[i].data().size(); xy++)
						validity.set(xy, clouds_block[i].data()[xy] == 0 || clouds_block[i].data()[xy] == cloudNoData);

					//set this validity for all scene bands
					assert(validity.size() == block_data[i].data().size());
					for (size_t ii = 0; ii < block_data.GetSceneSize(); ii++)
						block_data.at(i * block_data.GetSceneSize() + ii).SetValidity(validity);
				}
			}

			m_options.m_timerRead.stop();
		}
	}


	
	void CDesawtooth::ProcessBlock(int xBlock, int yBlock, const Landsat2::CLandsatWindow& window, OutputData& outputData)
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
			outputData.resize(window.GetNbScenes());
			for (size_t s = 0; s < outputData.size(); s++)
				outputData[s].insert(outputData[s].begin(), blockSize.m_x * blockSize.m_y, m_options.m_dstNodata);
		}



#pragma omp critical(ProcessBlock)
		{
			m_options.m_timerProcess.start();


//#pragma omp parallel for num_threads( m_options.m_CPU ) if (m_options.m_bMulti )
			for (int y = 0; y < blockSize.m_y; y++)
			{
				for (int x = 0; x < blockSize.m_x; x++)
				{
					int xy = y * blockSize.m_x + x;

					//Get pixel
					CRealArray years = ::convert(allpos(window.size()));
					CRealArray data(window.size());
					CBoolArray goods(window.size());

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


					for (size_t z = 0; z < window.size(); z++)
					{
						size_t zz = z;

						if (m_first_valid != NOT_INIT && zz < m_first_valid)
							zz = m_first_valid;
						if (m_last_valid != NOT_INIT && zz > m_last_valid)
							zz = m_last_valid;


						CLandsatPixel pixel = window.GetPixel(zz, x, y);
						goods[z] = pixel.IsValid();

						if (goods[z])
						{
							data[z] = window.GetPixelIndice(zz, m_options.m_indice, x, y, m_options.m_rings);
							//assert(data[z] != 0);
							if (data[z] == 0)
							{
								int k;
								k = 0;
							}
							goods[z] = data[z] != 0;//humm!!!
						}
					}
					
					size_t nbVal = sum(goods);
					if (nbVal > m_options.m_minneeded )//at least one valid pixel
					{
						REAL_TYPE minimum_x_year = years.min();
						assert(minimum_x_year == years[0]);

						CRealArray all_x = years - minimum_x_year;

						//Take out spikes that start && end at same value (to get rid of weird years
						//			left over after cloud filtering)
						CRealArray output_corr_factore(data.size());

						//compute Desawtooth for this time series indice
						assert(m_options.m_desawtooth_val < 1.0);
						CRealArray all_y = desawtooth(data, goods, m_options.m_desawtooth_val, &output_corr_factore);
						assert(all_y.size() == window.size());

						
						for (size_t z = 0; z < window.size(); z++)
						{
							if (goods[z])
							{
								double val = max(GetTypeLimit(m_options.m_outputType, true), min(GetTypeLimit(m_options.m_outputType, false), output_corr_factore[z]));
								outputData[z][xy] = val;
							}
						}
					}//if at least one valid pixel

#pragma omp atomic
					m_options.m_xx++;

				}//for x
				m_options.UpdateBar();
			}//for y

			m_options.m_timerProcess.stop();

		}//critical process
	}


	void CDesawtooth::WriteBlock(int xBlock, int yBlock, CGDALDatasetEx& outputDS, OutputData& outputData)
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
						pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(outputData[z][0]), outputRect.Width(), outputRect.Height(), GDT_Float64, 0, 0);
					}
					else
					{
						double noData = outputDS.GetNoData(z);
						pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &noData, 1, 1, GDT_Float64, 0, 0);
					}
				}
			}




			m_options.m_timerWrite.stop();
		}
	}

	void CDesawtooth::CloseAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS)
	{
		inputDS.Close();
		maskDS.Close();

		m_options.m_timerWrite.start();

		outputDS.Close(m_options);

		m_options.m_timerWrite.stop();

	}

}
