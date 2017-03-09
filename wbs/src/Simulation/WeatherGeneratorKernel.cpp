//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Jacques Régnière, Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//	
//******************************************************************************
// Abstract:    This class simulate daily air temperature, precipitation, humidity and wind speed from monthly normal
//
// See article : Régnière, J., St-Amant, R. 2007. Stochastic simulation of daily air 
//				 temperature and precipitation from monthly normals in Noth America north 
//				 of Mexico. Int J Biometeorol. 
//******************************************************************************
// 01/01/2016	Rémi Saint-Amant	Include into Weather-based simulation framework
// 01/12/2007	Jacques Régnière	Addition of new variable. DewPoint, Relitive Humidity and Wind Speed	
// 01/01/2007	Jacques Régnière	New kernel from article
// 04/03/2009	Rémi Saint-Amant	Correction of 2 bugs in the wind generation
// 10/02/2012	Rémi Saint-Amant	New random generator
// 26/07/2012   Rémi Saint-Amant	New wnd calculation. Remove correction of the mean.
//******************************************************************************
#include "stdafx.h"
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <crtdbg.h>
#include <float.h>

#include "Basic/UtilMath.h"
#include "Basic/UtilTime.h"
#include "Basic/WeatherStation.h"
#include "Simulation/WeatherGeneratorKernel.h"


using namespace WBSF::WEATHER;
using namespace WBSF::NORMALS_DATA;
using namespace WBSF::HOURLY_DATA;
using namespace std;

namespace WBSF
{

	//////////////////////////////////////////////////////////////////////
	// Construction/Destruction
	//////////////////////////////////////////////////////////////////////
	CWeatherGeneratorKernel::CWeatherGeneratorKernel(CWVariables variables)
	{
		m_variables = variables;

		ResetDeltaEpsilon();
	}

	CWeatherGeneratorKernel::~CWeatherGeneratorKernel()
	{

	}

	//****************************************************************************
	// Summary:		ResetDeltaEpsilon
	//
	// Description: Set Delta[3] and Epsilon[3] to zero
	//
	// Input:		
	//
	// Output:
	//
	// Note:		When DeltaEpsilon Is reset, these members will be init the next
	//				time the "Generate" method will be call
	//****************************************************************************
	void CWeatherGeneratorKernel::ResetDeltaEpsilon()
	{
		for (int i = 0; i < 3; i++)
		{
			m_delta[i] = 0;
			m_epsilon[i] = 0;
		}
	}

	//****************************************************************************
	// Summary:		Set the normal to the Kernel.
	//
	// Description: Set and adjust normal
	//
	// Input:		const CNormalsData& normals: the normals
	//
	// Output:
	//
	// Note:		We adjust monthly temperature to have better linear interpolation
	//****************************************************************************
	void CWeatherGeneratorKernel::SetNormals(const CNormalsData& normals)
	{
		m_normals = normals;
		m_normals.AdjustMonthlyMeans();
	}

