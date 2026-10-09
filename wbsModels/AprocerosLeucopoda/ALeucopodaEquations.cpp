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
// 2026-10-02   Rémi Saint-Amant    new calibration
// 2020-08-25   Rémi Saint-Amant    Creation 
//*****************************************************************************
#include "ALeucopodaEquations.h"
#include "ModelBase/DevRateEquation.h"
#include "ModelBase/SurvivalEquation.h"
#include "ModelBase/ModelDistribution.h"

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
			CDevRateEquation::Regniere_2012,//Egg		
			CDevRateEquation::Regniere_2012,//Larva		
			CDevRateEquation::Regniere_2012,//Prepupa	
			CDevRateEquation::Regniere_2012,//Pupa		
			CDevRateEquation::Regniere_2012,//Adult longevity
		};

		static const array< vector<double>, TZZ::NB_STAGES>  D_PARAMETERS =
		{ {
				//parameters
				{0.0574, 0.0897, 10.0, 33.7, 0.0528, 0.5077 },//Egg
				{0.0156, 0.0664, 0.10, 30.0, 47.218, 0.9773 },//Larval
				{0.1559, 0.0925, 10.0, 30.0, 0.0553, 1.8128 },//PrePupae
				{0.0806, 0.1001, 10.0, 31.8, 0.0486, 0.5112 },//Pupae
				{0.0614, 0.0772, 3.70, 89.9, 0.0192, 1.3228 },//Adult longevity
		} };




		//model was adjusted to Quebec observation 2021-2022. 
		//3 factors can explain this difference between European an North America 
		//1- Insects adaptation for North America climate
		//2- Host effect
		//3- Bias caused by laboratory rearing
		//4-Overheating by the sun?
		static const double BOOST_FACTOR = 1.225;

		assert(e < NB_STAGES);
		double r = max(0.0, CDevRateEquation::GetRate(D_EQ[e], D_PARAMETERS[e], T)) * BOOST_FACTOR;

		return r;
	}

	//*****************************************************************************
	//CSBRelativeDevRate : compute individual relative development rate 


	double CAprocerosLeucopodaEquations::GetRelativeDevRate(size_t s)const
	{
		
		static const array<double, NB_STAGES> SIGMA =
		{
			//Relative development Time (individual variation): sigma
			0.1141,//Egg
			0.1672,//Larval
			0.2173,//PrePupae
			0.1291,//Pupae
			0.2540 //Adult
		};

		CModelDistribution RDR_dist(CModelDistribution::LOG_NORMAL, -WBSF::Square(SIGMA[s]) / 2.0, SIGMA[s]);
		double RDR = RDR_dist.get_quantile(m_randomGenerator.Rand(0.005, 0.995));

		return RDR;
	}
	//*****************************************************************************
	//survival


	double CAprocerosLeucopodaEquations::ComputeDailySurvivalRate(size_t e, double T)const
	{
		static const CSurvivalEquation::TSurvivalEquation S_EQ[TZZ::NB_STAGES] =
		{
			CSurvivalEquation::Survival_12,//egg
			CSurvivalEquation::Survival_12,//Larva
			CSurvivalEquation::Survival_12,//Prepupa
			CSurvivalEquation::Survival_12,//Pupa
			CSurvivalEquation::Constant	   //Adult longevity
		};

		static const array<vector<double>, TZZ::NB_STAGES> S_PARAMETERS =
		{{
			{12.880,25.0426,32.4763,8.1437},
			{11.134,33.0543,77.9288,10.129},
			{8.0865,30.0559,201.657,3.2802},
			{7.4876,31.6510,999.905,2.0670},
			{1,0,0,0}
		}};


		ASSERT(e < NB_STAGES);

		double S = (e < ADULT) ? max(0.0, min(1.0, CSurvivalEquation::GetSurvival(S_EQ[e], S_PARAMETERS[e], T))) : 1;
		_ASSERTE(!_isnan(s) && _finite(s) && s >= 0 && s <= 1);

		return S;
	}



	//*****************************************************************************
	//
	void CAprocerosLeucopodaEquations::InitAdultEmergenceCDD(const CWeatherYear& weather)
	{
		static const CDegreeDays::TDailyMethod DD_METHOD = CDegreeDays::DOUBLE_SINE;

		m_DDmodel = CDegreeDays(DD_METHOD, m_EAS[Τᴴ¹], m_EAS[Τᴴ²]);
		m_DDmodel.GetCDD(m_EAS[Tᴼ], weather, m_CDD);
	}

	double CAprocerosLeucopodaEquations::GenerateAdultEmergingCDD()const
	{
		
		CModelDistribution emerging_dist(CModelDistribution::LOGISTIC, m_EAS[μ], m_EAS[ѕ]);
		
		double CDD = emerging_dist.get_quantile(m_randomGenerator.Rand(0.005, 0.995));
		while (CDD < 0 || CDD>15000)
			CDD = emerging_dist.get_quantile(m_randomGenerator.Rand(0.005, 0.995));


		return CDD;
	}

	CTRef CAprocerosLeucopodaEquations::GenerateAdultEmergingDate()const
	{
		CTRef TRef = m_CDD.GetLastTRef();
		
		double adult_emerging_CDD = GenerateAdultEmergingCDD();

		// Generate by AI: quick search on CDD
		auto it = std::lower_bound(m_CDD.begin(), m_CDD.end(), adult_emerging_CDD,
			[](const CModelStat& CDDmodel, double value) 
			{
				return CDDmodel[CDegreeDays::S_DD] < value;
			});
	
		if(it != m_CDD.end())
			TRef = m_CDD.GetFirstTRef() + long(std::distance(m_CDD.begin(), it));
		
		return TRef;
	}

	

	//****************************************************************************
	//

	//return individual fecundity [eggs]
	double CAprocerosLeucopodaEquations::GetFecondity()const
	{
		//After Papp 2017
		//28.2259016393443
		static const double Fo = 37;
		static const double SIGMA = 0.545;

		CModelDistribution F_dist(CModelDistribution::LOG_NORMAL, log(Fo)-WBSF::Square(SIGMA) / 2.0, SIGMA);
		double f = F_dist.get_quantile(m_randomGenerator.Rand(0.005, 0.995));
		while (f < 2 || f>100)
			f = F_dist.get_quantile(m_randomGenerator.Rand(0.005, 0.995));

		return f;
	}

	double CAprocerosLeucopodaEquations::ComputeOvipositionRatio(double T)const
	{
		static const TDevRateEquation F_EQ = CDevRateEquation::Lamb_1992;//lambda		

		//parameters
		//											    psi  ,  To ,deltaT1, deltaT2
		static const vector<double> F_PARAMETERS = { { 0.6871, 14.6, 1.4688, 5.2385 } };

		double lambda = max(0.0, CDevRateEquation::GetRate(F_EQ, F_PARAMETERS, T));
		_ASSERTE(!_isnan(lambda) && _finite(lambda));

		return lambda;

	}
	
	double CAprocerosLeucopodaEquations::GetBrood(double Fi, double T, double t, double delta_t)const
	{
		static const double to = 0.0;

		double broods = 0;
		if (t >= to)
		{
			double lambda = GetOvipositionRatio(T);
			broods = round(Fi * (exp(-lambda * (t - to)) - exp(-lambda * ((t - to + delta_t)))));
		}

		return broods;

	}


}
