//*****************************************************************************
// File: SBDevelopment.h
//*****************************************************************************
#pragma once

#include "crtdbg.h"
#include "ModelBase/EquationTableLookup.h"

namespace WBSF
{

	namespace FTC
	{
		enum TStages{ EGG, L1, L2, L3, L4, L5, PUPAE, ADULT, NB_STAGES, DEAD_ADULT = NB_STAGES };
		enum TGrayTree { LARGETOOTH_ASPEN, TREMBLING_ASPEN, NB_GRAY_TREES };
		enum TTree { BIRCH = NB_GRAY_TREES, RED_OAK, SUGAR_MAPLE, NB_TREES};
	}


	//*****************************************************************************
	//CForestTentCaterpillarEquations
	class CForestTentCaterpillarEquations : public CEquationTableLookup
	{
	public:

		CForestTentCaterpillarEquations(const CRandomGenerator& RG);
		
		virtual double ComputeRate(size_t stage, double t)const;

		//relative development
		double GetRelativeDevRate(size_t s)const;

		double GetFecondity()const;
	protected:

		static double GetGrayEggDevRate(size_t t, double T);
		double GetGrayEggRelDevRate(size_t t)const;

		static double GetDevRate(size_t s, double T);
		double GetRelDevRate(size_t s)const;

	};

}