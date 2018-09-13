//**********************************************************************
//Regniere, J & B. Bentz. 2007. Modelling cold tolerance in the mountain pine beetle, Dendroctonus ponderosae. 
//Modification: 
//11/05/2016		Rémi Saint-Amant    New compilation with WBSF
//27/03/2013		Rémi Saint-Amant    New compilation
//20/03/2007		Jacques Regniere	Conformed to published document 
//07/02/2007		Jacques Regniere	Journal of Insect Physiology doi: 10.1016/j.jinsphys.
//01/01/2005		JR, RSA				Creation
//**********************************************************************
#include "MPBColdTolerance.h"
#include "Basic/WeatherStation.h"
#include "Basic/UtilMath.h"

using namespace std;
using namespace WBSF::HOURLY_DATA;

namespace WBSF
{

	//First day of cold-tolerance accumulation: Aug 1. (date, base 0) 
	static const int T_0 = 212; //This is t sub 0 in Equation [7]
	static const int T_1 = 364; //This is t sub 1 in Equation [7]

	CMPBColdTolerance::CMPBColdTolerance()
	{
		//	Model parameters
		m_RhoG = 0.311;
		m_MuG = -5;
		m_SigmaG = 8.716;
		m_KappaG = -39.3;
		m_RhoL = 0.791;
		m_MuL = 33.9;
		m_SigmaL = 3.251;
		m_KappaL = -32.7;
		m_Lambda0 = 0.254;
		m_Lambda1 = 0.764;
		//	Generate phloem temperatures from air min/max
		m_bMicroClimate = true;
	}

	CMPBColdTolerance::~CMPBColdTolerance()
	{
	}

