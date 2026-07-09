//***********************************************************************
// program to extract points from image
//									 
//***********************************************************************
// version
// 1.0.0	05/06/2013	Rémi Saint-Amant	Creation from NormalsCreator

#include "stdafx.h" 
#include <float.h>
#include <math.h>
#include <algorithm>
#include <array>
#include <boost/multi_array.hpp>
#include <iostream>

#include "Basic/UtilMath.h"
#include "Basic/OpenMP.h"

#include "Geomatic/GDALBasic.h"
#include "Geomatic/ProjectionTransformation.h"

#include "NormalsCreator.h"
//#include "CSVFile.h"

#pragma warning(disable: 4275 4251)
#include "gdal_priv.h"
#include "ogr_spatialref.h"



using namespace std;
namespace WBSF
{

	using namespace WEATHER;
	using namespace HOURLY_DATA;
	using namespace NORMALS_DATA;
	const char* CNormalsCreator::VERSION = "1.0.0";
	const int CNormalsCreator::NB_THREAD_PROCESS = 2;



	CNormalsCreatorOption::CNormalsCreatorOption() :CBaseOptions(false)
	{

		m_appDescription = "This software compute BioSIM' Normals database from daily database and MonthlyMeanGrids";

		static const char* DEFAULT_OPTIONS[] = { "-q", "-multi", "-CPU", "-?", "-??", "-???", "-help" };
		for (int i = 0; i < sizeof(DEFAULT_OPTIONS) / sizeof(char*); i++)
			AddOption(DEFAULT_OPTIONS[i]);


		std::string m_inputDBFilePath;
		std::string m_outputDBFilePath;

		m_firstYear = 1991;
		m_lastYear = 2020;
		m_nbYearMin = 10;


		//climatic change section
		//m_bApplyCC=false;
		//m_inputMMGFilePath;

		//int m_firstRefYear;
		m_reference_period = P1991_2020;
		m_CC_period.reset();
		m_nbNeighbor = 3;
		m_maxDistance = 100000;//in m
		m_power = 2;


		static const COptionDef OPTIONS[] =
		{
			{ "-FirstYear", 1, "Year", false, "First year of daily data. 1991 by default." },
			{ "-LastYear", 1, "Year", false, "Last year of daily data. 2020 by default." },
			{ "-NbYears", 1, "NbYears", false, "Minimum number of years for valid weather station. 10 by default." },
			//{ "-ApplyCC", 1, "MMG_filePath", false, "Apply climate change on weather station using the Monthly Mean Grid (MMG) file path." },
			{ "-ReferencePeriod", 1, "PeriodIndex", false, "Climate change reference index period beginning at 0 (1961-1990) and ending at 11 (2071-2100). 3 (1991-2020) by default." },
			{ "-FuturPeriod", 1, "PeriodIndex", true, "Climate change future index period (same as reference). All index period after reference index period by default 4 (2011-2030) to 11 (2071-2100)." },
			{ "-NbNeighbor", 1, "N", false, "Number of nearest cell for climate change computation. 3 by default." },
			{ "-MaxDistance", 1, "Distance", false, "Maximum distance (km) between weather station and valid cell. 100 km by default." },
			{ "-Power", 1, "w", false, "Power to compute distance weight between weather station and cells. 2 by default." },
			{ "DailyDatabase", 0, "", false, "Daily database file path." },
			{ "MMG", 0, "", false, "Monthly Mean Grid (MMG) file path to apply climate change on weather station ." },
			{ "NormalsDatabse", 0, "", false, "Normals' output file path." }
		};

		for (int i = 0; i < sizeof(OPTIONS) / sizeof(COptionDef); i++)
			AddOption(OPTIONS[i]);


		static const CIOFileInfoDef IO_FILE_INFO[] =
		{
			{ "Input", "Daily database", "", "", "", "" },
			{ "Output", "Normals database", "", "", "", "" },
		};

		for (int i = 0; i < sizeof(IO_FILE_INFO) / sizeof(CIOFileInfoDef); i++)
			AddIOFileInfo(IO_FILE_INFO[i]);



	}


	ERMsg CNormalsCreatorOption::ParseOption(int argc, char* argv[])
	{
		ERMsg msg = CBaseOptions::ParseOption(argc, argv);

		if (msg && m_filesPath.size() != 3)
		{
			msg.ajoute("Invalid argument line. 2 files are needed: input daily and output Normals database file path.\n");
			msg.ajoute("Argument found: ");
			for (size_t i = 0; i < m_filesPath.size(); i++)
				msg.ajoute("   " + to_string(i + 1) + "- " + m_filesPath[i]);

		}

		//remove period 
		if (m_CC_period.none())
		{
			for (size_t p = m_reference_period + 1; p < m_CC_period.size(); p++)
				m_CC_period.set(p);
		}

		return msg;
	}

