﻿//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//	
//******************************************************************************
// 01-01-2020	Rémi Saint-Amant	Include into Weather-based simulation framework
//******************************************************************************
#include "stdafx.h"
#include <math.h>
#include <sstream>
#include <algorithm>
#include <random>

#include <memory>
#include <boost/math/distributions/normal.hpp>
#include <boost/math/distributions/lognormal.hpp>
#include <boost/math/distributions/poisson.hpp>
#include <boost/algorithm/string.hpp>
#include <cmath>
#include "basic/xml/zen/stl_tools.h"

#include "Basic/OpenMP.h"
#include "Basic/UtilMath.h"
#include "Basic/ModelStat.h"
#include "Basic/CSV.h"
#include "Basic/UtilStd.h"
#include "FileManager/FileManager.h"
#include "ModelBase/CommunicationStream.h"
#include "ModelBase/WGInput-ModelInput.h"
#include "Simulation/ExecutableFactory.h"
#include "Simulation/WeatherGenerator.h"
#include "Simulation/LoadStaticData.h"
#include "Simulation/InsectParameterization.h"

#include "WeatherBasedSimulationString.h"



using namespace std;
using namespace boost;
using namespace WBSF::WEATHER;
using namespace WBSF::DIMENSION;
using namespace WBSF::DevRateInput;


namespace WBSF
{

	static const double SIGMA_FACTOR = 5.0;
	static const double Fo_FACTOR = 5.0;

	static const double NULL_RATE_THRESHOLD = 1.0 / 1000.0;
	static const double LOWER_RATE_THRESHOLD = 0.0;
	static const double NA_TIME = 1000;//replace Time by 1000 days when NA
	//**********************************************************************************************

	//double GetRateStat(TDevRateEquation  e, const vector<double>& X, const vector<double>& T, double obs_time)
	//{
	//	CStatisticEx stat;
	//	if (T.size() == 1)//optimization at constant temperature (rate is always the same)
	//	{
	//		double rate = max(0.0, CDevRateEquation::GetRate(e, X, T[0]));//daily rate
	//		if (!isfinite(rate) || isnan(rate))
	//			return NAN;

	//		stat += rate;//sum of hourly rate
	//	}
	//	else if (T.size() == 24)//optimization at constant 24 hour cycle temperature (rate is always the same)
	//	{
	//		double rate = 0;
	//		for (size_t h = 0; h < 24; h++)
	//			rate += max(0.0, CDevRateEquation::GetRate(e, X, T[h])) / 24.0;//sum of hourly rate

	//		if (!isfinite(rate) || isnan(rate))
	//			return NAN;
	//		//for (size_t t = 0; t < obs_time; t++)
	//		stat += (obs_time * rate);//sum of hourly rate
	//	}
	//	else
	//	{
	//		for (size_t t = 0; t < obs_time; t++)
	//		{
	//			for (size_t h = 0; h < 24; h++)
	//			{
	//				double rate = max(0.0, CDevRateEquation::GetRate(e, X, T[t * 24 + h]));//daily rate
	//				if (!isfinite(rate) || isnan(rate))
	//					return NAN;

	//				stat += rate;//sum of hourly rate
	//			}
	//		}
	//	}

	//	return stat;
	//}

	//return time (days) to complete stage
	//double GetTimeStat(TDevRateEquation  e, const vector<double>& X, const vector<double>& T)
	//{
	//	double time = 0;
	//	if (T.size() == 1)//optimization at constant temperature (rate is always the same)
	//	{
	//		double rate = CDevRateEquation::GetRate(e, X, T[0]);//daily rate
	//		if (!isfinite(rate) || isnan(rate))
	//			return NAN;

	//		if (rate > 0)
	//			time = ceil(1 / rate);//sum of hourly rate
	//		else
	//			time = 1000.0 * (1 - rate);//let a chance to converge
	//	}
	//	else if (T.size() == 24)//optimization at constant 24 hour cycle temperature (rate is always the same)
	//	{
	//		double rate = 0;
	//		for (size_t h = 0; h < 24; h++)
	//			rate += max(0.0, CDevRateEquation::GetRate(e, X, T[h])) / 24.0;//sum of hourly rate

	//		if (!isfinite(rate) || isnan(rate))
	//			return NAN;
	//		//for (size_t t = 0; t < obs_time; t++)
	//		//stat += (obs_time*rate);//sum of hourly rate

	//		if (rate > 0)
	//			time = ceil(1 / rate);//sum of hourly rate
	//		else
	//			time = 1000.0 * (1 - rate);//let a chance to converge
	//	}
	//	else
	//	{
	//		double sum_rate = 0;
	//		for (size_t h = 0; h < T.size() && sum_rate < 1.0; h++)
	//		{
	//			double rate = max(0.0, CDevRateEquation::GetRate(e, X, T[h])) / 24.0;//daily rate
	//			if (!isfinite(rate) || isnan(rate))
	//				return NAN;

	//			sum_rate += rate;//sum of hourly rate
	//			time += 1;
	//		}

	//		if (sum_rate >= 1)
	//			time = ceil(time / 24.0);
	//		else
	//			time = 1000 / max(0.001, sum_rate);
	//	}

	//	return time;
	//}

	double GetDevRate(TDevRateEquation  e, const vector<double>& X, const CDevRateDataRow& info)
	{
		CStatisticEx stat;
		if (info.m_type == T_CONSTANT)//optimization at constant temperature (rate is always the same)
		{
			double T = info.T()[0];
			double R = max(0.0, CDevRateEquation::GetRate(e, X, T));//daily rate
			if (!isfinite(R) || isnan(R))
				return NAN;

			stat += R;//sum of rate
		}
		else if (info.m_type == T_TRANSFER)
		{
			double T1 = info.T()[0];
			double T2 = info.T()[1];
			double R1 = max(0.0, CDevRateEquation::GetRate(e, X, T1));//daily rate
			double R2 = max(0.0, CDevRateEquation::GetRate(e, X, T2));//daily rate

			if (isnan(R1) || isnan(R2))
				return NAN;

			assert(false);
			double t1 = info.t1();
			double t2 = info.t2();
			for (size_t h = 0; h < t1 * 24; h++)
				stat += R1;//sum of rate

			for (size_t h = 0; h < t2 * 24; h++)
				stat += R2;//sum of rate

		}
		else if (info.m_type == T_SQUARE || info.m_type == T_TRIANGULAR ||
			info.m_type == T_SINUS || info.m_type == T_SINUS_MEAN)//optimization at constant 24 hour cycle temperature (rate is always the same)
		{
			const vector<double>& T = info.T();
			ASSERT(T.size() == 24);

			for (size_t h = 0; h < 24; h++)
			{
				double R = max(0.0, CDevRateEquation::GetRate(e, X, T[h]));
				if (isnan(R))
					return NAN;

				stat += R;//sum of rate
			}
		}
		else
		{
			double t = info.t();
			const vector<double>& T = info.T();
			ASSERT(T.size() <= 24 * t);


			for (size_t td = 0; td < ceil(t); td++)//for all days
			{
				for (size_t h = 0; h < 24; h++)//for all hours
				{
					double th = td + double(h) / 24.0;

					if (th < t)
					{

						double R = max(0.0, CDevRateEquation::GetRate(e, X, T[td * 24 + h]));//daily rate
						if (!isfinite(R) || isnan(R))
							return NAN;

						stat += R;//sum of hourly rate
					}
				}

			}
		}

		return stat;
	}

	double GetSurvival(TSurvivalEquation e, const vector<double>& X, const CDevRateDataRow& info)
	{
		if (info.at(I_MEAN_TIME) <= 0)//when time is NA, survival is 0
			return 0;

		//if (obs_time <= 0)
			//return 0;

		double S = 1;
		if (info.m_type == T_CONSTANT)
		{
			//daily survival
			double T = info.T()[0];
			double s = CSurvivalEquation::GetSurvival(e, X, T);

			if (isnan(s))
				return NAN;


			//multiplication of all daily survival
			double t = info.t();
			ASSERT(t > 0);
			S = pow(s, t);
		}
		else if (info.m_type == T_TRANSFER)
		{

			double T1 = info.T()[0];
			double T2 = info.T()[1];
			double s1 = CSurvivalEquation::GetSurvival(e, X, T1);
			double s2 = CSurvivalEquation::GetSurvival(e, X, T2);

			if (isnan(s1) || isnan(s2))
				return NAN;

			//	s1 = max(0.0, min(1.0, s1));
				//s2 = max(0.0, min(1.0, s2));
				//multiplication of all daily survival
			double t1 = info.t1();
			double t2 = info.t2();
			ASSERT(t1 > 0);
			ASSERT(t2 > 0);
			S = pow(s1, t1) * pow(s2, t2);
		}
		else if (info.m_type == T_SQUARE || info.m_type == T_TRIANGULAR ||
			info.m_type == T_SINUS || info.m_type == T_SINUS_MEAN)//optimization at constant 24 hour cycle temperature (rate is always the same)
		{
			const vector<double>& T = info.T();
			ASSERT(T.size() == 24);

			double t = info.t();

			std::array<double, 24> s;
			for (size_t h = 0; h < 24; h++)
			{
				//daily survival
				double d_s = CSurvivalEquation::GetSurvival(e, X, T[h]);
				if (isnan(d_s))
					return NAN;

				//hourly survival
				s[h] = pow(d_s, 1.0 / 24.0);
				//multiplication of all hourly survival
				S *= pow(s[h], floor(t));
			}

			for (size_t h = 0; h < 24; h++)//for the last day
			{
				double th = floor(t) + double(h) / 24.0;

				if (th < t)
				{
					//multiplication of all hourly survival
					S *= pow(s[h], 1.0 / 24.0);
				}
			}

		}
		else
		{
			ASSERT(info.m_type == T_HOBO);
			const vector<double>& T = info.T();


			double t = info.t();
			ASSERT(t > 0);

			ASSERT(T.size() >= ceil(t));


			for (size_t td = 0; td < ceil(t); td++)//for all days
			{
				for (size_t h = 0; h < 24; h++)//for all hours
				{
					double th = td + double(h) / 24.0;

					if (th < t)
					{
						double d_s = CSurvivalEquation::GetSurvival(e, X, T[td * 24 + h]);

						if (isnan(d_s))
							return NAN;


						//hourly survival
						double s = pow(d_s, 1.0 / 24.0);
						//multiplication of all hourly survival
						S *= s;
					}
				}
			}
		}

		return S;
	}

	double GetFecundity(TDevRateEquation e, const vector<double>& X, const vector<double>& T, double tiˉ¹, double ti, double qi)
	{
		double to = 0;// X[X.size() - 3];
		double Fo = X[X.size() - 2];
		double sigma_f = X[X.size() - 1];

		boost::math::lognormal_distribution<double> LogNormal(log(Fo) - 0.5 * Square(sigma_f), sigma_f);
		double Fi = quantile(LogNormal, min(0.999, qi));


		ASSERT(tiˉ¹ - to >= 0);

		if (tiˉ¹ - to < 0)//when time is NA, oviposition is 0
			return 0;

		CStatistic stat_rate;
		double Ft = 0;//remaining fecundity at day t
		double Ftˉ¹ = 0;//remaining fecundity at day t-1
		if (T.size() == 1)//optimization at constant temperature (rate is always the same)
		{
			double lambda = max(0.0, CDevRateEquation::GetRate(e, X, T[0]));//remaining eggs

			if (isnan(lambda))
				return NAN;

			Ft = Fi * exp(-lambda * (ti - to));
			Ftˉ¹ = Fi * exp(-lambda * (tiˉ¹ - to));

		}
		else if (T.size() == 24)//optimization at constant 24 hour cycle temperature (rate is always the same)
		{

		}
		else
		{
			//Ft = sigma * Fo;
			//Ftˉ¹ = sigma * Fo;

			//for (size_t t = t0; t < ti; t++)
			//{
			//	for (size_t h = 0; h < 24; h++)
			//	{
			//		ASSERT(t * 24 + h < T.size());
			//		double lambda = max(0.0, CDevRateEquation::GetRate(e, computation.m_XP, T[t * 24 + h])) / 24.0;//hourly rate
			//		if (!isfinite(lambda) || isnan(lambda))
			//			return NAN;

			//		//xi += rate;//sum of hourly rate
			//			//				if (t < (ti - 1))
			//				//				xiˉ¹ += rate;
			//							//Ft = sigma * exp(-lambda * ((ti - t0) * 24));

			//							//if (t < (ti - 1))
			//								//Ftˉ¹ = sigma * exp(-lambda * ((ti - 1 - t0) * 24));
			//		Ft -= Ft * lambda;

			//		if (t < (ti - 1))
			//			Ftˉ¹ -= Ft * lambda;
			//	}
			//}
		}

		return Ftˉ¹ - Ft;
	}

	//double Regniere2021DevRate(TDevRateEquation  e, double sigma, const vector<double>& X, const vector<double>& T, double ti_1, double ti)
	//{
	//	ASSERT(sigma > 0);


	//	double xi = 0;
	//	double xiˉ¹ = 0;
	//	if (T.size() == 1)//optimization at constant temperature (rate is always the same)
	//	{
	//		//xi = integral (daily summation) of all daily rate at temperature at day ti
	//		//xiˉ¹ = integral (daily summation) of all daily rate at daily temperature at day ti-1

	//		double rate = max(0.0, CDevRateEquation::GetRate(e, X, T[0]));//daily rate
	//		if (!isfinite(rate) || isnan(rate))
	//			return NAN;

	//		xi = rate * ti;//sum of daily rate
	//		xiˉ¹ = rate * ti_1;
	//	}
	//	else if (T.size() == 24)//optimization at constant 24 hour cycle temperature (rate is always the same)
	//	{
	//		//xi = integral (hourly summation) of all hourly rate at hourly temperature at day ti
	//		//xiˉ¹ = integral (hourly summation) of all hourly rate at hourly temperature at day ti-1

	//		double rate = 0;
	//		for (size_t h = 0; h < 24; h++)
	//			rate += max(0.0, CDevRateEquation::GetRate(e, X, T[h])) / 24.0;//hourly rate;

	//		if (!isfinite(rate) || isnan(rate))
	//			return NAN;


	//		xi = rate * ti;//sum of daily rate
	//		xiˉ¹ = rate * ti_1;
	//	}
	//	else
	//	{
	//		//xi = integral (hourly summation) of all hourly rate at hourly temperature at day ti
	//		//xiˉ¹ = integral (hourly summation) of all hourly rate at hourly temperature at day ti-1
	//		for (size_t t = 0; t < ti; t++)
	//		{
	//			for (size_t h = 0; h < 24; h++)
	//			{
	//				ASSERT(t * 24 + h < T.size());
	//				double rate = max(0.0, CDevRateEquation::GetRate(e, X, T[t * 24 + h])) / 24.0;//hourly rate
	//				if (!isfinite(rate) || isnan(rate))
	//					return NAN;

	//				xi += rate;//sum of hourly rate
	//				if (t < ti_1)
	//					xiˉ¹ += rate;
	//			}
	//		}
	//	}

	//	//avoid division by zero
	//	xi = max(1E-20, xi);
	//	xiˉ¹ = max(1E-20, xiˉ¹);

	//	//compute probability of changing stage between ti-1 and ti
	//	boost::math::lognormal_distribution<double> LogNormal(-0.5*Square(sigma), sigma);
	//	double p = max(1e-200, cdf(LogNormal, 1.0 / xiˉ¹) - cdf(LogNormal, 1.0 / xi));
	//	//double p = max(1e-200, cdf(LogNormal, xi) - cdf(LogNormal, xiˉ¹));

	//	return log(p);
	//}

	//double Regniere2021DevRate(TDevRateEquation  e, double sigma, const vector<double>& X, const CDevRateDataRow& info)
	//{
	//	ASSERT(sigma > 0);
	//
	//	//xi = integral (daily summation) of all daily rate at temperature at day ti
	//	//xiˉ¹ = integral (daily summation) of all daily rate at daily temperature at day ti-1
	//	double xi = 0;
	//	double xiˉ¹ = 0;
	//
	//	if (info.m_type == T_CONSTANT)
	//	{
	//
	//		double T = info.T()[0];
	//		double rateD = max(LOWER_RATE_THRESHOLD, CDevRateEquation::GetRate(e, X, T));//daily rate
	//
	//		if (isinf(rateD) || isnan(rateD))
	//			return NAN;
	//
	//
	//		double t = info.t();
	//		double tˉ¹ = info.t(true);
	//
	//		//WARNING: do not remove the strange formulations of double division
	//		//because in this way, the inf and -inf is correctly managed
	//		//xi = t / (1.0 / rateD);
	//		//xiˉ¹ = tˉ¹ / (1.0 / rateD);
	//
	//		xi = t * rateD;
	//		xiˉ¹ = tˉ¹ * rateD;
	//	}
	//	else if (info.m_type == T_TRANSFER)
	//	{
	//		double T1 = info.T1();
	//		double T2 = info.T2();
	//
	//		double rateD1 = max(LOWER_RATE_THRESHOLD, CDevRateEquation::GetRate(e, X, T1));//daily rate
	//		double rateD2 = max(LOWER_RATE_THRESHOLD, CDevRateEquation::GetRate(e, X, T2));//daily rate
	//
	//		if (isinf(rateD1) || isinf(rateD2) || isnan(rateD1) || isnan(rateD2))
	//			return NAN;
	//
	//
	//		double t1 = info.t1();
	//		double t2 = info.t2();
	//		double t2ˉ¹ = info.t2(true);
	//		ASSERT(t1 >= 0);
	//
	//		//WARNING: do not remove the strange formulations of double division
	//		//because in this way, the inf and -inf is correctly managed
	//		//xi = t1 / (1.0 / rateD1) + t2 / (1.0 / rateD2);
	//		//xiˉ¹ = t1 / (1.0 / rateD1) + t2ˉ¹ / (1.0 / rateD2);
	//		xi = t1 * rateD1 + t2 * rateD2;
	//		xiˉ¹ = t1 * rateD1 + t2ˉ¹ * rateD2;
	//	}
	//	else if (info.m_type == T_SQUARE || info.m_type == T_SINUS || info.m_type == T_TRIANGULAR)//optimization at constant 24 hour cycle temperature (rate is always the same)
	//	{
	//		const vector<double>& T = info.T();
	//		ASSERT(T.size() == 24);
	//
	//		double mean_rateH = 0;
	//		double rateH24[24] = { 0 };
	//		for (size_t h = 0; h < 24; h++)
	//		{
	//			double rateH = max(LOWER_RATE_THRESHOLD, CDevRateEquation::GetRate(e, X, T[h])) / 24.0;//hourly rate
	//
	//			if (isinf(rateH) || isnan(rateH))
	//				return NAN;
	//
	//			rateH24[h] = rateH;
	//			mean_rateH += rateH;
	//		}
	//
	//		//there is probably a way to optimized that
	//		double t = info.t();
	//		double tˉ¹ = info.t(true);
	//
	//		//WARNING: do not remove the strange formulations of double division
	//		//because in this way, the inf and -inf is correctly managed
	//		//xi = floor(tˉ¹) / (1.0 / mean_rateH);//set up to the previous observation
	//		//xiˉ¹ = floor(tˉ¹) / (1.0 / mean_rateH);//set up to the previous observation
	//		xi = floor(tˉ¹) * mean_rateH;//set up to the previous observation
	//		xiˉ¹ = floor(tˉ¹) * mean_rateH;//set up to the previous observation
	//
	//		for (size_t td = size_t(floor(tˉ¹)); td < size_t(ceil(t)); td++)//Add the latest observation
	//		//for (size_t t = 0; t < ceil(t); t++)
	//		{
	//			for (size_t h = 0; h < 24; h++)
	//			{
	//				double th = td + double(h) / 24.0;
	//
	//				//WARNING: do not remove the strange formulations of double division
	//				//because in this way, the inf and -inf is correctly managed
	//				if (th < t)
	//					xi += rateH24[h];
	//
	//				if (th < tˉ¹)
	//					xiˉ¹ += rateH24[h];
	//			}
	//		}
	//	}
	//	else
	//	{
	//		ASSERT(info.m_type == T_HOBO);
	//		const vector<double>& T = info.T();
	//
	//
	//		double t = info.t();
	//		double tˉ¹ = info.t(true); //max(0.0, t - 1);
	//
	//		ASSERT(T.size() >= ceil(t));
	//		for (size_t td = 0; td < ceil(t); td++)
	//		{
	//			for (size_t h = 0; h < 24; h++)
	//			{
	//				double th = td + double(h) / 24.0;
	//
	//				ASSERT(t * 24 + h < T.size());
	//				double rateH = max(LOWER_RATE_THRESHOLD, CDevRateEquation::GetRate(e, X, T[td * 24 + h])) / 24.0;//hourly rate
	//
	//				if (isinf(rateH) || isnan(rateH))
	//					return NAN;
	//
	//				//WARNING: do not remove the strange formulations of double division
	//				//because in this way, the inf and -inf is correctly managed
	//				if (th < t)
	//					xi += rateH;
	//				//xi += 1.0 / (1.0 / rateH);
	//
	//				if (th < tˉ¹)
	//					xiˉ¹ += rateH;
	//				//xiˉ¹ += 1.0 / (1.0 / rateH);
	//			}
	//		}
	//	}
	//
	//	//compute probability of changing stage between ti-1 and ti
	//	boost::math::lognormal_distribution<double> LogNormal(-0.5 * Square(sigma), sigma);
	//	double p = (isfinite(xi) && isfinite(xiˉ¹)) ? cdf(LogNormal, xi) - cdf(LogNormal, xiˉ¹) : DBL_MIN;
	//
	//
	//	return log(max(DBL_MIN, p));//max: avoid log of 0
	//}

