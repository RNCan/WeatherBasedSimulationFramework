//******************************************************************************
//  project:		Weather-based simulation framework (WBSF)
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
#include <float.h>
#include <limits>

#include "Basic/UtilMath.h"
#include "Basic/psychrometrics_SI.h"

using namespace std;


namespace WBSF
{

	double InvLogistic(double y, double k1, double k2)
	{
		// Eq. [3] in Regniere, J. 1984. Canadian Entomologist 116: 1367-1376
		// inverted y = f(x)
		double x = 1.0 - log((pow(y, -k2) - 1) / (pow(0.5, -k2) - 1)) / k1;
		return x;
	}

	double InvWeibull(double y, double alpha, double beta, double gamma)
	{
		//inverted y = 1 - e( - ((x-gamma)/beta)^alpha)
		//Note that Gray et al's (2001) equ. [9] is in error, as alpha is exponent of negative sign as well...
		//Gray et al (1991) had their equ. right.
		double x = gamma + beta * pow((-log(1 - y)), (1 / alpha)); //Inverted Weibull function
		return x;
	}

	double Weibull(double t, double tb, double tm,
		double b1, double b2, double b3, double b4)
	{
		double w;
		double tau;
		double x, y;
		x = (t - tb);
		y = (tm - tb);


		_ASSERTE(y != 0);
		_ASSERTE(b4 != 0);
		_ASSERTE(/*(x!=0)&&*/(y != 0));


		tau = x / y;


		if ((tau >= .01) && (tau <= .99)) {
			w = (double)(b1*(1 / (1 + exp(b2 - tau * b3)) - exp((tau - 1) / b4)));
		}
		else w = 0;

		return w;
	}

	double Linearize(double x2, double x1, double y1, double x3, double y3)
	{

		_ASSERTE(x1 != x3); // x1 = x3 => not a function


		double m, b, y2 = 0;
		m = (y3 - y1) / (x3 - x1);
		b = y1 - m * x1;
		y2 = m * x2 + b;
		return y2;
	}



	double zScore(void)
	{
		//Polar method for producing two normal deviates via polar method by D. Knuth,
		//	volume 3, p. 117
		double u1, u2, v1, v2, s, x, a;

		do {
			u1 = Randu();
			u2 = Randu();
			v1 = 2 * u1 - 1;
			v2 = 2 * u2 - 1;
			s = (double)(pow(v1, 2) + pow(v2, 2));
		} while (s >= 1);

		_ASSERTE(s > 0);

		a = (double)sqrt(-2 * log(s) / s);
		x = v1 * a;
		return x;
	}



	double Polynom(double x, int degree, const double *coeffs)
	{
		double sum = 0.0;
		while (degree >= 0) {
			sum += (double)(coeffs[degree] * pow(x, degree));
			degree--;
		}
		return sum;
	}


	double Schoolfield(const double p[NB_SCHOOLFIELD_PARAMETERS], double T)
	{


		const double R = 1.987;
		double Tk = T + 273.15;
		double a1 = p[SF_PRHO25] * Tk / 298.15;
		double a2 = exp(p[SF_PHA] / R * (1 / 298.15 - 1 / Tk));
		double b1 = exp(p[SF_PHL] / R * (1 / p[SF_PTL] - 1 / Tk));
		double b2 = exp(p[SF_PHH] / R * (1 / p[SF_PTH] - 1 / Tk));
		double r = (a1*a2) / (1 + b1 + b2);

		ASSERT(r >= 0);
		return r;
	}

	// Visit http://www.johndcook.com/stand_alone_code.html for the source of this code and more like it.

	 // We require x > 0
	double Gamma(double x)
	{
		ASSERT(x > 0);

		// Split the function domain into three intervals:
		// (0, 0.001), [0.001, 12), and (12, infinity)

		///////////////////////////////////////////////////////////////////////////
		// First interval: (0, 0.001)
		//
		// For small x, 1/Gamma(x) has power series x + gamma x^2  - ...
		// So in this range, 1/Gamma(x) = x + gamma x^2 with error on the order of x^3.
		// The relative error over this interval is less than 6e-7.

		const double gamma = 0.577215664901532860606512090; // Euler's gamma constant

		if (x < 0.001)
			return 1.0 / (x*(1.0 + gamma * x));

		///////////////////////////////////////////////////////////////////////////
		// Second interval: [0.001, 12)

		if (x < 12.0)
		{
			// The algorithm directly approximates gamma over (1,2) and uses
			// reduction identities to reduce other arguments to this interval.

			double y = x;
			int n = 0;
			bool arg_was_less_than_one = (y < 1.0);

			// Add or subtract integers as necessary to bring y into (1,2)
			// Will correct for this below
			if (arg_was_less_than_one)
			{
				y += 1.0;
			}
			else
			{
				n = static_cast<int> (floor(y)) - 1;  // will use n later
				y -= n;
			}

			// numerator coefficients for approximation over the interval (1,2)
			static const double p[] =
			{
				-1.71618513886549492533811E+0,
				 2.47656508055759199108314E+1,
				-3.79804256470945635097577E+2,
				 6.29331155312818442661052E+2,
				 8.66966202790413211295064E+2,
				-3.14512729688483675254357E+4,
				-3.61444134186911729807069E+4,
				 6.64561438202405440627855E+4
			};

			// denominator coefficients for approximation over the interval (1,2)
			static const double q[] =
			{
				-3.08402300119738975254353E+1,
				 3.15350626979604161529144E+2,
				-1.01515636749021914166146E+3,
				-3.10777167157231109440444E+3,
				 2.25381184209801510330112E+4,
				 4.75584627752788110767815E+3,
				-1.34659959864969306392456E+5,
				-1.15132259675553483497211E+5
			};

			double num = 0.0;
			double den = 1.0;
			int i;

			double z = y - 1;
			for (i = 0; i < 8; i++)
			{
				num = (num + p[i])*z;
				den = den * z + q[i];
			}
			double result = num / den + 1.0;

			// Apply correction if argument was not initially in (1,2)
			if (arg_was_less_than_one)
			{
				// Use identity gamma(z) = gamma(z+1)/z
				// The variable "result" now holds gamma of the original y + 1
				// Thus we use y-1 to get back the orginal y.
				result /= (y - 1.0);
			}
			else
			{
				// Use the identity gamma(z+n) = z*(z+1)* ... *(z+n-1)*gamma(z)
				for (i = 0; i < n; i++)
					result *= y++;
			}

			return result;
		}

		///////////////////////////////////////////////////////////////////////////
		// Third interval: [12, infinity)

		if (x > 171.624)
		{
			// Correct answer too large to display. Force +infinity.
			//double temp = DBL_MAX;
			return std::numeric_limits<double>::infinity();
		}

		return exp(LogGamma(x));
	}

