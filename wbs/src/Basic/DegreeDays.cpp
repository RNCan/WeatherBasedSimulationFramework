//******************************************************************************
//  project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
//******************************************************************************


//1- le type
//2- time step: daily, hourly
//3- Cut-off


//Comment faire une référence sur 
//addVANTAGE Pro 6.0 Extensions and Crops

//Average
//The Averaging m_method uses a daily average temperature (historically daily minimum and maximum temperatures). The average is
//calculated, then the lower threshold value is subtracted from the result. Daily values are accumulated to give a cumulative degree-day value.
//DD = max(0.0, (Tmin+Tmax)/2 - TreshMin);


//Average (Adjusted)
//Options are available to the Average m_method (see also “Sensor Placement” on page 24 for more details on these options). The
//Average (adjusted) m_method selects only on the averaged minimum and maximum temperatures resulting out of the samples.
//The Average (adjusted) computing m_method is similar to that used for many agronomic crops in the midwest of the U.S. This m_method
//calculates degree-days using the daily minimum and maximum temperatures. Maximum temperatures above the upper threshold are
//set as being equal to the upper threshold. Minimum temperatures below the lower threshold are set as being equal to the lower
//threshold. The daily minimum and maximum values are added and then divided by two. The lower threshold value is then subtracted from
//the result. Daily values are accumulated to give a cumulative degreeday value.
//Tmin = Max( Tmin, TreshMin);
//Tmax = Min( Tmax, TreshMax);
//DD = max(0.0, (Tmin+Tmax)/2 - TreshMin);

//DAILY_AVERAGE_EXCLUSIVE
//if(Tmin>=TreshMin&&Tmax<=TreshMin)
//	DD = max(0.0, (Tmin+Tmax)/2 - TreshMin);
//else DD = 0;

//Single Triangle
//The Single Triangle m_method calculates degree-days using daily minimum and maximum temperatures to create triangles that
//estimate the shape of the daily temperature curve over a 24-hour period.The first side of the triangle consists of a line drawn from a day’s
//minimum temperature to that day’s maximum temperature. The second side is drawn using the same minimum temperature as the
//first side. The area under the triangle and above the lower threshold is used as an estimation of the degree-day value for a 24-hour period.
//An upper threshold setting is available when a cutoff m_method is selected. When a cutoff m_method is selected, the area in the triangle
//that is used to estimate the degree-day value is reduced according to the rules of the cutoff m_method.


//Double Triangle
//The Double Triangle m_method estimates the area under a daily temperature curve by drawing triangles over two 12-hour periods
//using minimum and maximum temperatures. The first side of the first triangle is drawn between the daily minimum and maximum. The
//second side is drawn as a vertical line through the maximum temperature. A second triangle is drawn for the second 12-hour period
//using the same guidelines but using the daily maximum temperature and minimum temperature from the next day. Degree-days are
//calculated as the sum of the areas under both curves and between any thresholds. An upper threshold setting is available when a cutoff
//m_method is selected. When a cutoff m_method is selected, the areas in the triangles that are used to estimate the heat-unit values are reduced
//according to the rules of the cutoff m_method.


//Modified


//More About Cutoff Methods
//The cutoff m_method is also a key component of a phenological model. It modifies the daily degree-day calculation to more accurately reflect an
//organism’s growth response to high temperatures. Three cutoff methods are included in this extension. They are the Horizontal,
//Intermediate, and Vertical cutoff methods.


//Horizontal Cutoff
//The Horizontal cutoff m_method treats phenological development as continuing at a constant rate above the upper threshold. The area
//calculated above the upper threshold is subtracted from the area above the lower threshold when this m_method is used.

//Intermediate Cutoff
//The Intermediate cutoff m_method is used in cases where development slows above the upper threshold. In this case, the area calculated
//above the upper threshold is subtracted twice from the area above the lower threshold.

//Vertical Cutoff
//The Vertical cutoff m_method is used in cases where no development above the upper threshold exists. Temperatures in the area above the
//upper threshold are not used in this case.



#include "stdafx.h"
#include <math.h>
#include <fstream>
#include <ostream>

#include "Basic/DegreeDays.h"


using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace WBSF::WEATHER;

