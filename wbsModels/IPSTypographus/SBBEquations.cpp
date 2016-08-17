//*****************************************************************************
// File: SBBDevelopment.h
//
// Class: CSBBDevelopment
//          
//
// Descrition: CSBBTableLookup compute daily devlopement rate of IPS Typographus (spruce bark beetle) 
// Reference:
//*****************************************************************************
// 22/01/2016	Rémi Saint-Amant	Using Weather-Based Simulation Framework (WBSF)
// 25/09/2013   Rémi Saint-Amant    Creation 
//*****************************************************************************

#include <boost\math\distributions.hpp>
#include "basic/UtilMath.h"
#include "ModelBase/IndividualBase.h"
#include "SBBEquations.h"

using namespace std;


namespace WBSF
{
	double plnorm(double x, double logmean, double logsd, bool lower_tail = true, bool log_p = false)
	{
		ASSERT(logsd > 0);

		boost::math::lognormal_distribution<double> uniformLogNormal(logmean, logsd);

		double p = 0;
		if (x > 0)
		{
			//p = boost::math::quantile(uniformLogNormal, log_p?x:log(x) );
			p = boost::math::cdf(uniformLogNormal, x);
			if (log_p)
				p = log(p);
		}


		return lower_tail ? p : 1 - p;
	}

	double qlnorm(double x, double logmean, double logsd, bool lower_tail = true, bool log_p = false)
	{
		ASSERT(logsd > 0);

		boost::math::lognormal_distribution<double> uniformLogNormal(logmean, logsd);

		x = lower_tail ? x : 1 - x;

		double q = 0;
		if (x > 0)
			q = boost::math::quantile(uniformLogNormal, log_p ? x : log(x));

		return q;
	}


	//*****************************************************************************
	//CSBBDevelopment class 


	//development rate parameters (4 stage, 6 parameters)
	const double CSBBTableLookup::DEFAULT_P[NB_STAGES][NB_PARAMETER] =
	{
		// p0                  	=   0.07595  
		//	p1                  	=  11.89790  
		//	p2                  	=   0.45628  
		//	p3                  	=   7.50983  
		//	p4                  	=  39.13573  
		//	k0                  	=   0.07755  
		//	k1                  	=  12.14752  
		//	k2                  	=   0.28739  
		//	k3                  	=   6.52692  
		//	k4                  	=  38.90885  
		//	s0                  	=   0.07836  
		//	s1                  	=  12.27255  
		//	s2                  	=   0.19472  
		//	s3                  	=   6.70386  
		//	s4                  	=  38.91888  
		// 
		//p0                  	=   0.07595  
		//p1                  	=  11.89790  
		//p2                  	=   0.45628  
		//p3                  	=   8.22885  
		//p4                  	=  38.97032  
		//k0                  	=   0.07755  
		//k1                  	=  12.14752  
		//k2                  	=   0.28739  
		//k3                  	=   6.13075  
		//k4                  	=  39.28919  
		//s0                  	=   0.07836  
		//s1                  	=  12.27244  
		//s2                  	=   0.19471  
		//s3                  	=   6.53564  
		//s4                  	=  39.29154  

		//0.07537, 12.76997, 0.01948,  
		//0.07812, 12.87708, 0.25011,  
		//0.08010, 12.15577, 0.01721,  
		//0.07595,11.89752, 0.45633, 8.63081,39.06797,
		//0.07755,12.14743, 0.28748, 8.63125,39.06804,
		//0.07836,12.27237, 0.19450, 8.63060,39.06900,



		//	  ALPHA ,     BETA ,      GAMMA      DTL,     TO ,    DTU,    TMAX	  K
		{ 0.12254, 8.08243, 0.09805, 7.9, 32.0, 39.4, 40.1, 51.8 },//Egg
		//{0.07764,	12.16095,	0.27765,	 8.7,	29.5,	39.1,	42.0,	 68.1},//Larvae L1: estimate from Larval and ratio 0.203 (Annila 1969)
		{ 0.07595, 11.89757, 0.45627, 8.7, 29.5, 39.1, 42.0, 22.7 },//Larvae L1: estimate from Larval and ratio 0.203 (Annila 1969)
		{ 0.07755, 12.14742, 0.28738, 8.7, 29.5, 39.1, 42.0, 22.7 },//Larvae L2: estimate from Larval and ratio 0.322 (Annila 1969)
		{ 0.07836, 12.27267, 0.19468, 8.7, 29.5, 39.1, 42.0, 22.7 },//Larvae L3: estimate from Larval and ratio 0.475 (Annila 1969)
		{ 0.14977, 6.65546, 0.02341, 1.6, 33.3, 39.9, 40.0, 57.7 },//Pupae
		{ 0.07373, 13.22339, 0.09997, 6.7, 27.7, 39.5, 41.1, 238.5 },//Teneral adult (Feeding): get from paper and simulated Annealing
		{ -999.00, 999.0000, -0.0100, 5.0, 24.0, 30.0, 40.0, 0.0 } //Adult: How long is the live of adult?
	};


