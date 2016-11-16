#include "GypsyMothCommon.h"
#include <math.h>
#include <crtdbg.h>

//int allen_sin_wave(double tmin1,double tmax,double tmin2,double peak,double *hourly,int step) /* this routines generates temperature by using allen sin wave method */
//{
//	int hour;
//
//	int time_factor=6;  /*  "rotates" the radian clock to put the peak at the top  */
//
//	double r_hour;     /*  hours converted to radians:  24=2*pi  */
//	double mean1;      /*  mean temperature for day1   */
//	double range1;     /*  half the difference of min and max temps of day1  */
//	double mean2;      /*  mean of max temp (day1) and min temp (day2)  */
//	double range2;     /*  half the difference of max (day1) and min (day2)  */
//	double mean;       /*  one or the other of the means     */
//	double range;      /*  one or the other of the ranges    */
//	double theta;
//
//
//
//
//	mean1=(tmax+tmin1)/2;
//	range1=(tmax-tmin1)/2;
//	mean2=(tmax+tmin2)/2;
//	range2=(tmax-tmin2)/2;
//	r_hour=3.14159f/12;
//
//	for(hour=0; hour<24; hour+=step)
//	{
//		if(hour<peak)mean=mean1;
//		else mean=mean2;
//		if(hour<peak)range=range1;
//		else range=range2;
//
//		theta=((double)hour-(double)time_factor)*r_hour;
//		hourly[hour]=(double)(mean+range*sin(theta));
//	}
//	return(0);
//}

double develop(int sex, int stage, std::vector<double>& t)
{
	_ASSERTE( t.size() > 0);
	//int hour;
	double devel_sum = 0;

	for (size_t i=0; i<t.size(); i++) 
	{
		devel_sum += rat(sex, stage, t[i]);
	}

	return (double)(devel_sum/t.size());
}

double rat(int sex, int stage, double t)
{
	/*   Development of 5th, 6th and Pupae of Gypsy Moth taken from

		 Sheenan, K.A. 1992. User's guide for GMPHEN: Gypsy Moth Phenology Model.
		 General Tech. Rep. NE-158. USDA For. Serv. Northeastern For. Esp.
		 Sta. 29 p.

		 -------------------------------------------------------
		 Males                    Females
		 -----------------      ---------------------
		 Param.     5th   6th   Pupae       5th    6th    Pupae
		 -------------------------------------------------------
		 Lower T    7.1    -     6.6        8.1    8.1     5.1
		 Upper T     41    -      41         41    41       41
		 DD      240    -     277        100    226     234
		 Slope   .004167  -   .003610      .01  .004425 .004274
		 -------------------------------------------------------

		 Guess at male longevity by A. Sharov (11 /3 / 1996)` 7 @30, 10 @20, 15 @11. J. Regniere adds
		 that Longevity remains as per 10C below 10C.

		 */

	double rate = 0;
	static const int ifct[2][8] = { { 6, 5, 6, 5, 0, 0, 0, 0 }, { 6, 5, 6, 5, 0, 0, 0, 0 } };
	static const double tp[2][8][5] =
	{
		{ { 0.964300f, 7.700000f, 0.142700f, 30.870001f, 12.650000f },
		{ 0.145400f, 0.172000f, 21.090000f, 4.688000f, 0.000000f },
		{ 1.203900f, 8.062000f, 0.173700f, 24.120001f, 8.494000f },
		{ 0.112000f, 0.142200f, 22.290000f, 5.358000f, 0.000000f },
		{ 7.200000f, 0.004167f, 41.000000f, 0.000000f, 0.000000f },
		{ 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f },
		{ 6.600000f, 0.003610f, 41.000000f, 0.000000f, 0.000000f },
		{ 0.000000f, 0.004000f, 100.000000f, 0.000000f, 0.000000f } },

		{ { 0.964300f, 7.700000f, 0.142700f, 30.870001f, 12.650000f },
		{ 0.145400f, 0.172000f, 21.090000f, 4.688000f, 0.000000f },
		{ 1.203900f, 8.062000f, 0.173700f, 24.120001f, 8.494000f },
		{ 0.112000f, 0.142200f, 22.290000f, 5.358000f, 0.000000f },
		{ 8.100000f, 0.010000f, 41.000000f, 0.000000f, 0.000000f },
		{ 8.100000f, 0.004425f, 41.000000f, 0.000000f, 0.000000f },
		{ 5.100000f, 0.004274f, 41.000000f, 0.000000f, 0.000000f },
		{ 0.000000f, 0.004000f, 100.000000f, 0.000000f, 0.000000f } }
	};

	static const double rtrng[2][8][2] = {
		10.000000f, 100.000000f,
		13.300000f, 100.000000f,
		13.300000f, 100.000000f,
		13.300000f, 100.000000f,
		0.000000f, 100.000000f,
		0.000000f, 100.000000f,
		0.000000f, 100.000000f,
		0.000000f, 100.000000f,

		10.000000f, 100.000000f,
		13.300000f, 100.000000f,
		13.300000f, 100.000000f,
		13.300000f, 100.000000f,
		0.000000f, 100.000000f,
		0.000000f, 100.000000f,
		0.000000f, 100.000000f,
		0.000000f, 100.000000f };

	if (stage == 8 && t < 10) t = 10;

	switch (ifct[sex][stage]) /* determine which function is to be used */
	{
	case 0:
		rate = (xlin(t, tp[sex][stage][0], tp[sex][stage][1], tp[sex][stage][2]));
		break;
	case 1:
		rate = (exprat(t, tp[sex][stage][0], tp[sex][stage][1], rtrng[sex][stage][0]));
		break;
	case 2:
		rate = (exptb(t, tp[sex][stage][0], tp[sex][stage][1]));
		break;
	case 3:
		rate = (blogan(t, tp[sex][stage][0], tp[sex][stage][1], tp[sex][stage][2], tp[sex][stage][3]));
		break;
	case 4:
		rate = (stnrat(t, tp[sex][stage][0], tp[sex][stage][1], tp[sex][stage][2], tp[sex][stage][3]));
		break;
	case 5:
		rate = (alogan(t, tp[sex][stage][0], tp[sex][stage][1], tp[sex][stage][2], tp[sex][stage][3], rtrng[sex][stage][0]));
		break;
	case 6:
		rate = (clogan(t, tp[sex][stage][0], tp[sex][stage][1], tp[sex][stage][2], tp[sex][stage][3], tp[sex][stage][4], rtrng[sex][stage][0]));
		break;
	case 7:
		rate = (typiii(t, tp[sex][stage][0], tp[sex][stage][1], tp[sex][stage][2], tp[sex][stage][3], tp[sex][stage][4]));
		break;
	case 8:
		rate = (sigmoid(t, tp[sex][stage][0], tp[sex][stage][1], tp[sex][stage][2]));
		break;
	}
	return(rate);
}