	bool Regniere2021DevRate(TDevRateEquation  e, const vector<double>& X, const CDevRateDataRow& info, map<double, double>& t_xi)
	{

		const vector<double>& T = info.T();

		if (info.m_type == T_CONSTANT)
		{

			double rateD = max(LOWER_RATE_THRESHOLD, CDevRateEquation::GetRate(e, X, T[0]));//daily rate

			if (isinf(rateD) || isnan(rateD))
				return false;

			for (auto it = t_xi.begin(); it != t_xi.end(); it++)
			{
				it->second = it->first * rateD;
			}
		}
		else if (info.m_type == T_TRANSFER)
		{
			double T1 = T[0];
			double T2 = T[1];

			double rateD1 = max(LOWER_RATE_THRESHOLD, CDevRateEquation::GetRate(e, X, T1));//daily rate
			double rateD2 = max(LOWER_RATE_THRESHOLD, CDevRateEquation::GetRate(e, X, T2));//daily rate

			if (isinf(rateD1) || isinf(rateD2) || isnan(rateD1) || isnan(rateD2))
				return false;


			double t1 = info.t1();
			//double t2 = info.t2();
			//double t2ˉ¹ = info.t2(true);
			ASSERT(t1 >= 0);


			//xiˉ¹ = t1 * rateD1 + t2ˉ¹ * rateD2;

			for (auto it = t_xi.begin(); it != t_xi.end(); it++)
			{
				ASSERT(it->first >= t1);
				it->second = t1 * rateD1 + (it->first - t1) * rateD2;
			}


		}
		else if (info.m_type == T_SQUARE || info.m_type == T_TRIANGULAR ||
			info.m_type == T_SINUS || info.m_type == T_SINUS_MEAN)//optimization at constant 24 hour cycle temperature (rate is always the same)
		{
			ASSERT(T.size() == 24);

			double mean_rateD = 0;
			double rateH24[24] = { 0 };
			for (size_t h = 0; h < 24; h++)
			{
				double rateH = max(LOWER_RATE_THRESHOLD, CDevRateEquation::GetRate(e, X, T[h])) / 24.0;//hourly rate

				if (isinf(rateH) || isnan(rateH))
					return false;

				rateH24[h] = rateH;
				mean_rateD += rateH;
			}

			//there is probably a way to optimized that
			//double t = info.t();
			//double tˉ¹ = info.t(true);



			for (auto it = t_xi.begin(); it != t_xi.end(); it++)
			{
				it->second = floor(it->first) * mean_rateD;

				for (size_t h = 0; h < 24; h++)
				{
					double th = floor(it->first) + double(h) / 24.0;

					if (th < it->first)
						it->second += rateH24[h];
				}
			}
		}
		else
		{
			double max_t = t_xi.rbegin()->first;

			ASSERT(T.size() >= ceil(max_t));
			for (size_t td = 0; td < ceil(max_t); td++)
			{
				for (size_t h = 0; h < 24; h++)
				{
					double th = td + double(h) / 24.0;

					ASSERT(td * 24 + h < T.size());
					double rateH = max(LOWER_RATE_THRESHOLD, CDevRateEquation::GetRate(e, X, T[td * 24 + h])) / 24.0;//hourly rate

					if (isinf(rateH) || isnan(rateH))
						return false;


					for (auto it = t_xi.begin(); it != t_xi.end(); it++)
					{
						if (th < it->first)
							it->second += rateH;
					}
				}
			}

		}

		return true;
	}



	double Regniere2021DevRateMeanSDn(TDevRateEquation  e, const vector<double>& X, const CDevRateDataRow& info)
	{
		CStatistic stat_rate;
		if (info.m_type == T_CONSTANT)
		{
			double T = info.T()[0];
			double rate = max(LOWER_RATE_THRESHOLD, CDevRateEquation::GetRate(e, X, T));//daily rate
			if (!isfinite(rate) || isnan(rate))
				return NAN;

			stat_rate = rate;//mean daily rate
		}
		else if (info.m_type == T_TRANSFER)
		{
			double T1 = info.T1();
			double T2 = info.T2();

			double rateD1 = max(LOWER_RATE_THRESHOLD, CDevRateEquation::GetRate(e, X, T1));//daily rate
			double rateD2 = max(LOWER_RATE_THRESHOLD, CDevRateEquation::GetRate(e, X, T2));//daily rate

			if (!isfinite(rateD1) || isnan(rateD1) || !isfinite(rateD2) || isnan(rateD2))
				return NAN;

			double t1 = info.t1();
			double t2 = info.t2();
			ASSERT(t1 >= 0);

			stat_rate = (t1 * rateD1 + t2 * rateD2) / (t1 + t2);

		}
		else if (info.m_type == T_SQUARE || info.m_type == T_TRIANGULAR ||
			info.m_type == T_SINUS || info.m_type == T_SINUS_MEAN)//optimization at constant 24 hour cycle temperature (rate is always the same)
		{
			const vector<double>& T = info.T();
			ASSERT(T.size() == 24);

			for (size_t h = 0; h < 24; h++)
			{
				double rate = max(LOWER_RATE_THRESHOLD, CDevRateEquation::GetRate(e, X, T[h]));//daily rate;
				if (!isfinite(rate) || isnan(rate))
					return NAN;

				stat_rate += rate;//mean daily rate
			}
		}
		else
		{
			ASSERT(info.m_type == T_HOBO);
			const vector<double>& T = info.T();


			double mean_time = info.t();
			ASSERT(ceil(mean_time) < T.size());
			for (size_t t = 0; t < mean_time; t++)
			{
				for (size_t h = 0; h < 24; h++)
				{
					double rate = max(LOWER_RATE_THRESHOLD, CDevRateEquation::GetRate(e, X, T[t * 24 + h]));//daily rate for each hour
					if (!isfinite(rate) || isnan(rate))
						return NAN;

					stat_rate += rate;//mean daily rate
				}
			}
		}

		double obs_time = info.at(I_MEAN_TIME);
		double time_SD = info.at(I_TIME_SD);
		double n = info.at(I_N);

		if (time_SD <= 0)
			time_SD = obs_time / 6.0;//an estimate of SD

		//1E-20: avoid division by zero
		double sim_time = 1.0 / max(1E-20, stat_rate[MEAN]);

		//compute probability 
		boost::math::normal_distribution<double> Normal(0, time_SD / sqrt(n));//In c++, Normals need SD

		ASSERT(isfinite(sim_time));
		double p = pdf(Normal, obs_time - sim_time);

		return log(max(DBL_MIN, p));
	}

	//double Regniere2021DevRateMean(TDevRateEquation  e, double sigma_mean, const vector<double>& X, const vector<double>& T, double mean_time, double n)
	//{
	//	ASSERT(sigma_mean > 0);


	//	CStatistic stat_rate;
	//	if (T.size() == 1)//optimization at constant temperature (rate is always the same)
	//	{
	//		double rate = max(0.0, CDevRateEquation::GetRate(e, X, T[0]));//daily rate
	//		if (!isfinite(rate) || isnan(rate))
	//			return NAN;

	//		stat_rate = rate;//mean daily rate
	//	}
	//	else if (T.size() == 24)//optimization at constant 24 hour cycle temperature (rate is always the same)
	//	{

	//		double rate = 0;
	//		for (size_t h = 0; h < 24; h++)
	//			rate += max(0.0, CDevRateEquation::GetRate(e, X, T[h])) / 24.0;//hourly rate;

	//		if (!isfinite(rate) || isnan(rate))
	//			return NAN;

	//		stat_rate = rate;//mean daily rate
	//	}
	//	else
	//	{
	//		for (size_t t = 0; t < mean_time; t++)
	//		{
	//			for (size_t h = 0; h < 24; h++)
	//			{
	//				double rate = max(0.0, CDevRateEquation::GetRate(e, X, T[t * 24 + h]));//rate for each hourly
	//				if (!isfinite(rate) || isnan(rate))
	//					return NAN;

	//				stat_rate += rate;//mean of rate
	//			}
	//		}
	//	}

	//	//1E-20: avoid division by zero
	//	double sim_time = 1.0 / max(1E-20, stat_rate[MEAN]);

	//	//compute probability of changing stage between ti-1 and ti
	//	boost::math::normal_distribution<double> Normal(1, sigma_mean / sqrt(n));
	//	double p = max(1e-200, pdf(Normal, mean_time / sim_time));

	//	return log(p);
	//}

	//double Regniere2021Survival(TSurvivalEquation e, const vector<double>& X, const vector<double>& T, double mean_time, double survival_obs, double n)
	//{
	//	double S = GetSurvival(e, X, T, mean_time);

	//	if (!isfinite(S) || isnan(S))
	//		return S;

	//	double expected = max(0.001, min(1e10, n * S));//limit to a very low expected when zero survival
	//	boost::math::poisson_distribution<double> Poisson(expected);

	//	//compute probability of surviving
	//	double p = max(1e-20, pdf(Poisson, survival_obs));
	//	return log(p);
	//}

	double Regniere2021Survival(TSurvivalEquation e, const vector<double>& X, const CDevRateDataRow& info)
	{
		const vector<double>& T = info.T();


		//double mean_time = info.at(I_MEAN_TIME);
		double S_obs = info.at(I_SURVIVAL);
		double n = info.at(I_N);

		double S = GetSurvival(e, X, info);

		if (!isfinite(S) || isnan(S))
			return S;

		double expected = max(0.0001, min(1e10, n * S));//limit to a very low expected when zero survival
		boost::math::poisson_distribution<double> Poisson(expected);

		//compute probability of surviving
		double p = max(DBL_MIN, pdf(Poisson, S_obs));
		return log(p);
	}

	//to : pre-oviposition period
	double Regniere2021FecundityTimeSeries(TDevRateEquation  e, const vector<double>& X, const vector<double>& T, double tiˉ¹, double ti, double brood_obs, double qBrood)
	{
		ASSERT(tiˉ¹ < ti);

		double to = 0;// X[X.size() - 3];
		//Get Fo and sigma from input parameters X
		double Fo = X[X.size() - 2];
		double sigma_f = X[X.size() - 1];

		//create relative fecundity unbiased log-normal distribution
		boost::math::lognormal_distribution<double> LogNormal(log(Fo) - 0.5 * Square(sigma_f), sigma_f);

		//compute individual fecondity (Fi) from Fo and female quantile
		double Fi = quantile(LogNormal, min(0.99999, qBrood));


		double Ft = Fi;//remaining fecundity at day t
		double Ftˉ¹ = Fi;//remaining fecundity at day t-1
		if (T.size() == 1)//optimization at constant temperature (rate is always the same)
		{
			//compute lambda from equations parameters and fixed temperature
			double lambda = max(0.0, CDevRateEquation::GetRate(e, X, T[0]));//remaining eggs
			if (!isfinite(lambda) || isnan(lambda))
				return NAN;

			//double Fi = brood_obs / (exp(-lambda * (tiˉ¹ - to)) - exp(-lambda * (ti - to)));

			//compute remainning fecundity
			if (ti - to >= 0)
				Ft = Fi * exp(-lambda * (ti - to));
			if (tiˉ¹ - to >= 0)
				Ftˉ¹ = Fi * exp(-lambda * (tiˉ¹ - to));


			//Ft = exp(-lambda * (ti - to));
			//Ftˉ¹ = exp(-lambda * (tiˉ¹ - to));
		}
		else if (T.size() == 24)//optimization at constant 24 hour cycle temperature (rate is always the same)
		{
			//xi = integral (hourly summation) of all hourly rate at hourly temperature at day ti
			//xiˉ¹ = integral (hourly summation) of all hourly rate at hourly temperature at day ti-1

			//double Ft = sigma * exp(-lambda * ((ti - t0-2) * 24));
			//double Ftˉ¹ = sigma * exp(-lambda * ((ti - 1 - t0) * 24));
			//double Fi[24] = { 0 };
			//double lambda[24] = { 0 };
			//for (size_t h = 0; h < 24; h++)
			//{
			//	lambda[h] = max(0.0, CDevRateEquation::GetRate(e, computation.m_XP, T[h])) / 24.0;//hourly rate;

			//	if (!isfinite(lambda[h]) || isnan(lambda[h]))
			//		return NAN;

			//	//Fi[h] = brood_obs / (exp(-lambda[h] * (tiˉ¹ - to)) - exp(-lambda[h] * (ti - to)));


			//}


			//Ft = brood_obs;
			//Ftˉ¹ = brood_obs;
			//for (size_t t = to; t < ti; t++)
			//{
			//	for (size_t h = 0; h < 24; h++)
			//	{
			//		Ft += Ft * lambda[h];

			//		if (t < (ti - 1))
			//			Ftˉ¹ -= Ft * lambda[h];
			//	}
			//}
			assert(false);//a vérifier

		}
		else
		{
			assert(false);//a vérifier

			//Ft = brood_obs;
			//Ftˉ¹ = brood_obs;

			//for (size_t t = to; t < ti; t++)
			//{
			//	for (size_t h = 0; h < 24; h++)
			//	{
			//		ASSERT(t * 24 + h < T.size());
			//		double lambda = max(0.0, CDevRateEquation::GetRate(e, computation.m_XP, T[t * 24 + h]));//hourly remaining eggs
			//		if (!isfinite(lambda) || isnan(lambda))
			//			return NAN;

			//		Ft -= Ft * lambda;

			//		if (t < (ti - 1))
			//			Ftˉ¹ -= Ft * lambda;
			//	}
			//}
		}


		//create Poisson distribution from expected values
		double expected = max(1e-5, Ftˉ¹ - Ft);

		boost::math::poisson_distribution<double> Poisson(expected);

		//compute probability to get observed brood
		double p = max(1e-100, boost::math::pdf(Poisson, brood_obs));

		//boost::math::poisson_distribution<double> Poisson(max(0.0001, brood_obs));
		//double expected = Ftˉ¹ - Ft;
		//double p = max(1e-20, boost::math::pdf(Poisson, expected));


		return log(p);
	}


	double Regniere2021Fecundity(TDevRateEquation  e, const vector<double>& X, const vector<double>& T, double time, double brood_obs, double qBrood)
	{
		//Get to, Fo and sigma from input parameters X
		double to = 0;// X[X.size() - 3];
		double Fo = X[X.size() - 2];
		double sigma_f = X.back();

		//create relative fecundity unbiased log-normal distribution
		boost::math::lognormal_distribution<double> LogNormal(log(Fo) - 0.5 * Square(sigma_f), sigma_f);

		//compute individual fecondity (Fi) from Fo and female quantile
		double Fi = quantile(LogNormal, qBrood);


		double Ft = Fi;//remaining fecundity at end
		if (T.size() == 1)//optimization at constant temperature (rate is always the same)
		{
			double lambda = max(0.001, CDevRateEquation::GetRate(e, X, T[0]));//remaining eggs
			if (!isfinite(lambda) || isnan(lambda))
				return NAN;

			//double Fi = brood_obs / (exp(-lambda * (tiˉ¹ - to)) - exp(-lambda * (ti - to)));

			if (time - to >= 0)
				Ft = Fi * (1 - exp(-lambda * (time - to)));
			//Fi = brood_obs / (1 - exp(-lambda * (ti - to)));
			//Ftˉ¹ = Fi * exp(-lambda * (tiˉ¹ - to));


			//Ft = exp(-lambda * (ti - to));
			//Ftˉ¹ = exp(-lambda * (tiˉ¹ - to));
		}
		else if (T.size() == 24)//optimization at constant 24 hour cycle temperature (rate is always the same)
		{
			//xi = integral (hourly summation) of all hourly rate at hourly temperature at day ti
			//xiˉ¹ = integral (hourly summation) of all hourly rate at hourly temperature at day ti-1

			//double Ft = sigma * exp(-lambda * ((ti - t0-2) * 24));
			//double Ftˉ¹ = sigma * exp(-lambda * ((ti - 1 - t0) * 24));
			//double Fi[24] = { 0 };
			//double lambda[24] = { 0 };
			//for (size_t h = 0; h < 24; h++)
			//{
			//	lambda[h] = max(0.0, CDevRateEquation::GetRate(e, computation.m_XP, T[h])) / 24.0;//hourly rate;

			//	if (!isfinite(lambda[h]) || isnan(lambda[h]))
			//		return NAN;

			//	//Fi[h] = brood_obs / (exp(-lambda[h] * (tiˉ¹ - to)) - exp(-lambda[h] * (ti - to)));


			//}


			//Ft = brood_obs;
			//Ftˉ¹ = brood_obs;
			//for (size_t t = to; t < ti; t++)
			//{
			//	for (size_t h = 0; h < 24; h++)
			//	{
			//		Ft += Ft * lambda[h];

			//		if (t < (ti - 1))
			//			Ftˉ¹ -= Ft * lambda[h];
			//	}
			//}
			assert(false);//a vérifier

		}
		else
		{
			assert(false);//a vérifier

			//Ft = brood_obs;
			//Ftˉ¹ = brood_obs;

			//for (size_t t = to; t < ti; t++)
			//{
			//	for (size_t h = 0; h < 24; h++)
			//	{
			//		ASSERT(t * 24 + h < T.size());
			//		double lambda = max(0.0, CDevRateEquation::GetRate(e, computation.m_XP, T[t * 24 + h]));//hourly remaining eggs
			//		if (!isfinite(lambda) || isnan(lambda))
			//			return NAN;

			//		Ft -= Ft * lambda;

			//		if (t < (ti - 1))
			//			Ftˉ¹ -= Ft * lambda;
			//	}
			//}
		}

		double expected = max(0.001, Ft);
		boost::math::poisson_distribution<double> Poisson(expected);

		////compute probability to get observed brood
		double p = max(1e-20, pdf(Poisson, brood_obs));

		//		boost::math::lognormal_distribution<double> LogNormal(log(Fo) - 0.5*Square(sigma_f), sigma_f);
		//	double p2 = max(1e-200, pdf(LogNormal, Ft));

		return log(p);// + log(p2)
	}

	double Regniere2021FecundityMeanSDn(TDevRateEquation e, const vector<double>& X, const vector<double>& T, double mean_time, double mean_broods, double broodSD, double n)
	{
		double to = 0;
		double Fo = X[X.size() - 2];

		double sim_broods = 0;
		CStatistic stat_lambda;
		if (T.size() == 1)//optimization at constant temperature (rate is always the same)
		{
			double lambda = max(0.0, CDevRateEquation::GetRate(e, X, T[0]));//remaining eggs
			if (!isfinite(lambda) || isnan(lambda))
				return NAN;

			//stat_lambda = lambda;//mean daily rate
			sim_broods = Fo * (1.0 - exp(-lambda * mean_time));
		}
		else if (T.size() == 24)//optimization at constant 24 hour cycle temperature (rate is always the same)
		{
			double lambda = 0;
			for (size_t h = 0; h < 24; h++)
				lambda += max(0.0, CDevRateEquation::GetRate(e, X, T[h])) / 24.0;//hourly rate;

			if (!isfinite(lambda) || isnan(lambda))
				return NAN;

			stat_lambda = lambda;//mean daily rate
		}
		else
		{
			for (size_t t = 0; t < mean_time; t++)
			{
				for (size_t h = 0; h < 24; h++)
				{
					double lambda = max(0.0, CDevRateEquation::GetRate(e, X, T[t * 24 + h]));//rate for each hour
					if (!isfinite(lambda) || isnan(lambda))
						return NAN;

					stat_lambda += lambda;//mean daily rate
				}
			}
		}

		//mean of brood rate eggs/day
		//double lambda = stat_lambda[MEAN];
		//double sim_broods = Fo * (1.0 - exp(-lambda * mean_time));

		//compute probability of changing stage between ti-1 and ti
		boost::math::normal_distribution<double> Normal(0, broodSD / sqrt(n));


		double p = max(1e-200, pdf(Normal, mean_broods - sim_broods));
		return log(p);

	}

	//**********************************************************************************************
	//CTobsSeries

	const char* CTobsSeries::INPUT_NAME[NB_TOBS_COL] = { "Tid","T" };

	CTobsSeries::CTobsSeries()
	{}

	CTobsSeries::~CTobsSeries()
	{}


	TTobsCol CTobsSeries::get_input(const std::string& name)
	{
		TTobsCol col = C_UNKNOWN;

		auto it = find_if(std::begin(INPUT_NAME), std::end(INPUT_NAME), [&](auto& s) {return boost::iequals(s, name); });
		if (it != std::end(INPUT_NAME))
			col = static_cast<TTobsCol>(std::distance(std::begin(INPUT_NAME), it));

		return col;

	}

	size_t CTobsSeries::get_pos(TTobsCol c)const
	{
		size_t pos = NOT_INIT;
		auto it = std::find(m_input_pos.begin(), m_input_pos.end(), c);
		if (it != m_input_pos.end())
			pos = std::distance(m_input_pos.begin(), it);

		return pos;
	}

	ERMsg CTobsSeries::load(const std::string& file_path)
	{
		ERMsg msg;

		//begin to read
		ifStream file;
		std::locale utf8_locale = std::locale(std::locale::classic(), new std::codecvt_utf8<size_t>());
		file.imbue(utf8_locale);
		//file.imbue(std::locale("en_US.UTF-8"));
		msg = file.open(file_path);
		if (msg)
		{
			msg = load(file);
			file.close();
		}

		return msg;
	}

