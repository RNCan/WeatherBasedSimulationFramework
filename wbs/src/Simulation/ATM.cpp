//******************************************************************************
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
#include "Simulation/ATM.h"

#include "WeatherBasedSimulationString.h"


using namespace std;

using namespace WBSF::HOURLY_DATA;
using namespace WBSF::WEATHER;

namespace WBSF
{

	static const int MAX_NUMBER_IMAGE_LOAD = 22;

static ERMsg TransformWRF2RUC(CCallback& callback);

size_t Hourly2ATM(size_t vv)
{
	size_t v = NOT_INIT;

	switch (vv)
	{
	case HOURLY_DATA::H_TAIR: v = ATM_TAIR; break;
	case HOURLY_DATA::H_TRNG: break;
	case HOURLY_DATA::H_PRCP: v = ATM_PRCP; break;
	case HOURLY_DATA::H_TDEW: break;
	case HOURLY_DATA::H_RELH: v = ATM_RH;  break;
	case HOURLY_DATA::H_WNDS: v = ATM_WNDU; break;
	case HOURLY_DATA::H_WNDD: v = ATM_WNDV; break;
	case HOURLY_DATA::H_SRAD: break;
	case HOURLY_DATA::H_PRES: v = ATM_PRES; break;
	case HOURLY_DATA::H_SNOW: break;
	case HOURLY_DATA::H_SNDH: break;
	case HOURLY_DATA::H_SWE:	 break;
	case HOURLY_DATA::H_ES:	 break;
	case HOURLY_DATA::H_EA:	 break;
	case HOURLY_DATA::H_VPD:	 break;
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
*     var p2 = p1.rhumbDestinationPoint(40300, 116.7); // p2.toString(): 50.9642°N, 001.8530°E
*/
CGeoPoint RhumbDestinationPoint(CGeoPoint pt, double distance, double bearing, double radius = 6371000)
{
	ASSERT(pt.IsGeographic());

	double δ = distance/radius; // angular distance in radians
	double φ1 = Deg2Rad(pt.m_lat);
	double λ1 = Deg2Rad(pt.m_lon);
	double θ = Deg2Rad(bearing);
	double Δφ = δ * cos(θ);
	double φ2 = φ1 + Δφ;
	// check for some daft bugger going past the pole, normalise latitude if so
	if (abs(φ2) > PI/2) 
		φ2 = φ2>0 ? PI - φ2 : -PI - φ2;

	double Δψ = log(tan(φ2 / 2 + PI / 4) / tan(φ1 / 2 + PI / 4));
	double q = abs(Δψ) > 10e-12 ? Δφ / Δψ : cos(φ1); // E-W course becomes ill-conditioned with 0/0
	double Δλ = δ*sin(θ) / q;
	double λ2 = λ1 + Δλ;

	λ2 = fmod(λ2 + 3 * PI,2 * PI) - PI; // normalise to -180..+180°

	return CGeoPoint(Rad2Deg(λ2), Rad2Deg(φ2), pt.GetPrjID());
};

CGeoPoint3D Geodesic2Geocentric(CGeoPoint3D pt)
{
	return CGeoPoint3D(pt(0), pt(1), pt(2), PRJ_GEOCENTRIC_BASE);
}

CGeoPoint3D UpdateCoordinate(CGeoPoint3D pt, const CGeoDistance3D& d)
{
	double distance = sqrt(d.m_x*d.m_x + d.m_y*d.m_y);
	double alpha = atan2(d.m_y, d.m_x);
	double bearing = fmod(360 + 90 - Rad2Deg(alpha), 360); 
	CGeoPoint pt2 = RhumbDestinationPoint(pt, distance, bearing);
	return CGeoPoint3D(pt2.m_x, pt2.m_y, pt.m_z + d.m_z, pt.GetPrjID());

	
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
enum TGeoH{ GEOH_0, GEOH_110, GEOH_323, GEOH_540, GEOH_762, GEOH_988, MAX_GEOH=10 };//max ~2000 m

//trouver les RAP de 2012/05 à actuel.
//http://soostrc.comet.ucar.edu/data/grib/rap/20130817/hybrid/

extern const char ATM_HEADER[] = "State|X|Y|Latitude|Longitude|Height|Scale|FlightSpeedAscent|FlightSpeedHorizontal|FlightSpeedDecent|FlightDirection|Distance|TotalDistance";


//p: pressure [Pa]
//t: temperature [°C]
//ω: vertical velocity [pa/s]
double CATMVariables::get_Uw(double p, double t, double ω)
{
	ASSERT(!_isnan(p) && !_isnan(t) && !_isnan(ω));

	static const double g = 9.80665;// [m/s²]
	static const double M = 0.028965338; //[kg/mol]
	static const double R = 8.314472; //[J/(mol•K)] //Pour l'air, r = (R/M) 8.314472 / 0.028965338 ≈ 287 J·kg-1·K-1

	double T = t + 273.15;//temperature in Kelvin
	
	double Uw = -(ω*R*T) / (M*g*p);

//At low altitudes above the sea level, the pressure decreases by about 1.2 kPa for every 100 meters.For higher altitudes within the troposphere, the following equation(the barometric formula) relates atmospheric pressure p to altitude h
//12 pa/m
	//double Uw2 = -ω / 12;
	//alt en m et Z en mbar


	return Uw;
}

const char* CATMParameters::MEMBERS_NAME[NB_MEMBERS] = { "Tmin", "Tmax", "Pmax", "Wmin", "LiftoffType", "LiftoffBegin", "LiftoffEnd", "LiftoffOffset", "DurationType", "DurationMean", "DurationSD", "HeightType", "HeightMin", "HeightMean", "HeightSD", "HeightMax", "Wascent", "WascentSD", "Whorzontal", "WhorzontalSD", "Wdescent", "WdescentSD", "WindStabilityType", "NbWeatherStations" };

size_t CATMWorld::get_t_liftoff_offset(double T)const
{
	double t_liftoff = 0;
	if (m_parameters2.m_t_liftoff_type == CATMParameters::OLD_TYPE)
	{
		static const double SIGMA_SQ = 6.1;
		static const double TSTART = 19.5;
		static const double TEND = 23.5;

		double tp = 12.67 + 0.33*T;
		if (tp < TSTART)
		{
			t_liftoff = TSTART;
		}
		else if (tp > TEND)
		{
			t_liftoff = TEND;
		}
		else
		{
			t_liftoff = m_random.RandNormal(tp, sqrt(SIGMA_SQ));
			while (t_liftoff<TSTART || t_liftoff>TEND)
				t_liftoff = m_random.RandNormal(tp, sqrt(SIGMA_SQ));
		}
	}
	else if(m_parameters2.m_t_liftoff_type == CATMParameters::NEW_TYPE)
	{
		double 	liftoff_μ = 0.190575425*T - 4.042102263;// +0.5;//+0.5 ajuted from radar data!
		double 	liftoff_σ = -0.044029363*T + 1.363107669;

		t_liftoff = m_random.RandNormal(liftoff_μ, liftoff_σ);
		while (t_liftoff<m_parameters2.m_t_liftoff_begin || t_liftoff>m_parameters2.m_t_liftoff_end)
			t_liftoff = m_random.RandNormal(liftoff_μ, liftoff_σ);

		//add offset correction
		t_liftoff += m_parameters2.m_t_liftoff_correction;
	}

	return size_t(t_liftoff * 3600.0);//liftoff offset in seconds
}


size_t CATMWorld::get_t_hunthing()const
{
	double t_hunting = 0;
	double ran = m_random.Randu(); //random value [0,1];
	int ran012 = m_random.Rand(2); //random value {0,1,2};

	if (ran012 == 0)
	{
		t_hunting = 0.5 + 0.5*ran;//first third hunts from 0.5 - 1.0 hr
	}
	else if (ran012 == 1)
	{
		t_hunting = 1.0 + 2.0*ran;// second third hunts from 1.0 - 3.0 hr
	}
	else
	{
		assert(ran012 == 2);
		t_hunting = 3.0 + 3.0*ran;// third third hunts from 3.0 - 6.0 hr
	}

	return size_t(t_hunting * 3600.0);//time of unting after litfoff
}

double CATMWorld::get_height()const
{
	double height = 0;

	if (m_parameters2.m_height_type == CATMParameters::OLD_TYPE)
	{
		//static const double SIGMA_SQ = 6.1;
		static const int NFT = 10;
		static const double HBOT = 0;
		static const double HTOP = 500;
		static const double DELTAH = (HTOP - HBOT) / NFT;
		//static const double FTOTAL = 836;
		//0+80+130 +200 +250 +100 +55 +9 +7 +5 +0
		//(80)/836
		//0,80,210,410,660,760,815,824,831,836
		//cumulative probabilty
		static const double ft[NFT + 1] = { 0.0 / 836.0, 80.0 / 836.0, 210.0 / 836.0, 410.0 / 836.0, 660.0 / 836.0, 760.0 / 836.0, 815.0 / 836.0, 824.0 / 836.0, 831.0 / 836.0, 836.0 / 836.0, 836 / 836 };
		//static const double h[NFT + 1] = { HBOT + 0 * DELTAH, HBOT + 1 * DELTAH, HBOT + 2 * DELTAH, HBOT + 3 * DELTAH, HBOT + 4 * DELTAH, HBOT + 5 * DELTAH, HBOT + 6 * DELTAH, HBOT + 7 * DELTAH, HBOT + 8 * DELTAH, HBOT + 9 * DELTAH, HBOT + 10 * DELTAH };

		
		double ran = m_random.Randu();
		size_t index = NFT;
		for (size_t k = 0; k < NFT; k++)
		{
			if (ran >= ft[k] && ran<ft[k+1])
			{
				index = k;
				break;
			}
		}

		//double bot = HBOT + index  * DELTAH;
		//double top = HBOT + (index +1) * DELTAH;
		double a1 = (ran - ft[index]) / (ft[index + 1] - ft[index]);
		//double a2 = (ft[index+1] - ran) / (ft[index + 1] - ft[index]);
		//height = a1*bot + a2*top;
		height = HBOT + (index + a1) * DELTAH;
	}
	else if (m_parameters2.m_height_type == CATMParameters::NEW_TYPE)
	{
		height = m_random.RandLogNormal(m_parameters2.m_height, m_parameters2.m_height_σ);
		while (height<m_parameters2.m_height_lo || height>m_parameters2.m_height_hi)
			height = m_random.RandLogNormal(m_parameters2.m_height, m_parameters2.m_height_σ);
	}

	return height;
}

size_t CATMWorld::get_duration()const
{
	double duration = 0;
	if (m_parameters2.m_duration_type== CATMParameters::OLD_TYPE)
	{
		duration = m_parameters2.m_duration + (m_random.Randu()-0.5)*m_parameters2.m_duration_σ;
	}
	else if (m_parameters2.m_duration_type == CATMParameters::NEW_TYPE)
	{
		duration = m_random.RandNormal(m_parameters2.m_duration, m_parameters2.m_duration_σ);
	}

	return size_t(duration * 3600.0); //duration in seconds
}

double CATMWorld::get_w_ascent()const
{
	double ran = m_random.Rand(-1.0, 1.0);//random value [-1,1];
	double w = max(0.0, m_parameters2.m_w_ascent + ran*m_parameters2.m_w_ascent_σ);
	ASSERT(w >= 0);

	return w*1000/3600;//convert from km/h to m/s
}

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
	//if (w > 0)
		//w *= -1;
//	ASSERT(w < 0);

	return w * 1000 / 3600;//convert from km/h to m/s
}


//***********************************************************************************************



CFlyer::CFlyer(CATMWorld& world):
	m_world(world)
{
	m_loc = 0;
	m_var = 0;
	m_scale = 0;
	m_state = NOT_CREATED;
	m_end_type = NO_END_DEFINE;
	m_creation_time = 0;
	m_log.fill(0);
//	m_strongFliyers = exp(m_world.random().Rand(-0.5, 0.5));
}


void CFlyer::init()
{
	size_t UTCTime = CTRef2Time(GetUTCTRef());
	size_t localSunset = m_world.get_local_sunset(m_TRef, m_location);
	size_t UTCSunset = LocalTime2UTCTime(localSunset, m_location.m_lon);

	CATMVariables w = m_world.get_weather(m_pt, UTCSunset);
	//CATMVariables w = m_world.get_weather(m_newLocation, UTCSunset);
	if (m_world.m_parameters2.m_t_liftoff_type == CATMParameters::OLD_TYPE)
		m_parameters.m_t_liftoff = UTCTime + m_world.get_t_liftoff_offset(w[ATM_TAIR]);
	else
		m_parameters.m_t_liftoff = UTCSunset + m_world.get_t_liftoff_offset(w[ATM_TAIR]);

	m_parameters.m_height = m_world.get_height();
	m_parameters.m_w_ascent = m_world.get_w_ascent();
	m_parameters.m_w_horizontal = m_world.get_w_horizontal();
	m_parameters.m_w_descent = m_world.get_w_descent();
	m_parameters.m_duration = m_world.get_duration();
	m_parameters.m_t_hunting = min(m_parameters.m_duration, m_world.get_t_hunthing());//attention ce n'est pa comme avant
}

void CFlyer::live()
{
	ResetStat();

	if (m_state == DESTROYED)
		return;
	

	CTRef UTCTRef = m_world.GetUTRef();
	CTRef localTRef = UTCTRef2LocalTRef(UTCTRef, m_location.m_lon);
	if (localTRef >= m_TRef )
	{
		for (size_t seconds = 0; seconds < 3600; seconds += m_world.get_time_step())
		{
			size_t UTCTime = m_world.get_UTC_time() + seconds;
			switch (m_state)
			{
			case NOT_CREATED:		create(UTCTime); break;
			case IDLE_BEGIN:		idle_begin(UTCTime); break;
			case LIFTOFF:			liftoff(UTCTime); break;
			case ASCENDING_FLIGHT:	ascent_flight(UTCTime); break;
			case HORIZONTAL_FLIGHT:	horizontal_flight(UTCTime); break;
			case DESCENDING_FLIGHT:	descent_flight(UTCTime); break;
			case LANDING:			landing(UTCTime); break;
			case IDLE_END:			idle_end(UTCTime);  break;
			case DESTROYED:			destroy(UTCTime);  break;
			default: assert(false);
			}
		}
	}
}



void CFlyer::create(size_t UTCTime)
{
	//init the object
	bool bFlight = false;
	if (m_world.is_over_defoliation(m_pt))
		bFlight = true;

	if (bFlight)
	{
		m_state = IDLE_BEGIN;
	}
	else
	{
		m_state = IDLE_END;
		m_end_type = NO_LIFTOFF;
	}

	m_log[T_CREATION] = m_world.get_UTC_time();
}

void CFlyer::idle_begin(size_t UTCTime)
{
	__int64 countdown = (__int64)UTCTime - m_parameters.m_t_liftoff;
	if (countdown >= 0 )
	{
		CATMVariables w = m_world.get_weather(m_pt, UTCTime);
		
		double ws = w.get_wind_speed(true) * 3600 / 1000;//tranform m/s -> km/h
		ASSERT(!IsMissing(w[ATM_TAIR]) && !IsMissing(w[ATM_PRCP]) && !IsMissing(w[ATM_WNDU]) && !IsMissing(w[ATM_WNDV]));

		if (w[ATM_PRCP] < m_world.m_parameters2.m_Pmax &&
			w[ATM_TAIR] > m_world.m_parameters2.m_Tmin &&
			w[ATM_TAIR] < m_world.m_parameters2.m_Tmax &&
			ws > m_world.m_parameters2.m_Wmin)
		{
			//update liftoff time
			m_state = LIFTOFF;
		}
		else
		{
			//flight abort
			m_state = IDLE_END;
			
			if (w[ATM_PRCP] > m_world.m_parameters2.m_Pmax)
				m_end_type = END_BY_RAIN;
			else if (w[ATM_TAIR] <= m_world.m_parameters2.m_Tmin || w[ATM_TAIR] >= m_world.m_parameters2.m_Tmax)
				m_end_type = END_BY_TAIR;
			else //(ws > m_world.m_parameters2.m_Wmin)
				m_end_type = END_BY_WNDS;
			
		}

		m_stat[S_TAIR] += w[ATM_TAIR];
		m_stat[S_PRCP] = w[ATM_PRCP];
		m_stat[S_W_ASCENT] += 0;	//ascent speed [m/s]
		m_stat[S_W_HORIZONTAL] += 0; //horizontal speed [m/s]
		m_stat[S_HEIGHT] += m_pt.m_z;	//flight height [m]
		m_stat[S_DIRECTION_X] += 0;
		m_stat[S_DIRECTION_Y] += 0;
		m_stat[S_DISTANCE] += 0;
	}
}


void CFlyer::liftoff(size_t UTCTime)
{
	m_log[T_LIFTOFF] = UTCTime;
	m_state = ASCENDING_FLIGHT;
}


CGeoDistance3D CFlyer::get_U(const CATMVariables& w)const
{
	ASSERT(!IsMissing(w[ATM_WNDV]) && !IsMissing(w[ATM_WNDU]));

	double alpha = 0;
	if (w[ATM_WNDV]!=0 || w[ATM_WNDU]!=0)
		alpha = atan2(w[ATM_WNDV], w[ATM_WNDU]);

	if (_isnan(alpha) || !_finite(alpha))
		alpha = 0;

	double Ux = (w[ATM_WNDU] + cos(alpha)*m_parameters.m_w_horizontal);	//[m/s]
	double Uy = (w[ATM_WNDV] + sin(alpha)*m_parameters.m_w_horizontal);	//[m/s]
	double Uz = 0;// m_world.m_parameters1.m_bUseVerticalVelocity ? w[ATM_WNDW] : 0;
	
	
	//
	switch (m_state)
	{
	case ASCENDING_FLIGHT:	Uz += w[ATM_WNDW]+m_parameters.m_w_ascent; break;	//[m/s]
	case HORIZONTAL_FLIGHT:	Uz = w[ATM_WNDW]; break;							//[m/s]
	case DESCENDING_FLIGHT:	Uz += w[ATM_WNDW]+m_parameters.m_w_descent; break;	//[m/s]
	default: assert(false);
	}

	//if (m_world.m_parameters1.m_bUseVerticalVelocity)

	ASSERT(Uz!=-999);
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


	//if (m_world.m_parameters1.m_bUseTurbulance)
	//{
	//	//double Δx = Signe(Ux)*m_world.random().Rand(0.0, 0.1);
	//	//double Δy = Signe(Uy)*m_world.random().Rand(0.0, 0.1);
	//	//Ux += Δx;
	//	//Uy += Δy;
	//	Ux *= (1 + m_world.random().Rand(-0.1, 0.1));
	//	Uy *= (1 + m_world.random().Rand(-0.1, 0.1));
	//	Uz *= (1 + m_world.random().Rand(-0.1, 0.1));
	//}

	return CGeoDistance3D(Ux, Uy, Uz, m_pt.GetPrjID());
}

CATMVariables CFlyer::get_weather(size_t UTCTime)const
{
	ASSERT(m_world.m_weather.IsLoaded(Time2CTRef(UTCTime)));
	ASSERT(m_pt.m_z>=0);

	CATMVariables w;


	//CGeoPoint3D pt° = m_pt;
	CGeoPoint3D pt° = m_pt;//m_newLocation;// m_pt;
	CATMVariables w° = m_world.get_weather(pt°, UTCTime);
	CGeoDistance3D U° = get_U(w°);

	double dt = m_world.get_time_step(); //[s]
	CGeoPoint3D pt¹ = UpdateCoordinate(/*m_newLocation*/m_pt, U°*dt);
	

	if (m_world.m_parameters1.m_bUsePredictorCorrectorMethod &&
		m_world.m_weather.IsLoaded(Time2CTRef(UTCTime + dt)) &&
		m_world.IsInside(pt¹) &&
		pt¹.m_z > 0 )
	{
		CATMVariables w¹ = m_world.get_weather(pt¹, UTCTime + dt);
		w = (w°+w¹)/2;
	}
	else
	{
		w = w°;
	}
	
	if (m_world.m_parameters1.m_bUseTurbulance)
	{
		w[ATM_WNDU] *= exp(m_world.random().Rand(-0.1, 0.1));
		w[ATM_WNDV] *= exp(m_world.random().Rand(-0.1, 0.1));
		w[ATM_WNDW] *= exp(m_world.random().Rand(-0.1, 0.1));
	}

	return w;
}



void CFlyer::ascent_flight(size_t UTCTime)
{
	ASSERT(UTCTime >= m_parameters.m_t_liftoff);

	size_t duration = UTCTime - m_parameters.m_t_liftoff;
	if (duration < m_parameters.m_duration)
	{
		CTRef UTCTRef = Time2CTRef(UTCTime);
		if (m_world.m_weather.IsLoaded(UTCTRef))
		{
			if (m_world.IsInside(m_pt) )
			{
				double dt = m_world.get_time_step(); //[s]
				CATMVariables w = get_weather(UTCTime);
				CGeoDistance3D U = get_U(w);
				CGeoDistance3D d = U*dt;

				if (w[ATM_PRCP] < m_world.m_parameters2.m_Pmax)
				{
					if (w[ATM_TAIR] > m_world.m_parameters2.m_Tmin)
					{
						//update coordinate
						((CGeoPoint3D&)m_pt) = UpdateCoordinate(m_pt, d);
						m_pt.m_z = max(10.0, m_pt.m_z);//hummm???
						//((CGeoPoint3D&)m_newLocation) = UpdateCoordinate(m_newLocation, d);
						//m_newLocation.m_z = max(10.0, m_newLocation.m_z);//hummm???

						((CGeoPoint3D&)m_newLocation) = UpdateCoordinate(m_newLocation, d);
						m_newLocation.m_z = max(10.0, m_newLocation.m_z);//hummm???

						//((CGeoPoint3D&)m_pt) = m_newLocation;
						//m_pt.Reproject(m_world.m_GEO2DEM);//convert from GEO to DEM projection
						//m_pt.m_z -= m_location.m_z;

						if (duration > m_parameters.m_t_hunting)
						{
							if (m_log[T_HUNTING] == 0)
								m_log[T_HUNTING] = UTCTime;//first time hunting

							if (m_world.is_over_host(m_pt))//look to find host
							{
								m_state = DESCENDING_FLIGHT;
								m_end_type = FIND_HOST;
							}
							else if (m_world.is_over_distraction(m_pt))//look for distraction
							{
								m_state = DESCENDING_FLIGHT;
								m_end_type = FIND_DISTRACTION;
							}
						}

						if (m_end_type == NO_END_DEFINE)
						{
							if (m_pt.m_z >= m_parameters.m_height)
							{
								m_state = HORIZONTAL_FLIGHT;
							}
						}
					}
					else
					{
						m_state = DESCENDING_FLIGHT;
						m_end_type = END_BY_TAIR;
					}
				}
				else
				{
					m_state = DESCENDING_FLIGHT;
					m_end_type = END_BY_RAIN;
				}

				m_stat[S_TAIR] += w[ATM_TAIR];
				m_stat[S_PRCP] = w[ATM_PRCP];
				m_stat[S_W_ASCENT] += U.m_z;	//ascent speed [m/s]
				m_stat[S_W_HORIZONTAL] += sqrt(U.m_x*U.m_x + U.m_y*U.m_y); //horizontal speed [m/s]
				m_stat[S_HEIGHT] += m_pt.m_z;	//flight height [m]
				m_stat[S_DIRECTION_X] += U.m_x;
				m_stat[S_DIRECTION_Y] += U.m_y;
				m_stat[S_DISTANCE] += sqrt(Square(d.m_x) + Square(d.m_y));
			}
			else
			{
				m_state = IDLE_END;
				m_end_type = OUTSIDE_MAP;
			}
		}
		else
		{
			m_state = IDLE_END;
			m_end_type = OUTSIDE_TIME_WINDOW;
		}
	}
	else
	{
		m_state = DESCENDING_FLIGHT;
		m_end_type = END_OF_TIME_FLIGHT;
	}


}

void CFlyer::horizontal_flight(size_t UTCTime)
{
	ASSERT(m_state == HORIZONTAL_FLIGHT);
	ASSERT(m_end_type == NO_END_DEFINE);


	size_t duration = UTCTime - m_parameters.m_t_liftoff;
	if (duration < m_parameters.m_duration)
	{
		CTRef UTCTRef = Time2CTRef(UTCTime);
		if (m_world.m_weather.IsLoaded(UTCTRef))
		{
			if (m_world.IsInside(m_pt) )
			{
				double dt = m_world.get_time_step(); //[s]
				CATMVariables w = get_weather(UTCTime);
				CGeoDistance3D U = get_U(w);
				CGeoDistance3D d = U*dt;


				if (w[ATM_PRCP] < m_world.m_parameters2.m_Pmax)
				{
					if (w[ATM_TAIR] > m_world.m_parameters2.m_Tmin)
					{
						//((CGeoPoint3D&)m_newLocation) = UpdateCoordinate(m_newLocation, d);
						//m_newLocation.m_z = max(10.0, m_newLocation.m_z);//hummm???
						((CGeoPoint3D&)m_pt) = UpdateCoordinate(m_pt, d);
						m_pt.m_z = max(10.0, m_pt.m_z);//hummm???
						((CGeoPoint3D&)m_newLocation) = UpdateCoordinate(m_newLocation, d);
						m_newLocation.m_z = max(10.0, m_newLocation.m_z);//hummm???

						//((CGeoPoint3D&)m_pt) = m_newLocation;
						//m_pt.Reproject(m_world.m_GEO2DEM);//convert from GEO to DEM projection
						//m_pt.m_z -= m_location.m_z;


						if (duration > m_parameters.m_t_hunting)
						{
							if (m_log[T_HUNTING] == 0)
								m_log[T_HUNTING] = UTCTime;//first time hunting

							if (m_world.is_over_host(m_pt))//look to find host
							{
								m_state = DESCENDING_FLIGHT;
								m_end_type = FIND_HOST;
							}
							else if (m_world.is_over_distraction(m_pt))//look for distraction
							{
								m_state = DESCENDING_FLIGHT;
								m_end_type = FIND_DISTRACTION;
							}
						}
						
					}
					else
					{
						m_state = DESCENDING_FLIGHT;
						m_end_type = END_BY_TAIR;
					}
				}
				else
				{
					m_state = DESCENDING_FLIGHT;
					m_end_type = END_BY_RAIN;
				}

				m_stat[S_TAIR] += w[ATM_TAIR];
				m_stat[S_PRCP] = w[ATM_PRCP];
				m_stat[S_W_HORIZONTAL] += sqrt(U.m_x*U.m_x + U.m_y*U.m_y); //horizontal speed [m/s]
				m_stat[S_HEIGHT] += m_pt.m_z;		 //flight height [m]
				m_stat[S_DIRECTION_X] += U.m_x;
				m_stat[S_DIRECTION_Y] += U.m_y;
				m_stat[S_DISTANCE] += sqrt(Square(d.m_x) + Square(d.m_y));
			}
			else
			{
				m_state = IDLE_END;
				m_end_type = OUTSIDE_MAP;
			}
		}
		else
		{
			m_state = IDLE_END;
			m_end_type = OUTSIDE_TIME_WINDOW;
		}
	}
	else
	{
		m_state = DESCENDING_FLIGHT;
		m_end_type = END_OF_TIME_FLIGHT;
	}
}

void CFlyer::descent_flight(size_t UTCTime)
{
	ASSERT(m_state == DESCENDING_FLIGHT);
	ASSERT(m_end_type != NO_END_DEFINE);
	
	CTRef UTCTRef = Time2CTRef(UTCTime);
	if (m_world.m_weather.IsLoaded(UTCTRef))
	{
		if (m_world.IsInside(m_pt) )
		{
			double dt = m_world.get_time_step(); //[s]
			CATMVariables w = get_weather(UTCTime);
			CGeoDistance3D U = get_U(w);
			CGeoDistance3D d = U*dt;

			((CGeoPoint3D&)m_pt) = UpdateCoordinate(m_pt, d);
			((CGeoPoint3D&)m_newLocation) = UpdateCoordinate(m_newLocation, d);

			if (m_pt.m_z > 0)
			{
				size_t duration = UTCTime - m_parameters.m_t_liftoff;
				if (duration < m_parameters.m_duration)
				{
					if (w[ATM_TAIR] > m_world.m_parameters2.m_Tmin &&
						m_end_type == END_BY_TAIR)
					{
						//go to horizontal flight state
						m_state = HORIZONTAL_FLIGHT;
						m_end_type = NO_END_DEFINE;
					}
				}


				if (duration > m_parameters.m_t_hunting)
				{
					if (m_log[T_HUNTING] == 0)
						m_log[T_HUNTING] = UTCTime;//first time hunting

					if (m_world.is_over_host(m_pt))//look to find host
					{
						m_state = DESCENDING_FLIGHT;
						m_end_type = FIND_HOST;
					}
					else if (m_world.is_over_distraction(m_pt))//look for distraction
					{
						m_state = DESCENDING_FLIGHT;
						m_end_type = FIND_DISTRACTION;
					}
				}

				m_stat[S_TAIR] += w[ATM_TAIR];
				m_stat[S_PRCP] = w[ATM_PRCP];
				m_stat[S_W_HORIZONTAL] += sqrt(U.m_x*U.m_x + U.m_y*U.m_y); //horizontal speed [m/s]
				m_stat[S_W_DESCENT] += U.m_z;	//descent speed [m/s]
				m_stat[S_HEIGHT] += m_pt.m_z;	//flight height [m]
				m_stat[S_DIRECTION_X] += U.m_x;
				m_stat[S_DIRECTION_Y] += U.m_y;
				m_stat[S_DISTANCE] += sqrt(Square(d.m_x) + Square(d.m_y));

			}
			else
			{
				m_state = LANDING;
			}

		}
		else
		{
			m_state = IDLE_END;
			m_end_type = OUTSIDE_MAP;
		}
	}
	else
	{
		m_state = IDLE_END;
		m_end_type = OUTSIDE_TIME_WINDOW;
	}
}

void CFlyer::landing(size_t UTCTime)
{
	m_log[T_LANDING] = UTCTime;
	m_state = IDLE_END;
	ASSERT(m_end_type != NO_END_DEFINE);
}

void CFlyer::idle_end(size_t UTCTime)
{
	if (m_log[T_IDLE_END] == 0)
		m_log[T_IDLE_END] = UTCTime;

	if (UTCTime - m_log[T_IDLE_END] > 3600)
		m_state = DESTROYED;

	ASSERT(m_end_type != NO_END_DEFINE);
}

void CFlyer::destroy(size_t UTCTime)
{
	m_log[T_DESTROY] = UTCTime;
}

//**************************************************************************************************************
//CATMWeatherCuboid

CATMVariables CATMWeatherCuboid::get_weather(const CGeoPoint3D& pt, bool bSpaceInterpol)const
{
	static const double POWER = 1;

	CATMVariables w;

	const CATMWeatherCuboid& me = *this;
	array<CStatistic, NB_ATM_VARIABLES> sumV;
	CStatistic sumP;

	double nearestD = DBL_MAX;
	CGeoPoint3DIndex nearest;
	//if (bSpaceInterpol)
	//{
	size_t dimSize = 2;// bSpaceInterpol ? 2 : 1;
	for (size_t z = 0; z < dimSize; z++)
	{
		for (size_t y = 0; y < dimSize; y++)
		{
			for (size_t x = 0; x < dimSize; x++)
			{
				double d = m_pt[z][y][x].GetDistance(pt) + 1;//add 1 meters to avoid division by zero
				double p = 1 / pow(d, POWER);
				sumP += p;
				
				if (d < nearestD)
				{
					nearestD = d;
					nearest = CGeoPoint3DIndex((int)x, (int)y, (int)z);
				}

				for (size_t v = 0; v < NB_ATM_VARIABLES; v++)
					sumV[v] += me[z][y][x][v] * p;
			}
		}
	}
	
	if (!bSpaceInterpol)
	{
		//take the nearest point
		for (size_t v = 0; v < NB_ATM_VARIABLES; v++)
			sumV[v] = me[nearest.m_z][nearest.m_y][nearest.m_x][v];

		sumP = 1;
	}
	

	//mean of 
	for (size_t v = 0; v < NB_ATM_VARIABLES; v++)
	{
		w[v] += sumV[v][SUM] / sumP[SUM];
		ASSERT(!_isnan(w[v]) && _finite(w[v]));
	}

	return w;
}

//time: time since 1 jan 1 [s]
CATMVariables CATMWeatherCuboids::get_weather(const CGeoPoint3D& pt, size_t time)const
{
	ASSERT(at(0).m_time<at(1).m_time);
	ASSERT(time >= at(0).m_time && time <= at(1).m_time);
	ASSERT(at(1).m_time - at(0).m_time == 3600);

	const CATMWeatherCuboids& me = *this;

	double f° = (double(time) - at(0).m_time) / (at(1).m_time - at(0).m_time); // get fraction of time
	if (!m_bUseTimeInterpolation)
		f° = f° >= 0.5 ? 1 : 0;

	double f¹ = (1 - f°);
	

	CATMVariables w° = me[0].get_weather(pt, m_bUseSpaceInterpolation);
	CATMVariables w¹ = me[1].get_weather(pt, m_bUseSpaceInterpolation);
	
	ASSERT(f° + f¹ == 1);
	return w°*f° + w¹*f¹;
}


//**************************************************************************************************************
//CATMWeather

CATMVariables CATMWeather::get_weather(const CGeoPoint3D& pt, size_t UTCTime)const
{
	ASSERT(pt.m_z >= 0);
	ASSERT(pt.IsGeographic());

	CATMVariables w1;
	if (m_world.m_parameters1.m_weather_type == CATMWorldParamters::FROM_GRIBS ||
		m_world.m_parameters1.m_weather_type == CATMWorldParamters::FROM_BOTH)
	{
		CGeoPoint3D pt2(pt);
		pt2.Reproject(m_world.m_GEO2WEA);
		CATMWeatherCuboidsPtr p_cuboid = get_cuboids(pt2, UTCTime);
		w1 = p_cuboid->get_weather(pt2, UTCTime);
	}
	
	CATMVariables w2;
	if (m_world.m_parameters1.m_weather_type == CATMWorldParamters::FROM_STATIONS ||
		m_world.m_parameters1.m_weather_type == CATMWorldParamters::FROM_BOTH)
	{
		w2 = get_station_weather(pt, UTCTime);
	}



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
	

	return weather;
}

size_t CATMWorld::get_local_sunset(CTRef TRef, const CLocation& loc)
{
	//CTRef TRef = GetLocalTRef(loc.m_lon);
	CSun sun(loc.m_lat, loc.m_lon);
	double sunset = sun.GetSunset(TRef);

	CTRef sunsetTRef(TRef.GetYear(), TRef.GetMonth(), TRef.GetDay(), 0);
	sunsetTRef += int(sunset);
	size_t sunsetTime = (sunsetTRef.GetRef() + (sunset - int(sunset))) * 3600.0;//time in second
	
	return sunsetTime;
}

//Ul: wind speed [m/s]
//ΔT: difference between air temperature and water temperature [°C]
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
	ASSERT(ΔT >= -50 && ΔT<50);
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
	//double f² = pow(z / Zr, α²);

	//double f = f¹;
	//double f = (f¹ + f²) / 2;

	Ur *= f;
	Vr *= f;
	ASSERT(!_isnan(Ur) && !_isnan(Vr));
	ASSERT(_finite(Ur) && _finite(Vr));
}

CATMVariables CATMWeather::get_station_weather(const CGeoPoint3D& pt, size_t  UTCTime)const
{
	CTRef UTCTRef = Time2CTRef(UTCTime);
	CATMVariables w° = get_station_weather(pt, UTCTRef);
	CATMVariables w¹ = get_station_weather(pt, UTCTRef + 1);
	ASSERT(GetHourlySeconds(UTCTime) >= 0 && GetHourlySeconds(UTCTime) <= 3600);
	

	double f° = GetHourlySeconds(UTCTime) / 3600.0;
	if (!m_world.m_parameters1.m_bUseTimeInterpolation)
		f° = f° >= 0.5 ? 1 : 0;

	double f¹ = (1 - f°);

	return w°*f° + w¹*f¹;
}

CATMVariables CATMWeather::get_station_weather(const CGeoPoint3D& pt, CTRef UTCTRef)const
{
	ASSERT(!_isnan(pt.m_x) && _finite(pt.m_x));
	ASSERT(!_isnan(pt.m_y) && _finite(pt.m_y));
	ASSERT(!_isnan(pt.m_z) && _finite(pt.m_z));
	ASSERT(m_iwd.find(UTCTRef) != m_iwd.end());
	ASSERT(pt.IsGeographic());

	CATMVariables weather;

	bool bOverWater = false;
	if (m_world.m_water_DS.IsOpen())
	{
		CGeoPoint pt2(pt);
		if (pt2.GetPrjID() != m_world.m_water_DS.GetPrjID())
		{
			ASSERT(pt.IsGeographic());
			pt2.Reproject(m_world.m_GEO2DEM);
		}

		CGeoPointIndex xy = m_world.m_water_DS.GetExtents().CoordToXYPos(pt2);
		bOverWater = m_world.m_water_DS.ReadPixel(0, xy) != 0;
	}

	CGridPoint gpt(pt.m_x, pt.m_y, 10, 0, 0, 0, 0, pt.GetPrjID());
	
	for (size_t v = 0; v <NB_ATM_VARIABLES; v++)
		weather[v] = m_iwd.at(UTCTRef)[v].Evaluate(gpt);
	
	double Tw = weather[ATM_WATER];		//water temperature [°C]
	double ΔT = weather[ATM_TAIR] - Tw;	//difference between air and water temperature
	
	GetWindProfileRelationship(weather[ATM_WNDU], weather[ATM_WNDV], pt.m_z, m_world.m_parameters2.m_windS_stability_type, bOverWater, ΔT);


	//adjust air temperature with flight height with a default gradient of 0.65°C/100m
	weather[ATM_TAIR] += -0.0065*pt.m_z;//flight height temperature correction



	return weather;
}

CGeoPointIndex CATMWeather::get_xy(const CGeoPoint& ptIn, CTRef UTCTRef)const
{
	CGeoExtents extents = m_p_weather_DS.GetExtents(UTCTRef);
	CGeoPoint pt = ptIn - CGeoDistance(extents.XRes() / 2, extents.YRes() / 2, extents.GetPrjID());
	//CGeoPoint pt = ptIn + CGeoDistance(extents.XRes() / 2, extents.YRes() / 2, extents.GetPrjID());

	return extents.CoordToXYPos(pt);//take the lower
	
}

int CATMWeather::get_level(const CGeoPointIndex& xy, double alt, CTRef UTCTRef, bool bLow)const
{
	vector<pair<double, int>> test;

	for (int l = 1; l < NB_LEVELS; l++)
	{
		size_t b = m_p_weather_DS.get_band(UTCTRef, ATM_HGT, l);
		double gph = m_p_weather_DS.GetPixel(UTCTRef, b, xy);
		test.push_back(make_pair(gph, l));

		if (alt < gph)
			break;
	}

	double grAlt = GetGroundAltitude(xy, UTCTRef);//get the first level over the ground
	test.push_back(make_pair(grAlt, 0));
	sort(test.begin(), test.end());

	int L = -1;
	for (int l = 0; l < (int)test.size(); l++)
	{
		if (alt < test[l].first)
		{
			L = test[bLow ? max(0, l - 1) : l].second;
			break;
		}
	}

	ASSERT(L >= 0 && L < NB_LEVELS);
	return L;
}

int CATMWeather::get_level(const CGeoPointIndex& xy, double alt, CTRef UTCTRef)const
{
	int L = NB_LEVELS - 1;//take the last level

	for (int l = 1; l < NB_LEVELS; l++)
	{
		size_t b = m_p_weather_DS.get_band(UTCTRef, ATM_HGT, l);
		double gph = m_p_weather_DS.GetPixel(UTCTRef, b, xy);

		if (alt < gph)
		{
			L = l;
			break;
		}
	}

	return L;
}

double CATMWeather::GetGroundAltitude(const CGeoPointIndex& xy, CTRef UTCTRef)const
{
	size_t b = m_p_weather_DS.get_band(UTCTRef, ATM_HGT, 0);
	double gph = m_p_weather_DS.GetPixel(UTCTRef, b, xy);

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
		double gph = m_p_weather_DS.GetPixel(UTCTRef, b, xyz) ;
		if (l == 0)//if the point is lower than 15 meters of the surface, we take surface
			gph += 15;


		if (pt.m_alt <= gph )
		{
			//xyz.m_z = int(b);
			xyz.m_z = int(l);
			break;
		}
	}

	return xyz;
}


CATMWeatherCuboidsPtr CATMWeather::get_cuboids(const CGeoPoint3D& ptIn, size_t UTCTime)const
{
	ASSERT(IsLoaded(Time2CTRef(UTCTime)) && IsLoaded(Time2CTRef(UTCTime) + 1));
	ASSERT(ptIn.m_z>0);

	CATMWeatherCuboidsPtr cuboids(new CATMWeatherCuboids);
	cuboids->m_bUseSpaceInterpolation = m_world.m_parameters1.m_bUseSpaceInterpolation;
	cuboids->m_bUseTimeInterpolation = m_world.m_parameters1.m_bUseTimeInterpolation;
	
	CTRef UTCTRef = Time2CTRef(UTCTime);
	if (!IsLoaded(UTCTRef) )
		return cuboids;//humm

	ASSERT(m_p_weather_DS.get_band(UTCTRef, ATM_WNDW, 0) != UNKNOWN_POS || m_p_weather_DS.get_band(UTCTRef, ATM_VVEL, 0) != UNKNOWN_POS);
	//if have VVEL, then it's RUC otherwise it's WRF
	size_t gribType = m_p_weather_DS.get_band(UTCTRef, ATM_VVEL, 0) != UNKNOWN_POS ? RUC_TYPE : WRF_TYPE;

	//fill cuboid
	for (size_t i = 0; i < TIME_SIZE; i++, UTCTRef++)
	{
		(*cuboids)[i].m_time = size_t(UTCTRef.GetRef() * 3600.0);//reference in second

		if (i == 1 && !IsLoaded(UTCTRef))
		{
			(*cuboids)[1] = (*cuboids)[0];
			return cuboids;
		}
			
		

		const CGeoExtents& extents = m_p_weather_DS.GetExtents(UTCTRef);
		double groundAlt = 0;
		
		//RUC is above sea level and WRF is above ground
		if (gribType == RUC_TYPE)
			groundAlt = m_world.GetGroundAltitude(ptIn);
		



		CGeoPoint3D pt(ptIn);
		pt.m_z += groundAlt;

		//CGeoPoint3DIndex xyz1 = get_xyz(pt, UTCTRef);
		
		
		ASSERT(extents.IsInside(pt));

		CGeoPointIndex xy1 = get_xy(pt, UTCTRef);

		//size_t dimSize = bUseCuboid ? 2 : 1;
		//size_t dimSize = 2;
		//(*cuboids)[i].m_dimSize = dimSize;

		for (size_t z = 0; z < NB_POINTS_Z; z++)
		{
			for (size_t y = 0; y < NB_POINTS_Y; y++)
			{
				for (size_t x = 0; x < NB_POINTS_X; x++)
				{
					CGeoPointIndex xy2 = xy1 + CGeoPointIndex((int)x, (int)y);
					int L = get_level(xy2, pt.m_z, UTCTRef, z==0);
					ASSERT(L >= 0 && L < NB_LEVELS);

				//	double groundAlt2 = GetGroundAltitude(xy2, UTCTRef);

					//CGeoPoint3DIndex xyz2 = xyz1 + CGeoPoint3DIndex((int)x, (int)y, (int)z);
					//if (xyz2.m_x >= extents.m_xSize)
						//xyz2.m_x = extents.m_xSize - 1;
					//if (xyz2.m_y >= extents.m_ySize)
						//xyz2.m_y = extents.m_ySize - 1;
					
					
					if (xy2.m_x >= extents.m_xSize)
						xy2.m_x = extents.m_xSize - 1;
					if (xy2.m_y >= extents.m_ySize)
						xy2.m_y = extents.m_ySize - 1;
					if (L > MAX_GEOH)
						L = MAX_GEOH;

					((CGeoPoint&)(*cuboids)[i].m_pt[z][y][x]) = extents.GetPixelExtents(xy2).GetCentroid();//Get the center of the cell

					size_t bGph = m_p_weather_DS.get_band(UTCTRef, ATM_HGT, L);
					double gph = m_p_weather_DS.GetPixel(UTCTRef, bGph, xy2);
					//(*cuboids)[i].m_pt[z][y][x].m_z = gph - groundAlt;
					(*cuboids)[i].m_pt[z][y][x].m_z = gph - groundAlt;

					for (size_t v = 0; v < ATM_WATER; v++)
					{
						bool bConvertVVEL = false;
						size_t b = m_p_weather_DS.get_band(UTCTRef, v, L);
						//size_t b = m_p_weather_DS.get_band(UTCTRef, v, (size_t)xyz2.m_z);

						if (b == UNKNOWN_POS && v == ATM_WNDW)
						{
							ASSERT(gribType == RUC_TYPE);
							b = m_p_weather_DS.get_band(UTCTRef, ATM_VVEL, L);
							bConvertVVEL = true;
						}
							

						if (b != UNKNOWN_POS)
						{
							(*cuboids)[i][z][y][x][v] = m_p_weather_DS.GetPixel(UTCTRef, b, xy2);
							if (v == ATM_PRCP)
								(*cuboids)[i][z][y][x][v] *= 3600; //convert mm/s into mm/h

							if (bConvertVVEL)
								(*cuboids)[i][z][y][x][v] = CATMVariables::get_Uw((*cuboids)[i][z][y][x][ATM_PRES]*100, (*cuboids)[i][z][y][x][ATM_TAIR], (*cuboids)[i][z][y][x][ATM_WNDW]);//convert VVEL into W
						}
						else
						{

							assert(v == ATM_PRES);
							assert(L>0 && L <= MAX_GEOH);

							//assert(false);//a vérifier chnager xyz2.m_z par gph
							//double P = 1013 * pow((293 - 0.0065*xyz2.m_z) / 293, 5.26);//pressure in hPa
							double P = 1013 * pow((293 - 0.0065*gph) / 293, 5.26);//pressure in hPa
							
							(*cuboids)[i][z][y][x][v] = P;
						}
					}//variable
				}//x
			}//y
		}//z
	}//for t1 and t2

	assert((*cuboids)[0].m_time<(*cuboids)[1].m_time);

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


		std::ios::pos_type length = file.length();
		callback.SetCurrentDescription("Load Gribs");
		callback.SetNbStep(length);

		for (CSVIterator loop(file); loop != CSVIterator() && msg; ++loop)
		{
			if ((*loop).size() == 2)
			{
				CTRef TRef;
				TRef.FromFormatedString((*loop)[0],"","-",1);
				assert(TRef.IsValid());

				m_filepath_map[TRef] = (*loop)[1];
			}
			
			callback.SetCurrentStepPos((double)file.tellg());
			msg += callback.StepIt(0);
		}

		if (msg)
			m_filePathGribs = filepath;
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
	//return TransformWRF2RUC(callback);


	if (hourlyDBFilepath.empty() && gribsFilepath.empty())
	{
		msg.ajoute(GetString(IDS_WG_NO_WEATHER));
		return msg;
	}

	m_extents.Reset();

	if (!gribsFilepath.empty())
	{
		msg += load_gribs(gribsFilepath, callback);
			
		if (msg)
		{
			CTRef TRef = m_filepath_map.begin()->first;
			msg = m_p_weather_DS.load(TRef, get_image_filepath(TRef), callback);
			if (msg)
			{
				//reproject into DEM projection
				m_extents.ExtendBounds(Get(TRef)->GetExtents());
				m_world.m_GEO2WEA = GetReProjection(PRJ_WGS_84, m_extents.GetPrjID());
				// (m_extents.GetPrjID() != m_world.m_DEM_DS.GetPrjID())
				//{
					//m_world.m_WEA2DEM = GetReProjection(m_extents.GetPrjID(), m_world.m_DEM_DS.GetPrjID());
					//m_extents.Reproject(m_world.m_WEA2DEM);
					//callback.AddMessage("WARNING: the projection of the DEM is not the same as the projection of the weather gribs. Severe bias in wind direction can be observed.");
				//}
			}
		}
	}

	if (!hourlyDBFilepath.empty())
	{
		if (!m_p_hourly_DB)
		{
			msg += load_hourly(hourlyDBFilepath, callback);
			if (msg)
			{
				if (m_world.m_GEO2WEA.GetSrc()==NULL)
					m_world.m_GEO2WEA = GetReProjection(PRJ_WGS_84, PRJ_WGS_84);

				for (size_t i = 0; i < m_p_hourly_DB->size(); i++)
				{
					//reproject into Gribs projection if any
					CLocation loc = m_p_hourly_DB->at(i);
					loc.Reproject(m_world.m_GEO2WEA);
					m_extents.ExtendBounds(loc);
				}
			}
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
				msg.ajoute("File for " + UTCTRef.GetFormatedString() + " is Missing");
			}

			if (!msg)
			{
				
				//replace image by the nearest image
				CTRef firstGoodImage;
				for (size_t h = 0; h < 12 && !firstGoodImage.IsInit(); h++)
				{
					int i = int(pow(-1, h+1)*(h+2) / 2);
					string filePath = get_image_filepath(UTCTRef+i);
					if (!filePath.empty() && m_p_weather_DS.load(UTCTRef, filePath, callback))
					{
						firstGoodImage = UTCTRef + i;
					}
				}

				if (firstGoodImage.IsInit())
				{
					//add a warning and remove error
					callback.AddMessage("WARNING: " + TrimConst(GetText(msg)));
					msg = ERMsg();

					//load firstGoodImage into UTCTRef
					msg = m_p_weather_DS.load(UTCTRef, get_image_filepath(firstGoodImage), callback);
				}
				else
				{
					UTCTRef.Transform(CTM(CTM::DAILY));
					callback.AddMessage("WARNING: Unable to fing a good starting Gribs weather for " + UTCTRef.GetFormatedString() + " (UTC)");
					m_bSkipDay = true;
					return msg;
				}
			}
			
			msg += callback.StepIt();
		}
	}
	
