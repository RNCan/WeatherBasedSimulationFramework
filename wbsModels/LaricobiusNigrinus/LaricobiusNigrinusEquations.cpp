//*****************************************************************************
// File: LaricobiusNigrinusEquations.h
//
// Class: CLaricobiusNigrinusEquations
//          
//
// Description: 
//				stage development rates, relative development rates
//				stage development rates use optimization table lookup
//
//*****************************************************************************
// 28/04/2026   Rémi Saint-Amant    Revision for publication
// 10/03/2019   Rémi Saint-Amant    Creation 
//*****************************************************************************
#include "LaricobiusNigrinusEquations.h"
#include <boost/math/distributions.hpp>
#include <boost/math/distributions/logistic.hpp>
#include "ModelBase/DevRateEquation.h"
#include "ModelBase/SurvivalEquation.h"

using namespace WBSF;
using namespace LNF;
using namespace std;

namespace WBSF
{

	//Minimum developmental threshold temperatures were estimated at
	//5.4°C for eggs, 3.2°C for larvae, 2.9°C for prepupae, and 3.1°C for pupae. Median development times
	//for eggs, larvae, prepupae, and pupae were 59.5, 208.3, 217.4, and 212.8 degree - days(DD) above
	//minimum developmental temperatures, respectively.


	//Parameters for logistic distribution
	//WARNING: logistic describe cumulative of egg abundance (cumulative under the curve)
	//here is calibrated directly from January first with 
	//				  mu     s     ThLo   ThHi	   N     Bias     MAE      RMSE     CD       R²
	//EggCreation:	327.2   60.9   0.8    21.7     71   -0.720   3.501    5.232    0.982    0.983
	//Larvae:       117.0   20.8   6.6    11.9    103   -0.911   5.011    8.385    0.958    0.961

	//parameters estimated with simulated annealing
	//individual creation of egg

	//parameters with egg and larval observation ( non-linear development equation)
	//WARNING: logistic describe cumulative of egg creation
	//NbVal = 174	Bias = -0.31065	MAE = 5.09708	RMSE = 8.51067	CD = 0.95783	R² = 0.95910
	//mu = 220.3
	//s = 47.5 
	//Th1 = 2.1
	//Th2 = 20.2

	//Egg creation and larval development (with correction)
	//NbVal = 173	Bias = -0.99017	MAE = 5.03757	RMSE = 8.31326	CD = 0.95879	R² = 0.96159
	//a1 = 0.813
	//a2 = 1.197
	//mu = 199.4
	//s = 47.86
	//Th1 = 2.4
	//Th2 = 18.2

	//Beginning of adult emergence from soil (with BalcksburgLab)
	//#NbVal = 100	Bias = 0.48893	MAE = 5.32455	RMSE = 7.49034	CD = 0.96157	R² = 0.96300
	//lam0 = 121
	//lam1 = 212
	//lam2 = -294.5
	//lam3 = 105.8
	//lam_a = 34.8
	//lam_b = 20
	//mu_ADE = 1157.8
	//s_ADE = 125.0
	//Th_ADE = -2.5



	//parameters estimated with simulated annealing
	//With linear version


	const std::array<double, LNF::NB_OVP_PARAMS> CLaricobiusNigrinusEquations::OVP = { 220.3, 47.5, 2.1, 20.2 };//logistic distribution
	const std::array<double, LNF::NB_ADE_PARAMS> CLaricobiusNigrinusEquations::ADE = { 121,212,-294.5,105.8,34.8,20 };//logistic distribution
	const std::array<double, LNF::NB_EAS_PARAMS> CLaricobiusNigrinusEquations::EAS = { 1157.8,125.0,-2.5 };//logistic distribution


	CLaricobiusNigrinusEquations::CLaricobiusNigrinusEquations(const CRandomGenerator& RG) :
		CEquationTableLookup(RG, NB_STAGES, -10, 35, 0.25),
		m_OVP(OVP),
		m_ADE(ADE),
		m_EAS(EAS)
	{

	}

	
	
	//Daily development rate
	double CLaricobiusNigrinusEquations::ComputeDailyDevlopmentRate(size_t e, double T)const
	{
		ASSERT(e < NB_STAGES);

		static const CDevRateEquation::TDevRateEquation D_EQ[NB_STAGES] =
		{
			CDevRateEquation::SharpeDeMichele_1977,	//Egg
			CDevRateEquation::SharpeDeMichele_1977,	//Larva
			CDevRateEquation::SharpeDeMichele_1977,	//PrePupa
			CDevRateEquation::SharpeDeMichele_1977,	//Pupa
			CDevRateEquation::Poly1,		//aestival diapause adult
			CDevRateEquation::LoganTb_1979	//adult longevity
		};

		static const array< vector<double>, NB_STAGES>  D_P =
		{ {
				//p25,To,Ha,HL,TL,HH,TH
				{0.0794, 10.9, 2.2245, -55.2932, 5.7, 25.6999, 21.6},
				{0.1205, 23.4, 1.5541, -41.8623, 5.8, 78.232, 24.9 },
				{0.053, 15.2, 1.6354, -20.5995, 4.7, 24.7415, 22.4 },
				{0.0635, 16.2, 1.6958, -51.3567, 2.8, 15.687, 20.3 },
				{1.0 / 198.0, 0},										//after Foley (2021), median time is 198 days
				{ 2.488e-02, 1.066e-01, 4, 9.998e+01                },	//adult 1.05: adjustment between Lo and Ln (from McAvoy unpublished)
		} };


		double r = max(0.0, CDevRateEquation::GetRate(D_EQ[e], D_P[e], T));
		_ASSERTE(!_isnan(r) && _finite(r) && r >= 0);

		return r;
	}