	ERMsg CTobsSeries::load(std::istream& io)
	{
		ERMsg msg;

		clear();
		m_input_pos.clear();
		//StringVector header;
		size_t pID = NOT_INIT;
		size_t pT = NOT_INIT;

		for (CSVIterator loop(io, ",;\t|", true, true); loop != CSVIterator() && msg; ++loop)
		{
			if (m_input_pos.empty())
			{
				for (size_t i = 0; i < loop.Header().size(); i++)
					m_input_pos.push_back(get_input(loop.Header()[i]));

				if (!have_var(C_TID))
				{
					msg.ajoute("Mandatory missing column. \"Tid\" must be define");
				}

				//check for mandatory columns
				if (!have_var(C_T))
				{
					msg.ajoute("Mandatory missing column: \"T\" must be define");
				}

				pID = get_pos(C_TID);
				pT = get_pos(C_T);
			}

			if (msg)
			{
				if (pID < loop->size() &&
					pT < loop->size())
				{
					double T = ToDouble((*loop)[pT]);
					(*this)[(*loop)[pID]].push_back(T);
				}
			}
		}



		return msg;
	}

	ERMsg CTobsSeries::verify(const CDevRateData& data)const
	{
		ERMsg msg;

		for (size_t i = 0; i < data.size(); i++)
		{
			if (data[i].m_type == T_HOBO)
			{
				if (find(data[i].m_treatment) == end())
				{
					msg.ajoute("treatment ID" + data[i].m_treatment + " not found in temperature file");
				}
				else
				{
					size_t nb_hours = at(data[i].m_treatment).size();
					size_t needed_hours = data[i].GetMaxTime() * 24;
					if (nb_hours < needed_hours)
					{
						msg.ajoute("Temperature profile for ID " + data[i].m_treatment + " don't have enought data");
						msg.ajoute("Profile have " + to_string(nb_hours) + " hours and " + to_string(needed_hours) + " is needed");
					}
				}

			}
		}

		return msg;
	}



	//generate temperature profile
	void CTobsSeries::generate(const CDevRateData& data)
	{
		//find number of maximum days for all treatment
		map<string, size_t> treatment;
		for (size_t i = 0; i < data.size(); i++)
		{
			if (data[i].m_type != T_HOBO)
			{
				if (treatment.find(data[i].m_treatment) == treatment.end())
				{
					treatment[data[i].m_treatment] = i;
				}
				else
				{
					double max_time = data[i].GetMaxTime();

					if (max_time > data[treatment[data[i].m_treatment]].GetMaxTime())
						treatment[data[i].m_treatment] = i;
				}
			}
		}

		//generate treatment
		for (auto it = treatment.begin(); it != treatment.end(); it++)
		{
			size_t i = it->second;
			ASSERT(data[i].m_type != T_HOBO);

			if (data[i].m_type == T_CONSTANT)
			{
				//constant T have only one value
				(*this)[data[i].m_treatment].resize(1, data[i].ComputeT(0));
			}
			else if (data[i].m_type == T_TRANSFER)
			{
				//transfer T have only two values
				(*this)[data[i].m_treatment].push_back(data[i].ComputeT(0));
				(*this)[data[i].m_treatment].push_back(data[i].ComputeT(1));
			}
			else if (data[i].m_type == T_SQUARE ||
				data[i].m_type == T_TRIANGULAR ||
				data[i].m_type == T_SINUS || data[i].m_type == T_SINUS_MEAN)
			{

				double T1 = data[i].ComputeT(0);
				double T2 = data[i].ComputeT(1);


				size_t h1 = data[i].GetH1();

				for (size_t h = 0; h < 24; h++)
				{
					double Ti = 0;
					if (data[i].m_type == T_SQUARE)
					{
						Ti = h < h1 ? T1 : T2;
					}
					else if (data[i].m_type == T_TRIANGULAR)
					{
						Ti = (h <= h1) ? T1 + (T2 - T1) * h / h1 : T2 + (T1 - T2) * (h - h1) / (24 - h1);
					}
					else if (data[i].m_type == T_SINUS || data[i].m_type == T_SINUS_MEAN)
					{
						double Tmean = (data[i].m_type == T_SINUS) ? (T1 + T2) / 2 : T1;
						double deltaT = (data[i].m_type == T_SINUS) ? (T2 - T1) / 2 : T2;
						//double Tmean = T1;
						//double deltaT = T2;
						ASSERT(h1 == 12);

						double theta = 2 * PI * h / 24.0;
						Ti = Tmean + deltaT * sin(theta);
					}
					


					(*this)[data[i].m_treatment].push_back(Ti);
				}//all hours
			}



		}//al treatment
	}

	void CTobsSeries::compute_stats()
	{
		//for all treatment
		for (auto it = begin(); it != end(); it++)
		{
			//for all temperature
			for (auto itt = it->second.begin(); itt != it->second.end(); itt++)
				m_stat[it->first] += *itt;
		}
	}


	//**********************************************************************************************
	//CDevRateInput

	TTemperature DevRateInput::get_TType(const std::string& name)
	{
		TTemperature  type = T_UNKNOWN;
		if (name.find(">") != string::npos && name.find(":") != string::npos)
			type = T_TRANSFER;
		else if (name.find("/") != string::npos)
			type = T_SQUARE;
		else if (name.find("^") != string::npos)
			type = T_TRIANGULAR;
		else if (name.find("~") != string::npos)
			type = T_SINUS;
		else if (name.find("±") != string::npos)
			type = T_SINUS_MEAN;
		else if (name.find_first_not_of("-0123456789.") == string::npos)
			type = T_CONSTANT;
		else
			type = T_HOBO;

		return type;
	}

	CDevRateDataRow::CDevRateDataRow()
	{
		m_type = -1;
		m_bIndividual = false;
		m_bTimeSeries = false;
		m_t1 = 0;
		m_t = 0;
		m_tˉ¹ = 0;
	}

	/*double CDevRateDataRow::GetT1T2(bool bT1) const
	{
		double T = -999;


		if (m_type == T_CONSTANT)
		{
			T = ToDouble(m_treatment);
		}
		else if (m_type != T_HOBO)
		{
			StringVector tmp(m_treatment, "/~^>:");
			ASSERT(tmp.size() == 2 || tmp.size() == 3);
			ASSERT(m_type != T_TRANSFER || tmp.size() == 3);

			if (tmp.size() == 2 || tmp.size() == 3)
				T = ToDouble(tmp[bT1 ? 0 : 1]);
		}
		else
		{
			ASSERT(false);
		}

		return T;
	}*/
	double CDevRateDataRow::ComputeT(size_t i) const
	{
		assert(i >= 0 && i <= 1);
		double T = -999;


		if (m_type == T_CONSTANT)
		{
			T = ToDouble(m_treatment);
		}
		else if (m_type != T_HOBO)
		{
			StringVector tmp(m_treatment, "/~^>:±");
			ASSERT(tmp.size() == 2 || tmp.size() == 3);
			ASSERT(m_type != T_TRANSFER || tmp.size() == 3);

			if (tmp.size() == 2 || tmp.size() == 3)
				T = ToDouble(tmp[i]);
		}
		else
		{
			ASSERT(false);
		}

		return T;
	}

	double CDevRateDataRow::GetTime1() const
	{
		ASSERT(m_type == T_TRANSFER);
		double Time1 = -999;
		if (m_type == T_TRANSFER)
		{
			StringVector tmp(m_treatment, ">:");
			ASSERT(tmp.size() == 3);

			if (tmp.size() == 3)
				Time1 = ToDouble(tmp[2]);
		}


		return Time1;
	}

	size_t CDevRateDataRow::GetH1() const
	{
		ASSERT(m_type == T_SQUARE || m_type == T_TRIANGULAR || m_type == T_SINUS || m_type == T_SINUS_MEAN);
		size_t h1 = 12;

		if (m_type == T_SQUARE || m_type == T_TRIANGULAR)
		{
			StringVector tmp(m_treatment, ":");
			if (tmp.size() == 2)//if ":", set h1 other than default value of 12 hours
			{
				h1 = ToSizeT(tmp[1]);
				ASSERT(h1 < 24);
			}
		}

		return h1;
	}

	double CDevRateDataRow::GetTime() const
	{
		ASSERT((find(I_TIME) != end()) != (find(I_MEAN_TIME) != end()));//Time or MeanTime , but not both
		//		ASSERT(find(I_TIME) != end() || at(I_N) == 1);

		double Time = -999;
		if (find(I_START) != end() && find(I_TIME) != end())
			Time = at(I_START) + at(I_TIME);
		else if (find(I_TIME) != end())
			Time = at(I_TIME);
		else if (find(I_MEAN_TIME) != end())
			Time = at(I_MEAN_TIME);


		return Time;
	}


	//double CDevRateDataRow::GetTime1Time2(bool bT1) const
	//{
	//	double Time = -999;

	//	if (bT1)
	//	{
	//		if (m_type == T_TRANSFER)
	//		{
	//			StringVector tmp(m_treatment, ">:");
	//			ASSERT(tmp.size() == 3);

	//			if (tmp.size() == 3)
	//				Time = ToDouble(tmp[2]);
	//		}
	//		else
	//		{
	//			ASSERT(find(I_TIME) != end() || find(I_MEAN_TIME) != end());//Time must be define of use MeanTime when n=1
	//			ASSERT(find(I_TIME) != end() || at(I_N) == 1);

	//			if (find(I_TIME) != end())
	//				Time = at(I_TIME);
	//			else if (find(I_MEAN_TIME) != end())
	//				Time = at(I_MEAN_TIME);
	//		}
	//	}
	//	else
	//	{
	//		if (m_type == T_TRANSFER)
	//		{
	//			double Time1 = GetTime1Time2(true);
	//			if (find(I_TIME) != end())
	//				Time = max(0.0, at(I_TIME)- Time1);
	//			else if (find(I_MEAN_TIME) != end())
	//				Time = max(0.0, at(I_MEAN_TIME)- Time1);
	//		}
	//		else
	//		{
	//			Time = 0;
	//		}
	//	}


	//	ASSERT(Time != -999);
	//	return Time;
	//}

	double CDevRateDataRow::GetMaxTime() const
	{
		double max_time = 0;

		if (find(I_TIME) != end())
		{
			max_time = ceil(at(I_TIME));
		}
		else if (find(I_MEAN_TIME) != end())
		{
			double mean = at(I_MEAN_TIME);
			if (mean > 0)
			{
				max_time = ceil(mean);

				if (find(I_TIME_SD) != end() && find(I_N) != end())
				{
					//GetDefaultSigma();
					size_t n = at(I_N);
					double sd = at(I_TIME_SD);

					if (sd > 0)//in case of NA
					{
						double cv = sd / mean;
						double sigma = CDevRateData::cv_2_sigma(cv, n);
						boost::math::lognormal_distribution<double> obsLogNormal(-0.5 * Square(sigma), sigma);

						double q = max(0.005, 1 / (n + 1.0));
						double RDR = quantile(obsLogNormal, q);

						max_time = ceil(mean / (RDR * exp(Square(sigma))));//estimate of time (approx)

						//	//double alpha = 0.05;
						//	//double q = pow(alpha, 1.0 / N);
						//	//max_time = ceil(quantile(obsLogNormal, q));



					}
				}
			}
		}

		return max_time;
	}

	const char* CDevRateData::INPUT_NAME[NB_DEV_INPUT] = { "Variable","T", "I", "Start", "Time","MeanTime","TimeSD","N", "ObsInt", "RDT", "qTime", "Rate", "RDR", "qRate", "Survival", "Brood", "MeanBrood", "BroodSD", "RFR", "qBrood" };
	TDevTimeCol CDevRateData::get_input(const std::string& name)
	{
		TDevTimeCol col = I_UNKNOWN;
		auto it = find_if(std::begin(INPUT_NAME), std::end(INPUT_NAME), [&](auto& s) {return boost::iequals(s, name); });
		if (boost::iequals("Stage", name))//also accept Stage instead of Variable
			it = std::begin(INPUT_NAME);

		if (it != std::end(INPUT_NAME))
			col = static_cast<TDevTimeCol>(std::distance(std::begin(INPUT_NAME), it));

		return col;
	}

	size_t CDevRateData::get_pos(TDevTimeCol c)const
	{
		size_t pos = NOT_INIT;
		auto it = find(m_input_pos.begin(), m_input_pos.end(), c);
		if (it != m_input_pos.end())
			pos = std::distance(m_input_pos.begin(), it);

		return pos;
	}

	CDevRateData::CDevRateData()
	{}

	CDevRateData::~CDevRateData()
	{}


	ERMsg CDevRateData::load(const std::string& file_path)
	{
		ERMsg msg;

		//begin to read
		ifStream file;
		//std::locale utf8_locale = std::locale(std::locale::classic(), new std::codecvt_utf8<size_t>());


		std::locale utf8_locale = std::locale(std::locale::classic(), new std::codecvt_utf8<size_t>());
		file.imbue(utf8_locale);

		//file.imbue(std::locale("en_US.UTF-8"));
		msg = file.open(file_path);
		if (msg)
		{
			msg = load(file);
			file.close();
		}

		if (!msg)
			msg.ajoute("Error when reading file:" + file_path);

		return msg;
	}


	void CDevRateData::clear()
	{
		CDevRateDataRowVector::clear();
		m_pos.clear();
		m_input_pos.clear();
		m_statsTime.clear();
		m_statsRate.clear();
		m_statsBrood.clear();
		m_statsRateByVariable.clear();
		m_statsTByVariable.clear();

	}