	//****************************************************************************
	// Summary:		Generate
	//
	// Description: simulate daily weather for one year
	//
	// Input:      
	//
	// Output:		CDailyData& dailyData: daily output
	//
	// Note:		We used the normals to simulate daily
	//****************************************************************************
	void CWeatherGeneratorKernel::Generate(CWeatherYear& dailyData)
	{

		//      Daily Generator Loop
		//randomization for temperature
		m_rand.Randomize(m_seed[H_TAIR2]);

		//temperature part
		if (m_variables[H_TMIN2] || m_variables[H_TAIR2] || m_variables[H_TMAX2])
		{
			ASSERT(m_normals.GetVariables()[H_TMIN2]);
			ASSERT(m_normals.GetVariables()[H_TMAX2]);

			//If delta and epsilon isn't init, we init it
			if (!IsDeltaEpsilonInit())
				InitDeltaEpsilon();



			//int date = 0;
			for (size_t m = 0; m < 12; m++) //loop over 12 months
			{
				double e_min[31] = { 0 };
				double e_max[31] = { 0 };

				GetRandomTraces(e_min, e_max, m);

				for (size_t d = 0; d < GetNbDayPerMonth(m); d++)
				{
					//_ASSERTE(date>= 0 && date<365);

					//Get today's temperature
					double tmin = m_normals.Interpole(m, d, TMIN_MN) + e_min[d];
					double tmax = m_normals.Interpole(m, d, TMAX_MN) + e_max[d];
					ASSERT(tmin>-99 && tmin<99);
					ASSERT(tmax>-99 && tmax<99);

					if (tmin > tmax)
						Switch(tmin, tmax);

					_ASSERTE(tmin < tmax);
					//if (m_variables[H_TAIR2])
						dailyData[m][d][H_TMIN2] = tmin;

					//if (m_variables[H_TAIR2])
						dailyData[m][d][H_TAIR2] = (tmin + tmax) / 2;

					//if (m_variables[H_TMAX2])
						dailyData[m][d][H_TMAX2] = tmax;

					
				} // for all days in the month
			}//for all months
		}

		//randomization for precipitation
		m_rand.Randomize(m_seed[H_PRCP]);
		//precipitation part
		if (m_variables[H_PRCP])
		{
			ASSERT(m_normals.GetVariables()[H_PRCP]);
			//copy normals and make random ajustement for precipitation
			CNormalsData normalTmp = m_normals;
			RandomizeNormals(normalTmp);

			for (int m = 0; m < 12; m++)
			{
				//distribute month's precip 
				DailyPrecipitation(m, normalTmp, dailyData);
			}
		}

		//randomization for humidity
		m_rand.Randomize(m_seed[H_RELH]);

		//relative humidity part
		if (m_variables[H_TDEW] || m_variables[H_RELH])
		{
			//ASSERT(m_variables[H_RELH] && m_variables[H_TDEW]);
			ASSERT(m_normals.GetVariables()[H_RELH]);

			for (size_t m = 0; m < 12; m++)
			{
				double mean = m_normals[m][RELH_MN] / 100;
				double variance = Square(m_normals[m][RELH_SD] / 100);

				double alpha = mean    *((mean*(1 - mean) / variance) - 1);
				double beta = (1 - mean)*((mean*(1 - mean) / variance) - 1);

				for (size_t d = 0; d < GetNbDayPerMonth(m); d++)
				{
					_ASSERTE(dailyData[m][d][H_TMIN2].IsInit());
					_ASSERTE(dailyData[m][d][H_TMAX2].IsInit());

					//NOTE: old beta distribution have a biasis of 1%, the new don't, RSA 28/07/2012
					double Hr = m_rand.RandBeta(alpha, beta) * 100;
					ASSERT(Hr >= 0 && Hr <= 100);
					double Tmean = (dailyData[m][d][H_TMIN2][MEAN] + dailyData[m][d][H_TMAX2][MEAN]) / 2;


					dailyData[m][d][H_RELH] = Hr;
					dailyData[m][d][H_TDEW] = Hr2Td(Tmean, Hr);
				}
			}
		}

		//randomization for wind speed
		m_rand.Randomize(m_seed[H_WNDS]);
		int sw1 = 0;
		int sw2 = 0;
		//wind speed part
		if (m_variables[H_WNDS] || m_variables[H_WND2])
		{
			ASSERT(m_normals.GetVariables()[H_WNDS]);
			//int jd=0;
			for (int m = 0; m < 12; m++)
			{
				//WARNING : the mean and maximum of wind speed have been replaced, RSA 26/07/2012
				//calibrate on 140 station with wind speed (missing less then 20 days) in Canada from 1981-2010
				double windSpeedMean = m_normals[m][WNDS_MN];
				double windSpeedSD = m_normals[m][WNDS_SD];
				double dailyPpt = max(0.1f, m_normals[m][PRCP_TT]) / GetNbDayPerMonth(m);
				double WSMax = exp(1.43319 + 0.74865*windSpeedMean + 0.71983*windSpeedSD);//*1.25;

				for (size_t d = 0; d < GetNbDayPerMonth(m); d++)
				{
					ASSERT(dailyData[m][d][H_PRCP][SUM] > MISSING);

					double Rp = dailyData[m][d][H_PRCP][SUM] / dailyPpt;

					static const double A = 0.9597;
					static const double B = 0.1371;
					static const double C = 0.8853;

					//double WS = exp(m_rand.RandNormal(windSpeedMean, windSpeedSD))*(A + B*log(Rp + C));
					double WS = m_rand.RandLogNormal(windSpeedMean, windSpeedSD)*(A + B*log(Rp + C));

					if (WS > WSMax)
					{
						while (WS<exp(windSpeedMean) || WS > WSMax)
							WS = m_rand.RandLogNormal(windSpeedMean, windSpeedSD)*(A + B*log(Rp + C));
							//WS = exp(m_rand.RandNormal(windSpeedMean, windSpeedSD))*(A + B*log(Rp + C));
					}

					if (WS < 0.1)
						WS = 0;

					if (m_variables[H_WNDS])
						dailyData[m][d][H_WNDS] = WS;

					//Wnd2 from WndS 
					if (m_variables[H_WND2])
						dailyData[m][d][H_WND2] = WS*4.87 / log(67.8 * 10 - 5.42);//wind speed at 2 meters
				}
			}
		}

		//set the day 365 for leap year : by RSA
		if (dailyData.GetTRef().IsLeap())
			dailyData[FEBRUARY][DAY_29] = dailyData[FEBRUARY][DAY_28];
	}


