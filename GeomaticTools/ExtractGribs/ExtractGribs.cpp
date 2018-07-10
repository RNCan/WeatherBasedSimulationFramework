//***********************************************************************
// program to merge Landsat image image over a period
//									 
//***********************************************************************
// version
// 1.1.1	22/05/2018	Rémi Saint-Amant	Compile with VS 2017
// 1.1.0	16/11/2017	Rémi Saint-Amant	Compile with GDAL 2.2
// 1.0.0	21/12/2015	Rémi Saint-Amant	Creation

//-te -1271940 7942380 -1249530 7956540 -of vrt -co "bigtiff=yes" -co "compress=LZW" -co "tiled=YES" -co "BLOCKXSIZE=1024" -co "BLOCKYSIZE=1024" -blocksize 1024 1024 -multi -Type SecondBest -stats -debug -dstnodata -32768 --config GDAL_CACHEMAX 1024 "U:\GIS1\LANDSAT_SR\LCC\2014\#57_2014_182-244.vrt" "U:\GIS\#documents\TestCodes\ExtractGribs\Test4\Nuage.vrt"
//-stats -Type BestPixel -te 1358100 6854400 1370300 6865500 -of VRT -ot Int16 -blockSize 1024 1024 -co "compress=LZW" -co "tiled=YES" -co "BLOCKXSIZE=1024" -co "BLOCKYSIZE=1024" --config GDAL_CACHEMAX 4096  -overview {2,4,8,16} -multi -IOCPU 3 -overwrite -Clouds "U:\GIS\#documents\TestCodes\ExtractGribs\Test2\Model\V4_SR_DTD1_cloudv4_skip100_200" "U:\GIS1\LANDSAT_SR\LCC\1999-2006.vrt" "U:\GIS\#documents\TestCodes\ExtractGribs\Test2\Output\Test.vrt"
//-stats -Type Oldest -TT OverallYears -of VRT -ot Int16 -blockSize 1024 1024 -co "compress=LZW" -co "tiled=YES" -co "BLOCKXSIZE=1024" -co "BLOCKYSIZE=1024" --config GDAL_CACHEMAX 4096  -overview {2,4,8,16} -multi -IOCPU 3 -overwrite "U:\GIS\#documents\TestCodes\BandsAnalyser\Test1\Input\Test1999-2014.vrt" "U:\GIS\#documents\TestCodes\ExtractGribs\Test0\output\Test.vrt"
//-RGB Natural -stats --config GDAL_CACHEMAX 4096  -overview {2,4,8,16} -multi -overwrite -of VRT -co "compress=LZW" -te 1718910 6620910 1751910 6652920 "D:\Travaux\MergeImage\input\2015-2016-2017.vrt" "D:\Travaux\MergeImage\input\subset\2015-2016-2017.vrt"


#include "stdafx.h"
#include <math.h>
#include <array>
#include <utility>
#include <iostream>

#include "ExtractGribs.h"
#include "Basic/OpenMP.h"
#include "Basic/UtilTime.h"
#include "Basic/UtilMath.h"
#include "Basic/CSV.h"


#pragma warning(disable: 4275 4251)
#include "gdal_priv.h"

using namespace std;

namespace WBSF
{


	const char* CExtractGribs::VERSION = "1.0.0";
	const int CExtractGribs::NB_THREAD_PROCESS = 2;


	//*********************************************************************************************************************