	double CSBBTableLookup::GetTo(int s)
	{
		const double alpha = DEFAULT_P[s][P_ALPHA];
		const double beta = DEFAULT_P[s][P_BETA];
		const double Tmax = DEFAULT_P[s][P_TMAX];

		return Tmax - (beta*log(alpha*beta) / (alpha*beta - 1));
	}



	CSBBTableLookup::CSBBTableLookup(const CRandomGenerator& RG) :
		CEquationTableLookup(RG, NB_STAGES, 0, 45, 0.25)
	{
		for (int i = 0; i < NB_STAGES; i++)
			for (int j = 0; j < NB_PARAMETER; j++)
				m_p[i][j] = DEFAULT_P[i][j];

		Init();
	}


	double CSBBTableLookup::ComputeRate(size_t s, double T)const
	{
		double Rt = 0;

		if (T >= m_p[s][P_DTL] && T <= m_p[s][P_DTU])
		{
			//Logan-Lactin equation
			//double rho25 = 1;//m_rho25Factor[s]*m_p[s][0];
			const double alpha = m_p[s][P_ALPHA];
			const double beta = m_p[s][P_BETA];
			const double gamma = m_p[s][P_GAMMA];
			const double Tmax = m_p[s][P_TMAX];
			//const double K = m_p[s][P_K];


			double a1 = exp(alpha*T);
			double a2 = exp(alpha*Tmax - (Tmax - T) / beta);
			Rt = a1 - a2 - gamma;

			//if(s>=L1 && s<=L3)
			//Rt*=3;//Take only 33% in each stages
		}

		return min(1.0, max(0.0, Rt)); //This is daily rate, if time step smaller, must be multiplied by time step...
	}

	double CSBBTableLookup::GetDayLengthFactor(int s, double t, double dayLength)
	{

		double w = 1;
		if (s == EGG || (s >= L3 && s <= TENERAL_ADULT))
		{
			//NbVal=    13	Bias=-1.39942	MAE= 3.83058	RMSE= 4.75118	CD= 0.94182	R²= 0.95667
			static const double TH_LOW = 8.781;
			static const double TH_UP = 23.0952;
			static const double PHI = 14.2372;
			static const double MU = 12.1937;
			static const double K = 0.3793;


			//Finding DI50 using day length
			double x1 = PHI*(dayLength - MU);
			double TI50 = TH_LOW + (TH_UP - TH_LOW)*(1 - 1 / (1 + exp(-x1)));

			//shifting t using DI50
			double x2 = K*(t - TI50);
			w = 1 / (1 + exp(-x2));
		}

		return w;


		//estimate from fig 1.
		//NbVal=   9	Bias=-2.39615	MAE= 3.23613	RMSE= 6.80948	CD= 0.97096	R²= 0.97801
		//static const double LAMDA = 0.48889;//8-22
		//static const double K = 21.17935;//8-22

		//

		////NbVal=     9	Bias=-2.35059	MAE= 3.16727	RMSE= 6.77381	CD= 0.97126	R²= 0.97807
		//static const double LAMDA = 0.48455;//10-20
		//static const double K = 14.67212;//10-20
		//
		////adjusting lamda to the current temperature
		////NbVal=    13	Bias=-0.04096	MAE= 6.52251	RMSE= 8.40819	CD= 0.81778	R²= 0.89384
		//double x1 = -37.53077*(Max(0, Min(1, (t-5)/(35-5)))-0.5);
		//double lamda = Max(0.01, Min(10, (LAMDA + 0.56007*(1/(1+exp(-x1)) - 0.5))));
		////double x2 = Max(0, Min(1, (dayLength-12)/(18-12)) );
		//double x2 = Max(0, Min(1, (dayLength-10)/(20-10)) );
		////double x2 = Max(0, Min(1, (dayLength-8)/(22-8)) );
		//double w = 0.56164 +  0.37089*(1-exp(-pow(x2/lamda, K)));

		////N=      1187	T=  0.01960	F=919.07001

		//x0                  	=   0.56164  
		//x1                  	=   0.37089  
		//k                   	= -37.53077  
		//lamda               	=   0.56007  

		//return w;
	}