	//****************************************************************************
	// Summary:		RandomizeNormals
	//
	// Description: Randomize only precipitation in normals to have year variation
	//
	// Input:		CNormalsData& normalTmp: the normal to randomize
	//
	// Output:		CNormalsData& normalTmp: randomized normals
	//
	// Note:		 
	//****************************************************************************
	void CWeatherGeneratorKernel::RandomizeNormals(CNormalsData& normalTmp)
	{

		for (int i = 0; i < 12; i++)
		{
			// Total monthly precipitation
			//Weibull Nu and Lambda, in p = 1-exp(-((Weibull/Lambda)^Nu)) Régnière (2007) Equ. [11]
			//which inverted gives
			//Weibull = Lambda*(-ln((1-p)^(1/Nu))...

			double Weibull = 1;
			//new equations and parameters for high sp values (29/10/2006)
			double Nu = 0.9277 / normalTmp[i][PRCP_SD] + 0.0752 / (normalTmp[i][PRCP_SD] * normalTmp[i][PRCP_SD]); //Régnière (2007) Equ. [12]
			double Lambda = 1.1174* pow(1 - exp(-4.4107*Nu), 7.5088); //Régnière (2007) Equ. [13]

			//Random number in [0,1[
			//double p_rand=Randv();
			double p_rand = m_rand.Randv();
			//Compute a Weibull-distributed variate.
			Weibull = min(25.0, max(0.0, (pow(-log(1.0 - p_rand), 1.0 / Nu)*Lambda)));

			normalTmp[i][PRCP_TT] *= float(Weibull);
		}
	}

