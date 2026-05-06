//*****************************************************************************
// File: SBDevelopment.h
//*****************************************************************************
#pragma once

#include <array>
#include <cassert>
#include "ModelBase/EquationTableLookup.h"

namespace WBSF
{

	namespace LNF
	{
		enum TStages{ EGG, LARVAE, PREPUPAE, PUPAE, AESTIVAL_DIAPAUSE_ADULT, ACTIVE_ADULT, DEAD_ADULT, NB_STAGES = DEAD_ADULT};
		enum TRDR { Ϙ, к, NB_RDR_PARAMS }; //relative development parameter
		enum TOvip{ μ, ѕ, Τᴴ, Τᴴ¹= Τᴴ, Τᴴ², NB_OVP_PARAMS };//Creation (original oviposition) parameters
		enum TAAD{ ʎ0, ʎ1, ʎ2, ʎ3, ʎa, ʎb, NB_ADE_PARAMS };//AestivalDiapauseEnd (Emerging adult begin)
		enum TEAS{ /*μ, ѕ, Τᴴ*/ NB_EAS_PARAMS=3 };//EmergingAdult from soil
	}


	//*****************************************************************************
	//CLaricobiusNigrinusEquations
	class CLaricobiusNigrinusEquations : public CEquationTableLookup
	{
	public:

		//static const std::array< std::array<double, LNF::NB_RDR_PARAMS>, LNF::NB_STAGES> RDR; //relative development parameter
		static const std::array<double, LNF::NB_OVP_PARAMS> OVP;//oviposition parameters
		static const std::array<double, LNF::NB_ADE_PARAMS> ADE;//AdultAestivalDiapauseEnd parameters
		static const std::array<double, LNF::NB_EAS_PARAMS> EAS;//EmergingAdult from soil
		
		//std::array< std::array<double, LNF::NB_RDR_PARAMS>, LNF::NB_STAGES> m_RDR; //relative development parameter
		std::array<double, LNF::NB_OVP_PARAMS> m_OVP;//Creation (initial oviposition) parameters
		std::array<double, LNF::NB_ADE_PARAMS> m_ADE;//AestivalDiapauseEnd parameters
		std::array<double, LNF::NB_EAS_PARAMS> m_EAS;//Emerging Adult from Soil parameters
		
		CLaricobiusNigrinusEquations(const CRandomGenerator& RG);
		
		//relative development
		double GetRelativeDevlopmentRate(size_t stage)const;
		double GetFecundity()const;
		//double GetOvipositionRatio(double T)const;
		double GetCreationCDD()const;
		double GetAdultEmergingCDD()const;
		double GetColdTolerence()const;

	protected:

		virtual double ComputeDailyDevlopmentRate(size_t e, double T)const override;
		virtual double ComputeDailySurvivalRate(size_t e, double T)const override;
		virtual double ComputeOvipositionRatio(double T)const override;
	};

}