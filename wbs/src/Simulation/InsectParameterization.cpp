//******************************************************************************
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

#include <cmath>
#include <sstream>
#include <algorithm>
#include <random>
#include <memory>
#include <limits>


#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_real_distribution.hpp>
#include <boost/math/distributions/normal.hpp>
#include <boost/math/distributions/lognormal.hpp>
#include <boost/math/distributions/poisson.hpp>
#include <boost/algorithm/string/predicate.hpp>

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
using namespace WBSF::WEATHER;
using namespace WBSF::DIMENSION;



namespace WBSF
{

	static const double SIGMA_FACTOR1 = 10.0;
	static const double SIGMA_FACTOR2 = 4.0;
	static const double Fo_FACTOR = 5.0;

	static const double NULL_RATE_THRESHOLD = 1.0 / 1000.0;
	static const double LOWER_RATE_THRESHOLD = 0.0;// 1e-3;
	static const double INF_TIME = 1000 - 100;//replace Time by 1000 days when NA

	//static const std::string DEAD_STR = "Dead";
	//static const std::string CENSORED_STR = "Censored";
	//static const std::string CASUALTIES_STR = "Casualties";

	std::string CDataRow::ADULT_NAME;



	//**********************************************************************************************
	static vector<size_t> Shuffle(size_t N)
	{
		// Define the range of numbers (e.g., 0 to N-1)
		// Create a vector with the numbers in the range
		std::vector<size_t> numbers;
		for (size_t i = 0; i < N; ++i)
			numbers.push_back(i);


		// Initialize a random number generator engine
		// Using std::random_device for a non-deterministic seed
		std::random_device rd;
		std::mt19937 g(rd()); // Mersenne Twister engine

		// Shuffle the vector
		std::shuffle(numbers.begin(), numbers.end(), g);

		return numbers;
	}


	//**********************************************************************************************

	TTreatment GetTreatmentType(const std::string& treatment)
	{
		TTreatment  type = T_NOT_INIT;
		if (treatment.find(">") != string::npos && treatment.find(":") != string::npos)
			type = T_TRANSFER;
		else if (treatment.find("|") != string::npos)
			type = T_SQUARE;
		//else if (treatment.find("^") != string::npos)
			//type = T_TRIANGULAR;
		else if (treatment.find("~") != string::npos)
			type = T_SINUS;
		//else if (treatment.find("±") != string::npos)
			//type = T_SINUS_MEAN;
		else if (treatment.find_first_not_of("-0123456789.") == string::npos)
			type = T_CONSTANT;
		else
			type = T_FLUCTUATING;

		return type;
	}

	double GetTreatmentTemperature(const std::string& treatment, size_t i)
	{
		assert(i >= 0 && i <= 1);
		double T = -999;

		TTreatment treatment_type = GetTreatmentType(treatment);

		if (treatment_type == T_CONSTANT)
		{
			T = ToDouble(treatment);
		}
		else if (treatment_type != T_FLUCTUATING)
		{
			StringVector tmp(treatment, "|~^>:±");
			ASSERT(tmp.size() == 2 || tmp.size() == 3);
			ASSERT(treatment_type != T_TRANSFER || tmp.size() == 3);

			if (tmp.size() == 2 || tmp.size() == 3)
				T = ToDouble(tmp[i]);
		}
		else
		{
			ASSERT(false);
		}

		return T;
	}

	double GetTreatmentH1(const std::string& treatment)
	{
		TTreatment treatment_type = GetTreatmentType(treatment);
		ASSERT(treatment_type == T_SQUARE || treatment_type == T_SINUS);
		double h1 = 12.0;

		if (treatment_type == T_SQUARE || treatment_type == T_SINUS)
		{
			StringVector tmp(treatment, ":");
			if (tmp.size() == 2)//if ":", set h1 other than default value of 12 hours
			{
				h1 = stof(tmp[1]);
				ASSERT(h1 < 24);
			}
		}

		return h1;
	}

	double GetTreatmentTime1(const std::string& treatment)//in days
	{
		TTreatment treatment_type = GetTreatmentType(treatment);
		ASSERT(treatment_type == T_TRANSFER);
		double Time1 = -999;
		if (treatment_type == T_TRANSFER)
		{
			StringVector tmp(treatment, ">:");
			ASSERT(tmp.size() == 3);

			if (tmp.size() == 3)
				Time1 = stof(tmp[2]);
		}


		return Time1;
	}

	static const std::array<std::string, NB_STAGE_END_STATUS> STAGE_END_STATUS_NAME =
	{
		"Alive", "Dead", "Censored", "Casualties"
	};

	size_t GetStageEndStatus(const std::string& name)
	{
		size_t stage_end_status = NOT_INIT;
		auto it = find_if(STAGE_END_STATUS_NAME.begin(), STAGE_END_STATUS_NAME.end(), [&](auto& s) {return boost::iequals(s, name); });
		if (it != STAGE_END_STATUS_NAME.end())
			stage_end_status = std::distance(STAGE_END_STATUS_NAME.begin(), it);

		return stage_end_status;
	}

