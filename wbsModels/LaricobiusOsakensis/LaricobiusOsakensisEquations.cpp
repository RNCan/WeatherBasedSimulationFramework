//*****************************************************************************
// File: LaricobiusOsakensisEquations.h
//
// Class: CLaricobiusOsakensisEquations
//          
//
// Description: 
//				stage development rates, relative development rates
//				stage daily survival
//				Adult longevity and fecundity 
//
//*****************************************************************************
// 21/07/2021	Rémi Saint-Amant    New aestivation diapause end and adult emergence from L. nigrinus
// 30/06/2021	Rémi Saint-Amant    L osakensis parameters
// 25/08/2020   Rémi Saint-Amant    Creation 
//*****************************************************************************
#include "LaricobiusOsakensisEquations.h"
#include <boost/math/distributions.hpp>
#include <boost/math/distributions/logistic.hpp>
#include "ModelBase/DevRateEquation.h"
#include "ModelBase/SurvivalEquation.h"

using namespace WBSF;
using namespace LOF;
using namespace std;

namespace WBSF
{

	//*********************************************************************************************
	//fitted parameters for cumulative egg creation from egg and larvae, CEC
	//	NbVal = 30	Bias = -0.16170	MAE = 3.85438	RMSE = 5.36376	CD = 0.98070	R² = 0.98077
	//mu = 1.58011
	//s = 228.2837
	//Th1 = 2.1
	//Th2 = 20.2


	//*********************************************************************************************
	//fitted parameters for Adult Emergence from Soil AES (from L. nigrinus)
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




	const double CLaricobiusOsakensisEquations::CEC[NB_CEC_PARAMS] = { 1.580,228.3,-999, 2.1, 20.2 };//Weibull distribution
	const double CLaricobiusOsakensisEquations::ADE[NB_ADE_PARAMS] = { 121,212,-294.5,105.8,34.8,20 };//logistic distribution
	const double CLaricobiusOsakensisEquations::EAS[NB_EAS_PARAMS] = { 1157.8,125.0,-2.5 };//logistic distribution



	CLaricobiusOsakensisEquations::CLaricobiusOsakensisEquations(const CRandomGenerator& RG) :
		CEquationTableLookup(RG, LOF::NB_STAGES, -20, 35, 0.25)
	{


		for (size_t p = 0; p < NB_CEC_PARAMS; p++)
			m_CEC[p] = CEC[p];

		for (size_t p = 0; p < NB_ADE_PARAMS; p++)
			m_ADE[p] = ADE[p];

		for (size_t p = 0; p < NB_EAS_PARAMS; p++)
			m_EAS[p] = m_EAS[p];
	}



	//Daily development rate
	double CLaricobiusOsakensisEquations::ComputeRate(size_t s, double T)const
	{
		ASSERT(s >= 0 && s < NB_STAGES);



		static const array<CDevRateEquation::TDevRateEquation, LOF::NB_STAGES> P_EQ =
		{
			//Non-linear
			CDevRateEquation::Regniere_2012,//Egg
			CDevRateEquation::Regniere_2012,//L1
			CDevRateEquation::Regniere_2012,//L2
			CDevRateEquation::Regniere_2012,//L3
			CDevRateEquation::Regniere_2012,//L4
			CDevRateEquation::Regniere_2012,//PrePupae
			CDevRateEquation::Regniere_2012,//Pupae
			CDevRateEquation::Unknown,		//aestival diapause adult
			CDevRateEquation::LoganTb_1979	//adult longevity
		};






		static const array< vector<double>, LOF::NB_STAGES>  P_DEV =
		{ {
				//Non-linear
				{ 4.034685e-02, 8.769218e-02, 3.000000e+00, 2.700000e+01, 9.286385e-01, 5.000079e-01 },//Egg
				{ 2.027189e-02, 8.476716e-02, 4.000000e+00, 2.700000e+01, 3.597608e-01, 5.000017e-01 },//L1
				{ 2.027189e-02, 8.476716e-02, 4.000000e+00, 2.700000e+01, 3.597608e-01, 5.000017e-01 },//L2
				{ 2.027189e-02, 8.476716e-02, 4.000000e+00, 2.700000e+01, 3.597608e-01, 5.000017e-01 },//L3
				{ 2.027189e-02, 8.476716e-02, 4.000000e+00, 2.700000e+01, 3.597608e-01, 5.000017e-01 },//L4
				{ 9.052423e-02, 4.551370e-02, 4.000000e+00, 3.400000e+01, 4.999814e+01, 7.393725e+00 },//PrePupae
				{ 9.747868e-03, 1.092349e-01, 0.000000e+00, 3.300000e+01, 1.000469e-01, 5.229575e-01 },//Pupae
				{                                                                                    },//aestival diapause adult
				{ 2.327918e-02, 1.065902e-01, 2.000000e+00, 9.996176e+01                             },//adult
			} };

		 
			
			
		//because the larval stage devrate and mortality was difficulte to estimate, we evaluate larval corrected by percent by stage 
		static const double PROP_BY_STAGE[LOF::NB_STAGES] = { 1, 0.206, 0.203, 0.227, 0.364, 1, 1, 1, 1 };


		//vector<double> p(begin(P_DEV[s]), end(P_DEV[s]));

		double r = max(0.0, CDevRateEquation::GetRate(P_EQ[s], P_DEV[s], T)) / PROP_BY_STAGE[s];
		_ASSERTE(!_isnan(r) && _finite(r) && r >= 0);





		return r;
	}

