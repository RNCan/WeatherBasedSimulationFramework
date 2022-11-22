//*****************************************************************************
// File: SpottedLanternflyEquations.h
//
// Class: CSpottedLanternflyEquations
//          
//
// Description: 
//				stage development rates, relative development rates
//				stage daily survival
//				Adult longevity and fecundity 
//
//*****************************************************************************
// 10/11/2022   Rémi Saint-Amant    Creation 
//*****************************************************************************
#include "SpottedLanternflyEquations.h"
#include "ModelBase/DevRateEquation.h"
#include "ModelBase/SurvivalEquation.h"



using namespace WBSF;
using namespace LDW;
using namespace std;

namespace WBSF
{


	CSpottedLanternflyEquations::CSpottedLanternflyEquations(const CRandomGenerator& RG) :
		CEquationTableLookup(RG, LDW::NB_STAGES, -20, 40, 0.25)
	{

		for (size_t p = 0; p < NB_CDD_PARAMS; p++)
			m_EOD[p] = EOD[p];

		/*for (size_t p = 0; p < NB_CEC_PARAMS; p++)
			m_CEC[p] = CEC[p];

		for (size_t p = 0; p < NB_ADE_PARAMS; p++)
			m_ADE[p] = ADE[p];

		for (size_t p = 0; p < NB_EAS_PARAMS; p++)
			m_EAS[p] = m_EAS[p];*/
	}



	//Daily development rate
	double CSpottedLanternflyEquations::ComputeRate(size_t s, double T)const
	{
		ASSERT(s >= 0 && s < NB_STAGES);



		static const array<CDevRateEquation::TDevRateEquation, NB_STAGES> P_EQ =
		{
			//Non-linear


			//CDevRateEquation::Briere2_1999,//Egg (park)



			//CDevRateEquation::Lamb_1992,//EggHatch(I)
			CDevRateEquation::Schoolfield_1981,//EggHatch(I with fields)
			CDevRateEquation::Regniere_2012,//L1(I)
			CDevRateEquation::Regniere_2012,//N2(I)
			CDevRateEquation::Regniere_2012,//N3(I)
			CDevRateEquation::Regniere_2012,//N4(I)



			//CDevRateEquation::Logan10_1976,//EggHacth(mena+SD+n)
			//CDevRateEquation::Briere1_1999,//L1(mean+SD+n)
			//CDevRateEquation::Briere1_1999,//N2(mean+SD+n)
			//CDevRateEquation::Briere1_1999,//N3(mean+SD+n)
			//CDevRateEquation::Briere1_1999,//N4(mean+SD+n)
			



			

			CDevRateEquation::Poly1,//Adult(same as N4)
			CDevRateEquation::Poly1//dead adult
		};
		
		//"Variable", "Equation", "psi", "To", "deltaT1", "deltaT2", "sigma"


		static const array< vector<double>, NB_STAGES>  P_DEV =
		{ {



				//Non-linear
				//{0.0000597,3.13,9.66,35.8},//Park 2015


				//{0.07089822,34.5058,10.71458,1.917592},//EggHacth(I):Lamb_1992
				{0.1214818,-0.7680791,-3.576105,27.1003,27.92709,33.07768},	//EggHatch(I with fields): Schoolfield_1981
				{ 0.006784378,0.2297697,7.054631,39.92767,0.1003763,4.975535  },	//N1(I):Regniere_2012
				{ 0.001377353,0.09565004,-14.85333,36.02547,2.614162,3.20383  },	//N2(I):Regniere_2012
				{ 0.03842951,0.05283159,18.54452,40.18887,0.1000026,0.5006708},	//N3(I):Regniere_2012
				{ 0.01515363,0.05512501,12.4848,37.226,0.1000279,0.503593    },	//N4(I):Regniere_2012








				//{0.07732734,103.8611,0.2053631,44.83064,3.40282},//EggHatch(mean+sd+n)
				//{6.13061e-05,11.54467,37.92623 },//N1 (Mean+SD+n)
				//{3.730774e-05,6.249316,36.00063 },//N2 (Mean+SD+n)
				//{2.435172e-05,8.786556,44.75016 },//N3 (Mean+SD+n)
				//{1.291956e-05,1.502176,42.62676 },//N4 (Mean+SD+n)
				




				{ 1.0/(11.5*7.0), 0},//From Park, 50% adult live 11.5 weeks
				{ 0, 0}//dead adult
		} };





		


		double r = max(0.0, CDevRateEquation::GetRate(P_EQ[s], P_DEV[s], T));
		_ASSERTE(!_isnan(r) && _finite(r) && r >= 0);

		return r;
	}

