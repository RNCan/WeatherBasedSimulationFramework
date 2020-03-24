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
		TDevRateEquation::Briere2_1999, //OBL post diapause
		TDevRateEquation::Briere2_1999, //Maggot OBL
		TDevRateEquation::Briere2_1999, //Maggot SBW
		TDevRateEquation::Briere2_1999, //Pupa
		TDevRateEquation::Unknown	    //Adult
	};






	const double  CActiaInterruptaEquations::EQ_P[NB_EQUATIONS][4]
	{
		{8.240e-06,  0.8868,   5.6, 40.0}, //OBL post diapause
		{2.234e-08,  0.4017,   2.6, 52.2}, //Post-Diapause Maggot in OBL 
		{7.393e-05,  4.4770, -14.0, 30.8}, //Summer Maggot SBW and OBL
		{1.188e-04, 15.8019, -17.3, 30.1}, //Pupa
		{0,0,0,0},					   //Adult
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
		double Rt = 0;
		

		if (e < EQ_ADULT)
			Rt = max(0.0, min(1.0, CDevRateEquation::GetRate(EQ_TYPE[e], p, T)));
		else
			Rt = 1.0 / 22.4; //Rt = 1.0/exp(log(44) - log(max(10.0, min(30.0, T)) / 10));
			



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
			{ 0.0000, 0.3488, 0.4, 2.0 },//OBL post diapause
			{ 0.0000, 0.3006, 0.5, 2.0 },//PostDiapause Maggot in OBL
			{ 0.0000, 0.1652, 0.5, 2.0 },//Summer Maggot SBW or OBL
			{ 0.0000, 0.1254, 0.7, 1.5 },//Pupa
			{ /*2.8207*/2.9729, 0.5517, 1.0, 100 },//Adult longevity
		};


		double 	r = m_randomGenerator.RandUnbiasedLogNormal(P[e][0], P[e][1]);
		while (r<P[e][2] || r>P[e][3])
			r = m_randomGenerator.RandUnbiasedLogNormal(P[e][0], P[e][1]);



		_ASSERTE(!_isnan(r) && _finite(r));
		if (_isnan(r) || !_finite(r))//just in case
			r = 1;

		if (e == EQ_ADULT)
			r = 22.4 / r ;//transform relative longevity into relative rate


		return r;

	}


	//*****************************************************************************
	//fecundity

	double CActiaInterruptaEquations::GetPmax()const
	{
		//maggots / female
		double fec = m_randomGenerator.RandNormal(129.43, 47.2);
		
		//limit between 1% and 99%
		while (fec < 39 || fec>225)
			fec=m_randomGenerator.RandNormal(129.43, 47.2);

		return fec;
	}
	
	

	//double CActiaInterruptaEquations::GetEº()const
	//{
	//	//							  x      s
	//	static const double P[2] = { 2.28, 0.0800 };
	//	double Eo = m_randomGenerator.RandUnbiasedLogNormal(P[0], P[1]);
	//	while (Eo < 0 || Eo>50)
	//		Eo = m_randomGenerator.RandUnbiasedLogNormal(P[0], P[1]);

	//	return Eo;
	//}

	//double CActiaInterruptaEquations::GetOᵗ(double T)
	//{
	//	return std::max(0.0, -2.7355 + 0.9555*T);
	//}
	//double CActiaInterruptaEquations::GetRᵗ(double T)
	//{
	//	return std::max(0.0, -26.9709 + 2.55415*T);
	//}



	//*****************************************************************************
	//survival rate 



	double CActiaInterruptaEquations::GetSurvivalRate(size_t s, double T)
	{
		//This is code for Tranosema rostrale, not applicable to Actia interrupta.
		static const double P[NB_STAGES][2] =
		{
			//  x      s
			{ 7.0251, -0.1735 },	//Maggot
			{ 9.6200, -0.2389 },	//Pupa
			{ 1.000, 0.0000 }		//Adult
		};

		double r = 0;switch (s)
		{
		case MAGGOT:
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