	// x must be positive
	double LogGamma(double x)
	{
		ASSERT(x > 0);

		if (x < 12.0)
		{
			return log(fabs(Gamma(x)));
		}

		// Abramowitz and Stegun 6.1.41
		// Asymptotic series should be good to at least 11 or 12 figures
		// For error analysis, see Whittiker and Watson
		// A Course in Modern Analysis (1927), page 252

		static const double c[8] =
		{
			 1.0 / 12.0,
			-1.0 / 360.0,
			1.0 / 1260.0,
			-1.0 / 1680.0,
			1.0 / 1188.0,
			-691.0 / 360360.0,
			1.0 / 156.0,
			-3617.0 / 122400.0
		};
		double z = 1.0 / (x*x);
		double sum = c[7];
		for (int i = 6; i >= 0; i--)
		{
			sum *= z;
			sum += c[i];
		}
		double series = sum / x;

		static const double halfLogTwoPi = 0.91893853320467274178032973640562;
		double logGamma = (x - 0.5)*log(x) - x + halfLogTwoPi + series;
		return logGamma;
	}



	double StringToCoord(const std::string& coordStr)
	{
		double coord = 0;
		double a = 0, b = 0, c = 0;
		int nbVal = sscanf(coordStr.c_str(), "%lf %lf %lf", &a, &b, &c);
		if (nbVal == 1)
		{
			//decimal degree 
			coord = a;
		}
		else if (nbVal >= 2 && nbVal <= 3)
		{
			//degree minute
			if (a >= -360 && a <= 360 &&
				b >= 0 && b <= 60 &&
				c >= 0 && c <= 60)
				coord = GetDecimalDegree((long)a, (long)b, c);
		}

		return coord;
	}

	std::string CoordToString(double coord, int prec)
	{
		double mult = pow(10.0, prec);
		std::string deg = ToString(GetDegrees(coord, mult));
		std::string min = ToString(GetMinutes(coord, mult));
		std::string sec = ToString(GetSeconds(coord, mult), prec);

		if (sec == "0" || sec == "-0")
			sec.clear();
		if (sec.empty() && (min == "0" || min == "-0"))
			min.clear();

		std::string str = deg + " " + min + " " + sec;
		Trim(str);

		return str;
	}



	//**************************************************************************************************************************
	//Humidity function

	//latent heat of vaporisation (J/kg)
	//Tair: dry bulb air temperature [°C]
	//L:	J kg-1 at T = 273.15 K
	double GetL(double Tair)
	{
		double T = Tair + 273.15;
		double a = (2.501E6 - 2.257E6) / (273.15 - 373.15);
		double b = 2.501E6 - a * 273.15;
		double L = a * T + b;
		return L; //J kg-1
	}



	void GetWindUV(double windSpeed, double windDir, double& U, double &V, bool from)
	{
		ASSERT(windSpeed >= 0 && windSpeed < 150);
		ASSERT(windDir >= 0 && windDir <= 360);

		double d = Deg2Rad(90 - windDir);
		double x = cos(d)*windSpeed;
		double y = sin(d)*windSpeed;
		ASSERT(fabs(sqrt(Square(x) + Square(y)) - windSpeed) < 0.0001);

		U = from ? -x : x;
		V = from ? -y : y;

		if (fabs(U) < 0.1)
			U = 0;
		if (fabs(V) < 0.1)
			V = 0;
	}


	double GetWindDirection(double u, double v, bool from)
	{
		double d = (180 / PI)*(atan2(u, v));
		ASSERT(d >= -180 && d <= 180);

		if (from)
			d = int(d + 360 + 180) % 360;
		else
			d = int(d + 360) % 360;

		return d;
	}


	//
	//Excel = 0.6108 * exp(17.27*T / (T + 237.3))
	//hourly vapor pressure
	//T : temperature [°C]
	//e°: saturation vapor pressure function [kPa]
	double eᵒ(double T)
	{
		return 0.6108 * exp(17.27*T / (T + 237.3));//Same as CASCE_ETsz
	}

	//daily vapor pressure
	double eᵒ(double Tmin, double Tmax)
	{
		return (eᵒ(Tmax) + eᵒ(Tmin)) / 2;
	}

	//Relative humidity [%] to dew point [°C]
	//Excel formula: =Min( T, (1/(1/(T+273.15) - (461*ln(max(0.0,Min(100,RH))/100))/2.5E6))-273.15)

	//Tair:	temperature of dry bulb	[°C]
	//Hr:	relative humidity		[%]
	double Hr2Td(double Tair, double Hr)
	{
		_ASSERTE(Tair > -999);
		_ASSERTE(Hr >= 0 && Hr <= 101);
		Hr = max(1.0, Hr);//limit to avoid division by zero

		double L = GetL(Tair);	//(J/kg)

		//static const double L = 2.453E6;
		static const double Rv = 461; //constant for moist air (J/kg)

		double T = Tair + 273.15;
		double Td = 1 / (1 / T - (Rv*log(Hr / 100)) / L);


		_ASSERTE(Td - 273.15 > -99 && Td - 273.15 < 99);
		return min(Tair, Td - 273.15);
	}

	//Dew point teprature [°C] to relative humidity [%]
	//Excel formula:   =max(0.0,  Min(100, exp( 2.5E6/461*(1/(T+273.15) - 1/Min(T+273.15, Td+273.15)))*100))

	//Tair:	Temperature of dry bulb	[°C]
	//Tdew:	Dew point temperature	[°C]
	double Td2Hr(double Tair, double Tdew)
	{
		double L = GetL(Tair);			//J/kg
		static const double Rv = 461;	//J/kg

		double T = Tair + 273.15;
		double Td = min(T, Tdew + 273.15);
		double Hr = exp(L / Rv * (1 / T - 1 / Td));

		_ASSERTE(Hr >= 0 && Hr <= 1);

		return Hr * 100;
	}

	static const double Ra = 287.058; // Specific gas constant for dry air J/(kg·K)
	static const double Rw = 461.495; // Specific gas constant for water vapor J/(kg·K)
	static const double Ma = 0.028964;//Molar mass of dry air,  kg/mol
	static const double Mw = 0.018016;//Molar mass of water vapor,  kg/mol
	static const double R = 8.314;	  //Universal gas constant J/(K·mol)
	static const double M = Mw / Ma;  // 0.6220135

	//Td : dew point temperature [°C]
	//Pv : vapor pressure [kPa]
	double Td2Pv(double Td)
	{
		//return Pv(Td) * 1000;
		//from : http://www.conservationphysics.org/atmcalc/atmoclc2.pdf
		return 0.61078 * exp(17.2694*Td / (Td + 238.3));
	}

	//Pv : vapor pressure [kPa]
	//Td : dew point temperature [°C]
	double Pv2Td(double Pv)
	{
		//from : http://www.conservationphysics.org/atmcalc/atmoclc2.pdf
		return (241.88 * log(Pv / 0.61078)) / (17.558 - log(Pv / 0.61078));
	}

	//mixing ratio (  kg[H2O]/kg[dry air] ) to specific humidity ( g(H²O)/kg(air) )
	//MR:  g[H2O]/kg[dry air] 
	//Hs:  g(H²O)/kg(air) 
	double MR2Hs(double MR)
	{
		return 1000 * (MR / (1000 * (1 + MR / 1000)));//Mixing ratio to Specific humidity [ g(H²O)/kg(air) ]
	}