	//double CSBBTableLookup::GetRate(int s, double t)const
	//{
	//	return Max(0, Min(1, CBugDevelopmentTable::GetRate(s, t)*GetDayLenghtFactor( t, dayLength)));
	//}

	const double CSBBTableLookup::V[NB_STAGES] =
		// Egg        L1		  L2		  L3		Pupae	maturation,		 Adult 
	{ 0.1711, 0.1590, 0.1590, 0.1590, 0.1777, 0.3162, 0.1 };


	double CSBBTableLookup::GetRelativeRate(const CRandomGenerator& RG, int s, int sex)
	{
		_ASSERTE(s >= 0 && s < NB_STAGES);

		double r = RG.RandLogNormal(0, V[s]);
		while (r<0.4 || r>2.5)
			r = RG.RandLogNormal(0, V[s]);


		//modulate relative rate by sex. Male 
		int sexFactor = (sex == MALE) ? 1 : -1;
		double Rsex = 1 + sexFactor*(0.5 + RG.Randu()) / 51;


		return r*Rsex;
	}

	//***********************************************************************************

	/*const double CSBBAttrition::p[NB_STAGES][3] =
	{
	0.000,  0.00000,  0.00000,//Egg
	0.000,  0.00000,  0.00000,//OW
	-6.3464,  0.10440, -0.00091,//L2
	-5.3884,  0.05502,  0.00000,//L3
	-5.6125,  0.06048,  0.00000,//L4
	-7.2000,  0.00618,  0.00288,//L5
	-2.0299, -0.85730,  0.02766,//L6
	8.3397, -1.5786, 0.04145,//PUPAE
	0.0000, 0.00000, 0.00000 //Adults (not used, nonsensical)
	};

	double CSBBAttrition::GetRate(int s, double Tin)
	{
	_ASSERTE( s>=0 && s<NB_STAGES);

	double T = max(0.,Tin);
	double att=1/(1+exp(p[s][0]+p[s][1]*T+p[s][2]*T*T));

	return att;
	};
	*/
	//***********************************************************************************
	//Fecondity parameters (6 parameters)
	const double CSBBOviposition::DEFAULT_P[NB_EQUATIONS][NB_PARAMETER] =
	{
		//	  ALPHA ,     BETA ,      GAMMA      DTL,     TO ,    DTU,    TMAX
		{ 0.11402, 8.79104, -0.11363, 7.9, 32.2, 33.7, 18.20 },//Ovipositind Adult (including pre-oviposition): get from simulated annealing (CD= 0.91349	R²= 0.91349)
		{ 0.07663, 2.63793, 1.99997, 7.9, 29.1, 33.7, 34.38 } //Oviposition rate (eggs/day). get from simulated annealing
	};



	double CSBBOviposition::GetRate(int e, double T)
	{
		ASSERT(e >= 0 && e<NB_EQUATIONS);

		double Or = 0.0;

		if (T>DEFAULT_P[e][P_DTL] && T < DEFAULT_P[e][P_DTU])
		{
			const double alpha = DEFAULT_P[e][P_ALPHA];
			const double beta = DEFAULT_P[e][P_BETA];
			const double gamma = DEFAULT_P[e][P_GAMMA];
			const double Tmax = DEFAULT_P[e][P_TMAX];

			double a1 = exp(alpha*T);
			double a2 = exp(alpha*Tmax - (Tmax - T) / beta);

			//Oviposition rate (eggs/day)
			Or = max(0.0, a1 - a2 - gamma);
		}

		return Or;
	}

