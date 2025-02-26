#include "StdAfx.h"

#include <deque>
#include "QualityControl.h"
#include "Basic\OpenMP.h"
#include "Basic\DailyDatabase.h"
#include "Basic\WeatherStation.h"
#include "Geomatic/UniversalKriging.h"
#include "isotree/include/isotree.hpp"
//#include "titanlib/include/titanlib.h"

#include "TaskFactory.h"
#include "../resource.h"
#include "WeatherBasedSimulationString.h"
#include "Geomatic/Variogram.h"
#include "Simulation/WeatherGenerator.h"
#include "Simulation/WeatherGeneration.h"

using namespace std;
using namespace WBSF::HOURLY_DATA;


namespace WBSF
{


	//int GetUnixTime(int year, int month, int day)
	//{
	//	std::tm tm = { /* .tm_sec  = */ 0,
	//		/* .tm_min  = */ 0,
	//		/* .tm_hour = */ 0,
	//		/* .tm_mday = */ (day),
	//		/* .tm_mon  = */ (month)-1,
	//		/* .tm_year = */ (year)-1900,
	//	};
	//	tm.tm_isdst = -1; // Use DST value from local time zone
	//	auto tp = std::chrono::system_clock::from_time_t(std::mktime(&tm));
	//	std::chrono::system_clock::duration d = tp.time_since_epoch();
	//	std::chrono::seconds s = std::chrono::duration_cast<std::chrono::seconds>(d);
	//
	//
	//	//std::chrono::day::days sec(1);
	//	//std::chrono::duration = ;
	//	//sys_seconds(tp);
	//
	//
	//	return s.count();
	//}


	//*********************************************************************

	static const bool TEST_SUBSET = false;
	static const double WTDR_THRESHOLD = 0.5;
	static const string TRANSFO_TYPE = "---";
	//static const double LOG_FACTOR = 2.0;
	static const double BOOST_FACTOR = 100.0;
	static const double LIMIT_SD = 1.5;

#if (NB_QC_VAR == 5)

	static const std::array<TVarH, NB_QC_VAR> VARIABLES = { H_TMIN, H_TMAX, H_PRCP, H_TDEW, H_WNDS };
	static const std::array<size_t, NB_QC_VAR> STAT = { MEAN, MEAN, SUM, MEAN, MEAN };
	static const std::array<char*, NB_QC_VAR> VAR_NAME = { "Tmin","Tmax","Prcp","Tdew","WndS" };

#elif (NB_QC_VAR == 3)

	static const std::array<TVarH, NB_QC_VAR> VARIABLES = { H_TMIN, H_TMAX, H_PRCP };
	static const std::array<size_t, NB_QC_VAR> STAT = { MEAN, MEAN, SUM };
	static const std::array<char*, NB_QC_VAR> VAR_NAME = { "Tmin","Tmax","Prcp" };

#else

	static const std::array<TVarH, NB_QC_VAR> VARIABLES = { H_PRCP };
	static const std::array<size_t, NB_QC_VAR> STAT = { SUM };
	static const std::array<char*, NB_QC_VAR> VAR_NAME = { "Prcp" };

#endif




	CQualityControl::CQualityControl(void)
	{}

	CQualityControl::~CQualityControl(void)
	{}

	ERMsg CQualityControl::Execute(CCallback& callback)
	{
		ERMsg msg;

		/*CStatisticXYEx test;
		double x[10] = {-101.58296, -101.16128, -100.10001, -100.61798, -100.06395,  -99.40746, -101.56898,  -99.26676,  -98.12333,  -99.94295};
		double y[10] = { 0.002221049, 0.024639853, 0.001783439, 0.001551815, 0.014108080, 0.005102343, 0.007075568, 0.004918996, 0.001482667, 0.009401860 };



		for (size_t i = 0; i < 10; i++)
			test.Add(x[i], y[i]);

		vector<double> cook = test.GetCookDistance();*/

		//msg = ExecuteHourly(callback);
		msg.ajoute("Not implemented yet.");
		/*


				CRegistry registry;
				string DEM_filePath = registry.GetString(CRegistry::GetGeoRegistryKey(L_WORLD_SRTM));
		*/

		//msg = CheckBasicValue(callback);
		//msg = CheckDailyWithNormal(callback);
		//msg = CreateVariogram(callback);

		return msg;
	}




	class CWeatherCorrection
	{
	public:

		string m_stationID;
		CTPeriod m_period;
		string m_error;

	};

	typedef std::deque<CWeatherCorrection> CWeatherCorrectionDeque;

	CWeatherCorrectionDeque DoInternalValidation(CWeatherStation station)
	{
		CWeatherCorrectionDeque report;

		return report;
	}

	ERMsg CQualityControl::CreateValidation(CCallback& callback)
	{
		ERMsg msg;

		//CPointData pPts;


		StringVector file_path;

		//file_path.push_back("G:\\Weather\\Canada-USA 1980-2020.DailyDB");
		//file_path.push_back("G:\\Weather\\Canada-USA 1920-2021.DailyDB");


		file_path.push_back("G:\\WeatherQc\\clip-quebec 1980-2020.DailyDB");
		//file_path.push_back("G:\\WeatherQc\\CoteNordFinal.DailyDB");
		//file_path.push_back("G:\\WeatherQc\\MDDELCC 2000-2020.DailyDB");
		//file_path.push_back("G:\\WeatherQc\\SM 2000-2020.DailyDB");
		//file_path.push_back("G:\\WeatherQc\\CIPRA 2000-2020.DailyDB");

		string output_file_path = "G:\\Travaux\\QualityControl\\Qc 2020.variogram.bin";


		int first_year = 2002;
		int last_year = 2002;

		set<int> years;
		for (int year = first_year; year <= last_year; year++)
			years.insert(year);


		//if (msg)
		//	msg = LoadStations(file_path, pPts, years, callback);


		callback.PushTask("Open weather databases (" + to_string(file_path.size()) + " databases)", file_path.size());

		size_t nb_stations = 0;
		vector<CDailyDatabase> DBs(file_path.size());
		for (size_t i = 0; i < file_path.size() && msg; i++)
		{
			msg += DBs[i].Open(file_path[i], CDailyDatabase::modeRead, callback);
			if (msg)
				nb_stations += DBs[i].size();

			msg += callback.StepIt();
		}

		callback.PopTask();


		ofStream file;
		if (msg)
			msg = file.open(output_file_path, ios_base::out | ios_base::binary);


		if (msg)
		{
			callback.PushTask("Creation of validation for all years (" + to_string(years.size()) + ")", years.size());

			for (size_t y = 0; y < years.size(); y++)
			{
				int year = first_year + int(y);

				callback.PushTask("Load weather data (" + to_string(nb_stations) + " stations) for year: " + to_string(year), nb_stations);

				vector<CWeatherStationVector> stations(DBs.size());
				for (size_t i = 0; i < DBs.size() && msg; i++)
				{
					stations[i].resize(DBs[i].size());
					for (size_t j = 0; j < DBs[i].size() && msg; j++)
					{
						msg += DBs[i].Get(stations[i][j], j, years);
						if (msg)
						{
							//string report = DoInternalValidation( stations[i][j] );
							//if(!report.empty())
							//	file << report << endl;
						}
					}
					msg += callback.StepIt();
				}

				callback.PopTask();

				callback.PushTask("Create validation for all days (" + to_string(WBSF::GetNbDaysPerYear(year)) + " days) for year: " + to_string(year), WBSF::GetNbDaysPerYear(year) * 3);
				//				size_t y = distance(years.begin(), it);

				for (size_t m = 0; m < 12 && msg; m++)//for all months
				{
					for (size_t d = 0; d < GetNbDayPerMonth(year, m) && msg; d++)//for all days
					{
						for (size_t v = 0; v < NB_QC_VAR && msg; v++)//for all variables
						{

							callback.PushTask("Create data points (" + to_string(nb_stations) + " points) for day: " + CTRef(year, m, d).GetFormatedString(), nb_stations);


							CGridPointVectorPtr pPts;
							pPts->reserve(nb_stations);


							for (size_t i = 0; i < stations.size() && msg; i++)
							{
								for (size_t j = 0; j < stations[i].size() && msg; j++)
								{
									if (stations[i][j].IsYearInit(year))
									{
										const CStatistic value = stations[i][j][year][m][d][VARIABLES[v]];
										if (value.IsInit())
										{
											//bool bIsValid = IsValid(v, value[STAT[v]]);


											CGridPoint pt(stations[i][j].m_x, stations[i][j].m_y, stations[i][j].m_z, stations[i][j].GetSlope(), stations[i][j].GetAspect(), value[STAT[v]], stations[i][j].m_lat, stations[i][j].GetShoreDistance(), stations[i][j].GetPrjID());
											pPts->push_back(pt);
										}
									}
									msg += callback.StepIt();
								}
							}

							callback.PopTask();


							double test = CVariogram::GetSuggestedLag(*pPts);

							CGridInterpolParam parameter;
							parameter.m_nbLags = 20;
							parameter.m_lagDist = 20000;
							parameter.m_nbPoints = 35;
							parameter.m_variogramModel = int(CGridInterpolParam::POWER);
							parameter.m_detrendingModel = CGridInterpolParam::NO_DETRENDING;
							parameter.m_externalDrift = CGridInterpolParam::ED_ELEV;

							//string comment = GetString(IDS_MAP_OPTIMISATION);
							//callback.PushTask(comment, parameterset.size());

							CUniversalKriging UK;
							//initialize with this parameters set
							UK.SetParam(parameter);
							UK.SetDataset(pPts);
							msg = UK.Initialization(callback);
							if (msg)
							{
								CVariogram variogram;
								UK.GetVariogram(variogram);

								//compute calibration X-Validation
								CXValidationVector XValidation;
								UK.GetXValidation(CGridInterpolParam::O_CALIB_VALID, XValidation, callback);


								callback.PushTask("Create data points (" + to_string(nb_stations) + " points) for day: " + CTRef(year, m, d).GetFormatedString(), nb_stations);


								CGridPointVectorPtr pPts;
								pPts->reserve(nb_stations);


								for (size_t i = 0, ii = 0; i < stations.size() && msg; i++)
								{
									for (size_t j = 0; j < stations[i].size() && msg; j++, ii++)
									{
										if (stations[i][j].IsYearInit(year))
										{
											const CStatistic value = stations[i][j][year][m][d][VARIABLES[v]];
											if (value.IsInit())
											{
												ASSERT(fabs(XValidation[ii].m_observed - value[STAT[v]]) < 0.01);

												bool bValid = true;
												if (!bValid)
												{
													//							string tmp = FormatA("%04d,%02d,%s,%d,%d,%2.0lf,%s,%7.4lf,%7.4lf,%7.4lf,%7.4lf,%7.4lf", year, m + 1, VAR_NAME[v], a, b, double(c), d.c_str(), optim[i].m_variogram.GetNugget(), optim[i].m_variogram.GetSill(), optim[i].m_variogram.GetRange(), vR²_y, R²_y);
													//file << tmp << endl;
												}

												//CGridPoint pt(stations[i][j].m_x, stations[i][j].m_y, stations[i][j].m_z, stations[i][j].GetSlope(), stations[i][j].GetAspect(), value[STAT[v]], stations[i][j].m_lat, stations[i][j].GetShoreDistance(), stations[i][j].GetPrjID());
												//pPts->push_back(pt);
											}
										}
										msg += callback.StepIt();
									}
								}

								callback.PopTask();


							}


							msg += callback.StepIt();
						}//for all variable
					}//for all days
				}//for all months

				callback.PopTask();
			}//for all years

			callback.PopTask();
		}//if msg

		file.close();


		return msg;
	}