	//specific humidity ( g(H²O)/kg(air) ) to mixing ratio (  g[H2O]/kg[dry air] )
	//Hs:  specific humidity		g(H²O)/kg(air) 
	//MR:  mixing ratio				g[H2O]/kg[dry air] 
	double Hs2MR(double Hs)
	{
		return 1000 * (Hs / (1000 * (1 - Hs / 1000)));	//specific humidity g(H²O)/kg(air) to mixing ratio g[H2O]/kg[dry air]
	}

	//Relative humidity [%] to specific humidity [ g(H²O)/kg(air) ]
	//From wikipedia : http://en.wikipedia.org/wiki/Humidity
	//validation was made with RH WMO without correction of the site : http://www.humidity-calculator.com/index.php
	//and also http://www.cactus2000.de/uk/unit/masshum.shtml

	//Tair:		Dry bulb temperature				[°C]
	//Hr:		Relative humidity WMO (over water)	[%]
	//altitude:	altitude							[m]
	double Hr2Hs(double Tair, double Hr)
	{
		_ASSERTE(Hr >= 0 && Hr <= 100);

		double Pv = Hr2Pv(Tair, Hr);
		return Pv2Hs(Pv);


		//double Psat = GetPsat(Tair);//Vapor pressure of water [Pa]
		//double Pv = Psat*Hr/100; //Vapor pressure [Pa]
		//double MR = M*Pv/(P-Pv);//Mixing ratio [ kg[H2O]/kg[dry air] ]
		//double Hs = MR/(1+MR);//Mixing ratio to Specific humidity [ kg(H²O)/kg(air) ]

		//return Hs*1000; //convert to [ g(H²O)/kg(air) ]
	}

	//hourly specific humidity [ g(H²O)/kg(air) ] to relative humidity [%]
	//Tair:		air temperature			°C
	//Hs:		specific humidity		g(H²O)/kg(air) 
	double Hs2Hr(double Tair, double Hs)
	{
		_ASSERTE(Hs > 0);

		double Pv = Hs2Pv(Hs);
		return Pv2Hr(Tair, Pv);
	}

	//daily specific humidity [ g(H²O)/kg(air) ] to relative humidity [%]
	//Tmin:		Daily minimum air temperature			°C
	//Tmax:		Daily maximum air temperature			°C
	//Hs:		Specific humidity						g(H²O)/kg(air) 
	double Hs2Hr(double Tmin, double Tmax, double Hs)
	{
		_ASSERTE(Hs > 0);

		double Pv = Hs2Pv(Hs);
		return Pv2Hr(Tmin, Tmax, Pv);
	}

	//Daily relative humidity [%] to specific humidity [ g(H²O)/kg(air) ]
	//Tmin:		Daily minimum air temperature			°C
	//Tmax:		Daily maximum air temperature			°C
	//Hr:		Relative humidity (over water)			[%]
	double Hr2Hs(double Tmin, double Tmax, double Hr)
	{
		_ASSERTE(Hr >= 0 && Hr <= 100);

		double Pv = Hr2Pv(Tmin, Tmax, Hr);
		return Pv2Hs(Pv);
	}

	//Pv : vapor pressure [Pa]
	double Pv2Hr(double Tair, double Pv)
	{
		double Psat = eᵒ(Tair) * 1000;//kPa --> Pa
		double Hr = 100 * Pv / Psat;

		return max(0.0, min(100.0, Hr));
	}

	//Tair: air temperature [°C]
	//Hr: relativce humidity [%]
	//Pv : vapor pressure [Pa]
	double Hr2Pv(double Tair, double Hr)
	{
		ASSERT(Hr >= 0 && Hr <= 100);

		double Psat = eᵒ(Tair) * 1000;//kPa --> Pa
		double Pv = Psat * Hr / 100.0;

		return Pv;
	}


	double Pv2Hr(double Tmin, double Tmax, double Pv)
	{
		double Psat = eᵒ(Tmin, Tmax) * 1000;//kPa --> Pa
		double Hr = 100 * Pv / Psat;

		return max(0.0, min(100.0, Hr));
	}

	//Tmin:		Daily minimum air temperature [°C]
	//Tmax:		Daily maximum air temperature [°C]
	//Hr:		Relative humidity [%]
	double Hr2Pv(double Tmin, double Tmax, double Hr)
	{
		ASSERT(Hr >= 0 && Hr <= 100);

		double Psat = eᵒ(Tmin, Tmax) * 1000;//kPa --> Pa
		double Pv = Hr * Psat / 100;


		return Pv;
	}


	//Pv2Hs:	vapor pressure [Pa] to Specific Humidity (g(H²O)/kg(air))
	//Pv:		partial pressure of water vapor in moist air	[Pa]
	double Pv2Hs(double Pv)
	{
		double P = 101325;//[Pa]

		double Hs = (M*Pv) / (P - Pv * (1 - M));
		ASSERT(Hs >= 0);

		return Hs * 1000;//Convert to g(H²O)/kg(air)
	}

	//Pv2Hs:	vapor pressure ([Pa]) to Specific Humidity ( g(H²O)/kg(air) )
	//Pv:		partial pressure of water vapor in moist air	[Pa]
	//Excel formala: = (101325 * (Hs/1000)) / (0.6220135 + (Hs /1000 )* (1 - 0.6220135))
	double Hs2Pv(double Hs)
	{
		double P = 101325;//Pa

		Hs /= 1000;//Convert to kg(H²O)/kg(air)
		double Pv = (P*Hs) / (M + Hs * (1 - M));


		return Pv;
	}





	//Tair: dry bulb air temperature [°C]
	//Hs: specific humidity ( g(H²O)/kg(air) )
	//Out: Dew point temperature [°C]
	double Hs2Td(double Tair, double Hs)
	{

		double Hr = Hs2Hr(Tair, Hs);
		return Hr2Td(Tair, Hr);
	}

	//Tair: dry bulb air temperature [°C]
	//Tdew: dewpoint temperature [°C]
	double Td2Hs(double Tair, double Tdew)
	{
		double Hr = Td2Hr(Tair, Tdew);
		return Hr2Hs(Tair, Hr);
	}


	//Wet-Bulb Temperature from Relative Humidity and Air Temperature
	//Roland Stull
	//University of British Columbia, Vancouver, British Columbia, Canada
	//T: dry air temperature [°C]
	//Hr: relativce humidity [%]
	//Twb: wet bulb temperature [°C]
	double Hr2Twb(double T, double RH)
	{
		ASSERT(RH >= 0 && RH <= 100);

		double Twb = T * atan(0.151977*sqrt(RH + 18.313659)) + atan(T + RH) - atan(RH - 1.676331) + 0.00391838*pow(RH, 3.0 / 2)*atan(0.023101*RH) - 4.686035;
		return Twb;
	}

