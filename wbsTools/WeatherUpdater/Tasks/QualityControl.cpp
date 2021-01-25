#include "StdAfx.h"
#include "QualityControl.h"
#include "Basic\DailyDatabase.h"
#include "Basic\WeatherStation.h"

#include "titanlib/include/titanlib.h"

#include "TaskFactory.h"
#include "../resource.h"
#include "WeatherBasedSimulationString.h"

using namespace std;
using namespace WBSF::HOURLY_DATA;


namespace WBSF
{


	int GetUnixTime(int year, int month, int day)
	{
		std::tm tm = { /* .tm_sec  = */ 0,
			/* .tm_min  = */ 0,
			/* .tm_hour = */ 0,
			/* .tm_mday = */ (day),
			/* .tm_mon  = */ (month)-1,
			/* .tm_year = */ (year)-1900,
		};
		tm.tm_isdst = -1; // Use DST value from local time zone
		auto tp = std::chrono::system_clock::from_time_t(std::mktime(&tm));
		std::chrono::system_clock::duration d = tp.time_since_epoch();
		std::chrono::seconds s = std::chrono::duration_cast<std::chrono::seconds>(d);


		//std::chrono::day::days sec(1);
		//std::chrono::duration = ;
		//sys_seconds(tp);


		return s.count();
	}


	//*********************************************************************

	CQualityControl::CQualityControl(void)
	{}

	CQualityControl::~CQualityControl(void)
	{}

	ERMsg CQualityControl::Execute(CCallback& callback)
	{
		ERMsg msg;
		

		//msg = ExecuteHourly(callback);
		msg.ajoute("Not implemented yet.");

		return msg;
	}

	ERMsg CQualityControl::ExecuteHourly(CCallback& callback)
	{
		ERMsg msg;
		string file_path_in = "G:\\Travaux\\QualityControl\\Weather\\Quebec 2018.HourlyDB";
		string file_path_out = file_path_in;
		SetFileExtension(file_path_out, ".HourlyQC.csv");


		CTPeriod period(CTRef(2018, JANUARY, DAY_01, 0), CTRef(2018, DECEMBER, DAY_31, 23));

		CHourlyDatabase db;
		msg = db.Open(file_path_in, CHourlyDatabase::modeRead, callback);
		if (msg)
		{
			ofStream file;
			msg = file.open(file_path_out);
			if (msg)
			{
				file << "KeyID,Name,Latitude,Longitude,Elevation,Date,Tmin,TminProb,TminFlag,Tmax,TmaxProb,TmaxFlag" << endl;
				file.close();

				callback.PushTask("Quality Control (" + to_string(period.GetNbYears()) + " years)", period.GetNbYears());


				for (int year = period.GetFirstYear(); year <= period.GetLastYear() && msg; year++)
				{
					CWeatherStationVector stations;
					stations.resize(db.size());


					callback.PushTask("Load weather for year "+ to_string(year) + " (" + to_string(db.size()) + " stations)", db.size());
					for (size_t i = 0; i < db.size()&&msg; i++)
					{
						db.Get(stations[i], i, year);
						msg += callback.StepIt();
					}
					callback.PopTask();

					CTPeriod period_year = period.Intersect(CTPeriod(CTRef(year, JULY, DAY_01, 0), CTRef(year, JULY, DAY_30, 23)));
					//CTPeriod period_year = period.Intersect(CTPeriod(CTRef(year, JANUARY, DAY_01, 0), CTRef(year, DECEMBER, DAY_31, 23)));
					callback.PushTask("Quality control for " + to_string(year) + " (" + ToString(period_year.GetNbHour()) + " hours)", period_year.GetNbHour());


					//for (size_t m = 0; m < 12 && msg; m++)
					//{
						//for (size_t d = 0; d < GetNbDayPerMonth(year, m)&&msg; d++)
					for (CTRef TRef = period_year.Begin(); TRef <= period_year.End() && msg; TRef++)
					{
						svec key_ID;
						svec name;
						vec ilats;
						vec ilons;
						vec ielevs;
						vec Tmin;
						vec Tmax;
						vec Prcp;


						key_ID.reserve(stations.size());
						name.reserve(stations.size());
						ilats.reserve(stations.size());
						ilons.reserve(stations.size());
						ielevs.reserve(stations.size());
						Tmin.reserve(stations.size());
						Tmax.reserve(stations.size());

						for (size_t i = 0; i < stations.size(); i++)
						{
							const CWeatherStation& station = stations[i];
							const CHourlyData& data = station.GetHour(TRef);

							if (data[H_TMIN] > -999 &&
								data[H_TMAX] > -999)
							{
								key_ID.push_back(station.m_ID);
								name.push_back(station.m_name);
								ilats.push_back(station.m_lat);
								ilons.push_back(station.m_lon);
								ielevs.push_back(station.m_elev);
								Tmin.push_back(data[H_TMIN]);
								Tmax.push_back(data[H_TMAX]);
							}
						}


						titanlib::Dataset dataset1(key_ID, name, ilats, ilons, ielevs, Tmin);
						titanlib::Dataset dataset2(key_ID, name, ilats, ilons, ielevs, Tmax);

						int num_min = 5;
						int num_max = 100;
						float inner_radius = 5000;
						float outer_radius = 50000;
						int num_iterations = 2;
						int num_min_prof = 30;
						float min_elev_diff = 50;
						float min_horizontal_scale = 1000;
						float vertical_scale = 100;
						vec t2pos(ilats.size(), 5.0);
						vec t2neg(ilats.size(), 5.0);
						vec eps2(ilats.size(), 0.33);

						
						dataset1.sct(
							num_min, num_max, inner_radius, outer_radius
							, num_iterations, num_min_prof, min_elev_diff, min_horizontal_scale
							, vertical_scale, t2pos, t2neg, eps2
						);

						dataset2.sct(
							num_min, num_max, inner_radius, outer_radius
							, num_iterations, num_min_prof, min_elev_diff, min_horizontal_scale
							, vertical_scale, t2pos, t2neg, eps2
						);



						file.open(file_path_out, ios_base::out | ios_base::app);
						for (size_t i = 0; i < dataset1.key_ID().size(); i++)
						{

							file << dataset1.key_ID()[i] << "," << dataset1.name()[i] << "," << dataset1.lat()[i] << "," << dataset1.lon()[i] << "," << dataset1.elev()[i] << ",";
							file << TRef.GetFormatedString() << ",";
							file << dataset1.value()[i] << "," << dataset1.prob_gross_error()[i] <<"," << dataset1.flags()[i] << ",";
							file << dataset2.value()[i] << "," << dataset2.prob_gross_error()[i] <<"," << dataset2.flags()[i] << endl;
						}

						file.close();

						msg += callback.StepIt();
					}//all days
				//}//month

					callback.PopTask();
					msg += callback.StepIt();
				}//years

				callback.PopTask();
			}//if msg


			/*float elev_gradient = -0.0065f;
			float min_std = 1;
			float threshold = 2;
			float max_elev_diff = 200;
			vec radius(1, 5000);
			ivec num_min2(1, 5);
			ivec obs_to_check;

			ivec flags2 = titanlib::buddy_check(ilats, ilons, ielevs, Tmin, radius, num_min2, threshold, max_elev_diff, elev_gradient, min_std, num_iterations, obs_to_check);
	*/


	/*int num_min3 = 5;
	float radius3 = 15000;
	float vertical_radius = 200;
	ivec flags3 = titanlib::isolation_check(ilats, ilons, ielevs, num_min3, radius3, vertical_radius);*/
	//
	//
	//
	//
	//			//for precipitation
	//			float event_threshold = 0.2f;
	//			elev_gradient = 0;
	//			threshold = 0.25;
	//			max_elev_diff = 0;
	//
	////titanlib::Dataset dataset2(ilats, ilons, ielevs, Prcp);
	//			ivec flags4 = titanlib::buddy_event_check(ilats, ilons, ielevs, Prcp, radius, num_min2, event_threshold, threshold, max_elev_diff, elev_gradient, num_iterations, obs_to_check);
	//
		}

		return msg;
	}

