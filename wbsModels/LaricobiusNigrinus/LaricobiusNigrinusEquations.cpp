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
	//parameters estimated with simulated annealing
	
	
	//NbVal=   336	Bias= 0.33689	MAE= 8.45652	RMSE=13.06280	CD= 0.86808	R²= 0.87127
	//a1 = 2.66685 
	//b1 = 15.78694
	//a2 = 0.23343 
	//b2 = 11.17501
	//a5 = 0.92619 
	//b5 = 16.21583
	//mu = 130.4701
	//s = 22.68548 
	//DDThreshold = 3.71283 
	//lam0 = 33.59438
	//lam1 = 3.06013 
	//lam2 = 3.59282 


	const double CLaricobiusNigrinusEquations::RDR[NB_STAGES][NB_RDR_PARAMS] =
	{
		//  a1      a2
		{ 2.67, 15.8 },//Egg
		{ 0.233, 11.2 },//Larvae
		{ 0.00 , 0.00 },//PrePupae
		{ 0.00 , 0.00 },//Pupae
		{ 0.926 , 16.2 },//Aestival diapause adult
		{ 0.00 , 0.00 },//Active adult

	};

	const double CLaricobiusNigrinusEquations::OVP[NB_OVP_PARAMS] = { 130.5, 22.7, 3.7 };
	const double CLaricobiusNigrinusEquations::AAD[NB_AAD_PARAMS] = { 33.6 , 3.06, 3.59};
	

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

		for (size_t p = 0; p < NB_AAD_PARAMS; p++)
			m_AAD[p] = AAD[p];
	}



//Daily development rate
	double CLaricobiusNigrinusEquations::ComputeRate(size_t s, double T)const
	{
		ASSERT(s >= 0 && s < NB_STAGES);

		double r = 0;


		static const CDevRateEquation::TDevRateEquation P_EQ[4] = { CDevRateEquation::LoganTb,CDevRateEquation::Janisch2,CDevRateEquation::Janisch2,CDevRateEquation::Logan6_1976 };
		
		static const double P_DEV[4][6] =
		{
			{7.223918e-01, 2.359667e-01, 2.008478e+01, 1.000034e+00},
			{6.624150e-02, 9.971532e+00, 1.114091e+00, 2.302115e+01},
			{4.482827e-02, 9.948521e+00, 1.123952e+00, 1.923865e+01},
			{9.975265e-03, 1.174867e-01, 2.032514e+01, 1.040053e+00},
		};

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

		if (s == EGG || s == LARVAE || s == AESTIVAL_DIAPAUSE_ADULT)
		{
			double Э = m_randomGenerator.Randu(true, true);
			rr = 1.0 - log((pow(Э, -m_RDR[s][Ϙ]) - 1.0) / (pow(0.5, -m_RDR[s][Ϙ]) - 1.0)) / m_RDR[s][к];//add -q by RSA 
			while (rr<0.4 || rr>2.5)
			{
				double Э = m_randomGenerator.Randu(true, true);
				rr = 1 - log((pow(Э, -m_RDR[s][Ϙ]) - 1) / (pow(0.5, -m_RDR[s][Ϙ]) - 1)) / m_RDR[s][к];
			}
		}

		_ASSERTE(!_isnan(rr) && _finite(rr));
		return rr;
	}

	double CLaricobiusNigrinusEquations::GetOvipositionDD()const
	{
		boost::math::logistic_distribution<double> rldist(m_OVP[μ], m_OVP[ѕ]);

		double DD = boost::math::quantile(rldist, m_randomGenerator.Randu());
		while (DD < 0 || DD>5000)
			DD = boost::math::quantile(rldist, m_randomGenerator.Randu());

		return DD;
	}



	//*****************************************************************************
	//

	//l: longevity [days]
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

	//active life[day]
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
	double CLaricobiusNigrinusEquations::GetAdultAestivalDiapauseRate(double T, double day_length)
	{
	//Residuals:
	//	1       2       3       4       5       6
	//		9.160 - 9.250 - 10.464  10.644   1.304 - 1.394

	//		Coefficients :
	//		Estimate Std.Error t value Pr(> | t | )
	//		(Intercept)-46.775     37.371 - 1.252   0.2994
	//		dl             8.933      2.345   3.809   0.0318 *
	//		T              6.491      1.149   5.651   0.0110 *
	//		-- -
	//		Signif.codes:  0 ‘***’ 0.001 ‘**’ 0.01 ‘*’ 0.05 ‘.’ 0.1 ‘ ’ 1

	//		Residual standard error : 11.49 on 3 degrees of freedom
	//		Multiple R - squared : 0.9393, Adjusted R - squared : 0.8989
	//		F - statistic : 23.22 on 2 and 3 DF, p - value : 0.01495


		//day length between 8 and 12 have low effect. So we pool 8h and 12 h together
		double nb_days_in_soil = max(10.0, min(300.0, m_AAD[ʎ0] + m_AAD[ʎ1] *(max(10.0, min(20.0, T))) + m_AAD[ʎ2] *(max(12.0, min(16.0, day_length)))));
		double r = 1.0 / nb_days_in_soil;

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
