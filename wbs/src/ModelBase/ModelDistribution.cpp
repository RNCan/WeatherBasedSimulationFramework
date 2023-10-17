//**************************************************************************************************************
// 10/11/2022	1.0.0	Rémi Saint-Amant	Creation
//**************************************************************************************************************

#include "stdafx.h"
#include "ModelBase/ModelDistribution.h"
#include "Basic/WeatherStation.h"
#include "Basic/DegreeDays.h"
#include "Basic/UtilMath.h"
#include <crtdbg.h>

using namespace std;
using namespace WBSF::HOURLY_DATA;


namespace WBSF
{

	//CModelDistribution::CModelDistribution(TType type, double p1, double p2)
	//{
	//	m_type = type;
	//	m_p1 = p1;
	//	m_p2 = p2;

	//	switch (m_type)
	//	{
	//	case NORMALS:      p_normal_distribution.reset(new boost::math::normal_distribution<double>(p1, p2)); break;
	//	case LOG_NORMAL:   p_lognormal_distribution.reset(new boost::math::lognormal_distribution<double>(p1, p2)); break;
	//	case LOGISTIC:	   p_logistic_distribution.reset(new boost::math::logistic_distribution<double>(p1, p2)); break;
	//	case WEIBULL:	   p_weibull_distribution.reset(new boost::math::weibull_distribution<double>(p1, p2)); break;
	//	case GAMMA:		   p_fisher_f_distribution.reset(new boost::math::fisher_f_distribution<double>(p1, p2)); break;
	//	case FISHER:	   p_extreme_value_distribution.reset(new boost::math::extreme_value_distribution<double>(p1, p2)); break;
	//	case EXTREME_VALUE:p_gamma_distribution.reset(new boost::math::gamma_distribution<double>(p1, p2)); break;
	//	case RAYLEIGH:	   p_rayleigh_distribution.reset(new boost::math::rayleigh_distribution<double>(p2)); break;
	//	case MODIFIED_LOGISTIC:
	//	case GOMPERTZ1:
	//	case GOMPERTZ2:	break;
	//	default:assert(false);
	//	}

	//}

	//double CModelDistribution::get_cdf(double x)const
	//{

	//	double CDF = 0;
	//	switch (m_type)
	//	{
	//	case NORMALS:      CDF = cdf(*p_normal_distribution, x); break;
	//	case LOG_NORMAL:   CDF = cdf(*p_lognormal_distribution, x); break;
	//	case LOGISTIC:	   CDF = cdf(*p_logistic_distribution, x); break;
	//	case WEIBULL:	   CDF = cdf(*p_weibull_distribution, x); break;
	//	case GAMMA:		   CDF = cdf(*p_fisher_f_distribution, x); break;
	//	case FISHER:	   CDF = cdf(*p_extreme_value_distribution, x); break;
	//	case EXTREME_VALUE:CDF = cdf(*p_gamma_distribution, x); break;
	//	case RAYLEIGH:		CDF = cdf(*p_rayleigh_distribution, max(0.0, x - m_p1)); break;
	//	case MODIFIED_LOGISTIC:CDF = 1 / (1 + exp(-(x - m_p1) / sqrt(m_p2 * x))); break;
	//	case GOMPERTZ1:		CDF = 1 - exp(-m_p1 * (exp(m_p2 * x) - 1)); break;
	//	case GOMPERTZ2:		CDF = exp(-exp(m_p1 - m_p2 * x)); break;

	//		//case SHIFTED_GOMPERTZ: CDF = (1-exp(- m_p2 * x))*exp(-m_p1*exp(-m_p2 * x)) ; break;

	//	default:assert(false);
	//	}

	//	return CDF;
	//}


	//double CModelDistribution::get_pdf(double x)const
	//{

	//	double PDF = 0;
	//	switch (m_type)
	//	{
	//	case NORMALS:      PDF = pdf(*p_normal_distribution, x); break;
	//	case LOG_NORMAL:   PDF = pdf(*p_lognormal_distribution, x); break;
	//	case LOGISTIC:	   PDF = pdf(*p_logistic_distribution, x); break;
	//	case WEIBULL:	   PDF = pdf(*p_weibull_distribution, x); break;
	//	case GAMMA:		   PDF = pdf(*p_fisher_f_distribution, x); break;
	//	case FISHER:	   PDF = pdf(*p_extreme_value_distribution, x); break;
	//	case EXTREME_VALUE:PDF = pdf(*p_gamma_distribution, x); break;
	//	case MODIFIED_LOGISTIC: {double S = sqrt(x * m_p2); PDF = exp(-(x - m_p1) / S) / (S * Square(1 + exp(-(x - m_p1) / S))); } break;
	//	case GOMPERTZ1:		PDF = m_p1 * m_p2 * exp(m_p1 + m_p2 * x - m_p1 * exp(m_p2 * x)); break;
	//	case GOMPERTZ2:		PDF = 0; break;//Unknown

	//	default:assert(false);
	//	}

	//	return PDF;
	//}



	//double CModelDistribution::get_quantile(double x)const
	//{

