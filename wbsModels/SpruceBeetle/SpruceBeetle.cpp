//*********************************************************************
//15/02/2007	Rémi Saint-Amant	Creation from matlab(.m) file 
//19/03/2007	Rémi Saint-Amant	Return VMISS and not -999 when we don't find Flight peak 
//22/05/2008	Rémi Saint-Amant	Return 0 and not VMISS when we don't find Flight peak 
//26/01/2010	Rémi Saint-Amant	New equation 
//14/09/2011	Rémi Saint-Amant	Update paramater with paper "Mapping landscap-scale voltinism of spruce beetle"
//*********************************************************************
#include "SpruceBeetle.h"
#include "Weather.h"
#include "UtilMath.h"

using namespace CFL;

//default parameter
//this parameter is change by BioSIM when the model is call
//double CSpruceBeetle::INTERCEPT=-4.4034;
//double CSpruceBeetle::SLOPE=0.01192;
//double CSpruceBeetle::NORTH_EFFECT=-0.8716;
//double CSpruceBeetle::HIGT_EFFECT=0.8943;
//double CSpruceBeetle::LOW_EFFECT=-1.5647;
const double CSpruceBeetle::INTERCEPT=-4.5273;
const double CSpruceBeetle::SLOPE=0.01128;
const double CSpruceBeetle::NORTH_EFFECT=-0.8714;
const double CSpruceBeetle::HIGT_EFFECT=0.8954;
const double CSpruceBeetle::LOW_EFFECT=-1.5644;


CSpruceBeetle::CSpruceBeetle()
{
	m_pri15=0;
	m_day15=0;
	m_peak=0;
	m_Hr17=0;
	m_propTree=0;

}

CSpruceBeetle::~CSpruceBeetle()
{
}

int CSpruceBeetle::GetPri15(const CWeatherYear& weatherYear)
{
	//Get the number of day when the max temp is over 15°C
	short nbDay = 0;
	for(int jd=0; jd<weatherYear.GetNbDay(); jd++)
	{
		if( weatherYear.GetDay(jd).GetTMax() >= 15)
			nbDay++;
	}

	return nbDay;
}

//compute peak emergence day
int CSpruceBeetle::GetFlightPeak(const CWeatherYear& weatherYear, int day15)
{
	//Get the day when the cumulative number of day (max temp is over 15°C) 
	//is equal to the day15 index

	short index = -1;
	short nbDay = 0;
	for(int jd=0; jd<weatherYear.GetNbDay(); jd++)
	{
		if( weatherYear.GetDay(jd).GetTMax() >= 15)
			nbDay++;

		if( nbDay == day15)
		{
			index = jd;
			break;
		}
	}

	return index;
}


int CSpruceBeetle::GetHr17(const CWeatherYear& weatherYear, int peakShifted)
{
	//Get number of hour over 17°C
	int Hr17=0;

	int fd = peakShifted;
	int ld = weatherYear.GetNbDay();
	for(int jd=fd; jd<ld; jd++)
	{
		const CWeatherDay& wDay = weatherYear.GetDay(jd);
		double Tmean = wDay.GetTMean();
		double Trange = wDay.GetTRange();
		for(int h=0; h<24; h++)
		{
			double cosH=cos(3.14159265*(h+8)/12); 
			double temp= Tmean + (Trange/2)*cosH;
			if( temp >= 17)
				Hr17++;
		}
	}

	return Hr17;
}

// return the Univoltine Brood Proportion
double CSpruceBeetle::GetUnivoltineBroodProportion(int Hr17, int mod)
{
	int N = mod&NORTH?1:0;	//1 if north, 0 otherwise
	int H = mod&HIGH?1:0;	//1 if height is 4.6m, 0 otherwise
	int L = mod&LOW?1:0;	//1 if height is ground level, 0 otherwise
		
	double logit = INTERCEPT + SLOPE*Hr17 + NORTH_EFFECT*N + HIGT_EFFECT*H + LOW_EFFECT*L;
	return  1/(1 + exp(-logit));
}

//??????????
//Compute day15 from equation [1] [2] and [3]: round up to integer
//m_day15 = int(ceil((0.4484 + 0.03408*m_pri15)/0.2057));
	

//This function calculates the proportion of 1 year spruce beetles given max min air temperatures.
bool CSpruceBeetle::Compute(const CWeatherYear& weatherYear1, const CWeatherYear& weatherYear2)
{

// 1- Calculate Pri15 ("m_pri15") from Year 1 temperature record.
//    pri15 is the number of days where Tmax is >= 15°C
	m_pri15 = GetPri15(weatherYear1);

// 2- Estimate the proportion (logit scale, "logitVi") of parent beetles on the univoltine cycle (Eq [2])
//	  Eq [2]: LogitVi = -8.442 + 0.06784 * Pri15 
	double logitVi = -8.442 + 0.06784*m_pri15;

// 3- Back-transform the logit into original scale (Eq [3], "parentVoltinism");
//	  Eq [3]: Voltinism = 1/(1 + exp(-LogitVi))
	double parentVoltinism = 1/(1+exp(-logitVi));

// 4- Solve Eq [1] for Logit = 0 using the "voltinism" value from above;
//	  Eq [1]: LogitEi = -2.5729 + 0.2116(Day15) - 2.9977(Voltinism)
//	  then, Day15 = [ 2.5729 + 2.9977(Voltinism) ]/0.2116;
	double day15tmp = ( 2.5729 + 2.9977*parentVoltinism)/0.2116;

// 5- Round-up to the nearest integer the solution to step 4 
	m_day15 = int(ceil(day15tmp));

// 6- Compute peak flight:	examine the temperature record for Year 2 for the condition matches the integer value.
	m_peak=GetFlightPeak(weatherYear2, m_day15);

	if( m_peak == -1)//if no peak flight is fount we retrun false
		return false;

// 7- Beginning 40 days after the estimated date of peak flight (from step 6), we count the cumulative hours above 17°C.
//    Because BioSIM uses daily maxima and minima, we must apply a sine-wave curve to these values.
	m_Hr17 = GetHr17(weatherYear2, m_peak+40);

// 8- We apply Eq [4] for the combinaison of aspect (NORTH, SOUTH) and height(LOW, MEDIUM, HIGHT)
//    Eq [4]: Logitij = -4.5273 + 0.01128(Hr17) - 0.8714(N) + 0.8954(H) - 1.5644(L)
//    NH = north bole aspect, 4.6 m
//    NM = north bole aspect, 1.8 m
//    NL = north bole aspect, ground
//    SH = south bole aspect, 4.6 m
//    SM = etc.

	double πNH = GetUnivoltineBroodProportion(m_Hr17, NORTH|HIGH);
	double πNM = GetUnivoltineBroodProportion(m_Hr17, NORTH|MEDIUM);
	double πNL = GetUnivoltineBroodProportion(m_Hr17, NORTH|LOW);
	double πSH = GetUnivoltineBroodProportion(m_Hr17, SOUTH|HIGH);
	double πSM = GetUnivoltineBroodProportion(m_Hr17, SOUTH|MEDIUM);
	double πSL = GetUnivoltineBroodProportion(m_Hr17, SOUTH|LOW);


// 9- Finally, Eq [5] is applied to derive tree or stand-level univoltine proportions.
//    Eq [5]: 0.34(πNH) + 0.34(πSH) + 0.11(πNM) + 0.11(πSM) + 0.05(πNL) + 0.05(πSL)
	m_propTree = 0.34*πNH + 0.34*πSH + 0.11*πNM + 0.11*πSM + 0.05*πNL + 0.05*πSL;


	return true;
}
