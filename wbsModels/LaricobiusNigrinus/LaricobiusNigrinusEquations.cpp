//*****************************************************************************
// File: LaricobiusNigrinusEquations.h
//
// Class: CLaricobiusNigrinusEquations
//          
//
// Description: 
//				stage development rates, relative development rates
//				stage development rates use optimization table lookup
//
//*****************************************************************************
// 10/03/2019   Rémi Saint-Amant    Creation 
//*****************************************************************************
#include "LaricobiusNigrinusEquations.h"
#include <boost/math/distributions.hpp>
#include <boost/math/distributions/logistic.hpp>
#include "ModelBase/DevRateEquation.h"

using namespace WBSF;
using namespace LNF;
using namespace std;

namespace WBSF
{

	//Minimum developmental threshold temperatures were estimated at
	//5.4C for eggs, 3.2C for larvae, 2.9C for prepupae, and 3.1C for pupae.Median development times
	//for eggs, larvae, prepupae, and pupae were 59.5, 208.3, 217.4, and 212.8 degree - days(DD) above
	//minimum developmental temperatures, respectively.


	//Parameters for logistic distribution
	//WARNING: logistic describe cumul of egg abundance (cumul under the curve)
	//here is calibrated directly from January first with 
	//				  mu     s     ThLo   ThHi	   N     Bias     MAE      RMSE     CD       R²
	//EggCreation:	327.2   60.9   0.8    21.7     71   -0.720   3.501    5.232    0.982    0.983
	//Larvae:       117.0   20.8   6.6    11.9    103   -0.911   5.011    8.385    0.958    0.961

	//parameters estimated with simulated annealing
	//individual creation of egg

	//parameters with egg and larval observation ( non-linear development equation)
	//WARNING: logistic describe cumul of egg creation
	//NbVal = 174	Bias = -0.31065	MAE = 5.09708	RMSE = 8.51067	CD = 0.95783	R² = 0.95910
	//mu = 220.3
	//s = 47.5 
	//Th1 = 2.1
	//Th2 = 20.2

	//Egg creation and larval development (with correction)
	//NbVal = 173	Bias = -0.99017	MAE = 5.03757	RMSE = 8.31326	CD = 0.95879	R² = 0.96159
	//a1 = 0.813
	//a2 = 1.197
	//mu = 199.4
	//s = 47.86
	//Th1 = 2.4
	//Th2 = 18.2

	//Beginning of adult emergence from soil (with BalcksburgLab)
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



	//parameters estimated with simulated annealing
	//With linear version





	const double CLaricobiusNigrinusEquations::RDR[NB_STAGES][NB_RDR_PARAMS] =
	{
		//  a1      a2
		{ 1.00, 0.00 },//Egg (correction factor)
		{ 1.00, 0.00 },//Larvae (correction factor)
		{ 0.00, 0.00 },//PrePupae
		{ 0.00, 0.00 },//Pupae
		{ 0.00, 0.00 },//Aestival diapause adult
		{ 0.00, 0.00 },//Active adult
	};

	
	
	const double CLaricobiusNigrinusEquations::OVP[NB_OVP_PARAMS] = { 220.3, 47.5, 2.1, 20.2 };//logistic distribution
	const double CLaricobiusNigrinusEquations::ADE[NB_ADE_PARAMS] = { 121,212,-294.5,105.8,34.8,20 };//logistic distribution
	const double CLaricobiusNigrinusEquations::EAS[NB_EAS_PARAMS] = { 1157.8,125.0,-2.5 };//logistic distribution











	CLaricobiusNigrinusEquations::CLaricobiusNigrinusEquations(const CRandomGenerator& RG) :
		CEquationTableLookup(RG, AESTIVAL_DIAPAUSE_ADULT, -10, 30, 0.25)
	{

		for (size_t s = 0; s < NB_STAGES; s++)
		{
			for (size_t p = 0; p < NB_RDR_PARAMS; p++)
			{
				m_RDR[s][p] = RDR[s][p];
			}
		}

		for (size_t p = 0; p < NB_OVP_PARAMS; p++)
			m_OVP[p] = OVP[p];

		for (size_t p = 0; p < NB_ADE_PARAMS; p++)
			m_ADE[p] = ADE[p];

		for (size_t p = 0; p < NB_EAS_PARAMS; p++)
			m_EAS[p] = m_EAS[p];
	}



