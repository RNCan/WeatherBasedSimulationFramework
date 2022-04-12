//***********************************************************************
// Compute horizon elevation from a DEM
//									 
//***********************************************************************
// version
// 1.0.0	28/03/2022	Rémi Saint-Amant	Creation


#include "stdafx.h"
#include <float.h>
#include <math.h>
#include <array>
#include <utility>
#include <iostream>
#include <boost/dynamic_bitset.hpp>
#include "Basic/Mtrx.h"
#include "HorizonImage.h"
#include "Basic/OpenMP.h"

#pragma warning(disable: 4275 4251)
#include "gdal_priv.h"


using namespace std;
using namespace WBSF::Landsat;

namespace WBSF
{

	static const double EVALUATE_EACH_ARC = 0.00833;//evaluate at each 30 seconds
	static const double pi = std::atan(1) * 4;

	template <typename T>
	T square(const T& x) { return x * x; }


	const char* CHorizonImage::VERSION = "1.0.0";
	std::string CHorizonImage::GetDescription() { return  std::string("HorizonImage version ") + CHorizonImage::VERSION + " (" + __DATE__ + ")"; }

	//*********************************************************************************************************************


	CHorizonImageOption::CHorizonImageOption()
	{
		//m_start = 0;
		//m_end = 360;
		//m_by = 45;
		//m_bRadian = false;//in radian by default
		m_nAngles = 8; //16 angles of 22.5 °
		m_maxDistance = 10000; //50km

		m_appDescription = "This software compute horizon for each angle define between first and last by step degrees";

		static const COptionDef OPTIONS[] =
		{
			//{ "-Start", 1, "type", false, "Start angle in degree (North=0). 0 by default." },
			//{ "-End", 1, "type", false, "End angle in degree (North=0). 360 by default." },
			//{ "-By", 1, "type", false, "Step site angle in degree. 45 by default. 0 mean only at start angle" },
			//{ "-Radian", 0, "", false, "Output angle in radian. Degree by default." },
			{ "-nAngles", 0, "", false, "Discrete number of angles. 8 by default (45°)." },
			//{ "-Precision", 0, "", false, "Angle size in degree. 22.5 by default." },
			{ "-Distance", 1, "", false, "Maximum distance (m) to find horizon height. 10 000 m by default" },
			{ "-Precision", 0, "", false, "Evaluate only point at each distance For optimization" },

			{ "srcfile", 0, "", false, "Input DEM file path." },
			{ "dstfile", 0, "", false, "Output image file path." }
		};


		for (int i = 0; i < sizeof(OPTIONS) / sizeof(COptionDef); i++)
			AddOption(OPTIONS[i]);

		static const CIOFileInfoDef IO_FILE_INFO[] =
		{
			{ "Input Image", "srcfile", "", "DEM with elevation", "" },
			{ "Output Image", "dstfile", "", "1", "Horizon at angle 1|Horizon at angle 2|Horizon at angle 3|...", "" },
		};

		for (int i = 0; i < sizeof(IO_FILE_INFO) / sizeof(CIOFileInfoDef); i++)
			AddIOFileInfo(IO_FILE_INFO[i]);
	}

	ERMsg CHorizonImageOption::ParseOption(int argc, char* argv[])
	{
		ERMsg msg = CBaseOptions::ParseOption(argc, argv);

		ASSERT(NB_FILE_PATH == 2);
		if (msg && m_filesPath.size() != NB_FILE_PATH)
		{
			msg.ajoute("ERROR: Invalid argument line. 2 files are needed: the DEM and destination image.");
			msg.ajoute("Argument found: ");
			for (size_t i = 0; i < m_filesPath.size(); i++)
				msg.ajoute("   " + to_string(i + 1) + "- " + m_filesPath[i]);
		}


		if (m_outputType == GDT_Unknown)
			m_outputType = GDT_Float32;


		m_BLOCK_THREADS = omp_get_num_procs();


		return msg;
	}

	ERMsg CHorizonImageOption::ProcessOption(int& i, int argc, char* argv[])
	{
		ERMsg msg;


		//if (IsEqual(argv[i], "-Radian"))
		//{
		//	m_bRadian = true;
		//}
		//else if (IsEqual(argv[i], "-Start"))
		//{
		//	m_start = atof(argv[++i]);

		//	if (m_start < 0 || m_start>360)
		//		msg.ajoute("Invalid -Start. Start must be between 0 and 360");
		//}
		//else if (IsEqual(argv[i], "-End"))
		//{
		//	m_end = atof(argv[++i]);
		//	if (m_end < 0 || m_end>360)
		//		msg.ajoute("Invalid -End. End must be between 0 and 360");
		//}
		//else if (IsEqual(argv[i], "-By"))
		//{
		//	m_by = atof(argv[++i]);
		//	if (m_start < 0 || m_start>360)
		//		msg.ajoute("Invalid -By. By must be between 0 and 360");
		//}

		if (IsEqual(argv[i], "-nAngles"))
		{
			m_nAngles = atoi(argv[++i]);
			//m_precision = 360.0 / m_nAngles;
			if (m_nAngles < 1 || m_nAngles>360)
				msg.ajoute("Invalid -nAngles. nAngles must be between 1 and 360");
		}
		else if (IsEqual(argv[i], "-Distance"))
		{
			m_maxDistance = atof(argv[++i]);
			if (m_maxDistance < 1 || m_maxDistance>100000)
				msg.ajoute("Invalid -Distance. Distance must be between 1 and 100 000 m");
		}
		else
		{
			//Look to see if it's a know base option
			msg = CBaseOptions::ProcessOption(i, argc, argv);
		}


		return msg;
	}


	ERMsg CHorizonImage::Execute()
	{
		ERMsg msg;

		if (!m_options.m_bQuiet)
		{
			cout << "Output: " << m_options.m_filesPath[CHorizonImageOption::OUTPUT_FILE_PATH] << endl;
			cout << "From:   " << m_options.m_filesPath[CHorizonImageOption::INPUT_FILE_PATH] << endl;

			if (!m_options.m_maskName.empty())
				cout << "Mask:   " << m_options.m_maskName << endl;
		}

		GDALAllRegister();

		CGDALDatasetEx inputDS;
		CGDALDatasetEx maskDS;
		CGDALDatasetEx outputDS;


		msg = OpenAll(inputDS, maskDS, outputDS);

		//CGeoPoint location = inputDS.GetExtents().GetCentroid();
		//CHorizon horizon = ComputeHorizon(location, inputDS, m_options.m_maxDistance, 360.0 / m_options.m_nAngles);


		if (msg)
		{
			//res must be convert in km

			CStatistic buffer = 0;


			vector<pair<int, int>> blocks= m_options.GetExtents().GetBlockList();
			for (size_t b = 0; b < blocks.size(); b++)
			{
				CEllipsoidProperties ellipsoid;
				CGeoExtents extents = m_options.GetExtents();
				CGeoDistance delta = ellipsoid.dist2deg(m_options.m_maxDistance / 1000.0, extents.GetCentroid());
				buffer+= max(0, min(m_options.GetExtents().m_xBlockSize, (int)round(delta.m_x / extents.XRes())));
				buffer+= max(0, min(m_options.GetExtents().m_yBlockSize, (int)round(delta.m_y / extents.YRes())));
				//int buffer_x = max(0, min( m_options.GetExtents().m_xBlockSize, (int)round(delta.m_x / extents.XRes()) - extents.m_xSize / 2));
				//int buffer_y = max(0, min( m_options.GetExtents().m_yBlockSize, (int)round(delta.m_y / extents.YRes()) - extents.m_ySize / 2));
				//buffer = max(buffer_x, buffer_y);
			}



			CBandsHolderMT bandHolder((int)buffer[HIGHEST], m_options.m_memoryLimit, m_options.m_IOCPU, m_options.m_BLOCK_THREADS);

			if (maskDS.IsOpen())
				bandHolder.SetMask(maskDS.GetSingleBandHolder(), m_options.m_maskDataUsed);


			msg += bandHolder.Load(inputDS, m_options.m_bQuiet, m_options.GetExtents());

			if (!msg)
				return msg;

			if (!m_options.m_bQuiet && m_options.m_bCreateImage)
			{
				cout << "Create output images (" << outputDS.GetRasterXSize() << " C x " << outputDS.GetRasterYSize() << " R x " << outputDS.GetRasterCount() << " B) with " << m_options.m_BLOCK_THREADS << " threads..." << endl;
			}

			CGeoExtents extents = bandHolder.GetExtents();
			m_options.ResetBar((size_t)extents.m_xSize * extents.m_ySize);

			vector<pair<int, int>> XYindex = extents.GetBlockList();


#pragma omp parallel for schedule(static, 1)  num_threads( m_options.m_BLOCK_THREADS ) if (m_options.m_bMulti) 
			for (int b = 0; b < (int)XYindex.size(); b++)
			{
				int xBlock = XYindex[b].first;
				int yBlock = XYindex[b].second;
				//
				int thread = ::omp_get_thread_num();
				//
				OutputData outputData;

				ReadBlock(xBlock, yBlock, bandHolder[thread]);
				ProcessBlock(xBlock, yBlock, bandHolder[thread], outputData);
				WriteBlock(xBlock, yBlock, bandHolder[thread], outputDS, outputData);
			}//for all blocks



			CloseAll(inputDS, maskDS, outputDS);
		}//if msg

		return msg;
	}