	ERMsg CDevRateData::load(std::istream& io)
	{
		ERMsg msg;

		clear();
		//m_input_pos.clear();


		//StringVector header;
		for (CSVIterator loop(io, ",;\t|", true, true); loop != CSVIterator() && msg; ++loop)
		{
			if (m_input_pos.empty())
			{
				for (size_t i = 0; i < loop.Header().size(); i++)
					m_input_pos.push_back(get_input(loop.Header()[i]));


				if (!have_var(I_TREATMENT))
				{
					msg.ajoute("Mandatory missing column. \"T\" must be define");
				}


				//check for mandatory columns
				if (!have_var(I_TIME) && !have_var(I_MEAN_TIME))
				{
					msg.ajoute("Mandatory missing column: \"Time\" or \"MeanTime\" must be define");
				}
			}


			if (msg && !loop->empty())
			{
				if (loop->size() != m_input_pos.size())
				{
					msg.ajoute("Bad number of column for line:" + loop->GetLastLine());
					return msg;
				}

				CDevRateDataRow row;
				for (size_t i = 0; i < m_input_pos.size(); i++)
				{
					if (m_input_pos[i] != I_UNKNOWN)
					{
						if (m_input_pos[i] == I_VARIABLE)
							row.m_variable = (*loop)[i];
						else if (m_input_pos[i] == I_TREATMENT)
							row.m_treatment = (*loop)[i];
						else if (m_input_pos[i] == I_I)
							row.m_i = (*loop)[i];
						else
						{
							row[m_input_pos[i]] = ((*loop)[i] != "NA") ? ToDouble((*loop)[i]) : -999.0;
						}
					}
				}



				/*if (have_var(I_TIME_SD) && row[I_TIME_SD] <= 0)
				{
					msg.ajoute("Standard deviation equal zero is not supported: " + loop->GetLastLine());
					return msg;
				}*/


				row.m_type = get_TType(row.m_treatment);
				row.m_bIndividual = row.find(I_TIME) != row.end() && row[I_TIME] > 0;
				row.m_bTimeSeries = find(m_input_pos.begin(), m_input_pos.end(), I_START) != m_input_pos.end() && find(m_input_pos.begin(), m_input_pos.end(), I_I) != m_input_pos.end();

				if (!have_var(I_START))
					row[I_START] = 0.0;

				if (!have_var(I_N))
					row[I_N] = 1.0;

				if (!have_var(I_OBS_INT))
					row[I_OBS_INT] = 1.0;//default observed interval is daily


				if (!have_var(I_I))
					row[I_I] = size() + 1.0;

				if (!have_var(I_RATE) && have_var(I_TIME))
					row[I_RATE] = 1.0 / row[I_TIME];

				//for optimization
				if (row.m_type == T_TRANSFER)
					row.SetTime1(row.GetTime1());

				assert(row.GetTime() - row[I_OBS_INT] >= 0);
				row.SetTime(row.GetTime());
				row.SetTimeˉ¹(max(0.0, row.GetTime() - row[I_OBS_INT]));
				if (row.m_type == T_TRANSFER && row.GetTime() > 0 && row.GetTime() < row.GetTime1())
				{
					msg.ajoute("Error: transfer row with Time1 greater than total Time");
					msg.ajoute((*loop).GetLastLine());
				}


				m_pos[row.m_variable][row.m_treatment].push_back(size());
				push_back(row);

				if (have_var(I_TIME))
				{
					for (size_t n = 0; n < row[I_N]; n++)
					{
						m_statsTime[row.m_variable][row.m_treatment].Add(row[I_TIME]);
						m_statsRate[row.m_variable][row.m_treatment].Add(row[I_RATE]);
						m_statsRateByVariable[row.m_variable].Add(row[I_RATE]);
					}

				}
				else if (have_var(I_MEAN_TIME) && have_var(I_N))
				{
					for (size_t n = 0; n < row[I_N]; n++)
					{
						if (row[I_MEAN_TIME] > 0)
						{
							m_statsTime[row.m_variable][row.m_treatment].Add(row[I_MEAN_TIME]);
							m_statsRate[row.m_variable][row.m_treatment].Add(1 / row[I_MEAN_TIME]);
							m_statsRateByVariable[row.m_variable].Add(1 / row[I_MEAN_TIME]);
						}
					}

				}

				if (have_var(I_BROOD))
				{
					m_statsBrood[row.m_treatment][row.m_i].Add(row[I_BROOD]);
				}
				else if (have_var(I_MEAN_BROOD) && have_var(I_MEAN_TIME) && have_var(I_N))
				{
					for (size_t n = 0; n < row[I_N]; n++)
						m_statsBrood[row.m_variable][row.m_treatment].Add(row[I_MEAN_BROOD]);
				}



			}

			//for all variables
			for (auto it = m_pos.begin(); it != m_pos.end(); it++)
			{
				//for all treatment
				for (auto iit = it->second.begin(); iit != it->second.end(); iit++)
				{
					const vector<size_t>& pos = iit->second;
					double last_time = -999;
					for (auto iiit = pos.begin(); iiit != pos.end(); iiit++)
					{
						CDevRateDataRow& row = at(*iiit);


						if (row.m_bIndividual && !row.m_bTimeSeries)
						{
							if (last_time <= 0)
							{
								last_time = max(0.0, row.GetTime() - row[I_OBS_INT]);//assume one day before the first observation
							}

							row.SetTimeˉ¹(last_time);

							assert(row.GetTime() > last_time);//if equal... 2 observation with the same time
							if (last_time != 0 && row.GetTime() <= last_time)
							{
								msg.ajoute("2 observations with the same time: " + to_string(last_time));
								return msg;
							}

							last_time = row.GetTime();
						}
					}
				}
			}

		}



		m_bIndividual = have_var(I_TIME);
		m_bAllTConstant = IsAllTConstant();


		if (m_bIndividual)
		{

			//compute reative time and qtime
			//for all variable
			for (auto it = m_statsTime.begin(); it != m_statsTime.end(); it++)
			{
				string variable = it->first;
				vector<pair<double, size_t>> qTime;
				double N = 0;
				CStatistic stat_q;

				//for all treatment
				for (auto iit = it->second.begin(); iit != it->second.end(); iit++)
				{
					string treatment = iit->first;

					double mean = iit->second[MEAN];
					double n = iit->second[NB_VALUE];
					N += n;

					for (size_t i = 0; i < size(); i++)
					{
						CDevRateDataRow& row = at(i);
						if (row.m_variable == variable && row.m_treatment == treatment)
						{
							double rel_time = row[I_TIME] / mean;
							row[I_RDT] = rel_time;
							qTime.push_back(make_pair(rel_time, i));
						}
					}
				}//for all traitement

				sort(qTime.begin(), qTime.end());

				//double alpha = 0.25;//alpha with 50% of the sample size
				//double p = 1.0 - pow(alpha, 1.0 / qTime.size());

				//double alpha = 0.05;//95% of the range
				//double p = 0.005; //99% of the range
				//double p = min(alpha, 1.0 - pow(alpha, 1.0 / qTime.size()));


				////double cumsum = 0;
				//for (size_t n = 0; n < qTime.size(); n++)
				//{
				//	//double q = p + (1.0 - 2.0*p)*n / (qTime.size() - 1);
				//	//nn = (0:(N - 1)) / (N - 1)

				//	double q = max(0.005, min(0.995, p + (1.0 - 2.0*p)*n / (qTime.size() - 1)));
				//	CDevRateDataRow& row = at(qTime[n].second);
				//	row[I_Q_TIME] = q;
				//	//cumsum += row[I_N];
				//}

				double cumsum = 0;
				for (auto it = qTime.begin(); it != qTime.end(); it++)
				{
					CDevRateDataRow& row = at(it->second);
					double q = max(0.005, min(0.995, (cumsum + row[I_N] / 2) / N));
					row[I_Q_TIME] = q;
					cumsum += row[I_N];
				}


			}//for all variable



			//compute relative rate and qRate
			//for all variable
			for (auto it = m_statsRate.begin(); it != m_statsRate.end(); it++)
			{
				string variable = it->first;
				vector<pair<double, size_t>> qRate;
				double N = 0;
				CStatistic stat_qRate;

				//for all treatment
				for (auto iit = it->second.begin(); iit != it->second.end(); iit++)
				{
					string treatment = iit->first;

					double mean = iit->second[MEAN];
					double n = iit->second[NB_VALUE];
					N += n;

					for (size_t i = 0; i < size(); i++)
					{
						CDevRateDataRow& row = at(i);
						if (row.m_variable == variable && row.m_treatment == treatment)
						{
							double RDR = row[I_RATE] / mean;
							row[I_RDR] = RDR;
							qRate.push_back(make_pair(RDR, i));
						}
					}
				}//for all traitement

				sort(qRate.begin(), qRate.end());

				//double alpha = 0.25;//alpha with 50% of the sample size
				//double p = 1.0 - pow(alpha, 1.0 / qTime.size());

				//double alpha = 0.05;//95% of the range
				//double p = 0.005; //99% of the range
				//double p = min(alpha, 1.0 - pow(alpha, 1.0 / qRate.size()));


				////double cumsum = 0;
				//for (size_t n = 0; n < qTime.size(); n++)
				//{
				//	//double q = p + (1.0 - 2.0*p)*n / (qTime.size() - 1);
				//	//nn = (0:(N - 1)) / (N - 1)

				//	double q = max(0.005, min(0.995, p + (1.0 - 2.0*p)*n / (qTime.size() - 1)));
				//	CDevRateDataRow& row = at(qTime[n].second);
				//	row[I_Q_TIME] = q;
				//	//cumsum += row[I_N];
				//}

				double cumsum = 0;
				for (auto it = qRate.begin(); it != qRate.end(); it++)
				{
					CDevRateDataRow& row = at(it->second);
					double q = max(0.005, min(0.995, (cumsum + row[I_N] / 2) / N));
					row[I_Q_RATE] = q;
					cumsum += row[I_N];
				}


			}//for all variable

		}

		m_bIndividualSeries = have_var(I_BROOD) && have_var(I_START) && have_var(I_TIME) && have_var(I_I);
		if (m_bIndividual && have_var(I_BROOD))
		{

			vector<pair<double, string>> qBrood;
			//CStatistic stat_qBrood;

			//CStatistic stat_fecundity;
			////for all treatment
			//for (auto it = m_statsBrood.begin(); it != m_statsBrood.end(); it++)
			//{
			//	//for all individual
			//	for (auto iit = it->second.begin(); iit != it->second.end(); iit++)
			//		stat_fecundity += iit->second[SUM];
			//}

			//double mean_fecundity = stat_fecundity[MEAN];

			//for all treatment
			for (auto it = m_statsBrood.begin(); it != m_statsBrood.end(); it++)
			{
				string treatment = it->first;

				CStatistic stat_fecundity;
				for (auto iit = it->second.begin(); iit != it->second.end(); iit++)
					stat_fecundity += iit->second[SUM];

				double mean_fecundity = stat_fecundity[MEAN];

				//for all individual

				//double fecundity = iit->second[SUM];
				//double RFR = fecundity / mean_fecundity;
				//qBrood.push_back(make_pair(RFR, iit->first));




				//for all observation
				for (auto iit = it->second.begin(); iit != it->second.end(); iit++)
				{
					string individual = iit->first;
					//for all individual
					//for (auto iit = it->second.begin(); iit != it->second.end(); iit++)
					//{
						//double fecundity = iit->second[SUM];
						//double RFR = fecundity / mean_fecundity;
					//	qBrood.push_back(make_pair(RFR, iit->first));
				//	}//for all individual

					double fecundity = iit->second[SUM];
					double RF = fecundity / mean_fecundity;
					qBrood.push_back(make_pair(RF, individual));


					//for all rows
					//for (size_t i = 0; i < size(); i++)
					//{
					//	CDevRateDataRow& row = at(i);
					//	if (row.m_variable == treatment && row.m_treatment == individual)
					//	{
					//		qBrood.push_back(make_pair(RF, i));
					//	}//for all individual
					//}
				}
			}//for all traitement


			sort(qBrood.begin(), qBrood.end());

			//double alpha = 0.05;//95% of the range
			//double p = min(alpha, 1.0 - pow(alpha, 1.0 / qBrood.size()));

			//double cumsum = 0;
			//double N = stat_fecundity[NB_VALUE];
			//for (auto it = qBrood.begin(); it != qBrood.end(); it++)
			//{

			//	for (auto i = m_statsBrood.begin(); i != m_statsBrood.end(); i++)
			//	{
			//		auto iit = i->second.find(it->second);
			//		if (iit != i->second.end())
			//		{
			//			//sum of individual and not brood
			//			double Brood = 1;// iit->second[SUM];
			//			double q = max(0.001, min(0.999, (cumsum + Brood / 2) / N));
			//			cumsum += Brood;


			//			for (size_t i = 0; i < size(); i++)
			//			{
			//				CDevRateDataRow& row = at(i);
			//				if (row.m_i == it->second)
			//				{
			//					row[I_Q_BROOD] = q;
			//				}
			//			}
			//			break;
			//		}
			//	}
			//}

			//size_t N = qBrood.size();//number of female
			//double cumsum = 0;
			for (size_t i = 0; i < qBrood.size(); i++)
			{

				//double q = max(0.005, min(0.995, (cumsum + row[I_N] / 2) / N));
				//double q = max(0.00001, min(0.9999, (i + 0.5) / qBrood.size()));
				//double q = max(0.00001, min(0.9999, (i + 1.0) / qBrood.size()));
				double q = (i + 1.0) / qBrood.size();

				//apply this quantile to all observation for this individual
				string individual = qBrood[i].second;
				for (size_t i = 0; i < size(); i++)
				{
					CDevRateDataRow& row = at(i);
					if (row.m_i == individual)
					{
						ASSERT(row[I_N] == 1);
						CDevRateDataRow& row = at(i);
						row[I_Q_BROOD] = q;
					}//for all individual
				}
			}


			//sort(begin(), end(), [](const CDevRateDataRow& a, const CDevRateDataRow& b) {return a.at(I_Q_BROOD) < b.at(I_Q_BROOD); });



		}//if individual
		else if (have_var(I_MEAN_TIME) && have_var(I_TIME_SD) && have_var(I_N))
		{
			//CDevRateData new_data;
			////for all variable
			//for (auto it = m_statsTime.begin(); it != m_statsTime.end(); it++)
			//{
			//	string variable = it->first;
			//	//vector<pair<double, size_t>> qTime;

			//	CStatistic stat_wm;
			//	CStatistic stat_n;

			//	//for all treatment
			//	//for (auto iit = it->second.begin(); iit != it->second.end(); iit++)
			//	//{
			//		//string treatment = iit->first;

			//		//double mean = iit->second[MEAN];
			//		//double n = iit->second[NB_VALUE];

			//	for (size_t i = 0; i < size(); i++)
			//	{
			//		CDevRateDataRow& row = at(i);
			//		if (row.m_variable == variable/* && row.m_treatment == treatment*/)
			//		{
			//			double mean = row[I_MEAN_TIME];
			//			double n = row[I_N];

			//			if (n > 0)
			//			{
			//				stat_wm += mean * n;
			//				stat_n += n;
			//			}
			//		}
			//	}
			//	//}//for all traitement


			//	double M = stat_n[NB_VALUE];
			//	double N = stat_n[SUM];
			//	double wm = stat_wm[SUM] / N;
			//	CStatistic stat_wsd;


			//	//for all treatment
			//	//for (auto iit = it->second.begin(); iit != it->second.end(); iit++)
			//	//{
			//		//string treatment = iit->first;

			//		//double mean = iit->second[MEAN];
			//		//double n = iit->second[NB_VALUE];

			//	for (size_t i = 0; i < size(); i++)
			//	{
			//		CDevRateDataRow& row = at(i);
			//		if (row.m_variable == variable /*&& row.m_treatment == treatment*/)
			//		{
			//			double mean = row[I_MEAN_TIME];
			//			double n = row[I_N];
			//			if (n > 0)
			//			{
			//				stat_wsd += n * Square(mean - wm);
			//			}
			//		}
			//	}
			//	//}//for all traitement

			//	double wsd = sqrt(stat_wsd[SUM] / ((M - 1) / M * N));
			//	double mu = log(Square(wm) / sqrt(Square(wsd) + Square(wm)));
			//	double sigma = sqrt(log(Square(wsd) / Square(wm) + 1));
			//	boost::math::lognormal_distribution<double> LogNormal(mu, sigma);


			//	//randomize order 
			//	vector<size_t> order(N);

			//	for (size_t i = 0; i < N; i++)
			//		order[i] = i;

			//	auto rng = std::default_random_engine{};
			//	std::shuffle(order.begin(), order.end(), rng);

			//	//for all treatment 
			//	//size_t ii = 0;
			//	//for (auto iit = it->second.begin(); iit != it->second.end(); iit++)
			//	//{
			//		//string treatment = iit->first;

			//		//double mean = iit->second[MEAN];
			//		//double n = iit->second[NB_VALUE];
			//		//N += n;

			//	for (size_t i = 0, ii=0; i < size(); i++)
			//	{
			//		CDevRateDataRow& row = at(i);
			//		if (row.m_variable == variable /*&& row.m_treatment == treatment*/)
			//		{
			//			for (size_t j = 0; j < size_t(row[I_N]); j++)
			//			{
			//				size_t n = order[ii];
			//				double p = 0.005;
			//				double q = p + (1.0 - 2 * p)*n / (N - 1);
			//				double time = ceil( quantile(LogNormal, q));
			//				//#row[I_Q_TIME] = q;
			//				//ow[I_TIME] = time;
			//				ii++;


			//				CDevRateDataRow new_row = row;
			//				new_row[I_Q_TIME] = q;
			//				new_row[I_TIME] = time;
			//				new_row[I_N] = 1;
			//				new_data.push_back(new_row);
			//			}
			//		}
			//	}
			//	//}//for all traitement

			//	//treat this simulation like an individual one

			//}//for all variable


			//new_data.m_bIndividual = true;
			//*this = new_data;
			//create individual values from mean, sd and N
			//for all variable
			//for (auto it = m_statsTime.begin(); it != m_statsTime.end(); it++)
			//{
			//	string varible = it->first;
			//	vector<pair<double, size_t>> qTime;
			//	double N = 0;
			//	//for all treatment of this variable
			//	for (auto iit = it->second.begin(); iit != it->second.end(); iit++)
			//		N += iit->second[NB_VALUE];

			//	for (auto iit = it->second.begin(); iit != it->second.end(); iit++)
			//	{
			//		string treatment = iit->first;

			//		double mean = iit->second[MEAN];
			//		double sd = iit->second[STD_DEV];
			//		double mu = log(Square(mean) / sqrt(Square(sd) + Square(mean)));
			//		double sigma = sqrt(log(Square(sd) / Square(mean) + 1));
			//		double n = iit->second[NB_VALUE];

			//		boost::math::lognormal_distribution<double> LogNormal(mu, sigma);
			//		for (size_t n = 0; n < N; n++)
			//		{
			//			//simulate N obs on the log-normal distribution with alpha = 0.05
			//			double q = 0.025 + (1.0 - 2*0.025)*n / (N - 1);
			//			double time = quantile(LogNormal, q);
			//		}
			//	}

			//	double cumsum = 0;
			//	for (auto it = qTime.begin(); it != qTime.end(); it++)
			//	{
			//		CDevRateDataRow& row = at(it->second);
			//		double q = max(0.001, min(0.999, (cumsum + row[I_N] / 2) / N));
			//		row[I_Q_TIME] = q;
			//		cumsum += row[I_N];
			//
			//	}
			//}

			//m_bIndividual = have_var(I_TIME);
		}


		//look there is no upper lower variable
		//MakeUpper(row.m_variable);



		return msg;
	}

	void CDevRateData::compute_T_stats(const CTobsSeries& Tobs)
	{
		//for all treatment
		for (size_t i = 0; i < size(); i++)
		{
			const CDevRateDataRow& row = at(i);
			m_statsTByVariable[row.m_variable] += Tobs.m_stat.at(row.m_treatment);
		}
	}

	bool CDevRateData::IsAllTConstant()const
	{
		bool bAllTConstant = true;

		for (size_t i = 0; i < size() && bAllTConstant; i++)
			bAllTConstant = at(i).m_type == T_CONSTANT;

		return bAllTConstant;
	}

	double CDevRateData::ei(size_t n) { return pow(1.0 + 1.0 / n, n); }
	double CDevRateData::cv_2_sigma(double cv, size_t n)
	{
		static const double e = exp(1);
		static const double p[3] = { 0.528196, 2.373248, 3.493202 };//with 10 000 replication
		return e * cv * (1 - p[0] * sqrt(e - ei(n))) / (p[1] + cv * (1 - p[2] * sqrt(e - ei(n))));
	}

	double CDevRateData::GetDefaultSigma(const std::string& variable)const
	{

		CStatistic stat_ws;
		CStatistic stat_n;


		//compute sigma
		if (m_bIndividual)
		{
			const std::map<std::string, CStatisticEx>& stat = m_statsTime.at(variable);
			//for all treatment
			for (auto it = stat.begin(); it != stat.end(); it++)
			{
				//string treatment = it->first;

				double mean = it->second[MEAN];
				double sd = it->second[STD_DEV];
				double n = it->second[NB_VALUE];
				if (n > 0)
				{
					double cv = sd / mean;
					double sigma = cv_2_sigma(cv, n);

					stat_ws += n * cv;
					stat_n += n;
				}
			}
		}

		else if (have_var(I_MEAN_TIME) && have_var(I_TIME_SD) && have_var(I_N))
		{
			//for all treatment
			for (size_t i = 0; i < size(); i++)
			{
				const CDevRateDataRow& row = at(i);
				if (row.m_variable == variable)
				{
					double mean = row.at(I_MEAN_TIME);
					double sd = row.at(I_TIME_SD);
					double n = row.at(I_N);

					if (mean > 0 && n > 0 && sd > 0)
					{
						double cv = sd / mean;
						double sigma = cv_2_sigma(cv, n);

						stat_ws += n * sigma;
						stat_n += n;
					}
				}
			}
		}
		else
		{
			//reasonable sigma value
			stat_ws += 0.2;
			stat_n += 1;
		}

		ASSERT(stat_n[SUM] > 0);
		double sigma = stat_ws[SUM] / stat_n[SUM];
		return sigma;
	}


	double CDevRateData::GetSigmaBrood(TDevRateEquation  e, const std::vector<double>& X, const vector<double>& T)const
	{
		double sigma = 0;

		//compute sigma
		if (m_bIndividualSeries)
		{

			//CStatistic stat_wm;
			//CStatistic stat_ws;
			//CStatistic stat_n;
			CStatistic stat;

			//for all treatment
			for (auto it = m_statsBrood.begin(); it != m_statsBrood.end(); it++)
			{


				const std::map<std::string, CStatisticEx>& individuals = it->second;
				//for all individuals
				for (auto iit = individuals.begin(); iit != individuals.end(); iit++)
				{
					double broods = iit->second[SUM];//sum af all observations
					double lambda = max(0.0, CDevRateEquation::GetRate(e, X, T[0]));//remaining eggs
					double Fi = broods / (1 - exp(-lambda * iit->second[NB_VALUE]));//a revoir estimer seulement....
					stat += Fi;
				}
			}


			double mean = stat[MEAN];//mean of all individuals
			double sd = stat[STD_DEV];
			sigma = sqrt(log(Square(sd) / Square(mean) + 1));

			//sigma = sqrt(log(Square(sd) / Square(mean) + 1));
			//sigma = stat_ws[SUM] / stat_n[SUM];
			//F = stat_wm[SUM] / stat_n[SUM];



		}
		else if (m_bIndividual)
		{
			CStatistic stat;
			//CStatistic stat_ws;
			//CStatistic stat_n;



			//for all treatment
			for (size_t i = 0; i < size(); i++)
			{
				const CDevRateDataRow& row = at(i);
				double lambda = max(0.0, CDevRateEquation::GetRate(e, X, T[0]));//remaining eggs
				double Fi = row.at(I_BROOD) / (1 - exp(-lambda * row.at(I_TIME)));
				stat += Fi;
			}

			ASSERT(stat[SUM] > 0);

			double mean = stat[MEAN];
			double sd = stat[STD_DEV];

			sigma = sqrt(log(Square(sd) / Square(mean) + 1));


		}

		return sigma;
	}

	double CDevRateData::GetDefaultSigma()const
	{

		double sigma = 0;
		CStatistic stat_wm;
		CStatistic stat_ws;
		CStatistic stat_n;



		//for all treatment
		for (size_t i = 0; i < size(); i++)
		{
			const CDevRateDataRow& row = at(i);
			//if (row.m_variable == variable)
			{
				double mean = row.at(I_MEAN_BROOD);
				double sd = row.at(I_BROOD_SD);
				double n = row.at(I_N);

				if (n > 0)
				{
					double cv = sd / mean;
					double sigma = cv_2_sigma(cv, n);

					stat_wm += n * mean;
					stat_ws += n * sigma;
					stat_n += n;
				}
			}
		}

		ASSERT(stat_n[SUM] > 0);

		sigma = stat_ws[SUM] / stat_n[SUM];
		//F = stat_wm[SUM] / stat_n[SUM];



		//double sigma = stat_ws[SUM] / stat_n[SUM];

		return sigma;
	}




	void CDevRateData::set_Tobs(const CTobsSeries& Tobs)
	{
		//find number of maximum days for all treatment

		//assign treatment temperature series to each row
		for (auto it = begin(); it != end(); it++)
		{
			it->SetTobs(std::make_shared<std::vector<double>>(Tobs.at(it->m_treatment)));
		}//al treatment
	}










	//**********************************************************************************************
	//CDevRateEqFile

	CDevRateEqFile::CDevRateEqFile()
	{}

	CDevRateEqFile::~CDevRateEqFile()
	{}

	ERMsg CDevRateEqFile::load(const std::string& file_path)
	{
		ERMsg msg;

		//begin to read
		ifStream file;
		std::locale utf8_locale = std::locale(std::locale::classic(), new std::codecvt_utf8<size_t>());
		file.imbue(utf8_locale);

		//file.imbue(std::locale("en_US.UTF-8"));
		msg = file.open(file_path);
		if (msg)
		{
			msg = load(file);
			file.close();
		}

		return msg;
	}

	ERMsg CDevRateEqFile::load(std::istream& io)
	{
		ERMsg msg;

		clear();

		static const char* COL_NAME[3] = { "Variable", "EqName", "P" };

		std::vector<size_t> col_pos;

		for (CSVIterator loop(io, ",;\t|", true, true); loop != CSVIterator() && msg; ++loop)
		{
			if (col_pos.empty())
			{
				for (size_t i = 0; i < 3; i++)
				{
					string find = COL_NAME[i];
					auto itr = std::find_if(loop.Header().begin(), loop.Header().end(),
						[&](auto& s)
						{
							if (IsEqual(s, find))
								return true;
							return false;
						}
					);

					if (itr != loop.Header().end())
					{
						col_pos.push_back(std::distance(loop.Header().begin(), itr));
					}
					else
					{
						msg.ajoute(string("Mandatory missing column. ") + COL_NAME[i] + " must be define");
					}
				}
			}


			if (msg)
			{
				string var_name = (*loop)[col_pos[0]];
				string eq_name = (*loop)[col_pos[1]];
				string str_param = (*loop)[col_pos[2]];

				std::vector<double> P;
				msg = CDevRateEquation::GetParamfromString(eq_name, str_param, P);
				if (msg)
				{
					(*this)[var_name] = make_pair(CDevRateEquation::eq(eq_name), P);
				}
			}
		}

		return msg;
	}



	//**********************************************************************************************
	//CSurvivalData 

	/*size_t CSurvivalData::get_pos(TDevTimeCol c)const
	{
		size_t pos = NOT_INIT;
		auto it = find(m_input_pos.begin(), m_input_pos.end(), c);
		if (it != m_input_pos.end())
			pos = std::distance(m_input_pos.begin(), it);

		return pos;
	}*/


	CSurvivalData::CSurvivalData()
	{}

	CSurvivalData::~CSurvivalData()
	{}


	ERMsg CSurvivalData::load(const std::string& file_path)
	{
		ERMsg msg;

		//begin to read
		ifStream file;
		std::locale utf8_locale = std::locale(std::locale::classic(), new std::codecvt_utf8<size_t>());
		file.imbue(utf8_locale);

		//file.imbue(std::locale("en_US.UTF-8"));
		msg = file.open(file_path);

		if (msg)
		{
			msg = load(file);
			file.close();


		}

		if (!msg)
			msg.ajoute("Error when reading file:" + file_path);

		return msg;
	}

