//*****************************************************************************
// File: LeucopisArgenticollisEquations.h
//
// Class: CLeucopisArgenticollisEquations
//          
//
// Description: 
//				stage development rates, relative development rates
//				stage development rates use optimization table lookup
//
//*****************************************************************************
// 18/10/2022   Rémi Saint-Amant    Creation 
//*****************************************************************************
#include "LeucopisArgenticollisEquations.h"
#include <boost/math/distributions.hpp>
#include <boost/math/distributions/logistic.hpp>
#include "ModelBase/DevRateEquation.h"
#include "ModelBase/SurvivalEquation.h"

using namespace WBSF;
using namespace LAZ;
using namespace std;

namespace WBSF
{


//Best parameters separate, v 1.0.1 (2024-03-30)
//NbVal = 900	Bias = -0.87144	MAE = 10.37989	RMSE = 13.96846	CD = 0.75957	R² = 0.79059
//La_g1_mu = 227.4  La_g1_s = 34.58

//NbVal = 995	Bias = 0.01658	MAE = 9.35849	RMSE = 12.32900	CD = 0.81913	R² = 0.83413
//La_g1_s = 32.05815 {  32.05599, 32.05963}	VM = { 0.00023,   0.00152 }
//EOD_mu = -9.89920 {  -9.89929, -9.89912}	VM = { 0.00002,   0.00009 }
//EOD_s = 259.75168 { 259.74991, 259.75457}	VM = { 0.00083,   0.00208 }




//NbVal = 1674	Bias = 0.56780	MAE = 4.87021	RMSE = 6.55241	CD = 0.93775	R² = 0.95184
//pupa0 = 0.13623
//pupa1 = 0.46349
//pupa2 = 3.62607
//pupa3 = 7.65870
//pupa4 = 30.3276
//pupa5 = 0.45743
//pupa6 = 0.33526
//fec1 = 1.177 
//fec2 = 0.987 


//#NbVal = 1686	Bias = -0.59704	MAE = 5.46918	RMSE = 7.98288	CD = 0.90379	R² = 0.92688
//pupa0 = 0.2404
//pupa1 = 0.3327
//pupa2 = 0.6
//pupa3 = 8.2
//pupa4 = 33.6
//pupa5 = 1.038
//pupa6 = 0.3404
//fec0 = 0.9475
//fec1 = 1.077
//fec2 = 1.000


	//const array<double, LAZ::NB_EMERGENCE_PARAMS> CLeucopisArgenticollisEquations::ADULT_EMERG = { 227.4, 34.58, 45, 2.5, 18.5 };//logistic distribution
	//const array<double, LAZ::NB_PUPA_PARAMS> CLeucopisArgenticollisEquations::PUPA_PARAM = { 0.13623,0.46349,3.62607,7.65870,30.3276,0.45743,0.33526 };//Wang/Lang/Ding parameters (without diapause)
	//const array<double, LAZ::NB_OVIP_PARAMS> CLeucopisArgenticollisEquations::OVIP_PARAM = { 1.000,1.177,0.987 };//correction factor to get reasonable parameters for Pupae
	const array<double, LAZ::NB_EMERGENCE_PARAMS> CLeucopisArgenticollisEquations::ADULT_EMERG = { 0.0, 32.06, 45, 2.5, 18.5 };//logistic distribution
	const array<double, LAZ::NB_PUPA_PARAMS> CLeucopisArgenticollisEquations::PUPA_PARAM = { 0.2404,0.3327,0.6,8.2,33.6,1.038,0.3404 };//Wang/Lang/Ding parameters (without diapause)
	const array<double, LAZ::NB_OVIP_PARAMS> CLeucopisArgenticollisEquations::OVIP_PARAM = { 0.9475, 1.077, 1.000 };//correction factor to get reasonable parameters for Pupae
	const array<double, LAZ::NB_EOD_PARAMS> CLeucopisArgenticollisEquations::EOD_PARAM = { -9.9, 259.8 };//End of diapause correction
	

