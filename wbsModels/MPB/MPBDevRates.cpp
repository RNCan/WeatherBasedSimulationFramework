//*********************************************************************
// File: DevRates.cpp
//
// Class: CDevRates
//          
//
//************** MODIFICATIONS  LOG ********************
// ?            ?                   Creation
// 01/09/2003   Rémi Saint-Amant    Replace MatLab code by NEWMAT code
// 27/10/2003   Rémi Saint-Amant    Reengineering
// 24/11/2005	Rémi Saint-Amant	Change matrix calculation to one day calculation
// 02/10/2007   Jacques Régnière    Clean-up, check math
// 31/10/2007	Remi Saint-Amant	New stage : EMERGED_ADULT
//									New TableLookup from 2 to 38 °C by step of 0.5°C
//									New GalleryLn method
// 17/09/2008   Jacques Régnière    New development rate parameters for teneral adults
// 13/10/2008	Jacques Régniere	New development rate params and equations for all stages
//									No longer using GallerLn
//									New equations for ovipositing females in CMountainPineBeetle::Develop()
// 27/10/2008	Jacques Regniere	New cold tolerance for pupae (same as eggs for now)
// 10/07/2012	Rémi Saint-Amant	New equation from new paper
// 25/12/2012   Rémi Saint-Amant	Remove static properties
//*********************************************************************
#include "MPBDevRates.h" 
#include "Basic/UtilMath.h"

using namespace std;

namespace WBSF
{


	//*****************************************************
	//CDevRates class 
	//*****************************************************

	const double CMPBDevelopmentTable::POTENTIAL_FECUNDITY = 81.8;

	//development rate parameters (8 stages, 8 parameters)
	const double CMPBDevelopmentTable::DEFAULT_P[NB_STAGES][NB_PARAMETERS] =
	{//   Tb DELTAb  Tm DELTAm  OMEGA    PSI     SIGMA    F0	
		{ 7.2, 1.70, 39.9, 8.7, 0.109, 0.04690, 0.0702, 1.0 }, //Egg
		{ 3.6, 0.10, 29.3, 3.8, 0.240, 0.01082, 0.2911, 1.0 }, //L1
		{ 7.0, 0.10, 28.9, 3.0, 0.371, 0.00642, 0.3800, 1.0 }, //L2
		{ 6.8, 0.10, 28.7, 2.5, 0.440, 0.00389, 0.3870, 1.0 }, //L3
		{ 16.2, 0.04, 28.0, 4.6, 0.259, 0.05030, 0.3930, 1.0 }, //L4
		{ 5.6, 0.11, 28.5, 2.9, 0.153, 0.02050, 0.3000, 1.0 }, //Pupa
		{ 4.2, 0.10, 35.0, 7.1, 0.146, 0.01170, 0.5280, 1.0 }, //Teneral
		{ 4.6, 0.10, 27.8, 3.1, 0.368, 0.00520, 0.2460, 81.8 }  //Oviposition
	};

	CMPBDevelopmentTable::CMPBDevelopmentTable(const CRandomGenerator& RG) :CEquationTableLookup(RG, NB_STAGES, 3, 40, 0.25)
	{
		for (int i = 0; i < NB_STAGES; i++)
		{
			for (int j = 0; j < NB_PARAMETERS; j++)
			{
				m_p[i][j] = DEFAULT_P[i][j];
			}
		}

		Init();
	}

	
	double CMPBDevelopmentTable::ComputeRate(size_t e, double T)const
	{
		ASSERT(e >= 0 && e < NB_STAGES);

		const double* p = m_p[e];

		double r = 0;

		if (T >= p[Tb] && T <= p[Tm])
		{
			//Equation [1]
			double tmp1 = exp(p[OMEGA] * (T - p[Tb]));
			double tmp2 = ((p[Tm] - T) / (p[Tm] - p[Tb]))*exp(-p[OMEGA] * (T - p[Tb]) / p[DELTAb]);
			double tmp3 = ((T - p[Tb]) / (p[Tm] - p[Tb]))*exp(p[OMEGA] * (p[Tm] - p[Tb]) - (p[Tm] - T) / p[DELTAm]);
			double tmp4 = tmp1 - tmp2 - tmp3;
			r = max(0.0, p[PSI] * tmp4);
		}

		ASSERT(r >= 0 && r <= 1);

		return r;
	}

	double CMPBDevelopmentTable::GetRelativeMean(int e)const
	{
		ASSERT(e >= 0 && e < NB_STAGES);
		return m_p[e][F0];

	}

	double CMPBDevelopmentTable::GetRelativeSigma(int e)const
	{
		ASSERT(e >= 0 && e < NB_STAGES);
		return m_p[e][SIGMA];

	}

}