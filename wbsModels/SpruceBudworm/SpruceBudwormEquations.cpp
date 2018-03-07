//*****************************************************************************
// File: SpruceBudwormEquations.h
//
// Class: CSpruceBudwormEquations
//          
//
// Description: Basic of Spruce Budworm calculation. 
//				stage development rates, relative development rates, oviposition
//				stage development rates use optimization table lookup
//
//*****************************************************************************
// 03/03/2017	Rémi Saint-Amant	Add eggs laid proportion
// 23/12/2016   Rémi Saint-Amant	Add flight activity equations
// 06/03/2015	Rémi Saint-Amant	Add use of CRandomGenerator to avoid thread random generation comflict
// 18/06/2013   Rémi Saint-Amant    new format of table
// 27/09/2011   Rémi Saint-Amant    add b1Factor multiplication
// 11/03/2010   Rémi Saint-Amant    Creation from new paper
//*****************************************************************************
#include "SpruceBudwormEquations.h"


using namespace WBSF;
using namespace SBW;
using namespace std;

namespace WBSF
{

	//*****************************************************************************
	//CWSBDevelopment class 


	//development rate parameters (12 equations, 6 parameters)
	const double CSpruceBudwormEquations::P[NB_EQUATION][NB_PARAMETER] =
	{
		//	b1      b2      b3      b4      Tb      Tm
		0.228, 3.120, 5.940, 0.073, 6.0, 35, //Egg
		0.277, 32.14, 11.63, 0.000, 6.2, 40, //L1
		0.194, 3.000, 5.840, 0.034, 2.5, 35, //Overwintering L2
		0.919, 2.910, 5.320, 0.061, 4.4, 38, //Feeding L2
		0.438, 3.060, 6.850, 0.061, 4.4, 38, //L3
		1.211, 3.800, 7.550, 0.148, 4.4, 38, //L4
		0.269, 3.020, 8.570, 0.005, 4.4, 38, //L5
		0.288, 2.670, 5.030, 0.151, 4.4, 38, //L6 male
		0.317, 3.060, 4.660, 0.136, 4.4, 38, //L6 female
		0.259, 2.750, 4.660, 0.053, 4.4, 35, //Pupa male
		0.205, 2.854, 6.275, 0.044, 4.4, 35, //Pupa female
//		57.80, -3.08, .0451, 0.000, 8.0, 35  //Adult
		57.80, -3.08, .0451, 0.000, -10.0, 40  //Adult By RSA 14-02-2018 to avoid stop development at low and high temperature
	};