	//	double v = 0;
	//	switch (m_type)
	//	{
	//	case NORMALS:      v = quantile(*p_normal_distribution, x); break;
	//	case LOG_NORMAL:   v = quantile(*p_lognormal_distribution, x); break;
	//	case LOGISTIC:	   v = quantile(*p_logistic_distribution, x); break;
	//	case WEIBULL:	   v = quantile(*p_weibull_distribution, x); break;
	//	case GAMMA:		   v = quantile(*p_fisher_f_distribution, x); break;
	//	case FISHER:	   v = quantile(*p_extreme_value_distribution, x); break;
	//	case EXTREME_VALUE:v = quantile(*p_gamma_distribution, x); break;
	//	case RAYLEIGH:		v = m_p1 + quantile(*p_rayleigh_distribution, x); break;
	//	case MODIFIED_LOGISTIC: v = m_p1 - log(1 / x - 1) * sqrt(m_p2 * x);  break;
	//	case GOMPERTZ1:		v = log(1.0 - log(x) / m_p1) / m_p2; break;
	//	case GOMPERTZ2:		v = -(log(-log(x)) - m_p1) / m_p2; break;
	//	default:assert(false);
	//	}

	//	return v;
	//}


	double CModelDistribution::get_cdf(double x, TType type, double p1, double p2, double p3)
	{
		double v = 0;

		

		switch (type)
		{
		case NORMALS:
		{
			boost::math::normal_distribution<double> distribution(p1, p2);
			v = cdf(distribution, x);
			break;
		}
		case LOG_NORMAL:
		{
			boost::math::lognormal_distribution<double> distribution(p1, p2);
			v = cdf(distribution, x);
			break;
		}
		case LOGISTIC:
		{
			boost::math::logistic_distribution<double> distribution(p1, p2);
			v = cdf(distribution, x);
			break;
		}
		case WEIBULL:
		{
			x = max(0.0, x - p3);
			boost::math::weibull_distribution<double> distribution(p1, p2);
			v = cdf(distribution, max(0.0, x));
			break;
		}
		case GAMMA:
		{
			boost::math::fisher_f_distribution<double> distribution(p1, p2);
			v = cdf(distribution, x);
			break;
		}
		case FISHER:
		{
			boost::math::extreme_value_distribution<double> distribution(p1, p2);
			v = cdf(distribution, x);
			break;
		}
		case EXTREME_VALUE:
		{
			boost::math::gamma_distribution<double> distribution(p1, p2);
			v = cdf(distribution, x);
			break;
		}
		case RAYLEIGH:
		{
			boost::math::rayleigh_distribution<double> distribution(p2);
			v = cdf(distribution, max(0.0, x - p1));
			break;
		}
		case MODIFIED_LOGISTIC:
		{
			if (x > 0)
			{
				boost::math::logistic_distribution<double> distribution(p1, sqrt(p2 * x));
				v = cdf(distribution, x);
			}
			break;
			//v = m_p1 - log(1 / x - 1) * sqrt(m_p2 * x);  break;
		}
		case GOMPERTZ1:
		{
			x = max(0.0, x - p3);
			v = 1 - exp(-p1 * (exp(p2 * max(0.0, x)) - 1));
			break;
		}
		case GOMPERTZ2:
		{
			x = max(0.0, x - p3);
			v = exp(-exp(p1 - p2 * x));
			break;
		}
		case FRECHET:
		{
			assert(p2 > 0.0 && p3 > 0.0);

			if (x <= p1)
				return 0.0;

			double z = (x - p1) / p2;
			return exp(-pow(z, -p3));
		}
		case LOG_LOGISTIC:
		{
			assert(p1 > 0.0 && p2 > 0.0);

			//exp(log(x) * p2 + p1) / (1 + exp(log(x) * p2 + p1));
			return 1 / (1 + pow(x / p1, -p2));
		}

	
		default:assert(false);
		}

		return v;
	}

	double CModelDistribution::get_quantile(double x, TType type, double p1, double p2, double p3)
	{

		double v = 0;
		switch (type)
		{
		case NORMALS:
		{
			boost::math::normal_distribution<double> distribution(p1, p2);
			v = quantile(distribution, x);
			break;
		}
		case LOG_NORMAL:
		{
			boost::math::lognormal_distribution<double> distribution(p1, p2);
			v = quantile(distribution, x);
			break;
		}
		case LOGISTIC:
		{
			boost::math::logistic_distribution<double> distribution(p1, p2);
			v = quantile(distribution, x);
			break;
		}
		case WEIBULL:
		{
			boost::math::weibull_distribution<double> distribution(p1, p2);
			v = p3 + quantile(distribution, max(0.0,x));
			break;
		}
		case GAMMA:
		{
			boost::math::fisher_f_distribution<double> distribution(p1, p2);
			v = quantile(distribution, x);
			break;
		}
		case FISHER:
		{
			boost::math::extreme_value_distribution<double> distribution(p1, p2);
			v = quantile(distribution, x);
			break;
		}
		case EXTREME_VALUE:
		{
			boost::math::gamma_distribution<double> distribution(p1, p2);
			v = quantile(distribution, x);
			break;
		}
		case RAYLEIGH:
		{
			boost::math::rayleigh_distribution<double> distribution(p2);
			v = p1 + quantile(distribution, x);
			break;
		}
		case MODIFIED_LOGISTIC:
		{
			if (x > 0)
			{
				boost::math::logistic_distribution<double> distribution(p1, sqrt(p2 * x));
				v = quantile(distribution, x);
			}
			break;
			//v = m_p1 - log(1 / x - 1) * sqrt(m_p2 * x);  break;
		}
		case GOMPERTZ1:
		{
			if (x > 0)
				v = p3 + log(1.0 - log(max(0.0, x)) / p1) / p2;
			break;
		}
		case GOMPERTZ2:
		{
			if (x > 0)
				v = p3  -(log(-log(x)) - p1) / p2;
			break;
		}
		case FRECHET:
		{
			v = p1 + p2 * pow(-log(x), -1.0 / p3);
		}
		case LOG_LOGISTIC:
		{
			assert(p1 > 0.0 && p2 > 0.0);
			return (p2 / p1) * pow(x / p1,p2 - 1.0) / Square(1.0 + pow(x / p1 , p2));
		}

		default:assert(false);
		}

		return v;
	}


