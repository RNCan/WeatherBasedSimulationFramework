//*****************************************************************************
// File: SBDevelopment.h
//*****************************************************************************
#pragma once

#include "crtdbg.h"
#include "ModelBase/EquationTableLookup.h"
#include "Basic/DegreeDays.h"



namespace WBSF
{

	namespace LPM// Leucotaraxis piniperda (Malloch) 
	{
		enum TStages{ EGG, LARVAE, PUPAE, ADULT, DEAD_ADULT, NB_STAGES= DEAD_ADULT};

		
		enum TRDR { σ, NB_RDR_PARAMS }; //Relative Development Rate
		enum TEmergence{ μ, ѕ, delta, Τᴴ¹, Τᴴ², NB_EMERGENCE_PARAMS };//Emergence of Adult parameters
		enum TPUPA { NB_PUPA_DEV=6, PUPA_S= NB_PUPA_DEV, NB_PUPA_PARAMS };//
		enum TOVIP { NB_C_PARAMS = 3 };//
	}


	//*****************************************************************************
	//CLeucotaraxisPiniperdaEquations
	class CLeucotaraxisPiniperdaEquations : public CEquationTableLookup
	{
	public:

		static const std::array<double, LPM::NB_EMERGENCE_PARAMS> ADULT_EMERG;
		static const std::array<double, LPM::NB_PUPA_PARAMS> PUPA_PARAM;//Pupa (without diapause) param
		static const std::array<double, LPM::NB_C_PARAMS> C_PARAM;//Correction factor

		std::array<double, LPM::NB_EMERGENCE_PARAMS> m_adult_emerg;//emergence of adult parameters
		std::array<double, LPM::NB_PUPA_PARAMS> m_pupa_param;//Pupa parameters
		std::array<double, LPM::NB_C_PARAMS> m_C_param;//Cumulative Egg Creation parameters
		
		
		CLeucotaraxisPiniperdaEquations(const CRandomGenerator& RG);
		
		virtual double ComputeRate(size_t stage, double t)const;

		//relative development
		double GetRelativeDevRate(size_t s)const;
		double GetDailySurvivalRate(size_t s, double T)const;
		double GetPreOvipPeriod()const;
		double GetFecundity(double L)const;

		void GetAdultEmergenceCDD(const CWeatherYears& weather, CModelStatVector& CDD)const;
		double GetAdultEmergingCDD()const;
		double GetPupaRate(double T)const;
		double GetPupaRDR()const;
	};

}