double xlin(double x, double p1, double p2, double p3)
/* p1: base temperature */
/* p2: slope after base */
/* p3: upper threshold */
{
	double t;
	if (x < p1 || x > p3)
	{
		return(0.0);
	}
	else
	{
		t = x - p1;
		return(p2 * t);
	}
}

double exprat(double x, double p1, double p2, double p3)
/* p1: psi */
/* p2: slope after psi */
/* p3: base temperature */
{
	double t;

	t = x - p3;
	return (double)(p1 * exp(p2 * t));
}

double exptb(double x, double p1, double p2)
/* p1: base temperature */
/* p2: slope after base temperature */
{
	return (double)(exp(p2 * (x - p1)) - 1.0);
}

double blogan(double x, double p1, double p2, double p3, double p4) /* to compute the PHASE II base termperature in Logan, et al. 1979 */
/* Simulating H. zea spring emergence. Environ. Entomol. 8:141-146 */
/* psi */
/* rho */
/* del T */
/* tb */
{
	double t;
	double x1;
	double x2;

	if (x < p4)
	{
		return(0.0);
	}
	else
	{
		t = x - p4;
		x1 = (double)exp(p2 * t);
		x2 = (double)exp(-t / p3);
		return(p1 * (x1 - x2));
	}
}

double stnrat(double x, double p1, double p2, double p3, double p4) /* Stinner et.al. developmental rate curve */
/* = c */
/* = k1 */
/* = k2 */
/* = T[opt] */
{
	double t;
	if (x > p4)
	{
		t = 2.0f * p4 - x;
	}
	else
	{
		t = x;
	}
	return (double)(p1 / (1.0 + exp(p2 + p3 * t)));
}