	//*****************************************************************************
	//CSBRelativeDevRate : compute individual relative development rate 


	double CLaricobiusOsakensisEquations::GetRelativeDevRate(size_t s)const
	{
		static const double SIGMA[NB_STAGES] =
		{
			//Relative devlopement Time (individual varition): sigma
			//Non-linear
			{0.1350},//Egg
			{0.1839},//L1
			{0.1839},//L2
			{0.1839},//L3
			{0.1839},//L4
			{0.2013},//PrePupae
			{0.1278},//Pupae
			{1.0000},//aestival diapause adult
			{0.4007}//adult
		};

		boost::math::lognormal_distribution<double> RDR_dist(-WBSF::Square(SIGMA[s]) / 2.0, SIGMA[s]);
		double RDR = boost::math::quantile(RDR_dist, m_randomGenerator.Randu());
		while (RDR < 0.2 || RDR>2.6)//base on individual observation
			RDR = exp(SIGMA[s] * boost::math::quantile(RDR_dist, m_randomGenerator.Randu()));

		_ASSERTE(!_isnan(RDR) && _finite(RDR));

		return RDR;
	}

	double CLaricobiusOsakensisEquations::GetCreationCDD()const
	{
		//boost::math::logistic_distribution<double> egg_creation_dist(m_CEC[μ], m_CEC[ѕ]);
		boost::math::weibull_distribution<double> egg_creation_dist(m_CEC[μ], m_CEC[ѕ]);
		//boost::math::non_central_f_distribution<double> egg_creation_dist(m_CEC[μ], m_CEC[ѕ], m_CEC[ʎf]);
		double CDD = boost::math::quantile(egg_creation_dist, m_randomGenerator.Rand(0.001, 0.999));

		return CDD;
	}


	double CLaricobiusOsakensisEquations::GetAdultEmergingCDD()const
	{
		boost::math::logistic_distribution<double> emerging_dist(m_EAS[μ], m_EAS[ѕ]);
		double CDD = boost::math::quantile(emerging_dist, m_randomGenerator.Rand(0.001, 0.999));

		return CDD;
	}


	//*****************************************************************************
	//survival










	double CLaricobiusOsakensisEquations::GetDailySurvivalRate(size_t s, double T)const
	{
		static const array<CSurvivalEquation::TSurvivalEquation, LOF::NB_STAGES> S_EQ =
		{
			CSurvivalEquation::Survival_07, //egg
			CSurvivalEquation::Survival_11, //L1
			CSurvivalEquation::Survival_11, //L2
			CSurvivalEquation::Survival_11, //L3
			CSurvivalEquation::Survival_11, //L4
			CSurvivalEquation::Survival_04,	//PrePupa
			CSurvivalEquation::Survival_14,	//Pupa
			CSurvivalEquation::Unknown,		//aestival diapause adult
			CSurvivalEquation::Unknown,		// adult
		};



		static const array< vector<double>, LOF::NB_STAGES>  P_SUR =
		{ {
			{ 7.101617e-01, -1.724674e-03, 2.814947e-01, 2.572669e-02},//egg
			{ 1.107977e-02, +1.838004e+01, 1.000000e+03, 1.736085e+00 },//L1
			{ 1.107977e-02, +1.838004e+01, 1.000000e+03, 1.736085e+00 },//L2
			{ 1.107977e-02, +1.838004e+01, 1.000000e+03, 1.736085e+00 },//L3
			{ 1.107977e-02, +1.838004e+01, 1.000000e+03, 1.736085e+00 },//L4
			{ 9.907457e-01, +2.543370e-01, 3.333060e-06, 6.471209e+01 },//PrePupa
			{ 9.952141e-01, +3.495538e+00, 2.853136e+01, 1.889230e+00 },//Pupa
			{},
			{}
		} };

		double sr = max(0.0, min(1.0, CSurvivalEquation::GetSurvival(S_EQ[s], P_SUR[s], T)));

		_ASSERTE(!_isnan(sr) && _finite(sr) && sr >= 0 && sr <= 1);
		return sr;
	}



	//*****************************************************************************
	//




	//****************************************************************************
	//


	//return fecundity [eggs]
	double CLaricobiusOsakensisEquations::GetFecondity()const
	{
		//AICc,maxLL
		//1690.46,-840.107,5
		static const double Fo = 100.4;
		static const double sigma = 0.355;

		//double Fi = m_randomGenerator.RandUnbiasedLogNormal(log(Fo), sigma);
		//while (Fi < 2 || Fi>120)
		//	Fi = m_randomGenerator.RandUnbiasedLogNormal(log(Fo), sigma);

		boost::math::lognormal_distribution<double> fecondity(log(Fo) - WBSF::Square(sigma) / 2.0, sigma);
		double Fi = boost::math::quantile(fecondity, m_randomGenerator.Rand(0.001, 0.999));

		ASSERT(!_isnan(Fi) && _finite(Fi));


		return Fi;
	}

	double CLaricobiusOsakensisEquations::GetFecondityRate(double age, double T)const
	{
		//AICc,maxLL
		//1698.68,-839.968
		static const CDevRateEquation::TDevRateEquation P_EQ = CDevRateEquation::Taylor_1981;

		static const vector<double> P_FEC = { 0.01518, 10.9, 6.535 };
		double r = max(0.0, CDevRateEquation::GetRate(P_EQ, P_FEC, T));

		_ASSERTE(!_isnan(r) && _finite(r) && r >= 0);

		return r;
	}

}
