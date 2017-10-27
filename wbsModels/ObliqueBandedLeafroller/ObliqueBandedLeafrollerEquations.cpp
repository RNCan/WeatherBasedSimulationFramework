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

namespace WBSF
{

	//*****************************************************************************
	//Tranosema daily devlopment rates

	CObliqueBandedLeafrollerEquations::CObliqueBandedLeafrollerEquations(const CRandomGenerator& RG) :
		CEquationTableLookup(RG, NB_STAGES, 0, 40, 0.25)
	{
	}

	double CObliqueBandedLeafrollerEquations::Equation1(size_t s, double T)
	{
		ASSERT(s >= EGG && s <= PUPA);

		//development rate parameters (2 variables (Eg, Pupa), 6 parameters)
		enum TParameters{ RHO25, HA, HL, TL, HH, TH, NB_PARAMETERS };
		static const double P[2][NB_PARAMETERS] =
		{
			//	rho25	   HA		   HL		TL		HH		TH
			0.1144,  -6.1490, -34.3960, 292.10, 108.24, 302.55, //Egg
			0.2972, -32.9135, -50.8942, 299.83, 100.00, 308.00, //Pupa 
		};

		double tK = T + 273;
		double num = P[s][RHO25] * tK / 298.0 * exp(P[s][HA] / 0.001987*(1.0 / 298.0 - 1.0 / tK));
		double den1 = exp(P[s][HL] / 0.001987*(1.0 / P[s][TL] - 1.0 / tK));
		double den2 = exp(P[s][HH] / 0.001987*(1.0 / P[s][TH] - 1.0 / tK));
		double Rt = (num / (1 + den1 + den2));

		return max(0.0, Rt);
	}

	double CObliqueBandedLeafrollerEquations::Equation2(size_t s, double T)
	{
		ASSERT(s == ADULT);

		//Adult development rate parameters (2 parameters)
		enum TRelDevParameters{ P1, P2, NB_PARAMETERS };
		static const double P[1][NB_PARAMETERS] = { -5.1696, 0.07065 }; //Adult, latest estimates 2016/03/14 JR

		double Rt = exp(P[0][P1])*pow(exp(T), P[0][P2]);

		return max(0.0, Rt);
	}


	//Compute daily development rate for table lookup
	double CObliqueBandedLeafrollerEquations::ComputeRate(size_t s, double T)const
	{
		ASSERT(s >= 0 && s < NB_STAGES);

		//reltive developement
		double Rt = 0;

		switch (s)
		{
		case EGG:
		case PUPA: Rt = Equation1(s, T); break;
		case ADULT:	Rt = Equation2(s, T); break;
		default: ASSERT(false);
		}


		_ASSERTE(!_isnan(Rt) && _finite(Rt));
		ASSERT(Rt >= 0);
		return Rt;
	}


	//*****************************************************************************
	// individual relative development rate 

	double CObliqueBandedLeafrollerEquations::Equation3(size_t s)const
	{
		ASSERT(s >= EGG && s <= ADULT);
		static const double P[NB_STAGES][2] =
		{
			//  x      s
			{ 0.000, 0.082050 },	//Egg
			{ 0.000, 0.090180 },	//Pupa
			{ 0.000, 0.268500 }		//Adult (-.5385/2, half variablity JR 2016/03/14)
		};


		double 	r = m_randomGenerator.RandLogNormal(P[s][0], P[s][1]);

		_ASSERTE(!_isnan(r) && _finite(r));
		if (_isnan(r) || !_finite(r))//just in case
			r = 1;

		return r;

	}

	double CObliqueBandedLeafrollerEquations::Getδ(size_t s)const
	{
		double r = Equation3(s);
		while (r<0.4 || r>2.5)
			r = Equation3(s);

		return r;
	}

	//*****************************************************************************
	//fecondity

	double CObliqueBandedLeafrollerEquations::GetPmax()const
	{
		//							  x   s
		static const double P[2] = { 143, 30 };

		double Pmax = m_randomGenerator.RandNormal(P[0], P[1]);
		while (Pmax < 43 || Pmax>243)
			Pmax = m_randomGenerator.RandNormal(P[0], P[1]);

		ASSERT(Pmax > 0);

		return Pmax;
	}

	double CObliqueBandedLeafrollerEquations::GetE°()const
	{
		//							  x      s
		static const double P[2] = { 2.28, 0.0800 };
		double Eo = m_randomGenerator.RandLogNormal(P[0], P[1]);
		while (Eo < 0 || Eo>50)
			Eo = m_randomGenerator.RandLogNormal(P[0], P[1]);

		return Eo;
	}

	double CObliqueBandedLeafrollerEquations::GetOᵗ(double T)
	{
		return std::max(0.0, -2.7355 + 0.9555*T);
	}
	double CObliqueBandedLeafrollerEquations::GetRᵗ(double T)
	{
		return std::max(0.0, -26.9709 + 2.55415*T);
	}


	//*****************************************************************************
	//survival rate 


	double CObliqueBandedLeafrollerEquations::GetSurvivalRate(size_t s, double T)
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

	double CObliqueBandedLeafrollerEquations::GetLuck(size_t s)
	{
		if (s == ADULT)
			return 1;

		return m_randomGenerator.Rand(0.0, 1.0);
	}
}