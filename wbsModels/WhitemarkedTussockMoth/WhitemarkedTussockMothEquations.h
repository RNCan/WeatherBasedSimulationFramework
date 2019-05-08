//*****************************************************************************
// File: SBDevelopment.h
//*****************************************************************************
#pragma once

#include "crtdbg.h"
#include "ModelBase/EquationTableLookup.h"

namespace WBSF
{

	namespace WTM
	{
		enum TStages{ EGG, LARVAE, PUPAE, ADULT, NB_STAGES, DEAD_ADULT=NB_STAGES };
		enum { Ϙ, к, NB_RDR_PARAMS };
		enum { startᴴ, μ, ѕ, Τᴴ, NB_HATCH_PARAMS };
		enum { NB_DEV_RATE_PARAMS = 6 };
	}


	//*****************************************************************************
	//CWhitemarkedTussockMothEquations
	class CWhitemarkedTussockMothEquations : public CEquationTableLookup
	{
	public:

		static const double P[WTM::NB_STAGES][WTM::NB_DEV_RATE_PARAMS];
		static const double D[WTM::NB_STAGES][WTM::NB_RDR_PARAMS]; //development parameter
		static const double H[WTM::NB_HATCH_PARAMS]; //hatch parameter
		
		double m_P[WTM::NB_STAGES][WTM::NB_DEV_RATE_PARAMS];
		double m_D[WTM::NB_STAGES][WTM::NB_RDR_PARAMS];
		double m_H[WTM::NB_HATCH_PARAMS];
		

		CWhitemarkedTussockMothEquations(const CRandomGenerator& RG);
		
		virtual double ComputeRate(size_t stage, double t)const;
		

		//relative development
		double GetRelativeDevRate(size_t s)const;
		double GetHatchCDD()const;
		double GetFecondity()const;
	

	protected:
		
	

	};

}