double alogan(double x, double p1, double p2, double p3, double p4, double p5)
/* psi */
/* rho */
/* Tmax, this is in degrees above tb */
/* delT */
/* base temperature -- this is nor really a parameter but arbitrary depending on theh particular data set */
/* -- can be any value but should be scaled to data */
{
	double t;
	double tb;
	double z1;
	double z2;
	double z3;

	tb = p5;
	t = x - tb;
	z1 = (double)exp(p2 * t);
	z2 = (double)exp(p2 * p3 - (p3 - t) / p4);
	z3 = p1 * (z1 - z2);
	if (z3 < 0.0)
	{
		return(0.0);
	}
	else
	{
		return(z3);
	}
}

double clogan(double x, double p1, double p2, double p3, double p4, double p5, double p6) /* modified eq.(10) from logan et al. 1976. Environ Ent. 5:1133-40. */
{
	double alp;
	double b;
	double c;
	double tm;
	double delt;
	double tb;
	double t;
	double xo;
	double tau;
	double xi;
	double xu;

	alp = p1;
	b = p2;
	c = p3;
	tm = p4;
	delt = p5;
	tb = p6;
	t = x - tb;
	xo = (double)(1.0 + b * exp(-c * t));
	xo = alp * (1.0f / xo);
	tau = (tm - t) / delt;
	xi = (double)(alp * (1.0 - exp(-tau)));
	xu = xo + xi - alp;
	if (xu < 0.0)
	{
		return(0.0);
	}
	else
	{
		return(xu);
	}
}

double typiii(double x, double p1, double p2, double p3, double p4, double p5)
/* psi */
/* T max */
/* del T */
/* D */
/* T min */
{
	double xt;
	double tau;
	double x1;
	double x2;
	double x3;

	if (x < p5)
	{
		return(0.0);
	}
	else
	{
		xt = x - p5;
		tau = (p2 - xt) / p3;
		x1 = (xt*xt) / ((xt*xt) + (p4*p4));
		x2 = (double)exp(-tau);
		x3 = p1 * (x1 - x2);
		if (x3 < 0.0)
		{
			return(0.0);
		}
		else
		{
			return(x3);
		}
	}
}

double sigmoid( double x, double p1, double p2, double p3)
/* k1 */
/* k2 */
/* C */
{
	return (double)(p3/(1+exp(p1+(p2*x))));
}

double stinnr(double x, double p1, double p2, double p3, double p4)
/* A */
/* B */
/* p1 */
/* p2 */
{
	double z;
	double ex1;
	double x1;

	if (x > p2)
	{
		return((double)1.0);
	}
	else if (x > p1)
	{
		z = (p2 - x) / (p2 - p1);
		ex1 = p3 * (double)pow((double)z, (double)p4);
		x1 = (double)pow((double)(1.0 - z), (double)ex1);
		return(x1);
	}
	else
	{
		return((double)0.0);
	}
}

double xlogst( double x, double p1, double p2) /* logistic cumulative probability function as given in Regniere, Can. Entomol. 116:1367-76. */
/* K a shaping parameter */
/* Q a skew parameter; =0 - symmetric; >0 - neg. skew; <0 - pos. skew */
{
	double x1;
	double x2;

	x1 = (double)(1.0 + exp((-p1) * (x - 1.0)) * ((1.0 / (double)pow(0.5,(double)p2)) - 1.0));
	x2 = (double)pow((double)x1,(double)(1.0 / p2));
	return (1.0f / x2);
}

double ybulcd(double v, double p1, double p2, double p3) /* computes weibull probability function for variate x */
/* xi the left hand side */
/* c the shaping constant */
/* alpha the scale parameter */
{
	double x;
	double x1;

	if (v < p1)
	{
		return((double)0.0);
	}
	else
	{
		x = v - p1;
		x1 = (double)pow((double)(x / p3), (double)p2);
		return (double)(1.0 - exp(-x1));
	}
}

