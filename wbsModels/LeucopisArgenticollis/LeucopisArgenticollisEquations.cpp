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


//Best parameters separate, v 1.0.1 (2024-02-25)
//Sp	G	n		To	Th1		Th2		mu		s		R²
//La	1	459		45	2.5		18.5	239.1	44.01	0.870


//			Th1		Th2		mu		s		delta	n		R²
//La G1		2.5		18.5	247.4   42.15 	45		491		0.882
//



//#NbVal = 1674	Bias = -1.43138	MAE = 4.23289	RMSE = 6.37190	CD = 0.94196	R² = 0.95629
//pupa0 = 0.65831
//pupa1 = 0.35164
//pupa2 = 4.9
//pupa3 = 19.8
//pupa4 = 29.6
//pupa5 = 0.59266
//pupa6 = 0.25481
//fec0 = 0.983
//fec1 = 1.164
//fec2 = 0.965



	const array<double, LAZ::NB_EMERGENCE_PARAMS> CLeucopisArgenticollisEquations::ADULT_EMERG = { 239.1,	44.01, 45, 2.5, 18.5 };//logistic distribution
	const double CLeucopisArgenticollisEquations::PUPA_PARAM[NB_PUPA_PARAMS] = { 0.65831,0.35164,4.9,19.8,29.6,0.59266,0.25481 };//Wang/Lang/Ding parameters
	const double CLeucopisArgenticollisEquations::OVIP_PARAM[NB_OVIP_PARAMS] = { 0.983,1.164,0.965 };//correction factor to get reasonable parameters for Pupae

	CLeucopisArgenticollisEquations::CLeucopisArgenticollisEquations(const CRandomGenerator& RG) :
		CEquationTableLookup(RG, LAZ::NB_STAGES, -20, 35, 0.25)
	{


		for (size_t p = 0; p < NB_EMERGENCE_PARAMS; p++)
			m_adult_emerg[p] = ADULT_EMERG[p];


		for (size_t p = 0; p < NB_PUPA_PARAMS; p++)
			m_pupa_param[p] = PUPA_PARAM[p];

		for (size_t p = 0; p < NB_OVIP_PARAMS; p++)
			m_ovip_param[p] = OVIP_PARAM[p];


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
			CDevRateEquation::WangLanDing_1982,//Pupa
			CDevRateEquation::Poly1,//Adult
		};






		static const double P_DEV[LAZ::NB_STAGES][6] =
		{
			//Non-linear
			{0.8509504, 0.1073008, 2.7, 34.8, 34.8, 1.245518},//egg
			{0.0478410, 0.2469562, 3.6, 11.3, 34.8, 0.847055},//Larva
			{0.1796100, 0.4200000, 2.5,  6.2, 30.1, 0.180910},//Pupa
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
		//double r = max(0.0, CDevRateEquation::GetRate(CDevRateEquation::WangEngel_1998, p, T));
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
			{0.105},//Pupae
			{0.350},//Adult
		};



		if (RDT[s][σ] <= 0)
			return 1;


		double rT = 0;
		if (s == ADULT)
		{
			boost::math::lognormal_distribution<double> lndist(0, RDT[s][σ]);
			rT = boost::math::quantile(lndist, m_randomGenerator.Randu());
			while (rT < 0.2 || rT>5.4)//base on individual observation
				rT = boost::math::quantile(lndist, m_randomGenerator.Randu());
		}
		else
		{
			boost::math::lognormal_distribution<double> lndist(-WBSF::Square(RDT[s][σ]) / 2.0, RDT[s][σ]);
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
			CSurvivalEquation::Survival_01,//Pupa
			CSurvivalEquation::Unknown,//Adult
		};



		static const double P_SUR[LAZ::NB_STAGES][6] =
		{
			{-3.562622, -0.2123614, 0.008848417},//egg
			{-4.033288, 0.04749421, -0.002219942},//Larva
			{-4.033288, 0.04749421, -0.002219942},//Pupa (use larva)
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


	double CLeucopisArgenticollisEquations::GetAdultEmergingCDD()const
	{
		//boost::math::weibull_distribution<double> emerging_dist(m_adult_emerg[ʎ], m_adult_emerg[к]);
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

	double CLeucopisArgenticollisEquations::GetPreOvipPeriod()const
	{
		//Range 3 to 18 days, median 13 days, n = 13 females who laid eggs
		//Range 4 to 57 days, median 22.5 days, n = 16 females

		static const double m = 13;
		static const double E = 2 * m;
		static const double s = 0.22;

		boost::math::lognormal_distribution<double> To(log(E - m), s);

		double to = E - boost::math::quantile(To, m_randomGenerator.Rand(0.01, 0.99));//Give 4 to 18 


		return to * m_ovip_param[0];
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