	double GetDevRate(TDevRateEquation  e, const vector<double>& X, const CTemporalVector& T, const CObservation& O)
	{
		CStatisticEx stat;
		if (T.m_treatment_type == T_CONSTANT)//optimization at constant temperature (rate is always the same)
		{
			double R = max(0.0, CDevRateEquation::GetRate(e, X, T[0]));//daily rate
			if (!isfinite(R) || isnan(R))
				return NAN;

			stat += R;//sum of rate
		}
		else if (T.m_treatment_type == T_TRANSFER)
		{
			double R1 = max(0.0, CDevRateEquation::GetRate(e, X, T[0]));//daily rate
			double R2 = max(0.0, CDevRateEquation::GetRate(e, X, T[1]));//daily rate

			if (isnan(R1) || isnan(R2))
				return NAN;

			double t1 = std::min(O.m_to, T.m_t_transfer);
			double t2 = std::max(0.0, O.m_t - T.m_t_transfer);
			stat += (t1 * R1 + t2 * R2) / (t1 + t2);

			//for (size_t h = 0; h < t1 * 24; h++)
			//	stat += R1;//sum of rate
			//
			//for (size_t h = 0; h < t2 * 24; h++)
			//	stat += R2;//sum of rate

		}
		else if (T.m_treatment_type == T_SQUARE || T.m_treatment_type == T_SINUS)
		{
			//optimization at constant 24 hour cycle temperature (rate is always the same)
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
			double t = O.m_t;//O.t();
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

		return stat[MEAN];
	}

	double GetSurvival(TSurvivalEquation e, const vector<double>& X, const CTemporalVector& T, const CObservation& O)
	{
		if (O.at(I_MEAN_TIME) <= 0)//when time is NA, survival is 0
			return 0;

		double S = 1;
		if (T.m_treatment_type == T_CONSTANT)
		{
			//daily survival
			double s = max(0.0, CSurvivalEquation::GetSurvival(e, X, T[0]));

			if (isnan(s))
				return NAN;


			//multiplication of all daily survival
			double t = O.m_t;//O.t();
			ASSERT(t > 0);
			S = pow(s, t);
		}
		else if (T.m_treatment_type == T_TRANSFER)
		{
			double s1 = max(0.0, CSurvivalEquation::GetSurvival(e, X, T[0]));
			double s2 = max(0.0, CSurvivalEquation::GetSurvival(e, X, T[1]));

			if (isnan(s1) || isnan(s2))
				return NAN;

			//multiplication of all daily survival
			double t1 = std::min(O.m_to, T.m_t_transfer);
			double t2 = std::max(0.0, O.m_t - T.m_t_transfer);

			if (t1 > 0)
				S *= pow(s1, t1);
			if (t2 > 0)
				S *= pow(s2, t2);
		}
		else if (T.m_treatment_type == T_SQUARE || T.m_treatment_type == T_SINUS)
		{
			//optimization at constant 24 hour cycle temperature (rate is always the same)
			ASSERT(T.size() == 24);

			double t = O.m_t;

			std::array<double, 24> s;
			for (size_t h = 0; h < 24; h++)
			{
				//daily survival
				double d_s = max(0.0, CSurvivalEquation::GetSurvival(e, X, T[h]));
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
			ASSERT(T.m_treatment_type == T_FLUCTUATING);



			double t = O.m_t;
			ASSERT(t > 0);

			ASSERT(T.size() >= ceil(t));


			for (size_t td = 0; td < ceil(t); td++)//for all days
			{
				for (size_t h = 0; h < 24; h++)//for all hours
				{
					double th = td + double(h) / 24.0;

					if (th < t)
					{
						double d_s = max(0.0, CSurvivalEquation::GetSurvival(e, X, T[td * 24 + h]));

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


	bool Regniere2021DevRate(TDevRateEquation  e, const vector<double>& X, const CTemporalVector& T, CxiVector& t_xi)
	{
		if (T.m_treatment_type == T_CONSTANT)
		{
			double rateD = max(LOWER_RATE_THRESHOLD, CDevRateEquation::GetRate(e, X, T[0]));//daily rate
			assert(!isnan(rateD));
			assert(!isinf(rateD));

			if (isinf(rateD) || isnan(rateD))
				return false;

			for (auto& xi : t_xi)
			{
				xi.first[1] = xi.first[0] * rateD;
				xi.second[1] = xi.second[0] * rateD;
			}
		}
		else if (T.m_treatment_type == T_TRANSFER)
		{
			double T1 = T[0];
			double T2 = T[1];

			double rateD1 = max(LOWER_RATE_THRESHOLD, CDevRateEquation::GetRate(e, X, T1));//daily rate
			double rateD2 = max(LOWER_RATE_THRESHOLD, CDevRateEquation::GetRate(e, X, T2));//daily rate

			if (isinf(rateD1) || isinf(rateD2) || isnan(rateD1) || isnan(rateD2))
				return false;

			//assert(T.IsHourly());

			for (auto& xi : t_xi)
			{
				array<double, 2> t1 = { std::min(xi.first[0], T.m_t_transfer), std::min(xi.second[0], T.m_t_transfer) };
				array<double, 2> t2 = { std::max(0.0, xi.first[0] - T.m_t_transfer), std::max(0.0, xi.second[0] - T.m_t_transfer) };

				xi.first[1] = t1[0] * rateD1 + t2[0] * rateD2;
				xi.second[1] = t1[1] * rateD1 + t2[1] * rateD2;
			}

		}
		else if (T.m_treatment_type == T_SQUARE || T.m_treatment_type == T_SINUS)
		{
			//optimization at constant 24 hour cycle temperature (rate is always the same)
			ASSERT(T.size() == 24);

			double mean_rateD = 0;
			double rateH24[24] = { 0 };
			for (size_t h = 0; h < 24; h++)
			{
				double rateH = max(LOWER_RATE_THRESHOLD, CDevRateEquation::GetRate(e, X, T[h]) / 24.0);//hourly rate

				if (isinf(rateH) || isnan(rateH))
					return false;

				rateH24[h] = rateH;
				mean_rateD += rateH;
			}


			for (auto& xi : t_xi)
			{
				xi.first[1] = floor(xi.first[0]) * mean_rateD;
				xi.second[1] = floor(xi.second[0]) * mean_rateD;

				size_t t_h1 = std::ceil((xi.first[0] - floor(xi.first[0])) * 24);
				size_t t_h2 = std::ceil((xi.second[0] - floor(xi.second[0])) * 24);
				assert(t_h1 <= 24 && t_h1 <= 24);

				size_t max_h = std::max(t_h1, t_h2);
				for (size_t h = 0; h < max_h; h++)
				{
					if (h < t_h1)
						xi.first[1] += rateH24[h];

					if (h < t_h2)
						xi.second[1] += rateH24[h];
				}
			}
		}
		else
		{
			//double min_t = 0;// t_xi.begin()->first[0];
			//double max_t = t_xi.rbegin()->second[0];

			ASSERT(t_xi.t_min <= t_xi.t_max);
			ASSERT(T.size() >= ceil(t_xi.t_max));
			for (size_t td = floor(t_xi.t_min); td < ceil(t_xi.t_max); td++)
			{
				for (size_t h = 0; h < 24; h++)
				{
					double th = td + double(h) / 24.0;

					ASSERT(td * 24 + h < T.size());
					double rateH = max(LOWER_RATE_THRESHOLD, CDevRateEquation::GetRate(e, X, T[td * 24 + h])) / 24.0;//hourly rate

					if (isinf(rateH) || isnan(rateH))
						return false;

					for (auto& xi : t_xi)
					{
						if (th >= xi.to && th < xi.to + xi.first[0])
							xi.first[1] += rateH;

						if (th >= xi.to && th < xi.to + xi.second[0])
							xi.second[1] += rateH;
					}
				}
			}
		}

		return true;
	}



	/*double Regniere2021DevRateMeanSDn(TDevRateEquation  e, const vector<double>& X, const CTemporalVector& T, const CObservation& O)
	{
		CStatistic stat_rate;
		if (T.m_treatment_type == T_CONSTANT)
		{

			double rate = max(LOWER_RATE_THRESHOLD, CDevRateEquation::GetRate(e, X, T[0]));//daily rate
			if (!isfinite(rate) || isnan(rate))
				return NAN;

			stat_rate = rate;//mean daily rate
		}
		else if (T.m_treatment_type == T_TRANSFER)
		{
			double rateD1 = max(LOWER_RATE_THRESHOLD, CDevRateEquation::GetRate(e, X, T[0]));//daily rate
			double rateD2 = max(LOWER_RATE_THRESHOLD, CDevRateEquation::GetRate(e, X, T[1]));//daily rate

			if (!isfinite(rateD1) || isnan(rateD1) || !isfinite(rateD2) || isnan(rateD2))
				return NAN;

			double t1 = std::min(O.m_to, T.m_t_transfer);
			double t2 = std::max(0.0, O.m_t - T.m_t_transfer);

			ASSERT(t1 >= 0);

			stat_rate = (t1 * rateD1 + t2 * rateD2) / (t1 + t2);

		}
		else if (T.m_treatment_type == T_SQUARE || T.m_treatment_type == T_TRIANGULAR ||
			T.m_treatment_type == T_SINUS || T.m_treatment_type == T_SINUS_MEAN)//optimization at constant 24 hour cycle temperature (rate is always the same)
		{

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
			ASSERT(T.m_treatment_type == T_FLUCTUATING);


			//double t = O.m_t;//O.t();
			double mean_time = O.m_t;
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

		double obs_time = O.at(I_MEAN_TIME);
		double time_SD = O.at(I_TIME_SD);
		double n = O.at(I_N);

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
	*/
	void Regniere2021DevRateMeanSDn(TDevRateEquation  e, const vector<double>& X, const CTemporalVector& T, CxiVector& t_xi)
	{
		//CStatistic stat_rate;
		if (T.m_treatment_type == T_CONSTANT)
		{
			double rateD = max(LOWER_RATE_THRESHOLD, CDevRateEquation::GetRate(e, X, T[0]));//daily rate
			assert(isfinite(rateD) && !isnan(rateD));


			//stat_rate = rate;//mean daily rate
			for (auto& xi : t_xi)
			{
				xi.first[1] = 1;
				xi.second[1] = rateD;
			}
		}
		else if (T.m_treatment_type == T_TRANSFER)
		{
			double rateD1 = max(LOWER_RATE_THRESHOLD, CDevRateEquation::GetRate(e, X, T[0]));//daily rate
			double rateD2 = max(LOWER_RATE_THRESHOLD, CDevRateEquation::GetRate(e, X, T[1]));//daily rate

			assert(isfinite(rateD1) && !isnan(rateD1));
			assert(isfinite(rateD2) && !isnan(rateD2));

			//double t1 = std::min(O.m_to, T.m_t_transfer);
			//double t2 = std::max(0.0, O.m_t - T.m_t_transfer);

			//ASSERT(t1 >= 0);

			//stat_rate = (t1 * rateD1 + t2 * rateD2) / (t1 + t2);

			for (auto& xi : t_xi)
			{
				double t1 = std::min(xi.to, T.m_t_transfer);
				double t2 = std::max(0.0, xi.second[0] - T.m_t_transfer);

				//stat_rate = (t1 * rateD1 + t2 * rateD2) / (t1 + t2);
				xi.first[1] = 1;
				xi.second[1] = (t1 * rateD1 + t2 * rateD2) / (t1 + t2);
			}

		}
		else if (T.m_treatment_type == T_SQUARE || T.m_treatment_type == T_SINUS)
		{
			//optimization at constant 24 hour cycle temperature (rate is always the same)
			ASSERT(T.size() == 24);

			//for (size_t h = 0; h < 24; h++)
			//{
			//	double rate = max(LOWER_RATE_THRESHOLD, CDevRateEquation::GetRate(e, X, T[h]));//daily rate;
			//	if (!isfinite(rate) || isnan(rate))
			//		return NAN;

			//	stat_rate += rate;//mean daily rate
			//}
			double mean_rateD = 0;
			for (size_t h = 0; h < 24; h++)
			{
				double rateH = max(LOWER_RATE_THRESHOLD, CDevRateEquation::GetRate(e, X, T[h]));//hourly rate
				assert(isfinite(rateH) && !isnan(rateH));

				mean_rateD += rateH;
			}

			for (auto& xi : t_xi)
			{
				xi.first[1] = 1;
				xi.second[1] = mean_rateD;
			}
		}
		else
		{
			ASSERT(T.m_treatment_type == T_FLUCTUATING);


			//double t = O.m_t;//O.t();
			//double mean_time = O.m_t;
			//ASSERT(ceil(mean_time) < T.size());
			//for (size_t t = 0; t < mean_time; t++)
			//{
			//	for (size_t h = 0; h < 24; h++)
			//	{
			//		double rate = max(LOWER_RATE_THRESHOLD, CDevRateEquation::GetRate(e, X, T[t * 24 + h]));//daily rate for each hour
			//		if (!isfinite(rate) || isnan(rate))
			//			return NAN;

			//		stat_rate += rate;//mean daily rate
			//	}
			//}
			ASSERT(t_xi.t_min == 0);
			ASSERT(t_xi.t_min <= t_xi.t_max);
			ASSERT(T.size() >= ceil(t_xi.t_max));
			for (size_t td = floor(t_xi.t_min); td < ceil(t_xi.t_max); td++)
			{
				for (size_t h = 0; h < 24; h++)
				{
					double th = td + double(h) / 24.0;

					ASSERT(td * 24 + h < T.size());
					double rateD = max(LOWER_RATE_THRESHOLD, CDevRateEquation::GetRate(e, X, T[td * 24 + h]));
					assert(isfinite(rateD) && !isnan(rateD));

					for (auto& xi : t_xi)
					{
						if (th >= xi.to && th < xi.to + xi.second[0])
						{
							xi.first[1]++;
							xi.second[1] += rateD;
						}
					}
				}
			}

			//compute mean
			for (auto& xi : t_xi)
			{
				xi.second[1] = xi.second[1] / xi.first[1];
			}
		}


		//convert rate in time
		for (auto& xi : t_xi)
		{
			xi.second[1] = min(1000.0, 1.0 / xi.second[1]);
		}


		//double sim_time = 1.0 / max(1E-20, stat_rate[MEAN]);


		//double obs_time = O.at(I_MEAN_TIME);
		//double time_SD = O.at(I_TIME_SD);
		//double n = O.at(I_N);

		//if (time_SD <= 0)
		//	time_SD = obs_time / 6.0;//an estimate of SD

		////1E-20: avoid division by zero
		//double sim_time = 1.0 / max(1E-20, stat_rate[MEAN]);

		////compute probability 
		//boost::math::normal_distribution<double> Normal(0, time_SD / sqrt(n));//In c++, Normals need SD

		//ASSERT(isfinite(sim_time));
		//double p = pdf(Normal, obs_time - sim_time);

		//return log(max(DBL_MIN, p));
	}

	double Regniere2021Survival(TSurvivalEquation e, const vector<double>& X, const CTemporalVector& T, const CObservation& O)
	{
		double S_obs = O.at(I_SURVIVAL);
		double n = O.at(I_N);

		double S = GetSurvival(e, X, T, O);

		if (!isfinite(S) || isnan(S))
			return S;

		double expected = max(0.0001, min(1e10, n * S));//limit to a very low expected when zero survival
		boost::math::poisson_distribution<double> Poisson(expected);

		//compute probability of surviving
		double p = max(DBL_MIN, pdf(Poisson, S_obs));
		return log(p);
	}

	//to : pre-oviposition period
	//double Regniere2021FecundityTimeSeries(TDevRateEquation  e, const vector<double>& X, const vector<double>& T, double tiˉ¹, double ti, double brood_obs, double qBrood)
	//{
	//	ASSERT(tiˉ¹ < ti);

	//	double to = 0;// X[X.size() - 3];
	//	//Get Fo and sigma from input parameters X
	//	double Fo = X[X.size() - 2];
	//	double sigma_f = X[X.size() - 1];

	//	//create relative fecundity unbiased log-normal distribution
	//	boost::math::lognormal_distribution<double> LogNormal(log(Fo) - 0.5 * Square(sigma_f), sigma_f);

	//	//compute individual fecundity (Fi) from Fo and female quartile
	//	double Fi = quantile(LogNormal, min(0.99999, qBrood));


	//	double Ft = Fi;//remaining fecundity at day t
	//	double Ftˉ¹ = Fi;//remaining fecundity at day t-1
	//	if (T.size() == 1)//optimization at constant temperature (rate is always the same)
	//	{
	//		//compute lambda from equations parameters and fixed temperature
	//		double lambda = max(0.0, CDevRateEquation::GetRate(e, X, T[0]));//remaining eggs
	//		if (!isfinite(lambda) || isnan(lambda))
	//			return NAN;

	//		//compute remaining fecundity
	//		if (ti - to >= 0)
	//			Ft = Fi * exp(-lambda * (ti - to));
	//		if (tiˉ¹ - to >= 0)
	//			Ftˉ¹ = Fi * exp(-lambda * (tiˉ¹ - to));
	//	}
	//	else if (T.size() == 24)//optimization at constant 24 hour cycle temperature (rate is always the same)
	//	{
	//		//xi = integral (hourly summation) of all hourly rate at hourly temperature at day ti
	//		//xiˉ¹ = integral (hourly summation) of all hourly rate at hourly temperature at day ti-1

	//		//double Ft = sigma * exp(-lambda * ((ti - t0-2) * 24));
	//		//double Ftˉ¹ = sigma * exp(-lambda * ((ti - 1 - t0) * 24));
	//		//double Fi[24] = { 0 };
	//		//double lambda[24] = { 0 };
	//		//for (size_t h = 0; h < 24; h++)
	//		//{
	//		//	lambda[h] = max(0.0, CDevRateEquation::GetRate(e, computation.m_XP, T[h])) / 24.0;//hourly rate;

	//		//	if (!isfinite(lambda[h]) || isnan(lambda[h]))
	//		//		return NAN;

	//		//	//Fi[h] = brood_obs / (exp(-lambda[h] * (tiˉ¹ - to)) - exp(-lambda[h] * (ti - to)));


	//		//}


	//		//Ft = brood_obs;
	//		//Ftˉ¹ = brood_obs;
	//		//for (size_t t = to; t < ti; t++)
	//		//{
	//		//	for (size_t h = 0; h < 24; h++)
	//		//	{
	//		//		Ft += Ft * lambda[h];

	//		//		if (t < (ti - 1))
	//		//			Ftˉ¹ -= Ft * lambda[h];
	//		//	}
	//		//}
	//		assert(false);//a vérifier

	//	}
	//	else
	//	{
	//		assert(false);//a vérifier

	//		//Ft = brood_obs;
	//		//Ftˉ¹ = brood_obs;

	//		//for (size_t t = to; t < ti; t++)
	//		//{
	//		//	for (size_t h = 0; h < 24; h++)
	//		//	{
	//		//		ASSERT(t * 24 + h < T.size());
	//		//		double lambda = max(0.0, CDevRateEquation::GetRate(e, computation.m_XP, T[t * 24 + h]));//hourly remaining eggs
	//		//		if (!isfinite(lambda) || isnan(lambda))
	//		//			return NAN;

	//		//		Ft -= Ft * lambda;

	//		//		if (t < (ti - 1))
	//		//			Ftˉ¹ -= Ft * lambda;
	//		//	}
	//		//}
	//	}


	//	//create Poisson distribution from expected values
	//	double expected = max(1e-5, Ftˉ¹ - Ft);

	//	boost::math::poisson_distribution<double> Poisson(expected);

	//	//compute probability to get observed brood
	//	double p = max(1e-100, boost::math::pdf(Poisson, brood_obs));

	//	//boost::math::poisson_distribution<double> Poisson(max(0.0001, brood_obs));
	//	//double expected = Ftˉ¹ - Ft;
	//	//double p = max(1e-20, boost::math::pdf(Poisson, expected));


	//	return log(p);
	//}


	bool Regniere2021FecundityTimeSeries(TDevRateEquation  e, const vector<double>& X, const CTemporalVector& T, CxiVector& f_xi)
	{
		//	ASSERT(tiˉ¹ < ti);

		double to = X[X.size() - 3];

		//if (T.size() == 1)//optimization at constant temperature (rate is always the same)
		//{
		//	//compute lambda from equations parameters and fixed temperature
		//	double lambda = max(0.0, CDevRateEquation::GetRate(e, X, T[0]));//remaining eggs
		//	if (!isfinite(lambda) || isnan(lambda))
		//		return NAN;

		//	//compute remaining fecundity
		//	if (ti - to >= 0)
		//		Ft = Fi * exp(-lambda * (ti - to));
		//	if (tiˉ¹ - to >= 0)
		//		Ftˉ¹ = Fi * exp(-lambda * (tiˉ¹ - to));
		//}
		//else
		if (T.m_treatment_type == T_CONSTANT)
		{
			double lambda = max(LOWER_RATE_THRESHOLD, CDevRateEquation::GetRate(e, X, T[0]));//remaining eggs
			assert(!isnan(lambda));
			assert(!isinf(lambda));

			if (isinf(lambda) || isnan(lambda))
				return false;

			for (auto& xi : f_xi)
			{
				//xi.first[1] = xi.first[0] * rateD;
				//xi.second[1] = xi.second[0] * rateD;

				//compute remaining fecundity
				//if (xi.first[0] - to >= 0)
				xi.first[1] = /*Fi *   */min(1.0, exp(-lambda * (xi.first[0] - to)));
				//if (xi.second[0] - to >= 0)
				xi.second[1] = /*Fi *  */min(1.0, exp(-lambda * (xi.second[0] - to)));
			}



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

		return true;
	}


	double Regniere2021Fecundity(TDevRateEquation  e, const vector<double>& X, const vector<double>& T, double time, double brood_obs, double qBrood)
	{
		//Get to, Fo and sigma from input parameters X
		double to = 0;// X[X.size() - 3];
		double Fo = X[X.size() - 2];
		double sigma_f = X.back();

		//create relative fecundity unbiased log-normal distribution
		boost::math::lognormal_distribution<double> LogNormal(log(Fo) - 0.5 * Square(sigma_f), sigma_f);

		//compute individual fecundity (Fi) from Fo and female quartile
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



	CTobsSeries::CTobsSeries()
	{
	}

	CTobsSeries::~CTobsSeries()
	{
	}


	const char* CTobsSeries::INPUT_NAME[NB_TOBS_COL] = { "Treatment", "Date", "Time", "Tair", "Light" };
	TTobsCol CTobsSeries::get_input(const std::string& name)
	{
		TTobsCol col = C_NOT_INIT;

		auto it = find_if(std::begin(INPUT_NAME), std::end(INPUT_NAME), [&](auto& s) {return boost::iequals(s, name); });
		if (boost::iequals("Tid", name))//also accept Tid instead of Treatment
			it = std::begin(INPUT_NAME) + C_TREATMENT;

		if (boost::iequals("DOY", name))//also accept DOY instead of date
			it = std::begin(INPUT_NAME) + C_DATE;

		if (boost::iequals("T", name))//also accept T instead of Tair
			it = std::begin(INPUT_NAME) + C_TEMPERATURE;

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
		std::locale utf8_locale("en_US.UTF-8");
		file.imbue(utf8_locale);
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

		CSVIterator loop(io, ",;\t", true, true);

		//pID = NOT_INIT;
		//size_t pT = NOT_INIT;

		//f (m_input_pos.empty())
		//{
		for (size_t i = 0; i < loop.Header().size(); i++)
			m_input_pos.push_back(get_input(loop.Header()[i]));


		//check for mandatory columns
		if (!have_var(C_TREATMENT))
		{
			msg.ajoute("Mandatory missing column. \"Treatment\" (or \"Tid\") must be define");
		}

		if (!have_var(C_DATE) && !have_var(C_TIME))
		{
			msg.ajoute("Mandatory missing column. \"Date\" or \"Time\" must be define");
		}

		if (have_var(C_DATE) && have_var(C_TIME))
		{
			msg.ajoute("Only \"Date\" or \"TIME\" must be define. Not both");
		}

		if (!have_var(C_TEMPERATURE))
		{
			msg.ajoute("Mandatory missing column: \"Tair\" (or \"T\") must be define");
		}

		size_t pTr = get_pos(C_TREATMENT);
		size_t pDa = have_var(C_DATE) ? get_pos(C_DATE) : get_pos(C_TIME);
		size_t pTe = get_pos(C_TEMPERATURE);
		//}

		for (; loop != CSVIterator() && msg; ++loop)
		{
			if (pTr < loop->size() &&
				pDa < loop->size() &&
				pTe < loop->size())
			{
				string treatment = (*loop)[pTr];
				CTemporalVector& Tv = (*this)[treatment];

				double T = ToDouble((*loop)[pTe]);

				CTRef TRef;
				double To = 0;
				if (have_var(C_DATE))
				{
					TRef.FromFormatedString((*loop)[pDa], "", "- :");
				}
				else
				{
					To = ToDouble((*loop)[pDa]);
					TRef = CTRef(0, 0, 0, 0, CTM::HOURLY) + (int)Round(To * 24);

				}

				if (TRef.IsValid())
				{
					if (Tv.empty())
					{
						Tv.m_treatment = treatment;
						Tv.m_treatment_type = GetTreatmentType(treatment);
						Tv.m_TRef = TRef;
						//Tv.m_to = To;
					}
					else
					{
						//data must be completed and ordered
						if (TRef - Tv.m_TRef != Tv.size())
						{
							msg.ajoute("Invalid date at line: " + loop->GetLastLine());
							msg.ajoute("Temperature data must be complete and ordered");
						}
					}


					Tv.push_back(T);
				}
				else
				{
					msg.ajoute("Invalid date at line: " + loop->GetLastLine());
				}

			}

		}



		return msg;
	}

	bool CDevRateData::have_individual()const
	{
		bool bRep = false;
		for (auto it = begin(); it != end() && !bRep; it++)//for all Stages
		{
			for (auto iit = it->second.begin(); iit != it->second.end() && !bRep; iit++)//for all Treatment
			{
				for (auto iiit = iit->second.begin(); iiit != iit->second.end() && !bRep; iiit++)//for all Observation( Individuals/Groups)
				{
					bRep = iiit->m_i_temporal == IT_INDIVIDUAL;
				}
			}
		}
		return bRep;
	}

	bool CDevRateData::have_time_series()const
	{
		bool bRep = false;
		for (auto it = begin(); it != end() && !bRep; it++)//for all Stages
		{
			for (auto iit = it->second.begin(); iit != it->second.end() && !bRep; iit++)//for all Treatment
			{
				for (auto iiit = iit->second.begin(); iiit != iit->second.end() && !bRep; iiit++)//for all Observation( Individuals/Groups)
				{
					bRep = iiit->m_i_temporal == IT_TIME_SERIES;
				}
			}
		}

		return bRep;
	}

	//double CDevRateTreatments::GetMaxTime()const
	//{
	//	double mt = 0;
	//}

	ERMsg CDevRateData::SetTobs(const CTobsSeries& Tobs)
	{
		ERMsg msg;

		for (auto& S : *this)//for all Stages
		{
			for (auto& T : S.second)//for all Treatment
			{
				auto itT = Tobs.find(T.first);
				if (itT != Tobs.end())
				{
					const CTemporalVector& TT = itT->second;
					T.second.SetT(std::make_shared<CTemporalVector>(TT));


					//Compute statistics
					double max_time = 0;
					for (auto& O : T.second)//for all Observation( Individuals/Groups)
					{


						if (TT.m_treatment_type == T_FLUCTUATING)
						{
							if (!TT.GetPeriod().IsInside(O.m_TRef0a) ||
								!TT.GetPeriod().IsInside(O.m_TRef1b))
							{
								msg.ajoute("Observation date outside temperature profile for treatment " + TT.m_treatment);
								msg.ajoute("Temperature profile period: " + TT.GetPeriod().GetFormatedString());
								msg.ajoute("Observation " + O.m_i + " between " + O.m_TRef0a.GetFormatedString() + " and " + O.m_TRef1b.GetFormatedString());

								return msg;
							}
						}

						max_time = max(max_time, O.GetMaxTime());

						assert(O.TRef0().GetTM() == CTM::HOURLY);
						assert(TT.m_TRef.GetTM() == CTM::HOURLY);


						if (O.m_to == -999)
						{
							assert(O.m_treatment_type == T_FLUCTUATING);
							O.m_to = (O.TRef0() - TT.m_TRef) / 24.0;
							for (auto& R : O.m_time_series)//update all to of the time series
								R.m_to = O.m_to;
						}


						//if (O[I_TIME] > 0)//if this insect survived at least this period of time, we add the temperature
						double min_time = O.GetMinTime();
						if (min_time > 0)//if this insect survived at least this period of time, we add the temperature
						{
							if (O.m_treatment_type != T_FLUCTUATING)
							{
								for (const auto& Ti : TT)
									S.second.m_stats_Tobs += Ti;
							}
							else
							{
								for (CTRef TRef = O.TRef0(); TRef <= O.m_TRef1b; TRef++)
									S.second.m_stats_Tobs += TT.at(TRef);
							}
						}


					}

					//verify that temporal series have enough data
					if (TT.m_treatment_type == T_FLUCTUATING && max_time >= TT.GetLength())
					{
						msg.ajoute("Temperature profile for treatment " + TT.m_treatment + " don't have enough data");
						msg.ajoute("Temperature profile have " + to_string(Round(TT.GetLength(), 1)) + " days but " + to_string(Round(max_time, 1)) + " days is needed");
					}
				}
				else
				{
					msg.ajoute("treatment " + T.first + " was not found in temperature data file");
				}
			}
		}

		//compute_T_stats(Tobs);

		//for (size_t i = 0; i < data.size(); i++)
		//{
		//	if (data[i].m_type == T_FLUCTUATING)
		//	{
		//		if (find(data[i].m_treatment) == end())
		//		{
		//			msg.ajoute("treatment ID" + data[i].m_treatment + " not found in temperature file");
		//		}
		//		else
		//		{
		//			size_t nb_hours = at(data[i].m_treatment).size();
		//			size_t needed_hours = data[i].GetMaxTime() * 24;
		//			if (nb_hours < needed_hours)
		//			{
		//				msg.ajoute("Temperature profile for ID " + data[i].m_treatment + " don't have enought data");
		//				msg.ajoute("Profile have " + to_string(nb_hours) + " hours and " + to_string(needed_hours) + " is needed");
		//			}
		//		}

		//	}
		//}

		return msg;
	}



	//double CDataRow::t1() const { return m_to; }
	//double CDataRow::t2(bool bPrevious) const { return std::max(0.0, t(bPrevious) - t1()); }
	//double CDataRow::t(bool bPrevious) const { return bPrevious ? m_tˉ¹ : m_t; }

	double CObservation::GetMaxTime()const//in days
	{
		double max_time = CDataRow::GetMaxTime();

		for (auto R : m_time_series)
			max_time = max(max_time, R.GetMaxTime());

		return max_time;
	}

	double CObservation::GetFecundity()const
	{
		double Fo = 0;

		if (!m_time_series.empty())
		{
			for (auto R : m_time_series)
				if (R.find(I_BROODS) != R.end())
					Fo += R.at(I_BROODS);
		}
		else
		{
			if (find(I_BROODS) != end())
				Fo = at(I_BROODS);
			else if (find(I_MEAN_BROOD) != end())
				Fo = at(I_MEAN_BROOD);
		}

		return Fo;
	}


	//ERMsg CObservation::SetTobs(const CTemporalVector& Tobs)
	//{
	//	ERMsg msg;
	//
	//
	//	if (m_treatment_type != T_FLUCTUATING || GetMaxTime() < Tobs.GetLength())
	//	{
	//		m_pTobs = std::make_shared<CTemporalVector>(Tobs);
	//	}
	//	else
	//	{
	//		msg.ajoute("Temperature profile for treatment " + m_treatment + " don't have enough data");
	//		msg.ajoute("Temperature profile have " + to_string(Round(Tobs.GetLength(), 1)) + " days but " + to_string(Round(GetMaxTime(), 1)) + " days is needed");
	//	}
	//
	//
	//	return msg;
	//}



	//generate temperature profile for non FLUCTUATING profile
	void CTobsSeries::generate(const CDevRateData& data)
	{
		CTobsSeries& Tobs = *this;
		set<string> treatments = data.GetAllTreatments();

		//generate treatment
		for (const auto& T : treatments)
		{

			TTreatment treatment_type = GetTreatmentType(T);
			if (treatment_type == T_FLUCTUATING)
				continue;

			Tobs[T].m_treatment_type = treatment_type;
			Tobs[T].m_TRef = CTRef(0, 0, 0, 0, CTM::HOURLY);

			if (treatment_type == T_CONSTANT)
			{
				//constant T have only one value
				Tobs[T].push_back(GetTreatmentTemperature(T, 0));
			}
			else if (treatment_type == T_TRANSFER)
			{
				//transfer T have only two values
				Tobs[T].push_back(GetTreatmentTemperature(T, 0));
				Tobs[T].push_back(GetTreatmentTemperature(T, 1));
				Tobs[T].m_t_transfer = GetTreatmentTime1(T);
			}
			else if (treatment_type == T_SQUARE || treatment_type == T_SINUS)
			{

				double T1 = GetTreatmentTemperature(T, 0);
				double T2 = GetTreatmentTemperature(T, 1);


				size_t h1 = Round(GetTreatmentH1(T));

				for (size_t h = 0; h < 24; h++)
				{
					double Ti = 0;
					if (treatment_type == T_SQUARE)
					{
						Ti = h < h1 ? T1 : T2;
					}
					/*else if (treatment_type == T_TRIANGULAR)
					{
						Ti = (h <= h1) ? T1 + (T2 - T1) * h / h1 : T2 + (T1 - T2) * (h - h1) / (24 - h1);
					}*/
					else if (treatment_type == T_SINUS)
					{
						//double Tmean = (treatment_type == T_SINUS) ? (T1 + T2) / 2 : T1;
						//double deltaT = (treatment_type == T_SINUS) ? (T2 - T1) / 2 : T2;
						double Tmean = T1;
						double deltaT = T2;
						ASSERT(h1 < 24);

						//double theta1 = 2 * PI * h / 24.0;
						//double theta2 = 2 * PI * h / 24.0;
						//Ti = Tmean + deltaT * sin(h < h1 ? theta1: theta2);

						//Rising segment : -cos goes from - 1 to 1 over rise_hours
						double theta1 = 1 - cos(PI * h / h1);
						//Falling segment : +cos goes from 1 back to - 1 over fall_hours
						double theta2 = 1 + cos(PI * (h - h1) / (24 - h1));

						Ti = (Tmean - deltaT / 2) + deltaT * (h < h1 ? theta1 : theta2);
					}



					Tobs[T].push_back(Ti);
				}//all hours
			}



		}//al treatment
	}

	//void CTobsSeries::compute_stats()
	//{
	//	//for all treatment
	//	for (auto it = begin(); it != end(); it++)
	//	{
	//		//for all temperature
	//		for (auto itt = it->second.begin(); itt != it->second.end(); itt++)
	//			it->second.m_stats += *itt;
	//	}
	//}


	//**********************************************************************************************
	//CDevRateInput


	const char* CDevRateData::INPUT_NAME[NB_DEV_INPUT] =
	{
		"Stage", "Treatment", "iID", "Start", "Time", "MeanTime", "TimeSD", "N", "ObsInt", "StageEnd", "StageEndStatus",
		"Survival", "Broods", "MeanBrood", "BroodSD", "Date", "End"
	};

	TDevTimeCol CDevRateData::get_input(const std::string& name)
	{
		TDevTimeCol col = I_NOT_INIT;
		auto it = find_if(std::begin(INPUT_NAME), std::end(INPUT_NAME), [&](auto& s) {return boost::iequals(s, name); });
		if (boost::iequals("Variable", name))//also accept Variable instead of Stage
			it = std::begin(INPUT_NAME) + I_STAGE;

		if (boost::iequals("I", name) || boost::iequals("ID", name))//also accept I or ID instead of IID
			it = std::begin(INPUT_NAME) + I_IID;

		if (boost::iequals("T", name))//also accept T instead of Treatment
			it = std::begin(INPUT_NAME) + I_TREATMENT;

		if (boost::iequals("DOY", name))//also accept DOY instead of date
			it = std::begin(INPUT_NAME) + I_DATE;



		//if (boost::iequals("End", name))//also accept DOY instead of time
			//it = std::begin(INPUT_NAME) + I_DATE;


		if (it != std::end(INPUT_NAME))
			col = static_cast<TDevTimeCol>(std::distance(std::begin(INPUT_NAME), it));

		return col;
	}

	size_t CDevRateData::get_pos(TDevTimeCol c)const
	{

		size_t pos = NOT_INIT;
		auto it = std::find(m_input_pos.begin(), m_input_pos.end(), c);
		if (it != m_input_pos.end())
			pos = std::distance(m_input_pos.begin(), it);

		return pos;

	}


	double CDataRow::GetMaxTime() const
	{
		assert(find(I_TIME) != end() || find(I_MEAN_TIME) != end());
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

				assert(find(I_TIME_SD) != end() && find(I_N) != end());
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
				}
			}
		}

		return max_time;
	}

	double CDataRow::GetMinTime() const
	{
		assert(find(I_TIME) != end() || find(I_MEAN_TIME) != end());
		double min_time = 0;

		if (find(I_TIME) != end())
		{
			min_time = floor(at(I_TIME));
		}
		else if (find(I_MEAN_TIME) != end())
		{
			double mean = at(I_MEAN_TIME);
			if (mean > 0)
			{
				min_time = floor(mean);

				assert(find(I_TIME_SD) != end() && find(I_N) != end());
				size_t n = at(I_N);
				double sd = at(I_TIME_SD);

				if (sd > 0)//in case of NA
				{
					double cv = sd / mean;
					double sigma = CDevRateData::cv_2_sigma(cv, n);
					boost::math::lognormal_distribution<double> obsLogNormal(-0.5 * Square(sigma), sigma);

					double q = min(0.995, n / (n + 1.0));
					double RDR = quantile(obsLogNormal, q);

					min_time = floor(mean / (RDR * exp(Square(sigma))));//estimate of time (approx)
				}
			}
		}

		return min_time;
	}


	CDevRateData::CDevRateData()
	{
	}

	CDevRateData::~CDevRateData()
	{
	}

	void CDevRateData::clear()
	{
		//CObservation::clear();
		map::clear();
		m_input_pos.clear();
	}

	//ERMsg CDevRateData::load(const std::vector<std::string>& file_path)
	//{
	//	ERMsg msg;
	//	//clear();

	//	for (auto f : file_path)
	//	{
	//		msg += load(f);
	//	}

	//	return msg;
	//}

	ERMsg CDevRateData::load(const std::string& file_path)
	{
		ERMsg msg;

		//begin to read
		ifStream file;
		//std::locale utf8_locale = std::locale(std::locale::classic(), new std::codecvt_utf8<size_t>());


		//std::locale utf8_locale = std::locale(std::locale::classic(), new std::codecvt_utf8<size_t>());
		//file.imbue(utf8_locale);

		std::locale utf8_locale("en_US.UTF-8");
		file.imbue(utf8_locale);
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




	ERMsg CDevRateData::load(std::istream& io)
	{
		ERMsg msg;

		//Set static adult name
		CDataRow::SetAdultName(m_adult_name);


		m_input_pos.clear();

		CSVIterator loop(io, ",;\t", true, true);

		for (size_t i = 0; i < loop.Header().size(); i++)
			m_input_pos.push_back(get_input(loop.Header()[i]));


		if (!have_var(I_TREATMENT))
		{
			msg.ajoute("Mandatory missing column. \"Treatment\" (\"T\") must be define");
			return msg;
		}

		bool bDevTableTimeSeries = have_var(I_DATE);


		if (bDevTableTimeSeries)//case of Time Series development table
		{
			if (!have_var(I_IID))
				msg.ajoute("Mandatory missing column for time series table: \"iID\" (or \"i\") must be define");

			if (!have_var(I_STAGE))
				msg.ajoute("Mandatory missing column for time series table: \"Stage\" must be define");

			if (!msg)
				return msg;

			size_t posT = get_pos(I_TREATMENT);
			size_t posI = get_pos(I_IID);
			size_t posD = get_pos(I_DATE);
			size_t posS = get_pos(I_STAGE);
			size_t posB = get_pos(I_BROODS);


			assert(posT != NOT_INIT);
			assert(posI != NOT_INIT);
			assert(posD != NOT_INIT);
			assert(posS != NOT_INIT);
			bool bDOY = boost::iequals("DOY", loop.Header()[posD]);

			std::map< std::string, std::map< std::string, std::vector<std::pair<CTRef, std::string> >> > TI;

			//read all individuals time series
			for (; loop != CSVIterator() && msg; ++loop)
			{
				if (msg && !loop->empty())
				{
					if (loop->size() != loop.Header().size())
					{
						msg.ajoute("Bad number of column for line:" + loop->GetLastLine());
						return msg;
					}

					string strT = (*loop)[posT];
					string strI = (*loop)[posI];
					string strD = (*loop)[posD];
					string strS = (*loop)[posS];

					if (posB != NOT_INIT)
					{
						assert(Tokenize(strS, ":").size() == 1);
						string strB = (*loop)[posB];

						if (strB != "NA")
							strS += ":" + strB;
					}

					CTRef TRef;
					if (bDOY)
					{
						double DOY = stof(strD);
						TRef = CTRef(0, 0, 0, 0, CTM::HOURLY) + (int)Round(DOY * 24);
					}
					else
					{
						TRef.FromFormatedString(strD, "", "- :");
					}


					if (!TRef.IsValid())
					{
						msg.ajoute("Invalid date format at line:" + loop->GetLastLine());
						return msg;
					}

					TI[strT][strI].push_back(make_pair(TRef, strS));
				}
			}

			typedef std::map<std::tuple<CTRef, CTRef, CTRef, size_t>, size_t> TupleMap;
			std::map < string, std::map < string, TupleMap>> map_I;


			//convert development table into DevRateDataRow
			for (const auto& itT : TI)//for all treatments
			{
				const auto& T = itT.second;
				for (const auto& itI : T)//for all individuals of this treatment
				{

					const auto& I = itI.second;

					CObservation OO;

					OO.m_treatment = itT.first;
					OO.m_treatment_type = GetTreatmentType(OO.m_treatment);
					OO.m_i = itI.first;
					OO.m_i_temporal = IT_TIME_SERIES;

					if (OO.m_treatment_type == T_TRANSFER)
						OO.m_t_transfer = GetTreatmentTime1(OO.m_treatment);



					string first_stage = Tokenize(I.front().second, ":")[0];
					string last_stage = first_stage;
					CTRef lastTRef = I.front().first;
					CTRef TRef0a = lastTRef;
					CTRef TRef0b = lastTRef;
					CObservation O = OO;

					ASSERT(Tokenize(I.back().second, ":").size() >= 1);
					string i_terminal = Tokenize(I.back().second, ":")[0];


					for (size_t o = 0; o < I.size(); o++)//for all observations for this individual
					{
						CTRef TRef = I[o].first;

						vector<string> stage_brood = Tokenize(I[o].second, ":");
						assert(stage_brood.size() == 1 || stage_brood.size() == 2);

						string stage = stage_brood[0];
						bool bChangeStage = stage != last_stage;


						if (TRef - lastTRef < 0)
						{
							msg.ajoute("Error in data for iID " + itI.first + " for treatment " + itT.first);
							msg.ajoute("Invalid date for individual time series.  Date must be ordered correctly");
							msg.ajoute("Date: " + TRef.GetFormatedString());
							return msg;
						}


						//skip Dead when multiple Dead define
						if (GetStageEndStatus(last_stage) != SE_ALIVE && GetStageEndStatus(stage) != SE_ALIVE)
							continue;


						if (GetStageEndStatus(last_stage) != SE_ALIVE)
						{
							//
							msg.ajoute("Error in data for iID " + itI.first + " for treatment " + itT.first);
							msg.ajoute("Dead insect can't resuscitate");
							msg.ajoute("Date: " + TRef.GetFormatedString());

							return msg;
						}


						O.m_variable = last_stage;
						O.m_bIsChangingStage = bChangeStage;

						O.m_TRef0a = TRef0a.as(CTM::HOURLY);
						O.m_TRef0b = TRef0b.as(CTM::HOURLY);
						O.m_TRef1a = lastTRef.as(CTM::HOURLY);
						O.m_TRef1b = TRef.as(CTM::HOURLY);



						O[I_N] = 1.0;
						O[I_START] = (O.m_TRef1a - O.TRef0()) / 24.0;// Time in decimal days
						O[I_TIME] = (O.m_TRef1b - O.m_TRef1a) / 24.0;// Time in decimal days
						//Will be init in SetTobs
						if (O.m_treatment_type == T_FLUCTUATING)
							O.m_to = -999;

						O.m_tˉ¹ = (O.m_TRef1a - O.TRef0()) / 24.0;//Decimal daily
						O.m_t = (O.m_TRef1b - O.TRef0()) / 24.0;//Decimal daily


						if (stage_brood.size() == 2)
						{
							O[I_BROODS] = stof(stage_brood[1]);
							O.m_statsFecundity.Add(stof(stage_brood[1]));
						}

						if (!bChangeStage && o == I.size() - 1)
						{
							assert(GetStageEndStatus(stage) == SE_ALIVE);
							O.m_bStillAlive = true;
						}

						if (O[I_TIME] > 0 || O[I_BROODS] > 0)
							O.m_time_series.push_back(O);

						if ((bChangeStage || o == I.size() - 1))
						{
							assert(GetStageEndStatus(O.m_variable) == SE_ALIVE);
							assert(O.m_TRef1a.GetTM() == CTM::HOURLY);
							assert(O.m_TRef1b.GetTM() == CTM::HOURLY);
							assert(O.TRef0().GetTM() == CTM::HOURLY);
							assert(!O.m_time_series.empty());


							O.m_stage_end_status = GetStageEndStatus(stage);
							O.m_stage_at_end = stage;
							O.m_i_terminal = i_terminal;


							O[I_START] = 0;
							O[I_TIME] = (O.m_TRef1b - O.TRef0()) / 24.0;// Time in decimal days


							CDevRateTreatments& T = (*this)[O.m_variable][O.m_treatment];
							if (O.HaveBrood())
							{
								auto it = std::find_if(T.begin(), T.end(), [&](const CObservation& p)
									{
										return p.m_i == O.m_i;
									});

								if (it == T.end())
								{
									T.push_back(O);
								}
								else
								{
									msg.ajoute("Observation of an individual in a time series must be all grouped");
									msg.ajoute(string("Stage: " + O.m_variable + ", Treatment: " + O.m_treatment + ", iID: " + O.m_i));
									return msg;
								}
							}
							else
							{

								//find an individual with the same characteristic (ie.: to, tˉ¹, t, dead, censored)
								auto the_tuple = std::make_tuple(O.TRef0(), O.m_TRef1a, O.m_TRef1b, O.GetStageEndStatus());
								TupleMap::iterator pItTuple = map_I[O.m_variable][O.m_treatment].find(the_tuple);

								if (pItTuple == map_I[O.m_variable][O.m_treatment].end())
								{
									map_I[O.m_variable][O.m_treatment][the_tuple] = T.size();
									T.push_back(O);
								}
								else
								{
									T[pItTuple->second][I_N] += O[I_N];
								}

							}

							last_stage = stage;
							TRef0a = lastTRef;
							TRef0b = TRef;

							O.m_time_series.clear();
							O.m_time_series.push_back(O);//Add the last observation as first observation
						}


						lastTRef = TRef;
					}
				}
			}

		}
		else //case of individual development table (Time) or mean+sd+n
		{
			//check for mandatory columns
			if (!(have_var(I_START) && have_var(I_DATE_END)) && !have_var(I_TIME) && !have_var(I_MEAN_TIME))
				msg.ajoute("Mandatory missing column: \"Start\"/\"End\" or \"Time\" or \"MeanTime\"/\"TimeSD\"/\"n\" must be define");

			if (((have_var(I_START) && have_var(I_DATE_END)) ? 1 : 0 + have_var(I_TIME) ? 1 : 0 + have_var(I_MEAN_TIME) ? 1 : 0) > 1)
				msg.ajoute("Only one \"Start\"/\"End\" or \"Time\" or \"MeanTime\"/\"TimeSD\"/\"n\" must be define. No more than one");

			if (have_var(I_DATE_END) && !have_var(I_START))
				msg.ajoute("Start (as date) is mandatory when end is used");

			//if (have_var(I_TIME) && have_var(I_START))
				//msg.ajoute("\"Start\" can't be used when \"Time\" is define. \"Time\" must be the time since the beginning of the experimentation. ");

			if (!msg)
				return msg;

			size_t posS = get_pos(I_START);
			size_t posE = get_pos(I_DATE_END);

			typedef std::map<std::tuple<CTRef, CTRef, CTRef, size_t>, size_t> TupleMap;
			std::map < string, std::map < string, TupleMap>> map_I;

			for (; loop != CSVIterator() && msg; ++loop)
			{
				if (msg && !loop->empty())
				{
					if (loop->size() != loop.Header().size())
					{
						msg.ajoute("Bad number of column for line:" + loop->GetLastLine());
						return msg;
					}

					CObservation O;
					for (size_t i = 0; i < m_input_pos.size(); i++)
					{
						if (m_input_pos[i] != I_NOT_INIT)
						{
							if (m_input_pos[i] == I_VARIABLE)
								O.m_variable = (*loop)[i];
							else if (m_input_pos[i] == I_TREATMENT)
								O.m_treatment = (*loop)[i];
							else if (m_input_pos[i] == I_IID)
								O.m_i = (*loop)[i];
							else if (m_input_pos[i] == I_STAGE_END)
							{
								O.m_stage_at_end = (*loop)[i];
								O.m_i_terminal = O.m_stage_at_end;//??? depend if we have the iID or not
							}
							else if (m_input_pos[i] == I_STAGE_END_STATUS)
							{
								O.m_stage_end_status = GetStageEndStatus((*loop)[i]);
							}
							else
							{
								O[m_input_pos[i]] = ((*loop)[i] != "NA") ? ToDouble((*loop)[i]) : -999.0;
							}
						}
					}



					if (!have_var(I_START))
						O[I_START] = 0.0;

					if (!have_var(I_N))
						O[I_N] = 1.0;


					if (!have_var(I_STAGE_END_STATUS))
					{
						if (have_var(I_STAGE_END))
							O.m_stage_end_status = O.m_stage_at_end != "NA";
						else
							O.m_stage_end_status = SE_ALIVE;
						//O[I_STAGE_END_STATUS] = O.m_stage_end_status;
					}

					if (!have_var(I_OBS_INT))
						O[I_OBS_INT] = have_var(I_START) ? O[I_TIME] : 1.0;//default observed interval is daily



					bool bTimeSeries = have_var(I_START) && (have_var(I_DATE_END) || have_var(I_TIME)) && have_var(I_IID);
					bool bIndividual = have_var(I_TIME);

					O.m_treatment_type = GetTreatmentType(O.m_treatment);
					O.m_i_temporal = bTimeSeries ? IT_TIME_SERIES : bIndividual ? IT_INDIVIDUAL : IT_MEAN;
					//O.m_stage_at_end
					//O.m_i_terminal


					if (have_var(I_BROODS))
						O.m_statsFecundity.Add(O[I_BROODS]);

					else if (have_var(I_MEAN_BROOD))
						O.m_statsFecundity.Add(O[I_MEAN_BROOD]);

					if (have_var(I_START) && have_var(I_DATE_END))//time series
					{
						assert(posS != NOT_INIT);
						assert(posE != NOT_INIT);


						//read all individuals time series
						string strS = (*loop)[posS];
						string strE = (*loop)[posE];


						O.m_TRef0a.FromFormatedString(strS).Transform(CTM::HOURLY);
						O.m_TRef0b = O.m_TRef0a;
						O.m_TRef1b.FromFormatedString(strE).Transform(CTM::HOURLY);
						O.m_TRef1a = O.m_TRef1b - (int)Round(O[I_OBS_INT] * 24);

						assert(O[I_TIME] - O[I_OBS_INT] >= 0);

						//must be set by SetTobs if fluctuating data
						if (O.m_treatment_type == T_FLUCTUATING)
							O.m_to = -999;

						O.m_tˉ¹ = (O.m_TRef1a - O.TRef0()) / 24.0;
						O.m_t = (O.m_TRef1b - O.TRef0()) / 24.0;
					}
					else if (have_var(I_TIME))//individuals
					{


						O.m_TRef0a = CTRef(0, 0, 0, 0) + (int)Round(O[I_START] * 24);
						O.m_TRef0b = O.m_TRef0a;
						O.m_TRef1a = O.m_TRef0a + (int)Round((O[I_TIME] - O[I_OBS_INT]) * 24);
						O.m_TRef1b = O.m_TRef0a + (int)Round(O[I_TIME] * 24);


						assert(O[I_TIME] == -999 || (O[I_TIME] - O[I_OBS_INT] >= 0));

						if (O.m_treatment_type == T_FLUCTUATING)
							O.m_to = -999;

						if (O[I_TIME] > -999)
						{
							O.m_tˉ¹ = O[I_START] + O[I_TIME] - O[I_OBS_INT];
							O.m_t = O[I_START] + O[I_TIME];
						}
						else
						{
							O.m_tˉ¹ = -999;
							O.m_t = -999;
						}

					}
					else//mean+sd+n
					{
						O.m_TRef0a = CTRef(0, 0, 0, 0);
						O.m_TRef0b = O.m_TRef0a;
						O.m_TRef1a = O.m_TRef0a + (int)Round(O[I_MEAN_TIME] * 24);
						O.m_TRef1b = O.m_TRef1a;


						//assert(O[I_TIME] - O[I_OBS_INT] >= 0);

						if (O.m_treatment_type == T_FLUCTUATING)
							O.m_to = -999;

						O.m_tˉ¹ = O[I_MEAN_TIME];
						O.m_t = O[I_MEAN_TIME];
					}


					if (O.m_treatment_type == T_TRANSFER)
						O.m_t_transfer = GetTreatmentTime1(O.m_treatment);


					CDevRateTreatments& T = (*this)[O.m_variable][O.m_treatment];
					if (O.m_i_temporal == IT_TIME_SERIES)
					{
						auto it = std::find_if(T.begin(), T.end(), [&](const CObservation& p)
							{
								return p.m_i == O.m_i;
							});

						if (it == T.end())
						{
							O.m_time_series.push_back(O);
							T.push_back(O);
						}
						else
						{
							assert(!it->m_time_series.empty());

							if (O[I_START] < it->m_time_series.back()[I_START])
							{
								msg.ajoute("Observation of an individual in a time series must be all grouped");
								msg.ajoute(string("Stage: " + O.m_variable + ", Treatment: " + O.m_treatment + ", iID: " + O.m_i));
								return msg;
							}

							it->m_time_series.push_back(O);
							//Update start and time for time series
							(*it)[I_TIME] += O[I_TIME];
							(*it).m_tˉ¹ = O.m_tˉ¹;
							(*it).m_t = O.m_t;

							if (O.HaveBrood())
							{
								(*it)[I_BROODS] += O[I_BROODS];
								it->m_statsFecundity += O.m_statsFecundity;
							}
						}
					}
					else
					{
						auto the_tuple = std::make_tuple(O.TRef0(), O.m_TRef1a, O.m_TRef1b, O.GetStageEndStatus());
						TupleMap::iterator pItTuple = map_I[O.m_variable][O.m_treatment].find(the_tuple);


						//Brooding female must not be compacted
						if (O.HaveBrood() || pItTuple == map_I[O.m_variable][O.m_treatment].end())
						{
							map_I[O.m_variable][O.m_treatment][the_tuple] = T.size();
							T.push_back(O);
						}
						else
						{
							T[pItTuple->second][I_N] += O[I_N];
						}
					}
				}
			}





		}//else development table (Date)

		compute_stats();

		return msg;
	}

	void CDevRateData::compute_stats()
	{
		CDevRateData& me = *this;


		for (auto& itS : me)//for all Stages
		{
			auto& S = itS.second;
			for (auto& itT : S)//for all Treatment
			{
				auto& T = itT.second;
				for (auto& O : T)//for all Individuals/Groups
				{
					if (O.GetMinTime() > 0)
					{
						for (size_t n = 0; n < O[I_N]; n++)
						{
							if (O.m_i_temporal == IT_MEAN)
							{
								T.m_stats_time.Add(O[I_MEAN_TIME]);
								S.m_stats_Rate.Add(1.0 / O.GetMinTime());
								S.m_stats_Rate.Add(1.0 / O[I_MEAN_TIME]);
								S.m_stats_Rate.Add(1.0 / O.GetMaxTime());
							}
							else
							{
								T.m_stats_time.Add(O[I_TIME]);
								T.m_stats_rate.Add(1.0 / max(0.001, O[I_TIME]));
								S.m_stats_Rate.Add(1.0 / O[I_TIME]);
							}
						}
					}


					if (O.HaveBrood())
					{
						//for (auto& R : O)//for all Individuals/Groups
						assert(O.GetFecundity() == O.m_statsFecundity[SUM]);


						T.m_stats_Fecundity.Add(O.m_statsFecundity[SUM]);
						S.m_stats_Fecundity.Add(O.m_statsFecundity[SUM]);
					}
				}//for all observations
			}//For all treatments
		}//For all stages

		//Compute Q_TIME
		for (auto& itS : me)//for all Stages
		{
			auto& S = itS.second;
			for (auto& itT : S)//for all Treatment
			{
				auto& T = itT.second;

				//compute relative time and ptime
				vector<pair<double, CDataRow*>> RDT;
				vector<pair<double, CDataRow*>> RDR;

				for (auto& O : T)//for all Individuals/Groups
				{
					double time = O.m_i_temporal == IT_MEAN ? O[I_MEAN_TIME] : O[I_TIME];
					if (time > 0)
					{
						O.m_RDT = time / T.m_stats_time[MEAN];
						RDT.push_back(make_pair(O.m_RDT, &O));

						O.m_RDR = time / T.m_stats_rate[MEAN];
						RDR.push_back(make_pair(O.m_RDR, &O));
					}//If individual

				}//for all observations

				{
					sort(RDT.begin(), RDT.end());


					double N = T.m_stats_time[NB_VALUE];
					double cumsum = 0;
					for (auto it = RDT.begin(); it != RDT.end(); it++)
					{
						CDataRow& O = *(it->second);
						O.m_pTime = max(0.05, min(0.95, cumsum / N));
						cumsum += O[I_N];
					}
				}

				{
					sort(RDR.begin(), RDR.end());
					double N = T.m_stats_time[NB_VALUE];
					double cumsum = 0;
					for (auto it = RDR.begin(); it != RDR.end(); it++)
					{
						CDataRow& O = *(it->second);
						O.m_pRate = max(0.05, min(0.95, cumsum / N));
						cumsum += O[I_N];
					}
				}


			}//For all treatments


			if (!S.m_stats_Fecundity.empty())
			{
				vector<pair<double, CObservation*>> RF;//Relative fecundity
				double mean_fecundity = S.m_stats_Fecundity[MEAN];

				//compute Relative fecundity
				for (auto& itT : S)//for all Treatment
				{
					auto& T = itT.second;
					for (auto& O : T)//for all Individuals/Groups
					{
						if (O.HaveBrood())
						{
							double fecundity = O.m_statsFecundity[SUM];
							double RFi = fecundity / mean_fecundity;
							RF.push_back(make_pair(RFi, &O));
						}
					}
				}//for all treatment

				//sort by relative fecundity
				sort(RF.begin(), RF.end());
				//compute quartile and apply to all observation for this individual
				for (size_t i = 0; i < RF.size(); i++)
				{
					CObservation& O = *(RF[i].second);
					double p = (i + 1.0) / (RF.size() + 1);
					ASSERT(O[I_N] == 1);
					O.m_RF = RF[i].first;
					O.m_pFecundity = p;
					for (auto& R : O.m_time_series)//Update time series
					{
						R.m_RF = O.m_RF;
						R.m_pFecundity = O.m_pFecundity;
					}
				}
			}
		}//For all stages



	}



	void CDevRateData::compute_T_stats(const CTobsSeries& Tobs)
	{
		CDevRateData& me = *this;

		//for all treatment (many record of the same treatment will add many stats !!!)
		//for (auto& S : me)//for all Stages
		//{
		//	for (auto& T : S.second)//for all Treatment
		//	{
		//		for (auto& O : T.second)//for all Individuals/Groups
		//		{
		//			//for (auto R : O.m_rows)//For all rows
		//			//{
		//				//	const CDataRow& row = at(i);
		//			if (!O.m_bDead)
		//			{
		//				for (CTRef TRef = O.TRef0(); TRef <= O.m_TRef1b; TRef++)
		//					S.second.m_stats_Tobs += Tobs.at(O.m_treatment).at(TRef);
		//			}
		//
		//			//assert(false);//todo
		//		//}
		//		}
		//	}
		//}
	}

	//void CDevRateData::update_time(const CTobsSeries& Tobs)
	//{

	//	//string last_i = "";
	//	//for all rows, update time and time-1
	//	for (size_t i = 0; i < size(); i++)
	//	{
	//		CDataRow& row = at(i);
	//		const CTemporalVector& T = Tobs.at(row.m_treatment);
	//		assert(T.m_TRef.GetTM() == row.m_date.GetTM());

	//		//if (row.m_i != last_i)//the first observation of this individual
	//		//{
	//		double delta_t = (row.m_date - T.m_TRef) / 24.0;
	//		row.SetTimeˉ¹(row.t(true) + delta_t);
	//		row.SetTime(row.t(false) + delta_t);

	//		//	last_i = row.m_i;
	//		//}
	//		//else//for all other observations of this individual
	//		//{
	//		//
	//		//	//to set when the observed data is loaded
	//		//	double t0 = (at(i - 1).m_date - T.m_TRef) / 24.0;//in decimal daily since the beginning
	//		//	double t1 = (row.m_date - T.m_TRef) / 24.0;
	//		//
	//		//	row.SetTimeˉ¹(t0);
	//		//	row.SetTime(t1);
	//		//}
	//	}
	//}

	std::set<std::string> CDevRateData::GetAllTreatments()const
	{
		std::set<std::string> Treatments;

		for (const auto& S : *this)//for all Stages
		{
			for (const auto& T : S.second)//for all Treatment
				Treatments.insert(T.first);
		}

		return Treatments;
	}

	std::set<std::string> CDevRateData::GetAllStages()const
	{
		std::set<std::string> Stages;

		for (const auto& S : *this)//for all Stages
		{
			Stages.insert(S.first);
		}

		return Stages;
	}


	std::set<std::string> CDevRateData::GetAllInsectTerminal()const
	{
		std::set<std::string> stages;

		size_t nb_i = 0;
		for (const auto& itS : *this)//for all Stages
		{
			for (const auto& itT : itS.second)//for all Treatments
			{
				for (const auto& O : itT.second)//for all observation
				{
					//if (O.GetStageEndStatus() == SE_ALIVE)
					if (GetStageEndStatus(O.m_i_terminal) == SE_ALIVE)
						stages.insert(O.m_i_terminal);
				}
			}
		}


		return stages;
	}


	std::set<std::string> CDevRateData::GetAllStageEndStatus()const
	{
		std::set<std::string> status;

		size_t nb_i = 0;
		for (const auto& itS : *this)//for all Stages
		{
			for (const auto& itT : itS.second)//for all Treatments
			{
				for (const auto& O : itT.second)//for all observation
				{
					status.insert(STAGE_END_STATUS_NAME[O.GetStageEndStatus()]);
				}
			}
		}

		return status;

	}

	ERMsg CDevRateData::VerifyInsectStillAlive()const
	{
		ERMsg msg;

		size_t nb_i = 0;
		for (const auto& itS : *this)//for all Stages
		{
			for (const auto& itT : itS.second)//for all Treatments
			{
				for (const auto& O : itT.second)//for all individual observation
				{
					//string st = O.m_stage_end_status;
					if (O.m_bStillAlive)//if there are more than one observation at the end of the stage
					{
						string str = FormatA("Insect %s for treatment %s ended at stage %s and are still alive.", O.m_i.c_str(), itT.first.c_str(), O.m_i_terminal.c_str());
						msg.ajoute(str);
						continue;
					}
				}
			}
		}

		return msg;

	}

	size_t CDevRateData::GetNbObjects()const
	{
		size_t nb_o = 0;
		for (const auto& itS : *this)//for all Stages
		{
			for (const auto& itT : itS.second)//for all Treatments
				nb_o += itT.second.size();
		}

		return nb_o;
	}

	size_t CDevRateData::GetNbIndividuals()const
	{
		size_t nb_i = 0;
		for (const auto& itS : *this)//for all Stages
		{
			for (const auto& itT : itS.second)//for all Treatments
			{
				for (const auto& itO : itT.second)//for all Objects
					nb_i += itO.at(I_N);
			}
		}

		return nb_i;
	}



	//bool CDevRateData::IsAllTConstant()const
	//{
	//	bool bAllTConstant = true;

	//	//for (size_t i = 0; i < size() && bAllTConstant; i++)
	//		//bAllTConstant = at(i).m_type == T_CONSTANT;
	//	for (auto S : *this)//for all Stages
	//	{
	//		for (auto T : S.second)//for all Treatment
	//		{
	//			for (auto I : T.second)//for all Individuals/Groups
	//			{
	//				bAllTConstant = I.m_treatment_type == T_CONSTANT;
	//			}
	//		}
	//	}

	//	return bAllTConstant;
	//}

	double CDevRateData::ei(size_t n) { return pow(1.0 + 1.0 / n, n); }
	double CDevRateData::cv_2_sigma(double cv, size_t n)
	{
		static const double e = exp(1);
		static const double p[3] = { 0.528196, 2.373248, 3.493202 };//with 10 000 replication
		return e * cv * (1 - p[0] * sqrt(e - ei(n))) / (p[1] + cv * (1 - p[2] * sqrt(e - ei(n))));
	}

	double CDevRateData::GetDefaultSigma(const std::string& variable)const
	{
		const CDevRateData& me = *this;

		CStatistic stat_ws;
		CStatistic stat_n;

		//compute sigma
		for (auto& itT : me.at(variable))
		{
			auto& T = itT.second;

			bool bMean = T.i_temporal() == IT_MEAN;
			if (bMean)
			{
				for (auto& O : T)
				{
					ASSERT(have_var(I_MEAN_TIME) && have_var(I_TIME_SD) && have_var(I_N));

					double mean = O.at(I_MEAN_TIME);
					double sd = O.at(I_TIME_SD);
					double n = O.at(I_N);

					if (mean > 0 && sd > 0 && n > 0)
					{
						double cv = sd / mean;
						double sigma = cv_2_sigma(cv, n);

						stat_ws += n * sigma;
						stat_n += n;
					}
				}//for all observations
			}
			else
			{
				const CStatistic& stat = T.m_stats_time;//stat over all treatment for this stage
				double mean = stat[MEAN];
				double sd = stat[STD_DEV];
				double n = stat[NB_VALUE];

				if (mean > 0 && sd > 0 && n > 0)
				{
					double cv = sd / mean;
					double sigma = cv_2_sigma(cv, n);

					stat_ws += n * sigma;
					stat_n += n;
				}
			}
		}//for all treatment

		ASSERT(stat_n[SUM] > 0);
		double sigma = stat_ws[SUM] / stat_n[SUM];
		return sigma;
	}

	//double CDevRateData::GetSigmaBrood(TDevRateEquation  e, const std::vector<double>& X, const vector<double>& Tobs)const
	//{
	//	//double sigma = 0;
	//	CStatistic stat_sigma;

	//	assert(false);//todo

	//	assert(size() == 1);//only one stage 
	//	auto S = *(this->begin());


	//	for (auto& T : S.second)
	//	{
	//		CStatistic stat;

	//		for (auto& O : T.second)
	//		{

	//			//compute sigma
	//			if (O.m_i_temporal  == IT_TIME_SERIES)
	//			{

	//				//CStatistic stat_wm;
	//				//CStatistic stat_ws;
	//				//CStatistic stat_n;


	//				//for all treatment
	//				//for (auto T : me.at(variable))
	//				//{
	//				//	CStatistic& stat = T.second.m_stats_Time;//stat over all treatment for this stage

	//				//	for (auto it = m_statsBrood.begin(); it != m_statsBrood.end(); it++)
	//				//	{


	//				//		const std::map<std::string, CStatisticEx>& individuals = it->second;



	//						//for (auto O : I)

	//						//for all individuals
	//						//for (auto iit = individuals.begin(); iit != individuals.end(); iit++)
	//						//{
	//				const CStatistic& stat_broods = O.m_statsBrood;//stat over all treatment for this stage


	//				double broods = stat_broods[SUM];//sum of all observations
	//				double lambda = max(0.0, CDevRateEquation::GetRate(e, X, Tobs[0]));//remaining eggs
	//				double Fi = broods / (1 - exp(-lambda * stat_broods[NB_VALUE]));//a revoir estimer seulement....
	//				stat += Fi;



	//				//double mean = stat[MEAN];//mean of all individuals
	//				//double sd = stat[STD_DEV];
	//				//sigma = sqrt(log(Square(sd) / Square(mean) + 1));

	//				//sigma = sqrt(log(Square(sd) / Square(mean) + 1));
	//				//sigma = stat_ws[SUM] / stat_n[SUM];
	//				//F = stat_wm[SUM] / stat_n[SUM];



	//			}
	//			else if (O.m_i_temporal == IT_INDIVIDUAL)
	//			{
	//				//assert(O.m_rows.size() == 1);
	//				//auto R = O.m_rows[0];
	//				//CStatistic stat;
	//				//CStatistic stat_ws;
	//				//CStatistic stat_n;



	//				//for all treatment
	//				//for (size_t i = 0; i < size(); i++)
	//				//{
	//				//const CDataRow& row = at(i);
	//				double lambda = max(0.0, CDevRateEquation::GetRate(e, X, Tobs[0]));//remaining eggs
	//				double Fi = O.at(I_BROODS) / (1 - exp(-lambda * O.at(I_TIME)));
	//				stat += Fi;
	//				//}

	//				ASSERT(stat[SUM] > 0);

	//				//double mean = stat[MEAN];
	//				//double sd = stat[STD_DEV];

	//				////sigma = sqrt(log(Square(sd) / Square(mean) + 1));
	//				//stat_sigma += sqrt(log(Square(sd) / Square(mean) + 1));

	//			}
	//		}

	//		double mean = stat[MEAN];
	//		double sd = stat[STD_DEV];

	//		stat_sigma += sqrt(log(Square(sd) / Square(mean) + 1));
	//	}


	//	return stat_sigma[MEAN];
	//}

	double CDevRateData::GetSigmaFecundity(const string& adult_name)const
	{
		assert(find(adult_name) != end());



		double sigma = 0;
		CStatistic stat_wm;
		CStatistic stat_ws;
		CStatistic stat_n;

		auto S = *(this->find(adult_name));

		for (auto& itT : S.second)
		{
			auto& T = itT.second;

			bool bMean = T.i_temporal() == IT_MEAN;

			if (bMean)
			{
				for (auto& O : T)
				{
					double mean = O.at(I_MEAN_BROOD);
					double sd = O.at(I_BROOD_SD);
					double n = O.at(I_N);

					if (mean > 0 && sd > 0 && n > 0)
					{
						double cv = sd / mean;
						double sigma = cv_2_sigma(cv, n);

						stat_wm += n * mean;
						stat_ws += n * sigma;
						stat_n += n;
					}
				}
			}
			else
			{
				const CStatistic& stat = T.m_stats_Fecundity;//stat over all treatment for this stage
				double mean = stat[MEAN];
				double sd = stat[STD_DEV];
				double n = stat[NB_VALUE];

				if (mean > 0 && sd > 0 && n > 0)
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




	//void CDevRateData::set_Tobs(const CTobsSeries& Tobs)
	//{
	//	//find number of maximum days for all treatment

	//	//assign treatment temperature series to each row
	//	for (auto it = begin(); it != end(); it++)
	//	{
	//		it->SetTobs(std::make_shared<std::vector<double>>(Tobs.at(it->m_treatment)));
	//	}//al treatment
	//}





	const CxiVector& CDevRateTreatments::get_t_xi(TInputTemporal i_temporal, bool bUseDeadIn, bool bUseCencoredIn, bool bUseCasualtiesIn)const
	{
		assert(i_temporal != IT_TIME_SERIES);

		CDevRateTreatments& me = const_cast<CDevRateTreatments&>(*this);

		if (m_xi.empty())
		{
			for (const auto& O : *this)
			{
				assert(O.GetStageEndStatus() != NOT_INIT);

				size_t stage_end_status = O.GetStageEndStatus();
				if (O.m_bStillAlive)
					stage_end_status = SE_CENSORED;//Alive insect at the end of observation used as Censored

				bool bAdult = O.IsAdult();// || O.HaveBrood();
				bool bAlive = stage_end_status == SE_ALIVE;
				bool bUseDead = bUseDeadIn && stage_end_status == SE_DEAD;
				bool bUseCencored = bUseCencoredIn && stage_end_status == SE_CENSORED;
				bool bUseCasualties = bUseCasualtiesIn && stage_end_status == SE_CASUALTIES;
				//bool bNotCencored2 = bUseCencored || !O.m_bStillAlive;
				//bool bStageNotCompleted = bUseCencored || !O.m_bStillAlive || O.IsAdult() || O.HaveBrood();



				if (bAdult || bAlive || bUseDead || bUseCencored || bUseCasualties)
				{
					Cxi xi;
					xi.treatment_type = O.m_treatment_type;
					xi.stage_end_status = O.GetStageEndStatus();
					xi.adult = bAdult;
					xi.n = O.at(I_N);
					xi.to = O.m_to;
					xi.first[0] = O.m_tˉ¹;
					xi.first[1] = 0.0;
					xi.second[0] = O.m_t;
					xi.second[1] = 0.0;

					if (i_temporal == IT_INDIVIDUAL)
					{
						assert(O.m_t >= O.m_tˉ¹);

						xi.pTime = O.m_pTime;
					}
					else if (i_temporal == IT_MEAN)
					{
						xi.TimeSD = O.at(I_TIME_SD);
						if (xi.TimeSD <= 0)
							xi.TimeSD = xi.second[0] / 6.0;//an estimate of SD
					}

					me.m_xi.push_back(xi);
				}
			}

			me.m_xi.t_min = 1e10;
			me.m_xi.t_max = 0;

			for (const auto& xi : m_xi)
			{
				if (xi.treatment_type == T_FLUCTUATING)
				{
					if (xi.to < me.m_xi.t_min)
						me.m_xi.t_min = xi.to;

					if (xi.to + xi.second[0] > me.m_xi.t_max)
						me.m_xi.t_max = xi.to + xi.second[0];
				}
			}

		}

		return m_xi;
	}

	const CxiVector& CDevRateTreatments::get_f_xi(TInputTemporal i_temporal)const
	{

		CDevRateTreatments& me = const_cast<CDevRateTreatments&>(*this);

		if (m_xi.empty())
		{
			for (const auto& O : *this)
			{
				if (O.HaveBrood())
				{
					if (i_temporal == IT_TIME_SERIES)
					{
						for (const auto& R : O.m_time_series)
						{
							Cxi xi;
							xi.treatment_type = O.m_treatment_type;
							xi.stage_end_status = O.GetStageEndStatus();
							xi.n = R.at(I_N);
							xi.to = R.m_to;
							xi.first[0] = R.m_tˉ¹;
							xi.first[1] = 0.0;
							xi.second[0] = R.m_t;
							xi.second[1] = 0.0;

							assert(O.m_t >= O.m_tˉ¹);

							//xi.pTime = O.m_pTime;

							xi.broods = R.at(I_BROODS);
							xi.pFecundity = R.m_pFecundity;

							assert(O.m_pFecundity == R.m_pFecundity);
							me.m_xi.push_back(xi);
						}

					}
					else
					{
						//assert(i_temporal == IT_INDIVIDUAL);
						assert(O.m_t >= O.m_tˉ¹);

						Cxi xi;
						xi.treatment_type = O.m_treatment_type;
						xi.stage_end_status = O.GetStageEndStatus();
						xi.n = O.at(I_N);
						xi.to = O.m_to;
						xi.first[0] = xi.to;//Fecundity over the entire life of the female
						xi.first[1] = 0.0;
						xi.second[0] = O.m_t;
						xi.second[1] = 0.0;
						//xi.pTime = O.m_pTime;


						xi.broods = O.m_statsFecundity[SUM];
						xi.pFecundity = O.m_pFecundity;



						me.m_xi.push_back(xi);
					}


				}//have fecundity
			}//For all observation


			me.m_xi.t_min = 1e10;
			me.m_xi.t_max = 0;

			for (const auto& xi : m_xi)
			{
				if (xi.treatment_type == T_FLUCTUATING)
				{
					if (xi.to < me.m_xi.t_min)
						me.m_xi.t_min = xi.to;

					if (xi.to + xi.second[0] > me.m_xi.t_max)
						me.m_xi.t_max = xi.to + xi.second[0];
				}
			}

		}

		return m_xi;
	}


	double CDevRateTreatments::GetMaxLL(TDevRateEquation  e, const vector<double>& X)
	{

		return 0;
	}









	//**********************************************************************************************
	//CDevRateEqFile

	CDevRateEqFile::CDevRateEqFile()
	{
	}

	CDevRateEqFile::~CDevRateEqFile()
	{
	}

	ERMsg CDevRateEqFile::load(const std::string& file_path)
	{
		ERMsg msg;

		//begin to read
		ifStream file;
		//std::locale utf8_locale = std::locale(std::locale::classic(), new std::codecvt_utf8<size_t>());
		//file.imbue(utf8_locale);

		//file.imbue(std::locale("en_US.UTF-8"));
		std::locale utf8_locale("en_US.UTF-8");
		file.imbue(utf8_locale);
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

		for (CSVIterator loop(io, ",;\t", true, true); loop != CSVIterator() && msg; ++loop)
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
	//CInsectParameterization
	const char* CInsectParameterization::DATA_DESCRIPTOR = "InsectParameterizationData";
	const char* CInsectParameterization::XML_FLAG = "InsectParameterization";
	const char* CInsectParameterization::MEMBERS_NAME[NB_MEMBERS_EX] = { "FitType", "DevRateEquations", "SurvivalEquations", "FecundityEquations", "StageTableEquations","EquationsOptions", "InputFileName", "TobsFileName", "OutputFileName", "Control", "Counstrain_Tlo","Tlo_Values","Counstrain_Thi","ThiValues", "Fixe_F0", "F0_Value", "Fixe_T0", "T0_Value", "LimitMaxRate", "LimitMaxRateP", "AvoidNullRateInTobs", "UseOutputAsInput", "OutputAsIntputFileName", "ShowTrace" , "UseDead", "UseCencored", "UseCasualties", "AdultName", "SAPreset", "OptimMethod" };
	const char* CInsectParameterization::TYPE_NAME[CInsectParameterization::NB_FIT_TYPE] = { "Development time","Survival", "Oviposition", "Stages table" };


	const int CInsectParameterization::CLASS_NUMBER = CExecutableFactory::RegisterClass(CInsectParameterization::GetXMLFlag(), &CInsectParameterization::CreateObject);


	CInsectParameterization::CInsectParameterization()
	{
		Reset();
	}

	CInsectParameterization::~CInsectParameterization()
	{
	}


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
		m_eqStageTable.set();
		m_eq_options.clear();
		m_inputFileName.clear();
		m_TobsFileName.clear();
		m_outputFileName = "%i";//same as input file name

		m_bConstrainTlo = false;
		m_Tlo = { 0,10 };
		m_bConstrainThi = false;
		m_Thi = { 35,45 };
		m_adult_name = "Adult|Male|Female";

		m_bFixeF0 = false;
		m_F0 = 100;
		m_bFixeT0 = false;
		m_T0 = 0;
		m_bLimitMaxRate = false;
		m_LimitMaxRateP = 1.2;
		m_bAvoidNullRateInTobs = false;


		m_bUseOutputAsInput = false;
		m_outputAsIntputFileName.clear();
		m_bShowTrace = false;
		m_bUseDead = false;
		m_bUseCencored = false;
		m_bUseCasualties = false;



		//Warning: parameters for Mean+SD+n need more accuracy than the individual one.
		m_SA_preset = SA_HI_SLOW;
		m_optim_method = OM_MLL;

		m_SAOptions = GetDefaultSAOptions((TSAOption)m_SA_preset);

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
			m_eqStageTable = in.m_eqStageTable;
			m_eq_options = in.m_eq_options;
			m_inputFileName = in.m_inputFileName;
			m_TobsFileName = in.m_TobsFileName;
			m_outputFileName = in.m_outputFileName;

			m_bConstrainTlo = in.m_bConstrainTlo;
			m_Tlo = in.m_Tlo;
			m_bConstrainThi = in.m_bConstrainThi;
			m_Thi = in.m_Thi;
			m_adult_name = in.m_adult_name;




			m_bFixeF0 = in.m_bFixeF0;
			m_F0 = in.m_F0;
			m_bFixeT0 = in.m_bFixeT0;
			m_T0 = in.m_T0;


			m_bLimitMaxRate = in.m_bLimitMaxRate;
			m_LimitMaxRateP = in.m_LimitMaxRateP;
			m_bAvoidNullRateInTobs = in.m_bAvoidNullRateInTobs;



			m_bUseOutputAsInput = in.m_bUseOutputAsInput;
			m_outputAsIntputFileName = in.m_outputAsIntputFileName;
			m_bShowTrace = in.m_bShowTrace;
			m_bUseDead = in.m_bUseDead;
			m_bUseCencored = in.m_bUseCencored;
			m_bUseCasualties = in.m_bUseCasualties;



			m_SAOptions = in.m_SAOptions;
			m_SA_preset = in.m_SA_preset;
			m_optim_method = in.m_optim_method;

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
		if (m_eqStageTable != in.m_eqStageTable) bEqual = false;

		if (m_eq_options != in.m_eq_options)bEqual = false;
		if (m_inputFileName != in.m_inputFileName) bEqual = false;
		if (m_TobsFileName != in.m_TobsFileName) bEqual = false;

		if (m_outputFileName != in.m_outputFileName) bEqual = false;
		if (m_SAOptions != in.m_SAOptions)bEqual = false;
		if (m_SA_preset != in.m_SA_preset)bEqual = false;
		if (m_optim_method != in.m_optim_method)bEqual = false;

		if (m_bConstrainTlo != in.m_bConstrainTlo)bEqual = false;
		if (m_Tlo != in.m_Tlo)bEqual = false;
		if (m_bConstrainThi != in.m_bConstrainThi)bEqual = false;
		if (m_Thi != in.m_Thi)bEqual = false;
		if (m_adult_name != in.m_adult_name)bEqual = false;




		if (m_bFixeF0 != in.m_bFixeF0)bEqual = false;
		if (m_F0 != in.m_F0)bEqual = false;
		if (m_bFixeT0 != in.m_bFixeT0)bEqual = false;
		if (m_T0 != in.m_T0)bEqual = false;
		if (m_bLimitMaxRate != in.m_bLimitMaxRate)bEqual = false;
		if (m_LimitMaxRateP != in.m_LimitMaxRateP)bEqual = false;
		if (m_bAvoidNullRateInTobs != in.m_bAvoidNullRateInTobs)bEqual = false;
		if (m_bUseOutputAsInput != in.m_bUseOutputAsInput)bEqual = false;
		if (m_outputAsIntputFileName != in.m_outputAsIntputFileName)bEqual = false;
		if (m_bShowTrace != in.m_bShowTrace)bEqual = false;
		if (m_bUseDead != in.m_bUseDead)bEqual = false;
		if (m_bUseCencored != in.m_bUseCencored)bEqual = false;
		if (m_bUseCasualties != in.m_bUseCasualties)bEqual = false;



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

			if (m_fitType == F_DEV_TIME)
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
			else if (m_fitType == F_STAGES_TABLE)
			{
				//for all equation
				for (size_t e = 0; e < m_eqStageTable.size(); e++)
				{
					if (m_eqStageTable.test(e))
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


	//template <typename T, template <typename Element, typename Alloc = std::allocator<Element>> class ContainerType>
	template <typename T> inline
		std::string ToString2(const std::set<T>& v, const std::string& be = "", const std::string& sep = ",", const std::string& en = "")
	{
		std::string str = be;
		for (typename std::set<T>::const_iterator it = v.begin(); it != v.end(); it++)
		{
			if (it != v.begin())
				str += sep;
			str += ToString(*it);
		}

		str += en;

		return str;
	}




	ERMsg CInsectParameterization::Execute(const CFileManager& fileManager, CCallback& callback)
	{
		ERMsg msg;


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
				CDBMetadata& metadata = resultDB.GetMetadata();
				metadata.SetLocations(info.m_locations);
				metadata.SetParameterSet(info.m_parameterset);
				metadata.SetNbReplications(info.m_nbReplications);
				metadata.SetTPeriod(info.m_period);
				metadata.SetOutputDefinition(info.m_variables);

				callback.AddMessage(FormatMsg(IDS_SIM_CREATE_DATABASE, m_name));
				callback.AddMessage(resultDB.GetFilePath(), 1);


				if (m_fitType == F_STAGES_TABLE)
				{
					msg += ExecuteStageTable(fileManager, callback);
				}
				else
				{
					msg += ExecuteOther(fileManager, callback);
				}


			}

			resultDB.Close();
		}



		return msg;
	}

	ERMsg CInsectParameterization::ExecuteStageTable(const CFileManager& fileManager, CCallback& callback)
	{
		ERMsg msg;

		string output_file_name = !m_outputFileName.empty() ? m_outputFileName : GetFileTitle(m_inputFileName);
		SetFileExtension(output_file_name, ".csv");
		ReplaceString(output_file_name, "%i", GetFileTitle(m_inputFileName));
		string outputFilePath = fileManager.GetOutputPath() + output_file_name;

		m_SA_ctrl = GetSAOptions();

		//Development time only
		bool bLogLikelyhoude = m_SA_ctrl.m_statisticType == LIKELIHOOD;


		//set vMiss value
		m_SA_ctrl.SetVMiss(m_SA_ctrl.AdjustFValue(DBL_MAX));


		//Load temperature profile
		//if (!m_TobsFileName.empty())
		//{
			//string TobsFilePath = fileManager.Input().GetFilePath(m_TobsFileName);
			//msg += m_Tobs.load(TobsFilePath);
		//}


		string inputFilePath = fileManager.Input().GetFilePath(m_inputFileName);
		//Load observations file
		CCStagesTableData* pData = &m_stages_table;

		//pData->m_adult_name = m_adult_name;
		msg = pData->load(inputFilePath);

		//CFitOutputVector output;
		TDevRateEquation eq = CDevRateEquation::eq(CDevRateEquation::Briere1_1999);
		string e_name = CDevRateEquation::GetEquationName(eq);

		CSAParameterVector params0 = CDevRateEquation::GetParameters(eq);
		if (m_eq_options.find(e_name) != m_eq_options.end())
		{
			ASSERT(m_eq_options[e_name].size() == CDevRateEquation::GetParameters(eq).size());
			params0 = m_eq_options[e_name];
		}


		double sigma = 0.15;
		params0.push_back(CSAParameter("sigma", sigma, sigma / SIGMA_FACTOR1, sigma * SIGMA_FACTOR2));

		CFitOutput output("StageTable", eq, params0);

		//callback.PushTask("Search optimum for " + to_string(TYPE_NAME[m_fitType]) + ": " + to_string(nb_e) + " equations  ", output.size());
		//
		//
		//
		//#ifndef _DEBUG
		//#pragma omp parallel for num_threads(CTRL.m_nbMaxThreads) 
		//#endif
		//		for (__int64 i = 0; i < (__int64)output.size(); i++)
		//		{
		//#pragma omp flush(msg)
		//			if (msg)
		//			{
		msg += Optimize(output.m_variable, output.m_equation, output.m_parameters, output.m_computation, callback);
		//
		//#pragma omp critical(WRITE_INFO)
		//				{
		//
		//					callback.AddMessage(output[i].m_variable + ": " + CDevRateEquation::GetEquationName(CDevRateEquation::eq(output[i].m_equation)));
		//
		//					WriteInfo(output[i].m_parameters, output[i].m_computation, callback);
		//				}
		//
		//
		//				//fileManager, result, callback
		//				msg += callback.StepIt();
		//#pragma omp flush(msg)
		//			}
		//		}
		//
		//		callback.PopTask();


		return msg;
	}

	ERMsg CInsectParameterization::ExecuteOther(const CFileManager& fileManager, CCallback& callback)
	{
		ERMsg msg;

		string output_file_name = !m_outputFileName.empty() ? m_outputFileName : GetFileTitle(m_inputFileName);
		SetFileExtension(output_file_name, ".csv");
		ReplaceString(output_file_name, "%i", GetFileTitle(m_inputFileName));
		string outputFilePath = fileManager.GetOutputPath() + output_file_name;

		m_SA_ctrl = GetSAOptions();

		//Development time only
		bool bLogLikelyhoude = m_SA_ctrl.m_statisticType == LIKELIHOOD;

		//set vMiss value
		m_SA_ctrl.SetVMiss(m_SA_ctrl.AdjustFValue(DBL_MAX));


		//Load temperature profile
		if (!m_TobsFileName.empty())
		{
			string TobsFilePath = fileManager.Input().GetFilePath(m_TobsFileName);
			msg += m_Tobs.load(TobsFilePath);
		}






		string inputFilePath = fileManager.Input().GetFilePath(m_inputFileName);
		//Load observations file
		CDevRateData* pData = GetCurrentDataFile();
		pData->m_adult_name = m_adult_name;
		msg = pData->load(inputFilePath);



		if (msg)
		{
			callback.AddMessage("Data have: ");
			callback.AddMessage(to_string(pData->GetAllStages().size()) + " Stages: " + ToString2(pData->GetAllStages()));
			callback.AddMessage(to_string(pData->GetAllTreatments().size()) + " Treatments: " + ToString2(pData->GetAllTreatments()));
			callback.AddMessage(to_string(pData->GetNbObjects()) + " Objects");
			callback.AddMessage(to_string(pData->GetNbIndividuals()) + " Individuals");
			callback.AddMessage(to_string(pData->GetAllStageEndStatus().size()) + " Stage Status: " + ToString2(pData->GetAllStageEndStatus()));
			callback.AddMessage(to_string(pData->GetAllInsectTerminal().size()) + " Terminal Stage: " + ToString2(pData->GetAllInsectTerminal()));

			callback.AddMessage(pData->VerifyInsectStillAlive());

			//complete temperature for non fluctuating treatment
			m_Tobs.generate(*pData);

			//Set temperature to treatment
			msg += pData->SetTobs(m_Tobs);
		}


		if (m_fitType == F_DEV_TIME &&
			!(m_devTime.have_var(I_DATE) || m_devTime.have_var(I_TIME) || (m_devTime.have_var(I_MEAN_TIME) && m_devTime.have_var(I_TIME_SD) && m_devTime.have_var(I_N))))
		{
			msg.ajoute(string("Calibration of ") + TYPE_NAME[m_fitType] + " need time series table, individual Time or MeanTime, TimeSD and n");
		}
		else if (m_fitType == F_FECUNDITY)
		{
			if (m_fecundity.find(this->m_adult_name) == m_fecundity.end())
				msg.ajoute(string("Adult stage \"") + m_adult_name + "\" not find in the input file");
		}


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


		CResult result;
		CFitOutputVector output;
		set<string> variables = pData->GetAllStages();




		//for all stage
		for (auto v = variables.begin(); v != variables.end() && msg; v++)
		{
			//for all equation
			if (m_fitType == F_DEV_TIME)
			{
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


						double sigma = m_devTime.GetDefaultSigma(*v);
						if (sigma == 0)
						{
							msg.ajoute("sigma=0. Invalid input Time or MeanTime/TimeSD.");
							return msg;
						}


						if (m_devTime.have_time_series() || m_devTime.have_individual())
						{
							params0.push_back(CSAParameter("sigma", sigma, sigma / SIGMA_FACTOR1, sigma * SIGMA_FACTOR2));
						}
						else
						{
							//fixed sigma here
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
										//if sigma, round to avoid bound problem
										if (it->first != "sigma")
											iit->m_initialValue = it->second;
										else
											iit->m_initialValue = Round(it->second, 3);

									}
								}

							}
						}

						CSAParameterVector params = params0;
						CFitOutput out(*v, eq, params);
						ERMsg msgEq = InitialiseComputationVariable(out.m_variable, out.m_equation, out.m_parameters, out.m_computation, callback);
						if (msgEq)
							output.push_back(out);
						else
							callback.AddMessage(msgEq);

						msg += callback.StepIt(0);
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
						ERMsg msgEq = InitialiseComputationVariable(out.m_variable, out.m_equation, out.m_parameters, out.m_computation, callback);
						if (msgEq)
							output.push_back(out);
						else
							callback.AddMessage(msgEq);

						msg += callback.StepIt(0);
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


						CSAParameter pto("F_to", 0, 0, 200);
						if (m_bFixeT0)
							pto = CSAParameter("F_to", m_T0, m_T0, m_T0);

						CSAParameter pFo("Fo", 100, 1, 1000);
						if (m_bFixeF0)
							pFo = CSAParameter("Fo", m_F0, m_F0, m_F0);




						double sigma = m_fecundity.GetSigmaFecundity(m_adult_name);
						if (m_fecundity.have_individual() || m_fecundity.have_time_series())
						{
							params.push_back(pto);
							params.push_back(pFo);
							params.push_back(CSAParameter("sigma", sigma, sigma / SIGMA_FACTOR1, sigma * SIGMA_FACTOR2));
							//params.push_back(CSAParameter("sigma", 0.2, 0.01, 0.9));

						}
						else
						{
							params.push_back(pto);
							params.push_back(pFo);
							params.push_back(CSAParameter("sigma", sigma, sigma, sigma));//fix sigma here

						}

						auto it = find_if(params.begin(), params.end(), [](const CSAParameter& m) -> bool { return m.m_name == "psi"; });
						if (it != params.end())
							it->m_bounds.m_upperBound = 10;


						CFitOutput out(*v, eq, params);
						ERMsg msgEq = InitialiseComputationVariable(out.m_variable, out.m_equation, out.m_parameters, out.m_computation, callback);
						if (msgEq)
							output.push_back(out);
						else
							callback.AddMessage(msgEq);

						msg += callback.StepIt(0);
					}
				}
			}
		}

		size_t nb_e = 0;
		if (m_fitType == F_DEV_TIME)
			nb_e = m_eqDevRate.count();
		else if (m_fitType == F_SURVIVAL)
			nb_e = m_eqSurvival.count();
		else if (m_fitType == F_FECUNDITY)
			nb_e = m_eqFecundity.count();


		if (!msg)
			return msg;


		if (m_bShowTrace && (variables.size() > 1 || nb_e > 1))
		{
			m_bShowTrace = false;
			callback.AddMessage("Warning: Show trace can't be active when multiple stages or equations");
		}



		callback.PushTask("Search optimum for " + to_string(TYPE_NAME[m_fitType]) + ": " + to_string(variables.size()) + " stages x " + to_string(nb_e) + " equations  = " + to_string(output.size()) + " curve to fits", output.size());



#ifndef _DEBUG
#pragma omp parallel for num_threads(CTRL.m_nbMaxThreads) 
#endif
		for (__int64 i = 0; i < (__int64)output.size(); i++)
		{
#pragma omp flush(msg)
			if (msg)
			{
				msg += Optimize(output[i].m_variable, output[i].m_equation, output[i].m_parameters, output[i].m_computation, callback);

#pragma omp critical(WRITE_INFO)
				{
					if (m_fitType == F_DEV_TIME)
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

		//begin to read
		ofStream file;


		ERMsg msg_file = file.open(outputFilePath);
		if (msg_file)//save result event if user cancel or error
		{

			sort(output.begin(), output.end(), [](const CFitOutput& a, const CFitOutput& b) {return a.m_computation.m_Fopt > b.m_computation.m_Fopt; });
			if (bLogLikelyhoude)
				file << "Stage,EqName,P,Eq,Math,n,k,maxLL,AICc" << endl;
			else
				file << "Stage,EqName,P,Eq,Math,n,k,RSS,R2" << endl;

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


						if (m_fitType == F_DEV_TIME)
						{
							TDevRateEquation eq = CDevRateEquation::eq(output[i].m_equation);
							name = CDevRateEquation::GetEquationName(eq);
							R_eq = CDevRateEquation::GetEquationR(eq);
							R_math = CDevRateEquation::GetMathPlot(eq);
							P = to_string(CDevRateEquation::GetParameters(eq, Xopt));
							P += " sigma=" + to_string(Xopt.back());
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

							P += " Fto=" + to_string(Xopt[Xopt.size() - 3]);
							P += " Fo=" + to_string(Xopt[Xopt.size() - 2]);
							P += " sigma=" + to_string(Xopt[Xopt.size() - 1]);
						}


						file << output[i].m_variable << "," << name << "," << P << ",\"" << R_eq << "\",\"" << R_math << "\",";

						if (bLogLikelyhoude)
							file << output[i].m_computation.m_n_opt << "," << output[i].m_computation.m_k_opt << "," << output[i].m_computation.m_MLLopt << "," << output[i].m_computation.m_AICCopt;
						else
							file << output[i].m_computation.m_Sopt[NB_VALUE] << "," << output[i].m_computation.m_Xopt.size() << "," << output[i].m_computation.m_Sopt[RSS] << "," << output[i].m_computation.m_Sopt[STAT_R²];


						file << endl;
					}
				}
			}

			file.close();
		}

		msg += msg_file;



		m_devTime.clear();
		m_Tobs.clear();
		m_devTime.clear();
		m_survival.clear();
		m_fecundity.clear();

		return msg;
	}


	CDevRateData* CInsectParameterization::GetCurrentDataFile()
	{
		CDevRateData* pData = nullptr;
		if (m_fitType == F_DEV_TIME)
		{
			pData = &m_devTime;
		}
		else if (m_fitType == F_SURVIVAL)
		{
			pData = &m_survival;
		}
		else if (m_fitType == F_FECUNDITY)
		{
			pData = &m_fecundity;
		}

		return pData;
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

		computation.Initialize(m_SA_ctrl.m_T, m_SA_ctrl.m_NEPS, m_SA_ctrl.GetVMiss());


		//try with random (uniform and normally distributed) values 
		bool bValid = false;
		size_t x = 0;
		static const size_t NB_TRY_I = 100000;
		//static const size_t NB_TRY_II = 50000;


		vector<double> rand_sd(computation.m_XP.size(), 0.0);

		//+ NB_TRY_II

		while (x < NB_TRY_I && !bValid && msg)//try to find valid parameters
		{
			//  Generate XP, the trial value of X. 
			// 1. try with initial value
			// 2. try with param around initial value with a growing normal up to (-0.5, 0.5)
			// 3. finally try with random value in variable range
			for (size_t i = 0; i < computation.m_XP.size(); i++)
			{
				bool bRand = x % (i + 1) == 0;
				//double rd = (x == 0) ? 0 : (x < NB_TRY_I) ? RandNormal(0.0, 0.15 * double(x) / NB_TRY_I) : Randu();
				//double ini = (x < NB_TRY_I) ? parameters[i].m_initialValue : parameters[i].m_bounds.GetLowerBound();
				double rd = bRand ? RandNormal(0.0, 0.15 * double(x) / NB_TRY_I) : RandNormal(0.0, 0.25 * double(NB_TRY_I - x) / NB_TRY_I);
				double ini = parameters[i].m_initialValue;
				double ext = parameters[i].m_bounds.GetExtent();
				double value = ini + ext * rd;
				computation.m_XP[i] = parameters[i].m_bounds.LimitTo(value);
			}

			bValid = IsParamValid(s, e, computation.m_XP) && IsRateValid(s, e, computation.m_XP, NULL_RATE_THRESHOLD);
			if (bValid)
				bValid = GetFValue(s, e, computation);

			msg += callback.StepIt(0);
			x++;
		}


		if (!bValid)
		{
			string eq_name;
			if (m_fitType == F_DEV_TIME)
				eq_name = CDevRateEquation::GetEquationName(CDevRateEquation::eq(e));
			else if (m_fitType == F_SURVIVAL)
				eq_name = CSurvivalEquation::GetEquationName(CSurvivalEquation::eq(e));
			else if (m_fitType == F_FECUNDITY)
				eq_name = CDevRateEquation::GetEquationName(CDevRateEquation::eq(e));

			msg.ajoute("Unable to find initial parameters: (" + s + ", " + eq_name + ")");

		}

		if (msg)
		{
			assert(computation.m_FP != m_SA_ctrl.GetVMiss());

			computation.m_NFCNEV++;

			computation.m_X = computation.m_XP;
			computation.m_S = computation.m_SP;
			computation.m_F = computation.m_FP;
			computation.m_AICC = computation.m_AICCP;
			computation.m_MLL = computation.m_MLLP;
			computation.m_n = computation.m_n_P;
			computation.m_k = computation.m_k_P;

			computation.m_Xopt = computation.m_XP;
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

		double F = m_SA_ctrl.m_bMax ? computation.m_Fopt : -computation.m_Fopt;
		bool bLogLikelyhoude = m_SA_ctrl.m_statisticType == LIKELIHOOD;

		//		CStatistic stat;
			//	for (size_t i = 0, j = 0; i < computation.m_Xstat.size(); i++)
				//	stat += 100.0 * computation.m_Xstat[j][RANGE] / computation.m_Xstat[j][MEAN];

		if (bLogLikelyhoude)
			line = FormatA("N=%10d\tT=%12.8f\tF=%8.5lf\tP=%8.5lf\nNbVal=%d\tAICc=%8.5lf\tmaxLL=%8.5lf", computation.m_NFCNEV, computation.m_T, F, computation.m_Pstat[HIGHEST], computation.m_n_opt, computation.m_AICCopt, computation.m_MLLopt);
		else if (computation.m_Sopt[NB_VALUE] > 0)
			line = FormatA("N=%10d\tT=%12.8f\tF=%8.5lf\tP=%8.5lf\nNbVal=%6.0lf\tBias=%8.5lf\tMAE=%8.5lf\tRMSE=%8.5lf\tCD=%8.5lf\tR²=%8.5lf", computation.m_NFCNEV, computation.m_T, F, computation.m_Pstat[HIGHEST], computation.m_Sopt[NB_VALUE], computation.m_Sopt[BIAS], computation.m_Sopt[MAE], computation.m_Sopt[RMSE], computation.m_Sopt[COEF_D], computation.m_Sopt[STAT_R²]);
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
		double F = m_SA_ctrl.m_bMax ? computation.m_Fopt : -computation.m_Fopt;
		bool bLogLikelyhoude = m_SA_ctrl.m_statisticType == LIKELIHOOD;


		if (bLogLikelyhoude)
			line = FormatA("N=%10d\tT=%12.8f\tF=%8.5lf\tP=%8.5lf\nNbVal=%d\tAICc=%8.5lf\tmaxLL=%8.5lf", computation.m_NFCNEV, computation.m_T, F, computation.m_Pstat[HIGHEST], computation.m_n_opt, computation.m_AICCopt, computation.m_MLLopt);
		else if (computation.m_Sopt[NB_VALUE] > 0)
			line = FormatA("N=%10d\tT=%12.8f\tF=%8.5lf\tP=%8.5lf\nNbVal=%6.0lf\tBias=%8.5lf\tMAE=%8.5lf\tRMSE=%8.5lf\tCD=%8.5lf\tR²=%8.5lf", computation.m_NFCNEV, computation.m_T, F, computation.m_Pstat[HIGHEST], computation.m_Sopt[NB_VALUE], computation.m_Sopt[BIAS], computation.m_Sopt[MAE], computation.m_Sopt[RMSE], computation.m_Sopt[COEF_D], computation.m_Sopt[STAT_R²]);
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
	//    3. RMARIN and RANMAR are designed to be portable; they should not
	//       cause any problems.
	ERMsg CInsectParameterization::Optimize(string s, size_t  e, CSAParameterVector& parameters, CComputationVariable& computation, CCallback& callback)
	{
		ERMsg msg;

		//TDevRateEquation eq = CDevRateEquation::eq(e);

		if (m_bShowTrace)
			WriteInfoEx(parameters, computation, callback);

		//  Initialize the random number generator RANMAR.
		CRandomizeNumber random(m_SA_ctrl.m_seed1, m_SA_ctrl.m_seed2);

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
			//long LNOBDS = 0;

			//randomize order of the parameters to not always begin with the same parameter
			vector < vector<size_t>> H(m_SA_ctrl.m_NT * m_SA_ctrl.m_NS);
			for (size_t j = 0; j < H.size(); j++)
				H[j] = Shuffle(computation.m_X.size());


			int NT = L <= m_SA_ctrl.m_nbSkipLoop ? 2 : m_SA_ctrl.m_NT;
			for (int M = 0; M < NT && msg; M++)
			{
				vector<int> NACP;
				NACP.insert(NACP.begin(), computation.m_X.size(), 0);

				int NS = L <= m_SA_ctrl.m_nbSkipLoop ? 2 : m_SA_ctrl.m_NS;
				for (size_t j = 0; j < NS && msg; j++)
				{
					for (size_t hh = 0; hh < NACP.size() && msg; hh++)
					{
						size_t h = H[M * NACP.size() + j][hh];
						//if (j % 2)
							//h = NACP.size() - h - 1;//to avoid always evaluate h in the same order

						//  If too many function evaluations occur, terminate the algorithm.
						if (computation.m_NFCNEV >= m_SA_ctrl.m_MAXEVL)
						{
							callback.AddMessage("Number of function evaluations (NFCNEV) is greater than the maximum number (MAXEVL).");
							//msg.ajoute();
							return msg;
						}

						//computation.m_XP.resize(computation.m_X.size());
						computation.m_XP = computation.m_X;
						computation.m_SP.Reset();
						computation.m_FP = m_SA_ctrl.GetVMiss();

						size_t nb_not_valid = 0;
						double factorVM = 1;
						bool bValid = false;
						//do
						while (!bValid)
						{
							nb_not_valid++;
							if ((nb_not_valid % 20) == 0)
								factorVM *= 0.9;//temporary decrease VM when too mush not valid

							computation.m_XP[h] = computation.m_X[h] + (random.Ranmar() * 2.0 - 1.0) * computation.m_VM[h] * factorVM;
							//  If XP is out of bounds, select a point in bounds for the trial.
							if (computation.m_bounds[h].IsOutOfBound(computation.m_XP[h]))
							{
								computation.m_XP[h] = computation.m_bounds[h].GetLowerBound() + computation.m_bounds[h].GetExtent() * random.Ranmar();
								computation.m_NOBDS++;
							}

							bValid = IsParamValid(s, e, computation.m_XP) && IsRateValid(s, e, computation.m_XP, NULL_RATE_THRESHOLD);
						}


						//  Evaluate the function with the trial point XP and return as FP.
						bValid = GetFValue(s, e, computation);
						assert(bValid);

						computation.m_XPstat[h] += computation.m_XP[h];
						computation.m_VMstat[h] += computation.m_VM[h];

						assert(computation.m_FP != m_SA_ctrl.GetVMiss());

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

						computation.m_NFCNEV++;


						msg += callback.StepIt(0);
					} //H
				} //J



				//  Adjust VM so that approximately half of all evaluations are accepted.
				ASSERT(computation.m_VM.size() == NACP.size());
				for (int I = 0; I < computation.m_VM.size(); I++)
				{
					double RATIO = double(NACP[I]) / double(m_SA_ctrl.m_NS);
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

			//compute Pstat
			computation.m_Pstat.clear();
			for (size_t i = 0; i < computation.m_XPstat.size(); i++)
				computation.m_Pstat += computation.m_XPstat[i][RANGE] / computation.m_XPstat[i][MEAN];


			if (m_bShowTrace)
				WriteInfoEx(parameters, computation, callback);


			//copy XPStat into XStat
			computation.m_Xstat = computation.m_XPstat;

			//clean XPstat stats
			for (size_t i = 0; i < computation.m_XPstat.size(); i++)
				computation.m_XPstat[i].Reset();

			//clean VM stats
			for (size_t i = 0; i < computation.m_VMstat.size(); i++)
				computation.m_VMstat[i].Reset();

			//  Loop again.
			bQuit = fabs(computation.m_F - computation.m_Fopt) <= m_SA_ctrl.m_EPS;//|| computation.m_Pstat[HIGHEST] < m_ctrl.m_P_EPS
			for (int I = 0; I < computation.m_FSTAR.size() && bQuit; I++)
			{
				if (fabs(computation.m_F - computation.m_FSTAR[I]) > m_SA_ctrl.m_EPS)//||  && computation.m_Pstat[HIGHEST] > m_ctrl.m_P_EPS
					bQuit = false;
			}





			//  If termination criteria is not met, prepare for another loop.
			computation.PrepareForAnotherLoop(m_SA_ctrl.m_RT, m_SA_ctrl.m_RT2, L);

		} while (!bQuit && msg);


		return msg;
	}




	bool CInsectParameterization::IsParamValid(const std::string& var, size_t e, const std::vector<double>& P)
	{
		bool bValid = true;

		if (m_fitType == F_DEV_TIME)
		{
			bValid = CDevRateEquation::IsParamValid(CDevRateEquation::eq(e), P);
		}
		else if (m_fitType == F_SURVIVAL)
		{
			bValid = CSurvivalEquation::IsParamValid(CSurvivalEquation::eq(e), P);
		}
		else if (m_fitType == F_FECUNDITY)
		{
			bValid = CFecundityEquation::IsParamValid(CFecundityEquation::eq(e), P);
		}


		return bValid;
	}

	bool CInsectParameterization::IsRateValid(const std::string& var, size_t e, const std::vector<double>& P, double null_rate_threshold)
	{
		bool bValid = true;



		if (m_fitType == F_DEV_TIME)
		{
			CDevRateEquation::TDevRateEquation eq = CDevRateEquation::eq(e);

			double Tobs_min = m_devTime.at(var).m_stats_Tobs[LOWEST];
			double Tobs_max = m_devTime.at(var).m_stats_Tobs[HIGHEST];

			double min_T = min(m_Tlo[0], Tobs_min);
			double max_T = max(m_Thi[1], Tobs_max);
			bool bExcludeThi = boost::iequals(var, m_adult_name);

			CStatistic stat_sim_Tmin;
			CStatistic stat_sim_Tmax;
			CStatistic stat_sim_Tobs;
			CStatistic stat_sim;
			for (double T = min_T; T <= max_T; T += 0.25)
			{
				double rate = CDevRateEquation::GetRate(eq, P, T);//daily rate

				//avoid NAN in range of temperature
				if (isnan(rate) || isinf(rate) || rate > 1e100)
					return false;

				stat_sim += max(0.0, rate);

				if (T >= m_Tlo[0] && T <= m_Tlo[1])
					stat_sim_Tmin += rate;

				if (T >= m_Thi[0] && T <= m_Thi[1])
					stat_sim_Tmax += rate;

				if (T >= Tobs_min && T <= Tobs_max)
					stat_sim_Tobs += rate;
			}

			if (m_bConstrainTlo)
				bValid &= stat_sim_Tmin[LOWEST] <= null_rate_threshold && stat_sim_Tmin[HIGHEST] > null_rate_threshold;

			if (m_bConstrainThi && !bExcludeThi)
				bValid &= stat_sim_Tmax[LOWEST] <= null_rate_threshold && stat_sim_Tmax[HIGHEST] > null_rate_threshold;

			if (m_bLimitMaxRate)
			{
				double K = m_LimitMaxRateP;
				double max_rate_obs = m_devTime[var].m_stats_Rate[HIGHEST];
				double max_rate_sim = stat_sim[HIGHEST];

				double k = max_rate_sim / max_rate_obs;
				bValid &= k <= K;
			}


			if (m_bAvoidNullRateInTobs)
			{
				double min_rate_sim = stat_sim_Tobs[LOWEST];
				bValid &= min_rate_sim > null_rate_threshold;
			}
		}
		else if (m_fitType == F_SURVIVAL)
		{
			CSurvivalEquation::TSurvivalEquation eq = CSurvivalEquation::eq(e);

			double Tobs_min = m_survival.at(var).m_stats_Tobs[LOWEST];
			double Tobs_max = m_survival.at(var).m_stats_Tobs[HIGHEST];

			for (double T = Tobs_min; T <= Tobs_max; T += 0.25)
			{
				double rate = max(0.0, CSurvivalEquation::GetSurvival(eq, P, T));//daily rate
				if (isnan(rate) || isinf(rate) || rate > 1e100)
					return false;
			}

		}
		else if (m_fitType == F_FECUNDITY)
		{
			CFecundityEquation::TDevRateEquation eq = CFecundityEquation::eq(e);

			double Tobs_min = m_fecundity.at(var).m_stats_Tobs[LOWEST];
			double Tobs_max = m_fecundity.at(var).m_stats_Tobs[HIGHEST];

			for (double T = Tobs_min; T <= Tobs_max; T += 0.25)
			{
				double rate = max(0.0, CFecundityEquation::GetRate(eq, P, T));//daily rate
				if (isnan(rate) || isinf(rate) || rate > 1e100)
					return false;
			}

		}


		return bValid;
	}

	/*double rate(double, double)
	{
	}

	void test(void)
	{
		double t;
		double dt;
		double X;
		double T;
		double sigma;

		namespace boost::math
		{

			//Fitted

			//Create log-normal distribution
			lognormal_distribution<double> LogNormal(-0.5 * Square(sigma), sigma);

			//X: parameters
			//T: temperature
			//t: observed time
			//dt: time between observation
			double time = 1.0 / rate(X, T);//compute time from simulated rate
			double LL = cdf(LogNormal, t/time ) - cdf(LogNormal, (t - dt)/time);//changing stage


			//X: parameters
			//T: temperature
			//t: observed time
			double time = 1.0 / rate(X, T);
			double LL = 1 - cdf(LogNormal, t/ time);//censored


			//X: parameters
			//T: temperature
			double time = 1.0 / rate(X, T);
			double LL = (time>t) ? 1 - cdf(LogNormal, t / time) : 0.0;//censored



			//X: parameters
			//T: temperature
			//t: observed time
			double time = 1.0 / rate(X, T);
			double LL = 1 - cdf(LogNormal, Square(1 - t/time) );//censored


			//X: parameters
			//T: temperature
			//t: observed time
			double mu = -0.5 * Square(sigma);
			double Xmode = exp(mu - Square(sigma));
			double time = 1.0 / rate(X, T);
			int i=0;


			if (i % 2 != 0)
				LL = 1 - cdf(LogNormal, t / time);//censored
			else
				LL = 1 - cdf(LogNormal, Square(Xmode - t / time));//censored

			i++;

		}
	}*/
	const int NUM_STAGES = 5;

	// Function to calculate Weighted Kappa
	double computeWeightedKappa(const vector<vector<double>>& matrix)
	{
		double total_N = 0;
		vector<double> row_sums(NUM_STAGES, 0.0);
		vector<double> col_sums(NUM_STAGES, 0.0);

		// 1. Calculate row, column, and total sums
		for (int i = 0; i < NUM_STAGES; ++i)
		{
			for (int j = 0; j < NUM_STAGES; ++j)
			{
				row_sums[i] += matrix[i][j];
				col_sums[j] += matrix[i][j];
				total_N += matrix[i][j];
			}
		}

		// 2. Build Observed vs Expected Probability Matrices
		vector<vector<double>> p_o(NUM_STAGES, vector<double>(NUM_STAGES, 0.0));
		vector<vector<double>> p_e(NUM_STAGES, vector<double>(NUM_STAGES, 0.0));

		double sum_p_o = 0.0;
		double sum_p_e = 0.0;

		for (int i = 0; i < NUM_STAGES; ++i)
		{
			for (int j = 0; j < NUM_STAGES; ++j)
			{
				p_o[i][j] = matrix[i][j] / total_N;
				p_e[i][j] = (row_sums[i] / total_N) * (col_sums[j] / total_N);

				// 3. Define Weights (Quadratic Weighting in this example)
				double weight = 1.0 - pow((i - j) / (double)(NUM_STAGES - 1), 2.0);

				sum_p_o += weight * p_o[i][j];
				sum_p_e += weight * p_e[i][j];
			}
		}

		// 4. Calculate Final Weighted Kappa
		if (1.0 - sum_p_e == 0)
		{
			return 0.0; // Avoid division by zero
		}

		return (sum_p_o - sum_p_e) / (1.0 - sum_p_e);
	}


	vector<vector<double>> Get_confusion_matrix(const vector<size_t>& observed, const vector<size_t>& simulated)
	{
		assert(observed.size() == simulated.size());

		// 1. Initialize 5x5 confusion matrix with zeros
		vector<vector<double>> confusion_matrix(NUM_STAGES, vector<double>(NUM_STAGES, 0.0));
		//double total_samples = std::accumulate(observed.begin(), observed.end(), 0.0);
		//assert(std::accumulate(simulated.begin(), simulated.end(), 0.0) == total_samples);

		// 2. Populate the confusion matrix
		for (size_t i = 0; i < observed.size(); ++i)
		{
			size_t i_obs = observed[i];
			size_t i_sim = simulated[i];
			confusion_matrix[i_obs][i_sim]++;
		}

		return confusion_matrix;
	}

	bool CInsectParameterization::GetFValue(string var, size_t e, CComputationVariable& computation)
	{
		assert(IsParamValid(var, e, computation.m_XP));
		assert(IsRateValid(var, e, computation.m_XP, NULL_RATE_THRESHOLD));

		bool bLogLikelyhoude = m_SA_ctrl.m_statisticType == LIKELIHOOD;
		double log_likelyhoude = 0;
		size_t N_likelyhoude = 0;
		CStatisticXYEx stat;

		if (m_fitType == F_DEV_TIME)
		{
			TDevRateEquation eq = CDevRateEquation::eq(e);

			double sigma = computation.m_XP.back();
			//compute probability of changing stage between ti-1 and ti

			boost::math::lognormal_distribution<double> LogNormal(-0.5 * Square(sigma), sigma);

			double mu = -0.5 * Square(sigma);
			double Xmode = exp(mu - Square(sigma));

			const CDevRateStages& S = m_devTime.at(var);

			//for all treatment
			for (const auto& itT : S)
			{
				const CDevRateTreatments& T = itT.second;

				TInputTemporal i_temporal = T.i_temporal() == IT_MEAN ? IT_MEAN : IT_INDIVIDUAL;
				CxiVector t_xi = T.get_t_xi(i_temporal, m_bUseDead, m_bUseDead, false);//m_bUseCencored, m_bUseCasualties


				if (i_temporal == IT_INDIVIDUAL)//use individual time
				{
					if (!Regniere2021DevRate(eq, computation.m_XP, T.Tobs(), t_xi))
						return false;

					size_t i = 0;
					for (const auto& xi : t_xi)
					{
						if (bLogLikelyhoude)
						{

							//When Time is infinite (INF_TIME) and rate is zero, after, L'Hopital low, the result is zero. log(1) = 0
							bool bLHopitalLaw = (xi.first[0] >= INF_TIME && xi.first[1] <= 0) && (xi.second[0] >= INF_TIME && xi.second[1] <= 0);

							double p = 1;
							if (!bLHopitalLaw)
							{
								if (xi.stage_end_status == SE_ALIVE || xi.adult)
								{
									p = cdf(LogNormal, xi.second[1]) - cdf(LogNormal, xi.first[1]);//changing stage
								}
								else
								{
									if (i % 2 != 0)//xi.censored || xi.casualties 
										p = 1 - cdf(LogNormal, xi.first[1]);//censored data (or death)
									else
										p = 1 - cdf(LogNormal, Square(1 - (xi.first[1] + xi.second[1]) / 2));//censored data (or death)
									//p = 1 - cdf(LogNormal, Square(Xmode - (xi.first[1] + xi.second[1]) / 2));//censored data (or death)


									i++;
								}
							}

							double LL = log(max(1e-20, p)) * xi.n;//max: avoid log of 0

							log_likelyhoude += LL;
							N_likelyhoude += xi.n;
						}
						else
						{
							if (xi.second[0] > 0)//Use only non NA
							{

								//Using the rate instead of the time give better result at daily observation
								//static const bool BY_TIME = true;
								//if (BY_TIME)
								//{
								double mean_time = xi.second[0] / xi.second[1];
								double RDT = quantile(LogNormal, xi.pTime);
								double obs = xi.second[0];
								double sim = min(1000.0, RDT * mean_time);
								assert(isfinite(sim) && !isnan(sim) && sim > -1E8 && sim < 1E8);

								for (size_t n = 0; n < xi.n; n++)
									stat.Add(sim, obs);
								//}
								//else
								//{
								//	double mean_rate = xi.second[1] / xi.second[0];
								//	double RDR = quantile(LogNormal, 1.0 - xi.pRate);
								//	//double RDR = quantile(LogNormal, xi.pTime);
								//	double obs = 1.0 / xi.second[0];
								//	double sim = (RDR * mean_rate);
								//	assert(isfinite(sim) && !isnan(sim) && sim > -1E8 && sim < 1E8);

								//	for (size_t n = 0; n < xi.n; n++)
								//		stat.Add(sim, obs);
								//}
							}
						}
					}
				}
				else//use mean+SD+n
				{
					Regniere2021DevRateMeanSDn(eq, computation.m_XP, T.Tobs(), t_xi);


					//for (auto O : T)
					for (const auto& xi : t_xi)
					{
						if (xi.second[0] > 0)//Use only non NA
						{
							if (bLogLikelyhoude)
							{
								double obs_time = xi.second[0];
								double sim_time = xi.second[1];
								double time_SD = xi.TimeSD;
								double n = xi.n;

								//compute probability 
								boost::math::normal_distribution<double> Normal(0, time_SD / sqrt(n));//In c++, Normals need SD
								double p = pdf(Normal, obs_time - sim_time);
								double LL = log(max(DBL_MIN, p));//LL seem to not have to be multiply by N!

								log_likelyhoude += LL;
								N_likelyhoude += n;//validate with Jacques
							}
							else
							{
								//double obs = O.at(I_MEAN_TIME);
								//double sim = 1.0 / max(0.001, GetDevRate(eq, computation.m_XP, T.Tobs(), O));
								double obs_time = xi.second[0];
								double sim_time = xi.second[1];
								assert(isfinite(sim_time) && !isnan(sim_time) && sim_time > -1E8 && sim_time < 1E8);

								for (size_t n = 0; n < xi.n; n++)
									stat.Add(sim_time, obs_time);
							}
						}
					}
				}
			}
		}
		else if (m_fitType == F_SURVIVAL)
		{
			TSurvivalEquation eq = CSurvivalEquation::eq(e);


			//	{
			const CDevRateStages& S = m_survival.at(var);
			for (auto& itT : S)
			{
				string treatment = itT.first;
				const auto& T = itT.second;

				for (auto& O : T)
				{
					if (bLogLikelyhoude)
					{
						//maximum likelihood
						double n = 1;// m_survival[i][I_N];
						double LL = Regniere2021Survival(eq, computation.m_XP, T.Tobs(), O);
						assert(isfinite(LL) && !isnan(LL));


						log_likelyhoude += n * LL;
						N_likelyhoude += n;
					}
					else
					{//RSS
						//stage survival
						if (O.at(I_MEAN_TIME) > 0)
						{
							double obs = O.at(I_SURVIVAL) / O.at(I_N);
							double sim = GetSurvival(eq, computation.m_XP, T.Tobs(), O);

							assert(isfinite(sim) && !isnan(sim) && sim > -1E8 && sim < 1E8);


							for (size_t n = 0; n < O.at(I_N); n++)
								stat.Add(sim, obs);
						}
					}
				}//for all observation
			}//for all treatment
			//}
		}
		else if (m_fitType == F_FECUNDITY)
		{
			TFecundityEquation eq = CFecundityEquation::eq(e);

			const CDevRateStages& S = m_fecundity.at(var);
			for (auto& itT : S)
			{
				string treatment = itT.first;
				const auto& T = itT.second;

				if (T.i_temporal() == IT_TIME_SERIES || T.i_temporal() == IT_INDIVIDUAL)//use individual time
				{
					CxiVector f_xi = T.get_f_xi(T.i_temporal());
					if (!Regniere2021FecundityTimeSeries(eq, computation.m_XP, T.Tobs(), f_xi))
						return false;


					//Get Fo and sigma from input parameters X
					double Fo = computation.m_XP[computation.m_XP.size() - 2];
					double sigma_f = computation.m_XP[computation.m_XP.size() - 1];

					//create relative fecundity unbiased log-normal distribution
					boost::math::lognormal_distribution<double> LogNormal(log(Fo) - 0.5 * Square(sigma_f), sigma_f);


					size_t i = 0;
					for (const auto& xi : f_xi)
					{
						if (bLogLikelyhoude)//use likelihood method
						{
							//compute individual fecundity (Fi) from Fo and female position
							double Fi = quantile(LogNormal, xi.pFecundity);


							//double Ft = Fi;//remaining fecundity at day t
							//double Ftˉ¹ = Fi;//remaining fecundity at day t-1

							//create Poisson distribution from expected values
							double expected = max(1e-5, Fi * (xi.first[1] - xi.second[1]));

							boost::math::poisson_distribution<double> Poisson(expected);

							//compute probability to get observed brood
							double p = max(1e-20, boost::math::pdf(Poisson, xi.broods));

							//boost::math::poisson_distribution<double> Poisson(max(0.0001, brood_obs));
							//double expected = Ftˉ¹ - Ft;
							//double p = max(1e-20, boost::math::pdf(Poisson, expected));
							double LL = log(p) * xi.n;

							log_likelyhoude += LL;
							N_likelyhoude += xi.n;

						}
						//else if (O.m_bIndividual)//use individual time
						//{


						//	double expected = max(0.001, Ft);
						//	boost::math::poisson_distribution<double> Poisson(expected);

						//	////compute probability to get observed brood
						//	double p = max(1e-20, pdf(Poisson, brood_obs));

						//	//		boost::math::lognormal_distribution<double> LogNormal(log(Fo) - 0.5*Square(sigma_f), sigma_f);
						//	//	double p2 = max(1e-200, pdf(LogNormal, Ft));

						//	return log(p);// + log(p2)


						//	LL = Regniere2021Fecundity(eq, computation.m_XP, T.Tobs(), O.at(I_START) + O.at(I_TIME), O.at(I_BROODS), O.at(I_P_BROOD));
						//	LL *= O.at(I_N);

						//	n = O.at(I_N);
						//}

					}//for all xi
				}//if time series or individual
				else//use mean+sd+n
				{
					for (auto& O : T)
					{
						if (bLogLikelyhoude)
						{

							//LL seem to not have to be multiply by N!
							double LL = Regniere2021FecundityMeanSDn(eq, computation.m_XP, T.Tobs(), O.at(I_MEAN_TIME), O.at(I_MEAN_BROOD), O.at(I_BROOD_SD), O.at(I_N));
							double n = O.at(I_N);


							if (!isfinite(LL) || isnan(LL))
								return false;

							log_likelyhoude += LL;
							N_likelyhoude += n;
						}
						else
						{
							assert(false);//not implemented
						}
					}
				}

			}//for all treatment
		}//If devRate, survival, fecundity
		else if (m_fitType == F_STAGES_TABLE)
		{
			//bLogLikelyhoude = false;

			//TDevRateEquation eq = CDevRateEquation::eq(e);


			//double sigma = computation.m_XP.back();

			//std::uint32_t seed = static_cast<std::uint32_t>(std::time(0));
			//boost::random::mt19937 generator{ seed };

			//boost::random::uniform_real_distribution<double> runif(0.01, 0.99);
			//boost::math::lognormal_distribution<double> LogNormal(-0.5 * Square(sigma), sigma);



			//if (m_I.empty())
			//{
			//	//create the simulator
			//	size_t NUM_INSECTS = 100;
			//	size_t N = m_stages_table.num_treatments() * NUM_INSECTS;
			//	m_I.reserve(N);


			//	for (size_t t = 0; t < m_stages_table.num_treatments(); t++)//for all experimentation
			//	{
			//		string treatment = m_stages_table.get_treatment(t);
			//		double temperature = stof(treatment);

			//		for (size_t i = 0; i < NUM_INSECTS; i++)//for all experimentation
			//		{
			//			COneSimulatedInsect I;
			//			I.resize(m_stages_table.num_stages());
			//			for (size_t s = 0; s < I.size(); s++)//for all stages
			//			{
			//				I[s].m_p = runif(generator);
			//				I[s].m_RDR = quantile(LogNormal, I[s].m_p);
			//				I[s].m_treatment = treatment;
			//				I[s].m_temperature = temperature;
			//			}


			//			m_I.push_back(I);
			//		}
			//	}

			//	m_I.run();

			//	double Rate = CDevRateEquation::GetRate(eq, computation.m_XP, temperature);
			//	I[i][ii].m_time = min(1000.0, 1.0 / (I[i][ii].m_RDR * Rate));
			//	for (auto& itN : m_stages_table)//for all experimentation
			//	{
			//		const auto& N = itN.second;
			//		for (auto itT = N.begin(); itT != N.end(); itT++)//For all treatments
			//		{
			//			string treatment = itT->first;
			//			double temperature = stof(treatment);

			//			//size_t i = std::distance(N.begin(), itT);
			//			//I.resize(100);//for all individuals
			//			//for (size_t i = 0; i < I.size(); i++)//for all individuals
			//			{
			//				COneSimulatedInsect i;
			//				I[i].resize(m_stages_table.num_stages());//for all stages

			//				for (size_t ii = 0; i < I[i].size(); ii++)//for all stages
			//				{
			//					I[i][ii].m_p = runif(generator);
			//					I[i][ii].m_RDR = quantile(LogNormal, I[i][ii].m_p);

			//					double Rate = CDevRateEquation::GetRate(eq, computation.m_XP, temperature);
			//					I[i][ii].m_time = min(1000.0, 1.0 / (I[i][ii].m_RDR * Rate));
			//				}

			//			}
			//		}
			//	}
			//}
			////Compute stage table from individuals
			//for (size_t i = 0; i < I.size(); i++)//for all probability
			//{
			//}

			//const std::vector<CStagesTableRow>& data = itT->second;
			////	CxiVector f_xi = data.get_t_xi();

			//for (size_t d = 0; d < data.size(); d++)//for all dates
			//{
			//	const std::vector<double>& obs = data[d].m_n;
			//	double n = std::accumulate(obs.begin(), obs.end(), 0.0);

			//	//std::vector<double> sim;
			//	//double Rate = CDevRateEquation::GetRate(eq, computation.m_XP, temperature);

			//	double mean_time = xi.second[0] / xi.second[1];
			//	double RDT = quantile(LogNormal, xi.pTime);
			//	double obs = xi.second[0];
			//	double sim = min(1000.0, RDT * mean_time);
			//	assert(isfinite(sim) && !isnan(sim) && sim > -1E8 && sim < 1E8);

			//	for (size_t n = 0; n < xi.n; n++)
			//		stat.Add(sim, obs);

		//}























		//	//CxiVector f_xi = T.get_f_xi(T.i_temporal());
		//	////create relative fecundity unbiased log-normal distribution
		//	//boost::math::lognormal_distribution<double> LogNormal(log(Fo) - 0.5 * Square(sigma_f), sigma_f);


		//	//size_t i = 0;
		//	//for (const auto& xi : f_xi)
		//	//{
		//	//	if (bLogLikelyhoude)//use likelihood method
		//	//	{
		//	//		//compute individual fecundity (Fi) from Fo and female position
		//	//		double Fi = quantile(LogNormal, xi.pFecundity);


		//	//		//double Ft = Fi;//remaining fecundity at day t
		//	//		//double Ftˉ¹ = Fi;//remaining fecundity at day t-1

		//	//		//create Poisson distribution from expected values
		//	//		double expected = max(1e-5, Fi * (xi.first[1] - xi.second[1]));

		//	//		boost::math::poisson_distribution<double> Poisson(expected);

		//	//		//compute probability to get observed brood
		//	//		double p = max(1e-20, boost::math::pdf(Poisson, xi.broods));

		//	//		//boost::math::poisson_distribution<double> Poisson(max(0.0001, brood_obs));
		//	//		//double expected = Ftˉ¹ - Ft;
		//	//		//double p = max(1e-20, boost::math::pdf(Poisson, expected));
		//	//		double LL = log(p) * xi.n;

		//	//		log_likelyhoude += LL;
		//	//		N_likelyhoude += xi.n;

		//	//	}
		//	//}

		//}//for all treatment
	//}
		}//If devRate, survival, fecundity

		if (bLogLikelyhoude)
		{
			//add penalty if over maximum

			assert(N_likelyhoude > 0);


			double k = computation.m_XP.size();
			double n = N_likelyhoude;
			double AIC = 2 * k - 2 * (log_likelyhoude);
			double AICc = AIC + (2 * k * (k + 1) / max(1.0, n - k - 1));// n/k < 40

			computation.m_k_P = k;
			computation.m_n_P = n;
			computation.m_AICCP = AICc;
			computation.m_MLLP = log_likelyhoude;
			computation.m_FP = m_SA_ctrl.AdjustFValue(log_likelyhoude);

		}
		else
		{
			if (stat[NB_VALUE] > 1)
			{
				computation.m_FP = m_SA_ctrl.AdjustFValue(stat[m_SA_ctrl.m_statisticType]);
				computation.m_SP = stat;
			}
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

	CSAControl CInsectParameterization::GetSAOptions(size_t SA_preset, size_t optim_method)const
	{
		ASSERT(SA_preset < NB_SA_OPTION);

		CSAControl options;
		if (SA_preset == SA_CUSTOM)
			options = m_SAOptions;
		else
			options = GetDefaultSAOptions((TSAOption)SA_preset);


		switch (optim_method)
		{
		case OM_MLL:
		{
			options.m_bMax = true;
			options.m_statisticType = LIKELIHOOD;

			break;
		}
		case OM_RSS:
		{
			options.m_bMax = false;
			options.m_statisticType = RSS;
			break;
		}
		default:assert(false);
		}

		return options;
	}

	CSAControl CInsectParameterization::GetDefaultSAOptions(TSAOption option)
	{
		CSAControl options;
		switch (option)
		{
		case SA_LOW_FAST:
		{
			options.m_MAXEVL = 200'000;
			options.m_NS = 20;
			options.m_NT = 25;
			options.m_T = 10;
			options.m_RT = 0.55;
			options.m_EPS = 0.005;

			break;
		}
		case SA_MEDIUM_MEDIUM:
		{
			options.m_MAXEVL = 400'000;
			options.m_NS = 30;
			options.m_NT = 30;
			options.m_T = 100;
			options.m_RT = 0.65;
			options.m_EPS = 0.001;

			break;
		}
		case SA_HI_SLOW:
		{
			options.m_MAXEVL = 800'000;
			options.m_NS = 40;
			options.m_NT = 35;
			options.m_T = 175;
			options.m_RT = 0.75;
			options.m_EPS = 0.001;

			break;
		}
		case SA_VERY_HI_SLOW:
		{
			options.m_MAXEVL = 1'200'000;
			options.m_NS = 50;
			options.m_NT = 50;
			options.m_T = 250;
			options.m_RT = 0.85;
			options.m_EPS = 0.0005;

			break;
		}
		case SA_CUSTOM: break;
		default: assert(false);
		}


		return options;
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
		out[GetMemberName(STAGE_TABLE_EQUATIONS)](m_eqStageTable);
		out[GetMemberName(EQ_OPTIONS)](m_eq_options);
		out[GetMemberName(INPUT_FILE_NAME)](m_inputFileName);
		out[GetMemberName(OUTPUT_FILE_NAME)](m_outputFileName);
		out[GetMemberName(TOBS_FILE_NAME)](m_TobsFileName);
		out[GetMemberName(CONTROL)](m_SAOptions);
		out[GetMemberName(SA_PRESET)](m_SA_preset);
		out[GetMemberName(OPTIM_METHOD)](m_optim_method);


		//out[GetMemberName(FIXE_TB)](m_bFixeTb);
		//out[GetMemberName(TB_VALUE)](ToString2(m_Tb));
		//out[GetMemberName(FIXE_TM)](m_bFixeTm);
		//out[GetMemberName(TM_VALUE)](ToString2(m_Tm));

		out[GetMemberName(COUNSTRAIN_T_LO)](m_bConstrainTlo);
		out[GetMemberName(T_LO_VALUES)](ToString2(m_Tlo));
		out[GetMemberName(COUNSTRAIN_T_HI)](m_bConstrainThi);
		out[GetMemberName(T_HI_VALUES)](ToString2(m_Thi));
		out[GetMemberName(ADULT_NAME)](m_adult_name);


		out[GetMemberName(FIXE_F0)](m_bFixeF0);
		out[GetMemberName(F0_VALUE)](ToString(m_F0));
		out[GetMemberName(FIXE_T0)](m_bFixeT0);
		out[GetMemberName(T0_VALUE)](ToString(m_T0));
		out[GetMemberName(LIMIT_MAX_RATE)](m_bLimitMaxRate);
		out[GetMemberName(LIMIT_MAX_RATE_P)](ToString(m_LimitMaxRateP));
		out[GetMemberName(AVOID_NULL_RATE_IN_TOBS)](m_bAvoidNullRateInTobs);
		out[GetMemberName(USE_OUTPUT_AS_INPUT)](m_bUseOutputAsInput);
		out[GetMemberName(OUTPUT_AS_INTPUT_FILENAME)](m_outputAsIntputFileName);
		out[GetMemberName(SHOW_TRACE)](m_bShowTrace);
		out[GetMemberName(USE_DEAD)](m_bUseDead);
		out[GetMemberName(USE_CENCORED)](m_bUseCencored);
		out[GetMemberName(USE_CASUALTIES)](m_bUseCasualties);



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
		in[GetMemberName(STAGE_TABLE_EQUATIONS)](m_eqStageTable);
		in[GetMemberName(EQ_OPTIONS)](m_eq_options);
		in[GetMemberName(INPUT_FILE_NAME)](m_inputFileName);
		in[GetMemberName(TOBS_FILE_NAME)](m_TobsFileName);
		in[GetMemberName(OUTPUT_FILE_NAME)](m_outputFileName);
		in[GetMemberName(CONTROL)](m_SAOptions);
		in[GetMemberName(SA_PRESET)](m_SA_preset);
		in[GetMemberName(OPTIM_METHOD)](m_optim_method);


		in[GetMemberName(COUNSTRAIN_T_LO)](m_bConstrainTlo);
		in[GetMemberName(T_LO_VALUES)](tmp); m_Tlo = ToArray<double, 2>(tmp);
		in[GetMemberName(COUNSTRAIN_T_HI)](m_bConstrainThi);
		in[GetMemberName(T_HI_VALUES)](tmp); m_Thi = ToArray<double, 2>(tmp);
		in[GetMemberName(ADULT_NAME)](m_adult_name);


		in[GetMemberName(FIXE_F0)](m_bFixeF0);
		in[GetMemberName(F0_VALUE)](m_F0);
		in[GetMemberName(FIXE_T0)](m_bFixeT0);
		in[GetMemberName(T0_VALUE)](m_T0);
		in[GetMemberName(LIMIT_MAX_RATE)](m_bLimitMaxRate);
		in[GetMemberName(LIMIT_MAX_RATE_P)](m_LimitMaxRateP);
		in[GetMemberName(AVOID_NULL_RATE_IN_TOBS)](m_bAvoidNullRateInTobs);
		in[GetMemberName(USE_OUTPUT_AS_INPUT)](m_bUseOutputAsInput);
		in[GetMemberName(OUTPUT_AS_INTPUT_FILENAME)](m_outputAsIntputFileName);
		in[GetMemberName(SHOW_TRACE)](m_bShowTrace);
		in[GetMemberName(USE_DEAD)](m_bUseDead);
		in[GetMemberName(USE_CENCORED)](m_bUseCencored);
		in[GetMemberName(USE_CASUALTIES)](m_bUseCasualties);





		return true;
	}


	std::array<std::string, CCStagesTableData::NB_COLUMNS > CCStagesTableData::INPUT_NAME = { "No", "Treatment", "Date", "Stage_" };
	CCStagesTableData::TColumns CCStagesTableData::get_input(const std::string& name)
	{
		TColumns pos = (TColumns)NOT_INIT;

		auto it = find_if(INPUT_NAME.begin(), INPUT_NAME.end(), [&](auto& s) {return boost::iequals(s, name); });
		if (it == INPUT_NAME.end() && boost::iequals(name, "DOY"))
			it = INPUT_NAME.begin() + C_DATE;

		if (it != INPUT_NAME.end())
		{
			pos = (TColumns)std::distance(INPUT_NAME.begin(), it);
		}
		else
		{
			if (boost::istarts_with(name, INPUT_NAME[C_STAGE]))
			{
				pos = C_STAGE;
			}
		}

		return pos;
	}

	CCStagesTableData::CCStagesTableData()
	{
	}

	CCStagesTableData::~CCStagesTableData()
	{
	}

	void CCStagesTableData::clear()
	{
		CCStagesTableDataBase::clear();
		m_treatments.clear();
		m_stage_columns.clear();
		m_input_pos.clear();
		m_treatments.clear();
	}


	ERMsg CCStagesTableData::load(const std::string& file_path)
	{
		ERMsg msg;

		//begin to read
		ifStream file;
		std::locale utf8_locale("en_US.UTF-8");
		file.imbue(utf8_locale);
		msg = file.open(file_path);
		if (msg)
		{
			msg = load(file);
			file.close();
		}

		return msg;

	}

	ERMsg CCStagesTableData::load(std::istream& io)
	{
		ERMsg msg;

		//All other column beginning with "S_" or "Stage_" is considerate as stage


		clear();

		CSVIterator loop(io, ",;\t", true, true);
		m_input_pos.resize(loop.Header().size());

		for (size_t i = 0; i < loop.Header().size(); i++)
		{
			string col_header = loop.Header()[i];
			m_input_pos[i] = get_input(col_header);
			if (m_input_pos[i] == C_STAGE)
			{
				m_stage_columns.push_back({ col_header.substr(6),i });
			}
		}


		if (!have_column(C_TREATMENT))
			msg.ajoute("Mandatory missing column for stages table . \"Treatment\" (\"T\") must be define");

		if (!have_column(C_DATE))
			msg.ajoute("Mandatory missing column for stages table : \"Date\" or \"DOY\" must be define");

		if (!msg)
			return msg;



		size_t posN = get_pos(C_EXP_NO);
		size_t posT = get_pos(C_TREATMENT);
		size_t posD = get_pos(C_DATE);

		assert(posT != NOT_INIT);
		assert(posD != NOT_INIT);

		bool bDOY = boost::iequals("DOY", loop.Header()[posD]);

		//read all individuals time series
		for (; loop != CSVIterator() && msg; ++loop)
		{
			if (msg && !loop->empty())
			{
				if (loop->size() != loop.Header().size())
				{
					msg.ajoute("Bad number of column for line:" + loop->GetLastLine());
					return msg;
				}

				string strN = posN != NOT_INIT ? (*loop)[posN] : "1";
				string strT = (*loop)[posT];
				string strD = (*loop)[posD];

				CTRef TRef;
				if (bDOY)
				{
					double DOY = stof(strD);
					TRef = CTRef(0, 0, 0, 0, CTM::HOURLY) + (int)Round(DOY * 24);
				}
				else
				{
					TRef.FromFormatedString(strD, "", "- :");
				}


				if (!TRef.IsValid())
				{
					msg.ajoute("Invalid date format at line:" + loop->GetLastLine());
					return msg;
				}

				CStagesTableRow row;
				row.m_TRef = TRef;

				row.m_n.resize(m_stage_columns.size());
				for (size_t i = 0; i < m_stage_columns.size(); i++)
				{
					size_t col = m_stage_columns[i].second;
					string s_n = (*loop)[col];
					row.m_n[i] = stof(s_n);
				}



				CCStagesTableData& me = *this;
				me[strN][strT].push_back(row);
				m_treatments.insert(strT);

			}
		}


		return msg;
	}


}

