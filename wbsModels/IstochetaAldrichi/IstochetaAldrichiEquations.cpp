//*****************************************************************************
// File: IstochetaAldrichiEquations.h
//
// Class: CIstochetaAldrichiEquations
//          
//
// Description: 
//				stage development rates, relative development rates
//				stage development rates use optimization table lookup
//
//*****************************************************************************
// 21/05/2026   Rémi Saint-Amant    Creation 
//*****************************************************************************
#include "IstochetaAldrichiEquations.h"
#include <boost/math/distributions.hpp>
#include <boost/math/distributions/logistic.hpp>
#include "ModelBase/DevRateEquation.h"
#include "ModelBase/SurvivalEquation.h"

using namespace WBSF;
using namespace IAM;
using namespace std;

namespace WBSF
{
	using namespace HOURLY_DATA;

	const array<double, IAM::NB_EMERGENCE_PARAMS> CIstochetaAldrichiEquations::ADULT_EMERG = { 10, 1, 0, 0.0, 50.0 };//logistic distribution (no longer used)
	const array<double, IAM::NB_PUPA_PARAMS> CIstochetaAldrichiEquations::PUPA_PARAM = { 0.124, 0.0839,  3.2, 33.0, 34.8, 1.36, 0.257 };//pupa development at spring
	const array<double, IAM::NB_C_PARAMS> CIstochetaAldrichiEquations::C_PARAM = { 0.75, 1.0, 1.0 };



	CIstochetaAldrichiEquations::CIstochetaAldrichiEquations(const CRandomGenerator& RG) :
		CEquationTableLookup(RG, IAM::NB_STAGES, -20, 35, 0.25)
	{
		m_adult_emerg = ADULT_EMERG;
		m_pupa_param = PUPA_PARAM;
		m_C_param = C_PARAM;
	}



	//Daily development rate
	double CIstochetaAldrichiEquations::ComputeDailyDevlopmentRate(size_t e, double T)const
	{
		ASSERT(e < NB_STAGES);



		static const CDevRateEquation::TDevRateEquation P_EQ[IAM::NB_STAGES] =
		{
			//Non-linear
			CDevRateEquation::Poly1,//Eggs
			CDevRateEquation::Poly1,//Larva
			CDevRateEquation::Briere1_1999,//Pupa (after diapause)
			CDevRateEquation::Briere1_1999,//Adult
			CDevRateEquation::Poly1//Dead adult
		};

		static const vector<double> P_DEV[IAM::NB_STAGES] =
		{
			//Non-linear from laboratory rearing
			{1.0 / 1.5, 0.0},//egg
			{1.0 / 5.5, 0.0},//Larva
			//{ 1.496e-05, 5.5, 35.81},//Pupa (Flurina)
			//{9.868973e-06, +0.8, 39.4 },//Pupa (after diapause)
			{ 1.171e-05, 3.1, 37.3 }, //Compute with transfer temperature from Abram 97 cold days
			//{7.625739e-06, -1.67, 40 }, //Compute with transfer temperature from Abram 163 cold days (fastest data)
			{3.813895e-05, -4.853518e+00, 5.649234e+01 },//With only two temperature treatments
			{0, 0}//Dead adult
			
			
			 
			//https://laidbackgardener.blog/2018/07/07/the-fly-that-controls-japanese-beetles/
			// Egg Hatch : Under optimal summer temperatures (25 °C) to (28 °C), the egg hatches in roughly 24 to 48 hours.
			// Larval Penetration : Immediately after hatching, the tiny first - instar larva bores down through the egg and into the beetle's exoskeleton, a process taking up to 24 hours.
			// 1st Instar Larva: This stage typically develops inside the beetle for 2 to 3 days.
			// 2nd & 3rd Instar Larvae: The fly rapidly consumes the host's internal organs(like flight muscles), which causes the beetle to become lethargic and bury itself in the soil.
			// The entire larval feeding and molting process is usually complete within 5 to 6 days after parasitization.
			// Pupation : The larva pupates within the dead host's cadaver, where it overwinter

		};


		double r = max(0.0, CDevRateEquation::GetRate(P_EQ[e], P_DEV[e], T));

		_ASSERTE(!_isnan(r) && _finite(r) && r >= 0);

		return r;
	}


	//*****************************************************************************
	//CSBRelativeDevRate : compute individual relative development rate 
	double CIstochetaAldrichiEquations::GetRelativeDevRate(size_t s)const
	{
		const double SIGMA[NB_STAGES] =
		{
			//Relative development rate (individual variation): sigma
			{0},//Egg
			{0},//Larva
			//{0.09},//Pupa (Flurina data without Abram97)
			{(0.09+0.135)/2.0},//Compromise with and without Abram 97 cold days
			//{0.135},//Compute with transfer temperature from Abram 97 cold days
			//{0.289},//Compute with transfer temperature from Abram 163 cold days (fastest data)
			//{1.0},//Adult
			{0.3},//Adult (reasonable value)
			{0}//Dead adult
		};

		if (SIGMA[s] <= 0)
			return 1;

		boost::math::lognormal_distribution<double> lndist(-WBSF::Square(SIGMA[s]) / 2.0, SIGMA[s]);
		double RDR = boost::math::quantile(lndist, m_randomGenerator.Rand(0.01, 0.99));

		_ASSERTE(!_isnan(RDR) && _finite(RDR));

		return RDR;
	}