	CExtractGribsOption::CExtractGribsOption()
	{
		m_appDescription = "This software extract weather from gribs file.";
		m_format = "VRT";
		m_levels.push_back(0);
		m_outputType = GDT_Float32;
		m_bWS = false;
		m_bWD = false;

		AddOption("-Period");
			//	AddOption("-RGB");
		static const COptionDef OPTIONS[] =
		{
			{ "-Levels", 1, "{level1,level2,...}", false, "Altitude (in meters) over ground to extract weather. Surface (0 - 10) by default." },
			{ "-Vars", 1, "{var1,var2,...}", false, "Select variable to extract. Standard variable are: TMP, " },
			{ "-Loc", 1, "filePath", false, "File path for locations list to extract point instead of images. " },
			{ "-WS", 0, "", false, "Extract wind speed from u and v component." },
			{ "-WD", 0, "", false, "Extract wind direction from u and v component." },
			{ "gribsfile", 0, "", false, "Input image file path." },
			{ "dstfile", 0, "", false, "Output image/CSV file path. VRT format by default. Use option -of XYZ to extract images as csv." }
		};

		for (int i = 0; i < sizeof(OPTIONS) / sizeof(COptionDef); i++)
			AddOption(OPTIONS[i]);

		static const CIOFileInfoDef IO_FILE_INFO[] =
		{
			{ "Input Gribs", "gribsfile", "", "any number of weather layers", "", "" },
			{ "Output Image", "dstfile", "", "nbVariable*nbLevels", "", "" },
		};

		for (int i = 0; i < sizeof(IO_FILE_INFO) / sizeof(CIOFileInfoDef); i++)
			AddIOFileInfo(IO_FILE_INFO[i]);
	}

	ERMsg CExtractGribsOption::ParseOption(int argc, char* argv[])
	{
		ERMsg msg = CBaseOptions::ParseOption(argc, argv);

		ASSERT(NB_FILE_PATH == 2);


		if (msg && m_filesPath.size() != NB_FILE_PATH)
		{
			msg.ajoute("ERROR: Invalid argument line. 2 files are needed: the gribs source and destination image.");
			msg.ajoute("Argument found: ");
			for (size_t i = 0; i < m_filesPath.size(); i++)
				msg.ajoute("   " + to_string(i + 1) + "- " + m_filesPath[i]);
		}

		if (m_format == "XYZ")
			m_bOutputCSV = true;


		return msg;
	}

	ERMsg CExtractGribsOption::ProcessOption(int& i, int argc, char* argv[])
	{
		ERMsg msg;

		if (IsEqual(argv[i], "-Levels"))
		{
			string tmp = argv[++i];
			StringVector levels(tmp, "{,}");

			m_levels.clear();
			for (StringVector::const_iterator it = levels.begin(); it != levels.end(); it++)
			{
				if (!it->empty())
					m_levels.push_back(ToDouble(*it));
			}
		}
		else if (IsEqual(argv[i], "-Vars"))
		{
			string tmp = argv[++i];
			StringVector vars(tmp, "{,}");

			m_variables.clear();
			for (StringVector::iterator it = vars.begin(); it != vars.end(); it++)
			{
				Trim(*it);
				if (!it->empty())
					m_variables.push_back(*it);
			}
		}
		else if (IsEqual(argv[i], "-Loc"))
		{
			vector<double> m_elevation;

			m_locations_file_path = argv[++i];
		}
		else if (IsEqual(argv[i], "-WS"))
		{
			m_bWS = true;
		}
		else if (IsEqual(argv[i], "-WD"))
		{
			m_bWD = true;
		}
		else
		{

			//Look to see if it's a know base option
			msg = CBaseOptions::ProcessOption(i, argc, argv);
		}


		return msg;
	}