	//****************************************************************************
	// Summary:		InitDeltaEpsilon
	//
	// Description: Initialize dalta and epsilon to have a starting point of temperature variation
	//
	// Input:		
	//
	// Output:		
	//
	// Note:		m_delta[3] and m_epsilon[3] are init
	//****************************************************************************
	void CWeatherGeneratorKernel::InitDeltaEpsilon()
	{
		ResetDeltaEpsilon();


		size_t month = DECEMBER;
		const double sigma_delta = m_normals[month][DEL_STD];
		const double sigma_epsilon = m_normals[month][EPS_STD];
		const double A1 = m_normals[month][TACF_A1];
		const double A2 = m_normals[month][TACF_A2];
		const double B1 = m_normals[month][TACF_B1];
		const double B2 = m_normals[month][TACF_B2];

		double sigma_gamma = 0;
		double sigma_zeta = 0;
		double P = m_normals[month].GetP(sigma_gamma, sigma_zeta);
		ASSERT(!_isnan(sigma_gamma));
		ASSERT(!_isnan(sigma_zeta));
		//Régnière (2007) Equ [3] in standard 2nd order autoregressive process formulation
		//Initialize time series 
		for (size_t i = 0; i < 50; i++)
		{
			double gamma = m_rand.RandNormal(0, sigma_gamma);
			double zeta = m_rand.RandNormal(0, sigma_zeta);

			m_delta[2] = A1*m_delta[1] + A2*m_delta[0] + gamma;
			m_epsilon[2] = (B1*m_epsilon[1] + B2*m_epsilon[0] + sigma_epsilon / sigma_delta * (P*gamma + (1 - abs(P))*zeta));

			//Shift time series
			m_delta[0] = m_delta[1];
			m_delta[1] = m_delta[2];
			m_epsilon[0] = m_epsilon[1];
			m_epsilon[1] = m_epsilon[2];
		}

	}

	//****************************************************************************
	// Summary:		GetRandomTraces
	//
	// Description: Get a random minimum and maximum series of temperature for a month
	//
	// Input:		int month
	//
	// Output:		double e_min[31], double e_max[31]
	//
	// Note:		Used normal
	//****************************************************************************
	void CWeatherGeneratorKernel::GetRandomTraces(double e_min[31], double e_max[31], size_t month)
	{
		const double sigma_delta = m_normals[month][DEL_STD];
		const double sigma_epsilon = m_normals[month][EPS_STD];
		const double A1 = m_normals[month][TACF_A1];
		const double A2 = m_normals[month][TACF_A2];
		const double B1 = m_normals[month][TACF_B1];
		const double B2 = m_normals[month][TACF_B2];

		double sigma_gamma = 0;
		double sigma_zeta = 0;
		double P = m_normals[month].GetP(sigma_gamma, sigma_zeta);
		ASSERT(!_isnan(sigma_zeta));
		//Régnière (2007) Equ [3] in standard 2nd order autoregressive process formulation
		//Initialize time series 

		double LimitTmin = 3.9*sigma_delta;
		double LimitTmax = 3.9*sigma_epsilon;
		for (size_t j = 0; j < GetNbDayPerMonth(month); j++)
		{
			//double gamma = RandNormal(0, sigma_gamma);
			//double zeta  = RandNormal(0, sigma_zeta);
			double gamma = m_rand.RandNormal(0, sigma_gamma);
			double zeta = m_rand.RandNormal(0, sigma_zeta);

			//Régnière (2007) Equ [3] in standard 2nd order autoregressive process formulation
			m_delta[2] = A1*m_delta[1] + A2*m_delta[0] + gamma;
			m_epsilon[2] = (B1*m_epsilon[1] + B2*m_epsilon[0] + sigma_epsilon / sigma_delta * (P*gamma + (1 - abs(P))*zeta));

			//store deviations
			e_min[j] = min(LimitTmin, max(-LimitTmin, m_delta[2]));
			e_max[j] = min(LimitTmax, max(-LimitTmax, m_epsilon[2]));

			//Shift time series
			m_delta[0] = m_delta[1];
			m_delta[1] = m_delta[2];
			m_epsilon[0] = m_epsilon[1];
			m_epsilon[1] = m_epsilon[2];
		}

		//now we can limit more
		LimitTmin = 2.33*sigma_delta;
		LimitTmax = 1.96*sigma_epsilon;
		for (size_t j = 0; j < GetNbDayPerMonth(month); j++)
		{
			e_min[j] = min(LimitTmin, max(-LimitTmin, e_min[j]));
			e_max[j] = min(LimitTmax, max(-LimitTmax, e_max[j]));
		}

	}