	void CMPBColdTolerance::ComputeAnnual(const CWeatherStation& weather)
	{
		_ASSERTE(weather.GetNbYears() > 1);

		m_firstDate = CTRef(weather.GetFirstYear());
		m_result.clear();
		m_result.resize(weather.GetNbYears());
		m_result.front().m_year = weather.GetFirstYear();

		double Psurv = 1;  //100% survival at the onset

		double yearTmin = 999;
		double yearPsurv = 1;


		double Ct = 0;  //Initial state of cold tolerance process
		bool reverse = false; //Flag indicating whether Ct can drop. Initially NO

		for (size_t y = 0; y < weather.GetNbYears(); y++) //There can be more than 1 year of weather input (actually there IS).
		{
			size_t firstDay = y == 0 ? T_0 : 0; //In first year model starts on startingDay. T_0 in equation [7]. Second year, Jan 1 (0)
			for (size_t d = firstDay; d < weather[y].GetNbDays(); d++) //Loop over days in weather year
			{
				const CWeatherDay& T = weather[y].GetDay(d); //This is the current day's min and max
				if (d >= T_1) reverse = true; //Ct can now drop because we are after Dec 31st. Equation [7]

				//New year reset when more than one full year is being simulated (multiple winters)
				if (y >= 1 && d == T_0)
				{
					Psurv = 1;
					Ct = 0;

					//New season: reversal of Ct not allowed 'till 31 December
					reverse = false;

					//annual output: Year, Tmin and Psurv
					m_result[y] = CMPBCTResult(weather[y].GetTRef().GetYear(), yearTmin, yearPsurv);

					//reset  Tmin and Psurv
					yearTmin = 999;
					yearPsurv = 1;
				}

				double G = 0; //Rate of synthesis
				double L = 0; //Rate of catalysis

				//Microclimate
				double Tmax = T[H_TMAX][MEAN];
				double Tmin = T[H_TMIN][MEAN];

				//If phloem temperature is being generated from air temperature
				if (m_bMicroClimate)
				{

					double Trange = Tmax - Tmin;//T[H_TRNG][MEAN];
					double Sin = sin(2 * 3.14159*(T.GetTRef().GetJDay() / 365. - 0.25));

					//convert air temperature to bark temperature
					Tmin = -0.1493 + 0.8359*Tmin + 0.5417*Sin + 0.16980*Trange + 0.00000*Tmin*Sin + 0.005741*Tmin*Trange + 0.02370*Sin*Trange;
					Tmax = 0.4196 + 0.9372*Tmax - 0.4265*Sin + 0.05171*Trange + 0.03125*Tmax*Sin - 0.004270*Tmax*Trange + 0.09888*Sin*Trange;


					//Tmax=T.GetTMax()+3.25*((T.GetTMax()-T.GetTMin())/24.4); //Equation [11] (max overheating)
					//Tmin=T.GetTMin()+1.8; //Equation [12] (min damping)

					//BUG, here sometime Tmin is greater than Tmax, RSA 29/07/2012
					if (Tmin > Tmax)
						Switch(Tmin, Tmax);
				}

				double deltaT = (Tmax - Tmin); //Daily temp range phloem temp
				double Tmean = (Tmax + Tmin) / 2.; //Daily mean phloem temp

				double TG = m_MuG + m_KappaG*Ct; //Equation [5]
				double xG = exp(-(Tmean - TG) / m_SigmaG); //fragment of equation [3]
				G = deltaT*m_RhoG*xG / (m_SigmaG*Square(1 + xG)); //Equation [3]

				if (Ct > 0.5 || reverse)
				{
					double TL = m_MuL + m_KappaL*Ct; //Equation [6]
					double xL = exp(-(Tmean - TL) / m_SigmaL); //fragment of equation [4]
					L = deltaT*m_RhoL*xL / (m_SigmaL*Square(1 + xL)); //Equation [4]
				}

				Ct = Ct + (1 - Ct)*G - Ct*L; //Equation [7]

				if (Ct < 0) Ct = 0; //Numerical integration checking
				if (Ct > 1) Ct = 1; //Numerical integration checking

				double p1 = max(0., min(1., (.5 - Ct) / (.5 - m_Lambda0)));   //Equation [9]
				double p3 = max(0., min(1., (Ct - 0.5) / (m_Lambda1 - 0.5))); //Equation [9]
				double p2 = 1. - (p1 + p3);                           //Equation [9]
				double alpha[3] = { -9.8, -21.2, -32.3 }; //Parameters of equation [1]
				double beta[3] = { 2.26, 1.47, 2.42 };    //Parameters of equation [1]
				double LT50 = p1*alpha[0] + p2*alpha[1] + p3*alpha[2]; //Equation [8]

				double Pmort = 0; //Probability of mortality, initially 0

				//Part of equation [10] (in fact: 1 - survival)
				Pmort = 1 - (p1 / (1 + exp(-(Tmin - alpha[0]) / beta[0])) + p2 / (1 + exp(-(Tmin - alpha[1]) / beta[1])) + p3 / (1 + exp(-(Tmin - alpha[2]) / beta[2])));

				//Compute winter survival: Under the hypothesis that individual's ranks are NOT reshuffled 
				//(this cold mortality is a selective process: less tolerant individuals are killed first)
				Psurv = min(Psurv, 1. - Pmort); //Equation [10]

				yearTmin = min(yearTmin, Tmin);
				yearPsurv = min(yearPsurv, Psurv);
			}
		}
	}