	CLeucopisArgenticollisEquations::CLeucopisArgenticollisEquations(const CRandomGenerator& RG) :
		CEquationTableLookup(RG, LAZ::NB_STAGES, -20, 35, 0.25)
	{
		m_adult_emerg = ADULT_EMERG;
		m_pupa_param = PUPA_PARAM;
		m_ovip_param = OVIP_PARAM;
		m_EOD_param = EOD_PARAM;
	}



	//Daily development rate
	double CLeucopisArgenticollisEquations::ComputeRate(size_t s, double T)const
	{
		ASSERT(s >= 0 && s < NB_STAGES);



		static const CDevRateEquation::TDevRateEquation P_EQ[LAZ::NB_STAGES] =
		{
			//Non-linear
			CDevRateEquation::WangLanDing_1982,//Eggs
			CDevRateEquation::WangLanDing_1982,//Larva
			CDevRateEquation::WangLanDing_1982,//Pupa (with diapause)
			CDevRateEquation::Poly1,//Adult
		};






		static const double P_DEV[LAZ::NB_STAGES][6] =
		{
			//Non-linear
			{0.8509504, 0.1073008, 2.7, 34.8, 34.8, 1.245518},//egg
			{0.0478410, 0.2469562, 3.6, 11.3, 34.8, 0.847055},//Larva
			{0.0274897,	0.0784325,-0.7, 34.8, 34.8,	0.942664},//Pupa (with diapause)
			{1 / 22.5, 0},//Range 4 to 57 days, median 22.5 days, n = 16 females
		};



		vector<double> p(begin(P_DEV[s]), end(P_DEV[s]));

		double r = max(0.0, CDevRateEquation::GetRate(P_EQ[s], p, T));

		_ASSERTE(!_isnan(r) && _finite(r) && r >= 0);

		return r;
	}

	double CLeucopisArgenticollisEquations::GetPupaRate(double T)const
	{
		//psi Tb To Tm sigma
		vector<double> p(begin(m_pupa_param), end(m_pupa_param));
		double r = max(0.0, CDevRateEquation::GetRate(CDevRateEquation::WangLanDing_1982, p, T));
		_ASSERTE(!_isnan(r) && _finite(r) && r >= 0);

		return r;
	}

	double CLeucopisArgenticollisEquations::GetPupaRDR()const
	{

		boost::math::lognormal_distribution<double> ln_dist(-WBSF::Square(m_pupa_param[PUPA_S]) / 2.0, m_pupa_param[PUPA_S]);
		double rT = boost::math::quantile(ln_dist, m_randomGenerator.Randu());
		while (rT < 0.2 || rT>2.6)//base on individual observation
			rT = boost::math::quantile(ln_dist, m_randomGenerator.Randu(true, true));

		_ASSERTE(!_isnan(rT) && _finite(rT));

		//covert relative development time into relative development rate
		//double rR = 1 / rT;don't do that!!

		return rT;
	}


	//*****************************************************************************
	//CSBRelativeDevRate : compute individual relative development rate 


	double CLeucopisArgenticollisEquations::GetRelativeDevRate(size_t s)const
	{
		const double RDT[NB_STAGES][LAZ::NB_RDR_PARAMS] =
		{
			//Relative development Time (individual variation): sigma
			//Non-linear
			{0.248},//Egg
			{0.210},//Larva
			{0.561},//Pupae (with diapause)
			{0.350},//Adult
		};



		if (RDT[s][σ] <= 0)
			return 1;

		double rdt = RDT[s][σ];
		if (s < PUPAE )
			rdt *= m_ovip_param[2];


		double rT = 0;
		if (s == ADULT)
		{
			boost::math::lognormal_distribution<double> lndist(0, rdt);
			rT = boost::math::quantile(lndist, m_randomGenerator.Randu());
			while (rT < 0.2 || rT>5.4)//base on individual observation
				rT = boost::math::quantile(lndist, m_randomGenerator.Randu());
		}
		else 
		{
			boost::math::lognormal_distribution<double> lndist(-WBSF::Square(rdt) / 2.0, rdt);
			rT = boost::math::quantile(lndist, m_randomGenerator.Randu());
			while (rT < 0.2 || rT>2.6)//base on individual observation
				rT = boost::math::quantile(lndist, m_randomGenerator.Randu());
		}



		_ASSERTE(!_isnan(rT) && _finite(rT));

		//covert relative development time into relative development rate
		//double rR = 1/rT;//do not inverse

		return rT;
	}


