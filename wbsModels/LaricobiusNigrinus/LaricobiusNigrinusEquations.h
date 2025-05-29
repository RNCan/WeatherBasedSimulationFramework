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

		static const std::array< std::array<double, LNF::NB_RDR_PARAMS>, LNF::NB_STAGES> RDR; //relative development parameter
		static const std::array<double, LNF::NB_OVP_PARAMS> OVP;//oviposition parameters
		static const std::array<double, LNF::NB_ADE_PARAMS> ADE;//AdultAestivalDiapauseEnd parameters
		static const std::array<double, LNF::NB_EAS_PARAMS> EAS;//AdultAestivalDiapauseEnd parameters
		
		std::array< std::array<double, LNF::NB_RDR_PARAMS>, LNF::NB_STAGES> m_RDR; //relative development parameter
		std::array<double, LNF::NB_OVP_PARAMS> m_OVP;//Creation (initial oviposition) parameters
		std::array<double, LNF::NB_ADE_PARAMS> m_ADE;//AestivalDiapauseEnd parameters
		std::array<double, LNF::NB_EAS_PARAMS> m_EAS;//Emerging Adult from Soil parameters
		
		CLaricobiusNigrinusEquations(const CRandomGenerator& RG);
		
		virtual double ComputeRate(size_t stage, double t)const;

		//relative development
		double GetRelativeDevRate(size_t s)const;
		double GetDailySurvivalRate(size_t s, double T)const;
		double GetFecondity()const;
		double GetFecondityRate(double age, double T)const;
		double GetAdultLongevity(size_t sex)const;
		//		double GetFecondity(double l)const;
		double GetCreationCDD()const;
		double GetAdultEmergingCDD()const;
		//double GetAestivalDiapauseEndCDD()const;
		double GetTimeInSoil(double T, double day_length);
		double GetAdultAestivalDiapauseRate(double T, double day_length, double creation_day, double pupation_time);
		double GetAdultAbundance(double T, size_t j_day_since_jan);

	};

}