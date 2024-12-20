//*****************************************************************************
// File: LeucotaraxisArgenticollisEquations.h
//
// Class: CLeucotaraxisArgenticollisEquations
//          
//
// Description: 
//				stage development rates, relative development rates
//				stage development rates use optimization table lookup
//
//*****************************************************************************
// 18/10/2022   Rémi Saint-Amant    Creation 
//*****************************************************************************
#include "LeucotaraxisArgenticollisEquations.h"

#include <boost/math/distributions.hpp>
#include <boost/math/distributions/logistic.hpp>

#include "ModelBase/DevRateEquation.h"
#include "ModelBase/SurvivalEquation.h"

using namespace WBSF;
using namespace LAZ;
using namespace std;

namespace WBSF
{
//Best parameters separate, v 1.0.3 (2024 - 05 - 23)
//Sp	G	n		To	Th1		Th2		mu		s		R²
//La	1	913		45	2.5		18.5	F(Tjan)	32.06	0.834
//La	2	1686	45	2.5		50.0	935.8	53.11	0.978


	const array<double, LAZ::NB_EMERGENCE_PARAMS> CLeucotaraxisArgenticollisEquations::ADULT_EMERG = { -999, 32.06, 45, 2.5, 18.5 };//logistic distribution
	const array<double, LAZ::NB_PUPA_PARAMS> CLeucotaraxisArgenticollisEquations::PUPA_PARAM = { 0.2404,0.3327,0.6,8.2,33.6,1.038,0.3404 };//Wang/Lang/Ding parameters (without diapause)
	const array<double, LAZ::NB_C_PARAMS> CLeucotaraxisArgenticollisEquations::C_PARAM = { 0.9475, 1.077, 1.000 };//correction factor to get reasonable parameters for Pupae
	const array<double, LAZ::NB_EOD_PARAMS> CLeucotaraxisArgenticollisEquations::EOD_PARAM = { -9.9, 259.8 };//End of diapause correction
	

	CLeucotaraxisArgenticollisEquations::CLeucotaraxisArgenticollisEquations(const CRandomGenerator& RG) :
		CEquationTableLookup(RG, LAZ::NB_STAGES, -20, 35, 0.25)
	{
		m_adult_emerg = ADULT_EMERG;
		m_pupa_param = PUPA_PARAM;
		m_C_param = C_PARAM;
		m_EOD_param = EOD_PARAM;
	}



