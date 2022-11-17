//*****************************************************************************
// File: SBDevelopment.h
//*****************************************************************************
#pragma once

#include <crtdbg.h>
#include <array>

#include "ModelBase/EquationTableLookup.h"
#include "ModelBase/ModelDistribution.h"


namespace WBSF
{

	
	

	

	//Lycorma delicatula (White)
	namespace LDW
	{
		enum TStages { EGG, N1, N2, N3, N4, ADULT, DEAD_ADULT, NB_STAGES };
		enum TPsy { NB_PSY = NB_STAGES };
	}
	

	//*****************************************************************************
	//CSpottedLanternflyEquations
	class CSpottedLanternflyEquations : public CEquationTableLookup
	{
	public:
		
		static const std::array<double, NB_CDD_PARAMS> EOD;
		
		//double m_emergence[LDW::NB_PARAMS];//Cumulative Egg Creation (first oviposition) parameters
		//double m_adult[LDW::NB_PARAMS];//Aestival Diapause End parameters
		std::array<double, NB_CDD_PARAMS> m_EOD;

		CSpottedLanternflyEquations(const CRandomGenerator& RG);
		virtual double ComputeRate(size_t stage, double t)const;

		std::array<double, LDW::NB_PSY > m_psy;

		//relative development
		double GetRelativeDevRate(size_t s)const;
		double GetRelativeDevRate(double sigma)const;
		//double GetFecondity()const;

		double GetDailySurvivalRate(size_t s, double T)const;
		double GetEndOfDiapauseCDD()const;
	
	};

}