	//****************************************************************************
	// Summary:		DailyPrecipitation
	//
	// Description: Get a random series of precipitation for a month
	//
	// Input:		int month, const CNormalsData& normalTmp, 
	//
	// Output:		CDailyData& dailyData
	//
	// Note:		We used normalTmp to generate precipitation
	//****************************************************************************
	void CWeatherGeneratorKernel::DailyPrecipitation(int m, const CNormalsData& normalTmp, CWeatherYear& dailyData)
	{
		//JR JAN 2001 Obtain the month's random numbers, and make sure temperature traces add-up to 0
		double p_test[31] = { 0 };
		double epsilon_prec[31] = { 0 };
		InitRandomNumber(m, p_test, epsilon_prec);


		//		Zero-initialize daily precip for the month
		for (size_t d = 0; d < GetNbDayPerMonth(m); d++)
			dailyData[m][d][H_PRCP] = 0;

		//Only in months with non-trace rainfall
		if (normalTmp[m][PRCP_TT] > 0.1)
		{
			//	determine Sp
			double Sp = normalTmp[m][PRCP_SD];

			// variable to know the day when the probability is maximum
			double maxPprecip = 0;
			size_t dayMaxPprecip = 0;

			// vector of weights of probability
			double Sum_x = 0;
			std::vector<double> weightOfPpt;
			weightOfPpt.resize(GetNbDayPerMonth(m));

			for (size_t d = 0; d < GetNbDayPerMonth(m); d++)
			{
				ASSERT(dailyData[m][d][H_TMIN2].IsInit());
				ASSERT(dailyData[m][d][H_TMAX2].IsInit());

				//		Get daily prob of precip given min, max, tot_precip of the month and the Sp
				double Pprecip = m_AP.GetPprecip(dailyData[m][d][H_TMIN2][MEAN], dailyData[m][d][H_TMAX2][MEAN], normalTmp[m][PRCP_TT], Sp);

				//Find the day when the probability of rain is the highest
				if (Pprecip > maxPprecip)
				{
					maxPprecip = Pprecip;
					dayMaxPprecip = d;
				}

				//test to know if there is precipitation
				//p_test is a [0,1] U-random number
				if (p_test[d] > Pprecip)
				{
					Pprecip = 0;
				}

				//if they is a probability of precipitation,
				//transform probability of precipitation to a weight of precipitation. 
				//epsilon_prec[j] is a [0, 1] U-random number
				if (Pprecip > 0)
				{
					weightOfPpt[d] = m_AP.XfromP(Sp, epsilon_prec[d]);
					Sum_x += weightOfPpt[d];
				}
			}

			//if we have rain
			if (Sum_x > 0)
			{
				//compute precipitation
				for (size_t d = 0; d < GetNbDayPerMonth(m); d++)
				{
					//int date = d0 + j;

					dailyData[m][d][H_PRCP] = float(weightOfPpt[d] / Sum_x * normalTmp[m][PRCP_TT]);

					//eliminate trace
					if (dailyData[m][d][H_PRCP][SUM] < 0.1f)
						dailyData[m][d][H_PRCP] = 0.0f;
				}
			}
			else
			{
				//if they is no precipitation (Sum_x=0), we force precipitation on day of max likelihood 
				dailyData[m][dayMaxPprecip][H_PRCP] = normalTmp[m][PRCP_TT];
			}
		}
	}


	//returns a float on the interval [0,1[
	//double CWeatherGeneratorKernel::Randv(void) 
	//{
	//	return Randu(false, true);
	//}


	// JR JAN 2001 Set new variables, especially: store month's random trace
	void CWeatherGeneratorKernel::InitRandomNumber(int m, double p_test[31], double epsilon_prec[31])
	{
		for (size_t d = 0; d < GetNbDayPerMonth(m); d++)
		{
			//p_test[d]=Randu();
			//epsilon_prec[d]=Randu();
			p_test[d] = m_rand.Randu();
			epsilon_prec[d] = m_rand.Randu();
		}
	}


}