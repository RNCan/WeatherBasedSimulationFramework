//*****************************************************************************
// File: GypsyMothEggHatchEquations.h
//
// Class: CGypsyMothEggHatchEquations
//          
//
// Description: 
//				stage development rates, relative development rates
//				stage development rates use optimization table lookup
//
//*****************************************************************************
// 2026-09-06	Rémi Saint-Amant    Creation 
//*****************************************************************************
#include "GypsyMothEggHatchEquations.h"

#include "ModelBase/DevRateEquation.h"
#include "ModelBase/SurvivalEquation.h"
#include "ModelBase/ModelDistribution.h"


double double_sine_degree_days(double tmin_today, double tmax_today, double tmin_next, double t_lower, double t_upper);


using namespace WBSF;
using namespace WBSF::HOURLY_DATA;
using namespace LDD;
using namespace std;

namespace WBSF
{
	/*Loop = 11, Iteration = 1, Cycle = 1
		N = 105601	T = 22.52541	F = 62515.31000
		NbVal = 348	Bias = 0.26466	MAE = 8.62500	RMSE = 13.40305	CD = 0.88598	R² = 0.89324
		mu = -1.05669 {  -1.06106, -1.05166}	VM = { 0.00098,   0.00441 }
		s = 1063.23637 {1061.29209, 1065.53877}	VM = { 0.35794,   1.82698 }
		P0 = 0.01059 {   0.01059, 0.01059}	VM = { 0.00000,   0.00000 }
		P1 = 0.06068 {   0.06065, 0.06069}	VM = { 0.00000,   0.00001 }
		P4 = 0.01031 {   0.01000, 49.97070}	VM = { 0.00017,   0.00084 }
		P5 = 1.12674 {   0.94625, 1.37448}	VM = { 0.02704,   0.10517 }
		sigma1 = 0.05610 {   0.05590, 0.05616}	VM = { 0.00001,   0.00009 }
		sigma2 = 0.09943 {   0.09909, 0.09984}	VM = { 0.00003,   0.00022 }*/



//parameters estimated with simulated annealing
	const std::array<double, LDD::NB_EOD_PARAMS> CGypsyMothEggHatchEquations::EOD = { 3, 1, 80, -1.05669, 1063.24, 0, 325, 80, 6.5, 17, 0, 0 };//logistic distribution
	//const std::array<double, LDD::NB_EOD_PARAMS> CGypsyMothEggHatchEquations::EOD = { 0.01, 0.1, 0, 35, 4, 5 };//Régnière
	const std::array<double, LDD::NB_EDP_PARAMS> CGypsyMothEggHatchEquations::EDP = { 0.01058, 0.06067, -2, 34, 0.0103, 1.1267 };//Régnière
	const std::array<double, LDD::NB_RDR_PARAMS> CGypsyMothEggHatchEquations::RDR = { 0.056, 0.099 };//log-normals distribution


	CGypsyMothEggHatchEquations::CGypsyMothEggHatchEquations(const CRandomGenerator& RG) :
		CEquationTableLookup(RG, NB_STAGES, -40, 40, 0.25),
		m_EOD(EOD),
		m_EDP(EDP),
		m_RDR(RDR)
	{

	}



	//Daily development rate
	double CGypsyMothEggHatchEquations::ComputeDailyDevlopmentRate(size_t e, double T)const
	{
		ASSERT(e < NB_STAGES);

		double r = 0;
		if (e == 1)
		{
			r = max(0.0, min(1.0, CDevRateEquation::GetRate(CDevRateEquation::Régnière_2012, { m_EDP.begin(), m_EDP.end() }, T)));
		}

		return r;
	}

	//*****************************************************************************
	//CSBRelativeDevRate : compute individual relative development rate 


	double CGypsyMothEggHatchEquations::GetRelativeDevlopmentRate(size_t stage)const
	{
		//static const double SIGMA[NB_STAGES] =
		//{
		//	//Relative development Time (individual variation): sigma
		//	{0},//Diapause egg
		//	{0.095},//Egg
		//	{0},//Larval
		//};

		//return CDevRateEquation::GetRelativeDevlopmentRate(m_randomGenerator, SIGMA[stage]);
		return CDevRateEquation::GetRelativeDevlopmentRate(m_randomGenerator, m_RDR[stage]);
	}

	//*****************************************************************************
	//Survival
	double CGypsyMothEggHatchEquations::ComputeDailySurvivalRate(size_t e, double T)const
	{
		static const array<CSurvivalEquation::TSurvivalEquation, NB_STAGES> S_EQ =
		{
			CSurvivalEquation::Unknown,//Diapause
			CSurvivalEquation::Survival_03, //Egg
			CSurvivalEquation::LastStage,	//larva
		};

		static const array< vector<double>, NB_STAGES>  S_P =
		{ {
			{ 0 },//Diapause
			{ 87.3002, 72.562, 0.0000, 127.341},//Egg
			{ 0 },//Larval
		} };

		assert(e < NB_STAGES);
		double sr = max(0.0, min(1.0, CSurvivalEquation::GetSurvival(S_EQ[e], S_P[e], T)));

		assert(!_isnan(sr) && _finite(sr) && sr >= 0 && sr <= 1);
		return sr;
	}