	ERMsg CNormalsCreatorOption::ProcessOption(int& i, int argc, char* argv[])
	{
		ERMsg msg;
		if (IsEqual(argv[i], "-FirstYear"))
		{
			m_firstYear = ToInt(argv[++i]);;
		}
		else if (IsEqual(argv[i], "-LastYear"))
		{
			m_lastYear = ToInt(argv[++i]);;
		}
		else if (IsEqual(argv[i], "-NbYears"))
		{
			m_nbYearMin = ToInt(argv[++i]);
		}
		//else if (IsEqual(argv[i], "-ApplyCC"))
		//{
		//	//m_bApplyCC = true;
		//	m_inputMMGFilePath = argv[++i];
		//}
		else if (IsEqual(argv[i], "-ReferencePeriod"))
		{
			m_reference_period = ToInt(argv[++i]);
			if (m_reference_period >= NB_PERIODS)
				msg.ajoute("Invalid reference period index. See help for more info");
		}
		else if (IsEqual(argv[i], "-FuturPeriod"))
		{
			size_t index = ToInt(argv[++i]);
			if (index < NB_PERIODS)
				m_CC_period.set(index);
			else
				msg.ajoute("Invalid reference period index. See help for more info");
		}
		else if (IsEqual(argv[i], "-NbNeighbor"))
		{
			m_nbNeighbor = ToInt(argv[++i]);
		}
		else if (IsEqual(argv[i], "-MaxDistance"))
		{
			m_maxDistance = ToInt(argv[++i]) * 1000;//convert in meters
		}
		else if (IsEqual(argv[i], "-Power"))
		{
			m_power = ToInt(argv[++i]);
		}
		else
		{
			//Look to see if it's a know base option
			msg = CBaseOptions::ProcessOption(i, argc, argv);
		}

		return msg;
	}

	int GetPeriodFirstYear(size_t p)
	{
		return int(1961 + p * 10);
	}

	int GetPeriodLastYear(size_t p)
	{
		return int(1990 + p * 10);
	}

	std::string CNormalsCreatorOption::GetOutputFilePath(size_t p)
	{
		string filepath = m_filesPath[CNormalsCreator::OUTPUT_FILE_PATH];
		string MMGFilePath = m_filesPath[CNormalsCreator::MMG_FILE_PATH];
		//if (!m_inputMMGFilePath.empty())
		filepath = FormatA("%s %d-%d %s%s", (GetPath(filepath) + GetFileTitle(filepath)).c_str(), GetPeriodFirstYear(p), GetPeriodLastYear(p), GetFileTitle(MMGFilePath).c_str(), GetFileExtension(filepath).c_str());

		return filepath;
	}




	//*************************************************************************************************************************************************************

	//#include <boost/format.hpp>

	ERMsg CNormalsCreator::Execute()
	{
		ERMsg msg;

		if (!m_options.m_bQuiet)
		{
			cout << endl;
			cout << "Output: " << m_options.m_filesPath[OUTPUT_FILE_PATH] << endl;
			cout << "From:   " << m_options.m_filesPath[INPUT_FILE_PATH] << endl;
			cout << "Using:  " << m_options.m_filesPath[MMG_FILE_PATH] << endl;
		}

		GDALAllRegister();

		CDailyDatabase inputDB;
		array<CNormalsDatabasePtr, CNormalsCreatorOption::NB_PERIODS> normalsDB;

		msg = OpenAll(inputDB, m_MMG, normalsDB);

		if (msg)
		{

			array< CBandsHolderPtr, NORMALS_DATA::NB_FIELDS> bandHolder;
			for (size_t i = 0; i < NORMALS_DATA::NB_FIELDS; i++)
			{
				if (m_MMG.m_supportedVariables[i])
				{
					bandHolder[i].reset(new CBandsHolder);//(1, m_options.m_memoryLimit, m_options.m_IOCPU, NB_THREAD_PROCESS
					msg = bandHolder[i]->Load(m_MMG.GetDataset(i), true, m_options.m_extents, m_options.m_period);
				}
			}

			if (!msg)
				return msg;


			//if (!m_options.m_bQuiet && m_options.m_bCreateImage)
				//printf("Extract %I64u points with %d threads...\n", ioFile.size(), m_options.m_bMulti ? m_options.m_CPU : 1);


			CGeoExtents extents = m_options.GetExtents();
			m_options.m_xxFinal = inputDB.size();
			//extents.YNbBlocks() * extents.XNbBlocks() *

			//**************************************

			omp_set_nested(1);//for IOCPU
			boost::dynamic_bitset<size_t> treated(inputDB.size());

			for (int yBlock = 0; yBlock < extents.YNbBlocks(); yBlock++)
			{
				//#pragma omp parallel for schedule(static, 1) num_threads( NB_THREAD_PROCESS ) if (m_options.m_bMulti)	
				for (int xBlock = 0; xBlock < extents.XNbBlocks(); xBlock++)
				{
					int blockThreadNo = ::omp_get_thread_num();

					//CGeoExtents extents = bandHolder[blockThreadNo].GetExtents();
					CGeoExtents blockExtents = extents.GetBlockExtents(xBlock, yBlock);
					size_t N = GetNbStationIn(blockExtents, inputDB);

					if (N > 0)
					{
						ReadBlock(xBlock, yBlock, bandHolder);//[blockThreadNo]
						ProcessBlock(xBlock, yBlock, bandHolder, inputDB, normalsDB, treated);//[blockThreadNo]
					}
					else
					{
						//#pragma omp atomic
												//m_options.m_xx += (int)inputDB.size();
						m_options.m_xx = treated.count();

						m_options.UpdateBar();
					}

				}//for xBloxk
			}//for yblock



			//add noData to untreated line
			//std::ostringstream st;
			//st << std::fixed << std::setprecision(m_options.m_precision) << m_options.m_dstNodata;
			/*string strNodata = TestToString(m_options.m_dstNodata, m_options.m_precision);
			for (size_t i = 0; i < ioFile.size(); i++)
			{
				if (!treated[i])
				{
					for (size_t z = 0; z < bandHolder[TMIN_MN].GetRasterCount(); z++)
						ioFile[i] += ',' + strNodata;
				}
			}

			m_options.m_timerWrite.Start();
			msg = ioFile.Save(m_options.m_filesPath[OUTPUT_FILE_PATH]);
			m_options.m_timerWrite.Stop();*/


			CloseAll(inputDB, m_MMG, normalsDB);
		}


		return msg;

	}