	ERMsg CExtractGribs::Execute()
	{
		ERMsg msg;

		if (!m_options.m_bQuiet)
		{
			cout << "Output: " << m_options.m_filesPath[CExtractGribsOption::OUTPUT_FILE_PATH] << endl;
			cout << "From:   " << m_options.m_filesPath[CExtractGribsOption::GRIBS_FILE_PATH] << endl;

			if (!m_options.m_maskName.empty())
				cout << "Mask:   " << m_options.m_maskName << endl;
		}

		GDALAllRegister();

		CGDALDatasetEx inputDS;
		CGDALDatasetEx maskDS;
		CGDALDatasetEx outputDS;

		CLocationVector locations;
		ofStream CSV_file_path;

		msg = OpenAll(inputDS, maskDS, locations, outputDS, CSV_file_path);


		CBandsHolderMT bandHolder(1, m_options.m_memoryLimit, m_options.m_IOCPU, NB_THREAD_PROCESS);
		if (msg && maskDS.IsOpen())
			bandHolder.SetMask(maskDS.GetSingleBandHolder(), m_options.m_maskDataUsed);

		if (msg)
			msg += bandHolder.Load(inputDS, m_options.m_bQuiet, m_options.GetExtents(), m_options.m_period);

		if (!msg)
			return msg;


		CGeoExtents extents = bandHolder.GetExtents();
		m_options.ResetBar((size_t)extents.m_xSize*extents.m_ySize);
		vector<pair<int, int>> XYindex = extents.GetBlockList(5, 5);

		vector < set<size_t>> imagesList(XYindex.size());

		if (!m_options.m_bQuiet && m_options.m_bCreateImage)
		{
			cout << "Create output images (" << outputDS.GetRasterXSize() << " C x " << outputDS.GetRasterYSize() << " R x " << outputDS.GetRasterCount() << " B) with " << m_options.m_CPU << " threads..." << endl;
		}

		m_options.ResetBar((size_t)extents.m_xSize*extents.m_ySize);

		omp_set_nested(1);//for IOCPU


#pragma omp parallel for schedule(static, 1) num_threads( NB_THREAD_PROCESS ) if (m_options.m_bMulti) 
		for (int b = 0; b < (int)XYindex.size(); b++)
		{
			int xBlock = XYindex[b].first;
			int yBlock = XYindex[b].second;

			int blockThreadNo = ::omp_get_thread_num();
//			imagesList[b] = GetImageList(xBlock, yBlock, bandHolder[blockThreadNo], inputDS);
			if (!imagesList[b].empty())
			{
#pragma omp critical(PreProcessBlock)
				{
	//				images.insert(imagesList[b].begin(), imagesList[b].end());
				}
			}
		}//for all blocks
		

		CloseAll(inputDS, maskDS, outputDS, CSV_file_path);

		return msg;
	}


	
	ERMsg CExtractGribs::OpenAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CLocationVector& locations, CGDALDatasetEx outputDS, ofStream& CSV_file)
	{
		ERMsg msg;

		if (!m_options.m_bQuiet)
			cout << endl << "Open input image..." << endl;

		msg = inputDS.OpenInputImage(m_options.m_filesPath[CExtractGribsOption::GRIBS_FILE_PATH], m_options);
		if (msg)
			inputDS.UpdateOption(m_options);


		if (msg && !m_options.m_bQuiet)
		{
			CGeoExtents extents = inputDS.GetExtents();
			CProjectionPtr pPrj = inputDS.GetPrj();
			string prjName = pPrj ? pPrj->GetName() : "Unknown";

			cout << "    Size           = " << inputDS.GetRasterXSize() << " cols x " << inputDS.GetRasterYSize() << " rows x " << inputDS.GetRasterCount() << " bands" << endl;
			cout << "    Extents        = X:{" << ToString(extents.m_xMin) << ", " << ToString(extents.m_xMax) << "}  Y:{" << ToString(extents.m_yMin) << ", " << ToString(extents.m_yMax) << "}" << endl;
			cout << "    Projection     = " << prjName << endl;
			cout << "    NbBands        = " << inputDS.GetRasterCount() << endl;
			cout << "    First image    = " << inputDS.GetPeriod().Begin().GetFormatedString() << endl;
			cout << "    Last image     = " << inputDS.GetPeriod().End().GetFormatedString() << endl;
			cout << "    Input period   = " << inputDS.GetPeriod().GetFormatedString() << endl;
		}

		if (msg && !m_options.m_maskName.empty())
		{
			if (!m_options.m_bQuiet)
				cout << "Open mask..." << endl;

			msg += maskDS.OpenInputImage(m_options.m_maskName);
		}

		if (m_options.m_bCreateImage)
		{
			CExtractGribsOption options(m_options);
			options.m_outputType = GDT_Int16;

			if (!m_options.m_bQuiet)
			{
				cout << endl;
				cout << "Open output images..." << endl;
				cout << "    Size           = " << options.m_extents.m_xSize << " cols x " << options.m_extents.m_ySize << " rows x " << options.m_nbBands << " bands" << endl;
				cout << "    Extents        = X:{" << ToString(options.m_extents.m_xMin) << ", " << ToString(options.m_extents.m_xMax) << "}  Y:{" << ToString(options.m_extents.m_yMin) << ", " << ToString(options.m_extents.m_yMax) << "}" << endl;
				cout << "    Output period   = " << m_options.m_period.GetFormatedString() << endl;
			}

			string filePath = options.m_filesPath[CExtractGribsOption::OUTPUT_FILE_PATH];
			/*string fileTitle = GetFileTitle(filePath);
			for (size_t b = 0; b < SCENES_SIZE; b++)
			{
				options.m_VRTBandsName += fileTitle + string("_") + Landsat::GetSceneName(b) + ".tif|";
			}*/


			msg += outputDS.CreateImage(filePath, options);

			if (msg && !options.m_bQuiet)
			{
				CTPeriod period = options.GetTTPeriod();
				cout << "    Output period:  " << period.GetFormatedString() << endl;
				cout << endl;
			}
		}


		return msg;
	}

	void CExtractGribs::ReadBlock(int xBlock, int yBlock, __int64 UTCWeatherTime, CGribsWeather& weather)
	{
#pragma omp critical(BlockIO)
		{
			m_options.m_timerRead.Start();

			CGeoExtents extents = m_options.m_extents.GetBlockExtents(xBlock, yBlock);
			weather.load(UTCWeatherTime);

			m_options.m_timerRead.Stop();
		}
	}

	

	void CExtractGribs::ProcessBlock(int xBlock, int yBlock, __int64 UTCWeatherTime, CGribsWeather& weather, OutputData& outputData)
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

		__int16 dstNodata = (__int16)m_options.m_dstNodata;
		outputData.resize(imagesList.size());
		for (size_t i = 0; i < outputData.size(); i++)
		{
			outputData[i].resize(SCENES_SIZE);
			for (size_t z = 0; z < outputData[i].size(); z++)
				outputData[i][z].resize(blockSize.m_x*blockSize.m_y, dstNodata);
		}



