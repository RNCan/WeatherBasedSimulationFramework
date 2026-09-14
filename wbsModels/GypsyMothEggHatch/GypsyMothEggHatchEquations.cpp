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
#include <boost/math/distributions.hpp>
#include <boost/math/distributions/logistic.hpp>
#include "ModelBase/DevRateEquation.h"
#include "ModelBase/SurvivalEquation.h"
#include "ModelBase/ModelDistribution.h"


struct DailyTemp;
// Calculates negative degree days for a single half-day period
//double calculateHalfDayNegativeDD(double t_min, double t_max, double t_base);
// Accumulates total degree days across a time series using the Double Sine approach
//double accumulateDoubleSineNegativeDD(const std::vector<DailyTemp>& weather_data, double t_base);
double calc_double_sine_negative_dd(double L, double U, double tmin_today, double tmax_today, double tmin_tomorrow);


using namespace WBSF;
using namespace WBSF::HOURLY_DATA;
using namespace LNF;
using namespace std;

namespace WBSF
{
	//Parameters for logistic distribution



// Set A
//Loop = 7, Iteration = 1, Cycle = 1
//N = 157707	T = 0.12500	F = 107229.56000
//NbVal = 278	Bias = 1.48489	MAE = 13.52014	RMSE = 19.63970	CD = 0.68270	R² = 0.71137
//DOYb = 71.87640 {  69.08669, 76.00501}	VM = { 0.67298,   1.89275 }
//DOYe = 90.69661 {  89.36979, 91.76199}	VM = { 0.23857,   0.53678 }
//mu = -251.80343 {-252.93762, -251.54032}	VM = { 0.04002,   0.21008 }
//s = 706.05574 { 704.95566, 706.33698}	VM = { 0.02484,   0.18628 }
//NCDDb = 312.92917 { 310.90757, 314.01831}	VM = { 0.71247,   1.42494 }
//NCDDe = 18.99195 {  16.27924, 20.50805}	VM = { 0.57185,   1.96574 }
//Th1 = 6.14504 {   6.14140, 6.14878}	VM = { 0.00052,   0.00461 }
//Th2 = 13.28849 {  13.28655, 13.29281}	VM = { 0.00025,   0.00156 }
//SIGMAb = 0.02889 {   0.02887, 0.02892}	VM = { 0.00000,   0.00002 }
//SIGMAe = 0.19870 {   0.19863, 0.19876}	VM = { 0.00001,   0.00007 }
//P0 = 0.01000 {   0.01000, 0.01000}	VM = { 0.00000,   0.00000 }
//P1 = 0.09996 {   0.09996, 0.09997}	VM = { 0.00000,   0.00000 }
//P2 = -0.06949 {  -0.06995, -0.06943}	VM = { 0.00001,   0.00013 }
//P3 = 29.05433 {  29.05303, 29.05862}	VM = { 0.00021,   0.00139 }
//P4 = 1.72852 {   1.72790, 1.72886}	VM = { 0.00006,   0.00032 }
//P5 = 4.81949 {   4.81926, 4.82002}	VM = { 0.00005,   0.00026 }
//sigma2 = 0.10011 {   0.10009, 0.10013}	VM = { 0.00000,   0.00001 }
//
//
//Loop = 4, Iteration = 1, Cycle = 1
//N = 81178	T = 12.50000	F = 129531.83000
//NbVal = 249	Bias = 0.56426	MAE = 16.01084	RMSE = 22.80807	CD = 0.59345	R² = 0.61918
//DOYb = 47.23316 {  47.14846, 47.38563}	VM = { 0.00492,   0.06925 }
//DOYe = 115.21732 { 115.05806, 115.41453}	VM = { 0.00401,   0.11409 }
//mu = -1269.46914 {-1270.40100, -1262.87983}	VM = { 0.05010,   1.62736 }
//s = 494.61217 { 486.59187, 505.76921}	VM = { 0.28241,   5.95715 }
//NCDDe = 75.18436 {  73.89727, 77.03988}	VM = { 0.56358,   1.40894 }
//Th1 = 2.92634 {   2.91901, 2.99669}	VM = { 0.00056,   0.01199 }
//Th2 = 11.91223 {  11.89633, 11.91831}	VM = { 0.00028,   0.00631 }
//SIGMAb = 0.01241 {   0.01227, 0.01268}	VM = { 0.00004,   0.00009 }
//SIGMAe = 0.01348 {   0.01304, 0.01358}	VM = { 0.00004,   0.00011 }
//P0 = 0.01001 {   0.01000, 0.01001}	VM = { 0.00000,   0.00000 }
//P1 = 0.09989 {   0.09984, 0.09991}	VM = { 0.00000,   0.00001 }
//P2 = -0.16349 {  -0.17015, -0.16235}	VM = { 0.00025,   0.00139 }
//P3 = 33.20632 {  33.20034, 33.23693}	VM = { 0.00347,   0.01170 }
//P4 = 1.43723 {   1.42026, 1.44221}	VM = { 0.00089,   0.00194 }
//P5 = 5.02659 {   5.02529, 5.03908}	VM = { 0.00089,   0.00182 }
//sigma2 = 0.12337 {   0.12317, 0.12366}	VM = { 0.00005,   0.00017 }
//