	size_t CNormalsCreator::GetNbStationIn(const CGeoExtents& blockExtents, const CDailyDatabase& inputDB)
	{
		size_t nb = 0;
		for (int i = 0; i < inputDB.size(); i++)
		{
			if (blockExtents.IsInside(inputDB[i]))
				nb++;
		}

		return nb;
	}



	ERMsg CNormalsCreator::OpenAll(CDailyDatabase& inputDB, CMonthlyMeanGrid& MMG, array<CNormalsDatabasePtr, CNormalsCreatorOption::NB_PERIODS>& normalsDB)
	{
		ERMsg msg;

		if (!m_options.m_bQuiet)
			cout << endl << "Open input image..." << endl;


		msg = inputDB.Open(m_options.m_filesPath[INPUT_FILE_PATH], CDailyDatabase::modeRead, CCallback::DEFAULT_CALLBACK, true);

		if (msg && !m_options.m_bQuiet)
		{
			cout << "    Nb stations       = " << inputDB.size() << endl;
		}


		if (msg)
		{
			if (!m_options.m_bQuiet)
				cout << endl << "Open MMG to apply climate change..." << endl;

			msg += MMG.Open(m_options.m_filesPath[CNormalsCreator::MMG_FILE_PATH], CCallback::DEFAULT_CALLBACK);
			if (msg) // get blocks extent 
			{
				assert(MMG.GetDataset(0).IsOpen());
				MMG.GetDataset(0).UpdateOption(m_options);
			}
		}


		if (msg)
		{

			//if (m_options.ApplyCC())
			//{
				//normalsDB.resize(m_options.m_CC_period.count());

			for (size_t p = 0; p < m_options.m_CC_period.size() && msg; p++)
			{
				if (m_options.m_CC_period.test(p))
				{
					std::string outputDBFilePath = m_options.GetOutputFilePath(p);
					CreateMultipleDir(GetPath(outputDBFilePath));

					//delete old database if any
					msg += CNormalsDatabase::DeleteDatabase(outputDBFilePath);

					//normal(write) database
//						CNormalsDatabase outputDB;
					normalsDB[p].reset(new CNormalsDatabase);
					msg += normalsDB[p]->Open(outputDBFilePath, CNormalsDatabase::modeEdit);

					if (msg)
						normalsDB[p]->SetPeriod(GetPeriodFirstYear(p), GetPeriodLastYear(p));
				}

			}
			//}
			//else
			//{
			//	std::string outputDBFilePath = m_options.m_filesPath[CNormalsCreator::OUTPUT_FILE_PATH];
			//	msg += CNormalsDatabase::DeleteDatabase(outputDBFilePath);
			//	//normalsDB.resize(1);

			//	msg += normalsDB[TMIN_MN]->Open(outputDBFilePath, CNormalsDatabase::modeEdit);
			//	normalsDB[TMIN_MN]->SetPeriod(m_options.m_firstYear, m_options.m_lastYear);
			//}
		}

		return msg;
	}

