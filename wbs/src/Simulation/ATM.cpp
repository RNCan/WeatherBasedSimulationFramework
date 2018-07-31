//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
// 17-07-2018	Rémi Saint-Amant	flags change and Bug correction in sub-hourly and time step, optimization of memory
// 01-05-2018	Rémi Saint-Amant	Modification to follow publication Régniere & Saint-Amant 2018
// 01-01-2016	Rémi Saint-Amant	Creation
//******************************************************************************
#include "stdafx.h"

#include <time.h>
#include "Basic/Statistic.h"
#include "Basic/CSV.h"
#include "Basic/Callback.h"
#include "Basic/OpenMP.h"
#include "Basic/WaterTemperature.h"
#include "Basic/CSV.h"
#include "Geomatic/GDALBasic.h"
#include "Geomatic/GDAL.h"
#include "cctz/time_zone_info.h"
#include "Simulation/ATM.h"

#include "WeatherBasedSimulationString.h"



//NOT_EMERGED			0
//READY_TO_FLY			1
//FLIGHT				2
//FINISHED				3

//NO_LIFTOFF_DEFINED                10
//NO_LIFTOFF_EMERGING  				11
//NO_LIFTOFF_NOT_READY			    12
//NO_LIFTOFF_NO_DEFOLIATION			13
//NO_LIFTOFF_NO_MORE_FLIGHT			14
//NO_LIFTOFF_TAIR					15
//NO_LIFTOFF_PRCP					16
//NO_LIFTOFF_WNDS					17
//NO_LIFTOFF_MISSING_WEATHER		18

//NO_FLIGHT_DEFINE		20
//WAIT_DEPARTURE		21
//FLIYNG				22
//LANDING				23
//END_BY_TAIR			24
//END_BY_PRCP			25
//END_BY_SUNRISE		26

//NO_END_DEFINE             30
//END_FULLFILLED			31
//END_OVER_WATER_TAIR		32
//END_OVER_WATER_PRCP		33
//END_OVER_WATER_SUNRISE	34
//END_OLD_AGE				35
//END_OUTSIDE_MAP			36
//END_OF_SIMULATION         37




using namespace std;

using namespace WBSF::HOURLY_DATA;
using namespace WBSF::WEATHER;

namespace WBSF
{

	static const double T_MINIMUM = 15.0;
	static const int MAX_NUMBER_IMAGE_LOAD = 18;
	static ERMsg CreateGribsFromText(CCallback& callback);
	static ERMsg CreateGribsFromNetCDF(CCallback& callback);

	size_t Hourly2ATM(size_t vv)
	{
		size_t v = NOT_INIT;

		switch (vv)
		{
		case HOURLY_DATA::H_TAIR2: v = ATM_TAIR; break;
		case HOURLY_DATA::H_TMIN2: break;
		case HOURLY_DATA::H_TMAX2: break;
		case HOURLY_DATA::H_PRCP: v = ATM_PRCP; break;
		case HOURLY_DATA::H_TDEW: break;
		case HOURLY_DATA::H_RELH: v = ATM_RH;  break;
		case HOURLY_DATA::H_WNDS: v = ATM_WNDU; break;
		case HOURLY_DATA::H_WNDD: v = ATM_WNDV; break;
		case HOURLY_DATA::H_SRAD2: break;
		case HOURLY_DATA::H_PRES: v = ATM_PRES; break;
		case HOURLY_DATA::H_SNOW: break;
		case HOURLY_DATA::H_SNDH: break;
		case HOURLY_DATA::H_SWE:  break;
		case HOURLY_DATA::H_WND2: break;
		default: ASSERT(false);
		}


		return v;
	}

	//from //http://www.movable-type.co.uk/scripts/latlong.html
	/**
	* Returns the destination point having travelled along a rhumb line from 'this' point the given
	* distance on the  given bearing.
	*
	* @param   {number} distance - Distance travelled, in same units as earth radius (default: metres).
	* @param   {number} bearing - Bearing in degrees from north.
	* @param   {number} [radius=6371e3] - (Mean) radius of earth (defaults to radius in metres).
	* @returns {LatLon} Destination point.
	*
	* @example
	*     var p1 = new LatLon(51.127, 1.338);
	*     var p2 = p1.rhumbDestinationPoint(40300, 116.7); // p2.toString(): 50.9642ᵒN, 001.8530ᵒE
	*/
	CGeoPoint RhumbDestinationPoint(CGeoPoint pt, double distance, double bearing, double radius = 6371000)
	{
		ASSERT(pt.IsGeographic());

		double δ = distance / radius; // angular distance in radians
		double φ1 = Deg2Rad(pt.m_lat);
		double λ1 = Deg2Rad(pt.m_lon);
		double θ = Deg2Rad(bearing);
		double Δφ = δ * cos(θ);
		double φ2 = φ1 + Δφ;
		// check for some daft bugger going past the pole, normalise latitude if so
		if (abs(φ2) > PI / 2)
			φ2 = φ2 > 0 ? PI - φ2 : -PI - φ2;

		double Δψ = log(tan(φ2 / 2 + PI / 4) / tan(φ1 / 2 + PI / 4));
		double q = abs(Δψ) > 10e-12 ? Δφ / Δψ : cos(φ1); // E-W course becomes ill-conditioned with 0/0
		double Δλ = δ * sin(θ) / q;
		double λ2 = λ1 + Δλ;

		λ2 = fmod(λ2 + 3 * PI, 2 * PI) - PI; // normalise to -180..+180ᵒ

		return CGeoPoint(Rad2Deg(λ2), Rad2Deg(φ2), pt.GetPrjID());
	};

	CGeoPoint3D Geodesic2Geocentric(CGeoPoint3D pt)
	{
		return CGeoPoint3D(pt(0), pt(1), pt(2), PRJ_GEOCENTRIC_BASE);
	}

	CGeoPoint3D UpdateCoordinate(const CGeoPoint3D& pt, const CGeoDistance3D& d, const CProjectionTransformation& ToWeather, const CProjectionTransformation& FromWeather)
	{
		_ASSERTE(!_isnan(pt.m_x));
		_ASSERTE(!_isnan(pt.m_y));

		/*double distance = sqrt(d.m_x*d.m_x + d.m_y*d.m_y);
		double alpha = atan2(d.m_y, d.m_x);
		double bearing = fmod(360 + 90 - Rad2Deg(alpha), 360);
		CGeoPoint pt2 = RhumbDestinationPoint(pt, distance, bearing);*/
		//return CGeoPoint3D(pt2.m_x, pt2.m_y, pt.m_z + d.m_z, pt.GetPrjID());

		//U and V component is in the map direction. So we have to convert the point into the map projection and convert kach int geographic

		CGeoPoint3D pt2;
		if (ToWeather.GetDst()->IsGeographic())
		{
			ASSERT(ToWeather.GetSrc()->IsGeographic());
			double λ = Rad2Deg(d.m_x / 6371000);
			double φ = Rad2Deg(d.m_y / 6371000);
			pt2 = CGeoPoint3D(pt.m_x + λ, pt.m_y + φ, pt.m_z + d.m_z, pt.GetPrjID());

		}
		else
		{
			pt2 = pt;
			pt2.Reproject(ToWeather);//project in the weather projection. Problem can append when we mixte projection from differnet weather type
			pt2 += d;
			pt2.Reproject(FromWeather);//reproject in geographic
		}

		_ASSERTE(!_isnan(pt2.m_x));
		_ASSERTE(!_isnan(pt2.m_y));


		return pt2;
	}

	//TRef in UTC time...mmmm
	//level definition
	//alt en m et Z en mbar
	//alt = (293 - ((Z / 1013) ^ (1 / 5.26)) * 293) / 0.0065
	//0:	~0		(surface 2 or 10 meters)
	//1:	~110m	(1000mb)
	//2:	~323m	(975mb)
	//3:	~540m	(950mb)
	//4:	~762m	(925mb)
	//5:	~988m	(900mb)
	enum TGeoH { GEOH_0, GEOH_110, GEOH_323, GEOH_540, GEOH_762, GEOH_988, MAX_GEOH = 36 };


	//from : http://ruc.noaa.gov/RUC.faq.html
	void Convert2ThrueNorth(const CGeoPoint& pt, double &u, double &v)
	{
		//**  ROTCON_P          WIND ROTATION CONSTANT = 1 FOR POLAR STEREO AND SIN(LAT_TAN_P) FOR LAMBERT CONFORMAL
		//**  LON_XX_P          MERIDIAN ALIGNED WITH CARTESIAN X - AXIS(DEG)
		//**  LAT_TAN_P         LATITUDE AT LAMBERT CONFORMAL PROJECTION IS TRUE(DEG)


		static const double LON_XX_P = -95.0;
		static const double LAT_TAN_P = 25.0;
		static const double ROTCON_P = sin(Deg2Rad(LAT_TAN_P)); //0.422618;

		double angle = ROTCON_P * Deg2Rad(pt.m_lon - LON_XX_P);
		double sinx = sin(angle);
		double cosx = cos(angle);

		double un = cosx * u + sinx * v;
		double vn = -sinx * u + cosx * v;

		//repace
		u = un;
		v = vn;
	}
	//trouver les RAP de 2012/05 à actuel.
	//http://soostrc.comet.ucar.edu/data/grib/rap/20130817/hybrid/



	extern const char ATM_HEADER[] = "FLIGH|SCALE|Sex|A|M|G|EGGS_LAID|State|X|Y|Latitude|Longitude|T|P|U|V|W|HEIGHT|DELTA_HEIGHT|CURRENT_HEIGHT|W_HORIZONTAL|W_VERTICAL|DIRECTION|DISTANCE|DISTANCE_FROM_OIRIGINE|LIFTOFF_TIME|LANDING_TIME|LIFTOFF_T|LANDINF_T|DEFOLIATION";

	//At low altitudes above the sea level, the pressure decreases by about 1.2 kPa for every 100 meters.For higher altitudes within the troposphere, the following equation(the barometric formula) relates atmospheric pressure p to altitude h
	//12 pa/m
	//double Uw2 = -ω / 12;
	//alt en m et Z en mbar

	//http://www.ncl.ucar.edu/Document/Functions/Contributed/omega_to_w.shtml
	//p: pressure [Pa]
	//t: temperature [ᵒC]
	//ω: vertical velocity [pa/s]
	//Uw: vertical wind speed [m/s]
	double CATMVariables::get_Uw(double p, double t, double ω)
	{
		ASSERT(!_isnan(p) && !_isnan(t) && !_isnan(ω));


		static const double rgas = 287.058; //J/(kg•K) = > m²/(s²•K)
		static const double g = 9.80665; //m/s²

		double T = t + 273.15;//temperature in Kelvin
		double rho = p / (rgas*T); //density => kg/m³
		double Uw = -ω / (rho*g); //vertical wind speed [m/s]


		return Uw;

	}


	const char* CSBWMothParameters::MEMBERS_NAME[NB_ATM_MEMBERS] = { "Pmax", "Wmin", "WingBeatScale", "ReductionFactor", "ReductionHeight", "Whorzontal", "WhorzontalSD", "Wdescent", "WdescentSD","FlightTimeAfterSunrize","MaximumFlights", "ReadyToFlyMaleShift", "ReadyToFlyFemaleShift" };
	const char* CATMWorldParamters::MEMBERS_NAME[NB_MEMBERS] = { "WeatherType", "SimulationPeriod", "TimeStep", "Seed", "UseSpaceInterpol", "UseTimeInterpol", "UsePredictorCorrectorMethod", "UseVerticalVelocity", "MaximumFlyers", "MaxMissHours", "ForceFirstFlight", "BroodTSource", "PSource", "DEM", "WaterLayer", "Gribs", "HourlyDB", "Defoliation", "OutputSubHourly", "OutputFileTitle", "OutputFrequency", "CreateEggsMap", "EggsMapTitle", "EggsMapRes", "WindStabilityType", "NbWeatherStations" };




	//***********************************************************************************************

	CSBWMoth::CSBWMoth(CATMWorld& world) :
		m_world(world)
	{
		m_ID = -1;
		m_sex = -1;
		m_A = 0;
		m_M = 0;
		m_ξ = 0;
		m_G = 0;
		m_Fᵒ = 0;
		m_age = 0;
		m_Fᴰ = 0;
		m_F = 0;
		//m_loc = 0;
		//m_par = 0;
		//m_rep = 0;
		m_flightNo = 0;
		m_w_descent = 0;
		m_liffoff_time = 0;
		m_duration = 0;
		m_p_exodus = 0;

		m_state = NOT_EMERGED;
		m_flight_flag = NO_FLIGHT_DEFINE;
		m_landing_flag = NO_FLIGHT_END_DEFINE;
		m_no_liftoff_flag = NO_LIFTOFF_DEFINED;
		m_finish_flag = NO_END_DEFINE;

		m_logTime.fill(-999);
		m_logT.fill(-999);
		m_UTCShift = 0;
		m_lastF = -999;
	}

	void CSBWMoth::live(CTRef TRef)
	{
		ASSERT(!m_world.is_over_water(m_newLocation));
		ASSERT(m_state != FINISHED);

		//The median hour after sunset that give the nearest temperature is 40 minutes after sunset
		__int64 UTCTimeº = CTimeZones::TRef2Time(TRef) - m_UTCShift;
		__int64 sunset = m_world.get_sunset(TRef, m_location);
		__int64 UTCTmean = UTCTimeº + sunset + 40 * 60;

		//Get nearest grid of this time
		UTCTmean = m_world.m_weather.GetNearestFloorTime(UTCTmean);
		double Tmean = m_world.m_weather.get_air_temperature(m_pt, UTCTmean, UTCTmean);

		m_age += ComputeRate(Tmean);


		//If female : brood eggs first
		static const double PRE_OVIP = 0.1;
		if (m_sex == CSBWMothParameters::FEMALE && m_age > PRE_OVIP)
		{
			double T = -999;
			switch (m_world.m_world_param.m_broodTSource)
			{
			case CATMWorldParamters::BROOD_T_17: T = 17; break;
			case CATMWorldParamters::BROOD_AT_SUNSET:T = Tmean; break;
			default: ASSERT(false);
			}

			Brood(T);

		}

	}


	bool CSBWMoth::init_new_night(CTRef TRef)
	{
		ASSERT(m_emergingDate <= TRef);
		ASSERT(TRef.GetTM() == CTM::DAILY);
		ASSERT(m_finish_flag == NO_END_DEFINE);
		ASSERT(m_sex == CSBWMothParameters::FEMALE || m_F < 0);
		ASSERT(m_state != FINISHED);

		m_flight_flag = NO_FLIGHT_DEFINE;
		m_landing_flag = NO_FLIGHT_END_DEFINE;
		m_no_liftoff_flag = NO_LIFTOFF_DEFINED;

		m_logTime.fill(-999);
		m_logT.fill(-999);
		m_liffoff_time = 0;
		m_pt.m_alt = 10;
		m_w_horizontal = m_world.get_w_horizontal();
		m_w_descent = m_world.get_w_descent();
		m_p_exodus = m_world.random().Randu();
		m_duration = 0;
		m_noLiftoff.fill(0);

		bool bForceFirst = ForceFirst();
		bool bCanFly = CanFly();
		bool bHaveEggs = m_sex == CSBWMothParameters::FEMALE && m_F > 0;

		if (bCanFly || bHaveEggs)
		{
			if (TRef <= m_world.m_world_param.m_simulationPeriod.End())
			{

				__int64 UTCTimeº = CTimeZones::TRef2Time(TRef) - m_UTCShift;
				__int64 sunset = m_world.get_sunset(TRef, m_location);

				if (m_age < 1)
				{
					if (TRef > m_emergingDate)//dont fly the day of emergence
					{
						double readyToFly = m_world.m_moths_param.m_ready_to_fly[m_sex];
						if (m_age >= readyToFly)
						{
							bool bOverDefol = m_world.is_over_defoliation(m_newLocation);
							if (bForceFirst || bOverDefol)
							{
								if (bCanFly)
								{
									__int64 UTCWeatherTime = m_world.m_weather.GetNearestFloorTime(UTCTimeº + sunset);
									if (m_world.m_weather.IsLoaded(UTCWeatherTime))
									{
										if (GetLiftoff(UTCTimeº, sunset, m_liffoff_time))
										{
											m_state = FLY;
											m_flight_flag = WAIT_DEPARTURE;

											//compute surise of the next day
											__int64 UTCTime¹ = UTCTimeº + 24 * 3600;
											__int64 sunriseTime = UTCTime¹ + CATMWorld::get_sunrise(TRef + 1, m_location); //sunrise of the next day


											//compute duration (max) from liftoff and sunrise
											m_duration = sunriseTime - m_liffoff_time /*+ m_world.m_moths_param.m_flight_after_sunrise * 3600*/;
											ASSERT(m_duration >= 0 && m_duration < 24 * 3600);
										}
										else
										{
											m_state = LIVE;
											m_no_liftoff_flag = GetNoLiftoffCode(m_noLiftoff);
										}
									}
									else
									{
										m_state = LIVE;
										m_no_liftoff_flag = NO_LIFTOFF_MISSING_WEATHER;
									}
								}
								else
								{
									m_state = LIVE;
									m_no_liftoff_flag = NO_LIFTOFF_NO_MORE_FLIGHT;
								}
							}
							else
							{
								m_state = LIVE;
								m_no_liftoff_flag = NO_LIFTOFF_NO_DEFOLIATION;
							}
						}
						else
						{
							m_state = LIVE;
							m_no_liftoff_flag = NO_LIFTOFF_NOT_READY;
						}
					}
					else
					{
						m_state = LIVE;
						m_no_liftoff_flag = NO_LIFTOFF_EMERGING;
					}
				}
				else
				{
					m_state = FINISHED;
					m_finish_flag = END_OLD_AGE;
				}
			}
			else
			{
				m_state = FINISHED;
				m_finish_flag = END_OF_SIMULATION;
			}

		}
		else
		{
			m_state = FINISHED;
			m_finish_flag = END_FULLFILLED;
		}


		return m_liffoff_time > 0;
	}

	void CSBWMoth::AddStat(const CATMVariables& w, const CGeoDistance3D& U, const CGeoDistance3D& d)
	{
		if (w.is_init())
		{
			for (size_t i = 0; i < NB_STATS; i++)
			{
				m_stat[i][S_TAIR] += w[ATM_TAIR];
				m_stat[i][S_PRCP] += w[ATM_PRCP];
				m_stat[i][S_U] += w[ATM_WNDU];
				m_stat[i][S_V] += w[ATM_WNDV];
				m_stat[i][S_W] += w[ATM_WNDW];

				m_stat[i][S_D_X] += d.m_x;
				m_stat[i][S_D_Y] += d.m_y;
				m_stat[i][S_D_Z] += d.m_z;
				m_stat[i][S_DISTANCE] += sqrt(Square(d.m_x) + Square(d.m_y));

				m_stat[i][S_W_HORIZONTAL] += sqrt(U.m_x*U.m_x + U.m_y*U.m_y); //horizontal speed [m/s]
				m_stat[i][S_W_VERTICAL] += U.m_z;	//ascent speed [m/s]
				m_stat[i][S_HEIGHT] += m_pt.m_z;	//flight height [m]

				ASSERT(sqrt(U.m_x*U.m_x + U.m_y*U.m_y) - w.get_wind_speed() >= 0);


				m_stat[i][S_MOTH_WH] += sqrt(U.m_x*U.m_x + U.m_y*U.m_y) - w.get_wind_speed(); //horizontal speed [m/s]
				m_stat[i][S_MOTH_WV] += U.m_z - w[ATM_WNDW]; //horizontal speed [m/s]
			}
		}
	}

	void CSBWMoth::AddStat(const CATMVariables& w)
	{
		if (w.is_init())
		{
			for (size_t i = 0; i < NB_STATS; i++)
			{
				m_stat[i][S_TAIR] += w[ATM_TAIR];
				m_stat[i][S_PRCP] += w[ATM_PRCP];
				m_stat[i][S_U] += w[ATM_WNDU];
				m_stat[i][S_V] += w[ATM_WNDV];
				m_stat[i][S_W] += w[ATM_WNDW];
			}
		}
	}

	//size_t CSBWMoth::GetState()const
	//{
	//	size_t flag = NOT_INIT;
	//	if (m_state == NOT_EMERGED)
	//		flag = 0;
	//	else if (m_state == LIVE)
	//		flag = 10 + m_no_liftoff_flag;
	//	else if (m_state == FLY)
	//		flag = 20 + m_flight_end_flag;
	//	else if (m_state == FINISHED)
	//		flag = 30 + m_finish_flag;