	void CHorizonImage::AllocateMemory(size_t sceneSize, CGeoSize blockSize, OutputData& outputData)
	{

		if (m_options.m_bCreateImage)
		{
			OutputDataType dstNodata = (OutputDataType)m_options.m_dstNodata;
			outputData.resize(sceneSize);
			for (size_t z = 0; z < outputData.size(); z++)
			{
				outputData[z].resize(blockSize.m_y * blockSize.m_x, dstNodata);
			}
		}
	}


	ERMsg CHorizonImage::OpenAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS)
	{
		ERMsg msg;


		if (!m_options.m_bQuiet)
			cout << endl << "Open input image..." << endl;

		msg = inputDS.OpenInputImage(m_options.m_filesPath[CHorizonImageOption::INPUT_FILE_PATH], m_options);
		//m_options.m_extents.m_xBlockSize = inputDS.GetRasterXSize();
		//m_options.m_extents.m_yBlockSize = inputDS.GetRasterYSize();


		if (!msg)
			return msg;

		if (msg)
		{
			inputDS.UpdateOption(m_options);
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
			//cout << "    Scene size     = " << inputDS.GetSceneSize() << endl;
			//cout << "    Entire period  = " << inputDS.GetPeriod().GetFormatedString() << " (nb scenes = " << inputDS.GetNbScenes() << ")" << endl;
			//cout << "    Input period   = " << m_options.m_period.GetFormatedString() << endl;
			//cout << "    Loaded period  = " << period.GetFormatedString() << " (nb scenes = " << nbSceneLoaded << ")" << endl;


			if (inputDS.GetSceneSize() != 1)
				cout << FormatMsg("WARNING: DEM must have only one band and not %1 bands", to_string(inputDS.GetSceneSize())) << endl;
		}



		if (!m_options.m_maskName.empty())
		{
			if (!m_options.m_bQuiet)
				cout << "Open mask..." << endl;

			msg += maskDS.OpenInputImage(m_options.m_maskName);
		}

		if (msg && m_options.m_bCreateImage)
		{
			CHorizonImageOption option(m_options);
			option.m_nbBands = m_options.GetNbAngles() + 3;

			if (!m_options.m_bQuiet)
			{
				cout << endl;
				cout << "Open output images..." << endl;
				cout << "    Size           = " << m_options.m_extents.m_xSize << " cols x " << m_options.m_extents.m_ySize << " rows x " << option.m_nbBands << " bands" << endl;
				cout << "    Extents        = X:{" << ToString(m_options.m_extents.m_xMin) << ", " << ToString(m_options.m_extents.m_xMax) << "}  Y:{" << ToString(m_options.m_extents.m_yMin) << ", " << ToString(m_options.m_extents.m_yMax) << "}" << endl;
			}


			string filePath = option.m_filesPath[CHorizonImageOption::OUTPUT_FILE_PATH];
			string title = GetFileTitle(filePath);
			
			for (size_t b = 0; b < m_options.GetNbAngles(); b++)
				option.m_VRTBandsName += title + "_" + ToString( 360.0*b/ m_options.GetNbAngles() ) + ".tif|";
			
			option.m_VRTBandsName += title + "_slope.tif";
			option.m_VRTBandsName += "|" + title + "_aspect.tif";
			option.m_VRTBandsName += "|" + title + "_SVF.tif";


			
			msg += outputDS.CreateImage(filePath, option);
		}



		return msg;
	}

	void CHorizonImage::ReadBlock(int xBlock, int yBlock, CBandsHolder& bandHolder)
	{
#pragma omp critical(BlockIORead)
		{
			m_options.m_timerRead.Start();

			CGeoExtents extents = m_options.m_extents.GetBlockExtents(xBlock, yBlock);
			bandHolder.LoadBlock(extents, m_options.m_period);

			m_options.m_timerRead.Stop();
		}
	}

	//	void CHorizonImage::ProcessBlock(int xBlock, int yBlock, CBandsHolder& bandHolder, OutputData& outputData)
	//	{
	//		CGeoExtents extents = bandHolder.GetExtents();
	//		CGeoSize blockSize = extents.GetBlockSize(xBlock, yBlock);
	//
	//		if (bandHolder.IsEmpty())
	//		{
	//#pragma omp critical(ProcessBlock)
	//			{
	//				int nbCells = blockSize.m_x * blockSize.m_y;
	//
	//#pragma omp atomic
	//				m_options.m_xx += nbCells;
	//
	//				m_options.UpdateBar();
	//			}
	//
	//			return;
	//		}
	//
	//
	//
	//		// Elevation vector length
	//		double distance = m_options.m_maxDistance;
	//		double angle = 360.0 / m_options.m_nAngles;
	//
	//
	//		vector<double> az;
	//		for (double i = -180; i < 180; i += angle)
	//			az.push_back(i);
	//
	//		if (m_options.m_bCreateImage)
	//			AllocateMemory(az.size() + 3, blockSize, outputData);
	//
	//
	//
	//		CEllipsoidProperties ellipsoid;
	//
	//		CGeoPoint centroid = extents.GetCentroid();
	//		CGeoDistance delta = ellipsoid.dist2deg(distance / 1000.0, centroid);
	//		CGeoRect clip(extents.GetPrjID());
	//		clip.m_xMin = centroid.m_lon - delta.m_lon;
	//		clip.m_xMax = centroid.m_lon + delta.m_lon;
	//		clip.m_yMin = centroid.m_lat - delta.m_lat;
	//		clip.m_yMax = centroid.m_lat + delta.m_lat;
	//		clip = extents.IntersectRect(clip);
	//
	//		CGeoRectIndex rect = extents.CoordToXYPos(clip);
	//		CGeoExtents studyArea_extents(clip, rect.GetGeoSize());
	//
	//		vector<CGeoPoint3D> meshgrid(studyArea_extents.GetNbPixels());
	//
	//
	//		//compute azimuth_threshold for all ring of 1 km each
	//		vector<double> azimuth_threshold(ceil(distance / 1000.0));
	//
	//		for (size_t y = 0; y < studyArea_extents.m_ySize; y++)
	//		{
	//			for (size_t x = 0; x < studyArea_extents.m_xSize; x++)
	//			{
	//				size_t xy = y * studyArea_extents.m_xSize + x;
	//				((CGeoPoint&)meshgrid[xy]) = studyArea_extents.XYPosToCoord(CGeoPointIndex((int)x, (int)y));
	//
	//				DataType e = bandHolder.GetPixel(0, (int)x, (int)y);
	//				if (bandHolder.IsValid(0, e))
	//					meshgrid[xy].m_z = e;
	//				else
	//					meshgrid[xy].m_z = -999;
	//
	//
	//				//outputData[0][xy] = ellipsoid.getAzimuth(centroid, meshgrid[xy]);
	//									// 
	//				//double d = centroid.GetDistance(meshgrid[xy]);
	//				//if (d > 250 && d < distance)
	//				//{
	//				//	double azimuth1 = ellipsoid.getAzimuth(centroid, meshgrid[xy] + CGeoDistance(extents.XRes() / 2, extents.YRes() / 2, extents.GetPrjID()));
	//				//	double azimuth2 = ellipsoid.getAzimuth(centroid, meshgrid[xy] + CGeoDistance(extents.XRes() / 2, -extents.YRes() / 2, extents.GetPrjID()));
	//				//	double azimuth3 = ellipsoid.getAzimuth(centroid, meshgrid[xy] + CGeoDistance(-extents.XRes() / 2, extents.YRes() / 2, extents.GetPrjID()));
	//				//	double azimuth4 = ellipsoid.getAzimuth(centroid, meshgrid[xy] + CGeoDistance(-extents.XRes() / 2, -extents.YRes() / 2, extents.GetPrjID()));
	//				//	double azimuthLo = min(azimuth1, min(azimuth2, min(azimuth3, azimuth4)));
	//				//	double azimuthHi = max(azimuth1, max(azimuth2, max(azimuth3, azimuth4)));
	//				//	//adjust pixels on the North transition
	//				//	if (azimuthLo < -90 && azimuthHi > 90)
	//				//	{
	//				//		double Lo = azimuthHi - 360;
	//				//		azimuthHi = azimuthLo;
	//				//		azimuthLo = Lo;
	//				//	}
	//				//
	//				//	assert(azimuthHi > azimuthLo);
	//				//	double delta_azimuth = azimuthHi - azimuthLo;
	//				//	if (delta_azimuth > azimuth_threshold[size_t(d / 1000.0)])
	//				//		azimuth_threshold[size_t(d / 1000.0)] = delta_azimuth;
	//				//}
	//
	//
	//			}
	//		}
	//
	//
	//		//pt = studyArea_extents.CoordToXYPos(location);
	//
	//
	//
	//		//#pragma omp critical(ProcessBlock)
	//		{
	//			m_options.m_timerProcess.Start();
	//
	//
	//#pragma omp parallel for num_threads( m_options.BLOCK_CPU() ) if (m_options.m_bMulti) 
	//			for (int yy = 0; yy < blockSize.m_y; yy++)//blockSize.m_y
	//			{
	//				//		int yy = blockSize.m_y / 2;
	//				for (int xx = 0; xx < blockSize.m_x; xx++)
	//				{
	//					//		int xx = blockSize.m_x / 2;
	//					size_t xxyy = yy * blockSize.m_x + xx;
	//
	//
	//					CGeoPointIndex loc_pt(xx, yy);
	//					CGeoPoint3D loc;
	//					// Corresponding position and elevation
	//					((CGeoPoint&)loc) = extents.XYPosToCoord(loc_pt);
	//					loc.m_z = bandHolder.GetPixel(0, loc_pt.m_x, loc_pt.m_y);
	//
	//
	//					if (!outputData.empty() && bandHolder.IsValid(0, loc.m_z))
	//					{
	//
	//						double S = 0;
	//						double A = 0;
	//						bandHolder.GetWindow(0)->GetSlopeAndAspect(xx, yy, S, A);
	//						//convert slope in degree
	//						S = float(atan(S / 100) * 180 / pi);
	//
	//						CStatistic SVF;
	//						//find the 500 nearest pixel in a direction
	//						for (size_t a = 0; a < az.size(); a++)
	//						{
	//							size_t n = max((size_t)ceil(delta.m_x / abs(studyArea_extents.XRes())), (size_t)ceil(delta.m_y / abs(studyArea_extents.XRes())));
	//							double d_geo = max(delta.m_x, delta.m_y);
	//
	//							double delta_x = d_geo * cos(pi / 180 * (-90 - az[a])) / n;
	//							double delta_y = d_geo * sin(pi / 180 * (-90 - az[a])) / n;
	//							CGeoDistance ray_step(delta_x, delta_y, loc.GetPrjID());
	//
	//
	//							size_t last_xy = loc_pt.m_y * studyArea_extents.m_xSize + loc_pt.m_x;
	//							CGeoPoint ray = loc;
	//							CStatistic E;
	//							while (ray.GetDistance(loc) < distance && studyArea_extents.IsInside(ray))
	//							{
	//								CGeoPointIndex pt = studyArea_extents.CoordToXYPos(ray);
	//								size_t xy = pt.m_y * studyArea_extents.m_xSize + pt.m_x;
	//								if (meshgrid[xy].m_z > -999 && xy != last_xy)
	//								{
	//									last_xy = xy;
	//									//double d = location.GetDistance(meshgrid[xy]);
	//									//double azimuth = ellipsoid.getAzimuth(loc, meshgrid[xy]);
	//
	//									//size_t xy = y * studyArea_extents.m_xSize + x;
	//									float elevation = ellipsoid.getElevationAngle(loc, meshgrid[xy]);
	//									if (elevation >= E[HIGHEST])
	//										E += max(0.0f, elevation);
	//									//if (elevation >= outputData[a][xy])
	//									//	outputData[a][xy] = max(0.0f, elevation);
	//								}
	//
	//								ray += ray_step;
	//							}
	//
	//							if (E[NB_VALUE] > 50)
	//								outputData[a][xxyy] = E[HIGHEST];
	//
	//							double H = outputData[a][xxyy];
	//							if (H >= 0 && S >= 0 && A >= 0 && A <= 360)
	//							{
	//								double F = cos(pi / 180 * S) * square(sin(pi / 180 * H)) + (sin(pi / 180 * S) * cos(pi / 180 * (az[a] + 180 - A)) * (H - sin(pi / 180 * H) * cos(pi / 180 * H)));
	//								assert(F > -15 && F < 15);
	//
	//								SVF += F;
	//							}
	//						}
	//
	//
	//						outputData[az.size()][xxyy] = S;
	//						outputData[az.size() + 1][xxyy] = A;
	//
	//						if (SVF.IsInit())
	//							outputData[az.size() + 2][xxyy] = SVF[MEAN];
	//
	//
	//
	//
	//						//for (size_t y = 0; y < studyArea_extents.m_ySize; y++)
	//						//{
	//						//	for (size_t x = 0; x < studyArea_extents.m_xSize; x++)
	//						//	{
	//						//		size_t xy = y * studyArea_extents.m_xSize + x;
	//						//		if (meshgrid[xy].m_z > -999)
	//						//		{
	//						//			double d = loc.GetDistance(meshgrid[xy]);
	//						//			double azimuth = ellipsoid.getAzimuth(loc, meshgrid[xy]);
	//						//
	//						//			bool bDoIt = d < 250;
	//						//			if (d < distance)
	//						//			{
	//						//				for (size_t a = 0; a < az.size() && !bDoIt; a++)
	//						//				{
	//						//					double delta_azimuth = abs(az[a] - azimuth);
	//						//					if (delta_azimuth > azimuth_threshold[size_t(d / 1000.0)])
	//						//						bDoIt = true;
	//						//				}
	//						//			}
	//						//
	//						//			if (bDoIt)
	//						//			{
	//						//				double azimuth1 = ellipsoid.getAzimuth(loc, meshgrid[xy] + CGeoDistance(extents.XRes() / 2, extents.YRes() / 2, extents.GetPrjID()));
	//						//				double azimuth2 = ellipsoid.getAzimuth(loc, meshgrid[xy] + CGeoDistance(extents.XRes() / 2, -extents.YRes() / 2, extents.GetPrjID()));
	//						//				double azimuth3 = ellipsoid.getAzimuth(loc, meshgrid[xy] + CGeoDistance(-extents.XRes() / 2, extents.YRes() / 2, extents.GetPrjID()));
	//						//				double azimuth4 = ellipsoid.getAzimuth(loc, meshgrid[xy] + CGeoDistance(-extents.XRes() / 2, -extents.YRes() / 2, extents.GetPrjID()));
	//						//				double azimuthLo = min(azimuth1, min(azimuth2, min(azimuth3, azimuth4)));
	//						//				double azimuthHi = max(azimuth1, max(azimuth2, max(azimuth3, azimuth4)));
	//						//
	//						//				//adjust pixels on the North transition
	//						//				if (azimuthLo < -90 && azimuthHi > 90)
	//						//				{
	//						//					double Lo = azimuthHi - 360;
	//						//					azimuthHi = azimuthLo;
	//						//					azimuthLo = Lo;
	//						//				}
	//						//
	//						//
	//						//				for (size_t a = 0; a < az.size(); a++)
	//						//				{
	//						//					if (az[a] >= azimuthLo && az[a] <= azimuthHi)
	//						//					{
	//						//						size_t xy = y * studyArea_extents.m_xSize + x;
	//						//						float elevation = ellipsoid.getElevationAngle(loc, meshgrid[xy]);
	//						//						if (elevation >= outputData[a][xxyy])
	//						//							outputData[a][xxyy] = max(0.0f, elevation);
	//						//						//if (elevation >= outputData[a][xy])
	//						//							//outputData[a][xy] = max(0.0f, elevation);
	//						//					}
	//						//				}
	//						//			}
	//						//		}
	//						//	}
	//						//}
	//
	//
	//					}//if output data
	//				//}
	//#pragma omp atomic 
	//					m_options.m_xx++;
	//
	//				}//for x
	//
	//				m_options.UpdateBar();
	//
	//			}//for y
	//
	//
	//
	//			m_options.m_timerProcess.Stop();
	//		}//critical
	//
	//	}
	//

	void CHorizonImage::ProcessBlock(int xBlock, int yBlock, CBandsHolder& bandHolder, OutputData& outputData)
	{
		CGeoExtents full_extents = bandHolder.GetExtents();
		CGeoExtents extents = full_extents.GetBlockExtents(xBlock, yBlock);// 
		CGeoSize blockSize = extents.GetSize();// .GetBlockSize(xBlock, yBlock);

		if (bandHolder.IsEmpty())
		{
#pragma omp critical(ProcessBlock) 
			{
				int nbCells = blockSize.m_x * blockSize.m_y;

#pragma omp atomic
				m_options.m_xx += nbCells;

				m_options.UpdateBar();
			}

			return;
		}



		// Elevation vector length
		double distance = m_options.m_maxDistance;
		double angle = 360.0 / m_options.m_nAngles;


		vector<double> az;
		for (double i = -180; i < 180; i += angle)
			az.push_back(i);

		if (m_options.m_bCreateImage)
			AllocateMemory(az.size() + 3, blockSize, outputData);


		m_options.m_timerProcess.Start();


		CEllipsoidProperties ellipsoid;

		CGeoPoint centroid = extents.GetCentroid();
		//CGeoDistance delta = ellipsoid.dist2deg(distance / 1000.0, centroid);
		


		vector<CGeoDistance> direction(az.size());
		//identify point in the outer pixel with the angle
		for (size_t a = 0; a < az.size(); a++)
		{

			CGeoPointIndex outer_pt(-1, -1);


			for (int y = 0; y < blockSize.m_y && outer_pt.m_x == -1; y++)
			{
				for (int x = 0; x < blockSize.m_x && outer_pt.m_x == -1; x++)
				{


					if (y == 0 || x == 0 || y == (blockSize.m_y - 1) || x == (blockSize.m_x - 1))
					{
						size_t xy = y * extents.m_xSize + x;

						CGeoPoint3D meshgrid;
						((CGeoPoint&)meshgrid) = extents.XYPosToCoord(CGeoPointIndex(x, y));
						//meshgrid.m_z = bandHolder.GetPixel(0, x, y);

						if (x == 0)
						{
							double azimuthNW = ellipsoid.getAzimuth(centroid, meshgrid + CGeoDistance(-extents.XRes() / 2, -extents.YRes() / 2, extents.GetPrjID()));
							double azimuthSW = ellipsoid.getAzimuth(centroid, meshgrid + CGeoDistance(-extents.XRes() / 2, extents.YRes() / 2, extents.GetPrjID()));
							if (az[a] >= azimuthSW && az[a] <= azimuthNW)
								outer_pt = CGeoPointIndex(x, y);
						}
						else if (x == (blockSize.m_x - 1))
						{
							double azimuthNE = ellipsoid.getAzimuth(centroid, meshgrid + CGeoDistance(extents.XRes() / 2, -extents.YRes() / 2, extents.GetPrjID()));
							double azimuthSE = ellipsoid.getAzimuth(centroid, meshgrid + CGeoDistance(extents.XRes() / 2, extents.YRes() / 2, extents.GetPrjID()));
							if (az[a] >= azimuthNE && az[a] <= azimuthSE)
								outer_pt = CGeoPointIndex(x, y);
						}

						if (y == 0)//|| x == 0 || x == (blockSize.m_x - 1)
						{
							double azimuthNW = ellipsoid.getAzimuth(centroid, meshgrid + CGeoDistance(-extents.XRes() / 2, -extents.YRes() / 2, extents.GetPrjID()));
							double azimuthNE = ellipsoid.getAzimuth(centroid, meshgrid + CGeoDistance(extents.XRes() / 2, -extents.YRes() / 2, extents.GetPrjID()));
							if (azimuthNW > azimuthNE)
								azimuthNW -= 360;

							if (az[a] >= azimuthNW && az[a] <= azimuthNE)
								outer_pt = CGeoPointIndex(x, y);
						}
						else if (y == (blockSize.m_y - 1))//|| x == 0 || x == (blockSize.m_x - 1)
						{
							double azimuthSE = ellipsoid.getAzimuth(centroid, meshgrid + CGeoDistance(extents.XRes() / 2, extents.YRes() / 2, extents.GetPrjID()));
							double azimuthSW = ellipsoid.getAzimuth(centroid, meshgrid + CGeoDistance(-extents.XRes() / 2, extents.YRes() / 2, extents.GetPrjID()));
							if (az[a] >= azimuthSE && az[a] <= azimuthSW)
								outer_pt = CGeoPointIndex(x, y);
						}

					}
				}
			}

			
			direction[a] = (extents.XYPosToCoord(outer_pt) - centroid);
			double res = sqrt(square(extents.XRes() / 2) + square(extents.YRes() / 2));
			double F = max(1.0, EVALUATE_EACH_ARC /res); 
			//double nbSteps = sqrt(square(extents.m_xSize / 2) + square(extents.m_ySize / 2)) / ;
			//double nbSteps = sqrt(square(delta.m_x) + square(delta.m_y)) / sqrt(square(extents.XRes() / 2) + square(extents.YRes() / 2));
			double nbPixels = sqrt(square(extents.m_xSize / 2.0) + square(extents.m_ySize / 2.0));
			double nbSteps = nbPixels / F;
			direction[a].m_x /= nbSteps;
			direction[a].m_y /= nbSteps;
		}

		
		

//#pragma omp parallel for num_threads( m_options.BLOCK_CPU() ) if (m_options.m_bMulti) 
		for (int yy = 0; yy < blockSize.m_y; yy++)//blockSize.m_y
		{
			//int yy = blockSize.m_y / 2;
			for (int xx = 0; xx < blockSize.m_x; xx++)
			{
				//int xx = blockSize.m_x / 2;


				size_t xxyy = yy * blockSize.m_x + xx;


				CGeoPointIndex loc_pt(xx, yy);
				CGeoPoint3D loc;
				// Corresponding position and elevation
				((CGeoPoint&)loc) = extents.XYPosToCoord(loc_pt);
				loc.m_z = bandHolder.GetPixel(0, loc_pt.m_x, loc_pt.m_y);


				if (!outputData.empty() && bandHolder.IsValid(0, loc.m_z))
				{
					double res = sqrt(square(extents.XRes() / 2) + square(extents.YRes() / 2));


					double S = 0;
					double A = 0;
					bandHolder.GetWindow(0)->GetSlopeAndAspect(xx, yy, S, A);
					//convert slope in degree
					S = atan(S / 100) * 180 / pi;//convert slope in degree

					vector<double> F(az.size());
					CStatistic SVF;

					for (size_t a = 0; a < az.size(); a++)
					{


						//identify point in the outer pixel with the angle
						//CGeoPointIndex outer_pt(-1, -1);

						//for (int y = 0; y < blockSize.m_y && outer_pt.m_x == -1; y++)
						//{
						//	for (int x = 0; x < blockSize.m_x && outer_pt.m_x == -1; x++)
						//	{
						//		if (x != xx || y != yy)//skip evaluated pixel
						//		{

						//			if (y == 0 || x == 0 || y == (blockSize.m_y - 1) || x == (blockSize.m_x - 1))
						//			{
						//				size_t xy = y * extents.m_xSize + x;

						//				CGeoPoint3D meshgrid;
						//				((CGeoPoint&)meshgrid) = extents.XYPosToCoord(CGeoPointIndex(x, y));
						//				//meshgrid.m_z = bandHolder.GetPixel(0, x, y);

						//				if (x == 0)
						//				{
						//					double azimuthNW = ellipsoid.getAzimuth(loc, meshgrid + CGeoDistance(-extents.XRes() / 2, -extents.YRes() / 2, extents.GetPrjID()));
						//					double azimuthSW = ellipsoid.getAzimuth(loc, meshgrid + CGeoDistance(-extents.XRes() / 2, extents.YRes() / 2, extents.GetPrjID()));
						//					if (az[a] >= azimuthSW && az[a] <= azimuthNW)
						//						outer_pt = CGeoPointIndex(x, y);
						//				}
						//				else if (x == (blockSize.m_x - 1))
						//				{
						//					double azimuthNE = ellipsoid.getAzimuth(loc, meshgrid + CGeoDistance(extents.XRes() / 2, -extents.YRes() / 2, extents.GetPrjID()));
						//					double azimuthSE = ellipsoid.getAzimuth(loc, meshgrid + CGeoDistance(extents.XRes() / 2, extents.YRes() / 2, extents.GetPrjID()));
						//					if (az[a] >= azimuthNE && az[a] <= azimuthSE)
						//						outer_pt = CGeoPointIndex(x, y);
						//				}

						//				if (y == 0 )//|| x == 0 || x == (blockSize.m_x - 1)
						//				{
						//					double azimuthNW = ellipsoid.getAzimuth(loc, meshgrid + CGeoDistance(-extents.XRes() / 2, -extents.YRes() / 2, extents.GetPrjID()));
						//					double azimuthNE = ellipsoid.getAzimuth(loc, meshgrid + CGeoDistance(extents.XRes() / 2, -extents.YRes() / 2, extents.GetPrjID()));
						//					if (azimuthNW > azimuthNE)
						//						azimuthNW -= 360;

						//					if (az[a] >= azimuthNW && az[a] <= azimuthNE)
						//						outer_pt = CGeoPointIndex(x, y);
						//				}
						//				else if (y == (blockSize.m_y - 1) )//|| x == 0 || x == (blockSize.m_x - 1)
						//				{
						//					double azimuthSE = ellipsoid.getAzimuth(loc, meshgrid + CGeoDistance(extents.XRes() / 2, extents.YRes() / 2, extents.GetPrjID()));
						//					double azimuthSW = ellipsoid.getAzimuth(loc, meshgrid + CGeoDistance(-extents.XRes() / 2, extents.YRes() / 2, extents.GetPrjID()));
						//					if (az[a] >= azimuthSE && az[a] <= azimuthSW)
						//						outer_pt = CGeoPointIndex(x, y);
						//				}

						//			}
						//		}
						//	}
						//}

						//if (extents.IsInside(outer_pt))
						//{



							//size_t nb_eval = 0;
						CGeoPoint ray = loc;
						//double d = ((CGeoPoint&)loc).GetDistance(ray);

						//	CGeoPointIndex ray_pt = extents.CoordToXYPos(ray);
						//CGeoDistance direction = (extents.XYPosToCoord(outer_pt) - loc);
						//double nbSteps = sqrt(square(extents.m_xSize / 2) + square(extents.m_ySize / 2)) / 3;
						//direction.m_x /= nbSteps;
						//direction.m_y /= nbSteps;

						CGeoPointIndex last_pt = extents.CoordToXYPos(ray);

						CStatistic E;
						while (((CGeoPoint&)loc).GetDistance(ray) < distance )//&& extents.IsInside(ray)
						{
							ray += direction[a];
							CGeoPointIndex ray_pt = extents.CoordToXYPos(ray);
							if (ray_pt != last_pt)
							{
								size_t xy = ray_pt.m_y * extents.m_xSize + ray_pt.m_x;

								CGeoPoint3D meshgrid;
								((CGeoPoint&)meshgrid) = ray;
								meshgrid.m_z = bandHolder.GetPixel(0, (int)ray_pt.m_x, (int)ray_pt.m_y);

								if (bandHolder.IsValid(0, meshgrid.m_z))
								{
									float elevation = ellipsoid.getElevationAngle(loc, meshgrid);
									E += max(0.0f, elevation);

									//outputData[a][xy] = max(0.0f, elevation);
								}

								//ray = extents.XYPosToCoord(ray_pt);
								//d = ((CGeoPoint&)loc).GetDistance(ray);
							}
						}
						//CStatistic E;
						//while (d < distance && extents.IsInside(ray))
						//{
						//	CGeoPointIndex ray_pt = extents.CoordToXYPos(ray);
						//	size_t xy = ray_pt.m_y * extents.m_xSize + ray_pt.m_x;

						//	CGeoPoint3D meshgrid;
						//	((CGeoPoint&)meshgrid) = ray;
						//	meshgrid.m_z = bandHolder.GetPixel(0, (int)ray_pt.m_x, (int)ray_pt.m_y);

						//	if (nb_eval > 0 && bandHolder.IsValid(0, meshgrid.m_z))
						//	{
						//		float elevation = ellipsoid.getElevationAngle(loc, meshgrid);
						//		E += max(0.0f, elevation);

						//		//		if (elevation >= outputData[a][xy])
						//			//		outputData[a][xy] = max(0.0f, elevation);
						//	}

						//	double azimuthNW = ellipsoid.getAzimuth(loc, meshgrid + CGeoDistance(-extents.XRes() / 2, -extents.YRes() / 2, extents.GetPrjID()));
						//	double azimuthNE = ellipsoid.getAzimuth(loc, meshgrid + CGeoDistance(extents.XRes() / 2, -extents.YRes() / 2, extents.GetPrjID()));
						//	double azimuthSE = ellipsoid.getAzimuth(loc, meshgrid + CGeoDistance(extents.XRes() / 2, extents.YRes() / 2, extents.GetPrjID()));
						//	double azimuthSW = ellipsoid.getAzimuth(loc, meshgrid + CGeoDistance(-extents.XRes() / 2, extents.YRes() / 2, extents.GetPrjID()));


						//	if (az[a] >= azimuthNW && az[a] <= azimuthNE)
						//		ray.m_y -= extents.YRes();
						//	else if (az[a] >= azimuthNE && az[a] <= azimuthSE)
						//		ray.m_x += extents.XRes();
						//	else if (az[a] >= azimuthSE && az[a] <= azimuthSW)
						//		ray.m_y += extents.YRes();
						//	else if (az[a] >= azimuthSW && az[a] <= azimuthNW)
						//		ray.m_x -= extents.XRes();
						//	else
						//		ray.m_y -= extents.YRes();


						//	//ray = extents.XYPosToCoord(ray_pt);
						//	d = ((CGeoPoint&)loc).GetDistance(ray);
						//	nb_eval++;
						//}

						if (E[NB_VALUE] >= 1)//Need at least 1 elevations to be valid
						{
							outputData[a][xxyy] = E[HIGHEST];

							//From: Jeff Dozier(2021) Revisiting Topographic Horizons in the Era of Big Data and Parallel Computing
							//Geoscience and Remote Sensing Letters

							double AA = pi / 180 * A;//aspect from 0 to 360 (in rad)
							double SS = pi / 180 * S;//Slope (in rad)
							double AZ = pi / 180 * (az[a] + 180);//Azimuth from 0 to 360 (in rad)
							double HH = pi / 180 * outputData[a][xxyy];//horizon angle (in rad)

							if (SS > 0 && cos(AA - AZ) < 0)
								HH = max(HH, asin(sqrt(1.0 - 1.0 / (1 + square(cos(AA - AZ)) * square(tan(SS))))));

							F[a] = cos(SS) * square(cos(HH)) + (sin(SS) * cos(AA - AZ) * (pi / 2 - HH - sin(HH) * cos(HH)));
							assert(F[a] >= 0 && F[a] < 2);

							SVF += F[a];

							//outputData[a][xxyy] = F[a];
						}
						//}

					}

					outputData[az.size() + 0][xxyy] = S;
					outputData[az.size() + 1][xxyy] = A;

					size_t s1 = (size_t)(SVF[NB_VALUE]);
					size_t s2 = az.size();
					if (s1 >= s2/2)
					{
						outputData[az.size() + 2][xxyy] = SVF[MEAN];
					}



#pragma omp atomic 
					m_options.m_xx++;

				}//for x

				m_options.UpdateBar();

			}//for y



			m_options.m_timerProcess.Stop();
		}//critical

	}


	void CHorizonImage::WriteBlock(int xBlock, int yBlock, CBandsHolder& bandHolder, CGDALDatasetEx& outputDS, OutputData& outputData)
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
					GDALRasterBand* pBand = outputDS.GetRasterBand(z);
					if (!outputData.empty())
					{
						//for (int y = 0; y < (int)outputData[z].size(); y++)
						pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(outputData[z][0]), outputRect.Width(), outputRect.Height(), GDT_Float32, 0, 0);
					}
					else
					{
						OutputDataType noData = (OutputDataType)outputDS.GetNoData(z);
						pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &noData, 1, 1, GDT_Float32, 0, 0);
					}
				}
			}



			m_options.m_timerWrite.Stop();
		}
	}

	void CHorizonImage::CloseAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS)
	{
		inputDS.Close();
		maskDS.Close();

		m_options.m_timerWrite.Start();

		outputDS.Close(m_options);

		m_options.m_timerWrite.Stop();
		m_options.PrintTime();
	}






	// Default horizon viewing distance(50 km), precision(1°) and ellipsoid model(WGS84)
	CHorizon CHorizonImage::ComputeHorizon(CGeoPoint loc_in, CGDALDatasetEx& DEM, double distance, double precision, const string& ellipsoidModel)
	{
		// Check input arguments
		assert(distance > 0 && distance <= 500000);
		assert(precision > 0);


		vector<double> az;
		for (double i = -180; i < 180; i += precision)
			az.push_back(i);



		//vector<double> horizon = 0;
		//CGeoPoint
		CGeoExtents extents = DEM.GetExtents();



		// Initialization DEM resolution
		//double demResolution = (extents.XRes() + extents.YRes()) / 2;
		// Retrieve the DEM pixel corresponding to location
		//[~, yPosition] = min(abs(dem.latitude - location.m_lat));
		//[~, xPosition] = min(abs(dem.longitude - location.m_lon));

		CGeoPointIndex pt = extents.CoordToXYPos(loc_in);
		CGeoPoint3D loc;
		((CGeoPoint&)loc) = extents.XYPosToCoord(pt);
		// Maximum computation distance
		//vector<int> xPosition();
		//vector<int> yPosition();


		CEllipsoidProperties ellipsoid(ellipsoidModel);

		CGeoDistance delta = ellipsoid.dist2deg(distance / 1000.0, loc);
		//[latitudeDistance, longitudeDistance] = ellipsoid.dist2deg(distance, location);
		//int topoLatDistance = (int)round(delta.m_lat / extents.YRes());// Distance between observer and the further topography point located on a latitude isoline
		//int topoLonDistance = (int)round(delta.m_lon / extents.XRes());// Distance between observer and the further topography point located on a longitude isoline
		// Take into consideration possible out mapping : if maximum distance is out
		// of the dem area, limit to the dem area
		//double northAreaDistance = topoLatDistance - abs(min(0.0, yPosition - topoLatDistance - 1));
		//double southAreaDistance = topoLatDistance - max(0, yPosition + topoLatDistance - size(dem.elevation, 1));
		//double westAreaDistance = topoLonDistance - abs(min(0, xPosition - topoLonDistance - 1));
		//double eastAreaDistance = topoLonDistance - max(0, xPosition + topoLonDistance - size(dem.elevation, 2));
		//int northAreaDistance = topoLatDistance - abs(min(0, pt.m_y - topoLatDistance - 1));
		//int southAreaDistance = topoLatDistance - max(0, pt.m_y + topoLatDistance - extents.m_ySize);
		//int westAreaDistance = topoLonDistance - abs(min(0, pt.m_x - topoLonDistance - 1));
		//int eastAreaDistance = topoLonDistance - max(0, pt.m_x + topoLonDistance - extents.m_xSize);


		//CGeoExtents clip(location.GetPrjID());
		CGeoRect clip(loc.GetPrjID());
		clip.m_xMin = loc.m_lon - delta.m_lon;
		clip.m_xMax = loc.m_lon + delta.m_lon;
		clip.m_yMin = loc.m_lat - delta.m_lat;
		clip.m_yMax = loc.m_lat + delta.m_lat;

		clip = CGeoRect::IntersectRect(extents, clip);

		CGeoRectIndex rect = extents.CoordToXYPos(clip);
		CGeoExtents studyArea_extents(clip, rect.GetGeoSize());
		pt = studyArea_extents.CoordToXYPos(loc);

		vector<__int16> DEMe(rect.size());
		DEM.GetRasterBand(0)->RasterIO(GF_Read, rect.m_x, rect.m_y, rect.m_xSize, rect.m_ySize, &(DEMe[0]), rect.m_xSize, rect.m_ySize, GDT_Int16, 0, 0);
		// Retrieve specific dem area for horizon computation

		vector<CGeoPoint3D> meshgrid(studyArea_extents.GetNbPixels());
		vector<float> azimuthLo(studyArea_extents.GetNbPixels());
		vector<float> azimuthHi(studyArea_extents.GetNbPixels());

		for (size_t y = 0; y < studyArea_extents.m_ySize; y++)
		{
			for (size_t x = 0; x < studyArea_extents.m_xSize; x++)
			{
				size_t xy = y * studyArea_extents.m_xSize + x;
				((CGeoPoint&)meshgrid[xy]) = studyArea_extents.XYPosToCoord(CGeoPointIndex((int)x, (int)y));
				meshgrid[xy].m_z = DEMe[xy];


				//azimuth[xy] = ellipsoid.getAzimuth((pi / 180) * location.m_lat, (pi / 180) * location.m_lon,
					//(pi / 180) * (meshgrid[xy].m_lat /*- extents.YRes() / 2*/), (pi / 180) * (meshgrid[xy].m_lon /*+ extents.XRes() / 2*/));

				//azimuth[xy] = ellipsoid.getAzimuth(location, meshgrid[xy]);
				double azimuth1 = ellipsoid.getAzimuth(loc, meshgrid[xy] + CGeoDistance(extents.XRes() / 2, extents.YRes() / 2, extents.GetPrjID()));
				double azimuth2 = ellipsoid.getAzimuth(loc, meshgrid[xy] + CGeoDistance(extents.XRes() / 2, -extents.YRes() / 2, extents.GetPrjID()));
				double azimuth3 = ellipsoid.getAzimuth(loc, meshgrid[xy] + CGeoDistance(-extents.XRes() / 2, extents.YRes() / 2, extents.GetPrjID()));
				double azimuth4 = ellipsoid.getAzimuth(loc, meshgrid[xy] + CGeoDistance(-extents.XRes() / 2, -extents.YRes() / 2, extents.GetPrjID()));

				azimuthLo[xy] = min(azimuth1, min(azimuth2, min(azimuth3, azimuth4)));
				azimuthHi[xy] = max(azimuth1, max(azimuth2, max(azimuth3, azimuth4)));


				//adjust pixels on the North transition
				if (azimuthLo[xy] < -90 && azimuthHi[xy] > 90)
				{
					float Lo = azimuthHi[xy] - 360;
					azimuthHi[xy] = azimuthLo[xy];
					azimuthLo[xy] = Lo;
				}
				//if (azimuthSE[xy] < azimuthSW[xy])
					//azimuthSW[xy] -= 360;
			}
		}

		//determine all cells to compute elevation
		deque< vector<CGeoPoint>> pts(az.size());
		for (size_t a = 0; a < pts.size(); a++)
			pts[a].reserve(1000);

		for (size_t y = 0; y < studyArea_extents.m_ySize; y++)
		{
			for (size_t x = 0; x < studyArea_extents.m_xSize; x++)
			{
				for (size_t a = 0; a < az.size(); a++)
				{
					size_t xy = y * studyArea_extents.m_xSize + x;

					if (az[a] >= azimuthLo[xy] && az[a] <= azimuthHi[xy])
						pts[a].push_back(CGeoPoint(x, y));
				}
			}
		}

		// Specific azimuth values for NE(-180 to 90) and NW(90 to 180) areas
		//vector<double> azimuthNE = azimuth;
		//azimuthNE(1 : yObs, xObs) = azimuthNE(1 : yObs, xObs) - 360;
		//vector<double> azimuthNW = azimuth;
		//azimuthNW(1 : yObs, xObs + 1) = azimuthNW(1 : yObs, xObs + 1) + 360;
		// Corresponding elevation angle
		loc.m_z = DEMe[pt.m_y * studyArea_extents.m_xSize + pt.m_x];

		CHorizon horizon(az.size());
		deque < vector<double>> elevation(az.size());
		for (size_t a = 0; a < az.size(); a++)
		{
			elevation[a].resize(pts[a].size());
			for (size_t i = 0; i < pts[a].size(); i++)
			{
				size_t xy = pts[a][i].m_y * studyArea_extents.m_xSize + pts[a][i].m_x;
				double e = ellipsoid.getElevationAngle(loc, meshgrid[xy]);
				if (e > horizon[a])
					horizon[a] = max(0.0, e);

				elevation[a][i] = e;
			}
		}



		return horizon;
	}



	//// Sub functions








	// getElevationAngle 
	// compute angular elevation between 2 points of any  ellipsoid using ellipsoidal height and geographic coordinates(latitude and longitude)
	double CEllipsoidProperties::getElevationAngle(double h_A, double h_B, double latitude_A, double latitude_B, double longitude_A, double longitude_B)const
	{
		double alpha = 0;

		latitude_A *= (pi / 180);
		latitude_B *= (pi / 180);
		longitude_A *= (pi / 180);
		longitude_B *= (pi / 180);

		// Compute cartesian coordinates of point A and B located at altitude
		// h_A and h_B from the ellipsoid surface (ellipsoidal heights)
		CGeoPoint3D A = geographic2cartesian(latitude_A, longitude_A, h_A);
		CGeoPoint3D B = geographic2cartesian(latitude_B, longitude_B, h_B);
		// Scalar product between ABand normal to the point A
		double innerProduct = (B.m_x - A.m_x) * cos(longitude_A) * cos(latitude_A) + (B.m_y - A.m_y) *
			sin(longitude_A) * cos(latitude_A) + (B.m_z - A.m_z) * sin(latitude_A);
		// Angular elevation computation
		double norm = sqrt(square(B.m_x - A.m_x) + square(B.m_y - A.m_y) + square(B.m_z - A.m_z));
		alpha = asin(innerProduct / norm) * 180 / pi;

		return alpha;

	}


	// *GETAZIMUTH - compute azimuth between 2 points of the given ellipsoid
	double CEllipsoidProperties::getAzimuth(double lat1, double lon1, double lat2, double lon2)const
	{
		// Retrieve isometric latitudes
		double L1 = getIsometricLatitude((pi / 180) * lat1);
		double L2 = getIsometricLatitude((pi / 180) * lat2);
		// Compute azimuth
		double az = atan2((pi / 180) * (lon1 - lon2), (L1 - L2));
		return (180 / pi) * az;
	}

	// *GETISOMETRICLATITUDE - compute isometric latitude from geographic latitude
	double CEllipsoidProperties::getIsometricLatitude(double latitude)const
	{
		assert(latitude >= -90 && latitude <= 90);
		double isometricLatitude = 0;

		//[~, ~, ~, e] = getEllipsoidProperties(ellipsoidModel);
		double term1 = tan((pi / 4) + (latitude / 2));
		double num = 1 - eccentricity * sin(latitude);
		double denom = 1 + eccentricity * sin(latitude);
		double term2 = pow(num / denom, eccentricity / 2);

		// Result
		isometricLatitude = log(term1 * term2);
		return isometricLatitude;
	}

	// *GEOGRAPHIC2CARTESIAN - convert geographic coordinates to cartesian
	// coordinates, with respect to the ellipsoid model
	CGeoPoint3D CEllipsoidProperties::geographic2cartesian(double latitude, double longitude, double altitude)const
	{
		CGeoPoint3D pt;


		// Compute ellipsoid normal
		double N = semiMajorAxis / sqrt(1.0 - square(eccentricity) * (square(sin(latitude))));
		// Compute cartesian coordinates from geographic coordinates
		pt.m_x = (N + altitude) * cos(longitude) * cos(latitude);
		pt.m_y = (N + altitude) * sin(longitude) * cos(latitude);
		pt.m_z = (N * (1 - square(eccentricity)) + altitude) * sin(latitude);

		return pt;
	}

	// DIST2DEG - convert metric distance into angular distance using local sphere approximation
	CGeoDistance CEllipsoidProperties::dist2deg(double distance, const CGeoPoint& location)const
	{

		CGeoDistance delta;

		// Initialization
		double distanceEps = 1e-2;
		double latMin = 0;
		double latMax = 10;
		double lonMin = 0;
		double lonMax = 10;
		// Computation(dichotomy)
		while (true)
		{

			delta.m_lat = (latMin + latMax) / 2;
			delta.m_lon = (lonMin + lonMax) / 2;

			double distVarLat = geodesic(pi / 180 * location.m_lat, pi / 180 * (location.m_lat + delta.m_lat), 0);
			double distVarLon = geodesic(pi / 180 * location.m_lat, pi / 180 * location.m_lat, pi / 180 * delta.m_lon);

			if (abs(distVarLat - distance) < distanceEps && abs(distVarLon - distance) < distanceEps)
				break;


			if (distVarLat < distance)
				latMin = delta.m_lat;
			else
				latMax = delta.m_lat;


			if (distVarLon < distance)
				lonMin = delta.m_lon;
			else
				lonMax = delta.m_lon;

		}

		return delta;
	}

	//* GEODESIC - compute geodesic on ellipsoid using local sphere approximation
	double CEllipsoidProperties::geodesic(double lat1, double lat2, double deltaLon)const
	{
		double arc_AB = 0;
		// Compute local sphere radius
		double R = localSphereRadius(lat1);
		// Compute angle
		double gamma = acos(cos(lat1) * cos(lat2) * cos(deltaLon) + sin(lat1) * sin(lat2));
		// Compute great circle
		arc_AB = R * gamma;

		return arc_AB;
	}

	//LOCALSPHERERADIUS - retrieve radius of the local sphere approximation, tangent to the ellipsoid
	double CEllipsoidProperties::localSphereRadius(double latitude)const
	{
		double R = 0;
		// Get ellipsoid definition with respect to given model
		double p = ((semiMajorAxis * (1.0 - square(eccentricity))) / (pow(1.0 - square(eccentricity) * square(sin(latitude)), 3.0 / 2.0))) * 0.001;
		double N = (semiMajorAxis / sqrt(1.0 - square(eccentricity) * square(sin(latitude)))) * 0.001;
		// Compute radius
		R = sqrt(N * p);

		return R;
	}


	//function[semiMajorAxis, flattening, semiMinorAxis, eccentricity]


	CEllipsoidProperties::CEllipsoidProperties(string model)
	{
		// Retrieve ellipsoid properties from model
		//
		//Summary:
	// Retrieve ellipsoid parameters from model.At present time, only
		// WGS84, ETRS89and IERS are defined, but user can easily add other
		// ellipsoid definitions(semi major axisand flattening)
		//
		//Syntax :
		//* [semiMajorAxis, flattening, semiMinorAxis, eccentricity] = ELLIPSOIDMODEL(model)
		//
		//Description :
		// *ELLIPSOIDMODEL(model) - return specific ellipsoid parameters
		//
		//Inputs :
		// *MODEL - string corresponding to the model¹ name('WGS84', etc.)
		//
		//¹Currently implemented models :
	// -WGS84, ETRS89, IERS
		//
		// Outputs :
		//* SEMIMAJORAXIS - semi major axis of the given ellipsoid
		// *FLATTENING - flattening of the given ellipsoid
		// *SEMIMINORAXIS - sem minor axis of the given ellipsoid
		// *ECCENTRICITY - eccentricity of the given ellipsoid
		//
		//Examples:
	// *getEllipsoidProperties('WGS84') retrieves all fundamental
		// parameters(semi - major and semi - minos axes, flattening and
			//eccentricity) of the corresponding reference ellipsoid World
		// Geodetic System 1984 (NGA, 2000. Department of Defense World
			// Geodetic System 1984 – its definition and relationships with local
			// geodetic systems.Tech.Rep., National Geospatial - Intelligence
			// Agency, Springfield, USA.)
		//
		//See also :
	// *SUNMASK
		// Author : Dr.Benjamin Pillot
		// Address : Universidade do Vale do Rio dos Sinos(Unisinos), São
		// Leopoldo, RS, Brazil
		// email : benjaminfp@unisinos.br
		// Website: http://www.
	// Date : 03 - Oct - 2016; Last revision : 03 - Oct - 2016
		//
		//Copyright(c) 2016, Benjamin Pillot
		// All rights reserved.
		// Check input arguments
		//narginchk(0, 1);
		//if nargin == 0
	//			model = 'WGS84';
		//	end
			//	model = validatestring(model, { 'WGS84', 'ETRS89', 'IERS' });
			// model = validatestring(model, { 'WGS84', 'ETRS89', 'IERS', 'your_model' });
		ASSERT(model == "WGS84" || model == "ETRS89" || model == "IERS");


		double flattening = 0;
		if (model == "WGS84")// World Geodetic System 1984
		{

			flattening = 1 / 298.257223563;
			semiMajorAxis = 6378137;
		}
		else if (model == "ETRS89")
			//case 'ETRS89' // European Terrestrial Reference System 1989
		{
			flattening = 1 / 298.257222101;
			semiMajorAxis = 6378137;
		}
		else if (model == "IERS")
			//case 'IERS' // International Earth Rotation(2003)
		{
			flattening = 1 / 298.25642;
			semiMajorAxis = 6378136.6;
		}
		// case 'your_model'
			// Add here parameters of your ellipsoid model(flattening and semi
				// major axis)


		double semiMinorAxis = semiMajorAxis - semiMajorAxis * flattening;
		eccentricity = sqrt(1 - ((semiMinorAxis * semiMinorAxis) / (semiMajorAxis * semiMajorAxis)));
	}

	CEllipsoidProperties::CEllipsoidProperties(double semiMajorAxis, double flattening)
	{
		// Computation of semi - minor axis and eccentricity
		double semiMinorAxis = semiMajorAxis - semiMajorAxis * flattening;
		eccentricity = sqrt(1 - ((semiMinorAxis * semiMinorAxis) / (semiMajorAxis * semiMajorAxis)));

	}
}