	if (msg && !m_filePathHDB.empty() )
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
					msg = m_p_hourly_DB->Search(result, it->m_newLocation, m_world.m_parameters2.m_nb_weather_stations * 5, CWVariables(FILTER_STR[v]), year);
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
				msg += callback.StepIt(1.0 /indexes.size());
			}

			//create IWD object
			for (size_t v = 0; v < NB_ATM_VARIABLES&&msg; v++)
			{
				CGridPointVectorPtr pts(new CGridPointVector);
				//pts->SetPrjID();
				//pts->m_bGeographic = m_world.m_DEM_DS.GetPrj()->IsGeographic();

				set<size_t> indexes;
				for (auto it = m_world.m_flyers.begin(); it != m_world.m_flyers.end() && msg; it++)
				{
					CSearchResultVector result;
					msg = m_p_hourly_DB->Search(result, it->m_newLocation, m_world.m_parameters2.m_nb_weather_stations * 5, CWVariables(FILTER_STR[v]), year);
					for (size_t ss = 0; ss < result.size(); ss++)
						indexes.insert(result[ss].m_index);
				}

				for (set<size_t>::const_iterator it = indexes.begin(); it != indexes.end() && msg; it++)
				{
					size_t index = *it;
					ASSERT(m_stations.find(index) != m_stations.end());

					const CWeatherStation& station = m_stations[index];
					CTRef TRef = UTCTRef2LocalTRef(UTCTRef, station.m_lon);
					//CLocation loc = station;
					//loc.Reproject(m_world.m_GEO2DEM);//convert weather station coordinate (Geo) into DEM coordinate

					switch (v)
					{
					case ATM_TAIR:
					{
						CStatistic Tair = station[TRef][H_TAIR];
						if (Tair.IsInit())
							//pts->push_back(CGridPoint(loc.m_x, loc.m_y, 10, 0, 0, Tair[MEAN], station.m_lat, loc.GetPrjID()));
							pts->push_back(CGridPoint(station.m_x, station.m_y, 10, 0, 0, Tair[MEAN], station.m_lat, station.GetPrjID()));
					}
					break;
					case ATM_PRES:
					{
						//double pres = 1013 * pow((293 - 0.0065*loc.m_z) / 293, 5.26);//ASCE2005
						double pres = 1013 * pow((293 - 0.0065*station.m_z) / 293, 5.26);//ASCE2005
						CStatistic presStat = station[TRef][H_PRES];
						if (presStat.IsInit())
							pres = presStat[MEAN];

						//pts->push_back(CGridPoint(loc.m_x, loc.m_y, 10, 0, 0, pres, station.m_lat, loc.GetPrjID()));//pres in Pa
						pts->push_back(CGridPoint(station.m_x, station.m_y, 10, 0, 0, pres, station.m_lat, station.GetPrjID()));
					}
					break;
					case ATM_PRCP: 
					{
						CStatistic prcp = station[TRef][H_PRCP];
						if (prcp.IsInit())
							//pts->push_back(CGridPoint(loc.m_x, loc.m_y, 10, 0, 0, prcp[SUM], station.m_lat, loc.GetPrjID()));
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

							double val = v == ATM_WNDU ? U : V ;
							//pts->push_back(CGridPoint(loc.m_x, loc.m_y, 10, 0, 0, val, station.m_lat, loc.GetPrjID()));
							pts->push_back(CGridPoint(station.m_x, station.m_y, 10, 0, 0, val, station.m_lat, station.GetPrjID()));
						}
					}
					break;

					case ATM_WNDW:
					{
						//pts->push_back(CGridPoint(loc.m_x, loc.m_y, 10, 0, 0, 0, station.m_lat, loc.GetPrjID()));
						pts->push_back(CGridPoint(station.m_x, station.m_y, 10, 0, 0, 0, station.m_lat, station.GetPrjID()));
						break;
					}
					case ATM_WATER:
					{
						CStatistic Tair = station[TRef][H_TAIR];
						if (Tair.IsInit())
						{
							double Tw = m_Twater[index].GetTwI(TRef.Transform(CTM(CTM::DAILY)));
							//pts->push_back(CGridPoint(loc.m_x, loc.m_y, 10, 0, 0, Tw, station.m_lat, loc.GetPrjID()));
							pts->push_back(CGridPoint(station.m_x, station.m_y, 10, 0, 0, Tw, station.m_lat, station.GetPrjID()));
						}
					}
					break;

					default:ASSERT(false);
					}

					msg += callback.StepIt(0);
				}

				CGridInterpolParam param;
				param.m_IWDModel = CGridInterpolParam::IWD_CLASIC;
				param.m_power = CGridInterpolParam::IWD_POWER;
				param.m_nbPoints = m_world.m_parameters2.m_nb_weather_stations;
				param.m_bGlobalLimit = false;
				param.m_bGlobalLimitToBound = false;
				param.m_maxDistance = 1000000;
				param.m_bUseElevation = false;

				ASSERT(pts->size() >= m_world.m_parameters2.m_nb_weather_stations);
				if (pts->size()<m_world.m_parameters2.m_nb_weather_stations)
				{
					if (v == ATM_PRCP)
					{
						callback.AddMessage("WARNING: Not enaught precipitation. replaced by zero.");
						while (pts->size()<m_world.m_parameters2.m_nb_weather_stations)
							pts->push_back(CGridPoint(0, 0, 10, 0, 0, 0, 45, PRJ_WGS_84));
					}
					else
					{
						msg.ajoute("Too mush missing value for " + UTCTRef.GetFormatedString() + " (UTC)");
					}
				}

				m_iwd[UTCTRef][v].SetDataset(pts);
				m_iwd[UTCTRef][v].SetParam(param);
				msg += m_iwd[UTCTRef][v].Initialization();
				msg += callback.StepIt(0);
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

	//remove old maps (h-24)
	for (iterator it = me.begin(); it != me.end()&&msg;)
	{
		
		if (TRef - it->first > m_max_hour_load)//24
		{
			it->second->Close();
			it = me.erase(it);
			msg=callback.StepIt();
		}
		else
		{
			it++;
		}
	}

	
	//me.m_mutex.unlock();

	return msg;
}