	//	WAITING_DEPARTURE/*=-1, NO_FLIGHT_END_DEFINE*/, FLYING

	//	return flag;
	//}

	size_t CSBWMoth::GetFlag()const
	{
		size_t state = GetState();

		size_t flag = 0;
		if (m_state == NOT_EMERGED)
			flag = 0;
		else if (m_state == LIVE)
			flag = 10 + m_no_liftoff_flag;
		else if (m_state == FLY && !IsLanded())
			flag = 20 + m_flight_flag;
		else if (m_state == FLY && IsLanded())
			flag = 20 + m_flight_flag + m_landing_flag;
		else if (m_state == FINISHED)
			flag = 30 + m_finish_flag;


		return flag;
	}


	void CSBWMoth::fly(__int64 UTCWeatherTime, __int64 UTCCurrentTime)
	{
		ASSERT(m_world.m_weather.IsLoaded(UTCWeatherTime));

		__int64 countdown = UTCCurrentTime - m_liffoff_time;
		if (countdown >= 0)
		{
			if (m_world.IsInside(m_pt))
			{
				if (m_flight_flag == WAIT_DEPARTURE || m_flight_flag == FLIYNG)
					flying(UTCWeatherTime, UTCCurrentTime);
				else if (m_flight_flag == LANDING)
					landing(UTCWeatherTime, UTCCurrentTime);
			}
			else
			{
				m_state = FINISHED;
				m_finish_flag = END_OUTSIDE_MAP;
			}
		}

	}


	void CSBWMoth::flying(__int64 UTCWeatherTime, __int64 UTCCurrentTime)
	{
		ASSERT(m_world.m_weather.IsLoaded(UTCWeatherTime));
		ASSERT(m_world.IsInside(m_pt));
		ASSERT(m_flight_flag == WAIT_DEPARTURE || m_flight_flag == FLIYNG);
		ASSERT(m_liffoff_time > 0);

		CATMVariables w = get_weather(UTCWeatherTime, UTCCurrentTime);
		if (w.is_init())
		{

			if (m_logTime[T_LIFTOFF] == -999)
			{
				m_flightNo++;//a new flight for this moth
				m_logTime[T_LIFTOFF] = UTCCurrentTime;
				m_logT[T_LIFTOFF] = w[ATM_TAIR];
				m_flight_flag = FLIYNG;
			}

			double Pmax = m_world.GetPmax();
			if (w[ATM_PRCP] < Pmax)
			{
				__int64 duration = UTCCurrentTime - m_liffoff_time;
				//if (m_Δv == 1 && m_pt.m_z > 60)//m_Δv is apply atfer lift-off when the moth reach an altitude of 60 meters
					//m_Δv = m_world.m_moths_param.m_Δv;

				//after greenbank : After dark (2200 h), the orientation soon became completely downwind at all altitudes.
				//if (m_world.m_world_param.m_bUseTurbulance)
				//{
				//	//change flight angle relative to wind angle
				//	__int64 last_Δangle = UTCWeatherTime - m_Δangle_time;
				//	if (last_Δangle > 60 * 5)//if it's the same angle since 5 minutes, change it
				//	{
				//		m_Δangle_time = UTCWeatherTime;
				//		m_Δangle = WBSF::Deg2Rad(m_world.random().RandNormal(0, 40));
				//	}
				//}

				__int64 final_duration = m_duration;
				if (duration >= final_duration && IsOverWater())
					final_duration += m_world.m_moths_param.m_flight_after_sunrise * 3600;

				if (duration < final_duration)
				{
					double dt = m_world.get_time_step(); //[s]

					CGeoDistance3D U = get_U(w, UTCWeatherTime);
					CGeoDistance3D d = U * dt;


					const CProjectionTransformation& toWea = m_world.GetToWeatherTransfo(UTCWeatherTime);
					const CProjectionTransformation& fromWea = m_world.GetFromWeatherTransfo(UTCWeatherTime);
					((CGeoPoint3D&)m_pt) = UpdateCoordinate(m_pt, d, toWea, fromWea);
					((CGeoPoint3D&)m_newLocation) = UpdateCoordinate(m_newLocation, d, toWea, fromWea);

					if (m_pt.m_z <= 5)
					{
						//avoid to get negative elevation
						m_newLocation.m_z -= m_pt.m_z;
						m_pt.m_z -= m_pt.m_z;

						m_flight_flag = LANDING;
						m_landing_flag = END_BY_TAIR;
					}

					AddStat(w, U, d);
				}
				else
				{

					m_flight_flag = LANDING;
					m_landing_flag = END_BY_SUNRISE;
				}
			}
			else
			{
				m_flight_flag = LANDING;
				m_landing_flag = END_BY_PRCP;
			}
		}
		else
		{
			m_state = FINISHED;
			m_finish_flag = END_OUTSIDE_MAP;
		}

	}


	void CSBWMoth::landing(__int64 UTCWeatherTime, __int64 UTCCurrentTime)
	{
		ASSERT(m_world.m_weather.IsLoaded(UTCWeatherTime));
		ASSERT(m_world.IsInside(m_pt));
		ASSERT(m_flight_flag == LANDING);
		ASSERT(!IsLanded());

		double dt = m_world.get_time_step(); //[s]
		CATMVariables w = get_weather(UTCWeatherTime, UTCCurrentTime);
		if (w.is_init())
		{

			if (m_logTime[T_LANDING_BEGIN] == -999)
			{
				m_logTime[T_LANDING_BEGIN] = UTCCurrentTime;
				m_logT[T_LANDING_BEGIN] = w[ATM_TAIR];
			}

			CGeoDistance3D U = get_U(w, UTCWeatherTime);
			CGeoDistance3D d = U * dt;

			const CProjectionTransformation& toWea = m_world.GetToWeatherTransfo(UTCWeatherTime);
			const CProjectionTransformation& fromWea = m_world.GetFromWeatherTransfo(UTCWeatherTime);
			((CGeoPoint3D&)m_pt) = UpdateCoordinate(m_pt, d, toWea, fromWea);
			((CGeoPoint3D&)m_newLocation) = UpdateCoordinate(m_newLocation, d, toWea, fromWea);

			if (m_pt.m_z <= 5)//let moth landing correcly
			{
				//it's the end
				m_logTime[T_LANDING_END] = UTCCurrentTime;
				m_logT[T_LANDING_END] = w[ATM_TAIR];

				//end at zero
				m_newLocation.m_z -= m_pt.m_z;
				m_pt.m_z -= m_pt.m_z;
			}

			ASSERT(m_pt.m_z >= 0);
			AddStat(w, U, d);
		}
		else
		{
			m_state = FINISHED;
			m_finish_flag = END_OUTSIDE_MAP;
		}
	}

	void CSBWMoth::KillByWater()
	{
		m_state = FINISHED;
		switch (m_landing_flag)
		{
		case END_BY_TAIR: m_finish_flag = END_OVER_WATER_TAIR; break;
		case END_BY_PRCP:m_finish_flag = END_OVER_WATER_PRCP; break;
		case END_BY_SUNRISE:m_finish_flag = END_OVER_WATER_SUNRISE; break;
		default: ASSERT(false);
		}

	}

	bool CSBWMoth::IsOverWater()const
	{
		return m_world.is_over_water(m_newLocation);
	}

	CGeoDistance3D CSBWMoth::get_U(const CATMVariables& w, __int64 UTCWeatherTime)const
	{
		ASSERT(!IsMissing(w[ATM_WNDV]) && !IsMissing(w[ATM_WNDU]));
		ASSERT(m_state == FLY);

		double alpha = 0;
		if (w[ATM_WNDV] != 0 || w[ATM_WNDU] != 0)
			alpha = atan2(w[ATM_WNDV], w[ATM_WNDU]);

		if (_isnan(alpha) || !_finite(alpha))
			alpha = 0;

		double Ux = (w[ATM_WNDU] + cos(alpha)*m_w_horizontal);	//[m/s]
		double Uy = (w[ATM_WNDV] + sin(alpha)*m_w_horizontal);	//[m/s]
		double Uz = 0;

		if (m_flight_flag == FLIYNG)
			Uz = w[ATM_WNDW] + get_Uz(UTCWeatherTime, w); 	//[m/s]
		else
			Uz = w[ATM_WNDW] + m_w_descent; 	//Landing [m/s]


		ASSERT(Uz != -999);
		ASSERT(!_isnan(Ux) && !_isnan(Uy) && !_isnan(Uz));
		ASSERT(_finite(Ux) && _finite(Uy) && _finite(Uz));
		ASSERT(Ux > -100 && Ux < 100);
		ASSERT(Uy > -100 && Uy < 100);
		ASSERT(Uz > -100 && Uz < 100);

		if (_isnan(Ux) || !_finite(Ux))
			Ux = 0;

		if (_isnan(Uy) || !_finite(Uy))
			Uy = 0;

		if (_isnan(Uz) || !_finite(Uz))
			Uz = 0;

		return CGeoDistance3D(Ux, Uy, Uz, m_world.m_weather.GetGribsPrjID(UTCWeatherTime));
	}

	CSBWMoth::TNoLiftoff CSBWMoth::GetNoLiftoffCode(const std::array<size_t, 3>& noLiftoff)
	{
		std::array<size_t, 3>::const_iterator it = std::max_element(noLiftoff.begin(), noLiftoff.end());

		size_t index = (size_t)std::distance(noLiftoff.begin(), it);
		ASSERT(index < 3);

		return TNoLiftoff(NO_LIFTOFF_TAIR + index);
	}


	const double CSBWMoth::K = 167.5;//proportionality constant [Hz·cm²/√g]
	const double CSBWMoth::a = 23.0;//°C
	const double CSBWMoth::b = 8.6957;//°C
	const double CSBWMoth::Vmax = 72.5;//Hz

	//compute potential wingbeat for the current temperature
	//T : air temperature [ᵒC]
	//Vᵀ: forewing frequency [Hz] 
	double CSBWMoth::get_Vᵀ(double T)
	{
		double Vᵀ = 0;
		if (T > 0)
			Vᵀ = Vmax / (1 + exp(-(T - a) / b));

		return Vᵀ;
	}

	//Tᶠ : sustain flight temperature [ᵒC] 
	double CSBWMoth::get_Tᶠ(double Δv)const
	{
		return get_Tᶠ(m_A, m_M, Δv);//no recduction factor at liftoff
	}

	//K : proportionality constant [Hz·cm²/√g]
	//A : forewing surface area [cm²]
	//M : dry weight [g]
	//Vᴸ : frequency required for takeoff
	double CSBWMoth::get_Vᴸ(double A, double M)
	{
		return K * sqrt(M) / A;
	}

	//K : proportionality constant [Hz·cm²/√g]
	//Vmax: maximum wingbeat [Hz]
	//A : forewing surface area [cm²]
	//M : dry weight [g]
	//Tᴸ : liftoff temperature [ᵒC] 
	double CSBWMoth::get_Tᴸ(double A, double M)
	{
		double Vᴸ = get_Vᴸ(A, M);
		double Tᴸ = (Vᴸ < Vmax) ? a - b * log(Vmax / Vᴸ - 1.0) : 40;
		ASSERT(!isnan(Tᴸ));

		return Tᴸ;
	}

	//K : proportionality constant [Hz·cm²/√g]
	//Vmax: maximum wingbeat [Hz]
	//A : forewing surface area [cm²]
	//M : dry weight [g]
	//Tᶠ : sustain flight temperature [ᵒC] 
	double CSBWMoth::get_Tᶠ(double A, double M, double Δv)
	{
		double Vᴸ = get_Vᴸ(A, M);
		double Tᴸ = (Vᴸ < Vmax) ? a - b * log(Δv*Vmax / Vᴸ - 1.0) : 40;
		ASSERT(!isnan(Tᴸ));

		return Tᴸ;
	}


	double CSBWMoth::get_Uz(__int64 UTCWeatherTime, const CATMVariables& w)const
	{
		ASSERT(m_state == FLY);

		double Uz = 0;

		double Δv = (m_pt.m_z > m_world.m_moths_param.m_Hv) ? m_world.m_moths_param.m_Δv : 1;
		double Vᶠ = get_Vᵀ(w[ATM_TAIR]) * Δv;
		double Vᴸ = get_Vᴸ(m_A, m_M);

		Uz = m_world.m_moths_param.m_w_α*(Vᶠ - Vᴸ) * 1000 / 3600;//Uz can be negative


		return Uz;//m/s
	}

	CATMVariables CSBWMoth::get_weather(__int64 UTCWeatherTime, __int64 UTCCurrentTime)const
	{
		ASSERT(m_world.m_weather.IsLoaded(UTCWeatherTime));

		CATMVariables w;

		CGeoPoint3D ptᵒ = m_pt;
		CATMVariables wᵒ = m_world.get_weather(ptᵒ, UTCWeatherTime, UTCCurrentTime);
		if (wᵒ.is_init())
		{

			CGeoDistance3D Uᵒ = get_U(wᵒ, UTCWeatherTime);
			double dt = m_world.get_time_step(); //[s]

			const CProjectionTransformation& toWea = m_world.GetToWeatherTransfo(UTCWeatherTime);
			const CProjectionTransformation& fromWea = m_world.GetFromWeatherTransfo(UTCWeatherTime);
			CGeoPoint3D pt¹ = UpdateCoordinate(m_pt, Uᵒ*dt, toWea, fromWea);

			__int64 UTCCurrentTime¹ = UTCWeatherTime + int(dt / 3600);
			__int64 UTCWeatherTime¹ = m_world.m_weather.GetNearestFloorTime(UTCCurrentTime¹);
			if (m_world.m_world_param.m_bUsePredictorCorrectorMethod &&
				m_world.m_weather.IsLoaded(UTCWeatherTime¹) &&
				m_world.IsInside(pt¹) &&
				pt¹.m_z > 0)
			{
				CATMVariables w¹ = m_world.get_weather(pt¹, UTCWeatherTime¹, UTCCurrentTime¹);
				if (w¹.is_init())
					w = (wᵒ + w¹) / 2;
				else
					w = wᵒ;
			}
			else
			{
				w = wᵒ;
			}
		}

		return w;
	}

	void CSBWMoth::Brood(double T)
	{
		ASSERT(m_sex == CSBWMothParameters::FEMALE);

		static const double A = -6.4648;
		static const double B = 1.3260;
		static const double C = 2.1400;
		static const double D = 1.3050;

		//extract error term from emerging insect
		if (m_ξ == 0)
			m_ξ = m_M / exp(A + B * m_G + C * m_A + D * m_G * m_A);

		double P = 0;

		//compute new G
		const double α = 0.489;
		const double β = 15.778;
		const double c = 2.08;

		do
		{
			double ξ = m_world.random().RandLogNormal(log(1), 0.1);
			double p = α / (1 + exp(-(T - β) / c));
			P = p * ξ;

		} while (P<0 || P > 0.7);


		double broods = m_F * P;
		ASSERT(broods >= 0 && broods < m_F);

		//after Harvey 1977, regniere 1983
		//29.8 *(1 - exp(-(0.214 ))) = 5.74

		if (m_F - broods < 5.74)//eliminate too small eggs/mass  deposition
			broods = m_F;

		m_lastF = m_F;
		m_F = m_F - broods;
		ASSERT(m_G > m_F / m_Fᵒ);

		m_G = m_F / m_Fᵒ;
		ASSERT(m_G >= 0.0 && m_G <= 1.0);

		//**************************************************************
		//compute new M


		m_M = exp(A + B * m_G + C * m_A + D * m_G * m_A)*m_ξ;
	}

	double CSBWMoth::ComputeRate(double T)
	{
		//compute age
		static const double P[3] = { 57.80, -3.08, .0451 };//current P for equation

		T = max(8.0, min(35.0, T));
		double Rt = 1 / (P[0] + P[1] * T + P[2] * T*T);
		return max(0.0, Rt);
	}

	bool CSBWMoth::ComputeExodus(double T, double P, double W, double tau)
	{
		static const double C = 1.0 - 2.0 / 3.0 + 1.0 / 5.0;

		bool bExodus = false;

		double Pmax = m_world.GetPmax();
		double Wmin = m_world.m_moths_param.m_Wmin * 1000 / 3600; //km/h -> m/s 
		double Vᴸ = get_Vᴸ(m_A, m_M);
		double Vᵀ = get_Vᵀ(T);

		if (Vᵀ > Vᴸ && P < Pmax && W >= Wmin)//No lift-off if hourly precipitation greater than 2.5 mm
		{
			double p = (C + tau - (2 * pow(tau, 3) / 3) + (pow(tau, 5) / 5)) / (2 * C);

			//potential wingbeat is greather than liftoff wingbeat, then exodus if ready
			if (p > m_p_exodus)
				bExodus = true;		//this insect is exodus
		}
		else
		{
			if (Vᵀ <= Vᴸ)
				m_noLiftoff[NO_LIFTOFF_TAIR - NO_LIFTOFF_TAIR]++;

			if (P >= Pmax)
				m_noLiftoff[NO_LIFTOFF_PRCP - NO_LIFTOFF_TAIR]++;

			if (W < Wmin)
				m_noLiftoff[NO_LIFTOFF_WNDS - NO_LIFTOFF_TAIR]++;
		}

		return bExodus;
	}


	bool CSBWMoth::GetLiftoff(__int64 UTCTimeº, __int64 sunset, __int64& liftoff)
	{
		bool bExodus = false;
		liftoff = 0;

		__int64 tº = 0;
		__int64 tᴹ = 0;

		get_t(UTCTimeº, sunset, tº, tᴹ);
		__int64 tᶜ = (tº + tᴹ) / 2;

		static const __int64 Δt = 60;
		for (__int64 t = tº; t <= tᴹ && !bExodus; t += Δt)
		{
			//now compute tau, p and flight
			double tau = double(t - tᶜ) / (tᴹ - tᶜ);

			__int64 UTCCurrentTime = UTCTimeº + t;
			__int64 UTCWeatherTime = m_world.m_weather.GetNearestFloorTime(UTCCurrentTime);
			CATMVariables w = m_world.get_weather(m_pt, UTCWeatherTime, UTCCurrentTime);
			bExodus = ComputeExodus(w[ATM_TAIR], w[ATM_PRCP], w.get_wind_speed(), tau);
			if (bExodus)//if exodus occurd, set liftoff
			{
				//temporaire
				//m_logT[T_LIFTOFF] = w[ATM_TAIR];
				liftoff = UTCTimeº + t;
			}

			AddStat(w);
		}//for t in exodus period


		return bExodus;
	}

	//tᶳ [in]: sunset [s] (since the begginning of the day)
	//tº [out]: start of liftoff [s] (since the begginning of the day)
	//tᴹ [out]: end of liftoff [s] (since the begginning of the day)
	void CSBWMoth::get_t(__int64 UTCTimeº, __int64 tᶳ, __int64 &tº, __int64 &tᴹ)const
	{
		ASSERT(tᶳ < 24 * 3600);//tᶳ is sunset [s] since the begginnig of the day

		static const __int64 Δtᶠ = 5 * 3600;//s
		static const __int64 Δtᶳ = 3600;//s
		static const __int64 Δt = 60;//s
		static const double Tº = 25.4;//°C



		//maximum range of exodus hours
		tº = tᶳ + Δtᶳ - Δtᶠ / 2.0; //subtract 1.5 hours
		tᴹ = (25 - 1) * 3600; // maximum at 1:00 daylight saving time next day, -1 for normal time
		__int64 tᵀº = 0;

		for (__int64 t = tº; t <= tᴹ && tᵀº == 0; t += Δt)
		{
			__int64 UTCCurrentTime = UTCTimeº + t;
			__int64 UTCWeatherTime = m_world.m_weather.GetNearestFloorTime(UTCCurrentTime);
			CATMVariables weather = m_world.get_weather(m_pt, UTCWeatherTime, UTCCurrentTime);
			if (weather[ATM_TAIR] <= Tº)
				tᵀº = t;
		}

		ASSERT(tᵀº != 0);
		if (tᵀº == 0)//if tᵀº equal 0, no temperature under Tº. set tᵀº at 22:00 normal time
			tᵀº = 22 * 3600;

		//now calculate the real tº, tᶬ and tᶜ
		tº = max(tᶳ + Δtᶳ - Δtᶠ / 2, tᵀº);
		tᴹ = min(tº + Δtᶠ, __int64((25 - 1) * 3600));

	}