	void CNormalsCreator::ReadBlock(int xBlock, int yBlock, std::array< CBandsHolderPtr, NORMALS_DATA::NB_FIELDS>& bandHolder)
	{
#pragma omp critical(BlockIO)
		{
			m_options.m_timerRead.Start();
			for (size_t i = 0; i < bandHolder.size(); i++)
			{
				if (bandHolder[i])
					bandHolder[i]->LoadBlock(xBlock, yBlock);
			}


			m_options.m_timerRead.Stop();
		}
	}


	void CNormalsCreator::ProcessBlock(int xBlock, int yBlock, std::array< CBandsHolderPtr, NORMALS_DATA::NB_FIELDS>& bandHolder, CDailyDatabase& inputDB, std::array<CNormalsDatabasePtr, CNormalsCreatorOption::NB_PERIODS>& normalsDB, boost::dynamic_bitset<size_t>& treated)
	{
		CGeoExtents extents = bandHolder[TMIN_MN]->GetExtents();
		CGeoSize blockSize = extents.GetBlockSize(xBlock, yBlock);
		//int nbCells = (size_t)extents.m_xSize*extents.m_ySize;
		CGeoExtents blockExtents = extents.GetBlockExtents(xBlock, yBlock);
		CGeoRectIndex blockRect = blockExtents.GetPosRect();
		//string strNodata = ToString(m_options.m_dstNodata, m_options.m_precision);

		if (bandHolder[TMIN_MN]->IsEmpty())
		{
//#pragma omp atomic		
//			m_options.m_xx += (int)inputDB.size();
	//		m_options.UpdateBar();

			return;
		}



		{
			m_options.m_timerProcess.Start();

			//array < vector<CDataWindowPtr>, NORMALS_DATA::NB_FIELDS> input;
			//for (size_t i = 0; i < bandHolder.size(); i++)
			//{
			//	if (bandHolder[i])
			//		bandHolder[i]->GetWindow(input[i]);
			//}


			set<int> years;
			for (int y = m_options.m_firstYear; y <= m_options.m_lastYear; y++)
				years.insert(y);

			//process all point
#pragma omp parallel for num_threads( m_options.m_CPU) if (m_options.m_bMulti)	
			for (int i = 0; i < inputDB.size(); i++)
			{
				//for all point in the table
				CGeoPoint coordinate = inputDB[i];
				CGeoPointIndex xy = blockExtents.CoordToXYPos(coordinate);

				//find position in the block
				if (blockRect.IsInside(xy))
				{
					ASSERT(!treated[i]);
					//CGeoSize block0 = extents.GetBlockSize(0, 0);
					treated.set(i);

					CWeatherStation dailyStation0;

					ERMsg msg = inputDB.Get(dailyStation0, i, years);
					//ERMsg msg_valid = dailyStation.IsValid();
					//if (!msg_valid)
					//{
					//	msg.ajoute("Invalid daily weather data for station: " + dailyStation.m_ID);
					//	callback.AddMessage(msg_valid);
					//}
					//msg.ajoute("Invalid weather data for station: "+ dailyStation.m_ID);
					dailyStation0.m_siteSpeceficInformation.clear();//remove all SSI
					dailyStation0.UseIt(true);



					//remove years not in the period 
					//dailyStation0.KeepOnlyYears(m_options.m_firstYear, m_options.m_lastYear);
					assert(dailyStation0.IsValid());

					//if the station have enough years
					if (dailyStation0.size() >= m_options.m_nbYearMin)
					{
						//CGeoPoint pt(dailyStation0);
						//
						//CGeoPointIndex index1 = extents.CoordToXYPos(pt);
						//DataType p1 = bandHolder[i]->GetPixel(0, index1.m_x, index1.m_y);
						//
						//CGeoPointIndex index2 = blockExtents.CoordToXYPos(pt);
						//DataType p2 = bandHolder[i]->GetPixel(0, index2.m_x, index2.m_y);


						for (size_t p = 0; p < CNormalsCreatorOption::NB_PERIODS; p++)
						{
							if (m_options.m_CC_period.test(p))
							{
								CWeatherStation dailyStation = dailyStation0;
								//adjust daily data to reflect climatic change
								if (UpdateData(p, blockExtents, bandHolder, dailyStation))
								{

									//create normal
									CAdvancedNormalStation normalStation;

									if (normalStation.FromDaily(dailyStation, m_options.m_nbYearMin))
									{
										//msg = station.IsValid();
										//if (!msg)
											//msg.ajoute("Invalid weather data for station: " + station.m_ID);
										ERMsg msg_valid = normalStation.IsValid();
										//if (!msg_valid)
										//{
										//	callback.AddMessage("Invalid normals weather data for station: " + station.m_ID, 1);
										//	callback.AddMessage(msg_valid);
										//}

										//if (msg)
										//{
										//if (m_options.ApplyCC())
										//{
											//now adjust standard deviation if they are present
										UpdateStandardDeviation(p, blockExtents, bandHolder, normalStation);
										//}

										//add normal to database
#pragma omp critical(ProcessBlock)
										{
											normalsDB[p]->Add(normalStation);
										}
										//if (messageTmp)
										//{
										//	nbStationAdded++;
										//}
									}
								}//used period
							}//keepit
						}//for all period
					}

#pragma omp atomic 
					m_options.m_xx++;

					m_options.UpdateBar();
				}//is in rect
			}//for all stations
		}//for batch 
	}