namespace WBSF
{

	const char CDegreeDays::HEADER[] = "DD";
	//****************************************************************************
	// Sommaire:     Constructeur.
	//
	// Description:  create a day with Tmin, Tmax and ppt
	//
	// Entrée:
	//
	// Sortie:
	//
	// Note:
	//****************************************************************************
	//
	//ERMsg CDegreeDays::Execute(CWeatherStation& weather)
	//{
	//	
	//}

	//http://en.wikipedia.org/wiki/Growing-degree_day
	//http://www.adcon.at
	//http://www.ipm.ucdavis.edu/WEATHER/ddroutines.html


	double CDegreeDays::GetDD(const CWeatherDay& in)const
	{
		ASSERT(m_method < CDegreeDays::NB_DAILY_METHOD);
		ASSERT(m_lowerThreshold <= m_upperThreshold);
		ASSERT(in[H_TMIN2].IsInit() && in[H_TMAX2].IsInit());

		double DD = 0;
		switch (m_method)
		{
		case DAILY_AVERAGE: DD = GetAverageDD(in); break;
		case DAILY_AVERAGE_ADJUSTED: DD = GetAverageAdjustedDD(in); break;
		case MODIFIED_ALLEN_WAVE:DD = GetModifiedAllenWaveDD(in); break;
		case SINGLE_TRIANGLE:	DD = GetTriangleDD(in); break;
		case DOUBLE_TRIANGLE:	DD = GetDoubleTriangleDD(in); break;
		case SINGLE_SINE:		DD = GetSineDD(in); break;
		case DOUBLE_SINE:		DD = GetDoubleSineDD(in); break;
		default: ASSERT(false);
		}

		return DD;
	}

	double CDegreeDays::GetDD(const CWeatherMonth& in, const CTPeriod& p)const
	{
		double DD = 0;
		for (size_t d = 0; d < in.size(); d++)
		{
			if (!p.IsInit() || p.IsInside(in[d].GetTRef()))
				DD += GetDD(in[d]);
		}

		return DD;
	}

	double CDegreeDays::GetDD(const CWeatherYear& in, const CTPeriod& p)const
	{
		double DD = 0;

		for (size_t m = 0; m < 12; m++)
			DD += GetDD(in[m], p);

		return DD;
	}

	double CDegreeDays::GetAverageDD(const CWeatherDay& in)const
	{
		ASSERT(m_lowerThreshold <= m_upperThreshold);
		ASSERT(in[H_TMIN2].IsInit() && in[H_TMAX2].IsInit());

		double DD = 0;
		double x1 = max(0.0, min((double)in[H_TNTX][MEAN], m_upperThreshold) - m_lowerThreshold);
		double x2 = min(0.0, m_upperThreshold - in[H_TNTX][MEAN]);

		switch (m_cutoffType)
		{
		case HORIZONTAL_CUTOFF: DD = x1; break;
		case INTERMEDIATE_CUTOFF:DD = max(0.0, x1 + x2); break;
		case VERTICAL_CUTOFF:DD = (in[H_TNTX][MEAN] <= m_upperThreshold) ? x1 : 0; break;
		default: ASSERT(false);
		}

		return DD;
	}

	double CDegreeDays::GetAverageAdjustedDD(const CWeatherDay& in)const
	{
		ASSERT(m_lowerThreshold <= m_upperThreshold);
		ASSERT(in[H_TMIN2].IsInit() && in[H_TMAX2].IsInit());

		double DD = 0;
		double Tmin = max(in[H_TMIN2][MEAN], m_lowerThreshold);
		double Tmax = min(in[H_TMAX2][MEAN], m_upperThreshold);
		double x1 = max(0.0, (Tmin + Tmax) / 2 - m_lowerThreshold);
		double x2 = min(0.0, m_upperThreshold - in[H_TMAX2][MEAN]);

		switch (m_cutoffType)
		{
		case HORIZONTAL_CUTOFF: DD = x1; break;
		case INTERMEDIATE_CUTOFF:DD = max(0.0, x1 + x2); break;
		case VERTICAL_CUTOFF:DD = (Tmin >= m_lowerThreshold && Tmax <= m_upperThreshold) ? x1 : 0; break;
		default: ASSERT(false);
		}

		return DD;
	}