	//parameters estimated with simulated annealing
	const std::array<double, LNF::NB_EOD_PARAMS> CGypsyMothEggHatchEquations::EOD = { 2, 30, 90, -294.5, 105.8, 0, 0, 90, -5, 15, 5, 10 };//logistic distribution
	//const std::array<double, LNF::NB_EOD_PARAMS> CGypsyMothEggHatchEquations::EOD = { 0.01, 0.1, 0, 35, 4, 5 };//Régnière
	const std::array<double, LNF::NB_EDP_PARAMS> CGypsyMothEggHatchEquations::EDP = { 0.01, 0.1, 0, 35, 4, 5 };//Régnière
	const std::array<double, LNF::NB_RDR_PARAMS> CGypsyMothEggHatchEquations::RDR = { 0.15, 0.15 };//log-normals distribution


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

		//static const CDevRateEquation::TDevRateEquation D_EQ[NB_STAGES] =
		//{
		//	CDevRateEquation::Unknown,		//Diapause egg
		//	CDevRateEquation::Régnière_2012,//Egg
		//	CDevRateEquation::LastStage		//Larva
		//};
		//
		//static const array< vector<double>, NB_STAGES>  D_P =
		//{ {
		//		//
		//		{0},
		//		{0.01,0.1,0,35,4,5},
		//		{0}
		//} };
		//
		//
		//double r = max(0.0, CDevRateEquation::GetRate(D_EQ[e], D_P[e], T));
		//_ASSERTE(r >= 0);