	ERMsg CQualityControl::LoadStations(const StringVector& file_path, std::vector<CLocationVector>& stations, CCallback& callback)
	{
		ERMsg msg;

		size_t nb_stations = 0;
		vector<CDailyDatabase> DBs(file_path.size());
		stations.resize(file_path.size());
		for (size_t i = 0; i < file_path.size() && msg; i++)
		{
			msg += DBs[i].Open(file_path[i], CDailyDatabase::modeRead, callback);
			if (msg)
			{
				for (size_t j = 0; j < DBs[i].size() && msg; j++)
					stations[i].push_back(DBs[i][j]);
			}

			msg += callback.StepIt(0);
		}

		return msg;
	}

	ERMsg CQualityControl::LoadStations(const StringVector& file_path, std::vector<CWeatherStationVector>& stations, int year, CCallback& callback)
	{
		ERMsg msg;

		callback.PushTask("Open weather databases (" + to_string(file_path.size()) + " databases)", file_path.size());

		size_t nb_stations = 0;
		vector<CDailyDatabase> DBs(file_path.size());
		stations.resize(file_path.size());
		for (size_t i = 0; i < file_path.size() && msg; i++)
		{
			msg += DBs[i].Open(file_path[i], CDailyDatabase::modeRead, callback);
			if (msg)
			{
				nb_stations += DBs[i].size();
				stations[i].resize(DBs[i].size());
			}


			msg += callback.StepIt();
		}

		callback.PopTask();

		if (msg)
		{
			callback.PushTask("Load weather data (" + to_string(nb_stations) + " stations)", nb_stations);

			for (size_t i = 0; i < DBs.size() && msg; i++)
			{

				size_t nb_j = TEST_SUBSET ? 100 : DBs[i].size();
				for (size_t j = 0; j < nb_j && msg; j++)
				{

					//CWeatherStation station;
					msg += DBs[i].Get(stations[i][j], j, year);
					//stations[i][j] = station;
					msg += callback.StepIt();
				}
			}

			callback.PopTask();
		}

		return msg;
	}


	ERMsg CQualityControl::LoadPts(std::vector<CWeatherStationVector>& stations, CQCPointInfo& pPts, CTRef TRef, TVarH v, CCallback& callback)
	{
		ERMsg msg;

		size_t nb_stations = 0;

		for (size_t i = 0; i < stations.size() && msg; i++)
		{
			nb_stations += stations[i].size();
			msg += callback.StepIt(0);
		}


		pPts.first.reset(new CGridPointVector);
		pPts.first->reserve(nb_stations);
		pPts.second.reserve(nb_stations);


		for (size_t i = 0; i < stations.size() && msg; i++)
		{
			for (size_t j = 0; j < stations[i].size() && msg; j++)
			{
				const CWeatherDay& wDay = stations[i][j].GetDay(TRef);
				const CStatistic& stat = wDay.GetStat(v);
				if (stat.IsInit())
				{
					double value = stat[MEAN];

					CGridPoint pt(stations[i][j].m_x, stations[i][j].m_y, stations[i][j].m_z, stations[i][j].GetSlope(), stations[i][j].GetAspect(), value, stations[i][j].m_lat, stations[i][j].GetShoreDistance(), stations[i][j].GetPrjID());
					pPts.first->push_back(pt);
					pPts.second.push_back({ i,j });
					msg += callback.StepIt(1.0 / nb_stations);
				}
			}
		}

		CANNSearch ANNSearch;

		CGridPointVectorPtr pts = pPts.first;
		ANNSearch.Init(pts, false, false);

		for (auto iit = pts->begin(); iit != pts->end() && msg; iit++)//for all points
		{
			CGridPointResultVector result;
			ANNSearch.Search(*iit, 5, result);
			if (result.rbegin()->d < 0.1)
				ANNSearch.Search(*iit, 25, result);

			for (size_t iii = 1; iii < result.size() && result[iii].d <= 0.1; iii++)//for all points
			{
				ASSERT(iii != result.size() - 1);//hummm there is at least 10 identical points...
				//same coord : add random distance
				iit->m_x += WBSF::Rand(-0.01, 0.01);
				iit->m_y += WBSF::Rand(-0.01, 0.01);
			}

			msg += callback.StepIt(1.0 / pts->size());
		}

		return msg;
	}

	ERMsg CQualityControl::LoadStations(const StringVector& file_path, std::vector<CLocationVector>& stations, CQCPointData& pPts, set<int> years, CCallback& callback)
	{
		ERMsg msg;

		callback.PushTask("Open weather databases (" + to_string(file_path.size()) + " databases)", file_path.size());

		size_t nb_stations = 0;
		vector<CDailyDatabase> DBs(file_path.size());
		stations.resize(file_path.size());
		for (size_t i = 0; i < file_path.size() && msg; i++)
		{
			msg += DBs[i].Open(file_path[i], CDailyDatabase::modeRead, callback);
			if (msg)
			{
				nb_stations += DBs[i].size();
				stations[i].resize(DBs[i].size());
			}


			msg += callback.StepIt();
		}

		callback.PopTask();

		if (msg)
		{
			callback.PushTask("Load weather data (" + to_string(nb_stations) + " stations)", nb_stations);
			//callback.AddMessage("Load weather station (" + to_string(nb_stations) + ")", nb_stations);
			//init pts
			for (size_t m = 0; m < 12; m++)
			{
				for (size_t v = 0; v < NB_QC_VAR; v++)
				{
					pPts[m][v].resize(years.size());
					for (size_t y = 0; y < years.size(); y++)
					{
						pPts[m][v][y].first.reset(new CGridPointVector);
						pPts[m][v][y].first->reserve(nb_stations);
						pPts[m][v][y].second.reserve(nb_stations);
					}
				}
			}


			for (size_t i = 0; i < DBs.size() && msg; i++)
			{

				size_t nb_j = TEST_SUBSET ? 100 : DBs[i].size();
				for (size_t j = 0; j < nb_j && msg; j++)
				{

					CWeatherStation station;
					msg += DBs[i].Get(station, j, years);
					stations[i][j] = station;


					for (size_t m = 0; m < 12 && msg; m++)//for all months
					{
						for (size_t v = 0; v < NB_QC_VAR && msg; v++)//for all variables
						{
							for (auto it = years.begin(); it != years.end() && msg; it++)//for all variables
							{
								//table daily values of the 15 of each month
//								static const array<TVarH, 3> VARIABLES = { H_TMIN, H_TMAX, H_PRCP };
	//							static const array<size_t, 3> STAT = { MEAN, MEAN, SUM };

								size_t y = distance(years.begin(), it);
								int year = *it;
								const CStatistic stat = station[year][m][DAY_15][VARIABLES[v]];
								if (stat.IsInit())
								{
									double value = stat[STAT[v]];


									//if (v < NB_QC_VAR-1)
									//{
									//if (VAR_NAME[v] == "Prcp")
									//{

									//	//value = exp(value/ BOOST_FACTOR);
									//	//value = log(value*BOOST_FACTOR + LOG_FACTOR);
									//	//value = sqrt(value);
									//}
									//else if (VAR_NAME[v] == "WtDr")
									//{
									//	//double x = (value < WTDR_THRESHOLD ? 1.0 / 1000.0 : 999.0 / 1000.0);
									//	//value = log(x / (1 - x));

									//	value = (value < WTDR_THRESHOLD ? 0.0 : 1.0);
									//}



									CGridPoint pt(station.m_x, station.m_y, station.m_z, station.GetSlope(), station.GetAspect(), value, station.m_lat, station.GetShoreDistance(), station.GetPrjID());
									pPts[m][v][y].first->push_back(pt);
									pPts[m][v][y].second.push_back({ i,j });
								}

								msg += callback.StepIt(1.0 / (NB_QC_VAR * 12 * years.size()));
							}
						}//for all variable
					}//for all months
				//}////if have year
				}//for all station
			}//for all database


			callback.PopTask();
		}//if msg

		//verify there is no duplication
		callback.PushTask("verify there is no duplication", 12 * NB_QC_VAR * years.size());


		for (size_t m = 0; m < 12 && msg; m++)//for all months
		{
			for (size_t v = 0; v < NB_QC_VAR && msg; v++)//for all variables
			{
				for (auto it = years.begin(); it != years.end() && msg; it++)//for all variables
				{
					size_t y = distance(years.begin(), it);
					int year = *it;
					CANNSearch ANNSearch;

					CGridPointVectorPtr pts = pPts[m][v][y].first;
					ANNSearch.Init(pts, false, false);

					for (auto iit = pts->begin(); iit != pts->end() && msg; iit++)//for all points
					{
						CGridPointResultVector result;
						ANNSearch.Search(*iit, 5, result);
						if (result.rbegin()->d < 0.1)
							ANNSearch.Search(*iit, 25, result);

						for (size_t iii = 1; iii < result.size() && result[iii].d <= 0.1; iii++)//for all points
						{
							ASSERT(iii != result.size() - 1);//hummm there is at least 10 identical points...
							//same coord : add random distance
							iit->m_x += WBSF::Rand(-0.01, 0.01);
							iit->m_y += WBSF::Rand(-0.01, 0.01);
						}

						msg += callback.StepIt(1.0 / pts->size());
					}
				}
			}
		}

		callback.PopTask();

		return msg;
	}