	ERMsg CQualityControl::ExecuteDaily(CCallback& callback)
	{
		ERMsg msg;
		string file_path_in = "G:\\Travaux\\QualityControl\\Weather\\Test2019.DailyDB";
		string file_path_out = file_path_in;
		SetFileExtension(file_path_out, ".DailyQC2.csv");


		CTPeriod period(2019, JANUARY, DAY_01, 2019, DECEMBER, DAY_31);

		CDailyDatabase db;
		msg = db.Open(file_path_in);
		if (msg)
		{
			ofStream file;
			msg = file.open(file_path_out);
			if (msg)
			{
				file << "KeyID,Name,Latitude,Longitude,Elevation,Date,Tmin,TminProb,TminRep,TminFlag,Tmax,TmaxProb,TmaxRep,TmaxFlag" << endl;
				file.close();

				callback.PushTask("Quality Control (" + ToString(period.GetNbYears()) + " years)", period.GetNbYears());


				for (int year = period.GetFirstYear(); year <= period.GetLastYear() && msg; year++)
				{
					CWeatherStationVector stations;
					stations.resize(db.size());


					for (size_t i = 0; i < db.size(); i++)
					{
						db.Get(stations[i], i, year);
					}

					CTPeriod period_year = period;
					period.Intersect(CTPeriod(year, JANUARY, DAY_01, year, DECEMBER, DAY_31));
					callback.PushTask("Quality control for " + to_string(year) + " (" + ToString(period.GetNbDay()) + " days)", period.GetNbDay());




					//for (size_t m = 0; m < 12 && msg; m++)
					//{
						//for (size_t d = 0; d < GetNbDayPerMonth(year, m)&&msg; d++)
					for (CTRef TRef = period_year.Begin(); TRef <= period_year.End() && msg; TRef++)
					{
						svec key_ID;
						svec name;
						vec ilats;
						vec ilons;
						vec ielevs;
						vec Tmin;
						vec Tmax;
						vec Prcp;


						key_ID.reserve(stations.size());
						name.reserve(stations.size());
						ilats.reserve(stations.size());
						ilons.reserve(stations.size());
						ielevs.reserve(stations.size());
						Tmin.reserve(stations.size());
						Tmax.reserve(stations.size());

						for (size_t i = 0; i < stations.size(); i++)
						{
							const CWeatherStation& station = stations[i];
							const CWeatherDay& data = station.GetDay(TRef);

							if (data[H_TMIN].IsInit() &&
								data[H_TMAX].IsInit())
							{

								ASSERT(data[H_TMIN][LOWEST] > -999);
								ASSERT(data[H_TMAX][HIGHEST] > -999);

								key_ID.push_back(station.m_ID);
								name.push_back(station.m_name);
								ilats.push_back(station.m_lat);
								ilons.push_back(station.m_lon);
								ielevs.push_back(station.m_elev);
								Tmin.push_back(data[H_TMIN][LOWEST]);
								Tmax.push_back(data[H_TMAX][HIGHEST]);
								//Prcp.push_back(station[y][m][d][H_PRCP][SUM]);
							}
						}


						titanlib::Dataset dataset1(key_ID, name, ilats, ilons, ielevs, Tmin);
						titanlib::Dataset dataset2(key_ID, name, ilats, ilons, ielevs, Tmax);

						int num_min = 5;
						int num_max = 100;
						float inner_radius = 5000;
						float outer_radius = 150000;
						int num_iterations = 5;
						int num_min_prof = 20;
						float min_elev_diff = 200;
						float min_horizontal_scale = 10000;
						float vertical_scale = 200;
						vec t2pos(ilats.size(), 4);
						vec t2neg(ilats.size(), 8);
						vec eps2(ilats.size(), 0.5);

						//ivec indices;

						//vec min(1, -60);
						//vec max(1, 50);
						//range_check(min, max, indices);

						//vec pos(1, 4);
						//vec neg(1, -4);
						//int unixtime = GetUnixTime(2019, 7, 15);
						//dataset.range_check_climatology(unixtime, pos, neg, indices);


						vec prob_gross_error;
						vec rep;
						dataset1.sct(
							num_min, num_max, inner_radius, outer_radius
							, num_iterations, num_min_prof, min_elev_diff, min_horizontal_scale
							, vertical_scale, t2pos, t2neg, eps2
						);

						dataset2.sct(
							num_min, num_max, inner_radius, outer_radius
							, num_iterations, num_min_prof, min_elev_diff, min_horizontal_scale
							, vertical_scale, t2pos, t2neg, eps2
						);



						file.open(file_path_out, ios_base::out | ios_base::app);
						for (size_t i = 0; i < dataset1.key_ID().size(); i++)
						{

							file << dataset1.key_ID()[i] << "," << dataset1.name()[i] << "," << dataset1.lat()[i] << "," << dataset1.lon()[i] << "," << dataset1.elev()[i] << ",";
							file << TRef.GetFormatedString() << ",";
							file << dataset1.value()[i] << "," << dataset1.prob_gross_error()[i] << "," << dataset1.rep()[i] << "," << dataset1.flags()[i] << ",";
							file << dataset2.value()[i] << "," << dataset2.prob_gross_error()[i] << "," << dataset2.rep()[i] << "," << dataset2.flags()[i] << endl;
						}

						file.close();

						msg += callback.StepIt();
					}//all days
				//}//month

					callback.PopTask();
					msg += callback.StepIt();
				}//years

				callback.PopTask();
			}//if msg


			/*float elev_gradient = -0.0065f;
			float min_std = 1;
			float threshold = 2;
			float max_elev_diff = 200;
			vec radius(1, 5000);
			ivec num_min2(1, 5);
			ivec obs_to_check;

			ivec flags2 = titanlib::buddy_check(ilats, ilons, ielevs, Tmin, radius, num_min2, threshold, max_elev_diff, elev_gradient, min_std, num_iterations, obs_to_check);
	*/


	/*int num_min3 = 5;
	float radius3 = 15000;
	float vertical_radius = 200;
	ivec flags3 = titanlib::isolation_check(ilats, ilons, ielevs, num_min3, radius3, vertical_radius);*/
	//
	//
	//
	//
	//			//for precipitation
	//			float event_threshold = 0.2f;
	//			elev_gradient = 0;
	//			threshold = 0.25;
	//			max_elev_diff = 0;
	//
	////titanlib::Dataset dataset2(ilats, ilons, ielevs, Prcp);
	//			ivec flags4 = titanlib::buddy_event_check(ilats, ilons, ielevs, Prcp, radius, num_min2, event_threshold, threshold, max_elev_diff, elev_gradient, num_iterations, obs_to_check);
	//
		}

		return msg;
	}


}






