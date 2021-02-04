//*****************************************************************************
// File: LaricobiusOsakensisEquations.h
//
// Class: CLaricobiusOsakensisEquations
//          
//
// Description: 
//				stage development rates, relative development rates
//				stage development rates use optimization table lookup
//
//*****************************************************************************
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

	

	const double CLaricobiusOsakensisEquations::OVP[NB_OVP_PARAMS] = { /*220.3, 47.5*/271.9, 98.3, 2.1, 20.2 };//logistic distribution
	const double CLaricobiusOsakensisEquations::ADE[NB_ADE_PARAMS] = { 264, 18.9, -191.9, 100, -30, 22.0 };//logistic distribution
	const double CLaricobiusOsakensisEquations::EAS[NB_EAS_PARAMS] = { 2.832, 247.0, 5.0 };//weibull distribution



	CLaricobiusOsakensisEquations::CLaricobiusOsakensisEquations(const CRandomGenerator& RG) :
		CEquationTableLookup(RG, LOF::AESTIVAL_DIAPAUSE_ADULT, -20, 35, 0.25)
	{

		/*for (size_t s = 0; s < NB_STAGES; s++)
		{
			for (size_t p = 0; p < NB_RDR_PARAMS; p++)
			{
				m_RDR[s][p] = RDR[s][p];
			}
		}*/

		for (size_t p = 0; p < NB_OVP_PARAMS; p++)
			m_OVP[p] = OVP[p];

		for (size_t p = 0; p < NB_ADE_PARAMS; p++)
			m_ADE[p] = ADE[p];

		for (size_t p = 0; p < NB_EAS_PARAMS; p++)
			m_EAS[p] = m_EAS[p];
	}



	//Daily development rate
	double CLaricobiusOsakensisEquations::ComputeRate(size_t s, double T)const
	{
		ASSERT(s >= 0 && s < NB_STAGES);

		

		static const CDevRateEquation::TDevRateEquation P_EQ[LOF::NB_STAGES] =
		{
			//Non-linear
			CDevRateEquation::WangLanDing_1982,
			CDevRateEquation::WangLanDing_1982,
			CDevRateEquation::WangLanDing_1982,
			CDevRateEquation::WangLanDing_1982,
			CDevRateEquation::WangLanDing_1982,
			CDevRateEquation::WangLanDing_1982,
			CDevRateEquation::WangLanDing_1982,
		};

		static const double P_DEV[LOF::NB_STAGES][6] =
		{
				//Non-linear
				{9.217242e-01,1.158581e-01,3.065010e+00,3.200000e+01,3.200000e+01,3.176815e+00},//egg
				{1.763115e+00,9.538295e-02,4.266326e+00,3.199998e+01,3.200000e+01,1.576118e+00},//L1
				{2.990933e+00,1.265557e-01,4.174986e+00,3.199998e+01,3.200000e+01,1.000002e+00},//L2
				{1.975334e+00,1.025401e-01,7.253662e+00,3.200000e+01,3.200000e+01,1.000003e+00},//L3
				{1.341273e+00,1.201731e-01,5.660822e+00,3.199994e+01,3.200000e+01,1.000011e+00},//L4
//				{3.474653e-01,1.036234e-01,6.432334e+00,3.200000e+01,3.200000e+01,1.000000e+00},//Larva
				{4.550437e-01,8.897248e-02,6.395555e+00,3.199968e+01,3.200000e+01,1.909403e+00},//PrePupa
				{4.731006e-01,1.247642e-01,2.705369e-03,3.200000e+01,3.200000e+01,1.000014e+00},//Pupa
		};



		vector<double> p(begin(P_DEV[s]), end(P_DEV[s]));

		double r = max(0.0, CDevRateEquation::GetRate(P_EQ[s], p, T));

		_ASSERTE(!_isnan(r) && _finite(r) && r >= 0);

		return r;
	}

	//*****************************************************************************
	//CSBRelativeDevRate : compute individual relative development rate 


	double CLaricobiusOsakensisEquations::GetRelativeDevRate(size_t s)const
	{
		const double RDR[NB_STAGES][LOF::NB_RDR_PARAMS] =
		{
			//Relative devlopement Time (individual varition): sigma
			//Non-linear
			{0.137226},//Egg
			{0.296594},//L1
			{0.437863},//L2
			{0.382477},//L3
			{0.300101},//L4
	//		{0.181915},//Larva
			{0.196098},//PrePupae
			{0.135804},//Pupae

		};

		boost::math::lognormal_distribution<double> lndist(-WBSF::Square(RDR[s][σ]) / 2.0, RDR[s][σ]);
		double rT = boost::math::quantile(lndist, m_randomGenerator.Randu());
		while (rT < 0.2 || rT>2.6)//base on individual observation
			rT = exp(RDR[s][μ] * boost::math::quantile(lndist, m_randomGenerator.Randu()));

		_ASSERTE(!_isnan(rT) && _finite(rT));

		//covert relative development time into relative development rate
		double rR = 1/rT;

		return rR;
	}

	double CLaricobiusOsakensisEquations::GetCreationCDD()const
	{
		boost::math::logistic_distribution<double> rldist(m_OVP[μ], m_OVP[ѕ]);

		double CDD = boost::math::quantile(rldist, m_randomGenerator.Rand(0.01, 0.99));
		//while (CDD < 0 || CDD>5000)
			//CDD = boost::math::quantile(rldist, m_randomGenerator.Randu());

		return CDD;
	}


	/*double CLaricobiusOsakensisEquations::GetAestivalDiapauseEndCDD()const
	{
		boost::math::logistic_distribution<double> rldist(m_EAS[μ], m_EAS[ѕ]);

		double CDD = boost::math::quantile(rldist, m_randomGenerator.Randu());
		while (CDD < 0 || CDD>15000)
			CDD = boost::math::quantile(rldist, m_randomGenerator.Randu());

		return CDD;
	}*/

	//*****************************************************************************
	//survival

	
	
	
	
	
	
	
	

	double CLaricobiusOsakensisEquations::GetDailySurvivalRate(size_t s, double T)const
	{
		static const CSurvivalEquation::TSurvivalEquation S_EQ[LOF::NB_STAGES] =
		{
			CSurvivalEquation::GompertzMakeham,//egg
			CSurvivalEquation::Wang2,		   //L1
			CSurvivalEquation::Wang2,		   //L2
			CSurvivalEquation::GompertzMakeham,//L3
			CSurvivalEquation::GompertzMakeham,//L4
			//CSurvivalEquation::Survival_03   //Larva
			CSurvivalEquation::Wang2,		   //PrePupa
			CSurvivalEquation::Wang2,		   //Pupa
		};

		static const double P_SUR[LOF::NB_STAGES][6] =
		{
			{-2.068145e-01, 4.470376e-03, -2.542096e-01, -9.996289e-01, 1.208148e+00},//egg
			{+1.376867e-02, 1.660348e-02, +9.161862e+02, +1.647764e+01, 2.360542e-03},//L1
			{+5.234181e+00, 1.921650e+01, +2.878089e-01, +1.041043e+00, 3.780321e-02},//L2
			{-1.398349e-03, 2.247845e-01, -3.206069e-01, -1.829357e-02, 1.265047e+00},//L3
			{-3.977939e-01, 2.359789e-02, -3.816586e-01, -9.954729e-02, 1.633458e+00},//L4
//			{+5.527855e+01, 7.573470e+01, +5.950859e+01, +1.138818e+01, },			  //Larva
			{+9.536743e-05, 3.249730e-03, +9.562198e+02, +1.207364e+01, 2.253732e-03},//PrePupa
			{+1.293211e+01, 1.881825e+01, +1.570737e+00, +1.824073e+00, 5.332873e-03},//Pupa
		};



		vector<double> p(begin(P_SUR[s]), end(P_SUR[s]));

		double sr = max(0.0, min( 1.0, CSurvivalEquation::GetSurvival(S_EQ[s], p, T)));

		_ASSERTE(!_isnan(sr) && _finite(sr) && sr >= 0 && sr <= 1);

		return sr;
	}



	//*****************************************************************************
	//


	double CLaricobiusOsakensisEquations::GetAdultEmergingCDD()const
	{
		boost::math::weibull_distribution<double> emerging_dist(m_EAS[ʎ], m_EAS[к]);


		double CDD = boost::math::quantile(emerging_dist, m_randomGenerator.Randu());
		while (CDD < 0 || CDD>15000)
			CDD = boost::math::quantile(emerging_dist, m_randomGenerator.Randu());

		return CDD;
	}



	//****************************************************************************
	//

	//l: longevity [days]
	//return fecundity [eggs]
	double CLaricobiusOsakensisEquations::GetFecondity(double l)const
	{
		//AFTER lAMB 2011
		//MEAN = 103.


		//Fecondity from Zilahi-Balogh(2001)
		//100.8 ± 89.6 (range, 2 - 396) eggs.

		double w = l / 7.0;//[days] --> [weeks]
		double fm = 6.1*w + 20.1;
		double fs = 2.6*w + 13.3;

		double f = m_randomGenerator.RandNormal(fm, fs);

		while (f < 2 || f>396)
			f = m_randomGenerator.RandNormal(fm, fs);


		return f;
	}

	//l: longevity [days]
	double CLaricobiusOsakensisEquations::GetAdultLongevity(size_t sex)const
	{
		//Longevity of female
		//value from fecundity/longevity Zilahi-Balogh(2001)
		static const double P[2][2] =
		{
			{ 36.6, 2.4 },
			{ 30.8, 2.2}
		};


		double w = m_randomGenerator.RandNormal(P[sex][0], P[sex][1]);
		ASSERT(w > 3);

		//adjustment for attrition
		return w * 7 * 0.65;//active life [day]
	}

	//T : temperature [°C]
	//day_length  : day length  [h]
	//return Time in soil [days]
	//double CLaricobiusOsakensisEquations::GetTimeInSoil(double T, double day_length)
	//{
	//	//data From Lamb 2005
	//	//NbVal = 8	Bias = 0.00263	MAE = 0.95222	RMSE = 1.25691	CD = 0.99785	R² = 0.99786
	//	//lam0 = 15.8101
	//	//lam1 = 2.50857
	//	//lam2 = 6.64395
	//	//lam3 = 7.81911
	//	//lam_a = 0.1634
	//	//lam_b = 0.2648

	//	static const double AADD[6] = { 15.8, 2.51, 6.64, 7.82, -0.163, 0.265 };
	//	double DiS = 120.0 + (215.0 - 120.0) * 1.0 / (1.0 + exp(-(T - AADD[0]) / AADD[1]));//day in soil base on temperature
	//	double DlF = exp(AADD[4] + AADD[5] * 1.0 / (1.0 + exp(-(day_length - AADD[2]) / AADD[3])));//day length factor

	//	double nb_days_in_soil = DiS * DlF;
	//	return nb_days_in_soil;
	//}



	////T : temperature [°C]
	////day_length  : day length  [h]
	////pupationTime : time to the soil before becoming adult (to complete pre-pupation and pupation) [days]
	////return aestival diapause rate [day-1]
	//double CLaricobiusOsakensisEquations::GetAdultAestivalDiapauseRate(double T, double day_length, double creation_day, double pupation_time)
	//{
	//	double TimeInSoil = GetTimeInSoil(T, day_length);//[day]

	//	double f = exp(m_ADE[ʎ0] + m_ADE[ʎ1] * 1.0 / (1.0 + exp(-(creation_day - m_ADE[ʎ2]) / m_ADE[ʎ3])));//day length factor
	//	double AestivalDiapauseTime = max(0.0, (TimeInSoil - pupation_time));
	//	double ADATime = f * AestivalDiapauseTime;//aestival diapause adult time

	//	double r = min(1.0, 1.0 / ADATime);
	//	return r;
	//}

	////T : temperature [°C]
	////j_day_since_jan : ordinal day since the 1 January of the emergence year (continue after December 31)
	////return: abundance (number of LN adult by normalized beating tray
	//double CLaricobiusOsakensisEquations::GetAdultAbundance(double T, size_t j_day_since_jan)
	//{

	//	//Coefficients:
	//	//	Estimate Std.Error t value Pr(> | t | )
	//	//		(Intercept)-2.815284   1.084082 - 2.597  0.01646 *
	//	//		T            0.088449   0.041528   2.130  0.04461 *
	//	//		jday         0.008683   0.002774   3.130  0.00487 **
	//	//		-- -
	//	//		Signif.codes:  0 ‘***’ 0.001 ‘**’ 0.01 ‘*’ 0.05 ‘.’ 0.1 ‘ ’ 1
	//	//
	//	//		Residual standard error : 0.4831 on 22 degrees of freedom
	//	//		Multiple R - squared : 0.4143, Adjusted R - squared : 0.361
	//	//		F - statistic : 7.78 on 2 and 22 DF, p - value : 0.002784

	//	static const double P[3] = { -2.815284, 0.088449, 0.008683 };
	//	return max(0.0, P[0] + P[1] * T + P[2] * j_day_since_jan);
	//}


}