	ERMsg CQualityControl::CreateVariogram(CCallback& callback)
	{
		ERMsg msg;

		//static const array<char*, 3> VAR_NAME = { "Tmin","Tmax","Prcp" };
		//array < array<vector<CGridPointVectorPtr>, 3>, 12> m_pPts;



		StringVector file_path;

		//
		//if (TEST_SUBSET)
			//file_path.push_back("G:\\Weather\\Canada-USA 2020-2021.DailyDB");
		//else
			//file_path.push_back("G:\\Weather\\Canada-USA 1980-2020.DailyDB");
		//string factor = WBSF::ReplaceString(to_string(LOG_FACTOR), ".", ",");

		//file_path.push_back("G:\\WeatherQc\\clip-quebec 1980-2020.DailyDB");
		//file_path.push_back("G:\\WeatherQc\\CoteNordFinal.DailyDB");
		//file_path.push_back("G:\\WeatherQc\\MDDELCC 2000-2020.DailyDB");
		//file_path.push_back("G:\\WeatherQc\\SM 2000-2020.DailyDB");
		//file_path.push_back("G:\\WeatherQc\\CIPRA 2000-2020.DailyDB");
		//file_path.push_back("G:\\WeatherQc\\AgroMeteo 2016-2020.DailyDB");
		//file_path.push_back("G:\\WeatherQc\\SOPFEU 1995-2020.DailyDB");
		file_path.push_back("G:\\Weather\\Daily\\Quebec+SOPFEU+Buffer 1991-2020.DailyDB");

		//file_path.push_back("G:\\WeatherQc\\CIPRA 2018.DailyDB");
		//file_path.push_back("G:\\WeatherQc\\AgroMeteo 2018.DailyDB");
		//file_path.push_back("G:\\WeatherQc\\MDDELCC 2018.DailyDB");
		//file_path.push_back("G:\\WeatherQc\\AgroMeteo+CIPRA+MDDELCC 2018.DailyDB");



		string limit = WBSF::ReplaceString(to_string(LIMIT_SD), ".", ",");


		string output_file_path = "G:\\Travaux\\QualityControl\\Qc 1991-2020(" + TRANSFO_TYPE + limit + ").variogram.csv";
		string output_file_path2 = "G:\\Travaux\\QualityControl\\Qc 1991-2020(" + TRANSFO_TYPE + limit + ").csv";

		ofStream file;
		msg = file.open(output_file_path);
		if (msg)
		{
			//write header
			file << "Year,Month,Variables,NbPoints,NbLags,LagDist,Type,Nugget,Sill,Range,VariogramR2,XvalidationR2" << endl;
			file.close();
		}

		ofStream file2;
		if (msg)
			msg = file2.open(output_file_path2);

		if (msg)
		{
			//write header
			//IsolationForestsScore
			file2 << "BD,KeyID,Year,Month,Day,Variables,Obs,Sim,NormalLo,NormalHight,Error,CookD,IFS,NormalsValid" << endl;
			file2.close();
		}

		


		int first_year = 2002;
		int last_year = 2002;
		//int first_year = 2018;
		//int last_year = 2018;

		std::vector<std::vector< array < array<CStatistic, NB_VAR_H>, 12>>> normals_stats;
		

		vector<CLocationVector> locations;

		if (msg)
			msg = LoadStations(file_path, locations, callback);

		size_t nbNormalsToUpdate = 0;
		for (size_t i = 0; i < locations.size() && msg; i++)
		{
			nbNormalsToUpdate += locations[i].size();
		}

		callback.PushTask("Create Normals (" + to_string(nbNormalsToUpdate) + ")", nbNormalsToUpdate);

		CWeatherGenerator WGBase;
		if (msg)
		{
			msg = InitDefaultWG(WGBase, callback);
		}
		if (msg)
		{
			normals_stats.resize(locations.size());
			for (size_t i = 0; i < locations.size() && msg; i++)
			{
				normals_stats[i].resize(locations[i].size());
				for (size_t j = 0; j < (TEST_SUBSET ? 100 : locations[i].size()) && msg; j++)
				{
					CWeatherGenerator WG(WGBase);
					WG.SetTarget(locations[i][j]);
					msg += WG.Initialize();//create gradient
					msg += WG.Generate(callback);

					if (msg)
					{

						//Compute Normals statistics

						for (size_t r = 0; r < WG.GetNbReplications() && msg; r++)
						{
							const CSimulationPoint& simulationPoint = WG.GetWeather(r);

							for (size_t y = 0; y < simulationPoint.size() && msg; y++)
							{
								for (size_t m = 0; m < simulationPoint[y].size() && msg; m++)//for all months
								{
									for (size_t d = 0; d < simulationPoint[y][m].size() && msg; d++)//for all days
									{
										for (TVarH v = H_FIRST_VAR; v < NB_VAR_H && msg; v++)//for all variables
										{
											normals_stats[i][j][m][v] += simulationPoint[y][m][d][v][MEAN];
										}
									}
								}
							}
						}
					}

					msg += callback.StepIt();
				}
			}

			callback.PopTask();

		}

		
		for (int year = first_year; year <= last_year&&msg; year++)
		{

			vector<CWeatherStationVector> stations;


			if (msg)
				msg = LoadStations(file_path, stations, year, callback);

			if (msg)
			{




				callback.PushTask("Validation for year "+to_string(year)+" of all days/variables (" + to_string(NB_QC_VAR * GetNbDaysPerYear(year)) + ")", NB_QC_VAR * GetNbDaysPerYear(year));


				//array < array<CVariogram, NB_QC_VAR>, 12> variogram;
				for (size_t m = 0; m < 12 && msg; m++)//for all months
				{


					for (size_t d = 0; d < GetNbDayPerMonth(year, m) && msg; d++)//for all days
					{
						for (size_t v = 0; v < NB_QC_VAR && msg; v++)//for all variables
						//size_t v = 0;
						{
							//Load pts
							CQCPointInfo pPts;
							CTRef TRef = CTRef(year, m, d);
							msg = LoadPts(stations, pPts, TRef, VARIABLES[v], callback);

							COptimizeInfo optim;
							if (msg)
								msg = OptimizeParameter(v, pPts, optim, callback);




							if (msg)
							{

								ofStream file;
								msg = file.open(output_file_path, ios_base::out | ios_base::app);
								ofStream file2;
								msg += file2.open(output_file_path2, ios_base::out | ios_base::app);
								if (msg)
								{
									//map < size_t, map < int, map<int, map<string, pair< CStatistic, CXValidationVector>>>>> all_years;


									//for (size_t i = 0; i < optim.size(); i++)//for all variables
									//{
									//	size_t a= optim[i].m_param.m_nbPoints;
									//	int b = optim[i].m_param.m_nbLags;
									//	int c = int(optim[i].m_param.m_lagDist);
									//	string d = optim[i].m_variogram.GetModelName();
									//	
									//	all_years[a][b][c][d].first += max(0.0, optim[i].m_variogram.GetR2());//not the best but..
									//	all_years[a][b][c][d].second.insert(all_years[a][b][c][d].second.end(),optim[i].m_XValidation.begin(), optim[i].m_XValidation.end());
									//}
									//	


									size_t nrow = optim.m_XValidation.size();
									size_t ncol = 2;
									//for (size_t i = 0; i < optim.size(); i++)//for all parameter set
									//nrow = 

									std::vector<double> X(nrow * ncol);
#define get_ix(row, col) (row + col*nrow)

									CStatisticXYEx Xval_stat;
									//for (size_t i = 0, jj = 0; i < optim.size(); i++)//for all parameter set
									//{
									for (size_t j = 0; j < optim.m_XValidation.size(); j++)
									{
										ASSERT(fabs(optim.m_XValidation[j].m_observed - -999) > EPSILON_NODATA);
										ASSERT(fabs(optim.m_XValidation[j].m_predicted - -999) > EPSILON_NODATA);
										Xval_stat.Add(optim.m_XValidation[j].m_predicted, optim.m_XValidation[j].m_observed);


										X[get_ix(j, 0)] = optim.m_XValidation[j].m_observed;
										X[get_ix(j, 1)] = optim.m_XValidation[j].m_predicted;
									}
									//}

									vector<double> CookD = Xval_stat.GetCookDistance();
									// Fit a small isolation forest model
										//(see 'fit_model.cpp' for the documentation) 
									ExtIsoForest iso;
									fit_iforest(NULL, &iso,         //IsoForest *model_outputs, ExtIsoForest *model_outputs_ext,
										X.data(), ncol,				//real_t numeric_data[],  size_t ncols_numeric,
										NULL, 0, NULL,				//int    categ_data[],    size_t ncols_categ,    int ncat[],
										NULL, NULL, NULL,			//real_t Xc[], sparse_ix Xc_ind[], sparse_ix Xc_indptr[],
										2, 3, Normal, false,		//size_t ndim, size_t ntry, CoefType coef_type, bool coef_by_prop,
										NULL, false, false,			//real_t sample_weights[], bool with_replacement, bool weight_as_sample,
										nrow, nrow, 200, 			//size_t nrows, size_t sample_size, size_t ntrees,
										0, 0,						//size_t max_depth, size_t ncols_per_tree,
										true, true,					//bool   limit_depth, bool penalize_range,
										false, NULL,				//bool   standardize_dist, double tmat[],
										NULL, false,				//double output_depths[], bool standardize_depth,
										NULL, false,				//real_t col_weights[], bool weigh_by_kurt,
										0., 0.,						//double prob_pick_by_gain_avg, double prob_split_by_gain_avg,
										0., 0.,						//double prob_pick_by_gain_pl,  double prob_split_by_gain_pl,
										0., Impute,					//double min_gain, MissingAction missing_action,
										SubSet, Smallest,			//CategSplit cat_split_type, NewCategAction new_cat_action,
										false, NULL, 0,				//bool   all_perm, Imputer *imputer, size_t min_imp_obs,
										Higher, Inverse, false,		//UseDepthImp depth_imp, WeighImpRows weigh_imp_rows, bool impute_at_fit,
										1, omp_get_num_threads());	//uint64_t random_seed, int nthreads)

									/*solation.forest(
										df,
										sample_size = NROW(df),
										ntrees = 500,
										ndim = min(3, NCOL(df)),
										ntry = 3,
										categ_cols = NULL,
										max_depth = ceiling(log2(sample_size)),
										ncols_per_tree = NCOL(df),
										prob_pick_avg_gain = 0,
										prob_pick_pooled_gain = 0,
										prob_split_avg_gain = 0,
										prob_split_pooled_gain = 0,
										min_gain = 0,
										missing_action = ifelse(ndim > 1, "impute", "divide"),
										new_categ_action = ifelse(ndim > 1, "impute", "weighted"),
										categ_split_type = "subset",
										all_perm = FALSE,
										coef_by_prop = FALSE,
										recode_categ = TRUE,
										weights_as_sample_prob = TRUE,
										sample_with_replacement = FALSE,
										penalize_range = FALSE,
										weigh_by_kurtosis = FALSE,
										coefs = "normal",
										assume_full_distr = TRUE,
										build_imputer = FALSE,
										output_imputations = FALSE,
										min_imp_obs = 3,
										depth_imp = "higher",
										weigh_imp_rows = "inverse",
										output_score = FALSE,
										output_dist = FALSE,
										square_dist = FALSE,
										sample_weights = NULL,
										column_weights = NULL,
										random_seed = 1,
										nthreads = parallel::detectCores()*/


										/* Check which row has the highest outlier score
										   (see file 'predict.cpp' for the documentation) */
									std::vector<double> outlier_scores(nrow);
									predict_iforest(X.data(), NULL,     //real_t numeric_data[], int categ_data[],
										true, ncol, 0,					//bool is_col_major, size_t ncols_numeric, size_t ncols_categ,
										NULL, NULL, NULL,				//real_t Xc[], sparse_ix Xc_ind[], sparse_ix Xc_indptr[],
										NULL, NULL, NULL,				//real_t Xr[], sparse_ix Xr_ind[], sparse_ix Xr_indptr[],
										nrow, omp_get_num_threads(), true,//size_t nrows, int nthreads, bool standardize,
										NULL, &iso,						//IsoForest *model_outputs, ExtIsoForest *model_outputs_ext,
										outlier_scores.data(), NULL);	//double output_depths[],   sparse_ix tree_num[])


									//for (size_t i = 0, jj = 0; i < optim.size(); i++)//for all parameterset
									//{
									//std::set<int>::iterator it = years.begin();
									//std::advance(it, optim[i].m_y);
									//int year = *it;
									const vector<array<size_t, 2>>& info = pPts.second;


									/*size_t a = optim.m_param.m_nbPoints;
									int b = optim.m_param.m_nbLags;
									int c = int(optim.m_param.m_lagDist / 1000);
									string d = optim.m_variogram.GetModelName();


									double vR²_y = max(-9.999, optim.m_variogram.GetR2());
									double R²_y = max(-9.999, Xval_stat[COEF_D]);

									string tmp = FormatA("%04d,%02d,%s,%d,%d,%2.0lf,%s,%7.4lf,%7.4lf,%7.4lf,%7.4lf,%7.4lf", year, m + 1, VAR_NAME[v], a, b, double(c), d.c_str(), optim.m_variogram.GetNugget(), optim.m_variogram.GetSill(), optim.m_variogram.GetRange(), vR²_y, R²_y);
									file << tmp << endl;*/


									ASSERT(!optim.m_XValidation.empty() || optim.m_XValidation.size() == info.size());
									for (size_t j = 0; j < optim.m_XValidation.size(); j++)//for all observations
									{

										double obs = optim.m_XValidation[j].m_observed;
										double sim = optim.m_XValidation[j].m_predicted;
										double error = optim.m_error[j];
										double TITAN_S = -999;// optim[i].m_TITAN_Score[j];

										if (VAR_NAME[v] == "Prcp")
										{
											sim = max(0.0, sim);
										}


										string title = GetFileTitle(file_path[info[j][0]]);

										const CWeatherStation& station = stations[info[j][0]][info[j][1]];

										ASSERT(station[year][m][d][VARIABLES[v]].IsInit());
										//if (station[year][m][d][VARIABLES[v]].IsInit())
										//{
										float value = station[year][m][d][VARIABLES[v]][MEAN];
										ASSERT(fabs(obs - value) < 0.01);
										/*if (v == H_RELH && value > 100 && value <= 105)
										{
											value = 100;
											station[y][m][d].SetStat(v, value);
										}

										if (v == H_SNDH && value < 0)
										{
											value = 0;
											station[y][m][d].SetStat(v, value);
										}*/

										//size_t Jday = station[y][m][d].GetTRef().GetJDay();
										//ASSERT(normals_stats[info[j][0]].find(station.m_ID)!= normals_stats[info[j][0]].end());
										const array<CStatistic, NB_VAR_H>& stats = normals_stats[info[j][0]][info[j][1]][m];
										bool valid = IsNormalsCheckValid(VARIABLES[v], value, stats);

										string tmp = FormatA("%s,%s,%04d,%02d,%02d,%s,%7.4lf,%7.4lf,%7.4lf,%7.4lf,%7.4lf,%7.4lg,%7.4lf,%d", title.c_str(), stations[info[j][0]][info[j][1]].m_ID.c_str(), year, m + 1, d + 1, VAR_NAME[v], obs, sim, stats[VARIABLES[v]][LOWEST], stats[VARIABLES[v]][HIGHEST], error, CookD[j], outlier_scores[j], int(valid ? 1 : 0));
										file2 << tmp << endl;


										//bool bAdditif = v == true:false;


										//if (!valid)
										//{
											//log_file << station.m_ID << "," << station.m_name << "," << station[y][m][d].GetTRef().GetFormatedString() << "," << GetVariableAbvr(v) << "," << ToString(value) << "," << ToString(normals_stats[m][v][LOWEST]) << "," << ToString(normals_stats[m][v][HIGHEST]) << endl;
											//station[y][m][d].SetStat(v, CStatistic());//reset
										//}
									//}


									}

									//}


									/*for (auto it1 = all_years.begin(); it1 != all_years.end() && msg; it1++)//for all years
									{
										for (auto it2 = it1->second.begin(); it2 != it1->second.end() && msg; it2++)//for all months
										{
											for (auto it3 = it2->second.begin(); it3 != it2->second.end() && msg; it3++)//for all days
											{
												for (auto it4 = it3->second.begin(); it4 != it3->second.end() && msg; it4++)
												{
													const CStatistic& stat = it4->second.first;
													const CXValidationVector& Xval = it4->second.second;

													double vR² = stat[MEAN];
													double R² = max(-9.999, Xval.GetStatistic(-999)[COEF_D]);

													string tmp = FormatA("%04d,%02d,%s,%d,%d,%2.0lf,%s,%7.4lf,%7.4lf,%7.4lf,%7.4lf,%7.4lf", -999, m + 1, VAR_NAME[v], it1->first, it2->first, double(it3->first), it4->first.c_str(), -999.0, -999.0, -999.0, vR², R²);
													file << tmp << endl;
												}
											}
										}
									}
		*/

		//normals_stats[365] = normals_stats[364];//for leap years

									//for (size_t y = 0; y < station.size() && msg; y++)
									//{
									//	for (size_t m = 0; m < station[y].size() && msg; m++)//for all months
									//	{
									//		for (size_t d = 0; d < station[y][m].size() && msg; d++)//for all days
									//		{
									//			for (TVarH v = H_FIRST_VAR; v < NB_VAR_H && msg; v++)//for all variables
									//			{

									//				if (station[y][m][d][v].IsInit())
									//				{
									//					float value = station[y][m][d][v][MEAN];

									//					if (v == H_RELH && value > 100 && value <= 105)
									//					{
									//						value = 100;
									//						station[y][m][d].SetStat(v, value);
									//					}

									//					if (v == H_SNDH && value < 0)
									//					{
									//						value = 0;
									//						station[y][m][d].SetStat(v, value);
									//					}

									//					//size_t Jday = station[y][m][d].GetTRef().GetJDay();
									//					bool valid = IsNormalsCheckValid(v, value, normals_stats[m]);

									//					//bool bAdditif = v == true:false;

									//					if (!valid)
									//					{
									//						log_file << station.m_ID << "," << station.m_name << "," << station[y][m][d].GetTRef().GetFormatedString() << "," << GetVariableAbvr(v) << "," << ToString(value) << "," << ToString(normals_stats[m][v][LOWEST]) << "," << ToString(normals_stats[m][v][HIGHEST]) << endl;
									//						station[y][m][d].SetStat(v, CStatistic());//reset
									//					}
									//				}

									//				msg += callback.StepIt(0);
									//			}
									//		}//for all days
									//	}//for all months

									//	//msg += callback.StepIt();
									//}//for all years

									//callback.PopTask();

									//DBout.Add(station);


								//	msg += callback.StepIt();
								//}//if msg
							//}//for all stations




									file.close();
									file2.close();
								}


							}//if msg
						//}//for all years

							msg += callback.StepIt();

						}//for all days
					}//for all variable
				}//for all month


				callback.PopTask();
			}//if msg
		}


		return msg;
	}

