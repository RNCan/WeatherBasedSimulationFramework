//*****************************************************************************
// File: PopilliaJaponicaEquations.h
//
// Class: CPopilliaJaponicaEquations
//          
//
// Description: 
//				stage development rates, relative development rates
//				stage daily survival
//				Adult longevity and fecundity 
//
//*****************************************************************************
// 20/09/2023   Rémi Saint-Amant    Creation
//*****************************************************************************
#include "PopilliaJaponicaEquations.h"
#include "ModelBase/DevRateEquation.h"
#include "ModelBase/SurvivalEquation.h"



using namespace WBSF;
using namespace PJN;
using namespace std;

namespace WBSF
{


	CPopilliaJaponicaEquations::CPopilliaJaponicaEquations(const CRandomGenerator& RG) :
		CEquationTableLookup(RG, PJN::NB_STAGES, -20, 40, 0.25)
	{

		m_EOD = EOD_NA;
		m_ACC = ACC;
		m_other = OTHER_NA;

	}

	bool CPopilliaJaponicaEquations::HaveChange()const
	{
		//std::vector<double> m_last_EOD;
		//std::array<double, NB_CDD_PARAMS> m_last_ACC;
		return m_psy == m_last_psy;
	}








	const std::array<double, PJN::NB_PSY> CPopilliaJaponicaEquations::PSY = { {1,1,1,1,1,1,1} };
	const std::array<double, NB_OTHER_PARAMS> CPopilliaJaponicaEquations::OTHER_NA = { 30.1, 0.3365, 0.93, 0.1815, 0, 18.2651, 1.68499, 0.605301 };
	const std::array<double, NB_OTHER_PARAMS> CPopilliaJaponicaEquations::OTHER_EU = { 30.1, 0.3365, 0.83, 0.3138, 0, 19.9462, 4.91279, 1.74064 };
	
 


		
		
		//Daily development rate
	double CPopilliaJaponicaEquations::ComputeRate(size_t s, double T)const
	{
		ASSERT(s >= 0 && s < NB_STAGES);



		static const array<CDevRateEquation::TDevRateEquation, NB_STAGES> P_EQ =
		{
			//Non-linear
			CDevRateEquation::WangLanDing_1982,//Egg
			CDevRateEquation::WangLanDing_1982,//L1
			CDevRateEquation::WangLanDing_1982,//L2
			CDevRateEquation::WangLanDing_1982,//L3
			CDevRateEquation::WangLanDing_1982,//Pupa
			//CDevRateEquation::Briere1_1999,//Gilioli 
			//CDevRateEquation::Briere1_1999,//Gilioli 
			CDevRateEquation::Poly1,//Adult
			CDevRateEquation::Poly1//dead adult
		};

		static const array< vector<double>, NB_STAGES>  P_DEV =
		{ {
				{0.1800617, 0.1900048, 11.59478, 24.88376, 37.01429, 2.331914},	//EggHatch
				{0.06889619, 0.4363623, -49.96485, 20.39596, 39.82338, 5.017848},//L1
				{0.1020107, 0.1316382, 12.23345, 22.66175, 37.124, 9.999995},	//L2
				{0.07475381, 0.07247321, 12.11973, 31.1684, 37.11498, 4.618004},//L3
				{0.8383731, 0.1105868, 9.75523, 39.50243, 39.50243, 7.732953},	//Pupa

				//{4.04E-5,12.8,29.8},//Gilioli 
				//{8.36E-5,11.4,39.1},//Gilioli 
				{ 1.0 / 30.1, 0},
				{ 0, 0}//dead adult
		} };

		//Adult longevity from ONeil 2008
		//Female: 37.8
		//Male: 32.1


		vector<double> P = P_DEV[s];

		P[0] = P[0] * m_psy[s];

		if (s == ADULT)
		{

			P[0] = 1.0 / m_other[ADULT_LONGEVITY];
		//	P[0] = 1.0 / (-14.0 / 20.0 * max(10.0, min(30.0, T)) + 49.0);
		}

		double r = max(0.0, CDevRateEquation::GetRate(P_EQ[s], P, T));
		_ASSERTE(!_isnan(r) && _finite(r) && r >= 0 && r <= 1);

		return r;
	}

	const vector<double>  CPopilliaJaponicaEquations::EOD_NA =
	{
		0.134796,4.03427,-28.1941,9.68908,20.0274,4.04442,1.33539
	};

	const vector<double>  CPopilliaJaponicaEquations::EOD_EU =
	{
		//0.07476 ,1.07951 ,-27.8375, -1.1266,10.29961,9.74160 ,0.60921
		0.15014 ,5.25655 ,-0.20547,9.75857,16.71604,1.92469 ,0.15349
	};










