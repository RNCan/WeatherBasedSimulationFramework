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


	const CDevRateEquation::TDevRateEquation CMeteorusTrachynotusEquations::EQ_TYPE[NB_EQUATIONS]
	{
		TDevRateEquation::Regniere_1982,   //Immature egg,larva in Generation 0 (overwintered OBL
		TDevRateEquation::Regniere_1982,   //Immature egg,larva in other Generations (in SBW or OBL)
		TDevRateEquation::Regniere_1982,   //Pupa
		TDevRateEquation::Regniere_1982    //Adult
	};



	const double  CMeteorusTrachynotusEquations::EQ_P[NB_EQUATIONS][5]
	{  //   p1       p2      p3      Tb      Tm
		//{0.01500, 2.4875, 0.0318, 0.0814, 35.8600}, //Immature (egg,larva) in OBL or SBW
		//{0.02967, 3.0176, 0.1441, 4.1731, 35.5319}, //Pupa (in cocoon) SBW and OBL
		//{0.00825, 3.1113, 0.0073, 0.3062, 35.9200}, //Adult
		{0.01659, 2.9072, 0.0117, 5.0000, 35.0000}, //Immature egg,larva in Generation 0 (overwintered OBL
		{0.02217, 1.9227, 0.0117, 5.0000, 35.0000}, //Immature egg,larva in other Generations (in SBW or OBL)
		{0.03228, 2.7270, 0.1285, 5.0000, 35.0000}, //Pupa (in cocoon) SBW and OBL
		{0.01243, 3.0574, 0.001, 5.0000, 40.0000}, //Adult

	};


	CMeteorusTrachynotusEquations::CMeteorusTrachynotusEquations(const CRandomGenerator& RG) :
		CEquationTableLookup(RG, NB_EQUATIONS, 0, 40, 0.25)
	{
	}

	//Compute daily development rate for table lookup
	double CMeteorusTrachynotusEquations::ComputeRate(size_t e, double T)const
	{
		ASSERT(e < NB_EQUATIONS);

		vector<double> p(begin(EQ_P[e]), end(EQ_P[e]));
		
		double Rt = max(0.0, CDevRateEquation::GetRate(EQ_TYPE[e], p, T));

		_ASSERTE(!_isnan(Rt) && _finite(Rt));
		ASSERT(Rt >= 0);

		return Rt;
	}


	//*****************************************************************************
	// individual relative development rate 

	double CMeteorusTrachynotusEquations::Getδ(size_t e)const
	{
		ASSERT(e < NB_EQUATIONS);

		static const double P[NB_EQUATIONS][4] =
		{
			//  x      s
			//{ 0.0000, 0.3321, 0.5, 2.0 },//Immature (egg,larva) in OBL or SBW
			//{ 0.0000, 0.1325, 0.7, 1.5 },//Pupa
			//{ 0.0000, 0.4317, 0.2, 3.0},//Adult longevity
			{ 0.0000, 0.3608, 0.4, 2.5 },//Immature (egg,larva) in initial OBL
			{ 0.0000, 0.2829, 0.5, 2.0 },//Immature (egg,larva) in subsequent generations OBL or SBW
			{ 0.0000, 0.1328, 0.7, 1.5 },//Pupa
			{ 0.0000, 0.4317, 0.2, 3.0},//Adult longevity

		};


		double 	r = m_randomGenerator.RandUnbiasedLogNormal(P[e][0], P[e][1]);
		while (r<P[e][2] || r>P[e][3])
			r = m_randomGenerator.RandUnbiasedLogNormal(P[e][0], P[e][1]);


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