	CGridInterpolParamVector CQualityControl::GetParamterset(size_t v)
	{
		CGridInterpolParam DEFAULT_PARAM;

		//CSugestedLagOptionNew op(10, 30, 5, 1, 3, 0.5);
		//op.LoadDefaultCtrl();

		//DEFAULT_PARAM.m_nbPoints = ;
		CGridInterpolParamVector parameterset;

		//static const size_t NB_PARAMTERS = 6;
		//static const size_t LAG_OPTIONS[NB_QC_VAR][NB_PARAMTERS][2] =
		//{
		//	{//Tmin
		//		{15,15},
		//		{15,25},
		//		{25,15},
		//		{20,20},
		//		{25,25},
		//		{30,30},
		//	},
		//	{//Tmax
		//		{15,15},
		//		{15,25},
		//		{25,15},
		//		{20,20},
		//		{25,25},
		//		{30,30},
		//	},
		//	{//Prcp
		//		{10,10},
		//		{10,15},
		//		{10,20},
		//		{15,15},
		//		{15,20},
		//		{15,25},
		//	},
		//	{//Tdew
		//		{10,10},
		//		{10,20},
		//		{10,25},
		//		{10,30},
		//		{15,30},
		//		{20,30},
		//	},
		//	{//WndS
		//		{10,10},
		//		{10,15},
		//		{10,20},
		//		{15,15},
		//		{15,20},
		//		{15,25},
		//	},

		//};
		// 
		static const size_t NB_PARAMTERS = 1;
		static const size_t LAG_OPTIONS[NB_QC_VAR][NB_PARAMTERS][2] =
		{
			{//Tmin
				{20,20},
			},
			{//Tmax
				{20,20},
			},
			{//Prcp
				{15,15},
			},
			{//Tdew
				{10,30},
			},
			{//WndS
				{15,15},
			},

		};
		//size_t nbStepLag = (size_t((op.m_nbLagMax - op.m_nbLagMin) / op.m_nbLagStep) + 1);
		//size_t nbStepLagDist = (size_t((op.m_lagDistMax - op.m_lagDistMin) / op.m_lagDistStep) + 1);
		//size_t nbStepPoints = 1;
		static const size_t NB_MODELS = 1;// CVariogram::NB_MODELS;
		static const size_t MODEL[NB_MODELS] = { CVariogram::POWER };//, CVariogram::EXPONENTIAL



		//int nbDetrending = 1;
		//int nbExternalDrift = 1;

		//size_t nbParamters = nbStepLag * nbStepLagDist*NB_MODELS;


		//double test = CVariogram::GetSuggestedLag(pts);
		double defaultLagDist = 1000;// CVariogram::GetSuggestedLag(*m_pPts);

		parameterset.resize(NB_PARAMTERS, DEFAULT_PARAM);

		for (size_t i = 0; i < NB_PARAMTERS; i++)
		{
			size_t index = i;


			//parameterset[index].m_nbLags = WBSF::Rand(5, 50);// int(op.m_nbLagMin + op.m_nbLagStep*i);
			//parameterset[index].m_lagDist = WBSF::Rand(1000, 50000);// defaultLagDist * (op.m_lagDistMin + j * op.m_lagDistStep);
			//parameterset[index].m_nbPoints = 35 + k * 10;

			parameterset[index].m_nbLags = int(LAG_OPTIONS[v][i][0]);
			parameterset[index].m_lagDist = defaultLagDist * LAG_OPTIONS[v][i][1];

			//parameterset[index].m_nbLags = 20;
			//parameterset[index].m_lagDist = 20000;
			parameterset[index].m_nbPoints = 35;


			parameterset[index].m_XvalPoints = 0.0;
			parameterset[index].m_bRegionalLimit = true;
			parameterset[index].m_bRegionalLimitToBound = true;
			parameterset[index].m_regionalLimitSD = LIMIT_SD;//attention pour toute les varaibles
			parameterset[index].m_variogramModel = MODEL[0];
			//parameterset[index].m_variogramModel = int(CGridInterpolParam::POWER);
			parameterset[index].m_detrendingModel = CGridInterpolParam::NO_DETRENDING;
			parameterset[index].m_externalDrift = CGridInterpolParam::ED_ELEV;
			parameterset[index].m_maxDistance = 10000000;
		}// for all 




		//size_t nbStepLag =  (size_t((op.m_nbLagMax - op.m_nbLagMin) / op.m_nbLagStep) + 1);
		//size_t nbStepLagDist =  (size_t((op.m_lagDistMax - op.m_lagDistMin) / op.m_lagDistStep) + 1);
		//size_t nbStepPoints = 1;
		//static const size_t NB_MODELS = 1;// CVariogram::NB_MODELS;
		//static const size_t MODEL[NB_MODELS] = { CVariogram::POWER};//, CVariogram::EXPONENTIAL



		////int nbDetrending = 1;
		////int nbExternalDrift = 1;

		//size_t nbParamters = nbStepLag * nbStepLagDist*NB_MODELS;

		////double test = CVariogram::GetSuggestedLag(pts);
		//double defaultLagDist = 10000;// CVariogram::GetSuggestedLag(*m_pPts);

		//parameterset.resize(nbParamters, DEFAULT_PARAM);

		//for (size_t i = 0; i < nbStepLag; i++)
		//{
		//	for (size_t j = 0; j < nbStepLagDist; j++)
		//	{
		//		for (size_t k = 0; k < NB_MODELS; k++)//put model at the and for optimization
		//		{
		//			size_t index = i * (nbStepLagDist*NB_MODELS) + j * NB_MODELS + k;


		//			//parameterset[index].m_nbLags = WBSF::Rand(5, 50);// int(op.m_nbLagMin + op.m_nbLagStep*i);
		//			//parameterset[index].m_lagDist = WBSF::Rand(1000, 50000);// defaultLagDist * (op.m_lagDistMin + j * op.m_lagDistStep);
		//			//parameterset[index].m_nbPoints = 35 + k * 10;
		//			
		//			parameterset[index].m_nbLags = int(op.m_nbLagMin + op.m_nbLagStep*i);
		//			parameterset[index].m_lagDist = defaultLagDist * (op.m_lagDistMin + j * op.m_lagDistStep);

		//			//parameterset[index].m_nbLags = 20;
		//			//parameterset[index].m_lagDist = 20000;
		//			parameterset[index].m_nbPoints = 35;


		//			parameterset[index].m_XvalPoints = 0.0;
		//			parameterset[index].m_bRegionalLimit = true;
		//			parameterset[index].m_bRegionalLimitToBound = true;
		//			parameterset[index].m_regionalLimitSD = LIMIT_SD;//attention pour toute les varaibles
		//			parameterset[index].m_variogramModel = MODEL[k];
		//			//parameterset[index].m_variogramModel = int(CGridInterpolParam::POWER);
		//			parameterset[index].m_detrendingModel = CGridInterpolParam::NO_DETRENDING;
		//			parameterset[index].m_externalDrift = CGridInterpolParam::ED_ELEV;
		//			parameterset[index].m_maxDistance = 10000000;


		//			//parameterset[index].m_bUseShore = false;
		//			//parameterset[index].m_externalDrift = CGridInterpolParam::ED_SHORE;
		//		}//for all model
		//	}//for all lag distance
		//}// for all nb lag



						//CGeoRegression m_externalDrift;
						//std::unique_ptr<CANNSearch> m_pANNSearch;
						//double m_defaultLagDist;



						//int em = m_param.m_externalDrift;
						////if (em < CGridInterpolParam::ED_STEPWISE)
						////{
						//m_externalDrift.resize(CGridInterpolParam::EX_DRIFT_TERM_DEFINE[em][0]);
						//for (size_t i = 0; i < CGridInterpolParam::EX_DRIFT_TERM_DEFINE[em][0]; i++)
						//	m_externalDrift[i] = CGridInterpolParam::EX_DRIFT_TERM_DEFINE[em][i + 1];
						//}
						//else
						//{
						//	if (m_externalDrift.empty())
						//	{
						//		CGridInterpolParam  sr_param;
						//		sr_param.m_regressOptimization = m_param.m_regressOptimization;
						//		sr_param.m_regressCriticalR2 = m_param.m_regressCriticalR2;
						//		sr_param.m_bUseLatLon = false;
						//		sr_param.m_bUseElevation = true;
						//		sr_param.m_bUseExposition = false;
						//		sr_param.m_bUseShore = true;

						//		CSpatialRegression sr;
						//		sr.SetParam(sr_param);
						//		sr.SetDataset(m_pPts);//send all point because sr remove 
						//		sr.Initialization(callback);

						//		m_externalDrift = sr.GetParam().m_regressionModel;

						//		//Compute with the real parameters
						//		m_externalDrift.Compute(*pCalibPts, m_prePostTransfo);
						//	}
						//}

						/*m_ed_stat.clear();
						m_ed_stat.resize(m_externalDrift.size());
						for (size_t i = 0; i < pCalibPts->size(); i++)
						{
							const CGridPoint& pt = m_pPts->at(i);
							for (size_t ii = 0; ii < m_externalDrift.size(); ii++)
							{
								m_ed_stat[ii] += m_externalDrift[ii].GetE(pt);
							}
						}*/

						//fill-in information
						/*CParamUK m_p;
						m_p.m_nbPoint = m_param.m_nbPoints;
						m_p.m_xsiz = m_info.m_cellSizeX;

						m_p.m_ysiz = m_info.m_cellSizeY;*/




		return parameterset;
	}

