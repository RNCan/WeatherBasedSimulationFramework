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

//NbVal = 13	Bias = -0.11439	MAE = 2.53532	RMSE = 3.07999	CD = 0.98610	R² = 0.98629
//mu = 68.07812 {  68.07790, 68.07856}	VM = { 0.00006,   0.00024 }
//s = 38.66296 {  38.66271, 38.66340}	VM = { 0.00008,   0.00026 }
//Th1 = 1.03981 {   1.03935, 1.04029}	VM = { 0.00007,   0.00020 }
//Th2 = 4.03636 {   4.03624, 4.03686}	VM = { 0.00006,   0.00017 }

//N = 64801	T = 0.00023	F = 318.20340
//NbVal = 13	Bias = -1.29676	MAE = 4.20702	RMSE = 4.94744	CD = 0.96501	R² = 0.96751
//mu = 105.08142 { 105.08118, 105.08159}	VM = { 0.00007,   0.00021 }
//s = 88.61601 {  88.61583, 88.61614}	VM = { 0.00009,   0.00014 }

//Beginning of adult emergence from soil (from L. nigrinus)
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



	const double CLaricobiusOsakensisEquations::OVP[NB_OVP_PARAMS] = { 105.1,88.6, 2.1, 20.2 };//logistic distribution
	const double CLaricobiusOsakensisEquations::ADE[NB_ADE_PARAMS] = { 121,212,-294.5,105.8,34.8,20 };//logistic distribution
	const double CLaricobiusOsakensisEquations::EAS[NB_EAS_PARAMS] = { 1157.8,125.0,-2.5 };//logistic distribution



	CLaricobiusOsakensisEquations::CLaricobiusOsakensisEquations(const CRandomGenerator& RG) :
		CEquationTableLookup(RG, LOF::NB_STAGES, -20, 35, 0.25)
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
			CDevRateEquation::LoganTb		//adult longevity
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
			{ 2.874981e-02, 1.065805e-01, 4.000000e+00, 9.998542e+01                             },//adult
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
			{0.13502},//Egg
			{0.18392},//L1
			{0.18392},//L2
			{0.18392},//L3
			{0.18392},//L4
			{0.20131},//PrePupae
			{0.12779},//Pupae
			{1.00000},//aestival diapause adult
			{0.40071}//adult
		};

		boost::math::lognormal_distribution<double> RDR_dist(-WBSF::Square(SIGMA[s]) / 2.0, SIGMA[s]);
		double RDR = boost::math::quantile(RDR_dist, m_randomGenerator.Randu());
		while (RDR < 0.2 || RDR>2.6)//base on individual observation
			RDR = exp(SIGMA[s] * boost::math::quantile(RDR_dist, m_randomGenerator.Randu()));

		_ASSERTE(!_isnan(RDR) && _finite(RDR));

		//covert relative development time into relative development rate
		//double rR = 1/rT;

		return RDR;
	}

	double CLaricobiusOsakensisEquations::GetCreationCDD()const
	{
		boost::math::logistic_distribution<double> rldist(m_OVP[μ], m_OVP[ѕ]);

		double CDD = boost::math::quantile(rldist, m_randomGenerator.Rand(0.001, 0.999));
		//while (CDD < 0 || CDD>5000)
			//CDD = boost::math::quantile(rldist, m_randomGenerator.Randu());

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
			CSurvivalEquation::Survival_12, //egg
			CSurvivalEquation::Survival_09, //L1
			CSurvivalEquation::Survival_09, //L2
			CSurvivalEquation::Survival_09, //L3
			CSurvivalEquation::Survival_09, //L4
			CSurvivalEquation::Survival_14,	//PrePupa
			CSurvivalEquation::Survival_13,	//Pupa
			CSurvivalEquation::Unknown,		//aestival diapause adult
			CSurvivalEquation::Unknown,		// adult
		};


		static const array< vector<double>, LOF::NB_STAGES>  P_SUR =
		{ {
			{+5.245993e+00, +3.051113e+00, +3.303175e+01, +3.197979e-01, 4.203672e+02},//egg
			{+9.779898e-01, +9.284325e-01, -1.356386e+01, -1.034326e-03, 3.889027e-01, 1.110094e+00},//L1
			{+9.779898e-01, +9.284325e-01, -1.356386e+01, -1.034326e-03, 3.889027e-01, 1.110094e+00},//L2
			{+9.779898e-01, +9.284325e-01, -1.356386e+01, -1.034326e-03, 3.889027e-01, 1.110094e+00},//L3
			{+9.779898e-01, +9.284325e-01, -1.356386e+01, -1.034326e-03, 3.889027e-01, 1.110094e+00},//L4
			{-9.350661e+01, -7.338661e+01, +7.080686e+01, +1.199692e+00, 1.326448e+03},//PrePupa
			{+9.952444e-01, +3.471469e+00, +2.855358e+01, +1.896005e+00},//Pupa
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
		//1698.68,-839.968
		static const double Fo = 100.556;
		static const double sigma = 0.357327;

		double Fi = m_randomGenerator.RandUnbiasedLogNormal(log(Fo), sigma);
		while (Fi < 2 || Fi>396)
			Fi = m_randomGenerator.RandUnbiasedLogNormal(log(Fo), sigma);

		/*boost::math::lognormal_distribution<double> fecondity(log(Fo)-WBSF::Square(sigma) / 2.0, sigma);
		double Fi = boost::math::quantile(fecondity, m_randomGenerator.Randu());
		while (Fi < 2 || Fi>396)
			Fi = boost::math::quantile(fecondity, m_randomGenerator.Randu());
*/
		ASSERT(!_isnan(Fi) && _finite(Fi));


		return Fi;
	}

	double CLaricobiusOsakensisEquations::GetFecondityRate(double age, double T)const
	{
		//AICc,maxLL
		//1698.68,-839.968
		static const CDevRateEquation::TDevRateEquation P_EQ = CDevRateEquation::SharpeDeMichele_1977;
		
		static const vector<double> P_FEC = { 5.959380e-04, 3.200656e+01, -3.489816e+04, -5.755484e+04, 1.215706e+01, 9.345022e+05, 2.519381e+01 };
		double r = max(0.0, CDevRateEquation::GetRate(P_EQ, P_FEC, T));

		_ASSERTE(!_isnan(r) && _finite(r) && r >= 0);

		return r;
	}

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