	//*****************************************************************************
	//survival



	double CIstochetaAldrichiEquations::ComputeDailySurvivalRate(size_t e, double T)const
	{
		static const array<CSurvivalEquation::TSurvivalEquation, IAM::NB_STAGES> S_EQ =
		{
			CSurvivalEquation::Unknown,//egg
			CSurvivalEquation::Unknown,//Larva
			CSurvivalEquation::Unknown,//Pupa (after diapause)
			CSurvivalEquation::Unknown,//Adult
			CSurvivalEquation::Unknown//Dead adult
		};


		static const array< vector<double>, IAM::NB_STAGES>  S_P =
		{ {
			{1.0},//egg
			{1.0},//Larva
			{1.0},//Pupa (after diapause)
			{1.0},//Adult
			{1.0}//Dead adult
		} };


		assert(e < NB_STAGES);
		double sr = max(0.0, min(1.0, CSurvivalEquation::GetSurvival(S_EQ[e], S_P[e], T)));
		assert(!_isnan(sr) && _finite(sr) && sr >= 0 && sr <= 1);

		return sr;
	}



	//*****************************************************************************
	//

	void CIstochetaAldrichiEquations::GetEndOfDiapauseNCDD(const CModelStatVector& Tsoil, CModelStatVector& output)const
	{
		//CDegreeDays DDmodel(CDegreeDays::ALLEN_WAVE, m_adult_emerg[Τᴴ¹], m_adult_emerg[Τᴴ²]);
		//CModelStatVector DD_daily;
		//DDmodel.Execute(weather, DD_daily);
		//DDmodel.GetCDD(int(m_adult_emerg[delta]), weather, CDD);
		assert(Tsoil.GetTPeriod().GetTM() == CTM::HOURLY);

		CTPeriod p = Tsoil.GetTPeriod();
		output.Init(p, 1, 0);

		int actual_year = p.GetFirstYear();
		double NCDD = 0;
		//for (size_t y = 0; y < p.GetNbYears(); y++)
		//{
			//Tsoil
			//CTPeriod p = Tsoil[y].GetEntireTPeriod();

		for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
		{
			int year = TRef.GetYear();
			if (year != actual_year)
			{
				actual_year = year;
				NCDD = 0;
			}

			//CHourlyData data = weather.GetHour(TRef);
			double T = Tsoil[TRef][0];
			double NDD = min(0.0, T - m_adult_emerg[Τᴴ¹])/24.0;//DD is negative and in daily values

			//size_t DOY = TRef.GetJDay();
			//if (DOY <= m_adult_emerg[delta] - 1)
			NCDD += NDD;

			output[TRef][0] = NCDD;
		}
		//}
	}


	//double CIstochetaAldrichiEquations::GetIndividualEndOfDiapauseNCDD(double NCDD)const
	//{

	//	//boost::math::logistic_distribution<double> emerging_dist(m_adult_emerg[μ], m_adult_emerg[ѕ]);
	//	boost::math::normal_distribution<double> emerging_dist(-WBSF::Square(m_pupa_param[σ]) / 2.0, m_pupa_param[σ]);
	//	double f = boost::math::quantile(emerging_dist, m_randomGenerator.Rand(0.01, 0.99));

	//	return NCDD * f;
	//}


	double CIstochetaAldrichiEquations::GetEmergenceFactor(double NCDD)const
	{
		//from 
		//y = 60.97 - 17.62 / (1 + exp(-(x - 92.35) / 10.23)).
		//count number of day under
		

		boost::math::logistic_distribution<double> emerging_dist(m_adult_emerg[μ], m_adult_emerg[ѕ]);
		//double NCDD = boost::math::quantile(emerging_dist, m_randomGenerator.Rand(0.01, 0.99));
		double CDF = boost::math::cdf(emerging_dist, NCDD);
		assert(CDF >= 0 && CDF <= 1);



		//static const double x = 0.195;
		static const double F = 0.4;
		double ff = -F + 2 * F * (1-CDF);//return value between 0.823 and 1.215
		assert(ff >= -F && ff <= F);
		double f = exp(ff);
		//assert(f >= 0.82 && f <= 1.22);

		return 1.22;
		//return f;

	}



	//****************************************************************************
	//
	double CIstochetaAldrichiEquations::GetPreOvipPeriod()const
	{
		return 0;
	}


	double CIstochetaAldrichiEquations::GetFecundity()const
	{
		//(normally ~80 eggs/female)
		return 80.0;
	}


}
