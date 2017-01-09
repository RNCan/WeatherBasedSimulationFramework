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
		57.80, -3.08, .0451, 0.000, 8.0, 35  //Adult
	};

	//Development rate altered for pupae, JR testing 2011/10/25
	double CSpruceBudwormEquations::b1Factor[NB_STAGES] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };

	CSpruceBudwormEquations::CSpruceBudwormEquations(const CRandomGenerator& RG) :
		CEquationTableLookup(RG, CSpruceBudwormEquations::NB_EQUATION, 0, 40, 0.25)
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
		while (A < 0.1)
			m_randomGenerator.RandNormal(A_MEAN[sex], A_SD[sex]);


		return A;
	}

	//sex : MALE (0) or FEMALE (1)
	//A : forewing surface area [cm²]
	//out : weight [g]
	double CSpruceBudwormEquations::get_M(size_t sex, double A)const
	{
		static const double M_A[2] = { -6.756, -6.543};
		static const double M_B[2] = { 3.790, 3.532};
		static const double M_E[2] = { 0.206, 0.289 };

		double E = m_randomGenerator.RandLogNormal(0, M_E[sex]);
		double M = exp(M_A[sex] + M_B[sex] * A);
		
		return M*E;
	}

	//double CSpruceBudwormEquations::get_M(size_t sex, double A, double G)const
	//{
	//	double M = 0;

	//	if (sex == MALE)
	//		M = get_M(sex, A);
	//	else
	//		M = exp(-6.465 + 0.974*G + 2.14*A + 1.305*G*A)*m_randomGenerator.RandLogNormal(0, 0.1604);

	//	return M;
	//}

	double CSpruceBudwormEquations::get_Mf(double A, double G)const
	{
		double Mf = exp(-6.465 + 0.974*G + 2.14*A + 1.305*G*A);
		return Mf;
	}

	double CSpruceBudwormEquations::get_p_exodus()const
	{
		return 	m_randomGenerator.Randu();
	}

}