	void CNormalsCreator::CloseAll(CDailyDatabase& inputDB, CMonthlyMeanGrid& MMG, array<CNormalsDatabasePtr, CNormalsCreatorOption::NB_PERIODS>& normalsDB)
	{


		inputDB.Close();
		MMG.Close();

		ERMsg msg;
		for (size_t p = 0; p < normalsDB.size(); p++)
		{
			if (normalsDB[p])
			{
				normalsDB[p]->Close();

				//reopen database to create NZop;
				std::string outputDBFilePath = m_options.GetOutputFilePath(p);
				msg += normalsDB[p]->Open(outputDBFilePath);
			}
		}

		if (!msg)

			m_options.PrintTime();
	}

	bool CNormalsCreator::UpdateData(size_t p, CGeoExtents blockExtents, std::array< CBandsHolderPtr, NORMALS_DATA::NB_FIELDS>& bandHolder, CWeatherStation& stationIn)
	{
		ASSERT(bandHolder[TMIN_MN]);
		int firstRefYear = GetPeriodFirstYear(m_options.m_reference_period);
		int firstCCYear = GetPeriodFirstYear(p);
		size_t nbRefYears = 30;
		size_t nbCCYears = 30;
		size_t nbNeighbor = m_options.m_nbNeighbor;
		double maxDistance = m_options.m_maxDistance;
		double power = m_options.m_power;


		CGeoPoint pt(stationIn);

		//CGeoExtents extents = bandHolder[TMIN_MN]->GetExtents();
		if (pt.GetPrjID() != blockExtents.GetPrjID())
		{
			pt.Reproject(CProjectionTransformationManager::Get(pt.GetPrjID(), blockExtents.GetPrjID()));
		}


		CGeoPointIndex index = blockExtents.CoordToXYPos(pt);

		if (!blockExtents.IsInside(index))
			return false;


		CGeoPointIndexVector pts;
		int level = (int)ceil((sqrt((double)nbNeighbor) - 1) / 2);
		blockExtents.GetNearestCellPosition(pt, Square((level + 1) * 2 + 1), pts);

		std::vector<double> d;
		for (size_t i = 0; i < pts.size(); i++)
		{
			CGeoPoint pti = blockExtents.XYPosToCoord(pts[i]);
			double di = max(0.000001, pt.GetDistance(pti));
			if (di < maxDistance)
				d.push_back(di);
		}

		pts.erase(pts.begin() + d.size(), pts.end());

		if (pts.empty())
			return false;


		double refMonthlyMean[12][NB_FIELDS] = { 0 };
		double ccMonthlyMean[12][NB_FIELDS] = { 0 };

		if (!GetMonthlyMean(firstRefYear, nbRefYears, nbNeighbor, power, pts, d, refMonthlyMean, bandHolder))
			return false;

		if (!GetMonthlyMean(firstCCYear, nbCCYears, nbNeighbor, power, pts, d, ccMonthlyMean, bandHolder))
			return false;


		CWeatherStation stationII;
		((CLocation&)stationII) = stationIn;
		stationII.CreateYears(firstCCYear, stationIn.size());

		for (size_t y = 0; y < stationIn.size(); y++)
		{
			int year = stationIn[y].GetTRef().GetYear();

			for (size_t m = 0; m < 12; m++)
			{
				//for (size_t d = 0; d < stationIn[y][m].size(); d++)
				//never take into account the leap year. Too much problem to convert no-leap an leap year
				for (size_t d = 0; d < WBSF::GetNbDayPerMonth(m); d++)
				{
					for (TVarH v = H_FIRST_VAR; v < NB_VAR_H; ((int&)v)++)
					{
						size_t f = V2F(v);

						if (f != -1 && stationIn[y][m][d][v].IsInit() && !IsMissing(ccMonthlyMean[m][f]) && !IsMissing(refMonthlyMean[m][f]))
						{
							if (v == HOURLY_DATA::H_TMIN)
							{
								//const CStatistic& statIn = stationIn[y][m][d][TMIN];
								CStatistic statOut = stationIn[y][m][d][H_TMIN][MEAN] + (ccMonthlyMean[m][TMIN_MN] - refMonthlyMean[m][TMIN_MN]);
								stationII[y][m][d][HOURLY_DATA::H_TMIN] = statOut[MEAN];

							}
							else if (v == HOURLY_DATA::H_TMAX)
							{
								//const CStatistic& statIn = stationIn[y][m][d][TMIN];
								CStatistic statOut = stationIn[y][m][d][H_TMAX][MEAN] + (ccMonthlyMean[m][TMAX_MN] - refMonthlyMean[m][TMAX_MN]);
								stationII[y][m][d][HOURLY_DATA::H_TMAX] = statOut[MEAN];
							}
							else if (v == HOURLY_DATA::H_PRCP)
							{
								double prcp = stationIn[y][m][d][v][SUM];
								prcp *= (ccMonthlyMean[m][f] / refMonthlyMean[m][f]);
								stationII[y][m][d][v] = prcp;
							}
							else if (v == HOURLY_DATA::H_TDEW)
							{
								if (IsMissing(refMonthlyMean[m][RELH_MN]))//no relative humidity. then take specific humidity
								{
									ASSERT(refMonthlyMean[m][f] < 20);//ccMonthlyMean must be specific humidity g[H2O]/kg[air]
									if (stationIn[y][m][d][H_RELH].IsInit() && stationIn[y][m][d][H_TMIN].IsInit() && stationIn[y][m][d][H_TMAX].IsInit())
									{
										ASSERT(stationIn[y][m][d][H_TMIN].IsInit());
										ASSERT(stationIn[y][m][d][H_TMAX].IsInit());

										//convert Hr to Hs with station temperature
										double Tmin = stationIn[y][m][d][H_TMIN][MEAN];
										double Tmax = stationIn[y][m][d][H_TMAX][MEAN];
										double Hr = stationIn[y][m][d][H_RELH][MEAN];
										double Hs = Hr2Hs(Tmin, Tmax, Hr);
										Hs *= (ccMonthlyMean[m][f] / refMonthlyMean[m][f]);//specific humidity ratio

										ASSERT(stationII[y][m][d].GetParent());
										//convert back Hs to Hr with the new station temperature
										double TminII = stationII[y][m][d][H_TMIN][MEAN];
										double TmaxII = stationII[y][m][d][H_TMAX][MEAN];
										Hr = Hs2Hr(TminII, TmaxII, Hs);

										stationII[y][m][d][H_RELH] = Hr;


										//we compute the best Tdew as we can. 
										double Pv = Hr2Pv(TminII, TmaxII, Hr);
										double Td = WBSF::Pv2Td(Pv / 1000);
										stationII[y][m][d][H_TDEW] = Td;
									}
								}
							}
							else if (v == HOURLY_DATA::H_RELH)
							{
								ASSERT(refMonthlyMean[m][f] >= 0 && refMonthlyMean[m][f] <= 100);//ccMonthlyMean must be relative humidity [%]
								ASSERT(stationIn[y][m][d][H_RELH].IsInit());

								if (stationIn[y][m][d][H_TMIN].IsInit() && stationIn[y][m][d][H_TMAX].IsInit())
								{
									//convert Hr to Hs with station temperature
									double Hr = stationIn[y][m][d][H_RELH][MEAN];
									Hr = max(0.0, min(100.0, (Hr * ccMonthlyMean[m][f] / refMonthlyMean[m][f])));
									stationII[y][m][d][H_RELH] = Hr;

									//we compute the best Tdew as we can. 
									double TminII = stationII[y][m][d][H_TMIN][MEAN];
									double TmaxII = stationII[y][m][d][H_TMAX][MEAN];
									assert(TminII >= -70 && TminII <= 70);
									assert(TmaxII >= -70 && TmaxII <= 70);

									double Pv = Hr2Pv(TminII, TmaxII, Hr);
									double Td = WBSF::Pv2Td(Pv / 1000);
									assert(Td >= -70 && Td <= 70);
									stationII[y][m][d][H_TDEW] = Td;
								}
							}
							else if (v == HOURLY_DATA::H_WNDS)
							{
								double wndS = stationIn[y][m][d][v][MEAN];
								wndS *= (ccMonthlyMean[m][f] / refMonthlyMean[m][f]);
								stationII[y][m][d][v] = wndS;
							}
						}//if is valid fields
					}//for all fields

					//complete humidity
					if (stationII[y][m][d][H_TMIN].IsInit() && stationII[y][m][d][H_TMAX].IsInit())
					{
						stationII[y][m][d][H_TAIR] = (stationII[y][m][d][H_TMIN][MEAN] + stationII[y][m][d][H_TMAX][MEAN]) / 2.0;
						if (stationII[y][m][d][H_TDEW].IsInit() &&
							!stationII[y][m][d][H_RELH].IsInit())
						{
							double RH = Td2Hr(stationII[y][m][d][H_TAIR][MEAN], stationII[y][m][d][H_TDEW][MEAN]);
							stationII[y][m][d][H_RELH] = RH;
						}
						else if (stationII[y][m][d][H_RELH].IsInit() &&
							!stationII[y][m][d][H_TDEW].IsInit())
						{
							double TDew = Hr2Td(stationII[y][m][d][H_TAIR][MEAN], stationII[y][m][d][H_RELH][MEAN]);
							stationII[y][m][d][H_TDEW] = TDew;
						}
					}
				}//for all days
			}//for all month
		}//for all years

		ASSERT(stationII.IsValid());
		stationIn = stationII;


		return true;
	}

