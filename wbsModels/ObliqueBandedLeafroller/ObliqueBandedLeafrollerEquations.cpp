//*****************************************************************************
// File: TranosemaEquations.h
//
// Class: CObliqueBandedLeafrollerEquations
//
// Description:  Basic of Tranosema calculations (development rates, oviposition rates, attrition rates..)
//				mean development rates use optimization table lookup
//*****************************************************************************
// 27/10/2017   Rémi Saint-Amant    Creation from paper
//*****************************************************************************

#include "Basic/UtilMath.h"
#include "ObliqueBandedLeafrollerEquations.h"


using namespace std;
using namespace WBSF::OBL;

namespace WBSF
{

	//*****************************************************************************
	//Tranosema daily devlopment rates

	CObliqueBandedLeafrollerEquations::CObliqueBandedLeafrollerEquations(const CRandomGenerator& RG) :
		CEquationTableLookup(RG, NB_STAGES*2, 0, 40, 0.4) //Was 0.25
	{
	}

	double CObliqueBandedLeafrollerEquations::Equation1(size_t e, double T)
	{
		size_t s = e % NB_STAGES;
		size_t sex = e / NB_STAGES;

		ASSERT( s < NB_STAGES);
		ASSERT(sex <2);

		//development rate parameters (11 variables (Egg...adult), 3 parameters)
		enum TParameters{ P_TL, P_A, P_B, NB_PARAMETERS };
		static const double P[NB_STAGES][NB_PARAMETERS] =
		{
//			Tl			a			b
			9.5,	-0.08330,	0.008744,	//Egg
			11.0,	-0.14279,	0.013027,	//L1
			9.9,	-0.18377,	0.018627,	//L2
			9.9,	-0.17239,	0.0173625,	//L3
			11.1,	-0.11872,	0.01069,	//L3D
			7.1,	-0.08368,	0.011875,	//L4
			8.8,	-0.12157,	0.0138175,	//L5
			11.4,	-0.12648,	0.011125,	//L6 ???? 0.0011125 ou 0.011125
			9.6,	-0.07901,	0.008305,	//PUPA
			11.9,	-0.33918,	0.028427,	//Adult preovip
			4.2,	-0.02667,	0.004		//Adult //was .0064, this makes longer-lived adults.
		};
		
		

		double r = 0;
		double Tʟ = P[s][P_TL];
		double a = P[s][P_A];
		double b = P[s][P_B];

		static const double L6_FACTOR[2] = { 1.23, 0.84 };
		if (s == L6)
		{
			a *= L6_FACTOR[sex];
			b *= L6_FACTOR[sex];
		}

		if (T > Tʟ)
			r = a + b * T;

		
		return max(0.0, r);
	}

	//Compute daily development rate for table lookup
	double CObliqueBandedLeafrollerEquations::ComputeRate(size_t e, double T)const
	{
		ASSERT(e >= 0 && e < 2*NB_STAGES);

		//reltive developement
		double r = Equation1(e, T); 


		_ASSERTE(!_isnan(r) && _finite(r));
		ASSERT(r >= 0);
		return r;
	}


	//*****************************************************************************
	// individual relative development rate 

	double CObliqueBandedLeafrollerEquations::Equation2()const
	{
		double 	r = m_randomGenerator.RandUnbiasedLogNormal(1.0, 0.4); //Was 0.25

		_ASSERTE(!_isnan(r) && _finite(r));
		if (_isnan(r) || !_finite(r))//just in case
			r = 1;

		return r;

	}

	double CObliqueBandedLeafrollerEquations::Getδ(size_t s)const
	{
		double r = Equation2();
		while (r<0.4 || r>2.5)
			r = Equation2();

		return r;
	}

	//*****************************************************************************
	//fecondity
	double CObliqueBandedLeafrollerEquations::GetEᵗ(double A0, double A1)
	{
		ASSERT(A0 <= A1);
		//if (A0<ADULT)
			//return 0;
		
		//if (A1 < ADULT)
			//A1 = ADULT;

		//double Eᵗ = 200 * (exp(-4 * (A0 - ADULT)) - exp(-4 * (A1 - ADULT)));
		double Eᵗ = 200 * (exp(-4 *A0) - exp(-4 *A1));
		ASSERT(Eᵗ >= 0);

		return Eᵗ;
	}
	

	//*****************************************************************************
	//survival rate 


	//double CObliqueBandedLeafrollerEquations::GetSurvivalRate(size_t s, double T)
	//{
	//	static const double P[NB_STAGES][2] =
	//	{
	//		//  x      s
	//		{ 7.0251, -0.1735 },	//Egg
	//		{ 9.6200, -0.2389 },	//Pupa
	//		{ 1.000, 0.0000 }		//Adult
	//	};

	//	double r = 0;
	//	switch (s)
	//	{
	//	case EGG:
	//	case PUPA:	r = 1 / (1 + exp(-(P[s][0] + P[s][1] * T))); break;
	//	case ADULT:	r = 1; break;
	//	default: _ASSERTE(false);
	//	}

	//	_ASSERTE(!_isnan(r) && _finite(r));
	//	if (_isnan(r) || !_finite(r))//just in case
	//		r = 1;

	//	return r;
	//}

	//double CObliqueBandedLeafrollerEquations::GetLuck(size_t s)
	//{
	//	if (s == ADULT)
	//		return 1;

	//	return m_randomGenerator.Rand(0.0, 1.0);
	//}
}