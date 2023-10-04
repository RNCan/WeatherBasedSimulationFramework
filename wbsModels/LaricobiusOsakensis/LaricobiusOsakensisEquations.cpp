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
				{ 3.962e-02, 8.769e-02, 3, 27, 9.287e-01, 5.000e-01 },//Egg
				{ 1.959e-02, 8.478e-02, 4, 27, 3.596e-01, 5.000e-01 },//L1
				{ 1.959e-02, 8.478e-02, 4, 27, 3.596e-01, 5.000e-01 },//L2
				{ 1.959e-02, 8.478e-02, 4, 27, 3.596e-01, 5.000e-01 },//L3
				{ 1.959e-02, 8.478e-02, 4, 27, 3.596e-01, 5.000e-01 },//L4
				{ 7.032e-02, 8.388e-02, 4, 34, 50.00e+01, 1.064e+01 },//PrePupae
				{ 9.590e-03, 1.092e-01, 0, 32, 1.000e-01, 5.001e-01 },//Pupae
				{                                                   },//aestival diapause adult
				{ 2.488e-02, 1.066e-01, 4, 9.998e+01                },//adult
			} };

		 
			
			
		//because the larval stage development rate and mortality was difficulty to estimate, we evaluate larval corrected by percent by stage 
		static const double PROP_BY_STAGE[LOF::NB_STAGES] = { 1, 0.206, 0.203, 0.227, 0.364, 1, 1, 1, 1 };
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
			//Relative development Time (individual variation): sigma
			//Non-linear
			{0.135},//Egg
			{0.184},//L1
			{0.184},//L2
			{0.184},//L3
			{0.184},//L4
			{0.201},//PrePupae
			{0.128},//Pupae
			{1.000},//aestival diapause adult
			{0.401}//adult
		};

		boost::math::lognormal_distribution<double> RDR_dist(-WBSF::Square(SIGMA[s]) / 2.0, SIGMA[s]);
		double RDR = boost::math::quantile(RDR_dist, m_randomGenerator.Randu());
		while (RDR < 0.2 || RDR>2.6)//base on individual observation
			RDR = boost::math::quantile(RDR_dist, m_randomGenerator.Randu());

		_ASSERTE(!_isnan(RDR) && _finite(RDR));

		return RDR;
	}

	double CLaricobiusOsakensisEquations::GetCreationCDD()const
	{
		boost::math::weibull_distribution<double> egg_creation_dist(m_CEC[μ], m_CEC[ѕ]);
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
			CSurvivalEquation::Survival_01, //egg
			CSurvivalEquation::Survival_01, //L1
			CSurvivalEquation::Survival_01, //L2
			CSurvivalEquation::Survival_01, //L3
			CSurvivalEquation::Survival_01, //L4
			CSurvivalEquation::Survival_01,	//PrePupa
			CSurvivalEquation::Survival_01,	//Pupa
			CSurvivalEquation::Unknown,		//aestival diapause adult
			CSurvivalEquation::Unknown,		// adult
		};
	
		static const array< vector<double>, LOF::NB_STAGES>  P_SUR =
		{ {
			{ -5.906e+00, +1.299e-01, -1.459e-03 },//egg
			{ -1.087e+00, -5.072e-01, +2.209e-02 },//L1
			{ -1.087e+00, -5.072e-01, +2.209e-02 },//L2
			{ -1.087e+00, -5.072e-01, +2.209e-02 },//L3
			{ -1.087e+00, -5.072e-01, +2.209e-02 },//L4
			{ -6.276e+00, +2.963e-01, -8.149e-03 },//PrePupa
			{ +8.134e+00, -1.635e+00, +5.088e-02 },//Pupa
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