	//after created the normal station from daily station, update normal standard deviation when they exist
	bool CNormalsCreator::UpdateStandardDeviation(size_t p, CGeoExtents blockExtents, std::array< CBandsHolderPtr, NORMALS_DATA::NB_FIELDS>& bandHolder, CNormalsStation& station)
	{
		int firstRefYear = GetPeriodFirstYear(m_options.m_reference_period);
		int firstCCYear = GetPeriodFirstYear(p);
		size_t nbRefYears = 30;
		size_t nbCCYears = 30;
		size_t nbNeighbor = m_options.m_nbNeighbor;
		double maxDistance = m_options.m_maxDistance;
		double power = m_options.m_power;



		if (!bandHolder[DEL_STD] || !bandHolder[EPS_STD] || !bandHolder[RELH_SD] || !bandHolder[WNDS_SD])
			return true;


		//		CGeoExtents extents = bandHolder[TMIN_MN]->GetExtents();
				//CProjectionTransformation PT(CProjectionManager::GetPrj(PRJ_WGS_84), CProjectionManager::GetPrj(blockExtents.GetPrjID()));
				////const CMonthlyMeanGrid& me = *this;
				//
		CGeoPoint pt(station.m_lon, station.m_lat, PRJ_WGS_84);
		//pt.Reproject(PT);
		//
		//CGeoPointIndex index = blockExtents.CoordToXYPos(pt);
		//
		//if (!blockExtents.IsInside(index))
		//	return false;
		//
		//
		//CGeoPointIndexVector pts;
		//int level = (int)ceil((sqrt((double)nbNeighbor) - 1) / 2);
		//blockExtents.GetNearestCellPosition(pt, Square((level + 1) * 2 + 1), pts);
		//
		//std::vector<double> d;
		//for (size_t i = 0; i < pts.size(); i++)
		//{
		//	CGeoPoint pti = blockExtents.XYPosToCoord(pts[i]);
		//	double di = max(0.000001, pt.GetDistance(pti));
		//	if (di < maxDistance)
		//		d.push_back(di);
		//}
		//
		//pts.erase(pts.begin() + d.size(), pts.end());
		//if (pts.empty())
		//	return false;

		CGeoPointIndexVector pts;
		std::vector<double> d;
		if (!GetNearestPoints(nbNeighbor, maxDistance, power, pt, blockExtents, pts, d, bandHolder))
			return false;



		double refMonthlyMean[12][NB_FIELDS] = { 0 };
		double ccMonthlyMean[12][NB_FIELDS] = { 0 };

		if (!GetMonthlyMean(firstRefYear, nbRefYears, nbNeighbor, power, pts, d, refMonthlyMean, bandHolder))
			return false;

		if (!GetMonthlyMean(firstCCYear, nbCCYears, nbNeighbor, power, pts, d, ccMonthlyMean, bandHolder))
			return false;


		CNormalsData data = station;
		for (size_t m = 0; m < 12; m++)
		{
			for (size_t v = 0; v < NB_FIELDS; v++)
			{
				if (!IsMissing(data[m][v]))
				{
					if (!IsMissing(ccMonthlyMean[m][v]) && !IsMissing(refMonthlyMean[m][v]))
					{
						if (v == DEL_STD || v == EPS_STD || v == PRCP_SD || v == RELH_SD)
						{
							data[m][v] *= float(ccMonthlyMean[m][v] / refMonthlyMean[m][v]);
							ASSERT(data[m][v] < 2000);
						}
						else if (v == WNDS_SD)
						{
							data[m][v] += float(log(ccMonthlyMean[m][v]) - log(refMonthlyMean[m][v]));
							if (data[m][v] < 0.0001)//sometime, variance are negative, in this case we take the variance of future period...
								data[m][v] = (float)log(ccMonthlyMean[m][v]);

							ASSERT(data[m][v] > 0);
						}
					}
				}
			}
		}

		((CNormalsData&)station) = data;

		return true;
	}