	void CGypsyMothEggHatchEquations::ComputeEndOfDiapause(const CWeatherYear& weather, CTRef& mu, double& sigma, CModelStatVector& diapause_end_NCDD)const
	{
		int year = weather.GetTRef().GetYear();
		CTPeriod p(year - 1, JANUARY, DAY_01, year, DECEMBER, DAY_31);//always init over 2 years
		//CTPeriod p(year, JANUARY, DAY_01, year, DECEMBER, DAY_31);
		diapause_end_NCDD.Init(p, 2, 0);

#if RSA_MODEL==1



		CTRef begin(year, JANUARY, DAY_01);
		//CTRef  end(year, MARCH, DAY_31);

		assert(weather.HavePrevious());
		//CJDayRef begin(year - 1, m_EOD[NCDDb] - 1);//-1: convert base 1 into base 0
		CJDayRef end(year, m_EOD[NCDDe] - 1);//-1: convert base 1 into base 0

		if (weather.HavePrevious())
			begin = CJDayRef(year - 1, m_EOD[NCDDb] - 1);//-1: convert base 1 into base 0




		array <double, 2> NCDD = { 0 };
		
		if (weather.HavePrevious())
			NCDD[1] = weather[JANUARY].GetStat(H_TMIN)[MEAN] - weather.GetPrevious()[AUGUST].GetStat(H_TMAX)[MEAN];
		else
			NCDD[1] = weather[JANUARY].GetStat(H_TMIN)[MEAN] - weather[AUGUST].GetStat(H_TMAX)[MEAN];


		for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
		{
			if (TRef >= begin && TRef <= end)
			{
				const CWeatherDay& wday = weather.GetDay(TRef);
				double NDD1 = double_sine_degree_days(wday[H_TMIN], wday[H_TMAX], wday.GetNextI< CWeatherDay>()[H_TMIN], m_EOD[Τᴴ¹], m_EOD[Τᴴ²]);
				NCDD[0] += NDD1;

				//########################################################
				//double T = max(m_EOD[Τᴴ¹], wday[H_TNTX][MEAN]);
				//double NDD2 = min(0.0, T - m_EOD[Τᴴ²]);//DD is negative

//				double NDD2 = (wday[H_TNTX][MEAN] >= m_EOD[Τᴴ¹]) ? min(0.0, wday[H_TNTX][MEAN] - m_EOD[Τᴴ²]) : 0;//DD is negative
	//			NCDD[1] += NDD2;

			}


			assert(NCDD[0] <= 0);
			assert(NCDD[1] <= 0);
			diapause_end_NCDD[TRef][0] = NCDD[0];
			diapause_end_NCDD[TRef][1] = NCDD[1];
		}

		size_t NCDDType = size_t(m_EOD[NCDD_TYPE]);
		assert(NCDDType == 0 || NCDDType == 1);
		double NCDDobs = NCDD[NCDDType];


		double DOYbegin = m_EOD[DOYb];
		double DOYend = m_EOD[DOYe];

		double Pmu = m_EOD[μ];
		if (CModelDistribution::TType(m_EOD[Distribution]) != CModelDistribution::LOGISTIC)
		{
			NCDDobs *= -1;
			Pmu *= -1;
			Switch(DOYbegin, DOYend);
		}

		double CDF = CModelDistribution::get_cdf(NCDDobs, CModelDistribution::TType(m_EOD[Distribution]), Pmu, m_EOD[ѕ]);
		int EOD_DOY_mu = (int)Round((DOYbegin - 1) + (DOYend - DOYbegin) * CDF);
		assert(EOD_DOY_mu >= 0 && EOD_DOY_mu <= 150);
		mu = CJDayRef(year, max(0, min(150, EOD_DOY_mu)));

		//assert(m_EOD[DOYe] >= 0 && m_EOD[DOYe] <= 150);
		//mu = CJDayRef(year, m_EOD[DOYe]);

		sigma = m_RDR[Ϙ1];
#endif
	}



	//Return individual cold tolerance temperature
	double CGypsyMothEggHatchEquations::GetColdTolerence()const
	{
		//todo
		static const double mu = -23.84615;
		static const double s = 1.923077;

		boost::math::logistic_distribution<double> establishment_dist(mu, s);
		double cold_tolerence = boost::math::quantile(establishment_dist, m_randomGenerator.Rand(0.01, 0.99));

		return cold_tolerence;
	}





}