	//Development rate altered for pupae, JR testing 2011/10/25
	double CSpruceBudwormEquations::b1Factor[NB_STAGES] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };

	CSpruceBudwormEquations::CSpruceBudwormEquations(const CRandomGenerator& RG) :
		CEquationTableLookup(RG, CSpruceBudwormEquations::NB_EQUATION, -10, 40, 0.25)
	{
	}



	size_t CSpruceBudwormEquations::e2s(size_t e)
	{
		if (e < E_L6_FEMALE)
			return e;
		if (e < E_PUPAE_FEMALE)
			return e - 1;

		return e - 2;
	}

	double CSpruceBudwormEquations::Equation1(size_t e, double T)
	{
		const double* p = P[e];//current P for equation
		size_t s = e2s(e);//compute stage for b1Factor

		double Tau = (T - p[PTB]) / (p[PTM] - p[PTB]);
		double p1 = 1 / (1 + exp(p[PB2] - p[PB3] * Tau));
		double p2 = exp((Tau - 1) / p[PB4]);
		double Rt = p[PB1] * b1Factor[s] * (p1 - p2);

		return max(0.0, Rt);
	}

	double CSpruceBudwormEquations::Equation2(size_t e, double T)
	{
		const double* p = P[e];//current P for equation
		size_t s = e2s(e);//compute stage for b1Factor

		double p1 = exp(-0.5*Square((T - p[PB2]) / p[PB3]));
		double Rt = p[PB1] * b1Factor[s] * p1;

		return max(0.0, Rt);
	}

	double CSpruceBudwormEquations::Equation3(size_t e, double T)
	{
		const double* p = P[e];//current P for equation
		size_t s = e2s(e);//compute stage for b1Factor
		ASSERT(s == ADULT);
		
		T = max(8.0, min(35.0, T) );
		double Rt = 1 / (p[PB1] * b1Factor[s] + p[PB2] * T + p[PB3] * T*T);
		return max(0.0, Rt);
	}

	//Daily development rate
	double CSpruceBudwormEquations::ComputeRate(size_t e, double T)const
	{
		ASSERT(e >= 0 && e < NB_EQUATION);


		double Rt = 0;

		if (T >= P[e][PTB] && T <= P[e][PTM])
		{
			switch (e)
			{
			case E_EGG:			Rt = Equation1(e, T); break;
			case E_L1:			Rt = Equation2(e, T); break;
			case E_L2o:			Rt = Equation1(e, T); break;
			case E_L2:			Rt = Equation1(e, T); break;
			case E_L3:			Rt = Equation1(e, T); break;
			case E_L4:			Rt = Equation1(e, T); break;
			case E_L5:			Rt = Equation1(e, T); break;
			case E_L6_MALE:		Rt = Equation1(e, T); break;
			case E_L6_FEMALE:	Rt = Equation1(e, T); break;
			case E_PUPAE_MALE:	Rt = Equation1(e, T); break;
			case E_PUPAE_FEMALE:Rt = Equation1(e, T); break;
			case E_ADULT:		Rt = Equation3(e, T); break;
			default: _ASSERTE(false);
			}
		}

		_ASSERTE(!_isnan(Rt) && _finite(Rt));
		ASSERT(Rt >= 0);
		return Rt;
	}

	//Get equation index from stage and sex because some stages have sex dependent equations
	size_t CSpruceBudwormEquations::GetEquationIndex(size_t stage, size_t sex)
	{
		ASSERT(stage >= 0 && stage < NB_STAGES);
		ASSERT(sex >= 0 && sex < NB_SEX);

		size_t e = stage;
		if (stage == L6)
			e += sex;
		else if (stage == PUPAE)
			e += 1 + sex;
		else if (stage == ADULT)
			e += 2;

		return e;
	}

	//*****************************************************************************
	//CSBRelativeDevRate : compute individual relative development rate 
	double CSpruceBudwormEquations::Equation4(const double p[NB_REL_DEV_PARAMETERS])const
	{
		double X = m_randomGenerator.Randu(true, true);
		return p[A1] * pow(-log(1 - X), 1 / p[A2]);
	}

	double CSpruceBudwormEquations::Equation5(const double p[NB_REL_DEV_PARAMETERS])const
	{
		double X = m_randomGenerator.Randu(true, true);
		return 1 - log((pow(X, p[A1]) - 1) / (pow(0.5, p[A1]) - 1)) / p[A2];
	}


	double CSpruceBudwormEquations::RelativeDevRateBase(size_t s)const
	{
		static const double P[NB_STAGES][NB_REL_DEV_PARAMETERS] =
		{
			//  a1      a2
			{ -3.98, 29.48 },//Egg
			{ 0.000, 0.000 },//L1
			{ 1.050, 6.680 },//L2o
			{ -1.68, 8.120 },//L2
			{ -1.13, 9.030 },//L3
			{ -1.31, 10.02 },//L4
			{ -0.60, 7.370 },//L5
			{ -1.80, 11.60 },//L6
			{ 0.000, 0.000 },//Pupa
			{ 0.000, 0.000 } //Adult
		};

		double r = 0;
		switch (s)
		{
		case EGG:	r = Equation5(P[s]); break;
		case L1:	r = 1; break;
		case L2o:	r = Equation4(P[s]); break;
		case L2:	r = Equation5(P[s]); break;
		case L3:	r = Equation5(P[s]); break;
		case L4:	r = Equation5(P[s]); break;
		case L5:	r = Equation5(P[s]); break;
		case L6:	r = Equation5(P[s]); break;
		case PUPAE:	r = 1; break;
		case ADULT:	r = 1; break;
		default: _ASSERTE(false);
		}

		_ASSERTE(!_isnan(r) && _finite(r));
		if (_isnan(r) || !_finite(r))//just in case
			r = 1;

		return r;

	}
	
	//sex : MALE (0) or FEMALE (1)
	double CSpruceBudwormEquations::RelativeDevRate(size_t s)const
	{
		double r = RelativeDevRateBase(s);
		while (r<0.4 || r>2.5)
			r = RelativeDevRateBase(s);

		return r;
	}

	
	
	//sex : MALE (0) or FEMALE (1)
	//out : forewing surface area [cm²]
	double CSpruceBudwormEquations::get_A(size_t sex)const
	{
		ASSERT(sex < 2);

		static const double A_MEAN[2] = { 0.361, 0.421 };
		static const double A_SD[2] = { 0.047, 0.063 };


		double A = m_randomGenerator.RandNormal(A_MEAN[sex], A_SD[sex]);
		while (A < 0.25 || A>0.6)
			A=m_randomGenerator.RandNormal(A_MEAN[sex], A_SD[sex]);
		 
		
		return A;
	}

	//sex : MALE (0) or FEMALE (1)
	//A : forewing surface area [cm²]
	//bE: add error term 
	//out : weight [g]
	double CSpruceBudwormEquations::get_M(size_t sex, double A, double G, bool bE)const
	{
		static const double M_A[2] = { -6.756, -6.465 };
		static const double M_B[2] = { 0.000, 1.326 };
		static const double M_C[2] = { 3.790, 2.140 };
		static const double M_D[2] = { 0.000, 1.305 };
		static const double M_E[2] = { 0.206, 0.160 };
		static const double M_L[2] = { 0.0015, 0.0024 };
		static const double M_H[2] = { 0.015, 0.050 };


		if (sex == MALE)
			G = 0;
		
		double M = 0;
		do
		{
			double ξ = bE ? m_randomGenerator.RandLogNormal(0, M_E[sex]) : 1;
			double m = exp(M_A[sex] + M_B[sex] * G + M_C[sex] * A + M_D[sex] * G*A);
			M = m*ξ;
		} while (M<M_L[sex] || M>M_H[sex]);


		return M;
	}


	double CSpruceBudwormEquations::get_p_exodus()const
	{
		return 	m_randomGenerator.Randu();
	}


	//A : forewing surface area [cm²]
	//Fº: Initial fecundity in absence of defoliation [eggs]
	double CSpruceBudwormEquations::get_Fº(double A)const
	{
		const double α = 1129.2;
		const double β = 1.760;

		double Fº = 0;
		do
		{
			double ξ = m_randomGenerator.RandLogNormal(log(1), 0.222);
			ASSERT(ξ >= 0.33 && ξ < 3.33);

			double F = α*pow(A, β);
			Fº = F*ξ;

		} while (Fº<25 || Fº > 500);


		ASSERT(Fº >= 25 && Fº <= 500);

		return Fº;
	}



	
	//T: daily mean temperature
	//P: egg laid proportion 
	double CSpruceBudwormEquations::get_P(double T)const
	{
		const double α = 0.489;
		const double β = 15.778;
		const double c = 2.08;

		double P = 0;
		do
		{
			double ξ = m_randomGenerator.RandLogNormal(log(1), 0.1);
			double p = α/(1+exp(-(T-β)/c));
			P = p*ξ;

		} while (P<0 || P > 0.7);

		return P;
	}

	double CSpruceBudwormEquations::get_defoliation(double defoliation)const
	{
		ASSERT(defoliation >= 0 && defoliation <= 100);
		if (defoliation > 0)
		{
			double d = defoliation / 100;
			double v = max(0.000623, 0.000623 + 0.1463*d + 0.1544*Square(d) - 0.32*Cube(d));

			double α = d*(d*(1 - d) / v - 1);
			double β = (1 - d)*(d*(1 - d) / v - 1);
			defoliation = m_randomGenerator.RandBeta(α, β) * 100;

			while (defoliation < 0 || defoliation>100)
				defoliation = m_randomGenerator.RandBeta(α, β) * 100;

		}
		ASSERT(defoliation >= 0 && defoliation <= 100);
		return defoliation;
	}

}