double cdfy(int stage, double age)
{
	int fctn[8] = { 4, 4, 5, 4, 0, 0, 0, 0 };
	static const double rhop[8][4] = {
		46.400002f, 4.281000f, 6.192000f, 2.630000f,
		20.719999f, 2.196000f, 6.192000f, 2.630000f,
		7.178000f, 1.126000f, 6.192000f, 2.630000f,
		20.719999f, 2.196000f, 6.192000f, 2.630000f,
		0.000000f, 0.000000f, 0.000000f, 0.000000f,
		0.000000f, 0.000000f, 0.000000f, 0.000000f,
		0.000000f, 0.000000f, 0.000000f, 0.000000f,
		0.000000f, 0.000000f, 0.000000f, 0.000000f };

	double xx;
	double hold;

	xx = age;
	/* check for physiological age = 0.0 and rate function used for cumulative distribution */
	if (xx <= 0.0)
		hold = 0.0;
	else
	{
		/* if the model is based on rates, convert t* to r* */
		if (fctn[stage] == 2 || fctn[stage] == 4)
			xx = 1.0f / age;
		/* compute the appropriate cumulative probability */
		if (fctn[stage] == 1)
			hold = stinnr(xx, rhop[stage][0], rhop[stage][1], rhop[stage][2], rhop[stage][3]);
		else if ((fctn[stage] == 2) || (fctn[stage] == 3))
			hold = ybulcd(xx, rhop[stage][0], rhop[stage][1], rhop[stage][2]);
		else if ((fctn[stage] == 4) || (fctn[stage] == 5))
			hold = xlogst(xx, rhop[stage][0], rhop[stage][1]);
		else {
			if (age >= 1)
				hold = 1;
			else
				hold = 0;
		}

		/* if the model is in mormalized rates, then convert to normalized times */
		if (fctn[stage] == 2 || fctn[stage] == 4)
			hold = 1.0f - hold;
	}
	return(hold);
}

int peak(double *arr, int n)
{
	int ret;
	int i;
	double max_arr = -99999999.0f;

	for (i = 0; i < n; ++i) {
		if (arr[i] > max_arr) {
			ret = i + 1;
			max_arr = arr[i];
		}
	}
	return (ret);
}

/****************************************************/
double         ddays_sine_method
/***************************************************
DESCRIPTION: this code was translated to C from the code of Allen (1976):
Envir.Entomol, 5:388-396 using FOR-C software. Only heating units are used.
*/
(double tmin1,     /* minimum temperature in this day */
double tmax,       /* maximum temperature in this day */
double tmin2,      /* minimum temperature in the next day */
double threshlow,  /* low temperature thresholds */
double threshhigh) /* high temperature thresholds */
{
	int i;
	double
		ahdd = 0.0,
		a, hdd, t1, t2, tbar, tmin, x1, x2, acdd = 0.0, cdd;

	/* calculate heating and cooling degree-days using Allen's modified
	 * sine wave method (without bias correction).
	 */
	for (i = 1; i <= 2; i++){
		if (i > 1)
			tmin = tmin2;
		else
			tmin = tmin1;
		tbar = (tmax + tmin) / 2.;
		a = (tmax - tmin) / 2.;
		if (threshlow >= tmax){
			hdd = 0.0;
			cdd = 0.5*(threshlow - tbar);
		}
		else if (threshhigh > tmin){
			if (threshlow > tmin){
				if (threshhigh < tmax){
					x1 = (threshlow - tbar) / a;
					x2 = (threshhigh - tbar) / a;
					t1 = atan(x1 / sqrt(1.0 - x1*x1));
					t2 = atan(x2 / sqrt(1.0 - x2*x2));
				}
				else{
					t2 = 1.570796;
					x1 = (threshlow - tbar) / a;
					t1 = atan(x1 / sqrt(1.0 - x1*x1));
				}
				hdd = .1591549*((tbar - threshlow)*(t2 - t1) + a*(cos(t1) -
					cos(t2)) + (threshhigh - threshlow)*(1.570796 - t2));
				cdd = .1591549*((threshlow - tbar)*(t1 + 1.570796) + a*cos(t1));
			}
			else if (tmax > threshhigh){
				t1 = -1.570796;
				x2 = (threshhigh - tbar) / a;
				t2 = atan(x2 / sqrt(1.0 - x2*x2));
				hdd = .1591549*((tbar - threshlow)*(t2 - t1) + a*(cos(t1) -
					cos(t2)) + (threshhigh - threshlow)*(1.570796 - t2));
				cdd = .1591549*((threshlow - tbar)*(t1 + 1.570796) + a*cos(t1));
			}
			else{
				hdd = 0.5*(tbar - threshlow);
				cdd = 0.0;
			}
		}
		else{
			hdd = 0.5*(threshhigh - threshlow);
			cdd = 0.0;
		}
		ahdd += hdd;
		acdd += cdd; /* cooling units 'acdd' are not used here */
	}
	return(ahdd);
}

//Rounds off double to integers
int round_off(double input)
{
	if (input < 0)return((int)(input - 0.5));
	else if (input>0)return((int)(input + 0.5));
	else return(0);
}