	bool CNormalsCreator::GetMonthlyMean(int firstYear, size_t nbYears, size_t nbNeighbor, double power, const CGeoPointIndexVector& pts, const std::vector<double>& d, double monthlyMean[12][NB_FIELDS], std::array< CBandsHolderPtr, NORMALS_DATA::NB_FIELDS>& bandHolder)const
	{
		//ASSERT(firstYear >= m_firstYear && firstYear <= m_lastYear);

		ERMsg msg;
		bool bRep = true;
		CStatistic MMStat[12][NB_FIELDS];


		for (size_t y = 0; y < nbYears && msg; y++)
		{
			int year = firstYear + int(y);
			for (size_t m = 0; m < 12 && msg; m++)
			{
				for (size_t v = 0; v < NB_FIELDS && msg; v++)
				{
					size_t vv = v;
					//there are never MMG for PRCP_SD, we use the value of PRCP_TT
					if (v == PRCP_SD)
						vv = PRCP_TT;

					float value = GetMonthlyMean(vv, year, m, nbNeighbor, power, pts, d, bandHolder);
					if (!IsMissing(value))
						MMStat[m][v] += value;

					//			msg += callback.StepIt(0);
				}
			}
		}
		//callback.PopTask();

		for (size_t m = 0; m < 12; m++)
		{
			for (size_t v = 0; v < NB_FIELDS; v++)
			{
				if (MMStat[m][v][NB_VALUE] >= 10)//Need at least 10 years of data
				{
					if (v == PRCP_SD)
						monthlyMean[m][v] = MMStat[m][v][COEF_VAR];//use coefficient of variation for precipitation
					else monthlyMean[m][v] = MMStat[m][v][MEAN];
				}
				else
				{
					monthlyMean[m][v] = MISSING;
					//if (m_grid[v].IsOpen())
						//bRep = false;
				}
			}
		}

		return bRep;
	}

