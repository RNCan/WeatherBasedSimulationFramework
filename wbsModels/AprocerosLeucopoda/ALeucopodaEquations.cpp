//*****************************************************************************
// File: AprocerosLeucopodaEquations.h
//
// Class: CAprocerosLeucopodaEquations
//          
//
// Description: 
//				stage development rates, relative development rates
//				stage development rates use optimization table lookup
//
//*****************************************************************************
// 2020-08-25   Rémi Saint-Amant    Creation 
//*****************************************************************************
#include "ALeucopodaEquations.h"
#include <boost/math/distributions.hpp>
#include <boost/math/distributions/logistic.hpp>
#include "ModelBase/DevRateEquation.h"
#include "ModelBase/SurvivalEquation.h"

using namespace WBSF;
using namespace std;

namespace WBSF
{

	using namespace TZZ;


	const double CAprocerosLeucopodaEquations::EWD[NB_EWD_PARAMS] = { 264, 18.9, -191.9, 100, -30, 22.0 };//logistic distribution

	//equation calibrate with Germany 2014
	//const double CAprocerosLeucopodaEquations::EAS[NB_EAS_PARAMS] = { 885.8, 40.96, -2.9, 999 };//logistic distribution
	//equation calibrate with larval g1 at Québec city 2022 (Lafrenière)
	const double CAprocerosLeucopodaEquations::EAS[NB_EAS_PARAMS] = { 360.6, 1.81, -0.3, 999 };//logistic distribution



	CAprocerosLeucopodaEquations::CAprocerosLeucopodaEquations(const CRandomGenerator& RG) :
		CEquationTableLookup(RG, TZZ::NB_STAGES, -20, 40, 0.25)
	{

		for (size_t p = 0; p < NB_EWD_PARAMS; p++)
			m_EWD[p] = EWD[p];

		for (size_t p = 0; p < NB_EAS_PARAMS; p++)
			m_EAS[p] = EAS[p];
	}



	//Daily development rate
	double CAprocerosLeucopodaEquations::ComputeDailyDevlopmentRate(size_t e, double T)const
	{
		ASSERT(e < NB_STAGES);



		static const array<CDevRateEquation::TDevRateEquation, TZZ::NB_STAGES> D_EQ =
		{
			//Non-linear
			CDevRateEquation::Briere2_1999,//Egg		
			CDevRateEquation::Briere2_1999,//Larva		
			CDevRateEquation::Briere2_1999,//Prepupa	
			CDevRateEquation::Briere2_1999,//Pupa		
			CDevRateEquation::Briere2_1999,//Adult longevity
		};

		static const array< vector<double>, TZZ::NB_STAGES>  D_P =
		{ {
				//parameters
				{0.0001758183,9.999999,-8.949422,39.75},
				{6.732698e-05,9.999959,-9.375635,39.75},
				{0.0003353867,9.999592,-16.85975,39.75},
				{0.0002921427,9.999999,-5.830057,39.75},
				{0.0001497986,9.998879,-27.15193,100.0},
		} };


		//model was adjusted to Quebec observation 2021-2022. 
		//3 factors can explain this difference between European an North America 
		//1- Insects adaptation for North America climate
		//2- Host effect
		//3- Bias caused by laboratory rearing
		static const double BOOST_FACTOR = 1.2;

		assert(e < NB_STAGES);
		double r = max(0.0, CDevRateEquation::GetRate(D_EQ[e], D_P[e], T)) * BOOST_FACTOR;
		assert(!_isnan(r) && _finite(r) && r >= 0);

		return r;
	}

	//*****************************************************************************
	//CSBRelativeDevRate : compute individual relative development rate 


