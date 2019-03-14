//*****************************************************************************
// File: SBDevelopment.h
//*****************************************************************************
#pragma once

#include "crtdbg.h"
#include "ModelBase/EquationTableLookup.h"

namespace WBSF
{

	namespace LNF
	{
		enum TStages{ EGG, L1, L2, L3, L4, PREPUPAE, PUPAE, ADULT, NB_STAGES, DEAD_ADULT = NB_STAGES };
		enum { Ϙ, к, NB_RDR_PARAMS };
	}


	//*****************************************************************************
	//CLaricobiusNigrinusEquations
	class CLaricobiusNigrinusEquations : public CEquationTableLookup
	{
	public:

		static const double P[LNF::NB_STAGES][LNF::NB_RDR_PARAMS];
		static const double F[4];
		
		double m_P[LNF::NB_STAGES][LNF::NB_RDR_PARAMS];
		double m_F[4];

		CLaricobiusNigrinusEquations(const CRandomGenerator& RG);
		
		virtual double ComputeRate(size_t stage, double t)const;

		//relative development
		double GetRelativeDevRate(size_t s)const;

	protected:
		

	};

}