//***********************************************************************
// program to output slope, aspect and sky view factor from DEM
//
//***********************************************************************
// version
// 1.0.0	10/06/2025	Rémi Saint-Amant	Creation from Christian R.Steger(2022)
//HORAYZON v1.2: an efficient and flexible ray-tracing algorithm to compute horizon and sky view factor



//G:\TravauxModels\Horizon  
//-te 7.70 46.3 8.30 46.75 -overwrite -of VRT -co COMPRESS=LZW -co TILED=YES -stats -multi ./MapInput/srtm_38_03_block.tif ./MapOutput/srtm_38_03_te.vrt

#include <math.h>
#include <array>
#include <utility>
#include <iostream>


#include "Horizon.h"
#include "HorizonImage.h"
#include "basic/OpenMP.h"
#include "geomatic/GDAL.h"

//#include <tbb/parallel_for.h>
//#include <tbb/parallel_reduce.h>





using namespace std;
//using namespace WBSF::Landsat2;
//using namespace LTR;



namespace WBSF
{
	const char* CHorizon::VERSION = "1.0.0";
	const size_t CHorizon::NB_THREAD_PROCESS = 2;


	//*********************************************************************************************************************

	CHorizonOption::CHorizonOption()
	{

		m_rings = 0;

		m_dist_search = 50;
		//m_ray_algorithm = "guess_constant";
		m_ray_algorithm = "binary_search";
		//m_ray_algorithm = "discrete_sampling";
		m_geom_type = "grid";
		m_ellps = "WGS84";
		m_azim_num = 360;
		m_hori_acc = 0.1;  // [degree]
		m_ray_org_elev = 2;//[m]
		m_elev_ang_low_lim = -89.98;// [degree]
		m_elev_ang_up_lim = 89.98;// [degree]


		m_appDescription = "This software compute slope, aspect, sky view factor from a DEM.";


		static const COptionDef OPTIONS[] =
		{
	
			//{ "-ray_algorithm", 1, "name", false, "Algorithm for horizon detection (discrete_sampling, binary_search, guess_constant). binary_search by default."},
			{ "-geom_type", 1, "name", false, "Embree geometry type(triangle, quad, grid). grid by default." },
			{ "-dist_search", 1, "r", false, "Search distance for horizon [km]. 50 km by default."},
			//{ "-Window", 1, "radius", false, "Compute window mean around the pixel where the radius is the number of pixels around the pixel: 1 = 1x1, 2 = 3x3, 3 = 5x5 etc. But can also be a float to get the average between 2 rings. For example 1.25 will be compute as follow: 0.75*(1x1) + 0.25*(3x3). 1 by default." },
			{ "-ellps", 1, "name", false, "Earth's surface approximation (sphere, GRS80 or WGS84). WGS84 by default"},
			{ "-Undulation", 0, "", false, "Compute the geoid undulation for the EGM96."},
			{ "-azim_num", 1, "n", false, "Number of azimuth sampling directions. 360 by default"},
			{ "-hori_acc", 1, "acc", false, "Accuracy of horizon computation [degree]. 0.1 degree by default." },
			{ "-ray_org_elev", 1, "acc", false, "Vertical elevation of ray origin above surface [metre]. 2 meters by default." },
			//{ "-hori_fill", 1, "acc", false, "Accuracy of horizon computation [degree]. 0.1 degree by default." },
			{ "-elev_ang_low_lim", 1, "acc", false, "Low horizon angle limit [degree]. -89.98 degree by default." },
			{ "-elev_ang_up_lim", 1, "acc", false, "high horizon angle limith [degree]. 89.98 degree by default." },
			{ "-output_angle", 0, "", false, "Export horizon angle for all azimuth. False by default." },
			{ "-output_distance", 0, "", false, "Export horizon distance for all azimuth. False by default." },
			{ "srcfile", 0, "", false, "Input image (DEM) file path." },
			{ "dstfile", 0, "", false, "Output image file path." }
		};

		AddOption("-ty");
		for (size_t i = 0; i < sizeof(OPTIONS) / sizeof(COptionDef); i++)
			AddOption(OPTIONS[i]);


		//Pour les trigger Bande 1 c’est - 125 quand on fait  ex.b1 1994 – b1 1995 ou b1 1996 – b1 1995.
		//Pour le tassel Cap brightness c’est + 750  ex.tcb1994 – tcb 1995 ou tcb 1996 – tcb 1995


		static const CIOFileInfoDef IO_FILE_INFO[] =
		{
			{ "Input Image", "srcfile", "", "", "Elevation", "" },
			{ "Output Image", "dstfile", "", "3", "Slope|Aspect|Sky View Factor", "" },
		};

		for (size_t i = 0; i < sizeof(IO_FILE_INFO) / sizeof(CIOFileInfoDef); i++)
			AddIOFileInfo(IO_FILE_INFO[i]);
	}