	double CAprocerosLeucopodaEquations::GetRelativeDevRate(size_t s)const
	{
		static const double SIGMA[NB_STAGES] =
		{
			//Relative development Time (individual variation): sigma
			//Non-linear
			//{0.102458},//Egg
			//{0.157012},//Larva
			//{0.202788},//Prepupae
			//{0.117576},//Pupae
			//0.102458,
			//0.157012,
			//0.202788,
			//0.117576,
			0.102458,
			0.157012,
			0.202788,
			0.117576,
			0.102458
			//0.253597,//Adult
		};

		boost::math::lognormal_distribution<double> RDR_dist(-WBSF::Square(SIGMA[s]) / 2.0, SIGMA[s]);
		double RDR = boost::math::quantile(RDR_dist, m_randomGenerator.Randu());
		while (RDR < 0.2 || RDR>2.6)//base on individual observation
			RDR = boost::math::quantile(RDR_dist, m_randomGenerator.Randu());


		_ASSERTE(!_isnan(RDR) && _finite(RDR));

		return RDR;
	}
	//*****************************************************************************
	//survival


	double CAprocerosLeucopodaEquations::ComputeDailySurvivalRate(size_t e, double T)const
	{
		static const CSurvivalEquation::TSurvivalEquation S_EQ[TZZ::NB_STAGES] =
		{
			CSurvivalEquation::Survival_01,//egg
			CSurvivalEquation::Survival_12,//Larva
			CSurvivalEquation::Survival_01,//Prepupa
			CSurvivalEquation::Survival_01,//Pupa
			CSurvivalEquation::Unknown		//Adult
		};

		static const double P_SURVIVAL[TZZ::NB_STAGES][6] =
		{
			{2.353567e+00 , -5.518595e-01, 1.474152e-02},
			{1.113231e+01 , 3.305863e+01 , 7.795624e+01, 1.012803e+01},
			{-3.038607e+01, -9.310410e+00, 3.210279e-01},
			{-2.293128e+01, -8.371444e+00, 2.841417e-01},
			{0,0,0}
		};

		ASSERT(e < NB_STAGES);
		vector<double> p(begin(P_SURVIVAL[e]), end(P_SURVIVAL[e]));

		double sr = (e < ADULT) ? max(0.0, min(1.0, CSurvivalEquation::GetSurvival(S_EQ[e], p, T))) : 1;
		_ASSERTE(!_isnan(sr) && _finite(sr) && sr >= 0 && sr <= 1);

		return sr;
	}



	//*****************************************************************************
	//


	double CAprocerosLeucopodaEquations::GetAdultEmergingCDD()const
	{
		//boost::math::weibull_distribution<double> emerging_dist(m_EAS[ʎ], m_EAS[к]);
		boost::math::logistic_distribution<double> emerging_dist(m_EAS[μ], m_EAS[ѕ]);



		double CDD = boost::math::quantile(emerging_dist, m_randomGenerator.Randu());
		while (CDD < 0 || CDD>15000)
			CDD = boost::math::quantile(emerging_dist, m_randomGenerator.Randu());

		return CDD;
	}



	//****************************************************************************
	//

	//return individual fecundity [eggs]
	double CAprocerosLeucopodaEquations::GetFecondity()const
	{
		//After Papp 2017
		//28.2259016393443
		static const double Fo = 37;
		static const double sigma = 0.545039;

		double f = m_randomGenerator.RandUnbiasedLogNormal(log(Fo), sigma);

		while (f < 2 || f>100)
			f = m_randomGenerator.RandUnbiasedLogNormal(log(Fo), sigma);

		return f;
	}

	double CAprocerosLeucopodaEquations::GetBrood(double Fi, double T, double t, double delta_t)const
	{
		static const double to = 0.0;
		static const TDevRateEquation LAMBDA_EQ = CDevRateEquation::Room_1986;//lambda		

		//parameters
		static const double P_LAMBDA[6] = { 6.865565e-01,2.307398e-01, 1.824046e-02, 1.462013e+01 };

		vector<double> p(begin(P_LAMBDA), end(P_LAMBDA));

		double lambda = max(0.0, CDevRateEquation::GetRate(LAMBDA_EQ, p, T));
		_ASSERTE(!_isnan(lambda) && _finite(lambda));

		double brood = 0;
		if (t >= to)
			brood = round(Fi * (exp(-lambda * (t - to)) - exp(-lambda * ((t - to + delta_t)))));

		return brood;

	}

}
