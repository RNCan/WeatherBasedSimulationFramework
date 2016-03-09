//*****************************************************************************
// File: TranosemaEquations.h
//
// Class: CTranosemaEquations
//
// Description:  Basic of Tranosema calculations (development rates, oviposition rates, attrition rates..)
//				mean development rates use optimization table lookup
//*****************************************************************************
// 22/01/2016	Rémi Saint-Amant	Using Weather-Based Simulation Framework (WBSF)
// 11/12/2015   Rémi Saint-Amant    Creation from paper
//*****************************************************************************

#include "Basic/UtilMath.h"
#include "TranosemaEquations.h"

using namespace std;
using namespace WBSF::Tranosema;

namespace WBSF
{

	//*****************************************************************************
	//Tranosema daily devlopment rates

	CTranosemaEquations::CTranosemaEquations(const CRandomGenerator& RG) :
		CEquationTableLookup(RG, NB_STAGES, 0, 40, 0.25)
	{
	}

	double CTranosemaEquations::Equation1(size_t s, double T)
	{
		ASSERT(s >= EGG && s <= PUPA);

		//development rate parameters (2 variables (Eg, Pupa), 6 parameters)
		enum TParameters{ RHO25, HA, HL, TL, HH, TH, NB_PARAMETERS };
		static const double P[2][NB_PARAMETERS] =
		{
			//	rho25	   HA		   HL		TL		HH		TH
			0.1140, -3.1910, -32.0461, 291.10, 108.19, 302.47, //Egg
			0.2951, -32.8779, -50.9263, 299.79, 100.00, 307.99, //Pupa 
		};

		double tK = T + 273;
		double num = P[s][RHO25] * tK / 298.0 * exp(P[s][HA] / 0.001987*(1.0 / 298.0 - 1.0 / tK));
		double den1 = exp(P[s][HL] / 0.001987*(1.0 / P[s][TL] - 1.0 / tK));
		double den2 = exp(P[s][HH] / 0.001987*(1.0 / P[s][TH] - 1.0 / tK));
		double Rt = (num / (1 + den1 + den2));

		return max(0.0, Rt);
	}

	double CTranosemaEquations::Equation2(size_t s, double T)
	{
		ASSERT(s == ADULT);

		//Adult development rate parameters (2 parameters)
		enum TRelDevParameters{ P1, P2, NB_PARAMETERS };
		static const double P[1][NB_PARAMETERS] = { -5.4165, 0.08626 }; //Adult

		double Rt = exp(P[0][P1])*pow(exp(T), P[0][P2]);

		return max(0.0, Rt);
	}


	//Compute daily development rate for table lookup
	double CTranosemaEquations::ComputeRate(size_t s, double T)const
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

	double CTranosemaEquations::Equation3(size_t s)const
	{
		ASSERT(s >= EGG && s <= ADULT);
		static const double P[NB_STAGES][2] =
		{
			//  x      s
			{ 0.000, 0.084740 },	//Egg
			{ 0.000, 0.090060 },	//Pupa
			{ 0.000, 0.262100 }		//Adult
		};


		double 	r = m_randomGenerator.RandLogNormal(P[s][0], P[s][1]);

		_ASSERTE(!_isnan(r) && _finite(r));
		if (_isnan(r) || !_finite(r))//just in case
			r = 1;

		return r;

	}

	double CTranosemaEquations::Getδ(size_t s)const
	{
		double r = Equation3(s);
		while (r<0.4 || r>2.5)
			r = Equation3(s);

		return r;
	}

	//*****************************************************************************
	//fecondity

	double CTranosemaEquations::GetPmax()const
	{
		//							  x   s
		static const double P[2] = { 143, 30 };

		double Pmax = m_randomGenerator.RandNormal(P[0], P[1]);
		ASSERT(Pmax > 0);

		return Pmax;
	}

	double CTranosemaEquations::GetE°()const
	{
		//							  x      s
		static const double P[2] = { 2.28, 0.0800 };
		double Eo = m_randomGenerator.RandLogNormal(P[0], P[1]);
		while (Eo < 0 || Eo>50)
			Eo = m_randomGenerator.RandLogNormal(P[0], P[1]);

		return Eo;
	}

	double CTranosemaEquations::GetOᵗ(double T)
	{
		return std::max(0.0, -2.7355 + 0.9555*T);
	}
	double CTranosemaEquations::GetRᵗ(double T)
	{
		return std::max(0.0, -26.9709 + 2.55415*T);
	}


	//*****************************************************************************
	//survival rate 


	double CTranosemaEquations::GetSurvivalRate(size_t s, double T)
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

	double CTranosemaEquations::GetLuck(size_t s)
	{
		if (s == ADULT)
			return 1;

		return m_randomGenerator.Rand(0.0, 1.0);
	}
}