	double CPopilliaJaponicaEquations::GetRateDiapause(double t)const
	{
		double r = max(0.0, CDevRateEquation::GetRate(CDevRateEquation::WangLanDing_1982, m_EOD, t));
		_ASSERTE(!_isnan(r) && _finite(r) && r >= 0);

		return r;// *m_psy[DEAD_ADULT];
	}

	//*****************************************************************************
	//CSBRelativeDevRate : compute individual relative development rate 


	double CPopilliaJaponicaEquations::GetRelativeDevRate(size_t s)const
	{
		static const double SIGMA[NB_STAGES] =
		{
			//Relative development Time (individual variation): sigma
			{0.0448909},//Egg
			{0.225073},//L1
			{0.209202},//L2
			{0.294282},//L3
			{0.076134},//Pupa
			{0.11956},//Adult
			{ 0.0}//dead adult
		};

		double S = SIGMA[s];
		if (s == ADULT)
			S = m_other[ADULT_SD];

		if (s == L3)
			S = m_other[L3_SD];
		

		return GetRelativeDevRate(S);
	}











	double CPopilliaJaponicaEquations::GetRelativeDevRate(double sigma)const
	{
		if (sigma == 0)
			return 1;

		boost::math::lognormal_distribution<double> dist(-WBSF::Square(sigma) / 2.0, sigma);
		double RDR = boost::math::quantile(dist, m_randomGenerator.Randu());

		//From observation, 0.5 and 2.3
		while (RDR < 0.5 || RDR > 2.3)
			RDR = boost::math::quantile(dist, m_randomGenerator.Randu());

		//double test1 = boost::math::quantile(dist, 0.001);
		//double test2 = boost::math::quantile(dist, 0.999);
		_ASSERTE(!_isnan(RDR) && _finite(RDR));


		//Don't inverse
		return RDR;//return relative development rate
	}

	//const std::array<double, NB_CDD_PARAMS> CPopilliaJaponicaEquations::EOD = { CModelDistribution::LOGISTIC, 100, 2, 0, -50, 5 };
	double CPopilliaJaponicaEquations::GetEndOfDiapauseCDD()const
	{
		return CModelDistribution::get_quantile(m_randomGenerator.Rand(0.01, 0.99), m_ACC);
	}



	//*****************************************************************************
	//survival


	double CPopilliaJaponicaEquations::GetDailySurvivalRate(size_t s, double T)const
	{


		static const array<CSurvivalEquation::TSurvivalEquation, NB_STAGES> S_EQ =
		{
			CSurvivalEquation::Unknown, //egg
			CSurvivalEquation::Unknown, //L1
			CSurvivalEquation::Unknown, //L2
			CSurvivalEquation::Unknown, //L3
			CSurvivalEquation::Unknown, //Pupa
			CSurvivalEquation::Unknown,	// adult
			CSurvivalEquation::Unknown,	// Dead
		};

		static const array< vector<double>, NB_STAGES>  P_SUR =
		{ {
			{0},//egg
			{0},//L1
			{0},//L2
			{0},//L3
			{0},//Pupa
			{0},//Adult
			{0},//Dead
		} };

		double sr = max(0.0, min(1.0, CSurvivalEquation::GetSurvival(S_EQ[s], P_SUR[s], T)));

		_ASSERTE(!_isnan(sr) && _finite(sr) && sr >= 0 && sr <= 1);
		return sr;
	}

	double CPopilliaJaponicaEquations::GetCoolingPoint()const
	{
		//after Ebbenga(2022) thesis
		//survive as bellow -2.77°C and super-cooling point at -7.39±0.912
		return CModelDistribution::get_quantile(m_randomGenerator.Rand(0.01, 0.99), CModelDistribution::NORMALS, -7.39, 0.912);
	}

	//*****************************************************************************
	//

	const std::array<double, NB_CDD_PARAMS> CPopilliaJaponicaEquations::ACC = { CModelDistribution::LOG_LOGISTIC, 345, 7.5, 0, 0, 15, 21.7 };
	double CPopilliaJaponicaEquations::GetAdultCatchCumul(double CDD)const
	{
		return CModelDistribution::get_cdf(CDD, m_ACC);
	}


	//****************************************************************************
	//


	//return fecundity [eggs]
	//double CPopilliaJaponicaEquations::GetFecondity()const
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

	//double CPopilliaJaponicaEquations::GetFecondityRate(double age, double T)const
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
