//*****************************************************************************
// File: ActiaInterruptaEquations.h
//
// Class: CActiaInterruptaEquations
//
// Description:  Basic of ActiaInterrupta calculations (development rates, oviposition rates, attrition rates..)
//				mean development rates use optimization table lookup
//*****************************************************************************
// 25/02/2020   Rémi Saint-Amant    Creation from paper
//*****************************************************************************

#include "Basic/UtilMath.h"
#include "ActiaInterruptaEquations.h"

using namespace std;
using namespace WBSF::ActiaInterrupta;

namespace WBSF
{

	//*****************************************************************************
	//ActiaInterrupta daily development rates

	const CDevRateEquation::TDevRateEquation CActiaInterruptaEquations::EQ_TYPE[NB_EQUATIONS]
	{
		TDevRateEquation::Briere2_1999,
		TDevRateEquation::Briere2_1999,
		TDevRateEquation::Briere2_1999,
		TDevRateEquation::Poly1
	};

	const double  CActiaInterruptaEquations::EQ_P[NB_EQUATIONS][4]
	{
		{0.0000081,1.0 / 1.1308,0.000,40},
		{0.0001,1.0 / 0.3778,4.5379,31.7286},
		{0.000174,1.0 / 0.1648,0.3073,30.5749},
		{1.0/22.0,0,0,0},
	};


	CActiaInterruptaEquations::CActiaInterruptaEquations(const CRandomGenerator& RG) :
		CEquationTableLookup(RG, NB_EQUATIONS, 0, 40, 0.25)
	{
	}

	//Compute daily development rate for table lookup
	double CActiaInterruptaEquations::ComputeRate(size_t e, double T)const
	{
		ASSERT(e >= 0 && e < NB_EQUATIONS);

		vector<double> p(begin(EQ_P[e]), end(EQ_P[e]));
		double Rt = max(0.0, min(1.0, CDevRateEquation::GetRate(EQ_TYPE[e], p, T)));

		_ASSERTE(!_isnan(Rt) && _finite(Rt));
		ASSERT(Rt >= 0);

		return Rt;
	}


	//*****************************************************************************
	// individual relative development rate 

	double CActiaInterruptaEquations::Getδ(size_t e)const
	{
		ASSERT(e <= NB_EQUATIONS);
		static const double P[NB_EQUATIONS][4] =
		{
			//  x      s
			{ 0.0000, 0.3488, 0.4, 2.0 },//Egg Obl
			{ 0.0000, 0.2135, 0.5, 2.0 },//Egg SBW
			{ 0.0000, 0.4695, 0.3, 2.5 },//Pupa
			{ 2.8207, 0.5517, 0.0, 9.9 },//Adult
		};


		double 	r = m_randomGenerator.RandUnbiasedLogNormal(P[e][0], P[e][1]);
		while (r<P[e][2] || r>P[e][3])
			r = m_randomGenerator.RandUnbiasedLogNormal(P[e][0], P[e][1]);



		_ASSERTE(!_isnan(r) && _finite(r));
		if (_isnan(r) || !_finite(r))//just in case
			r = 1;

		if (e == EQ_ADULT)
			r = 22.0 / (22.0+r);//transform relative longevity into relative rate


		return r;

	}


	//*****************************************************************************
	//fecundity

	double CActiaInterruptaEquations::GetPmax()const
	{
//		Preopiposition period : 5 days
	//	Total fecundity : 135 (all the same)
		//Maximum daily fecundity : 135 / (Longevity - 5)
		return 135;
	}


	double CActiaInterruptaEquations::GetEº()const
	{
		//est-ce correcte d'utilise la version Unbiased?????
		//							  x      s
		static const double P[2] = { 2.28, 0.0800 };
		double Eo = m_randomGenerator.RandUnbiasedLogNormal(P[0], P[1]);
		while (Eo < 0 || Eo>50)
			Eo = m_randomGenerator.RandUnbiasedLogNormal(P[0], P[1]);

		return Eo;
	}

	double CActiaInterruptaEquations::GetOᵗ(double T)
	{
		return std::max(0.0, -2.7355 + 0.9555*T);
	}
	double CActiaInterruptaEquations::GetRᵗ(double T)
	{
		return std::max(0.0, -26.9709 + 2.55415*T);
	}



	//*****************************************************************************
	//survival rate 


	double CActiaInterruptaEquations::GetSurvivalRate(size_t s, double T)
	{
		static const double P[NB_STAGES][2] =
		{
			//  x      s
			{ 7.0251, -0.1735 },	//Egg
			{ 9.6200, -0.2389 },	//Pupa
			{ 1.000, 0.0000 }		//Adult
		};

		double r = 0;
		switch (s)
		{
		case EGG:
		case PUPA:	r = 1 / (1 + exp(-(P[s][0] + P[s][1] * T))); break;
		case ADULT:	r = 1; break;
		default: _ASSERTE(false);
		}

		_ASSERTE(!_isnan(r) && _finite(r));
		if (_isnan(r) || !_finite(r))//just in case
			r = 1;

		return r;
	}


	double CActiaInterruptaEquations::GetLuck(size_t s)const
	{
		if (s == ADULT)
			return 1;

		return m_randomGenerator.Rand(0.0, 1.0);
	}
	
}