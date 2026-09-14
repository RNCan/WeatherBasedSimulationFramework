//*****************************************************************************
// File: WSBDevelopment.h
//
// Class: CWSBDevelopment
//          
//
// Description: the CWSBDevelopment can compute daily western spruce budworm development rate
//             CWSBEquations is an optimization table lookup
//*****************************************************************************
// 12/07/2022   Rémi Saint-Amant	Parameters correction HA L3 (9738 instead of 97380)
// 21/01/2016   Rémi Saint-Amant	Update with BioSIM 11.0
// 12/10/2012   Rémi Saint-Amant    Update with new template
// 28/01/2011	Rémi Saint-Amant    Add Attrition 
// 23/01/2009   Rémi Saint-Amant    Creation from Jacques code
//*****************************************************************************

#include "Basic/UtilMath.h"
#include "WSBEquations.h"




using namespace std;

namespace WBSF
{
	//*****************************************************************************
	//CWSBDevelopment class 


	//development rate parameters (9 stages, 6 parameters)
	const double CWSBEquations::DEFAULT_P[NB_STAGES][6] =
	{//Revised 2011-08-06from SAS output (JR)	
		//rho25	HA		HL		TL		HH		TH
		0.158, 11443, -59715., +283.900, 99958, 308.2,	//Egg
		0.217, 18186, -58752., +276.700, 99941, 300.7,	//L2o
		0.173, 12707, -50000., +277.400, 98106, 307.8,	//L2
		0.263,  9738, -100000, +285.100, 91262, 306.7,	//L3, Bug corrected by RSA, 2022-07-12 (9738 instead of 97380)
		0.330, 13271, -99659., +283.700, 91301, 306.2,	//L4
		0.303, 13031, -99659., +273.000, 91292, 305.5,	//L5
		0.187, 12651, -99659., +274.000, 91276, 304.0,	//L6
		0.288, 25810, -23000., +296.500, 59590, 306.4,	//Pupa
		120.0, -13.2, +0.5669, -0.00854, 0.000, 0.000	//Adult longevity (3RD DEGREE POLYNOMIAL ON TIME)
	};


	//L2 adjustment (final model)
	const double CWSBEquations::RHO25_FACTOR[NB_STAGES] = { 1, 1, 2.62, 1, 1, 1, 1, 1, 1 };

	CWSBEquations::CWSBEquations(const CRandomGenerator& RG) :
		CEquationTableLookup(RG, NB_STAGES, 0, 40, 0.25)
	{
		for (int i = 0; i < NB_STAGES; i++)
			for (int j = 0; j < NB_PARAMETER; j++)
				m_p[i][j] = DEFAULT_P[i][j];

		for (int i = 0; i < NB_STAGES; i++)
			m_rho25Factor[i] = RHO25_FACTOR[i];

		Init();
	}


	double CWSBEquations::ComputeDailyDevlopmentRate(size_t e, double T)const
	{
		ASSERT(e < NB_STAGES);

		double Rt = 0;

		if (T <= 0)
			return 0;


		if (e == ADULT) //Maximum 30 day longevity at lower T, but death in 1 day at T<=0
		{
			Rt = m_rho25Factor[e] / min(30.0, max(1.0, (m_p[e][0] + m_p[e][1] * T + m_p[e][2] * T*T + m_p[e][3] * T*T*T)));
		}
		else
		{
			double TK = T + 273.0;
			double rho25 = m_rho25Factor[e] * m_p[e][0];
			double num = rho25*TK / 298.0*exp(m_p[e][1] / 1.987*(1 / 298.0 - 1 / TK));
			double den1 = exp(m_p[e][2] / 1.987*(1 / m_p[e][3] - 1 / TK));
			double den2 = exp(m_p[e][4] / 1.987*(1 / m_p[e][5] - 1 / TK));

			Rt = num / (1 + den1 + den2);
		}

		return min(1.0, max(0.0, Rt)); //This is daily rate, if time step smaller, must be multiplied by time step...
	}

	//***********************************************************************************
	//
	//Revised 2011-03-07 from SAS output (JR)	
	const double CWSBRelativeDevRate::V[NB_STAGES] =
	//Egg    OvL2   FeedL2   L3     L4     L5     L6    Pupa   Adult
	{ 0.063, 0.242, 0.229, 0.428, 0.289, 0.254, 0.252, 0.101, 0.279 };