	const double CSBBOviposition::V[NB_EQUATIONS] = { 0.12831, 0.0981 };
	double CSBBOviposition::GetRelativeRate(const CRandomGenerator& RG, int e)
	{
		ASSERT(e >= 0 && e < NB_EQUATIONS);

		double r = RG.RandLogNormal(0, V[e]);
		while (r<0.4 || r>2.5)
			r = RG.RandLogNormal(0, V[e]);

		return r;
	}

	double CSBBOviposition::GetRelativeQuartile(double RR, int e)
	{
		ASSERT(e >= 0 && e < NB_EQUATIONS);
		return plnorm(RR, 0 - Square(V[e]) / 2.0, V[e]);
	}

	//
	const double CDiapause::MTS_CLASS_20[33][2] =
		//DayLength(h)	Diapause(%) at constant temperatrue of 20°C
	{
		{ 13.2, 100.00 },
		{ 13.3, 99.97 },
		{ 13.4, 99.40 },
		{ 13.5, 98.22 },
		{ 13.6, 96.73 },
		{ 13.7, 95.00 },
		{ 13.8, 92.81 },
		{ 13.9, 90.18 },
		{ 14.0, 87.21 },
		{ 14.1, 83.61 },
		{ 14.2, 79.67 },
		{ 14.3, 75.03 },
		{ 14.4, 69.71 },
		{ 14.5, 64.05 },
		{ 14.6, 58.15 },
		{ 14.7, 52.43 },
		{ 14.8, 46.25 },
		{ 14.9, 40.14 },
		{ 15.0, 34.52 },
		{ 15.1, 29.02 },
		{ 15.2, 24.09 },
		{ 15.3, 19.76 },
		{ 15.4, 15.96 },
		{ 15.5, 12.78 },
		{ 15.6, 9.88 },
		{ 15.7, 7.46 },
		{ 15.8, 5.54 },
		{ 15.9, 3.82 },
		{ 16.0, 2.51 },
		{ 16.1, 1.54 },
		{ 16.2, 0.60 },
		{ 16.3, 0.02 },
		{ 16.4, 0.00 }
	};


	//Exposure period			 L:D		T		mean T	Number of insects	Diapause  Swarming 
	//														(% dead)*					time in days
	//
	//No exposure				12:12		20				 94 (28)			No		50.9 ± 0.2 a
	//Egg – L2 (16 days)		12:12		20				326 (16)			No		55.9 ± 0.2 b
	//L2 – L3 (10 days)			12:12		20				220 (16)			No		56.8 ± 0.2 b
	//L3 (10 days)				12:12		20				280 (16)			No		64.3 ± 0.3 c
	//Pupa (6 days)				12:12		20				335 (22)			No		69.7 ± 0.1 c
	//Egg – L3 (25 days)		12:12		20				335 (20)			No		72.7 ± 0.1 d
	//L3 – Pupa (11 days)		12:12		20				169 (23)			Yes		No swarming
	//Pupa – Adult (21 days)	12:12		20				214 (33)			Yes		No swarming
	//L3 – Adult (15 days)		12:12		20				158 (21)			Yes		No swarming
	//Egg - Adult				12:12		22		22.0	 82 (26)			Yes		No swarming
	//Egg - Adult				12:12		23		23.0	265 (24)			No		52.3 ± 0.1 b
	//Egg - Adult				12:12		25		25.0	 92 (29)			No		42.0 ± 0.2 a
	//Egg - Adult				13:11	   26:13	20.0	456 (23)			No		49.1 ± 0.4 b
	//Egg - Adult				14:10	   26:6		19.3	117 (64)			Yes		No swarming
	//Egg - Adult				16:8	   26:6		19.3	182 (10)			No		70.1 ± 0.3 c
	//Egg - Adult				17:7	   20:6		15.9	336 (20)			No		120.5 ± 0.6 d
	//Egg - Adult				17:7		25		25.0	 64 (40)			No		41.7 ± 0.2 a
	//Egg - Adult				18:6		20		20.0	 94 (28)			No		50.9 ± 0.2 b

	//*Death rate was assessed when the logs were dissected on day 120. Most of the dead insects were lightly tanned adults.
	//Values (mean ± SD) followed by different letters differ significantly (one-way anova, P < 0.001).
	//*Death rate was assessed when the logs were dissected on day 100. All dead insects were adults,
	//most of them not fully tanned.
	//Values (mean ± SD) followed by different letters differ significantly (one-way anova, P < 0.001).

}