	//Relative humidity from temperature and wet bulb temperature
	//Tair: dry bulb temperature of air [°C]
	//Twet : wet bulb temperature [°C]
	double Twb2Hr(double Tair, double Twet)
	{
		static const double Cp = 1.005;	//specific heat of dry air at constant pressure(J/g) ~1.005 J/g
		static const double Cpv = 4.186;	//specific heat of water vapor at constant pressure(J/g) ~4.186 J/g
		static const double P = 1013;		//atmospheric pressure at surface ~1013 mb at sea-level

		//Latent heat of vaporization(J/g) ~2500 J/g
		double Lv = GetL(Tair) / 1000;
		//saturation vapor pressure at the wet bulb temperature(hPa)
		double Eswb = 6.11*pow(10.0, 7.5*Twet / (237.7 + Twet));

		//If you know the air temperature and the wet bulb temperature, you first want to calculate the actual mixing ratio of the air(W) using the following formula.
		//W=actual mixing ratio of air
		//(12) W=[(Tair-Twb)(Cp)-Lv(Eswb/P)]/[-(Tc-Twb)(Cpv)-Lv]
		double W = ((Tair - Twet)*Cp - Lv * (Eswb / P)) / (-(Tair - Twet)*Cpv - Lv);

		//Once you have the actual vapor pressure, you can use the following formula to calculate the saturation mixing ratio for the air.
		//(13) Ws=Es/P
		double Es = 6.11*pow(10.0, 7.5*Tair / (237.7 + Tair));
		double Ws = Es / P;

		//Once you have the actual mixing ratio and the saturation mixing ratio, you can use the following formula to calculate relative humidity.
		//(14) Relative Humidity(RH) in percent=(W/Ws)*100
		double RH = (W / Ws) * 100;

		//Note: The latent heat of vaporization(Lv) varies slightly with temperature. The value given above is an approximate value for the standard atmosphere at 0 degrees Celsius.
		//Note: Due to the large numbers of approximations using these formulas, your final answer may vary by as much as 10 percent.

		return RH;
	}

	//alt : altitude [m]
	//P : normal atmoshpheric pressure [Pa]
	double GetPressure(double alt)
	{
		// daily atmospheric pressure (Pa) as a function of elevation (m) 
		// From the discussion on atmospheric statics in:
		// Iribane, J.V., and W.L. Godson, 1981. Atmospheric Thermodynamics, 2nd
		// Edition. D. Reidel Publishing Company, Dordrecht, The Netherlands. (p. 168)

		static const double MA = 28.9644e-3;     // (kg mol-1) molecular weight of air 
		static const double R = 8.3143;          // (m3 Pa mol-1 K-1) gas law constant 
		static const double LR_STD = 0.0065;     // (-K m-1) standard temperature lapse rate 
		static const double G_STD = 9.80665;     // (m s-2) standard gravitational accel.  
		static const double P_STD = 101325.0;    // (Pa) standard pressure at 0.0 m elevation 
		static const double T_STD = 288.15;      // (K) standard temp at 0.0 m elevation   

		double t1 = 1.0 - (LR_STD * alt) / T_STD;
		double t2 = G_STD / (LR_STD * (R / MA));
		double P = P_STD * pow(t1, t2);
		return P;
	}

	//P : normal atmoshpheric pressure [Pa]
	//alt : altitude [m]
	double GetAltitude(double P)
	{
		// daily atmospheric pressure (Pa) as a function of elevation (m) 
		// From the discussion on atmospheric statics in:
		// Iribane, J.V., and W.L. Godson, 1981. Atmospheric Thermodynamics, 2nd
		// Edition. D. Reidel Publishing Company, Dordrecht, The Netherlands. (p. 168)

		static const double MA = 28.9644e-3;     // (kg mol-1) molecular weight of air 
		static const double R = 8.3143;          // (m3 Pa mol-1 K-1) gas law constant 
		static const double LR_STD = 0.0065;     // (-K m-1) standard temperature lapse rate 
		static const double G_STD = 9.80665;     // (m s-2) standard gravitational accel.  
		static const double P_STD = 101325.0;    // (Pa) standard pressure at 0.0 m elevation 
		static const double T_STD = 288.15;      // (K) standard temp at 0.0 m elevation   

		double t2 = G_STD / (LR_STD * (R / MA));
		double t1 = pow(P / P_STD, 1 / t2);
		double alt = (1.0 - t1)*T_STD / LR_STD;


		return alt;
	}



	double GetPolarInterpol(double T0, double T1, double T2, double h)
	{
		double Tair = 0;

		if (h < 12)
		{
			Tair = T0 + h * (T1 - T0) / 11.0;
		}
		else
		{
			Tair = T1 + (h - 12)*(T2 - T1) / 11.0;
		}

		return Round(Tair, 1);
	}



	double GetPolarSummer(double Tmin[3], double Tmax[3], double h)
	{
		return WBSF::GetDoubleSine(Tmin, Tmax, h, 05, 16);
	}

	double GetPolarWinter(double Tmin[3], double Tmax[3], double h)
	{
		double Tair[3] = { (Tmin[0] + Tmax[0]) / 2, (Tmin[1] + Tmax[1]) / 2, (Tmin[2] + Tmax[2]) / 2 };
		double T[3] =
		{
			(Tair[0] < Tair[1]) ? Tmin[1] : Tmax[1],
			(Tair[0] < Tair[1]) ? Tmax[1] : Tmin[1],
			(Tair[1] < Tair[2]) ? Tmin[2] : Tmax[2],
		};

		return GetPolarInterpol(T[0], T[1], T[2], h);
	}


	// original from Parton (1981), corrected by Brandsma (2006)
	//from : Application of nearest-neighbor resampling for homogenizing temperature records on a daily to sub - daily level
	//Brandsma (2006)
	double GetSineExponential(double Tmin[3], double Tmax[3], double t, double Tsr, double Tss, size_t method)
	{
		ASSERT(method < NB_SINE_EXP);

		double Tair = 0;
		double D = Tss - Tsr;

		if (D > 22)
		{
			//because of the correction, SineExponential can be use when day is 24 hour
			//we used Ebrs instead
			Tair = WBSF::GetSinePower(Tmin, Tmax, t, Tsr, Tss);
		}
		else
		{

			//Tsr = ceil(Tsr);
			//Tss = floor(Tss);
			//Tsr = floor(Tsr);
			//Tss = ceil(Tss);
			//Tsr = Round(Tsr);
			//Tss = Round(Tss);
			//D = Tss - Tsr;
			static const double ALPHA[NB_SINE_EXP] = { 2.59, 1.86 };
			static const double BETA[NB_SINE_EXP] = { 1.55, 0.00 };
			static const double GAMMA[NB_SINE_EXP] = { 2.20, 2.20 };

			double alpha = ALPHA[method];
			double beta = BETA[method];
			double gamma = GAMMA[method];

			//double Tn = Tsr;// +beta;


			if (t < Tsr)
			{
				//compute temperature at nightime before midnight
				//double fs = D / (D + 2 * a);
				double fs = (Tss - Tsr - beta) / (D + 2 * (alpha - beta));
				assert(sin(PI*fs) >= 0);
				double Tsun = float(Tmin[0] + (Tmax[0] - Tmin[0])*sin(PI*fs));

				double f = min(0.0, -gamma * (t + 24 - Tss) / (24 - D + beta));
				double fo = min(0.0, -gamma * (Tsr + 24 - Tss) / (24 - D + beta));
				//double f = min(0.0, -b*(t + 24 - Tss) / (24 - D));
				//double f° = min(0.0, -b*(Tsr + 24 - Tss) / (24 - D));
				double rho = (Tmin[1] - Tsun * exp(fo)) / (1 - exp(fo));
				Tair = rho + (Tsun - rho)*exp(f);

				ASSERT(Tair >= rho || Tair >= Tsun);
				ASSERT(f <= 0);
				ASSERT(f <= 0);
			}
			else if (t <= Tss)
			{
				//compute daylight (equation 3a)
				double f = max(0.0, (t - Tsr - beta) / (D + 2 * (alpha - beta)));
				//double f = (t - Tsr) / (D + 2 * a);
				assert(sin(PI*f) >= 0);

				Tair = Tmin[1] + (Tmax[1] - Tmin[1])*sin(PI*f);
				ASSERT(Tair >= Tmin[1]);
			}
			else //compute nightime (equation 1)
			{

				double fs = (Tss - Tsr - beta) / (D + 2 * (alpha - beta));
				//double fs = D / (D + 2 * a);
				assert(sin(PI*fs) >= 0);

				double Tsun = Tmin[1] + (Tmax[1] - Tmin[1])*sin(PI*fs);

				double f = min(0.0, -gamma * (t - Tss) / (24 - D + beta));
				double fo = min(0.0, -gamma * (Tsr + 24 - Tss) / (24 - D + beta));
				//double f = min(0.0, -b*(t - Tss) / (24 - D));
				//double f° = min(0.0, -b*(Tsr + 24 - Tss) / (24 - D));
				ASSERT(f <= 0 && fo <= 0);

				double rho = (Tmin[2] - Tsun * exp(fo)) / (1 - exp(fo));
				Tair = rho + (Tsun - rho)*exp(f);

				ASSERT(Tair >= rho || Tair >= Tsun);
			}
		}

		return Round(Tair, 1);
	}

