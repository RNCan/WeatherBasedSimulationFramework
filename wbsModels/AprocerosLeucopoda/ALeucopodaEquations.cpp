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
// 2020-08-25   Rémi Saint-Amant    Creation 
//*****************************************************************************
#include "ALeucopodaEquations.h"
#include <boost/math/distributions.hpp>
#include <boost/math/distributions/logistic.hpp>
#include "ModelBase/DevRateEquation.h"
#include "ModelBase/SurvivalEquation.h"

using namespace WBSF;
using namespace std;

namespace WBSF
{

	using namespace TZZ;


	const double CAprocerosLeucopodaEquations::EWD[NB_EWD_PARAMS] = { 264, 18.9, -191.9, 100, -30, 22.0 };//logistic distribution
	const double CAprocerosLeucopodaEquations::EAS[NB_EAS_PARAMS] = { 885.8, 40.96, -2.9, 999 };//logistic distribution


	CAprocerosLeucopodaEquations::CAprocerosLeucopodaEquations(const CRandomGenerator& RG) :
		CEquationTableLookup(RG, TZZ::NB_STAGES, -20, 30, 0.25)
	{

		/*for (size_t s = 0; s < NB_STAGES; s++)
		{
			for (size_t p = 0; p < NB_RDR_PARAMS; p++)
			{
				m_RDR[s][p] = RDR[s][p];
			}
		}*/

		//for (size_t p = 0; p < NB_OVP_PARAMS; p++)
		//	m_OVP[p] = OVP[p];

		for (size_t p = 0; p < NB_EWD_PARAMS; p++)
			m_EWD[p] = EWD[p];

		for (size_t p = 0; p < NB_EAS_PARAMS; p++)
			m_EAS[p] = EAS[p];
	}



	//Daily development rate
	double CAprocerosLeucopodaEquations::ComputeRate(size_t s, double T)const
	{
		ASSERT(s >= 0 && s < NB_STAGES);



		static const CDevRateEquation::TDevRateEquation P_EQ[TZZ::NB_STAGES] =
		{
			//Non-linear
			CDevRateEquation::Regniere_2012,//Egg		
			CDevRateEquation::Regniere_2012,//Larva		
			CDevRateEquation::Regniere_2012,//Prepupa	
			CDevRateEquation::Regniere_2012,//Pupa		
			CDevRateEquation::Lactin2_1995,//Adult longevity	
		};

		static const double P_DEV[TZZ::NB_STAGES][6] =
		{
			//parameters
			{4.766301e-02, 7.908493e-02, 7.000000e+00, 3.200000e+01, 2.168413e-01, 5.000062e-01},//egg
			{1.267821e-02, 9.799551e-02, 5.000000e+00, 3.200000e+01, 1.000016e-01, 3.001337e+00},//Larva
			{1.211711e-01, 7.678359e-02, 7.000000e+00, 2.700000e+01, 1.748349e-01, 7.239888e-01},//Prepupa
			{6.297599e-02, 8.857761e-02, 7.000000e+00, 3.000000e+01, 2.480182e-01, 5.000042e-01},//Pupa
			{8.431380e-02, 1.484822e-01, 1.000000e+02, 6.731093e+00},//Adults
		};



		vector<double> p(begin(P_DEV[s]), end(P_DEV[s]));

		double r = max(0.0, CDevRateEquation::GetRate(P_EQ[s], p, T));

		_ASSERTE(!_isnan(r) && _finite(r) && r >= 0);

		return r;
	}

	//*****************************************************************************
	//CSBRelativeDevRate : compute individual relative development rate 


	double CAprocerosLeucopodaEquations::GetRelativeDevRate(size_t s)const
	{
		static const double SIGMA[NB_STAGES] =
		{
			//Relative devlopement Time (individual varition): sigma
			//Non-linear
			{0.102458},//Egg
			{0.157012},//Larva
			{0.202788},//Prepupae
			{0.117576},//Pupae
			{0.253597},//Adult
		};

		boost::math::lognormal_distribution<double> RDR_dist(-WBSF::Square(SIGMA[s]) / 2.0, SIGMA[s]);
		double RDR = boost::math::quantile(RDR_dist, m_randomGenerator.Randu());
		while (RDR < 0.2 || RDR>2.6)//base on individual observation
			RDR = boost::math::quantile(RDR_dist, m_randomGenerator.Randu());
		//RDR = exp(SIGMA[s] * boost::math::quantile(RDR_dist, m_randomGenerator.Randu()));//?????

		_ASSERTE(!_isnan(RDR) && _finite(RDR));

		return RDR;
	}

	//double CAprocerosLeucopodaEquations::GetCreationCDD()const
	//{
	//	boost::math::logistic_distribution<double> rldist(m_OVP[μ], m_OVP[ѕ]);

	//	double CDD = boost::math::quantile(rldist, m_randomGenerator.Rand(0.01, 0.99));
	//	//while (CDD < 0 || CDD>5000)
	//		//CDD = boost::math::quantile(rldist, m_randomGenerator.Randu());

	//	return CDD;
	//}


	/*double CAprocerosLeucopodaEquations::GetAestivalDiapauseEndCDD()const
	{
		boost::math::logistic_distribution<double> rldist(m_EAS[μ], m_EAS[ѕ]);

		double CDD = boost::math::quantile(rldist, m_randomGenerator.Randu());
		while (CDD < 0 || CDD>15000)
			CDD = boost::math::quantile(rldist, m_randomGenerator.Randu());

		return CDD;
	}*/

	//*****************************************************************************
	//survival