		double r = 0;
		if (e == 1)
		{
			r = max(0.0, CDevRateEquation::GetRate(CDevRateEquation::Régnière_2012, { m_EDP.begin(), m_EDP.end() }, T));
			//vector<double> P = { m_EDP[0], m_EDP[2], m_EDP[3] };
			//r = max(0.0, CDevRateEquation::GetRate(CDevRateEquation::Briere1_1999, P, T));
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
		CTPeriod p(year - 1, JANUARY, DAY_01, year, DECEMBER, DAY_31);
		diapause_end_NCDD.Init(p, 2, 0);//always init over 2 years

#if RSA_MODEL==1



		CTRef begin(year, JANUARY, DAY_01);
		//CTRef  end(year, MARCH, DAY_31);

		//assert(weather.HavePrevious());
		//CJDayRef begin(year - 1, m_EOD[NCDDb] - 1);//-1: convert base 1 into base 0
		CJDayRef end(year, m_EOD[NCDDe] - 1);//-1: convert base 1 into base 0

		//if (weather.HavePrevious())
			//begin = CJDayRef(year - 1, m_EOD[NCDDb] - 1);//-1: convert base 1 into base 0



		//double NCDD1 = 0;
		double NCDD2 = 0;

		for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
		{
			if (TRef >= begin && TRef <= end)
			{
				const CWeatherDay& wday = weather.GetDay(TRef);

				//// 1. First half-day: from today's min to today's max
				//double ndd_half1 = -calculateHalfDayNegativeDD(wday[H_TMIN], wday[H_TMAX], m_EOD[Τᴴ²]);

				//// 2. Second half-day: from today's max to tomorrow's min
				//double ndd_half2 = -calculateHalfDayNegativeDD(wday.GetNextI< CWeatherDay>()[H_TMIN], wday[H_TMAX], m_EOD[Τᴴ²]);
				//double ndd = ndd_half1 + ndd_half2;


				////remove lower threshold
				//// 3. First half-day: from today's min to today's max
				//double dd_half1 = calculateHalfDayNegativeDD(wday[H_TMIN], wday[H_TMAX], m_EOD[Τᴴ¹]);

				//// 4. Second half-day: from today's max to tomorrow's min
				//double dd_half2 = calculateHalfDayNegativeDD(wday.GetNextI< CWeatherDay>()[H_TMIN], wday[H_TMAX], m_EOD[Τᴴ¹]);

				//double dd = dd_half1 + dd_half2;
				//double NDD1 = ndd + dd;
				//assert(NDD1 <= 0);

				//NCDD1 += NDD1;

				//double NDD1 = calc_double_sine_negative_dd(m_EOD[Τᴴ¹], m_EOD[Τᴴ²], wday[H_TMIN], wday[H_TMAX], wday.GetNextI< CWeatherDay>()[H_TMIN]);
				//NCDD1 += NDD1;

				//########################################################
				double T = max(m_EOD[Τᴴ¹], wday[H_TNTX][MEAN]);
				double NDD2 = min(0.0, T - m_EOD[Τᴴ²]);//DD is negative
				NCDD2 += NDD2;

			}

			assert(NCDD1 <= 0);
			assert(NCDD2 <= 0);
			diapause_end_NCDD[TRef][0] = 0; 
			diapause_end_NCDD[TRef][1] = NCDD2;
		}

		int EOD_DOY_mu =  (int)Round((m_EOD[DOYb] - 1) + (m_EOD[DOYe] - m_EOD[DOYb]) * CModelDistribution::get_cdf(-NCDD2, CModelDistribution::TType(m_EOD[Distribution]), -m_EOD[μ], m_EOD[ѕ]));
		assert(EOD_DOY_mu > 0 && EOD_DOY_mu < 150);

		mu = CJDayRef(year, max(0, min(150, EOD_DOY_mu)));

		//sigma = m_EOD[SIGMAb] + (m_EOD[SIGMAe] - m_EOD[SIGMAb]) * CModelDistribution::get_cdf(-NCDD, CModelDistribution::TType(m_EOD[Distribution]), -m_EOD[μ], m_EOD[ѕ]);
		//sigma = m_EOD[SIGMAb] + (m_EOD[SIGMAe] - m_EOD[SIGMAb]) * CModelDistribution::get_cdf(NCDD1, CModelDistribution::TType(m_EOD[Distribution]), m_EOD[μ], m_EOD[ѕ]);
		sigma = m_RDR[Ϙ1];
#endif
	}

	CTRef CGypsyMothEggHatchEquations::GetEndOfDiapause(CTRef mu, double sigma)const
	{
		//boost::math::logistic_distribution<double> EOD_dist(mu.GetJDay(), sigma);
		boost::math::weibull_distribution<double> EOD_dist(mu.GetJDay(), sigma);
		double EOD_DOY = boost::math::quantile(EOD_dist, m_randomGenerator.Rand(0.001, 0.999));
		assert(EOD_DOY >= 0 && EOD_DOY <= 365);


		return CJDayRef(mu.GetYear(), max(0.0, min(150.0, EOD_DOY)));
	}

	/*double CGypsyMothEggHatchEquations::GetEndOfDiapauseNCDD()const
	{
		boost::math::logistic_distribution<double> creation_dist(m_EOD[μ], m_EOD[ѕ]);
		double NCDD = boost::math::quantile(creation_dist, m_randomGenerator.Rand(0.001, 0.999));
		return NCDD;
	}*/





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