	//*****************************************************************************
	//CSBRelativeDevRate : compute individual relative development rate 


	double CLaricobiusNigrinusEquations::GetRelativeDevlopmentRate(size_t stage)const
	{
		static const double SIGMA[NB_STAGES] =
		{
			//Relative development Time (individual variation): sigma
			{0.095},//Egg
			{0.096},//Larval
			{0.121},//PrePupae
			{0.071},//Pupae
			{1.000},//aestival diapause adult
			{0.401}//adult
		};

		if (SIGMA[stage] == 1.0)
			return 1.0;


		boost::math::lognormal_distribution<double> RDR_dist(-WBSF::Square(SIGMA[stage]) / 2.0, SIGMA[stage]);
		//double RDR = boost::math::quantile(RDR_dist, m_randomGenerator.Rand(0.001, 0.999));

		double RDR = boost::math::quantile(RDR_dist, m_randomGenerator.Randu(true, true));
		while (RDR < 0.2 || RDR>2.6)//base on individual observation
			RDR = boost::math::quantile(RDR_dist, m_randomGenerator.Randu(true, true));

		_ASSERTE(!_isnan(RDR) && _finite(RDR));

		return RDR;
	}

	//*****************************************************************************
	//Survival
	double CLaricobiusNigrinusEquations::ComputeDailySurvivalRate(size_t e, double T)const
	{
		static const array<CSurvivalEquation::TSurvivalEquation, NB_STAGES> S_EQ =
		{
			CSurvivalEquation::Survival_03, //egg
			CSurvivalEquation::Survival_03, //Larval
			CSurvivalEquation::Survival_03,	//PrePupa
			CSurvivalEquation::Survival_03,	//Pupa
			CSurvivalEquation::Survival_constant,//aestival diapause adult
			CSurvivalEquation::Unknown,		//adult
		};

		static const array< vector<double>, NB_STAGES>  S_P =
		{ {
			{ 87.3002, 72.562, 0.0000, 127.341},//egg
			{ 103.791, 9.2726, 7.3132, 25.9916},//Larval
			{ 32.7736, 71.794, 8.9850, 92.7328},//PrePupa
			{ 71.2562, 99.449, 13.679, 95.9139},//Pupa
			//The mean historical subterranean survivorship of laboratory - reared Laricobius (Foley et al., 2021)
			//survival of 39.7% over a period of 198 days. Survival in laboratory is very low, we double the survival
			// D'Auria et al. 2025 show a survival of 40% up to 80%. We a reasonable value of 70% survival.
			//daily survival = (0.7)^(1/198) = 0.9982
			{0.9982},//aestival diapause adult
			{1.0}//adult
		} };

		assert(e < NB_STAGES);
		double sr = max(0.0, min(1.0, CSurvivalEquation::GetSurvival(S_EQ[e], S_P[e], T)));

		assert(!_isnan(sr) && _finite(sr) && sr >= 0 && sr <= 1);
		return sr;
	}




	double CLaricobiusNigrinusEquations::GetCreationCDD()const
	{
		boost::math::logistic_distribution<double> creation_dist(m_OVP[μ], m_OVP[ѕ]);
		double CDD = boost::math::quantile(creation_dist, m_randomGenerator.Rand(0.001, 0.999));
		return CDD;
	}



	//*****************************************************************************
	//Adult Emergence


	double CLaricobiusNigrinusEquations::GetAdultEmergingCDD()const
	{
		boost::math::logistic_distribution<double> emerging_dist(m_EAS[μ], m_EAS[ѕ]);
		double CDD = boost::math::quantile(emerging_dist, m_randomGenerator.Rand(0.001, 0.999));
		return CDD;
	}


	


	//Return individual cold tolerance temperature
	double CLaricobiusNigrinusEquations::GetColdTolerence()const
	{
		//From Crandall 2023: Establishment and post-release recovery of Laricobius nigrinus and Laricobius osakensis ...
		//In Crandall, this regression give the probability of establishment
		//Here, we use it as a direct proxy for cold tolerance
		static const double mu = -23.84615;
		static const double s = 1.923077;

		boost::math::logistic_distribution<double> establishment_dist(mu, s);
		double cold_tolerence = boost::math::quantile(establishment_dist, m_randomGenerator.Rand(0.01, 0.99));

		return cold_tolerence;
	}



	//****************************************************************************
	//Fecundity


	//return fecundity [eggs]
	double CLaricobiusNigrinusEquations::GetFecundity()const
	{
		//AICc,maxLL
		//1690.46,-840.107,5
		static const double Fo = 72.0;//from McAvoy 2024
		static const double sigma = 0.355;//from Foley 2022

		boost::math::lognormal_distribution<double> fecundity(log(Fo) - WBSF::Square(sigma) / 2.0, sigma);
		double Fi = boost::math::quantile(fecundity, m_randomGenerator.Rand(0.01, 0.99));

		ASSERT(!_isnan(Fi) && _finite(Fi));


		return Fi;
	}

	double CLaricobiusNigrinusEquations::ComputeOvipositionRatio(double T)const
	{
		//AICc,maxLL
		//1698.68,-839.968
		static const CDevRateEquation::TDevRateEquation F_EQ = CDevRateEquation::Taylor_1981;

		static const vector<double> F_P = { 0.01518, 10.9, 6.535 };
		double r = max(0.0, CDevRateEquation::GetRate(F_EQ, F_P, T));

		_ASSERTE(!_isnan(r) && _finite(r) && r >= 0);

		return r;
	}



}

