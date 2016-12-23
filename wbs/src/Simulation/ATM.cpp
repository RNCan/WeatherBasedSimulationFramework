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
//NO_END_DEFINE			10
//NO_LIFTOFF			11
//END_BY_RAIN			12
//END_BY_TAIR			13
//END_BY_WNDS			14
//END_OF_TIME_FLIGHT	15
//FIND_HOST				16
//FIND_DISTRACTION		17
//OUTSIDE_MAP			18
//OUTSIDE_TIME_WINDOW	19





using namespace std;

using namespace WBSF::HOURLY_DATA;
using namespace WBSF::WEATHER;

namespace WBSF
{
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
	*     var p2 = p1.rhumbDestinationPoint(40300, 116.7); // p2.toString(): 50.9642°N, 001.8530°E
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

		λ2 = fmod(λ2 + 3 * PI, 2 * PI) - PI; // normalise to -180..+180°

		return CGeoPoint(Rad2Deg(λ2), Rad2Deg(φ2), pt.GetPrjID());
	};

	CGeoPoint3D Geodesic2Geocentric(CGeoPoint3D pt)
	{
		return CGeoPoint3D(pt(0), pt(1), pt(2), PRJ_GEOCENTRIC_BASE);
	}

	CGeoPoint3D UpdateCoordinate(const CGeoPoint3D& pt, const CGeoDistance3D& d)
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

	

	extern const char ATM_HEADER[] = "Sex|State|X|Y|Latitude|Longitude|T|P|U|V|W|HEIGHT|DELTA_HEIGHT|CURRENT_HEIGHT|SCALE|W_HORIZONTAL|W_VERTICAL|DIRECTION|DISTANCE|DISTANCE_FROM_OIRIGINE|LIFTOFF_TIME|LANDING_TIME";

	//At low altitudes above the sea level, the pressure decreases by about 1.2 kPa for every 100 meters.For higher altitudes within the troposphere, the following equation(the barometric formula) relates atmospheric pressure p to altitude h
	//12 pa/m
	//double Uw2 = -ω / 12;
	//alt en m et Z en mbar

	//http://www.ncl.ucar.edu/Document/Functions/Contributed/omega_to_w.shtml
	//p: pressure [Pa]
	//t: temperature [°C]
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
	
	const char* CATMParameters::MEMBERS_NAME[NB_MEMBERS] = { "Tmin", "Tmax", "Pmax", "Wmin", "LiftoffOffset", "LiftoffSDCorr", "DurationMin", "DurationMax", "DurationAlpha", "DurationBeta", "CruiseRatio", "CruiseHeight", "HeightType", "WLogMean", "WlogSD", "WingBeatExponent", "WingBeatFactor", "Whorzontal", "WhorzontalSD", "Wdescent", "WdescentSD", "WindStabilityType", "NbWeatherStations" };

	const double CATMWorld::Δtᶠ = 3;
	const double CATMWorld::Δtᶳ = -0.5;
	const double CATMWorld::T° = 24.5;


	//Δtᵀ : delta [s]
	//return liftoff offset realtive to sunset [s]
	__int64 CATMWorld::get_t_liftoff_offset(__int64 ΔtᵀIn)const
	{
		//double t_liftoff = 0;
		/*if (m_parameters2.m_t_liftoff_type == CATMParameters::OLD_TYPE)
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
		else if (m_parameters2.m_t_liftoff_type == CATMParameters::NEW_TYPE)
	{*/

		//

		//double prcp = -1;
		//double sumF = 0;
		//double k0 = 10.;
		//double k1 = -8.25;
		//double twoPi = 2 * 3.14159 / 24.;
		//double fourPi = 4 * 3.14159 / 24.;

		//.373 - 0.339*cos(2 * 3.14159 / 24.*(h + -8.25)) - 0.183*sin(2 * 3.14159 / 24.*(h + -8.25)) + 0.157*cos(4 * 3.14159 / 24.*(h + -8.25)) + 0.184*sin(4 * 3.14159 / 24.*(h + -8.25))

		//		
		//double F = .373 - 0.339*cos(twoPi*(h + k1)) - 0.183*sin(twoPi*(h + k1)) + 0.157*cos(fourPi*(h + k1)) + 0.184*sin(fourPi*(h + k1)); //Simmons and Chen (1975)

		////effect of temperature. The amplitude of sumF is independent of size of time step.
		////Equation [4] in Regniere unpublished (from CJ Sanders buzzing data)
		////if (prcp >= 0)
		//=F*0.91*pow(max(0.0, (31. - weather[h][H_TNTX])), 0.3)*exp(-pow(max(0.0, (31. - weather[h][H_TNTX]) / 9.52), 1.3));

		//sumF += F / nbSteps;


		//double f_ppt = max(0.0, 1.0 - pow(prcp / k0, 2));
		//return sumF*f_ppt;
		

		//static const double liftoff_μ = max(-1.2, min (0.8, 0.190575425*T - 4.042102263));
		//static const double liftoff_σ = max( 0.2, min(0.8 , -0.044029363*T + 1.363107669));

		//double t_liftoff = m_random.RandNormal(liftoff_μ + m_parameters2.m_t_liftoff_correction, liftoff_σ*m_parameters2.m_t_liftoff_σ_correction);
		//while (t_liftoff<-5 || t_liftoff>5)
		//	t_liftoff = m_random.RandNormal(liftoff_μ + m_parameters2.m_t_liftoff_correction, liftoff_σ*m_parameters2.m_t_liftoff_σ_correction);


		//return __int64(t_liftoff * 3600.0);//liftoff offset in seconds
		
		//double tᶳ = localSunset;
		/*double t° = max(tᶳ + Δtᶳ - 0.5*Δtᶠ, tᶳ + Δtᵀ);
		double tᶬ = min(tᶳ+Δtᶠ+1, t° + Δtᶠ);
		double tᶜ = (t° + tᶬ) / 2;
		double t = t° + (tᶬ - t°)*m_random.RandBeta(4, 4);
		double teta = max(-1.0, min(1.0, (t - tᶜ) / (tᶬ - tᶜ)));
		ASSERT(teta >= -1 && teta <= 1);
		double fᵗ = Square(1 - teta*teta);

		return fᵗ - tᶳ;
*/
		static const double C = 1.0 - 2.0 / 3.0 + 1.0 / 5.0;
		static const double Δt = 10.0 / 3600.0;//10 seconds


		double p_exodus = m_random.Randu();


		double Δtᵀ = ΔtᵀIn / 3600.0;//convert seconds to hours
		double t° = max(Δtᶳ - 0.5*Δtᶠ, Δtᵀ);
		double tᶬ = min(4.0, t° + Δtᶠ);
		double tᶜ = (t° + tᶬ) / 2;

		double tau = tᶬ;
		for (double t = t°; t < tᶬ && tau == tᶬ; t += Δt)
		{
			double tau = (t - tᶜ) / (tᶬ - tᶜ);
			double p = (C + tau - 2 * pow(tau, 3) / 3 + pow(tau, 5) / 5) / 2 * C;
			if (p > p_exodus)
				tau = t;
		}

		//double t = t° + (tᶬ - t°)*m_random.RandBeta(4, 4);
		return (tau + m_parameters2.m_t_liftoff_correction) * 3600;//convert hours to seconds

		//double teta = max(-1.0, min(1.0, (t - tᶜ) / (tᶬ - tᶜ)));
		//ASSERT(teta >= -1 && teta <= 1);
		//double fᵗ = Square(1 - teta*teta);

		//return fᵗ*3600;//convert hours to seconds
	}


	//__int64 CATMWorld::get_t_hunthing()const
	//{
	//	double t_hunting = 0;
	//	double ran = m_random.Randu(); //random value [0,1];
	//	int ran012 = m_random.Rand(2); //random value {0,1,2};

	//	if (ran012 == 0)
	//	{
	//		t_hunting = 0.5 + 0.5*ran;//first third hunts from 0.5 - 1.0 hr
	//	}
	//	else if (ran012 == 1)
	//	{
	//		t_hunting = 1.0 + 2.0*ran;// second third hunts from 1.0 - 3.0 hr
	//	}
	//	else
	//	{
	//		assert(ran012 == 2);
	//		t_hunting = 3.0 + 3.0*ran;// third third hunts from 3.0 - 6.0 hr
	//	}

	//	return __int64(t_hunting * 3600.0);//time of unting after litfoff
	//}

	//double CATMWorld::get_height()const
	//{
	//	double height = 0;

	//	if (m_parameters2.m_height_type == CATMParameters::OLD_TYPE)
	//	{
	//		//static const double SIGMA_SQ = 6.1;
	//		static const int NFT = 10;
	//		static const double HBOT = 0;
	//		static const double HTOP = 500;
	//		static const double DELTAH = (HTOP - HBOT) / NFT;
	//		//cumulative probabilty
	//		static const double ft[NFT + 1] = { 0.0 / 836.0, 80.0 / 836.0, 210.0 / 836.0, 410.0 / 836.0, 660.0 / 836.0, 760.0 / 836.0, 815.0 / 836.0, 824.0 / 836.0, 831.0 / 836.0, 836.0 / 836.0, 836 / 836 };

	//		double ran = m_random.Randu();
	//		size_t index = NFT;
	//		for (size_t k = 0; k < NFT; k++)
	//		{
	//			if (ran >= ft[k] && ran < ft[k + 1])
	//			{
	//				index = k;
	//				break;
	//			}
	//		}
	//		double a1 = (ran - ft[index]) / (ft[index + 1] - ft[index]);
	//		height = HBOT + (index + a1) * DELTAH;
	//	}
	//	else if (m_parameters2.m_height_type == CATMParameters::NEW_TYPE)
	//	{
	//		height = m_random.RandNormal(m_parameters2.m_height, m_parameters2.m_height_σ);
	//		while (height<m_parameters2.m_height_lo || height>m_parameters2.m_height_hi)
	//			height = m_random.RandNormal(m_parameters2.m_height, m_parameters2.m_height_σ);

	//		//height = m_random.RandLogNormal(m_parameters2.m_height, m_parameters2.m_height_σ);
	//		//while (height<m_parameters2.m_height_lo || height>m_parameters2.m_height_hi)
	//		//	height = m_random.RandLogNormal(m_parameters2.m_height, m_parameters2.m_height_σ);
	//	}

	//	return height;
	//}

	__int64 CATMWorld::get_duration()const
	{
		double duration = 0;
		/*if (m_parameters2.m_duration_type == CATMParameters::OLD_TYPE)
		{
			duration = m_parameters2.m_duration + (m_random.Randu() - 0.5)*m_parameters2.m_duration_σ;
		}
		else if (m_parameters2.m_duration_type == CATMParameters::NEW_TYPE)
		{
		*/	//duration = m_random.RandNormal(m_parameters2.m_duration, m_parameters2.m_duration_σ);
		duration = m_random.RandBeta(m_parameters2.m_duration_α, m_parameters2.m_duration_β)*m_parameters2.m_duration_max;
		while (duration < m_parameters2.m_duration_min)//under 5 mintute, get a new flight duration
			duration = m_random.RandBeta(m_parameters2.m_duration_α, m_parameters2.m_duration_β)*m_parameters2.m_duration_max;
		
		//}


		//TEST
		duration = m_parameters2.m_duration_max;


		return __int64(duration * 3600.0); //duration h --> s
	}

	//double CATMWorld::get_w_ascent()const
	//{
	//	double ran = m_random.Rand(-1.0, 1.0);//random value [-1,1];
	//	double w = max(0.0, m_parameters2.m_w_ascent + ran*m_parameters2.m_w_ascent_σ);
	//	ASSERT(w >= 0);

	//	return w * 1000 / 3600;//convert from km/h to m/s
	//}

	//double CATMWorld::get_Wᴸ()const
	//{
	//	double Wᴸ = m_random.RandLogNormal(m_parameters2.m_w_Wᴸ, m_parameters2.m_w_σᴸ);
	//	while (Wᴸ < 0 || Wᴸ>1)
	//		Wᴸ = m_random.RandLogNormal(m_parameters2.m_w_Wᴸ, m_parameters2.m_w_σᴸ);


	//	//double Wᴸ = m_random.RandBeta(m_parameters2.m_w_Wᴸ, m_parameters2.m_w_σᴸ);
	//	//while (Wᴸ < 0 || Wᴸ>1)
	//		//Wᴸ = m_random.RandBeta(m_parameters2.m_w_Wᴸ, m_parameters2.m_w_σᴸ);


	//	return Wᴸ;
	//}
	
	size_t CATMWorld::get_S()const
	{
		double s = m_random.Randu();
		return s<=0.3 ? CATMParameters::MALE : CATMParameters::FEMALE;
	}
	
	//sex : MALE (0) or FEMALE (1)
	//out : forewing surface area [cm²]
	double CATMWorld::get_A(size_t sex)const
	{
		ASSERT(sex < CATMParameters::NB_SEX);

		static const double A_MEAN[CATMParameters::NB_SEX] = { 0.361, 0.421 };
		static const double A_SD[CATMParameters::NB_SEX] = { 0.047, 0.063 };

		
		double A = m_random.RandNormal(A_MEAN[sex], A_SD[sex]);
		while (A < 0.1 )
			m_random.RandNormal(A_MEAN[sex], A_SD[sex]);


		return A;
	}
	
	double CATMWorld::get_G()const
	{
		static const size_t NB_CLASS = 17;
		static const double P[NB_CLASS][2] =
		{
			{ 0.00, 0.611 },
			{ 0.07, 0.499 },
			{ 0.14, 0.457 },
			{ 0.21, 0.420 },
			{ 0.29, 0.379 },
			{ 0.37, 0.346 },
			{ 0.44, 0.326 },
			{ 0.53, 0.302 },
			{ 0.66, 0.279 },
			{ 0.76, 0.267 },
			{ 0.85, 0.253 },
			{ 0.92, 0.245 },
			{ 0.96, 0.225 },
			{ 0.97, 0.173 },
			{ 1.00, 0.100 },
		};

		double  r = m_random.Randu();
		
		size_t ii = NOT_INIT;
		for (size_t i = 0; i < NB_CLASS && ii == NOT_INIT; i++)
			if (r>=P[i][0])
				ii = i;

		double G = 0.1;
		if (ii < NB_CLASS-1)
			G = ((P[ii+1][0] - r)*P[ii][1] + (r - P[ii][0])*P[ii + 1][1]) / (P[ii + 1][0] - P[ii][0]);
		

		return G;
	}

	//sex : MALE (0) or FEMALE (1)
	//A : forewing surface area [cm²]
	//out : weight [g]
	double CATMWorld::get_M(size_t sex, double A)const
	{
		//double M = 0;

		//if (sex == CATMParameters::MALE)
		//{
		//	M = exp(-6.756 + 3.790*A );

		//}
		//else
		//{
		//	//exp(M°[sex] + M¹[sex] * A)*;
		//	//double Mg = 0.06128*A - 0.007697 + m_random.RandLogNormal(0, 0.00246);
		//	//double Ms = 0.00837*A - 0.000399 + m_random.RandLogNormal(0, 0.00078);

		//	double G = m_random.RandLogNormal(log(33), 0.15) / 100;
		//	while (G < 0 || G>1)
		//		G = m_random.RandLogNormal(log(30), 0.15) / 100;
		//	//double M = (1-G)*(0.00837*A - 0.000399) + G*(0.06128*A - 0.007697);

		//	M = exp(-6.465+0.974*G+2.14*A+1.305*G*A);
		//	ASSERT(M > 0);
		//}

		//static const double M_E[CATMParameters::NB_SEX] = { 0.206, 0.289 };
		//double E = m_random.RandLogNormal(0, M_E[sex]);
		//double Wᴸ = m_random.RandBeta(m_parameters2.m_w_Wᴸ, m_parameters2.m_w_σᴸ);
		//while (Wᴸ < 0 || Wᴸ>1)
		//Wᴸ = m_random.RandBeta(m_parameters2.m_w_Wᴸ, m_parameters2.m_w_σᴸ);

		static const double M_A[2] = { -6.756, -6.543 };
		static const double M_B[2] = { 3.790, 3.532 };
		static const double M_E[2] = { 0.206, 0.289 };


		double E = m_random.RandLogNormal(0, M_E[sex]);
		double M = exp(M_A[sex] + M_B[sex] * A)*E;

		
		
		return M*E;
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

		return w * 1000 / 3600;//convert from km/h to m/s
	}


	//***********************************************************************************************

	
	const double CFlyer::b[2] = { 21.35, 24.08 };
	const double CFlyer::c[2] = { 2.97, 6.63 };

	

	CFlyer::CFlyer(CATMWorld& world) :
		m_world(world)
	{
		m_sex = -1;
		m_rep = 0;
		m_loc = 0;
		m_var = 0;
		m_scale = 0;
		m_state = NOT_CREATED;
		m_end_type = NO_END_DEFINE;
		m_creation_time = 0;
		m_log.fill(0);
		m_UTCShift = 0;
	}


	void CFlyer::init()
	{
		CTRef UTCTRef = CTimeZones::LocalTRef2UTCTRef(m_localTRef, m_location);
		__int64 UTCTime = CTimeZones::UTCTRef2UTCTime(UTCTRef);
		__int64 localTime = CTimeZones::UTCTime2LocalTime(UTCTime, m_location);
		m_UTCShift = localTime - UTCTime;


		__int64 localSunset = m_world.get_local_sunset(m_localTRef, m_location);
		CTRef TRefSunset = CTimeZones::UTCTime2UTCTRef(localSunset);


		__int64 UTCSunset = CTimeZones::LocalTime2UTCTime(localSunset, m_location);
		//CTRef UTCSunsetTRef = CTimeZones::UTCTime2UTCTRef(UTCSunset);
		__int64 Δtᵀ = m_world.get_Δtᵀ(m_location, UTCSunset);
		
		if(m_sex==-1)
			m_sex = m_world.get_S();
		

		m_parameters.m_A = m_world.get_A(m_sex);
		m_parameters.m_M = m_world.get_M(m_sex, m_parameters.m_A);
		m_parameters.m_G = m_world.get_G();
		m_parameters.m_t_liftoff = UTCSunset + m_world.get_t_liftoff_offset(Δtᵀ);
		m_parameters.m_w_horizontal = m_world.get_w_horizontal();
		m_parameters.m_w_descent = m_world.get_w_descent();
		m_parameters.m_duration = m_world.get_duration();
		m_parameters.m_cruise_duration = m_world.m_parameters2.m_cruise_duration*3600;//h --> s
		
		//m_parameters.m_t_hunting = min(m_parameters.m_duration, m_world.get_t_hunthing());//attention ce n'est pa comme avant
	}

	void CFlyer::live()
	{
		ResetStat();

		if (m_state == DESTROYED)
			return;


		CTRef UTCTRef = m_world.GetUTRef();
		CTRef localTRef = UTCTRef + GetUTCShift();
		if (localTRef >= m_localTRef)
		{
			for (size_t seconds = 0; seconds < 3600; seconds += m_world.get_time_step())
			{
				if (!m_world.m_weather.IsLoaded(UTCTRef))
					m_state = IDLE_END, m_end_type = OUTSIDE_TIME_WINDOW;
				
				if (m_end_type == NO_END_DEFINE && !m_world.IsInside(m_pt))
					m_state = IDLE_END, m_end_type = OUTSIDE_MAP;
				
					

				__int64 UTCTime = m_world.get_UTC_time() + seconds;
				switch (m_state)
				{
				case NOT_CREATED:		create(UTCTRef, UTCTime); break;
				case IDLE_BEGIN:		idle_begin(UTCTRef, UTCTime); break;
				case LIFTOFF:			liftoff(UTCTRef, UTCTime); break;
				case FLIGHT:			flight(UTCTRef, UTCTime); break;
				case CRUISE:			cruise(UTCTRef, UTCTime); break;
				case LANDING:			landing(UTCTRef, UTCTime); break;
				case IDLE_END:			idle_end(UTCTRef, UTCTime);  break;
				case DESTROYED:			destroy(UTCTRef, UTCTime);  break;
				default: assert(false);
				}

			}
		}
	}



	void CFlyer::create(CTRef UTCTRef, __int64 UTCTime)
	{
		__int64 countdown = (__int64)UTCTime - m_parameters.m_t_liftoff;
		if (countdown >= -2*3600)//create object one hour and half before liftoff
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
	}

	void CFlyer::idle_begin(CTRef UTCTRef, __int64 UTCTime)
	{
		CATMVariables w = m_world.get_weather(m_pt, UTCTRef, UTCTime);


		__int64 countdown = (__int64)UTCTime - m_parameters.m_t_liftoff;
		if (countdown >= 0)
		{
			double Wmin = m_world.m_parameters2.m_Wmin * 1000 / 3600; //km/h -> m/s 
			double Tmin = m_world.m_parameters2.m_Tmin;
			double Tᴸ = m_world.m_parameters2.m_height_type == CATMParameters::WING_BEAT ? get_Tᴸ(m_parameters.m_A, m_parameters.m_M*get_MRatio()) : Tmin;
			double ws = w.get_wind_speed();

			ASSERT(!IsMissing(w[ATM_TAIR]) && !IsMissing(w[ATM_PRCP]) && !IsMissing(w[ATM_WNDU]) && !IsMissing(w[ATM_WNDV]));
			

			//if (w[ATM_PRCP] < m_world.m_parameters2.m_Pmax)
			//{
			if (w[ATM_PRCP] < m_world.m_parameters2.m_Pmax && w[ATM_TAIR] >= Tᴸ && ws >= Wmin)
			{
				m_state = LIFTOFF;
			}
			else
			{
				//__int64 duration = UTCTime - m_parameters.m_t_liftoff;
				//if (duration > (__int64)m_parameters.m_duration)
				//{
				//if (w[ATM_TAIR] < Tᴸ)							//flight abort
					//m_end_type = END_BY_TAIR;
				//else
					//m_end_type = END_BY_WNDS;
				m_end_type = NO_LIFTOFF;
				m_state = IDLE_END;
				//}
			}
			/*}
			else
			{
				m_state = IDLE_END;
				m_end_type = END_BY_RAIN;
			}*/
		}

		m_stat[S_TAIR] += w[ATM_TAIR];
		m_stat[S_PRCP] = w[ATM_PRCP];
		m_stat[S_U] += w[ATM_WNDU] * 3600 / 1000;
		m_stat[S_V] += w[ATM_WNDV] * 3600 / 1000;
		m_stat[S_W] += w[ATM_WNDW] * 3600 / 1000;
	}


	void CFlyer::liftoff(CTRef UTCTRef, __int64 UTCTime)
	{
		m_log[T_LIFTOFF] = UTCTime;
		m_state = FLIGHT;
	}


	double CFlyer::get_MRatio()const
	{
		double R = 1;

		if (m_sex == CATMParameters::FEMALE)
		{
			double M° = exp(-6.465 + 0.974 * 1 + 2.14*m_parameters.m_A + 1.305 * 1 * m_parameters.m_A);
			double M¹ = exp(-6.465 + 0.974*m_parameters.m_G + 2.14*m_parameters.m_A + 1.305*m_parameters.m_G*m_parameters.m_A);
			R = M¹ / M°;
		}

		return R;
	}

	CGeoDistance3D CFlyer::get_U(__int64 UTCTime, const CATMVariables& w)const
	{
		ASSERT(!IsMissing(w[ATM_WNDV]) && !IsMissing(w[ATM_WNDU]));

		double alpha = 0;
		if (w[ATM_WNDV] != 0 || w[ATM_WNDU] != 0)
			alpha = atan2(w[ATM_WNDV], w[ATM_WNDU]);

		if (_isnan(alpha) || !_finite(alpha))
			alpha = 0;

		if (m_state == CRUISE) //random alpha in cruise mode
		{
			if( w.get_wind_speed() < 0.7)
				alpha = m_world.random().Rand(0.0, 2 * PI);
			else
				alpha += Deg2Rad(120)*m_world.random().RandNormal(0.0, 1.0)/4.0;
		}
			

		double Ux = (w[ATM_WNDU] + cos(alpha)*m_parameters.m_w_horizontal);	//[m/s]
		double Uy = (w[ATM_WNDV] + sin(alpha)*m_parameters.m_w_horizontal);	//[m/s]
		double Uz = 0;


		switch (m_state)
		{
		case FLIGHT:	Uz = w[ATM_WNDW] + get_Uz(UTCTime, w); break;	//[m/s]; 
		case CRUISE:	Uz = m_pt.m_z>m_world.m_parameters2.m_cruise_height ?(w[ATM_WNDW] + m_parameters.m_w_descent):0; break;	//[m/s]
		case LANDING:	Uz = w[ATM_WNDW] + m_parameters.m_w_descent; break;	//[m/s]
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

		return CGeoDistance3D(Ux, Uy, Uz, m_pt.GetPrjID());
	}

	//double T : tair temperature [°C]

	double CFlyer::get_Vᵀ(double T)const
	{
		double Vmax = m_world.m_parameters2.m_Vmax * (m_sex == CATMParameters::MALE ? 1 : m_world.m_parameters2.m_w_Ex);

		double Vᴸ = 0;
		if (T > 0)
			Vᴸ = Vmax*(1 - exp(-pow(T / b[m_sex], c[m_sex])));
		

		return Vᴸ;
	}

	double CFlyer::get_Tᴸ(double A, double M)const
	{
		double K = m_world.m_parameters2.m_K;
		double Vmax = m_world.m_parameters2.m_Vmax * (m_sex == CATMParameters::MALE ? 1 : m_world.m_parameters2.m_w_Ex);
		double Vl = K* sqrt(M) / A;

		double Tᴸ = (Vl<Vmax)? b[m_sex] * pow(-log(1 - Vl / Vmax), 1.0 / c[m_sex]):40;

		ASSERT(!isnan(Tᴸ));

		return Tᴸ;
	}

	double CFlyer::get_Uz(__int64 UTCTime, const CATMVariables& w)const
	{
		ASSERT(m_state == FLIGHT);

		double Uz = 0;
		switch (m_world.m_parameters2.m_height_type)
		{
		case CATMParameters::WING_BEAT:
		{
			double K = m_world.m_parameters2.m_K;

			double Vᵀ = get_Vᵀ(w[ATM_TAIR]);
			double Vᴸ = K*sqrt(m_parameters.m_M*get_MRatio()) / m_parameters.m_A;
			Uz = m_world.m_parameters2.m_w_α*(Vᵀ - Vᴸ) * 1000 / 3600;//Uz can be negative

			break;
		}
		case CATMParameters::MAX_SPEED:
		case CATMParameters::MAX_TEMPERATURE:
		{
			
			double Tmin = m_world.m_parameters2.m_Tmin;
			//double Tᴸ = m_world.m_parameters2.m_height_type == CATMParameters::WING_BEAT ? get_Tᴸ(m_parameters.m_A, m_parameters.m_M) : Tmin;
			double v = (m_world.m_parameters2.m_height_type == CATMParameters::MAX_SPEED) ? w.get_wind_speed() : w[ATM_TAIR];

			CTRef UTCTRef = m_world.GetUTRef();
			__int64 UTCTime = m_world.get_UTC_time() + 3600/2;//evaluate wind 


			//find the layer with the maximum speed
			if (m_pt.m_z <= 50)
			{
				Uz = 0.6;// [m/s]
			}
			else
			{
				CGeoPoint3D pt¹ = m_pt;
				pt¹.m_z += 50;
				CATMVariables w¹ = m_world.get_weather(pt¹, UTCTRef, UTCTime);
				double v¹ = (m_world.m_parameters2.m_height_type == CATMParameters::MAX_SPEED) ? w¹.get_wind_speed() : w¹[ATM_TAIR];
				if (v¹ > v && w¹[ATM_TAIR] > Tmin)
				{
					Uz = min(0.6, 50.0 / m_world.get_time_step());// [m/s]
				}
				else
				{
					CGeoPoint3D pt¹ = m_pt;
					pt¹.m_z -= 50;

					CATMVariables w¹ = m_world.get_weather(pt¹, UTCTRef, UTCTime);
					double v¹ = (m_world.m_parameters2.m_height_type == CATMParameters::MAX_SPEED) ? w¹.get_wind_speed() : w¹[ATM_TAIR];
					if (v¹ > v)
						Uz = max(-2.0, -50.0 / m_world.get_time_step());// [m/s]
				}
			}
			break;
		}
		
		}
		
		return Uz ;//m/s
	}
	
	CATMVariables CFlyer::get_weather(CTRef UTCTRef, __int64 UTCTime)const
	{
		ASSERT(m_world.m_weather.IsLoaded(UTCTRef));
		ASSERT(m_pt.m_z >= 0);

		CATMVariables w;

		CGeoPoint3D pt° = m_pt;
		CATMVariables w° = m_world.get_weather(pt°, UTCTRef, UTCTime);
		CGeoDistance3D U° = get_U(UTCTime, w°);

		double dt = m_world.get_time_step(); //[s]
		CGeoPoint3D pt¹ = UpdateCoordinate(m_pt, U°*dt);


		if (m_world.m_parameters1.m_bUsePredictorCorrectorMethod &&
			m_world.m_weather.IsLoaded(UTCTRef + int(dt / 3600)) &&
			m_world.IsInside(pt¹) &&
			pt¹.m_z > 0)
		{
			CATMVariables w¹ = m_world.get_weather(pt¹, UTCTRef + int(dt / 3600), UTCTime + dt);
			w = (w° + w¹) / 2;
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

	
	void CFlyer::AddStat(const CATMVariables& w, const CGeoDistance3D& U, const CGeoDistance3D& d)
	{
		m_stat[S_TAIR] += w[ATM_TAIR];
		m_stat[S_PRCP] = w[ATM_PRCP];
		m_stat[S_U] += w[ATM_WNDU] * 3600 / 1000;
		m_stat[S_V] += w[ATM_WNDV] * 3600 / 1000;
		m_stat[S_W] += w[ATM_WNDW] * 3600 / 1000;

		m_stat[S_D_X] += d.m_x;
		m_stat[S_D_Y] += d.m_y;
		m_stat[S_D_Z] += d.m_z;
		m_stat[S_DISTANCE] += sqrt(Square(d.m_x) + Square(d.m_y));

		m_stat[S_W_HORIZONTAL] += sqrt(U.m_x*U.m_x + U.m_y*U.m_y); //horizontal speed [m/s]
		m_stat[S_W_VERTICAL] += U.m_z;	//ascent speed [m/s]
		m_stat[S_HEIGHT] += m_pt.m_z;	//flight height [m]
	}


	void CFlyer::flight(CTRef UTCTRef, __int64 UTCTime)
	{
		ASSERT(m_world.m_weather.IsLoaded(UTCTRef));
		ASSERT(m_world.IsInside(m_pt));
		ASSERT(m_state == FLIGHT);
		ASSERT(m_end_type == NO_END_DEFINE);


		CATMVariables w = get_weather(UTCTRef, UTCTime);
		if (w[ATM_PRCP] < m_world.m_parameters2.m_Pmax)
		{
			__int64 duration = UTCTime - m_parameters.m_t_liftoff;
			//bool bOverWater = false;
			//if (m_world.m_water_DS.IsOpen() && duration >= (__int64)m_parameters.m_duration)
			//{
			//	CGeoPoint pt(m_pt);
			//	if (pt.GetPrjID() != m_world.m_water_DS.GetPrjID())
			//	{
			//		ASSERT(pt.IsGeographic());
			//		pt.Reproject(m_world.m_GEO2DEM);
			//	}

			//	CGeoPointIndex xy = m_world.m_water_DS.GetExtents().CoordToXYPos(pt);
			//	bOverWater = m_world.m_water_DS.ReadPixel(0, xy) != 0;
			//}

			////|| bOverWater
			//if (duration < (__int64)m_parameters.m_duration || bOverWater)
			if (duration < (__int64)m_parameters.m_duration)
			{
				double dt = m_world.get_time_step(); //[s]
				

				//bool bBoostU = false;
				//if (duration < 120)//the first 2 min, the moth try to go to the wind
				//{
				//	double Wmin = max(0.0, min(4.0, m_world.m_parameters2.m_Wmin * 1000 / 3600)); //km/h -> m/s 
				//	double ws = w.get_wind_speed();

				//	// 0.7 m/s -> 60 m
				//	// 4 m/s -> 0 m
				//	//double h = max(0.0, min(60.0, 60.0 - (ws - Wmin) * 60 / (4.0 - Wmin)));
				//	//if (m_pt.m_z < h)
				//	if (ws >= Wmin && ws < 3.0)
				//		bBoostU = true;
				//}


				CGeoDistance3D U = get_U(UTCTime, w);

				//if (bBoostU)
					//U.m_z = max(U.m_z, 0.6);//after greenbank 0.6 m/s * t -> m

				CGeoDistance3D d = U*dt;



				((CGeoPoint3D&)m_pt) = UpdateCoordinate(m_pt, d);
				((CGeoPoint3D&)m_newLocation) = UpdateCoordinate(m_newLocation, d);

				//if (m_pt.m_z <= m_world.m_parameters2.m_cruise_height)
				//{
					//m_state = CRUISE;
					//if (m_world.is_over_host(m_pt))//look to find host
					//m_end_type = FIND_HOST;
					//else if (m_world.is_over_distraction(m_pt))//look for distraction
					//m_end_type = FIND_DISTRACTION;
				//}

				if (m_pt.m_z <= 5)
				{
					m_state = LANDING;

					double delta = 5 - m_pt.m_z;
					m_pt.m_z += delta;
					m_newLocation.m_z += delta;
				}

				AddStat(w, U, d);
			}
			else
			{
				//m_state = CRUISE;
				m_state = LANDING;
			}
		}
		else
		{
			m_state = LANDING;
			m_end_type = END_BY_RAIN;
		}

	}


	void CFlyer::cruise(CTRef UTCTRef, __int64 UTCTime)
	{
		ASSERT(m_world.m_weather.IsLoaded(UTCTRef));
		ASSERT(m_world.IsInside(m_pt));
		ASSERT(m_state == CRUISE);
		ASSERT(m_end_type == NO_END_DEFINE);

		if (m_log[T_CRUISE] == 0)
			m_log[T_CRUISE] = UTCTime;

		CATMVariables w = get_weather(UTCTRef, UTCTime);
		if (w[ATM_PRCP] < m_world.m_parameters2.m_Pmax)
		{
			
			bool bOverWater = false;
			if (m_world.m_water_DS.IsOpen() )//&& duration >= (__int64)m_parameters.m_cruise_duration
			{
				CGeoPoint pt(m_pt);
				if (pt.GetPrjID() != m_world.m_water_DS.GetPrjID())
				{
					ASSERT(pt.IsGeographic());
					pt.Reproject(m_world.m_GEO2DEM);
				}

				CGeoPointIndex xy = m_world.m_water_DS.GetExtents().CoordToXYPos(pt);
				bOverWater = m_world.m_water_DS.ReadPixel(0, xy) != 0;
			}

			__int64 duration = UTCTime - m_log[T_CRUISE];
			if (duration < (__int64)(m_parameters.m_cruise_duration) || bOverWater)
			{
				double dt = m_world.get_time_step(); //[s]

				CGeoDistance3D U = get_U(UTCTime, w);
				CGeoDistance3D d = U*dt;


				((CGeoPoint3D&)m_pt) = UpdateCoordinate(m_pt, d);
				((CGeoPoint3D&)m_newLocation) = UpdateCoordinate(m_newLocation, d);
				
				if (m_world.is_over_distraction(m_pt))//look for distraction
				{
					m_state = LANDING;
					m_end_type = FIND_DISTRACTION;
				}
				else if (m_world.is_over_host(m_pt))//look to find host
				{
					m_state = LANDING;
					m_end_type = FIND_HOST;
				}
				
				
				/*if (m_pt.m_z < 5)
				{
					m_state = LANDING;

					double delta = 5 - m_pt.m_z;
					m_pt.m_z += delta;
					m_newLocation.m_z += delta;
				}*/

				AddStat(w, U, d);

			}
			else
			{

				m_state = LANDING;
				m_end_type = END_OF_TIME_FLIGHT;
			}
		}
		else
		{
			m_state = LANDING;
			m_end_type = END_BY_RAIN;
		}
	}


	void CFlyer::landing(CTRef UTCTRef, __int64 UTCTime)
	{
		ASSERT(m_world.m_weather.IsLoaded(UTCTRef));
		ASSERT(m_world.IsInside(m_pt));

		ASSERT(m_state == LANDING);

		double dt = m_world.get_time_step(); //[s]
		CATMVariables w = get_weather(UTCTRef, UTCTime);
		CGeoDistance3D U = get_U(UTCTime, w);
		CGeoDistance3D d = U*dt;

		((CGeoPoint3D&)m_pt) = UpdateCoordinate(m_pt, d);
		((CGeoPoint3D&)m_newLocation) = UpdateCoordinate(m_newLocation, d);
		

		if (m_pt.m_z <= 0 && m_end_type == NO_END_DEFINE)
		{
			double Tmin = m_world.m_parameters2.m_Tmin;
			double Tᴸ = m_world.m_parameters2.m_height_type == CATMParameters::WING_BEAT ? get_Tᴸ(m_parameters.m_A, m_parameters.m_M*get_MRatio()) : Tmin;

			if (w[ATM_PRCP] > m_world.m_parameters2.m_Pmax)
				m_end_type = END_BY_RAIN;
			else if (w[ATM_TAIR] < Tᴸ)
				m_end_type = END_BY_TAIR;
			else
				m_end_type = END_OF_TIME_FLIGHT;
		}

		if (m_end_type != NO_END_DEFINE)
		{
			m_log[T_LANDING] = UTCTime;
			m_state = IDLE_END;

			//end at zero
			m_newLocation.m_z -= m_pt.m_z;
			m_pt.m_z -= m_pt.m_z;
		}

		ASSERT(m_pt.m_z >= 0);
		AddStat(w, U, d);
	}

void CFlyer::idle_end(CTRef UTCTRef, __int64 UTCTime)
{
	if (m_log[T_IDLE_END] == 0)
		m_log[T_IDLE_END] = UTCTime;

	if (UTCTime - m_log[T_IDLE_END] > 3600)
		m_state = DESTROYED;

	ASSERT(m_end_type != NO_END_DEFINE);
}

void CFlyer::destroy(CTRef UTCTRef, __int64 UTCTime)
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

	size_t dimSize = 2;
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
CATMVariables CATMWeatherCuboids::get_weather(const CGeoPoint3D& pt, __int64 time)const
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

CATMVariables CATMWeather::get_weather(const CGeoPoint3D& pt, CTRef UTCTRef, __int64 UTCTime)const
{
	ASSERT(pt.m_z >= 0);
	ASSERT(pt.IsGeographic());

	CATMVariables w1;
	if (m_world.m_parameters1.m_weather_type == CATMWorldParamters::FROM_GRIBS ||
		m_world.m_parameters1.m_weather_type == CATMWorldParamters::FROM_BOTH)
	{
		CGeoPoint3D pt2(pt);
		if (GetPrjID() == m_world.m_GEO2WEA.GetDst()->GetPrjID())
			pt2.Reproject(m_world.m_GEO2WEA);
		//else
			//pt2.Reproject(GetReProjection(PRJ_WGS_84, GetPrjID()));
		
		CATMWeatherCuboidsPtr p_cuboid = get_cuboids(pt2, UTCTRef, UTCTime);
		w1 = p_cuboid->get_weather(pt2, UTCTime);
	}
	
	CATMVariables w2;
	if (m_world.m_parameters1.m_weather_type == CATMWorldParamters::FROM_STATIONS ||
		m_world.m_parameters1.m_weather_type == CATMWorldParamters::FROM_BOTH)
	{
		w2 = get_station_weather(pt, UTCTRef, UTCTime);
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

__int64 CATMWorld::get_local_sunset(CTRef TRef, const CLocation& loc)
{
	//Get time at the begin of the day and add sunset
	__int64 sunsetTime = CTimeZones::LocalTRef2LocalTime(CTRef(TRef.GetYear(), TRef.GetMonth(), TRef.GetDay(), 0), loc);
	__int64 zone = CTimeZones::GetDelta(TRef, loc)/3600;
	
	CSun sun(loc.m_lat, loc.m_lon, zone);
	sunsetTime += __int64(sun.GetSunset(TRef)*3600);
	
	return sunsetTime;
}

__int64 CATMWorld::get_Δtᵀ(const CLocation& loc, __int64 UTCSunset)const
{
	__int64 h4 = 3600 * 4;
	__int64 Δtᵀ = h4;

	//__int64 UTCTime = CTimeZones::UTCTRef2UTCTime(UTCTRef);
	//UTCp += CTimeZones::UTCTime2UTCTRef(UTCLiftoff);

	__int64 to = UTCSunset - h4;//substract 4 hours
	__int64 tm = UTCSunset + h4;//add 4 hours

	for (__int64 t = to; t <= tm && Δtᵀ == h4; t += m_parameters1.m_time_step)
	{
		CTRef UTCTRef = CTimeZones::UTCTime2UTCTRef(t);
		if (m_weather.IsLoaded(UTCTRef))
		{
			CATMVariables v = m_weather.get_weather(loc, UTCTRef, t);

			if (v[ATM_TAIR] <= T°)
				Δtᵀ = t - UTCSunset;
		}
	}

	return Δtᵀ;
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

CATMVariables CATMWeather::get_station_weather(const CGeoPoint3D& pt, CTRef UTCTRef, __int64  UTCTime)const
{
	//CTRef UTCTRef = CTimeZones::UTCTime2UTCTRef(UTCTime);
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
	
	//if(m_iwd.find(UTCTRef) == m_iwd.end())
		//LoadWeather(UTCTRef, callback);

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
		if (b != NOT_INIT)
		{
			double gph = m_p_weather_DS.GetPixel(UTCTRef, b, xy); //geopotential height [m]
			test.push_back(make_pair(gph, l));

			if (alt < gph)
				break;
		}
	}

	double grAlt = GetGroundAltitude(xy, UTCTRef);//get the first level over the ground
	if (grAlt>-999)
		test.push_back(make_pair(grAlt, 0));

	sort(test.begin(), test.end());

	int L = NB_LEVELS-1;
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
		double gph = m_p_weather_DS.GetPixel(UTCTRef, b, xy); //geopotential height [m]

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
	ASSERT(b != NOT_INIT);
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


CATMWeatherCuboidsPtr CATMWeather::get_cuboids(const CGeoPoint3D& ptIn, CTRef UTCTRef, __int64 UTCTime)const
{
	ASSERT(IsLoaded(CTimeZones::UTCTime2UTCTRef(UTCTime)) && IsLoaded(CTimeZones::UTCTime2UTCTRef(UTCTime) + 1));
	ASSERT(ptIn.m_z>=0);

	CATMWeatherCuboidsPtr cuboids(new CATMWeatherCuboids);
	cuboids->m_bUseSpaceInterpolation = m_world.m_parameters1.m_bUseSpaceInterpolation;
	cuboids->m_bUseTimeInterpolation = m_world.m_parameters1.m_bUseTimeInterpolation;
	
	
	if (!IsLoaded(UTCTRef) )
		return cuboids;//humm

	ASSERT(m_p_weather_DS.get_band(UTCTRef, ATM_WNDW, 0) != UNKNOWN_POS || m_p_weather_DS.get_band(UTCTRef, ATM_VVEL, 0) != UNKNOWN_POS);
	//if have VVEL, then it's RUC otherwise it's WRF
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
		//if (gribType == RUC_TYPE)
		groundAlt = m_world.GetGroundAltitude(ptIn);
		

		CGeoPoint3D pt(ptIn);
		pt.m_z += groundAlt;

		
		ASSERT(extents.IsInside(pt));

		CGeoPointIndex xy1 = get_xy(pt, UTCTRef);

		for (size_t z = 0; z < NB_POINTS_Z; z++)
		{
			for (size_t y = 0; y < NB_POINTS_Y; y++)
			{
				for (size_t x = 0; x < NB_POINTS_X; x++)
				{
					CGeoPointIndex xy2 = xy1 + CGeoPointIndex((int)x, (int)y);
					int L = get_level(xy2, pt.m_z, UTCTRef, z==0);
					ASSERT(L >= 0 && L < NB_LEVELS);

					
					if (xy2.m_x >= extents.m_xSize)
						xy2.m_x = extents.m_xSize - 1;
					if (xy2.m_y >= extents.m_ySize)
						xy2.m_y = extents.m_ySize - 1;
					if (L<0 || L > MAX_GEOH)
						L = MAX_GEOH;

					((CGeoPoint&)(*cuboids)[i].m_pt[z][y][x]) = extents.GetPixelExtents(xy2).GetCentroid();//Get the center of the cell

					size_t bGph = m_p_weather_DS.get_band(UTCTRef, ATM_HGT, L);
					double gph = m_p_weather_DS.GetPixel(UTCTRef, bGph, xy2); //geopotential height [m]
					(*cuboids)[i].m_pt[z][y][x].m_z = gph - groundAlt;

					for (size_t v = 0; v < ATM_WATER; v++)
					{
						bool bConvertVVEL = false;
						size_t b = m_p_weather_DS.get_band(UTCTRef, v, L);

						if (b == UNKNOWN_POS && v == ATM_WNDW)
						{
							//ASSERT(gribType == RUC_TYPE);
							b = m_p_weather_DS.get_band(UTCTRef, ATM_VVEL, L);
							ASSERT(b != NOT_INIT);
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

							if (v == ATM_PRES)
							{
								assert(L > 0 && L <= MAX_GEOH);
								double P = 1013 * pow((293 - 0.0065*gph) / 293, 5.26);//pressure in hPa

								(*cuboids)[i][z][y][x][v] = P;
							}
						}
					}//variable
					
					//CGeoPoint geopt = (*cuboids)[i].m_pt[z][y][x];
					//geopt.Reproject(m_world.m_WEA2GEO);
					//Convert2ThrueNorth(geopt, (*cuboids)[i][z][y][x][ATM_WNDU], (*cuboids)[i][z][y][x][ATM_WNDV]);
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
		callback.PushTask("Load Gribs", length);
		//callback.SetNbStep(length);

		for (CSVIterator loop(file); loop != CSVIterator() && msg; ++loop)
		{
			if ((*loop).size() == 2)
			{
				CTRef TRef;
				TRef.FromFormatedString((*loop)[0],"","-",1);
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

	//CreateGribsFromText(callback);
	//CreateGribsFromNetCDF(callback);
	//return ERMsg(ERMsg::ERREUR, "Fin de la transformation");


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
				m_world.m_WEA2GEO = GetReProjection(m_extents.GetPrjID(), PRJ_WGS_84);
				// (m_extents.GetPrjID() != m_world.m_DEM_DS.GetPrjID())
				//{
					//m_world.m_WEA2DEM = GetReProjection(m_extents.GetPrjID(), m_world.m_DEM_DS.GetPrjID());
					//m_extents.Reproject(m_world.m_WEA2DEM);
					//callback.AddMessage("WARNING: the projection of the DEM is not the same as the projection of the weather gribs. Severe bias in wind direction can be observed.");
				//}
			}
		}
	}
	else
	{
		m_extents = m_world.m_DEM_DS.GetExtents();
		m_world.m_GEO2WEA = m_world.m_GEO2DEM;
		m_world.m_WEA2GEO = GetReProjection(m_world.m_GEO2DEM.GetDst()->GetPrjID(), m_world.m_GEO2DEM.GetSrc()->GetPrjID());
	}

	if (!hourlyDBFilepath.empty())
	{
		if (!m_p_hourly_DB)
		{
			msg += load_hourly(hourlyDBFilepath, callback);
			if (msg)
			{
				if (m_world.m_GEO2WEA.GetSrc() == NULL)
				{
					m_world.m_GEO2WEA = GetReProjection(PRJ_WGS_84, PRJ_WGS_84);
					m_world.m_WEA2GEO = GetReProjection(PRJ_WGS_84, PRJ_WGS_84);
				}
					

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
				msg.ajoute("File for " + UTCTRef.GetFormatedString("%Y-%m-%d-%H") + " is Missing");
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
					//UTCTRef.Transform(CTM(CTM::DAILY));
					callback.AddMessage("WARNING: Unable to fing a good starting Gribs weather for " + UTCTRef.GetFormatedString("%Y-%m-%d") + " (UTC)");
					m_bSkipDay = true;
					return msg;
				}
			}
			
			msg += callback.StepIt(0);
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
					msg = m_p_hourly_DB->Search(result, it->m_newLocation, m_world.m_parameters2.m_nb_weather_stations * 5, -1,CWVariables(FILTER_STR[v]), year);
					for (size_t ss = 0; ss < result.size(); ss++)
						indexes.insert(result[ss].m_index);

					msg += callback.StepIt(0);
				}
			}
			//callback.PopTask();

			//callback.PushTask("Load weather...", indexes.size() + NB_ATM_VARIABLES*indexes.size());
			//pre-load weather
			//callback.PushTask("Load stations", indexes.size());
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

			//callback.PopTask();

			//create IWD object
			//callback.PushTask("Load IWD", NB_ATM_VARIABLES*indexes.size());
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

							double val = v == ATM_WNDU ? U : V ;
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
						msg.ajoute("Too mush missing value for " + UTCTRef.GetFormatedString("%Y-%m-%d") + " (UTC)");
					}
				}

				m_iwd[UTCTRef][v].SetDataset(pts);
				m_iwd[UTCTRef][v].SetParam(param);
				msg += m_iwd[UTCTRef][v].Initialization();
				
			}//for all variables

			//callback.PopTask();
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

	if (!empty())
	{
		callback.PushTask("Discard weather for " + begin()->first.GetFormatedString("%Y-%m-%d"), size());

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

//******************************************************************************************************
const char* CATMWorldParamters::MEMBERS_NAME[NB_MEMBERS] = { "WeatherType", "TimeStep", "Seed", "Reversed", "UseSpaceInterpol", "UseTimeInterpol", "UsePredictorCorrectorMethod", "UseTurbulance", "UseVerticalVelocity", "EventThreshold", "DefoliationThreshold", "DistractionThreshold", "HostThreshold", "DEM", "WaterLayer", "Gribs", "HourlyDB", "Defoliation", "Distraction", "Host" };


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
		//p.Begin() += 12;//begin at noon 
		p.End() += 12 + 23;//finish at 11:00 on day after
	}

	return p;
}

set<CTRef> CATMWorld::get_TRefs( int year)const
{
	set<CTRef> TRefs;
	for (CFlyersCIt it = m_flyers.begin(); it != m_flyers.end(); it++)
	{
		if (year == YEAR_NOT_INIT || it->m_localTRef.GetYear() == year)
		{
			CTRef TRef = it->m_localTRef;//local time 
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
			//if (v != m_defoliation_DS.GetNoData(0)) no data is no defoliation
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
		CTRef TRef = it->m_localTRef;
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
		__int64  UTCLiftoff = (__int64)floor(fls[i]->P().m_t_liftoff);
		__int64  UTCLanding = (__int64)ceil(UTCLiftoff + fls[i]->P().m_duration);
		UTCp += CTimeZones::UTCTime2UTCTRef(UTCLiftoff);
		UTCp += CTimeZones::UTCTime2UTCTRef(UTCLanding) + 1 + 1;//add tree extra hours for landing and cuboid
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
				size_t UTCSunset = CTimeZones::LocalTime2UTCTime(localSunset, fls[i]->m_location);
				CTRef UTCTRefSunset = CTimeZones::UTCTime2UTCTRef(UTCSunset);
				for (int h = -4; h <= 4; h++)
					sunsetTRef.insert(UTCTRefSunset+h);
				//sunsetTRef.insert(UTCTRefSunset+1);
			}

			//in progress step
			CTRef TRef = *it;
			callback.PushTask("Load sunset weather for " + TRef.GetFormatedString("%Y-%m-%d"), sunsetTRef.size());

			//load sunset weather
			for (set<CTRef>::const_iterator itSunset = sunsetTRef.begin(); itSunset != sunsetTRef.end() && msg; itSunset++)
			{
				//load weather at sunset
				msg += m_weather.LoadWeather(*itSunset, callback);
				msg += callback.StepIt();
			}

			callback.PopTask();

			if (msg && !m_weather.SkipDay())
			{
				//init all flyers
				for (size_t i = 0; i < fls.size() && msg; i++)
				{
					fls[i]->init();
				}

				//get simulation hours for this day
				CTPeriod UTC_period = get_UTC_period(fls);
				ASSERT(UTC_period.IsInit()); 
				
				int nbSteps = 0;
				for (CTRef UTCTRef = UTC_period.Begin(); UTCTRef <= UTC_period.End() && msg; UTCTRef++)
					if (!m_weather.IsLoaded(UTCTRef))
						nbSteps++;

				callback.PushTask("Load weather for " + TRef.GetFormatedString("%Y-%m-%d"), nbSteps);

				//pre-Load weather for the day
				for (CTRef UTCTRef = UTC_period.Begin(); UTCTRef <= UTC_period.End() && msg; UTCTRef++)
				{
					if (!m_weather.IsLoaded(UTCTRef))
						msg += m_weather.LoadWeather(UTCTRef, callback);
					
					msg += callback.StepIt();
				}

				callback.PopTask();

				//Simulate dispersal
				callback.PushTask("Dispersal for " + TRef.GetFormatedString("%Y-%m-%d"), UTC_period.size()*fls.size());

				
				for (m_UTCTTime = CTimeZones::UTCTRef2UTCTime(m_UTCTRef = UTC_period.Begin()); m_UTCTRef <= UTC_period.End() && msg; m_UTCTRef++, m_UTCTTime += 3600)
				{
#pragma omp parallel for if (m_parameters1.m_weather_type == CATMWorldParamters::FROM_GRIBS) //est-ce que ça cause encore des problèmes??????
					for (__int64 i = 0; i < (__int64 )fls.size(); i++)
					{
#pragma omp flush(msg)
						if (msg)
						{
							CFlyer& flyer = *(fls[i]);
							flyer.live();
							
							if (flyer.GetState() > CFlyer::NOT_CREATED && flyer.GetState() < CFlyer::DESTROYED)
							{
								CTRef TRef = m_UTCTRef + flyer.GetUTCShift();
								if (output[flyer.m_rep][flyer.m_loc][flyer.m_var].IsInside(TRef))
								{
									CGeoPoint3D pt = flyer.m_pt;
									pt.Reproject(m_GEO2DEM);//convert from GEO to DEM projection

									output[flyer.m_rep][flyer.m_loc][flyer.m_var][TRef][ATM_SEX] = flyer.m_sex;
									output[flyer.m_rep][flyer.m_loc][flyer.m_var][TRef][ATM_STATE] = (flyer.GetState() == CFlyer::IDLE_END) ? 10 + flyer.GetEnd() : flyer.GetState();
									output[flyer.m_rep][flyer.m_loc][flyer.m_var][TRef][ATM_X] = pt.m_x;
									output[flyer.m_rep][flyer.m_loc][flyer.m_var][TRef][ATM_Y] = pt.m_y;
									output[flyer.m_rep][flyer.m_loc][flyer.m_var][TRef][ATM_LAT] = flyer.m_newLocation.m_lat;
									output[flyer.m_rep][flyer.m_loc][flyer.m_var][TRef][ATM_LON] = flyer.m_newLocation.m_lon;
									output[flyer.m_rep][flyer.m_loc][flyer.m_var][TRef][ATM_T] = flyer.GetStat(CFlyer::S_TAIR);
									output[flyer.m_rep][flyer.m_loc][flyer.m_var][TRef][ATM_P] = flyer.GetStat(CFlyer::S_PRCP);
									output[flyer.m_rep][flyer.m_loc][flyer.m_var][TRef][ATM_U] = flyer.GetStat(CFlyer::S_U);
									output[flyer.m_rep][flyer.m_loc][flyer.m_var][TRef][ATM_V] = flyer.GetStat(CFlyer::S_V);
									output[flyer.m_rep][flyer.m_loc][flyer.m_var][TRef][ATM_W] = flyer.GetStat(CFlyer::S_W);
									output[flyer.m_rep][flyer.m_loc][flyer.m_var][TRef][ATM_SCALE] = flyer.m_scale;

									//size_t time = 0;
									if (flyer.GetLog(CFlyer::T_LIFTOFF) > 0)
									//if (flyer.GetState() > CFlyer::LIFTOFF)
									{
										//time = m_UTCTTime + 3600;//time at the end of hour simulation
										//if (flyer.GetLog(CFlyer::T_LANDING) > 0)
											

										output[flyer.m_rep][flyer.m_loc][flyer.m_var][TRef][ATM_MEAN_HEIGHT] = flyer.GetStat(CFlyer::S_HEIGHT);
										output[flyer.m_rep][flyer.m_loc][flyer.m_var][TRef][ATM_CURRENT_HEIGHT] = flyer.m_pt.m_z;
										output[flyer.m_rep][flyer.m_loc][flyer.m_var][TRef][ATM_DELTA_HEIGHT] = flyer.GetStat(CFlyer::S_D_Z, SUM);
										
										output[flyer.m_rep][flyer.m_loc][flyer.m_var][TRef][ATM_W_HORIZONTAL] = flyer.GetStat(CFlyer::S_W_HORIZONTAL) * 3600 / 1000;
										output[flyer.m_rep][flyer.m_loc][flyer.m_var][TRef][ATM_W_VERTICAL] = flyer.GetStat(CFlyer::S_W_VERTICAL) * 3600 / 1000;

										double alpha = PI / 2;
										if (flyer.GetStat(CFlyer::S_D_Y) != 0 || flyer.GetStat(CFlyer::S_D_X) != 0)
											alpha = atan2(flyer.GetStat(CFlyer::S_D_Y), flyer.GetStat(CFlyer::S_D_X));


										double angle = int(360 + 90 - Rad2Deg(alpha)) % 360;
										ASSERT(angle >= 0 && angle <= 360);
										output[flyer.m_rep][flyer.m_loc][flyer.m_var][TRef][ATM_DIRECTION] = angle;

										double D° = flyer.m_newLocation.GetDistance(flyer.m_location, false);
										output[flyer.m_rep][flyer.m_loc][flyer.m_var][TRef][ATM_DISTANCE] = flyer.GetStat(CFlyer::S_DISTANCE, SUM);
										output[flyer.m_rep][flyer.m_loc][flyer.m_var][TRef][ATM_DISTANCE_FROM_OIRIGINE] = D°;

										
										//flyer.GetState() == CFlyer::IDLE_END && 
										if (flyer.GetLog(CFlyer::T_LANDING) > 0)
										{
											//time = flyer.GetLog(CFlyer::T_LANDING);
											size_t liftoffTime = CTimeZones::UTCTime2LocalTime(flyer.GetLog(CFlyer::T_LIFTOFF), flyer.m_location);
											size_t landingTime = CTimeZones::UTCTime2LocalTime(flyer.GetLog(CFlyer::T_LANDING), flyer.m_location);
											output[flyer.m_rep][flyer.m_loc][flyer.m_var][TRef][LIFTOFF_TIME] = CTimeZones::GetDecimalHour(liftoffTime);
											output[flyer.m_rep][flyer.m_loc][flyer.m_var][TRef][ATM_FLIGHT_TIME] = (landingTime - liftoffTime) / 3600.0;
											output[flyer.m_rep][flyer.m_loc][flyer.m_var][TRef][LANDING_TIME] = CTimeZones::GetDecimalHour(landingTime);
										}

									}
								}
								else
								{
								//	callback.PushTask("Discard weather for " + TRef.GetFormatedString("%Y-%m-%d"), UTC_period.size());
								}
							}//if flying

							callback.WaitPause();

							msg += callback.StepIt();
						}//if msg
					}//for all flyers
				}//for all valid hours

				callback.PopTask();

				
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
		poBand->FlushCache();
		poBand->FlushBlock();

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
	else if (strVar == "PRATE" || strVar == "APCP")
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
		bool bValid = strLevel.find('-') == string::npos && strLevel.length() >= 6 && isdigit(strLevel[0]) && strLevel.substr(strLevel.length()-2)=="mb";
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
	if (m_bands[ATM_VVEL][1] != UNKNOWN_POS && m_bands[ATM_VVEL][0] == UNKNOWN_POS)
	{
		m_bands[ATM_VVEL][0] = m_bands[ATM_VVEL][1];
	}


	return msg;
}

size_t CGDALDatasetCached::get_band(size_t v, size_t level )const
{
	ASSERT(level < NB_LEVELS);
	return m_bands[v][level];
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
	
	static const size_t NB_WRF_HOURS= 289;//289 hours
	callback.PushTask("Create gribs", NB_WRF_HOURS);
	
	for (size_t h = 0; h < NB_WRF_HOURS&&msg; h++)
	{
		CTRef UTCRef(2013, JULY, DAY_13, 0);
		UTCRef += int(h);

		callback.PushTask(UTCRef.GetFormatedString("%Y-%m-%d-%H"), 50652* NB_WRF_LEVEL);
		//callback.SetNbStep(7257 * NB_WRF_LEVEL);

		std::string filePathIn = FormatA("E:\\Travaux\\Bureau\\WRF2013\\WRF\\wrfbud_%03d.txt", h);
		CGDALDatasetEx geotif;
		



		string filePathOut = FormatA("E:\\Travaux\\Bureau\\WRF2013\\TIF\\WRF_%4d_%02d_%02d_%02d.tif", UTCRef.GetYear(), UTCRef.GetMonth()+1, UTCRef.GetDay()+1, UTCRef.GetHour());
		msg += geotif.CreateImage(filePathOut, options);
		//create .inv file
		string filePathInvIn = "E:\\Travaux\\Bureau\\WRF2013\\WRF\\template.inv";
		string filePathInvOut = FormatA("E:\\Travaux\\Bureau\\WRF2013\\TIF\\WRF_%4d_%02d_%02d_%02d.inv", UTCRef.GetYear(), UTCRef.GetMonth()+1, UTCRef.GetDay()+1, UTCRef.GetHour());
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
						ASSERT(str.size() == 5);
						
						for (size_t j = 0; j < NB_WRF_VARS; j++)
							csv_text[j] += str[2] + "," + str[0] + "," + str[1] + "," + str[4];

						CGeoPoint lastPoint(ToDouble(str[1]), ToDouble(str[0]), PRJ_WGS_84);
						msg += lastPoint.Reproject(Geo2LCC);

						ASSERT(geotif.GetExtents().IsInside(lastPoint));
						xy = geotif.GetExtents().CoordToXYPos(lastPoint);
						
						
						i=0;//restart cycle
						
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
							ASSERT(tmp.size()==2);

							str[3] = tmp[0];
							str.insert(str.begin()+4, tmp[1]);
						}
						ASSERT(str.size() == NB_WRF_VARS+1);

						size_t sounding = ((i-1)% NB_WRF_LEVEL);
						ASSERT(sounding < NB_WRF_LEVEL);
						ASSERT(sounding == ToInt(str[0])-1);
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
								v -= 273.15f;//convert K to °C
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

				string path = "E:\\Travaux\\Bureau\\WRF2013\\CSV\\";
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
	options.m_nbBands = NB_WRF_LEVEL * (NB_WRF_VARS-1) + 1;//prcp have only one band

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
		for (size_t i = 0; i < geotifIn.size()&&msg; i++)
		{
			string filePathIn = WBSF::FormatA("E:\\Travaux\\Bureau\\WRF2013\\NETCDF\\wrfout_d02_%d-%02d-%02d_%02d.nc", UTCRef.GetYear(), UTCRef.GetMonth()+1, UTCRef.GetDay()+1, UTCRef.GetHour());
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
									data[s][y][x] = ((phb1[y][x] + phb2[y][x]) / 2 + (ph1[y][x] + ph2[y][x])/2) / 9.8;//convert m²/s² into m
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

						//convert Kelvin to °C
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
									data[s][y][x] = (V[y][x] + V[y+1][x]) / 2;
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
						GDALRasterBand* pBand = geotifIn[l == 0 ? WRF_Q2 : WRF_QVAPOR].GetRasterBand(l == 0 ? 0:l-1);

						array<array<float, 252>, 201> W = { 0 };
						pBand->RasterIO(GF_Read, 0, 0, (int)W[0].size(), (int)W.size(), &(W[0][0]), (int)W[0].size(), (int)W.size(), GDT_Float32, 0, 0);
						
						for (size_t y = 0; y < data[s].size(); y++)
						{
							for (size_t x = 0; x < data[s][y].size(); x++)
							{
								double T = data[WRF_TAIR][y][x];//°C
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


//	//Open input
//	NcFilePtr ncFile;
//
//	try
//	{
//		string NCfilePathIn = FormatA("E:\\Travaux\\Bureau\\WRF2013\\NetCDF\\wrfout_d02_%4d-%02d-%02d_%02d.tif", UTCRef.GetYear(), UTCRef.GetMonth() + 1, UTCRef.GetDay() + 1, UTCRef.GetHour());
//		ncFile = NcFilePtr(new NcFile(NCfilePathIn, NcFile::read));
//	}
//	catch (...)
//	{
//		msg.ajoute("Unable to open input NetCDF file");
//		return msg;
//	}
//
//
//	NcVar& var1 = ncFile1[0]->getVar(VARIABLES_NAMES[0]);
//	NcVar& var2 = ncFile2[0]->getVar(VARIABLES_NAMES[0]);
//
//
//	size_t d1[NB_DIMS] = { 0 };
//	size_t d2[NB_DIMS] = { 0 };
//	for (int i = 0; i<NB_DIMS; i++)
//	{
//		d1[i] = var1.getDim(i).getSize();
//		d2[i] = var2.getDim(i).getSize();
//	}
//
//	ASSERT(d1[DIM_LON] == d2[DIM_LON]);
//	ASSERT(d1[DIM_LAT] == d2[DIM_LAT]);
//
//
//	//open output
//	CBaseOptions options;
//	GetOptions(options);
//
//	ASSERT(d1[DIM_LON] == options.m_extents.m_xSize);
//	ASSERT(d1[DIM_LAT] == options.m_extents.m_ySize);
//
//	CGDALDatasetEx grid[NB_FIELDS];
//	for (int v = 0; v<NB_FIELDS; v++)
//	{
//		string filePathOut = MMG.GetFilePath(v);
//		if (!filePathOut.empty())
//			msg += grid[v].CreateImage(filePathOut, options);
//	}
//
//	if (!msg)
//		return msg;
//
//
//	callback.SetCurrentDescription(RCP_NAME[rcp]);
//	callback.SetNbStep(options.m_nbBands * 5);
//
//
//	//********************************************
//	CMonthlyVariable output;
//	for (int v = 0; v < NB_FIELDS; v++)
//	{
//		if (grid[v].IsOpen())
//			output[v].resize(d1[DIM_LAT] * d1[DIM_LON]);
//	}
//
//
//	for (size_t b = 0; b < options.m_nbBands; b++)
//		//int b = (2100-1951+1)*12 - 1;
//	{
//		size_t* d = (b < d1[DIM_TIME] ? d1 : d2);
//		size_t base = (b < d1[DIM_TIME] ? 0 : d1[DIM_TIME]);
//		vector<size_t> startp(NB_DIMS);
//		vector<size_t> countp(NB_DIMS);
//
//		for (size_t j = 0; j < NB_DIMS; j++)
//		{
//			startp[j] = (j == DIM_TIME ? b - base : 0);
//			countp[j] = (j == DIM_TIME ? 1 : d[j]);//j==0 : TIME; only one month at a time
//		}
//
//
//
//		//read all data for this month
//		for (int v = 0, vv = 0; v<NB_FIELDS; v++)
//		{
//			if (!output[v].empty())
//			{
//
//				NcVar& var1 = ncFile1[vv]->getVar(VARIABLES_NAMES[vv]);
//				NcVar& var2 = ncFile2[vv]->getVar(VARIABLES_NAMES[vv]);
//				NcVar& var = (b < d1[DIM_TIME] ? var1 : var2);
//
//				var.getVar(startp, countp, &(output[v][0]));
//				vv++;
//
//				msg += callback.StepIt();
//			}
//		}
//
//		//convert wind speed and comput dew point temperature
//		ConvertData(output);
//
//		//save data
//		for (int v = 0; v<NB_FIELDS; v++)
//		{
//			if (!output[v].empty())
//			{
//				GDALRasterBand* pBand = grid[v]->GetRasterBand(int(b + 1));
//				//GDALRasterBand* pBand = grid[v]->GetRasterBand(1);
//				pBand->RasterIO(GF_Write, 0, 0, options.m_extents.m_xSize, options.m_extents.m_ySize, &(output[v][0]), options.m_extents.m_xSize, options.m_extents.m_ySize, GDT_Float32, 0, 0);
//			}
//		}
//
//
//	}
//
//	for (int v = 0; v < NB_FIELDS; v++)
//	{
//		if (grid[v].IsOpen())
//		{
//
//			callback.AddMessage("Close " + grid[v].GetFilePath() + " ...");
//			//grid[v].ComputeStats(true);
//			grid[v].Close();
//		}
//	}
//
//
//
//	return msg;
//}

}


//XLAT : description = "LATITUDE, SOUTH IS NEGATIVE";
//XLONG : description = "LONGITUDE, WEST IS NEGATIVE";
//LU_INDEX : description = "LAND USE CATEGORY";
//ZNU : description = "eta values on half (mass) levels";
//ZNW : description = "eta values on full (w) levels";
//ZS : description = "DEPTHS OF CENTERS OF SOIL LAYERS";
//DZS : description = "THICKNESSES OF SOIL LAYERS";
//VAR_SSO : description = "variance of subgrid-scale orography";
//LAP_HGT : description = "Laplacian of orography";
//U : description = "x-wind component";
//V : description = "y-wind component";
//W : description = "z-wind component";
//PH : description = "perturbation geopotential";
//PHB : description = "base-state geopotential";
//T : description = "perturbation potential temperature (theta-t0)";
//HFX_FORCE : description = "SCM ideal surface sensible heat flux";
//LH_FORCE : description = "SCM ideal surface latent heat flux";
//TSK_FORCE : description = "SCM ideal surface skin temperature";
//HFX_FORCE_TEND : description = "SCM ideal surface sensible heat flux tendency";
//LH_FORCE_TEND : description = "SCM ideal surface latent heat flux tendency";
//TSK_FORCE_TEND : description = "SCM ideal surface skin temperature tendency";
//MU : description = "perturbation dry air mass in column";
//MUB : description = "base state dry air mass in column";
//NEST_POS : description = "-";
//P : description = "perturbation pressure";
//PB : description = "BASE STATE PRESSURE";
//FNM : description = "upper weight for vertical stretching";
//FNP : description = "lower weight for vertical stretching";
//RDNW : description = "inverse d(eta) values between full (w) levels";
//RDN : description = "inverse d(eta) values between half (mass) levels";
//DNW : description = "d(eta) values between full (w) levels";
//DN : description = "d(eta) values between half (mass) levels";
//CFN : description = "extrapolation constant";
//CFN1 : description = "extrapolation constant";
//THIS_IS_AN_IDEAL_RUN : description = "T/F flag: this is an ARW ideal simulation";
//P_HYD : description = "hydrostatic pressure";
//Q2 : description = "QV at 2 M";
//T2 : description = "TEMP at 2 M";
//TH2 : description = "POT TEMP at 2 M";
//PSFC : description = "SFC PRESSURE";
//U10 : description = "U at 10 M";
//V10 : description = "V at 10 M";
//RDX : description = "INVERSE X GRID LENGTH";
//RDY : description = "INVERSE Y GRID LENGTH";
//RESM : description = "TIME WEIGHT CONSTANT FOR SMALL STEPS";
//ZETATOP : description = "ZETA AT MODEL TOP";
//CF1 : description = "2nd order extrapolation constant";
//CF2 : description = "2nd order extrapolation constant";
//CF3 : description = "2nd order extrapolation constant";
//ITIMESTEP : description = "";
//XTIME : description = "minutes since 2013-07-12 12:00:00";
//QVAPOR : description = "Water vapor mixing ratio";
//QCLOUD : description = "Cloud water mixing ratio";
//QRAIN : description = "Rain water mixing ratio";
//QICE : description = "Ice mixing ratio";
//QSNOW : description = "Snow mixing ratio";
//SHDMAX : description = "ANNUAL MAX VEG FRACTION";
//SHDMIN : description = "ANNUAL MIN VEG FRACTION";
//SNOALB : description = "ANNUAL MAX SNOW ALBEDO IN FRACTION";
//TSLB : description = "SOIL TEMPERATURE";
//SMOIS : description = "SOIL MOISTURE";
//SH2O : description = "SOIL LIQUID WATER";
//SMCREL : description = "RELATIVE SOIL MOISTURE";
//SEAICE : description = "SEA ICE FLAG";
//XICEM : description = "SEA ICE FLAG (PREVIOUS STEP)";
//SFROFF : description = "SURFACE RUNOFF";
//UDROFF : description = "UNDERGROUND RUNOFF";
//IVGTYP : description = "DOMINANT VEGETATION CATEGORY";
//ISLTYP : description = "DOMINANT SOIL CATEGORY";
//VEGFRA : description = "VEGETATION FRACTION";
//GRDFLX : description = "GROUND HEAT FLUX";
//ACGRDFLX : description = "ACCUMULATED GROUND HEAT FLUX";
//ACSNOM : description = "ACCUMULATED MELTED SNOW";
//SNOW : description = "SNOW WATER EQUIVALENT";
//SNOWH : description = "PHYSICAL SNOW DEPTH";
//CANWAT : description = "CANOPY WATER";
//SSTSK : description = "SKIN SEA SURFACE TEMPERATURE";
//COSZEN : description = "COS of SOLAR ZENITH ANGLE";
//LAI : description = "LEAF AREA INDEX";
//VAR : description = "OROGRAPHIC VARIANCE";
//TKE_PBL : description = "TKE from PBL";
//EL_PBL : description = "Length scale from PBL";
//MAPFAC_M : description = "Map scale factor on mass grid";
//MAPFAC_U : description = "Map scale factor on u-grid";
//MAPFAC_V : description = "Map scale factor on v-grid";
//MAPFAC_MX : description = "Map scale factor on mass grid, x direction";
//MAPFAC_MY : description = "Map scale factor on mass grid, y direction";
//MAPFAC_UX : description = "Map scale factor on u-grid, x direction";
//MAPFAC_UY : description = "Map scale factor on u-grid, y direction";
//MAPFAC_VX : description = "Map scale factor on v-grid, x direction";
//MF_VX_INV : description = "Inverse map scale factor on v-grid, x direction";
//MAPFAC_VY : description = "Map scale factor on v-grid, y direction";
//F : description = "Coriolis sine latitude term";
//E : description = "Coriolis cosine latitude term";
//SINALPHA : description = "Local sine of map rotation";
//COSALPHA : description = "Local cosine of map rotation";
//HGT : description = "Terrain Height";
//TSK : description = "SURFACE SKIN TEMPERATURE";
//P_TOP : description = "PRESSURE TOP OF THE MODEL";
//T00 : description = "BASE STATE TEMPERATURE";
//P00 : description = "BASE STATE PRESURE";
//TLP : description = "BASE STATE LAPSE RATE";
//TISO : description = "TEMP AT WHICH THE BASE T TURNS CONST";
//TLP_STRAT : description = "BASE STATE LAPSE RATE (DT/D(LN(P)) IN STRATOSPHERE";
//P_STRAT : description = "BASE STATE PRESSURE AT BOTTOM OF STRATOSPHERE";
//MAX_MSTFX : description = "Max map factor in domain";
//MAX_MSTFY : description = "Max map factor in domain";
//RAINC : description = "ACCUMULATED TOTAL CUMULUS PRECIPITATION";
//RAINSH : description = "ACCUMULATED SHALLOW CUMULUS PRECIPITATION";
//RAINNC : description = "ACCUMULATED TOTAL GRID SCALE PRECIPITATION";
//SNOWNC : description = "ACCUMULATED TOTAL GRID SCALE SNOW AND ICE";
//GRAUPELNC : description = "ACCUMULATED TOTAL GRID SCALE GRAUPEL";
//HAILNC : description = "ACCUMULATED TOTAL GRID SCALE HAIL";
//CLDFRA : description = "CLOUD FRACTION";
//SWDOWN : description = "DOWNWARD SHORT WAVE FLUX AT GROUND SURFACE";
//GLW : description = "DOWNWARD LONG WAVE FLUX AT GROUND SURFACE";
//SWNORM : description = "NORMAL SHORT WAVE FLUX AT GROUND SURFACE (SLOPE-DEPENDENT)";
//DIFFUSE_FRAC : description = "DIFFUSE FRACTION OF SURFACE SHORTWAVE IRRADIANCE";
//OLR : description = "TOA OUTGOING LONG WAVE";
//XLAT_U : description = "LATITUDE, SOUTH IS NEGATIVE";
//XLONG_U : description = "LONGITUDE, WEST IS NEGATIVE";
//XLAT_V : description = "LATITUDE, SOUTH IS NEGATIVE";
//XLONG_V : description = "LONGITUDE, WEST IS NEGATIVE";
//ALBEDO : description = "ALBEDO";
//CLAT : description = "COMPUTATIONAL GRID LATITUDE, SOUTH IS NEGATIVE";
//ALBBCK : description = "BACKGROUND ALBEDO";
//EMISS : description = "SURFACE EMISSIVITY";
//NOAHRES : description = "RESIDUAL OF THE NOAH SURFACE ENERGY BUDGET";
//TMN : description = "SOIL TEMPERATURE AT LOWER BOUNDARY";
//XLAND : description = "LAND MASK (1 FOR LAND, 2 FOR WATER)";
//UST : description = "U* IN SIMILARITY THEORY";
//PBLH : description = "PBL HEIGHT";
//HFX : description = "UPWARD HEAT FLUX AT THE SURFACE";
//QFX : description = "UPWARD MOISTURE FLUX AT THE SURFACE";
//LH : description = "LATENT HEAT FLUX AT THE SURFACE";
//ACHFX : description = "ACCUMULATED UPWARD HEAT FLUX AT THE SURFACE";
//ACLHF : description = "ACCUMULATED UPWARD LATENT HEAT FLUX AT THE SURFACE";
//SNOWC : description = "FLAG INDICATING SNOW COVERAGE (1 FOR SNOW COVER)";
//SR : description = "fraction of frozen precipitation";
//SAVE_TOPO_FROM_REAL : description = "1=original topo from real/0=topo modified by WRF";
//ISEEDARR_RAND_PERTURB : description = "Array to hold seed for restart, RAND_PERT";
//ISEEDARR_SPPT : description = "Array to hold seed for restart, SPPT";
//ISEEDARR_SKEBS : description = "Array to hold seed for restart, SKEBS";
//LANDMASK : description = "LAND MASK (1 FOR LAND, 0 FOR WATER)";
//LAKEMASK : description = "LAKE MASK (1 FOR LAKE, 0 FOR NON-LAKE)";
//SST : description = "SEA SURFACE TEMPERATURE";
//SST_INPUT : description = "SEA SURFACE TEMPERATURE FROM WRFLOWINPUT FILE";