	double CDegreeDays::GetTriangleDD(const CWeatherDay& in)const
	{
		ASSERT(m_lowerThreshold <= m_upperThreshold);
		ASSERT(in[H_TMIN2].IsInit() && in[H_TMAX2].IsInit());

		double Tmin = in[H_TMIN2][MEAN];
		double Tmax = in[H_TMAX2][MEAN];
		double DD = 2 * GetTriangleDD(Tmin, Tmax, m_lowerThreshold, m_upperThreshold, m_cutoffType);
		return max(0.0, DD);
	}

	double CDegreeDays::GetDoubleTriangleDD(const CWeatherDay& in)const
	{
		ASSERT(m_lowerThreshold <= m_upperThreshold);
		ASSERT(in[H_TMIN2].IsInit() && in[H_TMAX2].IsInit());

		double Tmin = in[H_TMIN2][MEAN];
		double Tmax = in[H_TMAX2][MEAN];
		double DD = GetTriangleDD(Tmin, Tmax, m_lowerThreshold, m_upperThreshold, m_cutoffType);

		const CWeatherDay& nextDay = in.GetNext();
		Tmin = nextDay[H_TMIN2][MEAN];
		Tmax = nextDay[H_TMAX2][MEAN];
		DD += GetTriangleDD(Tmin, Tmax, m_lowerThreshold, m_upperThreshold, m_cutoffType);

		return DD;
	}


	double CDegreeDays::GetSineDD(const CWeatherDay& in)const
	{
		ASSERT(m_lowerThreshold <= m_upperThreshold);
		ASSERT(in[H_TMIN2].IsInit() && in[H_TMAX2].IsInit());



		ASSERT(GetSineDD(1, 2, 2, 3, HORIZONTAL_CUTOFF) == 0);
		ASSERT(GetSineDD(1, 2, 2, 3, INTERMEDIATE_CUTOFF) == 0);
		ASSERT(GetSineDD(1, 2, 2, 3, VERTICAL_CUTOFF) == 0);
		GetSineDD(1, 2, 1.5, 2.5, HORIZONTAL_CUTOFF);//1/8
		GetSineDD(1, 2, 1.5, 2.5, INTERMEDIATE_CUTOFF);//1/8
		GetSineDD(1, 2, 1.5, 2.5, VERTICAL_CUTOFF);//1/8
		ASSERT(GetSineDD(1, 2, 0, 1, HORIZONTAL_CUTOFF) == 1);
		ASSERT(GetSineDD(1, 2, 0, 1, INTERMEDIATE_CUTOFF) == 0.5);
		ASSERT(GetSineDD(1, 2, 0, 1, VERTICAL_CUTOFF) == 0);
		GetSineDD(1, 2, 0, 3, HORIZONTAL_CUTOFF);// == 4.0/8.0);
		GetSineDD(1, 2, 0, 3, INTERMEDIATE_CUTOFF);// == 4.0/8.0);
		GetSineDD(1, 2, 0, 3, VERTICAL_CUTOFF);// == 4.0/8.0);
		GetSineDD(1, 2, 1.25, 1.75, HORIZONTAL_CUTOFF);
		GetSineDD(1, 2, 1.25, 1.75, INTERMEDIATE_CUTOFF);
		GetSineDD(1, 2, 1.25, 1.75, VERTICAL_CUTOFF);

		double Tmin = in[H_TMIN2][MEAN];
		double Tmax = in[H_TMAX2][MEAN];
		double DD = 2 * GetSineDD(Tmin, Tmax, m_lowerThreshold, m_upperThreshold, m_cutoffType);

		return max(0.0, DD);
	}

	double CDegreeDays::GetDoubleSineDD(const CWeatherDay& in)const
	{
		ASSERT(m_lowerThreshold <= m_upperThreshold);
		ASSERT(in[H_TMIN2].IsInit() && in[H_TMAX2].IsInit());

		double Tmin = in[H_TMIN2][MEAN];
		double Tmax = in[H_TMAX2][MEAN];
		double DD = GetSineDD(Tmin, Tmax, m_lowerThreshold, m_upperThreshold, m_cutoffType);

		const CWeatherDay& nextDay = in.GetNext();
		Tmin = nextDay[H_TMIN2][MEAN];
		Tmax = nextDay[H_TMAX2][MEAN];

		DD += GetSineDD(Tmin, Tmax, m_lowerThreshold, m_upperThreshold, m_cutoffType);

		return DD;
	}