	double GetErbs(double Tmin[3], double Tmax[3], double t)
	{
		size_t i = t < 14 ? 1 : 2;
		size_t ii = t < 05 ? 0 : 1;
		double Tmin² = Tmin[i];
		double Tmax² = Tmax[ii];
		double mean = (Tmin² + Tmax²) / 2;
		double range = Tmax² - Tmin²;

		double a = 2 * PI*t / 24;
		double c = 0.4632*cos(a - 3.805) + 0.0984*cos(2 * a - 0.36) + 0.0168*cos(3 * a - 0.822) + 0.0138*cos(4 * a - 3.513);
		double Tair = mean + range * c;


		return Round(Tair, 1);
	}

	double GetSinePower(double Tmin[3], double Tmax[3], double t, double Tsr, double Tss)
	{
		double Tair = 0;

		double D = Tss - Tsr;

		//Tsr = Round(Tsr);
		//Tss = Round(Tss);

		//Tsr = ceil(Tsr);
		//Tss = floor(Tss);
		//D = Tss - Tsr;

		static const double a = 1.86;//savage2015
		static const double b = 1.0 / 2.0;

		//compute nightime (modified equation 1)
		if (t < Tsr || t > Tss)
		{
			size_t i = t < Tsr ? 0 : 1;
			size_t ii = t < Tsr ? 1 : 2;

			double fs = D / (D + 2 * a);
			assert(sin(PI*fs) >= 0);
			double Tsun = float(Tmin[i] + (Tmax[i] - Tmin[i])*sin(PI*fs));

			if (t < Tsr)
				t += 24;

			double f = max(0.0, (t - Tss) / (24 - D));
			Tair = Tsun + (Tmin[ii] - Tsun)*pow(f, b);
			ASSERT(Tair >= Tmin[ii] || Tair >= Tsun);
		}
		else
		{
			double f = (t - Tsr) / (D + 2 * a);
			assert(sin(PI*f) >= 0);

			Tair = Tmin[1] + (Tmax[1] - Tmin[1])*sin(PI*f);
			ASSERT(Tair >= Tmin[1]);
		}
		//else //compute nightime (equation 1)
		//{
		//	double fs = D / (D + 2 * a);
		//	assert(sin(PI*fs) >= 0);
		//	double Tsun = Tmin[1] + (Tmax[1] - Tmin[1])*sin(PI*fs);

		//	double f = max(0.0, (t - Tss) / (24 - D));
		//	Tair = Tsun + (Tmin[2] - Tsun)*pow(f, 1.0 / 2);
		//	ASSERT(Tair >= Tmin[2] || Tair >= Tsun);
		//}
		//}

		return Round(Tair, 1);
	}
	//compute AllenWave temperature for one hour
	double GetDoubleSine(double Tmin[3], double Tmax[3], double h, double hourTmin, double hourTmax)
	{
		_ASSERTE(hourTmin < hourTmax);

		//force to arrive on extrem
		if (fabs(h - hourTmin) <= 0.5)
			h = hourTmin;

		if (fabs(h - hourTmax) <= 0.5)
			h = hourTmax;

		size_t i = h < hourTmin ? 0 : 1;
		size_t ii = h <= hourTmax ? 1 : 2;

		double Tmax² = Tmax[i];
		double Tmin² = Tmin[ii];


		double mean = (Tmin² + Tmax²) / 2;
		double range = Tmax² - Tmin²;
		//double theta = ((int)h - time_factor)*r_hour;
		double theta = 0;
		if (h >= hourTmin && h <= hourTmax)
		{
			//from Tmin to Tmax
			//size_t hh = (24 + h - hourTmin) % 24;
			double  hh = h - hourTmin;
			double nbh = hourTmax - hourTmin;
			theta = PI * (-0.5 + hh / nbh);
		}
		else
		{
			//From Tmax to Tmin
			//size_t hh = (24 + h - hourTmax) % 24;
			if (h < hourTmax)
				h += 24;

			double  hh = h - hourTmax;
			double nbh = 24 - (hourTmax - hourTmin);
			theta = PI * (0.5 - hh / nbh);
		}

		//round at .1
		return  Round(mean + range / 2 * sin(theta), 1);
	}

	//mean_sea_level to atmospheric pressure (at elevation)
	//po : mean sea level [kPa]
	double msl2atp(double po, double h)
	{


		//https://en.wikipedia.org/wiki/Atmospheric_pressure
		static const double M = 28.9644e-3;    // (kg mol-1) molecular weight of air 
		static const double R = 8.3143;         // (m3 Pa mol-1 K-1) gas law constant 
		static const double L = 0.0065;    // (K/m) standard temperature lapse rate 
		static const double g = 9.80665;    // (m s-2) standard gravitational accel.  
		static const double To = 288.15;		// (K) standard temp at 0.0 m elevation   

		const double a = 1 - (L*h) / To;
		const double b = (g*M) / (R*L);
		return po * pow(a, b);

		//return 1013 * pow((293 - 0.0065*elev) / 293, 5.26);//pressure at elevation [hPa] or [mbar]
	}

	//p : atmospheric pressure [kPa]
	double atp2msl(double p, double h)
	{//https://en.wikipedia.org/wiki/Atmospheric_pressure
		static const double M = 28.9644e-3;    // (kg mol-1) molecular weight of air 
		static const double R = 8.3143;         // (m3 Pa mol-1 K-1) gas law constant 
		static const double L = 0.0065;    // (K/m) standard temperature lapse rate 
		static const double g = 9.80665;    // (m s-2) standard gravitational accel.  
		static const double To = 288.15;		// (K) standard temp at 0.0 m elevation   

		const double a = 1 - (L*h) / To;
		const double b = (g*M) / (R*L);
		return p / pow(a, b);

	}