	ERMsg CQualityControl::OptimizeParameter(size_t v, const CQCPointInfo& pts, COptimizeInfo& p, CCallback& callback)
	{
		ERMsg msg;


		CGridInterpolParamVector parameterset = GetParamterset(v);
		//p.resize(pts.size()*parameterset.size());
		//p.resize(parameterset.size());

		string comment = GetString(IDS_MAP_OPTIMISATION);
		//callback.PushTask(comment, parameterset.size());

		CTimer timer;
		timer.Start();

		CGridInterpolInfo info;
		//info.m_bMulti = true;
		//info.m_nbCPU = 8;

		//for (size_t y = 0; y < pts.size() && msg; y++)//for all years
		//{
		if (pts.first->size() > 35)
		{
			CUniversalKriging UK;
			UK.SetInfo(info);
			UK.SetDataset(pts.first);

			COptimizeInfoVector pp(parameterset.size());
			for (size_t i = 0; i < parameterset.size() && msg; i++)
			{
				//initialize with this parameters set


				UK.SetParam(parameterset[i]);
				msg = UK.Initialization(callback);
				if (msg)
				{
					//UK.m_p.m_radius = parameterset[i].m_nbLags*parameterset[i].m_lagDist*1000;
//verify there is no duplication

					pp[i].m_y = 0;
					pp[i].m_param = parameterset[i];
					//pp[y*parameterset.size() + i].m_y = y;
					//pp[y*parameterset.size() + i].m_param = parameterset[i];
					//UK.GetVariogram(pp[y*parameterset.size() + i].m_variogram);
					UK.GetVariogram(pp[i].m_variogram);

					//compute calibration X-Validation
					//CXValidationVector XValidation;
					//UK.GetXValidation(CGridInterpolParam::O_CALIB_VALID, pp[y*parameterset.size() + i].m_XValidation, callback);
					//UK.GetXValidation(CGridInterpolParam::O_CALIB_VALID, pp[i].m_XValidation, callback);
					pp[i].m_XValidation.resize(pts.first->size());
					pp[i].m_error.resize(pts.first->size());

#pragma omp parallel for num_threads( info.m_nbCPU ) if (info.m_bMulti)
					for (int j = 0; j < pts.first->size(); j++)
					{
						//int l = (int)ceil((i) / m_inc);
						//int ii = int(l*m_inc);

						const CGridPoint& pt = pts.first->at(j);

						pp[i].m_XValidation[j].m_observed = pt.m_event;
						pp[i].m_XValidation[j].m_predicted = UK.EvaluateWithError(pt, j, pp[i].m_error[j]);
					}
				}

				msg += callback.StepIt();
			}

			map<double, size_t, std::greater<double>> map;
			//select the best parameterset
			for (size_t i = 0; i < pp.size(); i++)//for all parameterset
			{
				double XValR² = pp[i].m_XValidation.GetStatistic(-999)[COEF_D];
				double varioR² = pp[i].m_variogram.GetR2();

				double R² = XValR² * 4.0 / 5.0 + varioR² * 1.0 / 5.0;
				map[R²] = i;
			}

			//size_t i = 0;
			//for (auto it = map.begin(); it != map.end(); it++, i++)//for all parameterset
			//p = pp[it->second];

			p = pp[map.begin()->second];
			//sort(p.begin(), p.end(), );

			//svec key_ID;
			//svec name;
			//vec ilats;
			//vec ilons;
			//vec ialts;
			//vec ivals;
			//
			//
			//
			//
			//key_ID.resize(pts[y].first->size());
			//name.resize(pts[y].first->size());
			//ilats.resize(pts[y].first->size());
			//ilons.resize(pts[y].first->size());
			//ialts.resize(pts[y].first->size());
			//ivals.resize(pts[y].first->size());
			//
			//
			//for (size_t i = 0; i < pts[y].first->size(); i++)
			//{
			//	//key_ID.push_back(station.m_ID);
			//	//name.push_back(station.m_name);
			//	ilats[i] = pts[y].first->at(i).m_lat;
			//	ilons[i] = pts[y].first->at(i).m_lon;
			//	ialts[i] = pts[y].first->at(i).m_alt;
			//	ivals[i] = pts[y].first->at(i).m_event;
			//}


			//titanlib::Dataset dataset1(key_ID, name, ilats, ilons, ialts, ivals);

			//int num_min = 5;
			//int num_max = 100;
			//float inner_radius = 5000;
			//float outer_radius = 150000;
			//int num_iterations = 5;
			//int num_min_prof = 30;
			//float min_elev_diff = 50;
			//float min_horizontal_scale = 1000;
			//float vertical_scale = 100;
			//vec t2pos(ilats.size(), 4);
			//vec t2neg(ilats.size(), 8);
			//vec eps2(ilats.size(), 0.5);


			////vec prob_gross_error;
			////vec rep;
			//dataset1.sct(
			//	num_min, num_max, inner_radius, outer_radius
			//	, num_iterations, num_min_prof, min_elev_diff, min_horizontal_scale
			//	, vertical_scale, t2pos, t2neg, eps2
			//);


			//p[y].m_TITAN_Score = dataset1.prob_gross_error();

			//
			//reinit after each year
			//parameterset = GetParamterset();
		}
		else
		{
			callback.AddMessage("WARNING: Not enough points for some year");
		}
		//}

		timer.Stop();

		//callback.PopTask();




		return msg;
	}