	bool CSBWMoth::CanFly()const { return m_flightNo < m_world.m_moths_param.m_maxFlights; }
	bool CSBWMoth::ForceFirst()const { return m_flightNo == 0 && m_world.m_world_param.m_bForceFirstFlight; }
	//**************************************************************************************************************
	//CATMWeatherCuboid

	CATMVariables CATMWeatherCuboid::get_weather(const CGeoPoint3D& pt, bool bSpaceInterpol)const
	{
		static const double POWER = 1;

		CATMVariables w;

		const CATMWeatherCuboid& me = *this;
		array<CStatistic, NB_ATM_VARIABLES> sumV;
		array<CStatistic, NB_ATM_VARIABLES> sumP;
		array<array<array<array<double, NB_ATM_VARIABLES>, 2>, 2>, 2> weight = { 0 };

		double nearestD = DBL_MAX;
		double nearestZ = DBL_MAX;
		CGeoPoint3DIndex nearest;

		size_t dimSize = 2;
		for (size_t z = 0; z < dimSize; z++)
		{
			for (size_t y = 0; y < dimSize; y++)
			{
				for (size_t x = 0; x < dimSize; x++)
				{
					ASSERT(pt.m_z == 0 || !m_pt[0][y][x].IsInit() || !m_pt[1][y][x].IsInit() || pt.m_z >= m_pt[0][y][x].m_z);
					ASSERT(pt.m_z == 0 || !m_pt[0][y][x].IsInit() || !m_pt[1][y][x].IsInit() || pt.m_z <= m_pt[1][y][x].m_z);
					if (m_pt[z][y][x].IsInit())
					{
						//to avoid lost weight of elevation with distance, we compute 2 weight, one for distance and one for delta elevation
						double d_xy = max(1.0, ((CGeoPoint&)m_pt[z][y][x]).GetDistance(pt));//limit to 1 meters to avoid division by zero
						double d_z = max(0.001, fabs(m_pt[z][y][x].m_z - pt.m_z));///limit to 1 mm to avoid division by zero
						double p1 = 1 / pow(d_xy, POWER);
						double p2 = 1 / pow(d_z, POWER);

						if (d_xy < nearestD)
						{
							nearestD = d_xy;
							if (d_z < nearestZ)
							{
								nearestZ = d_z;
								nearest = CGeoPoint3DIndex((int)x, (int)y, (int)z);
							}
						}

						for (size_t v = 0; v < NB_ATM_VARIABLES; v++)
						{
							if (me[z][y][x][v] > -999)
							{
								sumV[v] += me[z][y][x][v] * p1 * p2;
								sumP[v] += p1 * p2;
								weight[z][x][y][v] = p1 * p2;
							}
						}
					}//is init
				}//x
			}//y
		}//z

		if (nearest.m_x >= 0 && nearest.m_y >= 0 && nearest.m_z >= 0)
		{
			if (!bSpaceInterpol)
			{
				//take the nearest point
				for (size_t v = 0; v < NB_ATM_VARIABLES; v++)
				{
					if (me[nearest.m_z][nearest.m_y][nearest.m_x][v] > -999)
					{
						sumV[v] = me[nearest.m_z][nearest.m_y][nearest.m_x][v];
						sumP[v] = 1;
					}
				}
			}

			double sumverif = 0;
			for (size_t z = 0; z < dimSize; z++)
			{
				for (size_t y = 0; y < dimSize; y++)
				{
					for (size_t x = 0; x < dimSize; x++)
					{
						for (size_t v = 0; v < NB_ATM_VARIABLES; v++)
						{
							weight[z][x][y][v] /= sumP[v][SUM];
						}

						sumverif += weight[z][x][y][ATM_WNDU];
					}
				}
			}

			ASSERT(fabs(sumverif - 1) < 0.0001);

			//mean of 
			for (size_t v = 0; v < NB_ATM_VARIABLES; v++)
			{
				if (sumP[v].IsInit())
				{
					w[v] = sumV[v][SUM] / sumP[v][SUM];
					ASSERT(!_isnan(w[v]) && _finite(w[v]));

				}
			}
		}

		return w;
	}

	//time: time since 1 jan 1 [s]
	CATMVariables CATMWeatherCuboids::get_weather(const CGeoPoint3D& pt, __int64 UTCCurrentTime)const
	{
		ASSERT(at(0).m_time <= at(1).m_time);
		ASSERT(UTCCurrentTime >= at(0).m_time && (at(0).m_time == at(1).m_time || UTCCurrentTime <= at(1).m_time));
		ASSERT(at(1).m_time - at(0).m_time >= 0 && at(1).m_time - at(0).m_time <= 3600);

		const CATMWeatherCuboids& me = *this;
		CATMVariables w;


		if (at(1).m_time != at(0).m_time)
		{
			double fᵒ = 1 - (double(UTCCurrentTime) - at(0).m_time) / (at(1).m_time - at(0).m_time); // get fraction of time
			if (!m_bUseTimeInterpolation)
				fᵒ = fᵒ >= 0.5 ? 1 : 0;

			double f¹ = (1 - fᵒ);
			ASSERT(fᵒ + f¹ == 1);

			CATMVariables wᵒ = me[0].get_weather(pt, m_bUseSpaceInterpolation);
			CATMVariables w¹ = me[1].get_weather(pt, m_bUseSpaceInterpolation);
			if (wᵒ.is_init() && w¹.is_init())
			{
				w = wᵒ * fᵒ + w¹ * f¹;
			}

		}
		else
		{
			//last image is missing
			CATMVariables wᵒ = me[0].get_weather(pt, m_bUseSpaceInterpolation);

			if (wᵒ.is_init())
				w = wᵒ;
		}

		ASSERT(w[ATM_WNDU] >= -999);
		ASSERT(w[ATM_WNDV] >= -999);

		return w;
	}


	//**************************************************************************************************************
	//CATMWeather

	CATMVariables CATMWeather::get_weather(const CGeoPoint3D& pt, __int64 UTCWeatherTime, __int64 UTCCurrentTime)const
	{
		ASSERT(pt.IsGeographic());

		//__int64 UTCWeatherTime = m_world.m_weather.GetNearestFloorTime(UTCCurrentTime);

		size_t weather_type = m_world.m_world_param.m_weather_type;
		if (weather_type == CATMWorldParamters::FROM_GRIBS &&
			(m_world.m_world_param.m_PSource == CATMWorldParamters::PRCP_WEATHER_STATION))
			weather_type = CATMWorldParamters::FROM_BOTH;


		CATMVariables w1;
		if (weather_type == CATMWorldParamters::FROM_GRIBS ||
			weather_type == CATMWorldParamters::FROM_BOTH)
		{
			CGeoPoint3D pt2(pt);


			size_t prjID = m_p_weather_DS.GetPrjID(UTCWeatherTime);
			ASSERT(prjID != NOT_INIT);
			pt2.Reproject(m_world.m_GEO2.at(prjID));


			CATMWeatherCuboidsPtr p_cuboid = get_cuboids(pt2, UTCWeatherTime);
			w1 = p_cuboid->get_weather(pt2, UTCCurrentTime);
		}


		CATMVariables w2;
		if (weather_type == CATMWorldParamters::FROM_STATIONS ||
			weather_type == CATMWorldParamters::FROM_BOTH)
		{
			w2 = get_station_weather(pt, UTCCurrentTime, m_world.m_world_param.m_bUseTimeInterpolation);
		}


		if (m_world.m_world_param.m_PSource == CATMWorldParamters::PRCP_WEATHER_STATION)
			w1[ATM_PRCP] = w2[ATM_PRCP];

		if (m_world.m_world_param.m_weather_type == CATMWorldParamters::FROM_BOTH)
		{
			if (w2[ATM_WATER] > -999)
				w1[ATM_WATER] = w2[ATM_WATER];	//replace Gribs Tw by station Tw 

			if (w1[ATM_WNDW] > -999)
				w2[ATM_WNDW] = w1[ATM_WNDW];	//replace station W by Gribs W
		}


		CATMVariables weather;
		switch (m_world.m_world_param.m_weather_type)
		{
		case CATMWorldParamters::FROM_GRIBS: weather = w1; break;
		case CATMWorldParamters::FROM_STATIONS:weather = w2; break;
		case CATMWorldParamters::FROM_BOTH:weather = (w1 + w2) / 2; break;
		default: ASSERT(false);
		}

		/*if (weather[ATM_PRCP] == -999 &&
			m_world.m_world_param.m_PSource == CATMWorldParamters::DONT_USE_PRCP)
			weather[ATM_PRCP] = 0;
*/
		return weather;
	}

	__int64 CATMWorld::get_sunset(CTRef TRef, const CLocation& loc)
	{
		CSun sun(loc.m_lat, loc.m_lon);
		return __int64(sun.GetSunset(TRef) * 3600);
	}

	__int64 CATMWorld::get_sunrise(CTRef TRef, const CLocation& loc)
	{
		CSun sun(loc.m_lat, loc.m_lon);
		return __int64(sun.GetSunrise(TRef) * 3600);
	}

	//Ul: wind speed [m/s]
	//ΔT: difference between air temperature and water temperature [ᵒC]
	double CATMWeather::LandWaterWindFactor(double Ul, double ΔT)
	{
		double F = 0;

		if (Ul >= 7.5)
			F = 1.02 - 0.018*ΔT;
		else if (Ul >= 5.0)
			F = 1.26 - 0.023*ΔT;
		else if (Ul >= 2.5)
			F = 1.48 - 0.029*ΔT;
		else
			F = 2.14 - 0.047*ΔT;

		ASSERT(F > 0);
		return F;
	}

	void CATMWeather::GetWindProfileRelationship(double& Ur, double& Vr, double z, int stabType, bool bOverWather, double ΔT)
	{
		ASSERT(ΔT >= -50 && ΔT < 50);
		ASSERT(z >= 0);

		z = max(0.0, z);

		//	//http://homepages.cae.wisc.edu/~chinwu/CEE618_Environmental_Fluid_Mechanics/Final_Project/2000_Final_Project/Yao/wind.htm
		if (bOverWather)//over wather, transform wind
		{
			double F = LandWaterWindFactor(sqrt(Ur*Ur + Vr * Vr), ΔT);
			Ur *= F;
			Vr *= F;
		}

		//if (stabType == CWindStability::AUTO_DETECT)
		//{
		//	stabType = CWindStability::STABLE; //CWindStability::NEUTRAL;//
		//}

		double α = CWindStability::GetAlpha(CWindStability::STABLE, bOverWather); //Hellmann exponent
		//double α² = CWindStability::GetAlpha(CWindStability::NEUTRAL, bOverWather); //Hellmann exponent
		static const double Zr = 10;

		//from: http://en.wikipedia.org/wiki/Wind_gradient
		double f = pow(z / Zr, α);

		Ur *= f;
		Vr *= f;
		ASSERT(!_isnan(Ur) && !_isnan(Vr));
		ASSERT(_finite(Ur) && _finite(Vr));
	}

	CATMVariables CATMWeather::get_station_weather(const CGeoPoint3D& pt, __int64  UTCWeatherTime, bool bUseTimeInterpolation)const
	{
		CATMVariables w;
		CATMVariables wᵒ = get_station_weather(pt, UTCWeatherTime);
		CATMVariables w¹ = get_station_weather(pt, UTCWeatherTime + 3600);
		ASSERT(GetHourlySeconds(UTCWeatherTime) >= 0 && GetHourlySeconds(UTCWeatherTime) <= 3600);


		double fᵒ = 1 - GetHourlySeconds(UTCWeatherTime) / 3600.0;
		if (!bUseTimeInterpolation)
			fᵒ = fᵒ >= 0.5 ? 1 : 0;

		double f¹ = (1 - fᵒ);
		if (wᵒ.is_init() && w¹.is_init())
			w = wᵒ * fᵒ + w¹ * f¹;

		return w;
	}

	CATMVariables CATMWeather::get_station_weather(const CGeoPoint3D& pt, __int64 UTCWeatherTime)const
	{
		ASSERT(!_isnan(pt.m_x) && _finite(pt.m_x));
		ASSERT(!_isnan(pt.m_y) && _finite(pt.m_y));
		ASSERT(!_isnan(pt.m_z) && _finite(pt.m_z));

		ASSERT(pt.IsGeographic());

		CATMVariables weather;
		bool bOverWater = m_world.is_over_water(pt);

		CGridPoint gpt(pt.m_x, pt.m_y, 10, 0, 0, 0, 0, pt.GetPrjID());
		CTRef UTCTRef = CTimeZones::Time2TRef(UTCWeatherTime);

		for (size_t v = 0; v < NB_ATM_VARIABLES; v++)
			weather[v] = m_iwd.at(UTCTRef)[v].Evaluate(gpt);

		double Tw = weather[ATM_WATER];		//water temperature [ᵒC]
		double ΔT = weather[ATM_TAIR] - Tw;	//difference between air and water temperature

		GetWindProfileRelationship(weather[ATM_WNDU], weather[ATM_WNDV], pt.m_z, m_world.m_world_param.m_windS_stability_type, bOverWater, ΔT);


		//adjust air temperature with flight height with a default gradient of 0.65ᵒC/100m
		weather[ATM_TAIR] += -0.0065*pt.m_z;//flight height temperature correction

		return weather;
	}

	CGeoPointIndex CATMWeather::get_xy(const CGeoPoint& ptIn, __int64 UTCWeatherTime)const
	{
		CGeoExtents extents = m_p_weather_DS.GetExtents(UTCWeatherTime);
		CGeoPoint pt = ptIn - CGeoDistance(extents.XRes() / 2, extents.YRes() / 2, extents.GetPrjID());

		return extents.CoordToXYPos(pt);//take the lower

	}

	//pt.m_z is flight hight above ground
	size_t CATMWeather::get_level(const CGeoPointIndex& xy, const CGeoPoint3D& pt, __int64 UTCWeatherTime, bool bLow)const
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



	double CATMWeather::GetFirstAltitude(const CGeoPointIndex& xy, __int64 UTCWeatherTime)const
	{
		size_t b = m_p_weather_DS.get_band(UTCWeatherTime, ATM_HGT, 0);

		if (b == NOT_INIT)
			return -999;


		double gph = m_p_weather_DS.GetPixel(UTCWeatherTime, b, xy); //geopotential height [m]

		return gph;
	}

	CGeoPoint3DIndex CATMWeather::get_xyz(const CGeoPoint3D& pt, __int64 UTCWeatherTime)const
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
	CATMWeatherCuboidsPtr CATMWeather::get_cuboids(const CGeoPoint3D& pt, __int64 UTCWeatherTime)const
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


	string CATMWeather::get_image_filepath(__int64 UTCWeatherTime)const
	{
		TTimeFilePathMap::const_iterator it = m_filepath_map.find(UTCWeatherTime);
		ASSERT(it != m_filepath_map.end());

		string path = ".";
		if (m_filePathGribs.find('/') != string::npos || m_filePathGribs.find('\\') != string::npos)
			path = GetPath(m_filePathGribs);

		return GetAbsolutePath(path, it->second);
	}

