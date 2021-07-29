//*****************************************************************************
// File: SBDevelopment.h
//*****************************************************************************
#pragma once

#include "crtdbg.h"
#include "ModelBase/EquationTableLookup.h"

namespace WBSF
{

	namespace LOF
	{
		enum TStages{ EGG, LARVAE1, LARVAE2, LARVAE3, LARVAE4, PREPUPAE, PUPAE, AESTIVAL_DIAPAUSE_ADULT, ACTIVE_ADULT, DEAD_ADULT, NB_STAGES = DEAD_ADULT};
		enum TRDR { σ, NB_RDR_PARAMS }; //Relative Development Rate
		//enum TCEC{ μ, ѕ, Τᴴ¹, Τᴴ², NB_CEC_PARAMS };//Cumulative Egg Creation (first oviposition)
		enum TCEC { μ, ѕ, ʎf, Τᴴ¹, Τᴴ², NB_CEC_PARAMS };//Cumulative Egg Creation (first oviposition)
		enum TADE{ ʎ0, ʎ1, ʎ2, ʎ3, ʎa, ʎb, NB_ADE_PARAMS };//Aestival Diapause End
		enum TEAS{ ʎ, к, Τᴴ, NB_EAS_PARAMS=3 };//Emerging Adult from Soil
	}


	//*****************************************************************************
	//CLaricobiusOsakensisEquations
	class CLaricobiusOsakensisEquations : public CEquationTableLookup
	{
	public:
		
		static const double CEC[LOF::NB_CEC_PARAMS];//Cumulative Egg Creation (first oviposition) default parameters
		static const double ADE[LOF::NB_ADE_PARAMS];//Aestival Diapause End default parameters
		static const double EAS[LOF::NB_EAS_PARAMS];//Emerging Adult from Soil default parameters
		
		
		double m_CEC[LOF::NB_CEC_PARAMS];//Cumulative Egg Creation (first oviposition) parameters
		double m_ADE[LOF::NB_ADE_PARAMS];//Aestival Diapause End parameters
		double m_EAS[LOF::NB_EAS_PARAMS];//Emerging Adult from Soil parameters
		
		CLaricobiusOsakensisEquations(const CRandomGenerator& RG);
		
		virtual double ComputeRate(size_t stage, double t)const;

		//relative development
		double GetRelativeDevRate(size_t s)const;
		double GetDailySurvivalRate(size_t s, double T)const;
		double GetFecondity()const;
		double GetFecondityRate(double age, double T)const;
		double GetCreationCDD()const;
		double GetAdultEmergingCDD()const;
	};

}