	ERMsg CQualityControl::ExecuteHourly(CCallback& callback)
	{
		ERMsg msg;
		//	string file_path_in = "G:\\Travaux\\QualityControl\\Weather\\Quebec 2018.HourlyDB";
		//	string file_path_out = file_path_in;
		//	SetFileExtension(file_path_out, ".HourlyQC.csv");


		//	CTPeriod period(CTRef(2018, JANUARY, DAY_01, 0), CTRef(2018, DECEMBER, DAY_31, 23));

		//	CHourlyDatabase db;
		//	msg = db.Open(file_path_in, CHourlyDatabase::modeRead, callback);
		//	if (msg)
		//	{
		//		ofStream file;
		//		msg = file.open(file_path_out);
		//		if (msg)
		//		{
		//			file << "KeyID,Name,Latitude,Longitude,Elevation,Date,Tmin,TminProb,TminFlag,Tmax,TmaxProb,TmaxFlag" << endl;
		//			file.close();

		//			callback.PushTask("Quality Control (" + to_string(period.GetNbYears()) + " years)", period.GetNbYears());


		//			for (int year = period.GetFirstYear(); year <= period.GetLastYear() && msg; year++)
		//			{
		//				CWeatherStationVector stations;
		//				stations.resize(db.size());


		//				callback.PushTask("Load weather for year " + to_string(year) + " (" + to_string(db.size()) + " stations)", db.size());
		//				for (size_t i = 0; i < db.size() && msg; i++)
		//				{
		//					db.Get(stations[i], i, year);
		//					msg += callback.StepIt();
		//				}
		//				callback.PopTask();

		//				CTPeriod period_year = period.Intersect(CTPeriod(CTRef(year, JULY, DAY_01, 0), CTRef(year, JULY, DAY_30, 23)));
		//				//CTPeriod period_year = period.Intersect(CTPeriod(CTRef(year, JANUARY, DAY_01, 0), CTRef(year, DECEMBER, DAY_31, 23)));
		//				callback.PushTask("Quality control for " + to_string(year) + " (" + ToString(period_year.GetNbHour()) + " hours)", period_year.GetNbHour());


		//				//for (size_t m = 0; m < 12 && msg; m++)
		//				//{
		//					//for (size_t d = 0; d < GetNbDayPerMonth(year, m)&&msg; d++)
		//				for (CTRef TRef = period_year.Begin(); TRef <= period_year.End() && msg; TRef++)
		//				{
		//					svec key_ID;
		//					svec name;
		//					vec ilats;
		//					vec ilons;
		//					vec ielevs;
		//					vec Tmin;
		//					vec Tmax;
		//					vec Prcp;


		//					key_ID.reserve(stations.size());
		//					name.reserve(stations.size());
		//					ilats.reserve(stations.size());
		//					ilons.reserve(stations.size());
		//					ielevs.reserve(stations.size());
		//					Tmin.reserve(stations.size());
		//					Tmax.reserve(stations.size());

		//					for (size_t i = 0; i < stations.size(); i++)
		//					{
		//						const CWeatherStation& station = stations[i];
		//						const CHourlyData& data = station.GetHour(TRef);

		//						if (data[H_TMIN] > -999 &&
		//							data[H_TMAX] > -999)
		//						{
		//							key_ID.push_back(station.m_ID);
		//							name.push_back(station.m_name);
		//							ilats.push_back(station.m_lat);
		//							ilons.push_back(station.m_lon);
		//							ielevs.push_back(station.m_elev);
		//							Tmin.push_back(data[H_TMIN]);
		//							Tmax.push_back(data[H_TMAX]);
		//						}
		//					}


		//					titanlib::Dataset dataset1(key_ID, name, ilats, ilons, ielevs, Tmin);
		//					titanlib::Dataset dataset2(key_ID, name, ilats, ilons, ielevs, Tmax);

		//					int num_min = 5;
		//					int num_max = 100;
		//					float inner_radius = 5000;
		//					float outer_radius = 50000;
		//					int num_iterations = 2;
		//					int num_min_prof = 30;
		//					float min_elev_diff = 50;
		//					float min_horizontal_scale = 1000;
		//					float vertical_scale = 100;
		//					vec t2pos(ilats.size(), 5.0f);
		//					vec t2neg(ilats.size(), 5.0f);
		//					vec eps2(ilats.size(), 0.33f);


		//					dataset1.sct(
		//						num_min, num_max, inner_radius, outer_radius
		//						, num_iterations, num_min_prof, min_elev_diff, min_horizontal_scale
		//						, vertical_scale, t2pos, t2neg, eps2
		//					);

		//					dataset2.sct(
		//						num_min, num_max, inner_radius, outer_radius
		//						, num_iterations, num_min_prof, min_elev_diff, min_horizontal_scale
		//						, vertical_scale, t2pos, t2neg, eps2
		//					);



		//					file.open(file_path_out, ios_base::out | ios_base::app);
		//					for (size_t i = 0; i < dataset1.key_ID().size(); i++)
		//					{

		//						file << dataset1.key_ID()[i] << "," << dataset1.name()[i] << "," << dataset1.lat()[i] << "," << dataset1.lon()[i] << "," << dataset1.elev()[i] << ",";
		//						file << TRef.GetFormatedString() << ",";
		//						file << dataset1.value()[i] << "," << dataset1.prob_gross_error()[i] << "," << dataset1.flags()[i] << ",";
		//						file << dataset2.value()[i] << "," << dataset2.prob_gross_error()[i] << "," << dataset2.flags()[i] << endl;
		//					}

		//					file.close();

		//					msg += callback.StepIt();
		//				}//all days
		//			//}//month

		//				callback.PopTask();
		//				msg += callback.StepIt();
		//			}//years

		//			callback.PopTask();
		//		}//if msg


		//		/*float elev_gradient = -0.0065f;
		//		float min_std = 1;
		//		float threshold = 2;
		//		float max_elev_diff = 200;
		//		vec radius(1, 5000);
		//		ivec num_min2(1, 5);
		//		ivec obs_to_check;

		//		ivec flags2 = titanlib::buddy_check(ilats, ilons, ielevs, Tmin, radius, num_min2, threshold, max_elev_diff, elev_gradient, min_std, num_iterations, obs_to_check);
		//*/


		///*int num_min3 = 5;
		//float radius3 = 15000;
		//float vertical_radius = 200;
		//ivec flags3 = titanlib::isolation_check(ilats, ilons, ielevs, num_min3, radius3, vertical_radius);*/
		////
		////
		////
		////
		////			//for precipitation
		////			float event_threshold = 0.2f;
		////			elev_gradient = 0;
		////			threshold = 0.25;
		////			max_elev_diff = 0;
		////
		//////titanlib::Dataset dataset2(ilats, ilons, ielevs, Prcp);
		////			ivec flags4 = titanlib::buddy_event_check(ilats, ilons, ielevs, Prcp, radius, num_min2, event_threshold, threshold, max_elev_diff, elev_gradient, num_iterations, obs_to_check);
		////
		//	}

		return msg;
	}

