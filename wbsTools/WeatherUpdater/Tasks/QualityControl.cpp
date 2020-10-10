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

//		CDailyDatabase db;
//		msg = db.Open("G:\\Travaux\\QualityControl\\Weather\\Test2019.DailyDB");
//		if (msg)
//		{
//			vec ilats;
//			vec ilons;
//			vec ielevs;
//			vec Tmin;
//			vec Tmax;
//			vec Prcp;
//
//
//
//			ilats.reserve(db.size());
//			ilons.reserve(db.size());
//			ielevs.reserve(db.size());
//			Tmin.reserve(db.size());
//			Tmax.reserve(db.size());
//			Prcp.reserve(db.size());
//
//			for (size_t i = 0; i < db.size(); i++)
//			{
//				CWeatherStation station;
//				db.Get(station, i);
//
//				//for (size_t y = 0; y < station.size(); y++)
//				{
//					//for (size_t m = 0; m < station[y].size(); m++)
//					{
//						//for (size_t d = 0; d < station[y][m].size(); d++)
//						{
//							size_t y = 0;
//							size_t m = JULY;
//							size_t d = DAY_15;
//
//							if (station[y][m][d][H_TMIN].IsInit() &&
//								station[y][m][d][H_TMAX].IsInit() &&
//								station[y][m][d][H_PRCP].IsInit()
//								)
//							{
//								ASSERT(station[y][m][d][H_TMIN][LOWEST] > -999);
//
//								ilats.push_back(station.m_lat);
//								ilons.push_back(station.m_lon);
//								ielevs.push_back(station.m_elev);
//								Tmin.push_back(station[y][m][d][H_TMIN][LOWEST]);
//								Tmax.push_back(station[y][m][d][H_TMAX][HIGHEST]);
//								Prcp.push_back(station[y][m][d][H_PRCP][SUM]);
//							}
//						}
//					}
//				}
//			}
//
//			titanlib::Dataset dataset1(ilats, ilons, ielevs, Tmin);
//			titanlib::Dataset dataset2(ilats, ilons, ielevs, Tmax);
//
//			int num_min = 5;
//			int num_max = 100;
//			float inner_radius = 5000;
//			float outer_radius = 150000;
//			int num_iterations = 5;
//			int num_min_prof = 20;
//			float min_elev_diff = 200;
//			float min_horizontal_scale = 10000;
//			float vertical_scale = 200;
//			vec t2pos(ilats.size(), 4);
//			vec t2neg(ilats.size(), 8);
//			vec eps2(ilats.size(), 0.5);
//
//
//			ivec indices;
//
//			//vec min(1, -60);
//			//vec max(1, 50);
//			//range_check(min, max, indices);
//
//			//vec pos(1, 4);
//			//vec neg(1, -4);
//			//int unixtime = GetUnixTime(2019, 7, 15);
//			//dataset.range_check_climatology(unixtime, pos, neg, indices);
//
//
//			vec prob_gross_error;
//			vec rep;
//			dataset1.sct(
//				num_min, num_max, inner_radius, outer_radius
//				, num_iterations, num_min_prof, min_elev_diff, min_horizontal_scale
//				, vertical_scale, t2pos, t2neg, eps2
//			); 
//
//			dataset2.sct(
//				num_min, num_max, inner_radius, outer_radius
//				, num_iterations, num_min_prof, min_elev_diff, min_horizontal_scale
//				, vertical_scale, t2pos, t2neg, eps2
//			);
//
//			/*float elev_gradient = -0.0065f;
//			float min_std = 1;
//			float threshold = 2;
//			float max_elev_diff = 200;
//			vec radius(1, 5000);
//			ivec num_min2(1, 5);
//			ivec obs_to_check;
//
//			ivec flags2 = titanlib::buddy_check(ilats, ilons, ielevs, Tmin, radius, num_min2, threshold, max_elev_diff, elev_gradient, min_std, num_iterations, obs_to_check);
//*/
//
//
//			/*int num_min3 = 5;
//			float radius3 = 15000;
//			float vertical_radius = 200;
//			ivec flags3 = titanlib::isolation_check(ilats, ilons, ielevs, num_min3, radius3, vertical_radius);*/
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
//		}

		return msg;
	}



}