	//Constants:
	//NL = 6.0221415·1023 mol-1		Avogadro constant NIST
	//R  = 8.31447215 J mol-1 K-1	Universal gas constant NIST
	//MH2O = 18.01534 g mol-1		molar mass of water
	//Mdry 28.9644 g mol-1			molar mass of dry air

	//http://www.gorhamschaffler.com/humidity_formulas.htm
	 //E: actual vapor pressure(kPa)
	//double GetTd(double E)
	//{
	//	double Td=(-430.22+237.7*log(E*10))/(-log(E*10)+19.08);
	//	return Td;
	////}
	//\begin{ align }P_{ s:m }(T) &= a\exp\bigg(\left(b - \frac{ T }{d}\right)\left(\frac{ T }{c + T}\right)\bigg); \\[8pt]
	//\gamma_m(T, R\!H) &= \ln\Bigg(\frac{ R\!H }{100}\exp
	//\bigg(\left(b - \frac{ T }{d}\right)\left(\frac{ T }{c + T}\right)\bigg)
	//\Bigg); \\
	//T_{ dp } &= \frac{ c\gamma_m(T, R\!H) }{b - \gamma_m(T, R\!H)}; \end{ align }
	//(where \scriptstyle{ 
	//a = 6.1121 [hKa]
	//b = 18.678
	//c = 257.14 [°C]
	//d = 234.5 [°C]
	//There are several different constant sets in use.The ones used in NOAA's

	//*******************************************************************
	//Randomize

	void Randomize(unsigned rand)
	{
		//time_t t;
		//srand((unsigned) time(&t));
		if (rand == unsigned(0))
			srand((unsigned)time(NULL));
		else
			srand((unsigned)rand);
	}

	//returns a double on the interval [0,1]
	/*double Randu(void)
	{
		double u,x;

		x = (double) rand();
		u = x/RAND_MAX;

		_ASSERTE ((u>=0)&&(u<=1));

		return u;
	}*/

	//returns a double on the interval 
	//[0,1] : rand()/RAND_MAX
	//]0,1] : (rand()+1)/(RAND_MAX+1)
	//[0,1[ : rand()/(RAND_MAX+1)
	//]0,1[ : (rand()+1)/(RAND_MAX+2)
	double Randu(bool bExcLower, bool bExcUpper)
	{
		double numerator = (double)rand();
		double denominator = (double)RAND_MAX;

		if (bExcLower)
		{
			numerator++;
			denominator++;
		}

		if (bExcUpper)
			denominator++;

		double u = numerator / denominator;

		_ASSERTE(bExcLower || (u >= 0));
		_ASSERTE(!bExcLower || (u > 0));
		_ASSERTE(bExcUpper || (u <= 1));
		_ASSERTE(!bExcUpper || (u < 1));

		return u;
	}

	//special case of randu
	double RanduExclusif(void)
	{
		return Randu(true, true);
	}

	//returns an integer on the range [0,num]
	int Rand(int num)
	{
		double x = (num + 1)*Randu(false, true);
		return (int)x;
	}

	//returns an integer on the interval [l,u] or [l,u[ or ]l,u] or ]l,u[
	int Rand(int l, int u)
	{
		if (l > u)
			Switch(l, u);

		_ASSERTE(l <= u);


		//get a number [0, u-l] and add l
		return l + Rand(u - l);
	}

	double Rand(double l, double u)
	{
		if (l > u)
			Switch(l, u);

		_ASSERTE(l <= u);

		//get a number [0, u-l] and add l
		return l + (u - l)*Randu();
	}


	double RandNormal(const double x, const double s)
	{
		return  x + (zScore()*s);
	}

	double RandLogNormal(double x, double s)
	{
		return exp(RandNormal(x - Square(s) / 2.0, s));
	}



	/**************************
	*   erf.cpp
	*   author:  Steve Strand
	*   written: 29-Jan-04
	***************************/

	//#include <iostream.h>
	//#include <iomanip.h>
	//#include <strstream.h>
	//#include <math.h>

	/*
	static const double rel_error= 1E-12;        //calculate 12 significant figures
	//you can adjust rel_error to trade off between accuracy and speed
	//but don't ask for > 15 figures (assuming usual 52 bit mantissa in a double)


	double erf(double x)
	//erf(x) = 2/sqrt(pi)*integral(exp(-t^2),t,0,x)
	//         = 2/sqrt(pi)*[x - x^3/3 + x^5/5*2! - x^7/7*3! + ...]
	//         = 1-erfc(x)
	{
		static const double two_sqrtpi=  1.128379167095512574;        // 2/sqrt(pi)
		if (fabs(x) > 2.2) {
			return 1.0 - erfc(x);        //use continued fraction when fabs(x) > 2.2
		}
		double sum= x, term= x, xsqr= x*x;
		int j= 1;
		do {
			term*= xsqr/j;
			sum-= term/(2*j+1);
			++j;
			term*= xsqr/j;
			sum+= term/(2*j+1);
			++j;
		} while (fabs(term)/sum > rel_error);
		return two_sqrtpi*sum;
	}


	double erfc(double x)
	//erfc(x) = 2/sqrt(pi)*integral(exp(-t^2),t,x,inf)
	//           = exp(-x^2)/sqrt(pi) * [1/x+ (1/2)/x+ (2/2)/x+ (3/2)/x+ (4/2)/x+
	//           = 1-erf(x)
	//expression inside [] is a continued fraction so '+' means add to denominator only
	{
		static const double one_sqrtpi=  0.564189583547756287;        // 1/sqrt(pi)
		if (fabs(x) < 2.2) {
			return 1.0 - erf(x);        //use series when fabs(x) < 2.2
		}
		//if (signbit(x)) {               //continued fraction only valid for x>0
		if (x>-DBL_MAX && x<-DBL_MIN) {
			return 2.0 - erfc(-x);
		}
		double a=1, b=x;                //last two convergent numerators
		double c=x, d=x*x+0.5;          //last two convergent denominators
		double q1=0,q2=b/d;             //last two convergents (a/c and b/d)
		double n= 1.0, t;
		do {
			t= a*n+b*x;
			a= b;
			b= t;
			t= c*n+d*x;
			c= d;
			d= t;
			n+= 0.5;
			q1= q2;
			q2= b/d;
		  } while (fabs(q1-q2)/q2 > rel_error);

		return one_sqrtpi*exp(-x*x)*q2;
	}
	*/

	static const double rel_error = 1E-12;        //calculate 12 significant figures
	//you can adjust rel_error to trade off between accuracy and speed
	//but don't ask for > 15 figures (assuming usual 52 bit mantissa in a double)