	ERMsg CSurvivalData::load(std::istream& io)
	{
		ERMsg msg;

		msg = CDevRateData::load(io);
		if (msg)
		{
			if (!have_var(I_MEAN_TIME))
			{
				msg.ajoute("Mandatory missing column.  \"MeanTime\" must be define");
			}

			if (!have_var(I_SURVIVAL))
			{
				msg.ajoute("Mandatory missing column: \"Survival\" must be define");
			}

			if (!have_var(I_N))
			{
				msg.ajoute("Mandatory missing column: \"N\" must be define");
			}


			//replace missing Time by NA_TIME
			for (auto it = begin(); it != end(); it++)
			{
				if (it->GetTime() <= 0)
				{
					it->SetTimeˉ¹(NA_TIME - 1);
					it->SetTime(NA_TIME);
				}

				ASSERT(it->t() > 0);
				ASSERT(it->t(true) > 0);
				ASSERT(it->m_type != T_TRANSFER || it->t1() > 0);
				ASSERT(it->m_type != T_TRANSFER || it->t2() > 0);

			}

		}

		//clear();
		//m_input_pos.clear();
		////m_bIndividual = false;



		//for (CSVIterator loop(io, ",;\t|", true, true); loop != CSVIterator() && msg; ++loop)
		//{
		//	if (m_input_pos.empty())
		//	{
		//		for (size_t i = 0; i < loop.Header().size(); i++)
		//			m_input_pos.push_back(CDevRateData::get_input(loop.Header()[i]));

		//		if (!have_var(I_TREATMENT))
		//		{
		//			msg.ajoute("Mandatory missing column. \"T\" must be define");
		//		}

		//		if (!have_var(I_MEAN_TIME))
		//		{
		//			msg.ajoute("Mandatory missing column.  \"MeanTime\" must be define");
		//		}

		//		if (!have_var(I_N))
		//		{
		//			msg.ajoute("Mandatory missing column: \"N\" must be define");
		//		}
		//		//check for mandatory columns
		//		if (!have_var(I_SURVIVAL))
		//		{
		//			msg.ajoute("Mandatory missing column: \"Survival\" must be define");
		//		}
		//	}


		//	if (msg && !loop->empty())
		//	{
		//		if (loop->size() != m_input_pos.size())
		//		{
		//			msg.ajoute("Bad number of column for line:" + loop->GetLastLine());
		//			return msg;
		//		}

		//		CDevRateDataRow row;
		//		for (size_t i = 0; i < m_input_pos.size(); i++)
		//		{
		//			if (m_input_pos[i] != I_UNKNOWN)
		//			{
		//				if (m_input_pos[i] == I_VARIABLE)
		//					row.m_variable = (*loop)[i];
		//				else if (m_input_pos[i] == I_TREATMENT)
		//					row.m_treatment = (*loop)[i];
		//				else
		//					row[m_input_pos[i]] = ((*loop)[i] != "NA") ? ToDouble((*loop)[i]) : -999.0;
		//				//row[m_input_pos[i]] = ToDouble((*loop)[i]);
		//			}
		//		}


		//		row.m_type = get_TType(row.m_treatment);

		//		push_back(row);

		//	}
		//}


		return msg;
	}




	//**********************************************************************************************
	//CInsectParameterization
	const char* CInsectParameterization::DATA_DESCRIPTOR = "InsectParameterizationData";
	const char* CInsectParameterization::XML_FLAG = "InsectParameterization";
	const char* CInsectParameterization::MEMBERS_NAME[NB_MEMBERS_EX] = { "FitType", "DevRateEquations", "SurvivalEquations", "FecundityEquations","EquationsOptions", "InputFileName", "TobsFileName", "OutputFileName", "Control", "Fixe_Tb", "Tb_Value", "Fixe_Tm", "Tm_Value", "Counstrain_Tlo","Tlo_Values","Counstrain_Thi","ThiValues", "Fixe_F0", "F0_Value", "LimitMaxRate", "LimitMaxRateP", "AvoidNullRateInTobs", "UseOutputAsInput", "OutputAsIntputFileName", "ShowTrace" };





	const int CInsectParameterization::CLASS_NUMBER = CExecutableFactory::RegisterClass(CInsectParameterization::GetXMLFlag(), &CInsectParameterization::CreateObject);

	CInsectParameterization::CInsectParameterization()
	{
		Reset();
	}

	CInsectParameterization::~CInsectParameterization()
	{}


	CInsectParameterization::CInsectParameterization(const CInsectParameterization& in)
	{
		operator=(in);
	}


	void CInsectParameterization::Reset()
	{
		CExecutable::Reset();

		m_fitType = F_DEV_TIME;
		m_name = "InsectParameterization";
		m_eqDevRate.set();
		m_eqSurvival.set();
		m_eqFecundity.set();
		m_eq_options.clear();
		m_inputFileName.clear();
		m_TobsFileName.clear();
		m_outputFileName = "%i";//same as input file name
		m_bFixeTb = false;
		m_Tb = { 0,5,1 };
		m_bFixeTm = false;
		m_Tm = { 35, 40,1 };

		m_bConstrainTlo = false;
		m_Tlo = { 0,10 };
		m_bConstrainThi = false;
		m_Thi = { 35,45 };

		m_bFixeF0 = false;
		m_F0 = 100;
		m_bLimitMaxRate = false;
		m_LimitMaxRateP = 1.2;
		m_bAvoidNullRateInTobs = false;



		m_bUseOutputAsInput = false;
		m_outputAsIntputFileName.clear();
		m_bShowTrace = false;


		//m_calibOn = CO_RATE;
//		m_bConverge01 = false;
	//	m_bCalibSigma = false;

		//Warning: parameters for Mean+SD+n need more accuracy than the individual one.
		m_ctrl.Reset();
		m_ctrl.m_bMax = true;
		m_ctrl.m_statisticType = LIKELIHOOD;
		m_ctrl.m_MAXEVL = 10000000;
		m_ctrl.m_NS = 60;
		m_ctrl.m_NT = 30;
		m_ctrl.m_T = 200;
		m_ctrl.m_RT = 0.85;
		//m_ctrl.m_RT2 = 0.75;
		m_ctrl.m_EPS = 0.0001;


		m_Tobs.clear();
		m_devTime.clear();
		m_survival.clear();
		m_fecundity.clear();
	}


	CInsectParameterization& CInsectParameterization::operator =(const CInsectParameterization& in)
	{
		if (&in != this)
		{
			CExecutable::operator =(in);

			m_fitType = in.m_fitType;
			m_eqDevRate = in.m_eqDevRate;
			m_eqSurvival = in.m_eqSurvival;
			m_eqFecundity = in.m_eqFecundity;
			m_eq_options = in.m_eq_options;
			m_inputFileName = in.m_inputFileName;
			m_TobsFileName = in.m_TobsFileName;
			m_outputFileName = in.m_outputFileName;

			m_bFixeTb = in.m_bFixeTb;
			m_Tb = in.m_Tb;
			m_bFixeTm = in.m_bFixeTm;
			m_Tm = in.m_Tm;

			m_bConstrainTlo = in.m_bConstrainTlo;
			m_Tlo = in.m_Tlo;
			m_bConstrainThi = in.m_bConstrainThi;
			m_Thi = in.m_Thi;




			m_bFixeF0 = in.m_bFixeF0;
			m_F0 = in.m_F0;
			m_bLimitMaxRate = in.m_bLimitMaxRate;
			m_LimitMaxRateP = in.m_LimitMaxRateP;
			m_bAvoidNullRateInTobs = in.m_bAvoidNullRateInTobs;

			m_bUseOutputAsInput = in.m_bUseOutputAsInput;
			m_outputAsIntputFileName = in.m_outputAsIntputFileName;
			m_bShowTrace = in.m_bShowTrace;



			m_ctrl = in.m_ctrl;
		}

		return *this;
	}

	bool CInsectParameterization::operator == (const CInsectParameterization& in)const
	{
		bool bEqual = true;

		if (CExecutable::operator!=(in))bEqual = false;
		if (m_fitType != in.m_fitType)bEqual = false;
		if (m_eqDevRate != in.m_eqDevRate) bEqual = false;
		if (m_eqSurvival != in.m_eqSurvival) bEqual = false;
		if (m_eqFecundity != in.m_eqFecundity) bEqual = false;
		if (m_eq_options != in.m_eq_options)bEqual = false;
		if (m_inputFileName != in.m_inputFileName) bEqual = false;
		if (m_TobsFileName != in.m_TobsFileName) bEqual = false;

		if (m_outputFileName != in.m_outputFileName) bEqual = false;
		if (m_ctrl != in.m_ctrl)bEqual = false;

		if (m_bFixeTb != in.m_bFixeTb)bEqual = false;
		if (m_Tb != in.m_Tb)bEqual = false;
		if (m_bFixeTm != in.m_bFixeTm)bEqual = false;
		if (m_Tm != in.m_Tm)bEqual = false;

		if (m_bConstrainTlo != in.m_bConstrainTlo)bEqual = false;
		if (m_Tlo != in.m_Tlo)bEqual = false;
		if (m_bConstrainThi != in.m_bConstrainThi)bEqual = false;
		if (m_Thi != in.m_Thi)bEqual = false;



		if (m_bFixeF0 != in.m_bFixeF0)bEqual = false;
		if (m_F0 != in.m_F0)bEqual = false;
		if (m_bLimitMaxRate != in.m_bLimitMaxRate)bEqual = false;
		if (m_LimitMaxRateP != in.m_LimitMaxRateP)bEqual = false;
		if (m_bAvoidNullRateInTobs != in.m_bAvoidNullRateInTobs)bEqual = false;
		if (m_bUseOutputAsInput != in.m_bUseOutputAsInput)bEqual = false;
		if (m_outputAsIntputFileName != in.m_outputAsIntputFileName)bEqual = false;
		if (m_bShowTrace != in.m_bShowTrace)bEqual = false;


		return bEqual;
	}

	ERMsg CInsectParameterization::GetParentInfo(const CFileManager& fileManager, CParentInfo& info, CParentInfoFilter filter)const
	{
		ERMsg msg;

		//same as weather generator variables
		if (filter[LOCATION])
		{
			info.m_locations.resize(1);
			info.m_locations[0].m_name = "Mean over location";
		}
		if (filter[PARAMETER])
		{
			info.m_parameterset.clear();

			CModelInput modelInput;

			modelInput.SetName("T");
			modelInput.push_back(CModelInputParam("T", "15"));
			info.m_parameterset.push_back(modelInput);

			info.m_parameterset.m_pioneer = modelInput;
			info.m_parameterset.m_variation.SetType(CParametersVariationsDefinition::SYSTEMATIC_VARIATION);
			info.m_parameterset.m_variation.push_back(CParameterVariation("T", true, CModelInputParameterDef::kMVReal, 0, 35, 0.5));

		}
		if (filter[REPLICATION])
		{
			info.m_nbReplications = 1;
		}
		if (filter[TIME_REF])
		{
			CTM TM(CTM::ATEMPORAL, CTM::OVERALL_YEARS);
			info.m_period = CTPeriod(CTRef(YEAR_NOT_INIT, 0, 0, 0, TM), CTRef(YEAR_NOT_INIT, 0, 0, 0, TM));
		}
		if (filter[VARIABLE])
		{
			info.m_variables.clear();

			if (m_fitType == F_DEV_TIME)//_WTH_SIGMA || m_fitType == F_DEV_TIME_ONLY
			{
				//for all equation
				for (size_t e = 0; e < m_eqDevRate.size(); e++)
				{
					if (m_eqDevRate.test(e))
					{
						TDevRateEquation  eq = CDevRateEquation::eq(e);

						std::string name = CDevRateEquation::GetEquationName(eq);
						std::string title = CDevRateEquation::GetEquationName(eq);
						std::string units = "1/day";
						std::string description = "Development rate";
						info.m_variables.push_back(CModelOutputVariableDef(name, title, units, description));
					}
				}
			}
			else if (m_fitType == F_SURVIVAL)
			{
				for (size_t e = 0; e < m_eqSurvival.size(); e++)
				{
					if (m_eqSurvival.test(e))
					{
						TSurvivalEquation eq = CSurvivalEquation::eq(e);

						std::string name = CSurvivalEquation::GetEquationName(eq);
						std::string title = CSurvivalEquation::GetEquationName(eq);
						std::string units = "%";
						std::string description = "Survival";
						info.m_variables.push_back(CModelOutputVariableDef(name, title, units, description));
					}
				}
			}
			else if (m_fitType == F_FECUNDITY)
			{
				for (size_t e = 0; e < m_eqFecundity.size(); e++)
				{
					if (m_eqFecundity.test(e))
					{
						TDevRateEquation  eq = CDevRateEquation::eq(e);

						std::string name = CDevRateEquation::GetEquationName(eq);
						std::string title = CDevRateEquation::GetEquationName(eq);
						std::string units = "Eggs/day";
						std::string description = "Fecundity rate";
						info.m_variables.push_back(CModelOutputVariableDef(name, title, units, description));
					}
				}
			}

		}

		return msg;
	}

	string to_string(const CSAParameterVector& P)
	{
		std::ostringstream streamObj;

		for (size_t i = 0; i < P.size(); i++)
		{
			if (i > 0)
				streamObj << " ";

			streamObj << P[i].m_name << "=" << std::scientific << std::setprecision(6) << P[i].m_initialValue;
		}

		// Get string from output string stream
		return streamObj.str();
	}





