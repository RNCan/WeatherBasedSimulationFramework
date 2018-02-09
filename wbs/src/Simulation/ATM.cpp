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

//NO_LIFTOFF_PRCP  	    10
//NO_LIFTOFF_TAIR  	    11
//NO_LIFTOFF_WNDS  	    12
//END_BY_PRCP			13
//END_BY_TAIR			14
//END_BY_SUNRISE		15
//OUTSIDE_MAP			16
//OUTSIDE_TIME_WINDOW	17





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
		double Δλ = δ*sin(θ) / q;
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
	enum TGeoH{ GEOH_0, GEOH_110, GEOH_323, GEOH_540, GEOH_762, GEOH_988, MAX_GEOH = 36 };


	//from : http://ruc.noaa.gov/RUC.faq.html
	void Convert2ThrueNorth(const CGeoPoint& pt, double &u, double &v)
	{
		//**  ROTCON_P          WIND ROTATION CONSTANT = 1 FOR POLAR STEREO AND SIN(LAT_TAN_P) FOR LAMBERT CONFORMAL
		//**  LON_XX_P          MERIDIAN ALIGNED WITH CARTESIAN X - AXIS(DEG)
		//**  LAT_TAN_P         LATITUDE AT LAMBERT CONFORMAL PROJECTION IS TRUE(DEG)


		static const double LON_XX_P = -95.0;
		static const double LAT_TAN_P = 25.0;
		static const double ROTCON_P = sin(Deg2Rad(LAT_TAN_P)); //0.422618;

		double angle = ROTCON_P*Deg2Rad(pt.m_lon - LON_XX_P);
		double sinx = sin(angle);
		double cosx = cos(angle);

		double un = cosx*u + sinx*v;
		double vn = -sinx*u + cosx*v;

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


	const char* CATMParameters::MEMBERS_NAME[NB_ATM_MEMBERS] = { "BroodTSource", "PSource", "Pmax", "Wmin", "WingBeatScale", "HorzontalScale", "Whorzontal", "WhorzontalSD", "Wdescent", "WdescentSD", "WindStabilityType", "NbWeatherStations" };


	double CATMWorld::get_w_horizontal()const
	{
		double ran = m_random.Rand(-1.0, 1.0);//random value [-1,1];
		double w = max(0.0, m_parameters2.m_w_horizontal + ran*m_parameters2.m_w_horizontal_σ);

		ASSERT(w >= 0);
		return w * 1000 / 3600;//convert from km/h to m/s
	}

	double CATMWorld::get_w_descent()const
	{
		double ran = m_random.Rand(-1.0, 1.0);//random value [-1,1];
		double w = min(-1.8, m_parameters2.m_w_descent + ran*m_parameters2.m_w_descent_σ);//force to descent at least at 1.8 km/h (0.5 m/s)
		ASSERT(w < 0);

		return w * 1000 / 3600;//convert from km/h to m/s
	}


	//***********************************************************************************************

	CFlyer::CFlyer(CATMWorld& world) :
		m_world(world)
	{
		m_sex = -1;
		m_A = 0;
		m_M = 0;
		m_G = 0;
		m_Fᵒ = 0;
		m_Δv = 0;
		m_broods = 0;
		m_eggsLeft = 0;
		m_loc = 0;
		m_par = 0;
		m_rep = 0;
		m_flightNo = 0;
		m_scale = 0;

		m_liftoffOffset = 0;
		m_w_descent = 0;
		m_liffoff_time = 0;
		m_duration = 0;
		//		m_Δangle_time = 0;
		//m_Δangle = 0;


		m_state = NOT_CREATED;
		m_end_type = NO_END_DEFINE;
		m_log.fill(0);
		m_UTCShift = 0;
	}


	void CFlyer::init()
	{
		CTRef UTCTRef = CTimeZones::LocalTRef2UTCTRef(m_localTRef, m_location);
		__int64 UTCTime = CTimeZones::UTCTRef2UTCTime(UTCTRef);
		__int64 localTime = CTimeZones::UTCTime2LocalTime(UTCTime, m_location);
		__int64 sunriseTime = CATMWorld::get_local_sunrise(m_localTRef + 12, m_location); //sunrise of the next day
		sunriseTime = CTimeZones::LocalTime2UTCTime(sunriseTime, m_location);

		m_UTCShift = localTime - UTCTime;

		ASSERT(m_liftoffOffset >= 0 && m_liftoffOffset <= 3600);



		m_state = NOT_CREATED;
		m_end_type = NO_END_DEFINE;
		m_log.fill(0);

		m_pt.m_alt = 10;
		m_liffoff_time = UTCTime + m_liftoffOffset;
		m_w_horizontal = m_world.get_w_horizontal();
		m_w_descent = m_world.get_w_descent();
		m_duration = sunriseTime - m_liffoff_time;
		ASSERT(m_duration >= 0 && m_duration < 24 * 3600);

	}

	void CFlyer::AddStat(const CATMVariables& w, const CGeoDistance3D& U, const CGeoDistance3D& d)
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

	void CFlyer::AddStat(const CATMVariables& w)
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


	void CFlyer::live(CTRef UTCTRef, __int64 UTCTime)
	{
		if (!m_world.m_weather.IsLoaded(UTCTRef))
			m_state = IDLE_END, m_end_type = OUTSIDE_TIME_WINDOW;

		if (m_end_type == NO_END_DEFINE && !m_world.IsInside(m_pt))
			m_state = IDLE_END, m_end_type = OUTSIDE_MAP;


		switch (m_state)
		{
		case NOT_CREATED:		create(UTCTRef, UTCTime); break;
		case IDLE_BEGIN:		idle_begin(UTCTRef, UTCTime); break;
		case LIFTOFF:			liftoff(UTCTRef, UTCTime); break;
		case FLIGHT:			flight(UTCTRef, UTCTime); break;
		case LANDING:			landing(UTCTRef, UTCTime); break;
		case IDLE_END:			idle_end(UTCTRef, UTCTime);  break;
		default: assert(false);
		}


	}

	void CFlyer::create(CTRef UTCTRef, __int64 UTCTime)
	{
		__int64 countdown = (__int64)UTCTime - m_liffoff_time;
		if (countdown >= 0)
		{
			m_state = IDLE_BEGIN;
			m_log[T_CREATION] = m_world.get_UTC_time();

			AddStat(m_world.get_weather(m_pt, UTCTRef, UTCTime));
		}
	}

	void CFlyer::idle_begin(CTRef UTCTRef, __int64 UTCTime)
	{
		CATMVariables w = m_world.get_weather(m_pt, UTCTRef, UTCTime);
		AddStat(w);

		double Wmin = m_world.m_parameters2.m_Wmin * 1000 / 3600; //km/h -> m/s 

		double Tᴸ = get_Tᴸ();
		double ws = w.get_wind_speed();

		ASSERT(!IsMissing(w[ATM_TAIR]) && !IsMissing(w[ATM_PRCP]) && !IsMissing(w[ATM_WNDU]) && !IsMissing(w[ATM_WNDV]));

		if (w[ATM_PRCP] < m_world.m_parameters2.m_Pmax && w[ATM_TAIR] >= Tᴸ && ws >= Wmin)
		{
			m_state = LIFTOFF;
		}
		else
		{
			__int64 duration = UTCTime - m_liffoff_time;
			if (duration > 2 * 3600)
			{
				//flight abort
				if (w[ATM_PRCP] > m_world.m_parameters2.m_Pmax)
					m_end_type = NO_LIFTOFF_PRCP;
				else if (w[ATM_TAIR] < Tᴸ)
					m_end_type = NO_LIFTOFF_TAIR;
				else if (ws < Wmin)
					m_end_type = NO_LIFTOFF_WNDS;

				m_state = IDLE_END;
			}
		}

	}


	void CFlyer::liftoff(CTRef UTCTRef, __int64 UTCTime)
	{
		m_flightNo++;//a new flight for this moth
		m_log[T_LIFTOFF] = UTCTime;
		m_state = FLIGHT;

		CATMVariables w = m_world.get_weather(m_pt, UTCTRef, UTCTime);
		AddStat(w);
	}


	void CFlyer::flight(CTRef UTCTRef, __int64 UTCTime)
	{
		ASSERT(m_world.m_weather.IsLoaded(UTCTRef));
		ASSERT(m_world.IsInside(m_pt));
		ASSERT(m_state == FLIGHT);
		ASSERT(m_end_type == NO_END_DEFINE);


		CATMVariables w = get_weather(UTCTRef, UTCTime);
		if (w.is_init())
		{
			if (w[ATM_PRCP] < m_world.m_parameters2.m_Pmax)
			{
				__int64 duration = UTCTime - m_liffoff_time;
				if (m_Δv == 0 && m_pt.m_z > 60)//m_Δv is apply atfer lift-off when the moth reach an altitude of 60 meters
					m_Δv = m_world.m_parameters2.m_Δv;

				//after greenbank : After dark (2200 h), the orientation soon became completely downwind at all altitudes.
				//if (m_world.m_parameters1.m_bUseTurbulance)
				//{
				//	//change flight angle relative to wind angle
				//	__int64 last_Δangle = UTCTime - m_Δangle_time;
				//	if (last_Δangle > 60 * 5)//if it's the same angle since 5 minutes, change it
				//	{
				//		m_Δangle_time = UTCTime;
				//		m_Δangle = WBSF::Deg2Rad(m_world.random().RandNormal(0, 40));
				//	}
				//}


				if (duration < m_duration)
				{
					double dt = m_world.get_time_step(); //[s]

					CGeoDistance3D U = get_U(w, UTCTRef, UTCTime);
					CGeoDistance3D d = U*dt;


					const CProjectionTransformation& toWea = m_world.GetToWeatherTransfo(UTCTRef);
					const CProjectionTransformation& fromWea = m_world.GetFromWeatherTransfo(UTCTRef);
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

	void CFlyer::landing(CTRef UTCTRef, __int64 UTCTime)
	{
		ASSERT(m_world.m_weather.IsLoaded(UTCTRef));
		ASSERT(m_world.IsInside(m_pt));
		ASSERT(m_end_type != NO_END_DEFINE);
		ASSERT(m_state == LANDING);

		double dt = m_world.get_time_step(); //[s]
		CATMVariables w = get_weather(UTCTRef, UTCTime);
		if (w.is_init())
		{
			CGeoDistance3D U = get_U(w, UTCTRef, UTCTime);
			CGeoDistance3D d = U*dt;

			const CProjectionTransformation& toWea = m_world.GetToWeatherTransfo(UTCTRef);
			const CProjectionTransformation& fromWea = m_world.GetFromWeatherTransfo(UTCTRef);
			((CGeoPoint3D&)m_pt) = UpdateCoordinate(m_pt, d, toWea, fromWea);
			((CGeoPoint3D&)m_newLocation) = UpdateCoordinate(m_newLocation, d, toWea, fromWea);

			if (m_pt.m_z <= 5)//let moth landing correcly
			{
				//it's the end
				m_log[T_LANDING] = UTCTime;
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

	void CFlyer::idle_end(CTRef UTCTRef, __int64 UTCTime)
	{
		ASSERT(m_end_type != NO_END_DEFINE);

		if (m_log[T_IDLE_END] == 0)
		{
			m_log[T_IDLE_END] = UTCTime;

			if (m_sex == CATMParameters::FEMALE)
			{
				bool bOverWater = m_world.is_over_water(m_newLocation);
				if (!bOverWater)
				{
					bool bOverDefol = m_world.is_over_defoliation(m_newLocation);

					if (bOverDefol&&m_world.m_parameters1.m_maxFlights > 1 && m_flightNo < m_world.m_parameters1.m_maxFlights)
					{

						double T = 17;
						//switch (m_parameters2.m_broodTSource)
						//{
						//case CATMParameters::BROOD_T_17: T = 17; break;
						//case CATMParameters::BROOD_T_SAME_AS_INPUT:
						//{
						//	//CATMVariables w = get_weather(UTCTRef, UTCTime);
						//	//T = w[ATM_TAIR]; 
						//	break;
						//}
						//case CATMParameters::BROOD_T_WEATHER_STATION:
						//	; break;
						//}


						Brood(T);
					}
					else
					{
						m_broods = m_eggsLeft;
					}
				}

			}//if female
			/*if (m_sex == CATMParameters::FEMALE)
			{
			if (!m_world.is_over_water(m_newLocation))
			{
			double T = 0;
			switch (m_world.m_parameters2.m_broodTSource)
			{
			case CATMParameters::BROOD_T_17: T = 17; break;
			case CATMParameters::BROOD_T_SAME_AS_INPUT:
			{
			CATMVariables w = get_weather(UTCTRef, UTCTime);
			T = w[ATM_TAIR]; break;
			}
			case CATMParameters::BROOD_T_WEATHER_STATION:
			; break;
			}


			Brood(T);
			}
			}*/

			AddStat(m_world.get_weather(m_pt, UTCTRef, UTCTime));
		}
	}



	void CFlyer::DestroyByOptimisation()
	{
		m_state = DESTROYED_BY_OPTIMIZATION;
	}

	CGeoDistance3D CFlyer::get_U(const CATMVariables& w, CTRef UTCTRef, __int64 UTCTime)const
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
		case FLIGHT:	Uz = w[ATM_WNDW] + get_Uz(UTCTime, w); break;	//[m/s]; 
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

		return CGeoDistance3D(Ux, Uy, Uz, m_world.m_weather.GetGribsPrjID(UTCTRef));
	}


	//const double CFlyer::b[2] = { 21.35, 24.08 };
	//const double CFlyer::c[2] = { 2.97, 6.63 };
	//const double CFlyer::b[2] = { 21.35, 21.35 };
	//const double CFlyer::c[2] = { 2.97, 2.97 };
	const double CFlyer::Vmax = 65.0;
	const double CFlyer::K = 166;
	const double CFlyer::b[2] = { 21.35, 24.08 };
	const double CFlyer::c[2] = { 2.97, 6.63 };
	const double CFlyer::deltaT[2] = { 0, 3.5 };



	//T : air temperature [ᵒC]
	//out: forewing frequency [Hz] for this temperature
	double CFlyer::get_Vᵀ(double T)const
	{
		return get_Vᵀ(m_sex, T);
	}

	//sex : male=0, female=1
	//Vmax: maximum wingbeat [Hz]
	//T : air temperature [ᵒC]
	//out: forewing frequency [Hz] 
	double CFlyer::get_Vᵀ(size_t sex, double T)
	{
		double Vᵀ = 0;
		if (T > 0)
			Vᵀ = Vmax*(1 - exp(-pow((T + deltaT[sex]) / b[sex], c[sex])));

		return Vᵀ;
	}

	//out : liftoff temperature [ᵒC] for this insect
	double CFlyer::get_Tᴸ()const
	{
		return get_Tᴸ(m_sex, m_A, m_M, m_Δv);
	}


	double CFlyer::get_Vᴸ(double A, double M, double Δv)
	{
		return K* sqrt(M) / ((1 - Δv)*A);
	}
	//K : constant
	//Vmax: maximum wingbeat [Hz]
	//A : forewing surface area [cm²]
	//M : dry weight [g]
	//out : liftoff temperature [ᵒC] 
	double CFlyer::get_Tᴸ(size_t sex, double A, double M, double Δv)
	{
		double Vᴸ = get_Vᴸ(A, M, Δv);
		double Tᴸ = (Vᴸ < Vmax) ? b[sex] * pow(-log(1 - Vᴸ / Vmax), 1.0 / c[sex]) - deltaT[sex] : 40;

		ASSERT(!isnan(Tᴸ));

		return Tᴸ;
	}

	double CFlyer::get_Uz(__int64 UTCTime, const CATMVariables& w)const
	{
		ASSERT(m_state == FLIGHT);

		double Uz = 0;
		double Vᵀ = get_Vᵀ(w[ATM_TAIR]);
		double Vᴸ = get_Vᴸ(m_A, m_M, m_Δv);

		Uz = m_world.m_parameters2.m_w_α*(Vᵀ - Vᴸ) * 1000 / 3600;//Uz can be negative


		return Uz;//m/s
	}

	CATMVariables CFlyer::get_weather(CTRef UTCTRef, __int64 UTCTime)const
	{
		ASSERT(m_world.m_weather.IsLoaded(UTCTRef));

		CATMVariables w;

		CGeoPoint3D ptᵒ = m_pt;
		CATMVariables wᵒ = m_world.get_weather(ptᵒ, UTCTRef, UTCTime);
		if (wᵒ.is_init())
		{

			CGeoDistance3D Uᵒ = get_U(wᵒ, UTCTRef, UTCTime);
			double dt = m_world.get_time_step(); //[s]

			const CProjectionTransformation& toWea = m_world.GetToWeatherTransfo(UTCTRef);
			const CProjectionTransformation& fromWea = m_world.GetFromWeatherTransfo(UTCTRef);
			CGeoPoint3D pt¹ = UpdateCoordinate(m_pt, Uᵒ*dt, toWea, fromWea);


			if (m_world.m_parameters1.m_bUsePredictorCorrectorMethod &&
				m_world.m_weather.IsLoaded(UTCTRef + int(dt / 3600)) &&
				m_world.IsInside(pt¹) &&
				pt¹.m_z > 0)
			{
				CATMVariables w¹ = m_world.get_weather(pt¹, UTCTRef + int(dt / 3600), UTCTime + dt);
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

	void CFlyer::Brood(double T)
	{
		ASSERT(m_sex == CATMParameters::FEMALE);

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
			P = p*ξ;

		} while (P<0 || P > 0.7);


		m_broods = m_eggsLeft *P;
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
					ASSERT(!m_pt[0][y][x].IsInit() || !m_pt[1][y][x].IsInit() || pt.m_z >= m_pt[0][y][x].m_z);
					ASSERT(!m_pt[0][y][x].IsInit() || !m_pt[1][y][x].IsInit() || pt.m_z <= m_pt[1][y][x].m_z);
					if (m_pt[z][y][x].IsInit())
					{
						//to avoid lost weight of elevation with distance, we compute 2 weight, one for distance and one for delta elevation
						double d_xy = max(1.0, ((CGeoPoint&)m_pt[z][y][x]).GetDistance(pt));//limit to 1 meters to avoid division by zero
						double d_z = max(0.001, fabs(m_pt[z][y][x].m_z - pt.m_z));///limit to 1 mm to avoid division by zero
						double p1 = 1 / pow(d_xy, POWER);
						double p2 = 1 / pow(d_z, POWER);
						//double p =

						//double d_ = max(1.0, ((CGeoPoint&)m_pt[z][y][x]).GetDistance(pt));//limit to 1 meters to avoid division by zero
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
							if (me[z][y][x][v]>-999)
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
					if (me[nearest.m_z][nearest.m_y][nearest.m_x][v]>-999)
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
	CATMVariables CATMWeatherCuboids::get_weather(const CGeoPoint3D& pt, __int64 time)const
	{
		ASSERT(at(0).m_time <= at(1).m_time);
		ASSERT(time >= at(0).m_time && (at(0).m_time == at(1).m_time || time <= at(1).m_time));
		ASSERT(at(1).m_time - at(0).m_time == 0 || at(1).m_time - at(0).m_time == 3600);

		const CATMWeatherCuboids& me = *this;
		CATMVariables w;


		if (at(1).m_time != at(0).m_time)
		{
			double fᵒ = 1 - (double(time) - at(0).m_time) / (at(1).m_time - at(0).m_time); // get fraction of time
			if (!m_bUseTimeInterpolation)
				fᵒ = fᵒ >= 0.5 ? 1 : 0;

			double f¹ = (1 - fᵒ);
			ASSERT(fᵒ + f¹ == 1);

			CATMVariables wᵒ = me[0].get_weather(pt, m_bUseSpaceInterpolation);
			CATMVariables w¹ = me[1].get_weather(pt, m_bUseSpaceInterpolation);
			if (wᵒ.is_init() && w¹.is_init())
			{
				w = wᵒ*fᵒ + w¹*f¹;
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

	CATMVariables CATMWeather::get_weather(const CGeoPoint3D& pt, CTRef UTCTRef, __int64 UTCTime)const
	{
		ASSERT(pt.IsGeographic());

		size_t weather_type = m_world.m_parameters1.m_weather_type;
		if (weather_type == CATMWorldParamters::FROM_GRIBS &&
			(m_world.m_parameters2.m_PSource == CATMParameters::PRCP_WEATHER_STATION))
			weather_type = CATMWorldParamters::FROM_BOTH;


		CATMVariables w1;
		if (weather_type == CATMWorldParamters::FROM_GRIBS ||
			weather_type == CATMWorldParamters::FROM_BOTH)
		{
			CGeoPoint3D pt2(pt);

			size_t prjID = m_p_weather_DS.GetPrjID(UTCTRef);
			ASSERT(prjID != NOT_INIT);
			pt2.Reproject(m_world.m_GEO2.at(prjID));


			CATMWeatherCuboidsPtr p_cuboid = get_cuboids(pt2, UTCTRef, UTCTime);
			w1 = p_cuboid->get_weather(pt2, UTCTime);
		}


		CATMVariables w2;
		if (weather_type == CATMWorldParamters::FROM_STATIONS ||
			weather_type == CATMWorldParamters::FROM_BOTH)
		{
			w2 = get_station_weather(pt, UTCTRef, UTCTime);
		}


		if (m_world.m_parameters2.m_PSource == CATMParameters::PRCP_WEATHER_STATION)
			w1[ATM_PRCP] = w2[ATM_PRCP];


		w2[ATM_WNDW] = w1[ATM_WNDW];	//replace station W by Gribs W
		w1[ATM_WATER] = w2[ATM_WATER];	//replace Gribs Tw by station Tw 

		CATMVariables weather;
		switch (m_world.m_parameters1.m_weather_type)
		{
		case CATMWorldParamters::FROM_GRIBS: weather = w1; break;
		case CATMWorldParamters::FROM_STATIONS:weather = w2; break;
		case CATMWorldParamters::FROM_BOTH:weather = (w1 + w2) / 2; break;
		default: ASSERT(false);
		}

		if (weather[ATM_PRCP] > -999 && m_world.m_parameters2.m_PSource == CATMParameters::DONT_USE_PRCP)
			weather[ATM_PRCP] = 0;

		return weather;
	}

	__int64 CATMWorld::get_local_sunrise(CTRef TRef, const CLocation& loc)
	{
		//Get time at the begin of the day and add sunset
		__int64 sunriseTime = CTimeZones::LocalTRef2LocalTime(CTRef(TRef.GetYear(), TRef.GetMonth(), TRef.GetDay(), 0), loc);
		__int64 zone = CTimeZones::GetDelta(TRef, loc) / 3600;

		CSun sun(loc.m_lat, loc.m_lon, zone);
		sunriseTime += __int64(sun.GetSunrise(TRef) * 3600);

		return sunriseTime;
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
			double F = LandWaterWindFactor(sqrt(Ur*Ur + Vr*Vr), ΔT);
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

	CATMVariables CATMWeather::get_station_weather(const CGeoPoint3D& pt, CTRef UTCTRef, __int64  UTCTime)const
	{
		CATMVariables w;
		CATMVariables wᵒ = get_station_weather(pt, UTCTRef);
		CATMVariables w¹ = get_station_weather(pt, UTCTRef + 1);
		ASSERT(GetHourlySeconds(UTCTime) >= 0 && GetHourlySeconds(UTCTime) <= 3600);


		double fᵒ = 1 - GetHourlySeconds(UTCTime) / 3600.0;
		if (!m_world.m_parameters1.m_bUseTimeInterpolation)
			fᵒ = fᵒ >= 0.5 ? 1 : 0;

		double f¹ = (1 - fᵒ);
		if (wᵒ.is_init() && w¹.is_init())
			w = wᵒ*fᵒ + w¹*f¹;

		return w;
	}

	CATMVariables CATMWeather::get_station_weather(const CGeoPoint3D& pt, CTRef UTCTRef)const
	{
		ASSERT(!_isnan(pt.m_x) && _finite(pt.m_x));
		ASSERT(!_isnan(pt.m_y) && _finite(pt.m_y));
		ASSERT(!_isnan(pt.m_z) && _finite(pt.m_z));

		ASSERT(pt.IsGeographic());

		CATMVariables weather;
		bool bOverWater = m_world.is_over_water(pt);

		CGridPoint gpt(pt.m_x, pt.m_y, 10, 0, 0, 0, 0, pt.GetPrjID());

		for (size_t v = 0; v < NB_ATM_VARIABLES; v++)
			weather[v] = m_iwd.at(UTCTRef)[v].Evaluate(gpt);

		double Tw = weather[ATM_WATER];		//water temperature [ᵒC]
		double ΔT = weather[ATM_TAIR] - Tw;	//difference between air and water temperature

		GetWindProfileRelationship(weather[ATM_WNDU], weather[ATM_WNDV], pt.m_z, m_world.m_parameters2.m_windS_stability_type, bOverWater, ΔT);


		//adjust air temperature with flight height with a default gradient of 0.65ᵒC/100m
		weather[ATM_TAIR] += -0.0065*pt.m_z;//flight height temperature correction

		return weather;
	}

	CGeoPointIndex CATMWeather::get_xy(const CGeoPoint& ptIn, CTRef UTCTRef)const
	{
		CGeoExtents extents = m_p_weather_DS.GetExtents(UTCTRef);
		CGeoPoint pt = ptIn - CGeoDistance(extents.XRes() / 2, extents.YRes() / 2, extents.GetPrjID());

		return extents.CoordToXYPos(pt);//take the lower

	}

	size_t CATMWeather::get_level(const CGeoPointIndex& xy, const CGeoPoint3D& ptIn, CTRef UTCTRef, bool bLow)const
	{
		CGeoPoint3D pt(ptIn);
		vector<pair<double, int>> test;

		//in some product, geopotentiel hight is above ground
		//in other product, geopotentiel hight is above sea level

		double grAlt = GetFirstAltitude(xy, UTCTRef);//get the first level over the ground

		if (grAlt <= -999)
			grAlt = m_world.GetGroundAltitude(pt);
		//if (firstAlt == 0)//if the geopotentiel hight is above ground level, we have to substract grouind level to elevation
		//pt.m_z = max(0.0, pt.m_z -grAlt);

		if (grAlt > -999)
			test.push_back(make_pair(grAlt, 0));


		for (int l = 1; l < NB_LEVELS; l++)
		{
			size_t b = m_p_weather_DS.get_band(UTCTRef, ATM_HGT, l);
			if (b != NOT_INIT)
			{
				double gph = m_p_weather_DS.GetPixel(UTCTRef, b, xy); //geopotential height [m]
				if (gph > -999)
					test.push_back(make_pair(gph, l));

				if (pt.m_alt < gph)
					break;
			}
			else
			{
				//see if it's a fixed high layer
				double elev = 0;
				if (m_p_weather_DS.get_fixed_elevation_level(UTCTRef, l, elev))
					test.push_back(make_pair(grAlt + elev, l));
			}
		}



		sort(test.begin(), test.end());

		size_t L = NOT_INIT;
		if (test.size() >= 2)
		{
			for (size_t l = 0; l < test.size(); l++)
			{
				if (pt.m_alt < test[l].first)
				{
					L = test[bLow ? (l == 0 ? 0 : l - 1) : l].second;
					break;
				}
			}
		}

		ASSERT(L == NOT_INIT || L < NB_LEVELS);
		return L;
	}

	

	double CATMWeather::GetFirstAltitude(const CGeoPointIndex& xy, CTRef UTCTRef)const
	{
		size_t b = m_p_weather_DS.get_band(UTCTRef, ATM_HGT, 0);
	
		if (b == NOT_INIT)
			return -999;


		double gph = m_p_weather_DS.GetPixel(UTCTRef, b, xy); //geopotential height [m]

		return gph;
	}

	CGeoPoint3DIndex CATMWeather::get_xyz(const CGeoPoint3D& pt, CTRef UTCTRef)const
	{
		CGeoExtents extents = m_p_weather_DS.GetExtents(UTCTRef);
		CGeoPoint3DIndex xyz;
		((CGeoPointIndex&)xyz) = extents.CoordToXYPos(pt + CGeoDistance3D(extents.XRes() / 2, extents.YRes() / 2, 0, extents.GetPrjID()));


		xyz.m_z = MAX_GEOH - 1;//take the last level (~2000m) on by default

		for (size_t l = 0; l < NB_LEVELS; l++)
		{
			size_t b = m_p_weather_DS.get_band(UTCTRef, ATM_HGT, l);
			double gph = m_p_weather_DS.GetPixel(UTCTRef, b, xyz); //geopotential height [m]
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


	CATMWeatherCuboidsPtr CATMWeather::get_cuboids(const CGeoPoint3D& ptIn, CTRef UTCTRef, __int64 UTCTime)const
	{
		ASSERT(IsLoaded(CTimeZones::UTCTime2UTCTRef(UTCTime)) && IsLoaded(UTCTRef));
		//ASSERT(ptIn.m_z>=0);

		CATMWeatherCuboidsPtr cuboids(new CATMWeatherCuboids);
		cuboids->m_bUseSpaceInterpolation = m_world.m_parameters1.m_bUseSpaceInterpolation;
		cuboids->m_bUseTimeInterpolation = m_world.m_parameters1.m_bUseTimeInterpolation;


		ASSERT(IsLoaded(UTCTRef));
		if (!IsLoaded(UTCTRef))
			return cuboids;//humm


		//ASSERT(m_p_weather_DS.get_band(UTCTRef, ATM_WNDW, 0) != UNKNOWN_POS || m_p_weather_DS.get_band(UTCTRef, ATM_VVEL, 0) != UNKNOWN_POS);
		//if have VVEL, then it's RUC otherwise it's WRF
		//in the case of HRDPS, VVEL is only available for some layer
		//size_t gribType = m_p_weather_DS.get_band(UTCTRef, ATM_VVEL, 0) != UNKNOWN_POS ? RUC_TYPE : WRF_TYPE;

		//fill cuboid
		for (size_t i = 0; i < TIME_SIZE; i++, UTCTRef++)
		{
			(*cuboids)[i].m_time = CTimeZones::UTCTRef2UTCTime(UTCTRef);//reference in second

			if (i == 1 && !IsLoaded(UTCTRef))
			{
				(*cuboids)[1] = (*cuboids)[0];
				return cuboids;
			}

			const CGeoExtents& extents = m_p_weather_DS.GetExtents(UTCTRef);

			double groundAlt = 0;

			//RUC is above sea level and WRF must be above sea level
			if (m_bHgtOverSea)
			{
				groundAlt = m_world.GetGroundAltitude(ptIn);
			}



			CGeoPoint3D pt(ptIn);
			pt.m_z = max(0.0, pt.m_z);
			pt.m_z += groundAlt;

			CGeoPointIndex xy1 = get_xy(pt, UTCTRef);

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
							size_t L = get_level(xy2, pt, UTCTRef, z == 0);
							if (L < MAX_GEOH)
							{
								((CGeoPoint&)(*cuboids)[i].m_pt[z][y][x]) = extents.GetPixelExtents(xy2).GetCentroid();//Get the center of the cell

								size_t bGph = m_p_weather_DS.get_band(UTCTRef, ATM_HGT, L);
								double gph = m_p_weather_DS.GetPixel(UTCTRef, bGph, xy2); //geopotential height [m]
								(*cuboids)[i].m_pt[z][y][x].m_z = gph - groundAlt;//groundAlt is equal 0 when is over ground

								for (size_t v = 0; v < ATM_WATER; v++)
								{

									bool bConvertVVEL = false;
									size_t b = m_p_weather_DS.get_band(UTCTRef, v, L);

									if (b == UNKNOWN_POS && v == ATM_WNDW)
									{
										b = m_p_weather_DS.get_band(UTCTRef, ATM_VVEL, L);
										if (b != NOT_INIT)
											bConvertVVEL = true;
									}


									if (b != UNKNOWN_POS)
									{
										(*cuboids)[i][z][y][x][v] = m_p_weather_DS.GetPixel(UTCTRef, b, xy2);
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


	string CATMWeather::get_image_filepath(CTRef TRef)const
	{
		string filePath;
		TRef.Transform(CTM(CTM::HOURLY));
		TRefFilePathMap::const_iterator it = m_filepath_map.find(TRef);
		if (it != m_filepath_map.end())
		{
			filePath = GetAbsolutePath(GetPath(m_filePathGribs), it->second);
		}

		return filePath;
	}

	ERMsg CATMWeather::load_gribs(const std::string& filepath, CCallback& callback)
	{
		ERMsg msg;

		ifStream file;
		msg = file.open(filepath);
		if (msg)
		{
			//init max image to load at the sime time
			m_p_weather_DS.m_max_hour_load = m_world.m_parameters2.m_max_hour_load;
			m_p_weather_DS.m_clipRect = CGeoRect(-84, 40, -56, 56, PRJ_WGS_84);// m_world.m_parameters2.m_clipRect;


			std::ios::pos_type length = file.length();
			callback.PushTask("Load Gribs", length);

			for (CSVIterator loop(file); loop != CSVIterator() && msg; ++loop)
			{
				if ((*loop).size() == 2)
				{
					CTRef TRef;
					TRef.FromFormatedString((*loop)[0], "", "-", 1);
					assert(TRef.IsValid());

					m_filepath_map[TRef] = (*loop)[1];
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

	bool CATMWeather::IsLoaded(CTRef TRef)const
	{
		bool bIsLoaded = true;
		if (!m_filePathGribs.empty())
			bIsLoaded = m_p_weather_DS.IsLoaded(TRef);

		if (bIsLoaded && !m_filePathHDB.empty())
			bIsLoaded = m_iwd.find(TRef) != m_iwd.end();

		return bIsLoaded;
	}

	ERMsg CATMWeather::LoadWeather(CTRef UTCTRef, CCallback& callback)
	{
		ERMsg msg;

		if (!m_filePathGribs.empty())
		{
			if (!m_p_weather_DS.IsLoaded(UTCTRef))
			{
				string filePath = get_image_filepath(UTCTRef);
				if (!filePath.empty())
				{
					msg = m_p_weather_DS.load(UTCTRef, get_image_filepath(UTCTRef), callback);
				}
				else
				{
					//replace image by the nearest image
					CTRef nearestImage;
					for (size_t h = 0; h < 4 && !nearestImage.IsInit(); h++)
					{
						int i = int(pow(-1, h + 1)*(h + 2) / 2);
						string filePath = get_image_filepath(UTCTRef + i);
						if (!filePath.empty() && m_p_weather_DS.load(UTCTRef, filePath, callback))
						{
							nearestImage = UTCTRef + i;
						}
					}

					if (nearestImage.IsInit())
					{
						//add a warning and remove error
						callback.AddMessage("WARNING: File for " + UTCTRef.GetFormatedString("%Y-%m-%d-%H") + " (UTC) is missing. Was replace by " + nearestImage.GetFormatedString("%Y-%m-%d-%H") + " (UTC)");
						//msg = ERMsg();

						//load firstGoodImage into UTCTRef
						msg = m_p_weather_DS.load(UTCTRef, get_image_filepath(nearestImage), callback);
					}
					else
					{
						msg.ajoute("Skip day");
						return msg;
					}

				}
				msg += callback.StepIt(0);

				if (msg && !m_bHgtOverSeaTested)
				{
					size_t b = m_p_weather_DS.get_band(UTCTRef, ATM_HGT, 0);
					if (b != -999)
					{
						m_bHgtOverSeaTested = true;
						//test to see if we have special WRL file with HGT over ground and not over sea level
						for (int x = 0; x < m_p_weather_DS.Get(UTCTRef)->GetRasterXSize() && !m_bHgtOverSea; x++)
						{
							//for (int y = 0; y < m_p_weather_DS.Get(UTCTRef)->GetRasterYSize() && !m_bHgtOverSea; y++)
							if (x < m_p_weather_DS.Get(UTCTRef)->GetRasterYSize())
							{
								double elev = m_p_weather_DS.Get(UTCTRef)->GetPixel(b, CGeoPointIndex(x, x));
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


			if (m_iwd.find(UTCTRef) == m_iwd.end())//if not already loaded
			{
				static const char* FILTER_STR[NB_ATM_VARIABLES] = { "T", "T", "P", "WS WD", "WS WD", "T", "T" };
				int year = UTCTRef.GetYear();


				//search all station used
				set<size_t> indexes;
				for (size_t v = 0; v < NB_ATM_VARIABLES&&msg; v++)
				{
					for (auto it = m_world.m_flyers.begin(); it != m_world.m_flyers.end() && msg; it++)
					{
						CSearchResultVector result;
						msg = m_p_hourly_DB->Search(result, it->m_newLocation, m_world.m_parameters2.m_nb_weather_stations * 5, -1, CWVariables(FILTER_STR[v]), year);
						for (size_t ss = 0; ss < result.size(); ss++)
							indexes.insert(result[ss].m_index);

						msg += callback.StepIt(0);
					}
				}


				//pre-load weather
				for (set<size_t>::const_iterator it = indexes.begin(); it != indexes.end() && msg; it++)
				{
					size_t index = *it;

					//load station in memory
					CWeatherStation& station = m_stations[index];
					msg += m_p_hourly_DB->Get(station, index, year);
					ASSERT(m_stations.find(index) != m_stations.end());

					m_Twater[index].Compute(station);//compute water temperature
					msg += callback.StepIt(0);
				}

				//create IWD object
				for (size_t v = 0; v < NB_ATM_VARIABLES&&msg; v++)
				{
					CGridPointVectorPtr pts(new CGridPointVector);

					set<size_t> indexes;
					for (auto it = m_world.m_flyers.begin(); it != m_world.m_flyers.end() && msg; it++)
					{
						CSearchResultVector result;
						msg = m_p_hourly_DB->Search(result, it->m_newLocation, m_world.m_parameters2.m_nb_weather_stations * 5, -1, CWVariables(FILTER_STR[v]), year);
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



					CGridInterpolParam param;
					param.m_IWDModel = CGridInterpolParam::IWD_CLASIC;
					param.m_power = CGridInterpolParam::IWD_POWER;
					param.m_nbPoints = m_world.m_parameters2.m_nb_weather_stations;
					param.m_bGlobalLimit = false;
					param.m_bGlobalLimitToBound = false;
					param.m_maxDistance = 1000000;
					param.m_bUseElevation = false;

					ASSERT(pts->size() >= m_world.m_parameters2.m_nb_weather_stations);
					if (pts->size() < m_world.m_parameters2.m_nb_weather_stations)
					{
						if (v == ATM_PRCP)
						{
							callback.AddMessage("WARNING: Not enaught stations with precipitation. replaced by zero.");
							while (pts->size() < m_world.m_parameters2.m_nb_weather_stations)
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

	size_t CATMWeather::GetGribsPrjID(CTRef TRef)const
	{
		size_t prjID = NOT_INIT;

		if (!m_p_weather_DS.IsLoaded(TRef))
			m_p_weather_DS.load(TRef, get_image_filepath(TRef), CCallback());

		prjID = m_p_weather_DS.GetPrjID(TRef);

		return prjID;
	}

	//*********************************************************************************************************
	CTRefDatasetMap::CTRefDatasetMap()
	{
		m_max_hour_load = MAX_NUMBER_IMAGE_LOAD;
	}

	ERMsg CTRefDatasetMap::load(CTRef TRef, const string& filePath, CCallback& callback)const
	{
		ERMsg msg;
		CTRefDatasetMap& me = const_cast<CTRefDatasetMap&>(*this);

		me[TRef].reset(new CGDALDatasetCached);
		msg = me[TRef]->OpenInputImage(filePath, true);

		return msg;
	}

	ERMsg CTRefDatasetMap::Discard(CCallback& callback)
	{
		ERMsg msg;

		if (!empty())
		{
			callback.PushTask("Discard weather for " + begin()->first.GetFormatedString("%Y-%m-%d") + " (nbImages=" + ToString(size()) + ")", size());

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

	double CTRefDatasetMap::GetPixel(CTRef TRef, const CGeoPoint3DIndex& index)const
	{
		ASSERT(at(TRef));

		double pixel = at(TRef)->GetPixel(index);
		return pixel;
	}

	const CGeoExtents& CTRefDatasetMap::GetExtents(CTRef TRef)const
	{
		ASSERT(at(TRef));
		ASSERT(at(TRef)->IsOpen());

		const CGeoExtents& extents = at(TRef)->GetExtents();
		return extents;
	}

	bool CTRefDatasetMap::IsLoaded(CTRef TRef)const
	{
		bool bRep = false;
		if (find(TRef) != end())
			bRep = at(TRef)->IsOpen();

		return bRep;
	}

	CGDALDatasetCachedPtr& CTRefDatasetMap::Get(CTRef TRef)
	{
		ASSERT(at(TRef));


		CGDALDatasetCachedPtr& dataset = at(TRef);
		return dataset;

	}

	size_t CTRefDatasetMap::get_band(CTRef TRef, size_t v, size_t level)const
	{
		size_t band = UNKNOWN_POS;

		band = at(TRef)->get_band(v, level);
		return band;
	}

	bool CTRefDatasetMap::get_fixed_elevation_level(CTRef TRef, size_t l, double& level)const
	{
		return at(TRef)->get_fixed_elevation_level(l, level);
	}

	//******************************************************************************************************
	const char* CATMWorldParamters::MEMBERS_NAME[NB_MEMBERS] = { "WeatherType", "Period", "TimeStep", "Seed", "UseSpaceInterpol", "UseTimeInterpol", "UsePredictorCorrectorMethod", "UseVerticalVelocity", "MaximumFlyers", "MaximumFlights", "DEM", "WaterLayer", "Gribs", "HourlyDB", "Defoliation", "OutputSubHourly", "OutputFileTitle", "OutputFrequency", "CreateEggsMap", "EggsMapTitle", "EggsMapRes" };


	std::set<int> CATMWorld::get_years()const
	{
		std::set<int> years;

		for (CFlyersCIt it = m_flyers.begin(); it != m_flyers.end(); it++)
			years.insert(it->m_localTRef.GetYear());

		return years;
	}

	CTPeriod CATMWorld::get_period(bool bUTC, int year)const
	{
		CTPeriod p;
		CStatistic maxDuration;
		for (CFlyersCIt it = m_flyers.begin(); it != m_flyers.end(); it++)
		{
			if (year == YEAR_NOT_INIT || it->m_localTRef.GetYear() == year)
			{
				CTRef TRef = it->m_localTRef;//local time
				if (bUTC)
					TRef -= (int)(it->m_location.m_lon / 15);

				p += TRef;
			}
		}

		if (p.IsInit())
		{
			p.End() += 12;// add 12 hours for flight
		}

		return p;
	}

	set<CTRef> CATMWorld::get_TRefs(int year)const
	{
		set<CTRef> TRefs;
		for (CFlyersCIt it = m_flyers.begin(); it != m_flyers.end(); it++)
		{
			if (year == YEAR_NOT_INIT || it->m_localTRef.GetYear() == year)
			{
				ASSERT(it->m_localTRef.GetTM().Type() == CTM::HOURLY);

				CTRef TRef = it->m_localTRef;//local time 
				if (TRef.m_hour < 12)//if liftoff after midnight, take day before
					TRef.m_hour -= 12;

				TRef.Transform(CTM(CTM::DAILY));
				TRefs.insert(TRef);
			}
		}

		return TRefs;
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

	const CProjectionTransformation& CATMWorld::GetFromWeatherTransfo(CTRef UTCRef)const
	{
		size_t prjID = m_weather.GetGribsPrjID(UTCRef);
		ASSERT(prjID != NOT_INIT);

		return m_2GEO.at(prjID);

	}


	const CProjectionTransformation& CATMWorld::GetToWeatherTransfo(CTRef UTCRef)const
	{
		size_t prjID = m_weather.GetGribsPrjID(UTCRef);
		ASSERT(prjID != NOT_INIT);

		return m_GEO2.at(prjID);
	}
	//TRef is local
	vector<CFlyersIt> CATMWorld::GetFlyers(CTRef localTRef2)
	{
		CTPeriod currentPeriod(localTRef2, localTRef2 + 1);
		currentPeriod.Transform(CTM::HOURLY);
		currentPeriod.Begin().m_hour = 14;
		currentPeriod.End().m_hour = 13;

		vector<CFlyersIt> fls;
		for (CFlyersIt it = m_flyers.begin(); it != m_flyers.end(); it++)
		{
			CTRef Liftoff = it->m_localTRef;
			if (currentPeriod.IsInside(Liftoff))
			{
				ASSERT(it->m_flightNo == 0);
				fls.push_back(it);
			}
		}


		if (m_parameters1.m_maxFliyers > 0)
		{
			while (fls.size() > m_parameters1.m_maxFliyers)
			{
				size_t i = m_random.Rand(0, int(fls.size() - 1));
				fls[i]->DestroyByOptimisation();
				fls.erase(fls.begin() + i);
			}
		}

		return fls;
	}

	CTPeriod CATMWorld::get_UTC_period(const vector<CFlyersIt>& fls)
	{
		CTPeriod UTCp;

		for (size_t i = 0; i < fls.size(); i++)
		{
			__int64  UTCLiftoff = (__int64)floor(fls[i]->m_liffoff_time);
			__int64  UTCLanding = (__int64)ceil(UTCLiftoff + fls[i]->m_duration);
			UTCp += CTimeZones::UTCTime2UTCTRef(UTCLiftoff);
			UTCp += CTimeZones::UTCTime2UTCTRef(UTCLanding) + 1 + 1;//add 2 extra hours for landing and cuboid
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
		assert(m_weather.is_init());
		assert(!m_flyers.empty());
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

		//if (m_host_DS.IsOpen())
		//{
		//	size_t prjID = m_host_DS.GetPrjID(); ASSERT(prjID != NOT_INIT);
		//	if (m_GEO2.find(prjID) == m_GEO2.end())
		//		const_cast<CATMWorld&>(*this).m_GEO2[prjID] = GetReProjection(PRJ_WGS_84, prjID);
		//	if (m_2GEO.find(prjID) == m_2GEO.end())
		//		const_cast<CATMWorld&>(*this).m_2GEO[prjID] = GetReProjection(prjID, PRJ_WGS_84);
		//}

		size_t prjID = m_DEM_DS.GetPrjID(); ASSERT(prjID != NOT_INIT);
		if (m_GEO2.find(prjID) == m_GEO2.end())
			const_cast<CATMWorld&>(*this).m_GEO2[prjID] = GetReProjection(PRJ_WGS_84, prjID);
		if (m_2GEO.find(prjID) == m_2GEO.end())
			const_cast<CATMWorld&>(*this).m_2GEO[prjID] = GetReProjection(prjID, PRJ_WGS_84);

		random().Randomize(m_parameters1.m_seed);

		const CGeoExtents& extent = m_DEM_DS.GetExtents();

		if (output_file.is_open())
		{
			//write file header

			output_file << "l,p,r,Year,Month,Day,Hour,Minute,Second,";
			output_file << "flight,scale,sex,A,M,G,EggsLaid,state,x,y,lat,lon,";
			output_file << "T,P,U,V,W,";
			output_file << "MeanHeight,CurrentHeight,DeltaHeight,HorizontalSpeed,VerticalSpeed,Direction,Distance,DistanceFromOrigine,Defoliation" << endl;
		}



		static const double ms2kmh = 3600.0 / 1000.0;

		//get period of simulation
		CTPeriod period = m_parameters1.m_simulationPeriod;

		size_t nbTotalFlight = 0;
		size_t nbRealFlight = 0;

		callback.PushTask("Execute dispersal for year = " + ToString(period.Begin().GetYear()) + " (" + ToString(period.GetNbDay()) + " days)", period.GetNbDay());
		//simulate for all days
		for (CTRef TRef = period.Begin(); TRef <= period.End() && msg; TRef++)
		{
			//get all flyers for this day
			vector<CFlyersIt> fls = GetFlyers(TRef);
			nbTotalFlight += fls.size();

			//init all flyers
			for (size_t i = 0; i < fls.size() && msg; i++)
				fls[i]->init();

			//get simulation hours for this day
			CTPeriod UTC_period = get_UTC_period(fls);
			//Simulate dispersal

			callback.AddMessage("Dispersal for " + TRef.GetFormatedString("%Y-%m-%d") + " (nb flyers = " + ToString(fls.size()) + ")");




			if (UTC_period.IsInit())
			{

				int nbSteps = 0;
				for (CTRef UTCTRef = UTC_period.Begin(); UTCTRef <= UTC_period.End() && msg; UTCTRef++)
					if (!m_weather.IsLoaded(UTCTRef))
						nbSteps++;


				callback.PushTask("Load weather for " + TRef.GetFormatedString("%Y-%m-%d") + " (nbImages=" + ToString(nbSteps) + ")", nbSteps);

				ERMsg msgLoad;
				//pre-Load weather for the day
				for (CTRef UTCTRef = UTC_period.Begin(); UTCTRef <= UTC_period.End() && msgLoad; UTCTRef++)
				{
					if (!m_weather.IsLoaded(UTCTRef))
					{
						msgLoad += m_weather.LoadWeather(UTCTRef, callback);
						if (msgLoad)
						{
							size_t prjID = m_weather.GetGribsPrjID(UTCTRef); ASSERT(prjID != NOT_INIT);
							if (m_GEO2.find(prjID) == m_GEO2.end())
								m_GEO2[prjID] = GetReProjection(PRJ_WGS_84, prjID);
							if (m_2GEO.find(prjID) == m_2GEO.end())
								m_2GEO[prjID] = GetReProjection(prjID, PRJ_WGS_84);
						}
					}

					msgLoad += callback.StepIt();
				}

				callback.PopTask();

				if (!msgLoad)
				{
					callback.AddMessage("WARNING: too much Gribs missing. Nightly flight for " + TRef.GetFormatedString("%Y-%m-%d") + " was skipped");

					if (callback.GetUserCancel())
						msg += callback.StepIt(0);

					continue;
				}

				
				//code to compare with Gary results
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



				callback.PushTask("Dispersal for " + TRef.GetFormatedString("%Y-%m-%d") + " (nb flyers = " + ToString(fls.size()) + ")", UTC_period.size()*fls.size());

				for (m_UTCTTime = CTimeZones::UTCTRef2UTCTime(m_UTCTRef = UTC_period.Begin()); m_UTCTRef <= UTC_period.End() && msg; m_UTCTRef++, m_UTCTTime += 3600)
				{
#pragma omp parallel for if (m_parameters1.m_weather_type == CATMWorldParamters::FROM_GRIBS && !output_file.is_open() )
					for (__int64 i = 0; i < (__int64)fls.size(); i++)
					{
#pragma omp flush(msg)
						if (msg)
						{
							CFlyer& flyer = *(fls[i]);

							for (size_t seconds = 0; seconds < 3600; seconds += get_time_step())
							{
								__int64 UTCTTime = m_UTCTTime + seconds;
								flyer.live(m_UTCTRef, UTCTTime);

								__int64 countdown1 = UTCTTime - flyer.m_liffoff_time;
								__int64 countdown2 = flyer.GetLog(CFlyer::T_IDLE_END)>0 ? UTCTTime - flyer.GetLog(CFlyer::T_IDLE_END) : 0;
								size_t state = (flyer.GetState() == CFlyer::IDLE_END) ? flyer.GetEnd() : flyer.GetState();
								CTRef localTRef = m_UTCTRef + int(flyer.GetUTCShift() / 3600);


								if (seconds == 0 &&
									countdown1 >= -3600 &&
									countdown2 <= 3600)
								{
									if (output[flyer.m_loc][flyer.m_par][flyer.m_rep].IsInside(localTRef))
									{
										double alpha = atan2(flyer.GetStat(CFlyer::HOURLY_STAT, CFlyer::S_D_Y), flyer.GetStat(CFlyer::HOURLY_STAT, CFlyer::S_D_X));
										double angle = int(360 + 90 - Rad2Deg(alpha)) % 360;
										ASSERT(angle >= 0 && angle <= 360);
										double Dᵒ = flyer.m_newLocation.GetDistance(flyer.m_location, false, false);

										size_t liftoffTime = CTimeZones::UTCTime2LocalTime(flyer.GetLog(CFlyer::T_LIFTOFF), flyer.m_location);
										size_t landingTime = CTimeZones::UTCTime2LocalTime(flyer.GetLog(CFlyer::T_LANDING), flyer.m_location);

										CGeoPoint3D pt = flyer.m_pt;

										pt.Reproject(m_GEO2.at(prjID));//convert from GEO to DEM projection

										bool bOverWater = is_over_water(flyer.m_newLocation);
										double defoliation = VMISS;
										if (!bOverWater && ((flyer.m_flightNo == 0 && state == 0) || state >= 10))
											defoliation = get_defoliation(flyer.m_newLocation);
										double broods = VMISS;
										if (flyer.m_sex == CATMParameters::FEMALE && !bOverWater && ((flyer.m_flightNo == 0 && state == 0) || state >= 10))
											broods = flyer.m_broods;


										output[flyer.m_loc][flyer.m_par][flyer.m_rep][localTRef][ATM_FLIGHT] = flyer.m_flightNo;
										output[flyer.m_loc][flyer.m_par][flyer.m_rep][localTRef][ATM_SCALE] = flyer.m_scale;
										output[flyer.m_loc][flyer.m_par][flyer.m_rep][localTRef][ATM_SEX] = flyer.m_sex;
										output[flyer.m_loc][flyer.m_par][flyer.m_rep][localTRef][ATM_A] = flyer.m_A;
										output[flyer.m_loc][flyer.m_par][flyer.m_rep][localTRef][ATM_M] = flyer.m_M;
										output[flyer.m_loc][flyer.m_par][flyer.m_rep][localTRef][ATM_G] = flyer.m_G;
										output[flyer.m_loc][flyer.m_par][flyer.m_rep][localTRef][ATM_EGGS_LAID] = broods;
										output[flyer.m_loc][flyer.m_par][flyer.m_rep][localTRef][ATM_STATE] = state;
										output[flyer.m_loc][flyer.m_par][flyer.m_rep][localTRef][ATM_X] = pt.m_x;
										output[flyer.m_loc][flyer.m_par][flyer.m_rep][localTRef][ATM_Y] = pt.m_y;
										output[flyer.m_loc][flyer.m_par][flyer.m_rep][localTRef][ATM_LAT] = flyer.m_newLocation.m_lat;
										output[flyer.m_loc][flyer.m_par][flyer.m_rep][localTRef][ATM_LON] = flyer.m_newLocation.m_lon;
										output[flyer.m_loc][flyer.m_par][flyer.m_rep][localTRef][ATM_T] = flyer.GetStat(CFlyer::HOURLY_STAT, CFlyer::S_TAIR);
										output[flyer.m_loc][flyer.m_par][flyer.m_rep][localTRef][ATM_P] = flyer.GetStat(CFlyer::HOURLY_STAT, CFlyer::S_PRCP);
										output[flyer.m_loc][flyer.m_par][flyer.m_rep][localTRef][ATM_U] = flyer.GetStat(CFlyer::HOURLY_STAT, CFlyer::S_U, ms2kmh);
										output[flyer.m_loc][flyer.m_par][flyer.m_rep][localTRef][ATM_V] = flyer.GetStat(CFlyer::HOURLY_STAT, CFlyer::S_V, ms2kmh);
										output[flyer.m_loc][flyer.m_par][flyer.m_rep][localTRef][ATM_W] = flyer.GetStat(CFlyer::HOURLY_STAT, CFlyer::S_W, ms2kmh);
										output[flyer.m_loc][flyer.m_par][flyer.m_rep][localTRef][ATM_DEFOLIATION] = bOverWater ? -1 : defoliation;

										//size_t time = 0;
										if (flyer.GetLog(CFlyer::T_LIFTOFF) > 0)
										{


											output[flyer.m_loc][flyer.m_par][flyer.m_rep][localTRef][ATM_MEAN_HEIGHT] = flyer.GetStat(CFlyer::HOURLY_STAT, CFlyer::S_HEIGHT);
											output[flyer.m_loc][flyer.m_par][flyer.m_rep][localTRef][ATM_CURRENT_HEIGHT] = flyer.m_pt.m_z;
											output[flyer.m_loc][flyer.m_par][flyer.m_rep][localTRef][ATM_DELTA_HEIGHT] = flyer.GetStat(CFlyer::HOURLY_STAT, CFlyer::S_D_Z, 1, SUM);
											output[flyer.m_loc][flyer.m_par][flyer.m_rep][localTRef][ATM_W_HORIZONTAL] = flyer.GetStat(CFlyer::HOURLY_STAT, CFlyer::S_W_HORIZONTAL, ms2kmh);
											output[flyer.m_loc][flyer.m_par][flyer.m_rep][localTRef][ATM_W_VERTICAL] = flyer.GetStat(CFlyer::HOURLY_STAT, CFlyer::S_W_VERTICAL, ms2kmh);
											output[flyer.m_loc][flyer.m_par][flyer.m_rep][localTRef][ATM_DIRECTION] = angle;
											output[flyer.m_loc][flyer.m_par][flyer.m_rep][localTRef][ATM_DISTANCE] = flyer.GetStat(CFlyer::HOURLY_STAT, CFlyer::S_DISTANCE, 1, SUM);
											output[flyer.m_loc][flyer.m_par][flyer.m_rep][localTRef][ATM_DISTANCE_FROM_OIRIGINE] = Dᵒ;


											if (flyer.GetLog(CFlyer::T_LANDING) > 0)
											{
												output[flyer.m_loc][flyer.m_par][flyer.m_rep][localTRef][ATM_LIFTOFF_TIME] = CTimeZones::GetDecimalHour(liftoffTime);
												output[flyer.m_loc][flyer.m_par][flyer.m_rep][localTRef][ATM_FLIGHT_TIME] = (landingTime - liftoffTime) / 3600.0;
												output[flyer.m_loc][flyer.m_par][flyer.m_rep][localTRef][ATM_LANDING_TIME] = CTimeZones::GetDecimalHour(landingTime);
											}

										}//log exists
									}//if output

									flyer.ResetStat(CFlyer::HOURLY_STAT);
								}//if seconds == 0 and output stat


								//output sub-hourly data
								if (output_file.is_open() &&
									seconds%m_parameters1.m_outputFrequency == 0 &&
									countdown1 >= -m_parameters1.m_outputFrequency &&
									countdown2 <= m_parameters1.m_outputFrequency)//|| flyer.GetStat(CFlyer::HOURLY_STAT, CFlyer::S_TAIR) != -999)
								{
									CGeoPoint3D pt = flyer.m_pt;

									pt.Reproject(m_GEO2.at(prjID));//convert from GEO to DEM projection

									double alpha = atan2(flyer.GetStat(CFlyer::SUB_HOURLY_STAT, CFlyer::S_D_Y), flyer.GetStat(CFlyer::SUB_HOURLY_STAT, CFlyer::S_D_X));
									double angle = int(360 + 90 - Rad2Deg(alpha)) % 360;
									ASSERT(angle >= 0 && angle <= 360);
									double Dᵒ = flyer.m_newLocation.GetDistance(flyer.m_location, false, false);

									double defoliation = -999;
									double broods = -999;
									if (flyer.GetLog(CFlyer::T_IDLE_END) > 0)
									{
										bool bOverWater = is_over_water(flyer.m_newLocation);
										//defoliation = bOverWater?-1:get_defoliation(flyer.m_newLocation);
										if (!bOverWater && ((flyer.m_flightNo == 0 && state == 0) || state >= 10))
											defoliation = get_defoliation(flyer.m_newLocation);

										if (flyer.m_sex == CATMParameters::FEMALE &&!bOverWater && ((flyer.m_flightNo == 0 && state == 0) || state >= 10))
											broods = flyer.m_broods;
									}



#pragma omp critical (SAVE_SUB_HOURLY)
									{
										size_t minutes = size_t(seconds / 60);
										output_file << flyer.m_loc + 1 << "," << flyer.m_par + 1 << "," << flyer.m_rep + 1 << ",";
										output_file << localTRef.GetYear() << "," << localTRef.GetMonth() + 1 << "," << localTRef.GetDay() + 1 << "," << localTRef.GetHour() << "," << minutes << "," << seconds - 60 * minutes << ",";
										output_file << flyer.m_flightNo << "," << flyer.m_scale << "," << flyer.m_sex << "," << flyer.m_A << "," << flyer.m_M << "," << flyer.m_G << "," << broods << "," << state << "," << pt.m_x << "," << pt.m_y << "," << flyer.m_newLocation.m_lat << "," << flyer.m_newLocation.m_lon << ",";
										output_file << flyer.GetStat(CFlyer::SUB_HOURLY_STAT, CFlyer::S_TAIR) << "," << flyer.GetStat(CFlyer::SUB_HOURLY_STAT, CFlyer::S_PRCP) << ",";
										output_file << flyer.GetStat(CFlyer::SUB_HOURLY_STAT, CFlyer::S_U, ms2kmh) << "," << flyer.GetStat(CFlyer::SUB_HOURLY_STAT, CFlyer::S_V, ms2kmh) << "," << flyer.GetStat(CFlyer::SUB_HOURLY_STAT, CFlyer::S_W, ms2kmh) << ",";

										if (flyer.GetLog(CFlyer::T_LIFTOFF) > 0)
										{
											//log exists
											output_file << flyer.GetStat(CFlyer::SUB_HOURLY_STAT, CFlyer::S_HEIGHT) << "," << flyer.m_pt.m_z << "," << flyer.GetStat(CFlyer::SUB_HOURLY_STAT, CFlyer::S_D_Z, 1, SUM) << ",";
											output_file << flyer.GetStat(CFlyer::SUB_HOURLY_STAT, CFlyer::S_W_HORIZONTAL, ms2kmh) << "," << flyer.GetStat(CFlyer::SUB_HOURLY_STAT, CFlyer::S_W_VERTICAL, ms2kmh) << ",";
											output_file << angle << "," << flyer.GetStat(CFlyer::SUB_HOURLY_STAT, CFlyer::S_DISTANCE, 1, SUM) << "," << Dᵒ << ",";
										}
										else
										{
											output_file << "-999,-999,-999,-999,-999,-999,-999,-999,";
										}


										output_file << defoliation;
										output_file << endl;
									}
									flyer.ResetStat(CFlyer::SUB_HOURLY_STAT);
								}//if sub-hourly output 
							}//for all time step

							//callback.WaitPause();
							msg += callback.StepIt();
						}//if msg
					}//for all flyers
				}//for alltime steps

				callback.PopTask();

				msg += m_weather.Discard(callback);

				//lay eggs and reshedule flyers 

				CTRef tomorrow = TRef + 1;
				for (auto it = fls.begin(); it != fls.end(); it++)
				{
					if ((*it)->GetLog(CFlyer::T_LIFTOFF) > 0)
						nbRealFlight++;

					bool bOverWater = is_over_water((*it)->m_newLocation);
					if (!bOverWater)
					{
						CTRef Liftoff = (*it)->m_localTRef;
						bool bOverDefol = is_over_defoliation((*it)->m_newLocation);

						if (bOverDefol && m_parameters1.m_maxFlights > 1 && (*it)->m_flightNo < m_parameters1.m_maxFlights)
						{
							if (TRef < period.End())//less than 3 flights)
							{
								//il y a un problème ici car on ne change pas l'heure du départ en fonction de la temperature.
								//here we have to adjust liftoff hour...
								//it->m_localTRef += 24;
								//to avoid problem with insect flight past midnight, we remove 12 hours chnage the date and add 12 hours.
								(*it)->m_localTRef -= 12;
								(*it)->m_localTRef.m_month = tomorrow.m_month;
								(*it)->m_localTRef.m_day = tomorrow.m_day;
								(*it)->m_localTRef += 12;
							}
						}

					}//not over water
				}//for all flyers, update egg and shedule new flight
			}//have flyers



			msg += callback.StepIt();
		}//for all valid days


		callback.AddMessage("Total flights = " + ToString(nbTotalFlight));
		callback.AddMessage("Real flights = " + ToString(nbRealFlight));
		callback.AddMessage("Re-flight = " + ToString(nbTotalFlight - m_flyers.size()));

		return msg;
	}

	ERMsg CATMWorld::Init(ofStream& output_file, CCallback& callback)
	{
		ASSERT(m_DEM_DS.IsOpen());
		assert(m_weather.is_init());
		assert(!m_flyers.empty());
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

		/*if (m_host_DS.IsOpen())
		{
			size_t prjID = m_host_DS.GetPrjID(); ASSERT(prjID != NOT_INIT);
			if (m_GEO2.find(prjID) == m_GEO2.end())
				const_cast<CATMWorld&>(*this).m_GEO2[prjID] = GetReProjection(PRJ_WGS_84, prjID);
			if (m_2GEO.find(prjID) == m_2GEO.end())
				const_cast<CATMWorld&>(*this).m_2GEO[prjID] = GetReProjection(prjID, PRJ_WGS_84);
		}*/

		size_t prjID = m_DEM_DS.GetPrjID(); ASSERT(prjID != NOT_INIT);
		if (m_GEO2.find(prjID) == m_GEO2.end())
			const_cast<CATMWorld&>(*this).m_GEO2[prjID] = GetReProjection(PRJ_WGS_84, prjID);
		if (m_2GEO.find(prjID) == m_2GEO.end())
			const_cast<CATMWorld&>(*this).m_2GEO[prjID] = GetReProjection(prjID, PRJ_WGS_84);


		random().Randomize(m_parameters1.m_seed);

		const CGeoExtents& extent = m_DEM_DS.GetExtents();

		if (output_file.is_open())
		{
			//write file header

			output_file << "l,p,r,Year,Month,Day,Hour,Minute,Second,";
			output_file << "flight,scale,sex,A,M,G,EggsLaid,state,x,y,lat,lon,";
			output_file << "T,P,U,V,W,";
			output_file << "MeanHeight,CurrentHeight,DeltaHeight,HorizontalSpeed,VerticalSpeed,Direction,Distance,DistanceFromOrigine" << endl;
		}




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
							if (output[l][p][r][t][ATM_EGGS_LAID]>0)
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

				extents.m_xSize = std::max(1, (int)ceil(fabs((extents.m_xMax - extents.m_xMin) / m_parameters1.m_eggMapsResolution)));
				extents.m_ySize = std::max(1, (int)ceil(fabs((extents.m_yMin - extents.m_yMax) / m_parameters1.m_eggMapsResolution)));

				extents.m_xMax = extents.m_xMin + extents.m_xSize*m_parameters1.m_eggMapsResolution;
				extents.m_yMin = extents.m_yMax - extents.m_ySize*m_parameters1.m_eggMapsResolution;

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
										if (output[l][p][r][t][ATM_EGGS_LAID]>0)
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
		enum TWRFVars{ WRF_PRES, WRF_HGHT, WRF_TAIR, WRF_WVMR, WRF_UWND, WRF_VWND, WRF_WWND, WRF_PRCP, NB_WRF_VARS };
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
		enum TWRFLEvels{ NB_WRF_LEVEL = 38 };
		enum TWRFVars{ WRF_PRES, WRF_HGHT, WRF_TAIR, WRF_UWND, WRF_VWND, WRF_WWND, WRF_RELH, WRF_PRCP, NB_WRF_VARS };


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

		for (CTRef UTCRef = begin; UTCRef <= end&&msg; UTCRef++)
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
							size_t b = s*NB_WRF_LEVEL + l;
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

