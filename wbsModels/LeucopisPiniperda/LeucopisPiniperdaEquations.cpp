//*****************************************************************************
// File: LeucopisPiniperdaEquations.h
//
// Class: CLeucopisPiniperdaEquations
//          
//
// Description: 
//				stage development rates, relative development rates
//				stage development rates use optimization table lookup
//
//*****************************************************************************
// 18/10/2022   Rémi Saint-Amant    Creation 
//*****************************************************************************
#include "LeucopisPiniperdaEquations.h"
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
//Sp	G	To	Th1		Th2		mu		s		n		R²
//Lp	1	45	2.5		18.5	651.1	38.66	916		0.980





	const array<double, LPM::NB_EMERGENCE_PARAMS> CLeucopisPiniperdaEquations::ADULT_EMERG = { 651.1,38.66, 45, 2.5, 18.5 };//logistic distribution
	const double CLeucopisPiniperdaEquations::PUPA_PARAM[NB_PUPA_PARAMS] = { 0, 0, 0, 0, 0, 0};//Not used
	const double CLeucopisPiniperdaEquations::OVIP_PARAM[NB_OVIP_PARAMS] = { 0.983,1.164,0.965 };//same as L. argenticollis (based on nothing)
	


	CLeucopisPiniperdaEquations::CLeucopisPiniperdaEquations(const CRandomGenerator& RG) :
		CEquationTableLookup(RG, LPM::NB_STAGES, -20, 35, 0.25)
	{


		for (size_t p = 0; p < NB_EMERGENCE_PARAMS; p++)
			m_adult_emerg[p] = ADULT_EMERG[p];


		for (size_t p = 0; p < NB_PUPA_PARAMS; p++)
			m_pupa_param[p] = PUPA_PARAM[p];

		for (size_t p = 0; p < NB_OVIP_PARAMS; p++)
			m_ovip_param[p] = OVIP_PARAM[p];


	}



	//Daily development rate
	double CLeucopisPiniperdaEquations::ComputeRate(size_t s, double T)const
	{
		ASSERT(s >= 0 && s < NB_STAGES);



		static const CDevRateEquation::TDevRateEquation P_EQ[LPM::NB_STAGES] =
		{
			//Non-linear
			CDevRateEquation::WangLanDing_1982,//Eggs
			CDevRateEquation::WangLanDing_1982,//Larva
			CDevRateEquation::WangLanDing_1982,//Pupa  (with diapause)
			CDevRateEquation::Poly1,//
		};






		static const double P_DEV[LPM::NB_STAGES][6] =
		{
			//Non-linear
			{0.8509504, 0.1073008, 2.7, 34.8, 34.8, 1.245518},//egg
			{0.0478410, 0.2469562, 3.6, 11.3, 34.8, 0.847055},//Larva
			{ 0.07183078,0.0839344,3.199214,34.81864,34.81868,2.429648},//Pupa (with diapause)
			{1/22.5, 0},//Range 4 to 57 days, median 22.5 days, n = 16 females (argenticollis)
		};



		vector<double> p(begin(P_DEV[s]), end(P_DEV[s]));

		double r = max(0.0, CDevRateEquation::GetRate(P_EQ[s], p, T));

		_ASSERTE(!_isnan(r) && _finite(r) && r >= 0);

		return r;
	}


	//*****************************************************************************
	//CSBRelativeDevRate : compute individual relative development rate 


	double CLeucopisPiniperdaEquations::GetRelativeDevRate(size_t s)const
	{
		const double RDT[NB_STAGES][LPM::NB_RDR_PARAMS] =
		{
			//Relative development Time (individual variation): sigma
			//Non-linear
			{0.247642},//Egg
			{0.209922},//Larva
			{0.372673},//Pupae
			{0.35},//Adult
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



	double CLeucopisPiniperdaEquations::GetDailySurvivalRate(size_t s, double T)const
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
			{3.133109,-1.159329,0.03617767},//Pupa (with diapause)
			{},//Adult
		};


		vector<double> p(begin(P_SUR[s]), end(P_SUR[s]));

		double sr = max(0.0, min(1.0, CSurvivalEquation::GetSurvival(S_EQ[s], p, T)));

		_ASSERTE(!_isnan(sr) && _finite(sr) && sr >= 0 && sr <= 1);

		return sr;
	}



	//*****************************************************************************
	//

	void CLeucopisPiniperdaEquations::GetAdultEmergingCDD(const CWeatherYears& weather, CModelStatVector& CDD)const
	{
		CDegreeDays DDmodel(CDegreeDays::ALLEN_WAVE, m_adult_emerg[Τᴴ¹], m_adult_emerg[Τᴴ²]);
		CModelStatVector DD_daily;
		DDmodel.Execute(weather, DD_daily);
		DDmodel.GetCDD(int(m_adult_emerg[delta]), weather, CDD);
	}


	double CLeucopisPiniperdaEquations::GetAdultEmergingCDD()const
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

	double CLeucopisPiniperdaEquations::GetPreOvipPeriod()const
	{
		//Based on L argenticollis
		static const double m = 13;
		static const double E = 2 * m;
		static const double s = 0.22;

		boost::math::lognormal_distribution<double> To(log(E - m), s);
		double to = E - boost::math::quantile(To, m_randomGenerator.Rand(0.01, 0.99));

		return to * m_ovip_param[0];//Give 4 to 18 
	}




	double CLeucopisPiniperdaEquations::GetFecundity(double L)const
	{
		//Base on L argenticollis
		static const array<double, 3> P = { -90.7,  77.5,   0.0234 };
		return max(4.0, min(163.0, P[0] + P[1] * exp(P[2] * L)));

	}




}