	const double CWSBRelativeDevRate::S[2][NB_STAGES] =
		//Revised 2011-03-07 from SAS output (JR)	
	{//Egg,    OvL2, FeedL2,     L3,     L4,      L5,      L6,   Pupa, Adult
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, -0.017, -0.039, 0.013, 0 } //Adjust development rate of females
	};

	double CWSBRelativeDevRate::GetRate(size_t s, size_t sex)
	{
		_ASSERTE(s >= 0 && s < NB_STAGES);
		ASSERT(sex == 0 || sex == 1);

		return (1 + S[sex][s])*RandLogNormal(0, V[s]);
	}

	//***********************************************************************************

	const double CWSBAttrition::p[NB_STAGES][3] =
	{
		+0.0000, +0.00000, +0.00000,//Egg  
		+0.0000, +0.00000, +0.00000,//OW
		-6.3464, +0.10440, -0.00091,//L2
		-5.3884, +0.05502, +0.00000,//L3
		-5.6125, +0.06048, +0.00000,//L4
		-7.2000, +0.00618, +0.00288,//L5
		-2.0299, -0.85730, +0.02766,//L6
		+8.3397, -1.57860, +0.04145,//PUPAE
		+0.0000, +0.00000, +0.00000 //Adults (not used, nonsensical)  
	};

	double CWSBAttrition::GetRate(size_t s, double Tin)
	{
		_ASSERTE(s >= 0 && s < NB_STAGES);

		double T = max(0., Tin);
		double att = 1 / (1 + exp(p[s][0] + p[s][1] * T + p[s][2] * T*T));

		return att;
	};

	//***********************************************************************************
	const double CWSBOviposition::X0 = -0.9952;
	const double CWSBOviposition::X1 = -0.0345;
	const double CWSBOviposition::X2 = +0.4425;
	const double CWSBOviposition::X3 = +0.3987;

	double CWSBOviposition::GetRate(double T, double f)
	{
		double l = 0.0;
		if (f>0)
			l = max(0.0, min(1.0, X0 + X1*T + X2*pow(T, 0.5) + X3 / f));

		return l;
	}


	//***********************************************************************************

	//sex : MALE (0) or FEMALE (1)
	//A : forewing surface area [cm²]
	double CWSBEquations::get_A(size_t sex)const
	{
		ASSERT(sex < 2);

		static const double A_MEAN[2] = { 0.361, 0.421 };
		static const double A_SD[2] = { 0.047, 0.063 };


		double A = m_randomGenerator.RandNormal(A_MEAN[sex], A_SD[sex]);
		while (A < 0.20 || A>0.6)
			A = m_randomGenerator.RandNormal(A_MEAN[sex], A_SD[sex]);


		return A;
	}

	//A : forewing surface area [cm²]
	//L : forewing length [cm]
	double CWSBEquations::get_L(double A)
	{
		return sqrt(A / 0.325);
	}

	//sex : MALE (0) or FEMALE (1)
	//A : forewing surface area [cm²]
	//out : dry weight [g]
	double CWSBEquations::get_M(size_t sex, double A, double G)const
	{
		static const double M_A[2] = { -6.7560, -6.4648 };
		static const double M_B[2] = { 0.0000,  1.3260 };
		static const double M_C[2] = { 3.7900,  2.1400 };
		static const double M_D[2] = { 0.0000,  1.3050 };

		return exp(M_A[sex] + M_B[sex] * G + M_C[sex] * A + M_D[sex] * G * A);
	}


	//sex : MALE (0) or FEMALE (1)
	//A : forewing surface area [cm²]
	//out : Dry weight error term
	double CWSBEquations::get_ξ(size_t sex, double A)const
	{
		static const double M_ξ[2] = { 0.2060,  0.1600 };
		static const double M_L[2] = { 0.0015,  0.0090 };//0.009 = low full female
		static const double M_H[2] = { 0.0150,  0.0600 };//0.06 = hi full female

		double Mfull = get_M(sex, A, 1);

		//find error term that will be good for male or for full or empty female 
		double ξ = m_randomGenerator.RandUnbiasedLogNormal(log(1), M_ξ[sex]);
		while (Mfull * ξ<M_L[sex] || Mfull * ξ >M_H[sex])
			ξ = m_randomGenerator.RandUnbiasedLogNormal(log(1), M_ξ[sex]);

		return ξ;
	}

	double CWSBEquations::get_p_exodus()const
	{
		return 	m_randomGenerator.Randu();
	}


	//A : forewing surface area [cm²]
	//Fº: Initial fecundity in absence of defoliation [eggs]
	double CWSBEquations::get_Fº(double A)const
	{
		const double α = 1129.2;
		const double β = 1.760;

		double Fº = 0;
		do
		{
			double ξ = m_randomGenerator.RandUnbiasedLogNormal(log(1), 0.222);
			ASSERT(ξ >= 0.33 && ξ < 3.33);

			double F = α * pow(A, β);
			Fº = F * ξ;

		} while (Fº < 25 || Fº > 500);

		ASSERT(Fº >= 25 && Fº <= 500);

		return Fº;
	}




	//T: daily mean temperature
	//P: egg laid proportion 
	double CWSBEquations::get_P(double T)const
	{
		const double α = 0.489;
		const double β = 15.778;
		const double c = 2.08;

		double P = 0;
		do
		{
			double ξ = m_randomGenerator.RandUnbiasedLogNormal(log(1), 0.1);
			double p = α / (1 + exp(-(T - β) / c));
			P = p * ξ;

		} while (P < 0 || P > 0.7);

		return P;
	}

	double CWSBEquations::get_defoliation(double defoliation)const
	{
		ASSERT(defoliation >= 0 && defoliation <= 100);
		if (defoliation > 0 && defoliation < 100)
		{
			//From Régnière 2018 part III Equation [15]
			double μ = defoliation / 100.0;
			double σ² = 0.008101 + 0.5289 * μ - 0.5228 * Square(μ);

			double α = μ * ((μ * (1 - μ) / σ²) - 1);
			double β = (1 - μ) * ((μ * (1 - μ) / σ²) - 1);

			defoliation = m_randomGenerator.RandBeta(α, β) * 100;
			while (defoliation < 0 || defoliation>100)
				defoliation = m_randomGenerator.RandBeta(α, β) * 100;

		}
		ASSERT(defoliation >= 0 && defoliation <= 100);
		return defoliation;
	}
}