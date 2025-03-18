//*****************************************************************************
// File: LeucotaraxisPiniperdaEquations.h
//
// Class: CLeucotaraxisPiniperdaEquations
//          
//
// Description: 
//				stage development rates, relative development rates
//				stage development rates use optimization table lookup
//
//*****************************************************************************
// 13/03/2025   Rémi Saint-Amant    New model starting at larval stage
// 03/03/2025   Rémi Saint-Amant    Add adult longevity, pre-oviposition period and fecundity based on few data observation from Tonya Bittner
// 18/10/2022   Rémi Saint-Amant    Creation 
//*****************************************************************************
#include "LeucotaraxisPiniperdaEquations.h"
#include <boost/math/distributions.hpp>
#include <boost/math/distributions/logistic.hpp>
#include "ModelBase/DevRateEquation.h"
#include "ModelBase/SurvivalEquation.h"

using namespace WBSF;
using namespace LPM;
using namespace std;

namespace WBSF
{


//Best parameters separate, v 1.0.1 (2024-02-25)
//Best parameters separate, v 1.0.3 (2024-05-23)
//Sp	G	n		To	Th1		Th2		mu		s		R²
//Lp	- 1860		45	2.5		18.5	647.8	41.96	0.979


//Best parameters separate, v 1.1.0 (2025-03-13)
//NbVal = 1860	Bias = 0.51871	MAE = 8.49113	RMSE = 12.50527	CD = 0.82072	R² = 0.82527
//pupa0 = 0.12436
//pupa1 = 0.08390
//pupa2=   3.20000  
//pupa3 = 33.02940
//pupa4 = 34.80000
//pupa5 = 1.36097
//pupa6 = 0.25667
//fec0 = 0.74949




	const array<double, LPM::NB_EMERGENCE_PARAMS> CLeucotaraxisPiniperdaEquations::ADULT_EMERG = { 647.8, 41.96, 45, 2.5, 18.5 };//logistic distribution (no longer used)
	const array<double, LPM::NB_PUPA_PARAMS> CLeucotaraxisPiniperdaEquations::PUPA_PARAM = { 0.124, 0.0839,  3.2, 33.0, 34.8, 1.36, 0.257};//pupa developement at spring
	const array<double, LPM::NB_C_PARAMS> CLeucotaraxisPiniperdaEquations::C_PARAM = { 0.75, 1.0, 1.0 };
	


	CLeucotaraxisPiniperdaEquations::CLeucotaraxisPiniperdaEquations(const CRandomGenerator& RG) :
		CEquationTableLookup(RG, LPM::NB_STAGES, -20, 35, 0.25)
	{
			m_adult_emerg = ADULT_EMERG;
			m_pupa_param = PUPA_PARAM;
			m_C_param = C_PARAM;
	}



	//Daily development rate
	double CLeucotaraxisPiniperdaEquations::ComputeRate(size_t s, double T)const
	{
		ASSERT(s >= 0 && s < NB_STAGES);



		static const CDevRateEquation::TDevRateEquation P_EQ[LPM::NB_STAGES] =
		{
			//Non-linear
			CDevRateEquation::WangLanDing_1982,//Eggs
			CDevRateEquation::WangLanDing_1982,//Larva
			CDevRateEquation::WangLanDing_1982,//Pupa  (with dormency)
			CDevRateEquation::Poly1,//Adult
		};

		static const vector<double> P_DEV[LPM::NB_STAGES] =
		{
			//Non-linear from laboratory rearing
			{0.1682, 0.2273, -0.1, 16.1, 34.7, 0.5003},//egg
			{0.0570, 0.3284,-50.0, 15.9, 34.9, 9.0197},//Larva
			{0.0718, 0.0839,  3.2, 34.8, 34.8, 2.4296},//Pupa (with dormency)
			{1/50.0, 0},//Range 24 to 64 days, median 50 days, n = 16 females
		};



		//vector<double> p(begin(P_DEV[s]), end(P_DEV[s]));
		//if (s == PUPAE)
			//p = vector<double>(begin(PUPA_PARAM), end(PUPA_PARAM));


		double r = max(0.0, CDevRateEquation::GetRate(P_EQ[s], P_DEV[s], T));

		_ASSERTE(!_isnan(r) && _finite(r) && r >= 0);

		return r;
	}


	double CLeucotaraxisPiniperdaEquations::GetPupaRate(double T)const
	{
		//psi Tb To Tm sigma
		vector<double> p(begin(m_pupa_param), end(m_pupa_param));
		double r = max(0.0, CDevRateEquation::GetRate(CDevRateEquation::WangLanDing_1982, p, T));
		_ASSERTE(!_isnan(r) && _finite(r) && r >= 0);

		return r;
	}