	ERMsg CHorizonOption::ParseOption(int argc, char* argv[])
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

		GetGDALDataType();


		return msg;
	}

	ERMsg CHorizonOption::ProcessOption(int& i, int argc, char* argv[])
	{
		ERMsg msg;

		 
		if (IsEqual(argv[i], "-m_dist_search"))
		{
			m_dist_search = atof(argv[++i]);
		}
		else if (IsEqual(argv[i], "-ray_algorithm"))
		{
			m_ray_algorithm = argv[++i];
		}
		else if (IsEqual(argv[i], "-geom_type"))
		{
			m_geom_type = argv[++i];
		}
		else if (IsEqual(argv[i], "-ellps"))
		{
			m_ellps = argv[++i];
		}
		else if (IsEqual(argv[i], "-azim_num"))
		{
			m_azim_num = atoi(argv[++i]);
		}
		else if (IsEqual(argv[i], "-hori_acc"))
		{
			m_hori_acc = atof(argv[++i]);
		}
		else if (IsEqual(argv[i], "-ray_org_elev"))
		{
			m_ray_org_elev = atof(argv[++i]);
		}
		else if (IsEqual(argv[i], "-ray_org_elev"))
		{
			m_elev_ang_low_lim = atof(argv[++i]);
		}
		else if (IsEqual(argv[i], "-ray_org_elev"))
		{
			m_elev_ang_up_lim = atof(argv[++i]);
		}
		else
		{
			//Look to see if it's a know base option
			msg = CBaseOptions::ProcessOption(i, argc, argv);
		}

		return msg;
	}


	ERMsg CHorizon::Execute()
	{
		ERMsg msg;

		if (!m_options.m_bQuiet)
		{
			cout << "Output: " << m_options.m_filesPath[CHorizonOption::OUTPUT_FILE_PATH] << endl;
			cout << "From:   " << m_options.m_filesPath[CHorizonOption::INPUT_FILE_PATH] << endl;
			//cout << "max_segments:   " << m_options.m_max_segments << endl;

			if (!m_options.m_maskName.empty())
				cout << "Mask:   " << m_options.m_maskName << endl;

		}


		GDALAllRegister();

		CGDALDatasetEx inputDS;
		CGDALDatasetEx maskDS;
		CGDALDatasetEx outputDS;

		msg = OpenAll(inputDS, maskDS, outputDS);
		if (!msg)
			return msg;


		//let a buffer in function os search distance and resolution
		m_options.m_rings = 0;// m_options.m_dist_search / ((m_options.m_extents.XRes() + m_options.m_extents.YRes() / 2));


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

			CRasterWindow inputData;
			OutputData outputData;
			BreaksData breaksData;
			ReadBlock(inputDS, xBlock, yBlock, inputData);
			ProcessBlock(xBlock, yBlock, inputData, outputData);
			WriteBlock(xBlock, yBlock, outputDS, outputData);
		}//for all blocks

		CloseAll(inputDS, maskDS, outputDS);

		return msg;
	}



	ERMsg CHorizon::OpenAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS)
	{
		ERMsg msg;

		if (!m_options.m_bQuiet)
			cout << endl << "Open input image..." << endl;

		msg = inputDS.OpenInputImage(m_options.m_filesPath[CHorizonOption::INPUT_FILE_PATH], m_options);

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
				//cout << "    NbBands        = " << inputDS.GetRasterCount() << endl;
				//cout << "    Scene size     = " << inputDS.GetSceneSize() << endl;
				//cout << "    Nb. Scenes     = " << inputDS.GetNbScenes() << endl;

				//if (inputDS.GetRasterCount() < 2)
					//msg.ajoute("Horizon need at least 2 bands");
			}


		}


		if (msg && !m_options.m_maskName.empty())
		{
			if (!m_options.m_bQuiet)
				cout << "Open mask image..." << endl;
			msg += maskDS.OpenInputImage(m_options.m_maskName);
		}


		if (msg && m_options.m_bCreateImage)
		{
			size_t nb_scenes = 3;// m_options.m_scene_extents[1] - m_options.m_scene_extents[0] + 1;
			CHorizonOption options(m_options);
			options.m_scenes_def.clear();
			options.m_nbBands = nb_scenes;

			if (!m_options.m_bQuiet)
			{
				cout << endl;
				cout << "Open output images..." << endl;
				cout << "    Size           = " << options.m_extents.m_xSize << " cols x " << options.m_extents.m_ySize << " rows x " << options.m_nbBands << " bands" << endl;
				cout << "    Extents        = X:{" << ToString(options.m_extents.m_xMin) << ", " << ToString(options.m_extents.m_xMax) << "}  Y:{" << ToString(options.m_extents.m_yMin) << ", " << ToString(options.m_extents.m_yMax) << "}" << endl;
				cout << "    Block size     = " << ToString(options.m_extents.m_xBlockSize) << " x " << ToString(options.m_extents.m_yBlockSize) << endl;
				cout << "    Blocks         = " << std::ceil(options.m_extents.m_ySize / options.m_extents.m_xBlockSize) << " x " << std::ceil(options.m_extents.m_ySize / options.m_extents.m_yBlockSize) << endl;
				//cout << "    NbBands        = " << nb_scenes << endl;
				//cout << "    Nb. Scenes     = " << nb_scenes << endl;
			}

			//std::string indices_name = Landsat2::GetIndiceName(m_options.m_indice);
			string filePath = options.m_filesPath[CHorizonOption::OUTPUT_FILE_PATH];

			static const array<string, 3> VAR_NAME = { "Slope","Aspect","SVF" };

			//replace the common part by the new name
			for (size_t zz = 0; zz < nb_scenes; zz++)
			{
				size_t z = zz;
				string subName = VAR_NAME[zz];
				options.m_VRTBandsName += GetFileTitle(filePath) + "_" + subName + ".tif|";

			}

			msg += outputDS.CreateImage(filePath, options);
		}

		return msg;
	}

	void CHorizon::ReadBlock(CGDALDatasetEx& inputDS, int xBlock, int yBlock, CRasterWindow& block_data)
	{
#pragma omp critical(BlockIO)
		{
			m_options.m_timerRead.start();

			CGeoExtents extents = m_options.m_extents.GetBlockExtents(xBlock, yBlock);
			inputDS.ReadBlock(extents, block_data, int(ceil(m_options.m_rings)), m_options.m_IOCPU, m_options.m_scene_extents[0], m_options.m_scene_extents[1]);

			m_options.m_timerRead.stop();
		}
	}



	void CHorizon::ProcessBlock(int xBlock, int yBlock, const CRasterWindow& window, OutputData& outputData)
	{
		//extents
		CGeoExtents extents = m_options.GetExtents().GetBlockExtents(xBlock, yBlock);
		CGeoSize blockSize = extents.GetSize();
		//(.GetBlockSize(xBlock, yBlock);

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
			outputData.resize(3);
			for (size_t s = 0; s < outputData.size(); s++)
				outputData[s].insert(outputData[s].begin(), blockSize.m_x * blockSize.m_y, float(m_options.m_dstNodata));
		}


		//const char* ray_algorithm = "binary_search";
		//const char* ray_algorithm = "guess_constant";
		//const char* geom_type = "grid";
		//const char* ellps = "WGS84";//Earth's surface approximation (sphere, GRS80 or WGS84)
		//int azim_num = 360; // number of azimuth sampling directions[-]
		//float hori_acc = 0.1f;  // [degree]
		//double dist_search = 50.0;// search distance for horizon[kilometre]
		//std::string DEM_name = "srtm_38_03.tif";
		//std::string DEM_name = "test.tif";