	bool CNormalsCreator::GetMonthlyValues(int firstYear, size_t nbYears, size_t nbNeighbor, double maxDistance, double power, const CGeoPoint& ptIn, CGeoExtents blockExtents, std::vector< std::array<std::array<float, NB_FIELDS>, 12>>& values, std::array< CBandsHolderPtr, NORMALS_DATA::NB_FIELDS>& bandHolder)const
	{
		//ASSERT(firstYear >= m_firstYear && firstYear <= m_lastYear);

		ERMsg msg;


		CGeoPointIndexVector pts;
		std::vector<double> d;
		if (!GetNearestPoints(nbNeighbor, maxDistance, power, ptIn, blockExtents, pts, d, bandHolder))
			return false;

		values.resize(nbYears);
		for (size_t y = 0; y < nbYears && msg; y++)
		{
			int year = firstYear + int(y);
			for (size_t m = 0; m < 12 && msg; m++)
			{
				for (size_t f = 0; f < NB_FIELDS && msg; f++)
				{
					values[y][m][f] = GetMonthlyMean(f, year, m, nbNeighbor, power, pts, d, bandHolder);
					//msg += callback.StepIt(0);
				}
			}
		}

		return msg;
	}

	bool CNormalsCreator::GetNearestPoints(size_t nbNeighbor, double maxDistance, double power, const CGeoPoint& ptIn, CGeoExtents blockExtents, CGeoPointIndexVector& pts, std::vector<double>& d, std::array< CBandsHolderPtr, NORMALS_DATA::NB_FIELDS>& bandHolder)const
	{
		ASSERT(bandHolder[TMIN_MN]);

		//CGeoExtents extents = bandHolder[TMIN_MN]->GetExtents();
		CGeoPoint pt(ptIn);
		if (pt.GetPrjID() != blockExtents.GetPrjID())
		{
			pt.Reproject(CProjectionTransformationManager::Get(pt.GetPrjID(), blockExtents.GetPrjID()));
		}

		CGeoPointIndex index = blockExtents.CoordToXYPos(pt);


		if (!blockExtents.IsInside(index))
			return false;



		int level = (int)ceil((sqrt((double)nbNeighbor) - 1) / 2);
		blockExtents.GetNearestCellPosition(pt, Square((level + 1) * 2 + 1), pts);


		for (size_t i = 0; i < pts.size(); i++)
		{
			CGeoPoint pti = blockExtents.XYPosToCoord(pts[i]);
			double di = max(0.000001, pt.GetDistance(pti));
			if (di < maxDistance)
				d.push_back(di);
		}

		pts.erase(pts.begin() + d.size(), pts.end());

		if (pts.empty())
			return false;

		return true;
	}

	float CNormalsCreator::GetMonthlyMean(size_t v, int year, size_t m, size_t nbNeighbor, double power, const CGeoPointIndexVector& pts, const std::vector<double>& d, std::array< CBandsHolderPtr, NORMALS_DATA::NB_FIELDS>& bandHolder)const
	{
		ASSERT(m_MMG.m_firstYear != -1);
		ASSERT(pts.size() == d.size());
		ASSERT(year >= m_MMG.m_firstYear && year <= m_MMG.m_lastYear);

		if (!bandHolder[v])
			return MISSING;

		//CMonthlyMeanGrid& me = const_cast<CMonthlyMeanGrid&>(*this);

		size_t band = (year - m_MMG.m_firstYear) * 12 + m;
		return (float)bandHolder[v]->GetWindowMean(band, (int)nbNeighbor, power, pts, d);
	}

}