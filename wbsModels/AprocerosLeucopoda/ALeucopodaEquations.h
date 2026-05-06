//*****************************************************************************
// File: SBDevelopment.h
//*****************************************************************************
#pragma once

#include <array>
#include "crtdbg.h"
#include "ModelBase/EquationTableLookup.h"

namespace WBSF
{

	namespace TZZ//Aproceros leucopoda
	{
		enum TStages{ EGG, LARVA, PREPUPA, PUPA, ADULT, DEAD_ADULT, NB_STAGES = DEAD_ADULT};
		enum TRDR { σ, NB_RDR_PARAMS }; //relative development parameter
		enum TEWD{ ʎ0, ʎ1, ʎ2, ʎ3, ʎa, ʎb, NB_EWD_PARAMS };//entering winter diapause 
		enum TEAS{ μ, ѕ, Τᴴ¹, Τᴴ², Tᴼ, NB_EAS_PARAMS };//Emerging Adult from Soil (spring)
	}


	//*****************************************************************************
	//CAprocerosLeucopodaEquations
	
	class CAprocerosLeucopodaEquations : public CEquationTableLookup
	{
	public:

		static const double EWD[TZZ::NB_EWD_PARAMS];//entering winter diapause default parameters
		static const double EAS[TZZ::NB_EAS_PARAMS];//Emerging Adult from Soil (spring) default  parameters
		
		std::array<double, TZZ::NB_EWD_PARAMS> m_EWD;//entering winter diapause  parameters
		std::array<double, TZZ::NB_EAS_PARAMS> m_EAS;//Emerging Adult from Soil (spring) parameters

		
		CAprocerosLeucopodaEquations(const CRandomGenerator& RG);

		//relative development
		double GetRelativeDevRate(size_t s)const;
		double GetFecondity()const;
		double GetBrood(double Fi, double T, double t, double delta_t)const;
		double GetAdultEmergingCDD()const;

	protected:

		virtual double ComputeDailyDevlopmentRate(size_t e, double T)const override;
		virtual double ComputeDailySurvivalRate(size_t e, double T)const override;
	};

}