	double CDegreeDays::GetModifiedAllenWaveDD(const CWeatherDay& in)const
	{
		double DD = 0;
		double x1 = GetModifiedAllenWaveDD(in, m_lowerThreshold, m_upperThreshold);
		double x2 = -GetModifiedAllenWaveDD(in, m_upperThreshold, 9999);

		switch (m_cutoffType)
		{
		case HORIZONTAL_CUTOFF: DD = x1; break;
		case INTERMEDIATE_CUTOFF:DD = max(0.0, x1 + x2); break;
		case VERTICAL_CUTOFF:DD = MISSING; break;
		default: ASSERT(false);
		}

		return DD;
	}


	//*********************************************
	//static
	double CDegreeDays::GetTriangleVerticalCutoff(double Tmin, double Tmax, double Tl, double Tu)
	{
		//Initializations
		if (Tmax <= Tu)
			return 0;

		if (Tmin == Tmax || Tmin >= Tu)
			return Tu - Tl;

		double q = (Tmax - Tu) / (Tmax - Tmin);
		return (q * (Tu - Tl)) / 2;
	}

	double CDegreeDays::GetTriangleDD(double Tmin, double Tmax, double Th)
	{

		ASSERT(Tmin <= Tmax);

		double DD = 0;

		if (Tmin<Tmax)
		{
			if (Tmin<Th)
			{
				if (Tmax>Th)
					DD = (0.5 * (Tmax - Th)*(Tmax - Th)) / (Tmax - Tmin);
			}
			else
			{
				DD = 0.5 * (Tmax + Tmin - (2 * Th));
			}
		}
		else
		{
			if (Tmax>Th)
				DD = Tmax - Th;
		}


		double Thx = max(Tmin, min(Tmax, Th));

		double x1 = max(0.0, Square(Tmax - Thx) / (2 * (Tmax - Tmin)));
		double x2 = max(0.0, Tmin - Th);

		ASSERT(x1 + x2 == DD);
		return (x1 + x2) / 2;
	}

	double CDegreeDays::GetTriangleDD(double Tmin, double Tmax, double Tl, double Tu, size_t m_cutoffType)
	{
		if (Tmin > Tmax)
			Switch(Tmin, Tmax);

		ASSERT(Tl <= Tu);
		ASSERT(Tmin <= Tmax);

		double DD = 0;
		double x1 = GetTriangleDD(Tmin, Tmax, Tl);
		double x2 = GetTriangleDD(Tmin, Tmax, Tu);
		double x3 = GetTriangleVerticalCutoff(Tmin, Tmax, Tl, Tu);

		//Compute area for triangular m_method

		switch (m_cutoffType)
		{
		case HORIZONTAL_CUTOFF: DD = max(0.0, x1 - x2); break;
		case INTERMEDIATE_CUTOFF:DD = max(0.0, x1 - x2 - min(x2, x3)); break;
		case VERTICAL_CUTOFF:DD = max(0.0, x1 - x2 - x3); break;
		default: ASSERT(false);
		}


		return DD;
	}

	//Single Sine
	//The Single Sine m_method estimates the area under a daily temperature curve by drawing a sine curve over a 24-hour period through the daily
	//minimum and maximum temperatures. Degree-day values are estimated as the area within the sine curve that is above the lower
	//threshold. When a cutoff m_method is selected, the area used for determining degree-day values is reduced according the to the rules of
	//the cutoff m_method. According to UC IPM, the single sine m_method with a horizontal cutoff has been the most commonly used m_method of
	//determining degree-day values in California for many years.