	//Daily development rate
	double CLaricobiusNigrinusEquations::ComputeRate(size_t s, double T)const
	{
		ASSERT(s >= 0 && s < NB_STAGES);

		double r = 0;

#if 0
		static const CDevRateEquation::TDevRateEquation P_EQ[4] =
		{
			CDevRateEquation::Poly1,
			CDevRateEquation::Poly1,
			CDevRateEquation::Poly1,
			CDevRateEquation::Poly1
		};

		static const double P_DEV[4][6] =
		{
			{-0.0907, 0.0165,0,0,0,0},
			{-0.0151, 0.0048,0,0,0,0},
			{-0.0132, 0.0047,0,0,0,0},
			{-0.0144, 0.0047,0,0,0,0}
		};
#else
		static const CDevRateEquation::TDevRateEquation P_EQ[4] =
		{
			CDevRateEquation::Logan6_1976,
			CDevRateEquation::Ratkowsky_1983,
			CDevRateEquation::HilbertLoganIII,
			CDevRateEquation::Bieri_1983,
		};

		static const double P_DEV[4][6] =
		{
			{1.112472e-02, 1.742345e-01, 2.209286e+01, 1.000115e+00},
			{1.597533e-02, 8.877193e-01, 3.528062e-05, 2.508830e+01},
			{1.314744e+02, 6.659223e+02, 4.997764e+01, 3.691572e+00},
			{4.714239e-03, 7.562195e+01, 2.942047e+00, 3.080090e+01},
		};
#endif


		vector<double> p(begin(P_DEV[s]), end(P_DEV[s]));

		switch (s)
		{
		case EGG:
		case LARVAE:
		case PREPUPAE:
		case PUPAE:	r = max(0.0, CDevRateEquation::GetRate(P_EQ[s], p, T)); break;
		default: _ASSERTE(false);
		}

		_ASSERTE(!_isnan(r) && _finite(r) && r >= 0);

		return r;
	}

	//*****************************************************************************
	//CSBRelativeDevRate : compute individual relative development rate 


	double CLaricobiusNigrinusEquations::GetRelativeDevRate(size_t s)const
	{
		double rr = 1;

		//if (s == EGG || s == LARVAE /*|| s == AESTIVAL_DIAPAUSE_ADULT*/)
		//{
		//	double Э = m_randomGenerator.Randu(true, true);
		//	rr = 1.0 - log((pow(Э, -m_RDR[s][Ϙ]) - 1.0) / (pow(0.5, -m_RDR[s][Ϙ]) - 1.0)) / m_RDR[s][к];//add -q by RSA 
		//	while (rr<0.4 || rr>2.5)
		//	{y65
		//		double Э = m_randomGenerator.Randu(true, true);
		//		rr = 1 - log((pow(Э, -m_RDR[s][Ϙ]) - 1) / (pow(0.5, -m_RDR[s][Ϙ]) - 1)) / m_RDR[s][к];
		//	}
		//}

		//if (s == EGG || s == LARVAE )
		//{
		//	ASSERT(m_RDR[s][μ] < 0.5);
		//	boost::math::logistic_distribution<double> rldist(0, m_RDR[s][ѕ]);
		//	double rr = exp(boost::math::quantile(rldist, m_randomGenerator.Rand(m_RDR[s][μ],1- m_RDR[s][μ])) );
		//	rr = max(0.5, min(2.0, rr));
		//	//while (rr < 1.0/3.0 || rr>3.0)
		//		//rr = exp(m_RDR[s][μ] * boost::math::quantile(rldist, m_randomGenerator.Randu()));
		//}

		_ASSERTE(!_isnan(rr) && _finite(rr));
		return rr;
	}

	double CLaricobiusNigrinusEquations::GetCreationCDD()const
	{
		boost::math::logistic_distribution<double> creation_dist(m_OVP[μ], m_OVP[ѕ]);

		double CDD = boost::math::quantile(creation_dist, m_randomGenerator.Rand(0.001, 0.999));
		//while (CDD < 0 || CDD>5000)
			//CDD = boost::math::quantile(rldist, m_randomGenerator.Randu());

		return CDD;
	}


	//double CLaricobiusNigrinusEquations::GetAestivalDiapauseEndCDD()const
	//{
	//	boost::math::logistic_distribution<double> ADE_dist(m_EAS[μ], m_EAS[ѕ]);

