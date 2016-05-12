//*********************************************************************
// File: DevRates.h
//
// Class: CDevRates
//          
//
// Descrition: the CMPBDevelopmentTable can compute daily MPB devlopement rate
//             CMPBDevelopmentTableTable is an optimisation table lookup
//*********************************************************************
#pragma once

#include "modelBase/EquationTableLookup.h"

namespace WBSF
{


	enum TStages{ EGG, L1, L2, L3, L4, PUPA, TENERAL_ADULT, OVIPOSITING_ADULT, NB_STAGES, DEAD_ADULT = NB_STAGES };

	class CMPBDevelopmentTable : public CEquationTableLookup
	{
	public:

		enum TParameters{ Tb, DELTAb, Tm, DELTAm, OMEGA, PSI, SIGMA, F0, NB_PARAMETERS };

		static const double POTENTIAL_FECUNDITY;
		static const double DEFAULT_P[NB_STAGES][NB_PARAMETERS];


		CMPBDevelopmentTable(const CRandomGenerator& RG);
		virtual double ComputeRate(size_t, double) const;
		//double ComputeRate(int e, double T)const;
		double GetRelativeMean(int e)const;
		double GetRelativeSigma(int e)const;

	private:



		double m_p[NB_STAGES][NB_PARAMETERS];


	};

}