	ERMsg CInsectParameterization::Execute(const CFileManager& fileManager, CCallback& callback)
	{
		ERMsg msg;

		//,"Development time only"
		static const char* TYPE_NAME[NB_FIT_TYPE] = { "Development time (with sigma)","Survival", "Oviposition" };
		bool bLogLikelyhoude = m_ctrl.m_statisticType == LIKELIHOOD;





		CResult resultDB;
		msg = resultDB.Open(GetDBFilePath(GetPath(fileManager)), std::fstream::out | std::fstream::binary);
		if (msg)
		{
			callback.AddMessage(GetString(IDS_WG_PROCESS_INPUT_ANALYSIS));
			callback.AddMessage(resultDB.GetFilePath(), 1);


			CParentInfo info;
			msg = GetParentInfo(fileManager, info);
			if (msg)
			{
				string output_file_name = !m_outputFileName.empty() ? m_outputFileName : GetFileTitle(m_inputFileName);
				SetFileExtension(output_file_name, ".csv");
				ReplaceString(output_file_name, "%i", GetFileTitle(m_inputFileName));
				string outputFilePath = fileManager.GetOutputPath() + output_file_name;



				CDBMetadata& metadata = resultDB.GetMetadata();
				metadata.SetLocations(info.m_locations);
				metadata.SetParameterSet(info.m_parameterset);
				metadata.SetNbReplications(info.m_nbReplications);
				metadata.SetTPeriod(info.m_period);
				metadata.SetOutputDefinition(info.m_variables);

				callback.AddMessage(FormatMsg(IDS_SIM_CREATE_DATABASE, m_name));
				callback.AddMessage(resultDB.GetFilePath(), 1);



				//set vMiss value
				m_ctrl.SetVMiss(m_ctrl.AdjustFValue(DBL_MAX));


				string inputFilePath = fileManager.Input().GetFilePath(m_inputFileName);

				if (m_fitType == F_DEV_TIME)//_WTH_SIGMA || m_fitType == F_DEV_TIME_ONLY
				{
					msg = m_devTime.load(inputFilePath);
				}
				else if (m_fitType == F_SURVIVAL)
				{
					msg += m_survival.load(inputFilePath);
				}
				else if (m_fitType == F_FECUNDITY)
				{
					msg += m_fecundity.load(inputFilePath);
				}

				//Load temperature profile
				if (!m_TobsFileName.empty())
				{
					string TobsFilePath = fileManager.Input().GetFilePath(m_TobsFileName);
					msg += m_Tobs.load(TobsFilePath);
				}

				//complete by generating fixed temperature
				if (m_fitType == F_DEV_TIME)//_WTH_SIGMA || m_fitType == F_DEV_TIME_ONLY
				{
					m_Tobs.generate(m_devTime);
				}
				else if (m_fitType == F_SURVIVAL)
				{
					m_Tobs.generate(m_survival);
				}
				else if (m_fitType == F_FECUNDITY)
				{
					m_Tobs.generate(m_fecundity);
				}




				//verify that all temperature profile have enough data
				if (msg)
				{
					if (m_fitType == F_DEV_TIME)//_WTH_SIGMA || m_fitType == F_DEV_TIME_ONLY
					{
						msg = m_Tobs.verify(m_devTime);
					}
					else if (m_fitType == F_SURVIVAL)
					{
						msg = m_Tobs.verify(m_survival);
					}
					else if (m_fitType == F_FECUNDITY)
					{
						m_Tobs.verify(m_fecundity);
					}
				}
				if (msg)
				{
					if (m_fitType == F_DEV_TIME)//_WTH_SIGMA || m_fitType == F_DEV_TIME_ONLY
					{
						m_devTime.set_Tobs(m_Tobs);
					}
					else if (m_fitType == F_SURVIVAL)
					{
						m_survival.set_Tobs(m_Tobs);
					}
					else if (m_fitType == F_FECUNDITY)
					{
						m_fecundity.set_Tobs(m_Tobs);
					}


					m_Tobs.compute_stats();
					m_devTime.compute_T_stats(m_Tobs);
				}


				if (m_fitType == F_DEV_TIME &&
					!(m_devTime.m_bIndividual || (m_devTime.have_var(I_MEAN_TIME) && m_devTime.have_var(I_TIME_SD) && m_devTime.have_var(I_N))))
				{
					msg.ajoute(string("Calibration of ") + TYPE_NAME[m_fitType] + " need individual Time or MeanTime, TimeSD and n");
				}

				//if (m_fitType == F_DEV_TIME_WTH_SIGMA && !bLogLikelyhoude)
				//{
				//	if (!(m_devTime.m_bIndividual || (m_devTime.have_var(I_MEAN_TIME) && m_devTime.have_var(I_TIME_SD) && m_devTime.have_var(I_N))))
				//	{
				//		msg.ajoute(string("Minimize residual sum of square for ") + TYPE_NAME[m_fitType] + " need individual Time or MeanTime, TimeSD and n.");
				//		return msg;
				//	}
				//}
				//if (m_fitType == F_DEV_TIME_ONLY && m_devTime.m_bIndividual)//base on mean rate only (no sigma)
				//{
				//	msg.ajoute(string("Calibration of \"") + TYPE_NAME[m_fitType] + "\" can not be used with individual Time. Use \"" + TYPE_NAME[F_DEV_TIME_WTH_SIGMA] + "\" instead or use MeanTime, TimeSD and n.");
				//	return msg;
				//}

				//if (m_fitType == F_DEV_TIME_ONLY && !m_devTime.m_bIndividual)//base on mean rate only (no sigma)
				//{
				//	if (!m_devTime.have_var(I_MEAN_TIME))
				//	{
				//		msg.ajoute(string("Calibration of ") + TYPE_NAME[m_fitType] + " need MeanTime.");
				//		return msg;
				//	}

				//	/*if (!(m_devTime.m_bIndividual || (m_devTime.have_var(I_MEAN_TIME))))
				//	{
				//		msg.ajoute(string("Calibration of ") + TYPE_NAME[m_fitType] + " need individual Time or MeanTime.");
				//		return msg;
				//	}*/
				//	//msg.ajoute(string("Maximize log likelihood is not available for ") + TYPE_NAME[m_fitType] + ". Use minize residual sum of square instead.");
				//	//return msg;
				//}

				std::map<std::string, std::map<std::string, std::map<std::string, double>>> output_params;
				if (m_bUseOutputAsInput)
				{
					string previousFilePath = fileManager.GetOutputPath() + m_outputAsIntputFileName;
					SetFileExtension(previousFilePath, ".csv");
					ReplaceString(previousFilePath, "%i", GetFileTitle(m_inputFileName));
					msg += ReadParametersFromFile(previousFilePath, output_params);
				}

				if (!msg)
					return msg;

				if (m_bFixeTb || m_bFixeTm)
				{
					for (size_t e = 0; e < m_eqDevRate.size() && msg; e++)
					{
						if (m_eqDevRate.test(e))
						{
							TDevRateEquation eq = CDevRateEquation::eq(e);
							string e_name = CDevRateEquation::GetEquationName(eq);

							CSAParameterVector params = CDevRateEquation::GetParameters(eq);

							auto it_Tb = find_if(params.begin(), params.end(), [](const CSAParameter& m) -> bool { return m.m_name == "Tb"; });
							auto it_Tm = find_if(params.begin(), params.end(), [](const CSAParameter& m) -> bool { return m.m_name == "Tm"; });
							if (m_bFixeTb && it_Tb == params.end() ||
								m_bFixeTm && it_Tm == params.end())
								m_eqDevRate.reset(e);//remove equation without Tb

						}
					}
				}


				CResult result;
				CFitOutputVector output;
				size_t nb_limits = 1;

				set<string> variables;

				if (m_fitType == F_DEV_TIME)//_WTH_SIGMA || m_fitType == F_DEV_TIME_ONLY
				{
					for (size_t s = 0; s < m_devTime.size(); s++)
						variables.insert(m_devTime[s].m_variable);
				}
				else if (m_fitType == F_SURVIVAL)
				{
					for (size_t s = 0; s < m_survival.size(); s++)
						variables.insert(m_survival[s].m_variable);
				}
				else if (m_fitType == F_FECUNDITY)
				{
					for (size_t s = 0; s < m_fecundity.size(); s++)
						variables.insert(m_fecundity[s].m_variable);
				}


				//for all stage
				for (auto v = variables.begin(); v != variables.end() && msg; v++)
				{
					//for all equation
					if (m_fitType == F_DEV_TIME)//_WTH_SIGMA || m_fitType == F_DEV_TIME_ONLY
					{
						//m_calibOn = m_fitType == F_DEV_TIME ? CO_TIME : CO_RATE;
						for (size_t e = 0; e < m_eqDevRate.size() && msg; e++)
						{
							if (m_eqDevRate.test(e))
							{
								TDevRateEquation eq = CDevRateEquation::eq(e);
								string e_name = CDevRateEquation::GetEquationName(eq);


								CSAParameterVector params0 = CDevRateEquation::GetParameters(eq);
								if (m_eq_options.find(e_name) != m_eq_options.end())
								{
									ASSERT(m_eq_options[e_name].size() == CDevRateEquation::GetParameters(eq).size());
									params0 = m_eq_options[e_name];
								}

								//if (m_fitType == F_DEV_TIME_WTH_SIGMA)// add relative development rate variance
								//{
								double sigma = m_devTime.GetDefaultSigma(*v);
								if (sigma == 0)
								{
									msg.ajoute("sigma=0. Invalid input Time or MeanTime/TimeSD.");
									return msg;
								}

								if (m_devTime.m_bIndividual)
								{
									params0.push_back(CSAParameter("sigma", sigma, sigma / SIGMA_FACTOR, sigma * SIGMA_FACTOR));
								}
								else
								{
									//fixe sigma here
									params0.push_back(CSAParameter("sigma", Round(sigma, 3), Round(sigma, 3), Round(sigma, 3)));
								}
								//}

								if (m_bUseOutputAsInput && !output_params.empty())
								{
									if (output_params.find(*v) != output_params.end() &&
										output_params[*v].find(e_name) != output_params[*v].end())
									{
										ASSERT(output_params[*v][e_name].size() >= CDevRateEquation::GetParameters(eq).size());
										for (auto it = output_params[*v][e_name].begin(); it != output_params[*v][e_name].end(); it++)
										{
											auto iit = find_if(params0.begin(), params0.end(), [&](CSAParameter& p) {return boost::iequals(p.m_name, it->first); });
											if (iit != params0.end())
											{
												//if sigma, round to avoid boubnd problem
												if (it->first != "sigma")
													iit->m_initialValue = it->second;
												else
													iit->m_initialValue = Round(it->second, 3);

											}
										}

									}
								}

								double Tk = (eq == CDevRateEquation::Schoolfield_1981 || eq == CDevRateEquation::Wagner_1988) ? 273.16 : 0;


								//if there is global lower/upper limit, set it
								size_t nb_Tb = (m_bFixeTb) ? ceil((m_Tb[1] - m_Tb[0]) / m_Tb[2]) + 1 : 1;
								size_t nb_Tm = (m_bFixeTm) ? ceil((m_Tm[1] - m_Tm[0]) / m_Tm[2]) + 1 : 1;
								nb_limits = nb_Tb * nb_Tm;


								for (size_t Tbi = 0; Tbi < nb_Tb; Tbi++)
								{
									CSAParameterVector params = params0;


									if (m_bFixeTb)
									{
										auto it = find_if(params.begin(), params.end(), [](const CSAParameter& m) -> bool { return m.m_name == "Tb"; });
										if (it != params.end())
										{
											it->m_initialValue = m_Tb[0] + Tbi * m_Tb[2] + Tk;
											it->m_bounds = CVariableBound(it->m_initialValue, it->m_initialValue);
										}
									}



									for (size_t Tmi = 0; Tmi < nb_Tm; Tmi++)
									{
										if (m_bFixeTm)
										{
											auto it = find_if(params.begin(), params.end(), [](const CSAParameter& m) -> bool { return m.m_name == "Tm"; });
											if (it != params.end())
											{
												it->m_initialValue = m_Tm[0] + Tmi * m_Tm[2] + Tk;
												it->m_bounds = CVariableBound(it->m_initialValue, it->m_initialValue);
											}
										}


										CFitOutput out(*v, eq, params);
										msg += InitialiseComputationVariable(out.m_variable, out.m_equation, out.m_parameters, out.m_computation, callback);
										output.push_back(out);
									}//for all Tm

								}//for all Tb
							}
						}
					}
					else if (m_fitType == F_SURVIVAL)
					{
						for (size_t e = 0; e < m_eqSurvival.size() && msg; e++)
						{
							if (m_eqSurvival.test(e))
							{
								TSurvivalEquation eq = CSurvivalEquation::eq(e);
								string e_name = CSurvivalEquation::GetEquationName(eq);

								CSAParameterVector params = CSurvivalEquation::GetParameters(eq);
								if (m_eq_options.find(e_name) != m_eq_options.end())
								{
									ASSERT(m_eq_options[e_name].size() == CSurvivalEquation::GetParameters(eq).size());
									params = m_eq_options[e_name];
								}

								if (m_bUseOutputAsInput && !output_params.empty())
								{
									if (output_params.find(*v) != output_params.end() &&
										output_params[*v].find(e_name) != output_params[*v].end())
									{
										ASSERT(output_params[*v][e_name].size() == CSurvivalEquation::GetParameters(eq).size());
										for (auto it = output_params[*v][e_name].begin(); it != output_params[*v][e_name].end(); it++)
										{
											auto iit = find_if(params.begin(), params.end(), [&](CSAParameter& p) {return boost::iequals(p.m_name, it->first); });
											if (iit != params.end())
												iit->m_initialValue = it->second;
										}

									}
								}

								CFitOutput out(*v, eq, params);
								msg += InitialiseComputationVariable(out.m_variable, out.m_equation, out.m_parameters, out.m_computation, callback);
								output.push_back(out);
							}
						}
					}
					else if (m_fitType == F_FECUNDITY)
					{
						for (size_t e = 0; e < m_eqFecundity.size() && msg; e++)
						{
							if (m_eqFecundity.test(e))
							{
								TDevRateEquation eq = CDevRateEquation::eq(e);
								string e_name = CDevRateEquation::GetEquationName(eq);


								CSAParameterVector params = CDevRateEquation::GetParameters(eq);
								if (m_eq_options.find(e_name) != m_eq_options.end())
								{
									ASSERT(m_eq_options[e_name].size() == CDevRateEquation::GetParameters(eq).size());
									params = m_eq_options[e_name];
								}

								if (m_bUseOutputAsInput && !output_params.empty())
								{
									if (output_params.find(*v) != output_params.end() &&
										output_params[*v].find(e_name) != output_params[*v].end())
									{
										ASSERT(output_params[*v][e_name].size() >= CDevRateEquation::GetParameters(eq).size());
										for (auto it = output_params[*v][e_name].begin(); it != output_params[*v][e_name].end(); it++)
										{
											auto iit = find_if(params.begin(), params.end(), [&](CSAParameter& p) {return boost::iequals(p.m_name, it->first); });
											if (iit != params.end())
												iit->m_initialValue = it->second;
										}

									}
								}


								//CSAParameter pto("to", 0, 0, 0);//fixe to at 0

								CSAParameter pFo("Fo", 100, 1, 1000);
								if (m_bFixeF0)
									pFo = CSAParameter("Fo", m_F0, m_F0, m_F0);


								if (m_fecundity.m_bIndividual || m_fecundity.m_bIndividualSeries)
								{
									//params.push_back(pto);
									params.push_back(pFo);
									params.push_back(CSAParameter("sigma", 0.2, 0.01, 0.9));
								}
								else
								{
									//fixe sigma here
									//params.push_back(pto);
									params.push_back(pFo);
									double sigma = m_fecundity.GetDefaultSigma();
									params.push_back(CSAParameter("sigma", sigma, sigma, sigma));
								}

								auto it = find_if(params.begin(), params.end(), [](const CSAParameter& m) -> bool { return m.m_name == "psi"; });
								if (it != params.end())
									it->m_bounds.m_upperBound = 10;


								CFitOutput out(*v, eq, params);
								msg += InitialiseComputationVariable(out.m_variable, out.m_equation, out.m_parameters, out.m_computation, callback);
								output.push_back(out);
							}
						}
					}
				}

				size_t nb_e = 0;
				if (m_fitType == F_DEV_TIME)//_WTH_SIGMA || m_fitType == F_DEV_TIME_ONLY
					nb_e = m_eqDevRate.count();
				else if (m_fitType == F_SURVIVAL)
					nb_e = m_eqSurvival.count();
				else if (m_fitType == F_FECUNDITY)
					nb_e = m_eqFecundity.count();


				if (!msg)
					return msg;


				callback.PushTask("Search optimum for " + to_string(TYPE_NAME[m_fitType]) + ": " + to_string(variables.size()) + " variables x " + to_string(nb_e) + " equations x " + to_string(nb_limits) + " fixed limits = " + to_string(output.size()) + " curve to fits", output.size());



#pragma omp parallel for num_threads(CTRL.m_nbMaxThreads) 
				for (__int64 i = 0; i < (__int64)output.size(); i++)
				{
#pragma omp flush(msg)
					if (msg)
					{
						msg += Optimize(output[i].m_variable, output[i].m_equation, output[i].m_parameters, output[i].m_computation, callback);

#pragma omp critical(WRITE_INFO)
						{
							if (m_fitType == F_DEV_TIME)//_WTH_SIGMA || m_fitType == F_DEV_TIME_ONLY
								callback.AddMessage(output[i].m_variable + ": " + CDevRateEquation::GetEquationName(CDevRateEquation::eq(output[i].m_equation)));
							else if (m_fitType == F_SURVIVAL)
								callback.AddMessage(output[i].m_variable + ": " + CSurvivalEquation::GetEquationName(CSurvivalEquation::eq(output[i].m_equation)));
							else if (m_fitType == F_FECUNDITY)
								callback.AddMessage(output[i].m_variable + ": " + CDevRateEquation::GetEquationName(CDevRateEquation::eq(output[i].m_equation)));


							WriteInfo(output[i].m_parameters, output[i].m_computation, callback);
						}


						//fileManager, result, callback
						msg += callback.StepIt();
#pragma omp flush(msg)
					}
				}

				callback.PopTask();



				callback.AddMessage(GetCurrentTimeString());
				std::string logText = GetOutputString(msg, callback, true);

				std::string filePath = GetLogFilePath(GetPath(fileManager));
				msg += WriteOutputMessage(filePath, logText);

				if (true)
				{
					//begin to read
					ofStream file;


					ERMsg msg_file = file.open(outputFilePath);
					if (msg_file)//save result event if user cancel or error
					{

						sort(output.begin(), output.end(), [](const CFitOutput& a, const CFitOutput& b) {return a.m_computation.m_Fopt > b.m_computation.m_Fopt; });
						if (bLogLikelyhoude)
							file << "Variable,EqName,P,Eq,Math,n,k,maxLL,AICc" << endl;
						else
							file << "Variable,EqName,P,Eq,Math,n,k,RSS,R2" << endl;

						for (auto v = variables.begin(); v != variables.end(); v++)
						{
							for (size_t i = 0; i < output.size(); i++)
							{
								if (output[i].m_variable == *v)
								{
									string name;
									string R_eq;
									string R_math;
									string P;
									const vector<double>& Xopt = output[i].m_computation.m_Xopt;


									if (m_fitType == F_DEV_TIME)//_WTH_SIGMA || m_fitType == F_DEV_TIME_ONLY
									{
										TDevRateEquation eq = CDevRateEquation::eq(output[i].m_equation);
										name = CDevRateEquation::GetEquationName(eq);
										R_eq = CDevRateEquation::GetEquationR(eq);
										R_math = CDevRateEquation::GetMathPlot(eq);
										P = to_string(CDevRateEquation::GetParameters(eq, Xopt));

										//if (m_fitType == F_DEV_TIME_WTH_SIGMA)
										//{
										P += " sigma=" + to_string(Xopt.back());
										//}
									}
									else if (m_fitType == F_SURVIVAL)
									{
										TSurvivalEquation eq = CSurvivalEquation::eq(output[i].m_equation);
										name = CSurvivalEquation::GetEquationName(eq);
										R_eq = CSurvivalEquation::GetEquationR(eq);
										R_math = CSurvivalEquation::GetMathPlot(eq);
										P = to_string(CSurvivalEquation::GetParameters(eq, Xopt));
									}
									else if (m_fitType == F_FECUNDITY)
									{
										TDevRateEquation eq = CDevRateEquation::eq(output[i].m_equation);
										name = CDevRateEquation::GetEquationName(eq);
										R_eq = CDevRateEquation::GetEquationR(eq);
										R_math = CDevRateEquation::GetMathPlot(eq);
										P = to_string(CDevRateEquation::GetParameters(eq, Xopt));

										//P += " to=" + to_string(Xopt[Xopt.size() - 3]);
										P += " Fo=" + to_string(Xopt[Xopt.size() - 2]);
										P += " sigma=" + to_string(Xopt[Xopt.size() - 1]);
									}


									file << output[i].m_variable << "," << name << "," << P << ",\"" << R_eq << "\",\"" << R_math << "\",";

									if (bLogLikelyhoude)
										file << output[i].m_computation.m_n_opt << "," << output[i].m_computation.m_k_opt << "," << output[i].m_computation.m_MLLopt << "," << output[i].m_computation.m_AICCopt;
									else //if (output[i].m_computation.m_Fopt != m_ctrl.GetVMiss())
										file << output[i].m_computation.m_Sopt[NB_VALUE] << "," << output[i].m_computation.m_Xopt.size() << "," << output[i].m_computation.m_Sopt[RSS] << "," << output[i].m_computation.m_Sopt[STAT_R²];
									//else
									//	file << "0,0,0,0";

									file << endl;
								}
							}
						}

						file.close();
					}

					msg += msg_file;
				}


			}

			resultDB.Close();
		}

		m_devTime.clear();
		m_Tobs.clear();
		m_devTime.clear();
		m_survival.clear();
		m_fecundity.clear();


		return msg;
	}



	//Initialize input parameter
	//user can override theses methods
	ERMsg CInsectParameterization::InitialiseComputationVariable(std::string s, size_t e, const CSAParameterVector& parameters, CComputationVariable& computation, CCallback& callback)
	{
		ERMsg msg;

		computation.m_bounds.resize(parameters.size());
		computation.m_C.resize(parameters.size());
		computation.m_X.resize(parameters.size());
		computation.m_XP.resize(parameters.size());
		computation.m_XPstat.resize(parameters.size());
		computation.m_VM.resize(parameters.size());
		computation.m_VMstat.resize(parameters.size());


		for (size_t i = 0; i < parameters.size(); i++)
		{
			computation.m_bounds[i] = parameters[i].m_bounds;
			computation.m_XP[i] = parameters[i].m_initialValue;
			computation.m_C[i] = 2;
			computation.m_VM[i] = computation.m_bounds[i].GetExtent();
			//ASSERT(!computation.m_bounds[i].IsOutOfBound(computation.m_XP[i]));
			//If the initial value is out of bounds, notify the user and return
			//to the calling routine.
			if (computation.m_bounds[i].IsOutOfBound(computation.m_XP[i]))
			{
				if (m_fitType == F_DEV_TIME)//_WTH_SIGMA || m_fitType == F_DEV_TIME_ONLY
					msg.ajoute(s + ": " + CDevRateEquation::GetEquationName(CDevRateEquation::eq(e)));
				else if (m_fitType == F_SURVIVAL)
					msg.ajoute(s + ": " + CSurvivalEquation::GetEquationName(CSurvivalEquation::eq(e)));
				else if (m_fitType == F_FECUNDITY)
					msg.ajoute(s + ": " + CDevRateEquation::GetEquationName(CDevRateEquation::eq(e)));


				msg.ajoute("The starting value (" + ToString(computation.m_XP[i]) + ") is not inside the bounds [" + ToString(computation.m_bounds[i].GetLowerBound()) + "," + ToString(computation.m_bounds[i].GetUpperBound()) + "].");
				return msg;
			}
		}

		computation.Initialize(m_ctrl.T(), m_ctrl.NEPS(), m_ctrl.GetVMiss());


		size_t x = 0;
		static const size_t NB_TRY_I = 10000;

		bool bValid = GetFValue(s, e, computation);
		while (x < NB_TRY_I && !bValid)//try to find valid parameters when initial parameters is invalid
		{
			//  Generate XP, the trial value of X. Note use of VM to choose XP.
			for (size_t i = 0; i < computation.m_X.size(); i++)
			{
				computation.m_XP[i] = max(parameters[i].m_bounds.GetLowerBound(), min(parameters[i].m_bounds.GetUpperBound(), parameters[i].m_initialValue * RandLogNormal(0, double(x) / NB_TRY_I)));

				if (parameters[i].m_name == "Tb" && m_bFixeTb)
					computation.m_XP[i] = m_Tb[0] + (m_Tb[1] - m_Tb[0]) * Randu();
				if (parameters[i].m_name == "Tm" && m_bFixeTm)
					computation.m_XP[i] = m_Tm[0] + (m_Tm[1] - m_Tm[0]) * Randu();


			}

			bValid = GetFValue(s, e, computation);
			x++;
		}

		//try with random values
		static const size_t NB_TRY_II = 10000;
		x = 0;
		while (x < NB_TRY_II && !bValid)//try to find valid parameters when initial parameters is invalid
		{
			//  Generate XP, the trial value of X. Note use of VM to choose XP.
			for (size_t i = 0; i < computation.m_X.size(); i++)
			{
				computation.m_XP[i] = parameters[i].m_bounds.GetLowerBound() + parameters[i].m_bounds.GetExtent() * Randu();
				if (parameters[i].m_name == "Tb" && m_bFixeTb)
					computation.m_XP[i] = m_Tb[0] + (m_Tb[1] - m_Tb[0]) * Randu();
				if (parameters[i].m_name == "Tm" && m_bFixeTm)
					computation.m_XP[i] = m_Tm[0] + (m_Tm[1] - m_Tm[0]) * Randu();

			}

			bValid = GetFValue(s, e, computation);
			x++;
		}




		computation.m_NFCNEV++;

		computation.m_X = computation.m_XP;
		computation.m_Xopt = computation.m_XP;

		if (computation.m_FP != m_ctrl.GetVMiss())
		{
			computation.m_S = computation.m_SP;
			computation.m_F = computation.m_FP;
			computation.m_AICC = computation.m_AICCP;
			computation.m_MLL = computation.m_MLLP;
			computation.m_n = computation.m_n_P;
			computation.m_k = computation.m_k_P;

			computation.m_Sopt = computation.m_SP;
			computation.m_Fopt = computation.m_FP;
			computation.m_AICCopt = computation.m_AICCP;
			computation.m_MLLopt = computation.m_MLLP;
			computation.m_n_opt = computation.m_n_P;
			computation.m_k_opt = computation.m_k_P;

			computation.m_FSTAR[0] = computation.m_FP;
		}

		return msg;
	}


	double CInsectParameterization::Exprep(const double& RDUM)
	{
		//  This function replaces exp to avoid under- and overflows and is
		//  designed for IBM 370 type machines. It may be necessary to modify
		//  it for other machines. Note that the maximum and minimum values of
		//  EXPREP are such that they has no effect on the algorithm.

		double EXPREP = 0;

		if (RDUM > 174.)
		{
			EXPREP = 3.69E+75;
		}
		else if (RDUM < -180.)
		{
			EXPREP = 0.0;
		}
		else
		{
			EXPREP = exp(RDUM);
		}

		return EXPREP;
	}



	void CInsectParameterization::WriteInfo(const CSAParameterVector& parameters, const CComputationVariable& computation, CCallback& callback)
	{
		string line;

		double F = m_ctrl.Max() ? computation.m_Fopt : -computation.m_Fopt;
		bool bLogLikelyhoude = m_ctrl.m_statisticType == LIKELIHOOD;

		CStatistic stat;
		for (size_t i = 0, j = 0; i < computation.m_Xstat.size(); i++)
			stat += 100.0 * computation.m_Xstat[j][RANGE] / computation.m_Xstat[j][MEAN];

		if (bLogLikelyhoude)
			line = FormatA("N=%10d\tT=%12.8f\tF=%8.5lf\tP=%8.5lf\nNbVal=%d\tAICc=%8.5lf\tmaxLL=%8.5lf", computation.m_NFCNEV, computation.m_T, F, stat[HIGHEST], computation.m_n_opt, computation.m_AICCopt, computation.m_MLLopt);
		else if (computation.m_Sopt[NB_VALUE] > 0)
			line = FormatA("N=%10d\tT=%12.8f\tF=%8.5lf\tP=%8.5lf\nNbVal=%6.0lf\tBias=%8.5lf\tMAE=%8.5lf\tRMSE=%8.5lf\tCD=%8.5lf\tR²=%8.5lf", computation.m_NFCNEV, computation.m_T, F, stat[HIGHEST], computation.m_Sopt[NB_VALUE], computation.m_Sopt[BIAS], computation.m_Sopt[MAE], computation.m_Sopt[RMSE], computation.m_Sopt[COEF_D], computation.m_Sopt[STAT_R²]);
		else
			line = "No optimum find yet...";



		callback.AddMessage(line);


		line.clear();
		if (computation.m_Xopt.size() == parameters.size())
		{

			bool bShowRange = !computation.m_XPstat.empty() && computation.m_XPstat[0][NB_VALUE] > 0;
			for (size_t j = 0; j < parameters.size(); j++)
			{
				string name = parameters[j].m_name; Trim(name);
				string tmp;

				if (bShowRange)
				{
					tmp = FormatA("% -20.20s\t=%10.5lf {%10.5lf,%10.5lf}\tVM={%10.5lf,%10.5lf}\n", name.c_str(), computation.m_Xopt[j], computation.m_XPstat[j][LOWEST], computation.m_XPstat[j][HIGHEST], computation.m_VMstat[j][LOWEST], computation.m_VMstat[j][HIGHEST]);
				}
				else
				{
					tmp = FormatA("%s = %5.3lg  ", name.c_str(), computation.m_Xopt[j]);
				}

				line += tmp;
			}

			callback.AddMessage(line);
		}
	}