	//Daily version of the model (outputs one line per day)
	void CMPBColdTolerance::ComputeDaily(const CWeatherStation& weather)
	{
		_ASSERTE(weather.GetNbYears() > 1);

		m_firstDate = weather.GetEntireTPeriod(CTM::DAILY).Begin();
		m_result.clear();
		m_result.resize(weather.GetNbDays());
		//Init value before T_0 with no data
		for (size_t d = 0; d < T_0; d++)
		{
			m_result[d].m_year = weather.GetFirstYear();
			m_result[d].m_day = d;
		}

		double Psurv = 1;  //100% survival at the onset

		double Ct = 0;  //Starting concentration of cryoprotectant
		bool reverse = false; //Flag indicating whether Ct can drop. Initially NO

		size_t  jd = T_0;
		for (size_t y = 0; y < weather.GetNbYears(); y++) //There can be more than 1 year of weather input (actually there IS).
		{
			size_t  firstDay = y == 0 ? T_0 : 0; //In first year model starts on startingDay. T_0 in equation [7]. Second year, Jan 1 (0)
			for (size_t d = firstDay; d < weather[y].GetNbDays(); d++) //Loop over days in weather year
			{
				const CWeatherDay& T = weather[y].GetDay(d); //This is the current day's min and max
				//Ct can drop below Lambda after Dec 31
				if (d >= T_1) reverse = true; //Ct can now drop because we are after Dec 31st. T_1 in Equation [7]

				//New year reset when more than one full year is being simulated (multiple winters)
				if (y >= 1 && d == T_0)
				{
					Psurv = 1;
					Ct = 0;

					//New season: reversal below Lambda impossible
					reverse = false;
				}

				double G = 0; //Rate of synthesis
				double L = 0; //Rate of catalysis

				//Microclimate
				double Tmax = T[H_TMAX][MEAN];
				double Tmin = T[H_TMIN][MEAN];

				//If phloem temperature is being generated from air temperature
				if (m_bMicroClimate)
				{

					double Trange = Tmax - Tmin;//T[H_TRNG][MEAN];
					double Sin = sin(2 * 3.14159*(T.GetTRef().GetJDay() / 365. - 0.25));

					//convert air temperature to bark temperature
					Tmin = -0.1493 + 0.8359*Tmin + 0.5417*Sin + 0.16980*Trange + 0.00000*Tmin*Sin + 0.005741*Tmin*Trange + 0.02370*Sin*Trange;
					Tmax = 0.4196 + 0.9372*Tmax - 0.4265*Sin + 0.05171*Trange + 0.03125*Tmax*Sin - 0.004270*Tmax*Trange + 0.09888*Sin*Trange;

					//BUG, here sometime Tmin is greater than Tmax, RSA 29/07/2012
					if (Tmin > Tmax)
						Switch(Tmin, Tmax);
				}


				double deltaT = (Tmax - Tmin); //Daily temp range phloem temp
				double Tmean = (Tmax + Tmin) / 2.; //Daily mean phloem temp

				double TG = m_MuG + m_KappaG*Ct; //Equation [5]
				double xG = exp(-(Tmean - TG) / m_SigmaG); //fragment of equation [3]
				G = deltaT*m_RhoG*xG / (m_SigmaG*Square(1 + xG)); //Equation [3]

				if (Ct > 0.5 || reverse)
				{
					double TL = m_MuL + m_KappaL*Ct; //Equation [6]
					double xL = exp(-(Tmean - TL) / m_SigmaL); //fragment of equation [4]
					L = deltaT*m_RhoL*xL / (m_SigmaL*Square(1 + xL)); //Equation [4]
				}

				Ct = Ct + (1 - Ct)*G - Ct*L; //Equation [7]

				if (Ct < 0) Ct = 0; //Numerical integration checking
				if (Ct > 1) Ct = 1; //Numerical integration checking

				double p1 = max(0., min(1., (.5 - Ct) / (.5 - m_Lambda0)));   //Equation [9]
				double p3 = max(0., min(1., (Ct - 0.5) / (m_Lambda1 - 0.5))); //Equation [9]
				double p2 = 1. - (p1 + p3);                           //Equation [9]
				const double alpha[3] = { -9.8, -21.2, -32.3 }; //Parameters of equation [1]
				const double beta[3] = { 2.26, 1.47, 2.42 };    //Parameters of equation [1]
				double LT50 = p1*alpha[0] + p2*alpha[1] + p3*alpha[2]; //Equation [8]

				double Pmort = 0; //Probability of mortality, initially 0

				//Part of equation [10] (in fact: 1 - survival)
				Pmort = 1 - (p1 / (1 + exp(-(Tmin - alpha[0]) / beta[0])) + p2 / (1 + exp(-(Tmin - alpha[1]) / beta[1])) + p3 / (1 + exp(-(Tmin - alpha[2]) / beta[2])));

				//Compute winter survival: Under the hypothesis that individual's ranks are NOT reshuffled 
				//(this cold mortality is a selective process: less tolerant individuals are killed first)
				Psurv = min(Psurv, 1. - Pmort); //Equation [10]

				m_result[jd] = (CMPBCTResult(weather[y].GetTRef().GetYear(), d, Tmin, Tmax, p1, p3, Ct, LT50, Psurv, Pmort));

				jd++;
			}//for days
		}//for years
	}

}