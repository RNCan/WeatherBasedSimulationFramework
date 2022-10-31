//*****************************************************************************
// File: SBDevelopment.h
//*****************************************************************************
#pragma once

#include "crtdbg.h"
#include "ModelBase/EquationTableLookup.h"

namespace WBSF
{

	namespace EAB
	{
		
		enum TStages { PUPAE, ADULT, DEAD_ADULT, NB_STAGES };
		//enum TParameters {NB_PARAMS=6 };
	}


	//*****************************************************************************
	//CAplanipennisEquations
	class CAplanipennisEquations : public CEquationTableLookup
	{
	public:
		
		
		
		//double m_emergence[EAB::NB_PARAMS];//Cumulative Egg Creation (first oviposition) parameters
		//double m_adult[EAB::NB_PARAMS];//Aestival Diapause End parameters
		
		
		CAplanipennisEquations(const CRandomGenerator& RG);
		
		virtual double ComputeRate(size_t stage, double t)const;


		//relative development
		double GetRelativeDevRate(size_t s)const;
		double GetRelativeDevRate(double sigma)const;
		//double GetFecondity()const;

	};

}