	void CInsectParameterization::WriteInfoEx(const CSAParameterVector& parameters, const CComputationVariable& computation, CCallback& callback)
	{
		string line;
		double F = m_ctrl.Max() ? computation.m_Fopt : -computation.m_Fopt;
		bool bLogLikelyhoude = m_ctrl.m_statisticType == LIKELIHOOD;

		CStatistic stat;
		for (size_t i = 0, j = 0; i < computation.m_XPstat.size(); i++)
			stat += 100.0 * computation.m_XPstat[j][RANGE] / computation.m_XPstat[j][MEAN];

		if (bLogLikelyhoude)
			line = FormatA("N=%10d\tT=%12.8f\tF=%8.5lf\tP=%8.5lf\nNbVal=%d\tAICc=%8.5lf\tmaxLL=%8.5lf", computation.m_NFCNEV, computation.m_T, F, stat[HIGHEST], computation.m_n_opt, computation.m_AICCopt, computation.m_MLLopt);
		else if (computation.m_Sopt[NB_VALUE] > 0)
			line = FormatA("N=%10d\tT=%12.8f\tF=%8.5lf\tP=%8.5lf\nNbVal=%6.0lf\tBias=%8.5lf\tMAE=%8.5lf\tRMSE=%8.5lf\tCD=%8.5lf\tR²=%8.5lf", computation.m_NFCNEV, computation.m_T, F, stat[HIGHEST], computation.m_Sopt[NB_VALUE], computation.m_Sopt[BIAS], computation.m_Sopt[MAE], computation.m_Sopt[RMSE], computation.m_Sopt[COEF_D], computation.m_Sopt[STAT_R²]);
		else
			line = "No optimum find yet...";



		callback.AddMessage(line);


		line.clear();
		bool bShowRange = !computation.m_XPstat.empty() && computation.m_XPstat[0][NB_VALUE] > 0;
		for (size_t i = 0, j = 0; i < parameters.size(); i++)
		{
			string name = parameters[i].m_name; Trim(name);
			string tmp;

			if (bShowRange)
			{
				tmp = FormatA("% -20.20s\t=%10.5lf {%10.5lf,%10.5lf}\tVM={%10.5lf,%10.5lf}\n", name.c_str(), computation.m_Xopt[j], computation.m_XPstat[j][LOWEST], computation.m_XPstat[j][HIGHEST], computation.m_VMstat[j][LOWEST], computation.m_VMstat[j][HIGHEST]);
			}
			else
			{
				tmp = FormatA("% -20.20s\t=%10.5lf  ", name.c_str(), computation.m_Xopt[j]);
			}

			line += tmp;
			j++;
		}

		callback.AddMessage(line);
		line.clear();


		/*double eps = fabs(computation.m_Fopt - computation.m_F);
		line = "Eps = [" + ToString(eps, -1) + "]{";

		for (int i = 0; i < computation.m_FSTAR.size(); i++)
		{

			string tmp;
			double eps = fabs(computation.m_F - computation.m_FSTAR[i]);

			if (eps < 10e6)
				tmp = FormatA("%8.5lf", eps);
			else tmp = "-----";


			if (i > 0)
				line += ", ";

			line += tmp;
		}

		line += "}";
*/
//callback.AddMessage(line);

//callback.AddMessage("***********************************");
//callback.StepIt(0);


//clean X stats
//for (size_t i = 0; i < computation.m_XPstat.size(); i++)
	//computation.m_XPstat[i].Reset();
	}


	std::string CInsectParameterization::GetPath(const CFileManager& fileManager)const
	{
		if (m_pParent == NULL)
			return fileManager.GetTmpPath() + m_internalName + "\\";

		return m_pParent->GetPath(fileManager) + m_internalName + "\\";
	}


	//**********************************************************************
	//CRandomizeNumber

	//  Version: 3.2
	//  Date: 1/22/94.
	//  Differences compared to Version 2.0:
	//     1. If a trial is out of bounds, a point is randomly selected
	//        from LB(i) to UB(i). Unlike in version 2.0, this trial is
	//        evaluated and is counted in acceptances and rejections.
	//        All corresponding documentation was changed as well.
	//  Differences compared to Version 3.0:
	//     1. If VM(i) > (UB(i) - LB(i)), VM is set to UB(i) - LB(i).
	//        The idea is that if T is high relative to LB & UB, most
	//        points will be accepted, causing VM to rise. But, in this
	//        situation, VM has little meaning; particularly if VM is
	//        larger than the acceptable region. Setting VM to this size
	//        still allows all parts of the allowable region to be selected.
	//  Differences compared to Version 3.1:
	//     1. Test made to see if the initial temperature is positive.
	//     2. WRITE statements prettied up.
	//     3. References to paper updated.
	//
	//  Synopsis:
	//  This routine implements the continuous simulated annealing global
	//  optimization algorithm described in Corana et al.'s article
	//  "Minimizing Multimodal Functions of Continuous Variables with the
	//  "Simulated Annealing" Algorithm" in the September 1987 (vol. 13,
	//  no. 3, pp. 262-280) issue of the ACM Transactions on Mathematical
	//  Software.
	//
	//  A very quick (perhaps too quick) overview of SA:
	//     SA tries to find the global optimum of an N dimensional function.
	//  It moves both up and downhill and as the optimization process
	//  proceeds, it focuses on the most promising area.
	//     To start, it randomly chooses a trial point within the step length
	//  VM (a vector of length N) of the user selected starting point. The
	//  function is evaluated at this trial point and its value is compared
	//  to its value at the initial point.
	//     In a maximization problem, all uphill moves are accepted and the
	//  algorithm continues from that trial point. Downhill moves may be
	//  accepted; the decision is made by the Metropolis criteria. It uses T
	//  (temperature) and the size of the downhill move in a probabilistic
	//  manner. The smaller T and the size of the downhill move are, the more
	//  likely that move will be accepted. If the trial is accepted, the
	//  algorithm moves on from that point. If it is rejected, another point
	//  is chosen instead for a trial evaluation.
	//     Each element of VM periodically adjusted so that half of all
	//  function evaluations in that direction are accepted.
	//     A fall in T is imposed upon the system with the RT variable by
	//  T(i+1) = RT*T(i) where i is the ith iteration. Thus, as T declines,
	//  downhill moves are less likely to be accepted and the percentage of
	//  rejections rise. Given the scheme for the selection for VM, VM falls.
	//  Thus, as T declines, VM falls and SA focuses upon the most promising
	//  area for optimization.
	//
	//  The importance of the parameter T:
	//     The parameter T is crucial in using SA successfully. It influences
	//  VM, the step length over which the algorithm searches for optima. For
	//  a small intial T, the step length may be too small; thus not enough
	//  of the function might be evaluated to find the global optima. The user
	//  should carefully examine VM in the intermediate output (set IPRINT =
	//  1) to make sure that VM is appropriate. The relationship between the
	//  initial temperature and the resulting step length is function
	//  dependent.
	//     To determine the starting temperature that is consistent with
	//  optimizing a function, it is worthwhile to run a trial run first. Set
	//  RT = 1.5 and T = 1.0. With RT > 1.0, the temperature increases and VM
	//  rises as well. Then select the T that produces a large enough VM.
	//
	//  For modifications to the algorithm and many details on its use,
	//  (particularly for econometric applications) see Goffe, Ferrier
	//  and Rogers, "Global Optimization of Statistical Functions with
	//  Simulated Annealing," Journal of Econometrics, vol. 60, no. 1/2, 
	//  Jan./Feb. 1994, pp. 65-100.
	//  For more information, contact 
	//              Bill Goffe
	//              Department of Economics and International Business
	//              University of Southern Mississippi 
	//              Hattiesburg, MS  39506-5072 
	//              (601) 266-4484 (office)
	//              (601) 266-4920 (fax)
	//              bgoffe@whale.st.usm.edu (Internet)
	//
	//  As far as possible, the parameters here have the same name as in
	//  the description of the algorithm on pp. 266-8 of Corana et al.
	//
	//  In this description, SP is single precision, DP is double precision,
	//  INT is integer, L is logical and (N) denotes an array of length n.
	//  Thus, DP(N) denotes a double precision array of length n.
	//
	//  Input Parameters:
	//    Note: The suggested values generally come from Corana et al. To
	//          drastically reduce runtime, see Goffe et al., pp. 90-1 for
	//          suggestions on choosing the appropriate RT and NT.
	//    N - Number of variables in the function to be optimized. (INT)
	//    X - The starting values for the variables of the function to be
	//        optimized. (DP(N))
	//    MAX - Denotes whether the function should be maximized or
	//          minimized. A true value denotes maximization while a false
	//          value denotes minimization. Intermediate output (see IPRINT)
	//          takes this into account. (L)
	//    RT - The temperature reduction factor. The value suggested by
	//         Corana et al. is .85. See Goffe et al. for more advice. (DP)
	//    EPS - Error tolerance for termination. If the final function
	//          values from the last neps temperatures differ from the
	//          corresponding value at the current temperature by less than
	//          EPS and the final function value at the current temperature
	//          differs from the current optimal function value by less than
	//          EPS, execution terminates and IER = 0 is returned. (EP)
	//    NS - Number of cycles. After NS*N function evaluations, each
	//         element of VM is adjusted so that approximately half of
	//         all function evaluations are accepted. The suggested value
	//         is 20. (INT)
	//    NT - Number of iterations before temperature reduction. After
	//         NT*NS*N function evaluations, temperature (T) is changed
	//         by the factor RT. Value suggested by Corana et al. is
	//         MAX(100, 5*N). See Goffe et al. for further advice. (INT)
	//    NEPS - Number of final function values used to decide upon termi-
	//           nation. See EPS. Suggested value is 4. (INT)
	//    MAXEVL - The maximum number of function evaluations. If it is
	//             exceeded, IER = 1. (INT)
	//    LB - The lower bound for the allowable solution variables. (DP(N))
	//    UB - The upper bound for the allowable solution variables. (DP(N))
	//         If the algorithm chooses X(I) .LT. LB(I) or X(I) .GT. UB(I),
	//         I = 1, N, a point is from inside is randomly selected. This
	//         This focuses the algorithm on the region inside UB and LB.
	//         Unless the user wishes to concentrate the search to a par-
	//         ticular region, UB and LB should be set to very large positive
	//         and negative values, respectively. Note that the starting
	//         vector X should be inside this region. Also note that LB and
	//         UB are fixed in position, while VM is centered on the last
	//         accepted trial set of variables that optimizes the function.
	//    C - Vector that controls the step length adjustment. The suggested
	//        value for all elements is 2.0. (DP(N))
	//    IPRINT - controls printing inside SA. (INT)
	//             Values: 0 - Nothing printed.
	//                     1 - Function value for the starting value and
	//                         summary results before each temperature
	//                         reduction. This includes the optimal
	//                         function value found so far, the total
	//                         number of moves (broken up into uphill,
	//                         downhill, accepted and rejected), the
	//                         number of out of bounds trials, the
	//                         number of new optima found at this
	//                         temperature, the current optimal X and
	//                         the step length VM. Note that there are
	//                         N*NS*NT function evalutations before each
	//                         temperature reduction. Finally, notice is
	//                         is also given upon achieveing the termination
	//                         criteria.
	//                     2 - Each new step length (VM), the current optimal
	//                         X (XOPT) and the current trial X (X). This
	//                         gives the user some idea about how far X
	//                         strays from XOPT as well as how VM is adapting
	//                         to the function.
	//                     3 - Each function evaluation, its acceptance or
	//                         rejection and new optima. For many problems,
	//                         this option will likely require a small tree
	//                         if hard copy is used. This option is best
	//                         used to learn about the algorithm. A small
	//                         value for MAXEVL is thus recommended when
	//                         using IPRINT = 3.
	//             Suggested value: 1
	//             Note: For a given value of IPRINT, the lower valued
	//                   options (other than 0) are utilized.
	//    ISEED1 - The first seed for the random number generator RANMAR.
	//             0 <= ISEED1 <= 31328. (INT)
	//    ISEED2 - The second seed for the random number generator RANMAR.
	//             0 <= ISEED2 <= 30081. Different values for ISEED1
	//             and ISEED2 will lead to an entirely different sequence
	//             of trial points and decisions on downhill moves (when
	//             maximizing). See Goffe et al. on how this can be used
	//             to test the results of SA. (INT)
	//
	//  Input/Output Parameters:
	//    T - On input, the initial temperature. See Goffe et al. for advice.
	//        On output, the final temperature. (DP)
	//    VM - The step length vector. On input it should encompass the
	//         region of interest given the starting value X. For point
	//         X(I), the next trial point is selected is from X(I) - VM(I)
	//         to  X(I) + VM(I). Since VM is adjusted so that about half
	//         of all points are accepted, the input value is not very
	//         important (i.e. is the value is off, SA adjusts VM to the
	//         correct value). (DP(N))
	//
	//  Output Parameters:
	//    XOPT - The variables that optimize the function. (DP(N))
	//    FOPT - The optimal value of the function. (DP)
	//    NACC - The number of accepted function evaluations. (INT)
	//    NFCNEV - The total number of function evaluations. In a minor
	//             point, note that the first evaluation is not used in the
	//             core of the algorithm; it simply initializes the
	//             algorithm. (INT).
	//    NOBDS - The total number of trial function evaluations that
	//            would have been out of bounds of LB and UB. Note that
	//            a trial point is randomly selected between LB and UB.
	//            (INT)
	//    IER - The error return number. (INT)
	//          Values: 0 - Normal return; termination criteria achieved.
	//                  1 - Number of function evaluations (NFCNEV) is
	//                      greater than the maximum number (MAXEVL).
	//                  2 - The starting value (X) is not inside the
	//                      bounds (LB and UB).
	//                  3 - The initial temperature is not positive.
	//                  99 - Should not be seen; only used internally.
	//
	//  Work arrays that must be dimensioned in the calling routine:
	//       RWK1 (DP(NEPS))  (FSTAR in SA)
	//       RWK2 (DP(N))     (XP    "  " )
	//       IWK  (INT(N))    (NACP  "  " )
	//
	//  Required Functions (included):
	//    EXPREP - Replaces the function EXP to avoid under- and overflows.
	//             It may have to be modified for non IBM-type main-
	//             frames. (DP)
	//    RMARIN - Initializes the random number generator RANMAR.
	//    RANMAR - The actual random number generator. Note that
	//             RMARIN must run first (SA does this). It produces uniform
	//             random numbers on [0,1]. These routines are from
	//             Usenet's comp.lang.fortran. For a reference, see
	//             "Toward a Universal Random Number Generator"
	//             by George Marsaglia and Arif Zaman, Florida State
	//             University Report: FSU-SCRI-87-50 (1987).
	//             It was later modified by F. James and published in
	//             "A Review of Pseudo-random Number Generators." For
	//             further information, contact stuart@ads.com. These
	//             routines are designed to be portable on any machine
	//             with a 24-bit or more mantissa. I have found it produces
	//             identical results on a IBM 3081 and a Cray Y-MP.
	//
	//  Required Subroutines (included):
	//    PRTVEC - Prints vectors.
	//    PRT1 ... PRT10 - Prints intermediate output.
	//    FCN - Function to be optimized. The form is
	//            SUBROUTINE FCN(N,X,F)
	//            INTEGER N
	//            DOUBLE PRECISION  X(N), F
	//            ...
	//            function code with F = F(X)
	//            ...
	//            RETURN
	//            END
	//          Note: This is the same form used in the multivariable
	//          minimization algorithms in the IMSL edition 10 library.
	//
	//  Machine Specific Features:
	//    1. EXPREP may have to be modified if used on non-IBM type main-
	//       frames. Watch for under- and overflows in EXPREP.
	//    2. Some FORMAT statements use G25.18; this may be excessive for
	//       some machines.
	//    3. RMARIN and RANMAR are designed to be protable; they should not
	//       cause any problems.
	ERMsg CInsectParameterization::Optimize(string s, size_t  e, CSAParameterVector& parameters, CComputationVariable& computation, CCallback& callback)
	{
		ERMsg msg;


		if (m_bShowTrace)
			WriteInfoEx(parameters, computation, callback);

		//  Initialize the random number generator RANMAR.
		CRandomizeNumber random(m_ctrl.Seed1(), m_ctrl.Seed2());

		bool bQuit = false;

		//  Start the main loop. Note that it terminates if :
		//(i) the algorithm successfully optimizes the function 
		//(ii) there are too many function evaluations (more than MAXEVL).
		int L = 0;
		do
		{
			L++;

			long NUP = 0;
			long NREJ = 0;
			long NNEW = 0;
			long NDOWN = 0;
			long LNOBDS = 0;

			int NT = L <= m_ctrl.m_nbSkipLoop ? 2 : m_ctrl.m_NT;
			for (int M = 0; M < NT && msg; M++)
			{
				vector<int> NACP;
				NACP.insert(NACP.begin(), computation.m_X.size(), 0);

				int NS = L <= m_ctrl.m_nbSkipLoop ? 2 : m_ctrl.m_NS;
				for (size_t j = 0; j < NS && msg; j++)
				{

					for (size_t h = 0; h < NACP.size() && msg; h++)
					{
						//  If too many function evaluations occur, terminate the algorithm.
						if (computation.m_NFCNEV >= m_ctrl.MAXEVL())
						{
							callback.AddMessage("Number of function evaluations (NFCNEV) is greater than the maximum number (MAXEVL).");
							//msg.ajoute();
							return msg;
						}

						computation.m_XP.resize(computation.m_X.size());
						computation.m_SP.Reset();
						computation.m_FP = m_ctrl.GetVMiss();



						size_t x = 0;
						bool bValid = true;
						do
						{
							x++;
							//  Generate XP, the trial value of X. Note use of VM to choose XP.
							for (size_t i = 0; i < computation.m_X.size(); i++)
							{
								if (i == h)
									computation.m_XP[i] = computation.m_X[i] + (random.Ranmar() * 2.0 - 1.0) * computation.m_VM[i];
								else
									computation.m_XP[i] = computation.m_X[i];


								//  If XP is out of bounds, select a point in bounds for the trial.
								if (computation.m_bounds[i].IsOutOfBound(computation.m_XP[i]))
								{
									computation.m_XP[i] = computation.m_bounds[i].GetLowerBound() + computation.m_bounds[i].GetExtent() * random.Ranmar();
									LNOBDS++;
									computation.m_NOBDS++;
								}
							}

							//  Evaluate the function with the trial point XP and return as FP.
							bValid = GetFValue(s, e, computation);
						} while (x < 10 && !bValid);//if parameters rejected, find new parameters


						//add X value to extreme XP statistic
						for (size_t i = 0; i < computation.m_XP.size() && i < computation.m_XPstat.size(); i++)
							computation.m_XPstat[i] += computation.m_XP[i];

						for (size_t i = 0; i < computation.m_VMstat.size(); i++)
							computation.m_VMstat[i] += computation.m_VM[i];



						//  Accept the new point if the function value increases.
						if (computation.m_FP != m_ctrl.GetVMiss())
						{
							if (computation.m_FP >= computation.m_F)
							{
								computation.m_X = computation.m_XP;
								computation.m_F = computation.m_FP;
								computation.m_AICC = computation.m_AICCP;
								computation.m_MLL = computation.m_MLLP;
								computation.m_S = computation.m_SP;
								computation.m_n = computation.m_n_P;
								computation.m_k = computation.m_k_P;


								computation.m_NACC++;
								NACP[h]++;
								NUP++;

								//  If greater than any other point, record as new optimum.
								if (computation.m_FP > computation.m_Fopt)
								{
									computation.m_Xopt = computation.m_XP;
									computation.m_Fopt = computation.m_FP;
									computation.m_AICCopt = computation.m_AICCP;
									computation.m_MLLopt = computation.m_MLLP;
									computation.m_MLL = computation.m_MLLP;
									computation.m_Sopt = computation.m_SP;
									computation.m_n_opt = computation.m_n_P;
									computation.m_k_opt = computation.m_k_P;
									NNEW++;
								}
							}
							//  If the point is lower, use the Metropolis criteria to decide on
							//  acceptance or rejection.
							else
							{
								double P = Exprep((computation.m_FP - computation.m_F) / computation.m_T);
								double PP = random.Ranmar();
								if (PP < P)
								{
									computation.m_X = computation.m_XP;
									computation.m_F = computation.m_FP;
									computation.m_AICC = computation.m_AICCP;
									computation.m_MLL = computation.m_MLLP;
									computation.m_S = computation.m_SP;
									computation.m_n = computation.m_n_P;
									computation.m_k = computation.m_k_P;


									computation.m_NACC++;
									NACP[h]++;
									NDOWN++;
								}
								else
								{
									NREJ = NREJ + 1;
								}
							} //if
						}
						else
						{
							NREJ = NREJ + 1;
							//Eliminate this evaluation??
							//h--;
						}

						computation.m_NFCNEV++;


						//if (msg)
						msg += callback.StepIt(0);
					} //H
				} //J



				//  Adjust VM so that approximately half of all evaluations are accepted.
				ASSERT(computation.m_VM.size() == NACP.size());
				for (int I = 0; I < computation.m_VM.size(); I++)
				{
					double RATIO = double(NACP[I]) / double(m_ctrl.m_NS);///????? a vérifier??? m_ctrl.m_NS ou NS???
					//double RATIO = double(NACP[I]) / double(m_ctrl.NS());
					if (RATIO > 0.6)
					{
						computation.m_VM[I] = computation.m_VM[I] * (1. + computation.m_C[I] * (RATIO - .6) / .4);
					}
					else if (RATIO < 0.4)
					{
						computation.m_VM[I] = computation.m_VM[I] / (1. + computation.m_C[I] * ((.4 - RATIO) / .4));
					}


					if (computation.m_VM[I] > computation.m_bounds[I].GetExtent())
					{
						computation.m_VM[I] = computation.m_bounds[I].GetExtent();
					}
				}//all VM
			}//M

			if (m_bShowTrace)
				WriteInfoEx(parameters, computation, callback);


			computation.m_Xstat = computation.m_XPstat;
			for (size_t i = 0; i < computation.m_XPstat.size(); i++)
				computation.m_XPstat[i].Reset();

			//clean VM stats
			for (size_t i = 0; i < computation.m_VMstat.size(); i++)
				computation.m_VMstat[i].Reset();

			//  Loop again.
			bQuit = fabs(computation.m_F - computation.m_Fopt) <= m_ctrl.EPS();
			for (int I = 0; I < computation.m_FSTAR.size() && bQuit; I++)
			{
				if (fabs(computation.m_F - computation.m_FSTAR[I]) > m_ctrl.EPS())
					bQuit = false;
			}

			//  If termination criteria is not met, prepare for another loop.
			computation.PrepareForAnotherLoop(m_ctrl.m_RT, m_ctrl.m_RT2, L);

		} while (!bQuit && msg);


		return msg;
	}