	double CLeucotaraxisPiniperdaEquations::GetPupaRDR()const
	{

		boost::math::lognormal_distribution<double> ln_dist(-WBSF::Square(m_pupa_param[PUPA_S]) / 2.0, m_pupa_param[PUPA_S]);
		double rT = boost::math::quantile(ln_dist, m_randomGenerator.Randu(true, true));
		while (rT < 0.2 || rT>2.6)//base on individual observation
			rT = boost::math::quantile(ln_dist, m_randomGenerator.Randu(true, true));

		_ASSERTE(!_isnan(rT) && _finite(rT));

		//covert relative development time into relative development rate
		//double rR = 1 / rT;don't do that!!

		return rT;
	}
	//*****************************************************************************
	//CSBRelativeDevRate : compute individual relative development rate 


	double CLeucotaraxisPiniperdaEquations::GetRelativeDevRate(size_t s)const
	{
		const double SIGMA[NB_STAGES] =
		{
			//Relative development Time (individual variation): sigma
			//Non-linear
			{0.1849},//Egg
			{0.2689},//Larva
			{0.3727},//Pupae (with dormency)
			{0.1200},//Adult
		};

		if (SIGMA[s] <= 0)
			return 1;

		double RDR = 0;
		if (s == ADULT)
		{
			//Longevity: Range 24 to 64 days, median 50 days, n = 11 females
			double L_median = 50.0;
			double L_sd = 0.12;
			boost::math::lognormal_distribution<double> lndist(log(L_median), L_sd);
			double L = (2* L_median - boost::math::quantile(lndist, m_randomGenerator.Rand(0.001, 0.999)));
			RDR = L_median/L;
			
		}
		else
		{
			DOUBLE sigma = SIGMA[s];
			//if (s == PUPAE)
				//sigma *= m_C_param[2];//for test only

			boost::math::lognormal_distribution<double> lndist(-WBSF::Square(sigma) / 2.0, sigma);
			RDR = boost::math::quantile(lndist, m_randomGenerator.Randu(true, true));
			while (RDR < 0.2 || RDR>2.6)//base on individual observation
				RDR = boost::math::quantile(lndist, m_randomGenerator.Randu(true, true));
		}

		_ASSERTE(!_isnan(RDR) && _finite(RDR));

		return RDR;
	}


	//*****************************************************************************
	//survival



	double CLeucotaraxisPiniperdaEquations::GetDailySurvivalRate(size_t s, double T)const
	{

		static const CSurvivalEquation::TSurvivalEquation S_EQ[LPM::NB_STAGES] =
		{
			CSurvivalEquation::Survival_01,//egg
			CSurvivalEquation::Survival_01,//Larva
			CSurvivalEquation::Survival_01,//Pupa
			CSurvivalEquation::Unknown,//Adult
		};



		static const double P_SUR[LPM::NB_STAGES][6] =
		{
			{-3.562622, -0.2123614, 0.008848417},//egg
			{-4.033288, 0.04749421, -0.002219942},//Larva
			{3.133109,-1.159329,0.03617767},//Pupa (with dormency)
			{},//Adult
		};


		vector<double> p(begin(P_SUR[s]), end(P_SUR[s]));

		double sr = max(0.0, min(1.0, CSurvivalEquation::GetSurvival(S_EQ[s], p, T)));

		_ASSERTE(!_isnan(sr) && _finite(sr) && sr >= 0 && sr <= 1);

		return sr;
	}



	//*****************************************************************************
	//

	void CLeucotaraxisPiniperdaEquations::GetAdultEmergenceCDD(const CWeatherYears& weather, CModelStatVector& CDD)const
	{
		CDegreeDays DDmodel(CDegreeDays::ALLEN_WAVE, m_adult_emerg[Τᴴ¹], m_adult_emerg[Τᴴ²]);
		CModelStatVector DD_daily;
		DDmodel.Execute(weather, DD_daily);
		DDmodel.GetCDD(int(m_adult_emerg[delta]), weather, CDD);
	}


	double CLeucotaraxisPiniperdaEquations::GetAdultEmergingCDD()const
	{
		boost::math::logistic_distribution<double> emerging_dist(m_adult_emerg[μ], m_adult_emerg[ѕ]);

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

	double CLeucotaraxisPiniperdaEquations::GetPreOvipPeriod()const
	{
		//Pre-oviposition: Range 15 to 50 days, median 22 days, n = 7 females who laid eggs
		static const double m = 22;
		static const double s = 0.125;

		boost::math::lognormal_distribution<double> To(log(m), s);
		double to = boost::math::quantile(To, m_randomGenerator.Rand(0.01, 0.99));

		return to;
	}




	double CLeucotaraxisPiniperdaEquations::GetFecundity(double L)const
	{
		//Fecundity: Range 1 to 132 eggs, median 34 eggs, n = 7 females who laid eggs (4 females laid 0 eggs, not included)
		static const array<double, 3> P = { 4.722, 0.131, 0.108 };
		return max(1.0, min(132.0, P[0] + P[1] * exp(P[2] * L)));

	}




}