	double CAprocerosLeucopodaEquations::GetDailySurvivalRate(size_t s, double T)const
	{
		static const CSurvivalEquation::TSurvivalEquation S_EQ[TZZ::NB_STAGES] =
		{
			CSurvivalEquation::Survival_01,//egg
			CSurvivalEquation::Survival_01,//Larva
			CSurvivalEquation::Survival_01,//Prepupa
			CSurvivalEquation::Survival_01,//Pupa
//			CSurvivalEquation::Survival_11,//egg
//			CSurvivalEquation::Survival_11,//Larva
//			CSurvivalEquation::Survival_11,//Prepupa
//			CSurvivalEquation::Survival_11,//Pupa

			

		};

		static const double P_SURVIVAL[TZZ::NB_STAGES][6] =
		{
			{+2.821737e+00, -6.107132e-01, 1.640952e-02},//egg
			{+5.191147e+00, -1.221510e+00, 3.646663e-02},//Larva
			{-1.011495e+01, +7.023644e-02, 9.873714e-03},//Prepupa
			{+1.720485e+01, -3.396854e+00, 1.051049e-01},//Pupa
			//{4.369530e-03, 2.081998e+01, 4.554699, 1.426475},//egg
			//{4.369530e-03, 2.081998e+01, 4.554699, 1.426475},//Larva
			//{4.369530e-03, 2.081998e+01, 4.554699, 1.426475},//Prepupa
			//{4.369530e-03, 2.081998e+01, 4.554699, 1.426475},//Pupa
		};


		vector<double> p(begin(P_SURVIVAL[s]), end(P_SURVIVAL[s]));

		double sr = (s < ADULT) ? max(0.0, min(1.0, CSurvivalEquation::GetSurvival(S_EQ[s], p, T))) : 1;

		_ASSERTE(!_isnan(sr) && _finite(sr) && sr >= 0 && sr <= 1);

		return sr;
	}



	//*****************************************************************************
	//


	double CAprocerosLeucopodaEquations::GetAdultEmergingCDD()const
	{
		//boost::math::weibull_distribution<double> emerging_dist(m_EAS[ʎ], m_EAS[к]);
		boost::math::logistic_distribution<double> emerging_dist(m_EAS[μ], m_EAS[ѕ]);



		double CDD = boost::math::quantile(emerging_dist, m_randomGenerator.Randu());
		while (CDD < 0 || CDD>15000)
			CDD = boost::math::quantile(emerging_dist, m_randomGenerator.Randu());

		return CDD;
	}



	//****************************************************************************
	//

	//return individual fecundity [eggs]
	double CAprocerosLeucopodaEquations::GetFecondity()const
	{
		//After Papp 2017
		//28.2259016393443
		static const double Fo = 37;
		static const double sigma = 0.545039;

		double f = m_randomGenerator.RandUnbiasedLogNormal(log(Fo), sigma);

		while (f < 2 || f>100)
			f = m_randomGenerator.RandUnbiasedLogNormal(log(Fo), sigma);

		return f;
	}

	double CAprocerosLeucopodaEquations::GetBrood(double Fi, double T, double t, double delta_t)const
	{
		static const double to = 0.0;
		static const TDevRateEquation LAMBDA_EQ = CDevRateEquation::Room_1986;//lambda		

		//parameters
		static const double P_LAMBDA[6] = { 6.865565e-01,2.307398e-01, 1.824046e-02, 1.462013e+01 };

		vector<double> p(begin(P_LAMBDA), end(P_LAMBDA));

		double lambda = max(0.0, CDevRateEquation::GetRate(LAMBDA_EQ, p, T));
		_ASSERTE(!_isnan(lambda) && _finite(lambda));

		double brood = 0;
		if (t >= to)
			brood = round(Fi * (exp(-lambda * (t - to)) - exp(-lambda * ((t - to + delta_t)))));

		return brood;

	}

	//l: longevity [days]
	//double CAprocerosLeucopodaEquations::GetAdultLongevity(double T)const
	//{
	//	//Longevity of female
	//	//value from fecundity/longevity Zilahi-Balogh(2001)
	//	static const double P[2][2] =
	//	{
	//		{ 36.6, 2.4 },
	//		{ 30.8, 2.2}
	//	};


	//	double w = m_randomGenerator.RandNormal(P[sex][0], P[sex][1]);
	//	ASSERT(w > 3);

	//	//adjustment for attrition
	//	return w * 7 * 0.65;//active life [day]
	//}

	//T : temperature [°C]
	//day_length  : day length  [h]
	//return Time in soil [days]
	//double CAprocerosLeucopodaEquations::GetTimeInSoil(double T, double day_length)
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
	//double CAprocerosLeucopodaEquations::GetAdultAestivalDiapauseRate(double T, double day_length, double creation_day, double pupation_time)
	//{
	//	double TimeInSoil = GetTimeInSoil(T, day_length);//[day]

	//	double f = exp(m_EWD[ʎ0] + m_EWD[ʎ1] * 1.0 / (1.0 + exp(-(creation_day - m_EWD[ʎ2]) / m_EWD[ʎ3])));//day length factor
	//	double AestivalDiapauseTime = max(0.0, (TimeInSoil - pupation_time));
	//	double ADATime = f * AestivalDiapauseTime;//aestival diapause adult time

	//	double r = min(1.0, 1.0 / ADATime);
	//	return r;
	//}

	////T : temperature [°C]
	////j_day_since_jan : ordinal day since the 1 January of the emergence year (continue after December 31)
	////return: abundance (number of LN adult by normalized beating tray
	//double CAprocerosLeucopodaEquations::GetAdultAbundance(double T, size_t j_day_since_jan)
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
