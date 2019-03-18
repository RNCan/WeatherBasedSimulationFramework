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
		enum TStages{ EGG, LARVAE, PREPUPAE, PUPAE, NB_STAGES, ADULT = NB_STAGES };
		enum { Ϙ, к, NB_RDR_PARAMS };
		enum { μ, ѕ, Τᴴ, NB_OVIP_PARAMS };
	}


	//*****************************************************************************
	//CLaricobiusNigrinusEquations
	class CLaricobiusNigrinusEquations : public CEquationTableLookup
	{
	public:

		static const double D[LNF::NB_STAGES][LNF::NB_RDR_PARAMS]; //development parameter
		static const double O[LNF::NB_OVIP_PARAMS];//oviposition parameters
		
		double m_D[LNF::NB_STAGES][LNF::NB_RDR_PARAMS];
		double m_O[LNF::NB_OVIP_PARAMS];
		
		CLaricobiusNigrinusEquations(const CRandomGenerator& RG);
		
		virtual double ComputeRate(size_t stage, double t)const;

		//relative development
		double GetRelativeDevRate(size_t s)const;
		double GetAdultLongevity()const;
		double GetFecondity(double l)const;
		double GetOvipositionDD()const;

	protected:
		
		static double Eq7(size_t s, double T);

	};

}