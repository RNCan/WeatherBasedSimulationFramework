//*****************************************************************************
// File: MeteorusTrachynotusEquations.h
//
// Class: CMeteorusTrachynotusEquations
//
// Description:  Basic of MeteorusTrachynotus calculations (development rates, oviposition rates, attrition rates..)
//				mean development rates use optimization table lookup
//*****************************************************************************
// 22/04/2020   Rémi Saint-Amant    Creation 
//*****************************************************************************

#include "Basic/UtilMath.h"
#include "MeteorusTrachynotusEquations.h"

using namespace std;
using namespace WBSF::MeteorusTrachynotus;


namespace WBSF
{

	//*****************************************************************************
	//MeteorusTrachynotus daily development rates

	

	const CDevRateEquation::TDevRateEquation COBLPostDiapauseEquations::EQ_TYPE[NB_OBL_STAGES]
	{
		TDevRateEquation::Regniere_1982,   //Immatures in OBL post diapause
	};

	const double  COBLPostDiapauseEquations::EQ_P[NB_OBL_STAGES][5]
	{  //   p1       p2      p3      Tb      Tm
		{0.01704, 5.3577, 0.1672, 0.4290, 30.7558}, //Immatures in OBL post diapause
	};


	COBLPostDiapauseEquations::COBLPostDiapauseEquations(const CRandomGenerator& RG) :
		CEquationTableLookup(RG, NB_OBL_STAGES, 0, 40, 0.25)
	{
	}

	//Compute daily development rate for table lookup
	double COBLPostDiapauseEquations::ComputeRate(size_t s, double T)const
	{
		ASSERT(s < NB_OBL_STAGES);

		vector<double> p(begin(EQ_P[s]), end(EQ_P[s]));

		double Rt = max(0.0, CDevRateEquation::GetRate(EQ_TYPE[s], p, T));
		//if (e == EQ_IMMATURE_EGG)
		//	Rt /= 0.143; //14.3% of the time in egg stage
		//else if (e == EQ_IMMATURE_LARVAL)
		//	Rt /= 0.857; //85.7% of the time in larval stage

		_ASSERTE(!_isnan(Rt) && _finite(Rt));
		ASSERT(Rt >= 0);

		return Rt;
	}

	//*****************************************************************************
	// individual relative development rate 

	double COBLPostDiapauseEquations::Getδ(size_t s)const
	{
		ASSERT(s < NB_OBL_STAGES);

		static const double P[NB_OBL_STAGES][4] =
		{
			//  x      s
			{ 0.0000, 0.3968, 0.3, 2.5 },//Immatures in OBL post diapause
		};


		double 	r = m_randomGenerator.RandUnbiasedLogNormal(P[s][0], P[s][1]);
		while (r<P[s][2] || r>P[s][3])
			r = m_randomGenerator.RandUnbiasedLogNormal(P[s][0], P[s][1]);


		return r;

	}


	//*****************************************************************************
	//MeteorusTrachynotus daily development rates


	const CDevRateEquation::TDevRateEquation CMeteorusTrachynotusEquations::EQ_TYPE[NB_STAGES]
	{
		TDevRateEquation::Regniere_1982,   //Egg in OBL or SBW
		TDevRateEquation::Regniere_1982,   //Larva in OBL or SBW
		TDevRateEquation::Regniere_1982,   //Pupa
		TDevRateEquation::Regniere_1982    //Adult
	};



	const double  CMeteorusTrachynotusEquations::EQ_P[NB_STAGES][5]
	{  //   p1       p2      p3      Tb      Tm
		//{0.01500, 2.4875, 0.0318, 0.0814, 35.8600}, //Immatures in OBL or SBW
		{0.09424, 5.074519, 0.1745, -3.7953, 48.70}, //Egg in OBL or SBW
		{0.14496, 2.898649, 0.0225,  6.9910, 47.59}, //Larva in OBL or SBW
		{0.02967, 3.0176, 0.1441, 4.1731, 35.5319}, //Pupa (in cocoon) SBW and OBL
		{0.00825, 3.1113, 0.0073, 0.3062, 35.9200}, //Adult
	};


	CMeteorusTrachynotusEquations::CMeteorusTrachynotusEquations(const CRandomGenerator& RG) :
		CEquationTableLookup(RG, NB_STAGES, 0, 40, 0.25)
	{
	}

	//Compute daily development rate for table lookup
	double CMeteorusTrachynotusEquations::ComputeRate(size_t s, double T)const
	{
		ASSERT(s < NB_STAGES);

		vector<double> p(begin(EQ_P[s]), end(EQ_P[s]));
		
		double Rt = max(0.0, CDevRateEquation::GetRate(EQ_TYPE[s], p, T));
	

		_ASSERTE(!_isnan(Rt) && _finite(Rt));
		ASSERT(Rt >= 0);

		return Rt;
	}

	//double CMeteorusTrachynotusEquations::GetRate(size_t s, double t)const
	//{
	//	ASSERT(s < NB_EQUATIONS);
	//	double Rt = 0;

	//	if (s == EGG || s == LARVAL)
	//	{
	//		double Rt1 = CEquationTableLookup::GetRate(EQ_OBL_POST_DIAPAUSE, t);
	//		double Rt2 = CEquationTableLookup::GetRate(EQ_IMMATURE, t);

	//		if (Rt1 > 0 && Rt2 > 0)
	//		{
	//			static const double Fr[2] = { 0.134, 0.857 };
	//			double Rt = 1.0 / (Fr[s] / (Rt1) +Fr[s] / (Rt2));


	//			if (s == EGG)
	//				Rt /= 0.143; //14.3% of the time in egg stage
	//			else if (s == LARVAL)
	//				Rt /= 0.857; //85.7% of the time in larval stage
	//		}
	//	}
	//	else
	//	{
	//		Rt = CEquationTableLookup::GetRate(EQ_PUPA + (s- PUPA), t);
	//	}
	//	
	//	
	//	
	//	return Rt;
	//}

	//*****************************************************************************
	// individual relative development rate 

	double CMeteorusTrachynotusEquations::Getδ(size_t s)const
	{
		ASSERT(s < NB_STAGES);

		static const double P[NB_STAGES][4] =
		{
			//  x      s
			//{ 0.0000, 0.3321, 0.5, 2.0 },//Immatures in OBL or SBW
			{ 0.0000, 0.3160, 0.5, 2.0 },//Egg
			{ 0.0000, 0.3347, 0.5, 2.0 },//Larva
			{ 0.0000, 0.1325, 0.7, 1.5 },//Pupa
			{ 0.0000, 0.4317, 0.2, 3.0},//Adult longevity
		};


		double 	r = m_randomGenerator.RandUnbiasedLogNormal(P[s][0], P[s][1]);
		while (r<P[s][2] || r>P[s][3])
			r = m_randomGenerator.RandUnbiasedLogNormal(P[s][0], P[s][1]);


		return r;

	}


	//*****************************************************************************
	//fecundity

	double CMeteorusTrachynotusEquations::GetPmax()const
	{
		//eggs / female
		double fec = m_randomGenerator.RandNormal(194.2, 91.1);

		//limit between 1% and 99%
		while (fec < 20 || fec>400)
			fec = m_randomGenerator.RandNormal(194.2, 91.1);

		return fec;
	}



	

}