	ERMsg CQualityControl::ExecuteDaily(CCallback& callback)
	{
		ERMsg msg;
		//	string file_path_in = "G:\\Travaux\\QualityControl\\Weather\\Test2019.DailyDB";
		//	string file_path_out = file_path_in;
		//	SetFileExtension(file_path_out, ".DailyQC2.csv");


		//	CTPeriod period(2019, JANUARY, DAY_01, 2019, DECEMBER, DAY_31);

		//	CDailyDatabase db;
		//	msg = db.Open(file_path_in);
		//	if (msg)
		//	{
		//		ofStream file;
		//		msg = file.open(file_path_out);
		//		if (msg)
		//		{
		//			file << "KeyID,Name,Latitude,Longitude,Elevation,Date,Tmin,TminProb,TminRep,TminFlag,Tmax,TmaxProb,TmaxRep,TmaxFlag" << endl;
		//			file.close();

		//			callback.PushTask("Quality Control (" + ToString(period.GetNbYears()) + " years)", period.GetNbYears());


		//			for (int year = period.GetFirstYear(); year <= period.GetLastYear() && msg; year++)
		//			{
		//				CWeatherStationVector stations;
		//				stations.resize(db.size());


		//				for (size_t i = 0; i < db.size(); i++)
		//				{
		//					db.Get(stations[i], i, year);
		//				}

		//				CTPeriod period_year = period;
		//				period.Intersect(CTPeriod(year, JANUARY, DAY_01, year, DECEMBER, DAY_31));
		//				callback.PushTask("Quality control for " + to_string(year) + " (" + ToString(period.GetNbDay()) + " days)", period.GetNbDay());




		//				//for (size_t m = 0; m < 12 && msg; m++)
		//				//{
		//					//for (size_t d = 0; d < GetNbDayPerMonth(year, m)&&msg; d++)
		//				for (CTRef TRef = period_year.Begin(); TRef <= period_year.End() && msg; TRef++)
		//				{
		//					svec key_ID;
		//					svec name;
		//					vec ilats;
		//					vec ilons;
		//					vec ielevs;
		//					vec Tmin;
		//					vec Tmax;
		//					vec Prcp;


		//					key_ID.reserve(stations.size());
		//					name.reserve(stations.size());
		//					ilats.reserve(stations.size());
		//					ilons.reserve(stations.size());
		//					ielevs.reserve(stations.size());
		//					Tmin.reserve(stations.size());
		//					Tmax.reserve(stations.size());

		//					for (size_t i = 0; i < stations.size(); i++)
		//					{
		//						const CWeatherStation& station = stations[i];
		//						const CWeatherDay& data = station.GetDay(TRef);

		//						if (data[H_TMIN].IsInit() &&
		//							data[H_TMAX].IsInit())
		//						{

		//							ASSERT(data[H_TMIN][LOWEST] > -999);
		//							ASSERT(data[H_TMAX][HIGHEST] > -999);

		//							key_ID.push_back(station.m_ID);
		//							name.push_back(station.m_name);
		//							ilats.push_back(station.m_lat);
		//							ilons.push_back(station.m_lon);
		//							ielevs.push_back(station.m_elev);
		//							Tmin.push_back(data[H_TMIN][LOWEST]);
		//							Tmax.push_back(data[H_TMAX][HIGHEST]);
		//							//Prcp.push_back(station[y][m][d][H_PRCP][SUM]);
		//						}
		//					}


		//					titanlib::Dataset dataset1(key_ID, name, ilats, ilons, ielevs, Tmin);
		//					titanlib::Dataset dataset2(key_ID, name, ilats, ilons, ielevs, Tmax);

		//					int num_min = 5;
		//					int num_max = 100;
		//					float inner_radius = 5000;
		//					float outer_radius = 150000;
		//					int num_iterations = 5;
		//					int num_min_prof = 20;
		//					float min_elev_diff = 200;
		//					float min_horizontal_scale = 10000;
		//					float vertical_scale = 200;
		//					vec t2pos(ilats.size(), 4);
		//					vec t2neg(ilats.size(), 8);
		//					vec eps2(ilats.size(), 0.5);

		//					//ivec indices;

		//					//vec min(1, -60);
		//					//vec max(1, 50);
		//					//range_check(min, max, indices);

		//					//vec pos(1, 4);
		//					//vec neg(1, -4);
		//					//int unixtime = GetUnixTime(2019, 7, 15);
		//					//dataset.range_check_climatology(unixtime, pos, neg, indices);


		//					vec prob_gross_error;
		//					vec rep;
		//					dataset1.sct(
		//						num_min, num_max, inner_radius, outer_radius
		//						, num_iterations, num_min_prof, min_elev_diff, min_horizontal_scale
		//						, vertical_scale, t2pos, t2neg, eps2
		//					);

		//					dataset2.sct(
		//						num_min, num_max, inner_radius, outer_radius
		//						, num_iterations, num_min_prof, min_elev_diff, min_horizontal_scale
		//						, vertical_scale, t2pos, t2neg, eps2
		//					);



		//					file.open(file_path_out, ios_base::out | ios_base::app);
		//					for (size_t i = 0; i < dataset1.key_ID().size(); i++)
		//					{

		//						file << dataset1.key_ID()[i] << "," << dataset1.name()[i] << "," << dataset1.lat()[i] << "," << dataset1.lon()[i] << "," << dataset1.elev()[i] << ",";
		//						file << TRef.GetFormatedString() << ",";
		//						file << dataset1.value()[i] << "," << dataset1.prob_gross_error()[i] << "," << dataset1.rep()[i] << "," << dataset1.flags()[i] << ",";
		//						file << dataset2.value()[i] << "," << dataset2.prob_gross_error()[i] << "," << dataset2.rep()[i] << "," << dataset2.flags()[i] << endl;
		//					}

		//					file.close();

		//					msg += callback.StepIt();
		//				}//all days
		//			//}//month

		//				callback.PopTask();
		//				msg += callback.StepIt();
		//			}//years

		//			callback.PopTask();
		//		}//if msg


		//		/*float elev_gradient = -0.0065f;
		//		float min_std = 1;
		//		float threshold = 2;
		//		float max_elev_diff = 200;
		//		vec radius(1, 5000);
		//		ivec num_min2(1, 5);
		//		ivec obs_to_check;

		//		ivec flags2 = titanlib::buddy_check(ilats, ilons, ielevs, Tmin, radius, num_min2, threshold, max_elev_diff, elev_gradient, min_std, num_iterations, obs_to_check);
		//*/


		///*int num_min3 = 5;
		//float radius3 = 15000;
		//float vertical_radius = 200;
		//ivec flags3 = titanlib::isolation_check(ilats, ilons, ielevs, num_min3, radius3, vertical_radius);*/
		////
		////
		////
		////
		////			//for precipitation
		////			float event_threshold = 0.2f;
		////			elev_gradient = 0;
		////			threshold = 0.25;
		////			max_elev_diff = 0;
		////
		//////titanlib::Dataset dataset2(ilats, ilons, ielevs, Prcp);
		////			ivec flags4 = titanlib::buddy_event_check(ilats, ilons, ielevs, Prcp, radius, num_min2, event_threshold, threshold, max_elev_diff, elev_gradient, num_iterations, obs_to_check);
		////
		//	}

		return msg;
	}

	ERMsg CQualityControl::InitDefaultWG(CWeatherGenerator& WG, CCallback& callback)
	{
		ERMsg msg;


		std::string NFilePath = "G:\\Weather\\Normals\\Canada-USA 1981-2010.NormalsDB";
		//std::string DFilePath;
		//std::string HFilePath;

		//open normal database
		CNormalsDatabasePtr pNormalDB;
		pNormalDB.reset(new CNormalsDatabase);
		msg = pNormalDB->Open(NFilePath, CNormalsDatabase::modeRead, callback);
		if (msg)
			msg = pNormalDB->OpenSearchOptimization(callback);//open here to be thread safe

		//open daily database
		//CDailyDatabasePtr pDailyDB;
		//if (msg && WGInput.IsDaily())
		//{
		//	pDailyDB.reset(new CDailyDatabase);
		//	msg = pDailyDB->Open(DFilePath, CDailyDatabase::modeRead, callback, WGInput.m_bSkipVerify);
		//	if (msg)
		//		msg = pDailyDB->OpenSearchOptimization(callback);//open here to be thread safe
		//}
		//
		//CHourlyDatabasePtr pHourlyDB;
		//if (msg && WGInput.IsHourly())
		//{
		//	pHourlyDB.reset(new CHourlyDatabase);
		//	msg = pHourlyDB->Open(HFilePath, CHourlyDatabase::modeRead, callback, WGInput.m_bSkipVerify);
		//	if (msg)
		//		msg = pHourlyDB->OpenSearchOptimization(callback);//open here to be thread safe
		//}

		if (msg)
		{


			WG.SetNormalDB(pNormalDB);


			CWGInput WGI;
			//WGI.m_variables = "TN T TX P TD H WS WD R Z S SD SWE WS2";
			WGI.m_variables = "TN T TX P TD H WS R WS2";

			WG.SetWGInput(WGI);
			WG.SetNbReplications(30);
			//WG.SetDailyDB(pDailyDB);
			//WG.SetHourlyDB(pHourlyDB);


		}

		return msg;
	}


