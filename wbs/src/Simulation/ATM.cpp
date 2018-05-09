3//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
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


//NOT_CREATED			0
//IDLE_BEGIN			1
//LIFTOFF				2
//FLIGHT				3
//LANDING				4
//IDLE_END				5
//DESTROYED				6

//END_BY_PRCP			10
//END_BY_TAIR			11
//END_BY_SUNRISE		12
//OUTSIDE_MAP			13
//OUTSIDE_TIME_WINDOW	14

//NO_LIFTOFF_OUTSIDE_FLIGHT_PERIOD	20
//NO_LIFTOFF_NO_DEFOLIATION			21
//NO_LIFTOFF_OVER_WATER				22
//NO_LIFTOFF_MISS_ENERGY			23
//NO_LIFTOFF_TAIR					24
//NO_LIFTOFF_PRCP					25
//NO_LIFTOFF_WNDS					26




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
		CGeoPoint3D pt2 = pt;
		pt2.Reproject(ToWeather);//project in the weather projection. Problem can append when we mixte projection from differnet weather type
		pt2 += d;
		pt2.Reproject(FromWeather);//reproject in geographic


		_ASSERTE(!_isnan(pt2.m_x));
		_ASSERTE(!_isnan(pt2.m_y));


		return pt2;


		//double λ = Rad2Deg(d.m_x / 6371000);
		//double φ = Rad2Deg(d.m_y / 6371000);
		//return CGeoPoint3D(pt.m_x + λ, pt.m_y + φ, pt.m_z + d.m_z, pt.GetPrjID());
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



	extern const char ATM_HEADER[] = "FLIGH|SCALE|Sex|A|M|G|EGGS_LAID|State|X|Y|Latitude|Longitude|T|P|U|V|W|HEIGHT|DELTA_HEIGHT|CURRENT_HEIGHT|W_HORIZONTAL|W_VERTICAL|DIRECTION|DISTANCE|DISTANCE_FROM_OIRIGINE|LIFTOFF_TIME|LANDING_TIME|DEFOLIATION";

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


	const char* CSBWMothParameters::MEMBERS_NAME[NB_ATM_MEMBERS] = { "Pmax", "Wmin", "WingBeatScale", "HorzontalScale", "Whorzontal", "WhorzontalSD", "Wdescent", "WdescentSD","FlightTimeAfterSunrize","MaximumFlights", "ReadyToFlyMaleShift", "ReadyToFlyFemaleShift" };
	const char* CATMWorldParamters::MEMBERS_NAME[NB_MEMBERS] = { "WeatherType", "SimulationPeriod", "FlightPeriod", "TimeStep", "Seed", "UseSpaceInterpol", "UseTimeInterpol", "UsePredictorCorrectorMethod", "UseVerticalVelocity", "MaximumFlyers", "MaxMissHours", "ForceFirstFlight", "BroodTSource", "PSource", "DEM", "WaterLayer", "Gribs", "HourlyDB", "Defoliation", "OutputSubHourly", "OutputFileTitle", "OutputFrequency", "CreateEggsMap", "EggsMapTitle", "EggsMapRes", "WindStabilityType", "NbWeatherStations" };




	//***********************************************************************************************

	CSBWMoth::CSBWMoth(CATMWorld& world) :
		m_world(world)
	{
		m_sex = -1;
		m_A = 0;
		m_M = 0;
		m_G = 0;
		m_Fᵒ = 0;
		m_Δv = 0;
		m_broods = VMISS;
		m_eggsLeft = 0;
		m_loc = 0;
		m_par = 0;
		m_rep = 0;
		m_flightNo = 0;
		m_scale = 0;

		//m_liftoffOffset = 0;
		m_w_descent = 0;
		m_liffoff_time = 0;
		m_duration = 0;
		m_p_exodus = 0;


		m_state = NOT_CREATED;
		m_end_type = NO_END_DEFINE;
		m_log.fill(0);
		m_UTCShift = 0;
	}


	bool CSBWMoth::init(CTRef TRef)
	{
		ASSERT(m_readyToFly <= TRef);
		ASSERT(TRef.GetTM() == CTM::DAILY);

		m_UTCShift = CTimeZones::GetTimeZone(m_location);


		m_state = NOT_CREATED;
		m_end_type = NO_END_DEFINE;
		m_log.fill(0);
		m_liffoff_time = 0;

		m_pt.m_alt = 10;
		m_w_horizontal = m_world.get_w_horizontal();
		m_w_descent = m_world.get_w_descent();
		m_p_exodus = m_world.random().Randu();
		m_duration = 0;
		m_noLiftoff.fill(0);
		m_broods = VMISS;



		__int64 UTCTimeº = CTimeZones::TRef2Time(TRef) - m_UTCShift;
		__int64 UTCsunset = UTCTimeº + m_world.get_sunset(TRef, m_location);


		bool bForceFirst = m_flightNo == 0 && m_world.m_world_param.m_bForceFirstFlight;
		bool bOverWater = m_world.is_over_water(m_newLocation);
		if (bForceFirst || !bOverWater)
		{

			//If female : brood eggs first
			if (m_sex == CSBWMothParameters::FEMALE)
			{
				double T = -999;
				switch (m_world.m_world_param.m_broodTSource)
				{
				case CATMWorldParamters::BROOD_T_17: T = 17; break;
				case CATMWorldParamters::BROOD_AT_SUNSET:
				{
					//The median hour after sunset that give the nearest temperature is 40 minutes after sunset
					__int64 UTCTmean = UTCsunset + 40 * 60;
					//Get nearest grid of this time
					UTCTmean = m_world.m_weather.GetNearestFloorTime(UTCTmean);
					T = m_world.m_weather.get_air_temperature(m_pt, UTCTmean);
					//CATMVariables w = get_weather(UTCTmean);
					//T = w[ATM_TAIR];
					break;
				}
				
				}

				Brood(T);
			}


			//so if it's the first flight, we do it anyway
			bool bOverDefol = m_world.is_over_defoliation(m_newLocation);
			if (bForceFirst || bOverDefol)
			{
				if (m_flightNo < m_world.m_moths_param.m_maxFlights)
				{
					if (m_world.m_world_param.m_flightPeriod.IsInside(TRef))
					{
						if (GetLiftoff(UTCsunset, m_liffoff_time))
						{
							//compute surise of the next day
							__int64 UTCTime¹ = UTCTimeº + 24 * 3600;
							__int64 sunriseTime = UTCTime¹ + CATMWorld::get_sunrise(TRef + 1, m_location); //sunrise of the next day

							//compute duration (max) from liftoff and sunrise
							m_duration = sunriseTime - m_liffoff_time + m_world.m_moths_param.m_flight_after_sunrise;
							ASSERT(m_duration >= 0 && m_duration < 24 * 3600);
						}
						else
						{
							m_end_type = GetNoLiftoffCode(m_noLiftoff);
						}
					}
					else
					{
						m_end_type = NO_LIFTOFF_NO_DEFOLIATION;
					}
				}
				else
				{
					m_end_type = NO_LIFTOFF_MISS_ENERGY;
				}

			}
			else
			{
				m_end_type = NO_LIFTOFF_OUTSIDE_FLIGHT_PERIOD;
			}
		}
		else
		{
			m_end_type = NO_LIFTOFF_OVER_WATER;
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
				m_stat[i][S_PRCP] = w[ATM_PRCP];
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
				m_stat[i][S_PRCP] = w[ATM_PRCP];
				m_stat[i][S_U] += w[ATM_WNDU];
				m_stat[i][S_V] += w[ATM_WNDV];
				m_stat[i][S_W] += w[ATM_WNDW];
			}
		}
	}



	void CSBWMoth::live(__int64 UTCCurrentTime)
	{
		if (m_liffoff_time == 0)
		{
			ASSERT(m_end_type != NO_END_DEFINE);
			m_state = IDLE_END;
		}
		else
		{
			__int64 weather_time = m_world.m_weather.GetNearestFloorTime(UTCCurrentTime);


			if (!m_world.m_weather.IsLoaded(weather_time))
				m_state = IDLE_END, m_end_type = OUTSIDE_TIME_WINDOW;
			else if (m_end_type == NO_END_DEFINE && !m_world.IsInside(m_pt))
				m_state = IDLE_END, m_end_type = OUTSIDE_MAP;


			switch (m_state)
			{
			case NOT_CREATED:		create(UTCCurrentTime); break;
			case IDLE_BEGIN:		idle_begin(UTCCurrentTime); break;
			case LIFTOFF:			liftoff(UTCCurrentTime); break;
			case FLIGHT:			flight(UTCCurrentTime); break;
			case LANDING:			landing(UTCCurrentTime); break;
			case IDLE_END:			idle_end(UTCCurrentTime);  break;
			default: assert(false);
			}
		}

	}

	void CSBWMoth::create(__int64 UTCCurrentTime)
	{
		__int64 countdown = UTCCurrentTime - m_liffoff_time;
		if (countdown >= 0)
		{
			m_state = IDLE_BEGIN;
			m_log[T_CREATION] = UTCCurrentTime;

			__int64 UTCWeatherTime = m_world.m_weather.GetNearestFloorTime(UTCCurrentTime);
			AddStat(m_world.get_weather(m_pt, UTCWeatherTime));
		}
	}

	void CSBWMoth::idle_begin(__int64 UTCCurrentTime)
	{
		__int64 UTCWeatherTime = m_world.m_weather.GetNearestFloorTime(UTCCurrentTime);
		CATMVariables w = m_world.get_weather(m_pt, UTCWeatherTime);
		AddStat(w);

		double Wmin = m_world.m_moths_param.m_Wmin * 1000 / 3600; //km/h -> m/s 

		double Tᴸ = get_Tᴸ();
		double ws = w.get_wind_speed();
		double Pmax = m_world.GetPmax();

		ASSERT(!IsMissing(w[ATM_TAIR]) && !IsMissing(w[ATM_PRCP]) && !IsMissing(w[ATM_WNDU]) && !IsMissing(w[ATM_WNDV]));

		if (w[ATM_PRCP] < Pmax && w[ATM_TAIR] >= Tᴸ && ws >= Wmin)
		{
			m_state = LIFTOFF;
		}
		//else
		//{
		//	__int64 duration = UTCCurrentTime - m_liffoff_time;
		//	if (duration > 2 * 3600)
		//	{
		//		//flight abort
		//		if (w[ATM_PRCP] > Pmax)
		//			m_end_type = NO_LIFTOFF_PRCP;
		//		else if (w[ATM_TAIR] < Tᴸ)
		//			m_end_type = NO_LIFTOFF_TAIR;
		//		else if (ws < Wmin)
		//			m_end_type = NO_LIFTOFF_WNDS;

		//		m_state = IDLE_END;
		//	}
		//}

	}


	void CSBWMoth::liftoff(__int64 UTCCurrentTime)
	{
		m_flightNo++;//a new flight for this moth
		m_log[T_LIFTOFF] = UTCCurrentTime;
		m_state = FLIGHT;

		__int64 UTCWeatherTime = m_world.m_weather.GetNearestFloorTime(UTCCurrentTime);
		CATMVariables w = m_world.get_weather(m_pt, UTCWeatherTime);
		AddStat(w);
	}


	void CSBWMoth::flight(__int64 UTCCurrentTime)
	{
		__int64 UTCWeatherTime = m_world.m_weather.GetNearestFloorTime(UTCCurrentTime);
		ASSERT(m_world.m_weather.IsLoaded(UTCWeatherTime));
		ASSERT(m_world.IsInside(m_pt));
		ASSERT(m_state == FLIGHT);
		ASSERT(m_end_type == NO_END_DEFINE);


		CATMVariables w = get_weather(UTCWeatherTime);
		if (w.is_init())
		{
			double Pmax = m_world.GetPmax();
			if (w[ATM_PRCP] < Pmax)
			{
				__int64 duration = UTCCurrentTime - m_liffoff_time;
				if (m_Δv == 0 && m_pt.m_z > 60)//m_Δv is apply atfer lift-off when the moth reach an altitude of 60 meters
					m_Δv = m_world.m_moths_param.m_Δv;

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


				if (duration < m_duration)
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
						//avoid to get negativ elevation
						m_newLocation.m_z -= m_pt.m_z;
						m_pt.m_z -= m_pt.m_z;
						m_state = LANDING;
						m_end_type = END_BY_TAIR;
					}

					AddStat(w, U, d);
				}
				else
				{
					m_state = LANDING;
					m_end_type = END_BY_SUNRISE;
				}
			}
			else
			{
				m_state = LANDING;
				m_end_type = END_BY_PRCP;
			}
		}
		else
		{
			m_state = IDLE_END;
			m_end_type = OUTSIDE_MAP;
		}

	}

	void CSBWMoth::landing(__int64 UTCCurrentTime)
	{
		__int64 UTCWeatherTime = m_world.m_weather.GetNearestFloorTime(UTCCurrentTime);
		ASSERT(m_world.m_weather.IsLoaded(UTCWeatherTime));
		ASSERT(m_world.IsInside(m_pt));
		ASSERT(m_end_type != NO_END_DEFINE);
		ASSERT(m_state == LANDING);

		double dt = m_world.get_time_step(); //[s]
		CATMVariables w = get_weather(UTCWeatherTime);
		if (w.is_init())
		{
			CGeoDistance3D U = get_U(w, UTCWeatherTime);
			CGeoDistance3D d = U * dt;

			const CProjectionTransformation& toWea = m_world.GetToWeatherTransfo(UTCWeatherTime);
			const CProjectionTransformation& fromWea = m_world.GetFromWeatherTransfo(UTCWeatherTime);
			((CGeoPoint3D&)m_pt) = UpdateCoordinate(m_pt, d, toWea, fromWea);
			((CGeoPoint3D&)m_newLocation) = UpdateCoordinate(m_newLocation, d, toWea, fromWea);

			if (m_pt.m_z <= 5)//let moth landing correcly
			{
				//it's the end
				m_log[T_LANDING] = UTCCurrentTime;
				m_state = IDLE_END;

				//end at zero
				m_newLocation.m_z -= m_pt.m_z;
				m_pt.m_z -= m_pt.m_z;
			}

			ASSERT(m_pt.m_z >= 0);
			AddStat(w, U, d);
		}
		else
		{
			m_state = IDLE_END;
			m_end_type = OUTSIDE_MAP;
		}
	}

	void CSBWMoth::idle_end(__int64 UTCCurrentTime)
	{
		ASSERT(m_end_type != NO_END_DEFINE);

		if (m_log[T_IDLE_END] == 0)
		{
			m_log[T_IDLE_END] = UTCCurrentTime;

			__int64 UTCWeatherTime = m_world.m_weather.GetNearestFloorTime(UTCCurrentTime);
			AddStat(m_world.get_weather(m_pt, UTCWeatherTime));
		}
	}



	void CSBWMoth::DestroyByOptimisation()
	{
		m_state = DESTROYED_BY_OPTIMIZATION;
	}

	CGeoDistance3D CSBWMoth::get_U(const CATMVariables& w, __int64 UTCWeatherTime)const
	{
		ASSERT(!IsMissing(w[ATM_WNDV]) && !IsMissing(w[ATM_WNDU]));

		double alpha = 0;
		if (w[ATM_WNDV] != 0 || w[ATM_WNDU] != 0)
			alpha = atan2(w[ATM_WNDV], w[ATM_WNDU]);

		if (_isnan(alpha) || !_finite(alpha))
			alpha = 0;



		//double w_horizontal = m_Δv*m_w_horizontal;
		double Ux = (w[ATM_WNDU] + cos(alpha)*m_w_horizontal);	//[m/s]
		double Uy = (w[ATM_WNDV] + sin(alpha)*m_w_horizontal);	//[m/s]
		double Uz = 0;


		switch (m_state)
		{
		case FLIGHT:	Uz = w[ATM_WNDW] + get_Uz(UTCWeatherTime, w); break;	//[m/s]; 
		case LANDING:	Uz = w[ATM_WNDW] + m_w_descent; break;	//[m/s]
		default: assert(false);
		}


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

	size_t CSBWMoth::GetNoLiftoffCode(const std::array<size_t, 3>& noLiftoff)
	{
		std::array<size_t, 3>::const_iterator it = std::max_element(noLiftoff.begin(), noLiftoff.end());

		size_t index = (size_t)std::distance(noLiftoff.begin(), it);
		ASSERT(index < 3);

		return NO_LIFTOFF_TAIR + index;
	}


	//const double CSBWMoth::b[2] = { 21.35, 24.08 };
	//const double CSBWMoth::c[2] = { 2.97, 6.63 };
	//const double CSBWMoth::b[2] = { 21.35, 21.35 };
	//const double CSBWMoth::c[2] = { 2.97, 2.97 };
	const double CSBWMoth::Vmax = 65.0;
	const double CSBWMoth::K = 166;
	const double CSBWMoth::b[2] = { 21.35, 24.08 };
	const double CSBWMoth::c[2] = { 2.97, 6.63 };
	const double CSBWMoth::deltaT[2] = { 0, 3.5 };



	//T : air temperature [ᵒC]
	//out: forewing frequency [Hz] for this temperature
	double CSBWMoth::get_Vᵀ(double T)const
	{
		return get_Vᵀ(m_sex, T);
	}

	//sex : male=0, female=1
	//Vmax: maximum wingbeat [Hz]
	//T : air temperature [ᵒC]
	//out: forewing frequency [Hz] 
	double CSBWMoth::get_Vᵀ(size_t sex, double T)
	{
		double Vᵀ = 0;
		if (T > 0)
			Vᵀ = Vmax * (1 - exp(-pow((T + deltaT[sex]) / b[sex], c[sex])));

		return Vᵀ;
	}

	//out : liftoff temperature [ᵒC] for this insect
	double CSBWMoth::get_Tᴸ()const
	{
		return get_Tᴸ(m_sex, m_A, m_M, m_Δv);
	}


	double CSBWMoth::get_Vᴸ(double A, double M, double Δv)
	{
		return K * sqrt(M) / ((1 - Δv)*A);
	}
	//K : constant
	//Vmax: maximum wingbeat [Hz]
	//A : forewing surface area [cm²]
	//M : dry weight [g]
	//out : liftoff temperature [ᵒC] 
	double CSBWMoth::get_Tᴸ(size_t sex, double A, double M, double Δv)
	{
		double Vᴸ = get_Vᴸ(A, M, Δv);
		double Tᴸ = (Vᴸ < Vmax) ? b[sex] * pow(-log(1 - Vᴸ / Vmax), 1.0 / c[sex]) - deltaT[sex] : 40;

		ASSERT(!isnan(Tᴸ));

		return Tᴸ;
	}

	double CSBWMoth::get_Uz(__int64 UTCWeatherTime, const CATMVariables& w)const
	{
		ASSERT(m_state == FLIGHT);

		double Uz = 0;
		double Vᵀ = get_Vᵀ(w[ATM_TAIR]);
		double Vᴸ = get_Vᴸ(m_A, m_M, m_Δv);

		Uz = m_world.m_moths_param.m_w_α*(Vᵀ - Vᴸ) * 1000 / 3600;//Uz can be negative


		return Uz;//m/s
	}

	CATMVariables CSBWMoth::get_weather(__int64 UTCWeatherTime)const
	{
		ASSERT(m_world.m_weather.IsLoaded(UTCWeatherTime));

		CATMVariables w;

		CGeoPoint3D ptᵒ = m_pt;
		CATMVariables wᵒ = m_world.get_weather(ptᵒ, UTCWeatherTime);
		if (wᵒ.is_init())
		{

			CGeoDistance3D Uᵒ = get_U(wᵒ, UTCWeatherTime);
			double dt = m_world.get_time_step(); //[s]

			const CProjectionTransformation& toWea = m_world.GetToWeatherTransfo(UTCWeatherTime);
			const CProjectionTransformation& fromWea = m_world.GetFromWeatherTransfo(UTCWeatherTime);
			CGeoPoint3D pt¹ = UpdateCoordinate(m_pt, Uᵒ*dt, toWea, fromWea);


			if (m_world.m_world_param.m_bUsePredictorCorrectorMethod &&
				m_world.m_weather.IsLoaded(UTCWeatherTime + int(dt / 3600)) &&
				m_world.IsInside(pt¹) &&
				pt¹.m_z > 0)
			{
				CATMVariables w¹ = m_world.get_weather(pt¹, UTCWeatherTime + dt);
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

		static const double END_G = 0.15;

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


		m_broods = m_eggsLeft * P;
		m_eggsLeft = max(0.0, m_eggsLeft - m_broods);

		m_G = max(0.0, min(1.0, m_eggsLeft / m_Fᵒ));


		//**************************************************************
		//compute new M
		static const double A = -6.465;
		static const double B = 1.326;
		static const double C = 2.140;
		static const double D = 1.305;


		m_M = exp(A + B * m_G + C * m_A + D * m_G * m_A);

	}

	bool CSBWMoth::ComputeExodus(double T, double P, double W, double tau)
	{
		static const double C = 1.0 - 2.0 / 3.0 + 1.0 / 5.0;

		bool bExodus = false;

		double Pmax = m_world.GetPmax();
		double Wmin = m_world.m_moths_param.m_Wmin * 1000 / 3600; //km/h -> m/s 
		double Vᴸ = get_Vᴸ(m_A, m_M, m_Δv);
		double Vᵀ = get_Vᵀ(T);

		//how to delay male???
		//GetStageAge() > EXODUS_AGE[m_sex] &&
		ASSERT(m_sex == CSBWMothParameters::MALE || m_broods > 0);//female have already broods
		if (Vᵀ > Vᴸ && P < Pmax && W >= Wmin)//No lift-off if hourly precipitation greater than 2.5 mm
		{
			double p = (C + tau - 2 * pow(tau, 3) / 3 + pow(tau, 5) / 5) / (2 * C);

			//potential wingbeat is greather than liftoff wingbeat, then exodus 
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


	bool CSBWMoth::GetLiftoff(__int64 sunset, __int64& liftoff)
	{
		//if ((*it)->GetSex() == FEMALE)
		//	(*it)->Brood(dayº);

		bool bExodus = false;
		liftoff = 0;

		//	const CWeatherDay& dayº = (const CWeatherDay&)*w.GetParent();


		__int64 tº = 0;
		__int64 tᴹ = 0;

		if (get_t(sunset, tº, tᴹ))
		{
			//calculate tᶜ
			__int64 tᶜ = (tº + tᴹ) / 2;

			//now compute tau, p and flight

			static const __int64 Δt = 60;
			for (__int64 t = tº; t <= tᴹ && !bExodus; t += Δt)
			{
				double tau = double(t - tᶜ) / (tᴹ - tᶜ);

				CATMVariables w = m_world.get_weather(m_pt, t);
				bExodus = ComputeExodus(w[ATM_TAIR], w[ATM_PRCP], w.get_wind_speed(), tau);
				if (bExodus)//if exodus occurd, set liftoff
					liftoff = t;

				AddStat(w);
			}//for t in exodus period
		}//if exodus occur

		return bExodus;
	}

	bool CSBWMoth::get_t(__int64 sunset, __int64 &tº, __int64 &tᴹ)const
	{
		static const __int64 Δtᶠ = 3 * 3600;
		static const __int64 Δtᶳ = -3600;
		static const __int64 Δt = 60;
		static const double Tº = 24.5;

		__int64 h4 = 4 * 3600;
		__int64 Δtᵀ = h4;
		tº = sunset - h4;//subtract 4 hours
		tᴹ = sunset + h4;//add 4 hours

		for (__int64 t = tº; t <= tᴹ && Δtᵀ == h4; t += Δt)
		{
			CATMVariables weather = m_world.get_weather(m_pt, t);
			if (weather[ATM_TAIR] <= Tº)
				Δtᵀ = t - sunset;
		}

		if (Δtᵀ < h4)//if the Δtᵀ is greater than 4, no temperature under Tº, then no exodus. probably rare situation
		{
			//now calculate the real tº, tᶬ and tᶜ
			tº = sunset + max((Δtᶳ - Δtᶠ / 2), Δtᵀ);
			tᴹ = min(sunset + h4, tº + Δtᶠ);
		}

		return Δtᵀ < h4;
	}
	//**************************************************************************************************************
	//CATMWeatherCuboid

	CATMVariables CATMWeatherCuboid::get_weather(const CGeoPoint3D& pt, bool bSpaceInterpol)const
	{
		static const double POWER = 1;

		CATMVariables w;

		const CATMWeatherCuboid& me = *this;
		array<CStatistic, NB_ATM_VARIABLES> sumV;
		array<CStatistic, NB_ATM_VARIABLES> sumP;
		array<array<array<array<double, NB_ATM_VARIABLES>, 2>, 2>, 2> weight;

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
		ASSERT(at(1).m_time - at(0).m_time == 0 || at(1).m_time - at(0).m_time == 3600);

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

	CATMVariables CATMWeather::get_weather(const CGeoPoint3D& pt, __int64 UTCCurrentTime)const
	{
		ASSERT(pt.IsGeographic());

		__int64 UTCWeatherTime = m_world.m_weather.GetNearestFloorTime(UTCCurrentTime);

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


		w2[ATM_WNDW] = w1[ATM_WNDW];	//replace station W by Gribs W
		w1[ATM_WATER] = w2[ATM_WATER];	//replace Gribs Tw by station Tw 

		CATMVariables weather;
		switch (m_world.m_world_param.m_weather_type)
		{
		case CATMWorldParamters::FROM_GRIBS: weather = w1; break;
		case CATMWorldParamters::FROM_STATIONS:weather = w2; break;
		case CATMWorldParamters::FROM_BOTH:weather = (w1 + w2) / 2; break;
		default: ASSERT(false);
		}

		//if (weather[ATM_PRCP] > -999 && m_world.m_moths_param.m_PSource == CSBWMothParameters::DONT_USE_PRCP)
			//weather[ATM_PRCP] = 0;

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

		return GetAbsolutePath(GetPath(m_filePathGribs), it->second);
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
					//CTRef TRef;
					//TRef.FromFormatedString((*loop)[0], "", "-", 1);
					//assert(TRef.IsValid());

					StringVector tmp((*loop)[0], "/- :");
					ASSERT(tmp.size() == 5 || tmp.size() == 6);
					if (tmp.size() >= 5)
					{
						tm timeinfo = { 0 };
						if (tmp.size() == 6)
							timeinfo.tm_sec = ToInt(tmp[5]);     // seconds after the minute - [0,59] 

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
			for (std::map<CTRef, std::array<CIWD, NB_ATM_VARIABLES>>::iterator it = m_iwd.begin(); it != m_iwd.end();)
			{
				if (UTCTRef - it->first > 24)
				{
					it = m_iwd.erase(it);
				}
				else
				{
					it++;
				}
			}

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
			//CTRef UTCbegin = m_world.m_world_param.m_simulationPeriod.Begin();
			
			//UTCbegin.Transform(CTM(CTM::HOURLY));
			//first = CTimeZones::TRef2Time(UTCbegin);
			//CTRef tmp = CTimeZones::Time2TRef(UTCTime);//automaticly trunked
			//first = CTimeZones::TRef2Time(tmp);
			
			CTRef tmp = CTimeZones::Time2TRef(UTCTime);//automaticly trunked
			first = CTimeZones::TRef2Time(tmp);
			//UTCTime = floor(UTCTime / 3600) * 3600;
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
			next = CTimeZones::TRef2Time(tmp+1);//get next time step
			
			//UTCTime = ceil(UTCTime / 3600) * 3600;
			
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
				p += CTPeriod(*years.begin(), FIRST_MONTH, FIRST_DAY, *years.rbegin(), LAST_MONTH, LAST_DAY);
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
					p += CTimeZones::Time2TRef(UTCTime).as(CTM::DAILY);
				}
			}
		}

		return p;
	}

	size_t CATMWeather::GetGribsPrjID(__int64 UTCWeatherTime)const
	{
		size_t prjID = NOT_INIT;


		if (m_world.m_world_param.m_weather_type == CATMWorldParamters::FROM_STATIONS )
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

	double CATMWeather::get_air_temperature(const CGeoPoint3D& pt, __int64 UTCWeatherTime)
	{
	
		CGridPoint gpt(pt.m_x, pt.m_y, 10, 0, 0, 0, 0, pt.GetPrjID());
		//__int64 UTCTime20 = UTCWeatherTime + 20 * 3600;
		//UTCTime20 = GetNearestFloorTime(UTCTime20);


		/*if (m_world.m_world_param.m_weather_type == CATMWorldParamters::FROM_STATIONS ||
			m_world.m_world_param.m_weather_type == CATMWorldParamters::FROM_BOTH)
		{
			CTRef UTCTRef = CTimeZones::Time2TRef(UTCWeatherTime);
			CStatistic Tair_s;
			Tair_s += m_world.m_weather.m_iwd.at(UTCTRef + h)[ATM_TAIR].Evaluate(gpt);
			
			Tair += Tair_s[MEAN];
		}

		if (m_world.m_world_param.m_weather_type == CATMWorldParamters::FROM_GRIBS ||
			m_world.m_world_param.m_weather_type == CATMWorldParamters::FROM_BOTH)
		{
			ASSERT(!m_filepath_map.empty());

			*/
			//Get nearest grid of this time
			
		CATMVariables w = get_weather(pt, UTCWeatherTime);
		return w[ATM_TAIR];
		//}

		
		//return Tair[MEAN];
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
			CTRef TRef = it->m_readyToFly;//local time
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
	vector<CSBWMothsIt> CATMWorld::GetFlyers(CTRef TRef)
	{
		vector<CSBWMothsIt> fls;
		for (CSBWMothsIt it = m_moths.begin(); it != m_moths.end(); it++)
		{
			ASSERT(it->m_readyToFly.m_type == CTM::DAILY);
			CTRef readyToFly = it->m_readyToFly + m_moths_param.m_ready_to_fly_shift[it->m_sex];
			if (readyToFly <= TRef)
			{
				if (it->GetState() != CSBWMoth::DESTROYED_BY_OPTIMIZATION)
					fls.push_back(it);
			}
		}


		if (m_world_param.m_maxFlyers > 0)
		{
			while (fls.size() > m_world_param.m_maxFlyers)
			{
				size_t i = m_random.Rand(0, int(fls.size() - 1));
				fls[i]->DestroyByOptimisation();
				fls.erase(fls.begin() + i);
			}
		}

		return fls;
	}


	CTimePeriod CATMWorld::get_UTC_sunset_period(CTRef TRef, const vector<CSBWMothsIt>& fls)
	{
		CTimePeriod UTCp(_I64_MAX, _I64_MIN);

		for (size_t i = 0; i < fls.size(); i++)
		{
			ASSERT(fls[i]->GetState() != CSBWMoth::DESTROYED_BY_OPTIMIZATION);

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
			ASSERT(fls[i]->GetState() != CSBWMoth::DESTROYED_BY_OPTIMIZATION);
			if (fls[i]->m_liffoff_time > 0)
			{
				__int64  UTCLanding = fls[i]->m_liffoff_time + fls[i]->m_duration;
				UTCp.first = min(UTCp.first, fls[i]->m_liffoff_time);
				UTCp.second = max(UTCp.second, UTCLanding + 3600);
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

	ERMsg CATMWorld::Execute(CATMOutputMatrix& output, ofStream& output_file, CCallback& callback)
	{
		ASSERT(m_DEM_DS.IsOpen());
		ASSERT(m_weather.is_init());
		ASSERT(!m_moths.empty());

		ERMsg msg;

		Init(callback);

		const int nbSubPerHour = 3600 / m_world_param.m_outputFrequency;

		CATMOutputMatrix sub_output;
		if (output_file.is_open())
		{
			//write file header

			output_file << "l,p,r,Year,Month,Day,Hour,Minute,Second,";
			output_file << "flight,scale,sex,A,M,G,EggsLaid,state,x,y,lat,lon,";
			output_file << "T,P,U,V,W,";
			output_file << "MeanHeight,CurrentHeight,DeltaHeight,HorizontalSpeed,VerticalSpeed,Direction,Distance,DistanceFromOrigine,Defoliation" << endl;

			CTPeriod p = m_world_param.m_flightPeriod;
			p.End()++;
			p.Transform(CTM::HOURLY);
			ASSERT(p.size()*nbSubPerHour < LONG_MAX);
			__int64 begin = CTimeZones::TRef2Time(p.Begin()) / m_world_param.m_outputFrequency;
			__int64 end = CTimeZones::TRef2Time(p.End()) / m_world_param.m_outputFrequency;

			CTPeriod outputPeriod(CTRef(begin, 0, 0, 0, CTM::ATEMPORAL), CTRef(end, 0, 0, 0, CTM::ATEMPORAL));
			sub_output.resize(output.size());
			for (size_t l = 0; l < sub_output.size(); l++)
			{
				sub_output[l].resize(output[l].size());//the number of input variables
				for (size_t p = 0; p < output[l].size(); p++)
				{
					sub_output[l][p].resize(output[l][p].size());
					for (size_t r = 0; r < sub_output[l][p].size(); r++)
					{
						sub_output[l][p][r].Init(outputPeriod, VMISS);
					}
				}
			}
		}

		//get period of simulation
		CTPeriod period = m_world_param.m_simulationPeriod;

		callback.PushTask("Execute dispersal for year = " + ToString(period.Begin().GetYear()) + " (" + ToString(period.GetNbDay()) + " days)", period.GetNbDay());
		//for all days
		for (CTRef TRef = period.Begin(); TRef <= period.End() && msg; TRef++)
		{
			//get all ready to flight flyers for this day
			vector<CSBWMothsIt> ready_to_fly = GetFlyers(TRef);

			if (!ready_to_fly.empty())
			{
				//get sunset hours for this day
				CTimePeriod UTC_period = get_UTC_sunset_period(TRef, ready_to_fly);

				//load hours around sunset
				UTC_period.first -= 4 * 3600;
				UTC_period.second += 4 * 3600;

				vector<__int64> weather_time = GetWeatherTime(UTC_period, callback);

				//Load weather for sunset
				if (!weather_time.empty())
					msg = LoadWeather(TRef, weather_time, callback);

				if (msg)
				{
					vector<CSBWMothsIt> flyers;
					vector<CSBWMothsIt> nonflyers;
					//init all moths : broods and liffoff time
					for (size_t i = 0; i < ready_to_fly.size(); i++)
					{
						if (ready_to_fly[i]->init(TRef))
							flyers.push_back(ready_to_fly[i]);
						else
							nonflyers.push_back(ready_to_fly[i]);
					}

					callback.AddMessage("Dispersal for " + TRef.GetFormatedString("%Y-%m-%d"));
					callback.AddMessage("Flyers = " + to_string(flyers.size()), 1);
					callback.AddMessage("Non flyers = " + to_string(nonflyers.size()), 1);


					if (!flyers.empty())
					{

						msg = Execute(TRef, flyers, output, sub_output, callback);
						if (msg && !sub_output.empty())
						{
							//save sub-hourly output
							for (size_t l = 0; l < sub_output.size(); l++)
							{
								sub_output[l].resize(output[l].size());
								for (size_t p = 0; p < output[l].size(); p++)
								{
									sub_output[l][p].resize(output[l][p].size());
									for (size_t r = 0; r < sub_output[l][p].size(); r++)
									{
										for (size_t t = 0; t < sub_output[l][p][r].size(); t++)
										{
											size_t seconds = 0;// (t * m_world_param.m_outputFrequency) % (24 * 3600);
											size_t hours = size_t(t / nbSubPerHour);
											size_t minutes = (t % nbSubPerHour) * (m_world_param.m_outputFrequency/60);
											ASSERT(seconds % 60 == 0);

											output_file << l + 1 << "," << p + 1 << "," << r + 1 << ",";
											output_file << TRef.GetYear() << "," << TRef.GetMonth() + 1 << "," << TRef.GetDay() + 1 << "," << hours << "," << minutes << "," << seconds - 60 * minutes;
											for (size_t v = 0; v < NB_ATM_OUTPUT; v++)
												output_file << "," << sub_output[l][p][r][t][v];
											output_file << endl;
										}//for all time step
									}//for all replications
								}//for all parameters
							}//for all locations
						}//if sub hourly output
					}//if flyers

					if (!nonflyers.empty())
					{
						CTRef TRef18 = TRef.as(CTM::HOURLY);
						TRef18.m_hour = 18;


						//only update output for eggs laid
						for (size_t i = 0; i < nonflyers.size(); i++)
						{
							CSBWMoth& flyer = *(nonflyers[i]);

							CTRef UTCRef = CTimeZones::LocalTRef2UTCTRef(TRef18, flyer.m_location);
							__int64 UTCTime = CTimeZones::TRef2Time(UTCRef);

							flyer.FillOutput(TRef18, output);
							flyer.live(UTCTime);
							flyer.FillOutput(TRef18++, output);
						}
					}//have non flyers
				}//if msg
			}//have ready to flight moths

			msg += callback.StepIt();
		}//for all valid days


		callback.PopTask();

		return msg;
	}


	static const double MS2KMH = 3600.0 / 1000.0;


	//daily execution
	ERMsg CATMWorld::Execute(CTRef TRef, vector<CSBWMothsIt>& fls, CATMOutputMatrix& output, CATMOutputMatrix& sub_output, CCallback& callback)
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
			//				__int64 UTCTime = CTimeZones::UTCTRef2UTCTime(UTCTRef);

			//				float time = as<float>(tmp[7]);
			//				UTCTRef += int(time - 21);
			//				UTCTime += __int64((time - 21) * 3600);
			//				if (no == 1)
			//				{
			//					ASSERT(fls[0]->m_liffoff_time == UTCTime);
			//				}
			//				double x = as<double>(tmp[5]);
			//				double y = as<double>(tmp[4]);
			//				CGeoExtents testExtent = m_weather.Get(UTCTRef)->GetExtents();
			//				CGeoPoint3D testCoord;
			//				//((CGeoPoint&)testCoord) = CGeoPoint(-267903.37025, 1935436.44478, testExtent.GetPrjID()) + CGeoDistance(x / 1040 * 356138.6973, (768 - y) / 768 * 243112.6437, testExtent.GetPrjID());
			//				((CGeoPoint&)testCoord) = CGeoPoint(x, y, PRJ_WGS_84);
			//				testCoord.m_z = as<double>(tmp[6]);
			//				//testCoord.Reproject(CProjectionTransformation(testExtent.GetPrjID(), PRJ_WGS_84));

			//				const CProjectionTransformation& toWea = GetToWeatherTransfo(UTCTRef);
			//				const CProjectionTransformation& fromWea = GetFromWeatherTransfo(UTCTRef);

			//				CATMVariables w1 = m_weather.get_weather(testCoord, UTCTRef, UTCTime);
			//				double dt = 20; //[s]

			//				//my wind speed
			//				CGeoDistance3D U1(w1[ATM_WNDU], w1[ATM_WNDV], w1[ATM_WNDW], m_weather.GetGribsPrjID(UTCTRef));
			//				CGeoDistance3D d1 = U1*dt;
			//				CGeoPoint3D testCoord2 = UpdateCoordinate(testCoord, d1, toWea, fromWea);
			//				CATMVariables w2 = m_weather.get_weather(testCoord2, UTCTRef, UTCTime);
			//				
			//				CATMVariables w;
			//				CGeoDistance3D d2(m_weather.GetGribsPrjID(UTCTRef));
			//				CGeoDistance3D d3(m_weather.GetGribsPrjID(UTCTRef));
			//				if (w1.is_init() && w2.is_init())
			//				{
			//					w = (w1 + w2) / 2.0;

			//					CGeoDistance3D U2(w[ATM_WNDU], w[ATM_WNDV], w[ATM_WNDW], m_weather.GetGribsPrjID(UTCTRef));
			//					d2 = U2*dt;

			//					//Gary wind speed
			//					CGeoDistance3D U3(ToDouble(tmp[1]), ToDouble(tmp[2]), ToDouble(tmp[3]), m_weather.GetGribsPrjID(UTCTRef));
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
			__int64 UTCTimeº = CTimeZones::TRef2Time(TRef);
			__int64 global_seconds = gribs_time[0];//(gribs_time[0] - UTCTimeº);//start at the actual second of the day
			int nbSubPerHour = 3600 / m_world_param.m_outputFrequency;

			for (size_t t = 1; t < gribs_time.size() && msg; t++)
			{
				CTRef UTCTRef = CTimeZones::Time2TRef(gribs_time[t - 1]);
				__int64 step_duration = gribs_time[t] - gribs_time[t - 1];

#pragma omp parallel for if (m_world_param.m_weather_type == CATMWorldParamters::FROM_GRIBS )
				for (__int64 i = 0; i < (__int64)fls.size(); i++)
				{
#pragma omp flush(msg)

					CSBWMoth& flyer = *(fls[i]);
					ASSERT(flyer.GetState() != CSBWMoth::DESTROYED_BY_OPTIMIZATION);
					if (msg)
					{
						ASSERT((3600 % get_time_step()) == 0);
						for (__int64 seconds = 0; seconds < step_duration; seconds += get_time_step(), global_seconds += get_time_step())
						{
							__int64 UTCCurrentTime = gribs_time[t - 1] + seconds;
							flyer.live(UTCCurrentTime);

							__int64 countdown1 = UTCCurrentTime - flyer.m_liffoff_time;
							__int64 countdown2 = flyer.GetLog(CSBWMoth::T_IDLE_END) > 0 ? UTCCurrentTime - flyer.GetLog(CSBWMoth::T_IDLE_END) : 0;
							size_t state = (flyer.GetState() == CSBWMoth::IDLE_END) ? flyer.GetEnd() : flyer.GetState();
							CTRef localTRef = UTCTRef + int(flyer.GetUTCShift() / 3600);

							//report oputput only each hour
							if (global_seconds % 3600 == 0)
							{
								if (countdown1 >= -3600 &&
									countdown2 <= 3600)
								{
									flyer.FillOutput(localTRef, output);
								}

								flyer.ResetStat(CSBWMoth::HOURLY_STAT);
							}

							if (!sub_output.empty() &&
								global_seconds % m_world_param.m_outputFrequency == 0)
							{
								ASSERT((3600 % m_world_param.m_outputFrequency) == 0);
								if (countdown1 >= -m_world_param.m_outputFrequency &&
									countdown2 <= m_world_param.m_outputFrequency)
								{

									//int sub = (global_seconds + flyer.GetUTCShift()) / m_world_param.m_outputFrequency;
									//int index = localTRef.Get__int32()*nbSubPerHour + sub % nbSubPerHour;
									int index = (global_seconds + flyer.GetUTCShift()) / m_world_param.m_outputFrequency;
									CTRef CTRef(index, 0, 0, 0, CTM::ATEMPORAL);
									flyer.FillOutput(CTRef, sub_output);
								}

								flyer.ResetStat(CSBWMoth::SUB_HOURLY_STAT);
							}
						}//for all time step

						 //callback.WaitPause();
						msg += callback.StepIt();
					}//if msg
				}//for all flyers
			}//for alltime steps

			msg += m_weather.Discard(callback);
			callback.PopTask();
		}//have flyers

		return msg;
	}


	//output sub-hourly data
	//							if (!sub_output.empty() &&
	//								seconds%m_world_param.m_outputFrequency == 0 &&
	//								countdown1 >= -m_world_param.m_outputFrequency &&
	//								countdown2 <= m_world_param.m_outputFrequency)
	//							{
	//								CGeoPoint3D pt = flyer.m_pt;
	//
	//								pt.Reproject(m_GEO2.at(prjID));//convert from GEO to DEM projection
	//
	//								double alpha = atan2(flyer.GetStat(CSBWMoth::SUB_HOURLY_STAT, CSBWMoth::S_D_Y), flyer.GetStat(CSBWMoth::SUB_HOURLY_STAT, CSBWMoth::S_D_X));
	//								double angle = int(360 + 90 - Rad2Deg(alpha)) % 360;
	//								ASSERT(angle >= 0 && angle <= 360);
	//								double Dᵒ = flyer.m_newLocation.GetDistance(flyer.m_location, false, false);
	//
	//								double defoliation = -999;
	//								double broods = -999;
	//								if (flyer.GetLog(CSBWMoth::T_IDLE_END) > 0)
	//								{
	//									bool bOverWater = is_over_water(flyer.m_newLocation);
	//									//defoliation = bOverWater?-1:get_defoliation(flyer.m_newLocation);
	//									if (!bOverWater && ((flyer.m_flightNo == 0 && state == 0) || state >= 10))
	//										defoliation = get_defoliation(flyer.m_newLocation);
	//
	//									if (flyer.m_sex == CSBWMothParameters::FEMALE && !bOverWater && ((flyer.m_flightNo == 0 && state == 0) || state >= 10))
	//										broods = flyer.m_broods;
	//								}
	//
	//
	//
	////#pragma omp critical (SAVE_SUB_HOURLY)
	////								{
	////									size_t minutes = size_t(seconds / 60);
	////									int subTRef = 0;
	////
	////									
	////									//output_file << flyer.m_loc + 1 << "," << flyer.m_par + 1 << "," << flyer.m_rep + 1 << ",";
	////									sub_output[subTRef][]
	////									output_file << localTRef.GetYear() << "," << localTRef.GetMonth() + 1 << "," << localTRef.GetDay() + 1 << "," << localTRef.GetHour() << "," << minutes << "," << seconds - 60 * minutes << ",";
	////									output_file << flyer.m_flightNo << "," << flyer.m_scale << "," << flyer.m_sex << "," << flyer.m_A << "," << flyer.m_M << "," << flyer.m_G << "," << broods << "," << state << "," << pt.m_x << "," << pt.m_y << "," << flyer.m_newLocation.m_lat << "," << flyer.m_newLocation.m_lon << ",";
	////									output_file << flyer.GetStat(CSBWMoth::SUB_HOURLY_STAT, CSBWMoth::S_TAIR) << "," << flyer.GetStat(CSBWMoth::SUB_HOURLY_STAT, CSBWMoth::S_PRCP) << ",";
	////									output_file << flyer.GetStat(CSBWMoth::SUB_HOURLY_STAT, CSBWMoth::S_U, ms2kmh) << "," << flyer.GetStat(CSBWMoth::SUB_HOURLY_STAT, CSBWMoth::S_V, ms2kmh) << "," << flyer.GetStat(CSBWMoth::SUB_HOURLY_STAT, CSBWMoth::S_W, ms2kmh) << ",";
	////
	////									if (flyer.GetLog(CSBWMoth::T_LIFTOFF) > 0)
	////									{
	////										//log exists
	////										output_file << flyer.GetStat(CSBWMoth::SUB_HOURLY_STAT, CSBWMoth::S_HEIGHT) << "," << flyer.m_pt.m_z << "," << flyer.GetStat(CSBWMoth::SUB_HOURLY_STAT, CSBWMoth::S_D_Z, 1, SUM) << ",";
	////										output_file << flyer.GetStat(CSBWMoth::SUB_HOURLY_STAT, CSBWMoth::S_W_HORIZONTAL, ms2kmh) << "," << flyer.GetStat(CSBWMoth::SUB_HOURLY_STAT, CSBWMoth::S_W_VERTICAL, ms2kmh) << ",";
	////										output_file << angle << "," << flyer.GetStat(CSBWMoth::SUB_HOURLY_STAT, CSBWMoth::S_DISTANCE, 1, SUM) << "," << Dᵒ << ",";
	////									}
	////									else
	////									{
	////										output_file << "-999,-999,-999,-999,-999,-999,-999,-999,";
	////									}
	////
	////
	////									output_file << defoliation;
	////									output_file << endl;
	////								}
	////								flyer.ResetStat(CSBWMoth::SUB_HOURLY_STAT);
	////							}//if sub-hourly output 
	void CSBWMoth::FillOutput(CTRef localTRef, CATMOutputMatrix& output)
	{
		if (output[m_loc][m_par][m_rep].IsInside(localTRef))
		{
			//m_lastOutput = localTRef;
			//m_lastState = state;

			size_t state = (GetState() == CSBWMoth::IDLE_END) ? GetEnd() : GetState();
			double alpha = atan2(GetStat(CSBWMoth::HOURLY_STAT, CSBWMoth::S_D_Y), GetStat(CSBWMoth::HOURLY_STAT, CSBWMoth::S_D_X));
			double angle = int(360 + 90 - Rad2Deg(alpha)) % 360;
			ASSERT(angle >= 0 && angle <= 360);
			double Dᵒ = m_newLocation.GetDistance(m_location, false, false);

			size_t liftoffTime = CTimeZones::UTCTime2LocalTime(GetLog(CSBWMoth::T_LIFTOFF), m_location);
			size_t landingTime = CTimeZones::UTCTime2LocalTime(GetLog(CSBWMoth::T_LANDING), m_location);

			CGeoPoint3D pt = m_pt;

			size_t prjID = m_world.m_DEM_DS.GetPrjID();
			pt.Reproject(m_world.m_GEO2.at(prjID));//convert from GEO to DEM projection

			bool bOverWater = m_world.is_over_water(m_newLocation);

			double defoliation = VMISS;
			if (!bOverWater && ((state == 0) || state >= 10))
				defoliation = m_world.get_defoliation(m_newLocation);

			double broods = VMISS;
			if (state == 0) //|| (state >= NO_LIFTOFF_PRCP && state <= NO_LIFTOFF_WNDS))
				broods = m_broods;

			output[m_loc][m_par][m_rep][localTRef][ATM_FLIGHT] = m_flightNo;
			output[m_loc][m_par][m_rep][localTRef][ATM_SCALE] = m_scale;
			output[m_loc][m_par][m_rep][localTRef][ATM_SEX] = m_sex;
			output[m_loc][m_par][m_rep][localTRef][ATM_A] = m_A;
			output[m_loc][m_par][m_rep][localTRef][ATM_M] = m_M;
			output[m_loc][m_par][m_rep][localTRef][ATM_G] = m_G;
			output[m_loc][m_par][m_rep][localTRef][ATM_EGGS_LAID] = broods;
			output[m_loc][m_par][m_rep][localTRef][ATM_STATE] = state;
			output[m_loc][m_par][m_rep][localTRef][ATM_X] = pt.m_x;
			output[m_loc][m_par][m_rep][localTRef][ATM_Y] = pt.m_y;
			output[m_loc][m_par][m_rep][localTRef][ATM_LAT] = m_newLocation.m_lat;
			output[m_loc][m_par][m_rep][localTRef][ATM_LON] = m_newLocation.m_lon;
			output[m_loc][m_par][m_rep][localTRef][ATM_DEFOLIATION] = bOverWater ? VMISS : defoliation;

			if (state != 0)
			{
				output[m_loc][m_par][m_rep][localTRef][ATM_T] = GetStat(CSBWMoth::HOURLY_STAT, CSBWMoth::S_TAIR);
				output[m_loc][m_par][m_rep][localTRef][ATM_P] = GetStat(CSBWMoth::HOURLY_STAT, CSBWMoth::S_PRCP);
				output[m_loc][m_par][m_rep][localTRef][ATM_U] = GetStat(CSBWMoth::HOURLY_STAT, CSBWMoth::S_U, MS2KMH);
				output[m_loc][m_par][m_rep][localTRef][ATM_V] = GetStat(CSBWMoth::HOURLY_STAT, CSBWMoth::S_V, MS2KMH);
				output[m_loc][m_par][m_rep][localTRef][ATM_W] = GetStat(CSBWMoth::HOURLY_STAT, CSBWMoth::S_W, MS2KMH);
			}

			if (GetLog(CSBWMoth::T_LIFTOFF) > 0)
			{
				output[m_loc][m_par][m_rep][localTRef][ATM_MEAN_HEIGHT] = GetStat(CSBWMoth::HOURLY_STAT, CSBWMoth::S_HEIGHT);
				output[m_loc][m_par][m_rep][localTRef][ATM_CURRENT_HEIGHT] = m_pt.m_z;
				output[m_loc][m_par][m_rep][localTRef][ATM_DELTA_HEIGHT] = GetStat(CSBWMoth::HOURLY_STAT, CSBWMoth::S_D_Z, 1, SUM);
				output[m_loc][m_par][m_rep][localTRef][ATM_W_HORIZONTAL] = GetStat(CSBWMoth::HOURLY_STAT, CSBWMoth::S_W_HORIZONTAL, MS2KMH);
				output[m_loc][m_par][m_rep][localTRef][ATM_W_VERTICAL] = GetStat(CSBWMoth::HOURLY_STAT, CSBWMoth::S_W_VERTICAL, MS2KMH);
				output[m_loc][m_par][m_rep][localTRef][ATM_DIRECTION] = angle;
				output[m_loc][m_par][m_rep][localTRef][ATM_DISTANCE] = GetStat(CSBWMoth::HOURLY_STAT, CSBWMoth::S_DISTANCE, 1, SUM);
				output[m_loc][m_par][m_rep][localTRef][ATM_DISTANCE_FROM_OIRIGINE] = Dᵒ;

				if (GetLog(CSBWMoth::T_LANDING) > 0)
				{

					output[m_loc][m_par][m_rep][localTRef][ATM_LIFTOFF_TIME] = CTimeZones::GetDecimalHour(liftoffTime);
					output[m_loc][m_par][m_rep][localTRef][ATM_FLIGHT_TIME] = (landingTime - liftoffTime) / 3600.0;
					output[m_loc][m_par][m_rep][localTRef][ATM_LANDING_TIME] = CTimeZones::GetDecimalHour(landingTime);
				}


			}//log exists


		}//if output
	}
	vector<__int64> CATMWorld::GetWeatherTime(CTimePeriod UTC_period, CCallback& callback)const
	{
		vector<__int64> gribs_time;

		if (UTC_period.first <= UTC_period.second)
		{
			__int64 firstGribTime = m_weather.GetNearestFloorTime(UTC_period.first);
			__int64 lastGribTime = m_weather.GetNextTime(UTC_period.second);
			__int64 UTCLast = firstGribTime;

			for (__int64 UTCWeatherTime = firstGribTime; UTCWeatherTime <= lastGribTime; UTCWeatherTime = m_weather.GetNextTime(UTCWeatherTime))
			{
				if (UTCWeatherTime - UTCLast <= m_world_param.m_max_missing_weather)
				{
					gribs_time.push_back(UTCWeatherTime);

					UTCLast = UTCWeatherTime;
				}
				else
				{
					CTRef TRef = CTimeZones::Time2TRef(firstGribTime);
					callback.AddMessage("WARNING: too much Gribs missing. Nightly flight for " + TRef.GetFormatedString("%Y-%m-%d") + " was skipped");
					gribs_time.clear();
					UTCWeatherTime = lastGribTime + 9999;//end loop for
				}
			}
		}

		return gribs_time;
	}

	ERMsg CATMWorld::LoadWeather(CTRef TRef, const vector<__int64>& weather_time, CCallback& callback)
	{
		ERMsg msg;


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
				//if (m_world_param.m_weather_type == CATMWorldParamters::FROM_GRIBS ||
				//	m_world_param.m_weather_type == CATMWorldParamters::FROM_BOTH)
				//{
				size_t prjID = m_weather.GetGribsPrjID(UTCWeatherTime); ASSERT(prjID != NOT_INIT);
				if (m_GEO2.find(prjID) == m_GEO2.end())
					m_GEO2[prjID] = GetReProjection(PRJ_WGS_84, prjID);
				if (m_2GEO.find(prjID) == m_2GEO.end())
					m_2GEO[prjID] = GetReProjection(prjID, PRJ_WGS_84);
				//}
				
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


	ERMsg CATMWorld::CreateEggDepositionMap(const string& outputFilePath, CATMOutputMatrix& output, CCallback& callback)
	{
		ASSERT(m_DEM_DS.IsOpen());

		ERMsg msg;

		if (!output.empty() && !output[0].empty() && !output[0][0].empty() && !output[0][0][0].empty())
		{

			CBaseOptions options;
			m_DEM_DS.UpdateOption(options);

			CTPeriod period;
			CStatistic seasonStat;
			CGeoExtents extents;


			size_t nbSteps = output.size() * output[0].size() * output[0][0].size() * output[0][0][0].size();
			callback.PushTask("Compute Eggs map extents", nbSteps);

			for (size_t l = 0; l < output.size() && msg; l++)
			{
				for (size_t p = 0; p < output[l].size() && msg; p++)
				{
					for (size_t r = 0; r < output[l][p].size() && msg; r++)
					{
						for (size_t t = 0; t < output[l][p][r].size() && msg; t++)
						{
							if (output[l][p][r][t][ATM_EGGS_LAID] > 0)
							{
								seasonStat += output[l][p][r][t][ATM_EGGS_LAID];
								CTRef TRef = output[l][p][r].GetFirstTRef() + t;
								period.Inflate(TRef);
								CGeoPoint pt(output[l][p][r][t][ATM_X], output[l][p][r][t][ATM_Y], options.m_extents.GetPrjID());
								extents.ExtendBounds(pt);
							}
							msg += callback.StepIt();
						}
					}
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

						for (size_t l = 0; l < output.size() && msg; l++)
						{
							for (size_t p = 0; p < output[l].size() && msg; p++)
							{
								for (size_t r = 0; r < output[l][p].size() && msg; r++)
								{
									for (size_t t = 0; t < output[l][p][r].size() && msg; t++)
									{
										if (output[l][p][r][t][ATM_EGGS_LAID] > 0)
										{
											CTRef TRef2 = output[l][p][r].GetFirstTRef() + t;
											if (TRef2.as(CTM::DAILY) == TRef1)
											{
												CGeoPoint pt(output[l][p][r][t][ATM_X], output[l][p][r][t][ATM_Y], extents.GetPrjID());
												ASSERT(extents.IsInside(pt));

												CGeoPointIndex xy = options.m_extents.CoordToXYPos(pt);
												size_t pos = xy.m_y*extents.m_xSize + xy.m_x;
												season[pos] += output[l][p][r][t][ATM_EGGS_LAID];
											}
										}
										msg += callback.StepIt();
									}
								}
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

			/*	GDALDriver *poDriver = (GDALDriver *)GDALGetDriverByName("VRT");
				GDALDataset * poVRTDS = poDriver->CreateCopy("", Dataset(), FALSE, NULL, NULL, NULL);
				poVRTDS->set
				for (nBand = 1; nBand <= poVRTDS->GetRasterCount(); nBand++)
				{
				char szFilterSourceXML[10000];
				GDALRasterBand *poBand = poVRTDS->GetRasterBand(nBand);
				sprintf(szFilterSourceXML,
				"<KernelFilteredSource>"
				"  <SourceFilename>%s</SourceFilename><SourceBand>%d</SourceBand>"
				"  <Kernel>"
				"    <Size>3</Size>"
				"    <Coefs>0.111 0.111 0.111 0.111 0.111 0.111 0.111 0.111 0.111</Coefs>"
				"  </Kernel>"
				"</KernelFilteredSource>",
				pszSourceFilename, nBand);
				poBand->SetMetadataItem("source_0", szFilterSourceXML, "vrt_sources");
				}
				*/


				//m_extentsSub = GetExtents();
				//if (m_clipRect.IsInit())
				//{
				//	size_t prjID = GetExtents().GetPrjID();
				//	ASSERT(prjID != NOT_INIT);
				//	CProjectionTransformation TT(m_clipRect.GetPrjID(), prjID);
				//	m_clipRect.Reproject(TT);

				//	m_extentsSub.IntersectRect(m_clipRect);
				//	m_extentsSub.AlignTo(GetExtents());
				//	//m_clipRect
				//}
				//
				//m_indexSub = GetExtents().CoordToXYPos(m_extentsSub);



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

