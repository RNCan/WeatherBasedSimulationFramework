//***********************************************************************
// program to merge Landsat image image over a period
//									 
//***********************************************************************
// version
// 1.0.0	13/07/2018	Rémi Saint-Amant	Creation

//-of VRT -stats -overview {2,4,8,16} -te 2058840 2790270 2397465 3074715 -period "2018-07-16-00" "2018-07-16-05" -var WNDS -var WNDD  -overwrite --config GDAL_CACHEMAX 1024 -co "compress=LZW" "D:\Travaux\Dispersal2018\Weather\Test.Gribs" "D:\Travaux\Dispersal2018\Weather\output2.vrt"
//-of XYZ -te 2058840 2790270 2397465 3074715 -period "2018-07-16-00" "2018-07-16-05" -var WNDS -var WNDD --config GDAL_CACHEMAX 1024 -co "compress=LZW" "D:\Travaux\Dispersal2018\Weather\Test.Gribs" "D:\Travaux\Dispersal2018\Weather\output2.csv"
//--config GDAL_CACHEMAX 1024 -co "compress=LZW" -var WNDS -var WNDD -Levels "0,200,400,600,800,1000" -loc "D:\Travaux\Dispersal2018\Loc\helikite.csv" "D:\Travaux\Dispersal2018\Weather\Test.Gribs" "D:\Travaux\Dispersal2018\Weather\output1.csv"


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
	enum TGEVar { GR_TAIR, GR_PRCP, GR_WNDU, GR_WNDV, GR_WNDS, GR_WNDD, NB_GE_VARIABLES };
	static const char* VAR_NAME[NB_GE_VARIABLES] = { "TAIR", "PRCP", "WNDU", "WNDV", "WNDS", "WNDD" };
	size_t GetVariable(const string& str)
	{
		size_t type = NOT_INIT;
		for (size_t i = 0; i < NB_GE_VARIABLES && type == NOT_INIT; i++)
			if (IsEqualNoCase(str, VAR_NAME[i]))
				type = i;

		return type;
	}


	//*********************************************************************************************************************

	CExtractGribsOption::CExtractGribsOption()
	{
		m_appDescription = "This software extract weather from gribs file.";
		m_format = "XYZ";
		m_levels.push_back(0);
		m_outputType = GDT_Float32;
		m_time_step = 3600;

		static const COptionDef OPTIONS[] =
		{
			{ "-Levels", 1, "{level1,level2,...}", false, "Altitude (in meters) over ground to extract weather. Surface (0 - 10) by default." },
			{ "-Var", 1, "var", true, "Select variable to extract. Variable available are: TAIR, PRCP, WNDU, WNDV, WNDS, WNDD" },
			{ "-Loc", 1, "filePath", false, "File path for locations list to extract point instead of images. " },
			{ "-SubHourly", 1, "seconds", false, "Output frequency. 3600 s (hourly) by default." },
			{ "-Period", 2, "begin end", false, "Period (in UTC) to extract. Format of date must be \"yyyy-mm-dd-hh\"."},
			{ "-Tonight", 2, "first last", false, "first and last hour (in UTC) to extract." },
			//{ "-units", 2, "var unit", false, "Period (in UTC) to extract. Format of date must be \"yyyy-mm-dd-hh\"." },
			//{ "-WS", 0, "", false, "Extract wind speed from u and v component." },
			//{ "-WD", 0, "", false, "Extract wind direction from u and v component." },
			{ "gribsfile", 0, "", false, "Input image file path." },
			{ "dstfile", 0, "", false, "Output image/CSV file path. XYZ (.csv) format by default. Use option -of VRT to extract images." }
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

		if (!m_locations_file_path.empty())
		{
			m_bCreateImage = false;
			m_format = "XYZ";
		}


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
		else if (IsEqual(argv[i], "-Var"))
		{
			string tmp = argv[++i];
			size_t type = GetVariable(tmp);
			if (type != NOT_INIT)
				m_variables.push_back(type);
			else
				msg.ajoute(string(tmp) + " is not a valid variable");

			/*StringVector vars(tmp, "{,}");

		m_variables.clear();
		for (StringVector::iterator it = vars.begin(); it != vars.end(); it++)
		{
			Trim(*it);
			if (!it->empty())
				m_variables.push_back(*it);
		}*/
		}
		else if (IsEqual(argv[i], "-Loc"))
		{
			vector<double> m_elevation;

			m_locations_file_path = argv[++i];
		}
		else if (IsEqual(argv[i], "-SubHourly"))
		{
			m_time_step = std::atoi(argv[++i]);
		}
		else if (IsEqual(argv[i], "-Tonight"))
		{
			int f = std::atoi(argv[++i]);
			int l = std::atoi(argv[++i]);
			CTRef now = CTRef::GetCurrentTRef(CTM::HOURLY);

			int df = f - (int)now.GetHour();
			int dl = l - (int)now.GetHour();
			if (df < 0)
				df += 24;
			if (dl < 0)
				dl += 24; 

			m_period = CTPeriod(now + df, now + dl);
		}
		else
		{
			//Look to see if it's a know base option
			msg = CBaseOptions::ProcessOption(i, argc, argv);
		}

		return msg;
	}

	bool AtLeastOnePointIn(const CGeoExtents& blockExtents, const CLocationVector& locations)
	{
		bool bAtLeastOne = false;
		for (size_t xy = 0; xy < locations.size() && !bAtLeastOne; xy++)
		{
			bAtLeastOne = blockExtents.IsInside(locations[xy]);
		}
			

		return bAtLeastOne;
	}


	ERMsg CExtractGribs::Execute()
	{
		ERMsg msg;

		if (!m_options.m_bQuiet)
		{
			cout << "Output: " << m_options.m_filesPath[CExtractGribsOption::OUTPUT_FILE_PATH] << endl;
			cout << "From:   " << m_options.m_filesPath[CExtractGribsOption::GRIBS_FILE_PATH] << endl;
		}

		GDALAllRegister();

		CGribsWeather weather;
		CGDALDatasetEx outputDS;

		CLocationVector locations;
		ofStream CSV_file;

		msg = OpenAll(weather, locations, outputDS, CSV_file);


		//CBandsHolderMT bandHolder(1, m_options.m_memoryLimit, m_options.m_IOCPU, NB_THREAD_PROCESS);
		//if (msg && maskDS.IsOpen())
		//	bandHolder.SetMask(maskDS.GetSingleBandHolder(), m_options.m_maskDataUsed);

		//if (msg)
		//	msg += bandHolder.Load(inputDS, m_options.m_bQuiet, m_options.GetExtents(), m_options.m_period);

		if (!msg)
			return msg;




		//load image
		for (size_t t = 0; t < m_options.m_times.size(); t++)
		{
			weather.load(weather.GetNearestFloorTime(m_options.m_times[t]));
			weather.load(weather.GetNextTime(m_options.m_times[t]));
		}

		cout << "     nb variables: " << m_options.m_variables.size() << endl;
		cout << "     nb levels:    " << m_options.m_levels.size() << endl;
		cout << "     nb times:     " << m_options.m_times.size() << endl;
		cout << "     extents:      X:{" << ToString(m_options.m_extents.m_xMin) << ", " << ToString(m_options.m_extents.m_xMax) << "}  Y:{" << ToString(m_options.m_extents.m_yMin) << ", " << ToString(m_options.m_extents.m_yMax) << "}" << endl;
		cout << "     period:       " << m_options.m_period.GetFormatedString() << endl;

		if (m_options.m_format == "XYZ")
		{
			
			cout << "     nb locations: " << locations.size() << endl;
			cout << "Create locations output" << endl;;
	//		if (AtLeastOnePointIn(blocExtents, locations))
		//	{
			OutputData outputData(m_options.m_variables.size()*m_options.m_levels.size()*m_options.m_times.size());
			for (size_t i = 0; i < outputData.size(); i++)
				outputData[i].resize(locations.size(), -999);

			ProcessBlock(locations, weather, outputData);
			WriteBlock(locations, outputData, CSV_file);
//			//}

		}
		else
		{
			if (!m_options.m_bQuiet && m_options.m_bCreateImage)
				cout << "     size:         " << outputDS.GetRasterXSize() << " cols x " << outputDS.GetRasterYSize() << " rows x " << outputDS.GetRasterCount() << " bands" << endl;

			cout << "Create image output" << endl;


			CGeoExtents extents = m_options.GetExtents();
			
			//m_options.ResetBar((size_t)extents.m_xSize*extents.m_ySize);
			vector<pair<int, int>> XYindex = extents.GetBlockList(5, 5);
			m_options.ResetBar((size_t)extents.m_xSize*extents.m_ySize*m_options.m_variables.size()*m_options.m_levels.size()*m_options.m_times.size());

			omp_set_nested(1);//for IOCPU

	//#pragma omp parallel for schedule(static, 1) num_threads( NB_THREAD_PROCESS ) if (m_options.m_bMulti) 
			for (int b = 0; b < (int)XYindex.size(); b++)
			{
				int xBlock = XYindex[b].first;
				int yBlock = XYindex[b].second;
				int blockThreadNo = ::omp_get_thread_num();

				CGeoExtents blocExtents = extents.GetBlockExtents(xBlock, yBlock);
				CGeoSize blockSize = blocExtents.GetSize();
				float dstNodata = -999;// (float)m_options.m_dstNodata;

				OutputData outputData(m_options.m_times.size()*m_options.m_levels.size()*m_options.m_variables.size());
				for (size_t i = 0; i < outputData.size(); i++)
					outputData[i].resize(blockSize.m_x*blockSize.m_y, dstNodata);

				for (size_t t = 0; t < m_options.m_times.size(); t++)
				{
					__int64 UTCWeatherTime = weather.GetNearestFloorTime(m_options.m_times[t]);
					weather.read_block(xBlock, yBlock, UTCWeatherTime);
					UTCWeatherTime = weather.GetNextTime(m_options.m_times[t]);
					weather.read_block(xBlock, yBlock, UTCWeatherTime);
				}

				ProcessBlock(xBlock, yBlock, weather, outputData);
				WriteBlock(xBlock, yBlock, outputData, outputDS);

			}//for all blocks
		}

		CloseAll(weather, outputDS, CSV_file);

		return msg;
	}



	ERMsg CExtractGribs::OpenAll(CGribsWeather& weather, CLocationVector& locations, CGDALDatasetEx& outputDS, ofStream& CSV_file)
	{
		ERMsg msg;

		if (!m_options.m_bQuiet)
			cout << endl << "Open input image..." << endl;

		msg = weather.open(m_options.m_filesPath[CExtractGribsOption::GRIBS_FILE_PATH], m_options);

		if (msg)
		{

			weather.UpdateOption(m_options);

			m_options.m_times.clear();
			__int64 time_start = CTimeZones::UTCTRef2UTCTime(m_options.m_period.Begin());//period ar in UTC
			__int64 time_end = CTimeZones::UTCTRef2UTCTime(m_options.m_period.End());//period ar in UTC

			for (size_t t = 0; t <= (time_end - time_start) / m_options.m_time_step; t++)
			{
				//CTRef UTCTRef = CTimeZones::LocalTRef2UTCTRef(m_options.m_period[t]);
				m_options.m_times.push_back(time_start + t * m_options.m_time_step);
			}


			CGeoExtents weather_extents = weather.GetExtents();
			CProjectionPtr pPrj = CProjectionManager::GetPrj(weather.GetPrjID());
			string prjName = pPrj ? pPrj->GetName() : "Unknown";

			cout << "    weather size       = " << weather_extents.m_xSize << " cols x " << weather_extents.m_ySize << " rows x " << weather.GetRasterCount() << " bands" << endl;
			cout << "    weather extents    = X:{" << ToString(weather_extents.m_xMin) << ", " << ToString(weather_extents.m_xMax) << "}  Y:{" << ToString(weather_extents.m_yMin) << ", " << ToString(weather_extents.m_yMax) << "}" << endl;
			cout << "    weather projection = " << prjName << endl;
			cout << "    weather period     = " << weather.GetEntireTPeriod().GetFormatedString() << endl;

			size_t prjID = weather.GetPrjID();
			m_options.toWea = GetReProjection(PRJ_WGS_84, prjID);
			m_options.toGeo = GetReProjection(prjID, PRJ_WGS_84);


			if (!m_options.m_locations_file_path.empty())
			{
				msg += locations.Load(m_options.m_locations_file_path);

				//transform location coord into weather coord
				for (size_t xy = 0; xy < locations.size(); xy++)
				{
					msg += locations[xy].Reproject(m_options.toWea);
				}
			}

			if (m_options.m_format == "XYZ" && m_options.m_locations_file_path.empty())
			{
				//create location form extent
			
				CGeoExtents extents = m_options.m_extents;
				locations.resize(extents.m_ySize*extents.m_xSize);
				for (size_t y = 0; y < extents.m_ySize; y++)
				{
					for (size_t x = 0; x < extents.m_xSize; x++)
					{
						CGeoPoint pt = extents.XYPosToCoord(CGeoPointIndex((int)x, (int)y));
						CGeoPointIndex index = weather_extents.CoordToXYPos(pt);
						
						//pt.Reproject(m_options.toGeo);

						size_t xy = y * extents.m_xSize + x;
						locations[xy] = CLocation(to_string(index.m_x + 1), to_string(index.m_y + 1), pt.m_lat, pt.m_lon, weather.GetFirstAltitude(index));
						locations[xy].SetPrjID(weather.GetPrjID());
					}
				}
			}

			string filePath = m_options.m_filesPath[CExtractGribsOption::OUTPUT_FILE_PATH];

			if (m_options.m_format == "XYZ")
			{
				msg = CSV_file.open(filePath);

				if (msg)
				{
					string header("KeyID,Name,Latitude,Longitude,Elevation,TimeUTC,Level");
					for (size_t v = 0; v < m_options.m_variables.size(); v++)
					{
						string varName = VAR_NAME[m_options.m_variables[v]];
						header += "," + varName;

					}

					CSV_file << header << endl;
				}
			}
			else
			{
					std::string m_locations_file_path;

					CExtractGribsOption options(m_options);
					options.m_nbBands = options.m_times.size()*options.m_levels.size()* options.m_variables.size();
					//options.m_format = "GTiff";

					if (!options.m_bQuiet)
					{
						cout << endl;
						cout << "Open output images..." << endl;
						//cout << "    Size           = " << options.m_extents.m_xSize << " cols x " << options.m_extents.m_ySize << " rows x " << options.m_nbBands << " bands" << endl;
						//cout << "    Extents        = X:{" << ToString(options.m_extents.m_xMin) << ", " << ToString(options.m_extents.m_xMax) << "}  Y:{" << ToString(options.m_extents.m_yMin) << ", " << ToString(options.m_extents.m_yMax) << "}" << endl;
						//cout << "    Output period   = " << m_options.m_period.GetFormatedString() << endl;
					}

					//if (!options.m_bQuiet && options.m_bCreateImage)
						//cout << "Create output images " << " x(" << options.m_extents.m_xSize << " C x " << options.m_extents.m_ySize << " R x " << options.m_nbBands << " bands) with " << options.m_CPU << " threads..." << endl;

					//replace the common part by the new name
					for (size_t t = 0; t < options.m_times.size(); t++)
					{
						for (size_t l = 0; l < options.m_levels.size(); l++)
						{
							for (size_t v = 0; v < options.m_variables.size(); v++)
							{
								__int64 UTCTime = options.m_times[t];
								//__int64 localTime = CTimeZones::UTCTime2LocalTime(UTCTime, pt);

								struct tm * timeinfo = gmtime(&UTCTime);

								char time[128] = { 0 };
								strftime(time, 128, "%Y-%m-%d-%H-%M", timeinfo);

								//string time = to_string(m_options.m_times[t]);// m_options.m_times[t].GetFormatedString("%Y%m%d%H");
								string levelName = ToString(options.m_levels[l], 4);
								string varName = VAR_NAME[options.m_variables[v]];
								options.m_VRTBandsName += GetFileTitle(filePath) + "_" + time + "_" + levelName + "_" + varName + ".tif|";
							}
						}
					}

					msg += outputDS.CreateImage(filePath, options);
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
			weather.read_block(xBlock, yBlock, UTCWeatherTime);

			m_options.m_timerRead.Stop();
		}
	}



	void CExtractGribs::ProcessBlock(int xBlock, int yBlock, CGribsWeather& weather, OutputData& outputData)
	{

		CGeoExtents extents = m_options.GetExtents();
		CGeoExtents blocExtents = extents.GetBlockExtents(xBlock, yBlock);
		CGeoSize blockSize = blocExtents.GetSize();


#pragma omp critical(ProcessBlock)
		{
			m_options.m_timerProcess.Start();

			for (size_t t = 0; t < m_options.m_times.size(); t++)
			{
				__int64 UTCWeatherTime = weather.GetNearestFloorTime(m_options.m_times[t]);
				for (size_t y = 0; y < blockSize.m_y; y++)
				{
					for (size_t x = 0; x < blockSize.m_x; x++)
					{
						for (size_t l = 0; l < m_options.m_levels.size(); l++)
						{
							CGeoPoint pt1 = blocExtents.XYPosToCoord(CGeoPointIndex(int(x), int(y)));
							CGeoPoint3D pt(pt1.m_x, pt1.m_y, m_options.m_levels[l], extents.GetPrjID());
							//CGeoPoint3DIndex xyz = weather.get_xyz(pt, UTCWeatherTime);

							//CATMWeatherCuboidsPtr pCuboid = weather.get_cuboids(pt, UTCWeatherTime);
							//CATMVariables var = pCuboid->get_weather(pt, UTCWeatherTime);
							CATMVariables var = weather.get_weather(pt, UTCWeatherTime, m_options.m_times[t]);

							for (size_t v = 0; v < m_options.m_variables.size(); v++)
							{
								size_t tlv = t * m_options.m_levels.size()*m_options.m_variables.size() + l * m_options.m_variables.size() + v;
								size_t xy = y * blockSize.m_x + x;

								switch (m_options.m_variables[v])
								{
								case GR_TAIR: outputData[tlv][xy] = var[ATM_TAIR]; break;
								case GR_PRCP: outputData[tlv][xy] = var[ATM_PRCP]; break;
								case GR_WNDU: outputData[tlv][xy] = var[ATM_WNDU] * 3600 / 1000; break;
								case GR_WNDV: outputData[tlv][xy] = var[ATM_WNDV] * 3600 / 1000; break;
								case GR_WNDS: outputData[tlv][xy] = var.get_wind_speed() * 3600 / 1000; break;
								case GR_WNDD: outputData[tlv][xy] = var.get_wind_direction(); break;
								default: ASSERT(false);
								}

#pragma omp atomic 
								m_options.m_xx++;
							}
						}
					}

					m_options.UpdateBar();
				}
			}
			m_options.m_timerProcess.Stop();
		}
	}

	void CExtractGribs::ProcessBlock(CLocationVector& locations, CGribsWeather& weather, OutputData& outputData)
	{

		m_options.ResetBar((size_t)m_options.m_times.size()*locations.size()*m_options.m_levels.size());

#pragma omp critical(ProcessBlock)
		{
			m_options.m_timerProcess.Start();
			for (size_t t = 0; t < m_options.m_times.size(); t++)
			{
				__int64 UTCWeatherTime = weather.GetNearestFloorTime(m_options.m_times[t]);

				for (size_t xy = 0; xy < locations.size(); xy++)
				{
					for (size_t l = 0; l < m_options.m_levels.size(); l++)
					{
						CGeoPoint3D pt(locations[xy].m_x, locations[xy].m_y, m_options.m_levels[l], locations[xy].GetPrjID());

						CATMVariables var = weather.get_weather(pt, UTCWeatherTime, m_options.m_times[t]);
						//CATMWeatherCuboidsPtr pCuboid = weather.get_cuboids(pt, UTCWeatherTime);
						//CATMVariables var = pCuboid->get_weather(pt, UTCWeatherTime);

						for (size_t v = 0; v < m_options.m_variables.size(); v++)
						{
							size_t tlv = t * m_options.m_levels.size()*m_options.m_variables.size() + l * m_options.m_variables.size() + v;

							switch (m_options.m_variables[v])
							{
							case GR_TAIR: outputData[tlv][xy] = var[ATM_TAIR]; break;
							case GR_PRCP: outputData[tlv][xy] = var[ATM_PRCP]; break;
							case GR_WNDU: outputData[tlv][xy] = var[ATM_WNDU]; break;
							case GR_WNDV: outputData[tlv][xy] = var[ATM_WNDV]; break;
							case GR_WNDS: outputData[tlv][xy] = var.get_wind_speed(); break;
							case GR_WNDD: outputData[tlv][xy] = var.get_wind_direction(); break;
							default: ASSERT(false);
							}
						}//v

						 //#pragma omp atomic 
						 m_options.m_xx++;
						 m_options.UpdateBar();
					}//l
				}
			}

			m_options.m_timerProcess.Stop();
		}
	}


	void CExtractGribs::WriteBlock(int xBlock, int yBlock, OutputData& outputData, CGDALDatasetEx& outputDS)
	{
#pragma omp critical(BlockIO)
		{
			m_options.m_timerWrite.Start();

			CGeoExtents extents = outputDS.GetExtents();
			CGeoRectIndex outputRect = extents.GetBlockRect(xBlock, yBlock);

			if (outputDS.IsOpen())
			{
				ASSERT(outputRect.m_x >= 0 && outputRect.m_x < outputDS.GetRasterXSize());
				ASSERT(outputRect.m_y >= 0 && outputRect.m_y < outputDS.GetRasterYSize());
				ASSERT(outputRect.m_xSize > 0 && outputRect.m_xSize <= outputDS.GetRasterXSize());
				ASSERT(outputRect.m_ySize > 0 && outputRect.m_ySize <= outputDS.GetRasterYSize());

				float noData = (float)m_options.m_dstNodata;
				for (size_t b = 0; b < outputDS.GetRasterCount(); b++)
				{
					GDALRasterBand *pBand = outputDS.GetRasterBand(b);
					if (!outputData.empty())
						pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(outputData[b][0]), outputRect.Width(), outputRect.Height(), GDT_Float32, 0, 0);
					else
						pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &noData, 1, 1, GDT_Float32, 0, 0);
				}
			}


			m_options.m_timerWrite.Stop();
		}
	}

	void CExtractGribs::WriteBlock(CLocationVector& locations, OutputData& outputData, ofStream& CSV_file)
	{
#pragma omp critical(BlockIO)
		{
			m_options.m_timerWrite.Start();

			if (CSV_file.is_open())
			{
				for (size_t xy = 0; xy < locations.size(); xy++)
				{
					for (size_t t = 0; t < m_options.m_times.size(); t++)
					{
						for (size_t l = 0; l < m_options.m_levels.size(); l++)
						{
							CGeoPoint pt = locations[xy];
							pt.Reproject(m_options.toGeo);

							__int64 UTCTime = m_options.m_times[t];
							//__int64 localTime = CTimeZones::UTCTime2LocalTime(UTCTime, pt);

							struct tm * timeinfo = gmtime(&UTCTime);

							char time[128] = { 0 };
							strftime(time, 128, "%Y-%m-%d-%H-%M", timeinfo);
							
							//string time = m_options.m_period[t].GetFormatedString("%Y-%m-%d-%H");
							string levelName = to_string(m_options.m_levels[l]);

							string line = FormatA("%s,%s,%.4lf,%.4lf,%.1lf,%s,%s", locations[xy].m_ID.c_str(), locations[xy].m_name.c_str(), pt.m_lat, pt.m_lon, locations[xy].m_alt, time, levelName.c_str());

							for (size_t v = 0; v < m_options.m_variables.size(); v++)
							{
								size_t tvl = t * m_options.m_variables.size()*m_options.m_levels.size() + l * m_options.m_variables.size() + v;
								line += FormatA(",%.2f", outputData[tvl][xy]);
							}//v

							CSV_file << line << std::endl;
						}//l
					}//t
				}//xy
			}

			m_options.m_timerWrite.Stop();
		}

	}

	void CExtractGribs::CloseAll(CGribsWeather& inputDS, CGDALDatasetEx& outputDS, ofStream& CSV_file)
	{
		inputDS.close();


		m_options.m_timerWrite.Start();

		outputDS.Close(m_options);
		CSV_file.close();

		m_options.m_timerWrite.Stop();
		m_options.PrintTime();
	}


	//*******************************************************************************************************************


	CATMVariables CGribsWeather::get_weather(CGeoPoint3D pt, __int64 UTCWeatherTime, __int64 UTCCurrentTime)const
	{
		ASSERT(pt.GetPrjID()==GetPrjID());

		CATMWeatherCuboidsPtr p_cuboid = get_cuboids(pt, UTCWeatherTime);
		CATMVariables weather = p_cuboid->get_weather(pt, UTCCurrentTime);

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
		//if (m_bHgtOverSea)
		//{
		grAlt = GetFirstAltitude(xy, UTCWeatherTime);//get the first level over the ground

		//	if (grAlt <= -999)
		//		grAlt = m_world.GetGroundAltitude(pt);
		//}
		//ASSERT(grAlt > -999);
		//if (grAlt > -999)
		//{
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
		//}

		ASSERT(L == NOT_INIT || L < NB_LEVELS);
		return L;
	}



	double CGribsWeather::GetFirstAltitude(const CGeoPointIndex& xy, __int64 UTCWeatherTime)const
	{
		if (UTCWeatherTime == 0)
			UTCWeatherTime = m_filepath_map.begin()->first;

		size_t b = m_p_weather_DS.get_band(UTCWeatherTime, ATM_HGT, 0);

		if (b == NOT_INIT)
			return -999;

		double gph = m_p_weather_DS.GetPixel(UTCWeatherTime, b, xy); //geopotential height at surface [m]

		return gph;
	}

	CGeoPoint3DIndex CGribsWeather::get_xyz(const CGeoPoint3D& pt, __int64 UTCWeatherTime)const
	{
		CGeoExtents extents = m_p_weather_DS.GetExtents(UTCWeatherTime);
		CGeoPoint3DIndex xyz;
		((CGeoPointIndex&)xyz) = extents.CoordToXYPos(pt + CGeoDistance3D(extents.XRes() / 2, extents.YRes() / 2, 0, extents.GetPrjID()));


		xyz.m_z = NB_LEVELS - 1;//take the last level (~2000m) on by default

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
		cuboids->m_bUseSpaceInterpolation = true;
		cuboids->m_bUseTimeInterpolation = true;


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
							if (L < NB_LEVELS)
							{
								double groundAlt = 0;

								//RUC is above sea level and WRF must be above sea level
							//	if (m_bHgtOverSea)
							//	{
									groundAlt = GetFirstAltitude(xy2, UTCWeatherTime);//get the first level over the ground
							//	}



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
											assert(L > 0 && L <= NB_LEVELS);
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

		assert((*cuboids)[0].m_time <= (*cuboids)[1].m_time);

		return cuboids;
	}


	string CGribsWeather::get_image_filepath(__int64 UTCWeatherTime)const
	{
		TTimeFilePathMap::const_iterator it = m_filepath_map.find(UTCWeatherTime);
		ASSERT(it != m_filepath_map.end());

		string path = ".";
		if (m_filePathGribs.find('/') != string::npos || m_filePathGribs.find('\\') != string::npos)
			path = GetPath(m_filePathGribs);

		return GetAbsolutePath(path, it->second);
	}

	ERMsg CGribsWeather::open(const std::string& filepath, CBaseOptions& options, CCallback& callback)
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

	ERMsg CGribsWeather::close(CCallback& callback)
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

		return msg;
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
				p += CTimeZones::Time2TRef(UTCTime).as(CTM::HOURLY);
			}
		}

		return p;
	}

	size_t CGribsWeather::GetPrjID(__int64 UTCWeatherTime)const
	{
		size_t prjID = NOT_INIT;

		if (UTCWeatherTime == 0)
			UTCWeatherTime = m_filepath_map.begin()->first;

		//if (!m_p_weather_DS.IsLoaded(UTCWeatherTime))
			//m_p_weather_DS.load(UTCWeatherTime, get_image_filepath(UTCWeatherTime), CCallback());
		
		if(const_cast<CGribsWeather*>(this)->load(UTCWeatherTime))
			prjID = m_p_weather_DS.GetPrjID(UTCWeatherTime);

		return prjID;
	}

	size_t CGribsWeather::GetRasterCount(__int64 UTCWeatherTime)const
	{
		size_t count = 0;

		if (UTCWeatherTime == 0)
			UTCWeatherTime = m_filepath_map.begin()->first;

		
		if (const_cast<CGribsWeather*>(this)->load(UTCWeatherTime))
			count = m_p_weather_DS.at(UTCWeatherTime)->GetRasterCount();

		return count;
	}

	void CGribsWeather::UpdateOption(CBaseOptions& options)
	{
		if (options.m_prj.empty())
		{
			options.m_prj = CProjectionManager::GetPrj(GetPrjID())->GetPrjStr();
		}

		

		if(!options.m_period.IsInit())
			options.m_period = GetEntireTPeriod();

		CGeoExtents mapExtents = GetExtents();
		if (!((CGeoRect&)options.m_extents).IsInit())
		{
			((CGeoRect&)options.m_extents) = mapExtents;//set entire extent
		}
		else
		{
			options.m_extents.SetPrjID(mapExtents.GetPrjID());//set only projection of input map
		}

		//step2 : initialization of the extent size
		if (options.m_extents.m_xSize == 0)
		{
			double xRes = mapExtents.XRes();
			if (options.m_bRes)
				xRes = options.m_xRes;

			options.m_extents.m_xSize = TrunkLowest(abs(options.m_extents.Width() / xRes));
			options.m_extents.m_xMax = options.m_extents.m_xMin + options.m_extents.m_xSize * abs(xRes);

		}


		if (options.m_extents.m_ySize == 0)
		{
			double yRes = mapExtents.YRes();
			if (options.m_bRes)
				yRes = options.m_yRes;


			options.m_extents.m_ySize = TrunkLowest(abs(options.m_extents.Height() / yRes));
			options.m_extents.m_yMin = options.m_extents.m_yMax - options.m_extents.m_ySize * abs(yRes);
		}

		//step3: initialization of block size
		if (options.m_extents.m_xBlockSize == 0)
			options.m_extents.m_xBlockSize = mapExtents.m_xBlockSize;

		if (options.m_extents.m_yBlockSize == 0)
			options.m_extents.m_yBlockSize = mapExtents.m_yBlockSize;

	}

	CGeoExtents CGribsWeather::GetExtents(__int64 UTCWeatherTime)const
	{
		CGeoExtents extents;

		if (UTCWeatherTime == 0)
			UTCWeatherTime = m_filepath_map.begin()->first;

		
		if (const_cast<CGribsWeather*>(this)->load(UTCWeatherTime))
			extents = m_p_weather_DS.GetExtents(UTCWeatherTime);

			//assume all gring with same extents and same projection
			return extents;
	}

	void CGribsWeather::read_block(int xBlock, int yBlock, __int64 UTCWeatherTime)
	{
		//ne fait rien pour l'instant
	}

	bool CGribsWeather::IsLoaded(__int64 UTCWeatherTime)const
	{
		return m_p_weather_DS.IsLoaded(UTCWeatherTime);
	}

}