	bool CInsectParameterization::IsParamValid(const std::string& var, CDevRateEquation::TDevRateEquation eq, const std::vector<double>& P)
	{
		bool bValid = CDevRateEquation::IsParamValid(eq, P);
		if (bValid)
		{
			//double min_T = m_devTime.m_statsTByVariable.at(var)[LOWEST];
			//double max_T = m_devTime.m_statsTByVariable.at(var)[HIGHEST];

			////do not let null Tb and Tm inside observed T
			//CSAParameterVector params = CDevRateEquation::GetParameters(eq);
			//size_t Tb_pos = std::distance(params.begin(), find_if(params.begin(), params.end(), [](const CSAParameter& m) -> bool { return m.m_name == "Tb"; }));
			//size_t Tm_pos = std::distance(params.begin(), find_if(params.begin(), params.end(), [](const CSAParameter& m) -> bool { return m.m_name == "Tm"; }));
			//if (!m_bFixeTb && Tb_pos < params.size())
			//	bValid &= P[Tb_pos]<= min_T;
			//
			//if (!m_bFixeTm && Tm_pos < params.size())
			//	bValid &= P[Tm_pos]>=max_T;
		}

		return bValid;
	}

	bool CInsectParameterization::IsRateValid(const std::string& var, CDevRateEquation::TDevRateEquation eq, const std::vector<double>& P, double null_rate_threshold)
	{
		bool bValid = true;


		if (m_bConstrainTlo)
		{
			CStatistic stat_sim;

			for (double T = m_Tlo[0]; T < m_Tlo[1]; T += 0.25)
			{
				double rate = CDevRateEquation::GetRate(eq, P, T);//daily rate

				if (isnan(rate))
					return false;

				if (rate < null_rate_threshold)
					rate = 0;

				stat_sim += rate;
			}

			bValid &= stat_sim[LOWEST] <= 0 && stat_sim[HIGHEST] > 0;
		}


		if (m_bConstrainThi)
		{
			CStatistic stat_sim;


			for (double T = m_Thi[0]; T < m_Thi[1]; T += 0.25)
			{
				double rate = CDevRateEquation::GetRate(eq, P, T);//daily rate

				if (isnan(rate))
					return false;

				if (rate < null_rate_threshold)
					rate = 0;

				stat_sim += rate;
			}

			bValid &= stat_sim[LOWEST] <= 0 && stat_sim[HIGHEST] > 0;
		}



		if (m_bLimitMaxRate)
		{

			//double min_T = m_devTime.m_statsTByVariable.at(var)[LOWEST];
			//double max_T = m_devTime.m_statsTByVariable.at(var)[HIGHEST];
			//if (m_bConstrainTlo && m_Tlo[0] < min_T)
			//	min_T = m_Tlo[0];
			//
			//if (m_bConstrainThi && m_Thi[1] > max_T)
			//	max_T = m_Thi[1];

			//min_T -= (max_T - min_T) / 5.0;
			//max_T += (max_T - min_T) / 5.0;

			//this option must not be use with aging 
			double min_T = -5;
			double max_T = 50;


			CStatistic stat_sim;
			for (double T = min_T; T < max_T; T += 0.25)
			{
				double rate = max(0.0, CDevRateEquation::GetRate(eq, P, T));//daily rate
				if (isnan(rate))
					return false;
				stat_sim += rate;
			}



			double K = m_LimitMaxRateP;
			double max_rate_obs = m_devTime.m_statsRateByVariable.at(var)[HIGHEST];
			double max_rate_sim = stat_sim[HIGHEST];

			double k = max_rate_sim / max_rate_obs;
			bValid &= k <= K;
			//bValid &= max_rate_obs / max_rate_sim <= K;
		}

		if (m_bAvoidNullRateInTobs)
		{
			double min_T = m_devTime.m_statsTByVariable.at(var)[LOWEST];
			double max_T = m_devTime.m_statsTByVariable.at(var)[HIGHEST];

			CStatistic stat_sim;
			for (double T = min_T; T <= max_T; T += 0.25)
			{
				double rate = max(0.0, CDevRateEquation::GetRate(eq, P, T));//daily rate

				if (isnan(rate))
					return false;

				stat_sim += rate;
			}

			double min_rate_sim = stat_sim[LOWEST];
			bValid &= min_rate_sim > null_rate_threshold;
		}


		return bValid;
	}

	bool CInsectParameterization::GetFValue(string var, size_t e, CComputationVariable& computation)
	{
		//bool bValid = false;
		bool bLogLikelyhoude = m_ctrl.m_statisticType == LIKELIHOOD;
		double log_likelyhoude = 0;
		size_t N_likelyhoude = 0;
		CStatisticXYEx stat;


		if (m_fitType == F_DEV_TIME)
		{
			TDevRateEquation eq = CDevRateEquation::eq(e);

			if (!IsParamValid(var, eq, computation.m_XP) || !IsRateValid(var, eq, computation.m_XP, NULL_RATE_THRESHOLD))
				return false;

			double sigma = computation.m_XP.back();

			//for all treatment
			for (auto it = m_devTime.m_pos[var].begin(); it != m_devTime.m_pos[var].end(); it++)
			{
				const vector<size_t>& pos = it->second;
				if (m_devTime.m_bIndividual)//use individual time
				{

					map<double, double> t_xi;
					for (auto iit = pos.begin(); iit != pos.end(); iit++)
					{
						if (iit == pos.begin())
						{
							ASSERT(m_devTime[*iit].t(true) >= 0);
							t_xi[m_devTime[*iit].t(true)] = 0;
						}

						ASSERT(m_devTime[*iit].t() > 0);
						t_xi[m_devTime[*iit].t()] = 0;
					}

					if (!Regniere2021DevRate(eq, computation.m_XP, m_devTime[pos.front()], t_xi))
						return false;

					//compute probability of changing stage between ti-1 and ti
					boost::math::lognormal_distribution<double> LogNormal(-0.5 * Square(sigma), sigma);


					//for (auto it = m_devTime.begin(); it != m_devTime.end(); it++)
					for (auto iit = pos.begin(); iit != pos.end(); iit++)
					{
						if (bLogLikelyhoude)
						{
							double xi = t_xi[m_devTime[*iit].t()];
							double xiˉ¹ = t_xi[m_devTime[*iit].t(true)];


							double p = (isfinite(xi) && isfinite(xiˉ¹)) ? cdf(LogNormal, xi) - cdf(LogNormal, xiˉ¹) : DBL_MIN;
							double LL = log(max(DBL_MIN, p));//max: avoid log of 0
							double n = m_devTime[*iit].at(I_N);
							LL *= n;///??????

							log_likelyhoude += LL;
							N_likelyhoude += n;
						}
						else
						{
							if (m_devTime[*iit][I_TIME] > 0)//Use only non NA
							{
								double obs = m_devTime[*iit][I_TIME];
								double sim = 1.0 / max(0.001, GetDevRate(eq, computation.m_XP, m_devTime[*iit]));

								if (!isfinite(sim) || isnan(sim) || sim < -1E8 || sim>1E8)
									return false;

								for (size_t n = 0; n < m_devTime[*iit][I_N]; n++)
									stat.Add(sim, obs);
							}
						}
					}
				}
				else//use mean
				{
					for (auto iit = pos.begin(); iit != pos.end(); iit++)
					{
						if (m_devTime[*iit][I_MEAN_TIME] > 0)//Use only non NA
						{
							if (bLogLikelyhoude)
							{
								//LL seem to not have to be multiply by N!
								double LL = Regniere2021DevRateMeanSDn(eq, computation.m_XP, m_devTime[*iit]);
								double n = m_devTime[*iit].at(I_N);

								log_likelyhoude += LL;
								N_likelyhoude += n;//validate with Jacques
							}
							else
							{
								double obs = m_devTime[*iit][I_MEAN_TIME];
								double sim = 1.0 / max(0.001, GetDevRate(eq, computation.m_XP, m_devTime[*iit]));

								if (!isfinite(sim) || isnan(sim) || sim < -1E8 || sim>1E8)
									return false;

								for (size_t n = 0; n < m_devTime[*iit][I_N]; n++)
									stat.Add(sim, obs);
							}
						}
					}
				}
			}
		}
		else if (m_fitType == F_SURVIVAL)
		{
			TSurvivalEquation eq = CSurvivalEquation::eq(e);
			if (CSurvivalEquation::IsParamValid(eq, computation.m_XP))
			{
				for (__int64 i = 0; i < (__int64)m_survival.size(); i++)
				{
					//if (WBSF::IsEqualNoCase(m_survival[i].m_variable, var))
					if (m_survival[i].m_variable == var)
					{
						if (bLogLikelyhoude)
						{
							//maximum likelihood
							double n = 1;// m_survival[i][I_N];
							double LL = Regniere2021Survival(eq, computation.m_XP, m_survival[i]);
							if (!isfinite(LL) || isnan(LL))
								return false;

							log_likelyhoude += n * LL;
							N_likelyhoude += n;
						}
						else
						{//RSS
							//stage survival
							if (m_survival[i][I_MEAN_TIME] > 0)
							{
								double obs = m_survival[i][I_SURVIVAL] / m_survival[i][I_N];
								double sim = GetSurvival(eq, computation.m_XP, m_survival[i]);

								if (!isfinite(sim) || isnan(sim) || sim < -1E8 || sim>1E8)
									return false;

								for (size_t n = 0; n < m_survival[i][I_N]; n++)
									stat.Add(sim, obs);
							}
						}
					}
				}
			}
		}
		else if (m_fitType == F_FECUNDITY)
		{
			TFecundityEquation eq = CFecundityEquation::eq(e);
			if (!IsParamValid(var, eq, computation.m_XP) || !IsRateValid(var, eq, computation.m_XP, NULL_RATE_THRESHOLD))
				return false;


			//if (CFecundityEquation::IsParamValid(eq, computation.m_XP))
			{
				for (__int64 i = 0; i < (__int64)m_fecundity.size(); i++)
				{
					//if (WBSF::IsEqualNoCase(m_fecundity[i].m_variable, var))
					if (m_fecundity[i].m_variable == var)
					{
						if (bLogLikelyhoude)//use likelihood method
						{
							double LL = 0;

							if (m_fecundity.m_bIndividualSeries)//use individual time
							{
								LL = Regniere2021FecundityTimeSeries(eq, computation.m_XP, m_Tobs[m_fecundity[i].m_treatment], m_fecundity[i][I_START], m_fecundity[i][I_START] + m_fecundity[i][I_TIME], m_fecundity[i][I_BROOD], m_fecundity[i][I_Q_BROOD]);
								LL *= m_fecundity[i][I_N];
							}
							else if (m_fecundity.m_bIndividual)//use individual time
							{
								LL = Regniere2021Fecundity(eq, computation.m_XP, m_Tobs[m_fecundity[i].m_treatment], m_fecundity[i][I_START] + m_fecundity[i][I_TIME], m_fecundity[i][I_BROOD], m_fecundity[i][I_Q_BROOD]);
								LL *= m_fecundity[i][I_N];
							}
							else//use mean+sd+n
							{
								//LL seem to not have to be multiply by N!
								LL = Regniere2021FecundityMeanSDn(eq, computation.m_XP, m_Tobs[m_fecundity[i].m_treatment], m_fecundity[i][I_MEAN_TIME], m_fecundity[i][I_MEAN_BROOD], m_fecundity[i][I_BROOD_SD], m_fecundity[i][I_N]);
							}

							if (!isfinite(LL) || isnan(LL))
								return false;

							log_likelyhoude += LL;
							N_likelyhoude += m_fecundity[i][I_N];
						}
						else//use least square method
						{
							if (m_fecundity.m_bIndividual)//use individual time
							{
								//double F = quantile(LogNormal, m_fecundity[i][I_Q_BROOD]);
								//double brood = GetFecundity(eq, computation, m_Tobs[m_fecundity[i].m_treatment], m_fecundity[i][I_START], m_fecundity[i][I_START] + m_fecundity[i][I_TIME], qi[i]);
								//if (!isfinite(brood) || isnan(brood))
								//	return;

								////boost::math::lognormal_distribution<double> LogNormal(-0.5*Square(sigma), sigma);
								////double RFR = quantile(LogNormal, m_fecundity[i][I_Q_BROOD]);
								//double sim = brood;
								//double obs = m_fecundity[i][I_BROOD];
								//for (size_t n = 0; n < m_fecundity[i][I_N]; n++)
								//	stat.Add(sim, obs);
							}
							else//use mean+sd+n 
							{
								//double F = quantile(LogNormal, m_fecundity[i][I_Q_BROOD]);
								//double brood = GetFecundity(eq, computation, m_Tobs[m_fecundity[i].m_treatment], m_fecundity[i][I_START], m_fecundity[i][I_START] + m_fecundity[i][I_MEAN_TIME], m_fecundity[i][I_Q_BROOD]);
								//if (!isfinite(brood) || isnan(brood))
								//	return;

								////boost::math::lognormal_distribution<double> LogNormal(-0.5*Square(sigma), sigma);
								////double RFR = quantile(LogNormal, m_fecundity[i][I_Q_BROOD]);
								//double sim = brood;
								//double obs = m_fecundity[i][I_MEAN_BROOD];
								//for (size_t n = 0; n < m_fecundity[i][I_N]; n++)
								//	stat.Add(sim, obs);

								/*double to = 0;
								double obs = m_fecundity[i][I_MEAN_BROOD];
								double sim = GetFecundity(eq, F, to, computation, m_Tobs[m_fecundity[i].m_treatment], m_fecundity[i][I_START], m_fecundity[i][I_START] + m_fecundity[i][I_TIME]);

								for (size_t n = 0; n < m_fecundity[i][I_N]; n++)
									stat.Add(sim, obs);*/
							}
						}
					}
				}
			}
		}



		if (bLogLikelyhoude)
		{
			//add penalty if over maximum



			if (N_likelyhoude > 0)
			{

				double k = computation.m_XP.size();
				double n = N_likelyhoude;
				double AIC = 2 * k - 2 * (log_likelyhoude);
				double AICc = AIC + (2 * k * (k + 1) / max(1.0, n - k - 1));// n/k < 40

				computation.m_k_P = k;
				computation.m_n_P = n;
				computation.m_AICCP = AICc;
				computation.m_MLLP = log_likelyhoude;
				computation.m_FP = m_ctrl.AdjustFValue(log_likelyhoude);
			}
		}
		else if (stat[NB_VALUE] > 1)
		{
			computation.m_FP = m_ctrl.AdjustFValue(stat[m_ctrl.m_statisticType]);
			computation.m_SP = stat;
		}

		//file.close();
		return true;
	}


	ERMsg CInsectParameterization::ReadParametersFromFile(const std::string& outputFilePath, std::map<std::string, std::map<std::string, std::map<std::string, double>>>& params)
	{
		ERMsg msg;

		params.clear();

		ifStream file;
		msg = file.open(outputFilePath);
		if (msg)
		{
			for (CSVIterator loop(file); loop != CSVIterator(); ++loop)
			{
				if ((*loop).size() >= 3)
				{
					string variable = (*loop)[0];
					string e_name = (*loop)[1];
					StringVector p((*loop)[2], " ");
					for (size_t i = 0; i < p.size(); i++)
					{
						StringVector pp(p[i], "=");
						assert(pp.size() == 2);
						if (pp.size() == 2)
						{
							params[variable][e_name][pp[0]] = as<double>(pp[1]);
						}
					}
				}
			}

			file.close();
		}

		return msg;
	}

	template <typename T> inline
		std::string ToString2(const T& v, const std::string& be = "[", const std::string& sep = ",", const std::string& en = "]")
	{
		std::string str = be;
		for (typename T::const_iterator it = v.begin(); it != v.end(); it++)
		{
			if (it != v.begin())
				str += sep;
			str += ToString(*it);
		}

		str += en;

		return str;
	}

	template <typename T, size_t size> inline
		const std::array<T, size> ToArray(const std::string& str, const std::string& be = "[", const std::string& sep = ",", const std::string& en = "]")
	{
		StringVector tmp = Tokenize(str, be + sep + en);
		std::array<T, size> v;

		size_t i = 0;
		for (StringVector::const_iterator it = tmp.begin(); it != tmp.end(); it++, i++)
			if (!it->empty() && i < v.size())
				v[i] = ToValue<T>(*it);

		return v;
	}


	void CInsectParameterization::writeStruc(zen::XmlElement& output)const
	{
		CExecutable::writeStruc(output);
		zen::XmlOut out(output);

		out[GetMemberName(FIT_TYPE)](m_fitType);
		out[GetMemberName(DEV_RATE_EQUATIONS)](m_eqDevRate);
		out[GetMemberName(SURVIVAL_EQUATIONS)](m_eqSurvival);
		out[GetMemberName(FECUNDITY_EQUATIONS)](m_eqFecundity);
		out[GetMemberName(EQ_OPTIONS)](m_eq_options);
		out[GetMemberName(INPUT_FILE_NAME)](m_inputFileName);
		out[GetMemberName(OUTPUT_FILE_NAME)](m_outputFileName);
		out[GetMemberName(TOBS_FILE_NAME)](m_TobsFileName);
		out[GetMemberName(CONTROL)](m_ctrl);
		out[GetMemberName(FIXE_TB)](m_bFixeTb);
		out[GetMemberName(TB_VALUE)](ToString2(m_Tb));
		out[GetMemberName(FIXE_TM)](m_bFixeTm);
		out[GetMemberName(TM_VALUE)](ToString2(m_Tm));

		out[GetMemberName(COUNSTRAIN_T_LO)](m_bConstrainTlo);
		out[GetMemberName(T_LO_VALUES)](ToString2(m_Tlo));
		out[GetMemberName(COUNSTRAIN_T_HI)](m_bConstrainThi);
		out[GetMemberName(T_HI_VALUES)](ToString2(m_Thi));

		out[GetMemberName(FIXE_F0)](m_bFixeF0);
		out[GetMemberName(F0_VALUE)](ToString(m_F0));
		out[GetMemberName(LIMIT_MAX_RATE)](m_bLimitMaxRate);
		out[GetMemberName(LIMIT_MAX_RATE_P)](ToString(m_LimitMaxRateP));
		out[GetMemberName(AVOID_NULL_RATE_IN_TOBS)](m_bAvoidNullRateInTobs);
		out[GetMemberName(USE_OUTPUT_AS_INPUT)](m_bUseOutputAsInput);
		out[GetMemberName(OUTPUT_AS_INTPUT_FILENAME)](m_outputAsIntputFileName);
		out[GetMemberName(SHOW_TRACE)](m_bShowTrace);

	}

	bool CInsectParameterization::readStruc(const zen::XmlElement& input)
	{
		string tmp;
		CExecutable::readStruc(input);
		zen::XmlIn in(input);
		in[GetMemberName(FIT_TYPE)](m_fitType);
		in[GetMemberName(DEV_RATE_EQUATIONS)](m_eqDevRate);
		in[GetMemberName(SURVIVAL_EQUATIONS)](m_eqSurvival);
		in[GetMemberName(FECUNDITY_EQUATIONS)](m_eqFecundity);
		in[GetMemberName(EQ_OPTIONS)](m_eq_options);
		in[GetMemberName(INPUT_FILE_NAME)](m_inputFileName);
		in[GetMemberName(TOBS_FILE_NAME)](m_TobsFileName);
		in[GetMemberName(OUTPUT_FILE_NAME)](m_outputFileName);
		in[GetMemberName(CONTROL)](m_ctrl);
		in[GetMemberName(FIXE_TB)](m_bFixeTb);
		in[GetMemberName(TB_VALUE)](tmp); m_Tb = ToArray<double, 3>(tmp);
		in[GetMemberName(FIXE_TM)](m_bFixeTm);
		in[GetMemberName(TM_VALUE)](tmp); m_Tm = ToArray<double, 3>(tmp);

		in[GetMemberName(COUNSTRAIN_T_LO)](m_bConstrainTlo);
		in[GetMemberName(T_LO_VALUES)](tmp); m_Tlo = ToArray<double, 2>(tmp);
		in[GetMemberName(COUNSTRAIN_T_HI)](m_bConstrainThi);
		in[GetMemberName(T_HI_VALUES)](tmp); m_Thi = ToArray<double, 2>(tmp);

		in[GetMemberName(FIXE_F0)](m_bFixeF0);
		in[GetMemberName(F0_VALUE)](m_F0);
		in[GetMemberName(LIMIT_MAX_RATE)](m_bLimitMaxRate);
		in[GetMemberName(LIMIT_MAX_RATE_P)](m_LimitMaxRateP);
		in[GetMemberName(AVOID_NULL_RATE_IN_TOBS)](m_bAvoidNullRateInTobs);
		in[GetMemberName(USE_OUTPUT_AS_INPUT)](m_bUseOutputAsInput);
		in[GetMemberName(OUTPUT_AS_INTPUT_FILENAME)](m_outputAsIntputFileName);
		in[GetMemberName(SHOW_TRACE)](m_bShowTrace);




		return true;
	}
}