	//Daily development rate
	double CLeucotaraxisArgenticollisEquations::ComputeRate(size_t s, double T)const
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
			{0.8509,	0.1073,	2.7,	34.8,	34.8,	1.2426},//egg
			{0.1383,	0.0759,	3.4,	19.1,	33.7,	9.8898},//Larva
			{0.0196,	0.0530, 3.1,	34.9,	34.9,	1.6836},//Pupa (with diapause)
			{1 / 22.5, 0},//Range 4 to 57 days, median 22.5 days, n = 16 females
		};










		vector<double> p(begin(P_DEV[s]), end(P_DEV[s]));

		double r = max(0.0, CDevRateEquation::GetRate(P_EQ[s], p, T));

		_ASSERTE(!_isnan(r) && _finite(r) && r >= 0);

		return r;
	}

	double CLeucotaraxisArgenticollisEquations::GetPupaRate(double T)const
	{
		//psi Tb To Tm sigma
		vector<double> p(begin(m_pupa_param), end(m_pupa_param));
		double r = max(0.0, CDevRateEquation::GetRate(CDevRateEquation::WangLanDing_1982, p, T));
		_ASSERTE(!_isnan(r) && _finite(r) && r >= 0);

		return r;
	}

	double CLeucotaraxisArgenticollisEquations::GetPupaRDR()const
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


	double CLeucotaraxisArgenticollisEquations::GetRelativeDevRate(size_t s)const
	{
		const double RDT[NB_STAGES][LAZ::NB_RDR_PARAMS] =
		{
			//Relative development Time (individual variation): sigma
			//Non-linear
			{0.2477},//Egg
			{0.1963},//Larva
			{0.3828},//Pupae (with diapause)

		};



		if (RDT[s][σ] <= 0)
			return 1;

		double rdt = RDT[s][σ];
		if (s < PUPAE )
			rdt *= m_C_param[2];


		double rT = 0;
		if (s == ADULT)
		{
			boost::math::lognormal_distribution<double> lndist(0, rdt);
			rT = boost::math::quantile(lndist, m_randomGenerator.Randu(true, true));
			while (rT < 0.2 || rT>5.4)//base on individual observation
				rT = boost::math::quantile(lndist, m_randomGenerator.Randu(true, true));
		}
		else 
		{
			boost::math::lognormal_distribution<double> lndist(-WBSF::Square(rdt) / 2.0, rdt);
			rT = boost::math::quantile(lndist, m_randomGenerator.Randu(true, true));
			while (rT < 0.2 || rT>2.6)//base on individual observation
				rT = boost::math::quantile(lndist, m_randomGenerator.Randu(true, true));
		}



		_ASSERTE(!_isnan(rT) && _finite(rT));

		//covert relative development time into relative development rate
		//double rR = 1/rT;//do not inverse

		return rT;
	}


	//*****************************************************************************
	//survival



	double CLeucotaraxisArgenticollisEquations::GetDailySurvivalRate(size_t s, double T)const
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

	void CLeucotaraxisArgenticollisEquations::GetAdultEmergenceCDD(const CWeatherYears& weather, array < CModelStatVector, 2>& CDD)const
	{
		CDegreeDays DDmodel0(CDegreeDays::ALLEN_WAVE, m_adult_emerg[Τᴴ¹], m_adult_emerg[Τᴴ²]);
		CDegreeDays DDmodel1(CDegreeDays::ALLEN_WAVE, m_adult_emerg[Τᴴ¹], 50);//for valudation perpose only

		DDmodel0.GetCDD(int(m_adult_emerg[delta]), weather, CDD[0]);
		DDmodel1.GetCDD(int(m_adult_emerg[delta]), weather, CDD[1]);
	}


	double CLeucotaraxisArgenticollisEquations::GetAdultEmergingCDD(double TjanIn)const
	{
		double Tjan = max(-9.3, TjanIn);
		double mu = m_EOD_param[EOD_B] * (max(-9.3, Tjan) - m_EOD_param[EOD_A]) / (1 + max(-9.3, Tjan) - m_EOD_param[EOD_A]);
		boost::math::logistic_distribution<double> emerging_dist(mu, m_adult_emerg[ѕ]);

		double CDD = boost::math::quantile(emerging_dist, m_randomGenerator.Randu(true, true));
		double p = boost::math::cdf(emerging_dist, CDD);
		while (p < 0.01 || p>0.99)
		{
			CDD = boost::math::quantile(emerging_dist, m_randomGenerator.Randu(true, true));
			p = boost::math::cdf(emerging_dist, CDD);
		}


		return CDD;
	}

	//****************************************************************************
	//

	double CLeucotaraxisArgenticollisEquations::GetPreOvipPeriod()const
	{
		//Range 3 to 18 days, median 13 days, n = 13 females who laid eggs
		//Range 4 to 57 days, median 22.5 days, n = 16 females

		static const double m = 13;
		static const double E = 2 * m;
		static const double s = 0.22;

		boost::math::lognormal_distribution<double> To(log(E - m), s);

		double to = E - boost::math::quantile(To, m_randomGenerator.Rand(0.01, 0.99));//Give 4 to 18 


		return to * m_C_param[0];//adjusted to avoid unrealistic rate for pupa
	}



	double CLeucotaraxisArgenticollisEquations::GetFecondity(double L)const
	{
		//The total number of eggs laid by a female ?
		////Range 5 to 163 eggs, median 39 eggs, n = 13 females who laid eggs(3 females laid 0 eggs, not included in statistics)

		//Based on 100 000 random fecundity and longevity
		//R² = 0.998
		static const array<double, 3> P = { -90.7,  77.5,   0.0234 };
		return max(4.0, min(163.0, P[0] + P[1] * exp(P[2] * L)));
	}
}