	ERMsg CQualityControl::CheckDailyWithNormal(CCallback& callback)
	{
		ERMsg msg;


		string input_file_path = "G:\\Weather\\Daily\\Quebec+SOPFEU+Buffer 1991-2020.DailyDB";
		string output_file_path = "G:\\Weather\\Daily\\Quebec+SOPFEU+Buffer 1991-2020 QC.DailyDB";
		string log_file_path = output_file_path;
		SetFileExtension(log_file_path, ".Normalslog.csv");




		CDailyDatabase DBin;
		msg += DBin.Open(input_file_path, CDailyDatabase::modeRead, callback);


		CDailyDatabase DBout;
		if (msg)
		{
			CDailyDatabase::DeleteDatabase(output_file_path, callback);
			msg += DBout.Open(output_file_path, CDailyDatabase::modeWrite, callback);
		}

		ofStream log_file;
		if (msg)
		{
			msg = log_file.open(log_file_path);
			if (msg)
				log_file << "ID,Name,Date,Variable,Value,Nmin,Nmax" << endl;
		}

		CWeatherGenerator WG;
		if (msg)
		{
			msg = InitDefaultWG(WG, callback);
		}



		if (msg)
		{


			callback.PushTask("Validation for all stations (" + to_string(DBin.size()) + ")", DBin.size());

			for (size_t i = 0; i < DBin.size() && msg; i++)
			{
				CWeatherStation station;
				msg += DBin.Get(station, i);


				WG.SetTarget(station);
				msg += WG.Initialize();//create gradient

				//CSimulationPointVector simulationPointVector(WG.GetNbReplications());
				msg += WG.Generate(callback);

				//CSimulationPointVector& simulationPointVector = WG.GetWeather()
				//msg += WG.GenerateNormals(simulationPointVector, callback);


				if (msg)
				{

					//Compute Normals statistics
					array < array<CStatistic, NB_VAR_H>, 12> normals_stats;
					for (size_t i = 0; i < WG.GetNbReplications() && msg; i++)
					{
						const CSimulationPoint& simulationPoint = WG.GetWeather(i);

						for (size_t y = 0; y < simulationPoint.size() && msg; y++)
						{
							for (size_t m = 0; m < simulationPoint[y].size() && msg; m++)//for all months
							{
								for (size_t d = 0; d < simulationPoint[y][m].size() && msg; d++)//for all days
								{
									for (TVarH v = H_FIRST_VAR; v < NB_VAR_H && msg; v++)//for all variables
									{
										//	size_t Jday = simulationPoint[y][m][d].GetTRef().GetJDay();
										normals_stats[m][v] += simulationPoint[y][m][d][v][MEAN];
									}
								}
							}
						}
					}

					//normals_stats[365] = normals_stats[364];//for leap years

					for (size_t y = 0; y < station.size() && msg; y++)
					{
						for (size_t m = 0; m < station[y].size() && msg; m++)//for all months
						{
							for (size_t d = 0; d < station[y][m].size() && msg; d++)//for all days
							{
								for (TVarH v = H_FIRST_VAR; v < NB_VAR_H && msg; v++)//for all variables
								{

									if (station[y][m][d][v].IsInit())
									{
										float value = station[y][m][d][v][MEAN];

										if (v == H_RELH && value > 100 && value <= 105)
										{
											value = 100;
											station[y][m][d].SetStat(v, value);
										}

										if (v == H_SNDH && value < 0)
										{
											value = 0;
											station[y][m][d].SetStat(v, value);
										}

										//size_t Jday = station[y][m][d].GetTRef().GetJDay();
										bool valid = IsNormalsCheckValid(v, value, normals_stats[m]);

										//bool bAdditif = v == true:false;

										if (!valid)
										{
											log_file << station.m_ID << "," << station.m_name << "," << station[y][m][d].GetTRef().GetFormatedString() << "," << GetVariableAbvr(v) << "," << ToString(value) << "," << ToString(normals_stats[m][v][LOWEST]) << "," << ToString(normals_stats[m][v][HIGHEST]) << endl;
											station[y][m][d].SetStat(v, CStatistic());//reset
										}
									}

									msg += callback.StepIt(0);
								}
							}//for all days
						}//for all months

						//msg += callback.StepIt();
					}//for all years

					//callback.PopTask();

					DBout.Add(station);


					msg += callback.StepIt();
				}//if msg
			}//for all stations

			callback.PopTask();


			log_file.close();
			msg += DBout.Close();
			msg += DBin.Close();

		}//if msg

		return msg;
	}


	ERMsg CQualityControl::CheckBasicValue(CCallback& callback)
	{
		ERMsg msg;


		string input_file_path = "G:\\Weather\\Daily\\Quebec+SOPFEU+Buffer 1991-2020.DailyDB";
		string output_file_path = "G:\\Weather\\Daily\\Quebec+SOPFEU+Buffer 1991-2020 QC.DailyDB";
		string log_file_path = output_file_path;
		SetFileExtension(log_file_path, ".log.csv");

		//int first_year = 2020;
		//int last_year = 2020;

		//set<int> years;
		//for (int year = first_year; year <= last_year; year++)
			//years.insert(year);


		//if (msg)
		//	msg = LoadStations(file_path, pPts, years, callback);


		//callback.PushTask("Open weather databases (" + to_string(file_path.size()) + " databases)", file_path.size());

		/*size_t nb_stations = 0;
		vector<CDailyDatabase> DBs(file_path.size());
		for (size_t i = 0; i < file_path.size() && msg; i++)
		{
			msg += DBs[i].Open(file_path[i], CDailyDatabase::modeRead, callback);
			if (msg)
				nb_stations += DBs[i].size();

			msg += callback.StepIt();
		}*/


		CDailyDatabase DBin;
		msg += DBin.Open(input_file_path, CDailyDatabase::modeRead, callback);


		CDailyDatabase DBout;
		if (msg)
		{
			CDailyDatabase::DeleteDatabase(output_file_path, callback);
			msg += DBout.Open(output_file_path, CDailyDatabase::modeWrite, callback);
		}

		ofStream log_file;
		if (msg)
		{
			msg = log_file.open(log_file_path);
			if (msg)
				log_file << "ID,Name,Date,Variable,Value" << endl;
		}


		if (msg)
		{


			callback.PushTask("Validation for all stations (" + to_string(DBin.size()) + ")", DBin.size());

			for (size_t i = 0; i < DBin.size() && msg; i++)
			{
				CWeatherStation station;
				msg += DBin.Get(station, i);

				//callback.PushTask("Validation for station "+ station.m_name + " (" + to_string(station.size()) + " years)", station.size());

				for (size_t y = 0; y < station.size() && msg; y++)
				{
					for (size_t m = 0; m < station[y].size() && msg; m++)//for all months
					{
						for (size_t d = 0; d < station[y][m].size() && msg; d++)//for all days
						{
							for (TVarH v = H_FIRST_VAR; v < NB_VAR_H && msg; v++)//for all variables
							{

								if (station[y][m][d][v].IsInit())
								{
									float value = station[y][m][d][v][MEAN];

									if (v == H_RELH && value > 100 && value <= 105)
									{
										value = 100;
										station[y][m][d].SetStat(v, value);
									}

									if (v == H_SNDH && value < 0)
									{
										value = 0;
										station[y][m][d].SetStat(v, value);
									}



									bool valid = IsBasicCheckValid(v, value);
									//bool bIsValid = IsValid(v, value[STAT[v]]);
									if (!valid)
									{
										log_file << station.m_ID << "," << station.m_name << "," << station[y][m][d].GetTRef().GetFormatedString() << "," << GetVariableAbvr(v) << "," << ToString(value) << endl;


										station[y][m][d].SetStat(v, CStatistic());//reset
									}
								}

								msg += callback.StepIt(0);
							}
						}//for all days
					}//for all months

					//msg += callback.StepIt();
				}//for all years

				//callback.PopTask();

				DBout.Add(station);


				msg += callback.StepIt();
			}//for all stations

			callback.PopTask();


			log_file.close();
			msg += DBout.Close();
			msg += DBin.Close();

		}//if msg

		return msg;

	}


	bool CQualityControl::IsBasicCheckValid(size_t v, float value)
	{
		bool bValid = true;

		switch (v)
		{
		case H_TMIN:
		case H_TAIR:
		case H_TMAX:
		case H_TDEW: bValid = value >= -60 && value <= 60; break;
		case H_PRCP: bValid = value >= 0 && value <= 250; break;
		case H_RELH: bValid = value >= 0 && value <= 100; break;
		case H_WNDS:
		case H_WND2: bValid = value >= 0 && value <= 200; break;
		case H_WNDD: bValid = value >= 0 && value <= 360; break;
		case H_SRAD: bValid = value >= 0 && value <= 400; break;
		case H_PRES: bValid = value >= 800 && value <= 1200; break;
		case H_SNOW: bValid = value >= 0 && value <= 500; break;
		case H_SNDH: bValid = value >= 0 && value <= 5000; break;
		case H_SWE:	 bValid = value >= 0 && value <= 5000; break;
		default: ASSERT(false);

		}

		return bValid;
	}

	bool CQualityControl::IsNormalsCheckValid(size_t v, float value, const array<CStatistic, NB_VAR_H>& normals_stats)
	{
		bool bValid = true;

		float deltaT = float(normals_stats[v][RANGE] * 50.0 / 100);
		float deltaH = float(normals_stats[v][RANGE] * 25.0 / 100);
		float factorP = 3.0f;
		float factorW = 1.5f;

		switch (v)
		{
		case H_TMIN:
		case H_TAIR:
		case H_TMAX: bValid = value >= (normals_stats[v][LOWEST] - deltaT) && value <= (normals_stats[v][HIGHEST] + deltaT); break;
		case H_TDEW: bValid = value >= (normals_stats[v][LOWEST] - deltaH) && value <= (normals_stats[v][HIGHEST] + deltaH); break;
		case H_PRCP: bValid = value <= max(50.0, normals_stats[v][HIGHEST] * factorP); break;
		case H_RELH: bValid = value >= max(0.0, normals_stats[v][LOWEST] - deltaH) && value <= min(100.0, normals_stats[v][HIGHEST] + deltaH); break;
		case H_WNDS:
		case H_WND2: bValid = value <= (normals_stats[v][HIGHEST] * factorW); break;
		case H_WNDD: break;
		case H_SRAD: bValid = value >= (normals_stats[v][LOWEST] - deltaH) && value <= (normals_stats[v][HIGHEST] + deltaH); break;
		case H_PRES: break;
		case H_SNOW:  break;
		case H_SNDH:  break;
		case H_SWE:	  break;
		default: ASSERT(false);

		}

		return bValid;
	}
}