	double erf(double x)
		//erf(x) = 2/sqrt(pi)*integral(exp(-t^2),t,0,x)
		//       = 2/sqrt(pi)*[x - x^3/3 + x^5/5*2! - x^7/7*3! + ...]
		//       = 1-erfc(x)
	{
		static const double two_sqrtpi = 1.128379167095512574;        // 2/sqrt(pi)
		if (fabs(x) > 2.2) {
			return 1.0 - erfc(x);        //use continued fraction when fabs(x) > 2.2
		}

		double sum = x, term = x, xsqr = x * x;
		int j = 1;
		do {
			term *= xsqr / j;
			sum -= term / (2 * j + 1);
			++j;
			term *= xsqr / j;
			sum += term / (2 * j + 1);
			++j;
		} while (fabs(term / sum) > rel_error);   // CORRECTED LINE

		return two_sqrtpi * sum;
	}


	double erfc(double x)
		//erfc(x) = 2/sqrt(pi)*integral(exp(-t^2),t,x,inf)
		//        = exp(-x^2)/sqrt(pi) * [1/x+ (1/2)/x+ (2/2)/x+ (3/2)/x+ (4/2)/x+ ...]
		//        = 1-erf(x)
		//expression inside [] is a continued fraction so '+' means add to denominator only
	{
		static const double one_sqrtpi = 0.564189583547756287;        // 1/sqrt(pi)

		if (fabs(x) < 2.2) {
			return 1.0 - erf(x);        //use series when fabs(x) < 2.2
		}

		if (x > -DBL_MAX && x < -DBL_MIN) {               //continued fraction only valid for x>0
			return 2.0 - erfc(-x);
		}

		double a = 1, b = x;                //last two convergent numerators
		double c = x, d = x * x + 0.5;          //last two convergent denominators
		double q1, q2 = b / d;             //last two convergents (a/c and b/d)
		double n = 1.0, t;
		do {
			t = a * n + b * x;
			a = b;
			b = t;
			t = c * n + d * x;
			c = d;
			d = t;
			n += 0.5;
			q1 = q2;
			q2 = b / d;
		} while (fabs(q1 - q2) / q2 > rel_error);

		return one_sqrtpi * exp(-x * x)*q2;
	}

	/*double TestExposure(double latDeg, double slopeDeg, double aspectDeg)
	{
		ASSERT( latDeg>=-90 && latDeg<=90);
		ASSERT( slopeDeg>=0 && slopeDeg<=90);
		ASSERT( aspectDeg>=0 && aspectDeg<=360);

		double latitude = Deg2Rad(latDeg);
		double slope = Deg2Rad(slopeDeg);
		double aspect = Deg2Rad(180-aspectDeg);

		double SRI = cos(latitude)*cos(slope) + sin(latitude)*sin(slope)*cos(aspect);

		return SRI;
	}
	*/

	double GetExposition(double latDeg, double slopePourcent, double aspectDeg)
	{
		double SRI = 0;
		if (slopePourcent != -999 && aspectDeg != -999 && slopePourcent != 0 && aspectDeg != 0)
		{
			double slopeDeg = Rad2Deg(atan(slopePourcent / 100));
			ASSERT(latDeg >= -90 && latDeg <= 90);
			ASSERT(slopeDeg >= 0 && slopeDeg <= 90);
			ASSERT(aspectDeg >= 0 && aspectDeg <= 360);

			double latitude = Deg2Rad(latDeg);
			double slope = Deg2Rad(slopeDeg);
			double aspect = Deg2Rad(180 - aspectDeg);

			SRI = cos(latitude)*cos(slope) + sin(latitude)*sin(slope)*cos(aspect);
		}

		return SRI;
		//********************************

	//	_ASSERTE( slopePourcent >= 0);
	//    _ASSERTE(aspect >= 0 && aspect <= 360);
	//    
	//    //Sin et cos carré
	//    double cosLat² = Square(cos(lat*DEG2RAD));
	//    double sinLat² = Square(sin(lat*DEG2RAD));
	//
	////    double fPente = DEG_PER_RAD * asin( fPentePourcent / 100 );
	//	//slope in degree
	//    double slope = RAD2DEG*atan( slopePourcent/100 );
	//    ASSERT( slope == Rad2Deg(atan( slopePourcent/100 )) );
	//    ASSERT(slope>=0 && slope<=90 );
	//	
	//
	//    int nPhi = ((aspect < 135) || (aspect >= 255))?1:-1;
	//    
	//	return slope*(cosLat²*nPhi + sinLat²*cos((aspect - 15)*DEG2RAD));

	}

	void ComputeSlopeAndAspect(double latDeg, double exposition, double& slopePourcent, double& aspect)
	{
		//double fCosLat2 = cos(Deg2Rad(lat)); fCosLat2 *= fCosLat2;
		//double fSinLat2 = sin(Deg2Rad(lat)); fSinLat2 *= fSinLat2;
		double slope = 0;
		int nbRun = 0;

		if (latDeg != 0)
		{
			double latitude = Deg2Rad(latDeg);
			aspect = -1;
			//double slope = 0;
			double denominateur = 0;

			do
			{
				slope = Rand(0.0, 89.0);
				slope = Deg2Rad(slope);
				//aspect = (float)Rand(0.0, 360.0);
				//int nPhi = ((aspect < 135) || (aspect >= 255))?1:-1;
				//_ASSERT(false); // a revoir avex la nouvelle exposition
				//denominateur = (fCosLat2*nPhi + fSinLat2*cos(Deg2Rad(aspect - 15)));
				denominateur = sin(latitude)*sin(slope);
				if (denominateur != 0)
				{
					double tmp = (exposition - cos(latitude)*cos(slope)) / denominateur;
					if (tmp >= -1 && tmp <= 1)
					{
						aspect = float(180 - Rad2Deg(acos(tmp)));
						//slopePourcent = (float)tan( slope )*100;
						//double test = GetExposition(latDeg, slopePourcent, aspect);

					}
				}
				//else
				//{
					//aspect can be anything
					//aspect = 0;
				//}

					//slope = exposition/denominateur;
				nbRun++;
			} while (nbRun < 500 && (denominateur == 0 || aspect < 0 || aspect>359));

			if (nbRun == 500)
			{
				slope = 0;
				aspect = 0;
			}
		}
		else
		{
			ASSERT(exposition >= -1 && exposition <= 1);
			slope = acos(exposition);
		}

		//fPentePourcent = (float)sin( fPente/DEG_PER_RAD )*100;
		slopePourcent = (float)tan(slope) * 100;
		_ASSERTE(slopePourcent >= 0);//&& fPentePourcent <= 100 );
		_ASSERTE(nbRun == 500 || fabs(exposition - GetExposition(latDeg, slopePourcent, aspect)) < 0.001);
	}


	double GetDistance(double lat1, double lon1, double lat2, double lon2)
	{
		double angle = 0;

		if (lat1 != lat2 || lon1 != lon2)
		{
			double P = (fabs(lon2 - lon1) <= 180) ? (lon2 - lon1) : (360.0 - fabs(lon2 - lon1));
			double cosA = sin(lat1*DEG2RAD)*sin(lat2*DEG2RAD) + cos(lat1*DEG2RAD)*cos(lat2*DEG2RAD)*cos(P*DEG2RAD);
			angle = acos(std::max(-1.0, std::min(1.0, cosA)));
		}

		return angle * 6371 * 1000;

	}