	ERMsg CATMWeather::load_gribs(const std::string& filepath, CCallback& callback)
	{
		ERMsg msg;

		ifStream file;
		msg = file.open(filepath);
		if (msg)
		{
			//init max image to load at the sime time
			m_p_weather_DS.m_max_hour_load = m_world.m_world_param.m_max_hour_load;
			m_p_weather_DS.m_clipRect = CGeoRect(-84, 40, -56, 56, PRJ_WGS_84);// m_world.m_moths_param.m_clipRect;


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

	ERMsg CATMWeather::Discard(CCallback& callback)
	{
		m_iwd.clear();
		return m_p_weather_DS.Discard(callback);
	}

	ERMsg CATMWeather::load_hourly(const std::string& filepath, CCallback& callback)
	{
		m_p_hourly_DB.reset(new CHourlyDatabase);
		ERMsg msg = m_p_hourly_DB->Open(filepath, CHourlyDatabase::modeRead, callback);

		if (msg)
			m_filePathHDB = filepath;

		return msg;
	}

	ERMsg CATMWeather::Load(const std::string& gribsFilepath, const std::string& hourlyDBFilepath, CCallback& callback)
	{
		ERMsg msg;


		if (!gribsFilepath.empty())
		{
			msg += load_gribs(gribsFilepath, callback);
		}

		if (!hourlyDBFilepath.empty())
		{
			if (!m_p_hourly_DB)
			{
				msg += load_hourly(hourlyDBFilepath, callback);
			}
		}


		return msg;

	}

	bool CATMWeather::IsLoaded(__int64 UTCWeatherTime)const
	{
		bool bIsLoaded = true;
		if (!m_filePathGribs.empty())
			bIsLoaded = m_p_weather_DS.IsLoaded(UTCWeatherTime);

		if (bIsLoaded && !m_filePathHDB.empty())
		{
			CTRef UTCTRef = CTimeZones::Time2TRef(UTCWeatherTime);
			bIsLoaded = m_iwd.find(UTCTRef) != m_iwd.end();
		}


		return bIsLoaded;
	}

	ERMsg CATMWeather::LoadWeather(__int64 UTCWeatherTime, CCallback& callback)
	{
		ERMsg msg;

		if (!m_filePathGribs.empty())
		{
			if (!m_p_weather_DS.IsLoaded(UTCWeatherTime))
			{
				string filePath = get_image_filepath(UTCWeatherTime);
				ASSERT(!filePath.empty());
				msg = m_p_weather_DS.load(UTCWeatherTime, get_image_filepath(UTCWeatherTime), callback);

				if (msg && !m_bHgtOverSeaTested)
				{
					size_t b = m_p_weather_DS.get_band(UTCWeatherTime, ATM_HGT, 0);
					if (b != -999)
					{
						m_bHgtOverSeaTested = true;
						//test to see if we have special WRF file with HGT over ground and not over sea level
						for (int x = 0; x < m_p_weather_DS.at(UTCWeatherTime)->GetRasterXSize() && !m_bHgtOverSea; x++)
						{
							//for (int y = 0; y < m_p_weather_DS.Get(UTCTRef)->GetRasterYSize() && !m_bHgtOverSea; y++)
							if (x < m_p_weather_DS.at(UTCWeatherTime)->GetRasterYSize())
							{
								double elev = m_p_weather_DS.at(UTCWeatherTime)->GetPixel(b, CGeoPointIndex(x, x));
								if (elev > 0)
									m_bHgtOverSea = true;
							}
						}
					}
				}
			}
		}

		if (msg && !m_filePathHDB.empty())
		{
			ASSERT(m_p_hourly_DB && m_p_hourly_DB->IsOpen());

			CTRef UTCTRef = CTimeZones::UTCTime2UTCTRef(UTCWeatherTime);
			if (m_iwd.find(UTCTRef) == m_iwd.end())//if not already loaded
			{
				static const char* FILTER_STR[NB_ATM_VARIABLES] = { "T", "T", "P", "WS WD", "WS WD", "T", "T" };
				int year = UTCTRef.GetYear();


				//search all station used
				set<size_t> indexes;
				for (size_t v = 0; v < NB_ATM_VARIABLES&&msg; v++)
				{
					if (v != ATM_PRCP || m_world.m_world_param.m_PSource != CATMWorldParamters::DONT_USE_PRCP)
					{
						for (auto it = m_world.m_moths.begin(); it != m_world.m_moths.end() && msg; it++)
						{
							CSearchResultVector result;
							msg = m_p_hourly_DB->Search(result, it->m_newLocation, m_world.m_world_param.m_nb_weather_stations * 5, -1, CWVariables(FILTER_STR[v]), year);
							for (size_t ss = 0; ss < result.size(); ss++)
								indexes.insert(result[ss].m_index);

							msg += callback.StepIt(0);
						}
					}
				}


				//pre-load weather
				for (set<size_t>::const_iterator it = indexes.begin(); it != indexes.end() && msg; it++)
				{
					size_t index = *it;

					//load station in memory
					msg += m_p_hourly_DB->Get(m_stations[index], index, year);
					ASSERT(m_stations.find(index) != m_stations.end());

					m_Twater[index].Compute(m_stations[index]);//compute water temperature
					msg += callback.StepIt(0);
				}

				//create IWD object
				for (size_t v = 0; v < NB_ATM_VARIABLES&&msg; v++)
				{

					CGridPointVectorPtr pts(new CGridPointVector);

					if (v != ATM_PRCP || m_world.m_world_param.m_PSource != CATMWorldParamters::DONT_USE_PRCP)
					{
						set<size_t> indexes;
						for (auto it = m_world.m_moths.begin(); it != m_world.m_moths.end() && msg; it++)
						{
							CSearchResultVector result;
							msg = m_p_hourly_DB->Search(result, it->m_newLocation, m_world.m_world_param.m_nb_weather_stations * 5, -1, CWVariables(FILTER_STR[v]), year);
							for (size_t ss = 0; ss < result.size(); ss++)
								indexes.insert(result[ss].m_index);
						}

						for (set<size_t>::const_iterator it = indexes.begin(); it != indexes.end() && msg; it++)
						{
							size_t index = *it;
							ASSERT(m_stations.find(index) != m_stations.end());

							const CWeatherStation& station = m_stations[index];
							CTRef TRef = CTimeZones::UTCTRef2LocalTRef(UTCTRef, station);

							switch (v)
							{
							case ATM_TAIR:
							{
								CStatistic Tair = station[TRef][H_TAIR2];
								if (Tair.IsInit())
									pts->push_back(CGridPoint(station.m_x, station.m_y, 10, 0, 0, Tair[MEAN], station.m_lat, station.GetPrjID()));
							}
							break;
							case ATM_PRES:
							{
								double pres = 1013 * pow((293 - 0.0065*station.m_z) / 293, 5.26);//ASCE2005
								CStatistic presStat = station[TRef][H_PRES];
								if (presStat.IsInit())
									pres = presStat[MEAN];

								pts->push_back(CGridPoint(station.m_x, station.m_y, 10, 0, 0, pres, station.m_lat, station.GetPrjID()));
							}
							break;
							case ATM_PRCP:
							{
								CStatistic prcp = station[TRef][H_PRCP];
								if (prcp.IsInit())
									pts->push_back(CGridPoint(station.m_x, station.m_y, 10, 0, 0, prcp[SUM], station.m_lat, station.GetPrjID()));
							}
							break;
							case ATM_WNDU:
							case ATM_WNDV:
							{
								CStatistic wndS = station[TRef][H_WNDS];
								CStatistic wndD = station[TRef][H_WNDD];

								if (wndS.IsInit() && wndD.IsInit())
								{
									double ws = wndS[MEAN] * 1000 / 3600;//km/h -> m/s
									double wd = wndD[MEAN];
									ASSERT(ws >= 0 && ws < 150);
									ASSERT(wd >= 0 && wd <= 360);

									double θ = Deg2Rad(-90 - wd);
									double U = cos(θ)*ws;
									double V = sin(θ)*ws;

									double val = v == ATM_WNDU ? U : V;
									pts->push_back(CGridPoint(station.m_x, station.m_y, 10, 0, 0, val, station.m_lat, station.GetPrjID()));
								}
							}
							break;

							case ATM_WNDW:
							{
								pts->push_back(CGridPoint(station.m_x, station.m_y, 10, 0, 0, 0, station.m_lat, station.GetPrjID()));
								break;
							}
							case ATM_WATER:
							{
								CStatistic Tair = station[TRef][H_TAIR2];
								if (Tair.IsInit())
								{
									double Tw = m_Twater[index].GetTwI(TRef.Transform(CTM(CTM::DAILY)));
									pts->push_back(CGridPoint(station.m_x, station.m_y, 10, 0, 0, Tw, station.m_lat, station.GetPrjID()));
								}
							}
							break;

							default:ASSERT(false);
							}

							msg += callback.StepIt(0);
						}//for all index
					}

					CGridInterpolParam param;
					param.m_IWDModel = CGridInterpolParam::IWD_CLASIC;
					param.m_power = CGridInterpolParam::IWD_POWER;
					param.m_nbPoints = m_world.m_world_param.m_nb_weather_stations;
					param.m_bGlobalLimit = false;
					param.m_bGlobalLimitToBound = false;
					param.m_maxDistance = 1000000;
					param.m_bUseElevation = false;


					if (pts->size() < m_world.m_world_param.m_nb_weather_stations)
					{
						if (v == ATM_PRCP)
						{
							if (m_world.m_world_param.m_PSource != CATMWorldParamters::DONT_USE_PRCP)
								callback.AddMessage("WARNING: Not enaught stations with precipitation. replaced by zero.");

							while (pts->size() < m_world.m_world_param.m_nb_weather_stations)
								pts->push_back(CGridPoint(0, 0, 10, 0, 0, 0, 45, PRJ_WGS_84));
						}
						else
						{
							msg.ajoute("Not enaught stations for " + UTCTRef.GetFormatedString("%Y-%m-%d"));
						}
					}

					m_iwd[UTCTRef][v].SetDataset(pts);
					m_iwd[UTCTRef][v].SetParam(param);
					msg += m_iwd[UTCTRef][v].Initialization(callback);

				}//for all variables
			}//if not loaded


			//remove old weather
			//remove old maps (h-24)
			/*for (std::map<CTRef, std::array<CIWD, NB_ATM_VARIABLES>>::iterator it = m_iwd.begin(); it != m_iwd.end();)
			{
				if (UTCTRef - it->first > 24)
				{
					it = m_iwd.erase(it);
				}
				else
				{
					it++;
				}
			}*/

		}//if hour database

		return msg;
	}

	__int64 CATMWeather::GetNearestFloorTime(__int64 UTCTime)const
	{
		__int64 first = 0;

		if (m_world.m_world_param.m_weather_type == CATMWorldParamters::FROM_STATIONS ||
			m_world.m_world_param.m_weather_type == CATMWorldParamters::FROM_BOTH)
		{
			ASSERT(m_p_hourly_DB && m_p_hourly_DB->IsOpen());

			CTRef tmp = CTimeZones::Time2TRef(UTCTime);//automaticly trunked
			first = CTimeZones::TRef2Time(tmp);
		}

		if (m_world.m_world_param.m_weather_type == CATMWorldParamters::FROM_GRIBS ||
			m_world.m_world_param.m_weather_type == CATMWorldParamters::FROM_BOTH)
		{
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
		}

		ASSERT(first != LLONG_MAX);
		return first;
	}


	__int64 CATMWeather::GetNextTime(__int64 UTCTime)const
	{
		__int64 next = LLONG_MAX;
		if (m_world.m_world_param.m_weather_type == CATMWorldParamters::FROM_STATIONS ||
			m_world.m_world_param.m_weather_type == CATMWorldParamters::FROM_BOTH)
		{
			ASSERT(m_p_hourly_DB && m_p_hourly_DB->IsOpen());

			CTRef tmp = CTimeZones::Time2TRef(UTCTime);//automaticly trunked
			next = CTimeZones::TRef2Time(tmp + 1);//get next time step
		}

		if (m_world.m_world_param.m_weather_type == CATMWorldParamters::FROM_GRIBS ||
			m_world.m_world_param.m_weather_type == CATMWorldParamters::FROM_BOTH)
		{
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
		}

		ASSERT(next != LLONG_MAX);
		return next;
	}

	__int64 CATMWeather::GetLastTime()const { return m_filepath_map.rbegin()->first; }

	CTPeriod CATMWeather::GetEntireTPeriod()const
	{
		CTPeriod p;
		if (m_world.m_world_param.m_weather_type == CATMWorldParamters::FROM_STATIONS ||
			m_world.m_world_param.m_weather_type == CATMWorldParamters::FROM_BOTH)
		{
			ASSERT(m_p_hourly_DB && m_p_hourly_DB->IsOpen());

			set<int> years = m_p_hourly_DB->GetYears();
			if (!years.empty())
			{
				//assume that the year is complete!!!
				p += CTPeriod(CTRef(*years.begin(), FIRST_MONTH, FIRST_DAY, FIRST_HOUR), CTRef(*years.rbegin(), LAST_MONTH, LAST_DAY, LAST_HOUR));
			}

		}

		if (m_world.m_world_param.m_weather_type == CATMWorldParamters::FROM_GRIBS ||
			m_world.m_world_param.m_weather_type == CATMWorldParamters::FROM_BOTH)
		{
			ASSERT(!m_filepath_map.empty());
			if (!m_filepath_map.empty())
			{
				for (TTimeFilePathMap::const_iterator it = m_filepath_map.begin(); it != m_filepath_map.end(); it++)
				{
					__int64 UTCTime = it->first;
					p += CTimeZones::Time2TRef(UTCTime);
				}
			}
		}

		return p;
	}

	size_t CATMWeather::GetGribsPrjID(__int64 UTCWeatherTime)const
	{
		size_t prjID = NOT_INIT;


		if (m_world.m_world_param.m_weather_type == CATMWorldParamters::FROM_STATIONS)
		{
			prjID = PRJ_WGS_84;
		}
		else
		{
			if (!m_p_weather_DS.IsLoaded(UTCWeatherTime))
				m_p_weather_DS.load(UTCWeatherTime, get_image_filepath(UTCWeatherTime), CCallback());

			prjID = m_p_weather_DS.GetPrjID(UTCWeatherTime);
		}

		return prjID;
	}

	double CATMWeather::get_air_temperature(const CGeoPoint3D& pt, __int64 UTCWeatherTime, __int64 UTCCurrentTime)
	{
		double Tair = 17; //mean temperature of 17 degree by default when there is no weather 
		if (IsLoaded(UTCWeatherTime))
		{
			CGridPoint gpt(pt.m_x, pt.m_y, 10, 0, 0, 0, 0, pt.GetPrjID());
			CATMVariables w = get_weather(pt, UTCWeatherTime, UTCCurrentTime);
			Tair = w[ATM_TAIR];
		}

		return Tair;
	}


	//*********************************************************************************************************
	CTimeDatasetMap::CTimeDatasetMap()
	{
		m_max_hour_load = MAX_NUMBER_IMAGE_LOAD;
	}

	ERMsg CTimeDatasetMap::load(__int64 UTCWeatherTime, const string& filePath, CCallback& callback)const
	{
		ERMsg msg;
		CTimeDatasetMap& me = const_cast<CTimeDatasetMap&>(*this);

		me[UTCWeatherTime].reset(new CGDALDatasetCached);
		msg = me[UTCWeatherTime]->OpenInputImage(filePath, true);

		return msg;
	}

	ERMsg CTimeDatasetMap::Discard(CCallback& callback)
	{
		ERMsg msg;

		if (!empty())
		{
			CTRef TRef = CTimeZones::Time2TRef(begin()->first);
			callback.PushTask("Discard weather for " + TRef.GetFormatedString("%Y-%m-%d") + " (nbImages=" + ToString(size()) + ")", size());

			for (iterator it = begin(); it != end() && msg;)
			{
				it->second->Close();
				it = erase(it);
				msg += callback.StepIt();
			}

			callback.PopTask();
		}

		return msg;
	}

	double CTimeDatasetMap::GetPixel(__int64 UTCWeatherTime, const CGeoPoint3DIndex& index)const
	{
		ASSERT(at(UTCWeatherTime));

		double pixel = at(UTCWeatherTime)->GetPixel(index);
		return pixel;
	}

	const CGeoExtents& CTimeDatasetMap::GetExtents(__int64 UTCWeatherTime)const
	{
		ASSERT(at(UTCWeatherTime));
		ASSERT(at(UTCWeatherTime)->IsOpen());

		const CGeoExtents& extents = at(UTCWeatherTime)->GetExtents();
		return extents;
	}

	bool CTimeDatasetMap::IsLoaded(__int64 UTCWeatherTime)const
	{

		bool bRep = false;
		if (find(UTCWeatherTime) != end())
			bRep = at(UTCWeatherTime)->IsOpen();

		return bRep;
	}

	size_t CTimeDatasetMap::get_band(__int64 UTCWeatherTime, size_t v, size_t level)const
	{
		size_t band = UNKNOWN_POS;

		band = at(UTCWeatherTime)->get_band(v, level);
		return band;
	}

	bool CTimeDatasetMap::get_fixed_elevation_level(__int64 UTCWeatherTime, size_t l, double& level)const
	{
		return at(UTCWeatherTime)->get_fixed_elevation_level(l, level);
	}

	__int64 CTimeDatasetMap::GetNearestFloorTime(__int64 UTCTime)const
	{
		ASSERT(!empty());

		CTimeDatasetMap::const_iterator hi = upper_bound(UTCTime);

		if (hi == begin())
			return begin()->first;

		if (hi == end())
			return rbegin()->first;


		return (--hi)->first;
	}

	__int64 CTimeDatasetMap::GetNextTime(__int64 UTCTime)const
	{
		ASSERT(!empty());

		if (empty())
			return 0;

		CTimeDatasetMap::const_iterator hi = upper_bound(UTCTime);
		if (hi == end())
			return rbegin()->first;

		return hi->first;
	}



	//******************************************************************************************************



	double CATMWorld::get_w_horizontal()const
	{
		double ran = m_random.Rand(-1.0, 1.0);//random value [-1,1];
		double w = max(0.0, m_moths_param.m_w_horizontal + ran * m_moths_param.m_w_horizontal_σ);

		ASSERT(w >= 0);
		return w * 1000 / 3600;//convert from km/h to m/s
	}

	double CATMWorld::get_w_descent()const
	{
		double ran = m_random.Rand(-1.0, 1.0);//random value [-1,1];
		double w = min(-1.8, m_moths_param.m_w_descent + ran * m_moths_param.m_w_descent_σ);//force to descent at least at 1.8 km/h (0.5 m/s)
		ASSERT(w < 0);

		return w * 1000 / 3600;//convert from km/h to m/s
	}

	CTPeriod CATMWorld::get_moths_period()const
	{
		CTPeriod p;
		for (CSBWMothsCIt it = m_moths.begin(); it != m_moths.end(); it++)
		{
			CTRef TRef = it->m_emergingDate;//local time
			p += TRef;
		}

		return p;
	}

	double CATMWorld::GetGroundAltitude(const CGeoPoint3D& pt)const
	{
		double alt = 0;
		if (m_DEM_DS.IsOpen())
		{
			CGeoPointIndex xy = m_DEM_DS.GetExtents().CoordToXYPos(pt);
			if (m_DEM_DS.GetExtents().IsInside(xy))
			{
				double v = m_DEM_DS.GetPixel(0, xy);
				if (v != m_DEM_DS.GetNoData(0))
					alt = v;
			}
		}

		return alt;
	}


	double CATMWorld::get_defoliation(const CGeoPoint3D& pt1)const
	{


		double defoliation = 0;
		if (m_defoliation_DS.IsOpen())
		{
			CGeoPoint3D pt2(pt1);
			if (pt2.GetPrjID() != m_defoliation_DS.GetPrjID())
			{
				size_t prjID = m_defoliation_DS.GetPrjID();

				ASSERT(pt2.IsGeographic());
				pt2.Reproject(m_GEO2.at(prjID));
			}

			CGeoPointIndex xy = m_defoliation_DS.GetExtents().CoordToXYPos(pt2);
			if (m_defoliation_DS.GetExtents().IsInside(xy))
			{
				defoliation = m_defoliation_DS.GetPixel(0, xy);
				if (defoliation < 0)
					defoliation = 0;
			}
		}

		return defoliation;
	}

	bool CATMWorld::is_over_water(const CGeoPoint3D& pt1)const
	{
		bool bOverWater = false;
		if (m_water_DS.IsOpen())
		{
			CGeoPoint3D pt2(pt1);
			if (pt2.GetPrjID() != m_water_DS.GetPrjID())
			{
				size_t prjID = m_water_DS.GetPrjID();
				ASSERT(pt2.IsGeographic());
				pt2.Reproject(m_GEO2.at(prjID));
			}

			CGeoPointIndex xy = m_water_DS.GetExtents().CoordToXYPos(pt2);
			if (m_water_DS.GetExtents().IsInside(xy))
				bOverWater = m_water_DS.GetPixel(0, xy) != 0;
		}

		return bOverWater;
	}

	bool CATMWorld::is_over_defoliation(const CGeoPoint3D& pt)const
	{
		double defoliation = get_defoliation(pt);
		return defoliation > 0;
	}

	const CProjectionTransformation& CATMWorld::GetFromWeatherTransfo(__int64 UTCWeatherTime)const
	{
		size_t prjID = m_weather.GetGribsPrjID(UTCWeatherTime);
		ASSERT(prjID != NOT_INIT);

		return m_2GEO.at(prjID);

	}


	const CProjectionTransformation& CATMWorld::GetToWeatherTransfo(__int64 UTCWeatherTime)const
	{
		size_t prjID = m_weather.GetGribsPrjID(UTCWeatherTime);
		ASSERT(prjID != NOT_INIT);

		return m_GEO2.at(prjID);
	}

	//TRef is local
	CTimePeriod CATMWorld::get_UTC_sunset_period(CTRef TRef, const vector<CSBWMothsIt>& fls)
	{
		CTimePeriod UTCp(_I64_MAX, _I64_MIN);

		for (size_t i = 0; i < fls.size(); i++)
		{
			ASSERT(fls[i]->GetState() != CSBWMoth::FINISHED);

			__int64 UTCTimeº = CTimeZones::TRef2Time(TRef) - CTimeZones::GetTimeZone(fls[i]->m_location);
			__int64 UTCsunset = UTCTimeº + get_sunset(TRef, fls[i]->m_location);
			UTCp.first = min(UTCp.first, UTCsunset);
			UTCp.second = max(UTCp.second, UTCsunset);
		}

		return UTCp;
	}

	CTimePeriod CATMWorld::get_UTC_flight_period(const vector<CSBWMothsIt>& fls)
	{
		CTimePeriod UTCp(_I64_MAX, _I64_MIN);

		for (size_t i = 0; i < fls.size(); i++)
		{
			ASSERT(fls[i]->GetState() != CSBWMoth::FINISHED);
			if (fls[i]->m_liffoff_time > 0)
			{
				__int64  UTCLanding = fls[i]->m_liffoff_time + fls[i]->m_duration + m_moths_param.m_flight_after_sunrise * 3600;
				UTCp.first = min(UTCp.first, fls[i]->m_liffoff_time);
				UTCp.second = max(UTCp.second, UTCLanding + 2 * 3600); //2 hours to be ure the moths have time to land
			}
		}

		return UTCp;
	}
	bool CATMWorld::IsInside(const CGeoPoint& pt)const
	{
		bool bIsInide = false;

		CGeoPoint pt2(pt);
		if (pt.GetPrjID() != m_DEM_DS.GetPrjID())
		{
			ASSERT(pt.IsGeographic());
			size_t prjID = m_DEM_DS.GetPrjID();
			pt2.Reproject(m_GEO2.at(prjID));
		}


		return 	m_DEM_DS.GetExtents().IsInside(pt2);
	}

	ERMsg CATMWorld::Execute(CATMOutputMatrix& output, ofStream& sub_hourly, CCallback& callback)
	{
		ASSERT(m_DEM_DS.IsOpen());
		ASSERT(m_weather.is_init());
		ASSERT(!m_moths.empty());

		ERMsg msg;

		CStatistic::SetVMiss(-999);

		Init(callback);
		if (sub_hourly.is_open())
			write_sub_hourly_header(sub_hourly);

		//get period of simulation
		CTPeriod period = m_world_param.m_simulationPeriod;
		callback.PushTask("Execute dispersal for year = " + ToString(period.Begin().GetYear()) + " (" + ToString(period.GetNbDay()) + " days)", period.GetNbDay() * 2);
		callback.AddMessage("Date              NotEmerged        Emerging          WaitingToFly      Flying            FinishingEggs     Finished       ");

		//for all days
		for (CTRef TRef = period.Begin(); TRef <= period.End() && msg; TRef++)
		{
			CATMOutputMatrix sub_output;
			if (sub_hourly.is_open())
				init_sub_hourly(TRef, sub_output);	//initmemory

			ExecuteOneNight(TRef, output, sub_output, callback);

			if (msg && sub_hourly.is_open())
			{
				msg += save_sub_output(TRef, sub_hourly, sub_output, callback);
			}//if sub hourly output
		}//for all valid days

		callback.PopTask();

		FinalizedOutput(output);

		return msg;
	}

	ERMsg CATMWorld::ExecuteOneNight(CTRef TRef, CATMOutputMatrix& output, CATMOutputMatrix& sub_output, CCallback& callback)
	{
		ERMsg msg;

		const int nbSubPerHour = 3600 / m_world_param.m_outputFrequency;

		CTRef TRef16 = TRef.as(CTM::HOURLY);
		TRef16.m_hour = 16;
		CTRef TRef17 = TRef16 + 1;

		vector<CSBWMothsIt> moths;

		//compute number of not emerged and dead
		vector<CSBWMothsIt> flyers;
		size_t not_emerged = 0;
		size_t emerging = 0;
		size_t waiting_to_fly = 0;
		size_t flying = 0;
		size_t finishing_laying_eggs = 0;
		size_t finished = 0;

		for (CSBWMothsIt it = m_moths.begin(); it != m_moths.end(); it++)
		{
			if (it->GetState() == CSBWMoth::FINISHED)
			{
				finished++;
			}
			else if (TRef < it->m_emergingDate)
			{
				not_emerged++;
			}
			else
			{
				moths.push_back(it);

				if (it->m_emergingDate == TRef)
				{
					//let a chanse to output initial fecondity
					it->FillOutput(CSBWMoth::HOURLY_STAT, TRef16, output);
					emerging++;
				}
			}
		}

		vector<__int64> weather_time;
		//get all flyers for this day
		if (!moths.empty())
		{


			//get sunset hours for this day
			CTimePeriod UTC_period = get_UTC_sunset_period(TRef, moths);

			//load hours around sunset
			UTC_period.first -= 2 * 3600;
			UTC_period.second += 4 * 3600;

			weather_time = GetWeatherTime(UTC_period, callback);



			//Load weather for sunset
			if (!weather_time.empty())
			{
				msg = LoadWeather(TRef, weather_time, callback);
			}
			else
			{
				CTRef now = CTRef::GetCurrentTRef();
				if (TRef < now)//don't set warning for future simulation
					callback.AddMessage("WARNING: daily development for " + TRef.GetFormatedString() + " was skipped");
			}


			//moth will live with a default temperature of 17°C when there is no data
			//but no flight will be sheduled
			if (msg)
			{
				callback.PushTask("Live and shedule flight for: " + TRef.GetFormatedString() + " (" + ToString(moths.size()) + " moths)", moths.size());
				//init all moths : broods and liffoff time. 

#pragma omp parallel for num_threads(m_nb_max_threads)
				for (__int64 i = 0; i < (__int64)moths.size(); i++)
				{
#pragma omp flush(msg)
					if (msg)
					{
						//make old
						moths[i]->live(TRef);

						//brood and shedule flight
						if (moths[i]->init_new_night(TRef))
						{
#pragma omp critical

							flyers.push_back(moths[i]);//add moth that have flight sheduled
						}

						//report live state
						moths[i]->FillOutput(CSBWMoth::HOURLY_STAT, TRef17, output);


						//count SBW of each type
						switch (moths[i]->GetState())
						{
						case CSBWMoth::LIVE:
						{
							bool bCanFly = moths[i]->CanFly();
							if (bCanFly)
#pragma omp atomic
								waiting_to_fly++;
							else
#pragma omp atomic
								finishing_laying_eggs++;
							break;

						}
						case CSBWMoth::FLY:
#pragma omp atomic
							flying++;
							break;
						case CSBWMoth::FINISHED:
#pragma omp atomic
							finished++;
							break;
						default:ASSERT(false);
						}

						msg += callback.StepIt();
#pragma omp flush(msg)
					}//if msg
				}//for all moths

			}//if msg

			callback.PopTask();

		}//if moths

		string s1 = FormatA("%-6ld (%5.2lf%%)", not_emerged, 100.0*not_emerged / m_seasonalIndividuals);
		string s2 = FormatA("%-6ld (%5.2lf%%)", emerging, 100.0*emerging / m_seasonalIndividuals);
		string s3 = FormatA("%-6ld (%5.2lf%%)", waiting_to_fly, 100.0*waiting_to_fly / m_seasonalIndividuals);
		string s4 = !weather_time.empty() ? FormatA("%-6ld (%5.2lf%%)", flyers.size(), 100.0*flying / m_seasonalIndividuals) : string("----   ( ----%)");
		string s5 = FormatA("%-6ld (%5.2lf%%)", finishing_laying_eggs, 100.0*finishing_laying_eggs / m_seasonalIndividuals);
		string s6 = FormatA("%-6ld (%5.2lf%%)", finished, 100.0*finished / m_seasonalIndividuals);
		string feedback = FormatA("%-15s   %-15s   %-15s   %-15s   %-15s   %-15s   %-15s",
			TRef.GetFormatedString("%Y-%m-%d").c_str(),
			s1.c_str(), s2.c_str(), s3.c_str(), s4.c_str(), s5.c_str(), s6.c_str());

		callback.AddMessage(feedback);

		msg += callback.StepIt();//step it for live

		if (!flyers.empty())
		{
			msg += SimulateOneNight(TRef, flyers, output, sub_output, callback);
		}//if flyers


		if (TRef == m_world_param.m_simulationPeriod.End())
		{
			//report state at the the last day of simulation 
			FinalizedOutput(output);
		}

		msg += m_weather.Discard(callback);
		msg += callback.StepIt();



		return msg;
	}

	static const double MS2KMH = 3600.0 / 1000.0;

	//daily execution
	ERMsg CATMWorld::SimulateOneNight(CTRef TRef, vector<CSBWMothsIt>& fls, CATMOutputMatrix& output, CATMOutputMatrix& sub_output, CCallback& callback)
	{
		ASSERT(!fls.empty());

		ERMsg msg;

		size_t prjID = m_DEM_DS.GetPrjID();
		CTimePeriod UTC_period = get_UTC_flight_period(fls);

		//get gribs for the entire night
		vector<__int64> gribs_time = GetWeatherTime(UTC_period, callback);
		if (!gribs_time.empty())//gribs_time is empty when too mush missing gribs
		{
			msg = LoadWeather(TRef, gribs_time, callback);

			//Simulate dispersal for this day
			callback.PushTask("Dispersal for " + TRef.GetFormatedString("%Y-%m-%d") + " (nb flyers = " + to_string(fls.size()) + ")", gribs_time.size()*fls.size());

			//code to compare with Gary results
			//const CGeoExtents& extent = m_DEM_DS.GetExtents();
			//ifStream file;
			//if (file.open("H:\\Travaux\\Dispersal2007\\Input\\RemiLL.txt"))
			//{
			//	ofStream fileOut;
			//	if (fileOut.open("H:\\Travaux\\Dispersal2007\\Input\\RemiLL2.csv"))
			//	{
			//		fileOut.write("No,u,v,w,latitude,longitude,elevation,time,u2,v2,w2,latitude2,longitude2,latitude3,longitude3\n");

			//		
			//		CGeoPoint3D pt2 = fls[0]->m_pt;
			//		CGeoPoint3D pt3 = fls[0]->m_pt;

			//		string line;
			//		std::getline(file, line);
			//		while (std::getline(file, line))
			//		{
			//			StringVector tmp;
			//			tmp.Tokenize(line, " ", true);
			//			if (tmp.size() == 8)
			//			{
			//				int no = ToInt(tmp[0]);
			//				if (no == 22)
			//				{
			//					int gg;
			//					gg = 0;
			//				}

			//				//simulation begin at 21:00 local standard time June 21, so at 3:00 June 22 UTC
			//				CTRef UTCTRef(2007, JUNE, DAY_22, 3);
			//				__int64 UTCBaseTime = CTimeZones::UTCTRef2UTCTime(UTCTRef);

			//				float time = as<float>(tmp[7]);
			//				//UTCTRef += int(time - 21);
			//				__int64 UTCCurrentTime = UTCBaseTime + __int64((time - 21) * 3600);
			//				if (no == 1)
			//				{
			//					fls[0]->m_liffoff_time = UTCCurrentTime;
			//				}
			//				double x = as<double>(tmp[5]);
			//				double y = as<double>(tmp[4]);
			//				
			//				__int64 UTCWeatherTime = m_weather.GetNearestFloorTime(UTCCurrentTime);
			//				m_weather.LoadWeather(UTCWeatherTime, callback);

			//				CGeoExtents testExtent = m_weather.at(UTCWeatherTime)->GetExtents();
			//				CGeoPoint3D testCoord;
			//				//((CGeoPoint&)testCoord) = CGeoPoint(-267903.37025, 1935436.44478, testExtent.GetPrjID()) + CGeoDistance(x / 1040 * 356138.6973, (768 - y) / 768 * 243112.6437, testExtent.GetPrjID());
			//				((CGeoPoint&)testCoord) = CGeoPoint(x, y, PRJ_WGS_84);
			//				testCoord.m_z = as<double>(tmp[6]);
			//				//testCoord.Reproject(CProjectionTransformation(testExtent.GetPrjID(), PRJ_WGS_84));

			//				const CProjectionTransformation& toWea = GetToWeatherTransfo(UTCWeatherTime);
			//				const CProjectionTransformation& fromWea = GetFromWeatherTransfo(UTCWeatherTime);

			//				
			//				CATMVariables w1 = m_weather.get_weather(testCoord, UTCWeatherTime, UTCCurrentTime);
			//				double dt = 20; //[s]

			//				//my wind speed
			//				CGeoDistance3D U1(w1[ATM_WNDU], w1[ATM_WNDV], w1[ATM_WNDW], m_weather.GetGribsPrjID(UTCWeatherTime));
			//				CGeoDistance3D d1 = U1*dt;
			//				__int64 UTCCurrentTime2 = UTCCurrentTime +dt;
			//				__int64 UTCWeatherTime2 = m_weather.GetNearestFloorTime(UTCCurrentTime2);
			//				CGeoPoint3D testCoord2 = UpdateCoordinate(testCoord, d1, toWea, fromWea);

			//				m_weather.LoadWeather(UTCWeatherTime2, callback);
			//				CATMVariables w2 = m_weather.get_weather(testCoord2, UTCWeatherTime2, UTCCurrentTime2);
			//				
			//				CATMVariables w;
			//				CGeoDistance3D d2(m_weather.GetGribsPrjID(UTCWeatherTime2));
			//				CGeoDistance3D d3(m_weather.GetGribsPrjID(UTCWeatherTime2));
			//				if (w1.is_init() && w2.is_init())
			//				{
			//					w = (w1 + w2) / 2.0;

			//					CGeoDistance3D U2(w[ATM_WNDU], w[ATM_WNDV], w[ATM_WNDW], m_weather.GetGribsPrjID(UTCWeatherTime2));
			//					d2 = U2*dt;

			//					//Gary wind speed
			//					CGeoDistance3D U3(ToDouble(tmp[1]), ToDouble(tmp[2]), ToDouble(tmp[3]), m_weather.GetGribsPrjID(UTCWeatherTime2));
			//					d3 = U3*dt;
			//				}
			//				string out;
			//				for (size_t i = 0; i < 8; i++)
			//					out += tmp[i] + ",";

			//				out += FormatA("%.3f,%.3f,%.3f,", w[ATM_WNDU], w[ATM_WNDV], w[ATM_WNDW]);
			//				out += FormatA("%.5f,%.5f,%.5f,%.5f\n", pt2.m_y, pt2.m_x, pt3.m_y, pt3.m_x);

			//				fileOut.write(out);

			//				//update coordinate
			//				((CGeoPoint3D&)pt2) = UpdateCoordinate(pt2, d2, toWea, fromWea);
			//				((CGeoPoint3D&)pt3) = UpdateCoordinate(pt3, d3, toWea, fromWea);

			//			}

			//		}
			//		fileOut.close();
			//	}
			//	file.close();
			//}

			ASSERT(!gribs_time.empty());

			//__int64 global_seconds = gribs_time[0];//start at the actual second of the day
			for (size_t t = 1; t < gribs_time.size() && msg; t++)
			{
				CTRef UTCTRef = CTimeZones::Time2TRef(gribs_time[t - 1]);
				__int64 step_duration = gribs_time[t] - gribs_time[t - 1];

#pragma omp parallel for num_threads(m_nb_max_threads)
				for (__int64 ii = 0; ii < (__int64)fls.size(); ii++)
				{
#pragma omp flush(msg)

					CSBWMoth& flyer = *(fls[ii]);

					ASSERT((3600 % get_time_step()) == 0);
					for (__int64 seconds = 0; seconds < step_duration && msg && !flyer.IsLanded() && flyer.GetState() != CSBWMoth::FINISHED; seconds += get_time_step())
					{
						__int64 UTCCurrentTime = gribs_time[t - 1] + seconds;
						__int64 local_seconds = UTCCurrentTime;
						__int64 countdown1 = UTCCurrentTime - flyer.m_liffoff_time;
						//__int64 local_seconds = global_seconds + seconds;

						flyer.fly(gribs_time[t - 1], UTCCurrentTime);
						//report only after liftoff
						if (countdown1 >= 0)
						{

							//report oputput only each hour
							if (local_seconds % 3600 == 0 || flyer.IsLanded() || flyer.GetState() == CSBWMoth::FINISHED)
							{
								CTRef localTRef = UTCTRef + int(flyer.GetUTCShift() / 3600);
								//if landing: report now but for the next hour
								if (local_seconds % 3600 != 0)
									//if (flyer.IsLanded() || flyer.GetState() == CSBWMoth::FINISHED)
									localTRef++;

								flyer.FillOutput(CSBWMoth::HOURLY_STAT, localTRef, output);
								if (flyer.IsLanded() && flyer.IsOverWater())
								{
									flyer.KillByWater();
									flyer.FillOutput(CSBWMoth::HOURLY_STAT, localTRef + 1, output);
								}


								flyer.ResetStat(CSBWMoth::HOURLY_STAT);
							}

							//save sub-hourly result
							if (!sub_output.empty())
							{

								if (local_seconds % m_world_param.m_outputFrequency == 0 ||
									flyer.IsLanded() ||
									flyer.GetState() == CSBWMoth::FINISHED)
								{
									//if landing or finishing: report now but for the next sub-hourly step
									//if (flyer.IsLanded() || flyer.GetState() == CSBWMoth::FINISHED)
									if (local_seconds % m_world_param.m_outputFrequency != 0)
										local_seconds = (__int64(local_seconds / m_world_param.m_outputFrequency) + 1)*m_world_param.m_outputFrequency;

									int sub_hour_index = int((local_seconds + flyer.GetUTCShift()) / m_world_param.m_outputFrequency);
									CTRef TRef(sub_hour_index, 0, 0, 0, CTM::ATEMPORAL);
									flyer.FillOutput(CSBWMoth::SUB_HOURLY_STAT, TRef, sub_output);

									if (flyer.IsLanded() && flyer.IsOverWater())
									{
										flyer.KillByWater();
										flyer.FillOutput(CSBWMoth::SUB_HOURLY_STAT, TRef + 1, sub_output);
									}


									flyer.ResetStat(CSBWMoth::SUB_HOURLY_STAT);
								}
							}
						}

						msg += callback.StepIt(0);
					}//for all time step

					msg += callback.StepIt();

				}//for all flyers

				//global_seconds += step_duration;
			}//for all grib time


			//sometime the moth don't finish before hte end of weather
			//force termination
			for (__int64 ii = 0; ii < (__int64)fls.size(); ii++)
			{
				ASSERT(fls[ii]->IsLanded());
			}



			callback.PopTask();
		}//have weather
		else
		{
			callback.AddMessage("WARNING: daily flight for " + TRef.GetFormatedString() + " was skipped");
		}



		return msg;
	}

	void CSBWMoth::FillOutput(size_t stat_type, CTRef localTRef, CATMOutputMatrix& output)
	{
		ASSERT(output[m_ID].IsInside(localTRef));
		if (output[m_ID].IsInside(localTRef))
		{
			double alpha = atan2(GetStat(stat_type, CSBWMoth::S_D_Y), GetStat(stat_type, CSBWMoth::S_D_X));
			double angle = int(360 + 90 - Rad2Deg(alpha)) % 360;
			ASSERT(angle >= 0 && angle <= 360);
			double Dᵒ = m_newLocation.GetDistance(m_location, false, false);

			CGeoPoint3D pt = m_pt;

			size_t prjID = m_world.m_DEM_DS.GetPrjID();
			pt.Reproject(m_world.m_GEO2.at(prjID));//convert from GEO to DEM projection

			double defoliation = -999;
			if (m_no_liftoff_flag != NO_LIFTOFF_DEFINED &&
				m_landing_flag != NO_FLIGHT_END_DEFINE)
				defoliation = m_world.get_defoliation(m_newLocation);

			double eggsLaid = -999;

			if (stat_type == HOURLY_STAT &&
				m_sex == CSBWMothParameters::FEMALE &&
				m_state == LIVE)
			{
				eggsLaid = max(0, int(Round(m_lastF, 0) - Round(m_F, 0)));
				m_lastF = -999;
			}

			ASSERT(NB_ATM_OUTPUT == 34);
			output[m_ID][localTRef][ATM_FLIGHT] = m_flightNo;
			output[m_ID][localTRef][ATM_AGE] = m_age;
			output[m_ID][localTRef][ATM_SEX] = m_sex;
			output[m_ID][localTRef][ATM_A] = m_A;
			output[m_ID][localTRef][ATM_M] = m_M;
			output[m_ID][localTRef][ATM_EGGS] = eggsLaid;
			output[m_ID][localTRef][ATM_G] = max(-999.0, m_G);
			output[m_ID][localTRef][ATM_F] = max(-999.0, Round(m_F, 0));
			output[m_ID][localTRef][ATM_STATE] = m_state;
			output[m_ID][localTRef][ATM_FLAG] = GetFlag();
			output[m_ID][localTRef][ATM_X] = pt.m_x;
			output[m_ID][localTRef][ATM_Y] = pt.m_y;
			output[m_ID][localTRef][ATM_LAT] = m_newLocation.m_lat;
			output[m_ID][localTRef][ATM_LON] = m_newLocation.m_lon;
			output[m_ID][localTRef][ATM_DEFOLIATION] = defoliation;


			if (m_state == LIVE || m_state == FLY)
			{
				output[m_ID][localTRef][ATM_T] = GetStat(stat_type, S_TAIR);
				output[m_ID][localTRef][ATM_P] = GetStat(stat_type, S_PRCP, 1, HIGHEST);
				output[m_ID][localTRef][ATM_U] = GetStat(stat_type, S_U, MS2KMH);
				output[m_ID][localTRef][ATM_V] = GetStat(stat_type, S_V, MS2KMH);
				output[m_ID][localTRef][ATM_W] = GetStat(stat_type, S_W, MS2KMH);
			}

			if (m_logTime[T_LIFTOFF] > -999)
			{
				output[m_ID][localTRef][ATM_MEAN_HEIGHT] = GetStat(stat_type, S_HEIGHT);
				output[m_ID][localTRef][ATM_CURRENT_HEIGHT] = m_pt.m_z;
				output[m_ID][localTRef][ATM_DELTA_HEIGHT] = GetStat(stat_type, S_D_Z, 1, SUM);
				output[m_ID][localTRef][ATM_MOTH_SPEED] = GetStat(stat_type, S_MOTH_WH, MS2KMH);
				output[m_ID][localTRef][ATM_W_HORIZONTAL] = GetStat(stat_type, S_W_HORIZONTAL, MS2KMH);
				output[m_ID][localTRef][ATM_W_VERTICAL] = GetStat(stat_type, S_W_VERTICAL, MS2KMH);
				output[m_ID][localTRef][ATM_DIRECTION] = angle;
				output[m_ID][localTRef][ATM_DISTANCE] = GetStat(stat_type, S_DISTANCE, 1, SUM);
				output[m_ID][localTRef][ATM_DISTANCE_FROM_OIRIGINE] = Dᵒ;

				if (m_logTime[T_LANDING_END] > -999)
				{
					size_t liftoffTime = CTimeZones::UTCTime2LocalTime(m_logTime[T_LIFTOFF], m_location);
					size_t landingTime = CTimeZones::UTCTime2LocalTime(m_logTime[T_LANDING_END], m_location);

					output[m_ID][localTRef][ATM_LIFTOFF_TIME] = CTimeZones::GetDecimalHour(liftoffTime);
					output[m_ID][localTRef][ATM_FLIGHT_TIME] = (landingTime - liftoffTime) / 3600.0;
					output[m_ID][localTRef][ATM_LANDING_TIME] = CTimeZones::GetDecimalHour(landingTime);
					output[m_ID][localTRef][ATM_LIFTOFF_T] = m_logT[T_LIFTOFF];
					output[m_ID][localTRef][ATM_LANDING_T] = m_logT[T_LANDING_END];
				}
			}//log exists
		}//if output

		ASSERT(output[m_ID][localTRef][ATM_DEFOLIATION] != 0);
	}

	void CATMWorld::init_sub_hourly(CTRef TRef, CATMOutputMatrix& sub_output)
	{

		CTRef TRef2 = TRef + 1;
		CTPeriod p = CTPeriod(CTRef(TRef.GetYear(), TRef.GetMonth(), TRef.GetDay(), 16), CTRef(TRef2.GetYear(), TRef2.GetMonth(), TRef2.GetDay(), 15));

		__int64 begin = CTimeZones::TRef2Time(p.Begin()) / m_world_param.m_outputFrequency;
		__int64 end = CTimeZones::TRef2Time(p.End()) / m_world_param.m_outputFrequency;

		CTPeriod outputPeriod(CTRef(begin, 0, 0, 0, CTM::ATEMPORAL), CTRef(end, 0, 0, 0, CTM::ATEMPORAL));
		sub_output.resize(m_moths.size());
		for (size_t l = 0; l < sub_output.size(); l++)
		{
			sub_output[l].Init(outputPeriod, -999);
		}
	}

	ERMsg CATMWorld::save_output(CTPeriod savedPeriod, ofStream& output_file, const CATMOutputMatrix& output, CCallback& callback)
	{
		ERMsg msg;

		//Simulate dispersal for this day
		const size_t size_struct = sizeof(__int32) + sizeof(float)*NB_ATM_OUTPUT;

		//save output
		for (size_t no = 0; no < output.size() && msg; no++)
		{
			ASSERT(output[no].size() == 24);
			std::streampos pos1 = no * (sizeof(size_t) + savedPeriod.size() * size_struct);
			output_file.seekp(pos1, ios::beg);
			output_file.write_value(no);

			CTRef TRef = output[no].GetFirstTRef();
			int offset = (TRef - savedPeriod.Begin());
			std::streampos pos2 = offset * size_struct;
			output_file.seekp(pos2, ios::cur);

			for (size_t t = 0; t < output[no].size() && msg; t++)
			{
				if (savedPeriod.IsInside(TRef + t))
				{
					__int32 tmp = (TRef + t).Get__int32();
					output_file.write_value(tmp);

					for (size_t v = 0; v < NB_ATM_OUTPUT; v++)
					{
						float value = output[no][t][v];
						output_file.write_value(value);
					}

					msg += callback.StepIt();
				}
			}//for all time step
		}//for all locations

		return msg;
	}

	void CATMWorld::init_output(CTRef TRef, CATMOutputMatrix& output)
	{
		CTRef TRef2 = TRef + 1;
		CTPeriod p = CTPeriod(CTRef(TRef.GetYear(), TRef.GetMonth(), TRef.GetDay(), 16), CTRef(TRef2.GetYear(), TRef2.GetMonth(), TRef2.GetDay(), 15));

		output.resize(m_moths.size());
		for (size_t no = 0; no < output.size(); no++)
		{
			output[no].Init(p, -999);
		}
	}

	ERMsg CATMWorld::save_sub_output(CTRef TRef, ofStream& sub_output_file, const CATMOutputMatrix& sub_output, CCallback& callback)
	{
		ERMsg msg;

		const int nbSubPerHour = 3600 / m_world_param.m_outputFrequency;

		size_t nbLines = sub_output.size()*sub_output[0].size();
		boost::dynamic_bitset<size_t> have_data(nbLines);

		//save sub-hourly output
		for (size_t no = 0; no < sub_output.size() && msg; no++)
		{
			for (size_t t = 0; t < sub_output[no].size() && msg; t++)
			{
				bool bHaveData = false;
				for (size_t v = 0; v < NB_ATM_OUTPUT && !bHaveData; v++)
				{
					if (sub_output[no][t][v] > -999)
						bHaveData = true;
				}

				if (bHaveData)
				{
					size_t i = no * sub_output[no].size() + t;
					have_data.set(i);
				}

				msg += callback.StepIt();
			}
		}


		//Simulate dispersal for this day
		callback.PushTask("Save sub-hourly data for " + TRef.GetFormatedString("%Y-%m-%d") + " (nb lines = " + to_string(have_data.count()) + ")", have_data.count());

		//save sub-hourly output
		for (size_t no = 0; no < sub_output.size() && msg; no++)
		{
			for (size_t t = 0; t < sub_output[no].size() && msg; t++)
			{
				size_t i = no * sub_output[no].size() + t;

				if (have_data.test(i))
				{
					size_t local_time = (sub_output[no].GetFirstTRef().m_ref + t) * m_world_param.m_outputFrequency;
					CTRef TRefH = CTimeZones::Time2TRef(local_time);
					size_t nb_seconds = local_time - CTimeZones::TRef2Time(TRefH);
					size_t minutes = nb_seconds / 60;
					size_t seconds = nb_seconds - minutes * 60;
					ASSERT(seconds == 0);

					//hour here are in UTC


					sub_output_file << no + 1 << ",";
					sub_output_file << TRefH.GetYear() << "," << TRefH.GetMonth() + 1 << "," << TRefH.GetDay() + 1 << "," << TRefH.GetHour() << "," << minutes << "," << seconds;
					for (size_t v = 0; v < NB_ATM_OUTPUT; v++)
						sub_output_file << "," << sub_output[no][t][v];

					sub_output_file << endl;
					msg += callback.StepIt();
				}
			}//for all time step
		}//for all locations

		callback.PopTask();
		return msg;
	}

	void CATMWorld::FinalizedOutput(CATMOutputMatrix& output)
	{
		CTPeriod period = m_world_param.m_simulationPeriod;
		CTRef TRefEnd = (period.End() + 1).as(CTM::HOURLY);
		TRefEnd.m_hour = 15;
		//output all non-finish moth
		for (CSBWMothsIt it = m_moths.begin(); it != m_moths.end(); it++)
		{
			if (it->GetState() != CSBWMoth::FINISHED)
			{
				it->init_new_night(period.End() + 1);
				//let a chanse to output finish flag
				it->FillOutput(CSBWMoth::HOURLY_STAT, TRefEnd, output);
			}
		}

	}

	vector<__int64> CATMWorld::GetWeatherTime(CTimePeriod UTC_period, CCallback& callback)const
	{
		vector<__int64> gribs_time;

		if (UTC_period.first <= UTC_period.second)
		{
			__int64 firstGribTime = m_weather.GetNearestFloorTime(UTC_period.first);
			__int64 lastGribTime = m_weather.GetNextTime(UTC_period.second);
			__int64 UTCLast = firstGribTime;
			__int64 p_required = (UTC_period.second - UTC_period.first) / 3600;
			__int64 p_available = (lastGribTime - firstGribTime) / 3600;

			if (p_required <= p_available)
			{
				//we use <  instead of <= to avoid getting a infinity loop
				for (__int64 UTCWeatherTime = firstGribTime; UTCWeatherTime < lastGribTime; UTCWeatherTime = m_weather.GetNextTime(UTCWeatherTime))
				{
					if (UTCWeatherTime - UTCLast <= __int64(m_world_param.m_max_missing_weather * 3600))
					{
						gribs_time.push_back(UTCWeatherTime);
						UTCLast = UTCWeatherTime;
					}
					else
					{
						CTRef TRef = CTimeZones::Time2TRef(UTC_period.first);
						callback.AddMessage("WARNING: too much Gribs missing for " + TRef.GetFormatedString("%Y-%m-%d"));
						gribs_time.clear();
						UTCWeatherTime = lastGribTime + 9999;//end loop for
					}
				}

				if (lastGribTime == m_weather.GetLastTime())
					gribs_time.push_back(lastGribTime);
			}
			else
			{
				if (p_available > 0)
				{
					CTRef TRef = CTimeZones::Time2TRef(UTC_period.first);
					callback.AddMessage("WARNING: too much Gribs missing for " + TRef.GetFormatedString("%Y-%m-%d"));
					gribs_time.clear();
				}
			}
		}



		return gribs_time;
	}

	ERMsg CATMWorld::LoadWeather(CTRef TRef, const vector<__int64>& weather_time, CCallback& callback)
	{
		ERMsg msg;

		//MessageBox(NULL, to_wstring(weather_time.size()).c_str(), L"Load weather", MB_OK);

		ASSERT(!weather_time.empty());

		vector<__int64> weatherToLoad;
		for (size_t i = 0; i < weather_time.size() && msg; i++)
		{
			if (!m_weather.IsLoaded(weather_time[i]))
				weatherToLoad.push_back(weather_time[i]);
		}

		callback.PushTask("Load weather for " + TRef.GetFormatedString("%Y-%m-%d") + " (nb hours=" + ToString(weatherToLoad.size()) + ")", weatherToLoad.size());

		//pre-Load weather for the day
		for (size_t i = 0; i < weatherToLoad.size() && msg; i++)
		{
			__int64 UTCWeatherTime = weatherToLoad[i];
			ASSERT(!m_weather.IsLoaded(UTCWeatherTime));

			msg += m_weather.LoadWeather(UTCWeatherTime, callback);
			if (msg)
			{
				size_t prjID = m_weather.GetGribsPrjID(UTCWeatherTime); ASSERT(prjID != NOT_INIT);
				if (m_GEO2.find(prjID) == m_GEO2.end())
					m_GEO2[prjID] = GetReProjection(PRJ_WGS_84, prjID);
				if (m_2GEO.find(prjID) == m_2GEO.end())
					m_2GEO[prjID] = GetReProjection(prjID, PRJ_WGS_84);
			}
			msg += callback.StepIt();
		}


		callback.PopTask();

		return msg;
	}

	ERMsg CATMWorld::Init(CCallback& callback)
	{
		ASSERT(m_DEM_DS.IsOpen());
		assert(m_weather.is_init());
		assert(!m_moths.empty());
		ERMsg msg;

		//register projection
		if (m_water_DS.IsOpen())
		{
			size_t prjID = m_water_DS.GetPrjID(); ASSERT(prjID != NOT_INIT);
			if (m_GEO2.find(prjID) == m_GEO2.end())
				const_cast<CATMWorld&>(*this).m_GEO2[prjID] = GetReProjection(PRJ_WGS_84, prjID);
			if (m_2GEO.find(prjID) == m_2GEO.end())
				const_cast<CATMWorld&>(*this).m_2GEO[prjID] = GetReProjection(prjID, PRJ_WGS_84);
		}

		if (m_defoliation_DS.IsOpen())
		{
			size_t prjID = m_defoliation_DS.GetPrjID(); ASSERT(prjID != NOT_INIT);
			if (m_GEO2.find(prjID) == m_GEO2.end())
				const_cast<CATMWorld&>(*this).m_GEO2[prjID] = GetReProjection(PRJ_WGS_84, prjID);
			if (m_2GEO.find(prjID) == m_2GEO.end())
				const_cast<CATMWorld&>(*this).m_2GEO[prjID] = GetReProjection(prjID, PRJ_WGS_84);
		}

		size_t prjID = m_DEM_DS.GetPrjID(); ASSERT(prjID != NOT_INIT);
		if (m_GEO2.find(prjID) == m_GEO2.end())
			const_cast<CATMWorld&>(*this).m_GEO2[prjID] = GetReProjection(PRJ_WGS_84, prjID);
		if (m_2GEO.find(prjID) == m_2GEO.end())
			const_cast<CATMWorld&>(*this).m_2GEO[prjID] = GetReProjection(prjID, PRJ_WGS_84);

		random().Randomize(m_world_param.m_seed);


		return msg;
	}

	void CATMWorld::write_sub_hourly_header(ofStream& output_file)
	{
		ASSERT(output_file.is_open());

		output_file << "no,Year,Month,Day,Hour,Minute,Second,";
		output_file << "flight,Age,sex,A,M,EggsLaid,G,F,state,flag,X,Y,lat,lon,";
		output_file << "T,P,U,V,W,";
		output_file << "MeanHeight,CurrentHeight,MeanDeltaHeight,MothSpeed,HorizontalSpeed,VerticalSpeed,Direction,Distance,DistanceFromOrigine,FlightTime,LiftoffTime,LandingTime,LiftoffT,LandingT,Defoliation" << endl;
	}


	ERMsg CATMWorld::CreateEggDepositionMap(const string& outputFilePath, CATMOutputMatrix& output, CCallback& callback)
	{
		ASSERT(m_DEM_DS.IsOpen());

		ERMsg msg;

		if (!output.empty() && !output[0].empty() && !output[0][0].empty() && !output[0]/*[0][0]*/.empty())
		{

			CBaseOptions options;
			m_DEM_DS.UpdateOption(options);

			CTPeriod period;
			CStatistic seasonStat;
			CGeoExtents extents;


			size_t nbSteps = output.size() * output[0].size() * output[0][0].size() * output[0]/*[0][0]*/.size();
			callback.PushTask("Compute Eggs map extents", nbSteps);

			for (size_t no = 0; no < output.size() && msg; no++)
			{
				for (size_t t = 0; t < output[no].size() && msg; t++)
				{
					if (output[no][t][ATM_EGGS] > 0)
					{
						seasonStat += output[no][t][ATM_EGGS];
						CTRef TRef = output[no].GetFirstTRef() + t;
						period.Inflate(TRef);
						CGeoPoint pt(output[no][t][ATM_X], output[no][t][ATM_Y], options.m_extents.GetPrjID());
						extents.ExtendBounds(pt);
					}
					msg += callback.StepIt();
				}
			}

			callback.PopTask();

			period.Transform(CTM::DAILY);

			//Get input info
			if (!extents.IsRectEmpty() && msg)
			{
				callback.PushTask("Compute Eggs map extents", nbSteps);

				extents.m_xSize = std::max(1, (int)ceil(fabs((extents.m_xMax - extents.m_xMin) / m_world_param.m_eggMapsResolution)));
				extents.m_ySize = std::max(1, (int)ceil(fabs((extents.m_yMin - extents.m_yMax) / m_world_param.m_eggMapsResolution)));

				extents.m_xMax = extents.m_xMin + extents.m_xSize*m_world_param.m_eggMapsResolution;
				extents.m_yMin = extents.m_yMax - extents.m_ySize*m_world_param.m_eggMapsResolution;

				options.m_bOverwrite = true;
				options.m_extents = extents;
				options.m_nbBands = 1;
				options.m_outputType = GDT_Float32;
				options.m_dstNodata = -9999;
				options.m_createOptions.push_back("-co COMPRESS=LZW");
				options.m_bComputeStats = true;
				options.m_overviewLevels = { { 2, 4, 8, 16 } };


				//Open input image
				CGDALDatasetEx outputDS;
				msg += outputDS.CreateImage(outputFilePath, options);


				if (msg)
				{
					vector<float> season(extents.m_ySize*extents.m_xSize);


					for (size_t d = 0; d < period.size() && msg; d++)
					{
						CTRef TRef1 = period.Begin() + d;

						for (size_t no = 0; no < output.size() && msg; no++)
						{
							for (size_t t = 0; t < output[no].size() && msg; t++)
							{
								if (output[no][t][ATM_EGGS] > 0)
								{
									CTRef TRef2 = output[no].GetFirstTRef() + t;
									if (TRef2.as(CTM::DAILY) == TRef1)
									{
										CGeoPoint pt(output[no][t][ATM_X], output[no][t][ATM_Y], extents.GetPrjID());
										ASSERT(extents.IsInside(pt));

										CGeoPointIndex xy = options.m_extents.CoordToXYPos(pt);
										size_t pos = xy.m_y*extents.m_xSize + xy.m_x;
										season[pos] += output[no][t][ATM_EGGS];
									}
								}
								msg += callback.StepIt();
							}
						}
					}

					GDALRasterBand* pBandOut = outputDS.GetRasterBand(0);//first band keep for the seanson
					pBandOut->RasterIO(GF_Write, 0, 0, extents.m_xSize, extents.m_ySize, &(season[0]), extents.m_xSize, extents.m_ySize, GDT_Float32, 0, 0);

					outputDS.Close();
				}
			}

			callback.PopTask();
		}

		return msg;
	}


	std::mutex CGDALDatasetCached::m_mutex;
	CGDALDatasetCached::CGDALDatasetCached()
	{
		array<size_t, NB_LEVELS> unknownPos;
		unknownPos.fill(UNKNOWN_POS);
		m_bands.fill(unknownPos);
	}

	double CGDALDatasetCached::GetPixel(const CGeoPoint3DIndex& xyz)const
	{
		if (!m_extents.IsInside(xyz))
			return -9999;

		if (xyz.m_z == -1)
			return -9999;


		CGeoBlock3DIndex ijk = m_extents.GetBlockIndex(xyz);
		CGeoPointIndex xy = xyz - m_extents.GetBlockRect(ijk.m_x, ijk.m_y).UpperLeft();
		ASSERT(m_extents.GetBlockExtents(ijk.m_x, ijk.m_y).GetPosRect().IsInside(xy));

		CGDALDatasetCached& me = const_cast<CGDALDatasetCached&>(*this);
		if (!IsCached(ijk))
			me.LoadBlock(ijk);

		assert(m_data[ijk.m_z][ijk.m_y][ijk.m_x]);
		assert(IsBlockInside(ijk));

		return m_data[ijk.m_z][ijk.m_y][ijk.m_x]->GetValue(xy.m_x, xy.m_y);

	}

	CBlockData::CBlockData(int nXBlockSize, int nYBlockSize, int dataType)
	{
		int dataSize = GDALGetDataTypeSize((GDALDataType)dataType) / sizeof(char);
		m_ptr = new char[nXBlockSize * nYBlockSize * dataSize];
		m_xBlockSize = nXBlockSize;
		m_yBlockSize = nYBlockSize;
		m_dataType = dataType;
	}

	double CBlockData::GetValue(int x, int y)const
	{
		size_t pos = size_t(y) * m_xBlockSize + x;
		return GetValue(pos);
	}

	double CBlockData::GetValue(size_t pos)const
	{
		double value = 0;
		switch (m_dataType)
		{
		case GDT_Byte:		value = double(((char*)m_ptr)[pos]); break;
		case GDT_UInt16:	value = double(((unsigned __int16*)m_ptr)[pos]); break;
		case GDT_Int16:		value = double(((__int16*)m_ptr)[pos]); break;
		case GDT_UInt32:	value = double(((unsigned __int32*)m_ptr)[pos]); break;
		case GDT_Int32:		value = double(((__int32*)m_ptr)[pos]); break;
		case GDT_Float32:	value = double(((float*)m_ptr)[pos]); break;
		case GDT_Float64:	value = double(((double*)m_ptr)[pos]); break;
		}

		return value;
	}

	void CBlockData::SetValue(int x, int y, double value)
	{
		size_t pos = size_t(y) * m_xBlockSize + x;
		SetValue(pos, value);
	}

	void CBlockData::SetValue(size_t pos, double value)
	{
		switch (m_dataType)
		{
		case GDT_Byte:		((char*)m_ptr)[pos] = (char)value; break;
		case GDT_UInt16:	((unsigned __int16*)m_ptr)[pos] = (unsigned __int16)value; break;
		case GDT_Int16:		((__int16*)m_ptr)[pos] = (__int16)value; break;
		case GDT_UInt32:	((unsigned __int32*)m_ptr)[pos] = (unsigned __int32)value; break;
		case GDT_Int32:		((__int32*)m_ptr)[pos] = (__int32)value; break;
		case GDT_Float32:	((float*)m_ptr)[pos] = (float)value; break;
		case GDT_Float64:	((double*)m_ptr)[pos] = (double)value; break;
		}
	}


	size_t GetVar(const string& strVar)
	{
		size_t var = UNKNOWN_POS;

		if (strVar == "TMP")
			var = ATM_TAIR;
		else if (strVar == "PRES")
			var = ATM_PRES;
		else if (strVar == "PRATE")//PRATE is in mm/s and APCP is in mm : problem.... || strVar == "APCP"
			var = ATM_PRCP;
		else if (strVar == "UGRD")
			var = ATM_WNDU;
		else if (strVar == "VGRD")
			var = ATM_WNDV;
		else if (strVar == "WGRD" || strVar == "DZDT")
			var = ATM_WNDW;
		else if (strVar == "VVEL")
			var = ATM_VVEL;
		else if (strVar == "HGT")
			var = ATM_HGT;
		else if (strVar == "RH")
			var = ATM_RH;

		return var;
	}

	size_t GetLevel(const string& strLevel)
	{
		size_t level = UNKNOWN_POS;


		if (strLevel == "sfc" || strLevel == "surface" || strLevel == "2 m above ground" || strLevel == "2 m above gnd" || strLevel == "10 m above ground" || strLevel == "10 m above gnd")
		{
			level = 0;
		}
		else if (Find(strLevel, "isotherm") || Find(strLevel, "tropopause") || Find(strLevel, "above ground") || Find(strLevel, "above gnd"))
		{
			//do nothing
		}
		else
		{
			bool bValid1 = strLevel.find('-') == string::npos && strLevel.length() >= 6 && isdigit(strLevel[0]) && strLevel.substr(strLevel.length() - 2) == "mb";
			bool bValid2 = strLevel.length() == 1 || strLevel.length() == 2;
			if (bValid1 || bValid2)
			{
				level = ToSizeT(strLevel);
				if (level >= 100 && level <= 1000)
				{
					//RUC format
					level = 38 - (level - 100) / 25 - 1;
					ASSERT(level > 0 && level < NB_LEVELS);
				}
			}
		}

		return level;
	}

	ERMsg CGDALDatasetCached::OpenInputImage(const std::string& filePath, bool bOpenInv, bool bSurfaceOnly)
	{
		ERMsg msg;



		msg = CGDALDatasetEx::OpenInputImage(filePath);
		if (msg)
		{
			InitCache();

			//Load band positions (not the same for all images
			if (bOpenInv)
			{
				string invFilePath(filePath);
				SetFileExtension(invFilePath, ".inv");
				if (FileExists(invFilePath))
				{
					ifStream file;
					msg = file.open(invFilePath);
					if (msg)
					{
						size_t i = 0;
						for (CSVIterator loop(file, ":", false); loop != CSVIterator() && msg; ++loop, i++)
						{

							ASSERT(loop->size() >= 6);
							if (loop->size() >= 6)
							{
								string strVar = (*loop)[3];
								string strLevel = (*loop)[4];
								size_t var = GetVar(strVar);
								size_t level = var != UNKNOWN_POS ? GetLevel(strLevel) : UNKNOWN_POS;
								if (bSurfaceOnly && level > 0)//open only surface for optimisation
									level = NOT_INIT;

								if (var < NB_ATM_VARIABLES_EX && level < m_bands[var].size())
								{
									m_bands[var][level] = i;
								}
							}
							else
							{
								if (!TrimConst(loop->GetLastLine()).empty())
									msg.ajoute("Bad .inv file : " + invFilePath);
							}
						}
					}
				}
				else
				{


					if (m_bVRT)
					{

						//add fixed level 40, 80 and 120
						m_fixedElevationLevel[1] = 40;
						m_fixedElevationLevel[2] = 80;
						m_fixedElevationLevel[3] = 120;

						//HRDPS gribs
						//find variable from file name
						ASSERT(m_internalName.size() == GetRasterCount());
						for (size_t i = 0; i < m_internalName.size(); i++)
						{

							//PRATE is only for forcast hours, not for the analyses hour... hummm
							string title = GetFileTitle(m_internalName[i]);
							StringVector tmp(title, "_");
							ASSERT(tmp.size() == 9);

							string strVar = tmp[3];
							string strType = tmp[4];
							string strLevel = tmp[5];
							size_t var = GetVar(strVar);

							if (var < NB_ATM_VARIABLES_EX)
							{
								size_t level = -1;
								if (strType == "SFC" || strType == "TGL")
								{
									if (strLevel == "0" || strLevel == "2" || strLevel == "10")
										level = 0;
									if (strLevel == "40")
										level = 1;
									if (strLevel == "80")
										level = 2;
									if (strLevel == "120")
										level = 3;
									//il y aurait moyen d'utilise TMP uwnd vwund a 40 80 et 120...
								}
								else if (strType == "ISBL")
								{
									size_t l = ToSizeT(strLevel);
									if (l == 1015)
										level = 4;
									else if (l == 1000)
										level = 5;
									else if (l == 985)
										level = 6;
									else if (l == 970)
										level = 7;
									else if (l == 950)
										level = 8;
									else if (l == 925)
										level = 9;
									else if (l == 900)
										level = 10;
									else if (l == 875)
										level = 11;
									else
									{
										//850 --> 12
										//50 --> 28
										level = 17 - l / 50 + 12;
										ASSERT(level > 4 && level < 42);
									}
								}

								if (bSurfaceOnly && level > 0)//open only surface for optimisation
									level = NOT_INIT;

								if (level < 38)
									m_bands[var][level] = i;
							}
						}
					}
					else
					{
						//get info from metadata
						BandsMetaData meta_data;

						GetBandsMetaData(meta_data);
						ASSERT(meta_data.size() == GetRasterCount());


						for (size_t i = 0; i < meta_data.size(); i++)
						{

							string strVar = meta_data[i]["GRIB_ELEMENT"];
							string strName = meta_data[i]["GRIB_SHORT_NAME"];
							StringVector description(meta_data[i]["description"], " =[]\"");
							if (description.size() > 3 && (description[2] == "ISBL" || description[2] == "SFC" || description[2] == "HTGL" || description[2] == "HYBL"))
							{
								if (!description.empty() && description[0].find('-') != NOT_INIT)
									description.clear();

								//description.empty();

								size_t var = GetVar(strVar);
								if (var < NB_ATM_VARIABLES_EX && !description.empty())
								{
									size_t level = as<size_t>(description[0]);

									if (description[2] != "HYBL")
									{
										if (level <= 10)
										{
											level = 0;
										}
										else if (level >= 10000 && level <= 100000)
										{
											level = 38 - (level / 100 - 100) / 25 - 1;
											ASSERT(level < NB_LEVELS);
										}
									}

									if (level < m_bands[var].size())
									{
										m_bands[var][level] = i;

									}
								}
							}
						}
					}
				}
			}
		}

		//report prep 0 to all level
		for (size_t j = 1; j < NB_LEVELS; j++)
			m_bands[ATM_PRCP][j] = m_bands[ATM_PRCP][0];

		//copy VVEL 1 to surface
		if (m_bands[ATM_VVEL][1] != UNKNOWN_POS && m_bands[ATM_VVEL][0] == UNKNOWN_POS)
		{
			m_bands[ATM_VVEL][0] = m_bands[ATM_VVEL][1];
		}


		return msg;
	}

	void CGDALDatasetCached::Close()
	{
		if (CGDALDatasetEx::IsOpen())
		{
			m_data.resize(boost::extents[0][0][0]);
			Dataset()->FlushCache();
			CGDALDatasetEx::Close();
		}
	}

	void CGDALDatasetCached::InitCache()const
	{
		assert(IsOpen());
		if (!IsCacheInit())
		{
			CGDALDatasetCached& me = const_cast<CGDALDatasetCached&>(*this);
			me.m_data.resize(boost::extents[GetRasterCount()][m_extents.YNbBlocks()][m_extents.XNbBlocks()]);
		}
	}

	void CGDALDatasetCached::LoadBlock(const CGeoBlock3DIndex& ijk)
	{


		m_mutex.lock();
		if (!IsCached(ijk))
		{
			assert(m_data[ijk.m_z][ijk.m_y][ijk.m_x] == NULL);
			CTimer readTime(TRUE);

			//copy part of the date
			CGeoExtents ext = GetExtents().GetBlockExtents(ijk.m_x, ijk.m_y);
			CGeoRectIndex ind = GetExtents().CoordToXYPos(ext);

			GDALRasterBand* poBand = m_poDataset->GetRasterBand(ijk.m_z + 1);

			if (m_bVRT)
			{
				int nXBlockSize = m_extents.m_xBlockSize;
				int nYBlockSize = m_extents.m_yBlockSize;


				GDALDataType type = poBand->GetRasterDataType();
				if (type == GDT_Float64)
					type = GDT_Float32;

				CBlockData* pBlockTmp = new CBlockData(nXBlockSize, nYBlockSize, type);
				poBand->RasterIO(GF_Read, ijk.m_x*nXBlockSize, ijk.m_y*nYBlockSize, nXBlockSize, nYBlockSize, pBlockTmp->m_ptr, nXBlockSize, nYBlockSize, type, 0, 0);
				poBand->FlushBlock(ijk.m_x, ijk.m_y);
				poBand->FlushCache();

				m_data[ijk.m_z][ijk.m_y][ijk.m_x].reset(pBlockTmp);
			}
			else
			{
				int nXBlockSize, nYBlockSize;


				poBand->GetBlockSize(&nXBlockSize, &nYBlockSize);


				ASSERT(nXBlockSize == m_extents.m_xBlockSize);
				ASSERT(nYBlockSize == m_extents.m_yBlockSize);
				GDALDataType type = poBand->GetRasterDataType();
				CBlockData* pBlockTmp = new CBlockData(nXBlockSize, nYBlockSize, type);
				poBand->ReadBlock(ijk.m_x, ijk.m_y, pBlockTmp->m_ptr);
				poBand->FlushBlock(ijk.m_x, ijk.m_y);
				poBand->FlushCache();

				if (type == GDT_Float64)
				{
					CBlockData* pBlockData = new CBlockData(nXBlockSize, nYBlockSize, GDT_Float32);
					for (size_t pos = 0; pos < nXBlockSize*nYBlockSize; pos++)
						pBlockData->SetValue(pos, pBlockTmp->GetValue(pos));

					delete pBlockTmp;
					pBlockTmp = pBlockData;
				}

				m_data[ijk.m_z][ijk.m_y][ijk.m_x].reset(pBlockTmp);

			}

			readTime.Stop();
			m_stats[0] += readTime.Elapsed();
		}
		m_mutex.unlock();

		ASSERT(IsCached(ijk));
	}

	size_t CGDALDatasetCached::get_band(size_t v, size_t level)const
	{
		ASSERT(level < NB_LEVELS);
		return m_bands[v][level];
	}

	bool CGDALDatasetCached::get_fixed_elevation_level(size_t l, double& elev)const
	{
		auto it = m_fixedElevationLevel.find(l);
		if (it != m_fixedElevationLevel.end())
			elev = it->second;


		return it != m_fixedElevationLevel.end();
	}

	//***************************************************************************************************************
	//class CWRFDatabase



	ERMsg CreateGribsFromText(CCallback& callback)
	{

		ERMsg msg;


		//Cells height [m]
		CGDALDatasetEx terrain_height;
		msg += terrain_height.OpenInputImage("E:\\Travaux\\Bureau\\WRF2013\\Terrain Height.tif");

		array<array<float, 252>, 201> height = { 0 };
		terrain_height.GetRasterBand(0)->RasterIO(GF_Read, 0, 0, (int)height[0].size(), (int)height.size(), &(height[0][0]), (int)height[0].size(), (int)height.size(), GDT_Float32, 0, 0);


		static const char* THE_PRJ = "PROJCS[\"unnamed\",GEOGCS[\"WGS 84\",DATUM[\"WGS_1984\",SPHEROID[\"WGS 84\", 6378137, 298.257223563,AUTHORITY[\"EPSG\", \"7030\"]],AUTHORITY[\"EPSG\", \"6326\"]],PRIMEM[\"Greenwich\", 0],UNIT[\"degree\", 0.0174532925199433],AUTHORITY[\"EPSG\", \"4326\"]],PROJECTION[\"Lambert_Conformal_Conic_2SP\"],PARAMETER[\"standard_parallel_1\", 30],PARAMETER[\"standard_parallel_2\", 60],PARAMETER[\"latitude_of_origin\", 48.000004],PARAMETER[\"central_meridian\", -69],PARAMETER[\"false_easting\", 0],PARAMETER[\"false_northing\", 0],UNIT[\"metre\", 1,AUTHORITY[\"EPSG\", \"9001\"]]]";
		msg += CProjectionManager::CreateProjection(THE_PRJ);

		size_t prjID = CProjectionManager::GetPrjID(THE_PRJ);
		CProjectionPtr prj = CProjectionManager::GetPrj(prjID);

		CProjectionTransformation Geo2LCC(PRJ_WGS_84, prjID);


		//std::string elevFilepath = "D:\\Travaux\\Brian Sturtevant\\MapInput\\DEMGrandLake4km.tif";
		//CGDALDatasetEx elevDS;
		//elevDS.OpenInputImage(elevFilepath);

		//vector < float > elev(elevDS.GetRasterXSize()* elevDS.GetRasterYSize());
		//ASSERT(elev.size()==7257);

		//GDALRasterBand* pBand = elevDS.GetRasterBand(0);
		//pBand->RasterIO(GF_Write, 0, 0, elevDS.GetRasterXSize(), elevDS.GetRasterYSize(), &(elev[0]), elevDS.GetRasterXSize(), elevDS.GetRasterYSize(), GDT_Float32, 0, 0);

		//1-pressure(hPa)
		//2-height(m above ground not above sea-level)
		//3-temperature(K)
		//4-water vapor mixing ratio(g/kg)
		//5-u - component wind speed(m/s, positive = eastward)
		//6-v - component wind speed(m/s, positive = northward)
		//7-w - component wind speed(m/s, positive = upward)
		//8-precipitation reaching the ground in the last hour(mm)

		static const size_t NB_WRF_LEVEL = 37;
		enum TWRFVars { WRF_PRES, WRF_HGHT, WRF_TAIR, WRF_WVMR, WRF_UWND, WRF_VWND, WRF_WWND, WRF_PRCP, NB_WRF_VARS };
		static const char* VAR_NAME[NB_WRF_VARS] = { "Pres", "Hght", "Tair", "WVMR", "Uwnd", "Vwnd", "Wwnd", "Prcp" };
		CBaseOptions options;
		options.m_nbBands = NB_WRF_LEVEL * (NB_WRF_VARS - 1) + 1;//prcp have only one band
		options.m_outputType = GDT_Float32;
		options.m_createOptions.push_back("COMPRESS=LZW");
		options.m_dstNodata = -9999;
		//options.m_extents = CGeoExtents(-280642.1,1903575,133144.6,2205765,101,74,101,1,prjID);
		options.m_extents = CGeoExtents(-524000, 402000, 488000, -402000, 252, 201, 252, 1, prjID);//attention ici il y a 252 pixel, mais dans le .nc il y en a 253!!! donc le x size n'est pas 4000...
		options.m_prj = THE_PRJ;
		options.m_bOverwrite = true;
		array<array<float, 252>, 201> last_prcp = { 0 };

		static const size_t NB_WRF_HOURS = 289;//289 hours
		callback.PushTask("Create gribs", NB_WRF_HOURS);

		for (size_t h = 0; h < NB_WRF_HOURS&&msg; h++)
		{
			CTRef UTCRef(2013, JULY, DAY_13, 0);
			UTCRef += int(h);

			callback.PushTask(UTCRef.GetFormatedString("%Y-%m-%d-%H"), 50652 * NB_WRF_LEVEL);

			std::string filePathIn = FormatA("E:\\Travaux\\Bureau\\WRF2013\\WRF\\wrfbud_%03d.txt", h);
			CGDALDatasetEx geotif;




			string filePathOut = FormatA("E:\\Travaux\\Bureau\\WRF2013\\TIF\\WRF_%4d_%02d_%02d_%02d.tif", UTCRef.GetYear(), UTCRef.GetMonth() + 1, UTCRef.GetDay() + 1, UTCRef.GetHour());
			msg += geotif.CreateImage(filePathOut, options);
			//create .inv file
			string filePathInvIn = "E:\\Travaux\\Bureau\\WRF2013\\WRF\\template.inv";
			string filePathInvOut = FormatA("E:\\Travaux\\Bureau\\WRF2013\\TIF\\WRF_%4d_%02d_%02d_%02d.inv", UTCRef.GetYear(), UTCRef.GetMonth() + 1, UTCRef.GetDay() + 1, UTCRef.GetHour());
			CopyOneFile(filePathInvIn, filePathInvOut, false);




			ifStream file;
			msg += file.open(filePathIn);
			if (msg)
			{
				std::string csv_text[NB_WRF_VARS];

				//write header
				for (size_t j = 0; j < NB_WRF_VARS; j++)
				{
					csv_text[j] = "KeyID,Latitude,Longitude,Elevation";
					for (size_t i = 0; i < NB_WRF_LEVEL; i++)
						csv_text[j] += "," + FormatA("sounding%02d", i + 1);

					csv_text[j] += "\n";
				}

				CGeoPointIndex xy;
				vector<array<array<array<float, 252>, 201>, NB_WRF_LEVEL>> data(NB_WRF_VARS);
				for (size_t j = 0; j < data.size(); j++)
					for (size_t s = 0; s < data[j].size(); s++)
						for (size_t y = 0; y < data[j][s].size(); y++)
							for (size_t x = 0; x < data[j][s][y].size(); x++)
								data[j][s][y][x] = -9999;

				int i = NB_WRF_LEVEL + 1;
				while (!file.eof() && msg)
				{
					string line;
					getline(file, line);
					if (!line.empty())
					{
						//if (i % NB_WRF_LEVEL == 0)//+1 for coordinate
						if (i == 38)
						{
							StringVector str(line, " ");
							ASSERT(str.size() == 5);

							for (size_t j = 0; j < NB_WRF_VARS; j++)
								csv_text[j] += str[2] + "," + str[0] + "," + str[1] + "," + str[4];

							CGeoPoint lastPoint(ToDouble(str[1]), ToDouble(str[0]), PRJ_WGS_84);
							msg += lastPoint.Reproject(Geo2LCC);

							ASSERT(geotif.GetExtents().IsInside(lastPoint));
							xy = geotif.GetExtents().CoordToXYPos(lastPoint);


							i = 0;//restart cycle

						}
						else
						{
							//std::stringstream stream(line);
							//fixed field length
							StringVector str(line, " ");
							//StringVector str;
							//str.reserve(NB_WRF_VARS + 1);
							//str.resize(NB_WRF_VARS + 1);
							//static const size_t FIELD_LENGTH[NB_WRF_VARS + 1] = { 2, 8, 9, 9, 9, 9, 9, 9, 9 };
							/*for (size_t j = 0; j < NB_WRF_VARS+1; j++)
							{
							char buffer[10] = { 0 };
							stream.read(buffer, FIELD_LENGTH[j]);
							if (strlen(buffer)>0)
							str.push_back(buffer);
							}
							*/
							if (str.size() == NB_WRF_VARS)
							{
								StringVector tmp(str[3], "-");
								ASSERT(tmp.size() == 2);

								str[3] = tmp[0];
								str.insert(str.begin() + 4, tmp[1]);
							}
							ASSERT(str.size() == NB_WRF_VARS + 1);

							size_t sounding = ((i - 1) % NB_WRF_LEVEL);
							ASSERT(sounding < NB_WRF_LEVEL);
							ASSERT(sounding == ToInt(str[0]) - 1);
							for (size_t j = 0; j < NB_WRF_VARS; j++)
							{
								float v = ToFloat(str[j + 1]);
								if (j == WRF_HGHT)
								{
									//add terrain height when WRF
									v += height[xy.m_y][xy.m_x];
								}
								else if (j == WRF_TAIR)//tmp
								{
									v -= 273.15f;//convert K to ᵒC
								}
								else if (j == WRF_PRCP)
								{
									if (sounding == 0)//modify souding zero let all other the original value
										v = max(0.0, (v - last_prcp[xy.m_y][xy.m_x]) / 3600.0);//convert mm/h to mm/s (but, in 2013 version it's cumulative)
								}

								//v = max(0.0, v / 3600.0 - last_prcp[xy.m_y][xy.m_x]);//convert mm/h to mm/s (but, in 2013 version it cumulative)
								//else if (j == WRF_WVMR)
								//v /= 3600;//convert mixing ratio in relative humidity

								data[j][sounding][xy.m_y][xy.m_x] = v;

								csv_text[j] += "," + str[j + 1];
							}

							if (i == 37)//last record
								for (size_t j = 0; j < NB_WRF_VARS; j++)
									csv_text[j] += "\n";



						}

						i++;//next line
					}//for all line


					msg += callback.StepIt();
				}//

				ASSERT(i == NB_WRF_LEVEL + 1);

				for (size_t j = 0; j < data.size(); j++)
				{
					size_t size = j == WRF_PRCP ? 1 : data[j].size();//save only one layer of precipitation
					for (size_t s = 0; s < size; s++)
					{
						GDALRasterBand* pBand = geotif.GetRasterBand(j * NB_WRF_LEVEL + s);
						for (size_t y = 0; y < data[j][s].size(); y++)
							pBand->RasterIO(GF_Write, 0, (int)y, (int)data[j][s][y].size(), 1, &(data[j][s][y][0]), (int)data[j][s][y].size(), 1, GDT_Float32, 0, 0);
					}

					string path = "E:\\Travaux\\Bureau\\WRF2013\\CSV\\";
					string fileTitle = FormatA("%4d_%02d_%02d_%02d_%s", UTCRef.GetYear(), UTCRef.GetMonth() + 1, UTCRef.GetDay() + 1, UTCRef.GetHour(), VAR_NAME[j]);

					ofStream fileOut;
					msg = fileOut.open(path + fileTitle + ".csv");
					if (msg)
					{
						fileOut.write(csv_text[j]);
						fileOut.close();

						msg = fileOut.open(path + fileTitle + ".vrt");
						if (msg)
						{
							static const char* VRT_FORMAT =
								"<OGRVRTDataSource>\n    <OGRVRTLayer name=\"%s\">\n        <SrcDataSource relativeToVRT=\"1\">%s.csv</SrcDataSource>\n        <GeometryType>wkbPoint</GeometryType>\n        <LayerSRS>WGS84</LayerSRS>\n        <GeometryField encoding = \"PointFromColumns\" x=\"Longitude\" y=\"Latitude\"/>\n    </OGRVRTLayer>\n</OGRVRTDataSource>\n";
							fileOut.write(Format(VRT_FORMAT, fileTitle, fileTitle));
							fileOut.close();

							msg = fileOut.open(path + fileTitle + ".csvt");
							if (msg)
							{
								fileOut.write("Integer,Real,Real,Real,Real,Real,Real,Real,Real,Real,Real,Real,Real,Real,Real,Real,Real,Real,Real,Real,Real,Real,Real,Real,Real,Real,Real,Real,Real,Real,Real,Real,Real,Real,Real,Real,Real,Real,Real,Real");
								fileOut.close();
							}
						}
					}
				}//for all output file


				for (size_t y = 0; y < data[WRF_PRCP][0].size(); y++)
					for (size_t x = 0; x < data[WRF_PRCP][0][y].size(); x++)
						last_prcp[y][x] = data[WRF_PRCP][1][y][x];//take sounding 1 because sounding 0 was modified

			}//if msg




			callback.PopTask();

			msg += callback.StepIt();
		}//for all 193 hours

		callback.PopTask();

		return msg;
	}

	//using namespace netCDF;
	//typedef std::unique_ptr < NcFile > NcFilePtr;
	//typedef std::array<NcFilePtr, NB_VARIABLES> NcFilePtrArray;


	ERMsg CreateGribsFromNetCDF(CCallback& callback)
	{
		enum TWRFLEvels { NB_WRF_LEVEL = 38 };
		enum TWRFVars { WRF_PRES, WRF_HGHT, WRF_TAIR, WRF_UWND, WRF_VWND, WRF_WWND, WRF_RELH, WRF_PRCP, NB_WRF_VARS };


		GDALSetCacheMax64(2000000000);

		ERMsg msg;

		CGDALDatasetEx terrain_height;
		msg += terrain_height.OpenInputImage("E:\\Travaux\\Bureau\\WRF2013\\Terrain Height.tif");

		if (!msg)
			return msg;

		array<array<float, 252>, 201> height = { 0 };
		terrain_height.GetRasterBand(0)->RasterIO(GF_Read, 0, 0, (int)height[0].size(), (int)height.size(), &(height[0][0]), (int)height[0].size(), (int)height.size(), GDT_Float32, 0, 0);




		CBaseOptions options;
		terrain_height.UpdateOption(options);
		options.m_outputType = GDT_Float32;
		options.m_createOptions.push_back("COMPRESS=LZW");
		options.m_dstNodata = -9999;
		options.m_bOverwrite = true;
		options.m_nbBands = NB_WRF_LEVEL * (NB_WRF_VARS - 1) + 1;//prcp have only one band

		enum { WRF_PHB, WRF_PH, WRF_PB, WRF_P, WRF_T, WRF_U, WRF_V, WRF_W, WRF_QVAPOR, WRF_RAINC, WRF_RAINNC, WRF_PSFC, WRF_T2, WRF_U10, WRF_V10, WRF_Q2, NB_VAR_BASE };
		static const char* VAR_NAME[NB_VAR_BASE] = { "PHB", "PH", "PB", "P", "T", "U", "V", "W", "QVAPOR", "RAINC", "RAINNC", "PSFC", "T2", "U10", "V10", "Q2" };


		CTRef begin = CTRef(2013, JULY, DAY_13, 0);
		CTRef end = CTRef(2013, JULY, DAY_25, 0);
		callback.PushTask("Create gribs", (end - begin + 1)*NB_WRF_LEVEL * NB_WRF_VARS);

		array<array<float, 252>, 201> last_RAIN_C = { 0 };
		array<array<float, 252>, 201> last_RAIN_NC = { 0 };

		for (CTRef UTCRef = begin; UTCRef <= end && msg; UTCRef++)
		{
			//create .inv file
			string filePathInvIn = "E:\\Travaux\\Bureau\\WRF2013\\NetCDF\\template.inv";
			string filePathInvOut = FormatA("E:\\Travaux\\Bureau\\WRF2013\\TIF2\\WRF_%4d_%02d_%02d_%02d.inv", UTCRef.GetYear(), UTCRef.GetMonth() + 1, UTCRef.GetDay() + 1, UTCRef.GetHour());
			CopyOneFile(filePathInvIn, filePathInvOut, false);


			CGDALDatasetEx geotifOut;
			string filePathOut = FormatA("E:\\Travaux\\Bureau\\WRF2013\\TIF2\\WRF_%4d_%02d_%02d_%02d.tif", UTCRef.GetYear(), UTCRef.GetMonth() + 1, UTCRef.GetDay() + 1, UTCRef.GetHour());
			msg += geotifOut.CreateImage(filePathOut, options);

			array<CGDALDatasetEx, NB_VAR_BASE> geotifIn;
			for (size_t i = 0; i < geotifIn.size() && msg; i++)
			{
				string filePathIn = WBSF::FormatA("E:\\Travaux\\Bureau\\WRF2013\\NETCDF\\wrfout_d02_%d-%02d-%02d_%02d.nc", UTCRef.GetYear(), UTCRef.GetMonth() + 1, UTCRef.GetDay() + 1, UTCRef.GetHour());
				string filePathTmp = WBSF::FormatA("E:\\Travaux\\Bureau\\WRF2013\\TIF2\\tmp\\%s.tif", VAR_NAME[i]);
				string command = WBSF::FormatA("\"C:\\Program Files\\QGIS\\bin\\GDAL_translate.exe\" -co \"COMPRESS=LZW\" -stats -ot Float32 -a_srs \"+proj=lcc +lat_1=30 +lat_2=60 +lat_0=48.000004 +lon_0=-69 +x_0=0 +y_0=0 +ellps=WGS84 +datum=WGS84\" -a_ullr -524000 402000 488000 -402000 NETCDF:\"%s\":%s \"%s\"", filePathIn.c_str(), VAR_NAME[i], filePathTmp.c_str());

				DWORD exist = 0;
				msg = WinExecWait(command.c_str(), "", SW_HIDE, &exist);

				if (msg)
					msg += geotifIn[i].OpenInputImage(filePathTmp);
			}


			if (msg)
			{

				for (int l = 0; l < NB_WRF_LEVEL && msg; l++)
				{
					array<array<array<float, 252>, 201>, NB_WRF_VARS> data = { 0 };
					for (size_t s = 0; s < NB_WRF_VARS&&msg; s++)
					{
						switch (s)
						{
						case WRF_PRES:
						{
							if (l == 0)
							{
								GDALRasterBand* pBand = NULL;
								pBand = geotifIn[WRF_PSFC].GetRasterBand(0);
								pBand->RasterIO(GF_Read, 0, 0, (int)data[s][0].size(), (int)data[s].size(), &(data[s][0][0]), (int)data[s][0].size(), (int)data[s].size(), GDT_Float32, 0, 0);
							}
							else
							{
								array<array<float, 252>, 201> pb = { 0 };
								array<array<float, 252>, 201> p = { 0 };
								GDALRasterBand* pBand1 = geotifIn[WRF_PB].GetRasterBand(l - 1);
								GDALRasterBand* pBand2 = geotifIn[WRF_P].GetRasterBand(l - 1);
								pBand1->RasterIO(GF_Read, 0, 0, (int)pb[0].size(), (int)pb.size(), &(pb[0][0]), (int)pb[0].size(), (int)pb.size(), GDT_Float32, 0, 0);
								pBand2->RasterIO(GF_Read, 0, 0, (int)p[0].size(), (int)p.size(), &(p[0][0]), (int)p[0].size(), (int)p.size(), GDT_Float32, 0, 0);

								for (size_t y = 0; y < data[s].size(); y++)
									for (size_t x = 0; x < data[s][y].size(); x++)
										data[s][y][x] = (pb[y][x] + p[y][x]);//add base and perturbation
							}

							for (size_t y = 0; y < data[s].size(); y++)
								for (size_t x = 0; x < data[s][y].size(); x++)
									data[s][y][x] /= 100.0f;//convert Pa into mbar

							break;
						}

						case WRF_HGHT:
						{
							if (l == 0)
							{
								array<array<float, 252>, 201> phb = { 0 };
								array<array<float, 252>, 201> ph = { 0 };
								GDALRasterBand* pBand1 = geotifIn[WRF_PHB].GetRasterBand(0);
								GDALRasterBand* pBand2 = geotifIn[WRF_PH].GetRasterBand(0);

								pBand1->RasterIO(GF_Read, 0, 0, (int)phb[0].size(), (int)phb.size(), &(phb[0][0]), (int)phb[0].size(), (int)phb.size(), GDT_Float32, 0, 0);
								pBand2->RasterIO(GF_Read, 0, 0, (int)ph[0].size(), (int)ph.size(), &(ph[0][0]), (int)ph[0].size(), (int)ph.size(), GDT_Float32, 0, 0);


								for (size_t y = 0; y < data[s].size(); y++)
									for (size_t x = 0; x < data[s][y].size(); x++)
										data[s][y][x] = (phb[y][x] + ph[y][x]) / 9.8;//convert m²/s² into m
							}
							else
							{
								array<array<float, 252>, 201> phb1 = { 0 };
								array<array<float, 252>, 201> ph1 = { 0 };
								array<array<float, 252>, 201> phb2 = { 0 };
								array<array<float, 252>, 201> ph2 = { 0 };
								GDALRasterBand* pBand1 = geotifIn[WRF_PHB].GetRasterBand(l - 1);
								GDALRasterBand* pBand2 = geotifIn[WRF_PH].GetRasterBand(l - 1);
								GDALRasterBand* pBand3 = geotifIn[WRF_PHB].GetRasterBand(l);
								GDALRasterBand* pBand4 = geotifIn[WRF_PH].GetRasterBand(l);
								pBand1->RasterIO(GF_Read, 0, 0, (int)phb1[0].size(), (int)phb1.size(), &(phb1[0][0]), (int)phb1[0].size(), (int)phb1.size(), GDT_Float32, 0, 0);
								pBand2->RasterIO(GF_Read, 0, 0, (int)ph1[0].size(), (int)ph1.size(), &(ph1[0][0]), (int)ph1[0].size(), (int)ph1.size(), GDT_Float32, 0, 0);
								pBand3->RasterIO(GF_Read, 0, 0, (int)phb2[0].size(), (int)phb2.size(), &(phb2[0][0]), (int)phb2[0].size(), (int)phb2.size(), GDT_Float32, 0, 0);
								pBand4->RasterIO(GF_Read, 0, 0, (int)ph2[0].size(), (int)ph2.size(), &(ph2[0][0]), (int)ph2[0].size(), (int)ph2.size(), GDT_Float32, 0, 0);


								for (size_t y = 0; y < data[s].size(); y++)
									for (size_t x = 0; x < data[s][y].size(); x++)
										data[s][y][x] = ((phb1[y][x] + phb2[y][x]) / 2 + (ph1[y][x] + ph2[y][x]) / 2) / 9.8;//convert m²/s² into m
							}

							break;
						}
						case WRF_TAIR:
						{
							if (l == 0)
							{
								GDALRasterBand* pBand = geotifIn[WRF_T2].GetRasterBand(0);
								pBand->RasterIO(GF_Read, 0, 0, (int)data[s][0].size(), (int)data[s].size(), &(data[s][0][0]), (int)data[s][0].size(), (int)data[s].size(), GDT_Float32, 0, 0);
							}
							else
							{
								array<array<float, 252>, 201> T = { 0 };
								GDALRasterBand* pBand = geotifIn[WRF_T].GetRasterBand(l - 1);
								pBand->RasterIO(GF_Read, 0, 0, (int)T[0].size(), (int)T.size(), &(T[0][0]), (int)T[0].size(), (int)T.size(), GDT_Float32, 0, 0);
								for (size_t y = 0; y < data[s].size(); y++)
									for (size_t x = 0; x < data[s][y].size(); x++)
										data[s][y][x] = (T[y][x] + 300)*pow(data[WRF_PRES][y][x] / 1000, 0.2854);//convert into Kelvin
							}

							//convert Kelvin to ᵒC
							for (size_t y = 0; y < data[s].size(); y++)
								for (size_t x = 0; x < data[s][y].size(); x++)
									data[s][y][x] -= 273.15f;

							break;
						}

						case WRF_UWND:
						{

							if (l == 0)
							{
								GDALRasterBand* pBand = geotifIn[WRF_U10].GetRasterBand(0);
								pBand->RasterIO(GF_Read, 0, 0, (int)data[s][0].size(), (int)data[s].size(), &(data[s][0][0]), (int)data[s][0].size(), (int)data[s].size(), GDT_Float32, 0, 0);
							}
							else
							{
								array<array<float, 253>, 201> U = { 0 };
								GDALRasterBand* pBand = geotifIn[WRF_U].GetRasterBand(l - 1);
								pBand->RasterIO(GF_Read, 0, 0, (int)U[0].size(), (int)U.size(), &(U[0][0]), (int)U[0].size(), (int)U.size(), GDT_Float32, 0, 0);

								for (size_t y = 0; y < data[s].size(); y++)
									for (size_t x = 0; x < data[s][y].size(); x++)
										data[s][y][x] = (U[y][x] + U[y][x + 1]) / 2;

							}

							break;
						}
						case WRF_VWND:
						{
							if (l == 0)
							{
								GDALRasterBand* pBand = geotifIn[WRF_V10].GetRasterBand(0);
								pBand->RasterIO(GF_Read, 0, 0, (int)data[s][0].size(), (int)data[s].size(), &(data[s][0][0]), (int)data[s][0].size(), (int)data[s].size(), GDT_Float32, 0, 0);
							}
							else
							{
								array<array<float, 252>, 202> V = { 0 };
								GDALRasterBand* pBand = geotifIn[WRF_V].GetRasterBand(l - 1);
								pBand->RasterIO(GF_Read, 0, 0, (int)V[0].size(), (int)V.size(), &(V[0][0]), (int)V[0].size(), (int)V.size(), GDT_Float32, 0, 0);

								for (size_t y = 0; y < data[s].size(); y++)
									for (size_t x = 0; x < data[s][y].size(); x++)
										data[s][y][x] = (V[y][x] + V[y + 1][x]) / 2;
							}

							break;
						}
						case WRF_WWND:
						{
							if (l == 0)
							{
								GDALRasterBand* pBand = geotifIn[WRF_W].GetRasterBand(0);
								pBand->RasterIO(GF_Read, 0, 0, (int)data[s][0].size(), (int)data[s].size(), &(data[s][0][0]), (int)data[s][0].size(), (int)data[s].size(), GDT_Float32, 0, 0);
							}
							else
							{
								array<array<float, 252>, 201> W1 = { 0 };
								array<array<float, 252>, 201> W2 = { 0 };
								GDALRasterBand* pBand1 = geotifIn[WRF_W].GetRasterBand(l - 1);
								GDALRasterBand* pBand2 = geotifIn[WRF_W].GetRasterBand(l);
								pBand1->RasterIO(GF_Read, 0, 0, (int)W1[0].size(), (int)W1.size(), &(W1[0][0]), (int)W1[0].size(), (int)W1.size(), GDT_Float32, 0, 0);
								pBand2->RasterIO(GF_Read, 0, 0, (int)W2[0].size(), (int)W2.size(), &(W2[0][0]), (int)W2[0].size(), (int)W2.size(), GDT_Float32, 0, 0);

								for (size_t y = 0; y < data[s].size(); y++)
									for (size_t x = 0; x < data[s][y].size(); x++)
										data[s][y][x] = (W1[y][x] + W2[y][x]) / 2;
							}

							break;
						}

						case WRF_RELH:
						{
							GDALRasterBand* pBand = geotifIn[l == 0 ? WRF_Q2 : WRF_QVAPOR].GetRasterBand(l == 0 ? 0 : l - 1);

							array<array<float, 252>, 201> W = { 0 };
							pBand->RasterIO(GF_Read, 0, 0, (int)W[0].size(), (int)W.size(), &(W[0][0]), (int)W[0].size(), (int)W.size(), GDT_Float32, 0, 0);

							for (size_t y = 0; y < data[s].size(); y++)
							{
								for (size_t x = 0; x < data[s][y].size(); x++)
								{
									double T = data[WRF_TAIR][y][x];//ᵒC
									double p = data[WRF_PRES][y][x];//hPa
									double es = 6.108 * exp(17.27*T / (T + 237.3));//hPa
									double ws = 0.62197*(es / (p - es));
									double Hr = max(1.0, min(100.0, (W[y][x] / ws) * 100));
									data[s][y][x] = Hr;
								}
							}

							break;
						}
						case WRF_PRCP:
						{

							GDALRasterBand* pBand1 = NULL;
							GDALRasterBand* pBand2 = NULL;
							if (l == 0)
							{
								array<array<float, 252>, 201> RAIN_C = { 0 };
								array<array<float, 252>, 201> RAIN_NC = { 0 };

								pBand1 = geotifIn[WRF_RAINC].GetRasterBand(0);
								pBand2 = geotifIn[WRF_RAINNC].GetRasterBand(0);
								pBand1->RasterIO(GF_Read, 0, 0, (int)RAIN_C[0].size(), (int)RAIN_C.size(), &(RAIN_C[0][0]), (int)RAIN_C[0].size(), (int)RAIN_C.size(), GDT_Float32, 0, 0);
								pBand2->RasterIO(GF_Read, 0, 0, (int)RAIN_NC[0].size(), (int)RAIN_NC.size(), &(RAIN_NC[0][0]), (int)RAIN_NC[0].size(), (int)RAIN_NC.size(), GDT_Float32, 0, 0);

								for (size_t y = 0; y < data[s].size(); y++)
								{
									for (size_t x = 0; x < data[s][y].size(); x++)
									{
										//precipitation is cumulative over simulation hours. 
										double prcp = (RAIN_NC[y][x] - last_RAIN_NC[y][x]) + (RAIN_C[y][x] - last_RAIN_C[y][x]);
										//if (prcp < 0.05)
										//prcp = 0;

										data[s][y][x] = prcp / 3600;//Convert from mm to mm/s
										last_RAIN_NC[y][x] = RAIN_NC[y][x];
										last_RAIN_C[y][x] = RAIN_C[y][x];
									}
								}
							}

							break;
						}
						}//switch

						if (l == 0 || s != WRF_PRCP)//save precipitation only once at surface
						{
							//size_t b = l == 0 ? s : (l - 1) * (NB_WRF_VARS - 1) + NB_WRF_VARS + s;
							size_t b = s * NB_WRF_LEVEL + l;
							GDALRasterBand* pBand = geotifOut.GetRasterBand(b);
							pBand->RasterIO(GF_Write, 0, 0, (int)data[s][0].size(), (int)data[s].size(), &(data[s][0][0]), (int)data[s][0].size(), (int)data[s].size(), GDT_Float32, 0, 0);
						}

						msg += callback.StepIt();
					}//variables
				}//level


				geotifOut.Close();

				for (size_t i = 0; i < geotifIn.size(); i++)
					geotifIn[i].Close();
			}//if msg

		}//for all hours


		return msg;
	}


}