	double CModelDistribution::get_cdf(double x, const std::array<double, NB_CDD_PARAMS>& P)
	{
		assert(size_t(P[CDD_DIST]) < NB_DISTRIBUTIONS);
		return get_cdf(x, TType(P[CDD_DIST]), P[CDD_P1], P[CDD_P2], P[CDD_P3]);
	}

	double CModelDistribution::get_quantile(double x, const std::array<double, NB_CDD_PARAMS>& P)
	{
		assert(size_t(P[CDD_DIST]) < NB_DISTRIBUTIONS);
		return get_quantile(x, TType(P[CDD_DIST]), P[CDD_P1], P[CDD_P2], P[CDD_P3]);
	}


	void CModelDistribution::get_CDD(const std::array<double, NB_CDD_PARAMS>& params, const CWeatherYear& weather, CModelStatVector& CDD)
	{
		CDegreeDays DDmodel(CDegreeDays::MODIFIED_ALLEN_WAVE, params[CDD_Τᴴ¹], params[CDD_Τᴴ²]);

		DDmodel.GetCDD(params[CDD_DELTA], weather, CDD);

		/*CModelStatVector DD_daily;
		DDmodel.Execute(weather, DD_daily);

		if (CDD.empty())
			CDD.Init(DD_daily.GetTPeriod(), 1, 0);

		CTPeriod p = weather.GetEntireTPeriod(CTM::DAILY);
		p.Begin() = p.Begin() + int(params[CDD_DELTA]);

		CDD[p.Begin()][0] = DD_daily[p.Begin()][CDegreeDays::S_DD];
		for (CTRef TRef = p.Begin() + 1; TRef <= p.End(); TRef++)
			CDD[TRef][0] = CDD[TRef - 1][0] + DD_daily[TRef][CDegreeDays::S_DD];*/


	}

	void CModelDistribution::get_CDD(const std::array<double, NB_CDD_PARAMS>& params, const CWeatherYears& weather, CModelStatVector& CDD)
	{
		CDD.Init(weather.GetEntireTPeriod(CTM::DAILY), 1, 0);

		for (size_t y = 0; y < weather.size(); y++)
			get_CDD(params, weather[y], CDD);
	}


	//Get chilling DD
	void CModelDistribution::get_ChillCDD(const std::array<double, NB_CDD_PARAMS>& P, const CWeatherYear& weather, CModelStatVector& CHCDD)
	{
		int year = weather.GetTRef().GetYear();
		CTPeriod pp(CTRef(year, JANUARY, DAY_01), CTRef(year, DECEMBER, DAY_31));

		if (CHCDD.empty())
			CHCDD.Init(pp, 1, 0);


		double CHDD = 0;
		for (CTRef TRef = pp.Begin(); TRef <= pp.End(); TRef++)
		{
			//if( P[CDD_DELTA]
			double Tair = weather.GetDay(TRef)[H_TNTX][MEAN];
			if (Tair > P[CDD_Τᴴ¹] && Tair < P[CDD_Τᴴ²])
				CHDD += -(Tair - P[CDD_Τᴴ²]);

			CHCDD[TRef][0] = CHDD;
		}

	}


	void CModelDistribution::get_CDF(const std::array<double, NB_CDD_PARAMS>& params, const CModelStatVector& CDD, CModelStatVector& CDF, size_t round)
	{
		CTPeriod p = CDD.GetTPeriod();

		if (CDF.empty())
			CDF.Init(p, 1, 0);

		for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
		{
			CDF[TRef][0] = CModelDistribution::get_cdf(CDD[TRef][0], params);
		}
	}

	void CModelDistribution::get_CDF(const std::array<double, NB_CDD_PARAMS>& params, const CWeatherYear& weather, CModelStatVector& CDF, size_t round)
	{
		CModelStatVector CDD;
		CModelDistribution::get_CDD(params, weather, CDD);
		CModelDistribution::get_CDF(params, CDD, CDF, round);
	}
}