	bool IsValidSlopeWindow(float window[3][3], double noData)
	{
		int nbNoData = 0;
		for (int y = 0; y < 3; y++)
		{
			for (int x = 0; x < 3; x++)
			{
				if (window[y][x] <= noData)
				{
					nbNoData++;
					window[y][x] = window[1][1];
				}
			}
		}

		//We don't need the center cell to compute slope and aspect
		//then if it's only the center cell that are missing we return true anyway
		return window[1][1] > noData || nbNoData == 1;
	}

	void GetSlopeAndAspect(float window[3][3], double ewres, double nsres, double scale, double& slope, double& aspect)
	{
		//slope = 0;
		//aspect = 0;
		//if( !IsValidSlopeWindow(window) )
			//return;

		double dx = ((window[0][2] + window[1][2] + window[1][2] + window[2][2]) -
			(window[0][0] + window[1][0] + window[1][0] + window[2][0])) / ewres;

		double dy = ((window[0][0] + window[0][1] + window[0][1] + window[0][2]) -
			(window[2][0] + window[2][1] + window[2][1] + window[2][2])) / nsres;

		double key = (dx * dx + dy * dy);

		slope = (float)(100 * (sqrt(key) / (8 * scale)));
		aspect = (float)(atan2(dy, -dx) * RAD2DEG);

		if (dx == 0 && dy == 0)
		{
			// lat area 
			aspect = 0;
		}
		else //transform from azimut angle to geographic angle
		{
			if (aspect > 90.0f)
				aspect = 450.0f - aspect;
			else
				aspect = 90.0f - aspect;
		}

		if (aspect == 360.0)
			aspect = 0.0f;
	}







	//**************************************************************
	const double CMathEvaluation::EPSILON = 0.000001;
	const char* CMathEvaluation::OPERATOR_STR[NB_OPERATORS] = { "==", "!=", ">=", "<=", ">", "<" };

	CMathEvaluation::CMathEvaluation(const char* strIn)
	{
		ASSERT(strIn != NULL);

		string str(strIn);
		size_t pos = str.find_first_not_of("!=<>");
		ASSERT(pos >= 0);

		m_op = GetOp(str.substr(0, pos));
		m_value = ToValue<double>(str.substr(pos));
	}

	CMathEvaluation::TOperation CMathEvaluation::GetOp(const string& str)
	{
		TOperation op = UNKNOWN;

		for (size_t i = 0; i < NB_OPERATORS; i++)
			if (str == OPERATOR_STR[i])
				op = (TOperation)i;

		//if (str == "==")
		//	op = EQUAL;
		//else if (str == "!=")
		//	op = NOT_EQUAL;
		//else if (str == ">=")
		//	op = GREATER_EQUAL;
		//else if (str == "<=")
		//	op = LOWER_EQUAL;
		//else if (str == ">")
		//	op = GREATER;
		//else if (str == "<")
		//	op = LOWER;

		return op;
	}

	bool CMathEvaluation::Evaluate(double value1, TOperation op, double value2)
	{
		bool bRep = false;
		switch (op)
		{
		case EQUAL:			bRep = fabs(value1 - value2) < EPSILON; break;
		case NOT_EQUAL:		bRep = !Evaluate(value1, EQUAL, value2); break;
		case GREATER_EQUAL:	bRep = value1 >= value2; break;
		case LOWER_EQUAL:	bRep = value1 <= value2; break;
		case GREATER:		bRep = value1 > value2; break;
		case LOWER:			bRep = value1 < value2; break;
		default: ASSERT(false);
		}

		return bRep;
	}

	//********************************************************************************************
	void CBetaDistribution::SetTable(double alpha, double beta)
	{

		int i;
		double tmp;
		double sum_P = 0;

		//Compute Beta cumulative probability distribution

		m_XP[0].m_x = 0;
		m_XP[0].m_p = 0;
		for (i = 1; i <= 50; ++i)
		{
			tmp = (double)i / 50.;
			m_XP[i].m_x = tmp;
			if (i == 50) tmp = 0.995;
			sum_P += pow(tmp, alpha - 1) * pow((1 - tmp), beta - 1);
			m_XP[i].m_p = sum_P;
		}
		//Scale so P is [0,1]
		for (i = 0; i <= 50; ++i)
		{
			m_XP[i].m_p /= sum_P;
		}
	}

	double CBetaDistribution::XfromP(double p)const
	{
		_ASSERTE(p >= 0.0 && p <= 1.0);
		double x = 0;
		for (int i = 49; i >= 0; --i)
		{
			if (p > m_XP[i].m_p)
			{
				double slope = (m_XP[i + 1].m_x - m_XP[i].m_x) / (m_XP[i + 1].m_p - m_XP[i].m_p);
				x = m_XP[i].m_x + (p - m_XP[i].m_p)* slope;
				break;
			}
		}

		ASSERT(!_isnan(x));
		ASSERT(x >= 0);
		return x;
	}

	double CSchoolfield::operator[](double T)const
	{
		const double R = 1.987;
		double Tk = T + 273.15;
		double a1 = m_p[PRHO25] * Tk / 298.15;
		double a2 = exp(m_p[PHA] / R * (1 / 298.15 - 1 / Tk));
		double b1 = exp(m_p[PHL] / R * (1 / m_p[PTL] - 1 / Tk));
		double b2 = exp(m_p[PHH] / R * (1 / m_p[PTH] - 1 / Tk));
		double r = (a1*a2) / (1 + b1 + b2);

		ASSERT(r >= 0);
		return r;
	}

	double CUnknown::operator[](double T)const
	{
		ASSERT(T >= -40 && T < 40);
		double Fo = m_p[PA] + m_p[PB] * pow(fabs(T - m_p[PT0]), m_p[PX]);
		return max(0.0, Fo);
	}

	/*unsigned int good_seed()
	{
		unsigned int random_seed, random_seed_a, random_seed_b;
		std::ifstream file ("/dev/urandom", std::ios::binary);
		if (file.is_open())
		{
			char * memblock;
			int size = sizeof(int);
			memblock = new char [size];
			file.read (memblock, size);
			file.close();
			random_seed_a = *reinterpret_cast<int*>(memblock);
			delete[] memblock;
		}// end if
		else
		{
			random_seed_a = 0;
		}
		random_seed_b = std::time(0);
		random_seed = random_seed_a xor random_seed_b;
		std::cout << "random_seed_a = " << random_seed_a << std::endl;
		std::cout << "random_seed_b = " << random_seed_b << std::endl;
		std::cout << " random_seed =  " << random_seed << std::endl;
		return random_seed;
	} // end good_seed()
	*/

	void CRandomGenerator::Randomize(size_t seed)
	{
		if (seed == RANDOM_SEED)
		{
			static unsigned long ID = 1;
			//seed = static_cast<unsigned long>(std::time(NULL)+ID*1000);
			seed = static_cast<unsigned long>(__rdtsc() + ID * 1000);

			ID++;
			m_gen.seed((ULONG)seed);
		}
		else
		{
			m_gen.seed((ULONG)seed);
		}
	}

}//namespace WBSF