#pragma omp critical(ProcessBlock)
		{
			m_options.m_timerProcess.Start();

			for (int y = 0; y < blockSize.m_y; y++)
			{
				for (int x = 0; x < blockSize.m_x; x++)
				{
					for (auto it = imagesList.begin(); it != imagesList.end(); it++)
					{
						size_t i = std::distance(imagesList.begin(), it);
						CLandsatPixel pixel = window.GetPixel(*it, x, y);
						if (pixel[JD] == -1)
							pixel[JD] = (__int16)(m_options.m_bAlpha - dstNodata);

						for (size_t z = 0; z < SCENES_SIZE; z++)
							outputData[i][z][y*blockSize.m_x + x] = pixel[z];
					}

#pragma omp atomic 
					m_options.m_xx++;
				}


				m_options.UpdateBar();
			}
			m_options.m_timerProcess.Stop();
		}
	}

	void CExtractGribs::WriteBlock(int xBlock, int yBlock, CGDALDatasetEx& outputDS, ofStream& CSV_file, OutputData& outputData)
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

				__int16 noData = (__int16)m_options.m_dstNodata;
				for (size_t s = 0; s < outputDS.GetRasterCount() / SCENES_SIZE; s++)
				{
					for (size_t z = 0; z < SCENES_SIZE; z++)
					{
						GDALRasterBand *pBand = outputDS.GetRasterBand(s*SCENES_SIZE + z);
						if (!outputData.empty())
							pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(outputData[s][z][0]), outputRect.Width(), outputRect.Height(), GDT_Int16, 0, 0);
						else
							pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &noData, 1, 1, GDT_Int16, 0, 0);
					}
				}
			}


			m_options.m_timerWrite.Stop();
		}
	}

	void CExtractGribs::CloseAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS, ofStream& CSV_file)
	{
		inputDS.Close();
		maskDS.Close();

		m_options.m_timerWrite.Start();

		outputDS.Close(m_options);
		CSV_file.close();

		m_options.m_timerWrite.Stop();
		m_options.PrintTime();
	}


	//*******************************************************************************************************************



	CATMVariables CGribsWeather::get_weather(const CGeoPoint3D& pt, __int64 UTCWeatherTime, __int64 UTCCurrentTime)const
	{
		ASSERT(pt.IsGeographic());


		CATMVariables w1;
		CGeoPoint3D pt2(pt);


		size_t prjID = m_p_weather_DS.GetPrjID(UTCWeatherTime);
		ASSERT(prjID != NOT_INIT);
		pt2.Reproject(m_world.m_GEO2.at(prjID));


		CATMWeatherCuboidsPtr p_cuboid = get_cuboids(pt2, UTCWeatherTime);
		weather = p_cuboid->get_weather(pt2, UTCCurrentTime);

		return weather;
	}



	CGeoPointIndex CGribsWeather::get_xy(const CGeoPoint& ptIn, __int64 UTCWeatherTime)const
	{
		CGeoExtents extents = m_p_weather_DS.GetExtents(UTCWeatherTime);
		CGeoPoint pt = ptIn - CGeoDistance(extents.XRes() / 2, extents.YRes() / 2, extents.GetPrjID());

		return extents.CoordToXYPos(pt);//take the lower

	}

	//pt.m_z is flight hight above ground
	size_t CGribsWeather::get_level(const CGeoPointIndex& xy, const CGeoPoint3D& pt, __int64 UTCWeatherTime, bool bLow)const
	{
		size_t L = NOT_INIT;


		//CGeoPoint3D pt(ptIn);
		vector<pair<double, int>> test;

		//in some product, geopotentiel hight is above ground
		//in other product, geopotentiel hight is above sea level

		double grAlt = 0;
		if (m_bHgtOverSea)
		{
			grAlt = GetFirstAltitude(xy, UTCWeatherTime);//get the first level over the ground

			if (grAlt <= -999)
				grAlt = m_world.GetGroundAltitude(pt);
		}
		ASSERT(grAlt > -999);
		if (grAlt > -999)
		{
			//if (firstAlt == 0)//if the geopotentiel hight is above ground level, we have to substract grouind level to elevation
			//pt.m_z = max(0.0, pt.m_z -grAlt);

			//if (grAlt > -999)
			//test.push_back(make_pair(grAlt, 0));


			for (int l = 0; l < NB_LEVELS; l++)
			{
				size_t b = m_p_weather_DS.get_band(UTCWeatherTime, ATM_HGT, l);
				if (b != NOT_INIT)
				{
					double gph = m_p_weather_DS.GetPixel(UTCWeatherTime, b, xy); //geopotential height [m]
					if (gph > -999)
						test.push_back(make_pair(gph, l));

					if (l != 0 && gph > (grAlt + pt.m_alt))
						break;
				}
				else if (grAlt > -999)
				{
					//see if it's a fixed high layer
					double elev = 0;
					if (m_p_weather_DS.get_fixed_elevation_level(UTCWeatherTime, l, elev))
						test.push_back(make_pair(grAlt + elev, l));
				}
			}



			sort(test.begin(), test.end());


			if (test.size() >= 2)
			{
				for (size_t l = 0; l < test.size(); l++)
				{
					if (pt.m_alt < (test[l].first - grAlt))
					{
						L = test[bLow ? (l == 0 ? 0 : l - 1) : l].second;
						break;
					}
				}
			}
		}

		ASSERT(L == NOT_INIT || L < NB_LEVELS);
		return L;
	}



	double CGribsWeather::GetFirstAltitude(const CGeoPointIndex& xy, __int64 UTCWeatherTime)const
	{
		size_t b = m_p_weather_DS.get_band(UTCWeatherTime, ATM_HGT, 0);

		if (b == NOT_INIT)
			return -999;


		double gph = m_p_weather_DS.GetPixel(UTCWeatherTime, b, xy); //geopotential height [m]

		return gph;
	}

	CGeoPoint3DIndex CGribsWeather::get_xyz(const CGeoPoint3D& pt, __int64 UTCWeatherTime)const
	{
		CGeoExtents extents = m_p_weather_DS.GetExtents(UTCWeatherTime);
		CGeoPoint3DIndex xyz;
		((CGeoPointIndex&)xyz) = extents.CoordToXYPos(pt + CGeoDistance3D(extents.XRes() / 2, extents.YRes() / 2, 0, extents.GetPrjID()));


		xyz.m_z = MAX_GEOH - 1;//take the last level (~2000m) on by default

		for (size_t l = 0; l < NB_LEVELS; l++)
		{
			size_t b = m_p_weather_DS.get_band(UTCWeatherTime, ATM_HGT, l);
			double gph = m_p_weather_DS.GetPixel(UTCWeatherTime, b, xyz); //geopotential height [m]
			if (l == 0)//if the point is lower than 5 meters of the surface, we take surface
				gph += 5;

			if (pt.m_alt <= gph)
			{
				xyz.m_z = int(l);
				break;
			}
		}

		return xyz;
	}

	//ptIn have elevation above ground
	CATMWeatherCuboidsPtr CGribsWeather::get_cuboids(const CGeoPoint3D& pt, __int64 UTCWeatherTime)const
	{
		ASSERT(IsLoaded(UTCWeatherTime));
		ASSERT(pt.m_z >= 0);

		CATMWeatherCuboidsPtr cuboids(new CATMWeatherCuboids);
		cuboids->m_bUseSpaceInterpolation = m_world.m_world_param.m_bUseSpaceInterpolation;
		cuboids->m_bUseTimeInterpolation = m_world.m_world_param.m_bUseTimeInterpolation;


		ASSERT(IsLoaded(UTCWeatherTime));
		if (!IsLoaded(UTCWeatherTime))
			return cuboids;//return empty cuboid

						   //fill cuboid
		for (size_t i = 0; i < TIME_SIZE; i++, UTCWeatherTime = GetNextTime(UTCWeatherTime))
		{
			(*cuboids)[i].m_time = UTCWeatherTime;//reference in second

			if (i == 1 && !IsLoaded(UTCWeatherTime))
			{
				(*cuboids)[1] = (*cuboids)[0];
				return cuboids;
			}

			const CGeoExtents& extents = m_p_weather_DS.GetExtents(UTCWeatherTime);
			CGeoPointIndex xy1 = get_xy(pt, UTCWeatherTime);

			for (size_t z = 0; z < NB_POINTS_Z; z++)
			{
				for (size_t y = 0; y < NB_POINTS_Y; y++)
				{
					for (size_t x = 0; x < NB_POINTS_X; x++)
					{
						CGeoPointIndex xy2 = xy1 + CGeoPointIndex((int)x, (int)y);
						if (xy2.m_x < extents.m_xSize &&
							xy2.m_y < extents.m_ySize)
						{
							size_t L = get_level(xy2, pt, UTCWeatherTime, z == 0);
							if (L < MAX_GEOH)
							{
								double groundAlt = 0;

								//RUC is above sea level and WRF must be above sea level
								if (m_bHgtOverSea)
								{
									groundAlt = GetFirstAltitude(xy2, UTCWeatherTime);//get the first level over the ground
									if (groundAlt <= -999)
										groundAlt = m_world.GetGroundAltitude(pt);
								}



								((CGeoPoint&)(*cuboids)[i].m_pt[z][y][x]) = extents.GetPixelExtents(xy2).GetCentroid();//Get the center of the cell

								size_t bGph = m_p_weather_DS.get_band(UTCWeatherTime, ATM_HGT, L);
								double gph = m_p_weather_DS.GetPixel(UTCWeatherTime, bGph, xy2); //geopotential height [m]
								(*cuboids)[i].m_pt[z][y][x].m_z = gph - groundAlt;//groundAlt is equal 0 when is over ground
								ASSERT(z == 1 || pt.m_z >= (*cuboids)[i].m_pt[0][y][x].m_z);
								ASSERT(z == 0 || pt.m_z <= (*cuboids)[i].m_pt[1][y][x].m_z);



								for (size_t v = 0; v < ATM_WATER; v++)
								{

									bool bConvertVVEL = false;
									size_t b = m_p_weather_DS.get_band(UTCWeatherTime, v, L);

									if (b == UNKNOWN_POS && v == ATM_WNDW)
									{
										b = m_p_weather_DS.get_band(UTCWeatherTime, ATM_VVEL, L);
										if (b != NOT_INIT)
											bConvertVVEL = true;
									}


									if (b != UNKNOWN_POS)
									{
										(*cuboids)[i][z][y][x][v] = m_p_weather_DS.GetPixel(UTCWeatherTime, b, xy2);
										if (v == ATM_PRCP)
											(*cuboids)[i][z][y][x][v] *= 3600; //convert mm/s into mm/h


										if (bConvertVVEL)
											(*cuboids)[i][z][y][x][v] = CATMVariables::get_Uw((*cuboids)[i][z][y][x][ATM_PRES] * 100, (*cuboids)[i][z][y][x][ATM_TAIR], (*cuboids)[i][z][y][x][ATM_WNDW]);//convert VVEL into W
									}
									else
									{

										if (v == ATM_PRES)
										{
											assert(L > 0 && L <= MAX_GEOH);
											double P = 1013 * pow((293 - 0.0065*gph) / 293, 5.26);//pressure in hPa

											(*cuboids)[i][z][y][x][v] = P;
										}
										else if (v == ATM_WNDW)
										{
											//humm : HRDPS have only few VVEL
											(*cuboids)[i][z][y][x][v] = 0;
										}

									}

									_ASSERTE(!_isnan((*cuboids)[i][z][y][x][v]));
								}//variable
							}//if valid level
						}//if valid position
					}//x
				}//y
			}//z
		}//for t1 and t2

		assert((*cuboids)[0].m_time < (*cuboids)[1].m_time);

		return cuboids;
	}


	string CGribsWeather::get_image_filepath(__int64 UTCWeatherTime)const
	{
		TTimeFilePathMap::const_iterator it = m_filepath_map.find(UTCWeatherTime);
		ASSERT(it != m_filepath_map.end());

		return GetAbsolutePath(GetPath(m_filePathGribs), it->second);
	}

	ERMsg CGribsWeather::open(const std::string& filepath, CCallback& callback)
	{
		ERMsg msg;

		ifStream file;
		msg = file.open(filepath);
		if (msg)
		{
			//init max image to load at the sime time
			std::ios::pos_type length = file.length();
			callback.PushTask("Load Gribs", length);

			for (CSVIterator loop(file); loop != CSVIterator() && msg; ++loop)
			{
				if ((*loop).size() == 2)
				{
					StringVector tmp((*loop)[0], "/- :");
					ASSERT(tmp.size() >= 4 && tmp.size() <= 6);
					if (tmp.size() >= 4)
					{
						tm timeinfo = { 0 };

						if (tmp.size() == 6)
							timeinfo.tm_sec = ToInt(tmp[5]);     // seconds after the minute - [0,59] 

						if (tmp.size() >= 5)
							timeinfo.tm_min = ToInt(tmp[4]);     // minutes after the hour - [0,59] 

						timeinfo.tm_hour = ToInt(tmp[3]);    // hours since midnight - [0,23] 
						timeinfo.tm_mday = ToInt(tmp[2]);    // day of the month - [1,31] 
						timeinfo.tm_mon = ToInt(tmp[1]) - 1;     // months since January - [0,11] 
						timeinfo.tm_year = ToInt(tmp[0]) - 1900;

						__int64 UTCTime = _mkgmtime(&timeinfo);
						m_filepath_map[UTCTime] = (*loop)[1];
					}
					else
					{
						callback.AddMessage("Bad time format " + (*loop)[1]);
					}
				}

				msg += callback.SetCurrentStepPos((double)file.tellg());
			}

			if (msg)
				m_filePathGribs = filepath;


			callback.PopTask();
		}

		return msg;

	}

	ERMsg CGribsWeather::Discard(CCallback& callback)
	{
		return m_p_weather_DS.Discard(callback);
	}


	ERMsg CGribsWeather::load(__int64 UTCWeatherTime, CCallback& callback)
	{
		ERMsg msg;

		if (!m_filePathGribs.empty())
		{
			if (!m_p_weather_DS.IsLoaded(UTCWeatherTime))
			{
				string filePath = get_image_filepath(UTCWeatherTime);
				ASSERT(!filePath.empty());
				msg = m_p_weather_DS.load(UTCWeatherTime, get_image_filepath(UTCWeatherTime), callback);
			}
		}
	}

	__int64 CGribsWeather::GetNearestFloorTime(__int64 UTCTime)const
	{
		__int64 first = 0;

		
		ASSERT(!m_filepath_map.empty());
		if (!m_filepath_map.empty())
		{

			TTimeFilePathMap::const_iterator hi = m_filepath_map.upper_bound(UTCTime);
			if (hi == m_filepath_map.begin())
				first = max(first, hi->first);

			if (hi == m_filepath_map.end())
				first = max(first, m_filepath_map.rbegin()->first);
			else
				first = max(first, (--hi)->first);
		}
		else
		{
			first = 0;
		}
		

		ASSERT(first != LLONG_MAX);
		return first;
	}


	__int64 CGribsWeather::GetNextTime(__int64 UTCTime)const
	{
		__int64 next = LLONG_MAX;
		
		ASSERT(!m_filepath_map.empty());
		if (!m_filepath_map.empty())
		{
			TTimeFilePathMap::const_iterator hi = m_filepath_map.upper_bound(UTCTime);
			if (hi == m_filepath_map.end())
				next = min(next, m_filepath_map.rbegin()->first);
			else
				next = min(next, hi->first);
		}
		else
		{
			next = 0;
		}
		

		ASSERT(next != LLONG_MAX);
		return next;
	}

	CTPeriod CGribsWeather::GetEntireTPeriod()const
	{
		CTPeriod p;
		
		ASSERT(!m_filepath_map.empty());
		if (!m_filepath_map.empty())
		{
			for (TTimeFilePathMap::const_iterator it = m_filepath_map.begin(); it != m_filepath_map.end(); it++)
			{
				__int64 UTCTime = it->first;
				p += CTimeZones::Time2TRef(UTCTime).as(CTM::DAILY);
			}
		}

		return p;
	}

	size_t CGribsWeather::GetGribsPrjID(__int64 UTCWeatherTime)const
	{
		size_t prjID = NOT_INIT;

		if (!m_p_weather_DS.IsLoaded(UTCWeatherTime))
			m_p_weather_DS.load(UTCWeatherTime, get_image_filepath(UTCWeatherTime), CCallback());

		prjID = m_p_weather_DS.GetPrjID(UTCWeatherTime);
		
		return prjID;
	}

}