	//*****************************************************************************
	//survival



	double CLeucopisArgenticollisEquations::GetDailySurvivalRate(size_t s, double T)const
	{

		static const CSurvivalEquation::TSurvivalEquation S_EQ[LAZ::NB_STAGES] =
		{
			CSurvivalEquation::Survival_01,//egg
			CSurvivalEquation::Survival_01,//Larva
			CSurvivalEquation::Survival_04,//Pupa (with diapause)
			CSurvivalEquation::Unknown,//Adult
		};



		static const double P_SUR[LAZ::NB_STAGES][6] =
		{
			{-3.784744, -0.1881758, 0.00825744, 0},//egg
			{-3.992746, 0.03016423, -0.001645645, 0},//Larva
			{1, 1.517107, 14.31644, 97.17447},//Pupa (with diapause)
			{},//Adult
		};

		




		vector<double> p(begin(P_SUR[s]), end(P_SUR[s]));

		double sr = max(0.0, min(1.0, CSurvivalEquation::GetSurvival(S_EQ[s], p, T)));

		_ASSERTE(!_isnan(sr) && _finite(sr) && sr >= 0 && sr <= 1);

		return sr;
	}



	//*****************************************************************************
	//

	void CLeucopisArgenticollisEquations::GetAdultEmergingCDD(const CWeatherYears& weather, CModelStatVector& CDD)const
	{
		CDegreeDays DDmodel(CDegreeDays::ALLEN_WAVE, m_adult_emerg[Τᴴ¹], m_adult_emerg[Τᴴ²]);
		CModelStatVector DD_daily;
		DDmodel.Execute(weather, DD_daily);
		DDmodel.GetCDD(int(m_adult_emerg[delta]), weather, CDD);
	}


	double CLeucopisArgenticollisEquations::GetAdultEmergingCDD(double Tjan)const
	{
		double mu = m_EOD_param[EOD_B] * (max(-9.5, Tjan) - m_EOD_param[EOD_A]) / (1 + max(-9.5, Tjan) - m_EOD_param[EOD_A]);
		boost::math::logistic_distribution<double> emerging_dist(mu, m_adult_emerg[ѕ]);

//		boost::math::logistic_distribution<double> emerging_dist(m_adult_emerg[μ], m_adult_emerg[ѕ]);
		double CDD = boost::math::quantile(emerging_dist, m_randomGenerator.Randu());
		double p = boost::math::cdf(emerging_dist, CDD);
		while (p < 0.01 || p>0.99)
		{
			CDD = boost::math::quantile(emerging_dist, m_randomGenerator.Randu());
			p = boost::math::cdf(emerging_dist, CDD);
		}


		return CDD;
	}

	//****************************************************************************
	//

	double CLeucopisArgenticollisEquations::GetPreOvipPeriod()const
	{
		//Range 3 to 18 days, median 13 days, n = 13 females who laid eggs
		//Range 4 to 57 days, median 22.5 days, n = 16 females

		static const double m = 13;
		static const double E = 2 * m;
		static const double s = 0.22;

		boost::math::lognormal_distribution<double> To(log(E - m), s);

		double to = E - boost::math::quantile(To, m_randomGenerator.Rand(0.01, 0.99));//Give 4 to 18 


		return to * m_ovip_param[0];//adjusted to avoid unrealistic rate for pupa
	}



	double CLeucopisArgenticollisEquations::GetFecondity(double L)const
	{
		//The total number of eggs laid by a female ?
		////Range 5 to 163 eggs, median 39 eggs, n = 13 females who laid eggs(3 females laid 0 eggs, not included in statistics)

		//Based on 100 000 random fecundity and longevity
		//R² = 0.998
		static const array<double, 3> P = { -90.7,  77.5,   0.0234 };
		return max(4.0, min(163.0, P[0] + P[1] * exp(P[2] * L)));
	}
}


