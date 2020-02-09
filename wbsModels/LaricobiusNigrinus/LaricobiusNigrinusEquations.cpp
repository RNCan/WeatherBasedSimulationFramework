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
	//here is calibrated directly from beginning of adult emergence
	//               mu       s     Th1    Th2		 N      Bias     MAE      RMSE      CD         R²
	//EggCreation:	535.9    30.3   4.4              75    -1.184    5.60     8.195    0.957     0.960
	//Larvae:       439.5    43.8   6.0    15.9     110     0.960   12.149   20.25     0.714     0.773
	//
	//we don't be able to get good result from the beginning of adult emergence. we used January first instead 


	//Parameters for logistic distribution
	//here is calibrated directly from January first with 
	//				  mu     s    ThLo    ThHi		 N    Bias    MAE      RMSE      CD        R²
	//EggCreation:	229.3   50.5   2.4    17.5      75   -0.888   5.656   10.80     0.922    0.929
	//Larvae:       198.5   33.9   5.4    15.0     110	 -0.732   4.543    8.06     0.962    0.964


	//parameters estimated with simulated annealing
	//individual creation, egg and larval relative development rate
	//with non-linear version
	
	//Egg creation and larval development 

	//NbVal = 316	Bias = 1.57809	MAE = 5.93900	RMSE = 9.69681	CD = 0.92217	R² = 0.92628 (2 ways)
	//a6 = 0.63858 (egg development factor correction)
	//b6 = 1.59243 (larvae development factor correction)
	//mu = 226.367
	//s = 49.80759
	//Th1 = 1.6636
	//Th2 = 20.685
	
	//NbVal = 173	Bias = -0.52445	MAE = 4.80838	RMSE = 8.15235	CD = 0.96166	R² = 0.96270 (1 way)
	//a6 = 0.97207 {   0.97194, 0.97222}	VM = { 0.00008,   0.00017 }
	//b6 = 1.04341 {   1.04316, 1.04367}	VM = { 0.00012,   0.00032 }
	//mu = 235.45986 { 235.45737, 235.46248}	VM = { 0.00095,   0.00254 }
	//s = 49.45781 {  49.45488, 49.45994}	VM = { 0.00049,   0.00222 }
	//Th1 = 1.73649 {   1.73633, 1.73666}	VM = { 0.00005,   0.00020 }
	//Th2 = 20.28993 {  20.28083, 20.29647}	VM = { 0.00316,   0.00948 }

	
	//Adult emergence from soil (with BalcksburgLab)
	//NbVal = 143	Bias = -0.84353	MAE = 5.94677	RMSE = 8.52876	CD = 0.93151	R² = 0.93617 (2 ways)
	//mu_ADE = 1.97941
	//s_ADE = 252.2679
	//lam0 = 268.17112
	//lam1 = 14.12983 
	//lam2 = -1458.296
	//lam_a = 19.17623
	//lam_b = 24.82122

	//NbVal = 82	Bias = 0.78171	MAE = 4.18902	RMSE = 5.58778	CD = 0.97990	R² = 0.98032 (1 way)
	//mu_ADE = 2.12744
	//s_ADE = 230.4847
	//lam0 = 269.21088
	//lam1 = 13.07374 
	//lam2 = -1585.933
	//lam_a = 15.41537
	//lam_b = 23.02092


	//parameters estimated with simulated annealing
	//With linear version
	//NbVal = 353	Bias = 0.70292	MAE = 8.84120	RMSE = 13.77512	CD = 0.84453	R² = 0.85222
	//a1 = 9.10753
	//b1 = 6.04934
	//a2 = 2.17652
	//b2 = 23.6698
	//mu = 133.020
	//s = 19.38961
	//Th= 3.97096





	const double CLaricobiusNigrinusEquations::RDR[NB_STAGES][NB_RDR_PARAMS] =
	{
		//  a1      a2
		//{ 2.67, 15.8 },//Egg
		{ 9.11, 6.05 },//Egg
		//{ 0.233, 11.2 },//Larvae
		{ 2.18, 23.67 },//Larvae
		{ 0.00, 0.00 },//PrePupae
		{ 0.00, 0.00 },//Pupae
		{ 0.00, 0.00 },//Aestival diapause adult
		{ 0.00, 0.00 },//Active adult
	};



	//	const double CLaricobiusNigrinusEquations::OVP[NB_OVP_PARAMS] = { 130.5, 22.7, 3.7 };
		//const double CLaricobiusNigrinusEquations::OVP[NB_OVP_PARAMS] = { 133.0, 19.4, 4.0 };
		//const double CLaricobiusNigrinusEquations::ADE[NB_ADE_PARAMS] = { 1327.3,95.0,-6.0,26.0,10.7,0.7,167.0,9.47 };
		//const double CLaricobiusNigrinusEquations::ADE[NB_ADE_PARAMS] = { 1353.4, 109.8, -4.5, 23.3, 11.0, 0.7, 167.0, 9.66 };


	const double CLaricobiusNigrinusEquations::OVP[NB_OVP_PARAMS] = { 997.7, 66.4, 0.9 , 50.0};
	const double CLaricobiusNigrinusEquations::ADE[NB_ADE_PARAMS] = { 1.984, 225.1,266.5,18.81,-786.2,20.0,11.1,18.1 };



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
			CDevRateEquation::HilberLoganIII,
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
		boost::math::logistic_distribution<double> rldist(m_OVP[μ], m_OVP[ѕ]);

		double CDD = boost::math::quantile(rldist, m_randomGenerator.Rand(0.01, 0.99));
		//while (CDD < 0 || CDD>5000)
			//CDD = boost::math::quantile(rldist, m_randomGenerator.Randu());

		return CDD;
	}


	double CLaricobiusNigrinusEquations::GetAestivalDiapauseEndCDD()const
	{
		boost::math::logistic_distribution<double> rldist(m_ADE[μ], m_ADE[ѕ]);

		double CDD = boost::math::quantile(rldist, m_randomGenerator.Randu());
		while (CDD < 0 || CDD>15000)
			CDD = boost::math::quantile(rldist, m_randomGenerator.Randu());

		return CDD;
	}


	//*****************************************************************************
	//

	double CLaricobiusNigrinusEquations::GetAdultEmergingCDD()const
	{
		boost::math::weibull_distribution<double> emerging_dist(m_ADE[μ], m_ADE[ѕ]);


		double CDD = boost::math::quantile(emerging_dist, m_randomGenerator.Randu());
		while (CDD < 0 || CDD>15000)
			CDD = boost::math::quantile(emerging_dist, m_randomGenerator.Randu());

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


	double CLaricobiusNigrinusEquations::GetAestivalDiapauseEndTavg30()const
	{
		boost::math::logistic_distribution<double> rldist(m_ADE[μ], m_ADE[ѕ]);

		double Tavg30 = boost::math::quantile(rldist, m_randomGenerator.Randu());
		while (Tavg30 < -10 || Tavg30>25)
			Tavg30 = boost::math::quantile(rldist, m_randomGenerator.Randu());

		return Tavg30;
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