ERMsg CTRefDatasetMap::Discard(CCallback& callback)
{
	ERMsg msg;
	for (iterator it = begin(); it != end()&&msg;)
	{
		it->second->Close();
		it = erase(it);
		msg += callback.StepIt();
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
//
//bool CTRefDatasetMap::convert_VVEL(CTRef TRef)const
//{
//	size_t band = UNKNOWN_POS;
//
//	return at(TRef)->convert_VVEL();
//}

//******************************************************************************************************
const char* CATMWorldParamters::MEMBERS_NAME[NB_MEMBERS] = { "WeatherType", "TimeStep", "Seed", "Reversed", "UseSpaceInterpol", "UseTimeInterpol", "UsePredictorCorrectorMethod", "UseTurbulance", "UseVerticalVelocity", "EventThreshold", "DefoliationThreshold", "DistractionThreshold", "HostThreshold", "DEM", "WaterLayer", "Gribs", "HourlyDB", "Defoliation", "Distraction", "Host" };


std::set<int> CATMWorld::get_years()const
{
	std::set<int> years;

	for (CFlyersCIt it = m_flyers.begin(); it != m_flyers.end(); it++)
		years.insert(it->m_TRef.GetYear());

	return years;
}

CTPeriod CATMWorld::get_period(bool bUTC, int year)const
{
	CTPeriod p;
	CStatistic maxDuration;
	for (CFlyersCIt it = m_flyers.begin(); it != m_flyers.end(); it++)
	{
		if (year == YEAR_NOT_INIT || it->m_TRef.GetYear() == year)
		{
			CTRef TRef = it->m_TRef;//local time
			if (bUTC)
				TRef -= (int)(it->m_location.m_lon / 15);

			p += TRef;
		}
	}
	
	if (p.IsInit())
	{
		
		p.Begin() += 12;//begin at noon 
		p.End() += 12 + 23;//finish at 11:00 on day after
	}

	return p;
}

set<CTRef> CATMWorld::get_TRefs( int year)const
{
	set<CTRef> TRefs;
	for (CFlyersCIt it = m_flyers.begin(); it != m_flyers.end(); it++)
	{
		if (year == YEAR_NOT_INIT || it->m_TRef.GetYear() == year)
		{
			CTRef TRef = it->m_TRef;//local time 
			ASSERT(TRef.GetTM().Type() == CTM::HOURLY);

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


bool CATMWorld::is_over_defoliation(const CGeoPoint3D& pt1)const
{
	bool defol = true;
	if (m_defoliation_DS.IsOpen())
	{
		CGeoPoint pt2(pt1);
		if (pt2.GetPrjID() != m_defoliation_DS.GetPrjID())
		{
			ASSERT(pt1.IsGeographic());
			pt2.Reproject(m_GEO2DEM);
		}

		CGeoPointIndex xy = m_defoliation_DS.GetExtents().CoordToXYPos(pt2);
		if(m_defoliation_DS.GetExtents().IsInside(xy))
		{
			double v = m_defoliation_DS.GetPixel(0, xy);
			if (v != m_defoliation_DS.GetNoData(0))
				defol = v>m_parameters1.m_defoliationThreshold;
		}
	}

	return defol;
}


bool CATMWorld::is_over_distraction(const CGeoPoint3D& pt1)const
{
	bool bRep = false;
	if (m_distraction_DS.IsOpen())
	{
		CGeoPoint pt2(pt1);
		if (pt2.GetPrjID() != m_distraction_DS.GetPrjID())
		{
			ASSERT(pt1.IsGeographic());
			pt2.Reproject(m_GEO2DEM);
		}


		CGeoPointIndex xy = m_distraction_DS.GetExtents().CoordToXYPos(pt2);
		if (m_distraction_DS.GetExtents().IsInside(xy))
		{
			
			double v = m_distraction_DS.GetPixel(0, xy);
			if (v != m_distraction_DS.GetNoData(0) && v>90)
				bRep = v > m_parameters1.m_distractionThreshold;
		}
	}

	return bRep;
}

bool CATMWorld::is_over_host(const CGeoPoint3D& pt1)const
{
	bool bRep = false;
	if (m_host_DS.IsOpen())
	{
		CGeoPoint pt2(pt1);
		if (pt2.GetPrjID() != m_distraction_DS.GetPrjID())
		{
			ASSERT(pt1.IsGeographic());
			pt2.Reproject(m_GEO2DEM);
		}


		CGeoPointIndex xy = m_host_DS.GetExtents().CoordToXYPos(pt2);
		if (m_host_DS.GetExtents().IsInside(xy))
		{
			double v = m_host_DS.GetPixel(0, xy);
			if (v != m_host_DS.GetNoData(0))
				bRep = v > m_parameters1.m_hostThreshold;
		}
	}

	return bRep;
}

//TRef is local
vector<CFlyersIt> CATMWorld::GetFlyers(CTRef localTRef)
{
	ASSERT(localTRef.GetTM().Type() == CTM::DAILY);

	vector<CFlyersIt> fls;
	for (CFlyersIt it = m_flyers.begin(); it != m_flyers.end(); it++)
	{
		CTRef TRef = it->m_TRef;
		TRef.Transform(CTM(CTM::DAILY));
		if (TRef == localTRef)
			fls.push_back(it);
	}

	return fls;
}

CTPeriod CATMWorld::get_UTC_period(const vector<CFlyersIt>& fls)
{
	CTPeriod UTCp;

	for (size_t i = 0; i < fls.size(); i++)
	{
		//CTRef sunsetTRef = get_local_sunset(fls[i]->P().m_t_liftoff, fls[i]->m_location);//add sunset
		size_t UTCLiftoff = (size_t)floor(fls[i]->P().m_t_liftoff);
		size_t UTCLanding = (size_t)ceil(UTCLiftoff + fls[i]->P().m_duration);
		UTCp += Time2CTRef(UTCLiftoff);
		UTCp += Time2CTRef(UTCLanding) + 1 + 1;//add tree extra hours for landing and cuboid
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
		pt2.Reproject(m_GEO2DEM);
	}
	
	CGeoPoint pt3(pt);
	if (pt.GetPrjID() != m_weather.GetPrjID())
	{
		ASSERT(pt.IsGeographic());
		pt3.Reproject(m_GEO2WEA);
	}

	return 	m_DEM_DS.GetExtents().IsInside(pt2) && m_weather.GetExtents().IsInside(pt3);
}

ERMsg CATMWorld::Execute(CATMOutputMatrix& output, CCallback& callback)
{
	assert(m_weather.is_init());
	assert(!m_flyers.empty());
	

	ERMsg msg;

	random().Randomize(m_parameters1.m_seed);
	
	const CGeoExtents& extent = m_DEM_DS.GetExtents();

	//init weather
	set<int> years = get_years();

	for (set<int>::const_iterator it = years.begin(); it != years.end() && msg; it++)
	{
		int year = *it;

		//get all days to simulate
		set<CTRef> TRefs = get_TRefs(year);
		callback.AddTask(TRefs.size()*4);
		
		//simulate for all days
		for (set<CTRef>::const_iterator it = TRefs.begin(); it != TRefs.end() && msg; it++)
		{
			//get all flyers for this day
			vector<CFlyersIt> fls = GetFlyers(*it);

			//Get all sunset hour for flyers. 
			set<CTRef> sunsetTRef;
			for (size_t i = 0; i < fls.size() && !m_weather.SkipDay() && msg; i++)
			{
				//load weather at sunset
				size_t localSunset = get_local_sunset(*it, fls[i]->m_location);
				size_t UTCSunset = LocalTime2UTCTime(localSunset, fls[i]->m_location.m_lon);
				CTRef UTCTRefSunset = Time2CTRef(UTCSunset);
				sunsetTRef.insert(UTCTRefSunset);
				sunsetTRef.insert(UTCTRefSunset+1);
			}

			//in progress step
			CTRef TRef = *it;
			size_t nbSteps = m_weather.HaveGribsWeather() ? sunsetTRef.size() : 0 + m_weather.HaveStationWeather() ? 1 : 0;
			callback.SetCurrentDescription("Load sunset weather for " + TRef.GetFormatedString());
			callback.SetNbStep(nbSteps);//+1 for weather station

			//load sunset weather
			for (set<CTRef>::const_iterator itSunset = sunsetTRef.begin(); itSunset != sunsetTRef.end() && msg; itSunset++)
			{
				//load weather at sunset
				msg += m_weather.LoadWeather(*itSunset, callback);
			}

			if (msg && !m_weather.SkipDay())
			{
				//init all flyers
				for (size_t i = 0; i < fls.size() && !m_weather.SkipDay() && msg; i++)
				{
					fls[i]->init();
				}

				//get simulation hours for this day
				CTPeriod UTC_period = get_UTC_period(fls);
				ASSERT(UTC_period.IsInit());
				
				callback.SetCurrentDescription("Load weather for " + TRef.GetFormatedString());
				callback.SetNbStep(UTC_period.size());//Load weather for all hours

				//pre-Load weather for the day
				for (CTRef UTCTRef = UTC_period.Begin(); UTCTRef <= UTC_period.End() && msg; UTCTRef++)
				{
					if (!m_weather.IsLoaded(UTCTRef))
						msg = m_weather.LoadWeather(UTCTRef, callback);
					else
						msg = callback.StepIt();
				}


				//Simulate dispersal
				callback.SetCurrentDescription("Dispersal for " + TRef.GetFormatedString());
				callback.SetNbStep(UTC_period.size());

				for (m_UTCTRef = UTC_period.Begin(); m_UTCTRef <= UTC_period.End() && msg; m_UTCTRef++)
				{
#pragma omp parallel for //if (m_parameters1.m_weather_type == CATMWorldParamters::FROM_GRIBS) est-ce que ça cause encore des problèmes??????
					for (__int64 i = 0; i < (__int64 )fls.size(); i++)
					//for (size_t i = 0; i < fls.size() && msg; i++)
					{
#pragma omp flush(msg)
						if (msg)
						{
							CFlyer& flyer = *(fls[i]);
							flyer.live();
							
							if (flyer.GetState() > CFlyer::IDLE_BEGIN && flyer.GetState() < CFlyer::DESTROYED)
							{
								CTRef TRef = UTCTRef2LocalTRef(m_UTCTRef, flyer.m_location.m_lon);
								CGeoPoint3D pt = flyer.m_pt;
								pt.Reproject(m_GEO2DEM);//convert from GEO to DEM projection

								output[flyer.m_loc][flyer.m_var][TRef][ATM_STATE] = (flyer.GetState() == CFlyer::IDLE_END) ? 10 + flyer.GetEnd() : flyer.GetState();
								output[flyer.m_loc][flyer.m_var][TRef][ATM_X] = pt.m_x;
								output[flyer.m_loc][flyer.m_var][TRef][ATM_Y] = pt.m_y;
								output[flyer.m_loc][flyer.m_var][TRef][ATM_LAT] = flyer.m_newLocation.m_lat;
								output[flyer.m_loc][flyer.m_var][TRef][ATM_LON] = flyer.m_newLocation.m_lon;
								output[flyer.m_loc][flyer.m_var][TRef][ATM_T] = flyer.GetStat(CFlyer::S_TAIR);
								output[flyer.m_loc][flyer.m_var][TRef][ATM_P] = flyer.GetStat(CFlyer::S_PRCP);
								output[flyer.m_loc][flyer.m_var][TRef][ATM_HEIGHT] = flyer.GetStat(CFlyer::S_HEIGHT);
								output[flyer.m_loc][flyer.m_var][TRef][ATM_SCALE] = flyer.m_scale;
								output[flyer.m_loc][flyer.m_var][TRef][ATM_W_ASCENT] = flyer.GetStat(CFlyer::S_W_ASCENT) * 3600 / 1000;
								output[flyer.m_loc][flyer.m_var][TRef][ATM_W_HORIZONTAL] = flyer.GetStat(CFlyer::S_W_HORIZONTAL) * 3600 / 1000;
								output[flyer.m_loc][flyer.m_var][TRef][ATM_W_DESCENT] = flyer.GetStat(CFlyer::S_W_DESCENT) * 3600 / 1000;

								double alpha = PI/2;
								if (flyer.GetStat(CFlyer::S_DIRECTION_Y) != 0 || flyer.GetStat(CFlyer::S_DIRECTION_X) != 0)
									alpha = atan2(flyer.GetStat(CFlyer::S_DIRECTION_Y), flyer.GetStat(CFlyer::S_DIRECTION_X));

								
								double angle = int(360 + 90 - Rad2Deg(alpha)) % 360;
								ASSERT(angle >= 0 && angle <= 360);
								output[flyer.m_loc][flyer.m_var][TRef][ATM_DIRECTION] = angle;

								double D° = flyer.m_newLocation.GetDistance(flyer.m_location, false);
								output[flyer.m_loc][flyer.m_var][TRef][ATM_DISTANCE] = flyer[CFlyer::S_DISTANCE].IsInit()?flyer[CFlyer::S_DISTANCE][SUM]:0;
								output[flyer.m_loc][flyer.m_var][TRef][ATM_DISTANCE_FROM_OIRIGINE] = D°;

							}//if flying

							callback.WaitPause();

							#pragma omp critical(stepIt)
							{
								#pragma omp flush(msg)
								if (msg)
									msg += callback.StepIt(0);
								#pragma omp flush(msg)
							}
							
						}//if msg
					}//for all flyers
					
					msg += callback.StepIt();
				}//for all valid hours


				callback.SetCurrentDescription("Discard weather for " + TRef.GetFormatedString() );
				callback.SetNbStep(UTC_period.size());//discard weather for all hours
				msg += m_weather.Discard(callback);
			}//if not skip day

			m_weather.ResetSkipDay();
		}//for all valid days
	}//for all years

	return msg;
}


//Question:
//s'il pleut dans la journé, est-ce que les papillon vol quand même?
//Si le papillon manque sont heure de vol, est-ce qu'il peut volé plus tard ou une autre journée? alors à quel cette autre journée
//le calcul hunt doit comment après le liftoff ou bione quand il arrive en vol horizontal (comme avant)
//est-ce que le calcul du temp vol inclus le temps d'ascention et la descebnte?
//est-ce zero ou Pmax qui influance liftoff, dans le code semble être zero,mais l'article Pmax????

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

double CBlockData::GetValue(int x, int y)
{
	size_t pos = size_t(y) * m_xBlockSize + x;
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

void CGDALDatasetCached::LoadBlock(const CGeoBlock3DIndex& ijk)
{
	
	
	m_mutex.lock();
	if (!IsCached(ijk))
	{
		assert(m_data[ijk.m_z][ijk.m_y][ijk.m_x] == NULL);
		CTimer readTime(TRUE);

		GDALRasterBand* poBand = m_poDataset->GetRasterBand(ijk.m_z + 1);
		int nXBlockSize, nYBlockSize;
		poBand->GetBlockSize(&nXBlockSize, &nYBlockSize);

		GDALDataType type = poBand->GetRasterDataType();
		CBlockData* pBlockData = new CBlockData(nXBlockSize, nYBlockSize, type);
		poBand->ReadBlock(ijk.m_x, ijk.m_y, pBlockData->m_ptr);
		m_data[ijk.m_z][ijk.m_y][ijk.m_x].reset(pBlockData);

		readTime.Stop();
		m_stats[0] += readTime.Elapsed();
	}
	m_mutex.unlock();

	ASSERT(IsCached(ijk));
}


size_t GetVar(const string& strVar)
{
	size_t var = UNKNOWN_POS;

	if (strVar == "TMP")
		var = ATM_TAIR;
	else if (strVar == "PRES")
		var = ATM_PRES;
	else if (strVar == "PRATE")
		var = ATM_PRCP;
	else if (strVar == "UGRD")
		var = ATM_WNDU;
	else if (strVar == "VGRD")
		var = ATM_WNDV;
	else if (strVar == "WGRD")
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

	//if (
	//	strLevel == "30-0 mb above ground" ||
	//	strLevel == "60-30 mb above ground" ||
	//	strLevel == "90-60 mb above ground" ||
	//	strLevel == "120-90 mb above ground" ||
	//	strLevel == "150-120 mb above ground" ||
	//	strLevel == "180-150 mb above ground" ||
	//	strLevel == "180-0 mb above ground")
	//{
	//	//do nothing, avoid assertion
	//}
	//else 
	
	if (strLevel == "surface" || strLevel == "2 m above ground" || strLevel == "10 m above ground")
	{
		level = 0;
	}
	else if (Find(strLevel, "isotherm") || Find(strLevel, "tropopause") || Find(strLevel, "above ground") )
	{
		//do nothing
	}
	else 
	{
		

		bool bValid = strLevel.find('-') == string::npos && !strLevel.empty() && isdigit(strLevel[0]);
		if (bValid)
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

ERMsg CGDALDatasetCached::OpenInputImage(const std::string& filePath, bool bOpenInv)
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
						
						if (var < NB_ATM_VARIABLES_EX && level < m_bands[var].size())
						{
							m_bands[var][level] = i;
						}
					}
					else
					{
						msg.ajoute("Bad .inv file : " + invFilePath);
					}
				}
			}
		}
	}
	
	//report prep 0 to all level
	for (size_t j = 1; j < NB_LEVELS; j++)
		m_bands[ATM_PRCP][j] = m_bands[ATM_PRCP][0];
	
	//copy VVEL 1 to surface
	if (m_bands[ATM_VVEL][1]!=UNKNOWN_POS)
		m_bands[ATM_VVEL][0] = m_bands[ATM_VVEL][1];


	


	return msg;
}

size_t CGDALDatasetCached::get_band(size_t v, size_t level )const
{
	ASSERT(level < NB_LEVELS);
	return m_bands[v][level];
}

//bool CGDALDatasetCached::convert_VVEL()const
//{
//	return m_bConverVVEL_into_WNDW;
//}

//***************************************************************************************************************
//class CWRFDatabase


ERMsg TransformWRF2RUC(CCallback& callback)
{
	ERMsg msg;
	static const char* THE_PRJ = "PROJCS[\"unnamed\",GEOGCS[\"WGS 84\",DATUM[\"WGS_1984\",SPHEROID[\"WGS 84\",6378137,298.257223563,AUTHORITY[\"EPSG\",\"7030\"]],AUTHORITY[\"EPSG\",\"6326\"]],PRIMEM[\"Greenwich\",0],UNIT[\"degree\",0.0174532925199433],AUTHORITY[\"EPSG\",\"4326\"]],PROJECTION[\"Lambert_Conformal_Conic_2SP\"],PARAMETER[\"standard_parallel_1\",40],PARAMETER[\"standard_parallel_2\",60],PARAMETER[\"latitude_of_origin\",30],PARAMETER[\"central_meridian\",-91],PARAMETER[\"false_easting\",0],PARAMETER[\"false_northing\",0],UNIT[\"metre\",1,AUTHORITY[\"EPSG\",\"9001\"]]]";
	msg += CProjectionManager::CreateProjection(THE_PRJ);
	
	size_t prjID = CProjectionManager::GetPrjID(THE_PRJ);
	CProjectionPtr prj = CProjectionManager::GetPrj(prjID);
	
	CProjectionTransformation Geo2LCC(PRJ_WGS_84, prjID);
	

	std::string elevFilepath = "D:\\Travaux\\Brian Sturtevant\\MapInput\\DEMGrandLake4km.tif";
	CGDALDatasetEx elevDS;
	elevDS.OpenInputImage(elevFilepath);

	vector < float > elev(elevDS.GetRasterXSize()* elevDS.GetRasterYSize());
	ASSERT(elev.size()==7257);

	GDALRasterBand* pBand = elevDS.GetRasterBand(0);
	pBand->RasterIO(GF_Write, 0, 0, elevDS.GetRasterXSize(), elevDS.GetRasterYSize(), &(elev[0]), elevDS.GetRasterXSize(), elevDS.GetRasterYSize(), GDT_Float32, 0, 0);

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
	options.m_extents = CGeoExtents(-280642.1,1903575,133144.6,2205765,101,74,101,1,prjID);
	options.m_prj = THE_PRJ;
	options.m_bOverwrite = true;
	
	
	static const size_t NB_WRF_HOURS= 193;//193 hours
	callback.SetNbTask(NB_WRF_HOURS);
	
	for (size_t h = 0; h < NB_WRF_HOURS&&msg; h++)
	{
		CTRef UTCRef(2007, JUNE, 21, 0);
		UTCRef += int(h);

		callback.SetCurrentDescription(UTCRef.GetFormatedString());
		callback.SetNbStep(7257 * NB_WRF_LEVEL);

		std::string filePathIn = FormatA("D:\\Travaux\\Brian Sturtevant\\Weather\\WRF\\Original\\wrfbud2_%03d.txt", h);
		CGDALDatasetEx geotif;
		
		
		string filePathOut = FormatA("D:\\Travaux\\Brian Sturtevant\\Weather\\WRF\\WRF_%4d_%02d_%02d_%02d.tif", UTCRef.GetYear(), UTCRef.GetMonth()+1, UTCRef.GetDay()+1, UTCRef.GetHour());
		msg += geotif.CreateImage(filePathOut, options);
		//create .inv file
		string filePathInvIn = "D:\\Travaux\\Brian Sturtevant\\Weather\\WRF\\template.inv";
		string filePathInvOut = FormatA("D:\\Travaux\\Brian Sturtevant\\Weather\\WRF\\WRF_%4d_%02d_%02d_%02d.inv", UTCRef.GetYear(), UTCRef.GetMonth()+1, UTCRef.GetDay()+1, UTCRef.GetHour());
		CopyOneFile(filePathInvIn, filePathInvOut, false);

		ifStream file;
		msg += file.open(filePathIn);
		if (msg)
		{
			std::string csv_text[NB_WRF_VARS];

			//write header
			for (size_t j = 0; j < NB_WRF_VARS; j++)
			{
				csv_text[j] = "KeyID,Latitude,Longitude";
				for (size_t i = 0; i < NB_WRF_LEVEL; i++)
					csv_text[j] += "," + FormatA("sounding%02d", i + 1);

				csv_text[j] += "\n";
			}

			CGeoPointIndex xy;
			vector<array<array<array<float, 101>, 74>, NB_WRF_LEVEL>> data(NB_WRF_VARS);
			for (size_t j = 0; j < data.size(); j++)
				for (size_t s = 0; s < data[j].size(); s++)
					for (size_t y = 0; y < data[j][s].size(); y++)
						for (size_t x = 0; x < data[j][s][y].size(); x++)
							data[j][s][y][x] = -9999;

			int i = NB_WRF_LEVEL+1;
			while (!file.eof()&&msg)
			{
				string line;
				getline(file, line);
				if (!line.empty())
				{
					//if (i % NB_WRF_LEVEL == 0)//+1 for coordinate
					if (i==38)
					{
						StringVector str(line, " ");
						ASSERT(str.size() == 3);
						
						for (size_t j = 0; j < NB_WRF_VARS; j++)
							csv_text[j] += str[2] + "," + str[0] + "," + str[1];

						CGeoPoint lastPoint(ToDouble(str[1]), ToDouble(str[0]), PRJ_WGS_84);
						msg += lastPoint.Reproject(Geo2LCC);

						ASSERT(geotif.GetExtents().IsInside(lastPoint));
						xy = geotif.GetExtents().CoordToXYPos(lastPoint);
						
						//groundAlt = elev[];
						i=0;//remove this index
						
					}
					else
					{
						
						StringVector str(line, " ");
						ASSERT(str.size() == NB_WRF_VARS);

						size_t sounding = i% NB_WRF_LEVEL;
						ASSERT(sounding < NB_WRF_LEVEL);
						for (size_t j = 0; j < NB_WRF_VARS; j++)
						{
							float v = ToFloat(str[j + 1]);
							if (j == WRF_TAIR)//tmp
								v -= 273.15f;//convert K to °C
							else if (j==WRF_PRCP)
								v /= 3600;//convert mm/h to mm/s
							else if (j == WRF_VWND)
								v /= 3600;//convert mixing ratio in relative humidity

							data[j][sounding][xy.m_y][xy.m_x] = v;
							
							csv_text[j] += "," + str[j + 1];
						}
					}

					if (i%NB_WRF_LEVEL == 0)
						for (size_t j = 0; j < NB_WRF_VARS; j++)
							csv_text[j] += "\n";

				
					i++;//next line
				}

				
				msg += callback.StepIt();
			}

			ASSERT(i == NB_WRF_LEVEL+1);

			for (size_t j = 0; j < data.size(); j++)
			{
				size_t size = j == WRF_PRCP ? 1 : data[j].size();//save only one layer of precipitation
				for (size_t s = 0; s < size; s++)
				{
					GDALRasterBand* pBand = geotif.GetRasterBand(j * NB_WRF_LEVEL + s);
					for (size_t y = 0; y < data[j][s].size(); y++)
						pBand->RasterIO(GF_Write, 0, (int)y, (int)data[j][s][y].size(), 1, &(data[j][s][y][0]), (int)data[j][s][y].size(), 1, GDT_Float32, 0, 0);
				}

				string path = "D:\\Travaux\\Brian Sturtevant\\Weather\\WRF\\CSV\\";
				string fileTitle = FormatA("%4d_%02d_%02d_%02d_%s", UTCRef.GetYear(), UTCRef.GetMonth()+1, UTCRef.GetDay()+1, UTCRef.GetHour(), VAR_NAME[j]);
				
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
		}
	}//for all 193 hours
	

	return msg;
}

}