#pragma omp critical(ProcessBlock)
		{
			m_options.m_timerProcess.start();

			// Precompute values of trigonometric functions
			// these arrays can be shared between threads (read-only)
			const CHorizonOption& op = m_options;

			std::vector<Azimuth> azimuth(op.m_azim_num);
			for (size_t i = 0; i < azimuth.size(); i++)
			{
				azimuth[i].angle = i * ((2 * M_PI) / azimuth.size());
				azimuth[i].sin = std::sin(azimuth[i].angle);
				azimuth[i].cos = std::cos(azimuth[i].angle);
			}
			
			
			//convert memory to pt obsject
			matrix<pt3> dem(blockSize.m_x, blockSize.m_y);
			for (int y = 0; y < blockSize.m_y; y++)
			{
				for (int x = 0; x < blockSize.m_x; x++)
				{
					CGeoPoint pt = extents.XYPosToCoord(CGeoPointIndex(x, y));
					dem(x, y).x = pt.m_x;
					dem(x, y).y = pt.m_y;
					dem(x, y).z = window.at(0).at(x, y);
				}
			}

			//convert to global ENU
			CGeoPoint pt = extents.GetCentroid();
			Transformer_LLA_ENU T(pt.m_x, pt.m_y, 0, op.m_ellps.c_str());
			matrix<pt3> dem_enu = T.lla2enu(dem);

			//convert from point to float buffer
			std::vector<float> vec_dem_enu = dem_enu.vectorize();
		

			
			embree_horizon EH(op.m_azim_num, op.m_dist_search, op.m_hori_acc, op.m_ray_org_elev,
							op.m_dstNodata, op.m_elev_ang_low_lim, op.m_elev_ang_up_lim);
			
			EH.init(vec_dem_enu.data(), blockSize.m_y, blockSize.m_x, op.m_ray_algorithm.c_str(), op.m_geom_type.c_str());

			size_t num_rays = 0;

			
			//num_rays += tbb::parallel_reduce(
				//tbb::blocked_range<size_t>(0, blockSize.m_y), 0.0,
				//[&](tbb::blocked_range<size_t> r, size_t num_rays)
				//{  // parallel
					//for (size_t y = r.begin(); y < r.end(); ++y)   // parallel


#pragma omp parallel for num_threads( m_options.m_CPU ) if (m_options.m_bMulti )
			for (int y = 0; y < blockSize.m_y; y++)//serial
			{
				for (int x = 0; x < blockSize.m_x; x++)
				{
					int xy = y * blockSize.m_x + x;

					std::vector<double> hori_buffer(azimuth.size());
					EH.compute_horizon(size_t(y), size_t(x), hori_buffer.data());


					double slope = 0;//slope in degree
					double aspect = 0;//aspect degree from North 
					get_slope_aspect(x, y, dem_enu, slope, aspect);
					pt3 N = T.Normal(slope, aspect);

					outputData[0][xy] = (float)slope;
					outputData[1][xy] = (float)aspect;//aspect from North 
					outputData[2][xy] = (float)sky_view_factor(azimuth, hori_buffer, N);


#pragma omp atomic
					m_options.m_xx++;
				}

				m_options.UpdateBar();
			}





			//	return num_rays;  // parallel
			//}, std::plus<size_t>());  // parallel


			EH.clean();



		}//critical process
	}


	void CHorizon::WriteBlock(int xBlock, int yBlock, CGDALDatasetEx& outputDS, OutputData& outputData)
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
						pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(outputData[z][0]), outputRect.Width(), outputRect.Height(), GDT_Float32, 0, 0);
					}
					else
					{
						float noData = (float)outputDS.GetNoData(z);
						pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &noData, 1, 1, GDT_Float32, 0, 0);
					}
				}
			}




			m_options.m_timerWrite.stop();
		}
	}

	void CHorizon::CloseAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS)
	{
		inputDS.Close();
		maskDS.Close();

		m_options.m_timerWrite.start();

		outputDS.Close(m_options);

		m_options.m_timerWrite.stop();

	}

}