	//	double CDD = boost::math::quantile(ADE_dist, m_randomGenerator.Rand(0.01, 0.99));
	//	//double CDD = boost::math::quantile(rldist, m_randomGenerator.Randu());
	//	//while (CDD < 0 || CDD>15000)
	//		//CDD = boost::math::quantile(rldist, m_randomGenerator.Randu());

	//	return CDD;
	//}


	//*****************************************************************************
	//


	double CLaricobiusNigrinusEquations::GetAdultEmergingCDD()const
	{
		//boost::math::weibull_distribution<double> emerging_dist(m_EAS[μ], m_EAS[ѕ]);
		boost::math::logistic_distribution<double> emerging_dist(m_EAS[μ], m_EAS[ѕ]);

		double CDD = boost::math::quantile(emerging_dist, m_randomGenerator.Rand(0.001, 0.999));
		//double CDD = boost::math::quantile(emerging_dist, m_randomGenerator.Randu());
		//while (CDD < 0 || CDD>15000)
			//CDD = boost::math::quantile(emerging_dist, m_randomGenerator.Randu());

		return CDD;
	}



	//****************************************************************************
	//

	//l: longevity [days]
	//return fecundity [eggs]
	double CLaricobiusNigrinusEquations::GetFecondity(double l)const
	{
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
	double CLaricobiusNigrinusEquations::GetAdultLongevity(size_t sex)const
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
	double CLaricobiusNigrinusEquations::GetTimeInSoil(double T, double day_length)
	{
		//data From Lamb 2005
		//NbVal = 8	Bias = 0.00263	MAE = 0.95222	RMSE = 1.25691	CD = 0.99785	R² = 0.99786
		//lam0 = 15.8101
		//lam1 = 2.50857
		//lam2 = 6.64395
		//lam3 = 7.81911
		//lam_a = 0.1634
		//lam_b = 0.2648

		static const double AADD[6] = { 15.8, 2.51, 6.64, 7.82, -0.163, 0.265 };
		double DiS = 120.0 + (215.0 - 120.0) * 1.0 / (1.0 + exp(-(T - AADD[0]) / AADD[1]));//day in soil base on temperature
		double DlF = exp(AADD[4] + AADD[5] * 1.0 / (1.0 + exp(-(day_length - AADD[2]) / AADD[3])));//day length factor

		double nb_days_in_soil = DiS * DlF;
		return nb_days_in_soil;
	}



	//T : temperature [°C]
	//day_length  : day length  [h]
	//pupationTime : time to the soil before becoming adult (to complete pre-pupation and pupation) [days]
	//return aestival diapause rate [day-1]
	double CLaricobiusNigrinusEquations::GetAdultAestivalDiapauseRate(double T, double day_length, double creation_day, double pupation_time)
	{
		double TimeInSoil = GetTimeInSoil(T, day_length);//[day]

		double f = exp(m_ADE[ʎ0] + m_ADE[ʎ1] * 1.0 / (1.0 + exp(-(creation_day - m_ADE[ʎ2]) / m_ADE[ʎ3])));//day length factor
		double AestivalDiapauseTime = max(0.0, (TimeInSoil - pupation_time));
		double ADATime = f * AestivalDiapauseTime;//aestival diapause adult time

		double r = min(1.0, 1.0 / ADATime);
		return r;
	}

	//T : temperature [°C]
	//j_day_since_jan : ordinal day since the 1 January of the emergence year (continue after December 31)
	//return: abundance (number of LN adult by normalized beating tray
	double CLaricobiusNigrinusEquations::GetAdultAbundance(double T, size_t j_day_since_jan)
	{

		//Coefficients:
		//	Estimate Std.Error t value Pr(> | t | )
		//		(Intercept)-2.815284   1.084082 - 2.597  0.01646 *
		//		T            0.088449   0.041528   2.130  0.04461 *
		//		jday         0.008683   0.002774   3.130  0.00487 **
		//		-- -
		//		Signif.codes:  0 ‘***’ 0.001 ‘**’ 0.01 ‘*’ 0.05 ‘.’ 0.1 ‘ ’ 1
		//
		//		Residual standard error : 0.4831 on 22 degrees of freedom
		//		Multiple R - squared : 0.4143, Adjusted R - squared : 0.361
		//		F - statistic : 7.78 on 2 and 22 DF, p - value : 0.002784

		static const double P[3] = { -2.815284, 0.088449, 0.008683 };
		return max(0.0, P[0] + P[1] * T + P[2] * j_day_since_jan);
	}


}