	//*****************************************************************************
	//CSBRelativeDevRate : compute individual relative development rate 


	double CSpottedLanternflyEquations::GetRelativeDevRate(size_t s)const
	{
		static const double SIGMA[NB_STAGES] =
		{
			//Relative development Time (individual variation): sigma
			//{ 0.2},//Egg(park)
			
			//{ 0.112523},//EggHacth(Mean+SD+n)
			//{ 0.296268},//N1	 (Mean+SD+n)
			//{ 0.317782},//N2	 (Mean+SD+n)
			//{ 0.741681},//N3	 (Mean+SD+n)
			//{ 0.257491},//N4	 (Mean+SD+n)
			

			//{0.126928},//EggHacth(I): Lamb_1992
			{0.147923},//EggHacth(I with fields):Schoolfield_1981
			{0.387903},//N1(I):Regniere_2012
			{0.301361},//N2(I):Regniere_2012
			{0.774204},//N3(I):Regniere_2012
			{0.269008},//N4(I):Regniere_2012
			

			{ 0.2},//Adult(Mean+SD+n)
			{ 0.0}//dead adult
		};
		
		//array<double, NB_STAGES> factor = { 1.37536,m_psy[DEAD_ADULT],m_psy[DEAD_ADULT],m_psy[DEAD_ADULT],m_psy[DEAD_ADULT],m_psy[DEAD_ADULT],m_psy[DEAD_ADULT]};
		// * factor[s]
		return GetRelativeDevRate(SIGMA[s]);
	}






	double CSpottedLanternflyEquations::GetRelativeDevRate(double sigma)const
	{
		if (sigma == 0)
			return 1;

		boost::math::lognormal_distribution<double> dist(-WBSF::Square(sigma) / 2.0, sigma);
		double RDR = boost::math::quantile(dist, m_randomGenerator.Randu());

		//From observation, 0.5 and 2.3
		while (RDR < 0.5 || RDR > 2.3)
			RDR = exp(sigma * boost::math::quantile(dist, m_randomGenerator.Randu()));

		_ASSERTE(!_isnan(RDR) && _finite(RDR));


		//Don't inverse
		return RDR;//return relative development rate
	}

	const std::array<double, NB_CDD_PARAMS> CSpottedLanternflyEquations::EOD= { CModelDistribution::LOGISTIC, 100, 2, 0, -50, 5 };
	double CSpottedLanternflyEquations::GetEndOfDiapauseCDD()const
	{
		//CModelDistribution dist(TType(m_EOD[CDD_DIST]), m_EOD[CDD_P1], m_EOD[CDD_P2]);
		return CModelDistribution::get_quantile(m_randomGenerator.Rand(0.01, 0.99), m_EOD);
	}



	//*****************************************************************************
	//survival


	double CSpottedLanternflyEquations::GetDailySurvivalRate(size_t s, double T)const
	{


		static const array<CSurvivalEquation::TSurvivalEquation, NB_STAGES> S_EQ =
		{
			CSurvivalEquation::Survival_11, //egg
			CSurvivalEquation::Survival_01, //L1
			CSurvivalEquation::Survival_02, //L2
			CSurvivalEquation::Survival_01, //L3
			CSurvivalEquation::Survival_01, //L4
			CSurvivalEquation::Unknown,		// adult
			CSurvivalEquation::Unknown,		// Dead
		};
	
		static const array< vector<double>, NB_STAGES>  P_SUR =
		{ {
			{0.002164923, 20.43299, 28.53151, 2.200288},//egg
			{-1.656907, -0.2801156, 0.007550485 },//L1
			{2.163025, -2.391411, 0.9919611, 1.234125},//L2
			{-3.413789, -0.1010626, 0.003373287},//L3
			{-5.283617, -0.1097521, 0.006198021},//L4
			{ 0 },//Adult
			{ 0 },//Dead
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
	//double CSpottedLanternflyEquations::GetFecondity()const
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

	//double CSpottedLanternflyEquations::GetFecondityRate(double age, double T)const
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