	//     PURPOSE:
	//			Calculate the area to be subtracted from degree-days
	//			computed with a horizontal cut off to produce a vertical cut off.
	//
	//    *******************************************************************
	// copyright 1985 - Regents of the University of California.  All rights reserved.
	double CDegreeDays::GetSineVerticalCutoff(double Tmin, double Tmax, double Tl, double Tu)
	{
		if (Tmax <= Tu)
			return 0;

		if (Tmin == Tmax || Tmin >= Tu)
			return Tu - Tl;

		//compute area for sine wave
		double tm = 0.5 * (Tmax + Tmin);
		double ta = 0.5 * (Tmax - Tmin);

		double theta = asin((Tu - tm) / ta);
		double q = PI - 2.0 * theta;

		return (q * (Tu - Tl) / (PI * 2.0)) / 2;
	}

	//     PURPOSE:
	//      To calculate degree-days accumulated above or below a
	//     threshold using a sine wave estimation of area under the curve.
	//
	//    *****************************************************************
	//Copyright 1985 - Regents of the University of California.  All rights reserved.

	double CDegreeDays::GetSineDD(double Tn, double Tx, double th)
	{
		ASSERT(Tn <= Tx);

		//Initialize variables
		double DD = 0.0;

		if (Tx <= th)
			return 0;

		if (Tn < Tx)
		{
			double Tm = 0.5 * (Tx + Tn);
			double Ta = 0.5 * (Tx - Tn);
			double arg = max(-1.0, min(1.0, (th - Tm) / Ta));
			double xa = abs(arg);

			//approximate value of theta at arg
			double q = abs(1.57079632 - sqrt(1.0 - xa) * (1.5707288 + xa*(-0.2121144 + xa * (0.0745610 - xa * 0.0187293))));

			if (arg<0)
				q = -q;

			double theta = q + 1.57079632;                  // theta = arccos (arg)
			DD = ((Tm - th) * (PI - theta) + Ta * sin(theta)) / PI;
		}
		else
		{
			if (Tx>th)
				DD = Tx - th;
		}


		return DD / 2;
	}

	double CDegreeDays::GetSineDD(double Tmin, double Tmax, double Tl, double Tu, size_t m_cutoffType)
	{
		if (Tmin > Tmax)
			Switch(Tmin, Tmax);

		double DD = 0;
		double x1 = GetSineDD(Tmin, Tmax, Tl);
		double x2 = GetSineDD(Tmin, Tmax, Tu);
		double x3 = GetSineVerticalCutoff(Tmin, Tmax, Tl, Tu);

		switch (m_cutoffType)
		{
		case HORIZONTAL_CUTOFF: DD = max(0.0, x1 - x2); break;
		case INTERMEDIATE_CUTOFF:DD = max(0.0, x1 - x2 - min(x2, x3)); break;
		case VERTICAL_CUTOFF:DD = max(0.0, x1 - x2 - x3); break;
		default: ASSERT(false);
		}

		return max(0.0, DD);
	}

	//Double Sine 
	//The Double Sine m_method estimates the area under a daily temperature curve by drawing sine curves over two 12-hour periods. The first curve
	//is drawn using the daily minimum temperature with the first half of the sine curve to represent the first 12 hours. The second curve is
	//drawn from the daily maximum temperature and the minimum temperature of the following day and using the second half of this
	//curve to represent the second 12 hours. An upper threshold setting is available when a cutoff m_method is selected. When a cutoff m_method is
	//selected, the area under the curves that is used to estimate the degree-day value is reduced according to the rules of the cutoff
	//m_method. Degree-days are calculated as the sum of the areas under both curves and between any thresholds.


