//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once

#define _INTSAFE_H_INCLUDED_
#define NOMINMAX


#include <boost/math/distributions/weibull.hpp>
#include <boost/math/distributions/logistic.hpp>
#include <boost/math/distributions/exponential.hpp>
#include <boost/math/distributions/non_central_f.hpp>
#include <boost/math/distributions/extreme_value.hpp>
#include <boost/math/distributions/fisher_f.hpp>
#include <boost/math/distributions/lognormal.hpp>
#include <boost/math/distributions/gamma.hpp>
#include <boost/math/distributions/rayleigh.hpp>
#include <random>

	

//Weather-Based Simulation Framework
namespace WBSF
{
	class CWeatherYear;
	class CWeatherYears;
	class CModelStatVector;

	
	enum TCDDParam { CDD_DIST, CDD_P1, CDD_P2, CDD_P3, CDD_DELTA, CDD_Τᴴ¹, CDD_Τᴴ², NB_CDD_PARAMS };

	class CModelDistribution
	{
	public:
		
		enum TType { NOT_INIT=-1, NORMALS, LOG_NORMAL, LOGISTIC, WEIBULL, GAMMA, FISHER, EXTREME_VALUE, RAYLEIGH, MODIFIED_LOGISTIC, GOMPERTZ1, GOMPERTZ2, FRECHET, LOG_LOGISTIC, NB_DISTRIBUTIONS };

		//CModelDistribution(TType type, double p1, double p2);


		//double get_cdf(double v)const;
		//double get_pdf(double v)const;
		//double get_quantile(double x)const;

		static double get_cdf(double x, TType type, double p1, double p2, double p3=0);
		static double get_quantile(double x, TType type, double p1, double p2, double p3=0);

		static double get_cdf(double x, const std::array<double, NB_CDD_PARAMS>& P);
		static double get_quantile(double x, const std::array<double, NB_CDD_PARAMS>& P);

		static void get_CDD(const std::array<double, NB_CDD_PARAMS>& params, const CWeatherYears& weather, CModelStatVector& CDD);
		static void get_CDD(const std::array<double, NB_CDD_PARAMS>& params, const CWeatherYear& weather, CModelStatVector& CDD);
		static void get_ChillCDD(const std::array<double, NB_CDD_PARAMS>& params, const CWeatherYear& weather, CModelStatVector& CDD);

		static void get_CDF(const std::array<double, NB_CDD_PARAMS>& params, const CModelStatVector& CDD, CModelStatVector& CDF, size_t round=1);
		static void get_CDF(const std::array<double, NB_CDD_PARAMS>& params, const CWeatherYear& weather, CModelStatVector& CDF, size_t round=1);

	protected:

		//TType m_type;
		//double m_p1;
		//double m_p2;

		//std::unique_ptr<boost::math::normal_distribution<double>> p_normal_distribution;
		//std::unique_ptr<boost::math::lognormal_distribution<double>> p_lognormal_distribution;
		//std::unique_ptr<boost::math::logistic_distribution<double>> p_logistic_distribution;
		//std::unique_ptr<boost::math::weibull_distribution<double>> p_weibull_distribution;
		//std::unique_ptr<boost::math::fisher_f_distribution<double>> p_fisher_f_distribution;
		//std::unique_ptr<boost::math::extreme_value_distribution<double>> p_extreme_value_distribution;
		//std::unique_ptr<boost::math::gamma_distribution<double>> p_gamma_distribution;
		//std::unique_ptr<boost::math::rayleigh_distribution<double>> p_rayleigh_distribution;

	};



}