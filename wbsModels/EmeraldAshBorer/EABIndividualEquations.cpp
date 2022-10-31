//*****************************************************************************
// File: AplanipennisEquations.h
//
// Class: CAplanipennisEquations
//          
//
// Description: 
//				stage development rates, relative development rates
//				stage daily survival
//				Adult longevity and fecundity 
//
//*****************************************************************************
// 30/10/2022   Rémi Saint-Amant    Creation 
//*****************************************************************************
#include "EABIndividualEquations.h"
#include <boost/math/distributions.hpp>
#include <boost/math/distributions/logistic.hpp>
#include "ModelBase/DevRateEquation.h"
#include "ModelBase/SurvivalEquation.h"

using namespace WBSF;
using namespace EAB;
using namespace std;

namespace WBSF
{


	CAplanipennisEquations::CAplanipennisEquations(const CRandomGenerator& RG) :
		CEquationTableLookup(RG, EAB::NB_STAGES, -20, 40, 0.25)
	{


		/*for (size_t p = 0; p < NB_CEC_PARAMS; p++)
			m_CEC[p] = CEC[p];

		for (size_t p = 0; p < NB_ADE_PARAMS; p++)
			m_ADE[p] = ADE[p];

		for (size_t p = 0; p < NB_EAS_PARAMS; p++)
			m_EAS[p] = m_EAS[p];*/
	}



	//Daily development rate
	double CAplanipennisEquations::ComputeRate(size_t s, double T)const
	{
		ASSERT(s >= 0 && s < NB_STAGES);



		static const array<CDevRateEquation::TDevRateEquation, NB_STAGES> P_EQ =
		{
			//Non-linear
			CDevRateEquation::Hansen_2011,//adult emergence
			CDevRateEquation::Hansen_2011,//adult longevity
			CDevRateEquation::Poly1//dead adult
		};
		

		static const array< vector<double>, NB_STAGES>  P_DEV =
		{ {
				//Non-linear
				{ 0.487154, 0.003237, 7.8, 37.1, 2.831609},//adult emergence
				{ 0.017566, 0.128830, 9.0, 14.3, 7.760818},//adult
				{ 0, 0}//dead adult
		} };



		double r = max(0.0, CDevRateEquation::GetRate(P_EQ[s], P_DEV[s], T));
		_ASSERTE(!_isnan(r) && _finite(r) && r >= 0);

		return r;
	}

	//*****************************************************************************
	//CSBRelativeDevRate : compute individual relative development rate 


	double CAplanipennisEquations::GetRelativeDevRate(size_t s)const
	{
		static const double SIGMA[NB_STAGES] =
		{
			//Relative development Time (individual variation): sigma
			//Non-linear
			{0.270},//adult emergence
			{0.269161},//adult
			{0.0}//dead adult
		};

		return GetRelativeDevRate(SIGMA[s]);
	}

	double CAplanipennisEquations::GetRelativeDevRate(double sigma)const
	{
		if (sigma == 0)
			return 1;

		boost::math::lognormal_distribution<double> RDR_dist(-WBSF::Square(sigma) / 2.0, sigma);
		double RDR = boost::math::quantile(RDR_dist, m_randomGenerator.Randu());
		
		//while (RDR < 0.2 || RDR>2.6)//base on individual observation
		while (boost::math::cdf(RDR_dist, RDR)<0.01|| boost::math::cdf(RDR_dist, RDR)>0.99)
			RDR = exp(sigma * boost::math::quantile(RDR_dist, m_randomGenerator.Randu()));

		_ASSERTE(!_isnan(RDR) && _finite(RDR));

		return RDR;
	}


	//*****************************************************************************
	//survival










	//double CAplanipennisEquations::GetDailySurvivalRate(size_t s, double T)const
	//{


	//	static const array<CSurvivalEquation::TSurvivalEquation, LOF::NB_STAGES> S_EQ =
	//	{
	//		CSurvivalEquation::Survival_01, //egg
	//		CSurvivalEquation::Survival_01, //L1
	//		CSurvivalEquation::Survival_01, //L2
	//		CSurvivalEquation::Survival_01, //L3
	//		CSurvivalEquation::Survival_01, //L4
	//		CSurvivalEquation::Survival_01,	//PrePupa
	//		CSurvivalEquation::Survival_01,	//Pupa
	//		CSurvivalEquation::Unknown,		//aestival diapause adult
	//		CSurvivalEquation::Unknown,		// adult
	//	};
	//
	//	static const array< vector<double>, LOF::NB_STAGES>  P_SUR =
	//	{ {
	//		{ -5.906e+00, +1.299e-01, -1.459e-03 },//egg
	//		{ -1.087e+00, -5.072e-01, +2.209e-02 },//L1
	//		{ -1.087e+00, -5.072e-01, +2.209e-02 },//L2
	//		{ -1.087e+00, -5.072e-01, +2.209e-02 },//L3
	//		{ -1.087e+00, -5.072e-01, +2.209e-02 },//L4
	//		{ -6.276e+00, +2.963e-01, -8.149e-03 },//PrePupa
	//		{ +8.134e+00, -1.635e+00, +5.088e-02 },//Pupa
	//		{},
	//		{}
	//	} };

	//	double sr = max(0.0, min(1.0, CSurvivalEquation::GetSurvival(S_EQ[s], P_SUR[s], T)));

	//	_ASSERTE(!_isnan(sr) && _finite(sr) && sr >= 0 && sr <= 1);
	//	return sr;
	//}



	//*****************************************************************************
	//




	//****************************************************************************
	//


	//return fecundity [eggs]
	//double CAplanipennisEquations::GetFecondity()const
	//{
	//	//AICc,maxLL
	//	//1690.46,-840.107,5
	//	static const double Fo = 100.4;
	//	static const double sigma = 0.355;
	//
	//	boost::math::lognormal_distribution<double> fecondity(log(Fo) - WBSF::Square(sigma) / 2.0, sigma);
	//	double Fi = boost::math::quantile(fecondity, m_randomGenerator.Rand(0.001, 0.999));

	//	ASSERT(!_isnan(Fi) && _finite(Fi));


	//	return Fi;
	//}

	//double CAplanipennisEquations::GetFecondityRate(double age, double T)const
	//{
	//	//AICc,maxLL
	//	//1698.68,-839.968
	//	static const CDevRateEquation::TDevRateEquation P_EQ = CDevRateEquation::Taylor_1981;

	//	static const vector<double> P_FEC = { 0.01518, 10.9, 6.535 };
	//	double r = max(0.0, CDevRateEquation::GetRate(P_EQ, P_FEC, T));

	//	_ASSERTE(!_isnan(r) && _finite(r) && r >= 0);

	//	return r;
	//}

}