	//DESCRIPTION: this code was translated to C from the code of Allen (1976):
	//  Envir.Entomol, 5:388-396 using FOR-C software. Only heating units are used.
	double CDegreeDays::GetModifiedAllenWaveDD(const CWeatherDay& in, double threshLow, double threshHigh)
	{
		ASSERT(threshLow <= threshHigh);
		ASSERT(in[H_TMIN2].IsInit() && in[H_TMAX2].IsInit());

		double ahdd = 0.0; //heating degree-days 
		double acdd = 0.0; //cooling degree-days 


		double tmin1 = in[H_TMIN2][MEAN];//minimum temperature in this day 
		double tmax = in[H_TMAX2][MEAN];//maximum temperature in this day 


		const CWeatherDay& nextDay = in.GetNext();
		double tmin2 = nextDay[H_TMIN2][MEAN];//minimum temperature in the next day 

		// calculate heating and cooling degree-days using Allen's modified
		// sine wave m_method (without bias correction).

		for (int j = 0; j < 2; j++)
		{
			double hdd = 0;
			double cdd = 0;

			double tmin = j == 0 ? tmin1 : tmin2;

			double tbar = (tmax + tmin) / 2.;
			double a = (tmax - tmin) / 2.;
			if (threshLow >= tmax)
			{
				hdd = 0.0;
				cdd = 0.5*(threshLow - tbar);
			}
			else if (threshHigh > tmin)
			{
				if (threshLow > tmin)
				{
					double x1 = 0, x2 = 0, t1 = 0, t2 = 0;
					if (threshHigh < tmax)
					{
						x1 = (threshLow - tbar) / a;
						x2 = (threshHigh - tbar) / a;
						t1 = atan(x1 / sqrt(1.0 - x1*x1));
						t2 = atan(x2 / sqrt(1.0 - x2*x2));
					}
					else
					{
						t2 = 1.570796;
						x1 = (threshLow - tbar) / a;
						t1 = atan(x1 / sqrt(1.0 - x1*x1));
					}
					hdd = .1591549*((tbar - threshLow)*(t2 - t1) + a*(cos(t1) - cos(t2)) + (threshHigh - threshLow)*(1.570796 - t2));
					cdd = .1591549*((threshLow - tbar)*(t1 + 1.570796) + a*cos(t1));
				}
				else if (tmax > threshHigh)
				{
					double t1 = -1.570796;
					double x2 = (threshHigh - tbar) / a;
					double t2 = atan(x2 / sqrt(1.0 - x2*x2));
					hdd = .1591549*((tbar - threshLow)*(t2 - t1) + a*(cos(t1) - cos(t2)) + (threshHigh - threshLow)*(1.570796 - t2));
					cdd = .1591549*((threshLow - tbar)*(t1 + 1.570796) + a*cos(t1));
				}
				else
				{
					hdd = 0.5*(tbar - threshLow);
					cdd = 0.0;
				}
			}
			else
			{
				hdd = 0.5*(threshHigh - threshLow);
				cdd = 0.0;
			}
			ahdd += hdd;
			acdd += cdd; // cooling units 'acdd' are not used here 
		}

		return ahdd;
	}

	void CDegreeDays::Execute(CWeatherStation& weather, CModelStatVector& output)
	{
		CTPeriod p = weather.GetEntireTPeriod(CTM::DAILY);// (TM);
		output.Init(p, NB_OUTPUT, 0, HEADER);

		for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
		{
			double dd = GetDD(weather.GetDay(TRef));
			output[TRef][S_DD] = dd;
		}
	}





	//***********************************************************************************************************************
	const char CDegreeHours::HEADER[] = "DH";
	void CDegreeHours::Execute(CWeatherStation& weather, CModelStatVector& output)
	{
		ASSERT(weather.IsHourly());
		
		
		CTPeriod p = weather.GetEntireTPeriod();
		output.Init(p, NB_OUTPUT, 0, HEADER);

		for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
		{
			double dh = GetDH(weather.GetHour(TRef));
			output[TRef][S_DH] = dh;
			if (m_bCumulative && TRef != p.Begin())
				output[TRef][S_DH] += output[TRef-1][S_DH];
		}
	}

	
	double CDegreeHours::GetDH(const CHourlyData& in)const
	{
		ASSERT(m_lowerThreshold <= m_upperThreshold);
		ASSERT(!IsMissing(in[H_TAIR2]));
	
		double DH = 0;
		double x1 = max(0.0, min((double)in[H_TAIR2], m_upperThreshold) - m_lowerThreshold);
		double x2 = min(0.0, m_upperThreshold - in[H_TAIR2]);

		switch (m_cutoffType)
		{
		case HORIZONTAL_CUTOFF: DH = x1; break;
		case INTERMEDIATE_CUTOFF:DH = max(0.0, x1 + x2); break;
		case VERTICAL_CUTOFF:DH = in[H_TAIR2] <= m_upperThreshold ? x1 : 0; break;
		default: ASSERT(false);
		}

		return DH;
	}

}//namespace WBSF